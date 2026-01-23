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

#ifdef DEBUG
#	define D_PRINTF(fmt, x) \
		printf(fmt, x)
#else
#	define D_PRINTF(fmt, x)
#endif

#define ERR(nv_ret)             \
	do {                    \
		nv_die(nv_ret); \
		assert(0);      \
	} while (0)

/* Global variables */
nvmlDevice_t *nv_device;
int *nv_temp_last;
int *nv_temp;
unsigned int nv_device_count;
int nv_inited;
nvmlReturn_t nv_ret;
int nv_last_speed;

static void
nv_cleanup()
{
	if (nv_inited) {
		nvmlShutdown();
	}
	free(nv_device);
	free(nv_temp_last);
	free(nv_temp);
	nv_device = NULL;
	nv_temp_last = NULL;
	nv_temp = NULL;
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
	nvmlReturn_t nv_ret = nvmlInit();
	if (nv_ret != NVML_SUCCESS)
		ERR(nv_ret);
	nv_inited = 1;
	nv_ret = nvmlDeviceGetCount(&nv_device_count);
	if (nv_ret != NVML_SUCCESS)
		ERR(nv_ret);
	nv_device = malloc(nv_device_count * sizeof(nvmlDevice_t));
	if (nv_device == NULL)
		ERR(nv_ret);
	nv_temp_last = calloc(nv_device_count, sizeof(int));
	if (nv_temp_last == NULL)
		ERR(nv_ret);
	memset(nv_temp_last, MIN_TEMP, nv_device_count * sizeof(int));
	nv_temp = malloc(nv_device_count * sizeof(int));
	if (nv_temp == NULL)
		ERR(nv_ret);
	for (unsigned int i = 0; i < nv_device_count; ++i) {
		nv_ret = nvmlDeviceGetHandleByIndex(i, nv_device + i);
		if (nv_ret != NVML_SUCCESS)
			ERR(nv_ret);
	}
	return 0;
}

static int
nv_mainloop(void)
{
	if (!nv_inited)
		nv_init();
	int speed;
	for (;;) {
		for (unsigned int i = 0; i < nv_device_count; ++i) {
			nv_ret = nvmlDeviceGetTemperature(nv_device[i], NVML_TEMPERATURE_GPU, (unsigned int *)nv_temp + i);
			if (unlikely(nv_ret != NVML_SUCCESS))
				ERR(nv_ret);
			D_PRINTF("temp:%d\n", nv_temp[i]);
			if (abs(nv_temp_last[i] - nv_temp[i]) > MIN_TEMP_DIFF) {
				switch (nv_temp[i]) {
					CASE_TEMP_SPEED(speed);
				}
				D_PRINTF("speed:%d\n", speed);
				if (abs(speed - nv_last_speed) > MIN_SPEED_DIFF) {
					nv_ret = nvmlDeviceSetFanSpeed_v2(nv_device[i], i, (unsigned int)speed);
					if (unlikely(nv_ret != NVML_SUCCESS))
						ERR(nv_ret);
				}
				nv_temp_last[i] = nv_temp[i];
			}
		}
		if (unlikely(sleep(INTERVAL)))
			ERR(nv_ret);
	}
	nv_cleanup();
	return EXIT_SUCCESS;
}

int
main(void)
{
	nv_mainloop();
}
