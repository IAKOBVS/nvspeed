#include "config.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define table_len sizeof(nv_table_temptospeed_med) 
#define FAN_CURVE_MEDIUM nv_table_temptospeed_med
#define FAN_CURVE_HIGH nv_table_temptospeed_high

void
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
		for (unsigned int j = 0; j <= 100; ++j) {
			const char *c;
			if (j == (unsigned int)(table[i])) {
				c = "x";
				printf("%s ", c);
				printf("%d%%", j);
				if (j <= 9)
					j += 3;
				else if (j <= 99)
					j += 4;
				else
					j += 5;
			} else {
				c = "-";
				printf("%s", c);
			}
		}
		putchar('\n');
	}
	printf("%s", "     ");
	for (unsigned int i = 0; i <= 100; ++i) {
		if (i % 5 == 0) {
			const char *space;
			if (i <= 9)
				space = "   ";
			else if (i <= 99)
				space = "  ";
			else
				space = " ";
			printf("%d%s ", i, space);
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

const unsigned char *find_curve()
{
	int fd = open(NVSPEED_PATH "/" NVSPEED_FILE_CURVE, O_RDONLY);
	assert(fd != -1);
	char curve[4096];
	int read_sz = read(fd, curve, sizeof(curve));
	assert(read_sz != -1);
	assert(close(fd) != -1);
	char *p = strchr(curve, '\n');
	if (p) {
		*p = '\0';
		--read_sz;
	}
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
