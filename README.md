# PiSampler

**Last updated (12/29/2019)**

**Quick disclaimer:** early commits of BCM control files were created and developed before I became aware of the open source pigpio library: https://github.com/joan2937/pigpio. I have built the base of my program off of my own interpretation of / approach toward the BCM2385 spec, but have referred to joan et al's implementation at times that I've run into dead ends. I neither want to pretend like these files are anything less than my own work, nor do I want to ignore credit where it is due. Any functions pulled directly from joan et al's files will be cited accordingly. - NN 

This is an in-progress personal project to turn a Raspberry Pi into a fully-functional audio sampler, looper, mixer, and writer. Ideally, for my own interests, it will run on a Pi Zero and be embedded in a guitar pedal, but I am actively testing it on a Pi 3B and intend to make it functional for any Pi model. The twist of this effects pedal is, because the software is running on an OS, you will be able to save your samples to WAV files while using it!
For my design I am using the PMOD I2S2 DAC/ADC hat from Digilent: https://store.digilentinc.com/pmod-i2s2-stereo-audio-input-and-output/, connected through the Pi's PCM interface. For now I am testing the design with the PMOD in MASTER mode, but will be generalizing my PCM code to support master or slave operation and (hopefully) support running with any other exeternal I2S hats.

# BCM files

*Register-level control files following the BCM2385 ARM Manual: https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf. Different models of the Pi have different BCM chips but each has roughly the same functionality as the 2385.* 

**clk.h/.c**: for specifying, enabling and disabling GPCLKs. See ~pg 105 of the BCM2385 manual.

**dma.h/.c**: for DMA registry mapping and specifying DMA control blocks. See ~pg 38 of the BCM2385 manual.

**gpio.h/.c**: for GPIO pin assignments.

**pcm.h/.c**: for I2S programming for the Pi. See ~pg 119 of the BCM2385 manaul.

**pimem.h/.c**: for mapping RAM to physical addresses in the Pi. See https://www.raspberrypi.org/documentation/hardware/raspberrypi/peripheral_addresses.md.

# PYTHON files

*These are mostly scripts used for simplifying design / testing rather than for the program itself*

**compile.py**: script for automating GCC compilation.

**git_corrupt_fix.py**: script for fixing "corrupt loose object" error in GIT.

# Remaining files

**globals.h**: for macros, variables and functions to be shared by / accessible to all files. For now, mostly used for debugging.

**main.c**: main file. BCM funcs are geared towards PMOD I2S2 model.

**wavewriter.c**: C funcs for handling WAVE files. Functions are very general and are adopted from http://blog.acipo.com/generating-wave-files-in-c/. This is not yet incorporated into the rest of the project and serves more as a test file.
