#pragma once

#include "qspi_flash_driver.hh"
#include <cstdint>
#include <span>

class FlashLoader {
public:
	FlashLoader();
	bool write_sectors(uint32_t base_addr, std::span<const uint8_t> buffer);
	bool write_sectors(uint32_t base_addr, std::span<const char> buffer);

	bool read_sectors(uint32_t base_addr, std::span<uint8_t> buffer);

	bool read(uint32_t base_addr, std::span<uint8_t> buffer) { return read_sectors(base_addr, buffer); }

	mdrivlib::QSpiFlash &flash_driver() { return flash; }

private:
	mdrivlib::QSpiFlash flash;
};
