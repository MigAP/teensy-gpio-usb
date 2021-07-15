# Teensy GPIO control over USB 

This project allows to control a total of 16 GPIO by USB. It can
simultaneously read and write up to 8 predefined Teensy GPIO using the
low level GPIO Teensy registers:

- `GPIOX_PDOR`: for writing the GPIO. 
- `GPIOX_PDIR`: for reading the GPIO. 

The current implementation uses the first 8 bits of the `GPIOD`
register for writing, and the first 8 bits of the `GPIOC` for
reading.

## Map between the physical pins and register numbers 

The relationship between the physical pins and register numbers is
available on the Teensy schematic (see the `docs` directory).

Register Number | Register | Pin Number (physical) 
----------------|----------|-----------------------
0 | C |15 
1 | C |22 
2 | C |23 
3 | C | 9 
4 | C |10 
5 | C |13 
6 | C |11 
7 | C |12 
--|---|--
0 | D | 2 
1 | D |14 
2 | D | 7 
3 | D | 8 
4 | D | 6 
5 | D |20 
6 | D |21 
7 | D | 5 

## Useful links 

- Bit-wise operations in C: https://www.pjrc.com/teensy/pins.html
- Low-level register for I/O: https://forum.pjrc.com/archive/index.php/t-17532.html
