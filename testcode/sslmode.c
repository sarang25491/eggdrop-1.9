#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>

#include "sockbuf.h"

typedef struct {
	BIO *rbio, *wbio;
	SSL *ssl;
	sockbuf_iobuf_t old;
} sslmode_t;

static SSL_CTX *global_ctx;
static SSL_METHOD *global_method;

static void try_read(int idx, int level, sslmode_t *sslinfo)
{
	sockbuf_iobuf_t my_iobuf;
	char buf[4096];

	my_iobuf.data = buf;
	my_iobuf.max = sizeof(buf);
	while ((my_iobuf.len = SSL_read(sslinfo->ssl, buf, sizeof(buf))) > 0) {
		sockbuf_filter(idx, SOCKBUF_READ, level, &my_iobuf);
	}
}

static void try_write(int idx, int level, sslmode_t *sslinfo)
{
	char buf[4096];
	int len;

	while ((len = BIO_read(sslinfo->wbio, buf, sizeof(buf))) > 0) {
		sockbuf_write_filter(idx, level, buf, len);
	}
}

static int sslmode_read(int idx, int event, int level, sockbuf_iobuf_t *new_data, sslmode_t *sslinfo)
{
	int len;

	/* Add this data to the ssl's input bio. */
	BIO_write(sslinfo->rbio, new_data->data, new_data->len);
	try_read(idx, level, sslinfo);

	/* Check if old data can be written now. */
	if (sslinfo->old.len) {
		len = SSL_write(sslinfo->ssl, sslinfo->old.data, sslinfo->old.len);
		if (len > 0) {
			free(sslinfo->old.data);
			memset(&sslinfo->old, 0, sizeof(sslinfo->old));
		}
	}

	/* Check for new output bytes (like for renegotiation). */
	try_write(idx, level, sslinfo);

	return(0);
}

static int sslmode_eof_and_err(int idx, int event, int level, void *ignore, sslmode_t *sslinfo)
{
	/* Pass on event for now (should clean up structs). */
	sockbuf_filter(idx, event, level, ignore);
	return(0);
}

static int sslmode_write(int idx, int event, int level, sockbuf_iobuf_t *data, sslmode_t *sslinfo)
{
	char buf[4096];
	int r;

	r = SSL_write(sslinfo->ssl, data->data, data->len);
	if (r < data->len) {
		/* Save the data for later. */
		/* Maybe the connection isn't negotiated yet. */
		if (r < 0) r = 0;
		sslinfo->old.data = (unsigned char *)realloc(sslinfo->old.data, sslinfo->old.len + data->len - r);
		memcpy(sslinfo->old.data+sslinfo->old.len, data->data+r, data->len-r);
		sslinfo->old.len += (data->len - r);
		sslinfo->old.max = sslinfo->old.len;
	}

	/* Pass on any output that was produced. */
	try_write(idx, level, sslinfo);
	return(0);
}

static sockbuf_event_t sslmode_filter = {
	(Function) 6,
	(Function) "ssl-mode",
	sslmode_read,
	NULL,
	sslmode_eof_and_err,
	sslmode_eof_and_err,
	NULL,
	sslmode_write
};

/* client_or_server = 0 for client, 1 for server */
int sslmode_on(int idx, int client_or_server)
{
	sslmode_t *sslinfo;
	int level;

	sslinfo = (sslmode_t *)calloc(sizeof(*sslinfo), 1);
	sslinfo->ssl = SSL_new(global_ctx);
	sslinfo->rbio = BIO_new(BIO_s_mem());
	sslinfo->wbio = BIO_new(BIO_s_mem());
	SSL_set_bio(sslinfo->ssl, sslinfo->rbio, sslinfo->wbio);
	level = sockbuf_attach_filter(idx, sslmode_filter, sslinfo);

	/* Are we client or server? */
	if (client_or_server) SSL_accept(sslinfo->ssl);
	else SSL_connect(sslinfo->ssl);

	try_write(idx, level-1, sslinfo);

	return(0);
}

int sslmode_off(int idx)
{
	return(0);
}

/* I think I got this prime from ssh code, but I don't remember. */
static unsigned char dh512_p[] = {
	0xDA,0x58,0x3C,0x16,0xD9,0x85,0x22,0x89,0xD0,0xE4,0xAF,0x75,
	0x6F,0x4C,0xCA,0x92,0xDD,0x4B,0xE5,0x33,0xB8,0x04,0xFB,0x0F,
	0xED,0x94,0xEF,0x9C,0x8A,0x44,0x03,0xED,0x57,0x46,0x50,0xD3,
	0x69,0x99,0xDB,0x29,0xD7,0x76,0x27,0x6B,0xA2,0xD3,0xD4,0x12,
	0xE2,0x18,0xF4,0xDD,0x1E,0x08,0x4C,0xF6,0xD8,0x00,0x3E,0x7C,
	0x47,0x74,0xE8,0x33
};
/* Pretty standard generator. */
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

	/* Set up diffie-hellman parameters to use in case the certificate is
		a DSA key. */
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
