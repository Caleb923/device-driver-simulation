# device-driver-simulation
Accepts read/write requests from a file system, translates them from physical block numbers into disk drive cylinder, track, and sector numbers, then instructs a disk device to carry out the read and write requests.

## Installation
This program can only be run on linux.
Downloading the C file and make sure you have the build essential packages by running:
```bash
sudo apt-get install build-essential
```

## Usage

To run this program download the driver file to your linux machine.
Open the terminal and change directory into the driver file and then execute the following 3 commands:
```bash
gcc -Wall -ansi -c driver.c
```
```bash
gcc -odriver driver.o filesys.o disk.o -lm
```
```bash
./driver
```

Note: Both compiled object files were given to me to simulate an actual file system and the physical disk. The driver.c file is the code that I wrote to translate between the two compiled object files. 
