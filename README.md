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
CUDA: for getting GPU temperature and setting fan speed with NVML
```
# Install dependencies
$ your-package-manager-install cuda
$ make config
# Configure config.h, and the Makefile
$ make
$ sudo make install
```
# Usage
Sudo is required to set fan speed for NVML.
```
$ sudo nvspeed
```
## Configuration
Temperature and fan speed are configured in config.h.

### Temperature file (`PRINT_TEMP`)
The daemon writes the current GPU temperature (max across all devices) to
`/tmp/nvspeed/temp` every `INTERVAL` seconds in **millidegrees Celsius**
(e.g. `45000\n` for 45 °C), mimicking the Linux hwmon sysfs interface.
This lets external tools read the temperature without calling NVML directly.

To disable, set `#define PRINT_TEMP 0` in `config.h` and rebuild.
