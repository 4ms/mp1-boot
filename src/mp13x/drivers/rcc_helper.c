#include "drivers/stm32mpxxx.h"
#include <stdint.h>

// These functions are defined in hal_rcc.c, we don't compile that file to save space (~9kB).

// Used to check if config value for DDR clock (533MHz) is within 10% of actual PLL settings
// Not fully functional! Only returns PLL2_R_Frequency! Only valid if PLL2 was setup as done in clocks.hh.
void HAL_RCC_GetPLL2ClockFreq(PLL2_ClocksTypeDef *PLL2_Clocks)
{
	uint32_t pll2m;
	uint32_t pll2fracen;
	uint32_t fracn1us;
	uint32_t R_Div;
	uint32_t pll2vco_1;
	float32_t fracn1;
	float32_t pll2vco;
	float32_t R_Freq;

	pll2m = ((RCC->PLL2CFGR1 & RCC_PLL2CFGR1_DIVM2) >> RCC_PLL2CFGR1_DIVM2_Pos) + 1U;
	pll2fracen = (RCC->PLL2FRACR & RCC_PLL2FRACR_FRACLE) >> RCC_PLL2FRACR_FRACLE_Pos;

	fracn1us = (pll2fracen * ((RCC->PLL2FRACR & RCC_PLL2FRACR_FRACV) >> RCC_PLL2FRACR_FRACV_Pos));
	fracn1 = (float32_t)fracn1us;

	pll2vco_1 = (RCC->PLL2CFGR1 & RCC_PLL2CFGR1_DIVN) + 1U;
	pll2vco = (float32_t)pll2vco_1 + (fracn1 / (float32_t)0x2000);
	pll2vco *= ((float32_t)HSE_VALUE / (float32_t)pll2m);

	R_Div = ((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVR) >> RCC_PLL2CFGR2_DIVR_Pos) + 1U;
	R_Freq = pll2vco / (float32_t)R_Div;
	PLL2_Clocks->PLL2_R_Frequency = (uint32_t)R_Freq;
}

// Not fully functional! Only valid if PLL1 was setup as done in clocks.hh.
uint32_t HAL_RCC_GetSystemCoreClockFreq(void)
{
	if (__HAL_RCC_GET_MPU_SOURCE() == RCC_MPUSOURCE_PLL1) {
		uint32_t pll1m;
		uint32_t pll1fracen;
		uint32_t fracn1us;
		uint32_t P_Div;
		uint32_t pll1vco_1;
		float32_t fracn1;
		float32_t pll1vco;
		float32_t P_Freq;

		pll1m = ((RCC->PLL1CFGR1 & RCC_PLL1CFGR1_DIVM1) >> RCC_PLL1CFGR1_DIVM1_Pos) + 1U;
		pll1fracen = (RCC->PLL1FRACR & RCC_PLL1FRACR_FRACLE) >> RCC_PLL1FRACR_FRACLE_Pos;
		fracn1us = (pll1fracen * ((RCC->PLL1FRACR & RCC_PLL1FRACR_FRACV) >> RCC_PLL1FRACR_FRACV_Pos));
		fracn1 = (float32_t)fracn1us;
		pll1vco_1 = (RCC->PLL1CFGR1 & RCC_PLL1CFGR1_DIVN) + 1U;
		pll1vco = (float32_t)pll1vco_1 + (fracn1 / (float32_t)0x2000); /* Intermediary value */
		pll1vco *= ((float32_t)HSE_VALUE / (float32_t)pll1m);
		P_Div = ((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVP) >> RCC_PLL1CFGR2_DIVP_Pos) + 1U;
		P_Freq = pll1vco / (float32_t)P_Div;
		return P_Freq;
	}

	return 0;
}

// Not fully functional! Only returns SDMMC1 clock speed if it's connected to HSI (with or without a divider)
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint64_t PeriphClk)
{
	if (PeriphClk == RCC_PERIPHCLK_SDMMC1)
		return HSI_VALUE >> __HAL_RCC_GET_HSI_DIV();
	else
		return 0;
}
