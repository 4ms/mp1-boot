#pragma once
#include "drivers/clocks.hh"
#include "drivers/i2c_conf.hh"
#include "drivers/leds.hh"

namespace Board
{
using RedLED = Led<GPIO::D, PinNum::_8, LedActive::Low>;
using GreenLED = Led<GPIO::B, PinNum::_3, LedActive::High>; // Debug2
using RedLED2 = RedLED;
using GreenLED2 = GreenLED;

using OrangeLED = RedLED2;
using BlueLED = GreenLED;

constexpr PinConf BootSelectPin{GPIO::E, PinNum::_2}; // DFU boot button
constexpr bool UseBootSelect = true;

constexpr bool UseFreezePin = true;
constexpr PinConf FreezePin{GPIO::B, PinNum::_4}; // DEBUG3 pin

constexpr uint32_t ConsoleUART = UART4_BASE;
constexpr PinConf UartRX{GPIO::A, PinNum::_15, PinAF::AF_8};
constexpr PinConf UartTX{GPIO::A, PinNum::_9, PinAF::AF_8};

namespace NORFlash
{
constexpr bool HasNORFlash = true;
constexpr PinConf d2{GPIO::F, PinNum::_6, PinAF::AF_9};
constexpr PinConf d3{GPIO::H, PinNum::_7, PinAF::AF_13};
} // namespace NORFlash

namespace PMIC
{
constexpr bool HasSTPMIC = false;
constexpr I2C_Config I2C_config{};
} // namespace PMIC

constexpr uint32_t HSE_Clock_Hz = 24000000;
constexpr uint32_t MPU_MHz = 1000;
constexpr auto ClockType = SystemClocks::HSEClockSource::Resonator;

} // namespace Board
