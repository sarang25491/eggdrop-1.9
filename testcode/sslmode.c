#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>

#include "sockbuf.h"

typedef struct {
	BIO *rbio, *wbio;
	SSL *ssl;
	int negotiating;
	sockbuf_iobuf_t old;
} sslmode_t;

static SSL_CTX *global_ctx;
static SSL_METHOD *global_method;

static int rand_offer(sslmode_t *sslinfo, int idx, int level)
{
	char buf[10];

	if (rand() & 1) sslinfo->negotiating = 'S';
	else sslinfo->negotiating = 'C';
	buf[0] = sslinfo->negotiating;
	sockbuf_write_filter(idx, level, buf, 1);
}

static int handle_negotiation(int idx, int level, unsigned char *data, int len, sslmode_t *sslinfo)
{
	char buf[4096];
	int them, r, i, buflen;

	data[len] = 0;
	printf("negotiating: %c, %d, %d, %s\n", sslinfo->negotiating, idx, len, data);
	if (sslinfo->negotiating == 'c') {
		BIO_write(sslinfo->rbio, data, len);
		r = SSL_connect(sslinfo->ssl);
		if (r > 0) {
			sslinfo->negotiating = 0;
			return(0);
		}
	}
	else if (sslinfo->negotiating == 's') {
		BIO_write(sslinfo->rbio, data, len);
		r = SSL_accept(sslinfo->ssl);
		if (r > 0) {
			sslinfo->negotiating = 0;
			return(0);
		}
	}
	else {
		r = 0;
		for (i = 0; i < len; i++) {
			them = data[i];
			if (them == sslinfo->negotiating) {
				rand_offer(sslinfo, idx, level);
			}
			else {
				r = 1;
				break;
			}
		}
		if (!r) return(1);
		i++;
		if (i < len) BIO_write(sslinfo->rbio, data+i, len-i);
		if (them == 'S') {
			r = SSL_connect(sslinfo->ssl);
			if (r > 0) sslinfo->negotiating = 0;
			else sslinfo->negotiating = 'c';
		}
		else {
			r = SSL_accept(sslinfo->ssl);
			if (r > 0) sslinfo->negotiating = 0;
			else sslinfo->negotiating = 's';
		}
	}
	/* Check for new output bytes (like for renegotiation). */
	while ((buflen = BIO_read(sslinfo->wbio, buf, sizeof(buf))) > 0) {
		sockbuf_write_filter(idx, level, buf, buflen);
	}
	return(sslinfo->negotiating);
}

static int sslmode_read(int idx, int event, int level, sockbuf_iobuf_t *new_data, sslmode_t *sslinfo)
{
	sockbuf_iobuf_t my_iobuf;
	char buf[4096];
	int len;

/*
	for (len = 0; len < new_data->len; len++) {
		printf("%d ", new_data->data[len]);
	}
	printf("\n");
	len = 0;
*/
	if (sslinfo->negotiating) {
		if (handle_negotiation(idx, level, new_data->data, new_data->len, sslinfo)) return(0);
		/* If it's done, fall through to check excess data. */
	}
	else {
		/* Add this data to the ssl's input bio. */
		BIO_write(sslinfo->rbio, new_data->data, new_data->len);
	}

	my_iobuf.data = buf;
	my_iobuf.max = sizeof(buf);
	while ((len = SSL_read(sslinfo->ssl, buf, sizeof(buf))) > 0) {
		my_iobuf.len = len;
		sockbuf_filter(idx, event, level, &my_iobuf);
	}

	/* Check if old data can be written now. */
	if (sslinfo->old.len) {
		len = SSL_write(sslinfo->ssl, sslinfo->old.data, sslinfo->old.len);
		if (len > 0) {
			printf("Wrote %d/%d bytes of queued data\n", len, sslinfo->old.len);
			free(sslinfo->old.data);
			memset(&sslinfo->old, 0, sizeof(sslinfo->old));
		}
		else {
			//printf("Error sending queued data\n");
			ERR_print_errors_fp(stderr);
		}
	}

	/* Check for new output bytes (like for renegotiation). */
	while ((len = BIO_read(sslinfo->wbio, buf, sizeof(buf))) > 0) {
		sockbuf_write_filter(idx, level, buf, len);
	}

	return(0);
}

static int sslmode_eof_and_err(int idx, int event, int level, void *ignore, sslmode_t *sslinfo)
{
	sockbuf_filter(idx, event, level, ignore);
	return(0);
}

static int sslmode_write(int idx, int event, int level, sockbuf_iobuf_t *data, sslmode_t *sslinfo)
{
	char buf[4096];
	int r = -1;

	if (!sslinfo->negotiating) r = SSL_write(sslinfo->ssl, data->data, data->len);
	if (r < data->len) {
		/* Save the data for later. */
		/* Maybe the connection isn't negotiated yet? */
		//printf("Write error, saving\n");
		//ERR_print_errors_fp(stderr);
		if (r < 0) r = 0;
		sslinfo->old.data = (unsigned char *)realloc(sslinfo->old.data, sslinfo->old.len + data->len - r);
		memcpy(sslinfo->old.data+sslinfo->old.len, data->data+r, data->len-r);
		sslinfo->old.len += (data->len - r);
		sslinfo->old.max = sslinfo->old.len;
	}

	/* Pass on any output that was produced. */
	if (!sslinfo->negotiating) while ((r = BIO_read(sslinfo->wbio, buf, sizeof(buf))) > 0) {
		sockbuf_write_filter(idx, level, buf, r);
	}
	return(0);
}

static sockbuf_event_t sslmode_filter = {
	(Function) 5,
	(Function) "ssl-mode",
	sslmode_read,
	NULL,
	sslmode_eof_and_err,
	sslmode_eof_and_err,
	sslmode_write
};

int sslmode_on(int idx)
{
	sslmode_t *sslinfo;
	int i;
	char buf[4096];

	sslinfo = (sslmode_t *)calloc(sizeof(*sslinfo), 1);
	sslinfo->ssl = SSL_new(global_ctx);
	sslinfo->rbio = BIO_new(BIO_s_mem());
	sslinfo->wbio = BIO_new(BIO_s_mem());
	SSL_set_bio(sslinfo->ssl, sslinfo->rbio, sslinfo->wbio);
	/* We have to randomly decide who is server and who is client. */
	rand_offer(sslinfo, idx, -1);
	sockbuf_attach_filter(idx, sslmode_filter, sslinfo);
	return(0);
}

int sslmode_off(int idx)
{
	return(0);
}

static unsigned char dh512_p[] = {
        0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
        0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
        0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
        0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
        0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
        0x47,0x74,0xE8,0x33
};
static unsigned char dh512_g[] = {
        0x02
};
static DH *get_dh512(void) {
        DH *dh=NULL;

        if ((dh=DH_new()) == NULL) return(NULL);
        dh->p=BN_bin2bn(dh512_p,sizeof(dh512_p),NULL);
        dh->g=BN_bin2bn(dh512_g,sizeof(dh512_g),NULL);
        if ((dh->p == NULL) || (dh->g == NULL))
                return(NULL);
        return(dh);
}

int sslmode_init()
{
	DH *dh = NULL;

	SSL_load_error_strings();
	SSL_library_init();
	global_method = SSLv23_method();
	//global_method = SSLv23_client_method();
	global_ctx = SSL_CTX_new(global_method);
	dh = get_dh512();
	SSL_CTX_set_tmp_dh(global_ctx, dh);
	DH_free(dh);
	if (SSL_CTX_use_certificate_file(global_ctx, "private/cert.pem", SSL_FILETYPE_PEM) < 1) {
		printf("Can't load certificate file\n");
		ERR_print_errors_fp(stderr);
	}
	if (SSL_CTX_use_PrivateKey_file(global_ctx, "private/key.pem", SSL_FILETYPE_PEM) < 1) {
		printf("Can't load private key file\n");
		ERR_print_errors_fp(stderr);
	}
	SSL_CTX_set_verify(global_ctx, SSL_VERIFY_NONE, NULL);
	return(0);
}
