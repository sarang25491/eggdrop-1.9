/* partymember.h: header for partyline.c
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
 * $Id: partymember.h,v 1.3 2004/10/17 05:14:06 stdarg Exp $
 */

#ifndef _EGG_PARTYMEMBER_H
#define _EGG_PARTYMEMBER_H

struct partymember {
        partymember_t *next;
	partymember_t *prev;

        int pid;
        char *nick, *ident, *host;
        user_t *user;
        int flags;

        partychan_t **channels;
        int nchannels;

        partyline_event_t *handler;
        void *client_data;
};

partymember_t *partymember_lookup_pid(int pid);
partymember_t *partymember_lookup_nick(const char *nick);
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partymember_delete(partymember_t *p, const char *text);
int partymember_update_info(partymember_t *p, const char *ident, const char *host);
int partymember_who(int **pids, int *len);
int partymember_write_pid(int pid, const char *text, int len);
int partymember_write(partymember_t *p, const char *text, int len);
int partymember_msg(partymember_t *p, partymember_t *src, const char *text, int len);
int partymember_printf_pid(int pid, const char *fmt, ...);
int partymember_printf(partymember_t *p, const char *fmt, ...);
int partymember_set_nick(partymember_t *p, const char *nick);

#endif /* !_EGG_PARTYMEMBER_H */

