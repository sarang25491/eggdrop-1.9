/* netstring.c
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
static const char rcsid[] = "$Id: netstring.c,v 1.1 2007/06/03 23:43:45 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <errno.h>

#define MAX_LEN 1000000

typedef struct {
	char *data;
	int len;
	int msg_len;
	int msg_offset;
} netstring_t;

static int netstring_on_write(void *client_data, int idx, const char *data, int len)
{
	char buf[517];
	char *sendbuf;
	int pos;

	if (len > MAX_LEN) len = MAX_LEN;
	if (len <= 512) sendbuf = buf;
	else sendbuf = malloc(len + 12);

	pos = sprintf(sendbuf, "%d:", len);
	memcpy(sendbuf + pos, data, len);
	pos += len;
	sendbuf[pos] = ',';
	sockbuf_on_write(idx, SOCKBUF_LEVEL_TEXT_BUFFER, sendbuf, pos + 1);
	if (sendbuf != buf) free(sendbuf);

	return 0;
}

static int netstring_on_read(void *client_data, int idx, char *data, int len)
{
	netstring_t *old_data = client_data;
	char *buffer, *start, *p;
	int buflen, msg_len, msg_offset;

	if (!old_data->data) {
		buffer = data;
		buflen = len;
		msg_len = -1;
		msg_offset = -1;
	} else {
		old_data->data = realloc(old_data->data, old_data->len + len);
		memcpy(old_data->data + old_data->len, data, len);
		old_data->len += len;
		buffer = old_data->data;
		buflen = old_data->len;
		msg_len = old_data->msg_len;
		msg_offset = old_data->msg_offset;
	}

	start = buffer;
	do {
		if (msg_len < 0) {
			p = memchr(start, ':', buflen);
			if (!p) {
				if (buflen >= 10) {
					sockbuf_on_eof(idx, SOCKBUF_LEVEL_INTERNAL, EIO, _("Invalid line format."));
					return 0;
				}
				break;
			}
			*p = 0;
			msg_len = atoi(start);
			if (msg_len > MAX_LEN) {
				sockbuf_on_eof(idx, SOCKBUF_LEVEL_INTERNAL, EIO, _("Invalid line format."));
				return 0;
			}
			msg_offset = p - start + 1;
		}

		if (buflen <= msg_len + msg_offset) break;

		if (start[msg_len + msg_offset] != ',') {
			sockbuf_on_eof(idx, SOCKBUF_LEVEL_INTERNAL, EIO, _("Invalid line format."));
			return 0;
		}

		start[msg_len + msg_offset] = 0;
		sockbuf_on_read(idx, SOCKBUF_LEVEL_TEXT_BUFFER, start + msg_offset, msg_len);
		if (!sockbuf_isvalid(idx)) return 0; /* We've been deleted */

		start += msg_offset + msg_len + 1;
		buflen -= msg_offset + msg_len + 1;
		msg_offset = -1;
		msg_len = -1;

	} while (buflen);

	old_data->len = buflen;
	old_data->msg_len = msg_len;
	old_data->msg_offset = msg_offset;

	if (start == old_data->data) return 0;

	if (!buflen) {
		if (old_data->data) free(old_data->data);
		old_data->data = NULL;
	} else {
		if (old_data->data) {
			memmove(old_data->data, start, buflen);
			old_data->data = realloc(old_data->data, buflen);
		} else {
			old_data->data = malloc(buflen);
			memcpy(old_data->data, start, buflen);
		}
	}
	return 0;
}

static int linemode_on_delete(void *client_data, int idx)
{
	netstring_t *old_data = client_data;
	/* When the sockbuf is deleted, just kill the associated data. */
	if (old_data->data) free(old_data->data);
	free(old_data);
	return 0;
}

static sockbuf_filter_t netstring_filter = {
	"netstring",
	SOCKBUF_LEVEL_TEXT_BUFFER,
	NULL, NULL, NULL,
	netstring_on_read, netstring_on_write, NULL,
	NULL, linemode_on_delete
};

/*!
 * \brief Enables the filter on an index
 *
 * \param idx the index to enable it for.
 * \return 0 on success, -1 on error.
 */

int netstring_on(int idx)
{
	netstring_t *old_data;

	if (netstring_check(idx)) return -1;
	old_data = malloc(sizeof(*old_data));
	old_data->data = NULL;
	old_data->len = 0;
	old_data->msg_len = 0;
	sockbuf_attach_filter(idx, &netstring_filter, old_data);
	return(0);
}

/*!
 * \brief Enables the filter on an index
 *
 * If there is text buffered it's send of immidiatly.
 *
 * \param idx the index to disable it for
 * \return 0 on success, -1 on error
 */

int netstring_off(int idx)
{
	int ret;
	netstring_t *old_data;

	ret = sockbuf_detach_filter(idx, &netstring_filter, &old_data);
	if (ret) return ret;
	if (old_data) {
		if (old_data->data) {
			if (old_data->len) {
				old_data->data[old_data->len] = 0;
				sockbuf_on_read(idx, SOCKBUF_LEVEL_TEXT_BUFFER, old_data->data, old_data->len);
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

int netstring_check(int idx)
{
	if (!sockbuf_isvalid(idx)) return -1;
	return !sockbuf_get_filter_data(idx, &netstring_filter, 0);
}
