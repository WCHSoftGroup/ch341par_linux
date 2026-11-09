/*
 * XX24 EEPROM application library
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
#include "eeprom_xx24.h"

/**
 * @brief Read data from specified address of eeprom
 * 
 * @param fd File descriptor of device.
 * @param type EEPROM type.
 * @param iAddress EEPROM data address.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~Maximum supported length of EEPROM in a single operation.
 *
 * @remark This function is only available for CH347T/F/CH339W/CH341F/A/B/T/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_ReadEEPROM(int fd, EEPROM_TYPE type, uint32_t iAddress,
			    void *oReadBuffer, uint32_t oReadLength)
{
	int retval;
	uint32_t oLen;
	uint32_t iAckCount;
	uint8_t iBuf[3] = { 0 };
	uint8_t *pBuf = (uint8_t *)oReadBuffer;

	if (oReadLength == 0)
		return -ERR_INVALID_PARAM;

	if (oReadBuffer == NULL)
		return -ERR_NULLPTR;

	/* 7-11 bit address */
	if (type >= ID_24C01 && type <= ID_24C16) {
		while (oReadLength) {
			iBuf[0] =
				(uint8_t)(0xA0 | ((iAddress >> 7) & 0x0E));
			iBuf[1] = (uint8_t)iAddress;
			oLen = min(oReadLength, 1024);
			retval = CH34X_StreamI2C(fd, true, true, 0, iBuf,
						 2, pBuf, oLen,
						 &iAckCount);
			if (retval)
				break;
			iAddress += oLen;
			oReadLength -= oLen;
			pBuf += oLen;
		}
	} else if (type >= ID_24C32 && type <= ID_24C4096) {
		/* 12-19 bit address */
		while (oReadLength) {
			iBuf[0] = (uint8_t)(0xA0 |
					    ((iAddress >> 15) & 0x0E));
			iBuf[1] = (uint8_t)(iAddress >> 8);
			iBuf[2] = (uint8_t)iAddress;
			oLen = min(oReadLength, 1024);
			retval = CH34X_StreamI2C(fd, true, true, 0, iBuf,
						 3, pBuf, oLen,
						 &iAckCount);
			if (retval)
				break;
			iAddress += oLen;
			oReadLength -= oLen;
			pBuf += oLen;
		}
	} else
		return -ERR_INVALID_PARAM;

	return retval;
}

/**
 * @brief Write data to specified address of eeprom
 * 
 * @param fd File descriptor of device.
 * @param type EEPROM type.
 * @param iAddress Address of eeprom.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be write, valid length: 1~Maximum supported length of EEPROM in a single operation.
 *
 * @remark This function is only available for CH347T/F/CH339W/CH341F/A/B/T/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_WriteEEPROM(int fd, EEPROM_TYPE type, uint32_t iAddress,
			     void *iWriteBuffer, uint32_t iWriteLength)
{
	int retval;
	uint32_t iLen;
	uint32_t iAckCount;
	uint8_t iBuf[256] = { 0 };
	uint8_t *pBuf = (uint8_t *)iWriteBuffer;

	if (iWriteLength == 0)
		return -ERR_INVALID_PARAM;

	if (iWriteBuffer == NULL)
		return -ERR_NULLPTR;

	/* 7-11 bit address */
	if (type >= ID_24C01 && type <= ID_24C16) {
		while (iWriteLength) {
			iBuf[0] =
				(uint8_t)(0xA0 | ((iAddress >> 7) & 0x0E));
			iBuf[1] = (uint8_t)iAddress;
			/* No more than page length */
			iLen = (type >= ID_24C04) ? 16 - (iAddress & 15) :
						    8 - (iAddress & 7);
			if (iLen > iWriteLength)
				iLen = iWriteLength;
			memcpy(&iBuf[2], pBuf, iLen);
			retval = CH34X_StreamI2C(fd, true, true, 0, iBuf,
						 2 + iLen, NULL, 0,
						 &iAckCount);
			if (retval)
				break;
			usleep(1000 * 10);
			iAddress += iLen;
			iWriteLength -= iLen;
			pBuf += iLen;
		}
	} else if (type >= ID_24C32 && type <= ID_24C4096) {
		/* 12-19 bit address */
		while (iWriteLength) {
			iBuf[0] = (uint8_t)(0xA0 |
					    ((iAddress >> 15) & 0x0E));
			iBuf[1] = (uint8_t)(iAddress >> 8);
			iBuf[2] = (uint8_t)iAddress;
			/* No more than page length */
			iLen = (type >= ID_24C512) ?
				       128 - (iAddress & 127) :
				       ((type >= ID_24C128) ?
						64 - (iAddress & 63) :
						32 - (iAddress & 31));
			if (iLen > iWriteLength)
				iLen = iWriteLength;
			memcpy(&iBuf[3], pBuf, iLen);
			retval = CH34X_StreamI2C(fd, true, true, 0, iBuf,
						 3 + iLen, NULL, 0,
						 &iAckCount);
			if (retval)
				break;
			usleep(1000 * 10);
			iAddress += iLen;
			iWriteLength -= iLen;
			pBuf += iLen;
		}
	} else
		return -ERR_INVALID_PARAM;

	return retval;
}
