/*
 * blowfish.c --
 *
 *	encryption and decryption of passwords
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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
/*
 * This code was originally in the public domain.
 */

#ifndef lint
static const char rcsid[] = "$Id: blowfish.c,v 1.10 2003/03/06 12:08:14 tothwolf Exp $";
#endif

#define MODULE_NAME "encryption"
#define MAKING_ENCRYPTION

#include "lib/eggdrop/module.h"
#include "blowfish.h"
#include "bf_tab.h"		/* P-box P-array, S-box */

#define start blowfish_LTX_start

static eggdrop_t *egg = NULL;

/* Each box takes up 4k so be very careful here */
#define BOXES 3

/* #define S(x,i) (bf_S[i][x.w.byte##i]) */
#define S0(x) (bf_S[0][x.w.byte0])
#define S1(x) (bf_S[1][x.w.byte1])
#define S2(x) (bf_S[2][x.w.byte2])
#define S3(x) (bf_S[3][x.w.byte3])
#define bf_F(x) (((S0(x) + S1(x)) ^ S2(x)) + S3(x))
#define ROUND(a,b,n) (a.word ^= bf_F(b) ^ bf_P[n])

/* Keep a set of rotating P & S boxes */
static struct box_t {
  u_32int_t *P;
  u_32int_t **S;
  char key[81];
  char keybytes;
  time_t lastuse;
} box[BOXES];

/* static u_32int_t bf_P[bf_N+2]; */
/* static u_32int_t bf_S[4][256]; */
static u_32int_t *bf_P;
static u_32int_t **bf_S;

static void blowfish_encipher(u_32int_t * xl, u_32int_t * xr)
{
  union aword Xl;
  union aword Xr;

  Xl.word = *xl;
  Xr.word = *xr;

  Xl.word ^= bf_P[0];
  ROUND(Xr, Xl, 1);
  ROUND(Xl, Xr, 2);
  ROUND(Xr, Xl, 3);
  ROUND(Xl, Xr, 4);
  ROUND(Xr, Xl, 5);
  ROUND(Xl, Xr, 6);
  ROUND(Xr, Xl, 7);
  ROUND(Xl, Xr, 8);
  ROUND(Xr, Xl, 9);
  ROUND(Xl, Xr, 10);
  ROUND(Xr, Xl, 11);
  ROUND(Xl, Xr, 12);
  ROUND(Xr, Xl, 13);
  ROUND(Xl, Xr, 14);
  ROUND(Xr, Xl, 15);
  ROUND(Xl, Xr, 16);
  Xr.word ^= bf_P[17];

  *xr = Xl.word;
  *xl = Xr.word;
}

static void blowfish_decipher(u_32int_t * xl, u_32int_t * xr)
{
  union aword Xl;
  union aword Xr;

  Xl.word = *xl;
  Xr.word = *xr;

  Xl.word ^= bf_P[17];
  ROUND(Xr, Xl, 16);
  ROUND(Xl, Xr, 15);
  ROUND(Xr, Xl, 14);
  ROUND(Xl, Xr, 13);
  ROUND(Xr, Xl, 12);
  ROUND(Xl, Xr, 11);
  ROUND(Xr, Xl, 10);
  ROUND(Xl, Xr, 9);
  ROUND(Xr, Xl, 8);
  ROUND(Xl, Xr, 7);
  ROUND(Xr, Xl, 6);
  ROUND(Xl, Xr, 5);
  ROUND(Xr, Xl, 4);
  ROUND(Xl, Xr, 3);
  ROUND(Xr, Xl, 2);
  ROUND(Xl, Xr, 1);
  Xr.word ^= bf_P[0];

  *xl = Xr.word;
  *xr = Xl.word;
}


static void blowfish_report(int idx, int details)
{
}

static void blowfish_init(u_8bit_t * key, int keybytes)
{
  int i, j, bx;
  time_t lowest = 0;
  u_32int_t data;
  u_32int_t datal;
  u_32int_t datar;
  union aword temp;

  /* drummer: Fixes crash if key is longer than 80 char. This may cause the key
   *          to not end with \00 but that's no problem.
   */
  if (keybytes > 80)
    keybytes = 80;

  /* Is buffer already allocated for this? */
  for (i = 0; i < BOXES; i++)
    if (box[i].P != NULL) {
      if ((box[i].keybytes == keybytes) &&
	  (!strncmp((char *) (box[i].key), (char *) key, keybytes))) {
	/* Match! */
	box[i].lastuse = egg_timeval_now.sec;
	bf_P = box[i].P;
	bf_S = box[i].S;
	return;
      }
    }
  /* No pre-allocated buffer: make new one */
  /* Set 'bx' to empty buffer */
  bx = (-1);
  for (i = 0; i < BOXES; i++) {
    if (box[i].P == NULL) {
      bx = i;
      i = BOXES + 1;
    }
  }
  if (bx < 0) {
    /* Find oldest */
    lowest = egg_timeval_now.sec;
    for (i = 0; i < BOXES; i++)
      if (box[i].lastuse <= lowest) {
	lowest = box[i].lastuse;
	bx = i;
      }
    free(box[bx].P);
    for (i = 0; i < 4; i++)
      free(box[bx].S[i]);
    free(box[bx].S);
  }
  /* Initialize new buffer */
  /* uh... this is over 4k */
  box[bx].P = (u_32int_t *) malloc((bf_N + 2) * sizeof(u_32int_t));
  box[bx].S = (u_32int_t **) malloc(4 * sizeof(u_32int_t *));
  for (i = 0; i < 4; i++)
    box[bx].S[i] = (u_32int_t *) malloc(256 * sizeof(u_32int_t));
  bf_P = box[bx].P;
  bf_S = box[bx].S;
  box[bx].keybytes = keybytes;
  strlcpy(box[bx].key, key, keybytes + 1);	/* + 1 for NULL */
  box[bx].lastuse = egg_timeval_now.sec;
  /* Robey: Reset blowfish boxes to initial state
   * (I guess normally it just keeps scrambling them, but here it's
   * important to get the same encrypted result each time)
   */
  for (i = 0; i < bf_N + 2; i++)
    bf_P[i] = initbf_P[i];
  for (i = 0; i < 4; i++)
    for (j = 0; j < 256; j++)
      bf_S[i][j] = initbf_S[i][j];

  j = 0;
  if (keybytes > 0) { /* drummer: fixes crash if key=="" */
    for (i = 0; i < bf_N + 2; ++i) {
      temp.word = 0;
      temp.w.byte0 = key[j];
      temp.w.byte1 = key[(j + 1) % keybytes];
      temp.w.byte2 = key[(j + 2) % keybytes];
      temp.w.byte3 = key[(j + 3) % keybytes];
      data = temp.word;
      bf_P[i] = bf_P[i] ^ data;
      j = (j + 4) % keybytes;
    }
  }
  datal = 0x00000000;
  datar = 0x00000000;
  for (i = 0; i < bf_N + 2; i += 2) {
    blowfish_encipher(&datal, &datar);
    bf_P[i] = datal;
    bf_P[i + 1] = datar;
  }
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 256; j += 2) {
      blowfish_encipher(&datal, &datar);
      bf_S[i][j] = datal;
      bf_S[i][j + 1] = datar;
    }
  }
}

#define SALT1  0xdeadd061
#define SALT2  0x23f6b095

/* Convert 64-bit encrypted password to text for userfile */
static char *base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int base64dec(char c)
{
  int i;

  for (i = 0; i < 64; i++)
    if (base64[i] == c)
      return i;
  return 0;
}

static void blowfish_encrypt_pass(char *text, char *new)
{
  u_32int_t left, right;
  int n;
  char *p;

  blowfish_init((unsigned char *) text, strlen(text));
  left = SALT1;
  right = SALT2;
  blowfish_encipher(&left, &right);
  p = new;
  *p++ = '+';			/* + means encrypted pass */
  n = 32;
  while (n > 0) {
    *p++ = base64[right & 0x3f];
    right = (right >> 6);
    n -= 6;
  }
  n = 32;
  while (n > 0) {
    *p++ = base64[left & 0x3f];
    left = (left >> 6);
    n -= 6;
  }
  *p = 0;
}

/* Returned string must be freed when done with it!
 */
static char *encrypt_string(char *key, char *str)
{
  u_32int_t left, right;
  unsigned char *p;
  char *s, *dest, *d;
  int i;

  /* Pad fake string with 8 bytes to make sure there's enough */
  s = (char *) malloc(strlen(str) + 9);
  strcpy(s, str);
  if ((!key) || (!key[0]))
    return s;
  p = s;
  dest = (char *) malloc((strlen(str) + 9) * 2);
  while (*p)
    p++;
  for (i = 0; i < 8; i++)
    *p++ = 0;
  blowfish_init((unsigned char *) key, strlen(key));
  p = s;
  d = dest;
  while (*p) {
    left = ((*p++) << 24);
    left += ((*p++) << 16);
    left += ((*p++) << 8);
    left += (*p++);
    right = ((*p++) << 24);
    right += ((*p++) << 16);
    right += ((*p++) << 8);
    right += (*p++);
    blowfish_encipher(&left, &right);
    for (i = 0; i < 6; i++) {
      *d++ = base64[right & 0x3f];
      right = (right >> 6);
    }
    for (i = 0; i < 6; i++) {
      *d++ = base64[left & 0x3f];
      left = (left >> 6);
    }
  }
  *d = 0;
  free(s);
  return dest;
}

/* Returned string must be freed when done with it!
 */
static char *decrypt_string(char *key, char *str)
{
  u_32int_t left, right;
  char *p, *s, *dest, *d;
  int i;

  /* Pad encoded string with 0 bits in case it's bogus */
  s = (char *) malloc(strlen(str) + 12);
  strcpy(s, str);
  if ((!key) || (!key[0]))
    return s;
  p = s;
  dest = (char *) malloc(strlen(str) + 12);
  while (*p)
    p++;
  for (i = 0; i < 12; i++)
    *p++ = 0;
  blowfish_init((unsigned char *) key, strlen(key));
  p = s;
  d = dest;
  while (*p) {
    right = 0L;
    left = 0L;
    for (i = 0; i < 6; i++)
      right |= (base64dec(*p++)) << (i * 6);
    for (i = 0; i < 6; i++)
      left |= (base64dec(*p++)) << (i * 6);
    blowfish_decipher(&left, &right);
    for (i = 0; i < 4; i++)
      *d++ = (left & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
    for (i = 0; i < 4; i++)
      *d++ = (right & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
  }
  *d = 0;
  free(s);
  return dest;
}

static char *script_encpass(char *pass)
{
	char *buf;

	buf = (char *)malloc(17);
	blowfish_encrypt_pass(pass, buf);
	return(buf);
}

static script_command_t blowfish_script_cmds[] = {
	{"", "encpass", (Function) script_encpass, NULL, 1, "s", "string", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "encrypt", (Function) encrypt_string, NULL, 2, "ss", "key string", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "decrypt", (Function) decrypt_string, NULL, 2, "ss", "key string", SCRIPT_STRING | SCRIPT_FREE, 0},
	{0}
};

/* You CANT -module an encryption module , so -module just resets it.
 */
static char *blowfish_close()
{
  return _("You can't unload an encryption module");
}

EXPORT_SCOPE char *start(eggdrop_t *);

static Function blowfish_table[] =
{
  /* 0 - 3 */
  (Function) start,
  (Function) blowfish_close,
  (Function) 0,
  (Function) blowfish_report,
  /* 4 - 7 */
  (Function) encrypt_string,
  (Function) decrypt_string,
};

char *start(eggdrop_t *eggdrop)
{
  int i;

  /* `global_funcs' is NULL if eggdrop is recovering from a restart.
   *
   * As the encryption module is never unloaded, only initialise stuff
   * that got reset during restart, e.g. the tcl bindings.
   */
  if (eggdrop) {
    egg = eggdrop;

    if (!module_rename("blowfish", MODULE_NAME))
      return _("Already loaded.");
    /* Initialize buffered boxes */
    for (i = 0; i < BOXES; i++) {
      box[i].P = NULL;
      box[i].S = NULL;
      box[i].key[0] = 0;
      box[i].lastuse = 0L;
    }
    module_register(MODULE_NAME, blowfish_table, 2, 1);
    if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
      module_undepend(MODULE_NAME);
      return _("This module requires eggdrop1.7.0 or later");
    }
    add_hook(HOOK_ENCRYPT_PASS, (Function) blowfish_encrypt_pass);
    add_hook(HOOK_ENCRYPT_STRING, (Function) encrypt_string);
    add_hook(HOOK_DECRYPT_STRING, (Function) decrypt_string);
  }
  script_create_commands(blowfish_script_cmds);
  return NULL;
}

