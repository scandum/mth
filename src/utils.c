#include "mud.h"

int cat_sprintf(char *dest, char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	int size;
	va_list args;

	va_start(args, fmt);
	size = vsprintf(buf, fmt, args);
	va_end(args);

	strcat(dest, buf);

	return size;
}

void arachnos_devel(char *fmt, ...)
{
	// mud specific broadcast goes here
}

void arachnos_mudlist(char *fmt, ...)
{
	// mud specific mudlist handler goes here
}
