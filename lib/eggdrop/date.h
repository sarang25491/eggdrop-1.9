#ifndef _EGG_DATE_H_
#define _EGG_DATE_H_

#define EGG_TIMEZONE_LOOKUP	-100000

int date_timezone(time_t time);
int date_scan(char *timestr, time_t *now, int zone, time_t *timeptr);

#endif
