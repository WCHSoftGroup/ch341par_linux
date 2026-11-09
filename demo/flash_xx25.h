#ifndef _FLASH_XX25_H
#define _FLASH_XX25_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
#define ERR_FILE_OC 1 /* File open/close/stat error */
#define ERR_FILE_RW 2 /* File read/write error */
#define ERR_FLASH_CONTENT 3 /* Flash content compare error */
#define ERR_FLASH_ADDRESS 4 /* Flash address error */
#define ERR_FLASH_BUSY 5 /* Flash status busy error */

#define CMD_FLASH_CHIP_ERASE 0x60
#define CMD_FLASH_SECTOR_ERASE 0x20
#define CMD_FLASH_BYTE_PROG 0x02
#define CMD_FLASH_READ 0x03
#define CMD_FLASH_RDSR 0x05
#define CMD_FLASH_WREN 0x06
#define CMD_FLASH_JEDEC_ID 0x9F

#define FLASH_PAGE 256

/**
 * @brief Read JEDEC ID of SPI FLASH via Read JEDEC ID instruction 0x9F.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_ID_Read(int fd);

/**
 * @brief Enable write operation of SPI FLASH via Write Enable instruction 0x06.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Write_Enable(int fd);

/**
 * @brief Read status register of SPI FLASH via Read Status Register instruction 0x05.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Wait(int fd, uint32_t iRetryTimes);

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
extern int FLASH_Wait_AutoQuery(int fd);

/**
 * @brief Erase SPI FLASH sector via Sector Erase instruction 0x20.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Sector_Erase(int fd, uint32_t iAddress);

/**
 * @brief Erase SPI FLASH via Chip Erase instruction 0x60.
 *     
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347T/F/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int FLASH_Chip_Erase(int fd);

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
			    uint32_t iLength);

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
			    uint32_t oLength);

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
			     uint32_t iLength);

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
extern int FLASH_File_Read(int fd, const char *path);

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
extern int FLASH_File_Write(int fd, const char *path);

#ifdef __cplusplus
}
#endif

#endif
