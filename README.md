# purpledrop-stm32

Embedded software for the PurpleDrop digital microfluidics platform microcontroller. 

## Target Device

This software runs on the STMF413ZH, a cortex M4 chip. 

## Project setup

The modm library is used for peripheral drivers. It is included as a git submodule. To build the modm library, you will need lbuild -- e.g. `pip install lbuild`. 

`git submodule update --init --recursive`
`lbuild build`

CMake is required for build: `apt install cmake`.

A python script is used for creating a DFU programming file, and it has some dependencies: 
`pip install click intelhex`.

You will also need the arm-none-eabi toolchain: `apt install gcc-arm-none-eabi`.

To build, you can simply run `make`. 

### Programming via USB bootloader

The PurpleDrop uses the STM32 built-in bootloader to enumerate as a USB DFU device for software updates. 

A DFU file can be programmed to the device using either ST's DFuSe utility program on windows, or `dfu-util`
on linux or MacOS. 

With dfu-util installed, you can program the purpledrop with: 

1. Boot the device into bootloader mode using the button on the PurpleDrop
2. Program the `purpledrop.dfu` output file (found in `build/cmake-build-release/`) with
`dfu-util -e 0483:df11 -a 0 -D purpledrop.dfu`
3. Reset the device back into application using the buttons

### Running tests

There are unit tests in the `test` directory. 

```
cd test
mkdir build
cd build
cmake ..
make
./PurpleDropTest
```

### Debugging

If you have an openocd supported debugger, you can debug on the target with gdb. I recommend an STLink-V2; 
others can work, but you may need to adjust the settings in `openocd.cfg`. The purpledrop debug connector
is designed for the [6-pin tag connector](tag-connect.com) with the `ARM20-CTX` adapter board. 

First, run `openocd` in the background, or in a secondary terminal. GDB will connect to the target through
the openocd process. Then, `make gdb` will launch a debug session. 

## Controlling a PurpleDrop

The PurpleDrop enumerates as a USB CDC virtual serial port to communicate with software on the host computer.
Software for controlling the purpledrop can be found at https://github.com/uwmisl/purpledrop-driver.
