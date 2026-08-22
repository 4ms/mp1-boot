#pragma once

#include <cstdint>
#include <span>

struct NorFlashWriter {
	NorFlashWriter();

	bool write(uint32_t nor_addr, std::span<const uint8_t> bytes);
	bool verify(uint32_t nor_addr, std::span<const uint8_t> bytes);
};
