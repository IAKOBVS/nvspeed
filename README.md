# nvspeed

## Dependencies:

- NVML: <https://developer.nvidia.com/nvidia-management-library-nvml><br>
You should have it if you have installed CUDA.

## Configuration:

Temperature, fan speed, delay, and minimum temperature difference can be configured in ./config.h.

## Installation:

Make sure that NVML\_HEADER in config.h and NVML\_LIB in ./build point to the correct paths.<br>

```
./build
```

## Usage:

```
sudo ./nvspeed
```
