#include <eggdrop/eggdrop.h>
#include "main.h"
#include "logfile.h"

static bind_table_t *BT_time, *BT_event;

void core_binds_init()
{
	BT_time = bind_table_add("time", 5, "iiiii", MATCH_MASK, BIND_STACKABLE);
	BT_event = bind_table_add("event", 1, "s", MATCH_MASK, BIND_STACKABLE);
}

void check_bind_time(struct tm *tm)
{
	char full[32];
	sprintf(full, "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
	bind_check(BT_time, full, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
}

void check_bind_event(char *event)
{
	bind_check(BT_event, event, NULL, event);
}
