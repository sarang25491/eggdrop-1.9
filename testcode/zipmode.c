#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "sockbuf.h"

#define ZIPMODE_LEVEL	SOCKBUF_LEVEL_COMPRESSION

typedef struct {
	z_stream instream, outstream;
	int counter;
} zipmode_t;

static int zipmode_read(void *client_data, int idx, char *data, int len)
{
	zipmode_t *zip = client_data;
	char buf[4096];
	int r, buflen;

	zip->instream.next_in = data;
	zip->instream.avail_in = len;
	do {
		zip->instream.next_out = buf;
		zip->instream.avail_out = sizeof(buf);
		r = inflate(&zip->instream, Z_SYNC_FLUSH);
		if (zip->instream.avail_out != sizeof(buf)) {
			buflen = sizeof(buf) - zip->instream.avail_out;
			sockbuf_on_read(idx, ZIPMODE_LEVEL, buf, buflen);
		}
	} while (r == Z_OK && zip->instream.avail_in);
	return(0);
}

static int zipmode_eof(void *client_data, int idx, int err, const char *errmsg)
{
	zipmode_t *zip = client_data;
	int buflen;
	char buf[8192];

	/* If there is any buffered data, do one more on->read callback. */
	zip->instream.next_out = buf;
	zip->instream.avail_out = sizeof(buf);
	inflate(&zip->instream, Z_FINISH);

	if (zip->instream.avail_out != sizeof(buf)) {
		buflen = sizeof(buf) - zip->instream.avail_out;
		sockbuf_on_read(idx, ZIPMODE_LEVEL, buf, buflen);
	}
	printf("zip stats: input %d compressed / %d uncompressed\n", zip->instream.total_in, zip->instream.total_out);
	printf("zip stats: output %d compressed / %d uncompressed\n", zip->outstream.total_out, zip->outstream.total_in);
	/* And now continue the EOF/ERR event chain. */
	sockbuf_on_eof(idx, ZIPMODE_LEVEL, err, errmsg);
	return(0);
}

static int zipmode_write(void *client_data, int idx, const char *data, int len)
{
	zipmode_t *zip = client_data;
	char buf[4096];
	int flags, r, buflen;

	zip->outstream.next_in = (char *)data;
	zip->outstream.avail_in = len;
	do {
		zip->outstream.next_out = buf;
		zip->outstream.avail_out = sizeof(buf);
		zip->counter++;
		if (zip->counter == 1) {
			zip->counter = 0;
			flags = Z_SYNC_FLUSH;
		}
		else flags = Z_NO_FLUSH;
		r = deflate(&zip->outstream, flags);
		if (zip->outstream.avail_out != sizeof(buf)) {
			buflen = sizeof(buf) - zip->outstream.avail_out;
			sockbuf_on_write(idx, ZIPMODE_LEVEL, buf, buflen);
		}
	} while (r == Z_OK && zip->outstream.avail_in);
	return(0);
}

static sockbuf_filter_t zipmode_filter = {
	"zipmode",
	ZIPMODE_LEVEL,
	NULL, zipmode_eof, NULL,
	zipmode_read, zipmode_write
};

int zipmode_on(int idx)
{
	zipmode_t *zip;

	zip = (zipmode_t *)calloc(1, sizeof(*zip));
	deflateInit(&zip->outstream, Z_DEFAULT_COMPRESSION);
	inflateInit(&zip->instream);

	sockbuf_attach_filter(idx, &zipmode_filter, zip);
	return(0);
}

int zipmode_off(int idx)
{
	zipmode_t *zip;

	sockbuf_detach_filter(idx, &zipmode_filter, &zip);
	if (zip) {
		deflateEnd(&zip->outstream);
		inflateEnd(&zip->instream);
		free(zip);
	}
	return(0);
}
