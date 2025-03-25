# Makefile for stm32-hydra
#
# Tom Trebisky  12-2-2020
# overhauled 3-10-2025 to inherit from above

ifndef TARGET
$(error Don't run make here)
endif

#BASE_OBJS = usb.o
#VCP_OBJS = misc.o  usbd_cdc_vcp.o  usbd_desc.o  usbd_usr.o
#VCP_OBJS = class.o usbd_cdc_vcp.o  usbd_desc.o
#VCP_OBJS = class.o desc.o usbd_cdc_vcp.o  usbd_desc.o
VCP_OBJS = class.o desc.o usbd_cdc_vcp.o

DRIVER_OBJS = usb_core.o  usb_dcd.o  usb_dcd_int.o
LIBR_OBJS = public.o usbd_cdc_core.o  usbd_core.o  usbd_ioreq.o  usbd_req.o usbd_usr.o

# Files we no longer need, but keep around for reference
OLD_EXTRA = misc.c usb_bsp.c
OLD_EXTRA_H = core_cm4.h  core_cmInstr.h misc.h  nvic.h  usb_bsp.h  usb_conf.h

#OBJS = $(BASE_OBJS) $(DRIVER_OBJS) $(LIBR_OBJS)
#OBJS = $(BASE_OBJS) $(VCP_OBJS) $(DRIVER_OBJS) $(LIBR_OBJS)
OBJS = $(VCP_OBJS) $(DRIVER_OBJS) $(LIBR_OBJS)

# # no effect here, CC is already defined
INCDEFS = -I./library -I.

all: ../usbf4.o

../usbf4.o: usbf4.o
	cp usbf4.o ..

usbf4.o:	$(OBJS)
	$(LD) -r $(OBJS) -o usbf4.o

# From VCP
class.o: vcp/class.c
	$(CC) -o $@ -c $<
desc.o: vcp/desc.c
	$(CC) -o $@ -c $<
usbd_cdc_vcp.o: vcp/usbd_cdc_vcp.c
	$(CC) -o $@ -c $<
usbd_desc.o: vcp/usbd_desc.c
	$(CC) -o $@ -c $<
usbd_cdc_core.o: vcp/usbd_cdc_core.c
	$(CC) -o $@ -c $<

# From driver
usb_core.o: driver/usb_core.c
	$(CC) -o $@ -c $<
usb_dcd.o: driver/usb_dcd.c
	$(CC) -o $@ -c $<
usb_dcd_int.o: driver/usb_dcd_int.c
	$(CC) -o $@ -c $<

# From library
#usbd_cdc_core.o: library/usbd_cdc_core.c
#	$(CC) -o $@ -c $<
public.o: library/public.c
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

# Doesn't work unless the compile command follows
%.o: %.c
	$(CC) -o $@ -c $<

clean:
	rm -f *.o
