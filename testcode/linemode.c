#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sockbuf.h"

static int linemode_read(int idx, int event, int level, sockbuf_iobuf_t *new_data, sockbuf_iobuf_t *old_data)
{
	unsigned char *line, *stop, *save, *data;
	int linelen, savelen, datalen;
	sockbuf_iobuf_t my_iobuf;

	data = new_data->data;
	datalen = new_data->len;

	while (1) {
		/* If there's a cr or lf, we have a line. */
		stop = memchr(data, '\n', datalen);
		if (!stop) {
			stop = memchr(data, '\r', datalen);
			if (!stop) {
				/* No line, save the whole thing. */
				save = data;
				savelen = datalen;
				break;
			}
		}

		/* Terminate the line and get the length. */
		*stop = 0;
		linelen = stop - data;
		save = stop+1;
		savelen = data + datalen - save;

		/* Check for crlf. */
		if (stop > data && *(stop-1) == '\r') linelen--;

		/* If there is buffered data, concat it all. */
		if (old_data->len) {
			int newmax = old_data->len + linelen + 1;

			if (newmax > old_data->max) {
				/* Buffer is too small -- enlarge it. */
				old_data->data = (unsigned char *)realloc(old_data->data, newmax);
				old_data->max = newmax;
			}

			/* Add the new data to the end. */
			memcpy(old_data->data+old_data->len, data, linelen+1);

			/* Our line is: */
			line = old_data->data;
			linelen += old_data->len;

			/* Reset stored data. */
			old_data->len = 0;
		}
		else {
			line = data;
			/* And linelen is still correct. */
		}

		my_iobuf.data = line;
		my_iobuf.len = linelen;
		my_iobuf.max = linelen;
		line[linelen] = 0;
		sockbuf_filter(idx, event, level, &my_iobuf);

		/* If we're out of data, we're done. */
		if (savelen <= 0) return(0);
		/* Otherwise, do this again to check for another line. */
		data = save;
		datalen = savelen;
	}

	/* No cr/lf, so we save the remaining data for next time. */
	if (savelen + old_data->len > old_data->max) {
		/* Expand the buffer with room for next time. */
		old_data->data = (unsigned char *)realloc(old_data->data, savelen + old_data->len + 512);
		old_data->max = savelen + old_data->len + 512;
	}
	memmove(old_data->data + old_data->len, save, savelen);
	old_data->len += savelen;
	return(0);
}

static int linemode_eof_and_err(int idx, int event, int level, void *ignore, sockbuf_iobuf_t *old_data)
{
	/* If there is any buffered data, do one more on->read callback. */
	if (old_data->len) {
		old_data->data[old_data->len] = 0;
		sockbuf_filter(idx, SOCKBUF_READ, level, old_data);
	}

	/* And now continue the EOF/ERR event chain. */
	sockbuf_filter(idx, event, level, old_data);
	return(0);
}

static sockbuf_event_t linemode_filter = {
	(Function) 4,
	(Function) "line-mode",
	linemode_read,
	NULL,
	linemode_eof_and_err,
	linemode_eof_and_err
};

int linemode_on(int idx)
{
	sockbuf_iobuf_t *iobuf;

	iobuf = (sockbuf_iobuf_t *)calloc(sizeof(*iobuf), 0);
	sockbuf_attach_filter(idx, linemode_filter, iobuf);
	return(0);
}

int linemode_off(int idx)
{
	sockbuf_iobuf_t *iobuf;

	sockbuf_detach_filter(idx, linemode_filter, &iobuf);
	if (iobuf) {
		if (iobuf->data) free(iobuf->data);
		free(iobuf);
	}
	return(0);
}
