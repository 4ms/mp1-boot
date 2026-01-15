#include "drivers/stm32mpxxx.h"

uint32_t SystemCoreClock = 24000000;

void SystemInit(void)
{
	// do not use global variables because this function is called before
	// reaching pre-main. RW section may be overwritten afterwards.

	// Invalidate entire Unified TLB
	__set_TLBIALL(0);

	EXTI_C1->IMR1 = 0;
	EXTI_C1->IMR2 = 0;
	EXTI_C1->IMR3 = 0;
	EXTI_C1->EMR1 = 0;
	EXTI_C1->EMR2 = 0;
	EXTI_C1->EMR3 = 0;

	// Invalidate entire branch predictor array
	__set_BPIALL(0);
	__DSB();
	__ISB();

	//  Invalidate instruction cache and flush branch target cache
	__set_ICIALLU(0);
	__DSB();
	__ISB();

	L1C_InvalidateDCacheAll();

	__FPU_Enable();

	L1C_EnableCaches();
	L1C_EnableBTAC();

#if (__L2C_PRESENT == 1)
	L2C_Enable();
#endif
}

void security_init()
{
	// Enable MCE
	__HAL_RCC_MCE_CLK_ENABLE();

	// TZC configuration
	__HAL_RCC_TZC_CLK_ENABLE();

	// Configure TZC to allow DDR region0 r/w non secure for all IDs
	TZC->GATE_KEEPER = 0;
	TZC->REG_ID_ACCESSO = 0xFFFFFFFF; // Allow DDR Region0 R/W  non secure for all IDs
	TZC->REG_ATTRIBUTESO = 0xC0000001;
	TZC->GATE_KEEPER |= 1; // Enable the access in secure Mode  // filter 0 request close

	// Enable ETZPC & BACKUP SRAM for security
	__HAL_RCC_ETZPC_CLK_ENABLE();
	//   LL_ETZPC_Set_All_PeriphProtection(ETZPC, LL_ETZPC_PERIPH_PROTECTION_READ_WRITE_NONSECURE);
	__HAL_RCC_BKPSRAM_CLK_ENABLE();

	// Unlock debugger
	BSEC->BSEC_DENABLE = 0x47f;
}
