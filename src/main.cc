// Note: select your board configuration in board_conf.hh
#include "board_conf.hh"

#include "boot_media_loader.hh"
#include "delay.h"
#include "drivers/clocks.hh"
#include "drivers/ddr/ram_tests.hh"
#include "drivers/ddr/stm32mp1_ram.h"
#include "drivers/leds.hh"
#include "drivers/pmic.hh"
#include "drivers/uart.hh"
#include "print.hh"
#include "stm32mp157cxx_ca7.h"
#include "systeminit.h"

void main()
{
	Board::OrangeLED led;

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

	auto boot_method = BootDetect::read_boot_method();
	print("Booted from ", BootDetect::bootmethod_string(boot_method).data(), "\n");

	BootLoader::LoadTarget image_type = BootLoader::LoadTarget::App;

	// Check Boot Select pin
	if constexpr (Board::UseBootSelect) {
		Board::BootSelectPin.init(PinMode::Input, PinPull::Up);
		// delay to allow pull-up to settle
		udelay(1000);
		if (!Board::BootSelectPin.read()) {
			image_type = BootLoader::LoadTarget::SSBL;
			print("Boot Select pin detected active: Loading alt image...\n");
		}
	}
	if (image_type == BootLoader::LoadTarget::App)
		print("Loading main app image...\n");

	BootMediaLoader loader{boot_method};
	bool image_ok = loader.load_image(image_type);

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
