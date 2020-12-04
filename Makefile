# Makefile for stm32-hydra
#
# Tom Trebisky  12-2-2020

#TARGET = black
TARGET = blue

# --------------------------------------
# Resist any urge to fool around below here.
# --------------------------------------

TOOLS = arm-none-eabi

# Assembling with gcc makes it want crt0 at link time.
#AS = $(TOOLS)-gcc
AS = $(TOOLS)-as

#LD = $(TOOLS)-gcc
LD = $(TOOLS)-ld.bfd
OBJCOPY = $(TOOLS)-objcopy
DUMP = $(TOOLS)-objdump -d
GDB = $(TOOLS)-gdb

BASE_OBJS = init.o main.o flash.o led.o serial.o nvic.o exti.o systick.o event.o iic.o

ifeq ($(TARGET),black)
CHIP = CHIP_F411
ARM_CPU = cortex-m4
LDS_FILE=f411.lds
OBJS = locore_411.o $(BASE_OBJS) rcc_411.o gpio_411.o
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f4x.cfg
else
CHIP = CHIP_F103
ARM_CPU = cortex-m3
LDS_FILE=f103.lds
OBJS = locore_103.o $(BASE_OBJS) rcc_103.o gpio_103.o
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg
endif

# Use the -g flag if you intend to use gdb
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -g
#CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Os

# In truth the implicit fn warnings are good, but for the purpose of
#  these demos, I can't be busy to set up prototypes as I should.
# CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Wno-implicit-function-declaration -fno-builtin

CC = $(TOOLS)-gcc -mcpu=$(ARM_CPU) -mthumb -Wno-implicit-function-declaration -fno-builtin  -D$(CHIP) -O

all: show hydra.elf hydra.dump hydra.bin

show:
ifeq ($(TARGET),black)
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

# I tried using gdb on Fedora 32, and apparently it is gone!
# It was last supplied as a Fedora package in Fedora 29, no telling why.

gdb:
	$(GDB) --eval-command="target remote localhost:3333" hydra.elf

gdbtui:
	$(GDB) -tui --eval-command="target remote localhost:3333" hydra.elf

clean:
	rm -f *.o hydra.elf hydra.dump hydra.bin
