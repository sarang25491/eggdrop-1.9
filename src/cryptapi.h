#ifndef _CRYPTAPI_H_
#define _CRYPTAPI_H_

/* Interface definitions for encryption and hash functions. */

typedef struct {
	char *name;
	void *(*hash_begin)();
	int (*hash_update)(void *context, char *buf, int len);
	int (*hash_end)(void *context, char **digest, int *len);
	int (*hash_size)();
} hash_interface_t;

typedef struct {
	char *name;
	void *(*crypto_begin)();
	int *(crypto_set_key)(void *context, char *key, int len);
	int *(*crypto_encrypt)(void *context, char *inbuf, char *outbuf, int len);
	int *(*crypto_decrypt)(void *context, char *inbuf, char *outbuf, int len);
	int (*crypto_end)(void *context);
} crypto_interface_t;

int crypto_register_module(hash_interface_t *hash, crypto_interface_t *crypto);
int crypto_unregister_module(hash_interface_t *hash, crypto_interface_t *crypto);

#endif /* _CRYPTAPI_H_ */
