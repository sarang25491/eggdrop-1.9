#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

char *msprintf(char *format, ...)
{
	char *output;
	int n, len;
	va_list args;

	output = (char *)malloc(128);
	len = 128;
	while (1) {
		va_start(args, format);
		n = vsnprintf(output, len, format, args);
		va_end(args);
		if (n > -1 && n < len) return(output);
		if (n > len) len = n+1;
		else len *= 2;
		output = (char *)realloc(output, len);
	}
	return(output);
}
