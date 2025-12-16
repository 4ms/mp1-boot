#include "print.hh"
#include <cstdint>

inline uint32_t read_raw_bootrom_itf()
{
	// Allocated by the linker, and value is written to the address by the startup script
	extern uint32_t boot_mode_table_addr[];

	print("&boot_mode_table_addr = 0x", Hex{(uint32_t)boot_mode_table_addr}, "\n");

	if ((uint32_t)boot_mode_table_addr == 0) {
		print("Boot mode table address not defined in linker script. Presuming SD Card boot\n");
		return 1;
	}

	auto table_addr = boot_mode_table_addr[0];
	print("boot_mode_table_addr = 0x", Hex{table_addr}, "\n");

	if (table_addr == 0) {
		print("Boot mode table was not set by startup.s. Presuming SD Card boot\n");
		return 1;
	}

	auto val = *reinterpret_cast<uint32_t *>(table_addr);
	print("boot_mode value = 0x", Hex{val}, "\n");
	return val;
}
