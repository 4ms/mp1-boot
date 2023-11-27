#pragma once
#include "boot_image_def.hh"
#include <cstdint>

struct BootLoader {
	enum class LoadTarget { App, SSBL, Unknown };

	virtual BootImageDef::image_header read_image_header(uint32_t header_addr) = 0;

	virtual bool load_image(uint32_t image_addr, uint32_t load_addr, uint32_t size) = 0;

	// Returns the address where the first uimg header is expected to be found
	virtual uint32_t get_first_header_addr(LoadTarget target) = 0;
};
