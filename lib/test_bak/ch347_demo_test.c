/*
 * ch347 application demo
 *
 * Copyright (C) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
 * Web:      http://wch.cn
 * Author:   WCH <tech@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Cross-compile with cross-gcc -I /path/to/cross-kernel/include
 *
 * V1.0 - initial version
 * V1.1 - add operation for HID mode
 * V1.2 - add serial port operation in HID and TTY mode
 * V1.3 - update with new library
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <endian.h>
#include <linux/hidraw.h>
#include <signal.h>

#include "ch347_lib.h"

#define CMD_FLASH_CHIP_ERASE 0x60
#define CMD_FLASH_SECTOR_ERASE 0x20
#define CMD_FLASH_BYTE_PROG 0x02
#define CMD_FLASH_READ 0x03
#define CMD_FLASH_RDSR 0x05
#define CMD_FLASH_WREN 0x06
#define CMD_FLASH_JEDEC_ID 0x9F

#define SPI_FLASH_PerWritePageSize 256

typedef enum _CH347FUNCTYPE {
	FUNC_UART = 0,
	FUNC_SPI_I2C_GPIO,
	FUNC_JTAG_GPIO,
	FUNC_SPI_I2C_JTAG_GPIO,
} CH347FUNCTYPE;

struct ch34x {
	int fd;
	char version[100];
	CHIP_TYPE chiptype;
	uint32_t dev_id;
	CH347FUNCTYPE functype;
};

static struct ch34x ch347device;

int clk_table1[] = { 60e6,   48e6,   36e6,   30e6,  28e6,  24e6,   18e6,
		     15e6,   14e6,   12e6,   9e6,   75e5,  7e6,	   6e6,
		     45e5,   375e4,  35e5,   3e6,   225e4, 1875e3, 175e4,
		     15e5,   1125e3, 9375e2, 875e3, 750e3, 5625e2, 46875e1,
		     4375e2, 375e3,  21875e1 };

int clk_index = 0;

int hid_baudrate = 9600;

bool W25X_Flash_Write_Page(uint8_t *pBuf, uint32_t address, uint32_t len);
bool Flash_Chip_Erase();

bool CH347_SPI_Init()
{
	bool ret;
	mSpiCfgS SpiCfg = { 0 };

	/* set spi interface in [mode3] & [15MHz] & [MSB] & output [0xFF] by default */
	SpiCfg.iMode = 0x03;
	SpiCfg.iByteOrder = 1;
	SpiCfg.iSpiOutDefaultData = 0xFF;
	SpiCfg.iChipSelect = 0x80;

	printf("spi frequency: %d\n", clk_table1[clk_index]);
	// CH347SPI_SetFrequency(ch347device.fd, clk_table1[clk_index++]);
	CH347SPI_SetFrequency(ch347device.fd, 30e6);

	// CH347SPI_SetAutoCS(ch347device.fd, true);

	/* set spi to 16bits transfer */
	CH347SPI_SetDataBits(ch347device.fd, 0);

	/* init spi interface */
	ret = CH347SPI_Init(ch347device.fd, &SpiCfg);
	if (!ret) {
		printf("Failed to init SPI interface.\n");
		return false;
	} else {
		printf("SPI Init ok.\n");
	}

	return true;
}

// bool CH347_SPI_Slave_Init()
// {
// 	bool ret;
// 	mSpiCfgS SpiCfg = { 0 };

// 	SpiCfg.iByteOrder = 1;
// 	SpiCfg.iSpiOutDefaultData = 0xFF;
// 	SpiCfg.iChipSelect = 0x80;
// 	SpiCfg.iMode = 0x83;

// 	/* set spi to 16bits transfer */
// 	// SpiCfg.iDataBits = 1;

// 	/* init spi interface */
// 	ret = CH347SPI_Init(ch347device.fd, &SpiCfg);
// 	if (!ret) {
// 		printf("Failed to init SPI interface.\n");
// 		return false;
// 	}

// 	return true;
// }

//  * ->bit0~2: set I2C SCL rate
//  * 			   --> 000 : low rate 20KHz
//  * 			   --> 001 : standard rate 100KHz
//  * 			   --> 010 : fast rate 400KHz
//  * 			   --> 011 : high rate 750KHz
//  * 			   --> 100 : rate 50KHz
//  * 			   --> 101 : standard rate 200KHz
//  * 			   --> 110 : fast rate 1MHz
//  * 			   --> 111 : high rate 2.4MHz

int imode = 0;

bool CH347_I2C_Init()
{
	bool ret;
	int iMode;

	/* set i2c interface in 750KHZ */
	iMode = imode++;
	if (imode > 7)
		imode = 0;
	// iMode = 0x02;

	/* init i2c interface */
	ret = CH347I2C_Set(ch347device.fd, iMode);
	if (!ret) {
		printf("Failed to init I2C interface.\n");
		return false;
	}

	// ret = CH347I2C_SetAckClk_DelayuS(ch347device.fd, 200);
	// if (!ret) {
	// 	printf("Failed to set I2C delay time.\n");
	// 	return false;
	// }

	/* set i2c clock stretch */
	ret = CH347I2C_SetStretch(ch347device.fd, false);
	if (!ret) {
		printf("Failed to set I2C Clock Stretch.\n");
		return false;
	}

	/* set i2c to push-pull mode */
	ret = CH347I2C_SetDriveMode(ch347device.fd, 0);
	if (!ret) {
		printf("Failed to set I2C Drive Mode.\n");
		return false;
	}

	return true;
}

bool Flash_ID_Read()
{
	int iChipSelect;
	int iLength;
	uint8_t ioBuffer[4] = { 0 };
	uint32_t Flash_ID;

	iChipSelect = 0x80;
	iLength = 4;
	ioBuffer[0] = CMD_FLASH_JEDEC_ID;
	memset(ioBuffer + 1, 0xFF, 3);

	// test for 16bits mode
	// ioBuffer[1] = CMD_FLASH_JEDEC_ID;
	// ioBuffer[0] = 0xFF;

	if (CH347SPI_WriteRead(ch347device.fd, false, iChipSelect, iLength,
			       ioBuffer) == false)
		return false;
	else {
		ioBuffer[0] = 0x00;
		memcpy(&Flash_ID, ioBuffer, 4);
	}
	Flash_ID = htole32(Flash_ID);
	printf("Read flash ID: 0x%x.\n", Flash_ID);

	if (Flash_ID == 0x000000 || Flash_ID == 0xffffff00) {
		printf("Read flash ID error.\n");
		return false;
	}

	return true;
}

unsigned int Flash_Block_Read(unsigned int address, uint8_t *pbuf,
			      unsigned int len)
{
	int iChipSelect;
	int iLength;

	iChipSelect = 0x80;
	iLength = 0x04;

	*pbuf = CMD_FLASH_READ;
	*(pbuf + 1) = (uint8_t)(address >> 16);
	*(pbuf + 2) = (uint8_t)(address >> 8);
	*(pbuf + 3) = (uint8_t)(address);

	if (!CH347SPI_Read(ch347device.fd, false, iChipSelect, iLength,
			   &len, pbuf)) {
		printf("Flash_Block_Read read %d bytes failed.\n", len);
		return 0;
	}

	return len;
}

unsigned int Flash_Block_Write(unsigned int address, uint8_t *pbuf,
			       unsigned int len)
{
	int ret;
	int i = 0;
	uint32_t DataLen = len, FlashAddr, NumOfPage, NumOfSingle;

	/* write flash from addr 0 */
	FlashAddr = address;

	NumOfPage = DataLen / SPI_FLASH_PerWritePageSize;
	NumOfSingle = DataLen % SPI_FLASH_PerWritePageSize;

	if (address % SPI_FLASH_PerWritePageSize) {
		printf("Flash address is not page aligned!\n");
		return false;
	}

	// printf("NumOfPage: %d, NumOfSingle: %d\n", NumOfPage, NumOfSingle);

	/* caculate flash write time */
	while (NumOfPage--) {
		ret = W25X_Flash_Write_Page(pbuf, FlashAddr,
					    SPI_FLASH_PerWritePageSize);
		if (ret == false)
			goto exit;
		pbuf += SPI_FLASH_PerWritePageSize;
		FlashAddr += SPI_FLASH_PerWritePageSize;
	}
	if (NumOfSingle) {
		ret = W25X_Flash_Write_Page(pbuf, FlashAddr, NumOfSingle);
		if (ret == false)
			goto exit;
	}

	return FlashAddr;

exit:
	printf("\tFlash Write: Addr [0x%x] write %d bytes failed.\n",
	       address, DataLen);
	return FlashAddr;
}

/**
 * get_file_size - get file size
 * @path: file name
 * The function return file size
 */
unsigned int get_file_size(const char *path)
{
	unsigned int filesize = -1;
	struct stat statbuff;

	if (stat(path, &statbuff) < 0) {
		return filesize;
	} else {
		filesize = statbuff.st_size;
	}
	return filesize;
}

bool Flash_File_Read()
{
	uint32_t FlashAddr, i;
	uint8_t *ioBuffer, *buf_cmp;
	char FmtStr1[8 * 1024 * 3 + 16] = "";
	double UseT;
	static struct timeval t1, t2;
	long long delta_sec, delta_usec;

	int nread;
	int ret, rv;

	FlashAddr = 0x00;

	FILE *fp = NULL;
	unsigned long long filesize = 0;
	static int count = 0;

	fp = fopen("ch347-flash", "r");
	if (fp == NULL) {
		printf("open file failed.\n");
		return false;
	}

	filesize = get_file_size("ch347-flash");
	if (filesize < 0) {
		printf("get_file_size error\n");
		return false;
	}

	ioBuffer = (uint8_t *)malloc(filesize);
	buf_cmp = (uint8_t *)malloc(filesize);

	ret = fread(buf_cmp, 1, filesize, fp);
	if (ret != filesize) {
		printf("file read failed.\n");
		goto exit;
	}

	/* caculate flash read time */
	gettimeofday(&t1, NULL);

	ret = Flash_Block_Read(FlashAddr, ioBuffer, filesize);
	if (ret != filesize) {
		printf("\tFlash Read: Addr[0x%x] read %lld bytes failed.\n",
		       FlashAddr, filesize);
		goto exit;
	}

	gettimeofday(&t2, NULL);

	delta_sec = t2.tv_sec - t1.tv_sec;
	delta_usec = t2.tv_usec - t1.tv_usec;

	if (t2.tv_usec < t1.tv_usec) {
		delta_sec--;
		delta_usec += 1000000;
	}
	UseT = (float)delta_sec + (float)delta_usec / 1000000;

	printf("\tFlash File Read in %.3f seconds.\n", UseT);

	rv = memcmp(ioBuffer, buf_cmp, filesize);
	if (rv) {
		printf("flash content compare error.\n");
		printf("read data:\n");
		for (i = 0; i < filesize; i++)
			printf("%02x ", ioBuffer[i]);
		printf("\n\n");
		printf("raw file data:\n");
		for (i = 0; i < filesize; i++)
			printf("%02x ", buf_cmp[i]);
		printf("\n\n");
		goto exit;
	}

	printf("******************** flash Test Passed--> count: %d *************\n",
	       count++);

	free(ioBuffer);
	free(buf_cmp);
	fclose(fp);
	return true;

exit:

	free(ioBuffer);
	free(buf_cmp);
	fclose(fp);
	return false;
}

bool Flash_File_Write()
{
	double UseT;
	uint32_t FlashAddr, i;
	uint8_t *ioBuffer;
	char FmtStr1[8 * 1024 * 3 + 16] = "";

	static struct timeval t1, t2;
	long long delta_sec, delta_usec;

	int nread;
	int ret, rv;

	FlashAddr = 0x00;

	FILE *fp = NULL;
	unsigned long long filesize = 0;
	static int count = 0;

	fp = fopen("ch347-flash", "r");
	if (fp == NULL) {
		printf("open file failed.\n");
		return false;
	}

	filesize = get_file_size("ch347-flash");
	if (filesize < 0) {
		printf("get_file_size error\n");
		return false;
	}

	/* caculate flash write time */
	gettimeofday(&t1, NULL);

	ret = Flash_Chip_Erase();
	if (!ret) {
		printf("flash chip erase failed.\n");
		goto exit;
	}

	gettimeofday(&t2, NULL);

	delta_sec = t2.tv_sec - t1.tv_sec;
	delta_usec = t2.tv_usec - t1.tv_usec;

	if (t2.tv_usec < t1.tv_usec) {
		delta_sec--;
		delta_usec += 1000000;
	}

	UseT = (float)delta_sec + (float)delta_usec / 1000000;

	printf("\tFlash Chip Erase in %.3f seconds.\n", UseT);

	ioBuffer = (uint8_t *)malloc(filesize);

	ret = fread(ioBuffer, 1, filesize, fp);
	if (ret != filesize) {
		printf("file read failed.\n");
		goto exit;
	}

	gettimeofday(&t1, NULL);

	ret = Flash_Block_Write(FlashAddr, ioBuffer, filesize);
	if (ret != filesize) {
		printf("\tFlash Write: Addr[0x%x] write %lld bytes failed.\n",
		       FlashAddr, filesize);
		goto exit;
	}

	gettimeofday(&t2, NULL);

	delta_sec = t2.tv_sec - t1.tv_sec;
	delta_usec = t2.tv_usec - t1.tv_usec;

	if (t2.tv_usec < t1.tv_usec) {
		delta_sec--;
		delta_usec += 1000000;
	}

	UseT = ((float)delta_sec + (float)delta_usec / 1000000);

	printf("\tFlash Write File in %.3f seconds.\n", UseT);

	free(ioBuffer);
	fclose(fp);
	return true;

exit:

	free(ioBuffer);
	fclose(fp);
	return false;
}

bool Flash_Block_Read_Test()
{
	uint32_t DataLen, FlashAddr, i;
	uint8_t ioBuffer[8192] = { 0 };
	char FmtStr1[8 * 1024 * 3 + 16] = "";

	static struct timeval t1, t2;
	long long delta_sec, delta_usec;

	FlashAddr = 0x00;
	DataLen = 0x500;

	DataLen = Flash_Block_Read(FlashAddr, ioBuffer, DataLen);
	if (DataLen <= 0) {
		printf("\tFlash Read: Addr[0x%x] read %d bytes failed.\n",
		       FlashAddr, DataLen);
		return false;
	}

	for (i = 0; i < DataLen; i++)
		sprintf(&FmtStr1[strlen(FmtStr1)], "%02x ", ioBuffer[i]);
	printf("\nRead: \n%s\n\n", FmtStr1);

	return true;
}

bool Flash_Block_Write_Test()
{
	int ret;
	int i = 0;
	uint32_t DataLen, FlashAddr, BeginAddr, NumOfPage, NumOfSingle;
	uint8_t ioBuffer[0x500] = { 0 };
	uint8_t *pbuf = ioBuffer;

	/* write flash from addr 0 */
	FlashAddr = 0x00;
	BeginAddr = FlashAddr;
	DataLen = 0x500;

	for (i = 0; i < DataLen; i++)
		ioBuffer[i] = i;

	NumOfPage = DataLen / SPI_FLASH_PerWritePageSize;
	NumOfSingle = DataLen % SPI_FLASH_PerWritePageSize;

	while (NumOfPage--) {
		ret = W25X_Flash_Write_Page(pbuf, FlashAddr,
					    SPI_FLASH_PerWritePageSize);
		if (ret == false)
			goto exit;
		pbuf += SPI_FLASH_PerWritePageSize;
		FlashAddr += SPI_FLASH_PerWritePageSize;
	}
	if (NumOfSingle) {
		ret = W25X_Flash_Write_Page(pbuf, FlashAddr, NumOfSingle);
		if (ret == false)
			goto exit;
	}
	return true;

exit:
	printf("\tFlash Write: Addr [0x%x] write %d bytes failed.\n",
	       BeginAddr, DataLen);
	return false;
}

bool Flash_Write_Enable()
{
	int iChipSelect;
	int iLength;
	uint8_t ioBuffer;

	iChipSelect = 0x80;
	iLength = 1;
	ioBuffer = CMD_FLASH_WREN;

	return CH347SPI_WriteRead(ch347device.fd, false, iChipSelect,
				  iLength, &ioBuffer);
}

bool Flash_Wait(int retry_times)
{
	int iChipSelect;
	int iLength;
	uint8_t ioBuffer[2];
	uint8_t status;

	iChipSelect = 0x80;
	iLength = 2;

	do {
		ioBuffer[0] = CMD_FLASH_RDSR;
		if (CH347SPI_WriteRead(ch347device.fd, false, iChipSelect,
				       iLength, ioBuffer) == false)
			return false;
		status = ioBuffer[1];
		usleep(100);
	} while ((status & 0x01) && (retry_times--));

	if ((status & 0x01) == 0)
		return true;
	else
		return false;
}

bool Flash_Sector_Erase(uint32_t StartAddr)
{
	int iChipSelect;
	int iLength;
	uint8_t ioBuffer[4];

	if (Flash_Write_Enable() == false)
		return false;

	iChipSelect = 0x80;
	iLength = 4;
	ioBuffer[0] = CMD_FLASH_SECTOR_ERASE;
	ioBuffer[1] = (uint8_t)(StartAddr >> 16 & 0xff);
	ioBuffer[2] = (uint8_t)(StartAddr >> 8 & 0xf0);
	ioBuffer[3] = 0x00;

	if (CH347SPI_WriteRead(ch347device.fd, false, iChipSelect, iLength,
			       ioBuffer) == false)
		return false;

	if (Flash_Wait(1000) == false)
		return false;

	return true;
}

bool Flash_Chip_Erase()
{
	int iChipSelect;
	int iLength;
	uint8_t ioBuffer[1];

	if (Flash_Write_Enable() == false)
		return false;

	iChipSelect = 0x80;
	iLength = 1;
	ioBuffer[0] = CMD_FLASH_CHIP_ERASE;

	if (CH347SPI_WriteRead(ch347device.fd, false, iChipSelect, iLength,
			       ioBuffer) == false)
		return false;

	if (Flash_Wait(10 * 10000) == false)
		return false;

	return true;
}

bool W25X_Flash_Write_Page(uint8_t *pBuf, uint32_t address, uint32_t len)
{
	int iChipSelect;
	int iLength;
	uint8_t ioBuffer[8192];

	if (!Flash_Write_Enable())
		return false;

	iChipSelect = 0x80;
	iLength = len + 4;
	ioBuffer[0] = CMD_FLASH_BYTE_PROG;
	ioBuffer[1] = (uint8_t)(address >> 16);
	ioBuffer[2] = (uint8_t)(address >> 8);
	ioBuffer[3] = (uint8_t)address;
	memcpy(&ioBuffer[4], pBuf, len);

	if (CH347SPI_Write(ch347device.fd, false, iChipSelect, iLength,
			   SPI_FLASH_PerWritePageSize + 4,
			   ioBuffer) == false)
		return false;

	memset(ioBuffer, 0, sizeof(uint8_t) * len);

	if (!Flash_Wait(1000))
		return false;

	return true;
}

/* EEPROM API */

bool EEPROM_Read()
{
	bool ret = false;
	EEPROM_TYPE eeprom;
	int iAddr;
	int iLength;
	int i;
	uint8_t oBuffer[256] = { 0 };

	eeprom = ID_24C02;
	iAddr = 0;
	iLength = 256;

	ret = CH347ReadEEPROM(ch347device.fd, eeprom, 0, iLength, oBuffer);
	if (ret == false)
		goto exit;

	printf("\nRead EEPROM data:\n");
	for (i = 0; i < iLength; i++) {
		printf("%02x ", oBuffer[i]);
		if (((i + 1) % 10) == 0)
			putchar(10);
	}
	putchar(10);

exit:
	return ret;
}

bool EEPROM_Write()
{
	bool ret = false;
	EEPROM_TYPE eeprom;
	int iAddr;
	int iLength;
	int i;
	uint8_t iBuffer[256] = { 0 };

	eeprom = ID_24C02;
	iAddr = 0;
	iLength = 256;
	for (i = 0; i < 256; i++)
		iBuffer[i] = i;

	printf("\nWrite EEPROM data:\n");
	ret = CH347WriteEEPROM(ch347device.fd, eeprom, iAddr, iLength,
			       iBuffer);
	if (ret == false)
		goto exit;

	for (i = 0; i < iLength; i++) {
		printf("%02x ", iBuffer[i]);
		if (((i + 1) % 10) == 0)
			putchar(10);
	}
	putchar(10);

exit:
	return ret;
}

bool SPI_Read_Test()
{
	int iChipSelect;
	int iLength;
	int oLength;
	uint8_t ioBuffer[1024 * 2048] = { 0 };

	iChipSelect = 0x80;
	iLength = 0;
	oLength = 1024 * 2048;

	if (CH347SPI_Read(ch347device.fd, false, iChipSelect, iLength,
			  &oLength, ioBuffer) == false)
		return false;

	printf("oLength: %d, ioBuffer[0]: 0x%2x\n", oLength, ioBuffer[0]);

	return true;
}

void ch34x_demo_flash_operate()
{
	bool ret = false;

	ret = CH347_SPI_Init();
	if (ret == false) {
		printf("Failed to init CH347 SPI interface.\n");
		return;
	}
	printf("CH347 SPI interface init succeed.\n");

	// ret = SPI_Read_Test();
	// if (!ret) {
	// 	printf("Failed to read CH347 SPI.\n");
	// 	return;
	// }

	/* read flash ID */
	ret = Flash_ID_Read();
	if (!ret) {
		printf("Failed to read flash ID.\n");
		return;
	}

	/* read flash block data */
	ret = Flash_Block_Read_Test();
	if (!ret) {
		printf("Failed to read flash.\n");
		return;
	}

	/* erase flash sector data */
	ret = Flash_Sector_Erase(0x00);
	if (!ret) {
		printf("Failed to erase flash.\n");
		return;
	}
	printf("Erase one sector from Addr[0x%x] of flash succeed.\n",
	       0x00);

	/* write flash block data */
	ret = Flash_Block_Write_Test();
	if (!ret) {
		printf("Failed to write flash.\n");
		return;
	}

	/* read flash block data */
	ret = Flash_Block_Read_Test();
	if (!ret) {
		printf("Failed to read flash.\n");
		return;
	}
}

#define MB (1 * 1024 * 1024)

// void ch34x_demo_spi_slave_operate(bool enable)
// {
// 	bool ret = false;
// 	uint8_t oBuffer[SPI_SLAVE_MAX_LENGTH];
// 	uint32_t oLength;
// 	uint64_t bytes_read = 0;
// 	struct timeval t1, t2;
// 	int delta_sec = 0;
// 	int delta_usec = 0;
// 	float speed_read = 0.0;
// 	int MB_Num = 10;
// 	int i, j;

// 	if (enable) {
// 		ret = CH347_SPI_Slave_Init();
// 		if (ret == false) {
// 			printf("Failed to init CH347 SPI interface.\n");
// 			return;
// 		}
// 		printf("CH347 SPI interface init succeed.\n");

// 		ret = CH347SPI_Slave_FIFOReset(ch347device.fd);
// 		if (ret == false) {
// 			printf("Failed to reset SPI slave fifo.\n");
// 			return;
// 		}
// 		printf("CH347 SPI slave fifo reset succeed.\n");

// 		ret = CH347SPI_Slave_Control(ch347device.fd, true);
// 		if (!ret)
// 			return;
// 		printf("Begin read data in slave mode.\n");
// 		while (1) {
// 			bytes_read = 0;
// 			gettimeofday(&t1, NULL);
// 			while (bytes_read <= MB_Num * MB) {
// 				ret = CH347SPI_Slave_QweryData(
// 					ch347device.fd, &oLength);
// 				if (!ret) {
// 					printf("CH347SPI_Slave_QweryData failed.\n");
// 					goto exit;
// 				}
// 				if (oLength == 0) {
// 					usleep(100);
// 					continue;
// 				}
// 				ret = CH347SPI_Slave_ReadData(
// 					ch347device.fd, oBuffer, &oLength);
// 				if (!ret) {
// 					printf("CH347SPI_Slave_ReadData failed.\n");
// 					goto exit;
// 				}
// 				if (oLength > 1) {
// 					for (i = 1; i < oLength; i++) {
// 						if (((oBuffer[i] -
// 						      oBuffer[i - 1]) !=
// 						     1) &&
// 						    (oBuffer[i] != 0)) {
// 							printf("Data Error, i = %d, buf[i] = 0x%2x, buf[i-1]: 0x%2x\n",
// 							       i,
// 							       oBuffer[i],
// 							       oBuffer[i -
// 								       1]);
// 							for (j = 0;
// 							     j < oLength;
// 							     j++)
// 								printf("0x%2x ",
// 								       oBuffer[j]);
// 							printf("\n");
// 							goto exit;
// 						}
// 					}
// 				}
// 				bytes_read += oLength;
// 			}
// 			gettimeofday(&t2, NULL);

// 			delta_sec = t2.tv_sec - t1.tv_sec;
// 			delta_usec = t2.tv_usec - t1.tv_usec;

// 			if (delta_usec < 0) {
// 				delta_sec--;
// 				delta_usec = 1000000 + delta_usec;
// 			}
// 			speed_read = (float)(MB_Num) /
// 				     (float)(delta_sec +
// 					     (float)delta_usec / 1000000);

// 			printf("%d MB takes: %2d sec %2d us, up speed: %4.2f MB/s\n",
// 			       MB_Num, delta_sec, delta_usec, speed_read);
// 		}
// 	} else
// 		ret = CH347SPI_Slave_Control(ch347device.fd, false);

// 	return;

// exit:
// 	CH347SPI_Slave_Control(ch347device.fd, false);
// }

unsigned long long g_bytes_read;

/**
 * sig_handler_readtimer - timer callback for read
 * @signo: signal value
 */
void sig_handler_readtimer(int signo)
{
	double r_speed;
	double cur;
	unsigned long long last_bytes_read;
	int i;

	switch (signo) {
	case SIGALRM:
		if (g_bytes_read > 0) {
			cur = g_bytes_read - last_bytes_read;
			r_speed = cur / 1024.0 / 1.0;
			printf("Total receive: %.2f KB, r speed: %.2f KB/s\n",
			       (double)g_bytes_read / 1024, r_speed);
			last_bytes_read = g_bytes_read;
		}
		break;
	default:
		break;
	}
}

/**
 * init_timer - timer init
 * @cmd: timer type
 *
 */
void init_timer()
{
	struct itimerval new_value, old_value;

	signal(SIGALRM, sig_handler_readtimer);
	new_value.it_value.tv_sec = 0;
	new_value.it_value.tv_usec = 1000;
	new_value.it_interval.tv_sec = 1;
	new_value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_value, &old_value);
}

// SPI Loopback and EEPROM Test
void ch34x_spi_i2c_pressure_test()
{
	bool ret = false;
	int len;
	int i;
	uint8_t tBuffer[0x1000] = { 0 };
	uint8_t rBuffer[0x1000] = { 0 };
	int count = 0;

	EEPROM_TYPE eeprom = ID_24C02;
	int iAddr = 0;
	int i2c_len = 256;

	ret = CH347_SPI_Init();
	if (ret == false) {
		printf("Failed to init CH347 SPI interface.\n");
		return;
	}
	printf("CH347 SPI interface init succeed.\n");

	ret = CH347_I2C_Init();
	if (!ret) {
		printf("Failed to init CH347 I2C.\n");
		return;
	}
	printf("CH347 I2C interface init succeed.\n");

retry:
	srand(time(0) + rand());
	len = rand() % 4000 + 50;

	// len = 500;

	len = 4000;

	for (i = 0; i < len; i++) {
		tBuffer[i] = rand() % 256;
	}

	memcpy(rBuffer, tBuffer, len);
	ret = CH347SPI_WriteRead(ch347device.fd, false, 0x80, len,
				 rBuffer);
	if (ret == false) {
		printf("Failed to CH347SPI_WriteRead.\n");
		return;
	}
	if (memcmp(tBuffer, rBuffer, len)) {
		printf("SPI data comare error!!! (len = %d)\n\n", len);

		printf("TX data:\n");
		for (i = 0; i < len; i++) {
			printf("0x%2x ", tBuffer[i]);
		}
		printf("\n");

		printf("RX data:\n");
		for (i = 0; i < len; i++) {
			printf("0x%2x ", rBuffer[i]);
		}
		printf("\n");
		return;
	} else {
		// printf("spi loopback ok (len = %d), count: %d\n", len, count);
		g_bytes_read += len;
	}

	// ret = CH347WriteEEPROM(ch347device.fd, eeprom, iAddr, i2c_len,
	// 		       tBuffer);
	// if (ret == false) {
	// 	printf("Failed to CH347WriteEEPROM.\n");
	// 	return;
	// }

	// ret = CH347ReadEEPROM(ch347device.fd, eeprom, 0, i2c_len, rBuffer);
	// if (ret == false) {
	// 	printf("Failed to CH347ReadEEPROM.\n");
	// 	return;
	// }

	// if (memcmp(tBuffer, rBuffer, i2c_len)) {
	// 	printf("I2C data comare error!!! (len = %d)\n\n", len);

	// 	printf("TX data:\n");
	// 	for (i = 0; i < i2c_len; i++) {
	// 		printf("0x%2x ", tBuffer[i]);
	// 	}
	// 	printf("\n");

	// 	printf("RX data:\n");
	// 	for (i = 0; i < i2c_len; i++) {
	// 		printf("0x%2x ", rBuffer[i]);
	// 	}
	// 	printf("\n");
	// 	return;
	// } else {
	// 	printf("i2c eeprom test ok (i2c_len = %d), count: %d\n",
	// 	       i2c_len, count++);
	// }
	// usleep(1000 * 1000);
	goto retry;
}

// FLASH and EEPROM Test
void ch34x_spi_i2c_pressure_test2()
{
	bool ret = false;
	int len;
	int i;
	uint8_t tBuffer[0x1000] = { 0 };
	uint8_t rBuffer[0x1000] = { 0 };
	int count = 0;

	EEPROM_TYPE eeprom = ID_24C02;
	int iAddr = 0;
	int i2c_len = 256;

	// srand(time(0) + rand());
	// for (i = 0; i < i2c_len; i++) {
	// 	tBuffer[i] = rand() % 256;
	// }

	for (i = 0; i < i2c_len; i++) {
		tBuffer[i] = i;
	}

	ret = CH347_SPI_Init();
	if (ret == false) {
		printf("Failed to init CH347 SPI interface.\n");
		return;
	}
	printf("CH347 SPI interface init succeed.\n");

	ret = CH347_I2C_Init();
	if (!ret) {
		printf("Failed to init CH347 I2C.\n");
		return;
	}
	printf("CH347 I2C interface init succeed.\n");

retry:
	ret = Flash_File_Write();
	if (ret == false) {
		printf("Failed to Flash_File_Write.\n");
		return;
	}

	ret = Flash_File_Read();
	if (ret == false) {
		printf("Failed to Flash_File_Read.\n");
		return;
	}

	// ret = CH347WriteEEPROM(ch347device.fd, eeprom, iAddr, i2c_len, tBuffer);
	// if (ret == false) {
	// 	printf("Failed to CH347WriteEEPROM.\n");
	// 	return;
	// }

	// ret = CH347ReadEEPROM(ch347device.fd, eeprom, 0, i2c_len, rBuffer);
	// if (ret == false) {
	// 	printf("Failed to CH347ReadEEPROM.\n");
	// 	return;
	// }

	// if (memcmp(tBuffer, rBuffer, i2c_len)) {
	// 	printf("I2C data comare error!!! (len = %d)\n\n", len);

	// 	printf("TX data:\n");
	// 	for (i = 0; i < i2c_len; i++) {
	// 		printf("0x%2x ", tBuffer[i]);
	// 	}
	// 	printf("\n");

	// 	printf("RX data:\n");
	// 	for (i = 0; i < i2c_len; i++) {
	// 		printf("0x%2x ", rBuffer[i]);
	// 	}
	// 	printf("\n");
	// 	return;
	// } else {
	// 	printf("i2c eeprom test ok (i2c_len = %d), count: %d\n", i2c_len, count++);
	// }

	// usleep(1000 * 1000);

	goto retry;
}

void ch34x_spi_master_to_slave_test()
{
	bool ret = false;
	int len;
	int i;
	uint8_t tBuffer[0x1000] = { 0 };
	int count = 0;

	ret = CH347_SPI_Init();
	if (ret == false) {
		printf("Failed to init CH347 SPI interface.\n");
		return;
	}
	printf("CH347 SPI interface init succeed.\n");

retry:
	srand(time(0) + rand());
	len = rand() % 4000 + 50;

	len = 4080;

	for (i = 0; i < len; i++) {
		tBuffer[i] = i;
	}

	ret = CH347SPI_Write(ch347device.fd, false, 0x80, len, len,
			     tBuffer);
	if (ret == false) {
		printf("Failed to CH347SPI_Write.\n");
		return;
	}

	ret = CH347SPI_WriteRead(ch347device.fd, false, 0x80, len,
				 tBuffer);
	if (ret == false) {
		printf("Failed to CH347SPI_WriteRead.\n");
		return;
	}

	printf("spi master send (len = %d), count: %d\n", len, count++);

	usleep(10 * 1000);
	goto retry;
}

int len_spi[] = { 6, 507, 510, 1000, 4000 };
int index_spi = 0;

void ch34x_demo_spi_packet_test()
{
	bool ret = false;

	ret = CH347_SPI_Init();
	if (!ret) {
		printf("Failed to init CH347 SPI.\n");
		return;
	}
	printf("CH347 SPI interface init succeed.\n");

	char ibuf[4096];
	char obuf[4096];
	int olen;

	ibuf[0] = 0x00;
	ibuf[1] = 0x01;
	ibuf[2] = 0x02;
	ibuf[3] = 0x03;
	ibuf[4] = 0x04;
	ibuf[5] = 0x05;
	ibuf[6] = 0x06;

	for (int i = 0; i < 4096; i++)
		ibuf[i] = i;

	ret = CH347SPI_WriteRead(ch347device.fd, false, 0x80,
				 len_spi[index_spi++], ibuf);
	printf("...%x %x %x\n", ret, ibuf[0], ibuf[1]);

	// ret = CH347SPI_Write(ch347device.fd, false, 0x80, 1000, 4000, ibuf);
	// printf("...%x %x %x\n", ret, ibuf[0], ibuf[1]);

	// olen = 4000;
	// ret = CH347SPI_Read(ch347device.fd, false, 0x80, 4000, &olen, ibuf);
	// printf("...%x %x %x\n", ret, ibuf[0], ibuf[1]);
	// printf("olen: %d\n", olen);

	return;
}

int len_i2c[] = { 10, 50, 100, 507, 510, 1000 };
int index_i2c = 0;

void ch34x_demo_i2c_packet_test()
{
	bool ret = false;
	int retAck;

	ret = CH347_I2C_Init();
	if (!ret) {
		printf("Failed to init CH347 I2C.\n");
		return;
	}
	printf("CH347 I2C interface init succeed.\n");

	// CH347I2C_SetDelaymS(ch347device.fd, 500);

	char ibuf[1024];
	char obuf[1024];

	ibuf[0] = 0xA0;
	ibuf[1] = 0x00;
	ibuf[2] = 0x01;
	ibuf[3] = 0x02;
	ibuf[4] = 0x03;
	ibuf[5] = 0x04;
	ibuf[6] = 0x05;

	ret = CH347StreamI2C(ch347device.fd, len_i2c[index_i2c++], ibuf, 0,
			     obuf);
	printf("...%x %x %x\n", ret, obuf[0], obuf[1]);

	// ibuf[0] = 0xA0;
	// ibuf[1] = 0x00;

	// ret = CH347StreamI2C_RetAck(ch347device.fd, 2, ibuf, 2, obuf, &retAck);
	// printf("...%x %x %x %x\n", ret, retAck, obuf[0], obuf[1]);
	return;

	// ret = CH347StreamI2C(ch347device.fd, 10, ibuf, 0, obuf);
	// 	ret = CH347StreamI2C(ch347device.fd, 10, ibuf, 0, obuf);
	// 		ret = CH347StreamI2C(ch347device.fd, 10, ibuf, 0, obuf);
	// 			ret = CH347StreamI2C(ch347device.fd, 10, ibuf, 0, obuf);

	// return;
}

void ch34x_demo_eeprom_operate()
{
	bool ret = false;

	ret = CH347_I2C_Init();
	if (!ret) {
		printf("Failed to init CH347 I2C.\n");
		return;
	}
	printf("CH347 I2C interface init succeed.\n");

	ret = EEPROM_Read();
	if (!ret) {
		printf("Failed to read eeprom.\n");
		return;
	}

	ret = EEPROM_Write();
	if (!ret) {
		printf("Failed to write eeprom.\n");
		return;
	}

	ret = EEPROM_Read();
	if (!ret) {
		printf("Failed to read eeprom.\n");
		return;
	}
}

void ch34x_demo_jtag_operate()
{
	int i;
	int oReadLength;
	uint8_t retue[32] = { 0 };
	uint8_t IDCODE[4096] = { 0 };

	oReadLength = 32;
	/* init jtag tck clock */
	CH347Jtag_INIT(ch347device.fd, 4);
	/* reset target jtag device */
	CH347Jtag_SwitchTapState(ch347device.fd, 0);
	/* SHIFT-DR Read the Target IDCODE */
	CH347Jtag_ByteReadDR(ch347device.fd, &oReadLength, &retue);

	printf("Target IDCODE: \n");
	for (i = 0; i < 4; i++) {
		printf("0x%2x", retue[3 - i]);
	}
	puts("");

	return;
}

static void ch34x_demo_gpio_input_operate()
{
	bool ret;
	int i, j;
	int gpiocount = 8;
	uint8_t iDir = 0xff;
	uint8_t iData = 0x00;

	ret = CH347GPIO_Get(ch347device.fd, &iDir, &iData);
	if (ret == false) {
		printf("CH347GPIO_Set error.\n");
		return;
	}

	printf("\n********** GPIO Input Start **********\n\n");
	for (i = 0; i < gpiocount; i++) {
		if ((iData & (1 << i)))
			printf("H");
		else
			printf("L");
	}
	printf("\n");
	printf("\n********** GPIO Input End **********\n\n");
}

static void ch34x_demo_gpio_output_operate()
{
	bool ret;
	int i, j;
	int gpiocount = 8;
	uint8_t iEnable = 0xff;
	uint8_t iSetDirOut = 0xff;
	uint8_t iSetDataOut = 0x00;

	/* analog leds here */
	CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);

	printf("\n********** GPIO Output Start **********\n");
	for (i = 0; i < gpiocount; i++) {
		iSetDataOut = 1 << i;
		ret = CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut,
				    iSetDataOut);
		if (ret == false) {
			printf("CH347GPIO_Set error.\n");
			return;
		}
		printf("\n");
		for (j = 0; j < gpiocount; j++) {
			if (j == i)
				printf("H");
			else
				printf("L");
		}
		printf("\n");
		usleep(200 * 1000);
	}
	iSetDataOut = 0x00;
	CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);
	printf("\n********** GPIO Output End **********\n\n");
}

static void ch34x_demo_isr_handler(int signo)
{
	static int int_times = 0;

	printf("ch34x interrupt times: %d\n", int_times++);
}

static void ch34x_demo_irq_operate(bool enable)
{
	bool ret;
	int gpioindex = 7;

	ret = CH347GPIO_IRQ_Set(ch347device.fd, gpioindex, enable,
				IRQ_TYPE_EDGE_FALLING,
				ch34x_demo_isr_handler);
	if (!ret) {
		printf("Failed to set CH347 irq function.");
		return;
	}
}

void ch34x_demo_uart_operate()
{
	bool ret = false;
	uint8_t iBuffer[256];
	uint8_t oBuffer[256];
	int ioLength;
	int i;

	ioLength = 256;
	for (i = 0; i < 256; i++)
		// iBuffer[i] = i;

		iBuffer[i] = 0x55;

	ret = CH347Uart_Init(ch347device.fd, hid_baudrate, 8, 0, 0, 1);
	if (!ret) {
		printf("Failed to init CH347 UART interface.\n");
		return;
	}
	printf("CH347 UART interface init succeed.\n");

	ret = CH347Uart_Write(ch347device.fd, iBuffer, &ioLength);
	if (ret == false) {
		printf("CH347Uart_Write failed.\n");
		return;
	}
	printf("Uart wrote %d bytes already.\n", ioLength);

	ret = CH347Uart_Read(ch347device.fd, oBuffer, &ioLength);
	if (ret == false) {
		printf("CH347Uart_Read failed.\n");
		return;
	}

	printf("\nRead Uart data:\n");
	for (i = 0; i < ioLength; i++) {
		printf("%02x ", oBuffer[i]);
		if (((i + 1) % 10) == 0)
			putchar(10);
	}
	putchar(10);
}

int baudrate_table[] = { 1200,	 1800,	 2400,	 3600,	 4800,	 9600,
			 14400,	 19200,	 28800,	 33600,	 38400,	 56000,
			 57600,	 76800,	 115200, 128000, 153600, 230400,
			 256000, 307200, 460800, 921600, 1e6,	 15e5,
			 2e6,	 3e6,	 4e6,	 5e6,	 6e6,	 7e6,
			 8e6,	 9e6 };

int byte_table[] = { 0, 1, 2, 3, 4 };
int byte_index = 0;

int bps_index = 0;

void ch34x_demo_uart_operate1()
{
	bool ret = false;
	uint8_t iBuffer[256];
	uint8_t oBuffer[256];
	uint32_t ioLength;
	int i;

	ioLength = 256;
	for (i = 0; i < 256; i++)
		iBuffer[i] = i;

	iBuffer[0] = 0x55;
	iBuffer[1] = 0x55;
	ioLength = 2;

rewrite:
	ret = CH347Uart_Init(ch347device.fd, hid_baudrate,
			     byte_table[byte_index++], 2, 0, 2);
	if (!ret) {
		printf("Failed to init CH347 UART interface.\n");
		return;
	}
	printf("CH347 UART interface init succeed.\n");

	sleep(1);

	ret = CH347Uart_Write(ch347device.fd, iBuffer, &ioLength);
	if (ret == false) {
		printf("CH347Uart_Write failed.\n");
		return;
	}
	printf("Uart wrote %d bytes already.\n", ioLength);

	usleep(1000 * 100);

	if (byte_index > sizeof(byte_table) / sizeof(int))
		return;

	goto rewrite;
}

char aa = 0x00;

void ch34x_demo_uart_pressure_test()
{
	bool ret = false;
	uint8_t iBuffer[256];
	uint8_t oBuffer[256];
	int ioLength;
	int i;

	ioLength = 256;
	for (i = 0; i < 256; i++)
		iBuffer[i] = aa++;

	ret = CH347Uart_Init(ch347device.fd, hid_baudrate, 3, 0, 0, 1);
	if (!ret) {
		printf("Failed to init CH347 UART interface.\n");
		return;
	}
	printf("CH347 UART interface init succeed.\n");

	sleep(1);

rewrite:
	ret = CH347Uart_Write(ch347device.fd, iBuffer, &ioLength);
	if (ret == false) {
		printf("CH347Uart_Write failed.\n");
		return;
	}

	ioLength = 256;
	ret = CH347Uart_Read(ch347device.fd, oBuffer, &ioLength);
	if (ret == false || ioLength != 256) {
		printf("CH347Uart_Read failed.\n");
		return;
	}
	if (memcmp(oBuffer, iBuffer, ioLength)) {
		printf("HID Uart data compare error!!! (len = %d)\n\n",
		       ioLength);

		printf("TX data:\n");
		for (i = 0; i < ioLength; i++) {
			printf("0x%2x ", iBuffer[i]);
		}
		printf("\n");

		printf("RX data:\n");
		for (i = 0; i < ioLength; i++) {
			printf("0x%2x ", oBuffer[i]);
		}
		printf("\n");
		return;
	}
	printf("Uart compare %d bytes succeed.\n", ioLength);

	goto rewrite;
}

bool Show_DevMsg(char *pathname)
{
	unsigned char buf[256];
	int ret;
	int i;
	struct hidraw_devinfo info;
	uint16_t vendor, product;
	CHIP_TYPE chiptype;

	if (strstr(pathname, "tty")) {
		printf("Device operating has function [UART].\n");
		ch347device.functype = FUNC_UART;
	} else if (strstr(pathname, "hidraw")) {
		/* Get Raw Name */
		ret = ioctl(ch347device.fd, HIDIOCGRAWNAME(256), buf);
		if (ret < 0) {
			perror("HIDIOCGRAWNAME");
			goto exit;
		} else
			printf("Raw Name: %s\n", buf);

		/* Get Raw Info */
		ret = ioctl(ch347device.fd, HIDIOCGRAWINFO, &info);
		if (ret < 0) {
			perror("HIDIOCGRAWINFO");
			goto exit;
		} else {
			printf("Raw Info:\n");
			printf("\tvendor: 0x%04hx\n", info.vendor);
			printf("\tproduct: 0x%04hx\n", info.product);
		}

		if (info.vendor == 0x1a86) {
			if (info.product == 0x55dc)
				ch347device.chiptype = CHIP_CH347T;
			else if (info.product == 0x55e5)
				ch347device.chiptype = CHIP_CH347F;
			else {
				printf("Current HID device PID is not CH347.\n");
				return -1;
			}
		} else {
			printf("Current HID device VID is not CH347.\n");
			return -1;
		}

		/* Get Physical Location */
		ret = ioctl(ch347device.fd, HIDIOCGRAWPHYS(256), buf);
		if (ret < 0) {
			perror("HIDIOCGRAWPHYS");
			goto exit;
		} else
			printf("Raw Phys: %s\n", buf);

		if (ch347device.chiptype == CHIP_CH347T) {
			if (strstr(buf, "input0")) {
				ch347device.functype = FUNC_UART;
				printf("Device operating has function [UART].\n");
			} else {
				ch347device.functype = FUNC_SPI_I2C_GPIO;
				printf("Device operating has function [SPI+I2C+GPIO].\n");
			}
		} else {
			if (strstr(buf, "input0")) {
				ch347device.functype = FUNC_UART;
				printf("Device operating has function [UART].\n");
			} else if (strstr(buf, "input2")) {
				ch347device.functype = FUNC_UART;
				printf("Device operating has function [UART].\n");
			} else {
				ch347device.functype = FUNC_SPI_I2C_GPIO;
				printf("Device operating has function [SPI+I2C+JTAG+GPIO].\n");
			}
		}
	} else if (strstr(pathname, "ch34x_pis")) {
		/* Get Driver Version */
		ret = CH34x_GetDriverVersion(ch347device.fd,
					     ch347device.version);
		if (ret == false) {
			printf("CH34x_GetDriverVersion error.\n");
			goto exit;
		}
		printf("Driver version: %s\n", ch347device.version);

		/* Get Chip Type */
		ret = CH34x_GetChipType(ch347device.fd,
					&ch347device.chiptype);
		if (ret == false) {
			printf("CH34x_GetChipType error.\n");
			goto exit;
		}
		if (ch347device.chiptype == CHIP_CH341) {
			printf("Current chip operating is CH341, please use ch341_demo.\n");
			goto exit;
		}

		/* Get Device ID */
		ret = CH34X_GetDeviceID(ch347device.fd,
					&ch347device.dev_id);
		if (ret == false) {
			printf("CH34X_GetDeviceID error.\n");
			goto exit;
		}
		vendor = ch347device.dev_id;
		product = ch347device.dev_id >> 16;
		printf("Vendor ID: 0x%4x, Product ID: 0x%4x\n", vendor,
		       product);
		if (product == 0x55db) {
			ch347device.functype = FUNC_SPI_I2C_GPIO;
			printf("Device operating has function [SPI+I2C+GPIO].\n");
		} else if (product == 0x55dd) {
			ch347device.functype = FUNC_JTAG_GPIO;
			printf("Device operating has function [JTAG+GPIO].\n");
		} else if (product == 0x55de || product == 0x55e7) {
			ch347device.functype = FUNC_SPI_I2C_JTAG_GPIO;
			printf("Device operating has function [SPI+I2C+JTAG+GPIO].\n");
		}
	}
	return true;

exit:
	return false;
}

int main(int argc, char *argv[])
{
	bool ret;
	char choice, ch;

	if (argc != 3) {
		printf("Usage: sudo %s [device]\n", argv[0]);
		return -1;
	}

	hid_baudrate = atoi(argv[2]);

	printf("hid baudrate: %d\n", hid_baudrate);

	/* open device */
	ch347device.fd = CH347OpenDevice(argv[1]);
	if (ch347device.fd < 0) {
		printf("CH347OpenDevice false.\n");
		return -1;
	}
	printf("Open device %s succeed, fd: %d\n", argv[1],
	       ch347device.fd);

	// init_timer();

	ret = Show_DevMsg(argv[1]);
	if (ret == false)
		return -1;

	sleep(1);

	if (strstr(argv[1], "ch34x_pis")) {
		ret = CH34xSetTimeout(ch347device.fd, 2000, 2000);
		if (ret == false) {
			printf("CH34xSetTimeout false.\n");
			return -1;
		}
	}

	ch34x_demo_spi_packet_test();

	return;

	// retry:
	// ch34x_demo_i2c_packet_test();
	// CH347_OE_Enable(ch347device.fd);
	// ch34x_demo_spi_packet_test();
	// while ((ch = getchar()) != EOF && ch != '\n')
	// 			;
	// goto retry;
	// return;

	switch (ch347device.functype) {
	case FUNC_UART:
		while (1) {
			printf("\npress u to operate uart, p to uart pressure test, q to quit.\n");
			scanf("%c", &choice);
			while ((ch = getchar()) != EOF && ch != '\n')
				;
			if (choice == 'q')
				break;
			switch (choice) {
			case 'u':
				ch34x_demo_uart_operate();
				break;
			case 'p':
				printf("HID Uart Pressure Test begin.\n");
				ch34x_demo_uart_pressure_test();
				break;
			default:
				break;
			}
		}
		break;
	case FUNC_SPI_I2C_GPIO:
	case FUNC_SPI_I2C_JTAG_GPIO:
		while (1) {
			printf("\npress f to operate spi flash, e to operate eeprom,\n"
			       "g to operate gpio interface, i to enable interrupt,\n"
			       "d to disable interrupt, p to spi master loop and i2c pressure test,\n"
			       "m to spi master send with slave, q to quit.\n");

			scanf("%c", &choice);
			while ((ch = getchar()) != EOF && ch != '\n')
				;
			if (choice == 'q')
				break;

			switch (choice) {
			case 'f':
				printf("FLASH Test begin.\n");
				ch34x_demo_flash_operate();
				break;
			case 'e':
				printf("EEPROM Test begin.\n");
				ch34x_demo_eeprom_operate();
				break;
			case 'a':
				printf("GPIO Input Test Begin.\n");
				ch34x_demo_gpio_input_operate();
				break;
			case 'g':
				printf("GPIO Output Test Begin.\n");
				ch34x_demo_gpio_output_operate();
				break;
			case 'i':
				printf("IRQ Test Begin.");
				ch34x_demo_irq_operate(true);
				break;
			case 'd':
				printf("IRQ Test Over.\n");
				ch34x_demo_irq_operate(false);
				break;
			case 'p':
				printf("SPI & I2C Pressure Test begin.\n");
				ch34x_spi_i2c_pressure_test2();
				break;
			case 'm':
				printf("SPI Master-to-Slave Test.\n");
				ch34x_spi_master_to_slave_test();
				break;
			default:
				printf("Bad choice, please input again.\n");
				break;
			}
		}
		break;
	case FUNC_JTAG_GPIO:
		while (1) {
			printf("\npress j to operate jtag interface, g to operate gpio interface,\n"
			       "q to quit.\n");

			scanf("%c", &choice);
			while ((ch = getchar()) != EOF && ch != '\n')
				;
			if (choice == 'q')
				break;

			switch (choice) {
			case 'j':
				printf("JTAG Test begin.\n");
				ch34x_demo_jtag_operate();
				break;
			case 'g':
				printf("GPIO Test begin.\n");
				ch34x_demo_gpio_output_operate();
			default:
				printf("Bad choice, please input again.\n");
				break;
			}
		}
		break;
	default:
		break;
	}

	/* close the device */
	if (CH347CloseDevice(ch347device.fd)) {
		printf("Close device succeed.\n");
	}

	return 0;
}
