#pragma once
#include "board_conf.hh"
#include "boot_image_def.hh"
#include "boot_loader.hh"
#include "print_messages.hh"
#include <cstdint>
#include <cstring>

struct BootDdrLoader : BootLoader {
	BootDdrLoader() = default;

	BootImageDef::image_header read_image_header(uint32_t header_addr) override
	{
		BootImageDef::image_header header;
		memcpy(&header, (const void *)header_addr, sizeof(header));
		return header;
	}

	bool load_image(uint32_t image_addr, uint32_t load_addr, uint32_t size) override
	{
		memcpy((void *)load_addr, (void const *)image_addr, size);
		return true;
	}

	uint32_t get_first_header_addr(LoadTarget target) override
	{
		unlock_backup_registers();

		// Should be 0x5C00A118
		volatile uint32_t *HeaderAddress = &(TAMP->BKP6R);
		static_assert(TAMP_BASE == 0x5C00A000);
		static_assert(offsetof(TAMP_TypeDef, BKP6R) == 0x118);

		*HeaderAddress = 0xFFFFFFFF;
#ifdef LOAD_BINARY
		print("Please load executable binary into an address in SRAM.\n");
		print("Then write that address to the TAMP_BKP6 register at 0x", Hex{(uint32_t)HeaderAddress});
		print("\nSystem will wait until TAMP_BKP6 register is changed, and then jump execution to the value.\n");
		while (*HeaderAddress == 0xFFFFFFFF)
			;

		print("Jumping to entry point: 0x", Hex{*HeaderAddress}, "\n");

		typedef void __attribute__((noreturn)) (*image_entry_noargs_t)();
		auto image_entry = reinterpret_cast<image_entry_noargs_t>(*HeaderAddress);

		__set_BPIALL(0);
		__DSB();
		__ISB();
		__set_ICIALLU(0);
		__DSB();
		__ISB();

		image_entry();

#else
		print("Please load multi-uimg binary into an address in DDR RAM.\n");
		print("Then write the address to the TAMP_BKP6 register at 0x", Hex{(uint32_t)HeaderAddress});
		print("\nSystem will hang until TAMP_BKP6 register is changed...");
		while (*HeaderAddress == 0xFFFFFFFF)
			;
#endif

		return *HeaderAddress;
	}

private:
	static void unlock_backup_registers()
	{
		// Turn on the Disable Backup Protection bit, to allow us to write to the TAMP backup registers
		PWR->CR1 = PWR->CR1 | PWR_CR1_DBP;
		while (!(PWR->CR1 & PWR_CR1_DBP))
			;

#ifdef STM32MP13
#else
		// Turn off Write protection on backup registers (BOOTROM seems to turn it on during MPU1 boot-up)
		TAMP->SMCR = (0 << TAMP_SMCR_BKPRWDPROT_Pos) | (0 << TAMP_SMCR_BKPWDPROT_Pos) | (1 << TAMP_SMCR_TAMPDPROT_Pos);
#endif
	}
};
