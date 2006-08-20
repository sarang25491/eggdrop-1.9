/* linemode.c
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
static const char rcsid[] = "$Id: linemode.c,v 1.8 2006/08/20 15:23:05 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <unistd.h>

#define LINEMODE_LEVEL SOCKBUF_LEVEL_TEXT_BUFFER

typedef struct {
	char *data;
	int len;
} linemode_t;

static int linemode_on_read(void *client_data, int idx, char *data, int len)
{
	linemode_t *old_data = client_data;
	char *line, *stop, *save;
	int linelen, savelen;

	while (1) {
		/* If there's a cr or lf, we have a line. */
		stop = memchr(data, '\n', len);
		if (!stop) {
			stop = memchr(data, '\r', len);
			if (!stop) {
				/* No line, save the whole thing. */
				save = data;
				savelen = len;
				break;
			}
		}

		/* Save the cursor position for the next iteration. */
		save = stop+1;
		savelen = len - (save - data);

		if (stop > data && *(stop-1) == '\r') stop--;
		linelen = stop - data;
		*stop = 0;

		/* If there is buffered data, concat it all. */
		if (old_data->len) {
			/* Expand the buffer. */
			old_data->data = realloc(old_data->data, old_data->len + linelen + 1);
			memcpy(old_data->data+old_data->len, data, linelen + 1);

			line = old_data->data;
			linelen += old_data->len;

			/* All the old data is used. */
			old_data->len = 0;
		}
		else {
			line = data;
		}

		sockbuf_on_read(idx, LINEMODE_LEVEL, line, linelen);

		/* If we're out of data, we're done. */
		if (savelen <= 0) return(0);
		/* Otherwise, do this again to check for another line. */
		data = save;
		len = savelen;
	}

	/* No cr/lf, so we save the remaining data for next time. */
	old_data->data = realloc(old_data->data, savelen + old_data->len + 1);
	memcpy(old_data->data + old_data->len, save, savelen);
	old_data->len += savelen;
	return(0);
}

static int linemode_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	linemode_t *old_data = client_data;
	/* If there is any buffered data, do one more on->read callback. */
	if (old_data->len) {
		old_data->data[old_data->len] = 0;
		sockbuf_on_read(idx, LINEMODE_LEVEL, old_data->data, old_data->len);
		old_data->len = 0;
	}

	/* And now continue the eof event chain. */
	return sockbuf_on_eof(idx, LINEMODE_LEVEL, err, errmsg);
}

static int linemode_on_delete(void *client_data, int idx)
{
	linemode_t *old_data = client_data;
	/* When the sockbuf is deleted, just kill the associated data. */
	if (old_data->data) free(old_data->data);
	free(old_data);
	return(0);
}

static sockbuf_filter_t linemode_filter = {
	"linemode",
	LINEMODE_LEVEL,
	NULL, linemode_on_eof, NULL,
	linemode_on_read, NULL, NULL,
	NULL, linemode_on_delete
};

/*!
 * \brief Turns linebuffering on an index on
 *
 * \param idx the index to turn on linebuffering for
 * \return 0 on success, -1 on error
 */

int linemode_on(int idx)
{
	linemode_t *old_data;

	if (linemode_check(idx)) return -1;
	old_data = calloc(1, sizeof(*old_data));
	sockbuf_attach_filter(idx, &linemode_filter, old_data);
	return(0);
}

/*!
 * \brief Turns linebuffering on an index off
 *
 * If there if a line currently buffered it's send of immidiatly.
 *
 * \param idx the index to turn off linebuffering for
 * \return 0 on success, -1 on error
 */

int linemode_off(int idx)
{
	int ret;
	linemode_t *old_data;

	ret = sockbuf_detach_filter(idx, &linemode_filter, &old_data);
	if (ret) return ret;
	if (old_data) {
		if (old_data->data) {
			if (old_data->len) {
				old_data->data[old_data->len] = 0;
				sockbuf_on_read(idx, LINEMODE_LEVEL, old_data->data, old_data->len);
				old_data->len = 0;
			}
			free(old_data->data);
		}
		free(old_data);
	}
	return(0);
}

/*!
 * \brief Checks if an index is in linebuffered mode
 *
 * \param idx the index to check
 * \return 1 if linebuffered, 0 if not linebuffered, -1 on error
 */

int linemode_check(int idx)
{
	if (!sockbuf_isvalid(idx)) return -1;
	return !sockbuf_get_filter_data(idx, &linemode_filter, 0);
}
