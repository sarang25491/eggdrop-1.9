/* core_binds.c: core bind tables
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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

#include <eggdrop/eggdrop.h>
#include "core_binds.h"			/* prototypes		*/

/*!
 * \bind
 * This bind is triggered once a minute. It is similar to ::BT_event minutely
 * but allows you to match a specified date and time.
 * \name time
 * \flags Ignored.
 * \match The currect date and time in the format: "minute hour day month year".
 *        All fields except for "year" are two digit numbers including a leading
 *        0 if necessary. "hour" is a number in 24 hour format between 0 and 23.
 *        "year" is a full 4 digit number.
 * \param int The "minute" field of the match string.
 * \param int The "hour" field of the match string.
 * \param int The "day" field of the match string.
 * \param int The "month" field of the match string.
 * \param int The "year" field of the match string.
 * \stackable
 * \noreturn
 */

static bind_table_t *BT_time = NULL;

/*!
 * \bind
 * This bind is triggered once every second. The bind is not triggered one second
 * after the time it was triggered last but one second after the time it should
 * have been triggered last. So it should be triggered 60 times per minute on avarage.
 * You should \b never do anything that \a might take longer than one second to
 * execute here.
 * \name secondly
 * \flags Ignored.
 * \match None.
 * \noparam
 * \stackable
 * \noreturn
 */

static bind_table_t *BT_secondly = NULL;

/*!
 * \bind
 * This bind is triggered every time a local partymember executes the ".status"
 * command on the partyline.
 * \name status
 * \flags Ignored.
 * \match None.
 * \param partymember The partymember who typed ".status" on the partyline.
 * \param string Whatever the user typed after ".status".
 * \stackable
 * \noreturn
 * \bug The flags and match shouldn't be ignored, right?
 */

static bind_table_t *BT_status = NULL;

/*!
 * \bind
 * This bind is triggered when the bot is originally started and then every
 * time after a restart. It's impossible to tell which of the two just happened.
 * \name init
 * \flags Ignored.
 * \match None.
 * \noparam
 * \stackable
 * \noreturn
 * \bug Should there be a way to tell a start from a restart?
 */

static bind_table_t *BT_init = NULL;

/*!
 * \bind
 * This bind is triggered when the bot shuts down and before every restart.
 * It's impossible to tell which of the two just happened.
 * \name shutdown
 * \flags Ignored.
 * \match None.
 * \noparam
 * \stackable
 * \noreturn
 * \bug Should there be a way to tell a shutdown from a restart?
 */

static bind_table_t *BT_shutdown = NULL;

int core_binds_init(void)
{
	BT_init = bind_table_add (BTN_CORE_INIT, 0, "", MATCH_NONE, BIND_STACKABLE);		/* DDD	*/
	BT_shutdown = bind_table_add (BTN_CORE_SHUTDOWN, 0, "", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	BT_time = bind_table_add(BTN_CORE_TIME, 5, "iiiii", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	BT_secondly = bind_table_add(BTN_CORE_SECONDLY, 0, "", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	BT_status = bind_table_add(BTN_CORE_STATUS, 2, "Ps", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/

	return (0);
}

int core_binds_shutdown(void)
{
	bind_table_del(BT_status);
	bind_table_del(BT_secondly);
	bind_table_del(BT_time);
	bind_table_del(BT_shutdown);
	bind_table_del(BT_init);

	return (0);
}

void check_bind_init(void)
{
	egg_assert(BT_init != NULL);

	bind_check (BT_init, NULL, NULL);
}

void check_bind_shutdown (void)
{
	egg_assert(BT_shutdown != NULL);

	bind_check (BT_shutdown, NULL, NULL);
}

void check_bind_time(struct tm *tm)
{
	char full[32];

	sprintf(full, "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
	bind_check(BT_time, NULL, full, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
}

void check_bind_secondly()
{
	bind_check(BT_secondly, NULL, NULL);
}

void check_bind_status(partymember_t *p, const char *text)
{
	bind_check(BT_status, NULL, NULL, p, text);
}
