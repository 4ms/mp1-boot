#pragma once
#include "boot_detect.hh"
#include <cstdint>

// Not being used by fsbl
// Might be an interesting way to dump more info about the boot process
// See mp13x_rom_trace_analyzer.py script

namespace BootDetect
{
// between 0x30001638 and +4096 2638
static constexpr uint32_t BOOTROM_TRACE_BEGIN = 0x3001638;
static constexpr uint32_t BOOTROM_TRACE_END = 0x3002638;
static constexpr uint32_t START_WORD = 0xFFDDBB00;

inline BootMethod scan_boot_method()
{
	// Scan for:
	// 0x30000011 : "SD_FsblsFound"
	// 0x80000006 : QSPI_FoundMagicAtDefaultLocation or
	// 0x80000005 : "QSPI_FoundGpt",

	uint32_t word = BOOTROM_TRACE_BEGIN;
	while (word < BOOTROM_TRACE_END - 16) {
		auto val = *reinterpret_cast<uint32_t *>(word);

		// Scan for the START_WORD
		if (val == START_WORD) {
			// Skip size, timestamp, level
			word += 16;

			auto val = *reinterpret_cast<uint32_t *>(word);
			if (val == 0x80000006 || val == 0x80000005) {
				return BootMethod::BOOT_NOR;
			}

			if (val == 0x30000011) {
				return BootMethod::BOOT_SDCARD;
			}
		}

		word += 4;
	}

	return BootMethod::BOOT_UNKNOWN;
}

}; // namespace BootDetect
