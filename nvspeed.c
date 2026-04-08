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
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#include "macros.h"

#define LEN(X)       (sizeof(X) / sizeof(X[0]))
#define S_LITERAL(s) s, S_LEN(s)
#define S_LEN(s)     (sizeof(s) - 1)

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

const unsigned char *nv_temptospeed = FAN_CURVE_DEFAULT;
/* Global variables */
typedef struct {
	nvmlDevice_t device;
	unsigned int num_fans;
	unsigned int speed_last;
} nv_ty;
static nv_ty *nv;
static unsigned int nv_device_count;
static int nv_inited;
static nvmlReturn_t nv_ret = NVML_SUCCESS;

static void
nv_cleanup()
{
	if (nv_inited) {
		if (nv) {
			/* Restore fan control policy. */
			for (unsigned int i = 0; i < nv_device_count; ++i)
				for (unsigned int j = 0; j < nv[i].num_fans; ++j) {
					nv_ret = nvmlDeviceSetFanControlPolicy(nv[i].device, i, NVML_FAN_POLICY_TEMPERATURE_CONTINOUS_SW);
					if (unlikely(nv_ret != NVML_SUCCESS))
						DIE(nv_ret);
				}
		}
		nvmlShutdown();
	}
	free(nv);
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

static int
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
	nv = calloc(nv_device_count, sizeof(*nv));
	if (nv == NULL)
		DIE_GRACEFUL(nv_ret);
	for (unsigned int i = 0; i < nv_device_count; ++i) {
		nv_ret = nvmlDeviceGetHandleByIndex(i, &nv[i].device);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
		unsigned int min;
		unsigned int max;
		nv_ret = nvmlDeviceGetMinMaxFanSpeed(nv[i].device, &min, &max);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
		printf("Min speed for GPU%d: %d\n", i, min);
		printf("Max speed for GPU%d: %d\n", i, max);
		nv_ret = nvmlDeviceGetFanSpeed(nv[i].device, &(nv[i].speed_last));
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
		/* Avoid underflow */
		if (unlikely(STEPDOWN_MAX > nv_temptospeed[0])) {
			fprintf(stderr, "STEPDOWN_MAX (%d) is greater than the minimum fan speed (%d).\n", STEPDOWN_MAX, nv_temptospeed[0]);
			DIE_GRACEFUL(nv_ret);
		}
		nv_ret = nvmlDeviceGetNumFans(nv[i].device, &nv[i].num_fans);
		if (nv_ret != NVML_SUCCESS)
			DIE_GRACEFUL(nv_ret);
	}
	return 0;
}

static ATTR_INLINE unsigned int
nv_step(unsigned int speed, unsigned int last_speed)
{
	/* Ramp down slower, STEPDOWN_MAX per update at maximum. */
	return (speed > last_speed - STEPDOWN_MAX) ? speed : (last_speed - STEPDOWN_MAX);
}

static int
nv_mainloop(void)
{
	unsigned int speed;
	unsigned int temp;
	for (;;) {
		for (unsigned int i = 0; i < nv_device_count; ++i) {
			nv_ret = nv_nvmlDeviceGetTemperature(nv[i].device, NVML_TEMPERATURE_GPU, &temp);
			if (unlikely(nv_ret != NVML_SUCCESS))
				DIE_GRACEFUL(nv_ret);
			DBG(fprintf(stderr, "%s:%d:%s: getting temp for GPU%d: %d.\n", __FILE__, __LINE__, ASSERT_FUNC, i, temp));
			speed = nv_temptospeed[temp];
			DBG(fprintf(stderr, "%s:%d:%s: getting speed for GPU%d: %d.\n", __FILE__, __LINE__, ASSERT_FUNC, i, speed));
			speed = nv_step(speed, nv[i].speed_last);
			/* Avoid updating if speed has not changed. */
			if (speed == nv[i].speed_last)
				continue;
			nv[i].speed_last = speed;
			for (unsigned int j = 0; j < nv[i].num_fans; ++j) {
				DBG(fprintf(stderr, "%s:%d:%s: setting speed %d to fan%d of GPU%d.\n", __FILE__, __LINE__, ASSERT_FUNC, speed, j, i));
				nv_ret = nvmlDeviceSetFanSpeed_v2(nv[i].device, j, (unsigned int)speed);
				if (unlikely(nv_ret != NVML_SUCCESS))
					DIE_GRACEFUL(nv_ret);
			}
		}
		if (unlikely(sleep(INTERVAL)))
			DIE_GRACEFUL(nv_ret);
	}
	return 0;
}

static int
nv_puts_len(const char *filename, const char *buf, unsigned int len)
{
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (unlikely(fd == -1))
		return -1;
	int write_sz = write(fd, buf, len);
	if (unlikely(write_sz == -1)) {
		close(fd);
		return -1;
	}
	if (unlikely(close(fd) == -1))
		return -1;
	return 0;
}

static void
nv_mode_setup()
{
	if (mkdir(NVSPEED_PATH, 0777) != 0)
		assert(errno == EEXIST);
	if (unlikely(nv_puts_len(NVSPEED_PATH "/" NVSPEED_FILE_LOCK, "", 0) == -1)) {
		fprintf(stderr, "nvspeed: another instance is already running.\n");
		exit(EXIT_FAILURE);
	}
	if (nv_temptospeed == nv_table_temptospeed_med) {
		if (unlikely(nv_puts_len(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, S_LITERAL("medium\n")) == -1)) {
			fprintf(stderr, "nvspeed: can't write to %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
			exit(EXIT_FAILURE);
		}
	} else if (nv_temptospeed == nv_table_temptospeed_high) {
		if (unlikely(nv_puts_len(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, S_LITERAL("high\n")) == -1)) {
			fprintf(stderr, "nvspeed: can't write to %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
			exit(EXIT_FAILURE);
		}
	}
	if (unlikely(chmod(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1)) {
		fprintf(stderr, "nvspeed: can't chmod %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
		exit(EXIT_FAILURE);
	}
}

#define _(x) x

const char *usage = _("Usage: nvspeed [OPTIONS]...\n")
                    _("Options:\n")
                    _("  --medium\n")
                    _("    Medium fan speed.\n")
                    _("  --high\n")
                    _("    High fan speed.\n");

int
main(int argc, char **argv)
{
	if (argc == 2) {
		if (!strcmp(argv[1], "--medium")) {
			printf("nvspeed: using medium fan speed.\n");
			nv_temptospeed = nv_table_temptospeed_med;
		} else if (!strcmp(argv[1], "--high")) {
			printf("nvspeed: using high fan speed.\n");
			nv_temptospeed = nv_table_temptospeed_high;
		} else {
			fprintf(stderr, "%s", usage);
			exit(EXIT_FAILURE);
		}
	}
	nv_mode_setup();
	if (unlikely(nv_init() != 0))
		DIE_GRACEFUL(nv_ret);
	if (unlikely(nv_mainloop() != 0))
		DIE_GRACEFUL(nv_ret);
	return EXIT_SUCCESS;
}
