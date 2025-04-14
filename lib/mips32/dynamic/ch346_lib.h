#ifndef _CH346_LIB_H
#define _CH346_LIB_H

#include <stdint.h>

#define SLAVE_MAX_LENGTH (1 * 1024 * 1024)

#ifndef ENUM_CHIP_TYPE
typedef enum _CHIP_TYPE {
	CHIP_CH341 = 0,
	CHIP_CH347T = 1,
	CHIP_CH347F = 2,
	CHIP_CH339W = 3,
	CHIP_CH346C = 4,
} CHIP_TYPE;
#define ENUM_CHIP_TYPE
#endif

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CH346GetLibInfo - get library information
 */
extern const char *CH346GetLibInfo(void);

/**
 * CH346OpenDevice - open device
 * @pathname: device path in /dev directory
 *
 * The function return positive file descriptor if successful, others if fail.
 */
extern int CH346OpenDevice(const char *pathname);

/**
 * CH346CloseDevice - close device
 * @fd: file descriptor of device
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346CloseDevice(int fd);

/**
 * CH34xSetTimeout - set USB data read and write timeout
 * @fd: file descriptor of device
 * @iWriteTimeout: data download timeout in milliseconds
 * @iReadTimeout: data upload timeout in milliseconds
 *
 * The function return true if successful, false if fail.
 */
extern bool CH34xSetTimeout(int fd, uint32_t iWriteTimeout, uint32_t iReadTimeout);

/**
 * CH34x_GetDriverVersion - get vendor driver version
 * @fd: file descriptor of device
 * @Drv_Version: pointer to version string
 *
 * The function return true if successful, false if fail.
 */
extern bool CH34x_GetDriverVersion(int fd, unsigned char *Drv_Version);

/**
 * CH34x_GetChipType - get chip type
 * @fd: file descriptor of device
 * @ChipType: pointer to chip type
 *
 * The function return true if successful, false if fail.
 */
extern bool CH34x_GetChipType(int fd, CHIP_TYPE *ChipType);

/**
 * CH34X_GetDeviceID - get device VID and PID
 * @fd: file descriptor of device
 * @id: pointer to store id which contains VID and PID
 *
 * The function return true if successful, false if fail.
 */
extern bool CH34X_GetDeviceID(int fd, uint32_t *id);

/**
 * CH346_SetMode - set chip work mode
 * @fd: file descriptor of device
 * @Mode: work mode, 0->mode0, 1->mode1
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_SetMode(int fd, uint8_t Mode);

/**
 * CH346_GetMode - get chip work mode
 * @fd: file descriptor of device
 * @Mode: pointer to work mode, 0->mode0, 1->mode1
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_GetMode(int fd, uint8_t *Mode);

/**
 * CH346_SlaveInit - CH346 Slave Initialization
 * @fd: file descriptor of device
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_SlaveInit(int fd);

/**
 * CH346_Slave_Control - switch of reading FIFO data from master 
 * @fd: file descriptor of device
 * @enable: true: start reading continuously, false: stop reading
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_Slave_Control(int fd, bool enable);

/**
 * CH346_Slave_WriteData - write fifo/spi data
 * @fd: file descriptor of device
 * @iBuffer: pointer to write buffer
 * @ioLength: pointer to write length
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_Slave_WriteData(int fd, void *iBuffer,
				  uint32_t *ioLength);

/**
 * CH346_Slave_QueryData - get spi/fifo data length
 * @fd: file descriptor of device
 * @oLength: pointer to read length
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_Slave_QueryData(int fd, uint32_t *oLength);

/**
 * CH346_Slave_FIFOReset - reset spi/fifo data fifo
 * @fd: file descriptor of device
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_Slave_FIFOReset(int fd);

/**
 * CH346_Slave_ReadData - read spi/fifo data in slave mode
 * @fd: file descriptor of device
 * @oReadBuffer: pointer to read buffer
 * @oReadLength: pointer to read length
 *
 * The function return true if successful, false if fail.
 */
extern bool CH346_Slave_ReadData(int fd, void *oReadBuffer,
				 uint32_t *oReadLength);

#ifdef __cplusplus
}
#endif

#endif
