/*
 * dns.c -- handles:
 *   DNS resolve calls and events
 *   provides the code used by the bot if the DNS module is not loaded
 *   DNS Tcl commands
 *
 * $Id: dns.c,v 1.23 2001/07/26 17:04:33 drummer Exp $
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

extern struct dcc_t	*dcc;
extern int		 dcc_total;
extern int		 resolve_timeout;
extern time_t		 now;
extern jmp_buf		 alarmret;
extern Tcl_Interp	*interp;

devent_t		*dns_events = NULL;

extern adns_state	 ads;
extern int		 af_preferred;

/*
 *   DCC functions
 */

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

static int expmem_dcc_dnswait(void *x)
{
  register struct dns_info *p = (struct dns_info *) x;
  int size = 0;

  if (p) {
    size = sizeof(struct dns_info);
    if (p->host)
      size += strlen(p->host) + 1;
    if (p->cbuf)
      size += strlen(p->cbuf) + 1;
  }
  return size;
}

static void kill_dcc_dnswait(int idx, void *x)
{
  register struct dns_info *p = (struct dns_info *) x;

  if (p) {
    if (p->host)
      nfree(p->host);
    if (p->cbuf)
      nfree(p->cbuf);
    nfree(p);
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
  expmem_dcc_dnswait,
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
        (!egg_strcasecmp(dcc[idx].u.dns->host, ip))) {
debug3("|DNS| idx: %d, dcchostbyip: %s is %s", idx, ip, hostn);
      nfree(dcc[idx].u.dns->host);
      dcc[idx].u.dns->host = get_data_ptr(strlen(hostn) + 1);
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
        !egg_strcasecmp(dcc[idx].u.dns->host, hostn)) {
debug3("|DNS| idx: %d, dccipbyhost: %s is %s", idx, ip, hostn);
      nfree(dcc[idx].u.dns->host);
      dcc[idx].u.dns->host = get_data_ptr(strlen(ip) + 1);
      strcpy(dcc[idx].u.dns->host, ip);
      if (ok)
        dcc[idx].u.dns->dns_success(idx);
      else
        dcc[idx].u.dns->dns_failure(idx);
    }
  }
}

static int dns_dccexpmem(void *other)
{
  return 0;
}

devent_type DNS_DCCEVENT_HOSTBYIP = {
  "DCCEVENT_HOSTBYIP",
  dns_dccexpmem,
  dns_dcchostbyip
};

devent_type DNS_DCCEVENT_IPBYHOST = {
  "DCCEVENT_IPBYHOST",
  dns_dccexpmem,
  dns_dccipbyhost
};

void dcc_dnsipbyhost(char *hostn)
{
  devent_t *de;

  for (de = dns_events; de; de = de->next) {
    if (de->type && (de->type == &DNS_DCCEVENT_IPBYHOST) &&
	(de->lookup == RES_IPBYHOST)) {
      if (de->hostname &&
	  !egg_strcasecmp(de->hostname, hostn))
	/* No need to add anymore. */
	return;
    }
  }

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_DCCEVENT_IPBYHOST;
  de->lookup = RES_IPBYHOST;
  de->hostname = nmalloc(strlen(hostn) + 1);
  strcpy(de->hostname, hostn);

  /* Send request. */
  dns_ipbyhost(hostn);
}

void dcc_dnshostbyip(char *ip)
{
  devent_t *de;
  
  for (de = dns_events; de; de = de->next) {
    if (de->type && (de->type == &DNS_DCCEVENT_HOSTBYIP) &&
	(de->lookup == RES_HOSTBYIP)) {
      if (!egg_strcasecmp(de->hostname, ip))
	/* No need to add anymore. */
	return;
    }
  }

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_DCCEVENT_HOSTBYIP;
  de->lookup = RES_HOSTBYIP;
  
  de->hostname = nmalloc(strlen(ip) + 1);
  strcpy(de->hostname, ip);

  /* Send request. */
  dns_hostbyip(ip);
}


/*
 *   Tcl events
 */

static void dns_tcl_iporhostres(char *ip, char *hostn, int ok, void *other)
{
  devent_tclinfo_t *tclinfo = (devent_tclinfo_t *) other;

  if (Tcl_VarEval(interp, tclinfo->proc, " ", ip, " ",
		  hostn, ok ? " 1" : " 0", tclinfo->paras, NULL) == TCL_ERROR)
    putlog(LOG_MISC, "*", DCC_TCLERROR, tclinfo->proc, interp->result);

  /* Free the memory. It will be unused after this event call. */
  nfree(tclinfo->proc);
  if (tclinfo->paras)
    nfree(tclinfo->paras);
  nfree(tclinfo);
}

static int dns_tclexpmem(void *other)
{
  devent_tclinfo_t *tclinfo = (devent_tclinfo_t *) other;
  int l = 0;

  if (tclinfo) {
    l = sizeof(devent_tclinfo_t);
    if (tclinfo->proc)
      l += strlen(tclinfo->proc) + 1;
    if (tclinfo->paras)
      l += strlen(tclinfo->paras) + 1;
  }
  return l;
}

devent_type DNS_TCLEVENT_HOSTBYIP = {
  "TCLEVENT_HOSTBYIP",
  dns_tclexpmem,
  dns_tcl_iporhostres
};

devent_type DNS_TCLEVENT_IPBYHOST = {
  "TCLEVENT_IPBYHOST",
  dns_tclexpmem,
  dns_tcl_iporhostres
};

static void tcl_dnsipbyhost(char *hostn, char *proc, char *paras)
{
  devent_t *de;
  devent_tclinfo_t *tclinfo;

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_TCLEVENT_IPBYHOST;
  de->lookup = RES_IPBYHOST;
  de->hostname = nmalloc(strlen(hostn) + 1);
  strcpy(de->hostname, hostn);

  /* Store additional data. */
  tclinfo = nmalloc(sizeof(devent_tclinfo_t));
  tclinfo->proc = nmalloc(strlen(proc) + 1);
  strcpy(tclinfo->proc, proc);
  if (paras) {
    tclinfo->paras = nmalloc(strlen(paras) + 1);
    strcpy(tclinfo->paras, paras);
  } else
    tclinfo->paras = NULL;
  de->other = tclinfo;

  /* Send request. */
  dns_ipbyhost(hostn);
}

static void tcl_dnshostbyip(char *ip, char *proc, char *paras)
{
  devent_t *de;
  devent_tclinfo_t *tclinfo;

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_TCLEVENT_HOSTBYIP;
  de->lookup = RES_HOSTBYIP;
  de->hostname = nmalloc(strlen(ip) + 1);
  strcpy(de->hostname, ip);

  /* Store additional data. */
  tclinfo = nmalloc(sizeof(devent_tclinfo_t));
  tclinfo->proc = nmalloc(strlen(proc) + 1);
  strcpy(tclinfo->proc, proc);
  if (paras) {
    tclinfo->paras = nmalloc(strlen(paras) + 1);
    strcpy(tclinfo->paras, paras);
  } else
    tclinfo->paras = NULL;
  de->other = tclinfo;

  /* Send request. */
  dns_hostbyip(ip);
}


/*
 *    Event functions
 */

inline static int dnsevent_expmem(void)
{
  devent_t *de;
  int tot = 0;
  
  for (de = dns_events; de; de = de->next) {
    tot += sizeof(devent_t);
    if (de->hostname)
      tot += strlen(de->hostname) + 1;
    if (de->type && de->type->expmem)
      tot += de->type->expmem(de->other);
  }
  return tot;
}

void call_hostbyip(char *ip, char *hostn, int ok)
{
  devent_t *de = dns_events, *ode = NULL, *nde = NULL;

  while (de) {
    nde = de->next;
    if ((de->lookup == RES_HOSTBYIP) &&
	(!de->hostname || !(de->hostname[0]) ||
	(!egg_strcasecmp(de->hostname, ip)))) {
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
          nfree(de->hostname);
      nfree(de);
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
	 !egg_strcasecmp(de->hostname, hostn))) {
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
	nfree(de->hostname);
      nfree(de);
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
    if (egg_inet_aton(ip, &addr))
	hp = gethostbyaddr((char *) &addr, sizeof addr, AF_INET);
#ifdef IPV6
    else if (egg_inet_pton(AF_INET6, ip, &addr6))
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
  if (egg_inet_aton(host, &inaddr)) {
    call_ipbyhost(host, host, 1);
    return;
  }
#ifdef IPV6
  /* Check if someone passed us an IPv6 address as hostname... */
  if (egg_inet_pton(AF_INET6, host, &in6addr)) {
    call_ipbyhost(host, host, 1);
    return;
  }
#endif
  if (!setjmp(alarmret)) {
    struct hostent *hp;
    char *p;
    int type;

    if (!egg_strncasecmp("ipv6%", host, 5)) {
	type = AF_INET6;
	p = host + 5;
debug1("|DNS| checking only AAAA record for %s", p);
    } else if (!egg_strncasecmp("ipv4%", host, 5)) {
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
      egg_inet_ntop(hp->h_addrtype, hp->h_addr_list[0], tmp, ADDRLEN-1);
      call_ipbyhost(host, tmp, 1);
      return;
    }
    /* Fall through. */
  }
  call_ipbyhost(host, "0.0.0.0", 0);
}

#endif

/*
 *   Misc functions
 */

int expmem_dns(void)
{
  return dnsevent_expmem();
}


/*
 *   Tcl functions
 */

/* dnslookup <ip-address> <proc> */
static int tcl_dnslookup STDVAR
{
  struct in_addr inaddr;
#ifdef IPV6
  struct in6_addr inaddr6;
#endif
  char *paras = NULL;

  if (argc < 3) {
    Tcl_AppendResult(irp, "wrong # args: should be \"", argv[0],
		     " ip-address/hostname proc ?args...?\"", NULL);
    return TCL_ERROR;
  }

  if (argc > 3) {
    int l = 0, p;

    /* Create a string with a leading space out of all provided
     * additional parameters.
     */
    paras = nmalloc(1);
    paras[0] = 0;
    for (p = 3; p < argc; p++) {
      l += strlen(argv[p]) + 1;
      paras = nrealloc(paras, l + 1);
      strcat(paras, " ");
      strcat(paras, argv[p]);
    }
  }

  if (egg_inet_aton(argv[1], &inaddr)
#ifdef IPV6
	|| egg_inet_pton(AF_INET6, argv[1], &inaddr6)
#endif
      )
    tcl_dnshostbyip(argv[1], argv[2], paras);
  else
    tcl_dnsipbyhost(argv[1], argv[2], paras);
  if (paras)
    nfree(paras);
  return TCL_OK;
}

tcl_cmds tcldns_cmds[] =
{
  {"dnslookup",	tcl_dnslookup},
  {NULL,	NULL}
};

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

    if (egg_inet_pton(AF_INET, ip, &in.sin_addr)) {
debug1("|DNS| adns_dns_hostbyip(\"%s\") (IPv4)", ip);
	origname = nmalloc(strlen(ip) + 1);
	strcpy(origname, ip);
	in.sin_family = AF_INET;
	in.sin_port = 0;
	r = adns_submit_reverse(ads, (struct sockaddr *) &in,
	    adns_r_ptr, 0, origname, &q);
	if (r) {
debug1("|DNS| adns_submit_reverse failed, errno: %d", r);
	    call_hostbyip(ip, ip, 0);
	}
#ifdef IPV6
    } else if (egg_inet_pton(AF_INET6, ip, &in6.sin6_addr)) {
debug1("|DNS| adns_dns_hostbyip(\"%s\") (IPv6)", ip);
	origname = nmalloc(strlen(ip) + 1);
	strcpy(origname, ip);
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

    if (egg_inet_pton(AF_INET, host, &in.sin_addr)
#ifdef IPV6
	|| egg_inet_pton(AF_INET6, host, &in6.sin6_addr)
#endif
	) {
	/* It's an IP! */
	call_ipbyhost(host, host, 1);
    } else {
	char *p;
	char *origname;
	int type, r;
	origname = nmalloc(strlen(host) + 1);
	strcpy(origname, host);
	if (!egg_strncasecmp("ipv6%", host, 5)) {
#ifdef IPV6
	    type = adns_r_addr6;
	    p = host + 5;
debug1("|DNS| checking only AAAA record for %s", p);
#else
debug1("|DNS| compiled without IPv6 support, can't resolv %s", host);
	    call_ipbyhost(host, "0.0.0.0", 0);
	    return;
#endif
	} else if (!egg_strncasecmp("ipv4%", host, 5)) {
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
