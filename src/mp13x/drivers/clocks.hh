#pragma once
#include "drivers/rcc.hh"
#include "stm32mp13xx_hal_rcc.h"

struct SystemClocks {
	enum class HSEClockSource { AnalogOsc, DigitalOsc, Resonator };

	static unsigned init_core_clocks(uint32_t HSE_Clock = 24000000,
									 uint32_t MPU_MHz = 1000,
									 HSEClockSource hse_clk_src = HSEClockSource::AnalogOsc)
	{

		HAL_RCC_DeInit();
		RCC_ClkInitTypeDef RCC_ClkInitStructure;
		RCC_OscInitTypeDef RCC_OscInitStructure;

		/* Enable all available oscillators*/
		RCC_OscInitStructure.OscillatorType =
			(RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_CSI | RCC_OSCILLATORTYPE_LSI |
			 RCC_OSCILLATORTYPE_LSE);

		RCC_OscInitStructure.HSIState = RCC_HSI_ON;
		RCC_OscInitStructure.HSEState = RCC_HSE_ON;
		RCC_OscInitStructure.LSEState = RCC_LSE_ON;
		RCC_OscInitStructure.LSIState = RCC_LSI_ON;
		RCC_OscInitStructure.CSIState = RCC_CSI_ON;

		RCC_OscInitStructure.HSICalibrationValue = 0x00; // Default reset value
		RCC_OscInitStructure.CSICalibrationValue = 0x10; // Default reset value
		RCC_OscInitStructure.HSIDivValue = RCC_HSI_DIV1; // Default value

		/* PLL configuration */
		RCC_OscInitStructure.PLL.PLLState = RCC_PLL_ON;
		RCC_OscInitStructure.PLL.PLLSource = RCC_PLL12SOURCE_HSE;
		RCC_OscInitStructure.PLL.PLLM = 3;
		RCC_OscInitStructure.PLL.PLLN = 81;
		RCC_OscInitStructure.PLL.PLLP = 1;
		RCC_OscInitStructure.PLL.PLLQ = 2;
		RCC_OscInitStructure.PLL.PLLR = 2;
		RCC_OscInitStructure.PLL.PLLFRACV = 0x800;
		RCC_OscInitStructure.PLL.PLLMODE = RCC_PLL_FRACTIONAL;

		RCC_OscInitStructure.PLL2.PLLState = RCC_PLL_ON;
		RCC_OscInitStructure.PLL2.PLLSource = RCC_PLL12SOURCE_HSE;
		RCC_OscInitStructure.PLL2.PLLM = 3;
		RCC_OscInitStructure.PLL2.PLLN = 66;
		RCC_OscInitStructure.PLL2.PLLP = 2;
		RCC_OscInitStructure.PLL2.PLLQ = 2;
		RCC_OscInitStructure.PLL2.PLLR = 1;
		RCC_OscInitStructure.PLL2.PLLFRACV = 0x1400;
		RCC_OscInitStructure.PLL2.PLLMODE = RCC_PLL_FRACTIONAL;

		RCC_OscInitStructure.PLL3.PLLState = RCC_PLL_ON;
		RCC_OscInitStructure.PLL3.PLLSource = RCC_PLL3SOURCE_HSE;
		RCC_OscInitStructure.PLL3.PLLM = 2;
		RCC_OscInitStructure.PLL3.PLLN = 34;
		RCC_OscInitStructure.PLL3.PLLP = 2;
		RCC_OscInitStructure.PLL3.PLLQ = 17;
		RCC_OscInitStructure.PLL3.PLLR = 2;
		RCC_OscInitStructure.PLL3.PLLRGE = RCC_PLL3IFRANGE_1;
		RCC_OscInitStructure.PLL3.PLLFRACV = 0x1a04;
		RCC_OscInitStructure.PLL3.PLLMODE = RCC_PLL_FRACTIONAL;

		RCC_OscInitStructure.PLL4.PLLState = RCC_PLL_ON;
		RCC_OscInitStructure.PLL4.PLLSource = RCC_PLL4SOURCE_HSE;
		RCC_OscInitStructure.PLL4.PLLM = 2;
		RCC_OscInitStructure.PLL4.PLLN = 50;
		RCC_OscInitStructure.PLL4.PLLP = 12;
		RCC_OscInitStructure.PLL4.PLLQ = 60;
		RCC_OscInitStructure.PLL4.PLLR = 6;
		RCC_OscInitStructure.PLL4.PLLRGE = RCC_PLL4IFRANGE_1;
		RCC_OscInitStructure.PLL4.PLLFRACV = 0;
		RCC_OscInitStructure.PLL4.PLLMODE = RCC_PLL_INTEGER;

		/* Enable access to RTC and backup registers */
		SET_BIT(PWR->CR1, PWR_CR1_DBP);
		/* Configure LSEDRIVE value */
		__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);

		if (HAL_RCC_OscConfig(&RCC_OscInitStructure) != HAL_OK) {
			return 0;
		}

		/* Select PLLx as MPU, AXI and MCU clock sources */
		RCC_ClkInitStructure.ClockType =
			(RCC_CLOCKTYPE_MPU | RCC_CLOCKTYPE_ACLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK4 | RCC_CLOCKTYPE_PCLK5 |
			 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK6 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3);

		RCC_ClkInitStructure.MPUInit.MPU_Clock = RCC_MPUSOURCE_PLL1;
		RCC_ClkInitStructure.MPUInit.MPU_Div = RCC_MPU_DIV2;
		RCC_ClkInitStructure.AXISSInit.AXI_Clock = RCC_AXISSOURCE_PLL2;
		RCC_ClkInitStructure.AXISSInit.AXI_Div = RCC_AXI_DIV1;
		RCC_ClkInitStructure.MLAHBInit.MLAHB_Clock = RCC_MLAHBSSOURCE_PLL3;
		RCC_ClkInitStructure.MLAHBInit.MLAHB_Div = RCC_MLAHB_DIV1;
		RCC_ClkInitStructure.APB1_Div = RCC_APB1_DIV2;
		RCC_ClkInitStructure.APB2_Div = RCC_APB2_DIV2;
		RCC_ClkInitStructure.APB3_Div = RCC_APB3_DIV2;
		RCC_ClkInitStructure.APB4_Div = RCC_APB4_DIV2;
		RCC_ClkInitStructure.APB5_Div = RCC_APB5_DIV4;
		RCC_ClkInitStructure.APB6_Div = RCC_APB6_DIV2;

		if (HAL_RCC_ClockConfig(&RCC_ClkInitStructure) != HAL_OK) {
			return 0;
		}

		return HAL_RCC_GetSystemCoreClockFreq();
	}
};
