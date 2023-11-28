#pragma once
#include "boot_ddr.hh"
#include "boot_detect.hh"
#include "boot_image_def.hh"
#include "boot_nor.hh"
#include "boot_sd.hh"
#include "compiler.h"
#include "mach/stm32.h"
#include "print_messages.hh"
#include <optional>

struct ImageInfo {
	uint32_t load_addr = 0;
	uint32_t entry_point = 0;
	uint32_t size = 0;
};

class BootMediaLoader {
	using enum BootLoader::LoadTarget;
	static constexpr auto header_size = sizeof(BootImageDef::image_header);

public:
	BootMediaLoader(BootDetect::BootMethod boot_method)
	{
		set_bootmethod(boot_method);
		if (!_loader)
			pr_err("BootMediaLoader(): Unknown boot method\n");
	}

	bool load_image(BootLoader::LoadTarget target)
	{
		if (!_loader) {
			pr_err("BootMediaLoader::load_image(): Unknown boot method\n");
			return false;
		}

		auto current_header_addr = _loader->get_first_header_addr(target);

		while (true) {
			log("\nReading 0x", Hex{current_header_addr}, " to check for uimg header\n");

			auto header = _loader->read_image_header(current_header_addr);
			auto info = _parse_header(header);

			if (info) {
				uint32_t body_size = info->size - header_size;
				uint32_t body_addr = current_header_addr + header_size;
				uint32_t load_end_addr = info->load_addr + body_size;

				if (valid_addr(info->load_addr) && valid_addr(load_end_addr)) {

					log("Loading from 0x", Hex{body_addr}, "-0x", Hex{body_addr + body_size});
					log(" to 0x", Hex{info->load_addr}, "-0x", Hex{load_end_addr}, "\n");

					_loader->load_image(body_addr, info->load_addr, body_size);

				} else {
					log("Skipping Section with invalid load address range: ");
					log("0x", Hex{info->load_addr}, "-0x", Hex{load_end_addr}, "\n");
				}

				current_header_addr += info->size;

			} else {
				log("No more sections found\n");
				break;
			}
		}

		return true;
	}

	void boot_image()
	{
		if (_entry_point.has_value()) {

			typedef void __attribute__((noreturn)) (*image_entry_noargs_t)();
			auto image_entry = reinterpret_cast<image_entry_noargs_t>(_entry_point.value());
			log("Jumping to image entry point: 0x", Hex{_entry_point.value()}, "\n");

			image_entry();

		} else {
			pr_err("No images containing an entry point have been loaded.\n");
		}
	}

	// You may call this to change boot methods. For example
	// if load_image() fails, you can try a different boot method
	bool set_bootmethod(BootDetect::BootMethod new_boot_method)
	{
		_entry_point = std::nullopt;

		_loader = _get_boot_loader(new_boot_method);
		return (_loader != nullptr);
	}

private:
	std::optional<ImageInfo> _parse_header(BootImageDef::image_header const &header)
	{
		debug("Raw header (big-endian):\n");
		debug("  ih_magic: ", Hex{header.ih_magic}, "\n");
		debug("  ih_hcrc:  ", Hex{header.ih_hcrc}, "\n");
		debug("  ih_time:  ", Hex{header.ih_time}, "\n");
		debug("  ih_size:  ", Hex{header.ih_size}, "\n");
		debug("  ih_load:  ", Hex{header.ih_load}, "\n");
		debug("  ih_ep:    ", Hex{header.ih_ep}, "\n");
		debug("  ih_dcrc:  ", Hex{header.ih_dcrc}, "\n");
		debug("  ih_os:    ", Hex{header.ih_os}, "\n");
		debug("  ih_arch:  ", Hex{header.ih_arch}, "\n");
		debug("  ih_type:  ", Hex{header.ih_type}, "\n");
		debug("  ih_comp:  ", Hex{header.ih_comp}, "\n");
		debug("  ih_name:  ");
		for (char c : header.ih_name)
			if (c >= 32 && c <= 126)
				debug(c);
		debug("\n");

		ImageInfo image_info{};

		uint32_t magic = be32_to_cpu(header.ih_magic);
		if (magic == BootImageDef::IH_MAGIC) {
			auto entry_point = be32_to_cpu(header.ih_ep);
			auto type = header.ih_type;
			auto load_addr = be32_to_cpu(header.ih_load);
			auto size = be32_to_cpu(header.ih_size);

			if (type == BootImageDef::IH_TYPE_KERNEL) {
				if (entry_point >= load_addr && entry_point < (load_addr + size)) {
					if (_entry_point.has_value())
						pr_err("Error: more than one kernel image with a valid entry point found.\n");
					else {
						log("Setting entry point to 0x", Hex{entry_point}, "\n");
						_entry_point = entry_point;
						image_info.entry_point = entry_point;
					}
				} else
					debug("Bad entry point found: 0x", Hex{entry_point}, "\n");
			}

			image_info.load_addr = load_addr;
			image_info.size = size;

			log("Image load addr: 0x", Hex{image_info.load_addr}, "\n");
			log("     entry addr: 0x", Hex{image_info.entry_point}, "\n");
			log("           size: 0x", Hex{image_info.size}, "\n");

			return image_info;

		} else {
			log("Not an image. Magic was ", Hex{magic}, ", expected ", Hex{BootImageDef::IH_MAGIC}, "\n");
		}

		return std::nullopt;
	}

	bool valid_addr(uint64_t addr)
	{
		if (addr >= RETRAM_BASE && addr <= (RETRAM_BASE + STM32MP15x_RETRAM_SIZE))
			return true;

		if (addr >= SRAM_BASE && addr <= (SRAM_BASE + STM32MP15x_SRAM_SIZE))
			return true;

		if (addr >= STM32_DDR_BASE && addr <= STM32_DDR_END)
			return true;

		return false;
	};

	std::optional<uint32_t> _entry_point{};

	// We don't have dynamic memory, so we use placement new.
	// TODO: use std::variant or keep an instance of each type and point _loader to the active one.
	uint8_t loader_storage[std::max(std::max(sizeof(BootSDLoader), sizeof(BootNorLoader)), sizeof(BootDdrLoader))];
	BootLoader *_loader;

	BootLoader *_get_boot_loader(BootDetect::BootMethod bootmethod)
	{
		return bootmethod == BootDetect::BOOT_NOR	 ? new (loader_storage) BootNorLoader :
			   bootmethod == BootDetect::BOOT_SDCARD ? new (loader_storage) BootSDLoader :
			   bootmethod == BootDetect::BOOT_DDR	 ? new (loader_storage) BootDdrLoader :
													   static_cast<BootLoader *>(nullptr);
	}

};
