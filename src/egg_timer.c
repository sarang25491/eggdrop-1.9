#include <stdio.h> /* NULL */
#include <string.h> /* memcpy() */
#include <stdlib.h> /* malloc(), free() */
#include <sys/time.h> /* gettimeofday() */

typedef int (*Function)();
#include "egg_timer.h"
#include "script_api.h"
#include "script.h"

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
static unsigned int timer_next_id = 1;

static int script_single_timer(int nargs, int sec, int usec, script_callback_t *callback);
static int script_repeat_timer(int nargs, int sec, int usec, script_callback_t *callback);

static script_command_t script_cmds[] = {
	{"", "timer", script_single_timer, NULL, 2, "iic", "seconds ?microseconds? callback", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT | SCRIPT_PASS_COUNT},
	{"", "rtimer", script_repeat_timer, NULL, 2, "iic", "seconds ?microseconds? callback", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT | SCRIPT_PASS_COUNT},
	{"", "killtimer", timer_destroy, NULL, 1, "i", "timer-id", SCRIPT_INTEGER, 0},
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

int timer_create_complex(egg_timeval_t *howlong, Function callback, void *client_data, int flags)
{
	egg_timer_t *timer, *prev;
	egg_timeval_t trigger_time;

	timer_get_time(&trigger_time);
	trigger_time.sec += howlong->sec;
	trigger_time.usec += howlong->usec;

	/* Find out where this should go in the list. */
	prev = NULL;
	for (timer = timer_list_head; timer; timer = timer->next) {
		if (trigger_time.sec < timer->trigger_time.sec) break;
		if (trigger_time.sec == timer->trigger_time.sec && trigger_time.usec < timer->trigger_time.usec) break;
		prev = timer;
	}

	/* Fill out a new timer. */
	timer = (egg_timer_t *)malloc(sizeof(*timer));
	timer->callback = callback;
	timer->client_data = client_data;
	timer->flags = flags;
	memcpy(&timer->howlong, howlong, sizeof(*howlong));
	memcpy(&timer->trigger_time, &trigger_time, sizeof(trigger_time));
	timer->id = timer_next_id++;

	/* Insert into timer list. */
	if (prev) {
		timer->next = prev->next;
		prev->next = timer;
	}
	else {
		timer->next = timer_list_head;
		timer_list_head = timer;
	}

	if (timer_next_id == 0) timer_next_id++;

	return(timer->id);
}

/* Destroy a timer, given an id. */
int timer_destroy(int timer_id)
{
	egg_timer_t *prev, *timer;

	prev = NULL;
	for (timer = timer_list_head; timer; timer = timer->next) {
		if (timer->id == timer_id) break;
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
	egg_timeval_t curtime;
	egg_timer_t *timer = timer_list_head;

	/* No timers? Boo. */
	if (!timer) return(1);

	timer_get_time(&curtime);

	if (timer->trigger_time.sec <= curtime.sec) howlong->sec = 0;
	else howlong->sec = timer->trigger_time.sec - curtime.sec;

	if (timer->trigger_time.usec <= curtime.usec) howlong->usec = 0;
	else howlong->usec = timer->trigger_time.usec - curtime.usec;

	return(0);
}

int timer_run()
{
	egg_timeval_t curtime;
	egg_timer_t *timer;
	Function callback;
	void *client_data;

	while (timer_list_head) {
		timer = timer_list_head;
		timer_get_time(&curtime);
		if (timer->trigger_time.sec > curtime.sec || (timer->trigger_time.sec == curtime.sec && timer->trigger_time.usec > curtime.usec)) break;

		timer_list_head = timer_list_head->next;

		callback = timer->callback;
		client_data = timer->client_data;

		if (timer->flags & TIMER_REPEAT) {
			egg_timer_t *prev, *tptr;

			/* Update timer. */
			timer->trigger_time.sec += timer->howlong.sec;
			timer->trigger_time.usec += timer->howlong.usec;

			prev = NULL;
			for (tptr = timer_list_head; tptr; tptr = tptr->next) {
				if (tptr->trigger_time.sec > timer->trigger_time.sec || (tptr->trigger_time.sec == timer->trigger_time.sec && tptr->trigger_time.usec > timer->trigger_time.usec)) break;
				prev = tptr;
			}
			if (prev) {
				timer->next = prev->next;
				prev->next = timer;
			}
			else {
				timer->next = timer_list_head;
				timer_list_head = timer;
			}
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
