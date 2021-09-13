#!/usr/bin/env python3

import click
import intelhex
import os
import sys

from dfu_build import build

# Defines start, size for the DFU sections
STM32_FLASH_TABLE = [
    (0x8000000, 16 * 1024),
    (0x8010000, 1472 * 1024 ),
]
STM32_DEVICE = "0x0483:0xdf11"

SAM_FLASH_TABLE = [
    (0x404000, 496 * 1024),
]
SAM_DEVICE = "0x1209:0xccaa"

# ST uses "DFuse" protocol, which is quite different from the USB DFU1.1 protocol. 
# dfu-util decides which method to use based on a version stored in the DFU file.
STM32_DFU_VERSION = 0x011a
SAM_DFU_VERSION = 0x0101

@click.command()
@click.argument('ihex_input')
@click.argument('dfu_output')
@click.option('--device', '-d', help="One of 'stm32' or 'sam'")
def main(ihex_input, dfu_output, device):
    if not os.path.isfile(ihex_input):
        raise RuntimeError(f"Input file {ihex_input} doesn't exist")
    ihex = intelhex.IntelHex(ihex_input)

    if device == 'stm32':
        flash_table = STM32_FLASH_TABLE
        device = STM32_DEVICE
        dfu_version = STM32_DFU_VERSION
    elif device == 'sam':
        flash_table = SAM_FLASH_TABLE
        device = SAM_DEVICE
        dfu_version = SAM_DFU_VERSION
    else:
        print("Must specify a device")
        sys.exit(-1)

    target = []
    for segment in flash_table:
        # TODO: Could be more efficient here by finding the max address within the 
        # FLASH_TABLE segment. As-is, the first section will always be padded to fill the 
        # section, even though its much shorter
        segment_size = min(ihex.maxaddr() - segment[0], segment[1])
        bindata = bytes(ihex.tobinarray(start=segment[0], size=segment_size))
        print(f"Writing data from start={hex(segment[0])} size={segment_size}")
        print(f"Size: {len(bindata)}")
        target.append({'address': segment[0], 'data': bindata})
    
    build(dfu_output, [target], device, dfu_version)

if __name__ == '__main__':
    main()


