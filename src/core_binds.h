/*
 * core_binds.h --
 */
/*
 * Copyright (C) 2002, 2003 Eggheads Development Team
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

#ifndef _CORE_BINDS_H_
#define _CORE_BINDS_H_

void core_binds_init();
void check_bind_time(struct tm *tm);
void check_bind_event(char *event);
void check_bind_dcc(const char *, int, const char *);
void check_bind_chjn(const char *, const char *, int, char, int, const char *);
void check_bind_chpt(const char *, const char *, int, int);
void check_bind_bot(const char *, const char *, const char *);
void check_bind_link(const char *, const char *);
void check_bind_disc(const char *);
const char *check_bind_filt(int, const char *);
int check_bind_note(const char *, const char *, const char *);
void check_bind_listen(const char *, int);
void check_bind_time(struct tm *);
void check_bind_nkch(const char *, const char *);
void check_bind_away(const char *, int, const char *);
int check_bind_chat(const char *, int, const char *);
void check_bind_act(const char *, int, const char *);
void check_bind_bcst(const char *, int, const char *);
void check_bind_chon(char *, int);
void check_bind_chof(char *, int);

#endif				/* !_CORE_BINDS_H_ */
