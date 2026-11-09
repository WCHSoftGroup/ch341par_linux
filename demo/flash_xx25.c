/*
 * XX25 SPI flash application library
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "ch34x_lib.h"
#include "flash_xx25.h"

/**
 * @brief Read JEDEC ID of SPI FLASH via Read JEDEC ID instruction 0x9F.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_ID_Read(int fd)
{
	int retval;
	uint32_t ioLength;
	uint8_t ioBuffer[4] = { 0 };
	uint32_t ID;

	ioLength = 4;
	ioBuffer[0] = CMD_FLASH_JEDEC_ID;
	memset(ioBuffer + 1, 0xFF, 3);

	retval = CH34X_SPIWriteRead(fd, true, true, ioBuffer, ioLength);
	if (retval)
		return retval;
	else {
		ioBuffer[0] = 0x00;
		memcpy(&ID, ioBuffer, 4);
	}
	ID = htole32(ID);
	printf("Read FLASH ID: 0x%x.\n", ID);

	if (ID == 0x000000 || ID == 0xffffff00) {
		printf("Read FLASH ID error.\n");
		return -1;
	}

	return ERR_SUCCESS;
}

/**
 * @brief Enable write operation of SPI FLASH via Write Enable instruction 0x06.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Write_Enable(int fd)
{
	uint32_t iWriteLength;
	uint8_t iWriteBuffer[1];

	iWriteLength = 1;
	iWriteBuffer[0] = CMD_FLASH_WREN;

	return CH34X_SPIFastWrite(fd, true, true, iWriteBuffer,
				  iWriteLength);
}

/**
 * @brief Read status register of SPI FLASH via Read Status Register instruction 0x05.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Wait(int fd, uint32_t iRetryTimes)
{
	int retval;
	uint32_t ioLength;
	uint8_t ioBuffer[2];
	uint8_t status;

	ioLength = 2;
	do {
		ioBuffer[0] = CMD_FLASH_RDSR;
		retval = CH34X_SPIWriteRead(fd, true, true, ioBuffer,
					    ioLength);
		if (retval)
			goto exit;
		status = ioBuffer[1];
		usleep(100);
	} while ((status & 0x01) && (iRetryTimes--));

	if ((status & 0x01) == 0)
		return ERR_SUCCESS;
	else
		return -ERR_FLASH_BUSY;

exit:
	return retval;
}

/**
 * @brief Read SPI FLASH status register via Read Status Register instruction 0x05.
 *     Unlike the FLASH_Wait API, the chip automatically polls the status after sending an request
 *     until it succeeds or times out, which can reduce CPU usage and improve polling efficiency.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH9341C devices. 
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Wait_AutoQuery(int fd)
{
	int retval;
	uint16_t iCount = 10000; /* Retry times */
	uint8_t iInterval = 200; /* Polling interval */
	uint8_t iWriteLength = 1;
	uint8_t oLength = 1;
	uint8_t iWriteBuffer[1], maskBuffer[1], targetBuffer[1];

	/* The SPI data to write */
	iWriteBuffer[0] = CMD_FLASH_RDSR;

	/* The data to ANDed with SPI data readed */
	maskBuffer[0] = 0x01;

	/* The target data to compare with */
	targetBuffer[0] = 0x00;

	/*  SPI auto read operation, max query times: 1000, interval: 100us */
	retval = CH34X_SPIReadQuery(fd, true, true, iCount, iInterval,
				    iWriteBuffer, iWriteLength, maskBuffer,
				    targetBuffer, oLength);

	if (retval == -ERR_AGAIN)
		return -ERR_FLASH_BUSY;

	return retval;
}

/**
 * @brief Erase SPI FLASH sector via Sector Erase instruction 0x20.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Sector_Erase(int fd, uint32_t iAddress)
{
	int retval;
	int iWriteLength;
	uint8_t iWriteBuffer[4];

	retval = FLASH_Write_Enable(fd);
	if (retval)
		return retval;

	iWriteLength = 4;
	iWriteBuffer[0] = CMD_FLASH_SECTOR_ERASE;
	iWriteBuffer[1] = (uint8_t)(iAddress >> 16 & 0xff);
	iWriteBuffer[2] = (uint8_t)(iAddress >> 8 & 0xf0);
	iWriteBuffer[3] = 0x00;

	retval = CH34X_SPIFastWrite(fd, true, true, iWriteBuffer,
				    iWriteLength);
	if (retval)
		return retval;

	retval = FLASH_Wait(fd, 1000);
	if (retval)
		return retval;

	return ERR_SUCCESS;
}

/**
 * @brief Erase SPI FLASH via Chip Erase instruction 0x60.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Chip_Erase(int fd)
{
	int retval;
	int iWriteLength;
	uint8_t iWriteBuffer[1];
	int iRetryTimes = 10;

	retval = FLASH_Write_Enable(fd);
	if (retval)
		return retval;

	iWriteLength = 1;
	iWriteBuffer[0] = CMD_FLASH_CHIP_ERASE;

	retval = CH34X_SPIFastWrite(fd, true, true, iWriteBuffer,
				    iWriteLength);
	if (retval)
		return retval;

	while (iRetryTimes--) {
		retval = FLASH_Wait(fd, 100 * 1000);
		if (retval)
			continue;
		else
			break;
	}

	return retval;
}

/**
 * @brief Write one page of SPI FLASH via Page Program instruction 0x02.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Write_Page(int fd, uint8_t *iBuffer, uint32_t iAddress,
			    uint32_t iLength)
{
	int retval;
	uint32_t iWriteLength;
	uint8_t iWriteBuffer[1024];

	retval = FLASH_Write_Enable(fd);
	if (retval)
		return retval;

	iWriteLength = iLength + 4;
	iWriteBuffer[0] = CMD_FLASH_BYTE_PROG;
	iWriteBuffer[1] = (uint8_t)(iAddress >> 16);
	iWriteBuffer[2] = (uint8_t)(iAddress >> 8);
	iWriteBuffer[3] = (uint8_t)iAddress;
	memcpy(&iWriteBuffer[4], iBuffer, iWriteLength);

	retval = CH34X_SPIFastWrite(fd, true, true, iWriteBuffer,
				    iWriteLength);
	if (retval)
		return retval;

	/* Wait for the write operation to complete */
	retval = FLASH_Wait(fd, 1000);
	if (retval)
		return retval;

	return ERR_SUCCESS;
}

/**
 * @brief Read data by block of SPI FLASH via Read Data instruction 0x03.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Block_Read(int fd, uint32_t iAddress, uint8_t *oBuffer,
			    uint32_t oLength)
{
	int retval;
	uint8_t iWriteBuffer[4] = { 0 };
	int iWriteLength;
	int iReadTimes;
	uint32_t oReadLength;
	uint32_t oRestLength, oReadIndex = 0;
	bool bFirst = true, bLast = false;
	uint8_t *oReadBuffer = oBuffer;

	iWriteLength = 0x04;

	iWriteBuffer[0] = CMD_FLASH_READ;
	iWriteBuffer[1] = (uint8_t)(iAddress >> 16);
	iWriteBuffer[2] = (uint8_t)(iAddress >> 8);
	iWriteBuffer[3] = (uint8_t)(iAddress);

	if (oLength % SPI_MAX_LENGTH == 0)
		iReadTimes = oLength / SPI_MAX_LENGTH;
	else
		iReadTimes = oLength / SPI_MAX_LENGTH + 1;

	if (iReadTimes == 1) {
		oReadLength = oLength;
		retval = CH34X_SPIFastRead(fd, true, true, iWriteBuffer,
					   iWriteLength, oReadBuffer,
					   oReadLength);
		if (retval) {
			printf("CH34X_SPIFastRead Error, Code = %d.\n",
			       retval);
			return retval;
		}
	} else {
		while (iReadTimes--) {
			oRestLength = oLength - oReadIndex;
			oReadLength = oRestLength <= SPI_MAX_LENGTH ?
					      oRestLength :
					      SPI_MAX_LENGTH;
			if (iReadTimes == 0) {
				bLast = true;
			}
			if (bFirst) {
				retval = CH34X_SPIFastRead(
					fd, true, false, iWriteBuffer,
					iWriteLength,
					oReadBuffer + oReadIndex,
					oReadLength);
				bFirst = false;
			} else if (bLast) {
				retval = CH34X_SPIFastRead(
					fd, false, true, NULL, 0,
					oReadBuffer + oReadIndex,
					oReadLength);
			} else {
				retval = CH34X_SPIFastRead(
					fd, false, false, NULL, 0,
					oReadBuffer + oReadIndex,
					oReadLength);
			}
			if (retval) {
				printf("CH34X_SPIFastRead Error, Code = %d.\n",
				       retval);
				return retval;
			}
			oReadIndex += oReadLength;
		}
	}

	return ERR_SUCCESS;
}

/**
 * @brief Write data by block to SPI FLASH via FLASH_Write_Page API.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Block_Write(int fd, uint32_t iAddress, uint8_t *iBuffer,
			     uint32_t iLength)
{
	int retval;
	uint32_t NumOfPage, NumOfSingle;

	NumOfPage = iLength / FLASH_PAGE;
	NumOfSingle = iLength % FLASH_PAGE;

	if (iAddress % FLASH_PAGE) {
		printf("FLASH iAddress is not page aligned!\n");
		return -ERR_FLASH_ADDRESS;
	}

	printf("NumOfPage: %d, NumOfSingle: %d.\n", NumOfPage,
	       NumOfSingle);

	/* Caculate FLASH write time */
	while (NumOfPage--) {
		retval = FLASH_Write_Page(fd, iBuffer, iAddress,
					  FLASH_PAGE);
		if (retval)
			goto exit;
		iBuffer += FLASH_PAGE;
		iAddress += FLASH_PAGE;
	}
	if (NumOfSingle) {
		retval = FLASH_Write_Page(fd, iBuffer, iAddress,
					  NumOfSingle);
		if (retval)
			goto exit;
	}

	return ERR_SUCCESS;

exit:
	return retval;
}

/**
 * @brief Read data from SPI FLASH and auto compare with the file content.
 *     
 * @param fd File descriptor of device.
 * @param path File path.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_File_Read(int fd, const char *path)
{
	uint32_t iAddress;
	uint32_t ufilesize;
	size_t buffer_size;
	size_t read_size;
	size_t i;

	uint8_t *oBuffer = NULL;
	uint8_t *cBuffer = NULL;

	double UseT;
	struct timeval t1, t2;
	struct stat statbuff;

	int retval = -ERR_FILE_RW;
	FILE *fp = NULL;

	static unsigned int count = 0;

	if (path == NULL) {
		printf("Invalid file path.\n");
		return -ERR_FILE_OC;
	}

	fp = fopen(path, "rb");
	if (fp == NULL) {
		perror("Open file failed");
		return -ERR_FILE_OC;
	}

	if (fstat(fileno(fp), &statbuff) == -1) {
		perror("Get file size failed");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	if (!S_ISREG(statbuff.st_mode)) {
		printf("Not a regular file.\n");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	if (statbuff.st_size <= 0) {
		printf("File is empty or has an invalid size.\n");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	if ((uintmax_t)statbuff.st_size > (uintmax_t)UINT32_MAX ||
	    (uintmax_t)statbuff.st_size > (uintmax_t)SIZE_MAX) {
		printf("File too large.\n");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	ufilesize = (uint32_t)statbuff.st_size;
	buffer_size = (size_t)statbuff.st_size;

	oBuffer = malloc(buffer_size);
	cBuffer = malloc(buffer_size);

	if (oBuffer == NULL || cBuffer == NULL) {
		printf("Allocate buffer failed.\n");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	read_size = fread(cBuffer, 1, buffer_size, fp);
	if (read_size != buffer_size) {
		if (ferror(fp))
			printf("File read error.\n");
		else
			printf("Unexpected end of file.\n");

		retval = -ERR_FILE_RW;
		goto exit;
	}

	/* Calculate FLASH read time */
	if (gettimeofday(&t1, NULL) == -1) {
		perror("Get start time failed");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	iAddress = 0x00;
	retval = FLASH_Block_Read(fd, iAddress, oBuffer, ufilesize);
	if (retval != 0) {
		printf("FLASH_Block_Read Error, Code = %d.\n", retval);
		goto exit;
	}

	if (gettimeofday(&t2, NULL) == -1) {
		perror("Get end time failed");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	UseT = ((double)t2.tv_sec - (double)t1.tv_sec) +
	       ((double)t2.tv_usec - (double)t1.tv_usec) / 1000000.0;

	printf("\tFLASH File Read in %.3f seconds.\n", UseT);

	retval = memcmp(oBuffer, cBuffer, buffer_size);
	if (retval != 0) {
		printf("FLASH content compare error, retval: %d.\n",
		       retval);

		printf("Read data:\n");
		for (i = 0; i < buffer_size; i++)
			printf("%02x ", (unsigned int)oBuffer[i]);

		printf("\n\nRaw file data:\n");
		for (i = 0; i < buffer_size; i++)
			printf("%02x ", (unsigned int)cBuffer[i]);

		printf("\n\n");

		retval = -ERR_FLASH_CONTENT;
		goto exit;
	}

	printf("******************** FLASH Test Passed"
	       "--> count: %u *************\n",
	       count++);

	retval = ERR_SUCCESS;

exit:
	free(oBuffer);
	free(cBuffer);

	if (fp != NULL)
		fclose(fp);

	return retval;
}

/**
 * @brief Write the file content to SPI FLASH.
 *     
 * @param fd File descriptor of device.
 * @param path File path.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_File_Write(int fd, const char *path)
{
	double UseT;
	uint32_t iAddress;
	uint32_t ufilesize;

	size_t buffer_size;
	size_t read_size;

	uint8_t *iBuffer = NULL;
	struct timeval t1, t2;
	struct stat statbuff;

	int retval = -ERR_FILE_RW;
	FILE *fp = NULL;

	if (path == NULL) {
		printf("Invalid file path.\n");
		return -ERR_FILE_OC;
	}

	fp = fopen(path, "rb");
	if (fp == NULL) {
		perror("Open file failed");
		return -ERR_FILE_OC;
	}

	if (fstat(fileno(fp), &statbuff) == -1) {
		perror("Get file size failed");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	if (!S_ISREG(statbuff.st_mode)) {
		printf("Not a regular file.\n");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	if (statbuff.st_size <= 0) {
		printf("File is empty or has an invalid size.\n");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	if ((uintmax_t)statbuff.st_size > (uintmax_t)UINT32_MAX ||
	    (uintmax_t)statbuff.st_size > (uintmax_t)SIZE_MAX) {
		printf("File too large.\n");
		retval = -ERR_FILE_OC;
		goto exit;
	}

	ufilesize = (uint32_t)statbuff.st_size;
	buffer_size = (size_t)statbuff.st_size;

	iBuffer = malloc(buffer_size);
	if (iBuffer == NULL) {
		printf("Allocate buffer failed.\n");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	read_size = fread(iBuffer, 1, buffer_size, fp);
	if (read_size != buffer_size) {
		if (ferror(fp))
			printf("File read error.\n");
		else
			printf("Unexpected end of file.\n");

		retval = -ERR_FILE_RW;
		goto exit;
	}

	/* Calculate FLASH erase time */
	if (gettimeofday(&t1, NULL) == -1) {
		perror("Get start time failed");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	retval = FLASH_Chip_Erase(fd);
	if (retval != 0) {
		printf("FLASH_Chip_Erase Error, Code = %d.\n", retval);
		goto exit;
	}

	if (gettimeofday(&t2, NULL) == -1) {
		perror("Get end time failed");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	UseT = ((double)t2.tv_sec - (double)t1.tv_sec) +
	       ((double)t2.tv_usec - (double)t1.tv_usec) / 1000000.0;

	printf("\tFLASH Chip Erase in %.3f seconds.\n", UseT);

	/* Calculate FLASH write time */
	if (gettimeofday(&t1, NULL) == -1) {
		perror("Get start time failed");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	iAddress = 0x00;
	retval = FLASH_Block_Write(fd, iAddress, iBuffer, ufilesize);
	if (retval != 0) {
		printf("FLASH_Block_Write Error, Code = %d.\n", retval);
		goto exit;
	}

	if (gettimeofday(&t2, NULL) == -1) {
		perror("Get end time failed");
		retval = -ERR_FILE_RW;
		goto exit;
	}

	UseT = ((double)t2.tv_sec - (double)t1.tv_sec) +
	       ((double)t2.tv_usec - (double)t1.tv_usec) / 1000000.0;

	printf("\tFLASH Write File in %.3f seconds.\n", UseT);

	retval = ERR_SUCCESS;

exit:
	free(iBuffer);

	if (fp != NULL)
		fclose(fp);

	return retval;
}
