#include <stdio.h>
#include <stdarg.h>

char *msprintf(char *format, ...)
{
	va_list args;
	char *output;
	int n, len;

	va_start(args, format);
	output = (char *)malloc(128);
	len = 127;
	while ((n = vsnprintf(output, len, format, args)) < 0 || n >= len) {
		len *= 2;
		output = (char *)realloc(output, len+1);
	}
	return(output);
}
