#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define table_len sizeof(nv_table_temptospeed_med) 
#define FAN_CURVE_MEDIUM nv_table_temptospeed_med
#define FAN_CURVE_HIGH nv_table_temptospeed_high

static void
print_table(const unsigned char *table)
{
	for (unsigned int i = 0; i < table_len; ++i) {
		const char *space;
		if (i <= 9)
			space = "  ";
		else if (i <= 99)
			space = " ";
		else
			space = "";
		printf("%s%dc ", space, i);
		unsigned int speed = table[i];
		for (unsigned int j = 0; j <= 100; ++j) {
			if (j == speed) {
				printf("x %u%%", speed);
				if (speed <= 9)
					j += 3;
				else if (speed <= 99)
					j += 4;
				else
					j += 5;
			} else {
				putchar('-');
			}
		}
		putchar('\n');
	}
	printf("     ");
	for (unsigned int i = 0; i <= 100; ++i) {
		if (i % 5 == 0) {
			const char *space;
			if (i <= 9)
				space = "   ";
			else if (i <= 99)
				space = "  ";
			else
				space = " ";
			printf("%u%s ", i, space);
			if (i <= 9)
				i += 3;
			else if (i <= 99)
				i += 4;
			else
				i += 5;
		}
	}
	putchar('\n');
}

static const unsigned char *
find_curve(void)
{
	int fd = open(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "nvspeed: can't open %s. Is the daemon running?\n",
		        NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
		exit(EXIT_FAILURE);
	}
	char curve[256] = {0};
	int read_sz = read(fd, curve, sizeof(curve) - 1);
	if (read_sz < 0) {
		fprintf(stderr, "nvspeed: error reading %s.\n",
		        NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
		exit(EXIT_FAILURE);
	}
	if (close(fd) == -1) {
		fprintf(stderr, "nvspeed: error closing %s.\n",
		        NVSPEED_PATH "/" NVSPEED_FILE_CURVE);
		exit(EXIT_FAILURE);
	}
	char *p = strchr(curve, '\n');
	if (p)
		*p = '\0';
	const unsigned char *table = FAN_CURVE_DEFAULT;
	if (!strcmp(curve, "medium"))
		table = FAN_CURVE_MEDIUM;
	else if (!strcmp(curve, "high"))
		table = FAN_CURVE_HIGH;
	return table;
}

int
main()
{
	print_table(find_curve());
	return 0;
}
