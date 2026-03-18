#pragma once

#ifdef STM32MP13x
#define QSPI_HandleTypeDef XSPI_HandleTypeDef
#define QSPI_CommandTypeDef XSPI_RegularCmdTypeDef
#include "stm32mp13xx_hal_xspi.h"
#else
#include "stm32mp1xx_hal_qspi.h"
#endif

#include "board_conf.hh"
#include "qspi_flash_registers.h"
#include <cstdint>

namespace mdrivlib
{

// TODO: Add chip profiles
//  - chip id (so we can have check_id() that calls read_chip_id() and checks result
//  - flash size
//  - timeouts required for erasing
//  - Enter QPI mode method and commands
//  - preferred READ and WRITE commands for Single/Dual/Quad modes
//  - sector layout: [how??]
//  	  - S25FL127S: [4k x 16 erased with 0x20 + 64k x 255 erased with 0xD8]
//     - S25FL128L: [4k x 4096 erased with 0x20]
//  Maybe do something like this?
struct FlashDefS24FL127L {
	constexpr unsigned get_num_sectors() { return 16 + 255; }
	constexpr unsigned get_sector_size(unsigned sector_num) { return (sector_num < 16) ? 4096 : 64 * 1024; }
};
struct FlashDefS24FL127S {
	constexpr unsigned get_num_sectors() { return 4096; }
	constexpr unsigned get_sector_size(unsigned sector_num) { return 4096; }
};

class QSpiFlash {

public:
	enum ErasableSizes : uint32_t {
		SECTOR = QSPI_SECTOR_SIZE,
		BLOCK_32K = QSPI_32KBLOCK_SIZE,
		BLOCK_64K = QSPI_64KBLOCK_SIZE,
		ENTIRE_CHIP = 0xFFFFFFFF
	};

	enum FlashStatus { STATUS_READY, STATUS_WIP, STATUS_RXING, STATUS_RX_COMPLETE, STATUS_TXING, STATUS_TX_COMPLETE };

private:
	QSPI_HandleTypeDef handle;
	QSPI_CommandTypeDef s_command;

	const uint32_t data_mode;
	const int quad_write_cmd;
	const int quad_read_dummy_cycles;

	HAL_StatusTypeDef write_enable();
	void GPIO_init_IO0_IO1();
	void GPIO_init_IO2_IO3_AF();
	HAL_StatusTypeDef auto_polling_mem_ready(uint32_t Timeout);
	HAL_StatusTypeDef auto_polling_mem_ready_it();
	HAL_StatusTypeDef enter_memory_QPI();
	void init_command(QSPI_CommandTypeDef *s_command);

	bool done_TXing() { return QSPI_status == STATUS_TX_COMPLETE; }

	uint8_t test_encode_num(uint32_t num, uint32_t sector_num);

public:
	QSpiFlash();

	bool is_ready() { return QSPI_status == STATUS_READY; }

	bool test();
	bool test_sector(uint8_t sector_num);

	uint32_t get_64kblock_addr(unsigned block64k_num);
	uint32_t get_32kblock_addr(unsigned block32k_num);
	uint32_t get_sector_addr(unsigned sector_num);

	HAL_StatusTypeDef Reset();

	bool read(uint8_t *pData, uint32_t read_addr, uint32_t num_bytes);

	bool write(const uint8_t *pData, uint32_t write_addr, uint32_t num_bytes);
	bool write_page(const uint8_t *pData, uint32_t write_addr, uint32_t num_bytes);

	bool erase(uint32_t size, uint32_t base_addr);

	bool read_config(uint32_t *data);
	bool read_chip_id(uint32_t *chip_id_ptr);

	// Attempts a few times to read the ID, returns true if it matches the expected
	bool check_chip_id(uint32_t expected_id, uint32_t mask);

	uint32_t get_chip_size_bytes() { return Board::NORFlash::SizeBytes; }

	// for use in callbacks and IRQ:
	volatile enum FlashStatus QSPI_status = STATUS_READY;
	static QSpiFlash *instance_;
};

} // namespace mdrivlib
