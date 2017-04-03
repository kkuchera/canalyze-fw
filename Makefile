# MCU settings
CORE = CORE_M0
CPU = cortex-m0
#HSI = 8000000
HSE = 16000000
TARGET_DEVICE = STM32F042x6
OPENOCD_CFG = stm32f0x.cfg

# Compilation variables
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Compilation defines
DEFS = -D$(CORE) -D$(TARGET_DEVICE) -DHSE_VALUE=$(HSE)

# Compilation flags
CFLAGS = -g -Os -pedantic -Wall -Wextra -Werror -mthumb -mcpu=$(CPU) $(DEFS)
CFLAGSLIB = -g -Os -Wall -mthumb -mcpu=$(CPU) $(DEFS)
LDFLAGS = -fno-exceptions -ffunction-sections -fdata-sections -Wl,--gc-sections
LDFLAGS += -TSTM32F042x6xx_FLASH.ld

# Project variables
LIBDIR = lib
SRCDIR = src
OBJDIR = obj
TARGETDIR = bin
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.c))
OBJS += $(patsubst $(SRCDIR)/%.s, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.s))

# STM32Cube variables
CUBELIBDIR = $(LIBDIR)/STM32Cube_FW_F0_V1.7.0
CUBEOBJDIR = $(LIBDIR)/stm32cube
CUBELIB = $(CUBEOBJDIR)/stm32cube.a
HAL_SRCDIR = $(CUBELIBDIR)/Drivers/STM32F0xx_HAL_Driver/Src
HAL_SRCS = $(filter-out $(wildcard $(HAL_SRCDIR)/*template.c), $(wildcard $(HAL_SRCDIR)/*.c))
HAL_OBJS = $(patsubst $(HAL_SRCDIR)/%.c, $(CUBEOBJDIR)/%.o, $(HAL_SRCS))
USBCORE_SRCDIR = $(CUBELIBDIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Src
USBCORE_SRCS = $(filter-out $(wildcard $(USBCORE_SRCDIR)/*template.c), $(wildcard $(USBCORE_SRCDIR)/*.c))
USB_OBJS = $(patsubst $(USBCORE_SRCDIR)/%.c, $(CUBEOBJDIR)/%.o, $(USBCORE_SRCS))

LIBS = $(CUBELIB)
TARGET = $(TARGETDIR)/canalyze

# Includes
INCLUDE = -I inc
INCLUDE += -I $(CUBELIBDIR)/Drivers/CMSIS/Include
INCLUDE += -I $(CUBELIBDIR)/Drivers/CMSIS/Device/ST/STM32F0xx/Include
INCLUDE += -I $(CUBELIBDIR)/Drivers/STM32F0xx_HAL_Driver/Inc
INCLUDE += -I $(CUBELIBDIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc

# The dependency file names.
DEPS = $(OBJS:.o=.d)

# Make options
default: $(TARGET).elf
all: $(TARGET).hex $(TARGET).bin
cubelib: $(CUBELIB)

flash: $(TARGET).elf
	openocd -f $(OPENOCD_CFG) -c "program $< verify reset exit"

dfu: $(TARGET).bin
	dfu-util -a 0 -s 0x08000000 -D $<

# Application dependencies
$(TARGET).hex: $(TARGET).elf 
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).elf: $(OBJS) $(CUBELIB)
	@mkdir -p $(TARGETDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SIZE) $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -MMD -o $@ -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.s
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -MMD -o $@ -c $<

# STM32Cube dependencies
$(CUBELIB): $(HAL_OBJS) $(USB_OBJS)
	$(AR) -r $@ $^

$(CUBEOBJDIR)/%.o: $(HAL_SRCDIR)/%.c
	@mkdir -p $(CUBEOBJDIR)
	$(CC) $(CFLAGSLIB) $(INCLUDE) -o $@ -c $<

$(CUBEOBJDIR)/%.o: $(USBCORE_SRCDIR)/%.c
	@mkdir -p $(CUBEOBJDIR)
	$(CC) $(CFLAGSLIB) $(INCLUDE) -o $@ -c $<

# Cleanup
.PHONY: all, clean, veryclean
clean:
	$(RM) -r $(OBJDIR)

veryclean: clean
	$(RM) -r $(TARGETDIR)
	$(RM) -r $(CUBEOBJDIR)

# Let make read the dependency files and handle them.
-include $(DEPS)
