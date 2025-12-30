#include "config.h"

/* See /path/to/nvml/example/example.c for more examples. */

#include NVML_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "macros.h"

#define ERR(x)                  \
	do {                    \
		xnvml_die(ret); \
		x;              \
		assert(0);      \
	} while (0)

int init = 0;
nvmlDevice_t *dev;
int *tmp_temp;
int *temp;

static void
xnvml_cleanup()
{
	if (init) {
		free(dev);
		free(tmp_temp);
		free(temp);
		dev = NULL;
		tmp_temp = NULL;
		temp = NULL;
		nvmlShutdown();
	}
}

static void
xnvml_die(nvmlReturn_t ret)
{
	perror("\n");
	fprintf(stderr, "%s\n\n", nvmlErrorString(ret));
	xnvml_cleanup();
}

INLINE
static int
loop(void)
{
	nvmlReturn_t ret = nvmlInit();
	if (ret != NVML_SUCCESS)
		return EXIT_FAILURE;
	init = 1;
	unsigned int num_dev;
	ret = nvmlDeviceGetCount(&num_dev);
	if (ret != NVML_SUCCESS)
		xnvml_die(ret);
	dev = malloc(num_dev * sizeof(nvmlDevice_t));
	if (dev == NULL)
		xnvml_die(ret);
	tmp_temp = malloc(num_dev * sizeof(int));
	if (tmp_temp == NULL)
		xnvml_die(ret);
	memset(tmp_temp, MIN_TEMP, num_dev * sizeof(int));
	temp = malloc(num_dev * sizeof(unsigned int));
	if (temp == NULL)
		xnvml_die(ret);
	for (unsigned int i = 0; i < num_dev; ++i)
		if ((ret = nvmlDeviceGetHandleByIndex(i, dev + i)) != NVML_SUCCESS)
			xnvml_die(ret);
	unsigned int speed;
	for (;;) {
		for (unsigned int i = 0; i < num_dev; ++i) {
			ret = nvmlDeviceGetTemperature(dev[i], NVML_TEMPERATURE_GPU, (unsigned int *)temp + i);
			if (unlikely(ret != NVML_SUCCESS))
				xnvml_die(ret);
			if (abs(tmp_temp[i] - temp[i]) > MIN_TEMP_DIFF) {
				switch (temp[i]) {
					CASE_TEMP_SPEED(speed);
				}
				ret = nvmlDeviceSetFanSpeed_v2(dev[i], i, speed);
				if (unlikely(ret != NVML_SUCCESS))
					xnvml_die(ret);
				tmp_temp[i] = temp[i];
			}
		}
		if (unlikely(sleep(1)))
			xnvml_die(ret);
	}
	xnvml_cleanup();
	return EXIT_SUCCESS;
}

int
main(void)
{
	loop();
}
