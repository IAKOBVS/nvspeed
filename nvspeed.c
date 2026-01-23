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

#include "config.h"

#include NVML_HEADER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "macros.h"

#define ERR(nv_ret)             \
	do {                    \
		nv_die(nv_ret); \
		assert(0);      \
	} while (0)

int nv_inited;
nvmlDevice_t *nv_device;
int *nv_temp_last;
int *nv_temp;
unsigned int nv_device_num;

static void
nv_cleanup()
{
	if (nv_inited) {
		nvmlShutdown();
		free(nv_device);
		free(nv_temp_last);
		free(nv_temp);
		nv_device = NULL;
		nv_temp_last = NULL;
		nv_temp = NULL;
	}
}

static void
nv_die(nvmlReturn_t nv_ret)
{
	perror("\n");
	fprintf(stderr, "%s\n\n", nvmlErrorString(nv_ret));
	nv_cleanup();
}

static int
nv_init()
{
	nvmlReturn_t ret = nvmlInit();
	if (ret != NVML_SUCCESS)
		return -1;
	nv_inited = 1;
	unsigned int device_num;
	ret = nvmlDeviceGetCount(&device_num);
	if (ret != NVML_SUCCESS)
		ERR(ret);
	nv_device = malloc(device_num * sizeof(nvmlDevice_t));
	if (nv_device == NULL)
		ERR(ret);
	nv_temp_last = malloc(device_num * sizeof(int));
	if (nv_temp_last == NULL)
		ERR(ret);
	memset(nv_temp_last, MIN_TEMP, device_num * sizeof(int));
	nv_temp = malloc(device_num * sizeof(int));
	if (nv_temp == NULL)
		ERR(ret);
	for (unsigned int i = 0; i < nv_device_num; ++i)
		if ((ret = nvmlDeviceGetHandleByIndex(i, nv_device + i)) != NVML_SUCCESS)
			ERR(ret);
	return 0;
}

static int
nv_loop(void)
{
	if (!nv_inited)
		nv_init();
	nvmlReturn_t ret = 0;
	unsigned int speed;
	for (;;) {
		for (unsigned int i = 0; i < nv_device_num; ++i) {
			ret = nvmlDeviceGetTemperature(nv_device[i], NVML_TEMPERATURE_GPU, (unsigned int *)nv_temp + i);
			if (unlikely(ret != NVML_SUCCESS))
				ERR(ret);
			if (abs(nv_temp_last[i] - nv_temp[i]) > MIN_TEMP_DIFF) {
				switch (nv_temp[i]) {
					CASE_TEMP_SPEED(speed);
				}
				ret = nvmlDeviceSetFanSpeed_v2(nv_device[i], i, speed);
				if (unlikely(ret != NVML_SUCCESS))
					ERR(ret);
				nv_temp_last[i] = nv_temp[i];
			}
		}
		if (unlikely(sleep(INTERVAL)))
			ERR(ret);
	}
	nv_cleanup();
	return EXIT_SUCCESS;
}

int
main(void)
{
	nv_loop();
}
