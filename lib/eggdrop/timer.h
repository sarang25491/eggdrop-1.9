/* timer.h: header for timer.c
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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
 *
 * $Id: timer.h,v 1.1 2004/06/21 20:35:11 wingman Exp $
 */

#ifndef _EGG_TIMER_H_
#define _EGG_TIMER_H_

typedef struct egg_timeval {
	int sec;
	int usec;
} egg_timeval_t;

#define TIMER_REPEAT	1

/* Create a simple timer with no client data and no flags. */
#define timer_create(howlong,name,callback) timer_create_complex(howlong, name, callback, NULL, 0)

/* Create a simple timer with no client data, but it repeats. */
#define timer_create_repeater(howlong,name,callback) timer_create_complex(howlong, name, callback, NULL, TIMER_REPEAT)

extern int timer_init();
extern int timer_shutdown();
extern int timer_get_time(egg_timeval_t *curtime);
extern void timer_get_now(egg_timeval_t *_now);
extern int timer_get_now_sec(int *sec);
extern int timer_update_now(egg_timeval_t *_now);
extern int timer_diff(egg_timeval_t *from_time, egg_timeval_t *to_time, egg_timeval_t *diff);
extern int timer_create_complex(egg_timeval_t *howlong, const char *name, Function callback, void *client_data, int flags);
extern int timer_destroy(int timer_id);
extern int timer_destroy_all();
extern int timer_get_shortest(egg_timeval_t *howlong);
extern int timer_run();
extern int timer_list(int **ids);
extern int timer_info(int id, char **name, egg_timeval_t *initial_len, egg_timeval_t *trigger_time);
extern char *timer_get_timestamp(void);

#endif /* !_EGG_TIMER_H_ */
