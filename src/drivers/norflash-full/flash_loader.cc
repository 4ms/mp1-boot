#include "flash_loader.hh"
#include "print_messages.hh"

FlashLoader::FlashLoader() {}

bool FlashLoader::check_flash_chip()
{
	return flash.check_chip_id(0x180001, 0x00180001); // 182001 or 186001 or 1840EF
}

bool FlashLoader::write_sectors(uint32_t base_addr, std::span<const uint8_t> buffer)
{
	constexpr bool verbose_log = false;

	// round up to smallest 4kB blocks that contains image
	uint32_t block_size = (buffer.size() < 64 * 1024) ? 4096 : 64 * 1024;
	uint32_t block_size_mask = block_size - 1;

	unsigned bytes_to_erase = (buffer.size() + block_size_mask) & ~block_size_mask;

	uint32_t addr = base_addr;

	while (bytes_to_erase) {
		if constexpr (verbose_log)
			log("Erasing %u-byte block at 0x%x\n", block_size, (unsigned)addr);

		if (!flash.erase(block_size, addr)) {
			pr_err("ERROR: Flash failed to erase block\n");
			return false;
		}
		addr += block_size;
		bytes_to_erase -= block_size;
	}

	if constexpr (verbose_log)
		log("Writing 0x%x B to %x\n", buffer.size(), base_addr);

	if (!flash.write(buffer.data(), base_addr, buffer.size())) {
		pr_err("ERROR: Flash failed to write\n");
		return false;
	}

	return true;
}

bool FlashLoader::write_sectors(uint32_t base_addr, std::span<const char> buffer)
{
	auto buff_u8 = std::span<const uint8_t>{reinterpret_cast<const uint8_t *>(buffer.data()), buffer.size()};
	return write_sectors(base_addr, buff_u8);
}

bool FlashLoader::read_sectors(uint32_t base_addr, std::span<uint8_t> buffer)
{
	return flash.read(buffer.data(), base_addr, buffer.size());
}
