#include "boot_media_loader.hh"
#include "delay.h"
#include "drivers/clocks.hh"
#include "drivers/ddr/ram_tests.hh"
#include "drivers/ddr/stm32mp1_ram.h"
#include "drivers/leds.hh"
#include "drivers/pmic.hh"
#include "drivers/uart.hh"
#include "norflash/norflash-loader.hh"
#include "print.hh"
#include "stm32mp157cxx_ca7.h"
#include "systeminit.h"

// Note: select your board configuration in board_conf.hh
#include "board_conf.hh"

void main()
{
	Board::OrangeLED led;
	Board::DFUMode.init(PinMode::Input, PinPull::Up);

	auto clockspeed = SystemClocks::init_core_clocks(Board::HSE_Clock_Hz, Board::MPU_MHz, Board::ClockType);
	security_init();

	Uart<Board::ConsoleUART> console(Board::UartRX, Board::UartTX, 115200);
	print("\n\nMP1-Boot\n\n");
	print("MPU Clock: ", clockspeed, " Hz\n");

	if constexpr (Board::PMIC::HasSTPMIC) {
		STPMIC1 pmic{Board::PMIC::I2C_config};

		if (!pmic.setup_vddcore_pwr())
			panic("Could not setup PMIC VDDCORE\n");

		if (!pmic.setup_ddr3_pwr())
			panic("Could not setup PMIC DDR voltages\n");
	}

	print("Initializing RAM\n");
	stm32mp1_ddr_setup();

	print("Testing RAM.\n");
	RamTests::run_all(DRAM_MEM_BASE, stm32mp1_ddr_get_size());

	// Look for DFU pin pulled down
	if (!Board::DFUMode.read()) {
		print("DFU Mode pin detected low => Starting USB DFU mode.\n");

		// Start USB DFU
		NorFlashDFULoader flash_writer;
		flash_writer.start();

		print("Attach a USB cable and run dfu-util\n");

		// TODO: timer, if not connection in 10seconds? then continue booting
		while (true)
			;
	}

	auto boot_method = BootDetect::read_boot_method();
	print("Booted from ", BootDetect::bootmethod_string(boot_method).data(), "\n");

	print("Loading app image...\n");

	BootMediaLoader loader{boot_method};

	bool image_ok = loader.load_image();

	if (image_ok) {
		print("Jumping to app\n");
		loader.boot_image();
	}

	// Should not reach here, but in case we do, blink LED rapidly
	print("FAILED!\n");
	constexpr uint32_t dlytime = 50000;
	while (true) {
		led.on();
		udelay(dlytime);
		led.off();
		udelay(dlytime);
	}
}

void putchar_s(const char c) { Uart<Board::ConsoleUART>::putchar(c); }
