#ifndef _EEPROM_XX24_H
#define _EEPROM_XX24_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) (((x) < (y)) ? (y) : (x))
#endif

#ifndef ENUM_EEPROM_TYPE
typedef enum _EEPROM_TYPE {
	ID_24C01,
	ID_24C02,
	ID_24C04,
	ID_24C08,
	ID_24C16,
	ID_24C32,
	ID_24C64,
	ID_24C128,
	ID_24C256,
	ID_24C512,
	ID_24C1024,
	ID_24C2048,
	ID_24C4096
} EEPROM_TYPE;
#define ENUM_EEPROM_TYPE
#endif

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
			    void *oReadBuffer, uint32_t oReadLength);

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
			     void *iWriteBuffer, uint32_t iWriteLength);

#ifdef __cplusplus
}
#endif

#endif
