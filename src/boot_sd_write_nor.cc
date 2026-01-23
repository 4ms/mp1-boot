#include "boot_sd_write_nor.hh"
#include "board_conf.hh"
#include "print.hh"
#include "print_messages.hh"

#include "qspi_flash_driver.hh"

NorFlashWriter::NorFlashWriter() {}

// Singleton, lazy-constructed
mdrivlib::QSpiFlash &flash_loader()
{
	static mdrivlib::QSpiFlash flash;
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
	if (nor_addr < 0x7000'0000 || (nor_addr + bytes.size()) > 0x8000'0000) {
		pr_err("Can only write to NOR Flash (0x70000000)\n");
	}

	nor_addr -= 0x7000'0000;

	auto &flash = flash_loader();

	if ((nor_addr & 0xFFF) == 0) {
		log("Erasing 4k block at 0x", Hex{nor_addr}, "\n");

		if (!flash.erase(4096, nor_addr)) {
			pr_err("ERROR: Flash failed to erase block\n");
			return false;
		}
	}

	debug("Writing to ", Hex{nor_addr}, " ", bytes.size(), " bytes\n");

	auto ok = flash.write(bytes.data(), nor_addr, bytes.size());

	if (!ok) {
		pr_err("Failed to write to flash address ", Hex{nor_addr}, "\n");
		return false;
	}

	debug("Wrote OK\n");
	return true;
}
