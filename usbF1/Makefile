# build the usbF1 subsystem
# Tom Trebisky  3-2-2025

TOOLS = arm-none-eabi

# This is correct for the stm32F103
ARM_CPU = cortex-m3
MCU = MCU_STM32F103C8

CC = $(TOOLS)-gcc -mcpu=$(ARM_CPU) -mthumb -Wno-implicit-function-declaration -fno-builtin -DHYDRA -D$(MCU) -O -I.
LD = $(TOOLS)-ld.bfd

all: ../usbf1.o

../usbf1.o: usbf1.o
	cp usbf1.o ..

LIB_OBJS = usb_core.o  usb_init.o  usb_mem.o  usb_regs.o
DRIVER_OBJS = usb.o  usb_cdcacm.o  usb_reg_map.o

#OBJS = $(BASE_OBJS) $(LIB_OBJS) $(DRIVER_OBJS)
OBJS = $(LIB_OBJS) $(DRIVER_OBJS)

usbf1.o:	$(OBJS)
	$(LD) -r $(OBJS) -o usbf1.o

# From Lib
usb_core.o: lib/usb_core.c
	$(CC) -o $@ -c $<
usb_init.o: lib/usb_init.c
	$(CC) -o $@ -c $<
usb_mem.o: lib/usb_mem.c
	$(CC) -o $@ -c $<
usb_regs.o: lib/usb_regs.c
	$(CC) -o $@ -c $<

# From Driver
usb.o: driver/usb.c
	$(CC) -o $@ -c $<
usb_cdcacm.o: driver/usb_cdcacm.c
	$(CC) -o $@ -c $<
usb_reg_map.o: driver/usb_reg_map.c
	$(CC) -o $@ -c $<

clean:
	rm *.o
	rm ../usbf1.o

# THE END
