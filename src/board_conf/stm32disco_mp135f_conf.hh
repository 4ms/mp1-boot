#pragma once
#include "drivers/clocks.hh"
#include "drivers/i2c_conf.hh"
#include "drivers/leds.hh"

namespace Board
{
using RedLED = Led<GPIO::A, PinNum::_6, LedActive::High>;
using GreenLED = Led<GPIO::A, PinNum::_6, LedActive::High>;

using RedLED2 = RedLED;
using GreenLED2 = GreenLED;

using OrangeLED = RedLED2;
using BlueLED = GreenLED;

constexpr PinConf BootSelectPin{GPIO::A, PinNum::_14}; // DFU boot button
constexpr bool UseBootSelect = true;

constexpr bool UseFreezePin = true;
constexpr PinConf FreezePin{GPIO::A, PinNum::_13}; // DEBUG3 pin

constexpr uint32_t ConsoleUART = UART4_BASE;
constexpr PinConf UartRX{GPIO::D, PinNum::_6, PinAF::AF_8};
constexpr PinConf UartTX{GPIO::D, PinNum::_8, PinAF::AF_8};

namespace NORFlash
{
constexpr bool HasNORFlash = true;
constexpr PinConf d2{GPIO::Unused, PinNum::_0};
constexpr PinConf d3{GPIO::Unused, PinNum::_0};
} // namespace NORFlash

namespace PMIC
{
constexpr bool HasSTPMIC = true;
constexpr I2C_Config I2C_config{
	.periph = I2C_Periph::I2C4_,
	.sda_pin = {GPIO::B, PinNum::_9, PinAF::AF_6},
	.scl_pin = {GPIO::E, PinNum::_15, PinAF::AF_6},
};
} // namespace PMIC

constexpr uint32_t HSE_Clock_Hz = 24'000'000;
constexpr uint32_t MPU_MHz = 1000;
constexpr auto ClockType = SystemClocks::HSEClockSource::Resonator;

} // namespace Board
