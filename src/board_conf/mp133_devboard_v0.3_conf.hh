#pragma once
#include "drivers/clocks.hh"
#include "drivers/i2c_conf.hh"
#include "drivers/leds.hh"

namespace Board
{
using RedLED = Led<GPIO::B, PinNum::_5, LedActive::High>; // DEBUG0 pin
using GreenLED = Led<GPIO::A, PinNum::_8, LedActive::High>;

using RedLED2 = RedLED;
using GreenLED2 = GreenLED;

using OrangeLED = RedLED2;
using BlueLED = GreenLED;

constexpr PinConf BootSelectPin{GPIO::G, PinNum::_0, PinAF::AFNone, PinPolarity::Inverted}; // DFU boot button
constexpr bool UseBootSelect = true;

constexpr bool UseFreezePin = true;
constexpr PinConf FreezePin{GPIO::C, PinNum::_7}; // last DEBUG pin

constexpr uint32_t ConsoleUART = UART7_BASE;
constexpr PinConf UartRX{GPIO::B, PinNum::_3, PinAF::AF_13};
constexpr PinConf UartTX{GPIO::H, PinNum::_2, PinAF::AF_8};

namespace NORFlash
{
constexpr bool HasNORFlash = true;
constexpr PinConf d2{GPIO::F, PinNum::_6, PinAF::AF_9};
constexpr PinConf d3{GPIO::H, PinNum::_8, PinAF::AF_13};
} // namespace NORFlash

namespace PMIC
{
constexpr bool HasSTPMIC = false;
constexpr I2C_Config I2C_config{};
} // namespace PMIC

constexpr uint32_t HSE_Clock_Hz = 24'000'000;
constexpr uint32_t MPU_MHz = 1000;
constexpr auto ClockType = SystemClocks::HSEClockSource::Resonator;

} // namespace Board
