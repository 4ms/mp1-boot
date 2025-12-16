#pragma once
#include <cstdint>
#include <string_view>

namespace BootDetect
{

enum BootMethod {
	BOOT_UNKNOWN = 0,
	BOOT_SDCARD = 1,
	BOOT_EMMC = 2,
	BOOT_NAND = 3,
	BOOT_NOR = 4,
	BOOT_UART = 5,
	BOOT_USB = 6,
	BOOT_SPINAND = 7,

	NUM_BOOT_METHODS = 8,

	BOOT_DDR = 9,
};

constexpr std::string_view bootmethod_string(BootMethod method)
{
	switch (method) {
		case BOOT_SDCARD:
			return "SD Card";
			break;
		case BOOT_EMMC:
			return "EMMC";
			break;
		case BOOT_NAND:
			return "NAND";
			break;
		case BOOT_NOR:
			return "NOR";
			break;
		case BOOT_UART:
			return "UART";
			break;
		case BOOT_USB:
			return "USB";
			break;
		case BOOT_SPINAND:
			return "SPI NAND";
			break;
		default:
			return "Unknown";
	}
}

BootMethod read_boot_method();

}; // namespace BootDetect
