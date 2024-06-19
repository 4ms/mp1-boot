#!/usr/bin/env python3

# From https://github.com/WerWolv/STM32MP1OS

import argparse
import struct

def add_mp2_fsbl_header(binary_file):
    if binary_file[:4] == b"STM2":
        exit("Header already present")

    payload = bytearray(binary_file[0x100:])

    header = struct.pack("<ccccQQQQQQQQIIIIIIIIIII8xII",
        bytes('S', "ascii"), bytes('T', "ascii"), bytes('M', "ascii"), bytes('2', "ascii"), # Header magic
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                     # ECDSA signature (512bits=64Bytes=8 quads), unsigned here
        sum(payload),                                                                       # Checksum of payload, sum of all bytes
        0x00020200,                                                                         # Header version 1.0
        len(payload),                                                                       # Length of payload
        0x2FFC0000 + 0x2400 + 0x100,                                                        # Entrypoint address. ????
        0x00, 0x00, 0x00,                                                                   # Reserved (96 bits = 3 quads)
        0x00000000,                                                                         # Image version (32bits)
        0x00000000,                                                                         # Option flags (32bits) , header padding disabled, auth disabled, decrypt disabled
        0x00000000,                                                                         # Header extension lengths (32bits)
        0x00000030,                                                                         # Binary type (32bits). FSLBM = 0x30
        # Padding 8 bytes
        0x00000000,                                                                         # Non-secure payload length (32bits)
        0x00000000,                                                                         # Non-secure payload hash (32bits)
        )

    return header, payload


def add_mp1_fsbl_header(binary_file):

    if binary_file[:4] == b"STM2":
        exit("Header already present")

    payload = bytearray(binary_file[0x100:])

    header = struct.pack("<ccccQQQQQQQQIIIIIIIIIIQQQQQQQQ83xb",
        bytes('S', "ascii"), bytes('T', "ascii"), bytes('M', "ascii"), bytes('2', "ascii"), # Header magic
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                     # ECDSA signature, unsigned here
        sum(payload),                                                                       # Checksum of payload, sum of all bytes
        0x00010000,                                                                         # Header version 1.0
        len(payload),                                                                       # Length of payload
        0x2FFC0000 + 0x2400 + 0x100,                                                        # Entrypoint address. SYSRAM + 0x2400 to skip past boot ROM data + 0x100 to skip past this header
        0x00,                                                                               # Reserved
        0x2FFC0000 + 0x2400,                                                                # Load address of image, unused
        0x00,                                                                               # Reserved
        0x00,                                                                               # Image version
        0x01,                                                                               # Option flags, disable signature verification
        0x01,                                                                               # ECDSA algorithm set to P-256 NIST, unused
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                     # ECDSA signature, unsigned here
        0x00                                                                                # Binary type: U-Boot
        )

    return header, payload

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Add STM32 fsbl header to a binary file")
    parser.add_argument("binary_file", help="Input binary file")
    parser.add_argument("output_file", help="Output fsbl file")
    parser.add_argument("--format", help="Format type", choices=["mp1", "mp2"], default="mp1")
    args = parser.parse_args()

    with open(args.binary_file, "rb") as bin_file:

        binary_file = bin_file.read()

        if args.format == "mp2":
            header, payload = add_mp2_fsbl_header(binary_file)
        else:
            header, payload = add_mp1_fsbl_header(binary_file)

        with open(args.output_file, "wb") as fsbl_file:
            fsbl_file.write(header + payload)


