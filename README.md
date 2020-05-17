# VGA signal generation with PIC24FJ256GA705

This project is dedicated to use the PIC24FJ256GA705 microcontroller for the generation of a VGA video signal. 

## Structure

The repository is structured as follows:
- **src**: Contains all the source code for the microcontroller.
- **nbproject**: Contains all the MPLABX project. That includes all the Makefiles and IDE configuration as well as all the environmental variables definition.
- **PCB_Design/VGA_adapter**: Contains the schematics and PCB design for the VGA Adaptor to be plug in the [PIC24F Curiosity Board](https://www.microchip.com/DevelopmentTools/ProductDetails/DM240004).
- **dithering**: Contains the python scripts that have been used for this project to generate and adapt images to be used in the microcontroller.

## Installing and compilation

This project has been created using [MPLABX X IDE](https://www.microchip.com/mplab/mplab-x-ide) from Microchip. To compile the project and flash it into the [PIC24F Curiosity Board](https://www.microchip.com/DevelopmentTools/ProductDetails/DM240004), download **nbproject** and **src** into the same folder. Then MPLAB X IDE will detect this as a MPLAB project and set it as an active project. From there, there is two configurations available:
- *PIC24FJ256GA705_SIM*: This is the configuration for the simulation of the project. It has the same setting as the configuration for the target. Use this to simulate the project in your computer without the actual hardware.
- *PIC24FJ256GA705_TARGET*: This is the configuration for the target. It has been set to work with the PIC24F Curiosity Board, that is, for the use of PKOB Starter Kit Debugger. You can use another flasher or debugger to work with. This is the one that I have used.

## Specifications

The system aims to generate a VGA video signal but the microcontroller used can only be set to run at 16 MIPS (32 MHz). With that limitation on mind, the system has the following specifications:
- 150 x 96 pixel resolution
- [3-bit RGB palette](https://en.wikipedia.org/wiki/List_of_monochrome_and_RGB_palettes#3-bit_RGB)
- Only ~8% of CPU time available, as the rest of the time is used to generate the signal.
- Certain level of asynchronism, as the lowest frequency VGA signal is spected to have 25.175 MHz as pixel frecuency. However, this can be compensated if the TV Monitor used is modern.

In spite all these limitations, **an implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)** has been achieved.
The project has the *user.c* and *user.h* files to be used by the user to define the function that he wants. Currently it implements the game [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) as a demonstration. However this can be changed at discretion.

## Auxiliary tools

The project includes a python script with functions that can be used to generate a header file for the definition pixel C array. In order to use them the following dependencies needs to be installed:

- Python3
- PIL

Help can be found invoking the following command:

```bash
> python3 img_dith.py -h
usage: img_dith.py [-h] [-o O] [-width WIDTH] [-height HEIGHT] [-bits BITS]
                   [-header HEADER]
                   input

Process dithering to picture

positional arguments:
  input           input picture

optional arguments:
  -h, --help      show this help message and exit
  -o O            output picture
  -width WIDTH    width of the pic
  -height HEIGHT  heigth of the pic
  -bits BITS      bit depth of the pic
  -header HEADER  output header
```
    
Take into account that the script as is will process the image using the [Flos-Steinberg dithering](https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering) algorithm. In order to disable the dithering, modify the following line in *img_dith.py*:

```python
# Process the image. Resize, Floyd–Steinberg dithering or Posterize
img = Image.open(in_img)
img = resize_wo_deform(img,(w,h))
#img = posterize(img, bit_depth) <-- Uncomment this line
img = fs_dithering(img,bit_depth) #<-- Comment this line
```

## PCB VGA Adaptor

The project requires to manufacture an VGA adaptor for the PIC24F Curiosity Board. The signals are mapped as following:

| uC Pin   | VGA Signal    |  DB15 Pin |
|----------|---------------|-----------|
| RA0      | RED           | 1         |
| RA1      | GREEN         | 2         |
| RA2      | BLUE          | 3         |
| RB7      | H_SYNC        | 13        |
| RB9      | V_SYNC        | 14        |

For the proposed design go to the **PCG_Design** folder. This PCB can be plug into the PIC24F Curiosity board.
