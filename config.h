#ifndef NVSPEED_CONFIG_H
#define NVSPEED_CONFIG_H

#define NVML_HEADER   "/opt/cuda/targets/x86_64-linux/include/nvml.h"
#define MIN_TEMP      36
#define DELAY_MS      200
#define MIN_TEMP_DIFF 2

#define TEMP_SPEED(TEMP, SPEED, VAR) \
case TEMP: VAR = SPEED; break;

#define CASE_TEMP_SPEED(SPEED)       \
	TEMP_SPEED(29, 33, SPEED);   \
	TEMP_SPEED(30, 33, SPEED);   \
	TEMP_SPEED(31, 33, SPEED);   \
	TEMP_SPEED(32, 33, SPEED);   \
	TEMP_SPEED(33, 33, SPEED);   \
	TEMP_SPEED(34, 33, SPEED);   \
	TEMP_SPEED(35, 33, SPEED);   \
	TEMP_SPEED(36, 33, SPEED);   \
	TEMP_SPEED(37, 33, SPEED);   \
	TEMP_SPEED(38, 33, SPEED);   \
	TEMP_SPEED(39, 33, SPEED);   \
	TEMP_SPEED(40, 33, SPEED);   \
	TEMP_SPEED(41, 33, SPEED);   \
	TEMP_SPEED(42, 33, SPEED);   \
	TEMP_SPEED(43, 34, SPEED);   \
	TEMP_SPEED(44, 35, SPEED);   \
	TEMP_SPEED(45, 36, SPEED);   \
	TEMP_SPEED(46, 37, SPEED);   \
	TEMP_SPEED(47, 38, SPEED);   \
	TEMP_SPEED(48, 39, SPEED);   \
	TEMP_SPEED(49, 40, SPEED);   \
	TEMP_SPEED(50, 42, SPEED);   \
	TEMP_SPEED(51, 44, SPEED);   \
	TEMP_SPEED(52, 46, SPEED);   \
	TEMP_SPEED(53, 48, SPEED);   \
	TEMP_SPEED(54, 50, SPEED);   \
	TEMP_SPEED(55, 54, SPEED);   \
	TEMP_SPEED(56, 56, SPEED);   \
	TEMP_SPEED(57, 57, SPEED);   \
	TEMP_SPEED(58, 58, SPEED);   \
	TEMP_SPEED(59, 60, SPEED);   \
	TEMP_SPEED(60, 62, SPEED);   \
	TEMP_SPEED(61, 64, SPEED);   \
	TEMP_SPEED(62, 66, SPEED);   \
	TEMP_SPEED(63, 68, SPEED);   \
	TEMP_SPEED(64, 70, SPEED);   \
	TEMP_SPEED(65, 72, SPEED);   \
	TEMP_SPEED(66, 74, SPEED);   \
	TEMP_SPEED(67, 76, SPEED);   \
	TEMP_SPEED(68, 78, SPEED);   \
	TEMP_SPEED(69, 80, SPEED);   \
	TEMP_SPEED(70, 82, SPEED);   \
	TEMP_SPEED(71, 84, SPEED);   \
	TEMP_SPEED(72, 86, SPEED);   \
	TEMP_SPEED(73, 87, SPEED);   \
	TEMP_SPEED(74, 88, SPEED);   \
	TEMP_SPEED(75, 89, SPEED);   \
	TEMP_SPEED(76, 90, SPEED);   \
	TEMP_SPEED(77, 91, SPEED);   \
	TEMP_SPEED(78, 92, SPEED);   \
	TEMP_SPEED(79, 93, SPEED);   \
	TEMP_SPEED(80, 94, SPEED);   \
	TEMP_SPEED(81, 96, SPEED);   \
	TEMP_SPEED(82, 98, SPEED);   \
	TEMP_SPEED(83, 100, SPEED);  \
	TEMP_SPEED(84, 100, SPEED);  \
	TEMP_SPEED(85, 100, SPEED);  \
	TEMP_SPEED(86, 100, SPEED);  \
	TEMP_SPEED(87, 100, SPEED);  \
	TEMP_SPEED(88, 100, SPEED);  \
	TEMP_SPEED(89, 100, SPEED);  \
	TEMP_SPEED(90, 100, SPEED);  \
	TEMP_SPEED(91, 100, SPEED);  \
	TEMP_SPEED(92, 100, SPEED);  \
	TEMP_SPEED(93, 100, SPEED);  \
	TEMP_SPEED(94, 100, SPEED);  \
	TEMP_SPEED(95, 100, SPEED);  \
	TEMP_SPEED(96, 100, SPEED);  \
	TEMP_SPEED(97, 100, SPEED);  \
	TEMP_SPEED(98, 100, SPEED);  \
	TEMP_SPEED(99, 100, SPEED);  \
	TEMP_SPEED(100, 100, SPEED); \
	TEMP_SPEED(101, 100, SPEED); \
default: SPEED = 33; break;

#endif /* NVSPEED_CONFIG_H */
