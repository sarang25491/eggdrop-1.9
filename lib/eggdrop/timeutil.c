/* timeutil.c: functions to help dealing with time
 *
 * Copyright (C) 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: timeutil.c,v 1.4 2004/08/29 07:34:12 takeda Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Returns pointer to static buffer containing string
   "12 days, 34 hours and 56 minutes"
   or NULL */
char *duration_to_string(time_t difftime)
{
	static char durstring[64];
	time_t days, hours, mins;
	int len;

	*durstring = '\0';
	days = difftime/86400;
	difftime -= days*86400;
	hours = difftime/3600;
	difftime -= hours*3600;
	mins = difftime/60;

	if (days)
		snprintf(durstring, sizeof(durstring), "%ld %s", days, days==1?"day":"days");
	if (hours) {
		len = strlen(durstring);
		snprintf(&durstring[len], sizeof(durstring) - len, "%s%ld %s",
			!len?"":mins?", ":"and ", hours, hours==1?"hour":"hours");
	}
	if (mins) {
		len = strlen(durstring);
		snprintf(&durstring[len], sizeof(durstring) - len, "%s%ld %s",
			len?" and ":"", mins, mins==1?"minute":"minutes");
	}

	return strlen(durstring)?durstring:"less than minute";
}

/* Returns time_t calcualted out of %XdYhZm */
time_t parse_expire_string(char *s)
{
	time_t tmp, exptime = 0;
	char *endptr = NULL;

	/* No need to:
	   if (!s || *s!='%') return 0;
	   caller already made sure
	*/
	s++;
	while (*s) {
		tmp = strtoul(s, &endptr, 10);
		switch(tolower(*endptr)) {
			case 'd':
				if (tmp > 365)
				tmp = 365;
				exptime += 86400 * tmp;
				break;
			case 'h':
				if (tmp > 8760)
				tmp = 8760;
				exptime += 3600 * tmp;
				break;
			case 'm':
				if (tmp > 525600)
				tmp = 525600;
				exptime += 60 * tmp;
				break;
			default:
				return 0;
		}
		s = endptr+1;
	}
	return exptime;
}
