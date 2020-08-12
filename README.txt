# device-driver-simulation
Accepts read/write requests from a file system, translates them from physical block numbers into disk drive cylinder, track, and sector numbers, then instructs a disk device to carry out the read and write requests.

This program can only be run on linux.
To run this program download the driver file to your linux machine.
Open the terminal and change directory into the driver file and then execute the following 3 commands:
  gcc -Wall -ansi -c driver.c 
  gcc -odriver driver.o filesys.o disk.o -lm
  ./driver

Note: Both compiled object files were given to me to simulate an actual file system and the physical disk. The driver.c file is the code that I wrote to translate between the two compiled object files. 