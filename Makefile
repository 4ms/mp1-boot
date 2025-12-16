BINARYNAME = fsbl

SERIES ?= stm32mp15x

OPTFLAG = -O2
# build/obj/obj/third-party/STM32MP13x_HAL_Driver/Src/stm32mp13xx_hal_ddr.o: OPTFLAG = -O0

SRCDIR = src
EXTLIBDIR = third-party

ifeq ($(SERIES),stm32mp13x)
LINKSCR := linkscript-mp13x.ld
SERIESDIR := ${SRCDIR}/mp13x
HALDIR := $(EXTLIBDIR)/STM32MP13x_HAL_Driver
else
LINKSCR := linkscript.ld
SERIESDIR := ${SRCDIR}/mp15x
HALDIR := $(EXTLIBDIR)/STM32MP1xx_HAL_Driver
endif

SD_DISK_STEM ?= 

SOURCES = $(SERIESDIR)/startup.s \
		  $(SRCDIR)/main.cc \
		  $(SRCDIR)/libc_stub.c \
		  $(SRCDIR)/libcpp_stub.cc \
		  $(SRCDIR)/print.cc \
		  $(SRCDIR)/drivers/ram_tests.cc \
		  $(SRCDIR)/uboot-port/common/memsize.c \
		  $(SRCDIR)/uboot-port/lib/crc32.c \
		  $(SRCDIR)/drivers/norflash/qspi_ll.c \
		  $(SRCDIR)/drivers/norflash/qspi_norflash_read.c \
		  $(SRCDIR)/gpt/gpt.cc \
		  $(HALDIR)/Src/stm32mp1xx_ll_usart.c \
		  $(HALDIR)/Src/stm32mp1xx_ll_rcc.c \
		  $(HALDIR)/Src/stm32mp1xx_hal.c \
		  $(HALDIR)/Src/stm32mp1xx_ll_sdmmc.c \
		  $(HALDIR)/Src/stm32mp1xx_hal_sd.c \
		  $(SERIESDIR)/systeminit.c \
		  $(SERIESDIR)/drivers/ddr/stm32mp1_ram.cc


INCLUDES = -I. \
		   -I$(SRCDIR) \
		   -I$(SRCDIR)/board_conf \
		   -I$(SRCDIR)/uboot-port/include \
		   -I$(SRCDIR)/uboot-port/arch/arm/include \
		   -I$(HALDIR)/Inc \
		   -I$(SERIESDIR) \
		   -I$(EXTLIBDIR)/CMSIS/Core_A/Include \
		   -I$(EXTLIBDIR)/CMSIS/Device/ST/STM32MP1xx/Include

ifeq ($(SERIES),stm32mp13x)
	SOURCES += $(HALDIR)/Src/stm32mp13xx_hal_ddr.c
	SOURCES += $(HALDIR)/Src/stm32mp13xx_hal_rcc.c
	SOURCES += $(HALDIR)/Src/stm32mp13xx_hal_rcc_ex.c # Required only for HAL_SD_InitCard to get SDMMC clock speed
else
	SOURCES += $(SERIESDIR)/drivers/ddr/stm32mp1_ddr.cc
endif



MCU = -mcpu=cortex-a7 -march=armv7ve -mfpu=neon-vfpv4 -mlittle-endian -mfloat-abi=hard


ARCH_CFLAGS = -DUSE_FULL_LL_DRIVER \
			  -DCORE_CA7

ifeq ($(SERIES),stm32mp13x)
	ARCH_CFLAGS += -DSTM32MP135Dxx -DSTM32MP13
else
	ARCH_CFLAGS += -DSTM32MP157Cxx -DSTM32MP1
endif

ifeq ("$(BOARD_CONF)","OSD32")
	ARCH_CFLAGS += -DBOARD_CONF_OSD32
else
ifeq ("$(BOARD_CONF)","DK2")
	ARCH_CFLAGS += -DBOARD_CONF_DK2
else
ifneq ("$(BOARD_CONF)","")
	ARCH_CFLAGS += -DBOARD_CONF_PATH=$(BOARD_CONF)
endif
endif
endif

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
		-std=c++20 \
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
		 -Wl,--no-warn-rwx-segments \
		 $(EXTRALDFLAGS) \

DEPFLAGS = -MMD -MP -MF $(OBJDIR)/$(basename $<).d

# By default, this uses the toolchain on your path
# Override by invoking make with TOOLCHAIN_DIR=/my/location/ (final slash is required)
TOOLCHAIN_DIR ?= 
ARCH    = arm-none-eabi
CC      = ${TOOLCHAIN_DIR}$(ARCH)-gcc
CXX     = ${TOOLCHAIN_DIR}$(ARCH)-g++
LD      = ${TOOLCHAIN_DIR}$(ARCH)-g++
AS      = ${TOOLCHAIN_DIR}$(ARCH)-as
OBJCPY  = ${TOOLCHAIN_DIR}$(ARCH)-objcopy
OBJDMP  = ${TOOLCHAIN_DIR}$(ARCH)-objdump
GDB     = ${TOOLCHAIN_DIR}$(ARCH)-gdb
SZ      = ${TOOLCHAIN_DIR}$(ARCH)-size

SZOPTS 	= -d

ELF 	= $(BUILDDIR)/$(BINARYNAME).elf
HEX 	= $(BUILDDIR)/$(BINARYNAME).hex
BIN 	= $(BUILDDIR)/$(BINARYNAME).bin


OBJECTS   = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(SOURCES))))
DEPS   	  = $(addprefix $(OBJDIR)/, $(addsuffix .d, $(basename $(SOURCES))))
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj/obj

all: Makefile $(ELF) $(UIMAGENAME) image
	@:

mp13x:
	$(MAKE) SERIES=stm32mp13x BOARD_CONF=brainboard-mp13_conf.hh

mp13x-load:
	$(MAKE) load SERIES=stm32mp13x BOARD_CONF=brainboard-mp13_conf.hh

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
	$(info Linking with $(LINKSCR)...)
	@$(LD) $(LFLAGS) -o $@ $(OBJECTS) 

$(BIN): $(ELF)
	$(OBJCPY) -O binary $< $@

$(HEX): $(ELF)
	@$(OBJCPY) --output-target=ihex $< $@
	@$(SZ) $(SZOPTS) $(ELF)

image: $(BUILDDIR)/$(BINARYNAME).stm32

$(BUILDDIR)/$(BINARYNAME).stm32: $(BIN)
	python3 fsbl_header.py $(BUILDDIR)/$(BINARYNAME).bin $(BUILDDIR)/$(BINARYNAME).stm32 $(SERIES)
	@ls -l $(BUILDDIR)/$(BINARYNAME).stm32


load: image
	@read -p "What is the disk device stem (Enter for $(SD_DISK_STEM)): " DISKSTEM && \
	DISKSTEM=$${DISKSTEM:-$(SD_DISK_STEM)} && \
	echo "Writing to $${DISKSTEM}1 and $${DISKSTEM}2" && \
	sudo dd if=$(BUILDDIR)/$(BINARYNAME).stm32 of=$${DISKSTEM}1 && \
	sudo dd if=$(BUILDDIR)/$(BINARYNAME).stm32 of=$${DISKSTEM}2 
    
%.d: ;

clean:
	rm -rf build

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif

.PRECIOUS: $(DEPS) $(OBJECTS) $(ELF)
.PHONY: all clean image load
