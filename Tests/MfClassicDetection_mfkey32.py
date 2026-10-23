#!/usr/bin/env python3
"""Read MF_CLASSIC_DETECTION data from a ChameleonMini (DETECTION? command),
check it and print mfkey32v2 command lines.

Layout (210 bytes): block 0 (16) | 6 saves key A (16 each) | 6 saves key B | CRC_A (2)
Save: auth cmd + block + CRC (4) | nt (4) | {nr} (4) | {ar} (4)

Usage: MfClassicDetection_mfkey32.py /dev/ttyACM<x> (or wherever your RevE appears)
Needs pyserial (pip install pyserial).
"""
import sys
import serial

CANARY = b'REVEV1.4'
SAVES_PER_KEY = 6
SAVE_SIZE = 16


def crc_a(data):
    crc = 0x6363
    for b in data:
        b ^= crc & 0xFF
        b = (b ^ (b << 4)) & 0xFF
        crc = ((crc >> 8) ^ (b << 8) ^ (b << 3) ^ (b >> 4)) & 0xFFFF
    return bytes([crc & 0xFF, crc >> 8])


def main(port):
    with serial.Serial(port, 115200, timeout=2) as ser:
        ser.reset_input_buffer()
        ser.write(b"DETECTION?\r")
        raw = ser.read(210)
        status = ser.readline()
    if len(raw) != 210:
        sys.exit(f"short read: {len(raw)} bytes, status {status!r}")

    print("status        :", status.strip().decode(errors="replace"))
    print("CRC           :", "ok" if crc_a(raw[:208]) == raw[208:] else "WRONG")

    block0 = raw[:16]
    uid = block0[:4]
    print("block 0       :", block0.hex(" "))
    print("  UID/BCC     :", uid.hex(), f"{block0[4]:02x}",
          "(BCC ok)" if uid[0] ^ uid[1] ^ uid[2] ^ uid[3] == block0[4] else "(BCC WRONG)")
    print("  SAK/ATQA    :", block0[5:8].hex(" "))
    print("  canary      :", "ok" if block0[8:16] == CANARY else f"missing ({block0[8:16].hex()})")
    if block0[:8] == b"\xff" * 8:
        print("  -> bytes 0..7 are FF: UID is read from the wrong memory (bug!)")

    for name, base in (("A", 16), ("B", 16 + SAVES_PER_KEY * SAVE_SIZE)):
        saves = []
        for i in range(SAVES_PER_KEY):
            s = raw[base + i * SAVE_SIZE: base + (i + 1) * SAVE_SIZE]
            if s[0] in (0x60, 0x61):
                saves.append(s)
        print(f"key {name} saves   : {len(saves)}")
        for s in saves:
            print(f"  blk {s[1]:3d}  nt {s[4:8].hex()}  nr {s[8:12].hex()}  ar {s[12:16].hex()}")
        # mfkey32v2 needs two attempts on the same block with the same key
        by_block = {}
        for s in saves:
            by_block.setdefault(s[1], []).append(s)
        for blk, ss in by_block.items():
            if len(ss) >= 2:
                a, b = ss[0], ss[1]
                print(f"  mfkey32v2 {uid.hex()} {a[4:8].hex()} {a[8:12].hex()} {a[12:16].hex()}"
                      f" {b[4:8].hex()} {b[8:12].hex()} {b[12:16].hex()}   # key {name}, block {blk}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0")
