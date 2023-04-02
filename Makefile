BINARYNAME = fsbl

OPTFLAG = -O3

SRCDIR = src
EXTLIBDIR = third-party
LINKSCR = linkscript.ld
PATCHEDSTM32HALDIR = src/stm32hal_patched

SD_DISK_DEV ?= /dev/disk4

SOURCES = $(SRCDIR)/startup.s \
		  $(SRCDIR)/main.cc \
		  $(SRCDIR)/systeminit.c \
		  $(SRCDIR)/libc_stub.c \
		  $(SRCDIR)/libcpp_stub.cc \
		  $(SRCDIR)/print.cc \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_ll_usart.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_ll_rcc.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_rcc.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_ll_sdmmc.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_sd.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_qspi.c \
		  $(SRCDIR)/drivers/ddr/stm32mp1_ddr.cc \
		  $(SRCDIR)/drivers/ddr/stm32mp1_ram.cc \
		  $(SRCDIR)/drivers/ddr/ram_tests.cc \
		  $(SRCDIR)/uboot-port/common/memsize.c \
		  $(SRCDIR)/uboot-port/lib/crc32.c \
		  $(SRCDIR)/drivers/norflash/qspi_ll.c \
		  $(SRCDIR)/drivers/norflash/qspi_norflash_read.c \
		  $(SRCDIR)/gpt/gpt.cc \
		  $(SRCDIR)/usb_dfu/usbd_conf.c \
		  $(SRCDIR)/usb_dfu/usbd_desc.c \
		  $(SRCDIR)/usb_dfu/usbd_dfu_media.c \
		  $(SRCDIR)/usb_dfu/norflash/qspi_flash_driver.cc \
		  $(EXTLIBDIR)/STM32_USB_Device_Library/Core/Src/usbd_core.c \
		  $(EXTLIBDIR)/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
		  $(EXTLIBDIR)/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
		  $(EXTLIBDIR)/STM32_USB_Device_Library/Class/DFU/Src/usbd_dfu.c \
		  $(PATCHEDSTM32HALDIR)/stm32mp1xx_hal.c \
		  $(PATCHEDSTM32HALDIR)/stm32mp1xx_hal_pcd.c \
		  $(PATCHEDSTM32HALDIR)/stm32mp1xx_hal_pcd_ex.c \
		  $(PATCHEDSTM32HALDIR)/stm32mp1xx_ll_usb.c \
		  $(PATCHEDSTM32HALDIR)/stm32mp1xx_ll_usb_phy.c \

		  # $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal.c \

INCLUDES = -I. \
		   -I$(SRCDIR) \
		   -I$(SRCDIR)/board_conf \
		   -I$(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Inc \
		   -I$(EXTLIBDIR)/CMSIS/Core_A/Include \
		   -I$(EXTLIBDIR)/CMSIS/Device/ST/STM32MP1xx/Include \
		   -I$(EXTLIBDIR)/STM32_USB_Device_Library/Class/DFU/Inc \
		   -I$(EXTLIBDIR)/STM32_USB_Device_Library/Core/Inc \
		   -I$(SRCDIR)/drivers/ddr/ \
		   -I$(SRCDIR)/uboot-port/include \
		   -I$(SRCDIR)/uboot-port/arch/arm/include \
		   -I$(SRCDIR)/usb_dfu \
		   -I$(PATCHEDSTM32HALDIR) \

MCU = -mcpu=cortex-a7 -march=armv7ve -mfpu=neon-vfpv4 -mlittle-endian -mfloat-abi=hard
ARCH_CFLAGS = -DUSE_FULL_LL_DRIVER \
			  -DUSE_HAL_DRIVER \
			  -DSTM32MP157Cxx \
			  -DSTM32MP1 \
			  -DCORE_CA7 \
			  $(EXTRA_ARCH_CFLAGS) \

AFLAGS = $(MCU)

CFLAGS = -g2 \
		 -fno-common \
		 $(ARCH_CFLAGS) \
		 $(MCU) \
		 $(INCLUDES) \
		 -fdata-sections -ffunction-sections \
		 -nostartfiles \
		 -ffreestanding \
		 $(EXTRACFLAGS)\

CXXFLAGS = $(CFLAGS) \
		-std=c++2a \
		-fno-rtti \
		-fno-exceptions \
		-fno-unwind-tables \
		-ffreestanding \
		-fno-threadsafe-statics \
		-mno-unaligned-access \
		-Werror=return-type \
		-Wdouble-promotion \
		-Wno-register \
		-Wno-volatile \
		 $(EXTRACXXFLAGS) \

LFLAGS = -Wl,--gc-sections \
		 -Wl,-Map,$(BUILDDIR)/$(BINARYNAME).map,--cref \
		 $(MCU)  \
		 -T $(LINKSCR) \
		 -nostdlib \
		 -nostartfiles \
		 -ffreestanding \
		 $(EXTRALDFLAGS) \

DEPFLAGS = -MMD -MP -MF $(OBJDIR)/$(basename $<).d

ARCH 	= arm-none-eabi
CC 		= $(ARCH)-gcc
CXX 	= $(ARCH)-g++
LD 		= $(ARCH)-g++
AS 		= $(ARCH)-as
OBJCPY 	= $(ARCH)-objcopy
OBJDMP 	= $(ARCH)-objdump
GDB 	= $(ARCH)-gdb
SZ 		= $(ARCH)-size

SZOPTS 	= -d

ELF 	= $(BUILDDIR)/$(BINARYNAME).elf
HEX 	= $(BUILDDIR)/$(BINARYNAME).hex
BIN 	= $(BUILDDIR)/$(BINARYNAME).bin


OBJECTS   = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(SOURCES))))
DEPS   	  = $(addprefix $(OBJDIR)/, $(addsuffix .d, $(basename $(SOURCES))))
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj/obj

all: image
all: Makefile $(ELF) $(UIMAGENAME)


$(OBJDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(info Building $< at $(OPTFLAG))
	@$(AS) $(AFLAGS) $< -o $@ 

$(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	@mkdir -p $(dir $@)
	$(info Building $< at $(OPTFLAG))
	@$(CC) -c $(DEPFLAGS) $(OPTFLAG) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.c[cp]* $(OBJDIR)/%.d
	@mkdir -p $(dir $@)
	$(info Building $< at $(OPTFLAG))
	@$(CXX) -c $(DEPFLAGS) $(OPTFLAG) $(CXXFLAGS) $< -o $@

$(ELF): $(OBJECTS) $(LINKSCR)
	$(info Linking...)
	@$(LD) $(LFLAGS) -o $@ $(OBJECTS) 

$(BIN): $(ELF)
	$(OBJCPY) -O binary $< $@

$(HEX): $(ELF)
	@$(OBJCPY) --output-target=ihex $< $@
	@$(SZ) $(SZOPTS) $(ELF)

image: $(BIN)
	python3 fsbl_header.py $(BUILDDIR)/$(BINARYNAME).bin $(BUILDDIR)/$(BINARYNAME).stm32
	@ls -l $(BUILDDIR)/$(BINARYNAME).stm32

load: image
	@read -p "What is the disk device (Enter for $(SD_DISK_DEV)): " DISK && \
	DISK=$${DISK:-$(SD_DISK_DEV)} && \
	echo "Writing to $${DISK}s1 and $${DISK}s2" && \
	sudo dd if=$(BUILDDIR)/$(BINARYNAME).stm32 of=$${DISK}s1 && \
	sudo dd if=$(BUILDDIR)/$(BINARYNAME).stm32 of=$${DISK}s2 && \
	diskutil unmountDisk $${DISK}

%.d: ;

clean:
	rm -rf build

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif

.PRECIOUS: $(DEPS) $(OBJECTS) $(ELF)
.PHONY: all clean 

.PHONY: compile_commands
compile_commands:
	compiledb make

