#include <cstdint>

inline uint32_t read_raw_bootrom_itf()
{
	static constexpr uint32_t BOOTROM_PARAM_ADDR = 0x2FFC0078;
	auto val = *reinterpret_cast<uint32_t *>(BOOTROM_PARAM_ADDR);
	return val;
}
