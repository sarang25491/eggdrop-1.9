/* socktimer.c
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
 */

#ifndef lint
static const char rcsid[] = "$Id: socktimer.c,v 1.1 2008/11/02 17:46:13 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <unistd.h>

#define SOCKTIMER_LEVEL SOCKBUF_LEVEL_INTERNAL + 2

typedef struct {
	int idx;
	int timerid;
	int timerflags;
	void (*Callback)(int idx, void *client_data);
	void *client_data;
	event_owner_t *original_owner;
	event_owner_t new_owner;
} socktimer_t;

static int socktimer_on_delete(void *client_data, int idx)
{
	socktimer_t *data = client_data;

	data->idx = -1;
	if (data->timerid >= 0) timer_destroy(data->timerid);
	else free(data);
	return 0;
}

static int on_timer_delete(event_owner_t *owner, void *client_data)
{
	socktimer_t *data = client_data;

	if (data->original_owner && data->original_owner->on_delete) {
		data->original_owner->on_delete(data->original_owner, data->client_data);
	}
	data->timerid = -1;
	if (data->idx <= 0) free(data);

	return 0;
}

static int timer_callback(void *client_data)
{
	socktimer_t *data = client_data;

	if (!data->Callback) {
		sockbuf_delete(data->idx);
		return 0;
	}

	data->Callback(data->idx, data->client_data);
	return 0;
}

static sockbuf_filter_t socktimer_filter = {
	"socktimer",
	SOCKTIMER_LEVEL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, socktimer_on_delete
};

/*!
 * \brief Turns linebuffering on an index on
 *
 * \param idx the index to turn on linebuffering for
 * \return 0 on success, -1 on error
 */

int socktimer_on(int idx, int timeout, int flags, void (*Callback)(int idx, void *cd), void *client_data, event_owner_t *owner)
{
	egg_timeval_t timeval = {timeout, 0};
	socktimer_t *data;

	if (!sockbuf_isvalid(idx)) return -1;
	if (!Callback) flags = 0;
	if (!sockbuf_get_filter_data(idx, &socktimer_filter, &data)) {
		if (data->timerid >= 0) timer_destroy(data->timerid);
	} else {
		data = malloc(sizeof(*data));
		sockbuf_attach_filter(idx, &socktimer_filter, data);
	}

	data->idx = idx;
	data->timerflags = flags;
	data->Callback = Callback;
	data->client_data = client_data;
	data->original_owner = owner;
	data->new_owner = *owner;
	data->new_owner.on_delete = on_timer_delete;
	data->timerid = timer_create_complex(&timeval, "socktimer", timer_callback, data, flags, &data->new_owner);

	return 0;
}

/*!
 * \brief Turns linebuffering on an index off
 *
 * If there if a line currently buffered it's send of immidiatly.
 *
 * \param idx the index to turn off linebuffering for
 * \return 0 on success, -1 on error
 */

int socktimer_off(int idx)
{
	int ret;
	socktimer_t *data;

	ret = sockbuf_detach_filter(idx, &socktimer_filter, &data);
	if (ret) return ret;
	if (data) {
		if (data->timerid >= 0) {
			timer_destroy(data->timerid);
		}
		free(data);
	}
	return 0;
}

/*!
 * \brief Checks if an index is in linebuffered mode
 *
 * \param idx the index to check
 * \return 1 if linebuffered, 0 if not linebuffered, -1 on error
 */

int socktimer_check(int idx)
{
	if (!sockbuf_isvalid(idx)) return -1;
	return !sockbuf_get_filter_data(idx, &socktimer_filter, 0);
}
