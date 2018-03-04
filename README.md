# Tizen RT

This repository is a fork of the official Tizen RT github repository from 01.25.2018: https://github.com/Samsung/TizenRT .

It contains hastily porting of the Nuttx implementation of the stm32f429-disco board support and micropython.

## STM32F429-Disco board support and Tizen RT

stm32f429-disco board support was tested only by loading of the simple Tizen RT configuration at the board. Serial port 1 was tested as main serial port for access to the Tizen RT TASH console. Other peripherials were not tested.

```
os/arch/arm/src/stm32
os/arch/arm/src/stm32f429-disco
os/arch/arm/include/stm32
```

These folders contain a code for stm32f429-disco support. 

*build/configs/stm32* folder contains default configurations, linker script and download script for stm32f429-disco board support.

These default configuration uses serial port 1 as main serial port for the TASH console.

Serial port 1 at the board:

```
PA9  - Tx
PA10 - Rx
GND
```

*stm32_download.sh* script contains a script to download Tizen RT image in the board. This script uses **stlink** utility to write image to the flash. So you should install this utility to use this script. You can download this utility from https://github.com/texane/stlink .


### Build and Run

```
cd TizenRT/os
cd tools
./configire.sh stm32/stm32f429_disco
cd ..
make menuconfig
make
```

Install **stlink** utility and connect board to the USB.

```
make download
```

Connect serial port to the serial port 1 of the board: *PA9, PA10, GND pins*.

```
minicom -D /dev/ttyUSBn
```

Reset the board by reset button. Then you should see:

```
System Information:
        Version: 1.1
        Commit Hash: c9b73b049d5b3812b39eeca9ead25eba2f3e7df8
        Build User: alex@localhost.localdomain
        Build Time: 2018-03-04 13:34:10
        System Time: 08 Jun 2010, 00:00:00 [s] UTC 
Hello, World!!

TASH>>
TASH>>help
TASH>>   TASH command list 
         --------------------
cat              cd               date             df               
exit             free             getenv           heapinfo         
hello            help             kernel_sample    kill             
killall          logm             ls               micropython      
mkdir            mount            ps               pwd              
rm               rmdir            setenv           sh               
sleep            umount           unsetenv         uptime           

TASH>>ps
TASH>>
  PID | PRIO | FLAG |  TYPE   | NP |  STATUS  | NAME
------|------|------|---------|----|----------|----------
    0 |    0 | FIFO | KTHREAD |    | READY    | Idle Task
    1 |  224 | RR   | KTHREAD |    | WAITSIG  | hpwork
    2 |   50 | RR   | KTHREAD |    | WAITSIG  | lpwork
    3 |  110 | RR   | KTHREAD |    | WAITSIG  | logm
    5 |  125 | RR   | TASK    |    | RUNNING  | tash

```


For a more detailed description of the process of building and launching, see https://github.com/Samsung/TizenRT .


## micropython

Tizen RT can use micropython. It uses micropython 1.3.8 from https://github.com/micropython/micropython .

*/apps/system/micropython* contains micropython implementation.

You can enable micropython by menuconfig:

```
CONFIG_INTERPRETERS_MICROPYTHON:
Enable support for the Micro Python interpreter

Symbol: INTERPRETERS_MICROPYTHON 
Prompt: Micro Python support
  Location:
  -> Application Configuration
  -> System Libraries and Add-Ons
```

Also you can use *artik053/nettest_micropython* and *stm32/stm32f429_disco_micropython* configuration with already enabled mycropython.


```
TASH>>micropython
Micro Python 1.1_Public_Release-659-gc9b73b04 on 2018-03-04
Type "help()" for more information.
>>> 
>>> 
>>> 2 * 100
200 
>>> 10000 / 7
1428.571

```
