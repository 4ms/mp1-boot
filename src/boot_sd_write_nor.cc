#include "boot_sd_write_nor.hh"
#include "board_conf.hh"
#include "print.hh"
#include "print_messages.hh"

#include "flash_loader.hh"

NorFlashWriter::NorFlashWriter() {}

// Singleton, lazy-constructed
FlashLoader &flash_loader()
{
	static FlashLoader flash;
	return flash;
}

static void dump_bytes(std::span<const uint8_t> bytes)
{
	int i = 0;
	for (auto b : bytes) {
		if (b < 0x10)
			print("0");
		print(Hex{b}, " ");

		if (++i % 16 == 0)
			print("\n");
	}
}

bool NorFlashWriter::write(uint32_t nor_addr, std::span<const uint8_t> bytes)
{
	// Fix case where loader wants to write the uimg header.
	// This is a problem because it could overwrite a previous image.
	if (nor_addr == 0x7007'FFC0) { // app
		bytes = bytes.subspan(0x40);
		nor_addr = 0x7008'0000;
	}
	if (nor_addr == 0x7004'FFC0) { // ssbl
		bytes = bytes.subspan(0x40);
		nor_addr = 0x7005'0000;
	}
	if (nor_addr == 0x7003'FFC0) { // fsbl2
		bytes = bytes.subspan(0x40);
		nor_addr = 0x7005'0000;
	}
	if (nor_addr == 0x6FFF'FFC0) { // fsbl1
		bytes = bytes.subspan(0x40);
		nor_addr = 0x7000'0000;
	}

	if (nor_addr < 0x7000'0000) {
		pr_err("Can only write to NOR Flash (0x70000000)\n");
	}

	debug("Writing to ", Hex{nor_addr}, " ", bytes.size(), " bytes\n");

	auto ok = flash_loader().write_sectors(nor_addr, bytes);
	if (ok) {
		debug("Wrote OK\n");
	} else {
		pr_err("Failed to write to flash address ", Hex{nor_addr}, "\n");
	}

	return true;
}
