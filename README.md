# stm32-hydra

This is a sort of mini-operating system or kernel for stm32 boards.
I just finished doing a bunch of development for the STM32F411 "black pill" and
wanted to "blend in" code I have written for the STM32F103 "blue pill".
This will be interesting in itself, and allow me to invest my energies into
one codebase for both chips.

So this project began 12-2-2020 by taking the files from my stm32f411/delay
demo and going from there.

Why "Hydra" -- well, why not.  It seemed like a good name, and there are a lot
of STM32 chip variants, so it might become a Hydra.

The USB subsystem for the F411 is of particular interest, see README.usb
