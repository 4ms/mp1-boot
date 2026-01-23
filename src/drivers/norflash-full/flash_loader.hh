#pragma once

// #include "stm32mp13xx_hal_xspi.h"

#define QSPI_HandleTypeDef XSPI_HandleTypeDef
// using QSPI_HandleTypeDef = XSPI_HandleTypeDef;
// using QSPI_CommandTypeDef = XSPI_RegularCmdTypeDef;
#define QSPI_CommandTypeDef XSPI_RegularCmdTypeDef

#include "qspi_flash_driver.hh"
#include <cstdint>
#include <span>

namespace MetaModule
{

class FlashLoader {
public:
	FlashLoader();
	bool check_flash_chip();
	bool write_sectors(uint32_t base_addr, std::span<uint8_t> buffer);
	bool write_sectors(uint32_t base_addr, std::span<char> buffer);

	bool read_sectors(uint32_t base_addr, std::span<uint8_t> buffer);

	bool read(uint32_t base_addr, std::span<uint8_t> buffer) { return read_sectors(base_addr, buffer); }

	mdrivlib::QSpiFlash &flash_driver() { return flash; }

private:
	mdrivlib::QSpiFlash flash;
};

} // namespace MetaModule
