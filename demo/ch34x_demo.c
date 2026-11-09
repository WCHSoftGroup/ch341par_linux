/*
 * Application demo for CH341/CH346/CH347/CH339/CH9341/CH9346 and WCH USB to UART chips.
 *
 * Copyright (C) 2026 Nanjing Qinheng Microelectronics Co., Ltd.
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
 * V1.1 - optimize the program so that a single demo supports all chip models
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
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>

#include "ch34x_lib.h"
#include "flash_xx25.h"
#include "eeprom_xx24.h"

static CH34X_DEVICE_INFO gCH34XDevice[CH34X_MAX_NUMBER];
static uint8_t NumDevs;
static volatile sig_atomic_t gExitRequested;
static volatile sig_atomic_t gIRQPending;
static bool gGPIOIRQEnabled;

/* Worker threads inherit blocked SIGINT; main handles Ctrl+C and cleanup. */
static int CH34X_Demo_CreateThread(pthread_t *id, void *(*entry)(void *),
				   void *arg)
{
	sigset_t set, old;
	int ret;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	ret = pthread_sigmask(SIG_BLOCK, &set, &old);
	if (ret)
		return ret;
	ret = pthread_create(id, NULL, entry, arg);
	pthread_sigmask(SIG_SETMASK, &old, NULL);
	return ret;
}

/*****************************************************************/
/****************** Device SPI Master Test APIs ******************/
/*****************************************************************/

static int CH34X_Demo_SPI_Master_Init(int fd)
{
	int retval;
	mSPI_Master_Cfg mSPIMasterCfg = { 0 };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
		mSPIMasterCfg.SPI_Clock =
			6 *
			1000000; /* SPI clock frequency setting: 6MHz */
		mSPIMasterCfg.SPI_OutDefaultData =
			0xFF; /* SPI master default transmit bytes */
		mSPIMasterCfg.SPI_Mode =
			0; /* SPI mode configuration: MODE0 */
		mSPIMasterCfg.SPI_CS0Init =
			1; /* SPI chip select SCS0: initialize to default polarity */
		mSPIMasterCfg.SPI_CS1Init =
			0; /* SPI chip select SCS1: do not initialize  */
		mSPIMasterCfg.SPI_DataBits =
			0; /* SPI data bits setting: 8-bit */
		mSPIMasterCfg.SPI_BitOrder =
			0; /* SPI byte transmission bit order setting: MSB */
		mSPIMasterCfg.SPI_CS0Polar =
			0; /* SPI chip select SCS0 polarity setting: active low */
		mSPIMasterCfg.SPI_CS1Polar =
			0; /* SPI chip select SCS1 polarity setting: active low */
		mSPIMasterCfg.SPI_DataToDataDelay =
			0; /* SPI communication delay setting between each two bytes: 0*/
		mSPIMasterCfg.SPI_CsToDataDelay =
			0; /* SPI communication delay setting between chip select activate with data start: 0*/
		mSPIMasterCfg.SPI_DataToCsDelay =
			0; /* SPI communication delay setting between data transmit over with chip select deactivate: 0*/
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH339W_VENDOR) {
		mSPIMasterCfg.SPI_Clock =
			6 *
			1000000; /* SPI clock frequency setting: 6MHz */
		mSPIMasterCfg.SPI_OutDefaultData =
			0xFF; /* SPI master default transmit bytes */
		mSPIMasterCfg.SPI_Mode =
			0; /* SPI mode configuration: MODE0 */
		mSPIMasterCfg.SPI_CS0Init =
			1; /* SPI chip select SCS0: initialize to default polarity */
		mSPIMasterCfg.SPI_BitOrder =
			0; /* SPI byte transmission bit order setting: MSB */
		mSPIMasterCfg.SPI_CS0Polar =
			0; /* SPI chip select SCS0 polarity setting: active low */
		mSPIMasterCfg.SPI_DataToDataDelay =
			0; /* SPI communication delay setting between each two bytes: 0*/
		mSPIMasterCfg.SPI_CsToDataDelay =
			0; /* SPI communication delay setting between chip select activate with data start: 0*/
		mSPIMasterCfg.SPI_DataToCsDelay =
			0; /* SPI communication delay setting between data transmit over with chip select deactivate: 0*/
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		mSPIMasterCfg.SPI_Clock =
			6 *
			1000000; /* SPI clock frequency setting: 6MHz */
		mSPIMasterCfg.SPI_OutDefaultData =
			0xFF; /* SPI master default transmit bytes */
		mSPIMasterCfg.SPI_Mode =
			0; /* SPI mode configuration: MODE0 */
		mSPIMasterCfg.SPI_CS0Init =
			1; /* SPI chip select SCS0: initialize to default polarity */
		mSPIMasterCfg.SPI_CS1Init =
			0; /* SPI chip select SCS1: do not initialize  */
		mSPIMasterCfg.SPI_CS2Init =
			0; /* SPI chip select SCS2: do not initialize  */
		mSPIMasterCfg.SPI_CS3Init =
			0; /* SPI chip select SCS3: do not initialize  */
		mSPIMasterCfg.SPI_DataBits =
			0; /* SPI data bits setting: 8-bit */
		mSPIMasterCfg.SPI_BitOrder =
			0; /* SPI byte transmission bit order setting: MSB */
		mSPIMasterCfg.SPI_CS0Polar =
			0; /* SPI chip select SCS0 polarity setting: active low */
		mSPIMasterCfg.SPI_CS1Polar =
			0; /* SPI chip select SCS1 polarity setting: active low */
		mSPIMasterCfg.SPI_CS2Polar =
			0; /* SPI chip select SCS2 polarity setting: active low */
		mSPIMasterCfg.SPI_CS3Polar =
			0; /* SPI chip select SCS3 polarity setting: active low */
		mSPIMasterCfg.SPI_DataToDataDelay =
			0; /* SPI communication delay setting between each two bytes: 0*/
		mSPIMasterCfg.SPI_CsToDataDelay =
			0; /* SPI communication delay setting between chip select activate with data start: 0*/
		mSPIMasterCfg.SPI_DataToCsDelay =
			0; /* SPI communication delay setting between data transmit over with chip select deactivate: 0*/
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH341_VENDOR) {
		mSPIMasterCfg.SPI_Mode =
			3; /* SPI mode configuration: MODE3 */
		mSPIMasterCfg.SPI_BitOrder =
			0; /* SPI byte transmission bit order setting: MSB */
	} else {
		return -ERR_INCOMPATIBLE;
	}

	/* Initialize SPI master controller */
	retval = CH34X_SPIMasterInit(fd, &mSPIMasterCfg);
	if (retval) {
		printf("CH34X_SPIMasterInit Error, Code: %d.\n", retval);
		goto exit;
	}

	retval = CH34X_GetInfo_SPIMaster(fd, &mSPIMasterCfg);
	if (retval) {
		printf("CH34X_GetInfo_SPIMaster Error, Code: %d.\n",
		       retval);
		goto exit;
	}

	/* List SPI master controller configuration */
	printf("\nDump SPI master controller configuration:\n");
	if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
		printf("mSPIMasterCfg.SPI_Clock: %d\n",
		       mSPIMasterCfg.SPI_Clock);
		printf("mSPIMasterCfg.SPI_OutDefaultData: 0x%x\n",
		       mSPIMasterCfg.SPI_OutDefaultData);
		printf("mSPIMasterCfg.SPI_Mode: %d\n",
		       mSPIMasterCfg.SPI_Mode);
		printf("mSPIMasterCfg.SPI_DataBits: %d\n",
		       mSPIMasterCfg.SPI_DataBits);
		printf("mSPIMasterCfg.SPI_BitOrder: %d\n",
		       mSPIMasterCfg.SPI_BitOrder);
		printf("mSPIMasterCfg.SPI_CS0Init: %d\n",
		       mSPIMasterCfg.SPI_CS0Init);
		printf("mSPIMasterCfg.SPI_CS1Init: %d\n",
		       mSPIMasterCfg.SPI_CS1Init);
		printf("mSPIMasterCfg.SPI_CS0Polar: %d\n",
		       mSPIMasterCfg.SPI_CS0Polar);
		printf("mSPIMasterCfg.SPI_CS1Polar: %d\n",
		       mSPIMasterCfg.SPI_CS1Polar);
		printf("mSPIMasterCfg.SPI_DataToDataDelay: %d\n",
		       mSPIMasterCfg.SPI_DataToDataDelay);
		printf("mSPIMasterCfg.SPI_CsToDataDelay: %d\n",
		       mSPIMasterCfg.SPI_CsToDataDelay);
		printf("mSPIMasterCfg.SPI_DataToCsDelay: %d\n",
		       mSPIMasterCfg.SPI_DataToCsDelay);
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH339W_VENDOR) {
		printf("mSPIMasterCfg.SPI_Clock: %d\n",
		       mSPIMasterCfg.SPI_Clock);
		printf("mSPIMasterCfg.SPI_OutDefaultData: 0x%x\n",
		       mSPIMasterCfg.SPI_OutDefaultData);
		printf("mSPIMasterCfg.SPI_Mode: %d\n",
		       mSPIMasterCfg.SPI_Mode);
		printf("mSPIMasterCfg.SPI_BitOrder: %d\n",
		       mSPIMasterCfg.SPI_BitOrder);
		printf("mSPIMasterCfg.SPI_CS0Init: %d\n",
		       mSPIMasterCfg.SPI_CS0Init);
		printf("mSPIMasterCfg.SPI_CS0Polar: %d\n",
		       mSPIMasterCfg.SPI_CS0Polar);
		printf("mSPIMasterCfg.SPI_DataToDataDelay: %d\n",
		       mSPIMasterCfg.SPI_DataToDataDelay);
		printf("mSPIMasterCfg.SPI_CsToDataDelay: %d\n",
		       mSPIMasterCfg.SPI_CsToDataDelay);
		printf("mSPIMasterCfg.SPI_DataToCsDelay: %d\n",
		       mSPIMasterCfg.SPI_DataToCsDelay);
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		printf("mSPIMasterCfg.SPI_Clock: %d\n",
		       mSPIMasterCfg.SPI_Clock);
		printf("mSPIMasterCfg.SPI_OutDefaultData: 0x%x\n",
		       mSPIMasterCfg.SPI_OutDefaultData);
		printf("mSPIMasterCfg.SPI_Mode: %d\n",
		       mSPIMasterCfg.SPI_Mode);
		printf("mSPIMasterCfg.SPI_DataBits: %d\n",
		       mSPIMasterCfg.SPI_DataBits);
		printf("mSPIMasterCfg.SPI_BitOrder: %d\n",
		       mSPIMasterCfg.SPI_BitOrder);
		printf("mSPIMasterCfg.SPI_CS0Init: %d\n",
		       mSPIMasterCfg.SPI_CS0Init);
		printf("mSPIMasterCfg.SPI_CS1Init: %d\n",
		       mSPIMasterCfg.SPI_CS1Init);
		printf("mSPIMasterCfg.SPI_CS2Init: %d\n",
		       mSPIMasterCfg.SPI_CS2Init);
		printf("mSPIMasterCfg.SPI_CS3Init: %d\n",
		       mSPIMasterCfg.SPI_CS3Init);
		printf("mSPIMasterCfg.SPI_CS0Polar: %d\n",
		       mSPIMasterCfg.SPI_CS0Polar);
		printf("mSPIMasterCfg.SPI_CS1Polar: %d\n",
		       mSPIMasterCfg.SPI_CS1Polar);
		printf("mSPIMasterCfg.SPI_CS2Polar: %d\n",
		       mSPIMasterCfg.SPI_CS2Polar);
		printf("mSPIMasterCfg.SPI_CS3Polar: %d\n",
		       mSPIMasterCfg.SPI_CS3Polar);
		printf("mSPIMasterCfg.SPI_DataToDataDelay: %d\n",
		       mSPIMasterCfg.SPI_DataToDataDelay);
		printf("mSPIMasterCfg.SPI_CsToDataDelay: %d\n",
		       mSPIMasterCfg.SPI_CsToDataDelay);
		printf("mSPIMasterCfg.SPI_DataToCsDelay: %d\n",
		       mSPIMasterCfg.SPI_DataToCsDelay);
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH341_VENDOR) {
		printf("mSPIMasterCfg.SPI_Mode: %d\n",
		       mSPIMasterCfg.SPI_Mode);
		printf("mSPIMasterCfg.SPI_BitOrder: %d\n",
		       mSPIMasterCfg.SPI_BitOrder);
	} else {
		return -ERR_INCOMPATIBLE;
	}

	/* Setting CS0 as default CS */
	retval = CH34X_SPISetDefaultCS(fd, 0);
	if (retval) {
		printf("CH34X_SPISetDefaultCS Error, Code: %d.\n", retval);
		goto exit;
	}

exit:
	return retval;
}

static int CH34X_Demo_FLASH_Block_Read_Test(int fd)
{
	int retval;
	uint32_t oLength, iAddress, i;
	uint8_t oBuffer[4096] = { 0 };
	char FmtStr1[8 * 1024 * 3 + 16] = "";

	iAddress = 0x00;
	oLength = 0x500;

	retval = FLASH_Block_Read(fd, iAddress, oBuffer, oLength);
	if (retval) {
		printf("FLASH_Block_Read Error, Code = %d.\n", retval);
		return retval;
	}

	for (i = 0; i < oLength; i++)
		sprintf(&FmtStr1[strlen(FmtStr1)], "%02x ", oBuffer[i]);
	printf("\nRead: \n%s\n\n", FmtStr1);

	return ERR_SUCCESS;
}

static int CH34X_Demo_FLASH_Block_Write_Test(int fd)
{
	int retval;
	uint32_t i = 0;
	uint32_t iLength, iAddress, NumOfPage, NumOfSingle;
	uint8_t iBuffer[0x500] = { 0 };
	uint32_t iWriteIndex = 0;

	/* Write FLASH from addr 0 */
	iAddress = 0x00;
	iLength = 0x500;

	for (i = 0; i < iLength; i++)
		iBuffer[i] = i;

	NumOfPage = iLength / FLASH_PAGE;
	NumOfSingle = iLength % FLASH_PAGE;

	while (NumOfPage--) {
		retval = FLASH_Write_Page(fd, iBuffer + iWriteIndex,
					  iAddress, FLASH_PAGE);
		if (retval)
			goto exit;
		iWriteIndex += FLASH_PAGE;
		iAddress += FLASH_PAGE;
	}
	if (NumOfSingle) {
		retval = FLASH_Write_Page(fd, iBuffer + iWriteIndex,
					  iAddress, NumOfSingle);
		if (retval)
			goto exit;
	}
	return ERR_SUCCESS;

exit:
	printf("FLASH_Write_Page Error, Code = %d.\n", retval);
	return retval;
}

static int CH34X_Demo_SPI_Master_FLASH_Test(int fd)
{
	int retval;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH347T_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH347F_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH341_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	retval = CH34X_Demo_SPI_Master_Init(fd);
	if (retval) {
		printf("CH34X_Demo_SPI_Master_Init Error, Code = %d.\n",
		       retval);
		goto exit;
	}
	printf("CH34X SPI interface initialize succeed.\n");

	/* Read FLASH ID */
	retval = FLASH_ID_Read(fd);
	if (retval) {
		printf("FLASH_ID_Read Error, Code = %d.\n", retval);
		goto exit;
	}

	/* Read FLASH block data */
	retval = CH34X_Demo_FLASH_Block_Read_Test(fd);
	if (retval) {
		printf("CH34X_Demo_FLASH_Block_Read_Test Error, Code = %d.\n",
		       retval);
		goto exit;
	}

	/* Erase FLASH sector data */
	retval = FLASH_Sector_Erase(fd, 0x00);
	if (retval) {
		printf("FLASH_Sector_Erase Error, Code = %d.\n", retval);
		goto exit;
	}
	printf("Erase one sector from Addr[0x%x] of FLASH succeed.\n",
	       0x00);

	/* Write FLASH by block */
	retval = CH34X_Demo_FLASH_Block_Write_Test(fd);
	if (retval) {
		printf("CH34X_Demo_FLASH_Block_Write_Test Error, Code = %d.\n",
		       retval);
		goto exit;
	}

	/* Read FLASH by block */
	retval = CH34X_Demo_FLASH_Block_Read_Test(fd);
	if (retval) {
		printf("CH34X_Demo_FLASH_Block_Write_Test Error, Code = %d.\n",
		       retval);
		goto exit;
	}

	return ERR_SUCCESS;
exit:
	return retval;
}

/*****************************************************************/
/********************* Device I2C Test APIs **********************/
/*****************************************************************/

static int CH34X_Demo_I2C_Master_Init(int fd)
{
	int retval;
	mI2C_Master_Cfg I2CMasterCfg = { 0 };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH339W_VENDOR) {
		I2CMasterCfg.I2C_Clock = 750 * 1000; /* I2C clock */
		/* The following parameters are only supported by CH347T */
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			I2CMasterCfg.I2C_DriverMode =
				0; /* Setting I2C signal drive mode to open-drain */
			I2CMasterCfg.I2C_Continue_OnNACK =
				0; /* Setting I2C transmission to stop when a device NACK is encountered */
			I2CMasterCfg.I2C_WriteDelay_9thClock =
				0; /* Setting I2C write delay between the 8th clock edge and the 9th clock edge */
			I2CMasterCfg.I2C_ReadDelay_9thClock =
				0; /* Setting I2C read delay between the 8th clock edge and the 9th clock edge */
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		I2CMasterCfg.I2C_Clock = 750 * 1000; /* I2C clock */
		I2CMasterCfg.I2C_Stretch_Timeout =
			10; /* Setting I2C clock stretching SCL detection timeout to 1ms */
		/* Valid when I2C clock > 100KHz */
		I2CMasterCfg.I2C_DutyCycle =
			1; /* Setting I2C dutycycle to Tlow:Thigh = 2:1 */
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH341_VENDOR) {
		I2CMasterCfg.I2C_Clock = 400 * 1000; /* I2C clock */
	} else {
		return -ERR_INCOMPATIBLE;
	}

	/* Initialize I2C interface */
	retval = CH34X_I2CMasterInit(fd, &I2CMasterCfg);
	if (retval) {
		printf("CH34X_I2CMasterInit Error, Code: %d.\n", retval);
		goto exit;
	} else {
		printf("I2C Master Initialize successful.\n");
	}

	retval = CH34X_GetInfo_I2CMaster(fd, &I2CMasterCfg);
	if (retval) {
		printf("CH34X_GetInfo_I2CMaster Error, Code: %d.\n",
		       retval);
		goto exit;
	}

	/* List I2C configuration */
	printf("\nDump I2C master configuration:\n");
	if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH339W_VENDOR) {
		printf("I2CMasterCfg.I2C_Clock: %d\n",
		       I2CMasterCfg.I2C_Clock);
		/* The following parameters are only supported by CH347T */
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			printf("I2CMasterCfg.I2C_DriverMode: %d\n",
			       I2CMasterCfg.I2C_DriverMode);
			printf("I2CMasterCfg.I2C_Continue_OnNACK: %d\n",
			       I2CMasterCfg.I2C_Continue_OnNACK);
			printf("I2CMasterCfg.I2C_WriteDelay_9thClock: %d\n",
			       I2CMasterCfg.I2C_WriteDelay_9thClock);
			printf("I2CMasterCfg.I2C_ReadDelay_9thClock: %d\n",
			       I2CMasterCfg.I2C_ReadDelay_9thClock);
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		printf("I2CMasterCfg.I2C_Clock: %d\n",
		       I2CMasterCfg.I2C_Clock);
		printf("I2CMasterCfg.I2C_Stretch_Timeout: %d\n",
		       I2CMasterCfg.I2C_Stretch_Timeout);
		printf("I2CMasterCfg.I2C_DutyCycle: %d\n",
		       I2CMasterCfg.I2C_DutyCycle);
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH341_VENDOR) {
		printf("I2CMasterCfg.I2C_Clock: %d\n",
		       I2CMasterCfg.I2C_Clock);
	} else {
		return -ERR_INCOMPATIBLE;
	}

exit:
	return retval;
}

static int CH34X_Demo_I2C_Master_EEPROM_Read(int fd)
{
	uint32_t i;
	int retval;
	EEPROM_TYPE type;
	uint32_t iAddress;
	uint32_t oReadLength;
	uint8_t oBuffer[256] = { 0 };

	type = ID_24C02;
	iAddress = 0;
	oReadLength = 256;

	retval =
		CH34X_ReadEEPROM(fd, type, iAddress, oBuffer, oReadLength);
	if (retval)
		goto exit;

	printf("\nRead EEPROM data:\n");
	for (i = 0; (uint32_t)i < oReadLength; i++) {
		printf("%02x ", oBuffer[i]);
		if (((i + 1) % 10) == 0)
			putchar(10);
	}
	putchar(10);

exit:
	return retval;
}

static int CH34X_Demo_I2C_Master_EEPROM_Write(int fd)
{
	uint32_t i;
	int retval;
	EEPROM_TYPE type;
	uint32_t iAddress;
	uint32_t iWriteLength;
	uint8_t iWriteBuffer[256] = { 0 };

	type = ID_24C02;
	iAddress = 0;
	iWriteLength = 256;
	for (i = 0; i < 256; i++)
		iWriteBuffer[i] = i;

	retval = CH34X_WriteEEPROM(fd, type, iAddress, iWriteBuffer,
				   iWriteLength);
	if (retval)
		goto exit;

	printf("\nWrite EEPROM data:\n");
	for (i = 0; i < iWriteLength; i++) {
		printf("%02x ", iWriteBuffer[i]);
		if (((i + 1) % 10) == 0)
			putchar(10);
	}
	putchar(10);

exit:
	return retval;
}

static int CH34X_Demo_I2C_Master_EEPROM_Test(int fd)
{
	int retval;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH347T_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH347F_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH341_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	retval = CH34X_Demo_I2C_Master_Init(fd);
	if (retval) {
		printf("CH34X_Demo_I2C_Master_Init Error, Code = %d.\n",
		       retval);
		goto exit;
	}
	printf("CH34X I2C interface initialize succeed.\n");

	retval = CH34X_Demo_I2C_Master_EEPROM_Read(fd);
	if (retval) {
		printf("CH34X_Demo_I2C_Master_EEPROM_Read Error, Code = %d.\n",
		       retval);
		goto exit;
	}

	retval = CH34X_Demo_I2C_Master_EEPROM_Write(fd);
	if (retval) {
		printf("CH34X_Demo_I2C_Master_EEPROM_Write Error, Code = %d.\n",
		       retval);
		goto exit;
	}

	retval = CH34X_Demo_I2C_Master_EEPROM_Read(fd);
	if (retval) {
		printf("CH34X_Demo_I2C_Master_EEPROM_Read Error, Code = %d.\n",
		       retval);
		goto exit;
	}
exit:
	return retval;
}

/*****************************************************************/
/************** Device SPI/I2C/FIFO Slave Test APIs **************/
/*****************************************************************/

/*
 * Note: The following test logic for SPI/I2C/FIFO slave devices is as follows:
 * Data write and data read threads are enabled separately. The write thread writes one packet (1024 bytes) of data each time,
 * containing a 4-byte packet sequence number plus a loop of data from 0x00 to 0xFF. The write speed is printed every time 1MiB is written.
 * The read thread continuously queries the received data volume, reading it all at once when 1MiB is received.
 * It then parses the data in 1024-byte packets, first checking the packet sequence number and then verifying if the data content matches the sent data.
 * If it fails, it exits early; otherwise, it prints the read speed and continues execution.
 * 
 * Taking the SPI interface as an example, after USB writing, the data is buffered inside the chip, waiting for the SPI master to read it.
 * The SPI master should then send the received data back to the chip intact, i.e., through SPI data writing.
 * The complete SPI master operation is as follows:
 * continuously query the effective data length of the chip's SPI transmit buffer;
 * when data is available,it reads the data and sends it back when the chip's receive buffer is not full.
*/

struct _SlaveThreadArg {
	int fd;
	int interface;
} __attribute__((packed));

typedef enum _INTF_TYPE {
	INTF_SPI = 0,
	INTF_I2C = 1,
	INTF_FIFO = 2,
} INTF_TYPE;

#define MB (1 * 1024 * 1024)
#define PACKLEN 1024
#define DATACOMPARE
#undef DATACOMPARE

static pthread_t sSPIThreadId, rSPIThreadId;
static bool sSPIThreadStarted, rSPIThreadStarted;
static atomic_bool sSPIThreadLoop, rSPIThreadLoop;
static bool bSPISlaveEnable;

static pthread_t sI2CThreadId, rI2CThreadId;
static bool sI2CThreadStarted, rI2CThreadStarted;
static atomic_bool sI2CThreadLoop, rI2CThreadLoop;
static bool bI2CSlaveEnable;

static pthread_t sFIFOThreadId, rFIFOThreadId;
static bool sFIFOThreadStarted, rFIFOThreadStarted;
static atomic_bool sFIFOThreadLoop, rFIFOThreadLoop;
static bool bFIFOSlaveEnable;

static void CH34X_Demo_StopThreads(int interface)
{
	pthread_t send_id, read_id;
	bool *send_started, *read_started;
	if (interface == INTF_SPI) {
		sSPIThreadLoop = false;
		rSPIThreadLoop = false;
		send_id = sSPIThreadId;
		read_id = rSPIThreadId;
		send_started = &sSPIThreadStarted;
		read_started = &rSPIThreadStarted;
	} else if (interface == INTF_I2C) {
		sI2CThreadLoop = false;
		rI2CThreadLoop = false;
		send_id = sI2CThreadId;
		read_id = rI2CThreadId;
		send_started = &sI2CThreadStarted;
		read_started = &rI2CThreadStarted;
	} else {
		sFIFOThreadLoop = false;
		rFIFOThreadLoop = false;
		send_id = sFIFOThreadId;
		read_id = rFIFOThreadId;
		send_started = &sFIFOThreadStarted;
		read_started = &rFIFOThreadStarted;
	}
	if (*send_started) {
		pthread_join(send_id, NULL);
		*send_started = false;
	}
	if (*read_started) {
		pthread_join(read_id, NULL);
		*read_started = false;
	}
}

/**
 * @brief Slave query read function type definition
 * 
 * @param fd File descriptor of device.
 * @param pReadLength Pointer to number of bytes of data cached in the driver, valid length: 0~1MiB.
 *
 * @return 0 if successful, negative number if fail.
 */
typedef int (*pCH_SlaveQuery_ROUTINE)(int fd, uint32_t *pReadLength);

/**
 * @brief Slave read function type definition
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than *pReadLength bytes.
 * @param pReadLength Pointer to number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @return 0 if successful, negative number if fail.
 */
typedef int (*pCH_SlaveRead_ROUTINE)(int fd, void *oReadBuffer,
				     uint32_t *pReadLength);

static void *CH34X_Demo_Slave_Read(void *arg)
{
	int retval;
	uint8_t oBuffer[SLAVE_MAX_LENGTH];
	uint8_t cBuffer[PACKLEN];
	uint32_t oLength;
	uint64_t oReadIndex = 0;
	struct timeval t1, t2;
	int delta_sec = 0;
	int delta_usec = 0;
	float speed_read = 0.0;
	int MB_Num = 1;
	int i, j, k;
	uint32_t targetIndex = 0;
	uint32_t packIndex, oReadLength;
	time_t time_start = time(NULL), time_end;
	struct _SlaveThreadArg *pArg = (struct _SlaveThreadArg *)arg;
	bool bThreadLoop;
	int iIndex;
	pCH_SlaveRead_ROUTINE threadReadFunc;
	pCH_SlaveQuery_ROUTINE threadQueryFunc;

	iIndex = CH34X_GetDeviceIndex(pArg->fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER) {
		printf("CH34X_GetDeviceIndex Error, Code = %d.\n", iIndex);
		return NULL;
	}

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		if (pArg->interface != INTF_SPI &&
		    pArg->interface != INTF_I2C) {
			printf("CH9341C only supports SPI and I2C slave.\n");
			return NULL;
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH346C_VENDOR) {
		if (pArg->interface != INTF_SPI &&
		    pArg->interface != INTF_FIFO) {
			printf("CH346C only supports SPI and FIFO slave.\n");
			return NULL;
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9346W_VENDOR) {
		if (pArg->interface != INTF_FIFO) {
			printf("CH9346W only supports SPI and FIFO slave.\n");
			return NULL;
		}
	} else {
		printf("This function only supports CH9341C, CH346C and CH9346W.\n");
		return NULL;
	}

	if (pArg->interface == INTF_SPI) {
		threadQueryFunc = CH34X_SPISlaveQueryRead;
		threadReadFunc = CH34X_SPISlaveRead;
		retval = CH34X_SPISlaveControl(pArg->fd, true);
		if (retval) {
			printf("CH34X_SPISlaveControl Error, Code = %d.\n",
			       retval);
			return NULL;
		}
		bSPISlaveEnable = true;
		retval = CH34X_SPISlaveClear(pArg->fd);
		if (retval) {
			printf("CH34X_SPISlaveClear Error, Code = %d.\n",
			       retval);
			goto exit;
		}
	} else if (pArg->interface == INTF_I2C) {
		threadQueryFunc = CH34X_I2CSlaveQueryRead;
		threadReadFunc = CH34X_I2CSlaveRead;
		retval = CH34X_I2CSlaveControl(pArg->fd, true);
		if (retval) {
			printf("CH34X_I2CSlaveControl Error, Code = %d.\n",
			       retval);
			return NULL;
		}
		bI2CSlaveEnable = true;
		retval = CH34X_I2CSlaveClear(pArg->fd);
		if (retval) {
			printf("CH34X_I2CSlaveClear Error, Code = %d.\n",
			       retval);
			goto exit;
		}
	} else {
		threadQueryFunc = CH34X_FIFOSlaveQueryRead;
		threadReadFunc = CH34X_FIFOSlaveRead;
		retval = CH34X_FIFOSlaveControl(pArg->fd, true);
		if (retval) {
			printf("CH34X_FIFOSlaveControl Error, Code = %d.\n",
			       retval);
			return NULL;
		}
		bFIFOSlaveEnable = true;
		retval = CH34X_FIFOSlaveClear(pArg->fd);
		if (retval) {
			printf("CH34X_FIFOSlaveClear Error, Code = %d.\n",
			       retval);
			goto exit;
		}
	}

	printf("Begin read slave data from master...\n");

	time_start = time(NULL);

	while (1) {
		if (pArg->interface == INTF_SPI)
			bThreadLoop = rSPIThreadLoop;
		else if (pArg->interface == INTF_I2C)
			bThreadLoop = rI2CThreadLoop;
		else
			bThreadLoop = rFIFOThreadLoop;
		if (!bThreadLoop)
			break;

		oReadIndex = 0;
		gettimeofday(&t1, NULL);
		while (oReadIndex < (uint64_t)MB_Num * MB) {
			if (pArg->interface == INTF_SPI)
				bThreadLoop = rSPIThreadLoop;
			else if (pArg->interface == INTF_I2C)
				bThreadLoop = rI2CThreadLoop;
			else
				bThreadLoop = rFIFOThreadLoop;
			if (!bThreadLoop)
				break;

			retval = threadQueryFunc(pArg->fd, &oLength);
			if (retval) {
				printf("SlaveQueryRead Error, Code = %d.\n",
				       retval);
				goto exit;
			}
			/* Check if the length is large enough */
			if (oLength >= SLAVE_MAX_LENGTH) {
				oReadLength = SLAVE_MAX_LENGTH;
				retval = threadReadFunc(pArg->fd, oBuffer,
							&oReadLength);
				if (retval) {
					printf("SlaveRead(Func=%d) Error, Code=%d.\n",
					       pArg->interface, retval);
					goto exit;
				}

				if (oReadLength != SLAVE_MAX_LENGTH) {
					printf("Unexpected read length: %u; expected complete %u-byte packets.\n",
					       oReadLength,
					       SLAVE_MAX_LENGTH);
					goto exit;
				}

#ifdef DATACOMPARE
				for (i = 0; (uint32_t)i < oReadLength;
				     i += PACKLEN) {
					/* Every packet, the first 4 bytes are the packet index */
					memcpy(&packIndex, oBuffer + i,
					       sizeof(packIndex));
					if (be32toh(packIndex) !=
					    targetIndex) {
						printf("Bad packet: %d, last packet: %d\n",
						       be32toh(packIndex),
						       targetIndex);
						goto exit;
					}
					/* Check the data in every PACKLEN packet */
					memcpy(cBuffer, oBuffer + i,
					       PACKLEN);
					for (j = 0; j < PACKLEN; j++) {
						if (j < 4)
							continue;
						if (cBuffer[j] !=
						    (uint8_t)j) {
							printf("Error packet: %d\n",
							       targetIndex);
							printf("Data Error, j = %d, buf[j] = 0x%2x, buf[j-1]: 0x%2x\n",
							       j,
							       cBuffer[j],
							       cBuffer[j -
								       1]);
							for (k = 0;
							     k < PACKLEN;
							     k++)
								printf("%.2x ",
								       cBuffer[k]);
							printf("\n\n");
							goto exit;
						}
					}
					targetIndex++;
				}
#endif
				oReadIndex += oReadLength;
			} else {
				/* No data received this time */
				usleep(100);
				continue;
			}
		}
		gettimeofday(&t2, NULL);

		delta_sec = t2.tv_sec - t1.tv_sec;
		delta_usec = t2.tv_usec - t1.tv_usec;

		if (delta_usec < 0) {
			delta_sec--;
			delta_usec = 1000000 + delta_usec;
		}
		speed_read =
			(float)(MB_Num) /
			(float)(delta_sec + (float)delta_usec / 1000000);

		printf("RX %d MB takes: %2d sec %2d us, up speed: %4.2f MB/s\n",
		       MB_Num, delta_sec, delta_usec, speed_read);
	}

exit:
	time_end = time(NULL);
	printf("Stop time: %s\n", ctime(&time_end));
	printf("CH34X_Demo_Slave_Read continues %f secs...\n",
	       difftime(time_end, time_start));

	if (pArg->interface == INTF_SPI) {
		retval = CH34X_SPISlaveControl(pArg->fd, false);
		if (retval) {
			printf("CH34X_SPISlaveControl Error, Code = %d.\n",
			       retval);
			return NULL;
		}
		bSPISlaveEnable = false;
	} else if (pArg->interface == INTF_I2C) {
		retval = CH34X_I2CSlaveControl(pArg->fd, false);
		if (retval) {
			printf("CH34X_I2CSlaveControl Error, Code = %d.\n",
			       retval);
			return NULL;
		}
		bI2CSlaveEnable = false;
	} else if (pArg->interface == INTF_FIFO) {
		retval = CH34X_FIFOSlaveControl(pArg->fd, false);
		if (retval) {
			printf("CH34X_FIFOSlaveControl Error, Code = %d.\n",
			       retval);
			return NULL;
		}
		bFIFOSlaveEnable = false;
	}

	return NULL;
}

/**
 * @brief Slave write function type definition
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be write, valid length: 1~64KiB.
 *
 */
typedef int (*pCH_SlaveWrite_ROUTINE)(int fd, void *iWriteBuffer,
				      uint32_t iWriteLength);

static void *CH34X_Demo_Slave_Write(void *arg)
{
	int retval = ERR_SUCCESS;
	uint8_t iWriteBuffer[PACKLEN];
	uint64_t bytes_write = 0;
	struct timeval t1, t2;
	int delta_sec = 0;
	int delta_usec = 0;
	float speed_write = 0.0;
	int MB_Num = 1;
	int i;
	uint32_t packIndex = 0;
	uint32_t tIndex;
	uint32_t iWriteLength;
	struct _SlaveThreadArg *pArg = (struct _SlaveThreadArg *)arg;
	bool bThreadLoop;
	int iIndex;
	pCH_SlaveWrite_ROUTINE threadWriteFunc;

	iIndex = CH34X_GetDeviceIndex(pArg->fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER) {
		printf("CH34X_GetDeviceIndex Error, Code = %d.\n", iIndex);
		return NULL;
	}

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		if (pArg->interface != INTF_SPI &&
		    pArg->interface != INTF_I2C) {
			printf("CH9341C only supports SPI and I2C slave.\n");
			return NULL;
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH346C_VENDOR) {
		if (pArg->interface != INTF_SPI &&
		    pArg->interface != INTF_FIFO) {
			printf("CH346C only supports SPI and FIFO slave.\n");
			return NULL;
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9346W_VENDOR) {
		if (pArg->interface != INTF_FIFO) {
			printf("CH9346W only supports FIFO slave.\n");
			return NULL;
		}
	} else {
		printf("This function only supports CH9341C, CH346C and CH9346W.\n");
		return NULL;
	}

	sleep(2);

	printf("Begin write slave data to chip...\n");

	for (i = 0; i < PACKLEN; i++)
		iWriteBuffer[i] = i;

	iWriteLength = PACKLEN;

	if (pArg->interface == INTF_SPI) {
		threadWriteFunc = CH34X_SPISlaveWrite;
	} else if (pArg->interface == INTF_I2C) {
		threadWriteFunc = CH34X_I2CSlaveWrite;
	} else {
		threadWriteFunc = CH34X_FIFOSlaveWrite;
	}

	while (1) {
		if (pArg->interface == INTF_SPI)
			bThreadLoop = sSPIThreadLoop;
		else if (pArg->interface == INTF_I2C)
			bThreadLoop = sI2CThreadLoop;
		else
			bThreadLoop = sFIFOThreadLoop;
		if (!bThreadLoop)
			break;

		bytes_write = 0;
		gettimeofday(&t1, NULL);
		while (bytes_write < (uint64_t)MB_Num * MB) {
			/* Every packet, the first 4 bytes are the packet index */
			tIndex = htobe32(packIndex);
			memcpy(iWriteBuffer, &tIndex, 4);
			packIndex++;
retry:
			if (pArg->interface == INTF_SPI)
				bThreadLoop = sSPIThreadLoop;
			else if (pArg->interface == INTF_I2C)
				bThreadLoop = sI2CThreadLoop;
			else
				bThreadLoop = sFIFOThreadLoop;
			if (!bThreadLoop)
				break;

			retval = threadWriteFunc(pArg->fd, iWriteBuffer,
						 iWriteLength);
			if (retval) {
				printf("SlaveWrite(Func=%d) Error, Code=%d.\n",
				       pArg->interface, retval);
				if (retval == -ERR_USB_TIMEOUT) {
					sleep(1);
					goto retry;
				} else
					goto exit;
			}
			bytes_write += iWriteLength;
		}
		gettimeofday(&t2, NULL);

		delta_sec = t2.tv_sec - t1.tv_sec;
		delta_usec = t2.tv_usec - t1.tv_usec;

		if (delta_usec < 0) {
			delta_sec--;
			delta_usec = 1000000 + delta_usec;
		}
		speed_write =
			(float)(MB_Num) /
			(float)(delta_sec + (float)delta_usec / 1000000);

		if (retval == 0)
			printf("TX %d MB takes: %2d sec %2d us, up speed: %4.2f MB/s\n",
			       MB_Num, delta_sec, delta_usec, speed_write);
	}

exit:
	printf("Pthread Stop write data to chip...\n");

	return NULL;
}

static int CH34X_Demo_SPI_Slave_Init(int fd)
{
	int retval;
	mSPI_Slave_Cfg mSPISlaveCfg = { 0 };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH346C_VENDOR) {
		printf("This function only supports CH9341C and CH346C.\n");
		return -ERR_INCOMPATIBLE;
	}

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH346C_VENDOR) {
		/* Initialize SPI interface */
		retval = CH34X_SPISlaveInit(fd, NULL);
		if (retval) {
			printf("CH34X_SPISlaveInit Error, Code: %d.\n",
			       retval);
			goto exit;
		} else {
			printf("SPI Slave Initialize successful.\n");
		}
		retval = CH34X_GetInfo_SPISlave(fd, &mSPISlaveCfg);
		if (retval) {
			printf("CH34X_GetInfo_SPISlave Error, Code: %d.\n",
			       retval);
			goto exit;
		}
	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		/* Setting here */
		mSPISlaveCfg.SPI_DataBits = 0;
		mSPISlaveCfg.SPI_BitOrder = 0;
		mSPISlaveCfg.SPI_CS0Polar = 0;
		mSPISlaveCfg.SPI_Slave_Read_TimeOut = 10;

		/* Initialize SPI interface */
		retval = CH34X_SPISlaveInit(fd, &mSPISlaveCfg);
		if (retval) {
			printf("CH34X_SPISlaveInit Error, Code: %d.\n",
			       retval);
			goto exit;
		} else {
			printf("SPI Slave Initialize successful.\n");
		}

		retval = CH34X_GetInfo_SPISlave(fd, &mSPISlaveCfg);
		if (retval) {
			printf("CH34X_GetInfo_SPISlave Error, Code: %d.\n",
			       retval);
			goto exit;
		}

		/* List SPI configuration */
		printf("\nDump SPI Slave Configuration:\n");
		printf("mSPISlaveCfg.SPI_DataBits: %d\n",
		       mSPISlaveCfg.SPI_DataBits);
		printf("mSPISlaveCfg.SPI_BitOrder: %d\n",
		       mSPISlaveCfg.SPI_BitOrder);
		printf("mSPISlaveCfg.SPI_CS0Polar: %d\n",
		       mSPISlaveCfg.SPI_CS0Polar);
		printf("mSPISlaveCfg.SPI_Slave_Read_TimeOut: %d\n",
		       mSPISlaveCfg.SPI_Slave_Read_TimeOut);
	}
exit:
	return retval;
}

static int CH34X_Demo_SPI_Slave_Test(int fd)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	struct _SlaveThreadArg thread_arg = { .fd = fd,
					      .interface = INTF_SPI };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH346C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	while (!gExitRequested) {
		printf("\nPress i to initialize SPI slave, press w to write and read SPI data, r to read SPI data from SPI master to chip,\n"
		       "\tq to quit.\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;

		switch (choice) {
		case 'i':
			retval = CH34X_Demo_SPI_Slave_Init(fd);
			if (retval) {
				printf("CH34X_Demo_SPI_Slave_Init Error, Code = %d.\n",
				       retval);
			}
			break;
		case 'w':
			if (sSPIThreadStarted) {
				printf("Thread already started; return with q before restarting.\n");
				break;
			}
			sSPIThreadLoop = true;
			retval = CH34X_Demo_CreateThread(
				&sSPIThreadId, CH34X_Demo_Slave_Write,
				&thread_arg);
			if (retval) {
				printf("pthread-write create error.\n");
				goto exit;
			}
			sSPIThreadStarted = true;
			break;
		case 'r':
			if (rSPIThreadStarted) {
				printf("Thread already started; return with q before restarting.\n");
				break;
			}
			rSPIThreadLoop = true;
			retval = CH34X_Demo_CreateThread(
				&rSPIThreadId, CH34X_Demo_Slave_Read,
				&thread_arg);
			if (retval) {
				printf("pthread-read create error.\n");
				goto exit;
			}
			rSPIThreadStarted = true;
			break;
		default:
			break;
		}
	}

exit:
	CH34X_Demo_StopThreads(INTF_SPI);
	return retval;
}

static int CH34X_Demo_I2C_Slave_Init(int fd)
{
	int retval;
	mI2C_Slave_Cfg I2CSlaveCfg = { 0 };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR) {
		printf("This function only supports CH9341C.\n");
		return -ERR_INCOMPATIBLE;
	}

	/* Setting here */
	I2CSlaveCfg.I2C_AddrMode = 0;
	I2CSlaveCfg.I2C_Slave_Addr = 0x20;
	I2CSlaveCfg.I2C_Slave_Reg = 0xc0;
	I2CSlaveCfg.I2C_Slave_Read_TimeOut = 100;

	/* Initialize I2C interface */
	retval = CH34X_I2CSlaveInit(fd, &I2CSlaveCfg);
	if (retval) {
		printf("CH34X_I2CSlaveInit Error, Code: %d.\n", retval);
		goto exit;
	} else {
		printf("I2C Slave Initialize successful.\n");
	}

	retval = CH34X_GetInfo_I2CSlave(fd, &I2CSlaveCfg);
	if (retval) {
		printf("CH34X_GetInfo_I2CSlave Error, Code: %d.\n",
		       retval);
		goto exit;
	}

	/* List I2C configuration */
	printf("\nDump I2C slave configuration:\n");
	printf("I2CSlaveCfg.I2C_AddrMode: %d\n", I2CSlaveCfg.I2C_AddrMode);
	printf("I2CSlaveCfg.I2C_Slave_Addr: 0x%x\n",
	       I2CSlaveCfg.I2C_Slave_Addr);
	printf("I2CSlaveCfg.I2C_Slave_Reg: 0x%x\n",
	       I2CSlaveCfg.I2C_Slave_Reg);
	printf("I2CSlaveCfg.I2C_Slave_Read_TimeOut: %d\n",
	       I2CSlaveCfg.I2C_Slave_Read_TimeOut);

exit:
	return retval;
}

static int CH34X_Demo_I2C_Slave_Test(int fd)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	struct _SlaveThreadArg thread_arg = { .fd = fd,
					      .interface = INTF_I2C };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	while (!gExitRequested) {
		printf("\nPress i to initialize I2C slave, press w to write and read I2C data, r to read I2C data from I2C master to chip,\n"
		       "\tq to quit.\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;

		switch (choice) {
		case 'i':
			retval = CH34X_Demo_I2C_Slave_Init(fd);
			if (retval) {
				printf("CH34X_Demo_I2C_Slave_Init Error, Code = %d.\n",
				       retval);
			}
			break;
		case 'w':
			if (sI2CThreadStarted) {
				printf("Thread already started; return with q before restarting.\n");
				break;
			}
			sI2CThreadLoop = true;
			retval = CH34X_Demo_CreateThread(
				&sI2CThreadId, CH34X_Demo_Slave_Write,
				&thread_arg);
			if (retval) {
				printf("pthread-write create error.\n");
				goto exit;
			}
			sI2CThreadStarted = true;
			break;
		case 'r':
			if (rI2CThreadStarted) {
				printf("Thread already started; return with q before restarting.\n");
				break;
			}
			rI2CThreadLoop = true;
			retval = CH34X_Demo_CreateThread(
				&rI2CThreadId, CH34X_Demo_Slave_Read,
				&thread_arg);
			if (retval) {
				printf("pthread-read create error.\n");
				goto exit;
			}
			rI2CThreadStarted = true;
			break;
		default:
			break;
		}
	}

exit:
	CH34X_Demo_StopThreads(INTF_I2C);
	return retval;
}

static int CH34X_Demo_FIFO_Slave_Init(int fd)
{
	int retval;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH346C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH9346W_VENDOR) {
		printf("This function only supports CH346C and CH9346W.\n");
		return -ERR_INCOMPATIBLE;
	}

	/* Initialize FIFO interface */
	retval = CH34X_FIFOSlaveInit(fd);
	if (retval) {
		printf("CH34X_FIFOSlaveInit Error, Code: %d.\n", retval);
		goto exit;
	} else {
		printf("FIFO Slave Initialize successful.\n");
	}

exit:
	return retval;
}

static int CH34X_Demo_FIFO_Slave_Test(int fd)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	struct _SlaveThreadArg thread_arg = { .fd = fd,
					      .interface = INTF_FIFO };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH346C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH9346W_VENDOR) {
		printf("This function only supports CH346C and CH9346W.\n");
		return -ERR_INCOMPATIBLE;
	}

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH346C_VENDOR) {
		if (gCH34XDevice[iIndex].ChipMode != 0) {
			printf("CH346C chip is in non-FIFO mode, current mode is %d.\n",
			       gCH34XDevice[iIndex].ChipMode);
			printf("\nPress y to initialize to FIFO mode, other key to quit.\n");
			if (scanf("%c", &choice) != 1)
				goto exit;
			while ((ch = getchar()) != EOF && ch != '\n')
				;
			if (choice == 'y') {
				retval = CH34X_SetDeviceMode(fd, 0);
				if (retval) {
					printf("CH34X_SetDeviceMode Error, Code: %d.\n",
					       retval);
					goto exit;
				}
				retval = CH34X_GetDeviceMode(
					fd,
					&gCH34XDevice[iIndex].ChipMode);
				if (retval) {
					printf("CH34X_GetDeviceMode Error, Code: %d.\n",
					       retval);
					goto exit;
				}
				printf("Switch to FIFO mode successful.\n");
			} else {
				return -ERR_INCOMPATIBLE;
			}
		}
	}

	while (!gExitRequested) {
		printf("\nPress i to initialize FIFO slave, press w to write and read FIFO data, r to read FIFO data from FIFO master to chip,\n"
		       "\tq to quit.\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;

		switch (choice) {
		case 'i':
			retval = CH34X_Demo_FIFO_Slave_Init(fd);
			if (retval) {
				printf("CH34X_Demo_FIFO_Slave_Init Error, Code = %d.\n",
				       retval);
			}
			break;
		case 'w':
			if (sFIFOThreadStarted) {
				printf("Thread already started; return with q before restarting.\n");
				break;
			}
			sFIFOThreadLoop = true;
			retval = CH34X_Demo_CreateThread(
				&sFIFOThreadId, CH34X_Demo_Slave_Write,
				&thread_arg);
			if (retval) {
				printf("pthread-write create error.\n");
				goto exit;
			}
			sFIFOThreadStarted = true;
			break;
		case 'r':
			if (rFIFOThreadStarted) {
				printf("Thread already started; return with q before restarting.\n");
				break;
			}
			rFIFOThreadLoop = true;
			retval = CH34X_Demo_CreateThread(
				&rFIFOThreadId, CH34X_Demo_Slave_Read,
				&thread_arg);
			if (retval) {
				printf("pthread-read create error.\n");
				goto exit;
			}
			rFIFOThreadStarted = true;
			break;
		default:
			break;
		}
	}

exit:
	CH34X_Demo_StopThreads(INTF_FIFO);
	return retval;
}

/*****************************************************************/
/********************* Device GPIO Test APIs *********************/
/*****************************************************************/

static int CH34X_Demo_GPIO_Input_Test(int fd)
{
	int retval;
	mGPIO_Cfg *pGPIOCfg = NULL;
	uint64_t iGPIOChannel;
	uint64_t iGpioLevel;
	uint8_t iCh;
	int ch;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	pGPIOCfg = malloc(sizeof(mGPIO_Cfg) *
			  gCH34XDevice[iIndex].MaxGPIOChannel);
	if (pGPIOCfg == NULL)
		return -ERR_MALLOC;
	memset(pGPIOCfg, 0,
	       sizeof(mGPIO_Cfg) * gCH34XDevice[iIndex].MaxGPIOChannel);

	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				0; /* GPIO direction setting: input */
			pGPIOCfg[iCh].GPIO_InterruptEnable =
				0; /* GPIO interrupt function disabled */
			pGPIOCfg[iCh].GPIO_InterruptMode =
				0; /* GPIO interrupt mode configuration: falling edge */
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9341C_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				0; /* GPIO direction setting: input */
			pGPIOCfg[iCh].GPIO_InterruptEnable =
				0; /* GPIO interrupt function disabled */
			pGPIOCfg[iCh].GPIO_InterruptMode =
				0; /* GPIO interrupt mode configuration: falling edge */
			pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode =
				0; /* GPIO input mode setting: floating*/
			pGPIOCfg[iCh].GPIO_FilterDelay =
				0; /* GPIO delay filter time setting: 0 */
			pGPIOCfg[iCh].GPIO_DefaultEnable =
				0; /* GPIO power-down save function setting: ignore */
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9346W_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				0; /* GPIO direction setting: input */
			pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode =
				0; /* GPIO input mode setting: floating*/
			pGPIOCfg[iCh].GPIO_DefaultEnable =
				0; /* GPIO power-down save function setting: ignore */
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH341_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				0; /* GPIO direction setting: input */
		} else {
			retval = -ERR_INCOMPATIBLE;
			goto exit;
		}
	}

	/* Initialize GPIO interface */
	iGPIOChannel =
		(UINT64_C(1) << gCH34XDevice[iIndex].MaxGPIOChannel) - 1;
	retval = CH34X_GPIOInit(fd, iGPIOChannel, pGPIOCfg);
	if (retval) {
		printf("CH34X_GPIOInit Error, Code: %d.\n", retval);
		goto exit;
	}

	retval = CH34X_GetInfo_GPIO(fd, pGPIOCfg);
	if (retval) {
		printf("CH34X_GetInfo_GPIO Error, Code: %d.\n", retval);
		goto exit;
	}

	/* List GPIO configuration */
	printf("\nDump GPIO configuration:\n\n");
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		printf("GPIO-%d Information:\n", iCh);
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
			printf("\tGPIOCfg.GPIO_InterruptEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptEnable);
			printf("\tGPIOCfg.GPIO_InterruptMode: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptMode);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9346W_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
			printf("\tGPIOCfg.GPIO_Mode: %d\n",
			       pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode);
			printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_DefaultEnable);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9341C_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
			printf("\tGPIOCfg.GPIO_Mode: %d\n",
			       pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode);
			printf("\tGPIOCfg.GPIO_WakeUpEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_WakeUpEnable);
			printf("\tGPIOCfg.GPIO_InterruptEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptEnable);
			printf("\tGPIOCfg.GPIO_InterruptMode: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptMode);
			printf("\tGPIOCfg.GPIO_FilterDelay: %d\n",
			       pGPIOCfg[iCh].GPIO_FilterDelay);
			printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_DefaultEnable);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH341_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
		} else {
			retval = -ERR_INCOMPATIBLE;
			goto exit;
		}
		printf("\n");
	}

retry:
	if (gExitRequested)
		goto exit;

	printf("\n********** GPIO Input Start **********\n\n");
	retval = CH34X_GPIOGet(fd, &iGpioLevel);
	if (retval) {
		printf("CH34X_GPIOGet Error, Code: %d.\n", retval);
		goto exit;
	}
	printf("\nGPIO level status from GPIO0 to GPIO%d:\n\n",
	       gCH34XDevice[iIndex].MaxGPIOChannel - 1);
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		if (iGpioLevel & (1 << iCh))
			printf("H-");
		else
			printf("L-");
	}
	printf("\n");
	printf("\n********** GPIO Input End **********\n\n");

	printf("Please input char to continue...\n");
	while ((ch = getchar()) != EOF && ch != '\n')
		;
	if (ch == EOF)
		goto exit;
	goto retry;

exit:
	free(pGPIOCfg);
	return retval;
}

static int CH34X_Demo_GPIO_Output_Test(int fd)
{
	int retval;
	mGPIO_Cfg *pGPIOCfg = NULL;
	uint64_t iGPIOChannel;
	uint8_t iCh;
	int j;
	int iIndex;
	int ch;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	pGPIOCfg = malloc(sizeof(mGPIO_Cfg) *
			  gCH34XDevice[iIndex].MaxGPIOChannel);
	if (pGPIOCfg == NULL)
		return -ERR_MALLOC;
	memset(pGPIOCfg, 0,
	       sizeof(mGPIO_Cfg) * gCH34XDevice[iIndex].MaxGPIOChannel);

	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				1; /* GPIO direction setting: output */
			pGPIOCfg[iCh].GPIO_PinLevel =
				1; /* GPIO pin output level state: high */
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9341C_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				1; /* GPIO direction setting: output */
			pGPIOCfg[iCh].GPIO_PinLevel =
				1; /* GPIO pin output level state: high */
			pGPIOCfg[iCh].GPIO_DefaultEnable =
				0; /* GPIO power-down save function setting: ignore */
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9346W_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				1; /* GPIO direction setting: output */
			pGPIOCfg[iCh].GPIO_PinLevel =
				1; /* GPIO pin output level state: high */
			pGPIOCfg[iCh].GPIO_DefaultEnable =
				0; /* GPIO power-down save function setting: ignore */
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH341_VENDOR) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				1; /* GPIO direction setting: output */
			pGPIOCfg[iCh].GPIO_PinLevel =
				1; /* GPIO pin output level state: high */
		} else {
			retval = -ERR_INCOMPATIBLE;
			goto exit;
		}
	}

	/* Initialize GPIO interface */
	iGPIOChannel =
		(UINT64_C(1) << gCH34XDevice[iIndex].MaxGPIOChannel) - 1;
	retval = CH34X_GPIOInit(fd, iGPIOChannel, pGPIOCfg);
	if (retval) {
		printf("CH34X_GPIOInit Error, Code: %d.\n", retval);
		goto exit;
	}

	sleep(1);

	retval = CH34X_GetInfo_GPIO(fd, pGPIOCfg);
	if (retval) {
		printf("CH34X_GetInfo_GPIO Error, Code: %d.\n", retval);
		goto exit;
	}

	/* List GPIO configuration */
	printf("\nDump GPIO configuration:\n\n");
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		printf("GPIO-%d Information:\n", iCh);
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9341C_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
			printf("\tGPIOCfg.GPIO_Mode: %d\n",
			       pGPIOCfg[iCh].GPIO_Mode.GPIO_OutputMode);
			printf("\tGPIOCfg.GPIO_WakeUpEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_WakeUpEnable);
			printf("\tGPIOCfg.GPIO_InterruptEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptEnable);
			printf("\tGPIOCfg.GPIO_InterruptMode: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptMode);
			printf("\tGPIOCfg.GPIO_FilterDelay: %d\n",
			       pGPIOCfg[iCh].GPIO_FilterDelay);
			printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_DefaultEnable);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9346W_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
			printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_DefaultEnable);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH341_VENDOR) {
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
		} else {
			retval = -ERR_INCOMPATIBLE;
			goto exit;
		}
		printf("\n");
	}

	printf("\nSet all GPIO output to low then continue...\n\n");
	retval = CH34X_GPIOSet(fd, iGPIOChannel, 0x00);
	if (retval) {
		printf("CH34X_GPIOSet Error, Code: %d.\n", retval);
		goto exit;
	}

	printf("Please input char to continue...\n");
	while ((ch = getchar()) != EOF && ch != '\n')
		;

	printf("\n********** GPIO Output Start **********\n\n");
	printf("\nGPIO level control from GPIO0 to GPIO%d:\n\n",
	       gCH34XDevice[iIndex].MaxGPIOChannel - 1);
	usleep(200 * 1000);
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		retval = CH34X_GPIOSet(fd, iGPIOChannel, UINT64_C(1) << iCh);
		if (retval) {
			printf("CH34X_GPIOSet Error, Code: %d.\n", retval);
			goto exit;
		}
		printf("\n");
		for (j = 0; j < gCH34XDevice[iIndex].MaxGPIOChannel; j++) {
			if (j == iCh)
				printf("H-");
			else
				printf("L-");
		}
		printf("\n");
		usleep(200 * 1000);

		printf("Please input char to continue...\n");
		while ((ch = getchar()) != EOF && ch != '\n')
			;
	}
	printf("\n********** GPIO Output End **********\n\n");

exit:
	free(pGPIOCfg);
	return retval;
}

static int CH34X_Demo_GPIO_Factory_Setting_Test(int fd)
{
	int retval = ERR_SUCCESS;
	mGPIO_Cfg *pGPIOCfg = NULL;
	uint64_t iGPIOChannel;
	uint64_t iGpioLevel;
	uint8_t iCh;
	char choice;
	int ch;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	/* Only CH9341C supports GPIO factory settings */
	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH9346W_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	printf("Type 's' to set, type 'g' to get, type 'r' to reset, type 'q' to quit.\n");
	if (scanf("%c", &choice) != 1)
		goto exit;
	while ((ch = getchar()) != EOF && ch != '\n')
		;

	pGPIOCfg = malloc(sizeof(mGPIO_Cfg) *
			  gCH34XDevice[iIndex].MaxGPIOChannel);
	if (pGPIOCfg == NULL)
		return -ERR_MALLOC;
	memset(pGPIOCfg, 0,
	       sizeof(mGPIO_Cfg) * gCH34XDevice[iIndex].MaxGPIOChannel);

	switch (choice) {
	case 's':
		/* Setting GPIO to input */
		for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel;
		     iCh++) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_Direction =
				0; /* GPIO direction setting: input */
			if (iCh % 2 == 0)
				pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode =
					2; /* GPIO input mode setting: pull-down input*/
			else
				pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode =
					1; /* GPIO input mode setting: pull-up input*/
			pGPIOCfg[iCh].GPIO_DefaultEnable =
				1; /* GPIO power-down save function setting: enable power-down retention */
		}

		/* Initialize GPIO interface */
		iGPIOChannel = (UINT64_C(1)
				<< gCH34XDevice[iIndex].MaxGPIOChannel) -
			       1;
		retval = CH34X_GPIOInit(fd, iGPIOChannel, pGPIOCfg);
		if (retval) {
			printf("CH34X_GPIOInit Error, Code: %d.\n",
			       retval);
			goto exit;
		}
		printf("CH34X_GPIOInit Done...\n\n");
		break;
	case 'g':
		retval = CH34X_GetInfo_GPIO(fd, pGPIOCfg);
		if (retval) {
			printf("CH34X_GetInfo_GPIO Error, Code: %d.\n",
			       retval);
			goto exit;
		}

		/* List GPIO configuration */
		printf("\nDump GPIO configuration:\n\n");
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
			for (iCh = 0;
			     iCh < gCH34XDevice[iIndex].MaxGPIOChannel;
			     iCh++) {
				printf("GPIO-%d Information:\n", iCh);
				printf("\tGPIOCfg.GPIO_Channel: %d\n",
				       pGPIOCfg[iCh].GPIO_Channel);
				printf("\tGPIOCfg.GPIO_Direction: %d\n",
				       pGPIOCfg[iCh].GPIO_Direction);
				printf("\tGPIOCfg.GPIO_Mode: %d\n",
				       pGPIOCfg[iCh]
					       .GPIO_Mode.GPIO_InputMode);
				printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
				       pGPIOCfg[iCh].GPIO_PinLevel);
				printf("\tGPIOCfg.GPIO_WakeUpEnable: %d\n",
				       pGPIOCfg[iCh].GPIO_WakeUpEnable);
				printf("\tGPIOCfg.GPIO_InterruptEnable: %d\n",
				       pGPIOCfg[iCh].GPIO_InterruptEnable);
				printf("\tGPIOCfg.GPIO_InterruptMode: %d\n",
				       pGPIOCfg[iCh].GPIO_InterruptMode);
				printf("\tGPIOCfg.GPIO_FilterDelay: %d\n",
				       pGPIOCfg[iCh].GPIO_FilterDelay);
				printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
				       pGPIOCfg[iCh].GPIO_DefaultEnable);
				printf("\n");
			}
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9346W_VENDOR) {
			for (iCh = 0;
			     iCh < gCH34XDevice[iIndex].MaxGPIOChannel;
			     iCh++) {
				printf("GPIO-%d Information:\n", iCh);
				printf("\tGPIOCfg.GPIO_Channel: %d\n",
				       pGPIOCfg[iCh].GPIO_Channel);
				printf("\tGPIOCfg.GPIO_Direction: %d\n",
				       pGPIOCfg[iCh].GPIO_Direction);
				printf("\tGPIOCfg.GPIO_Mode: %d\n",
				       pGPIOCfg[iCh]
					       .GPIO_Mode.GPIO_InputMode);
				printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
				       pGPIOCfg[iCh].GPIO_PinLevel);
				printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
				       pGPIOCfg[iCh].GPIO_DefaultEnable);
				printf("\n");
			}
		}
		printf("\n********** GPIO Input Start **********\n\n");
		printf("\nGPIO level status from GPIO0 to GPIO%d:\n\n",
		       gCH34XDevice[iIndex].MaxGPIOChannel - 1);
		retval = CH34X_GPIOGet(fd, &iGpioLevel);
		if (retval) {
			printf("CH34X_GPIOGet Error, Code: %d.\n", retval);
			goto exit;
		}
		for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel;
		     iCh++) {
			if (iGpioLevel & (1 << iCh))
				printf("H-");
			else
				printf("L-");
		}
		printf("\n");
		printf("\n********** GPIO Input End **********\n\n");
		break;
	case 'r':
		/* Disable GPIO power-down retention */
		for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel;
		     iCh++) {
			pGPIOCfg[iCh].GPIO_Channel =
				iCh; /* GPIO channel number setting */
			pGPIOCfg[iCh].GPIO_DefaultEnable =
				2; /* GPIO power-down save function setting: disable power-down retention */
		}

		/* Initialize GPIO interface */
		iGPIOChannel = (UINT64_C(1)
				<< gCH34XDevice[iIndex].MaxGPIOChannel) -
			       1;
		retval = CH34X_GPIOInit(fd, iGPIOChannel, pGPIOCfg);
		if (retval) {
			printf("CH34X_GPIOInit Error, Code: %d.\n",
			       retval);
			goto exit;
		}
		printf("CH34X_GPIOInit Done...\n\n");
		break;
	case 'q':
	default:
		break;
	}

exit:
	free(pGPIOCfg);
	return retval;
}

static void CH34X_Demo_IRQ_Handler(int signo)
{
	(void)signo;
	gIRQPending = 1;
}

static int CH34X_Demo_GPIO_IRQ_Test(int fd, uint8_t enable)
{
	int retval;
	mGPIO_Cfg *pGPIOCfg = NULL;
	uint8_t iCh;
	uint8_t iGPIOIndex =
		0; /* CH9341C only supports interrupt functionality on GPIO0/1/2/7/8/9 */
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	/* Only CH347T/CH347F/CH9341C supports GPIO interrupt */
	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH347T_VENDOR &&
	    gCH34XDevice[iIndex].ChipType != CHIP_CH347F_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	pGPIOCfg = malloc(sizeof(mGPIO_Cfg) *
			  gCH34XDevice[iIndex].MaxGPIOChannel);
	if (pGPIOCfg == NULL)
		return -ERR_MALLOC;
	memset(pGPIOCfg, 0,
	       sizeof(mGPIO_Cfg) * gCH34XDevice[iIndex].MaxGPIOChannel);

	if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
	    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
		pGPIOCfg[iGPIOIndex].GPIO_Channel =
			iGPIOIndex; /* GPIO channel number setting */
		pGPIOCfg[iGPIOIndex].GPIO_Direction =
			0; /* GPIO direction setting: input */
		if (enable)
			pGPIOCfg[iGPIOIndex].GPIO_InterruptEnable =
				1; /* GPIO interrupt function enabled */
		else
			pGPIOCfg[iGPIOIndex].GPIO_InterruptEnable =
				0; /* GPIO interrupt function disabled */
		pGPIOCfg[iGPIOIndex].GPIO_InterruptMode =
			0; /* GPIO interrupt mode configuration: falling edge */

	} else if (gCH34XDevice[iIndex].ChipType == CHIP_CH9341C_VENDOR) {
		/* Falling Edge */
		pGPIOCfg[iGPIOIndex].GPIO_Mode.GPIO_InputMode =
			1; /* GPIO input mode setting: pull-up input*/
		if (enable)
			pGPIOCfg[iGPIOIndex].GPIO_InterruptEnable =
				1; /* GPIO interrupt function enabled */
		else
			pGPIOCfg[iGPIOIndex].GPIO_InterruptEnable =
				0; /* GPIO interrupt function disabled */
		pGPIOCfg[iGPIOIndex].GPIO_InterruptMode =
			0; /* GPIO interrupt mode configuration: falling edge */
	} else {
		retval = -ERR_INCOMPATIBLE;
		goto exit;
	}

	/* Initialize GPIO interface */
	retval = CH34X_GPIOInit(fd, 1 << iGPIOIndex, pGPIOCfg);
	if (retval) {
		printf("CH34X_GPIOInit Error, Code: %d.\n", retval);
		goto exit;
	}
	printf("CH34X_GPIOInit Success.\n");

	retval = CH34X_GetInfo_GPIO(fd, pGPIOCfg);
	if (retval) {
		printf("CH34X_GetInfo_GPIO Error, Code: %d.\n", retval);
		goto exit;
	}

	/* List GPIO configuration */
	printf("\nDump GPIO configuration:\n\n");
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxGPIOChannel; iCh++) {
		if (gCH34XDevice[iIndex].ChipType == CHIP_CH347T_VENDOR ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347T_HID ||
		    gCH34XDevice[iIndex].ChipType == CHIP_CH347F_VENDOR) {
			printf("GPIO-%d Information:\n", iCh);
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
		} else if (gCH34XDevice[iIndex].ChipType ==
			   CHIP_CH9341C_VENDOR) {
			printf("GPIO-%d Information:\n", iCh);
			printf("\tGPIOCfg.GPIO_Channel: %d\n",
			       pGPIOCfg[iCh].GPIO_Channel);
			printf("\tGPIOCfg.GPIO_Direction: %d\n",
			       pGPIOCfg[iCh].GPIO_Direction);
			printf("\tGPIOCfg.GPIO_PinLevel: %d\n",
			       pGPIOCfg[iCh].GPIO_PinLevel);
			printf("\tGPIOCfg.GPIO_Mode: %d\n",
			       pGPIOCfg[iCh].GPIO_Mode.GPIO_InputMode);
			printf("\tGPIOCfg.GPIO_WakeUpEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_WakeUpEnable);
			printf("\tGPIOCfg.GPIO_InterruptEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptEnable);
			printf("\tGPIOCfg.GPIO_InterruptMode: %d\n",
			       pGPIOCfg[iCh].GPIO_InterruptMode);
			printf("\tGPIOCfg.GPIO_FilterDelay: %d\n",
			       pGPIOCfg[iCh].GPIO_FilterDelay);
			printf("\tGPIOCfg.GPIO_DefaultEnable: %d\n",
			       pGPIOCfg[iCh].GPIO_DefaultEnable);
		} else {
			retval = -ERR_INCOMPATIBLE;
			goto exit;
		}
		printf("\n");
	}

	if (enable)
		retval = CH34X_GPIOSetIntRoutine(fd, iGPIOIndex,
						 CH34X_Demo_IRQ_Handler);
	else
		retval = CH34X_GPIOSetIntRoutine(fd, iGPIOIndex, NULL);
	if (retval) {
		printf("CH34X_GPIOSetIntRoutine Error, Code: %d.\n",
		       retval);
		goto exit;
	}
	gGPIOIRQEnabled = enable != 0;
	printf("CH34X_GPIOSetIntRoutine Success.\n");
exit:
	free(pGPIOCfg);
	return retval;
}

static int CH34X_Demo_GPIO_Test(int fd)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].USBType == TYPE_TTY) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	while (!gExitRequested) {
		printf("\nPress o to GPIO output test, i to GPIO input test,\n"
		       "\tt to GPIO default setting test,\n"
		       "\te to enable GPIO IRQ function, d to disable GPIO IRQ,\n"
		       "\tq to quit.\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (gIRQPending) {
			gIRQPending = 0;
			printf("GPIO interrupt received (notifications may be coalesced).\n");
		}
		if (choice == 'q')
			break;

		switch (choice) {
		case 'i':
			printf("GPIO Input Test Begin.\n");
			retval = CH34X_Demo_GPIO_Input_Test(fd);
			if (retval)
				goto exit;
			break;
		case 'o':
			printf("GPIO Output Test Begin.\n");
			retval = CH34X_Demo_GPIO_Output_Test(fd);
			if (retval)
				goto exit;
			break;
		case 't':
			printf("GPIO Default Setting Test.\n");
			retval = CH34X_Demo_GPIO_Factory_Setting_Test(fd);
			if (retval)
				goto exit;
			break;
		case 'e':
			printf("IRQ Test Begin.\n");
			retval = CH34X_Demo_GPIO_IRQ_Test(fd, true);
			if (retval)
				goto exit;
			break;
		case 'd':
			printf("IRQ Test Over.\n");
			retval = CH34X_Demo_GPIO_IRQ_Test(fd, false);
			if (retval)
				goto exit;
			break;
		default:
			break;
		}
	}

exit:
	return retval;
}

static int CH34X_Demo_UART_Init(int fd)
{
	int retval;
	mUART_Cfg mUARTCfg = { 0 };
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	/* Initialize UART interface */
	mUARTCfg.UART_BaudRate =
		115200; /* UART baud rate setting: 115200bps */
	mUARTCfg.UART_DataBits = 8; /* UART data bits setting: 8 bits */
	mUARTCfg.UART_Parity = 0; /* UART parity bit setting: none */
	mUARTCfg.UART_StopBits = 0; /* UART stop bits setting: 1 */
	mUARTCfg.UART_FlowControl =
		0; /* UART flow control setting: none */
	mUARTCfg.UART_ReceiveTimeout =
		0; /* UART receive timeout setting: 0 */

	retval = CH34X_UARTInit(fd, &mUARTCfg);
	if (retval) {
		printf("CH34X_UARTInit Error, Code: %d.\n", retval);
		goto exit;
	}
	printf("CH34X_UARTInit Success.\n");

	retval = CH34X_GetInfo_UART(fd, &mUARTCfg);
	if (retval) {
		printf("CH34X_GetInfo_UART Error, Code: %d.\n", retval);
		goto exit;
	}

	/* List UART configuration */
	printf("\nDump UART configuration:\n\n");
	printf("\tUARTCfg.UART_Channel: %d\n", mUARTCfg.UART_Channel);
	printf("\tUARTCfg.UART_BaudRate: %d\n", mUARTCfg.UART_BaudRate);
	printf("\tUARTCfg.UART_DataBits: %d\n", mUARTCfg.UART_DataBits);
	printf("\tUARTCfg.UART_Parity: %d\n", mUARTCfg.UART_Parity);
	printf("\tUARTCfg.UART_StopBits: %d\n", mUARTCfg.UART_StopBits);
	printf("\tUARTCfg.UART_FlowControl: %d\n",
	       mUARTCfg.UART_FlowControl);
	printf("\tUARTCfg.UART_ReceiveTimeout: %d\n",
	       mUARTCfg.UART_ReceiveTimeout);
	printf("\n");

exit:
	return retval;
}

static int CH34X_Demo_UART_Test(int fd)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	uint8_t buffer[4096];
	int i;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].USBType == TYPE_VENDOR) {
		printf("This function only supports TTY devices and HID device mode of CH347T.\n");
		return -ERR_INCOMPATIBLE;
	}

	while (!gExitRequested) {
		printf("\nPress i to initialize UART channel,\n"
		       "\tw to write data from UART, r to read data from UART,\n"
		       "\tq to quit.\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;

		switch (choice) {
		case 'i':
			printf("UART Initialize Begin.\n");
			retval = CH34X_Demo_UART_Init(fd);
			if (retval)
				goto exit;
			break;
		case 'w':
			printf("UART Send Data Begin.\n");
			for (i = 0; i < 256; i++)
				buffer[i] = i;
			retval = CH34X_UARTWrite(fd, buffer, 256);
			if (retval < 0)
				goto exit;
			printf("Write %d bytes succeed.\n", retval);
			retval = ERR_SUCCESS;
			break;
		case 'r':
			printf("UART Receive Data Begin.\n");
			retval =
				CH34X_UARTRead(fd, buffer, sizeof(buffer));
			if (retval < 0)
				goto exit;
			printf("Read %d bytes succeed.\n", retval);
			printf("\nReceived Data:\n");
			for (i = 0; i < retval; i++)
				printf("%02x ", buffer[i]);
			printf("\n");
			retval = ERR_SUCCESS;
			break;
		default:
			break;
		}
	}

exit:
	return retval;
}

static int CH34X_Demo_PWM_Init(int fd)
{
	int retval;
	mPWM_Cfg *pPWMCfg = NULL;
	uint8_t iCh;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	/* Only CH9341C supports PWM function */
	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR) {
		printf("This chip does not support this function.\n");
		return -ERR_INCOMPATIBLE;
	}

	pPWMCfg = malloc(sizeof(*pPWMCfg) *
			 gCH34XDevice[iIndex].MaxPWMChannel);
	if (pPWMCfg == NULL)
		return -ERR_MALLOC;
	memset(pPWMCfg, 0,
	       sizeof(mPWM_Cfg) * gCH34XDevice[iIndex].MaxPWMChannel);

	/* Initialize PWM interface */
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxPWMChannel; iCh++) {
		pPWMCfg[iCh].PWM_Channel = iCh; /* PWM channel setting */
		pPWMCfg[iCh].PWM_Polarity =
			1; /* PWM polarity setting: active high */
		pPWMCfg[iCh].PWM_Start = 0; /* PWM output off */
		pPWMCfg[iCh].PWM_Cycle =
			10 *
			1000000; /* PWM output cycle number setting, PWM output time = cycle number * clock cycle, CH9341 clock cycle: 1/96 us */
		pPWMCfg[iCh].PWM_DutyCycle =
			5 *
			1000000; /* PWM output duty cycle setting, duty cycle = PWM_DutyCycle/PWM_Cycle = 50% */
		pPWMCfg[iCh].PWM_RepeatTimes =
			10; /* PWM output repeat times setting: 10 */

		retval = CH34X_PWMInit(fd, iCh, &pPWMCfg[iCh]);
		if (retval) {
			printf("CH34X_PWMInit Error, Code: %d.\n", retval);
			goto exit;
		}

		retval = CH34X_GetInfo_PWM(fd, iCh, &pPWMCfg[iCh]);
		if (retval) {
			printf("CH34X_GetInfo_PWM (Ch=%d) Error, Code: %d.\n",
			       iCh, retval);
			goto exit;
		}
	}

	/* List PWM configuration */
	printf("\nDump PWM configuration:\n\n");
	for (iCh = 0; iCh < gCH34XDevice[iIndex].MaxPWMChannel; iCh++) {
		printf("PWM-%d Information:\n", iCh);
		printf("\tPWMCfg.PWM_Channel: %d\n",
		       pPWMCfg[iCh].PWM_Channel);
		printf("\tPWMCfg.PWM_Polarity: %d\n",
		       pPWMCfg[iCh].PWM_Polarity);
		printf("\tPWMCfg.PWM_Start: %d\n", pPWMCfg[iCh].PWM_Start);
		printf("\tPWMCfg.PWM_Cycle: %d\n", pPWMCfg[iCh].PWM_Cycle);
		printf("\tPWMCfg.PWM_DutyCycle: %d\n",
		       pPWMCfg[iCh].PWM_DutyCycle);
		printf("\n");
	}

exit:
	free(pPWMCfg);
	return retval;
}

static int CH34X_Demo_PWM_Test(int fd)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	int iIndex;

	iIndex = CH34X_GetDeviceIndex(fd);
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER)
		return -ERR_INVALID_HANDLE;

	if (gCH34XDevice[iIndex].ChipType != CHIP_CH9341C_VENDOR) {
		printf("This function only supports CH9341C.\n");
		return -ERR_INCOMPATIBLE;
	}

	while (!gExitRequested) {
		printf("\nPress i to initialize all PWM channels,\n"
		       "\ts to start all PWM channels, p to stop all PWM channels,\n"
		       "\tq to quit.\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;

		switch (choice) {
		case 'i':
			printf("PWM Initialize Begin.\n");
			retval = CH34X_Demo_PWM_Init(fd);
			if (retval)
				goto exit;
			break;
		case 's':
			printf("PWM Output Start.\n");
			retval = CH34X_PWMControl(fd, 0, true);
			if (retval)
				goto exit;
			retval = CH34X_PWMControl(fd, 1, true);
			if (retval)
				goto exit;
			break;
		case 'p':
			printf("PWM Output Stop.\n");
			retval = CH34X_PWMControl(fd, 0, false);
			if (retval)
				goto exit;
			retval = CH34X_PWMControl(fd, 1, false);
			if (retval)
				goto exit;
			break;
		default:
			break;
		}
	}

exit:
	return retval;
}

static void CH34X_Demo_SIG_Handler(int signo)
{
	(void)signo;
	gExitRequested = 1;
}

/*****************************************************************/
/******************** Device Miscellaneous APIs ******************/
/*****************************************************************/

static void CH34X_Demo_Dump_DeviceList()
{
	int iIndex;

	for (iIndex = 0; iIndex < CH34X_MAX_NUMBER; iIndex++) {
		if (gCH34XDevice[iIndex].pathname[0] == '\0')
			continue;
		printf("Device[%d] Information:\n", iIndex);
		printf("\tDevice Type: %d\n",
		       gCH34XDevice[iIndex].ChipType);
		printf("\tManufacturer String: %s\n",
		       gCH34XDevice[iIndex].manufacturer);
		printf("\tProduct String: %s\n",
		       gCH34XDevice[iIndex].product);
		printf("\tSerial String: %s\n",
		       gCH34XDevice[iIndex].serial);
		printf("\tUSB Location: %s\n",
		       gCH34XDevice[iIndex].location);
		printf("\tDevice Path: %s\n",
		       gCH34XDevice[iIndex].pathname);
		printf("\tDevice Index: %d\n",
		       gCH34XDevice[iIndex].iIndex);
		printf("\tDevice Mode: 0x%d\n",
		       gCH34XDevice[iIndex].ChipMode);
		printf("\tFirmware Version: 0x%x\n",
		       gCH34XDevice[iIndex].fwVer);
		printf("\tDevice GPIO Channel: %d\n",
		       gCH34XDevice[iIndex].MaxGPIOChannel);
		printf("\tDevice PWM Channel: %d\n",
		       gCH34XDevice[iIndex].MaxPWMChannel);
		printf("\n");
	}
}

static void CH34X_Demo_USB_Hotplug_Handler(char *device, int event)
{
	printf("\n\tDetect USB hotplug event, device = %s, event = %d.\n\n",
	       device, event);
}

/*****************************************************************/
/************************** Main Routine *************************/
/*****************************************************************/

int main(void)
{
	int retval = ERR_SUCCESS;
	char choice;
	int ch;
	int iIndex;
	int i;
	CH34X_DEVICE_INFO infoList[CH34X_MAX_NUMBER];
	int fd = -1;
	bool notify_enabled = false;
	sigset_t signal_set, old_mask;
	struct sigaction action = { 0 };
	char sDriverVersion[20] = { 0 };
	char sLibraryVersion[20] = { 0 };

	action.sa_handler = CH34X_Demo_SIG_Handler;
	sigemptyset(&action.sa_mask);
	if (sigaction(SIGINT, &action, NULL) < 0)
		return EXIT_FAILURE;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGINT);
	retval = pthread_sigmask(SIG_BLOCK, &signal_set, &old_mask);
	if (retval)
		return EXIT_FAILURE;
	retval = CH34X_SetDeviceNotify(CH34X_Demo_USB_Hotplug_Handler);
	pthread_sigmask(SIG_SETMASK, &old_mask, NULL);
	if (retval) {
		printf("CH34X_SetDeviceNotify Error, Code: %d.\n", retval);
		goto exit;
	}
	notify_enabled = true;

rescan:
	if (gExitRequested)
		goto exit;
	retval = CH34X_GetDeviceInfoList(infoList, &NumDevs);
	if (retval) {
		printf("CH34X_GetDeviceInfoList Error, Code: %d.\n",
		       retval);
		goto exit;
	}
	/* GetDeviceInfoList exports a compact list. Store each item by its
     * library index so GetDeviceIndex(fd) can directly address this cache. */
	memset(gCH34XDevice, 0, sizeof(gCH34XDevice));
	for (i = 0; i < NumDevs; i++) {
		int index = infoList[i].iIndex;
		if (index < 0 || index >= CH34X_MAX_NUMBER) {
			retval = -ERR_INVALID_PARAM;
			goto exit;
		}
		gCH34XDevice[index] = infoList[i];
	}
	printf("Scan %d devices in total.\n", NumDevs);
	if (NumDevs == 0) {
		printf("Please check hardware connection or if driver loaded already.\n");
		sleep(2);
		goto rescan;
	}

	/* SIGINT handler is installed before enabling USB notifications. */

	CH34X_Demo_Dump_DeviceList();

	printf("Please input device iIndex number to operate: ");
	if (scanf("%d", &iIndex) != 1) {
		retval = gExitRequested ? ERR_SUCCESS : -ERR_INVALID_PARAM;
		goto exit;
	}
	if (iIndex < 0 || iIndex >= CH34X_MAX_NUMBER ||
	    gCH34XDevice[iIndex].pathname[0] == '\0') {
		retval = -ERR_INVALID_PARAM;
		printf("Invalid device iIndex.\n");
		goto exit;
	}
	while ((ch = getchar()) != EOF && ch != '\n')
		;
	printf("\nSelected device iIndex: %d\n", iIndex);

	sleep(1);

	/* Open device */
	gCH34XDevice[iIndex].fd = CH34X_OpenDevice(iIndex);
	if (gCH34XDevice[iIndex].fd < 0) {
		printf("CH34X_OpenDevice Error, Code: %d.\n",
		       gCH34XDevice[iIndex].fd);
		retval = gCH34XDevice[iIndex].fd;
		goto exit;
	}
	fd = gCH34XDevice[iIndex].fd;
	printf("Open device %s succeed, fd: %d.\n",
	       gCH34XDevice[iIndex].pathname, gCH34XDevice[iIndex].fd);

	/* Get Library Version */
	retval = CH34X_GetLibraryVersion(sLibraryVersion);
	if (retval) {
		printf("CH34X_GetLibraryVersion error.\n");
		goto exit;
	}
	printf("Library version: %s\n", sLibraryVersion);

	if (gCH34XDevice[iIndex].USBType == TYPE_VENDOR ||
	    gCH34XDevice[iIndex].USBType == TYPE_HID) {
		/* Setting USB Timeout */
		retval = CH34X_SetUSBTimeout(gCH34XDevice[iIndex].fd, 2000,
					     2000);
		if (retval) {
			printf("CH34X_SetUSBTimeout Error, Code: %d.\n",
			       retval);
			goto exit;
		}
	}

	if (gCH34XDevice[iIndex].USBType == TYPE_VENDOR) {
		/* Get Driver Version */
		retval = CH34X_GetDriverVersion(gCH34XDevice[iIndex].fd,
						sDriverVersion);
		if (retval) {
			printf("CH34X_GetDriverVersion error.\n");
			goto exit;
		}
		printf("Driver version: %s\n", sDriverVersion);
	}

	while (!gExitRequested) {
		printf("\nPress f to operate SPI FLASH, e to operate EEPROM,\n"
		       "\tg to GPIO and IRQ function test,\n"
		       "\ts to SPI slave test, i to I2C slave test,\n"
		       "\to to FIFO slave test, u to UART test,\n"
		       "\tw to PWM test\n");

		if (scanf("%c", &choice) != 1)
			goto exit;
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;
		switch (choice) {
		case 'f':
			printf("FLASH Test Begin...\n");
			retval = CH34X_Demo_SPI_Master_FLASH_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 'e':
			printf("EEPROM Test Begin...\n");
			retval = CH34X_Demo_I2C_Master_EEPROM_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 'g':
			retval = CH34X_Demo_GPIO_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 's':
			printf("SPI Slave Test Begin...\n");
			retval = CH34X_Demo_SPI_Slave_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 'i':
			printf("I2C Slave Test Begin...\n");
			retval = CH34X_Demo_I2C_Slave_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 'o':
			printf("FIFO Slave Test Begin...\n");
			retval = CH34X_Demo_FIFO_Slave_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 'u':
			printf("UART Test Begin...\n");
			retval = CH34X_Demo_UART_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		case 'w':
			retval = CH34X_Demo_PWM_Test(
				gCH34XDevice[iIndex].fd);
			if (retval)
				goto exit;
			break;
		default:
			printf("Bad choice, please input again.\n");
			break;
		}
	}

exit:
	sSPIThreadLoop = rSPIThreadLoop = false;
	sI2CThreadLoop = rI2CThreadLoop = false;
	sFIFOThreadLoop = rFIFOThreadLoop = false;
	CH34X_Demo_StopThreads(INTF_SPI);
	CH34X_Demo_StopThreads(INTF_I2C);
	CH34X_Demo_StopThreads(INTF_FIFO);
	if (fd >= 0) {
		int cleanup;
		if (gGPIOIRQEnabled) {
			cleanup = CH34X_Demo_GPIO_IRQ_Test(fd, false);
			if (!retval)
				retval = cleanup;
		}
		cleanup = CH34X_CloseDevice(fd);
		if (!retval)
			retval = cleanup;
	}
	if (notify_enabled) {
		int cleanup = CH34X_SetDeviceNotify(NULL);
		if (!retval)
			retval = cleanup;
	}
	return retval ? EXIT_FAILURE : EXIT_SUCCESS;
}
