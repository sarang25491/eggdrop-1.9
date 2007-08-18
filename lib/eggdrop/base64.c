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
static const char rcsid[] = "$Id: base64.c,v 1.4 2007/08/18 22:32:23 sven Exp $";
#endif

#include <stdlib.h>
#include "base64.h"

static char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *b64enc(const unsigned char *data, int len)
{
	char *dest;

	dest = malloc(4 * len / 3 + 4);
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

	dest = malloc(len+1);
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
static char base64to[256] = {
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,	 0,  0,  0,  0,  0,  0,  0,  0,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
	 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 62,  0, 63,  0,  0,
	 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static int tobase64[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '[', ']'
};

/*!
 * \brief Decodes a base64 string into an int.
 *
 * \note Uses the non-standard eggdrop1.6 style algorithm.
 *
 * \param b A base64 string.
 *
 * \return The decoded int value.
 */

int b64dec_int(const char *b)
{
	int i = 0;

	while (*b) i = (i << 6) + base64to[(unsigned char) *b++];
	return i;
}

/*!
 * \brief Calculate the base64 value of an int.
 *
 * \note Uses the non-standard eggdrop1.6 style algotithm.
 *
 * \param i The number to convert.
 *
 * \return A string containing the base64 value of \e i.
 *
 * \warning This function returns a pointer to a static buffer. Calling it
 *          again will overwrite it. \b Always \b remember!
 */

const char *b64enc_int(int i)
{
	char *pos;
	static char ret[12];

	pos = ret + 11;
	*pos = 0;

	do {
		--pos;
		*pos = tobase64[i & 0x3F];
		i >>= 6;
	} while (i);

	return pos;
}
