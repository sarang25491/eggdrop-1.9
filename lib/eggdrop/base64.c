/* base64.c: base64 encoding/decoding
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
static const char rcsid[] = "$Id: base64.c,v 1.2 2003/12/17 07:39:14 wcc Exp $";
#endif

#include <stdlib.h>
#include "base64.h"

static char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *b64enc(const unsigned char *data, int len)
{
	char *dest;

	dest = (char *)malloc(4 * len / 3 + 4);
	b64enc_buf(data, len, dest);
	return(dest);
}

int b64enc_buf(const unsigned char *data, int len, char *dest)
{
	char *buf = dest;

	/* Encode 3 bytes at a time. */
	while (len >= 3) {
		buf[0] = b64chars[(data[0] >> 2) & 0x3f];
		buf[1] = b64chars[((data[0] << 4) & 0x30) | ((data[1] >> 4) & 0xf)];
		buf[2] = b64chars[((data[1] << 2) & 0x3c) | ((data[2] >> 6) & 0x3)];
		buf[3] = b64chars[data[2] & 0x3f];
		data += 3;
		buf += 4;
		len -= 3;
	}

	if (len > 0) {
		buf[0] = b64chars[(data[0] >> 2) & 0x3f];
		buf[1] = b64chars[(data[0] << 4) & 0x30];
		if (len > 1) {
			buf[1] += (data[1] >> 4) & 0xf;
			buf[2] = b64chars[(data[1] << 2) & 0x3c];
		}
		else buf[2] = '=';
		buf[3] = '=';
		buf += 4;
	}

	*buf = '\0';
	return(buf - dest);
}

char *b64dec(const unsigned char *data, int len)
{
	char *dest;

	dest = (char *)malloc(len+1);
	b64dec_buf(data, len, dest);
	return(dest);
}

int b64dec_buf(const unsigned char *data, int len, char *dest)
{
	unsigned char c;
	int cur = 0, val = 0, i = 0;

	while (len) {
		c = *data;
		len--;
		data++;

		if (c >= 'A' && c <= 'Z') val = c - 'A';
		else if (c >= 'a' && c <= 'z') val = c - 'a' + 26;
		else if (c >= '0' && c <= '9') val = c - '0' + 52;
		else if (c == '+') val = 62;
		else if (c == '/') val = 63;
		else if (c == '=') break;
		else continue;

		switch (cur++) {
			case 0:
				dest[i] = (val << 2) & 0xfc;
				break;
			case 1:
				dest[i] |= (val >> 4) & 0x03;
				i++;
				dest[i] = (val << 4) & 0xf0;
				break;
			case 2:
				dest[i] |= (val >> 2) & 0x0f;
				i++;
				dest[i] = (val << 6) & 0xc0;
				break;
			case 3:
				dest[i] |= val & 0x3f;
				i++;
				cur = 0;
				break;
		}
	}
	dest[i] = 0;
	return(i);
}
