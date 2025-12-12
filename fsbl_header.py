# From https://github.com/WerWolv/STM32MP1OS
# 

import struct
import sys

with open(sys.argv[1], "rb") as fsbl_file:
    payload = fsbl_file.read();

if len(sys.argv) > 3 and sys.argv[3] == "stm32mp13x":
    arch = "stm32mp13x"
else:
    arch = "stm32mp15x"


if arch == "stm32mp15x":
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
else:
    header = struct.pack("<ccccQQQQQQQQIIIIIIIIII20x384x",
        bytes('S', "ascii"), bytes('T', "ascii"), bytes('M', "ascii"), bytes('2', "ascii"), # cccc Header magic
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                     # QQQQQQQQ ECDSA signature, unsigned here
        sum(payload),                                                                       # I Checksum of payload, sum of all bytes
        0x00020000,                                                                         # I Header version 1.0
        len(payload),                                                                       # I Length of payload (without header)
        0x2FFE0000 - 0x200,                                                                 # I Entrypoint address. SYSRAM - size of this header (0x200) so payload starts at SYSRAM start.
        0x00000000,                                                                         # I Reserved (1/3)
        0x00000000,                                                                         # I Reserved (2/3)
        0x00000000,                                                                         # I Reserved (3/3)
        0x00,                                                                               # I Image version
        0x80000000,                                                                         # I Option flags, disable authentication and decryption, enable padding
        0x00,                                                                               # I Header extensions length
                                                                                            # 20B padding
                         # 384B padding
        # bytes('S', "ascii"), bytes('T', "ascii"), 0x00, 0x02,                               # Auth Header ext type
        )

with open(sys.argv[2], "wb") as fsbl_file:
    fsbl_file.write(header + payload)
