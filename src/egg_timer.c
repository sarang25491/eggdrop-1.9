/*
 * egg_timer.c --
 *
 */
/*
 * Copyright (C) 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: egg_timer.c,v 1.14 2002/09/20 02:06:25 stdarg Exp $";
#endif

#include <stdio.h> /* NULL */
#include <eggdrop/eggdrop.h>

/* From main.c */
extern egg_timeval_t egg_timeval_now;

static int script_single_timer(int nargs, int sec, int usec, script_callback_t *callback);
static int script_repeat_timer(int nargs, int sec, int usec, script_callback_t *callback);
static int script_timers(script_var_t *retval);
static int script_timer_info(script_var_t *retval, int timer_id);

static script_command_t script_cmds[] = {
	{"", "timer", script_single_timer, NULL, 2, "iic", "seconds ?microseconds? callback", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT | SCRIPT_PASS_COUNT},
	{"", "rtimer", script_repeat_timer, NULL, 2, "iic", "seconds ?microseconds? callback", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT | SCRIPT_PASS_COUNT},
	{"", "killtimer", timer_destroy, NULL, 1, "i", "timer-id", SCRIPT_INTEGER, 0},
	{"", "timers", script_timers, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{"", "timer_info", script_timer_info, NULL, 1, "i", "timer-id", 0, SCRIPT_PASS_RETVAL},
	{0}
};

void timer_init()
{
	script_create_commands(script_cmds);
}

static int script_timer(int sec, int usec, script_callback_t *callback, int flags)
{
	egg_timeval_t howlong;
	int id;

	howlong.sec = sec;
	howlong.usec = usec;
	callback->syntax = (char *)malloc(1);
	callback->syntax[0] = 0;
	id = timer_create_complex(&howlong, callback->callback, callback, flags);
	return(id);
}

static int script_single_timer(int nargs, int sec, int usec, script_callback_t *callback)
{
	if (nargs < 3) {
		sec = usec;
		usec = 0;
	}

	callback->flags |= SCRIPT_CALLBACK_ONCE;
	return script_timer(sec, usec, callback, 0);
}

static int script_repeat_timer(int nargs, int sec, int usec, script_callback_t *callback)
{
	if (nargs < 3) {
		sec = usec;
		usec = 0;
	}

	return script_timer(sec, usec, callback, TIMER_REPEAT);
}

static int script_timers(script_var_t *retval)
{
	int *timers, ntimers;

	ntimers = timer_list(&timers);

	/* A malloc'd array of ints. */
	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_INTEGER;
	retval->value = timers;
	retval->len = ntimers;
	return(0);
}

static int script_timer_info(script_var_t *retval, int timer_id)
{
	int *info;
	egg_timeval_t howlong, trigger_time, start_time, diff;

	if (timer_info(timer_id, &howlong, &trigger_time)) {
		retval->type = SCRIPT_STRING | SCRIPT_ERROR;
		retval->value = "Timer not found";
		return(0);
	}

	/* We have 11 fields. */
	info = (int *)malloc(sizeof(int) * 11);

	/* Timer id, when it started, initial timer length,
		how long it's run already, how long until it triggers,
		absolute time when it triggers. */
	info[0] = timer_id;

	timer_diff(&howlong, &trigger_time, &start_time);
	info[1] = start_time.sec;
	info[2] = start_time.usec;

	info[3] = howlong.sec;
	info[4] = howlong.usec;

	timer_diff(&start_time, &egg_timeval_now, &diff);
	info[5] = diff.sec;
	info[6] = diff.usec;

	timer_diff(&egg_timeval_now, &trigger_time, &diff);
	info[7] = diff.sec;
	info[8] = diff.usec;

	info[9] = trigger_time.sec;
	info[10] = trigger_time.usec;

	/* A malloc'd array of ints. */
	retval->type = SCRIPT_FREE | SCRIPT_ARRAY | SCRIPT_INTEGER;
	retval->value = info;
	retval->len = 11;

	return(0);
}
