/*
 * dns.c -- handles:
 *   DNS resolve calls and events
 *   provides the code used by the bot if the DNS module is not loaded
 *   DNS script commands
 *
 * $Id: dns.c,v 1.29 2001/10/19 01:55:05 tothwolf Exp $
 */
/*
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

#include "main.h"
#include <netdb.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dns.h"
#include "adns/adns.h"
#include "script_api.h"
#include "script.h"

extern struct dcc_t	*dcc;
extern int		 dcc_total;
extern int		 resolve_timeout;
extern time_t		 now;
extern jmp_buf		 alarmret;

devent_t		*dns_events = NULL;

extern adns_state	 ads;
extern int		 af_preferred;

/*
 *   DCC functions
 */

static int script_dnslookup(char *iporhost, script_callback_t *callback);

static script_simple_command_t scriptdns_cmds[] = {
	{"", NULL, NULL, NULL, 0},
	{"dnslookup", script_dnslookup, "sc", "ip-address/hostname callback", SCRIPT_INTEGER},
	0
};

void dns_init()
{
	script_create_simple_cmd_table(scriptdns_cmds);
}

void dcc_dnswait(int idx, char *buf, int len)
{
  /* Ignore anything now. */
}

void eof_dcc_dnswait(int idx)
{
  putlog(LOG_MISC, "*", "Lost connection while resolving hostname [%s/%d]",
	 dcc[idx].addr, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_dnswait(int idx, char *buf)
{
  sprintf(buf, "dns   waited %lus", now - dcc[idx].timeval);
}

static void kill_dcc_dnswait(int idx, void *x)
{
  register struct dns_info *p = (struct dns_info *) x;

  if (p) {
    if (p->host)
      free(p->host);
    if (p->cbuf)
      free(p->cbuf);
    free(p);
  }
}

struct dcc_table DCC_DNSWAIT =
{
  "DNSWAIT",
  DCT_VALIDIDX,
  eof_dcc_dnswait,
  dcc_dnswait,
  0,
  0,
  display_dcc_dnswait,
  kill_dcc_dnswait,
  0
};


/*
 *   DCC events
 */

/* Walk through every dcc entry and look for waiting DNS requests
 * of RES_HOSTBYIP for our IP address.
 */
static void dns_dcchostbyip(char *ip, char *hostn, int ok, void *other)
{
  int idx;

  for (idx = 0; idx < dcc_total; idx++) {
    if ((dcc[idx].type == &DCC_DNSWAIT) &&
        (dcc[idx].u.dns->dns_type == RES_HOSTBYIP) &&
        (!strcasecmp(dcc[idx].u.dns->host, ip))) {
debug3("|DNS| idx: %d, dcchostbyip: %s is %s", idx, ip, hostn);
      free(dcc[idx].u.dns->host);
      dcc[idx].u.dns->host = calloc(1, strlen(hostn) + 1);
      strcpy(dcc[idx].u.dns->host, hostn);
      if (ok)
        dcc[idx].u.dns->dns_success(idx);
      else
        dcc[idx].u.dns->dns_failure(idx);
    }
  }
}

/* Walk through every dcc entry and look for waiting DNS requests
 * of RES_IPBYHOST for our hostname.
 */
static void dns_dccipbyhost(char *ip, char *hostn, int ok, void *other)
{
  int idx;

  for (idx = 0; idx < dcc_total; idx++) {
    if ((dcc[idx].type == &DCC_DNSWAIT) &&
        (dcc[idx].u.dns->dns_type == RES_IPBYHOST) &&
        !strcasecmp(dcc[idx].u.dns->host, hostn)) {
debug3("|DNS| idx: %d, dccipbyhost: %s is %s", idx, ip, hostn);
      free(dcc[idx].u.dns->host);
      dcc[idx].u.dns->host = calloc(1, strlen(ip) + 1);
      strcpy(dcc[idx].u.dns->host, ip);
      if (ok)
        dcc[idx].u.dns->dns_success(idx);
      else
        dcc[idx].u.dns->dns_failure(idx);
    }
  }
}

devent_type DNS_DCCEVENT_HOSTBYIP = {
  "DCCEVENT_HOSTBYIP",
  dns_dcchostbyip
};

devent_type DNS_DCCEVENT_IPBYHOST = {
  "DCCEVENT_IPBYHOST",
  dns_dccipbyhost
};

void dcc_dnsipbyhost(char *hostn)
{
  devent_t *de;

  for (de = dns_events; de; de = de->next) {
    if (de->type && (de->type == &DNS_DCCEVENT_IPBYHOST) &&
	(de->lookup == RES_IPBYHOST)) {
      if (de->hostname &&
	  !strcasecmp(de->hostname, hostn))
	/* No need to add anymore. */
	return;
    }
  }

  de = calloc(1, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_DCCEVENT_IPBYHOST;
  de->lookup = RES_IPBYHOST;
  malloc_strcpy(de->hostname, hostn);

  /* Send request. */
  dns_ipbyhost(hostn);
}

void dcc_dnshostbyip(char *ip)
{
  devent_t *de;
  
  for (de = dns_events; de; de = de->next) {
    if (de->type && (de->type == &DNS_DCCEVENT_HOSTBYIP) &&
	(de->lookup == RES_HOSTBYIP)) {
      if (!strcasecmp(de->hostname, ip))
	/* No need to add anymore. */
	return;
    }
  }

  de = calloc(1, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_DCCEVENT_HOSTBYIP;
  de->lookup = RES_HOSTBYIP;
  malloc_strcpy(de->hostname, ip);

  /* Send request. */
  dns_hostbyip(ip);
}


/*
 *   Script events
 */

static void dns_script_iporhostres(char *ip, char *hostn, int ok, void *other)
{
  script_callback_t *callback = (script_callback_t *)other;

  callback->callback(callback, ip, hostn, ok);
  callback->delete(callback);
}

devent_type DNS_SCRIPTEVENT_HOSTBYIP = {
  "SCRIPTEVENT_HOSTBYIP",
  dns_script_iporhostres
};

devent_type DNS_SCRIPTEVENT_IPBYHOST = {
  "SCRIPTEVENT_IPBYHOST",
  dns_script_iporhostres
};

static void script_dnsipbyhost(char *hostn, script_callback_t *callback)
{
  devent_t *de;

  de = calloc(1, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_SCRIPTEVENT_IPBYHOST;
  de->lookup = RES_IPBYHOST;
  malloc_strcpy(de->hostname, hostn);
  malloc_strcpy(callback->syntax, "ssi");
  de->other = callback;

  /* Send request. */
  dns_ipbyhost(hostn);
}

static void script_dnshostbyip(char *ip, script_callback_t *callback)
{
  devent_t *de;

  de = calloc(1, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_SCRIPTEVENT_HOSTBYIP;
  de->lookup = RES_HOSTBYIP;
  malloc_strcpy(de->hostname, ip);
  malloc_strcpy(callback->syntax, "ssi");
  de->other = callback;

  /* Send request. */
  dns_hostbyip(ip);
}


/*
 *    Event functions
 */

void call_hostbyip(char *ip, char *hostn, int ok)
{
  devent_t *de = dns_events, *ode = NULL, *nde = NULL;

  while (de) {
    nde = de->next;
    if ((de->lookup == RES_HOSTBYIP) &&
	(!de->hostname || !(de->hostname[0]) ||
	(!strcasecmp(de->hostname, ip)))) {
      /* Remove the event from the list here, to avoid conflicts if one of
       * the event handlers re-adds another event. */
      if (ode)
	ode->next = de->next;
      else
	dns_events = de->next;
debug3("|DNS| call_hostbyip: ip: %s host: %s ok: %d", ip, hostn, ok);
      if (de->type && de->type->event)
	de->type->event(ip, hostn, ok, de->other);
      else
	putlog(LOG_MISC, "*", "(!) Unknown DNS event type found: %s",
	       (de->type && de->type->name) ? de->type->name : "<empty>");
      if (de->hostname)
          free(de->hostname);
      free(de);
      de = ode;
    }
    ode = de;
    de = nde;
  }
}

void call_ipbyhost(char *hostn, char *ip, int ok)
{
  devent_t *de = dns_events, *ode = NULL, *nde = NULL;

  while (de) {
    nde = de->next;
    if ((de->lookup == RES_IPBYHOST) &&
	(!de->hostname || !(de->hostname[0]) ||
	 !strcasecmp(de->hostname, hostn))) {
      /* Remove the event from the list here, to avoid conflicts if one of
       * the event handlers re-adds another event. */
      if (ode)
	ode->next = de->next;
      else
	dns_events = de->next;

      if (de->type && de->type->event)
	de->type->event(ip, hostn, ok, de->other);
      else
	putlog(LOG_MISC, "*", "(!) Unknown DNS event type found: %s",
	       (de->type && de->type->name) ? de->type->name : "<empty>");

      if (de->hostname)
	free(de->hostname);
      free(de);
      de = ode;
    }
    ode = de;
    de = nde;
  }
}

#ifdef DISABLE_ADNS
/*
 *    Async DNS emulation functions
 */

void dns_hostbyip(char *ip)
{
  struct hostent *hp = 0;
  struct in_addr addr;
#ifdef IPV6
  struct in6_addr addr6;
#endif
  static char s[UHOSTLEN];

  if (!setjmp(alarmret)) {
    alarm(resolve_timeout);
    if (inet_aton(ip, &addr))
	hp = gethostbyaddr((char *) &addr, sizeof addr, AF_INET);
#ifdef IPV6
    else if (inet_pton(AF_INET6, ip, &addr6))
	hp = gethostbyaddr((char *) &addr6, sizeof addr6, AF_INET6);
#endif
    else
	hp = 0;
    alarm(0);
    if (hp)
      strncpyz(s, hp->h_name, sizeof s);
  } else
    hp = 0;
  if (!hp)
    strncpyz(s, ip, sizeof s);
debug2("|DNS| block_dns_hostbyip: ip: %s -> host: %s", ip, s);
  /* Call hooks. */
  call_hostbyip(ip, s, hp ? 1 : 0);
}

void block_dns_ipbyhost(char *host)
{
  struct in_addr inaddr;
#ifdef IPV6
  struct in6_addr in6addr;
#endif

  /* Check if someone passed us an IPv4 address as hostname
   * and return it straight away */
  if (inet_aton(host, &inaddr)) {
    call_ipbyhost(host, host, 1);
    return;
  }
#ifdef IPV6
  /* Check if someone passed us an IPv6 address as hostname... */
  if (inet_pton(AF_INET6, host, &in6addr)) {
    call_ipbyhost(host, host, 1);
    return;
  }
#endif
  if (!setjmp(alarmret)) {
    struct hostent *hp;
    char *p;
    int type;

    if (!strncasecmp("ipv6%", host, 5)) {
	type = AF_INET6;
	p = host + 5;
debug1("|DNS| checking only AAAA record for %s", p);
    } else if (!strncasecmp("ipv4%", host, 5)) {
	type = AF_INET;
	p = host + 5;
debug1("|DNS| checking only A record for %s", p);
    } else {
	type = AF_INET; /* af_preferred */
	p = host;
    }

    alarm(resolve_timeout);
#ifndef IPV6
    hp = gethostbyname(p);
#else
    hp = gethostbyname2(p, type);
    if (!hp && (p == host))
	hp = gethostbyname2(p, (type == AF_INET6 ? AF_INET : AF_INET6));
#endif
    alarm(0);

    if (hp) {
      char tmp[ADDRLEN];
      inet_ntop(hp->h_addrtype, hp->h_addr_list[0], tmp, ADDRLEN-1);
      call_ipbyhost(host, tmp, 1);
      return;
    }
    /* Fall through. */
  }
  call_ipbyhost(host, "0.0.0.0", 0);
}

#endif

/*
 *   Script functions
 */

/* dnslookup <ip-address> <proc> */
static int script_dnslookup(char *iporhost, script_callback_t *callback)
{
  struct in_addr inaddr;
#ifdef IPV6
  struct in6_addr inaddr6;
#endif

  if (inet_aton(iporhost, &inaddr)
#ifdef IPV6
	|| inet_pton(AF_INET6, iporhost, &inaddr6)
#endif
      )
    script_dnshostbyip(iporhost, callback);
  else
    script_dnsipbyhost(iporhost, callback);
  return(0);
}


/*********** ADNS support ***********/

#ifndef DISABLE_ADNS

void dns_hostbyip(char *ip)
{
    adns_query q;
    int r;
    struct sockaddr_in in;
#ifdef IPV6
    struct sockaddr_in6 in6;
#endif
    char *origname;

    if (inet_pton(AF_INET, ip, &in.sin_addr)) {
debug1("|DNS| adns_dns_hostbyip(\"%s\") (IPv4)", ip);
	malloc_strcpy(origname, ip);
	in.sin_family = AF_INET;
	in.sin_port = 0;
	r = adns_submit_reverse(ads, (struct sockaddr *) &in,
	    adns_r_ptr, 0, origname, &q);
	if (r) {
debug1("|DNS| adns_submit_reverse failed, errno: %d", r);
	    call_hostbyip(ip, ip, 0);
	}
#ifdef IPV6
    } else if (inet_pton(AF_INET6, ip, &in6.sin6_addr)) {
debug1("|DNS| adns_dns_hostbyip(\"%s\") (IPv6)", ip);
	malloc_strcpy(origname, ip);
	in6.sin6_family = AF_INET6;
	in6.sin6_port = 0;
	r = adns_submit_reverse(ads, (struct sockaddr *) &in6,
	    adns_r_ptr_ip6, 0, origname, &q);
	if (r) {
debug1("|DNS| adns_submit_reverse failed, errno: %d", r);
	    call_hostbyip(ip, ip, 0);
	}
#endif
     } else {
debug1("|DNS| adns_dns_hostbyip: got invalid ip: %s", ip);
	call_hostbyip(ip, ip, 0);
    }
}

void dns_ipbyhost(char *host)
{
    adns_query q4;
    struct sockaddr_in in;
#ifdef IPV6
    struct sockaddr_in6 in6;
#endif
    
debug1("|DNS| adns_dns_ipbyhost(\"%s\");", host);

    if (inet_pton(AF_INET, host, &in.sin_addr)
#ifdef IPV6
	|| inet_pton(AF_INET6, host, &in6.sin6_addr)
#endif
	) {
	/* It's an IP! */
	call_ipbyhost(host, host, 1);
    } else {
	char *p;
	char *origname;
	int type, r;
	malloc_strcpy(origname, host);
	if (!strncasecmp("ipv6%", host, 5)) {
#ifdef IPV6
	    type = adns_r_addr6;
	    p = host + 5;
debug1("|DNS| checking only AAAA record for %s", p);
#else
debug1("|DNS| compiled without IPv6 support, can't resolv %s", host);
	    call_ipbyhost(host, "0.0.0.0", 0);
	    return;
#endif
	} else if (!strncasecmp("ipv4%", host, 5)) {
	    type = adns_r_addr;
	    p = host + 5;
debug1("|DNS| checking only A record for %s", p);
	} else {
	    type = adns_r_addr; /* af_preferred */
	    p = host;
	}
	r = adns_submit(ads, p, type, 0, origname, &q4);
	if (r) {
debug1("|DNS| adns_submit failed, errno: %d", r);
	    call_ipbyhost(host, "0.0.0.0", 0);
	}
    }
}

#endif
