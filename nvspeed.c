/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 James Tirta Halim <tirtajames45 at gmail dot com>
 * This file is part of nvspeed.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

/* TODO: use graphs to generate fan speed */
#include "config.h"

#include NVML_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include "macros.h"

#ifdef DEBUG
#	define D(x) (x)
#	define D_PRINTF(fmt, x) \
		printf(fmt, x)
#else
#	define D(x)
#	define D_PRINTF(fmt, x)
#endif

#define ERR(nv_ret)             \
	do {                    \
		nv_die(nv_ret); \
		assert(0);      \
	} while (0)

static nvmlReturn_t
nv_nvmlDeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int *temp)
{
#if USE_NVML_DEVICEGETTEMPERATUREV
	nvmlTemperature_t tmp;
	tmp.sensorType = sensorType;
	tmp.version = nvmlTemperature_v1;
	const nvmlReturn_t ret = nvmlDeviceGetTemperatureV(device, &tmp);
	*temp = (unsigned int)tmp.temperature;
	return ret;
#else
	return nvmlDeviceGetTemperature(device, sensorType, temp);
#endif
}

/* Global variables */
static nvmlDevice_t *nv_device;
static unsigned int nv_device_count;
static int nv_inited;
static nvmlReturn_t nv_ret;

typedef struct {
	unsigned int temp;
	unsigned int speed;
} nv_mon_ty;
static nv_mon_ty *nv_last;

static void
nv_cleanup()
{
	if (nv_inited)
		nvmlShutdown();
	free(nv_device);
	free(nv_last);
}

static void
nv_die(nvmlReturn_t ret)
{
	perror("\n");
	fprintf(stderr, "%s\n\n", nvmlErrorString(ret));
	nv_cleanup();
}

static int
nv_init()
{
	nv_ret = nvmlInit();
	if (nv_ret != NVML_SUCCESS)
		ERR(nv_ret);
	nv_inited = 1;
	nv_ret = nvmlDeviceGetCount(&nv_device_count);
	if (nv_ret != NVML_SUCCESS)
		ERR(nv_ret);
	nv_device = (nvmlDevice_t *)malloc(nv_device_count * sizeof(nvmlDevice_t));
	if (nv_device == NULL)
		ERR(nv_ret);
	nv_last = (nv_mon_ty *)calloc(nv_device_count, sizeof(nv_mon_ty));
	if (nv_last == NULL)
		ERR(nv_ret);
	for (unsigned int i = 0; i < nv_device_count; ++i) {
		nv_ret = nvmlDeviceGetHandleByIndex(i, nv_device + i);
		if (nv_ret != NVML_SUCCESS)
			ERR(nv_ret);
		unsigned int min;
		unsigned int max;
		nvmlDeviceGetMinMaxFanSpeed(nv_device[i], &min, &max);
		if (nv_ret != NVML_SUCCESS)
			ERR(nv_ret);
		D_PRINTF("nvspeed: min_speed: %d\n", min);
		D_PRINTF("nvspeed: max_speed: %d\n", max);
	}
	return 0;
}

static int
nv_mainloop(void)
{
	if (!nv_inited)
		nv_init();
	unsigned int speed = 0;
	unsigned int temp;
	for (;;) {
		for (unsigned int i = 0; i < nv_device_count; ++i) {
			nv_ret = nv_nvmlDeviceGetTemperature(nv_device[i], NVML_TEMPERATURE_GPU, &temp);
			if (unlikely(nv_ret != NVML_SUCCESS))
				ERR(nv_ret);
			/* */
			D_PRINTF("nvspeed: temp:%d.\n\n", temp);
			D_PRINTF("nvspeed: last temp:%d.\n\n", nv_last[i].temp);
			/* Avoid updating if temperature has not changed. */
			if (temp == nv_last[i].temp)
				continue;
			nv_last[i].temp = temp;
			speed = table_percent[temp];
			/* */
			D(fprintf(stderr, "nvspeed: temp:%d speed;%d.\n\n", temp, speed));
			/* Avoid updating if speed has not changed. */
			if (speed == nv_last[i].speed)
				continue;
			nv_last[i].speed = speed;
			nv_ret = nvmlDeviceSetFanSpeed_v2(nv_device[i], i, (unsigned int)speed);
			if (unlikely(nv_ret != NVML_SUCCESS))
				ERR(nv_ret);
			/* */
			D_PRINTF("nvspeed: setting speed:%d.\n", speed);
			D_PRINTF("nvspeed: last speed:%d.\n", nv_last[i].speed);
		}
		if (unlikely(sleep(INTERVAL)))
			ERR(nv_ret);
	}
	/* No need to cleanup, as it will exit. */
	/* nv_cleanup(); */
	return EXIT_SUCCESS;
}

int
main(void)
{
	return nv_mainloop();
}
