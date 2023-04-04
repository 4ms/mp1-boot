#pragma once
#include "boot_image_def.hh"

struct BootLoader {
	enum class LoadTarget { App, SSBL };
	virtual BootImageDef::image_header read_image_header(LoadTarget target) = 0;
	virtual bool load_image(uint32_t load_addr, uint32_t size, LoadTarget target) = 0;
};
