#!/usr/bin/env python3

import click
import intelhex
import os

from dfu_build import build

# Defines start, size for the DFU sections
FLASH_TABLE = [
    (0x8000000, 16 * 1024),
    (0x8010000, 1472 * 1024 ),
]

@click.command()
@click.argument('ihex_input')
@click.argument('dfu_output')
def main(ihex_input, dfu_output):
    if not os.path.isfile(ihex_input):
        raise RuntimeError(f"Input file {ihex_input} doesn't exist")
    ihex = intelhex.IntelHex(ihex_input)

    target = []
    for segment in FLASH_TABLE:
        # TODO: Could be more efficient here by finding the max address within the 
        # FLASH_TABLE segment. As-is, the first section will always be padded to fill the 
        # section, even though its much shorter
        segment_size = min(ihex.maxaddr() - segment[0], segment[1])
        bindata = bytes(ihex.tobinarray(start=segment[0], size=segment_size))
        print(f"Writing data from start={hex(segment[0])} size={segment_size}")
        print(f"Size: {len(bindata)}")
        target.append({'address': segment[0], 'data': bindata})
    
    build(dfu_output, [target])

if __name__ == '__main__':
    main()


