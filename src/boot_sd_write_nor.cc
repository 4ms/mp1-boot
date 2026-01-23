#include "boot_sd_write_nor.hh"
#include "board_conf.hh"
#include "drivers/norflash/qspi_norflash_read.h"
#include "print.hh"
#include "print_messages.hh"

NorFlashWriter::NorFlashWriter()
{
	Board::NORFlash::d2.init(PinMode::Alt);
	Board::NORFlash::d3.init(PinMode::Alt);

	QSPI_init(QSPIMode::SingleIO);
}

bool NorFlashWriter::write(uint32_t nor_addr, std::span<const uint8_t> bytes)
{
	// TODO: skip 0x7008'0000 - 0x40?
	// It loads the uimg header which could overwrite a previous image

	// Skip out of range addresses
	if (nor_addr < 0x7000'0000) {
		bytes = bytes.subspan(0x7000'0000 - nor_addr);
		nor_addr = 0x7000'0000;
	}

	debug("Writing to ", Hex{nor_addr}, " ", bytes.size(), " bytes\n");

	// if (nor_addr < 0x7000'0000) {
	// 	int i = 0;
	// 	for (auto b : bytes) {
	// 		print(Hex{b}, " ");
	// 		i++;
	// 		if (i % 16 == 0)
	// 			print("\n");
	// 	}
	// }
	return true;
}
