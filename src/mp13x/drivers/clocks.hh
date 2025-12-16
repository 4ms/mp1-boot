#pragma once
#include "drivers/leds.hh"

#include "drivers/rcc.hh"
#include "stm32mp13xx_hal_rcc.h"

struct SystemClocks {
	enum class HSEClockSource { AnalogOsc, DigitalOsc, Resonator };

	static unsigned init_core_clocks(uint32_t HSE_Clock = 24000000,
									 uint32_t MPU_MHz = 1000,
									 HSEClockSource hse_clk_src = HSEClockSource::AnalogOsc)
	{

		HAL_Init();

		Led<GPIO::B, PinNum::_3, LedActive::High> d;
		Led<GPIO::B, PinNum::_4, LedActive::High> e;
		e.on();
		d.off();
		d.on();
		d.off();
		e.off();

		constexpr uint32_t pll1m = 2;
		const uint32_t pll1n = 54; //(MPU_MHz == 650 ? 81U : 100U);
		constexpr uint32_t pll1p = 1;
		constexpr uint32_t pll1q = 2;
		constexpr uint32_t pll1r = 2;
		constexpr uint32_t pll1frac = 0;

		constexpr uint32_t pll2m = 2;
		constexpr uint32_t pll2n = 44;
		constexpr uint32_t pll2p = 2;
		constexpr uint32_t pll2q = 2;
		constexpr uint32_t pll2r = 1;
		constexpr uint32_t pll2frac = 0;

		using namespace mdrivlib;
		using namespace mdrivlib::RCC_Clocks;
		MPUClockSrc::write(RCC_Clocks::MPUClockSrcHSI);
		AXISClockSrc::write(RCC_Clocks::AXISClockSrcHSI);

		// HSE
		{
			// Turn HSE off
			OscEnableHSEON::clear();
			while (HSEClockReady::read()) {
				d.on();
				d.off();
			}

			// Select the source
			switch (hse_clk_src) {
				case HSEClockSource::AnalogOsc:
					OscEnableDIGBYP::clear();
					OscEnableHSEBYP::set();
					break;

				case HSEClockSource::DigitalOsc:
					OscEnableDIGBYP::set();
					OscEnableHSEBYP::set();
					break;

				case HSEClockSource::Resonator:
					OscEnableDIGBYP::clear(); // doesn't matter
					OscEnableHSEBYP::clear();
					break;
			}

			OscEnableHSEON::set();
			while (!HSEClockReady::read()) {
				d.on();
				d.on();
				d.off();
			}
		}

		e.on();
		e.off();

		// PLL1
		{
			PLL1::DIVPEnable::clear();
			PLL1::DIVQEnable::clear();
			PLL1::DIVREnable::clear();
			PLL1::Enable::clear();
			while (PLL1::Ready::read())
				;

			PLL12Source::write(PLL12SourceHSE);
			while (!PLL12SourceReady::read())
				;
			PLL1::DIVM1::write(pll1m - 1);
			PLL1::DIVN::write(pll1n - 1);
			PLL1::DIVP::write(pll1p - 1);
			PLL1::DIVQ::write(pll1q - 1);
			PLL1::DIVR::write(pll1r - 1);
			PLL1::FRACLatch::clear();
			PLL1::FRACValue::write(0);
			PLL1::FRACLatch::set();
			PLL1::SpreadSpectrumClockGen::Enable::clear();

			PLL1::Enable::set();
			while (!PLL1::Ready::read())
				;
			PLL1::DIVPEnable::set();
			PLL1::DIVQEnable::set();
			PLL1::DIVREnable::set();
		}

		e.on();
		e.off();
		e.on();
		e.off();

		// PLL2
		{
			PLL2::DIVPEnable::clear();
			PLL2::DIVQEnable::clear();
			PLL2::DIVREnable::clear();
			PLL2::Enable::clear();
			while (PLL2::Ready::read())
				;

			PLL2::DIVM2::write(pll2m - 1);
			PLL2::DIVN::write(pll2n - 1);
			PLL2::DIVP::write(pll2p - 1);
			PLL2::DIVQ::write(pll2q - 1);
			PLL2::DIVR::write(pll2r - 1);
			PLL2::FRACLatch::clear();
			PLL2::FRACValue::write(pll2frac);
			PLL2::FRACLatch::set();
			PLL2::SpreadSpectrumClockGen::Enable::clear();

			// Enable it
			PLL2::Enable::set();
			while (!PLL2::Ready::read())
				;
			PLL2::DIVPEnable::set();
			PLL2::DIVQEnable::set();
			PLL2::DIVREnable::set();
		}

		e.on();
		e.on();
		e.off();
		e.off();
		e.on();
		e.off();
		e.on();
		e.on();
		e.off();
		e.off();

		MPUClockSrc::write(MPUClockSrcPLL1);
		while (!MPUClockSrcReady::read())
			;

		AXISClockSrc::write(AXISClockSrcPLL2);
		while (!AXISClockSrcReady::read())
			;

		AXIDiv::write(AXIDivOff);
		while (!AXIDivReady::read())
			;

		e.on();
		e.off();
		e.on();
		e.off();

		float coreclock = (float)HSE_Clock * ((float)pll1n + (float)pll1frac / 8191.f) / ((float)pll1m * (float)pll1p);
		SystemCoreClock = static_cast<uint32_t>(coreclock);
		return SystemCoreClock;

		// HAL_RCC_DeInit();

		// RCC_ClkInitTypeDef RCC_ClkInitStructure;
		// RCC_OscInitTypeDef RCC_OscInitStructure;

		// /* Enable all available oscillators*/
		// RCC_OscInitStructure.OscillatorType =
		// 	(RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_CSI | RCC_OSCILLATORTYPE_LSI |
		// 	 RCC_OSCILLATORTYPE_LSE);

		// RCC_OscInitStructure.HSIState = RCC_HSI_ON;
		// RCC_OscInitStructure.HSEState = RCC_HSE_ON;
		// RCC_OscInitStructure.LSEState = RCC_LSE_ON;
		// RCC_OscInitStructure.LSIState = RCC_LSI_ON;
		// RCC_OscInitStructure.CSIState = RCC_CSI_ON;

		// RCC_OscInitStructure.HSICalibrationValue = 0x00; // Default reset value
		// RCC_OscInitStructure.CSICalibrationValue = 0x10; // Default reset value
		// RCC_OscInitStructure.HSIDivValue = RCC_HSI_DIV1; // Default value

		// /* PLL configuration */
		// RCC_OscInitStructure.PLL.PLLState = RCC_PLL_ON;
		// RCC_OscInitStructure.PLL.PLLSource = RCC_PLL12SOURCE_HSE;
		// RCC_OscInitStructure.PLL.PLLM = 3;
		// RCC_OscInitStructure.PLL.PLLN = 81;
		// RCC_OscInitStructure.PLL.PLLP = 1;
		// RCC_OscInitStructure.PLL.PLLQ = 2;
		// RCC_OscInitStructure.PLL.PLLR = 2;
		// RCC_OscInitStructure.PLL.PLLFRACV = 0x800;
		// RCC_OscInitStructure.PLL.PLLMODE = RCC_PLL_FRACTIONAL;

		// RCC_OscInitStructure.PLL2.PLLState = RCC_PLL_ON;
		// RCC_OscInitStructure.PLL2.PLLSource = RCC_PLL12SOURCE_HSE;
		// RCC_OscInitStructure.PLL2.PLLM = 3;
		// RCC_OscInitStructure.PLL2.PLLN = 66;
		// RCC_OscInitStructure.PLL2.PLLP = 2;
		// RCC_OscInitStructure.PLL2.PLLQ = 2;
		// RCC_OscInitStructure.PLL2.PLLR = 1;
		// RCC_OscInitStructure.PLL2.PLLFRACV = 0x1400;
		// RCC_OscInitStructure.PLL2.PLLMODE = RCC_PLL_FRACTIONAL;

		// RCC_OscInitStructure.PLL3.PLLState = RCC_PLL_ON;
		// RCC_OscInitStructure.PLL3.PLLSource = RCC_PLL3SOURCE_HSE;
		// RCC_OscInitStructure.PLL3.PLLM = 2;
		// RCC_OscInitStructure.PLL3.PLLN = 34;
		// RCC_OscInitStructure.PLL3.PLLP = 2;
		// RCC_OscInitStructure.PLL3.PLLQ = 17;
		// RCC_OscInitStructure.PLL3.PLLR = 2;
		// RCC_OscInitStructure.PLL3.PLLRGE = RCC_PLL3IFRANGE_1;
		// RCC_OscInitStructure.PLL3.PLLFRACV = 0x1a04;
		// RCC_OscInitStructure.PLL3.PLLMODE = RCC_PLL_FRACTIONAL;

		// RCC_OscInitStructure.PLL4.PLLState = RCC_PLL_ON;
		// RCC_OscInitStructure.PLL4.PLLSource = RCC_PLL4SOURCE_HSE;
		// RCC_OscInitStructure.PLL4.PLLM = 2;
		// RCC_OscInitStructure.PLL4.PLLN = 50;
		// RCC_OscInitStructure.PLL4.PLLP = 12;
		// RCC_OscInitStructure.PLL4.PLLQ = 60;
		// RCC_OscInitStructure.PLL4.PLLR = 6;
		// RCC_OscInitStructure.PLL4.PLLRGE = RCC_PLL4IFRANGE_1;
		// RCC_OscInitStructure.PLL4.PLLFRACV = 0;
		// RCC_OscInitStructure.PLL4.PLLMODE = RCC_PLL_INTEGER;

		// /* Enable access to RTC and backup registers */
		// SET_BIT(PWR->CR1, PWR_CR1_DBP);
		// /* Configure LSEDRIVE value */
		// __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);

		// if (HAL_RCC_OscConfig(&RCC_OscInitStructure) != HAL_OK) {
		// 	return 0;
		// }

		// /* Select PLLx as MPU, AXI and MCU clock sources */
		// RCC_ClkInitStructure.ClockType =
		// 	(RCC_CLOCKTYPE_MPU | RCC_CLOCKTYPE_ACLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK4 | RCC_CLOCKTYPE_PCLK5 |
		// 	 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK6 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3);

		// RCC_ClkInitStructure.MPUInit.MPU_Clock = RCC_MPUSOURCE_PLL1;
		// RCC_ClkInitStructure.MPUInit.MPU_Div = RCC_MPU_DIV2;
		// RCC_ClkInitStructure.AXISSInit.AXI_Clock = RCC_AXISSOURCE_PLL2;
		// RCC_ClkInitStructure.AXISSInit.AXI_Div = RCC_AXI_DIV1;
		// RCC_ClkInitStructure.MLAHBInit.MLAHB_Clock = RCC_MLAHBSSOURCE_PLL3;
		// RCC_ClkInitStructure.MLAHBInit.MLAHB_Div = RCC_MLAHB_DIV1;
		// RCC_ClkInitStructure.APB1_Div = RCC_APB1_DIV2;
		// RCC_ClkInitStructure.APB2_Div = RCC_APB2_DIV2;
		// RCC_ClkInitStructure.APB3_Div = RCC_APB3_DIV2;
		// RCC_ClkInitStructure.APB4_Div = RCC_APB4_DIV2;
		// RCC_ClkInitStructure.APB5_Div = RCC_APB5_DIV4;
		// RCC_ClkInitStructure.APB6_Div = RCC_APB6_DIV2;

		// d.on();
		// if (HAL_RCC_ClockConfig(&RCC_ClkInitStructure) != HAL_OK) {
		// 	return 0;
		// }
		// d.off();

		// d.on();
		// auto sp = HAL_RCC_GetSystemCoreClockFreq();
		// d.off();

		// return sp;
	}
};
