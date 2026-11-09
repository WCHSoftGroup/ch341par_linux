# CH341/CH346/CH347/CH339/CH9341/CH9346 Linux SDK 说明文档

## 简介

本目录包含 3 个部分：**USB 设备驱动**、**应用库**以及**示例代码**。该驱动和应用程序支持沁恒（WCH）的 USB 总线接口芯片：CH341、CH346、CH347 、CH339、CH9341、CH9346 以及 WCH USB 转串口系列芯片。

- **CH341**：实现全速 USB 转 UART/SPI/I2C/GPIO/打印并口等。
- **CH346**：实现高速 USB 转 FIFO/UART 等。
- **CH347**：实现高速 USB 转 UART/JTAG/SWD/SPI/I2C/GPIO 等。
- **CH339**：实现高速 USB 集线器（Hub）/ 读卡器 / 以太网以及 USB 转 UART/JTAG/SPI/I2C/GPIO 等。
- **CH9341**：实现全速USB 转 SPI/I2C/GPIO/PWM 等。
- **CH9346**：实现超高速 USB 转 FIFO/UART 等。

> ⚠️ **注意**：该设备驱动程序不包含 Linux TTY 串口设备驱动，仅用于实现 SPI / I2C / GPIO / PWM 等其他接口功能。当使用SDK包提供的设备管理以及串口 API 访问 USB 转串口设备时，需要为串口加载 VCP 厂商驱动程序。

## 驱动操作指南

1. 打开“终端”（Terminal）。
2. 切换到 `driver` 目录。
3. 输入 `make` 编译驱动。如果编译成功，你将看到生成的内核模块文件 `ch34x_pis.ko`。
4. 输入 `sudo make load` 或 `sudo insmod ch34x_pis.ko` 以动态加载驱动。
5. 输入 `sudo make unload` 或 `sudo rmmod ch34x_pis.ko` 以卸载驱动。
6. 输入 `sudo make install` 以使驱动永久生效（安装到系统）。
7. 输入 `sudo make uninstall` 以卸载并移除该驱动。

在驱动正常工作前，请确保 USB 设备已插入且工作正常。你可以通过终端命令 `lsusb` 或 `dmesg` 来确认。上述芯片的厂商识别码（VID）均为 `[1A86]`。

如果设备工作正常，驱动加载后将在 `/dev` 目录下创建名为 `ch34x_pis*` 的设备节点。

## 应用程序操作指南

1. 将 `lib` 目录下的动态链接库复制到系统默认库路径：

   ```
   sudo cp libch34x.so /usr/lib
   ```

2. 切换到 `demo` 目录。

3. 输入以下命令编译示例程序：

   ```
   gcc ch34x_demo.c eeprom_xx24.c flash_xx25.c -o app -lch34x
   ```

## 注意事项

各芯片支持的工作模式如下：

### 芯片工作模式列表

| **芯片型号**   | **模式**                                   | **支持的功能及设备名称**                                     |
| -------------- | ------------------------------------------ | :----------------------------------------------------------- |
| **CH341F/A/B** | Mode 0<br />Mode 1<br />Mode 2             | `[UART]` （Vendor 驱动模式，设备名：`/dev/ttyCH*`） <br />`[SPI + I2C + 并口 + GPIO]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`）<br />`[Printer]`（USB Printer 类驱动模式，设备名：`/dev/usb/lp*` ） |
| **CH341T**     | Mode 0<br />Mode 1<br />                   | `[UART]` （Vendor 驱动模式，设备名：`/dev/ttyCH*`） <br />`[I2C + GPIO]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*`）<br /> |
| **CH347T**     | Mode 0<br />Mode 1<br />Mode 2<br />Mode 3 | `[UART * 2]`（Vendor 驱动模式，设备名：`/dev/ttyCH*`）<br />`[SPI + I2C + UART + GPIO]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`） <br />`[SPI + I2C + UART]`（HID 驱动模式，设备名：`/dev/hidraw*`）<br /> `[JTAG + UART + GPIO]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`） |
| **CH347F**     | Mode 0                                     | `[UART * 2 + SPI + I2C + JTAG]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`） |
| **CH339W**     | Multi-Mode                                 | 当工作在 Vendor 驱动模式下支持 UART/JTAG/SPI/I2C/GPIO 等，设备名为 `/dev/ch34x_pis*` 和 `/dev/ttyCH*`。详情请参考数据手册。 |
| **CH346C**     | Mode 0<br />Mode 1<br /> Mode 2            | `[FIFO + UART]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`）<br /> `[SPI + UART]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`）<br /> `[UART]`（VCP 驱动模式，设备名：`/dev/ttyCH*`） |
| **CH9341C**    | Mode 0                                     | `[SPI + I2C + GPIO + PWM]`（设备名：`/dev/ch34x_pis*` 和 `/dev/ttyCH*`） |
| **CH9346W**    | Mode 0<br />Mode 1<br /> Mode 2            | `[FIFO]`（Vendor 驱动模式，设备名：`/dev/ch34x_pis*`）<br /> `[MEDIA + UART]`（UVC 类驱动模式，设备名：`/dev/video*` 和 `/dev/ttyACM*`）<br /> `[DVP + UART]`（UVC 类驱动模式，设备名：`/dev/video*` 和 `/dev/ttyACM*`） |

### 驱动适用的芯片与模式

- CH341F/A/B/T：`Mode 1`
- CH347T：`Mode 1` / `Mode 3`
- CH347F：`Mode 0`
- CH339W：部分特定模式
- CH346C：`Mode 0` / `Mode 1`
- CH9341C: `Mode 0`
- CH9346W: `Mode 0`

其他情况下无需加载此驱动程序。

如有任何疑问，可发送技术反馈至邮箱：tech@wch.cn