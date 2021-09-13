from pyocd.core.helpers import ConnectHelper
import time

session = ConnectHelper.session_with_chosen_probe()

FCMD_ADDR = 0x400E0A04
FSR_ADDR = 0x400E0A08
FRR_ADDR = 0x400E0A0C
FKEY = 0x5A << 24

SGPB_CMD = 0xB
GGPB_CMD = 0xD

def set_gpnvm(target, bit):
    target.write32(FCMD_ADDR, SGPB_CMD | FKEY | (bit<< 8))
    start = time.time()
    fsr = target.read32(FSR_ADDR)
    while fsr == 0:
        if time.time() - start > 10:
            raise RuntimerError("Timeout waiting for flash status")
        fsr = target.read32(FSR_ADDR)
    
    print("Got FSR: %x" % (fsr))

    print("Result: %x" % target.read32(FRR_ADDR))

def read_gpnvm(target):
    target.write32(FCMD_ADDR, GGPB_CMD | FKEY)
    start = time.time()
    fsr = target.read32(FSR_ADDR)
    while fsr == 0:
        if time.time() - start > 10:
            raise RuntimerError("Timeout waiting for flash status")
        fsr = target.read32(FSR_ADDR)
    
    print("Got FSR: %x" % (fsr))

    print("Result: %x" % target.read32(FRR_ADDR))

# When the 'with' statement exits, the session is closed.
with session:
    target = session.board.target

    read_gpnvm(target)

    # Set the BOOTMODE bit
    set_gpnvm(target, 1)

    read_gpnvm(target)
