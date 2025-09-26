# Silky Design - Flash ROM Programmer

This is a programmer for the popular SST39SF010A/020A/040 devices from MicroChip.

These devices are 128Mx8, 256Mx8, and 512Mx8, and are used in a number of retro-computing memory and single-board-computers.

The programmer uses a Raspberry Pi Pico2 (RP2350) micro-controller to read image files from a MicroSD card or from the USB port from an application running on a host computer. The RPi Pico2 buffers the image in its internal memory and then writes it to the device.

The programmer can read a Flash device and provides options of saving the device image to the SD card, sending it to a host computer, or writing the image onto another Flash device.

The programmer is powered from the USB port on the RPi Pico2.

The programmer includes a small display and an input knob and switch to navigate through the options, SD card contents, etc.

## Repo Contents

This repo includes:

1. the hardware schematic, PCB layout, and other files needed to produce the PCB.
2. the software for the RPi Pico2, including CMake files to build the RPi Pico2 image.
3. some software for a host computer to communicate with the programmer and transfer files.
4. some component data sheets (for the more unique components)
5. 3D print files for a simple enclosure

## License

Except where noted, the software and hardware for this project is designed/authored by AESilky, and is licensed under the MIT License (included in the repo in the LICENSE file).
