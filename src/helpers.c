#include <stdlib.h>

#include "helpers.h"

/* Get the pixel values of a named colour colstr. Returns pixel values. */
uint32_t
get_color(const char *hex)
{
	uint32_t rgb48;
	char strgroups[7] = {
		hex[1], hex[2], hex[3], hex[4], hex[5], hex[6], '\0'
	};

	rgb48 = strtol(strgroups, NULL, 16);
	return rgb48 | 0xff000000;
}
