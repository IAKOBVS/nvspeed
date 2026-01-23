# nvspeed
Control Nvidia GPU fan speed in C
# Building
```
make
```
# Usage
```
sudo ./nvspeed
```
## Configuration
Temperature, fan speed, delay, and minimum temperature difference are configured in config.h.
## Dependencies
CUDA: for getting and setting GPU fan speed wiht NVML
