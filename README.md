# purpledrop-stm32

Embedded software for the PurpleDrop digital microfluidics platform microcontroller. 

## Target Device

This software runs on the STMF413ZH, a cortex M4 chip. 

## Project setup

The modm library is used for peripheral drivers. It is included as a git submodule. To build the modm library, you will need lbuild -- e.g. `pip3 install lbuild`. 

`git submodule update --init --recursive`
`lbuild build`

CMake is required for build: `apt install cmake`.

You will also need the arm-none-eabi toolchain: `apt install gcc-arm-none-eabi`

To build, you can simply run `make`. 

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

If you have an openocd supported debugger, you can debug on the target with gdb. I recommend an STLink-V2; others can work, but you may need to adjust the settings in `openocd.cfg`. The purpledrop debug connector is designed for the [6-pin tag connector](tag-connect.com) with the `ARM20-CTX` adapter board. 

First, run `openocd` in the background, or in a secondary terminal. GDB will connect to the target through the openocd process. 

Then `make gdb` will launch a debug session. 

