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

namespace SDMMC
{
constexpr int DataWidth = 4;
}

namespace NORFlash
{
constexpr bool HasNORFlash = true;
constexpr PinConf clk{GPIO::F, PinNum::_10, PinAF::AF_9};
constexpr PinConf cs{GPIO::B, PinNum::_2, PinAF::AF_9};
constexpr PinConf d0{GPIO::F, PinNum::_8, PinAF::AF_10};
constexpr PinConf d1{GPIO::F, PinNum::_9, PinAF::AF_10};
constexpr PinConf d2{GPIO::F, PinNum::_6, PinAF::AF_9};
constexpr PinConf d3{GPIO::H, PinNum::_7, PinAF::AF_13};

constexpr uint32_t SizeBytes = 16 * 1024 * 1024;

enum AddressBits { AddrBits24 = 24, AddrBits32 = 32 };
constexpr inline AddressBits address_bits = AddrBits24;

enum ChipID { IS25L, S25FLxxxL, W25Q128JV };
constexpr inline ChipID chip_id = W25Q128JV;

enum IOMode { SingleSPI, DualSPI, QuadSPI };
constexpr inline IOMode io_mode = QuadSPI;

enum Bank { Bank1, Bank2 };
constexpr inline Bank bank = Bank1;

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
