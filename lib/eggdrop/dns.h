/* dns.h: header for dns.c
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
 * $Id: dns.h,v 1.3 2006/09/12 01:50:50 sven Exp $
 */

#ifndef _EGG_DNS_H_
#define _EGG_DNS_H_

#define DNS_IPV4	1
#define DNS_IPV6	2
#define DNS_REVERSE	3

#define DNS_PORT 53

typedef int (*dns_callback_t)(void *client_data, const char *query, char **result);

int egg_dns_init(void);
int egg_dns_shutdown(void);

void egg_dns_send(char *query, int len);
int egg_dns_lookup(const char *host, int timeout, dns_callback_t callback, void *client_data, event_owner_t *owner);
int egg_dns_reverse(const char *ip, int timeout, dns_callback_t callback, void *client_data, event_owner_t *owner);
int egg_dns_cancel(int id, int issue_callback);

#endif /* !_EGG_DNS_H_ */
