# CH341/CH346/CH347/CH339/CH9341/CH9346 Linux SDK Instruction

## Introduction

This directory consists of 3 parts: the USB device driver, the application library, and sample code. This driver and application support the USB bus interface chips from WCH (Qinheng Microelectronics): CH341, CH346, CH347, CH339, CH9341, CH9346, as well as the WCH USB-to-serial series chips.

- **CH341:** Achieves Full-Speed USB to UART/SPI/I2C/GPIO/printer parallel port, etc.
- **CH346:** Achieves High-Speed USB to FIFO/UART, etc.
- **CH347:** Achieves High-Speed USB to UART/JTAG/SWD/SPI/I2C/GPIO, etc.
- **CH339:** Achieves High-Speed USB Hub / Card Reader / Ethernet, and USB to UART/JTAG/SPI/I2C/GPIO, etc.
- **CH9341:** Achieves Full-Speed USB to SPI/I2C/GPIO/PWM, etc.
- **CH9346:** Achieves SuperSpeed USB to FIFO/UART, etc.

> ⚠️ **Note:** This device driver does not include the Linux TTY serial device driver; it is only used to implement other interface functions such as SPI / I2C / GPIO / PWM. When accessing USB-to-serial devices using the device management and serial APIs provided in this SDK package, you need to load the VCP vendor driver for the serial port.

## Driver Operation Guide

1. Open the "Terminal".
2. Switch to the `driver` directory.
3. Enter `make` to compile the driver. If the compilation is successful, you will see the generated kernel module file `ch34x_pis.ko`.
4. Enter `sudo make load` or `sudo insmod ch34x_pis.ko` to dynamically load the driver.
5. Enter `sudo make unload` or `sudo rmmod ch34x_pis.ko` to unload the driver.
6. Enter `sudo make install` to make the driver persistent (install it into the system).
7. Enter `sudo make uninstall` to uninstall and remove the driver.

Before the driver can function normally, please ensure that the USB device is plugged in and working properly. You can verify this using the terminal commands `lsusb` or `dmesg`. The Vendor ID (VID) for all the aforementioned chips is **[1A86]**.

If the device is working properly, the driver will create a device node named `/dev/ch34x_pis*` after it is loaded.

## Application Operation Guide

1. Copy the dynamic link library from the `lib` directory to the system's default library path:

   ```
   sudo cp libch34x.so /usr/lib
   ```

2. Switch to the `demo` directory.

3. Enter the following command to compile the sample program:

   ```
   gcc ch34x_demo.c eeprom_xx24.c flash_xx25.c -o app -lch34x
   ```

## Notes

The working modes supported by each chip are as follows:

### Chip Working Mode List

| **Chip Model** | **Mode**                             | **Supported Functions and Device Names**                     |
| -------------- | ------------------------------------ | ------------------------------------------------------------ |
| **CH341F/A/B** | Mode 0<br>Mode 1<br>Mode 2           | `[UART]` (Vendor driver mode, device name: `/dev/ttyCH*`)<br> `[SPI + I2C + Parallel Port + GPIO]` (Vendor driver mode, device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`)<br> `[Printer]` (USB Printer class driver mode, device name: `/dev/usb/lp*`) |
| **CH341T**     | Mode 0<br>Mode 1                     | `[UART]` (Vendor driver mode, device name: `/dev/ttyCH*`)<br> `[I2C + GPIO]` (Vendor driver mode, device name: `/dev/ch34x_pis*`) |
| **CH347T**     | Mode 0<br>Mode 1<br>Mode 2<br>Mode 3 | `[UART * 2]` (Vendor driver mode, device name: `/dev/ttyCH*`)<br> `[SPI + I2C + UART + GPIO]` (Vendor driver mode, device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`)<br> `[SPI + I2C + UART]` (HID driver mode, device name: `/dev/hidraw*`)<br> `[JTAG + UART + GPIO]` (Vendor driver mode, device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`) |
| **CH347F**     | Mode 0                               | `[UART * 2 + SPI + I2C + JTAG]` (Vendor driver mode, device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`) |
| **CH339W**     | Multi-Mode                           | Supports `UART/JTAG/SPI/I2C/GPIO` etc., when working in Vendor driver mode, with device names `/dev/ch34x_pis*` and `/dev/ttyCH*`. Please refer to the datasheet for details. |
| **CH346C**     | Mode 0<br>Mode 1<br>Mode 2           | `[FIFO + UART]` (Vendor driver mode, device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`)<br> `[SPI + UART]` (Vendor driver mode, device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`)<br> `[UART]` (VCP driver mode, device name: `/dev/ttyCH*`) |
| **CH9341C**    | Mode 0                               | `[SPI + I2C + GPIO + PWM]` (Device names: `/dev/ch34x_pis*` and `/dev/ttyCH*`) |
| **CH9346W**    | Mode 0<br/>Mode 1<br/>Mode 2         | `[FIFO]` (Vendor driver mode, device name: `/dev/ch34x_pis*`)<br>`[MEDIA + UART]` (UVC class driver mode, device names: `/dev/video*` and `/dev/ttyACM*`)<br> `[DVP + UART]` (UVC class driver mode, device names: `/dev/video*` and `/dev/ttyACM*`) \| |

### Drivers Applicable to Specific Chips and Modes

- **CH341F/A/B/T:** Mode 1
- **CH347T:** Mode 1 / Mode 3
- **CH347F:** Mode 0
- **CH339W:** Certain specific modes
- **CH346C:** Mode 0 / Mode 1
- **CH9341C:** Mode 0
- **CH9346W:** Mode 0

In any other cases, there is no need to load this driver program.

If you have any questions, you can send technical feedback to the email: tech@wch.cn