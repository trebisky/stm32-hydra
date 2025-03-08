# Makefile for stm32-hydra
#
# Tom Trebisky  12-2-2020

TARGET = BLACK

# --------------------------------------
# Resist any urge to fool around below here.
# --------------------------------------

TOOLS = arm-none-eabi

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

BASE_OBJS = usb.o
#VCP_OBJS = misc.o  usbd_cdc_vcp.o  usbd_desc.o  usbd_usr.o
VCP_OBJS = usbd_cdc_vcp.o  usbd_desc.o
DRIVER_OBJS = usb_core.o  usb_dcd.o  usb_dcd_int.o
LIBR_OBJS = usbd_cdc_core.o  usbd_core.o  usbd_ioreq.o  usbd_req.o usbd_usr.o

# Files we no longer need, but keep around for reference
OLD_EXTRA = misc.c usb_bsp.c
OLD_EXTRA_H = core_cm4.h  core_cmInstr.h misc.h  nvic.h  usb_bsp.h  usb_conf.h

# We don't need to set CHIP here, this is usbF4 after all.
ifeq ($(TARGET),black)
#CHIP = CHIP_F411
ARM_CPU = cortex-m4
else
#CHIP = CHIP_F103
ARM_CPU = cortex-m3
endif

OBJS = $(BASE_OBJS) $(VCP_OBJS) $(DRIVER_OBJS) $(LIBR_OBJS)

# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -g
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Os

# In truth the implicit fn warnings are good, but for the purpose of
#  these demos, I can't be busy to set up prototypes as I should.
# CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Wno-implicit-function-declaration -fno-builtin

#CC = $(TOOLS)-gcc -mcpu=$(ARM_CPU) -mthumb -Wno-implicit-function-declaration -fno-builtin -DHYDRA -D$(CHIP) -O -I.
CC = $(TOOLS)-gcc -mcpu=$(ARM_CPU) -mthumb -Wno-implicit-function-declaration -fno-builtin -DHYDRA -O -I.. -I.

all: ../usbf4.o

../usbf4.o: usbf4.o
	cp usbf4.o ..

usbf4.o:	$(OBJS)
	$(LD) -r $(OBJS) -o usbf4.o

# From VCP
#misc.o: vcp/misc.c
#	$(CC) -o $@ -c $<
#usb_bsp.o: vcp/usb_bsp.c
#	$(CC) -o $@ -c $<
usbd_cdc_vcp.o: vcp/usbd_cdc_vcp.c
	$(CC) -o $@ -c $<
usbd_desc.o: vcp/usbd_desc.c
	$(CC) -o $@ -c $<

# From driver
usb_core.o: driver/usb_core.c
	$(CC) -o $@ -c $<
usb_dcd.o: driver/usb_dcd.c
	$(CC) -o $@ -c $<
usb_dcd_int.o: driver/usb_dcd_int.c
	$(CC) -o $@ -c $<

# From library
usbd_cdc_core.o: library/usbd_cdc_core.c
	$(CC) -o $@ -c $<
usbd_core.o: library/usbd_core.c
	$(CC) -o $@ -c $<
usbd_ioreq.o: library/usbd_ioreq.c
	$(CC) -o $@ -c $<
usbd_req.o: library/usbd_req.c
	$(CC) -o $@ -c $<
usbd_usr.o: library/usbd_usr.c
	$(CC) -o $@ -c $<

bogus:

hydra.elf: 	$(OBJS) $(LDS_FILE)
	$(LD) -T $(LDS_FILE) -o hydra.elf $(OBJS)

# Doesn't work unless the compile command follows
%.o: %.c
	$(CC) -o $@ -c $<

clean:
	rm -f *.o
