#include "drivers/stm32mpxxx.h"

#if defined DDR_TYPE_DDR3_4Gb
#include "stm32mp13xx-ddr3-4Gb.h"
#elif defined DDR_TYPE_DDR3_8Gb
#include "stm32mp13xx-ddr3-8Gb.h"
#elif defined DDR_TYPE_LPDDR2_4Gb
#include "stm32mp13xx-lpddr2-4Gb.h"
#elif defined DDR_TYPE_LPDDR3_4Gb
#include "stm32mp13xx-lpddr3-4Gb.h"
#else /* DDR_TYPE_DDR3_4Gb */
#endif

#include "print.hh"

void stm32mp1_ddr_setup()
{
	DDR_InitTypeDef hddr;
	hddr.wakeup_from_standby = false;
	hddr.self_refresh = false;
	hddr.zdata = 0;
	hddr.clear_bkp = false;

	if (auto res = HAL_DDR_Init(&hddr) != HAL_OK) {
		print("DDR Init error: ", Hex{res}, "\n");
	}
}

uint32_t stm32mp1_ddr_get_size() { return DDR_MEM_SIZE; }
