# nvspeed
A minimal Nvidia GPU fan speed controller written in C.
# Installation
## Arch Linux
```
$ git clone https://aur.archlinux.org/packages/nvspeed-git
$ cd nvspeed-git
$ makepkg --nobuild
$ cd src/nvspeed/src/
$ make config
# Configure config.h, and the Makefile
$ cd ../../..
$ makepkg -si
```
# Building
## Dependencies
NVML (CUDA): for getting GPU temperature and setting fan speed
```
# Install dependencies
$ your-package-manager-install cuda
$ make config
# Configure config.h, and the Makefile
$ make
$ sudo make install
```
# Usage
```
# requires sudo to set fan speed
$ sudo nvspeed
```
## Configuration
Temperature and fan speed are configured in config.h.
