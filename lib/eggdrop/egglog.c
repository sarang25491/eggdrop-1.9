#include <eggdrop/eggdrop.h>

static bind_table_t *BT_log = NULL;

void egglog_init()
{
	BT_log = bind_table_add("log", 4, "issi", MATCH_NONE, BIND_STACKABLE);
}

int putlog(int flags, const char *chan, const char *format, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, format);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, format, args);
	va_end(args);
	bind_check(BT_log, NULL, flags, chan, ptr, len);
	if (ptr != buf) free(ptr);
	return(len);
}
