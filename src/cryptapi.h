/* cryptapi.h: API for modules that want to provide hash/crypto
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
 *
 * $Id: cryptapi.h,v 1.2 2003/12/16 21:45:35 wcc Exp $
 */

#ifndef _EGG_CRYPTAPI_H_
#define _EGG_CRYPTAPI_H_

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

#endif /* !_EGG_CRYPTAPI_H_ */
