/*
 * dns.h
 *   stuff used by dns.c
 *
 * $Id: dns.h,v 1.10 2002/02/07 22:19:05 wcc Exp $
 */
/*
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Eggheads Development Team
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

#ifndef _EGG_DNS_H
#define _EGG_DNS_H

/* Flags for dns_type
 */
#define RES_HOSTBYIP  1         /* hostname to IP address               */
#define RES_IPBYHOST  2         /* IP address to hostname               */

#ifndef MAKING_MODS

void dns_hostbyip(char *);
void dns_ipbyhost(char *);
void call_hostbyip(char *, char *, int);
void call_ipbyhost(char *, char *, int);
void dcc_dnshostbyip(char *);
void dcc_dnsipbyhost(char *);

#endif /* MAKING_MODS */

#endif	/* _EGG_DNS_H */
