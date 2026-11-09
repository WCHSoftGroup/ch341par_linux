#ifndef _CH34X_LIB_H
#define _CH34X_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <linux/usb/ch9.h>

#define CH34X_MAX_NUMBER 128

/* Error code define */
#define ERR_SUCCESS 0 /* Operation successful */
#define ERR_INVALID_HANDLE 1 /* No such device or device is not open */
#define ERR_INVALID_PARAM 2 /* Invalid argument */
#define ERR_MALLOC 3 /* Out of memory */
#define ERR_NULLPTR 4 /* Null pointer error */
#define ERR_DEVICE_OPEN 5 /* Open device error */
#define ERR_DEVICE_CLOSE 6 /* Close device error */

#define ERR_INVALID_CMD 10 /* Device response command error */
#define ERR_PROTOCOL 11 /* Device response error */
#define ERR_AGAIN 12 /* Device busy or operation timeout */
#define ERR_INCOMPATIBLE 13 /* Device incompatible with this API */

#define ERR_USB_COMM 20 /* USB communication error */
#define ERR_USB_TIMEOUT 21 /* USB communication timeout */
#define ERR_USBIN_INCOMPLETE 22 /* USB data read length not as expected */
#define ERR_USB_NOFITY 23 /* USB hotplug notification setting error */

#define ERR_ROLE_MS 30 /* Interface master/slave role error */
#define ERR_SPI_NOTINIT 31 /* SPI interface not initialized */

#define ERR_I2C_NOTINIT 40 /* I2C interface not initialized */
#define ERR_I2C_NACK 41 /* I2C Device NACK Error */
#define ERR_I2C_WRITE 42 /* I2C write operation error or timeout */
#define ERR_I2C_READ 43 /* I2C read operation error or timeout */
#define ERR_I2C_ADDRESS 44 /* I2C address error or timeout */

#define ERR_GPIO_PROPERTY 50 /* GPIO property setting error */
#define ERR_GPIO_SET 51 /* GPIO setting error */

/* Maximum single write/read length supported in SPI master mode*/
#define SPI_MAX_LENGTH (64 * 1024)

/* Maximum single write/read length supported in I2C master mode*/
#define I2C_MAX_LENGTH (64 * 1024)

/* Maximum single write/read length supported in SPI/I2C/FIFO slave mode*/
#define SLAVE_MAX_LENGTH (64 * 1024)

/* Basic type for the double-link list.  */
typedef struct list_head {
	struct list_head *next;
	struct list_head *prev;
} list_t;

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/*
 * Get offset of a member variable.
 *
 * @param[in]	type	the type of the struct this embedded in
 * @param[in]	member	the name of the variable within the struct.
 */
#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)

/*
 * Get the struct for this entry.
 *
 * @para[in]	ptr 	the list head to take the element from.
 * @para[in]	type 	the type of the struct this is embedded in.
 * @para[in]	member	the name of the variable within the struct.
 */
#define container_of(ptr, type, member)                            \
	({                                                         \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})

/**
 * @brief GPIO interrupt callback function type definition.
 * 
 * @param SigNum Signal number SIGIO.
 *
 */
typedef void (*pCH_INT_ROUTINE)(int SigNum);

/**
 * @brief USB hotplug callback function type definition.
 * 
 * @param Pathname Device name associated with the event.
 * @param Event USB event type, 0: USB remove, 1: USB insert.
 *
 */
typedef void (*pCH_NOTIFY_ROUTINE)(char *Pathname, int Event);

typedef enum {
	CHIPBASE_VENDOR_BASE = 0,
	CHIPBASE_TTYCH343_BASE = 20,
	CHIPBASE_TTYCH9344_BASE = 80,
	CHIPBASE_TTYCH341_BASE = 100,
} CHIPBASE_TYPE;

#ifndef ENUM_CHIP_TYPE
typedef enum _CHIP_TYPE {
	CHIP_CH341_VENDOR = CHIPBASE_VENDOR_BASE,
	CHIP_CH347T_VENDOR,
	CHIP_CH347T_HID,
	CHIP_CH347F_VENDOR,
	CHIP_CH339W_VENDOR,
	CHIP_CH346C_VENDOR,
	CHIP_CH9341C_VENDOR,
	CHIP_CH9346W_VENDOR,

	CHIP_CH342F = CHIPBASE_TTYCH343_BASE,
	CHIP_CH342K,
	CHIP_CH343GP,
	CHIP_CH343G_AUTOBAUD,
	CHIP_CH343K,
	CHIP_CH343J,
	CHIP_CH344L,
	CHIP_CH344L_V2,
	CHIP_CH344Q,
	CHIP_CH347TF,
	CHIP_CH9101UH,
	CHIP_CH9101RY,
	CHIP_CH9102F,
	CHIP_CH9102X,
	CHIP_CH9103M,
	CHIP_CH9104L,
	CHIP_CH340B,
	CHIP_CH339W,
	CHIP_CH9111L_M0,
	CHIP_CH9111L_M1,
	CHIP_CH9114L,
	CHIP_CH9114W,
	CHIP_CH9114F,
	CHIP_CH346C_M0,
	CHIP_CH346C_M1,
	CHIP_CH346C_M2,
	CHIP_CH9105W,
	CHIP_CH9433K,

	CHIP_CH9344L = CHIPBASE_TTYCH9344_BASE,
	CHIP_CH9344Q,
	CHIP_CH348L,
	CHIP_CH348Q,
	
	CHIP_CH341 = CHIPBASE_TTYCH341_BASE,
} CHIP_TYPE;
#define ENUM_CHIP_TYPE
#endif

/**
 * CH_OPEN_TYPE - Parameter type selection when open the device
 * 
 * @CH_OPEN_BY_SN: Open device by USB Serial Number.
 * @CH_OPEN_BY_MANU_STRING: Open device by USB Manufacturer String.
 * @CH_OPEN_BY_PRODUCT_STRING: Open device by USB Product String.
 * @CH_OPEN_BY_LOCID: Open device by USB Location ID.
 *
 */
typedef enum {
	CH_OPEN_BY_SN = 0,
	CH_OPEN_BY_MANU_STRING,
	CH_OPEN_BY_PRODUCT_STRING,
	CH_OPEN_BY_LOCID,
} CH_OPEN_TYPE;

typedef enum {
	FUNC_UART = 0,
	FUNC_SPI_I2C_GPIO,
	FUNC_JTAG_GPIO,
} CH347FUNCTYPE;

typedef enum {
	TYPE_TTY = 0,
	TYPE_HID,
	TYPE_VENDOR,
} USBTYPE;

/* Device Structure using with APIs */
typedef struct _ch34x_device_info {
	char manufacturer[32]; /* iManufacturer string, if present */
	char product[32]; /* iProduct string, if present */
	char serial[32]; /* iSerialNumber string, if present */
	char location[32]; /* Device USB location string */
	char pathname[32]; /* Device path string */
	struct usb_device_descriptor device_desc; /* USB device descriptor */
	int iIndex; /* Device index number in list */
	CHIP_TYPE ChipType; /* Device chip type */
	CH347FUNCTYPE FuncType; /* Device function type */
	USBTYPE USBType; /* USB device type */
	uint8_t ChipMode; /* Device chip mode */
	int fd; /* Device file descriptor */
	uint16_t fwVer; /* Device firmware version */
	uint32_t MaxSPIFreq; /* Maximum SPI frequency supported by the device */
	uint8_t MaxSCSChannel; /* Maximum SPI SCS channel supported by the device */
	uint8_t MaxUARTChannel; /* Maximum UART channel supported by the device */
	uint8_t MaxGPIOChannel; /* Maximum GPIO channel supported by the device */
	uint8_t MaxPWMChannel; /* Maximum PWM channel supported by the device */
	uint8_t SPI_DefaultCS; /* SPI default chip select */

	/* Miscellaneous parameters, users should not access them */
	pCH_INT_ROUTINE isr_routine; /* GPIO interrupt callback function */
	pthread_mutex_t mutex; /* IO mutex */

	/* The following private parameters are used for CH347F/T, users should not access them */
	bool bNewVersion; /* used for CH347T version */
	uint8_t iClockIndex; /* used for CH347F/T non-standard SPI clock */
	uint32_t iWriteTimeout; /* used for CH347T HID mode USB write timeout */
	uint32_t iReadTimeout; /* used for CH347T HID mode USB read timeout */

	uint16_t SPI_CsToDataDelay; /* SPI communication delay between chip select activate with data start, unit: mircosecond */
	uint16_t SPI_DataToCsDelay; /* SPI communication delay between data transmit over with chip select deactivate, unit: mircosecond */

	uint32_t I2C_Clock; /* I2C clock frequency, maximum not exceeding 1MHz */
	uint8_t I2C_Disable_Stretch; /* I2C clock stretching enabled: 0: enabled; 1: disabled */
	uint8_t I2C_DriverMode; /* I2C signal drive mode selection, 0: open-drain; 1: push-pull */
	uint8_t I2C_Continue_OnNACK; /* Controlling whether I2C transmission continues when a device NACK is encountered, 0: stop; 1: continue */
	uint32_t I2C_WriteDelay_9thClock; /* I2C write delay setting between the 8th clock edge and the 9th clock edge, unit: us */
	uint32_t I2C_ReadDelay_9thClock; /* I2C read delay setting between the 8th clock edge and the 9th clock edge, unit: us */
	uint8_t I2C_Stretch_Timeout; /* I2C clock stretching SCL detection timeout setting, unit: 100 us */

	uint8_t UART_Channel; /* UART channel number, 0-7: UART0-7 */

	uint32_t CMDPKT_DATA_MAX_BITS; /* Private varible for JTAG */
	uint32_t MaxBitsPerBulk; /* Private varible for JTAG */
	uint32_t MaxBytesPerBulk; /* Private varible for JTAG */
	uint32_t CmdDataMaxSize; /* Private varible for JTAG */

	/* The following private parameters are used for CH341, users should not access them  */
	uint8_t SPI_BitOrder; /* SPI byte transmission bit order setting, 0: MSB; 1: LSB */
	uint8_t StreamMode; /* SPI/I2C stream mode */

	uint32_t CH341_I2C_Clock; /* I2C clock frequency, maximum not exceeding 750KHz */
	uint8_t CH341_GPIO_Direction
		[15]; /* GPIO direction setting, 0: input; 1: output */
	uint32_t CH341_GPIO_EnableVal;
	uint32_t CH341_GPIO_DirVal;
	uint32_t CH341_GPIO_OutVal;

	/* The following private parameters are used for CH9341, users should not access them */
	uint8_t I2C_10BitAddr; /* I2C address mode, 0: 7-bit address; 1: 10-bit address; */
	uint8_t I2C_Send_ACK_OnReadOver; /* When reading the last byte of data by I2C, the host sends NACK or ACK, 0: send NACK; 1: send ACK */

	bool spi_inbatch; /* SPI batch operation switch */
	struct list_head usb_cmd_list_used; /* batch operation list */
	struct list_head usb_cmd_list_free; /* batch operation list */
} CH34X_DEVICE_INFO;

/* SPI Master Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint32_t SPI_Clock; /* SPI clock frequency */
	uint8_t SPI_OutDefaultData; /* SPI master default transmit bytes */
	uint8_t SPI_Mode; /* SPI mode configuration: 0/1/2/3 */
	uint8_t SPI_Enable_Line3; /* SPI data line configuration, 0: 4-line SPI; 1: 3-line SPI */
	uint8_t SPI_DataBits; /* SPI data bit setting, 0: 8-bit data; 1: 16-bit data */
	uint8_t SPI_BitOrder; /* SPI byte transmission bit order setting, 0: MSB; 1: LSB */
	uint8_t SPI_CS0Init; /* SPI chip select SCS0 initialization switch, 0: no initialization, 1: initialize to default polarity */
	uint8_t SPI_CS1Init; /* SPI chip select SCS1 initialization switch, 0: no initialization, 1: initialize to default polarity */
	uint8_t SPI_CS2Init; /* SPI chip select SCS2 initialization switch, 0: no initialization, 1: initialize to default polarity */
	uint8_t SPI_CS3Init; /* SPI chip select SCS3 initialization switch, 0: no initialization, 1: initialize to default polarity */
	uint8_t SPI_CS0Polar; /* SPI chip select SCS0 polarity setting, 0: active low, 1: active high */
	uint8_t SPI_CS1Polar; /* SPI chip select SCS1 polarity setting, 0: active low, 1: active high */
	uint8_t SPI_CS2Polar; /* SPI chip select SCS2 polarity setting, 0: active low, 1: active high */
	uint8_t SPI_CS3Polar; /* SPI chip select SCS3 polarity setting, 0: active low, 1: active high */
	uint16_t SPI_DataToDataDelay; /* SPI communication delay between each two bytes, unit: mircosecond */
	uint16_t SPI_CsToDataDelay; /* SPI communication delay between chip select activate with data start, unit: mircosecond */
	uint16_t SPI_DataToCsDelay; /* SPI communication delay between data transmit over with chip select deactivate, unit: mircosecond */
	uint8_t Reserved[10]; /* Bytes reserved */
} mSPI_Master_Cfg, *pSPI_Master_Cfg;

/* SPI Slave Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint8_t SPI_Enable_Line3; /* SPI data line configuration, 0: 4-line SPI; 1: 3-line SPI */
	uint8_t SPI_DataBits; /* SPI data bit setting, 0: 8-bit data; 1: 16-bit data */
	uint8_t SPI_BitOrder; /* SPI byte transmission bit order setting, 0: MSB; 1: LSB */
	uint8_t SPI_CS0Polar; /* SPI chip select SCS0 polarity setting, 0: active low, 1: active high */
	uint8_t SPI_Slave_Read_TimeOut; /* Receive timeout setting in SPI slave mode, unit: 100 us */
	uint8_t Reserved[10]; /* Bytes reserved */
} mSPI_Slave_Cfg, *pSPI_Slave_Cfg;

/* I2C Master Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint32_t I2C_Clock; /* I2C clock frequency, maximum not exceeding 1MHz */
	uint8_t I2C_10BitAddr; /* I2C address mode, 0: 7-bit address; 1: 10-bit address */
	uint8_t I2C_Enable_SoftMode; /* Enable I2C software driver mode, 0: hardware I2C; 1: software I2C */
	uint8_t I2C_Enable_SMBus; /* Enable SMBus function, 0: I2C function; 1: SMBus function */
	uint8_t I2C_Disable_Stretch; /* I2C clock stretching enabled: 0: enabled; 1: disabled */
	uint8_t I2C_Send_ACK_OnReadOver; /* When reading the last byte of data by I2C, the host sends NACK or ACK, 0: send NACK; 1: send ACK */
	uint8_t I2C_Stretch_Timeout; /* I2C clock stretching SCL detection timeout setting, unit: 100 us */
	uint8_t I2C_DutyCycle; /* I2C clock duty cycle setting, 0: 16/9, 1: 2/1, i.e., Tlow/Thigh */
	uint8_t I2C_DriverMode; /* I2C signal drive mode selection, 0: open-drain; 1: push-pull */
	uint8_t I2C_Continue_OnNACK; /* Controlling whether I2C transmission continues when a device NACK is encountered, 0: stop; 1: continue */
	uint32_t I2C_WriteDelay_9thClock; /* I2C write delay setting between the 8th clock edge and the 9th clock edge, unit: us */
	uint32_t I2C_ReadDelay_9thClock; /* I2C read delay setting between the 8th clock edge and the 9th clock edge, unit: us */
	uint8_t Reserved[10]; /* Bytes reserved */
} mI2C_Master_Cfg, *pI2C_Master_Cfg;

/* I2C Slave Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint8_t I2C_Slave_Read_TimeOut; /* Receive timeout setting in I2C slave mode, unit: 100 us */
	uint8_t I2C_AddrMode; /* I2C slave address mode, 0: 7-bit address; 1: 10-bit address */
	uint16_t I2C_Slave_Addr; /* I2C slave device address */
	uint8_t I2C_Slave_Reg; /* I2C slave data register */
	uint8_t Reserved[10]; /* Bytes reserved */
} mI2C_Slave_Cfg, *pI2C_Slave_Cfg;

/* UART Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint8_t UART_Channel; /* UART channel number, 0-7: UART0-7, read-only */
	uint32_t UART_BaudRate; /* UART baud rate */
#define USB_CH34X_1_STOP_BITS 0
#define USB_CH34X_1_5_STOP_BITS 1
#define USB_CH34X_2_STOP_BITS 2
	uint8_t UART_StopBits; /* UART stop bits, 0: 1 stop bit, 1: 1.5 stop bits, 2: 2 stop bits */
#define USB_CH34X_NO_PARITY 0
#define USB_CH34X_ODD_PARITY 1
#define USB_CH34X_EVEN_PARITY 2
#define USB_CH34X_MARK_PARITY 3
#define USB_CH34X_SPACE_PARITY 4
	uint8_t UART_Parity; /* UART parity, 0: none, 1: odd, 2: even, 3: mark, 4: space */
#define USB_CH34X_5_DATA_BITS 5
#define USB_CH34X_6_DATA_BITS 6
#define USB_CH34X_7_DATA_BITS 7
#define USB_CH34X_8_DATA_BITS 8
#define USB_CH34X_16_DATA_BITS 16
	uint8_t UART_DataBits; /* UART data bits, 5: 5 bits, 6: 6 bits, 7: 7 bits, 8: 8 bits, 16: 16 bits */
#define USB_CH34X_NO_FlowControl 0
#define USB_CH34X_HW_FlowControl 1
#define USB_CH34X_SW_FlowControl 2
	uint8_t UART_FlowControl; /* UART flow control, 0: none, 1: hardware flow control, 2: software flow control */
	uint32_t UART_ReceiveTimeout; /* UART receive timeout, unit: 100ms */
} mUART_Cfg, *pUART_Cfg;

/* GPIO Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint8_t GPIO_Channel; /* GPIO channel number */
	uint8_t GPIO_Direction; /* GPIO direction setting, 0: input, 1: output */
	union {
		uint8_t GPIO_InputMode; /* GPIO input mode setting, 0: floating, 1: pull-up input, 2: pull-down input */
		uint8_t GPIO_OutputMode; /* GPIO output mode setting, 0: push-pull output, 1: open-drain output */
	} GPIO_Mode;
	uint8_t GPIO_PinLevel; /* GPIO pin output level state, 0: low, 1: high */
	uint8_t GPIO_WakeUpEnable; /* GPIO wake-up function enabled, 0: disabled, 1: enabled */
	uint8_t GPIO_InterruptEnable; /* GPIO interrupt function enabled, 0: disabled, 1: enabled */
	uint8_t GPIO_InterruptMode; /* GPIO interrupt mode configuration, 0: falling edge, 1: rising edge, 2: low level, 3: high level, 4: both edges */
	uint8_t GPIO_FilterDelay; /* GPIO delay filter time, in us. This setting applies to all input GPIOs simultaneously */
	uint8_t GPIO_DefaultEnable; /* Enables GPIO power-down save function setting; 0: ignore this setting, 1: enable power-down retention, 2: disable power-down retention */
	uint8_t Reserved[10]; /* Bytes reserved */
} mGPIO_Cfg, *pGPIO_Cfg;

/* PWM Structure using with APIs */
typedef struct __attribute__((packed)) {
	uint8_t PWM_Channel; /* PWM channel number */
	uint8_t PWM_Polarity; /* PWM polarity control, 0: active low, 1: active high */
	uint8_t PWM_Start; /* PWM output switch, 0: PWM output off, 1: PWM output on */
	uint32_t PWM_Cycle; /* PWM output cycle number setting, PWM output time = cycle number * clock cycle, CH9341 clock cycle: 1/96 us */
	uint32_t PWM_DutyCycle; /* PWM output duty cycle setting, duty cycle = PWM_DutyCycle/PWM_Cycle */
	uint16_t PWM_RepeatTimes; /* PWM output repeat times setting, 0 indicates continuous output */
	uint8_t Reserved[10]; /* Bytes reserved */
} mPWM_Cfg, *pPWM_Cfg;

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************/
/******************** Device Management APIs *********************/
/*****************************************************************/

/**
 * @brief Gets device operating mode.
 * 
 * @param fd File descriptor of device.
 * @param pMode Pointer to chip operating mode, for chips with only one operating mode, this mode value is 0.
 * 
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetDeviceMode(int fd, uint8_t *pMode);

/**
 * @brief Sets device operating mode.
 * 
 * @param fd File descriptor of device.
 * @param iMode Chip operating mode.
 *
 * @remark This function is only available for CH346C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SetDeviceMode(int fd, uint8_t iMode);

/**
 * @brief Gets the information list and device count of all devices
 *     currently connected to the system, after successful execution, you can traverse
 *     each element of the array pointed to by pInfoList to obtain detailed information
 *     about each device.
 * 
 * @param pInfoList Pointer to an array that stores all device information structures.
 *     The array must be allocated in advance by the application program.
 * @param pNumDevs Pointer to device count.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C
 *     and all serial port devices which named "/dev/ttyCH*".
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetDeviceInfoList(CH34X_DEVICE_INFO *pInfoList,
				   uint8_t *pNumDevs);

/**
 * @brief Gets the relevant information of the specified device
 *     connected to the system.
 *     When the device connection status changes, you need to call the
 *     CH34X_GetDeviceInfoList function again.
 * 
 * @param iIndex Device index number of device list.
 * @param pInfo Pointer to device information structure.
 * 
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C
 *     and all serial port devices which named "/dev/ttyCH*".
 *
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetDeviceInfo(int iIndex, CH34X_DEVICE_INFO *pInfo);

/**
 * @brief Gets device index number from global device information array via file descriptor.
 * 
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C
 *     and all serial port devices which named "/dev/ttyCH*".
 *
 * @return Device index number if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetDeviceIndex(int fd);

/*****************************************************************/
/************* Device Open/Close And Hotplug APIs ****************/
/*****************************************************************/

/**
 * @brief Open device with device iIndex number.
 *     If successful, the device handle is returned for subsequent operations.
 * 
 * @param iIndex The iIndex number of the connected USB device.
 *     This value can be obtained from CHXX_GetDeviceInfoList.
 *     Users should use the iIndex member of pInfoList.
 *
 * @return Positive file descriptor if successful. Otherwise, a negative error number.
 */
extern int CH34X_OpenDevice(int iIndex);

/**
 * @brief Open device with device information string.
 *     If successful, the device handle is returned for subsequent operations.
 * 
 * @param type Refer to CH_OPEN_TYPE.
 * @param pArg Pointer to USB string, whose type depends on the type parameter,
 *        must be a Pointer to a null-terminated string.
 * 
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C
 *     and all serial port devices which named "/dev/ttyCH*".
 *
 * @return Positive file descriptor if successful. Otherwise, a negative error number.
 */
extern int CH34X_OpenDeviceEx(CH_OPEN_TYPE type, void *pArg);

/**
 * @brief Close device.
 * 
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C
 *     and all serial port devices which named "/dev/ttyCH*".
 *
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_CloseDevice(int fd);

/**
 * @brief Perform a USB bus reset on the device.
 * 
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for devices in vendor mode which named "/dev/ch34x_pis*".
 *
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_ResetDevice(int fd);

/**
 * @brief USB hotplug detection function setting.
 * 
 * @param iNotifyRoutine The handler to call when device USB hotplug event occurs, 
 *     if it is NULL, the routine will be ignored.
 * 
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C
 *     and all serial port devices which named "/dev/ttyCH*".
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SetDeviceNotify(pCH_NOTIFY_ROUTINE iNotifyRoutine);

/*****************************************************************/
/********************* Miscellaneous  APIs ***********************/
/*****************************************************************/

/**
 * @brief Gets device driver version.
 * 
 * @param fd File descriptor of device.
 * @param pVersion Pointer to version string, the buffer memory size is
 *     recommended no less than 20 bytes.
 *
 * @remark This function is only available for devices in vendor mode which named "/dev/ch34x_pis*".
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetDriverVersion(int fd, char *pVersion);

/**
 * @brief Gets library version.
 * 
 * @param pVersion Pointer to version string, the buffer memory size is
 *     recommended no less than 20 bytes.
 * 
 * @remark This function is only available for devices in vendor mode which named "/dev/ch34x_pis*".
 *
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetLibraryVersion(char *pVersion);

/**
 * @brief Set the USB device read and write timeout.
 * 
 * @param iWriteTimeout The USB write data timeout in milliseconds. 0 means no timeout,
 *     and the function blocks until the operation is completed or cancled.
 * @param iReadTimeout The USB read data timeout in milliseconds. 0 means no timeout,
 *     and the function blocks until the operation is completed or cancled.
 * 
 * @remark This function is available for devices in vendor mode which named "/dev/ch34x_pis*"
 *     and CH347T devices in HID mode which named "/dev/hidraw*".
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SetUSBTimeout(int fd, uint32_t iWriteTimeout,
			       uint32_t iReadTimeout);

/*****************************************************************/
/******************* SPI Master Relevant APIs ********************/
/*****************************************************************/

/**
 * @brief Gets device SPI master configuration.
 * 
 * @param fd File descriptor of device.
 * @param pSPIMasterCfg Pointer to user SPI master configuration structure.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_SPIMaster(int fd, mSPI_Master_Cfg *pSPIMasterCfg);

/**
 * @brief SPI master controller initialization.
 * 
 * @param fd File descriptor of device.
 * @param pSPIMasterCfg Pointer to SPI master configuration structure.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPIMasterInit(int fd, mSPI_Master_Cfg *pSPIMasterCfg);

/**
 * @brief Set the default SPI chip select channel, after calling API function CH34X_SPIMasterInit,
 *     this API function should be called to specify the chip select channel.
 *     After using this API function, subsequent SPI read and write functions will default to
 *     operating on this chip select channel.
 * 
 * @param fd File descriptor of device.
 * @param iCSChannel SCSx channel, 0: SCS0, 1: SCS1, 2: SCS2, 3: SCS3.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISetDefaultCS(int fd, uint8_t iCSChannel);

/**
 * @brief Manual SPI chip select control, this API is suitable for applications 
 *     requiring flexible chip select control or operating multiple SPI slaves.
 *     This function is optional.
 * 
 * @param fd File descriptor of device.
 * @param iCSChannel SCSx channel, 0: SCS0, 1: SCS1, 2: SCS2, 3: SCS3.
 * @param iCSActivate Chipselect activate switch, 0: deactivate, 1: activate.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPIChangeCS(int fd, uint8_t iCSChannel,
			     uint8_t iCSActivate);

/**
 * @brief SPI fast write operation is suitable for applications
 *     that require only unidirectional write operation. 
 *     The bCSAutoStart and bCSAutoStop parameters are used to control whether the SPI
 *     transfer automatically controls the chip select. When manually controlling the
 *     chip select using the CHXX_SPIChangeCS this automatic control must be disabled.
 * 
 * @param fd File descriptor of device.
 * @param bCSAutoStart automatic chip select control before SPI transmission starts, false: no control, true: control.
 * @param bCSAutoStop automatic chip select control after SPI transmission ends, false: no control, true: control.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPIFastWrite(int fd, bool bCSAutoStart, bool bCSAutoStop,
			      void *iWriteBuffer, uint32_t iWriteLength);

/**
 * @brief SPI fast read operation is suitable for applications
 *     that require only unidirectional reading or mainly focus on reading data.
 *     The bCSAutoStart and bCSAutoStop parameters are used to control whether the SPI
 *     transfer automatically controls the chip select. When manually controlling the
 *     chip select using the CHXX_SPIChangeCS this automatic control must be disabled.
 *     If no data needs to be written, the parameter iWriteLength must be zero.
 * 
 * @param fd File descriptor of device.
 * @param bCSAutoStart automatic chip select control before SPI transmission starts, false: no control, true: control.
 * @param bCSAutoStop automatic chip select control after SPI transmission ends, false: no control, true: control.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 0~64KiB.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPIFastRead(int fd, bool bCSAutoStart, bool bCSAutoStop,
			     void *iWriteBuffer, uint32_t iWriteLength,
			     void *oReadBuffer, uint32_t oReadLength);

/**
 * @brief SPI general read and write operations are suitable for general applications
 *     other than fast read and write.
 *     This function writes data from the ioBuffer buffer and writes ioLength bytes, and attempt to reads
 *     back ioLength bytes of data at the same time.
 *     After the operation is completed, the read SPI data is stored in the ioBuffer buffer.
 *     The bCSAutoStart and bCSAutoStop parameters are used to control whether the SPI
 *     transfer automatically controls the chip select. When manually controlling the
 *     chip select using the CHXX_SPIChangeCS this automatic control must be disabled.
 *     
 * @param fd File descriptor of device.
 * @param bCSAutoStart Automatic chip select control before SPI transmission starts, false: no control, true: control.
 * @param bCSAutoStop Automatic chip select control after SPI transmission ends, false: no control, true: control.
 * @param ioBuffer Pointer to write and read buffer, the buffer size of which is no less than ioLength bytes.
 * @param ioLength Number of bytes of data to be written and read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPIWriteRead(int fd, bool bCSAutoStart, bool bCSAutoStop,
			      void *ioBuffer, uint32_t ioLength);

/**
 * @brief SPI auto read operation which is suitable for applications
 *     where polling and reading are required to query the SPI slave device status, such as FLASH.
 *     This function writes data from the iWriteBuffer buffer and writes iWriteLength bytes, and reads back 
 *     oReadLength bytes of data, then the read data are logically ANDed with each byte of maskBuffer.
 *     If the result matches the content of targetBuffer, the execution is successful.
 *     The chip will automatically retry iCount times at the iInterval time until the execution
 *     succeeds or times out.
 *     
 *     The bCSAutoStart and bCSAutoStop parameters are used to control whether the SPI
 *     transfer automatically controls the chip select. When manually controlling the
 *     chip select using the CHXX_SPIChangeCS this automatic control must be disabled.
 *     
 * @param fd File descriptor of device.
 * @param bCSAutoStart Automatic chip select control before SPI transmission starts, false: no control, true: control.
 * @param bCSAutoStop Automatic chip select control after SPI transmission ends, false: no control, true: control.
 * @param iCount SPI automatic retry times.
 * @param iInterval SPI automatic retry interval, unit: us.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length of iWriteLength plus oReadLength: 1~64bytes.
 * @param maskBuffer Pointer to mask buffer, the SPI read data will be logically ANDed with the data.
 * @param targetBuffer Pointer to target buffer, for comparing the data which ANDed above, when the function returns,
 *     this buffer will store the data last read from the SPI.
 * @param oReadLength Number of bytes of data to be read, valid length of iWriteLength plus oReadLength: 1~64bytes.
 * 
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPIReadQuery(int fd, bool bCSAutoStart, bool bCSAutoStop,
			      uint16_t iCount, uint8_t iInterval,
			      void *iWriteBuffer, uint32_t iWriteLength,
			      void *maskBuffer, void *targetBuffer,
			      uint32_t oReadLength);

/*****************************************************************/
/******************** SPI Slave Relevant APIs ********************/
/*****************************************************************/

/**
 * @brief Gets device SPI slave configuration.
 * 
 * @param fd File descriptor of device.
 * @param pSPISlaveCfg Pointer to user SPI slave configuration structure.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_SPISlave(int fd, mSPI_Slave_Cfg *pSPISlaveCfg);

/**
 * @brief SPI slave controller initialization.
 * 
 * @param fd File descriptor of device.
 * @param pSPISlaveCfg Pointer to SPI slave configuration structure.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISlaveInit(int fd, mSPI_Slave_Cfg *pSPISlaveCfg);

/**
 * @brief Switch of read SPI data from SPI master to chip.
 *     Once enabled, the driver will continuously read data and cache it in the FIFO.
 * 
 * @param fd File descriptor of device.
 * @param bEnable Slave continuous read operation switch, false: disable, true: enable.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISlaveControl(int fd, bool bEnable);

/**
 * @brief Write SPI data to chip, the data will be cached in chip waiting SPI master to read.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISlaveWrite(int fd, void *iWriteBuffer,
			       uint32_t iWriteLength);

/**
 * @brief Clear SPI data fifo cached in the driver.
 * 
 * @param fd File descriptor of device.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISlaveClear(int fd);

/**
 * @brief Query the SPI data fifo length cached in the driver.
 * 
 * @param fd File descriptor of device.
 * @param oReadLength Pointer to number of bytes of data cached in the driver, valid length: 0~1MiB.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISlaveQueryRead(int fd, uint32_t *oReadLength);

/**
 * @brief Read the SPI data fifo cached in the driver, after the function executes successfully,
 *     oReadLength will store the actual data length.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than *oReadLength bytes.
 * @param oReadLength Pointer to number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH346C/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SPISlaveRead(int fd, void *oReadBuffer,
			      uint32_t *oReadLength);

/*****************************************************************/
/******************* I2C Master Relevant APIs ********************/
/*****************************************************************/

/**
 * @brief Gets device I2C master configuration.
 * 
 * @param fd File descriptor of device.
 * @param pI2CMaster Pointer to user I2C master configuration structure.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_I2CMaster(int fd, mI2C_Master_Cfg *pI2CMaster);

/**
 * @brief I2C master controller initialization.
 * 
 * @param fd File descriptor of device.
 * @param pI2CMasterCfg Pointer to I2C master configuration structure.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CMasterInit(int fd, mI2C_Master_Cfg *pI2CMasterCfg);

/**
 * @brief Write/Read I2C in stream mode
 *     The bI2CStart and bI2CStop parameters are used to control whether the I2C
 *     transfer automatically issue the I2C Start or Stop.
 * 
 * @param fd File descriptor of device.
 * @param bI2CStart Issue I2C Start before I2C transmission starts, false: no control, true: control.
 * @param bI2CStop Issue I2C Stop after I2C transmission ends, false: no control, true: control.
 * @param iWriteReadDelay Interval between I2C write and read, unit: us.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 *                The first byte or the first two bytes are usually the I2C device address (including read/write direction bit),
 *                and the subsequent bytes are the address of the operation register.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 0~64KiB. This parameter should be set to 0
 *           when only data needs to be written to the I2C slave.
 * @param iAckCount Pointer to the number of ACK bits for the I2C write operation.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/T/CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_StreamI2C(int fd, bool bI2CStart, bool bI2CStop,
			   uint16_t iWriteReadDelay, void *iWriteBuffer,
			   uint32_t iWriteLength, void *oReadBuffer,
			   uint32_t oReadLength, uint32_t *iAckCount);

/*****************************************************************/
/******************** I2C Slave Relevant APIs ********************/
/*****************************************************************/

/**
 * @brief Gets device I2C slave configuration.
 * 
 * @param fd File descriptor of device.
 * @param pI2CSlave Pointer to user I2C slave configuration structure.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_I2CSlave(int fd, mI2C_Slave_Cfg *pI2CSlave);

/**
 * @brief I2C slave controller initialization.
 * 
 * @param fd File descriptor of device.
 * @param pI2CSlaveCfg Pointer to I2C slave configuration structure.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CSlaveInit(int fd, mI2C_Slave_Cfg *pI2CSlaveCfg);

/**
 * @brief Switch of read I2C data from I2C master to chip.
 *     Once enabled, the driver will continuously read data and cache it in the FIFO.
 * 
 * @param fd File descriptor of device.
 * @param bEnable Slave continuous read operation switch, false: disable, true: enable.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CSlaveControl(int fd, bool bEnable);

/**
 * @brief Write I2C data to chip via USB, the data will be cached in chip waiting I2C master to read.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CSlaveWrite(int fd, void *iWriteBuffer,
			       uint32_t iWriteLength);

/**
 * @brief Clear I2C data fifo cached in the driver.
 * 
 * @param fd File descriptor of device.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CSlaveClear(int fd);

/**
 * @brief Query the I2C data fifo length cached in the driver,
 *     when using the CHXX_I2CSlaveRead interface, it is recommended to query this API 
 *     before reading I2C data cached in the driver each time.
 * 
 * @param fd File descriptor of device.
 * @param oReadLength Pointer to number of bytes of data cached in the driver, valid length: 0~1MiB.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CSlaveQueryRead(int fd, uint32_t *oReadLength);

/**
 * @brief Read the I2C data fifo cached in the driver, after the function executes successfully,
 *     oReadLength will store the actual data length.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than *oReadLength bytes.
 * @param oReadLength Pointer to number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_I2CSlaveRead(int fd, void *oReadBuffer,
			      uint32_t *oReadLength);

/*****************************************************************/
/********************** UART Relevant APIs ***********************/
/*****************************************************************/

/**
 * @brief Gets configuration of UART.
 * 
 * @param fd File descriptor of device.
 * @param pUARTCfg Pointer to UART user configuration structure.
 * 
 * @remark This function is only available for CH347T in HID mode which named "/dev/hidraw*" and
 *     CH34X series in TTY mode which named "/dev/ttyCH*".
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_UART(int fd, mUART_Cfg *pUARTCfg);

/**
 * @brief Initialization of UART.
 * 
 * @param fd File descriptor of device.
 * @param pUARTCfg Pointer to UART user configuration structure.
 *
 * @remark This function is only available for CH347T in HID mode which named "/dev/hidraw*" and
 *     CH34X series in TTY mode which named "/dev/ttyCH*".
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_UARTInit(int fd, mUART_Cfg *pUARTCfg);

/**
 * @brief Read data from UART.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~64KiB.
 * 
 * @remark This function is only available for CH347T in HID mode which named "/dev/hidraw*" and
 *     CH34X series in TTY mode which named "/dev/ttyCH*".
 * 
 * @return The actual length if successful. Otherwise, a negative error number.
 */
extern int CH34X_UARTRead(int fd, void *oReadBuffer, uint32_t oReadLength);

/**
 * @brief Write data to UART.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 * 
 * @remark This function is only available for CH347T in HID mode which named "/dev/hidraw*" and
 *     CH34X series in TTY mode which named "/dev/ttyCH*".
 * 
 * @return The actual length if successful. Otherwise, a negative error number.
 */
extern int CH34X_UARTWrite(int fd, void *iWriteBuffer,
			   uint32_t iWriteLength);

/*****************************************************************/
/********************** GPIO Relevant APIs ***********************/
/*****************************************************************/

/**
 * @brief Gets all GPIO configurations for the device.
 * 
 * @param fd File descriptor of device.
 * @param pGPIOCfgArray Pointer to GPIO user configurations structure array.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C/CH9346W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_GPIO(int fd, mGPIO_Cfg *pGPIOCfgArray);

/**
 * @brief GPIO initialization, supports single or multiple GPIOs.
 * 
 * @param fd File descriptor of device.
 * @param iGPIOChannel GPIO channel, BITx represents GPIOx, where x represents the channel number. BITx is 0: ignore, 1: set.
 * @param pGPIOCfgArray Pointer to GPIO user configurations structure array.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C/CH9346W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GPIOInit(int fd, uint64_t iGPIOChannel,
			  mGPIO_Cfg *pGPIOCfgArray);

/**
 * @brief Controls GPIO output high or low level,
 *     supports single or multiple GPIOs.
 * 
 * @param fd File descriptor of device.
 * @param iGPIOChannel GPIO channel, BITx represents GPIOx, where x represents the channel number.
 *     BITx is 0: ignore, 1: set.
 * @param iGPIOLevel GPIO output level, BITx represents GPIOx, where x represents the channel number.
 *     BITx is 0: low, 1: high.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C/CH9346W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GPIOSet(int fd, uint64_t iGPIOChannel,
			 uint64_t iGPIOLevel);

/**
 * @brief Gets the level status of all GPIO.
 * 
 * @param fd File descriptor of device.
 * @param iGpioLevel Pointer to GPIO level, BITx represents GPIOx, where x represents the channel number.
 *     BITx is 0: low, 1: high.
 *
 * @remark This function is only available for CH347F/T/CH339W/CH341F/A/B/CH9341C/CH9346W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GPIOGet(int fd, uint64_t *iGpioLevel);

/**
 * @brief GPIO interrupt function setting.
 * 
 * @param fd File descriptor of device.
 * @param iGPIOIndex Device GPIO iIndex number, 0 ≤ iGPIOIndex < MaxGPIOChannel of the chip.
 * @param iIntRoutine handler to call when interrupt occurs, if it is NULL,
 *     indicates that the interrupt function has been cancelled.
 * 
 * @remark This function is only available for CH347F/T/CH9341C devices. 
 *
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GPIOSetIntRoutine(int fd, uint8_t iGPIOIndex,
				   pCH_INT_ROUTINE iIntRoutine);

/*****************************************************************/
/*********************** PWM Relevant APIs ***********************/
/*****************************************************************/

/**
 * @brief Gets PWM controller configuration of the specified PWM channel.
 * 
 * @param fd File descriptor of device.
 * @param iPWMChannel PWM channel number.
 * @param pPWMCfg Pointer to PWM user configuration structure.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_GetInfo_PWM(int fd, uint8_t iPWMChannel,
			     mPWM_Cfg *pPWMCfg);

/**
 * @brief Initialization of the specified PWM channel.
 * 
 * @param fd File descriptor of device.
 * @param iPWMChannel PWM channel number.
 * @param pPWMCfg Pointer to PWM user configuration structure.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_PWMInit(int fd, uint8_t iPWMChannel, mPWM_Cfg *pPWMCfg);

/**
 * @brief Controls the start or stop of the output of the specified PWM channel.
 * 
 * @param fd File descriptor of device.
 * @param iPWMChannel PWM channel number.
 * @param bStart PWM output control switch, false: stop, true: start.
 *
 * @remark This function is only available for CH9341C devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_PWMControl(int fd, uint8_t iPWMChannel, bool bStart);

/*****************************************************************/
/*************** FIFO Slave Operation Relevant APIs **************/
/*****************************************************************/

/**
 * @brief FIFO slave controller initialization.
 * 
 * @param fd File descriptor of device.
 *
 * @remark This function is only available for CH346/CH9346 devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_FIFOSlaveInit(int fd);

/**
 * @brief Switch of read FIFO data from FIFO master to chip.
 *     Once enabled, the driver will continuously read data and cache it in the FIFO.
 * 
 * @param fd File descriptor of device.
 * @param bEnable Slave continuous read operation switch, false: disable, true: enable.
 *
 * @remark This function is only available for CH346/CH9346 devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_FIFOSlaveControl(int fd, bool bEnable);

/**
 * @brief Write FIFO data to chip via USB, the data will be cached in chip waiting FIFO master to read.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH346/CH9346 devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_FIFOSlaveWrite(int fd, void *iWriteBuffer,
				uint32_t iWriteLength);

/**
 * @brief Clear FIFO data fifo cached in the driver.
 * 
 * @param fd File descriptor of device.
 *
 * @remark This function is only available for CH346/CH9346 devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_FIFOSlaveClear(int fd);

/**
 * @brief Query the FIFO data fifo length cached in the driver,
 *     when using the CH34X_FIFOSlaveRead interface, it is recommended to query this API 
 *     before reading FIFO data cached in the driver each time.
 * 
 * @param fd File descriptor of device.
 * @param oReadLength Pointer to number of bytes of data cached in the driver, valid length: 0~1MiB.
 *
 * @remark This function is only available for CH346/CH9346 devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_FIFOSlaveQueryRead(int fd, uint32_t *oReadLength);

/**
 * @brief Read the FIFO data fifo cached in the driver, after the function executes successfully,
 *     oReadLength will store the actual data length.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than *oReadLength bytes.
 * @param oReadLength Pointer to number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH346/CH9346 devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_FIFOSlaveRead(int fd, void *oReadBuffer,
			       uint32_t *oReadLength);

/*****************************************************************/
/********** MEM/EPP Parallel Port Operation Relevant APIs ********/
/*****************************************************************/

/**
 * @brief Set device parallel port working mode.
 * 
 * @param fd File descriptor of device.
 * @param iMode Parallel port working mode, 0x00: EPP mode V1.7,0x01: EPP mode V1.9,
 *     0x02: MEM mode
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_SetParallelMode(int fd, uint8_t iMode);

/**
 * @brief Reset device parallel port and initialize it, RST# output low pulse.
 * 
 * @param fd File descriptor of device.
 * @param iMode Parallel port working mode, 0x00: EPP mode V1.7, 0x01: EPP mode V1.9,
 *     0x02: MEM mode.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_InitParallel(int fd, uint8_t iMode);

/**
 * @brief Read the parallel data from device in EPP mode, condition: WR#=1, DS#=0, AS#=1, D0-D7=input.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_EPPReadData(int fd, void *oReadBuffer,
			     uint32_t oReadLength);

/**
 * @brief Read the parallel address from device in EPP mode, condition: WR#=1, DS#=1, AS#=0, D0-D7=input.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_EPPReadAddr(int fd, void *oReadBuffer,
			     uint32_t oReadLength);

/**
 * @brief Write the parallel data to device in EPP mode, condition: WR#=0, DS#=0, AS#=1, D0-D7=output.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_EPPWriteData(int fd, void *iWriteBuffer,
			      uint32_t iWriteLength);

/**
 * @brief Write the parallel address to device in EPP mode, condition: WR#=0, DS#=1, AS#=0, D0-D7=output.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_EPPWriteAddr(int fd, void *iWriteBuffer,
			      uint32_t iWriteLength);

/**
 * @brief Setting the parallel address in EPP mode, condition: WR#=0, DS#=1, AS#=0, D0-D7=output
 * 
 * @param fd File descriptor of device.
 * @param iAddress Address in EPP mode.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_EPPSetAddr(int fd, uint8_t iAddress);

/**
 * @brief Read the parallel data from address 0 in MEM mode, condition: WR#=1, DS#/RD#=0, AS#/ADDR=0, D0-D7=input.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_MEMReadAddr0(int fd, void *oReadBuffer,
			      uint32_t oReadLength);

/**
 * @brief Read the parallel data from address 1 in MEM mode, condition: WR#=1, DS#/RD#=0, AS#/ADDR=1, D0-D7=input.
 * 
 * @param fd File descriptor of device.
 * @param oReadBuffer Pointer to the receive data buffer, the buffer size of which is no less than oReadLength bytes.
 * @param oReadLength Number of bytes of data to be read, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_MEMReadAddr1(int fd, void *oReadBuffer,
			      uint32_t oReadLength);

/**
 * @brief Write the parallel data to address 0 in MEM mode, condition: WR#=0, DS#/RD#=1, AS#/ADDR=0, D0-D7=output.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_MEMWriteAddr0(int fd, void *iWriteBuffer,
			       uint32_t iWriteLength);

/**
 * @brief Write the parallel data to address 1 in MEM mode, condition: WR#=0, DS#/RD#=1, AS#/ADDR=1, D0-D7=output.
 * 
 * @param fd File descriptor of device.
 * @param iWriteBuffer Pointer to write buffer, the buffer size of which is no less than iWriteLength bytes.
 * @param iWriteLength Number of bytes of data to be written, valid length: 1~64KiB.
 *
 * @remark This function is only available for CH341F/A/B devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_MEMWriteAddr1(int fd, void *iWriteBuffer,
			       uint32_t iWriteLength);

/*****************************************************************/
/********************** JTAG Relevant APIs ***********************/
/*****************************************************************/

/**
 * @brief Reset Tap Status, more than six consecutive TCK and TMS high levels will set the state machine to the Test-Logic Reset state.
 * 
 * @param fd File descriptor of device.
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGReset(int fd);

/** 
 * @brief Hard-reset JTAG device.
 * 
 * @param fd File descriptor of device.
 * @param TRSTLevel Reset level, false: low, true: high.
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGResetTRST(int fd, bool TRSTLevel);

/**
 * @brief Initialize the JTAG interface, set mode and speed.
 * 
 * @param fd File descriptor of device.
 * @param iClockRate Communication speed, valid value is 0-5, the higher the value, the faster the speed.
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGInit(int fd, uint8_t iClockRate);

/**
 * @brief Get JTAG speed setting.
 * 
 * @param fd File descriptor of device.
 * @param ClockRate Pointer to communication speed, valid value is 0-5, the higher the value, the faster the speed.
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGGetCfg(int fd, uint8_t *ClockRate);

/**
 * @brief Change TMS value on the rising edge of TCK to switch its Tap state.
 * 
 * @param BitBangPkt Protocol package
 * @param Tms TMS value to change
 * @param BI Length of protocol package
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return Length of protocol package.
 */
extern uint32_t CH34X_JTAGChangeTMSOnEdge(uint8_t *BitBangPkt,
					  uint32_t Tms, uint32_t BI);

/**
 * @brief Ensure the clock is in low status.
 * 
 * @param BitBangPkt Protocol package
 * @param BI Length of protocol package
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return Length of protocol package.
 */
extern uint32_t CH34X_JTAGIdleClock(uint8_t *BitBangPkt, uint32_t BI);

/**
 * @brief Change TMS value to switch state.
 * 
 * @param fd File descriptor of device
 * @param tmsValue Pointer to TMS value, unit: byte
 * @param Step The valid bits which stored in tmsValue
 * @param Skip Valid start bit
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGChangeTMS(int fd, uint8_t *tmsValue, uint32_t Step,
			       uint32_t Skip);

/**
 * @brief Read and write in the Shift-DR/IR state, if it is the last package, switch to Exit DR/IR; if not, stop at Shift-DR/IR.
 * 
 * @param fd File descriptor of device
 * @param DataBits Data bits to be transmitted
 * @param DataBitsNb Number of bits to be transmitted
 * @param IsRead Whether to read data
 * @param IsLastPkt Whether it is the last package
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGIOScanT(int fd, uint8_t *DataBits,
			     uint32_t DataBitsNb, bool IsRead,
			     bool IsLastPkt);

/**
 * @brief Read and write in the Shift-DR/IR state, and switch to Exit DR/IR after execution.
 * 
 * @param fd File descriptor of device
 * @param DataBits Data bits to be transmitted
 * @param DataBitsNb Number of bits to be transmitted
 * @param IsRead Whether to read data
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGIOScan(int fd, uint8_t *DataBits, uint32_t DataBitsNb,
			    bool IsRead);

/**
 * @brief Bitband mode JTAG IR/DR data read and write which is applicable for a small amount of data.
 *     Exp: command operation, state machine switching and other control transmission.
 *     For batch data transmission, it is recommended to use CH34X_JTAGFastWriteRead.
 *     Command packets are read and written in batches in 4096 bytes.
 *     State machine: Run-Test -> Shift-IR/DR.. -> exit IR/DR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param IsDR true: DR data read and write, false: IR data read and write
 * @param iWriteBitLength Write length
 * @param iWriteBitBuffer Pointer to write buffer
 * @param oReadBitLength Pointer to read length, returns the actual number of bytes read on success
 * @param oReadBitBuffer Pointer to read buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGWriteRead(int fd, bool IsDR, uint32_t iWriteBitLength,
			       void *iWriteBitBuffer,
			       uint32_t *oReadBitLength,
			       void *oReadBitBuffer);

/**
 * @brief JTAG IR/DR data read and write in batches for multi-byte continuous operation.
 *     Exp: JTAG firmware download operation. Hardware has a 4K buffer, such as write then read, the length should not exceed 4096 bytes.
 *     The buffer size can be adjusted.
 *     State machine: Run-Test -> Shift-IR/DR.. -> exit IR/DR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param IsDR true: DR data read and write, false: IR data read and write
 * @param iWriteLength Write length
 * @param iWriteBuffer Pointer to write buffer
 * @param oReadLength Pointer to read length, returns the actual number of bytes read on success
 * @param oReadBuffer Pointer to read buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGFastWriteRead(int fd, bool IsDR, uint32_t iWriteLength,
				   void *iWriteBuffer,
				   uint32_t *oReadLength,
				   void *oReadBuffer);

/**
 * @brief Switch JTAG state machine.
 * 
 * @param fd File descriptor of device
 * @param TapState Machine state
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGSwitchTAP(int fd, uint8_t TapState);

/**
 * @brief JTAG DR write in bytes which used for multi-byte continuous operation. Exp: JTAG firmware download operation.
 * State machine: Run-Test -> Shift-DR.. -> exit DR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param iWriteLength Write length
 * @param iWriteBuffer Pointer to write buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGWriteDR(int fd, uint32_t iWriteLength, void *iWriteBuffer);

/**
 * @brief JTAG DR read in bytes which used for multi-byte continuous operation.
 * State machine: Run-Test -> Shift-DR.. -> exit DR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param oReadLength Pointer to read length, returns the actual number of bytes read on success
 * @param oReadBuffer Pointer to read buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGReadDR(int fd, uint32_t *oReadLength,
			    void *oReadBuffer);

/**
 * @brief JTAG IR write in bytes which used for multi-byte continuous operation.
 * State machine: Run-Test -> Shift-IR.. -> exit IR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param iWriteLength Write length
 * @param iWriteBuffer Pointer to write buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGWriteIR(int fd, uint32_t iWriteLength, void *iWriteBuffer);

/**
 * @brief JTAG IR read in bytes which used for multi-byte continuous operation.
 * State machine: Run-Test -> Shift-IR.. -> exit IR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param oReadLength Pointer to read length, returns the actual number of bytes read on success
 * @param oReadBuffer Pointer to read buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGReadIR(int fd, uint32_t *oReadLength,
			    void *oReadBuffer);

/**
 * @brief Bitband mode JTAG DR data write which is applicable for a small amount of data.
 *     Exp: command operation, state machine switching and other control transmission.
 *     For batch data transmission, it is recommended to use CH34X_JTAGWriteDR.
 *     State machine: Run-Test -> Shift-DR.. -> exit DR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param iWriteBitLength Pointer to write length
 * @param iWriteBitBuffer Pointer to write buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGBitWriteDR(int fd, uint32_t iWriteBitLength,
				void *iWriteBitBuffer);

/**
 * @brief Bitband mode JTAG IR data write which is applicable for a small amount of data.
 *     Exp: command operation, state machine switching and other control transmission.
 *     For batch data transmission, it is recommended to use CH34X_JTAGWriteIR.
 *     State machine: Run-Test -> Shift-IR.. -> exit IR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param iWriteBitLength Pointer to write length
 * @param iWriteBitBuffer Pointer to write buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGBitWriteIR(int fd, uint32_t iWriteBitLength,
				void *iWriteBitBuffer);

/**
 * @brief Bitband mode JTAG IR data read which is applicable for a small amount of data.
 *     Exp: command operation, state machine switching and other control transmission.
 *     For batch data transmission, it is recommended to use CH34X_JTAGReadIR.
 *     State machine: Run-Test -> Shift-IR.. -> exit IR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param oReadBitLength Pointer to read length, returns the actual number of bytes read on success
 * @param oReadBitBuffer Pointer to read buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGBitReadIR(int fd, uint32_t *oReadBitLength,
			       void *oReadBitBuffer);

/**
 * @brief Bitband mode JTAG DR data read which is applicable for a small amount of data.
 *     Exp: command operation, state machine switching and other control transmission.
 *     For batch data transmission, it is recommended to use CH34X_JTAGReadDR.
 *     State machine: Run-Test -> Shift-DR.. -> exit DR -> Run-Test
 * 
 * @param fd File descriptor of device
 * @param oReadBitLength Pointer to read length, returns the actual number of bytes read on success
 * @param oReadBitBuffer Pointer to read buffer
 * 
 * @remark This function is only available for CH347F/T/CH339W devices.
 * 
 * @return 0 if successful. Otherwise, a negative error number.
 */
extern int CH34X_JTAGBitReadDR(int fd, uint32_t *oReadBitLength,
			       void *oReadBitBuffer);

#ifdef __cplusplus
}
#endif

#endif
