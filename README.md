# nvspeed
A minimal Nvidia GPU fan speed controller written in C.
# Building
```
make
```
# Usage
```
# requires sudo to set fan speed
sudo nvspeed
```
## Configuration
Temperature, fan speed, delay, and minimum temperature difference are configured in config.h.
## Dependencies
NVML (CUDA): for getting GPU temperature and setting fan speed
