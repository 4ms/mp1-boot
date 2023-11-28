#pragma once
#include "boot_image_def.hh"
#include "boot_loader.hh"
#include "drivers/pinconf.hh"
#include "gpt/gpt.hh"
#include "print_messages.hh"
#include "stm32mp1xx_hal_sd.h"
#include <array>
#include <cstdint>
#include <span>

struct BootSDLoader : BootLoader {
	BootSDLoader()
	{
		RCC->SDMMC12CKSELR = 3; // HSI = 64MHz. Default value (just showing it here for educational purposes)

		HAL_SD_DeInit(&hsd);
		hsd.Instance = SDMMC1;
		hsd.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
		hsd.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
		hsd.Init.BusWide = SDMMC_BUS_WIDE_4B;
		hsd.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
		hsd.Init.ClockDiv = 2; // 64MHz/2 / 2 = 16MHz, seems to be the max OSD32-BRK can handle reliably

		// These pins are not board-specific, they are required by BOOTROM
		// for booting with SDMMC1
		// D1 - D3 are not used by BOOTROM, so need to be init by FSBL
		PinConf{GPIO::C, PinNum::_9, PinAF::AF_12}.init(PinMode::Alt);
		PinConf{GPIO::C, PinNum::_10, PinAF::AF_12}.init(PinMode::Alt);
		PinConf{GPIO::C, PinNum::_11, PinAF::AF_12}.init(PinMode::Alt);

		// D0, CK, CMD are used by BOOTROM and should already be init. We re-init them just in case...
		PinConf{GPIO::C, PinNum::_8, PinAF::AF_12}.init(PinMode::Alt);
		PinConf{GPIO::C, PinNum::_12, PinAF::AF_12}.init(PinMode::Alt);
		PinConf{GPIO::D, PinNum::_2, PinAF::AF_12}.init(PinMode::Alt);

		auto ok = HAL_SD_Init(&hsd);
		if (ok != HAL_OK)
			init_error();
	}

	BootImageDef::image_header read_image_header(uint32_t header_addr) override
	{
		BootImageDef::image_header header;

		log("SD: Reading header at 0x", Hex{header_addr}, "\n");

		if (read(header, header_addr))
			return header;
		else {
			log("Failed to read header\n");
			return {};
		}
	}

	bool load_image(uint32_t image_addr, uint32_t load_addr, uint32_t size) override
	{
		debug("SD: load_image\n");
		auto load_dst = std::span<uint8_t>{reinterpret_cast<uint8_t *>(load_addr), size};

		if (read(load_dst, image_addr))
			return true;
		else {
			log("Failed to read\n");
			return {};
		}
	}

	uint32_t get_first_header_addr(LoadTarget target) override
	{
		auto image_part_num = target == LoadTarget::App ? app_part_num : ssbl_part_num;

		uint32_t image_blockaddr = InvalidPartitionNum;

		// TODO: get_next_gpt_header(&gpt_hdr)
		gpt_header gpt_hdr;
		const uint32_t last_block = hsd.SdCard.BlockNbr;
		const uint32_t gpt_addrs[2] = {1, last_block - 1};

		for (auto blockaddr : gpt_addrs) {
			debug("Looking for gpt header at block ", blockaddr, "\n");
			read(gpt_hdr, blockaddr * BlockSize);
			if (validate_gpt_header(&gpt_hdr, blockaddr, last_block)) {

				image_blockaddr = get_gpt_partition_startaddr(gpt_hdr, image_part_num);
				if (image_blockaddr != InvalidPartitionNum)
					break;
			}
		}
		if (image_blockaddr == InvalidPartitionNum) {
			pr_err("No valid GPT header found\n");
			return {};
		}

		debug("GPT partition header says partition ", image_part_num, " is at block ", image_blockaddr, "\n");
		return image_blockaddr * BlockSize;
	}

	bool has_error() { return _has_error; }

private:
	static constexpr uint32_t ssbl_part_num = BootImageDef::SDCardSSBLPartition - 1;
	static constexpr uint32_t app_part_num = BootImageDef::SDCardAppPartition - 1;

	static constexpr uint32_t InvalidPartitionNum = 0xFFFFFFFF;

	SD_HandleTypeDef hsd;
	bool _has_error = false;

	// Given a gpt_header, find the starting address (LBA) of the SSBL partition
	// Validate the gpt partition entry, too.
	uint64_t get_gpt_partition_startaddr(gpt_header &gpt_hdr, uint32_t image_part_num)
	{
		std::array<gpt_entry, 4> ptes;

		// Make sure we're loading 512B into a variable that's 512B
		static_assert(sizeof(ptes) == 512, "An array of four gpt_entry must be 512 bytes");

		uint32_t part_lba = gpt_hdr.partition_entry_lba + (image_part_num / 4);
		read(ptes, part_lba * BlockSize);
		if (validate_partition_entry(ptes[image_part_num % 4])) {
			return ptes[image_part_num % 4].starting_lba;
		}

		return InvalidPartitionNum;
	}

	// Read block-aligned data from SD card
	bool read_blocks(std::span<uint8_t> buf, uint32_t first_block_num)
	{
		constexpr uint32_t timeout = 2000; // 2 seconds
		uint32_t block_num = first_block_num;
		uint32_t bytes_to_read = buf.size_bytes();
		auto read_ptr = buf.data();

		bool buf_is_aligned = !((uint32_t)buf.data() & 0b11);

		debug("Read ", bytes_to_read, " bytes (", bytes_to_read / BlockSize);
		debug(" blocks) starting from block ", block_num, " to ", Hex{(uint32_t)buf.data()}, "\n");

		if (buf_is_aligned && (bytes_to_read >= BlockSize)) {
			uint32_t numblocks = bytes_to_read / BlockSize;

			debug("Reading ", numblocks, " block(s) aligned\n");
			if (HAL_SD_ReadBlocks(&hsd, read_ptr, block_num, numblocks, timeout) != HAL_OK)
				read_error();

			uint32_t bytes_read = numblocks * BlockSize;
			debug("...read ", bytes_read, " bytes of ", bytes_to_read, "\n");
			if (bytes_to_read == bytes_read)
				return true;

			// setup for reading the remainder
			bytes_to_read -= bytes_read;
			block_num += numblocks;
			read_ptr += bytes_read;
		}

		while (bytes_to_read > 0) {
			alignas(4) uint8_t aligned_data[BlockSize];
			constexpr uint32_t numblocks = 1;

			debug(bytes_to_read, "B remain. Reading one block unaligned into `aligned_data`\n");
			if (HAL_SD_ReadBlocks(&hsd, aligned_data, block_num, numblocks, timeout) != HAL_OK)
				read_error();

			uint32_t bytes_to_copy = std::min(bytes_to_read, BlockSize);
			debug("Copy first ", bytes_to_copy, " from `aligned_data` to ", Hex{uint32_t(read_ptr)}, "\n");
			for (unsigned i = 0; i < bytes_to_copy; i++) {
				*read_ptr++ = aligned_data[i];
			}
			bytes_to_read -= bytes_to_copy;
			block_num++;
		}
		return true;
	}

	// Read from SD card into a generic data structure from an address
	bool read(std::span<uint8_t> data, uint32_t address)
	{
		// We only support cards with a BlockSize of 512B
		if (hsd.SdCard.BlockSize != 512) {
			if (hsd.SdCard.BlockSize > 0)
				sdcard_error();
			return false;
		}

		debug("SD read from 0x", Hex{address}, " to 0x", Hex{(uint32_t)data.data()}, " for ", data.size(), "B\n");

		uint32_t aligned_addr = (address / BlockSize) * BlockSize;
		if (aligned_addr < address) {
			uint32_t bytes_to_drop = address - aligned_addr;
			uint32_t bytes_to_keep = std::min<uint32_t>(BlockSize - bytes_to_drop, data.size());
			std::array<uint8_t, BlockSize> tmp;
			debug("Read a block, drop first ", bytes_to_drop, " bytes, keep last ", bytes_to_keep, "\n");
			read_blocks(tmp, address / BlockSize);

			debug("Copying ", bytes_to_keep, " bytes from tmp to `data` (", Hex{(uint32_t)data.data()}, ")\n");
			auto source = std::span<uint8_t>{&tmp[bytes_to_drop], &tmp[512]};
			for (auto i = 0; auto &d : data.subspan(0, bytes_to_keep))
				d = source[i++];

			// for (unsigned i = 0; i < bytes_to_keep; i++) {
			// 	data[i] = tmp[i + bytes_to_drop];
			// }

			if (data.size() == bytes_to_keep) {
				debug("SD read done\n");
				return true;
			}

			address += bytes_to_keep;
			data = data.subspan(bytes_to_keep);
			debug("Moved data ptr to ", Hex{(uint32_t)data.data()}, " size= ", data.size(), "\n");
			debug("address 0x", Hex{address}, " now is aligned\n");
		}

		return read_blocks(data, address / BlockSize);
	}

	bool read(auto &data, uint32_t address)
	{
		std::span<uint8_t> dest{reinterpret_cast<uint8_t *>(&data), sizeof(data)};
		return read(dest, address);
	}

	void init_error()
	{
		_has_error = true;
		// panic("SDInit not ok");
	}

	void sdcard_error()
	{
		_has_error = true;
		panic("SD Card block size is ", hsd.SdCard.BlockSize, " but only 512 is supported\n");
	}

	void read_error()
	{
		_has_error = true;
		pr_err("HAL Read SD not ok\n");
	}

	constexpr static uint32_t BlockSize = 512;
};
