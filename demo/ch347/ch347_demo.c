/*
 * ch347/ch339 application demo
 *
 * Copyright (C) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
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
 * V1.4 - add gpio interrupt funciton, update with new library,
 *      - support more SPI and I2C stretching
 *      - support I2C clock stretch
 * V1.5 - add support for ch339w
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

bool W25X_Flash_Write_Page(uint8_t *pBuf, uint32_t address, uint32_t len);
bool Flash_Chip_Erase();
bool CH347_SPI_Init()
{
	bool ret;
	mSpiCfgS SpiCfg = { 0 };

	/* set spi interface in [mode3] & [15MHz] & [MSB] & output [0xFF] by default */
	SpiCfg.iMode = 3;
	SpiCfg.iByteOrder = 1;
	SpiCfg.iSpiOutDefaultData = 0xFF;
	SpiCfg.iChipSelect = 0x80;

	ret = CH347SPI_SetFrequency(ch347device.fd, 30e6);
	if (!ret) {
		printf("Failed to set SPI frequency.\n");
		return false;
	}

	/* init spi interface */
	ret = CH347SPI_Init(ch347device.fd, &SpiCfg);
	if (!ret) {
		printf("Failed to init SPI interface.\n");
		return false;
	}

	return true;
}

bool CH347_I2C_Init()
{
	bool ret;
	int iMode;

	/* set i2c interface in 750KHZ */
	iMode = 0x03;

	/* init i2c interface */
	ret = CH347I2C_Set(ch347device.fd, iMode);
	if (!ret) {
		printf("Failed to init I2C interface.\n");
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
	double UseT;

	static struct timeval t1, t2;
	int delta_sec, delta_usec;
	/* write flash from addr 0 */
	FlashAddr = address;

	NumOfPage = DataLen / SPI_FLASH_PerWritePageSize;
	NumOfSingle = DataLen % SPI_FLASH_PerWritePageSize;

	if (address % SPI_FLASH_PerWritePageSize) {
		printf("Flash address is not page aligned!\n");
		return false;
	}
	/* caculate flash write time */
	gettimeofday(&t1, NULL);
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
	gettimeofday(&t2, NULL);

	delta_sec = t2.tv_sec - t1.tv_sec;
	delta_usec = t2.tv_usec - t1.tv_usec;
	UseT = ((float)delta_sec + (float)delta_usec / 1000000);

	printf("\tFlash Write: Addr[0x%x] write %d bytes in %.3f seconds.\n",
	       address, DataLen, UseT / 1000);

	return FlashAddr;
exit:
	printf("\tFlash Write: Addr [0x%x] write %d bytes failed.\n",
	       address, DataLen);
	return FlashAddr;
}
bool Flash_Block_Read_Test()
{
	double UseT;
	uint32_t DataLen, FlashAddr, i;
	uint8_t ioBuffer[8192] = { 0 };
	char FmtStr1[8 * 1024 * 3 + 16] = "";

	static struct timeval t1, t2;
	int delta_sec, delta_usec;

	FlashAddr = 0x00;
	DataLen = 0x500;

	gettimeofday(&t1, NULL);
	DataLen = Flash_Block_Read(FlashAddr, ioBuffer, DataLen);
	if (DataLen <= 0) {
		printf("\tFlash Read: Addr[0x%x] read %d bytes failed.\n",
		       FlashAddr, DataLen);
		return false;
	}
	gettimeofday(&t2, NULL);

	delta_sec = t2.tv_sec - t1.tv_sec;
	delta_usec = t2.tv_usec - t1.tv_usec;
	UseT = (float)delta_sec + (float)delta_usec / 1000000;

	printf("\tFlash Read: Addr[0x%x] read %d bytes in %.3f seconds.\n",
	       FlashAddr, DataLen, UseT);
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

void ch34x_demo_flash_operate()
{
	bool ret = false;

	ret = CH347_SPI_Init();
	if (ret == false) {
		printf("Failed to init CH347 SPI interface.\n");
		return;
	}
	printf("CH347 SPI interface init succeed.\n");

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
	int gpioindex = 6;

	ret = CH347GPIO_IRQ_Set(ch347device.fd, gpioindex, enable,
				IRQ_TYPE_EDGE_BOTH,
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
		iBuffer[i] = i;

	ret = CH347Uart_Init(ch347device.fd, 115200, 8, 0, 0, 1);
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

bool Show_DevMsg(char *pathname)
{
	unsigned char buf[256];
	int ret;
	int i;
	struct hidraw_devinfo info;
	uint16_t vendor, product;
	CHIP_TYPE chiptype;

	printf("Library version: %s\n", CH347GetLibInfo());

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

	if (argc != 2) {
		printf("Usage: sudo %s [device]\n", argv[0]);
		return -1;
	}

	/* open device */
	ch347device.fd = CH347OpenDevice(argv[1]);
	if (ch347device.fd < 0) {
		printf("CH347OpenDevice false.\n");
		return -1;
	}
	printf("Open device %s succeed, fd: %d\n", argv[1],
	       ch347device.fd);

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

	switch (ch347device.functype) {
	case FUNC_UART:
		while (1) {
			printf("\npress u to operate uart, q to quit.\n");
			scanf("%c", &choice);
			while ((ch = getchar()) != EOF && ch != '\n')
				;
			if (choice == 'q')
				break;
			switch (choice) {
			case 'u':
				ch34x_demo_uart_operate();
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
			       "a to get gpio status, g to gpio output test, j to operate jtag interface,\n"
			       "i to enable interrupt, d to disable interrupt, q to quit.\n");

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
			case 'j':
				if (ch347device.functype ==
				    FUNC_SPI_I2C_JTAG_GPIO) {
					printf("JTAG Test begin.\n");
					ch34x_demo_jtag_operate();
				} else {
					printf("Chip is not CH347F.\n");
				}
				break;
			default:
				printf("Bad choice, please input again.\n");
				break;
			}
		}
		break;
	case FUNC_JTAG_GPIO:
		while (1) {
			printf("\npress j to operate jtag interface, a to get gpio status,\n"
			       "g to gpio output test q to quit.\n");

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
			case 'a':
				printf("GPIO Input Test Begin.\n");
				ch34x_demo_gpio_input_operate();
				break;
			case 'g':
				printf("GPIO Output Test begin.\n");
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
