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
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#include "macros.h"

static struct timespec nv_ts = { .tv_sec = INTERVAL, .tv_nsec = 0 };

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

static const unsigned char *nv_temptospeed = FAN_CURVE_DEFAULT;
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
static volatile sig_atomic_t nv_quit;

static void
nv_mode_cleanup()
{
	(void)unlink(NVSPEED_PATH "/" NVSPEED_FILE_TEMP);
	(void)unlink(NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
	(void)unlink(NVSPEED_PATH "/" NVSPEED_FILE_LOCK);
	(void)rmdir(NVSPEED_PATH);
}

static void
nv_cleanup()
{
	if (nv_inited) {
		if (nv) {
			/* Restore fan control policy. */
			for (unsigned int i = 0; i < nv_device_count; ++i)
				for (unsigned int j = 0; j < nv[i].num_fans; ++j) {
					nv_ret = nvmlDeviceSetFanControlPolicy(nv[i].device, j, NVML_FAN_POLICY_TEMPERATURE_CONTINOUS_SW);
					if (unlikely(nv_ret != NVML_SUCCESS))
						DIE(nv_ret);
				}
		}
		nvmlShutdown();
	}
	nv_mode_cleanup();
	free(nv);
}

static void
nv_exit(nvmlReturn_t ret)
{
	if (errno)
		perror("nvspeed");
	fprintf(stderr, "nvspeed: %s\n", nvmlErrorString(ret));
	nv_cleanup();
	_Exit(ret == NVML_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
nv_sig_handler(int signum)
{
	nv_quit = 1;
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
		DBG(fprintf(stderr, "Min speed for GPU%d: %d\n", i, min));
		DBG(fprintf(stderr, "Max speed for GPU%d: %d\n", i, max));
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
	unsigned int low = (last_speed > STEPDOWN_MAX) ? last_speed - STEPDOWN_MAX : 0;
	return (speed > low) ? speed : low;
}

#if PRINT_TEMP
static int global_fd_temp = -1;
#endif
static unsigned int global_temp_old_sz = 0;

static ATTR_INLINE char *
c_utoa_lt3_p(unsigned int num, char *buf)
{
	if (likely((unsigned int)(num - 10) < 90)) {
		*(buf + 0) = (num / 10) + '0';
		*(buf + 1) = (num % 10) + '0';
		return buf + 2;
	}
	if (num > 99) {
		*(buf + 0) = (num / 100) + '0';
		*(buf + 1) = ((num / 10) % 10) + '0';
		*(buf + 2) = (num % 10) + '0';
		return buf + 3;
	}
	*(buf + 0) = num + '0';
	return buf + 1;
}

static int
nv_temp_init(void)
{
	int fd = open(NVSPEED_PATH "/" NVSPEED_FILE_TEMP, O_CREAT | O_WRONLY);
	if (unlikely(fd < 0)) {
		DIE_GRACEFUL(nv_ret);
		return -1;
	}
	if (unlikely(chmod(NVSPEED_PATH "/" NVSPEED_FILE_TEMP, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1)) {
		fprintf(stderr, "nvspeed: can't chmod %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_TEMP);
		exit(EXIT_FAILURE);
	}
	return fd;
}

static int
nv_temp_write(int fd, unsigned int temp)
{
	char buf[8];
	unsigned int size = c_utoa_lt3_p(temp, buf) - buf;
	/* Print milidegrees, like sysfs. */
	buf[size] = '0';
	++size;
	buf[size] = '0';
	++size;
	buf[size] = '0';
	++size;
	buf[size] = '\n';
	++size;
	buf[size] = '\0';
	if (unlikely(size != global_temp_old_sz)) {
		global_temp_old_sz = size;
		ftruncate(fd, size);
	}
	if (unlikely(pwrite(fd, buf, size, 0) < 0)) {
		DIE_GRACEFUL(nv_ret);
		return -1;
	}
	return 0;
}

static void
nv_mainloop(void)
{
#if PRINT_TEMP
	global_fd_temp = nv_temp_init();
#endif
	unsigned int speed;
	unsigned int temp;
	for (; !nv_quit; ) {
		unsigned int max = 0;
		for (unsigned int i = 0; i < nv_device_count; ++i) {
			nv_ret = nv_nvmlDeviceGetTemperature(nv[i].device, NVML_TEMPERATURE_GPU, &temp);
			if (unlikely(nv_ret != NVML_SUCCESS))
				DIE_GRACEFUL(nv_ret);
			if (temp > max)
				max = temp;
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
#if PRINT_TEMP
		/* Write to tmpfs. */
		nv_temp_write(global_fd_temp, max);
#endif
		while (nanosleep(&nv_ts, &nv_ts) == -1 && errno == EINTR)
			if (nv_quit)
				break;
	}
}

static int
nv_puts_len(const char *filename, int oflag, const char *buf, unsigned int len)
{
	int fd = open(filename, O_WRONLY | oflag);
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
	if (mkdir(NVSPEED_PATH, 0777) != 0 && errno != EEXIST) {
		fprintf(stderr, "nvspeed: can't mkdir %s: ", NVSPEED_PATH);
		perror("");
		exit(EXIT_FAILURE);
	}
	if (unlikely(nv_puts_len(NVSPEED_PATH "/" NVSPEED_FILE_LOCK, O_CREAT | O_EXCL, "", 0) == -1)) {
		fprintf(stderr, "nvspeed: another instance is already running.\n");
		exit(EXIT_FAILURE);
	}
	if (nv_temptospeed == nv_table_temptospeed_med) {
		if (unlikely(nv_puts_len(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, O_CREAT | O_EXCL, S_LITERAL("medium\n")) == -1)) {
			fprintf(stderr, "nvspeed: can't write to %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
			exit(EXIT_FAILURE);
		}
	} else if (nv_temptospeed == nv_table_temptospeed_high) {
		if (unlikely(nv_puts_len(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, O_CREAT | O_EXCL, S_LITERAL("high\n")) == -1)) {
			fprintf(stderr, "nvspeed: can't write to %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
			exit(EXIT_FAILURE);
		}
	}
	if (unlikely(chmod(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1)) {
		fprintf(stderr, "nvspeed: can't chmod %s.\n", NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
		exit(EXIT_FAILURE);
	}
}

static const char *usage = "Usage: nvspeed [OPTIONS]...\n"
                           "Options:\n"
                           "  --medium\n"
                           "    Medium fan speed.\n"
                           "  --high\n"
                           "    High fan speed.\n";

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
	nv_mainloop();
	nv_cleanup();
	return EXIT_SUCCESS;
}
