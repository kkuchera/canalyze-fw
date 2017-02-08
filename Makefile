# MCU settings
CORE = CORE_M0
CPU = cortex-m0
#HSI = 8000000
HSE = 16000000
TARGET_DEVICE = STM32F042x6
OPENOCD_CFG = stm32f0.cfg

# Compilation variables
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
OBJCOPY = arm-none-eabi-objcopy

# Compilation defines
DEFS = -D$(CORE) -D$(TARGET_DEVICE) -DHSE_VALUE=$(HSE)

# Compilation flags
CFLAGS = -g -Os -pedantic -Wall -Wextra -Werror -mthumb -mcpu=$(CPU) $(DEFS)
LDFLAGS = -fno-exceptions -ffunction-sections -fdata-sections -Wl,--gc-sections
LDFLAGS += -TSTM32F042x6xx_FLASH.ld

# Project variables
LIBDIR = lib
SRCDIR = src
OBJDIR = obj
TARGETDIR = bin
#INCLUDE = 
#LIBS = 
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.c))
OBJS += $(patsubst $(SRCDIR)/%.s, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.s))
TARGET = $(TARGETDIR)/CANalyze

# The dependency file names.
DEPS = $(OBJS:.o=.d)

# Make options
default: $(TARGET).elf
all: $(TARGET).hex $(TARGET).bin
flash: $(TARGET).elf
	openocd -f $(OPENOCD_CFG) -c "program $< verify reset exit"

# Dependencies
$(TARGET).hex: $(TARGET).elf 
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).elf: $(OBJS)
	@mkdir -p $(TARGETDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -MMD -o $@ -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.s
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -MMD -o $@ -c $<

# Cleanup
.PHONY: all, clean, veryclean
clean:
	$(RM) -r $(OBJDIR)

veryclean: clean
	$(RM) -r $(TARGETDIR)

# Let make read the dependency files and handle them.
-include $(DEPS)
