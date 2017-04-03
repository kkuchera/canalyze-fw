# CANalyze Firmware
Native Linux CAN interface. This firmware can be used with the CANalyze
[hardware](https://github.com/kkuchera/canalyze-hw).

# Features
- Native CAN interface in Linux
- Uses [SocketCAN](https://github.com/linux-can) and [8dev device
  driver](https://github.com/torvalds/linux/blob/master/drivers/net/can/usb/usb_8dev.c).
- USB 2.0 FS and CAN 2.0 interface
- Support for 11-bit and 29-bit CAN IDs
- Normal, listen only, loopback and one shot modes
- Reports CAN errors
- User defined baud rates
- Built entirely using open source software
- Designed for reverse engineering

## Installation
You need the CANalyze [hardware](https://github.com/kkuchera/canalyze-hw).

Install required packages
```shell
$ sudo apt-get install gcc-arm-none-eabi openocd dfu-util
$ git clone https://github.com/kkuchera/canalyze-fw
$ cd canalyze-fw
$ mkdir lib
```
Download and unzip
[STM32CubeF0](http://www.st.com/en/embedded-software/stm32cubef0.html) to lib/.

Connect the BOOT pins and press reset. Then run
```shell
$ make dfu
```
to make and download the firmware onto the device. Finally remove the BOOT pins
jumper and press reset again to start the application.

If you are going to do a lot of flashing it's probably easier to get a STM32
programmer, such as the
[ST-LINK/V2](http://www.st.com/en/development-tools/st-link-v2.html), and
connect the SWD pins. Then you just need to run
```shell
$ make flash
```
when you want make and flash the firmware onto the device.

## Getting started
Bring up CAN interface
```shell
$ sudo ip link set can0 up type can bitrate 500000
```
Sniff CAN messages
```shell
$ cansniffer -c can0
```
or dump all CAN messages
```shell
$ candump can0
```
Send a CAN message
```shell
$ cansend can0 666#01020304
```
