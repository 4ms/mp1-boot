#pragma once
#include "board_conf.hh"
#include "boot_image_def.hh"
#include "boot_loader.hh"
#include "drivers/norflash/qspi_norflash_read.h"
#include "drivers/pinconf.hh"
#include "print_messages.hh"
#include <cstdint>

struct BootNorLoader : BootLoader {
	BootNorLoader()
	{
		Board::NORFlash::d2.init(PinMode::Alt);
		Board::NORFlash::d3.init(PinMode::Alt);

		QSPI_init();
	}

	BootImageDef::image_header read_image_header(uint32_t header_addr) override
	{
		BootImageDef::image_header header;

		auto ok = QSPI_read_MM((uint8_t *)(&header), header_addr, BootImageDef::HeaderSize);
		if (!ok) {
			pr_err("Failed reading NOR Flash\n");
			return {};
		}

		return header;
	}

	bool load_image(uint32_t image_addr, uint32_t load_addr, uint32_t size) override
	{
		auto load_dst = reinterpret_cast<uint8_t *>(load_addr);
		return QSPI_read_MM(load_dst, image_addr, size);
	}

	uint32_t get_first_header_addr(LoadTarget target) override
	{
		return (target == LoadTarget::App) ? BootImageDef::NorFlashAppAddr : BootImageDef::NorFlashSSBLAddr;
	}
};
