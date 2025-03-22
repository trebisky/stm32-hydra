# Hydra for STM32 boards

This began back in 12-2-2020
by taking the files from my stm32f411/delay "bare metal" demo and going from there.

The original idea was that rather than continue working on bare metal demos
for the F411, I would merge together my F103 and F411 bare metal code and be
able to work on a single code base for both the blue and black pill.

Since then my work has focused on the STM32F4xx family and now includes
the F429 and F407 as well as the F411.
Hydra is becoming a sort of "mini-operating system" or kernel for stm32 boards.

Along the way learning about USB became a particular interest of mine and Hydra
became a platform for USB related development and experimentation.
I took USB code from Arduino-STM32 and have been overhauling it to suit
my own tastes.

See README.usb for details on all of that, and also my web blog on this
project.  Right now Hydra runs on:

1. Blue Pill STM32F103 boards (and Maple)
1. Black Pill STM32F411 boards
1. Olimex STM32-E407 board
1. Olimex STM32-P405 board
1. STM32F429 discovery kit board

Why "Hydra" -- well, why not.  It seemed like a good name, and there are a lot
of STM32 chip variants, so it might become a Hydra.
