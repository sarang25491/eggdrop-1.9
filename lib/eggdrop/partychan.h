/* partychan.h: header for partychan.c
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
 * $Id: partychan.h,v 1.2 2004/10/17 05:14:06 stdarg Exp $
 */

#ifndef _EGG_PARTYCHAN_H
#define _EGG_PARTYCHAN_H

struct partychan_member {
        partymember_t *p;
        int flags;
};

struct partychan {
        partychan_t *next;
	partychan_t *prev;

        int cid;
        char *name;
        int flags;

        partychan_member_t *members;
        int nmembers;
};

struct partymember_common {
        partymember_common_t *next;
        partymember_t **members;
        int len;
        int max;
};

/* Channel functions. */
partychan_t *partychan_new(int cid, const char *name);
void partychan_delete(partychan_t *chan);
partychan_t *partychan_lookup_cid(int cid);
partychan_t *partychan_lookup_name(const char *name);
partychan_t *partychan_get_default(partymember_t *p);
int partychan_ison_name(const char *chan, partymember_t *p);
int partychan_ison(partychan_t *chan, partymember_t *p);
int partychan_join_name(const char *chan, partymember_t *p);
int partychan_join_cid(int cid, partymember_t *p);
int partychan_join(partychan_t *chan, partymember_t *p);
int partychan_part_name(const char *chan, partymember_t *p, const char *text);
int partychan_part_cid(int cid, partymember_t *p, const char *text);
int partychan_part(partychan_t *chan, partymember_t *p, const char *text);
int partychan_msg_name(const char *name, partymember_t *src, const char *text, int len);
int partychan_msg_cid(int cid, partymember_t *src, const char *text, int len);
int partychan_msg(partychan_t *chan, partymember_t *src, const char *text, int len);
partymember_common_t *partychan_get_common(partymember_t *p);
int partychan_free_common(partymember_common_t *common);

#endif /* !_EGG_PARTYCHAN_H */

