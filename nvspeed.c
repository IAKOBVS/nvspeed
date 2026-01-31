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
#include <errno.h>
#include <signal.h>

#include "macros.h"

typedef enum {
	NV_RET_SUCC = 0,
	NV_RET_ERR
} nv_ret_ty;

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
static unsigned int *nv_num_fans;
static unsigned int nv_device_count;
static int nv_inited;
static nvmlReturn_t nv_ret = NVML_SUCCESS;

static unsigned int *nv_speed_last;

static void
nv_cleanup()
{
	if (nv_inited) {
		/* Restore fan control policy. */
		if (nv_num_fans) {
			setbuf(stdout, NULL);
			for (unsigned int i = 0; i < nv_device_count; ++i)
				for (unsigned int j = 0; j < nv_num_fans[i]; ++j) {
					printf("Setting speed to fan%d of GPU%d to default.\n", j, i);
					nv_ret = nvmlDeviceSetDefaultFanSpeed_v2(nv_device[i], j);
					if (unlikely(nv_ret != NVML_SUCCESS))
						DIE(nv_ret);
				}
		}
		nvmlShutdown();
	}
	free(nv_device);
	free(nv_speed_last);
}

static void
nv_exit(nvmlReturn_t ret)
{
	if (errno)
		perror("");
	fprintf(stderr, "nvspeed: %s\n", nvmlErrorString(ret));
	nv_cleanup();
	_Exit(ret == NVML_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
nv_sig_handler(int signum)
{
	nv_cleanup();
	_Exit(EXIT_SUCCESS);
	(void)signum;
}

static void
nv_sig_setup()
{
	if (unlikely(signal(SIGTERM, nv_sig_handler) == SIG_ERR))
		DIE(nv_ret);
	if (unlikely(signal(SIGINT, nv_sig_handler) == SIG_ERR))
		DIE(nv_ret);
}

static nv_ret_ty
nv_init()
{
	nv_sig_setup();
	nv_ret = nvmlInit();
	if (nv_ret != NVML_SUCCESS)
		DIE_GRACEFUL(nv_ret);
	nv_inited = 1;
	nv_ret = nvmlDeviceGetCount(&nv_device_count);
	if (nv_ret != NVML_SUCCESS)
		DIE_GRACEFUL(nv_ret);
	nv_device = (nvmlDevice_t *)calloc(nv_device_count, sizeof(nvmlDevice_t));
	if (nv_device == NULL)
		DIE_GRACEFUL(nv_ret);
	nv_speed_last = (unsigned int *)calloc(nv_device_count, sizeof(unsigned int));
	if (nv_speed_last == NULL)
		DIE_GRACEFUL(nv_ret);
	nv_num_fans = (unsigned int *)calloc(nv_device_count, sizeof(unsigned int));
	if (nv_num_fans == NULL)
		DIE_GRACEFUL(nv_ret);
	for (unsigned int i = 0; i < nv_device_count; ++i) {
		nv_ret = nvmlDeviceGetHandleByIndex(i, nv_device + i);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
		unsigned int min;
		unsigned int max;
		nv_ret = nvmlDeviceGetMinMaxFanSpeed(nv_device[i], &min, &max);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
		printf("Min speed for GPU%d: %d\n", i, min);
		printf("Max speed for GPU%d: %d\n", i, max);
		nv_ret = nvmlDeviceGetFanSpeed(nv_device[i], nv_speed_last + i);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
		/* Avoid underflow */
		if (unlikely(STEPDOWN_MAX > table_percent[0])) {
			fprintf(stderr, "STEPDOWN_MAX (%d) is greater than the minimum fan speed (%d).\n", STEPDOWN_MAX, table_percent[0]);
			DIE_GRACEFUL(nv_ret);
		}
		nv_ret = nvmlDeviceGetNumFans(nv_device[i], nv_num_fans + i);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
	}
	return NV_RET_SUCC;
}

static ATTR_INLINE unsigned int
nv_step(unsigned int speed, unsigned int last_speed)
{
	/* Ramp down slower, STEPDOWN_MAX per update at maximum. */
	return (speed > last_speed - STEPDOWN_MAX) ? speed : (last_speed - STEPDOWN_MAX);
}

static nv_ret_ty
nv_mainloop(void)
{
	unsigned int speed;
	unsigned int temp;
	for (;;) {
		for (unsigned int i = 0; i < nv_device_count; ++i) {
			nv_ret = nv_nvmlDeviceGetTemperature(nv_device[i], NVML_TEMPERATURE_GPU, &temp);
			if (unlikely(nv_ret != NVML_SUCCESS))
				DIE_GRACEFUL(nv_ret);
			DBG(fprintf(stderr, "%s:%d:%s: getting temp: %d.\n", temp, __FILE__, __LINE__, ASSERT_FUNC));
			speed = table_percent[temp];
			DBG(fprintf(stderr, "%s:%d:%s: getting speed: %d.\n", speed, __FILE__, __LINE__, ASSERT_FUNC));
			/* Avoid updating if speed has not changed. */
			if (speed == nv_speed_last[i])
				continue;
			speed = nv_step(speed, nv_speed_last[i]);
			nv_speed_last[i] = speed;
			for (unsigned int j = 0; j < nv_num_fans[i]; ++j) {
				DBG(fprintf(stderr, "%s:%d:%s: setting speed %d to fan%d of GPU%d.\n", speed, j, i, __FILE__, __LINE__, ASSERT_FUNC));
				nv_ret = nvmlDeviceSetFanSpeed_v2(nv_device[i], j, (unsigned int)speed);
				if (unlikely(nv_ret != NVML_SUCCESS))
					DIE_GRACEFUL(nv_ret);
			}
		}
		if (unlikely(sleep(INTERVAL)))
			DIE_GRACEFUL(nv_ret);
	}
	return NV_RET_SUCC;
}

int
main(void)
{
	if (unlikely(nv_init() != NV_RET_SUCC))
		DIE_GRACEFUL(nv_ret);
	if (unlikely(nv_mainloop() != NV_RET_SUCC))
		DIE_GRACEFUL(nv_ret);
	return EXIT_SUCCESS;
}
