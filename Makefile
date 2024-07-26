BINARYNAME = fsbl

OPTFLAG = -O3

SRCDIR = src
EXTLIBDIR = third-party
LINKSCR = linkscript.ld

SD_DISK_STEM ?= /dev/disk4s

SOURCES = $(SRCDIR)/startup.s \
		  $(SRCDIR)/main.cc \
		  $(SRCDIR)/systeminit.c \
		  $(SRCDIR)/libc_stub.c \
		  $(SRCDIR)/libcpp_stub.cc \
		  $(SRCDIR)/print.cc \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_ll_usart.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_ll_rcc.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_ll_sdmmc.c \
		  $(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Src/stm32mp1xx_hal_sd.c \
		  $(SRCDIR)/drivers/ddr/stm32mp1_ddr.cc \
		  $(SRCDIR)/drivers/ddr/stm32mp1_ram.cc \
		  $(SRCDIR)/drivers/ddr/ram_tests.cc \
		  $(SRCDIR)/uboot-port/common/memsize.c \
		  $(SRCDIR)/uboot-port/lib/crc32.c \
		  $(SRCDIR)/drivers/norflash/qspi_ll.c \
		  $(SRCDIR)/drivers/norflash/qspi_norflash_read.c \
		  $(SRCDIR)/gpt/gpt.cc \

INCLUDES = -I. \
		   -I$(SRCDIR) \
		   -I$(SRCDIR)/board_conf \
		   -I$(EXTLIBDIR)/STM32MP1xx_HAL_Driver/Inc \
		   -I$(EXTLIBDIR)/CMSIS/Core_A/Include \
		   -I$(EXTLIBDIR)/CMSIS/Device/ST/STM32MP1xx/Include \
		   -I$(SRCDIR)/drivers/ddr/ \
		   -I$(SRCDIR)/uboot-port/include \
		   -I$(SRCDIR)/uboot-port/arch/arm/include \

MCU = -mcpu=cortex-a7 -march=armv7ve -mfpu=neon-vfpv4 -mlittle-endian -mfloat-abi=hard


ARCH_CFLAGS = -DUSE_FULL_LL_DRIVER \
			  -DSTM32MP157Cxx \
			  -DSTM32MP1 \
			  -DCORE_CA7

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

$(BUILDDIR)/$(BINARYNAME).stm32: $(BIN)
	python3 fsbl_header.py $(BUILDDIR)/$(BINARYNAME).bin $(BUILDDIR)/$(BINARYNAME).stm32
	@ls -l $(BUILDDIR)/$(BINARYNAME).stm32

image: $(BUILDDIR)/$(BINARYNAME).stm32

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

.PHONY: compile_commands
compile_commands:
	compiledb make
	compdb -p ./ list > compile_commands.tmp 2>/dev/null
	rm compile_commands.json
	mv compile_commands.tmp compile_commands.json

