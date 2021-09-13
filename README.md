# purpledrop-stm32

Embedded software for the PurpleDrop digital microfluidics platform microcontroller.

## Project setup

The software is written in C++ and uses [modm](http://modm.io) for build system and
peripheral libraries.

Remember, it is not generally necessary to compile the embedded software in
order to use PurpleDrop. Pre-built releases are available
[on github](https://github.com/uwmisl/purpledrop/releases) and can be programmed
onto the purpledrop, and customized drop control can be done by writing python
code to run on your own laptop, controlling the PurpleDrop via USB. If you
have ideas for features which need to be added to the embedded software,
please open an [issue](https://github.com/uwmisl/purpledrop/issues)!

For STM32 based boards, the software can be programmed via the USB port using
the built-in DFU bootloader.

For SAMG55 based boards, the bootloader must first be programmed using a SWD
debugger. Once the bootloader is installed, the application software can be
updated via USB.

Development has been primarily done on Linux, and this documentation is geared
towards that environment. However, it is possible to build on other platforms.
See the [modm installation guide](https://modm.io/guide/installation/) for details
on setup on Linux, MacOS or Windows.

### Dependencies

The modm library is included as a git submodule. To make sure your submodule is checked
out and up to date, run:

`git submodule update --init --recursive`

Modm has some python dependenciestools, such as `lbuild`, which can be installed with

`pip install modm`.

Install cmake: `apt install cmake`.

Install other python dependencies: `pip install click intelhex`.

You will need the "arm-none-eabi" [embedded ARM toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
to compile.

## Target Device

This software supports two different cortex M4 microcontrollers:

- stm32: An STM32F413ZH, on hardware prior to revision 6.2.
- sam: A SAMG55J19, used on hardware revision 6.4.

The transition was made due to part shortages in 2021, and it's possible that a future
revision of the board will return to using the stm32 microcontroller.

## File Organization

Three separate executable files can be built from this repository: The STM32 application,
the SAMG55 application, and a DFU bootloader for the SAMG55 (The STM32 has a built-in DFU
bootloader, but the SAMG55 does not).

Directory structure:

- lib: This directory contains common code shared between the sam and stm32 applications.
- stm32: STM32 specific code, and the build target for the STM32.
- sam: SAMG55 specific code, and the build target for the SAMG55.
- dfu-sam: The DFU bootloader for the SAMG55
- ext: Contains git submodules linking to external repositories (e.g. modm)
- tools: Utilities for building, debugging, or otherwise working with the project
- templated: This directory contains an [lbuild](https://pypi.org/project/lbuild/) repository,
  with templated drivers used by the applications.

## Building the applications

Each application can be built in it's own sub-directory: `stm32`, `sam`, or `dfu-sam`.

### Building stm32 or sam applications

Enter the build directory: `cd sam`, or `cd stm32`.

Generate the modm library: `lbuild build`. This creates a library in the `sam/modm`
directory, based on the options defined in `project.xml`. This must be re-run
anytime the `ext/modm` git submodule is updated, or if `project.xml` is changed.

Build the application: `make`.

Program the application: `make program` (this assumes you have a openocd supported debugger attached).

### Building the bootloader for SAMG55

Building the bootloader is slightly different, as it uses the modm default build tool: scons.

Enter the build directory: `cd dfu-sam`.

Generate the modm library: `lbuild build`.

Build the application: `scons build`.

Program via debugger: `scons program`.

Some additional commands that may be useful:
  - `scons program profile=debug` to build the debug profile.
  - `scons build verbose=1` to see the full build commands executed

## Programming via USB bootloader

The PurpleDrop can enumerate as a USB DFU device for software updates.

A DFU file can be programmed to the device using either ST's DFuSe utility program on windows, or `dfu-util`
on linux or MacOS.

The exact procedure depends on which version -- STM32 or SAMG55 -- you are using. On STM32
the bootloader is build into the chip. On SAMG55, it must be programmed via SWD debugger
first. The STM32 is programmed with a DFU file, while the SAMG is programmed with a plain
binary image (.bin). Finally, the DFU will show up with different USB VID/PIDs for the
two different chips.

With dfu-util, you can program the purpledrop by:

1. Boot the device into bootloader mode by holding the BOOT0 button while releasing the RST button
2. Program the `purpledrop-stm32.dfu` (stm32) or `purpledrop-sam.bin` output file (found in `build/cmake-build-release/`) with
    - `dfu-util -d 0483:df11 -a 0 -D purpledrop-stm32.dfu`, or
    - `dfu-util -d 1209 -a 0 -D purpledrop-sam.bin`
3. Reset the device back into application using the RST button

## Memory Regions

### STM32 memory regions

The STM32 always resets to zero, so the vector table must go into the first page.
Then, two independently erasable pages are used to store two copies of non-volatile
configuration (two pages to allow for safe erase and write). To accomodate this,
a DFU must be created from the compiled ELF file with two regions -- the vector
table and the application -- so that these get programmed without erasing the
configuration pages in between. This is handled by the python scripts in `tools/`.

| Start Address | Size  | Description   |
| ------------- | ----- | ------------- |
| 0x8000000     | 32K   | Vector Table  |
| 0x8008000     | 16K   | Config Page A |
| 0x800C000     | 16K   | Config Page B |
| 0x08010000    | 1472K | Application   |

### SAMG55 memory regions

On SAMG55, there is no built in bootloader. The bootloader will jump to the
application address, and reset the processor vector table location as well,
so there is no need to split the application into two memory sections. This
does mean however that the bootloader must be programmed in order to run
the application.

| Start Address | Size | Description   |
| ------------- | ---- | ------------- |
| 0x400000      | 8K   | Bootloader    |
| 0x402000      | 4K   | Config Page A |
| 0x403000      | 4K   | Config Page B |
| 0x404000      | 496K | Application   |

## Running tests

There are unit tests in the `test` directory.

```
cd test
mkdir build
cd build
cmake ..
make
./PurpleDropTest
```

## Debugging

If you have an openocd supported debugger, you can debug on the target with gdb. I recommend an STLink-V2;
others can work, but you may need to adjust the settings in `openocd.cfg`. The purpledrop debug connector
is designed for the [6-pin tag connector](tag-connect.com) with the `ARM20-CTX` adapter board.

First, run `openocd` in the background, or in a secondary terminal. GDB will connect to the target through
the openocd process. Then, `make gdb` will launch a debug session.

You may need to install gdb-multiarch: `apt install gdb-multiarch`.

## Controlling a PurpleDrop

The PurpleDrop enumerates as a USB CDC virtual serial port to communicate with software on the host computer.
Software for controlling the purpledrop can be found at https://github.com/uwmisl/purpledrop-driver.
