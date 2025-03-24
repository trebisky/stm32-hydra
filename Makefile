# Makefile for stm32-hydra
#
# Tom Trebisky  12-2-2020

# "e407" is my Olimex E407 board
# "disco" is my STM32F429 discovery board
# "black" is my STM32F411 "black pill" board
# "blue" is my STM32F103 "blue pill" board (or olimex or maple)

#TARGET = e407
TARGET = disco
#TARGET = p405
#TARGET = black
#TARGET = blue

TOOLS = arm-none-eabi

# --------------------------------------
# Resist any urge to fool around below here.
# --------------------------------------

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

BASE_OBJS = init.o main.o flash.o led.o serial.o nvic.o exti.o systick.o event.o iic.o

# One or the other
USB_OBJS = usbf4.o
#USB_OBJS = usb411.o usb_console.o

ifeq ($(TARGET),e407)
	CHIPDEFS = -DCHIP_F411 -DCHIP_F407
	ARM_CPU = cortex-m4
	LDS_FILE=f411.lds
	OBJS = locore_411.o $(BASE_OBJS) rcc_411.o gpio_411.o $(USB_OBJS)
	OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
else ifeq ($(TARGET),p405)
	CHIPDEFS = -DCHIP_F411 -DCHIP_F405
	ARM_CPU = cortex-m4
	LDS_FILE=f411.lds
	OBJS = locore_411.o $(BASE_OBJS) rcc_411.o gpio_411.o $(USB_OBJS)
	#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
	OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
else ifeq ($(TARGET),disco)
	CHIPDEFS = -DCHIP_F411 -DCHIP_F429
	ARM_CPU = cortex-m4
	LDS_FILE=f411.lds
	OBJS = locore_411.o $(BASE_OBJS) rcc_411.o gpio_411.o $(USB_OBJS)
	#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
	OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
else ifeq ($(TARGET),black)
	CHIPDEFS = -DCHIP_F411
	ARM_CPU = cortex-m4
	LDS_FILE=f411.lds
	OBJS = locore_411.o $(BASE_OBJS) rcc_411.o gpio_411.o $(USB_OBJS)
	#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
	OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
else
	CHIPDEFS = -DCHIP_F103
	ARM_CPU = cortex-m3
	LDS_FILE=f103.lds
	OBJS = locore_103.o $(BASE_OBJS) rcc_103.o gpio_103.o
	#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg
	#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
	OCDCFG = -f /usr/share/openocd/scripts/interface/stlink.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
endif

# This sends all of our variables to sub-makefiles
export

# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -g
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Os

# In truth the implicit fn warnings are good, but for the purpose of
#  these demos, I can't be busy to set up prototypes as I should.
# CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Wno-implicit-function-declaration -fno-builtin

# CDEFS = -D$(CHIP) -DHYDRA -DHYDRA_USB
CDEFS = $(CHIPDEFS) -DHYDRA -DHYDRA_USB

INCDEFS = -I./library -I.

CC = $(TOOLS)-gcc -mcpu=$(ARM_CPU) -mthumb -Wno-implicit-function-declaration -fno-builtin  $(CDEFS) $(INCDEFS) -O

all: show tags hydra.elf hydra.dump hydra.bin

usbf4.o:	bogus
	cd usbF4; make

bogus:

tags:	bogus
	ctags -R .
	rm -f cscope.*
	cscope -Rqb

# Make is stupid about ifeq and @echo
# if you want to use both, they must be inside a target
show:
ifeq ($(TARGET),e407)
	@echo "Building for Olimex E407 (STM32F407)"
else ifeq ($(TARGET),disco)
	@echo "Building for STM32F429 discovery (STM32F429)"
else ifeq ($(TARGET),black)
	@echo "Building for black-pill (STM32F411)"
else
	@echo "Building for blue-pill (STM32F103)"
endif

# Look at object file sections
zoot:
	$(TOOLS)-objdump -h *.o
	$(TOOLS)-objdump -h hydra.elf

hydra.dump:	hydra.elf
	$(DUMP) hydra.elf >hydra.dump

hydra.elf: 	$(OBJS) $(LDS_FILE)
	$(LD) -T $(LDS_FILE) -o hydra.elf $(OBJS)

hydra.bin:        hydra.elf
	$(OBJCOPY) hydra.elf hydra.bin -O binary

locore.o:	locore.s
	$(AS) locore.s -o locore.o

# Doesn't work unless the compile command follows
%.o: %.c hydra.h
	$(CC) -o $@ -c $<

#.c.o:
#	$(CC) -o $@ -c $<

flash:  hydra.elf
	openocd $(OCDCFG) -c "program hydra.elf verify reset exit"

ocd:
	openocd $(OCDCFG)

# --------------------------------------------------------------
# For some reason that I don't understand, when I leave my board overnight,
# I will come back to it and when I try "make flash" it will fail.
# Among the openocd messages is this one:
# Warn : target stm32f4x.cpu examination failed
# I have details on my web page. 
# Someday I should dig into this more deeply
# The fix (so far) has been to
# connect an external ST-Link device, then download something
# using the following:

UB_OCDCFG = -f /usr/share/openocd/scripts/interface/sthack.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg

unbrick: unbrick.elf
	openocd $(UB_OCDCFG) -c "program unbrick.elf verify reset exit"

# I use my very simple bare metal serial demo
unbrick.elf:
	cp /u1/Projects/STM32/Black_pill/STM32F411/serial1/serial.elf unbrick.elf

# to connect the external ST-link, 3 wires are required.
# remove CN4 jumpers and one jumper from JP2 on the back
#  Green - ground
#  Yellow - SWCLK - to pin 2 on CN4
#  Blue - SWDIO - to pin 4 on CN4
#  do NOT connect power from the external ST-Link
# poower the board using the mini-USB as always.

# --------------------------------------------------------------
# I tried using gdb on Fedora 32, and apparently it is gone!
# It was last supplied as a Fedora package in Fedora 29, no telling why.
# I found it again and installed it on Fedora 41, years later.

gdb:
	$(GDB) --eval-command="target remote localhost:3333" hydra.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" hydra.elf

clean:
	cd usbF4; make clean
	rm -f *.o hydra.elf hydra.dump hydra.bin
