CC = arm-none-eabi-gcc
LD = arm-none-eabi-g++
AR = arm-non-eabi-ar
OBJCOPY = arm-none-eabi-objcopy

OBJDIR = build
PERIPH = lib/STM32F4xx_StdPeriph_Driver
USB = lib/STM32_USB_OTG_Driver
USBD = lib/STM32_USB_Device_Library

TARGET_NAME = spincoater
ELF = $(OBJDIR)/$(TARGET_NAME).elf
HEX = $(OBJDIR)/$(TARGET_NAME).hex
BIN = $(OBJDIR)/$(TARGET_NAME).bin


SOURCES += \
	$(USB)/src/usb_core.c \
	$(USB)/src/usb_dcd_int.c \
	$(USB)/src/usb_dcd.c \
	$(USBD)/Class/cdc/src/usbd_cdc_core_loopback.c \
	$(USBD)/Core/src/usbd_core.c \
	$(USBD)/Core/src/usbd_ioreq.c \
	$(USBD)/Core/src/usbd_req.c
SOURCES += \
	$(PERIPH)/src/misc.c \
	$(PERIPH)/src/stm32f4xx_exti.c \
	$(PERIPH)/src/stm32f4xx_gpio.c \
	$(PERIPH)/src/stm32f4xx_flash.c \
	$(PERIPH)/src/stm32f4xx_rcc.c \
	$(PERIPH)/src/stm32f4xx_syscfg.c \
	$(PERIPH)/src/stm32f4xx_tim.c 
SOURCES += \
	src/main.cpp \
	src/stm32f4xx_ip_dbg.c \
	src/stm32f4xx_it.c \
	src/syscalls.c \
	src/system_stm32f4xx.c \
	src/startup_stm32f413_423xx.s \
	src/usb/usb_bsp.c \
	src/usb/usb_vcp.cpp \
	src/usb/usbd_desc.c \
	

INCLUDES += \
	-Isrc \
	-Isrc/usb \
	-I$(PERIPH)/inc \
	-I$(USB)/inc \
	-I$(USBD)/Core/inc \
	-I$(USBD)/Class/cdc/inc \
	-Ilib/CMSIS/Include \
	-Ilib/CMSIS/Device/ST/STM32F4xx/Include


OBJECTS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(SOURCES))))

LDSCRIPT = memory.ld
LDFLAGS += -T$(LDSCRIPT) --specs=nosys.specs -mthumb -mcpu=cortex-m4 -mfloat-abi=hard 

DEFINES = -DSTM32F413_423xx \
	-DUSE_USB_OTG_FS \
	-DUSE_STDPERIPH_DRIVER \
	-DHSE_VALUE=8000000

CFLAGS  = -O0 -g -Wall -I.\
	-mcpu=cortex-m4 -mthumb \
	-mfpu=fpv4-sp-d16 -mfloat-abi=hard \
	--specs=nosys.specs \
	$(DEFINES) \
	$(INCLUDES) 

CPPFLAGS = $(CFLAGS) -std=c++14

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(ELF): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(OBJDIR)/%.o: %.c $(DEPS)
	mkdir -p $(dir $@)
	$(CC) -MMD -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cpp $(DEPS)
	mkdir -p $(dir $@)
	$(CC) -MMD -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@


-include $(shell find $(OBJDIR)/ -type f -name '*.d')

all: $(BIN) $(HEX)

gdb: $(ELF)
	arm-none-eabi-gdb -q -x openocd.gdb

clean:
	rm -rf build