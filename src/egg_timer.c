#include <stdio.h> /* NULL */
#include <string.h> /* memcpy() */
#include <stdlib.h> /* malloc(), free() */
#include <sys/time.h> /* gettimeofday() */

#include <eggdrop/eggdrop.h>
#include "egg_timer.h"

/* From main.c */
extern egg_timeval_t egg_timeval_now;

/* Internal use only. */
typedef struct egg_timer_b {
	struct egg_timer_b *next;
	int id;
	Function callback;
	void *client_data;
	egg_timeval_t howlong;
	egg_timeval_t trigger_time;
	int flags;
} egg_timer_t;

/* We keep a sorted list of active timers. */
static egg_timer_t *timer_list_head = NULL;
static int timer_next_id = 1;

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
	script_create_cmd_table(script_cmds);
}

/* Based on TclpGetTime from Tcl 8.3.3 */
int timer_get_time(egg_timeval_t *curtime)
{
	struct timeval tv;
	struct timezone tz;

	(void) gettimeofday(&tv, &tz);
	curtime->sec = tv.tv_sec;
	curtime->usec = tv.tv_usec;
	return(0);
}

/* Find difference between two timers. */
int timer_diff(egg_timeval_t *from_time, egg_timeval_t *to_time, egg_timeval_t *diff)
{
	diff->sec = to_time->sec - from_time->sec;
	if (diff->sec < 0) {
		diff->sec = 0;
		diff->usec = 0;
		return(1);
	}

	diff->usec = to_time->usec - from_time->usec;

	if (diff->usec < 0) {
		if (diff->sec == 0) {
			diff->usec = 0;
			return(1);
		}
		diff->sec -= 1;
		diff->usec += 1000000;
	}

	return(0);
}

static int timer_add_to_list(egg_timer_t *timer)
{
	egg_timer_t *prev, *ptr;

	/* Find out where this should go in the list. */
	prev = NULL;
	for (ptr = timer_list_head; ptr; ptr = ptr->next) {
		if (timer->trigger_time.sec < ptr->trigger_time.sec) break;
		if (timer->trigger_time.sec == ptr->trigger_time.sec && timer->trigger_time.usec < ptr->trigger_time.usec) break;
		prev = ptr;
	}

	/* Insert into timer list. */
	if (prev) {
		timer->next = prev->next;
		prev->next = timer;
	}
	else {
		timer->next = timer_list_head;
		timer_list_head = timer;
	}
	return(0);
}

int timer_create_complex(egg_timeval_t *howlong, Function callback, void *client_data, int flags)
{
	egg_timer_t *timer;

	/* Fill out a new timer. */
	timer = (egg_timer_t *)malloc(sizeof(*timer));
	timer->id = timer_next_id++;
	timer->callback = callback;
	timer->client_data = client_data;
	timer->flags = flags;
	timer->howlong.sec = howlong->sec;
	timer->howlong.usec = howlong->usec;
	timer->trigger_time.sec = egg_timeval_now.sec + howlong->sec;
	timer->trigger_time.usec = egg_timeval_now.usec + howlong->usec;

	timer_add_to_list(timer);

	return(timer->id);
}

/* Destroy a timer, given an id. */
int timer_destroy(int timer_id)
{
	egg_timer_t *prev, *timer;

	prev = NULL;
	for (timer = timer_list_head; timer; timer = timer->next) {
		if (timer->id == timer_id) break;
		prev = timer;
	}

	if (!timer) return(1); /* Not found! */

	/* Unlink it. */
	if (prev) prev->next = timer->next;
	else timer_list_head = timer->next;

	free(timer);
	return(0);
}

int timer_destroy_all()
{
	egg_timer_t *timer, *next;

	for (timer = timer_list_head; timer; timer = next) {
		next = timer->next;
		free(timer);
	}
	timer_list_head = NULL;
	return(0);
}

int timer_get_shortest(egg_timeval_t *howlong)
{
	egg_timer_t *timer = timer_list_head;

	/* No timers? Boo. */
	if (!timer) return(1);

	timer_diff(&egg_timeval_now, &timer->trigger_time, howlong);

	return(0);
}

int timer_run()
{
	egg_timer_t *timer;
	Function callback;
	void *client_data;

	while (timer_list_head) {
		timer = timer_list_head;
		if (timer->trigger_time.sec > egg_timeval_now.sec || (timer->trigger_time.sec == egg_timeval_now.sec && timer->trigger_time.usec > egg_timeval_now.usec)) break;

		timer_list_head = timer_list_head->next;

		callback = timer->callback;
		client_data = timer->client_data;

		if (timer->flags & TIMER_REPEAT) {
			/* Update timer. */
			timer->trigger_time.sec += timer->howlong.sec;
			timer->trigger_time.usec += timer->howlong.usec;
			if (timer->trigger_time.usec >= 1000000) {
				timer->trigger_time.usec -= 1000000;
				timer->trigger_time.sec += 1;
			}

			/* Add it back into the list. */
			timer_add_to_list(timer);
		}
		else {
			free(timer);
		}

		callback(client_data);
	}
	return(0);
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
	egg_timer_t *timer;

	/* Count timers. */
	ntimers = 0;
	for (timer = timer_list_head; timer; timer = timer->next) ntimers++;

	/* Fill in array. */
	timers = (int *)malloc(sizeof(int) * ntimers);
	ntimers = 0;
	for (timer = timer_list_head; timer; timer = timer->next) {
		timers[ntimers++] = timer->id;
	}

	/* A malloc'd array of ints. */
	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_INTEGER;
	retval->value = timers;
	retval->len = ntimers;
	return(0);
}

static int script_timer_info(script_var_t *retval, int timer_id)
{
	egg_timer_t *timer;
	static int timer_info[11];
	egg_timeval_t start_time, diff;

	for (timer = timer_list_head; timer; timer = timer->next) {
		if (timer->id == timer_id) break;
	}
	if (!timer) {
		retval->type = SCRIPT_STRING | SCRIPT_ERROR;
		retval->value = "Timer not found";
		return(0);
	}

	/* Timer id, when it started, initial timer length,
		how long it's run already, how long until it triggers,
		absolute time when it triggers. */
	timer_info[0] = timer->id;

	timer_diff(&timer->howlong, &timer->trigger_time, &start_time);
	timer_info[1] = start_time.sec;
	timer_info[2] = start_time.usec;

	timer_info[3] = timer->howlong.sec;
	timer_info[4] = timer->howlong.usec;

	timer_diff(&start_time, &egg_timeval_now, &diff);
	timer_info[5] = diff.sec;
	timer_info[6] = diff.usec;

	timer_diff(&egg_timeval_now, &timer->trigger_time, &diff);
	timer_info[7] = diff.sec;
	timer_info[8] = diff.usec;

	timer_info[9] = timer->trigger_time.sec;
	timer_info[10] = timer->trigger_time.usec;

	/* A malloc'd array of ints. */
	retval->type = SCRIPT_ARRAY | SCRIPT_INTEGER;
	retval->value = timer_info;
	retval->len = 11;

	return(0);
}
