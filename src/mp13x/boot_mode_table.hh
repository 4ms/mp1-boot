#include "print.hh"
#include <cstdint>

inline uint32_t read_raw_bootrom_itf()
{
	// Allocated by the linker, and value is written to the address by the startup script
	extern uint32_t boot_mode_table_addr[];

	if ((uint32_t)boot_mode_table_addr == 0) {
		print("Boot mode table address not defined in linker script. Presuming SD Card boot\n");
		return 1;
	}

	auto table_addr = boot_mode_table_addr[0];

	if (table_addr == 0) {
		print("Boot mode table was not set by startup.s. Presuming SD Card boot\n");
		return 1;
	}

	auto val = *reinterpret_cast<uint32_t *>(table_addr); // 0x10001
	return val;
}
