# ch341/ch347/ch339 linux SDK

## Description

This directory contains 3 parts, usb device driver, application library and examples. This driver and applications support usb bus interface chip ch341/ch347/ch339.

ch341 implements full-speed usb to uart/spi/i2c/gpio/parport/etc., ch347 implements high-speed usb to uart/jtag/swd/spi/i2c/gpio/etc., ch339 implements high-speed usb hub/card reader/ethernet and usb to uart/jtag/spi/i2c/gpio/etc., This driver is not applicable to uart.

## Driver Operating Overview

1. Open "Terminal"
2. Switch to "driver" directory
3. Compile the driver using "make", you will see the module "ch34x_pis.ko" if successful
4. Type "sudo make load" or "sudo insmod ch34x_pis.ko" to load the driver dynamically
5. Type "sudo make unload" or "sudo rmmod ch34x_pis.ko" to unload the driver
6. Type "sudo make install" to make the driver work permanently
7. Type "sudo make uninstall" to remove the driver

Before the driver works, you should make sure that the ch341/ch347/ch339 device has been plugged in and is working properly, you can use shell command "lsusb" or "dmesg" to confirm that, VID of ch341/ch347/ch339 is [1A86].

If ch341/ch347/ch339 device works well, the driver will created devices named "ch34x_pis*" in /dev directory.

## Application Operating Overview

1. Copy the dynamic library in "lib" directory to system default library path: "sudo cp libch347.so /usr/lib"
2. Switch to "demo" directory
3. Type "gcc ch347_demo.c -o app -lch347" to compile the demo

## Note

**ch341 supports 3 modes:**

mode0: [uart]

mode1: [spi + i2c + parallel + gpio]

mode2: [printer]

**ch347t supports 4 modes:**

mode0: [uart * 2] in vcp/cdc driver mode, the devices are named /dev/tty*

mode1: [spi + i2c + uart * 1] in vcp driver mode, the devices are named /dev/ch34x_pis* and /dev/tty*

mode2: [spi + i2c + uart * 1] in hid driver mode, the devices are named /dev/hidraw*

mode3: [jtag + uart * 1] in vcp driver mode, the devices are named /dev/ch34x_pis* and /dev/tty*

**ch347f supports 1 mode:**

mode0: [uart * 2 + spi + i2c + jtag] in vcp driver mode, the devices are named /dev/ch34x_pis* and /dev/tty*

**ch339w supports many modes:**

when the mode supports uart/jtag/spi/i2c/gpio in vcp driver mode, the devices are named /dev/ch34x_pis* and /dev/tty*, the details you can refer to the datasheet.

This driver only can be used with **ch341 mode1**, **ch347t mode1/mode3**, **ch347f**, **several modes of ch339w**.

This application library and examples can be used with all modes.

Any question, you can send feedback to mail: tech@wch.cn
