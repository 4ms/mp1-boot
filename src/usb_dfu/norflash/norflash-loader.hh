#pragma once
#include "qspi_flash_driver.hh"
#include "print.hh"
#include "stm32mp1xx.h"

struct NorFlashLoader {
	using QSpiFlash = mdrivlib::QSpiFlash;
	QSpiFlash flash;

	NorFlashLoader(const mdrivlib::QSPIFlashConfig &conf)
		: flash{conf}
	{
		print("QSPI is initialized.\n\r");
	}
};
