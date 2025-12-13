#include "drivers/clocks.hh"
#include "drivers/i2c_conf.hh"
#include "drivers/leds.hh"

namespace Board
{
using OrangeLED = Led<GPIO::H, PinNum::_7, LedActive::High>;

using BlueLED = Led<GPIO::D, PinNum::_11, LedActive::High>;

using GreenLED = Led<GPIO::A, PinNum::_14, LedActive::High>;

using RedLED = Led<GPIO::A, PinNum::_13, LedActive::High>;

using RedLED2 = OrangeLED; // For compatibility with OSD32BRK board
using GreenLED2 = BlueLED; // For compatibility with OSD32BRK board

// Button USER2 (also activates RedLED)
constexpr bool UseBootSelect = false;
constexpr PinConf BootSelectPin{GPIO::A, PinNum::_13};

// Freeze mode: halts booting after initializing everything
// so you can load firmware via SWD/JTAG
constexpr bool UseFreezePin = false;
constexpr PinConf FreezePin{GPIO::Unused, PinNum::_0};

constexpr uint32_t ConsoleUART = UART4_BASE;
constexpr PinConf UartRX{GPIO::B, PinNum::_2, PinAF::AF_8};
constexpr PinConf UartTX{GPIO::G, PinNum::_11, PinAF::AF_6};

namespace NORFlash
{
constexpr bool HasNORFlash = false;
constexpr PinConf d2{};
constexpr PinConf d3{};
} // namespace NORFlash

namespace PMIC
{
constexpr bool HasSTPMIC = false;
constexpr I2C_Config I2C_config{};
} // namespace PMIC

constexpr uint32_t HSE_Clock_Hz = 24000000;
constexpr uint32_t MPU_MHz = 650;
constexpr auto ClockType = SystemClocks::HSEClockSource::AnalogOsc;
} // namespace Board
