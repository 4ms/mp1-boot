#include "board_conf.hh"
#include "drivers/pinconf.hh"
#include "qspi_flash_driver.hh"
#include "qspi_flash_registers.h"

// #define XSPI_DEBUG_PRINTF

#ifdef XSPI_DEBUG_PRINTF
#include "print.hh"
#define xspi_printf print
#else
#define xspi_printf(...)
#endif

namespace mdrivlib
{

// #define XSPI_DO_TESTS

uint32_t QSpiFlash::get_64kblock_addr(unsigned block64k_num)
{
	if (block64k_num >= Board::NORFlash::SizeBytes / QSPI_64KBLOCK_SIZE)
		return 0;

	return block64k_num * QSPI_64KBLOCK_SIZE;
}

uint32_t QSpiFlash::get_32kblock_addr(unsigned block32k_num)
{
	if (block32k_num >= Board::NORFlash::SizeBytes / QSPI_32KBLOCK_SIZE)
		return 0;

	return block32k_num * QSPI_32KBLOCK_SIZE;
}

uint32_t QSpiFlash::get_sector_addr(unsigned sector_num)
{
	if (sector_num >= Board::NORFlash::SizeBytes / QSPI_SECTOR_SIZE)
		return 0;

	return sector_num * QSPI_SECTOR_SIZE;
}

QSpiFlash::QSpiFlash()
	: data_mode{Board::NORFlash::io_mode == Board::NORFlash::IOMode::QuadSPI ? XSPI_DATA_4_LINES :
				Board::NORFlash::io_mode == Board::NORFlash::IOMode::DualSPI ? XSPI_DATA_2_LINES :
																			   XSPI_DATA_1_LINE}
	, quad_write_cmd{Board::NORFlash::chip_id == Board::NORFlash::ChipID::IS25L		? IS25LQ0x0B_QUAD_IN_FAST_PROG_CMD :
					 Board::NORFlash::chip_id == Board::NORFlash::ChipID::W25Q128JV ? W25Q128JV_QUAD_IN_FAST_PROG_CMD :
					 Board::NORFlash::chip_id == Board::NORFlash::ChipID::W25Q16JV	? W25Q128JV_QUAD_IN_FAST_PROG_CMD :
																					  S25FLxxxL_QUAD_IN_FAST_PROG_CMD}
	, quad_read_dummy_cycles{
		  Board::NORFlash::chip_id == Board::NORFlash::ChipID::IS25L	 ? IS25LQ0x0B_QSPI_DUMMY_CYCLES_READ_QUAD_IO :
		  Board::NORFlash::chip_id == Board::NORFlash::ChipID::W25Q128JV ? W25Q128JV_QSPI_DUMMY_CYCLES_READ_QUAD_IO :
		  Board::NORFlash::chip_id == Board::NORFlash::ChipID::W25Q16JV	 ? W25Q128JV_QSPI_DUMMY_CYCLES_READ_QUAD_IO :
																		   S25FLxxxL_QSPI_DUMMY_CYCLES_READ_QUAD_IO}
{
	instance_ = this;

	handle.Instance = QUADSPI;

	HAL_XSPI_DeInit(&handle);

	__HAL_RCC_QSPI_CLK_ENABLE();

	// Initialize chip pins in single IO mode
	GPIO_init_IO0_IO1();

	__HAL_RCC_QSPI_FORCE_RESET();
	__HAL_RCC_QSPI_RELEASE_RESET();

	handle.Init.ClockPrescaler = 7;
	handle.Init.FifoThresholdByte = 4;							// was 1
	handle.Init.SampleShifting = XSPI_SAMPLE_SHIFTING_NONE;		// was HALF_CYCLE
	handle.Init.MemorySize = Board::NORFlash::address_bits - 1; // 23 here, is 25 in ST example
	handle.Init.ChipSelectHighTimeCycle = 8; // 1 is too fast after an erase: 8 guarentees required >= 50ns
	handle.Init.ClockMode = XSPI_CLOCK_MODE_0;
	handle.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;

	[[maybe_unused]] auto res = HAL_XSPI_Init(&handle);
	if (res != HAL_OK)
		xspi_printf("Failed to init QSPI: ", res, "\n");

	init_command(&s_command);

	QSPI_status = STATUS_READY; // NOLINT

	if ([[maybe_unused]] auto res = Reset())
		xspi_printf("Failed to reset QSPI: ", res, "\n");

	if (Board::NORFlash::chip_id == Board::NORFlash::W25Q128JV || Board::NORFlash::chip_id == Board::NORFlash::W25Q16JV)
	{
		// Set non-volatile QE bit to disable /HOLD and /WP pins, or else BOOTROM will fail to read.
		// Only needed on the -IM/-JM parts (-IQ/-JQ parts have it set by default).
		if ([[maybe_unused]] auto res = winbond_set_quad_enable())
			xspi_printf("Failed to set Winbond QE bit: ", res, "\n");
	}

	if (Board::NORFlash::io_mode == Board::NORFlash::QuadSPI) {
		// Now that chip is in QSPI mode, IO2 and IO3 can be initialized
		if (Board::NORFlash::chip_id == Board::NORFlash::IS25L)
			enter_memory_QPI(); // S25FL doesn't seem to need this?
		GPIO_init_IO2_IO3_AF();
	}

	if ([[maybe_unused]] auto res = auto_polling_mem_ready(HAL_XSPI_TIMEOUT_DEFAULT_VALUE))
		xspi_printf("Failed to get autopolling ready for QSPI: ", res, "\n");

	if ([[maybe_unused]] auto res = write_enable())
		xspi_printf("Failed to write enable QSPI: ", res, "\n");

#ifdef XSPI_DO_TESTS
	xspi_printf("Running QSPI tests...\n");
	// Erase(ENTIRE_CHIP, 0, EXECUTE_FOREGROUND);
	if (!test()) {
		__BKPT();
	}
#endif
}

void QSpiFlash::GPIO_init_IO0_IO1()
{
	Board::NORFlash::clk.init(PinMode::Alt);
	Board::NORFlash::cs.init(PinMode::Alt);
	Board::NORFlash::d0.init(PinMode::Alt);
	Board::NORFlash::d1.init(PinMode::Alt);

	// Set /HOLD and /WP pins high to disable holding and write protection, until we enter QPI mode
	if (Board::NORFlash::d2.gpio != GPIO::Unused) {
		Board::NORFlash::d2.init(PinMode::Output);
		Board::NORFlash::d2.high();
	}

	if (Board::NORFlash::d3.gpio != GPIO::Unused) {
		Board::NORFlash::d3.init(PinMode::Output);
		Board::NORFlash::d3.high();
	}
}

void QSpiFlash::GPIO_init_IO2_IO3_AF()
{
	Board::NORFlash::d2.init(PinMode::Alt);
	Board::NORFlash::d3.init(PinMode::Alt);
}

void QSpiFlash::init_command(QSPI_CommandTypeDef *s_command)
{
	s_command->InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command->AddressWidth = XSPI_ADDRESS_24_BITS; // TODO: make this 32 if defs.AddrBits == 32?
	// s_command->DdrMode = XSPI_DDR_MODE_DISABLE;
	// s_command->DdrHoldHalfCycle = XSPI_DDR_HHC_ANALOG_DELAY;
	s_command->SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
}

HAL_StatusTypeDef QSpiFlash::Reset()
{
	// Enable Reset
	// s_command.Instruction = RESET_ENABLE_CMD;
	// s_command.AddressMode = XSPI_ADDRESS_NONE;
	// s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	// s_command.DataMode = XSPI_DATA_NONE;
	// s_command.DummyCycles = 0;

	s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command.AddressWidth = XSPI_ADDRESS_24_BITS;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DTRMode = XSPI_DTR_MODE_DISABLE;
	s_command.DelayHoldHalfCycle = XSPI_DHHC_ANALOG_DELAY;
	s_command.SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
	s_command.Instruction = RESET_ENABLE_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.Address = 0;
	s_command.DataMode = XSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.IOSelect = HAL_XSPI_SELECT_IO_3_0;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	// Delay (from ST example project)
	for (int temp = 0; temp < 0x2f; temp++) {
		__NOP();
	}

	// Perform Reset
	// s_command.Instruction = RESET_CMD;
	s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command.AddressWidth = XSPI_ADDRESS_24_BITS;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DTRMode = XSPI_DTR_MODE_DISABLE;
	s_command.DelayHoldHalfCycle = XSPI_DHHC_ANALOG_DELAY;
	s_command.SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
	s_command.Instruction = RESET_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.Address = 0;
	s_command.DataMode = XSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.IOSelect = HAL_XSPI_SELECT_IO_3_0;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

// Tests entire chip sector-by-sector
// Returns 1 if passed, 0 if failed
bool QSpiFlash::test()
{
	unsigned sector;
	for (sector = 0; sector < Board::NORFlash::SizeBytes / QSPI_SECTOR_SIZE; sector++) {
		if (!test_sector(sector))
			return false; // fail
	}
	return true; // pass
}

// Tests one sector
// Returns true if passed, false if failed
bool QSpiFlash::test_sector(uint8_t sector_num)
{
	uint32_t i;
	uint8_t test_buffer[QSPI_SECTOR_SIZE];
	uint32_t test_addr = get_sector_addr(sector_num);

	read(test_buffer, test_addr, QSPI_SECTOR_SIZE);

	for (i = 0; i < QSPI_SECTOR_SIZE; i++)
		test_buffer[i] = test_encode_num(i, sector_num);

	while (!is_ready())
		;

	if (!erase(SECTOR, test_addr))
		return false;

	while (!is_ready())
		;

	for (i = 0; i < (QSPI_SECTOR_SIZE / QSPI_PAGE_SIZE); i++) {
		// Benchmark: ~380us/page
		if (!write_page(&(test_buffer[i * QSPI_PAGE_SIZE]), test_addr + i * QSPI_PAGE_SIZE, QSPI_PAGE_SIZE))
			return false;
		while (!is_ready())
			;
	}

	for (i = 0; i < QSPI_SECTOR_SIZE; i++)
		test_buffer[i] = 0;

	if (!read(test_buffer, test_addr, QSPI_SECTOR_SIZE))
		return false;

	while (!is_ready())
		;

	for (i = 0; i < (QSPI_SECTOR_SIZE - 1); i++) {
		if (test_buffer[i] != test_encode_num(i, sector_num))
			return false;
	}

	return true;
}

uint8_t QSpiFlash::test_encode_num(uint32_t num, uint32_t sector_num)
{
	return (((num * 9) + (num >> 7)) + sector_num) & 0xFF;
}

bool QSpiFlash::erase(uint32_t size, uint32_t base_addr)
{
	uint8_t status;
	uint32_t timeout;

	if (write_enable() != HAL_OK)
		return false;

	if (size == SECTOR) {
		s_command.Instruction = SECTOR_ERASE_CMD;
		s_command.Address = base_addr;
		s_command.AddressMode = XSPI_ADDRESS_1_LINE;
		timeout = QSPI_SECTOR_ERASE_MAX_TIME_SYSTICKS;
	} else if (size == BLOCK_32K) {
		s_command.Instruction = BLOCK_ERASE_32K_CMD;
		s_command.Address = base_addr;
		s_command.AddressMode = XSPI_ADDRESS_1_LINE;
		timeout = QSPI_32KBLOCK_ERASE_MAX_TIME_SYSTICKS;
	} else if (size == BLOCK_64K) {
		s_command.Instruction = BLOCK_ERASE_64K_CMD;
		s_command.Address = base_addr;
		s_command.AddressMode = XSPI_ADDRESS_1_LINE;
		timeout = QSPI_64KBLOCK_ERASE_MAX_TIME_SYSTICKS;
	} else if (size == ENTIRE_CHIP) {
		s_command.Instruction = BULK_ERASE_CMD;
		s_command.Address = XSPI_ADDRESS_NONE;
		s_command.AddressMode = XSPI_ADDRESS_NONE;
		timeout = QSPI_CHIP_ERASE_MAX_TIME_SYSTICKS;
	} else
		return false; // invalid size

	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_NONE;
	s_command.DummyCycles = 0;

	if (HAL_XSPI_Command(&handle, &s_command, 200 /*HAL_XSPI_TIMEOUT_DEFAULT_VALUE*/) != HAL_OK)
		return false;

	status = auto_polling_mem_ready(timeout);

	if (status != HAL_OK)
		return false;

	return true;
}

bool QSpiFlash::write(const uint8_t *pData, uint32_t write_addr, uint32_t num_bytes)
{
	uint32_t end_addr, current_size, current_addr;

	if (write_addr >= Board::NORFlash::SizeBytes)
		return false;

	if (write_addr + num_bytes >= Board::NORFlash::SizeBytes)
		return false;

	// Calculation of the size between the write address and the end of the page
	current_addr = 0;

	while (current_addr <= write_addr)
		current_addr += QSPI_PAGE_SIZE;

	current_size = current_addr - write_addr;

	// Check if the size of the data is less than the remaining in the page
	if (current_size > num_bytes)
		current_size = num_bytes;

	current_addr = write_addr;
	end_addr = write_addr + num_bytes;

	// Perform the write page by page
	do {
		if (write_enable() != HAL_OK)
			return false;

		if (Board::NORFlash::io_mode == Board::NORFlash::QuadSPI) {
			s_command.Instruction = quad_write_cmd;
		} else {
			// This is for DDR mode also, on S25FL128L/256L
			s_command.Instruction = PAGE_PROG_CMD;
		}
		s_command.AddressMode = XSPI_ADDRESS_1_LINE;
		s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
		s_command.DataMode = data_mode;
		s_command.DummyCycles = 0;
		s_command.Address = current_addr;
		s_command.DataLength = current_size;

		if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return false;

		if (HAL_XSPI_Transmit(&handle, const_cast<uint8_t *>(pData), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return false;

		if (auto_polling_mem_ready(HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return false;

		current_addr += current_size;
		pData += current_size;
		current_size = ((current_addr + QSPI_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : QSPI_PAGE_SIZE;
	} while (current_addr < end_addr);

	return true;
}

// Writes within a page (256 Bytes)
// Data to be written must not cross page boundaries.
// Setting use_interrupt to 1 means HAL_QSPI_TxCpltCallback() interrupt will be called when TX is
// done, but you must still check the chip status before accessing it again.
//
bool QSpiFlash::write_page(const uint8_t *pData, uint32_t write_addr, uint32_t num_bytes)
{
	// Cannot write more than a page
	if (num_bytes > QSPI_PAGE_SIZE)
		return false;

	uint32_t start_page = write_addr >> QSPI_PAGE_ADDRESS_BITS;
	uint32_t end_page = (write_addr + num_bytes - 1) >> QSPI_PAGE_ADDRESS_BITS;

	// Cannot cross page boundaries
	if (start_page != end_page)
		return false;

	if (write_enable() != HAL_OK)
		return false;

	if (Board::NORFlash::io_mode == Board::NORFlash::QuadSPI) {
		s_command.Instruction = quad_write_cmd;
	} else {
		// This is for DDR mode also, on S25FL128L/256L
		s_command.Instruction = PAGE_PROG_CMD;
	}
	s_command.AddressMode = XSPI_ADDRESS_1_LINE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = data_mode;
	s_command.DummyCycles = 0;
	s_command.Address = write_addr;
	s_command.DataLength = num_bytes;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;

	QSPI_status = STATUS_TXING;
	if (HAL_XSPI_Transmit(&handle, const_cast<uint8_t *>(pData), HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;

	if (auto_polling_mem_ready(HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;
	QSPI_status = STATUS_READY;

	return true;
}

bool QSpiFlash::read(uint8_t *pData, uint32_t read_addr, uint32_t num_bytes)
{
	uint8_t status;

	// Todo: take advantage of AX Read mode (see datasheet)
	// by setting:
	// s_command.SIOOMode			= QSPI_SIOO_INST_ONLY_FIRST_CMD;
	// s_command.AlternateByteMode 	= XSPI_ALT_BYTES_4_LINES;
	// s_command.AlternateBytesWidth 	= XSPI_ALT_BYTES_8_BITS;
	// s_command.AlternateBytes 		= 0xA0;

	if (Board::NORFlash::io_mode == Board::NORFlash::QuadSPI) {
		s_command.Instruction = QUAD_INOUT_FAST_READ_CMD;
		s_command.AddressMode = XSPI_ADDRESS_4_LINES;
		s_command.DummyCycles = quad_read_dummy_cycles;
		s_command.AlternateByteMode = XSPI_ALT_BYTES_4_LINES;
		s_command.AlternateBytesWidth = XSPI_ALT_BYTES_8_BITS;
		s_command.AlternateBytes = 0x00;

		// TODO:: DDR Mode: DUAL_OUT_FAST_READ_CMD
	} else {
		s_command.Instruction = FAST_READ_CMD;
		s_command.AddressMode = XSPI_ADDRESS_1_LINE;
		s_command.DummyCycles = QSPI_DUMMY_CYCLES_FAST_READ;
		s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	}
	s_command.Address = read_addr;
	s_command.DataMode = data_mode;
	s_command.DataLength = num_bytes;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	status = HAL_XSPI_Receive(&handle, pData, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	QSPI_status = STATUS_READY;

	if (status != HAL_OK)
		return false;

	return true;
}

bool QSpiFlash::read_chip_id(uint32_t *chip_id_ptr)
{
	s_command.Instruction = READ_ID_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DataLength = 3;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;

	if (HAL_XSPI_Receive(&handle, (uint8_t *)chip_id_ptr, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;

	return true;
}
bool QSpiFlash::check_chip_id(uint32_t expected_id, uint32_t mask)
{
	uint32_t timeout = 100;
	uint32_t id = 0;
	do {
		HAL_Delay(1);
		read_chip_id(&id);
		id &= mask;
		if (id == expected_id)
			return true;
	} while (timeout--);
	return false;
}

bool QSpiFlash::read_config(uint32_t *data)
{
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DataLength = 4;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;

	if (HAL_XSPI_Receive(&handle, (uint8_t *)data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return false;

	return true;
}

HAL_StatusTypeDef QSpiFlash::write_enable()
{
	XSPI_AutoPollingTypeDef s_config;

	/* Enable write operations */
	s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = WRITE_ENABLE_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DTRMode = XSPI_DTR_MODE_DISABLE;
	s_command.DelayHoldHalfCycle = XSPI_DHHC_ANALOG_DELAY;
	s_command.SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
	s_command.DataLength = 1;
	s_command.IOSelect = HAL_XSPI_SELECT_IO_3_0;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	/* Configure automatic polling mode to wait for write enabling */
	s_config.MatchValue = QSPI_SR_WREN;
	s_config.MatchMask = QSPI_SR_WREN;
	s_config.MatchMode = XSPI_MATCH_MODE_AND;
	s_config.IntervalTime = 0x10;
	s_config.AutomaticStop = XSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.DataMode = XSPI_DATA_1_LINE;
	ConfigCmd = s_command;
	if (HAL_XSPI_AutoPolling(&handle, &s_config, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

/**
 * @brief	This function put QSPI memory in QPI mode (quad I/O).
 * @retval None
 */
HAL_StatusTypeDef QSpiFlash::enter_memory_QPI()
{
	XSPI_AutoPollingTypeDef s_config;

	s_command.Instruction = WRITE_ENABLE_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_NONE;
	s_command.DummyCycles = 0;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	// FIXME: Some chips need this: some use 0x38 enter QSPI mode
	s_command.Instruction = WRITE_STATUS_REG_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DataLength = 1;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	uint8_t reg = QSPI_SR_QUADEN;
	if (HAL_XSPI_Transmit(&handle, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	/* 40ms	Write Status/Configuration Register Cycle Time */
	HAL_Delay(8);

	/* Configure automatic polling mode to wait the QUADEN bit=1 and WIP bit=0 */
	s_config.MatchValue = QSPI_SR_QUADEN;
	s_config.MatchMask = QSPI_SR_QUADEN /*|QSPI_SR_WIP*/;
	s_config.MatchMode = XSPI_MATCH_MODE_AND;
	s_config.IntervalTime = 0x10;
	s_config.AutomaticStop = XSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.DataMode = XSPI_DATA_1_LINE;

	ConfigCmd = s_command;
	if (HAL_XSPI_AutoPolling(&handle, &s_config, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

/**
 * @brief	Sets the Quad Enable (QE) bit in Status Register-2 on Winbond W25Qxx chips, if not already set.
 *			This is a non-volatile write, so it persists across power cycles. On -IQ/-JQ parts QE defaults
 *			to 1 and nothing is written.
 * @retval HAL_OK if QE is set (or was already set)
 */
HAL_StatusTypeDef QSpiFlash::winbond_set_quad_enable()
{
	auto read_sr2 = [this](uint8_t &sr2) -> HAL_StatusTypeDef {
		s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
		s_command.Instruction = W25Q_READ_STATUS_REG2_CMD;
		s_command.AddressMode = XSPI_ADDRESS_NONE;
		s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
		s_command.DataMode = XSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
		s_command.DTRMode = XSPI_DTR_MODE_DISABLE;
		s_command.DelayHoldHalfCycle = XSPI_DHHC_ANALOG_DELAY;
		s_command.SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
		s_command.DataLength = 1;
		s_command.IOSelect = HAL_XSPI_SELECT_IO_3_0;

		if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

		if (HAL_XSPI_Receive(&handle, &sr2, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

		return HAL_OK;
	};

	uint8_t sr2 = 0;
	if (read_sr2(sr2) != HAL_OK)
		return HAL_ERROR;

	if (sr2 & W25Q_SR2_QE) {
		xspi_printf("Winbond QE bit already set (SR2=0x", Hex{sr2}, ")\n");
		return HAL_OK;
	}

	xspi_printf("Winbond QE bit not set (SR2=0x", Hex{sr2}, "), setting it\n");

	// Write Enable (0x06), not Volatile SR Write Enable (0x50): we want QE to be non-volatile
	if (write_enable() != HAL_OK)
		return HAL_ERROR;

	sr2 |= W25Q_SR2_QE;

	s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = W25Q_WRITE_STATUS_REG2_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DataLength = 1;

	if (HAL_XSPI_Command(&handle, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	if (HAL_XSPI_Transmit(&handle, &sr2, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	// tW (write status register time) is up to 15ms
	if (auto_polling_mem_ready(HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	// Verify
	if (read_sr2(sr2) != HAL_OK)
		return HAL_ERROR;

	if (!(sr2 & W25Q_SR2_QE)) {
		xspi_printf("Winbond QE bit failed to set (SR2=0x", Hex{sr2}, ")\n");
		return HAL_ERROR;
	}

	return HAL_OK;
}

/**
 * @brief	Repeatedly reads the status register of the chip and waits until it indicates Write In
 * Progress is complete.
 * @param	Timeout
 * @retval None
 */
HAL_StatusTypeDef QSpiFlash::auto_polling_mem_ready(uint32_t Timeout)
{
	XSPI_AutoPollingTypeDef s_config;

	// Configure automatic polling mode to wait for memory ready
	s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DTRMode = XSPI_DTR_MODE_DISABLE;
	s_command.DelayHoldHalfCycle = XSPI_DHHC_ANALOG_DELAY;
	s_command.SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
	s_command.DataLength = 1;
	s_command.IOSelect = HAL_XSPI_SELECT_IO_3_0;

	s_config.MatchValue = 0;
	s_config.MatchMask = QSPI_SR_WIP;
	s_config.MatchMode = XSPI_MATCH_MODE_AND;
	s_config.IntervalTime = 0x10;
	s_config.AutomaticStop = XSPI_AUTOMATIC_STOP_ENABLE;

	// Set XSPI internal global:
	ConfigCmd = s_command;
	if (HAL_XSPI_AutoPolling(&handle, &s_config, Timeout) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

/**
 * @brief	Sets up auto-polling to call the HAL_XSPI_StatusMatchCallback() when the status register
 * indicates Write In Progress is cleared.
 * @param	None
 * @retval None
 */

HAL_StatusTypeDef QSpiFlash::auto_polling_mem_ready_it()
{
	XSPI_AutoPollingTypeDef s_config;

	// Configure automatic polling mode to wait for memory ready
	s_command.InstructionMode = XSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = XSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = XSPI_ALT_BYTES_NONE;
	s_command.DataMode = XSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DTRMode = XSPI_DTR_MODE_DISABLE;
	s_command.DelayHoldHalfCycle = XSPI_DHHC_ANALOG_DELAY;
	s_command.SIOOMode = XSPI_SIOO_INST_EVERY_CMD;
	s_command.DataLength = 1;
	s_command.IOSelect = HAL_XSPI_SELECT_IO_3_0;

	s_config.MatchValue = 0;
	s_config.MatchMask = QSPI_SR_WIP;
	s_config.MatchMode = XSPI_MATCH_MODE_AND;
	s_config.IntervalTime = 0x10;
	s_config.AutomaticStop = XSPI_AUTOMATIC_STOP_ENABLE;

	QSPI_status = STATUS_WIP;

	ConfigCmd = s_command;
	if (HAL_XSPI_AutoPolling_IT(&handle, &s_config) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

QSpiFlash *QSpiFlash::instance_;

// Callbacks & IRQ handlers

extern "C" void HAL_XSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
	QSpiFlash::instance_->QSPI_status = QSpiFlash::STATUS_READY;
}

extern "C" void HAL_XSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	if (QSpiFlash::instance_->QSPI_status == QSpiFlash::STATUS_RXING)
		QSpiFlash::instance_->QSPI_status = QSpiFlash::STATUS_READY;
}

extern "C" void HAL_XSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	if (QSpiFlash::instance_->QSPI_status == QSpiFlash::STATUS_TXING)
		QSpiFlash::instance_->QSPI_status = QSpiFlash::STATUS_TX_COMPLETE;
}

} // namespace mdrivlib
