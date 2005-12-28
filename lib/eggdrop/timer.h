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
 * $Id: timer.h,v 1.5 2005/12/28 17:27:31 sven Exp $
 */

#ifndef _EGG_TIMER_H_
#define _EGG_TIMER_H_

typedef struct egg_timeval {
	long sec;
	long usec;
} egg_timeval_t;

typedef struct egg_timer {
	struct egg_timer *next;
	int id;
	char *name;
	Function callback;
	void *client_data;
	egg_timeval_t howlong;
	egg_timeval_t trigger_time;
	int flags;
	event_owner_t *owner;
} egg_timer_t;

#define TIMER_REPEAT	1

/* Create a simple timer with no client data and no flags. */
#define timer_create(howlong,name,callback) timer_create_complex(howlong, name, callback, NULL, 0, NULL)

/* Create a simple timer with no client data, but it repeats. */
#define timer_create_repeater(howlong,name,callback) timer_create_complex(howlong, name, callback, NULL, TIMER_REPEAT, NULL)

extern int timer_init();
extern int timer_shutdown();
extern int timer_get_time(egg_timeval_t *curtime);
extern void timer_get_now(egg_timeval_t *_now);
extern long timer_get_now_sec(long *sec);
extern long timer_update_now(egg_timeval_t *_now);
extern int timer_diff(egg_timeval_t *from_time, egg_timeval_t *to_time, egg_timeval_t *diff);
extern int timer_create_secs(long secs, const char *name, Function callback);
extern int timer_create_complex(egg_timeval_t *howlong, const char *name, Function callback, void *client_data, int flags, event_owner_t *owner);
extern int timer_destroy(int timer_id);
extern int timer_destroy_all();
extern void timer_destroy_by_module(egg_module_t *module);
extern int timer_get_shortest(egg_timeval_t *howlong);
extern int timer_run();
extern egg_timer_t *timer_list();
extern egg_timer_t *timer_find(int id);
extern char *timer_get_timestamp(void);

#endif /* !_EGG_TIMER_H_ */
