#include "config.h"

/* See /path/to/nvml/example/example.c for more examples. */

#include NVML_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

INLINE
static int
loop(void)
{
	nvmlReturn_t ret = nvmlInit();
	if (ret != NVML_SUCCESS)
		goto exit;
	unsigned int num_dev;
	ret = nvmlDeviceGetCount(&num_dev);
	if (ret != NVML_SUCCESS)
		goto shutdown;
	nvmlDevice_t *const dev = malloc(num_dev * sizeof(nvmlDevice_t));
	if (dev == NULL)
		goto shutdown;
	int *const tmp = malloc(num_dev * sizeof(int));
	if (tmp == NULL)
		goto shutdown_free_dev;
	memset(tmp, MIN_TEMP, num_dev * sizeof(int));
	int *const temp = malloc(num_dev * sizeof(int));
	if (temp == NULL)
		goto shutdown_free_tmp;
	for (unsigned int i = 0; i < num_dev; ++i)
		if ((ret = nvmlDeviceGetHandleByIndex(i, dev + i)) != NVML_SUCCESS)
			goto cleanup;
	unsigned int speed;
	for (;;) {
		for (unsigned int i = 0; i < num_dev; ++i) {
			ret = nvmlDeviceGetTemperature(dev[i], NVML_TEMPERATURE_GPU, (unsigned int *)temp + i);
			if (unlikely(ret != NVML_SUCCESS))
				goto cleanup;
			if (abs(tmp[i] - temp[i]) > MIN_TEMP_DIFF) {
				switch (temp[i]) {
					CASE_TEMP_SPEED(speed);
				}
				ret = nvmlDeviceSetFanSpeed_v2(dev[i], i, speed);
				if (unlikely(ret != NVML_SUCCESS))
					goto cleanup;
				tmp[i] = temp[i];
			}
		}
		if (unlikely(sleep(1)))
			goto cleanup;
	}
	free(dev);
	free(tmp);
	free(temp);
	ret = nvmlShutdown();
	if (ret != NVML_SUCCESS)
		goto exit;
	return EXIT_SUCCESS;
cleanup:
	free(temp);
shutdown_free_tmp:
	free(tmp);
shutdown_free_dev:
	free(dev);
shutdown:
	fputs(nvmlErrorString(ret), stderr);
	nvmlShutdown();
exit:
	perror("");
	return EXIT_FAILURE;
}

int
main(void)
{
	loop();
}
