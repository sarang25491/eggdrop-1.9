/* 
 * net.c -- handles:
 *   all raw network i/o
 * 
 * $Id: net.c,v 1.39 2001/08/19 02:24:41 drummer Exp $
 */
/* 
 * This is hereby released into the public domain.
 * Robey Pointer, robey@netcom.com
 */

#include <fcntl.h>
#include "main.h"
#include <limits.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#if HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <setjmp.h>

#include "adns/adns.h"

#if !HAVE_GETDTABLESIZE
#  ifdef FD_SETSIZE
#    define getdtablesize() FD_SETSIZE
#  else
#    define getdtablesize() 200
#  endif
#endif

#ifndef IN6_IS_ADDR_V4MAPPED
#  define IN6_IS_ADDR_V4MAPPED(a) \
	((((uint32_t *) (a))[0] == 0) && (((uint32_t *) (a))[1] == 0) && \
	 (((uint32_t *) (a))[2] == htonl (0xffff)))
#endif

extern struct dcc_t	*dcc;
extern int		 backgrd, use_stderr, resolve_timeout, dcc_total;
extern unsigned long	 otraffic_irc_today, otraffic_bn_today,
			 otraffic_dcc_today, otraffic_filesys_today,
			 otraffic_trans_today, otraffic_unknown_today;
extern char		 natip[];

char	hostname[121] = "";	/* Hostname can be specified in the config
				   file					    */
char	myip[121] = "";		/* IP can be specified in the config file   */
char	myip6[121] = "";	/* IP6 can be specified in the config file  */
char	firewall[121] = "";	/* Socks server for firewall		    */
int	firewallport = 1080;	/* Default port of Sock4/5 firewalls	    */
char	botuser[21] = "eggdrop"; /* Username of the user running the bot    */
sock_list *socklist = NULL;	/* Enough to be safe			    */
int	MAXSOCKS = 0;
jmp_buf	alarmret;		/* Env buffer for alarm() returns	    */

adns_state ads;

/* Types of proxy */
#define PROXY_SOCKS   1
#define PROXY_SUN     2


/* I need an UNSIGNED long for dcc type stuff
 */
IP my_atoul(char *s)
{
  IP ret = 0;

  while ((*s >= '0') && (*s <= '9')) {
    ret *= 10;
    ret += ((*s) - '0');
    s++;
  }
  return ret;
}

/* Initialize the socklist
 */
void init_net()
{
  int i;

  for (i = 0; i < MAXSOCKS; i++) {
    socklist[i].flags = SOCK_UNUSED;
  }
  
  /* init ADNS */
  i = adns_init(&ads, adns_if_noautosys, 0);
  if (i)
      fatal(adns_strerror(i), 0);
}

int expmem_net()
{
  int i, tot = 0;

  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED)) {
      if (socklist[i].inbuf != NULL)
	tot += strlen(socklist[i].inbuf) + 1;
      if (socklist[i].outbuf != NULL)
	tot += socklist[i].outbuflen;
    }
  }
  return tot;
}

/* Get my ip number
 */
IP getmyip()
{
  struct hostent *hp;
  char s[121];
  IP ip;
  struct in_addr *in;

  /* Could be pre-defined */
  if (myip[0]) {
    if ((myip[strlen(myip) - 1] >= '0') && (myip[strlen(myip) - 1] <= '9'))
      return (IP) inet_addr(myip);
  }
  /* Also could be pre-defined */
  if (hostname[0])
    hp = gethostbyname(hostname);
  else {
    gethostname(s, 120);
    hp = gethostbyname(s);
  }
  if (hp == NULL)
    fatal("Hostname self-lookup failed.", 0);
  in = (struct in_addr *) (hp->h_addr_list[0]);
  ip = (IP) (in->s_addr);
  return ip;
}

#ifdef IPV6
struct in6_addr getmyip6()
{
  struct in6_addr ip;

  /* Could be pre-defined */
  if (myip6[0])
    egg_inet_pton(AF_INET6, myip6, &ip);
  else {
    /* get system's default IPv6 ip -- FIXME!! */
    /* there is know way?! - drummer */
    ip = in6addr_any;
  }
  return ip;
}

struct in6_addr ipv4to6(IP a)
{
    struct in6_addr ip;
    if (a == INADDR_ANY)
	return in6addr_any;
    else {
	((uint32_t *)&ip)[0] = 0;
	((uint32_t *)&ip)[1] = 0;
	((uint16_t *)&ip)[4] = 0;
	((uint16_t *)&ip)[5] = 0xffff;
	((uint32_t *)&ip)[3] = a;
    }
    return ip;
}

#endif

void neterror(char *s)
{
  switch (errno) {
  case EADDRINUSE:
    strcpy(s, "Address already in use");
    break;
  case EADDRNOTAVAIL:
    strcpy(s, "Address invalid on remote machine");
    break;
  case EAFNOSUPPORT:
    strcpy(s, "Address family not supported");
    break;
  case EALREADY:
    strcpy(s, "Socket already in use");
    break;
  case EBADF:
    strcpy(s, "Socket descriptor is bad");
    break;
  case ECONNREFUSED:
    strcpy(s, "Connection refused");
    break;
  case EFAULT:
    strcpy(s, "Namespace segment violation");
    break;
  case EINPROGRESS:
    strcpy(s, "Operation in progress");
    break;
  case EINTR:
    strcpy(s, "Timeout");
    break;
  case EINVAL:
    strcpy(s, "Invalid namespace");
    break;
  case EISCONN:
    strcpy(s, "Socket already connected");
    break;
  case ENETUNREACH:
    strcpy(s, "Network unreachable");
    break;
  case ENOTSOCK:
    strcpy(s, "File descriptor, not a socket");
    break;
  case ETIMEDOUT:
    strcpy(s, "Connection timed out");
    break;
  case ENOTCONN:
    strcpy(s, "Socket is not connected");
    break;
  case EHOSTUNREACH:
    strcpy(s, "Host is unreachable");
    break;
  case EPIPE:
    strcpy(s, "Broken pipe");
    break;
#ifdef ECONNRESET
  case ECONNRESET:
    strcpy(s, "Connection reset by peer");
    break;
#endif
#ifdef EACCES
  case EACCES:
    strcpy(s, "Permission denied");
    break;
#endif
  case 0:
    strcpy(s, "Error 0");
    break;
  default:
    sprintf(s, "Unforseen error %d", errno);
    break;
  }
}

/* Sets/Unsets options for a specific socket.
 * 
 * Returns:  0   - on success
 *           -1  - socket not found
 *           -2  - illegal operation
 */
int sockoptions(int sock, int operation, int sock_options)
{
  int i;

  for (i = 0; i < MAXSOCKS; i++)
    if ((socklist[i].sock == sock) && !(socklist[i].flags & SOCK_UNUSED)) {
      if (operation == EGG_OPTION_SET)
	      socklist[i].flags |= sock_options;
      else if (operation == EGG_OPTION_UNSET)
	      socklist[i].flags &= ~sock_options;
      else
	      return -2;
      return 0;
    }
  return -1;
}

/* Return a free entry in the socket entry
 */
int allocsock(int sock, int options)
{
  int i;

  for (i = 0; i < MAXSOCKS; i++) {
    if (socklist[i].flags & SOCK_UNUSED) {
      /* yay!  there is table space */
      socklist[i].inbuf = socklist[i].outbuf = NULL;
      socklist[i].inbuflen = socklist[i].outbuflen = 0;
      socklist[i].flags = options;
      socklist[i].sock = sock;
      return i;
    }
  }
  fatal("Socket table is full!", 0);
  return -1; /* Never reached */
}

/* Request a normal socket for i/o
 */
void setsock(int sock, int options)
{
  int i = allocsock(sock, options);
  int parm;

  if (((sock != STDOUT) || backgrd) &&
      !(socklist[i].flags & SOCK_NONSOCK)) {
    parm = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &parm, sizeof(int));

    parm = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *) &parm, sizeof(int));
  }
  if (options & SOCK_LISTEN) {
    /* Tris says this lets us grab the same port again next time */
    parm = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &parm, sizeof(int));
  }
  /* Yay async i/o ! */
  fcntl(sock, F_SETFL, O_NONBLOCK);
}

int getsock(int options)
{
#ifdef IPV6
  int sock = socket(AF_INET6, SOCK_STREAM, 0);
#else
  int sock = socket(AF_INET, SOCK_STREAM, 0);
#endif

  if (sock >= 0)
    setsock(sock, options);
  else
    putlog(LOG_MISC, "*", "Warning: Can't create new socket!");
  return sock;
}

/* Done with a socket
 */
void killsock(register int sock)
{
  register int	i;

  /* Ignore invalid sockets.  */
  if (sock < 0)
    return;
  for (i = 0; i < MAXSOCKS; i++) {
    if ((socklist[i].sock == sock) && !(socklist[i].flags & SOCK_UNUSED)) {
      close(socklist[i].sock);
      if (socklist[i].inbuf != NULL) {
	nfree(socklist[i].inbuf);
	socklist[i].inbuf = NULL;
      }
      if (socklist[i].outbuf != NULL) {
	nfree(socklist[i].outbuf);
	socklist[i].outbuf = NULL;
	socklist[i].outbuflen = 0;
      }
      socklist[i].flags = SOCK_UNUSED;
      return;
    }
  }
  putlog(LOG_MISC, "*", "Attempt to kill un-allocated socket %d !!", sock);
}

/* Send connection request to proxy
 */
static int proxy_connect(int sock, char *host, int port, int proxy)
{
  unsigned char x[10];
  struct hostent *hp;
  char s[256];
  int i;

  /* socks proxy */
  if (proxy == PROXY_SOCKS) {
    /* numeric IP? */
    if (host[strlen(host) - 1] >= '0' && host[strlen(host) - 1] <= '9') {
      IP ip = ((IP) inet_addr(host)); /* drummer */      
      egg_memcpy(x, &ip, 4);	/* Beige@Efnet */
    } else {
      /* no, must be host.domain */
      if (!setjmp(alarmret)) {
	alarm(resolve_timeout);
	hp = gethostbyname(host);
	alarm(0);
      } else {
	hp = NULL;
      }
      if (hp == NULL) {
	killsock(sock);
	return -2;
      }
      egg_memcpy(x, hp->h_addr, hp->h_length);
    }
    for (i = 0; i < MAXSOCKS; i++)
      if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
	socklist[i].flags |= SOCK_PROXYWAIT; /* drummer */
    egg_snprintf(s, sizeof s, "\004\001%c%c%c%c%c%c%s", (port >> 8) % 256,
		 (port % 256), x[0], x[1], x[2], x[3], botuser);
    tputs(sock, s, strlen(botuser) + 9); /* drummer */
  } else if (proxy == PROXY_SUN) {
    egg_snprintf(s, sizeof s, "%s %d\n", host, port);
    tputs(sock, s, strlen(s)); /* drummer */
  }
  return sock;
}

/* Starts a connection attempt to a socket
 * 
 * If given a normal hostname, this will be resolved to the corresponding
 * IP address first. PLEASE try to use the non-blocking dns functions
 * instead and then call this function with the IP address to avoid blocking.
 * 
 * returns <0 if connection refused:
 *   -1  neterror() type error
 *   -2  can't resolve hostname
 */
int open_telnet_raw(int sock, char *server, int sport)
{
#ifdef IPV6
  struct sockaddr_in6 name, vname;
#else
  struct sockaddr_in name, vname;
#endif
  struct hostent *hp;
  char host[121];
  int i, port;
  volatile int proxy;

debug2("|NET| open_telnet_raw: %s %d", server, sport);

  /* firewall?  use socks */
  if (firewall[0]) {
    if (firewall[0] == '!') {
      proxy = PROXY_SUN;
      strcpy(host, &firewall[1]);
    } else {
      proxy = PROXY_SOCKS;
      strcpy(host, firewall);
    }
    port = firewallport;
  } else {
    proxy = 0;
    strcpy(host, server);
    port = sport;
  }

  egg_bzero((char *) &name, sizeof name);
#ifdef IPV6
  name.sin6_family = AF_INET6;
  name.sin6_port = htons(port);
#else
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
#endif

  if (!setjmp(alarmret)) {
      char *p;
      int type;
      
      alarm(resolve_timeout);

      if (!egg_strncasecmp("ipv6%", host, 5)) {
	type = AF_INET6;
	p = host + 5;
debug1("|NET| checking only AAAA record for %s", p);
      } else if (!egg_strncasecmp("ipv4%", host, 5)) {
	type = AF_INET;
	p = host + 5;
debug1("|NET| checking only A record for %s", p);
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
  } else {
      hp = NULL;
  }
  if (hp == NULL)
      return -2;
      
#ifndef IPV6      
  egg_memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
  name.sin_family = hp->h_addrtype;
#else
  if (hp->h_addrtype == AF_INET6)
    egg_memcpy(&name.sin6_addr, hp->h_addr, hp->h_length);
  else if (hp->h_addrtype == AF_INET)
    name.sin6_addr = ipv4to6(*((unsigned int*) hp->h_addr));
  else
    return -123; /* panic :P */
  name.sin6_family = AF_INET6;
#endif

  egg_bzero((char *) &vname, sizeof vname);
#ifndef IPV6
  vname.sin_family = AF_INET;
  vname.sin_addr.s_addr = (myip[0] ? getmyip() : INADDR_ANY);
#else
  vname.sin6_family = AF_INET6;
  if (IN6_IS_ADDR_V4MAPPED(&name.sin6_addr))
    vname.sin6_addr = ipv4to6(myip[0] ? getmyip() : INADDR_ANY);
  else
    vname.sin6_addr = (myip6[0] ? getmyip6() : in6addr_any);
#endif
  if (bind(sock, (struct sockaddr *) &vname, sizeof vname) < 0)
    return -1;
  
  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == sock))
      socklist[i].flags = (socklist[i].flags & ~SOCK_VIRTUAL) | SOCK_CONNECT;
  }
  if (connect(sock, (struct sockaddr *) &name, (sizeof name)) < 0) {
    if (errno == EINPROGRESS) {
      /* Firewall?  announce connect attempt to proxy */
      if (firewall[0])
	return proxy_connect(sock, server, sport, proxy);
      return sock;		/* async success! */
    } else
      return -1;
  }
  /* Synchronous? :/ */
  if (firewall[0])
    return proxy_connect(sock, server, sport, proxy);
  return sock;
}

/* Ordinary non-binary connection attempt */
int open_telnet(char *server, int port)
{
  int sock = getsock(0),
      ret = open_telnet_raw(sock, server, port);

  if (ret < 0)
    killsock(sock);
  return ret;
}

/* Returns a socket number for a listening socket that will accept any
 * connection on a certain address -- port # is returned in port
 * "" means on any address
 */
int open_address_listen(char *addr, int *port)
{
  int sock;
  unsigned int addrlen;
#ifdef IPV6
  struct sockaddr_in6 name;
#else
  struct sockaddr_in name;
#endif

  if (firewall[0]) {
    /* FIXME: can't do listen port thru firewall yet */
    putlog(LOG_MISC, "*", "!! Cant open a listen port (you are using a firewall)");
    return -1;
  }

  sock = getsock(SOCK_LISTEN);
  if (sock < 1)
    return -1;
    
debug2("|NET| open_address_listen(\"%s\", %d)", addr, *port);

  egg_bzero((char *) &name, sizeof name);
#ifdef IPV6
  name.sin6_family = AF_INET6;
  name.sin6_port = htons(*port);	/* 0 = just assign us a port */
  if (!addr[0])
      name.sin6_addr = in6addr_any;
  else if (!egg_inet_pton(AF_INET6, addr, &name.sin6_addr)) {
      struct in_addr a4;
      if (egg_inet_aton(addr, &a4))
          name.sin6_addr = ipv4to6(a4.s_addr);
      else
	  name.sin6_addr = in6addr_any;
  }
#else
  name.sin_family = AF_INET;
  name.sin_port = htons(*port);	/* 0 = just assign us a port */
  if (addr[0])
      egg_inet_aton(addr, &name.sin_addr);
  else
      name.sin_addr.s_addr = INADDR_ANY;
#endif
  if (bind(sock, (struct sockaddr *) &name, sizeof name) < 0) {
    killsock(sock);
    if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT)
      return -2;
    else
      return -1;
  }
  /* what port are we on? */
  addrlen = sizeof name;
  if (getsockname(sock, (struct sockaddr *) &name, &addrlen) < 0) {
    killsock(sock);
    return -1;
  }
#ifdef IPV6
  *port = ntohs(name.sin6_port);
#else
  *port = ntohs(name.sin_port);
#endif
  if (listen(sock, 1) < 0) {
    killsock(sock);
    return -1;
  }
  return sock;
}

/* Returns a socket number for a listening socket that will accept any
 * connection -- port # is returned in port
 */
inline int open_listen(int *port, int af)
{
  if (af == AF_INET)
    return open_address_listen(myip, port);
#ifdef IPV6
  else if (af == AF_INET6)
    return open_address_listen(myip6, port);
#endif
  else
    return -1;
}

/* Given a network-style IP address, returns the hostname. The hostname
 * will be in the "##.##.##.##" format if there was an error.
 * 
 * NOTE: This function is depreciated. Try using the async dns approach
 *       instead.
 */
char *hostnamefromip(unsigned long ip)
{
  struct hostent *hp;
  unsigned long addr = ip;
  unsigned char *p;
  static char s[UHOSTLEN];

  if (!setjmp(alarmret)) {
    alarm(resolve_timeout);
    hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
    alarm(0);
  } else {
    hp = NULL;
  }
  if (hp == NULL) {
    p = (unsigned char *) &addr;
    sprintf(s, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
    return s;
  }
  strncpyz(s, hp->h_name, sizeof s);
  return s;
}

/* Returns the given network byte order IP address in the
 * dotted format - "##.##.##.##"
 * (it's IPv4 only, and it's okey - drummer)
 */
char *iptostr(IP ip)
{
  struct in_addr a;

  a.s_addr = ip;
  return inet_ntoa(a);
}

char *getlocaladdr(int sock)
{
    static char buf[ADDRLEN];
#ifdef IPV6
    struct sockaddr_in6 sa;
#else
    struct sockaddr_in sa;
#endif
    int len = sizeof sa;

    if (sock == -1) { /* assuming IPv4... */
	sprintf(buf, "%lu", (unsigned long int)
		    ntohl(natip[0] ? inet_addr(natip) : getmyip()));
	return buf;
    }

    if (getsockname(sock, (struct sockaddr*) &sa, &len) == -1) {
debug1("|NET| getsockname() failed for sock %d", sock);
	return 0;
    }
#ifdef IPV6
    if (IN6_IS_ADDR_V4MAPPED(&sa.sin6_addr))
	sprintf(buf, "%lu", 
		(unsigned long int) ntohl(((uint32_t *)&sa.sin6_addr)[3]));
    else
	egg_inet_ntop(AF_INET6, &(sa.sin6_addr), buf, sizeof buf);
#else
    sprintf(buf, "%lu", (unsigned long int) ntohl(sa.sin_addr.s_addr));
#endif
    return buf;
}

/* Short routine to answer a connect received on a socket made previously
 * by open_listen ... returns hostname of the caller & the new socket
 * does NOT dispose of old "public" socket!
 */
int answer(int sock, char *caller, char *ip, unsigned short *port,
	   int binary)
{
  int new_sock;
  unsigned int addrlen;
#ifdef IPV6
  struct sockaddr_in6 from;
#else
  struct sockaddr_in from;
#endif

  addrlen = sizeof from;
  new_sock = accept(sock, (struct sockaddr *) &from, &addrlen);
  if (new_sock < 0)
    return -1;
  if (ip != NULL) {
#ifdef IPV6
    if (IN6_IS_ADDR_V4MAPPED(&from.sin6_addr))
	egg_inet_ntop(AF_INET, &(((uint32_t *)&from.sin6_addr)[3]), ip, ADDRMAX);
    else
	egg_inet_ntop(AF_INET6, &(from.sin6_addr), ip, ADDRMAX);
#else
    egg_inet_ntop(AF_INET, &(from.sin_addr.s_addr), ip, ADDRMAX);
#endif
    /* This is now done asynchronously. We now only provide the IP address.
     *
     * strncpy(caller, hostnamefromip(*ip), 120);
     */
    strncpyz(caller, ip, 121);
  }
  if (port != NULL)
#ifdef IPV6
    *port = ntohs(from.sin6_port);
#else
    *port = ntohs(from.sin_port);
#endif
  /* Set up all the normal socket crap */
  setsock(new_sock, (binary ? SOCK_BINARY : 0));
  return new_sock;
}

/* Like open_telnet, but uses server & port specifications of dcc
 */
int open_telnet_dcc(int sock, char *server, char *port)
{
  int p;
  struct in_addr ia;

debug2("|NET| open_telnet_dcc: %s %s", server, port);
  if (port != NULL)
    p = atoi(port);
  else
    return -3;
  if (server == NULL)
    return -3;
  /* fix the IPv4 IP format (ie: 167772161 -> 10.0.0.1) */
  if (egg_inet_aton(server, &ia))
    return open_telnet_raw(sock, inet_ntoa(ia), p);
  else
    return open_telnet_raw(sock, server, p);
}

void egg_dns_gotanswer(int status, adns_answer *aw, char *origname)
{
    char name[UHOSTLEN];

    if (!aw) {
debug0("|DNS| egg_dns_gotanswer: ANSWER IS NULL!");
	return;
    }
    if (!origname) {
debug0("|DNS| egg_dns_gotanswer: origname is NULL!");
	return;
    }

debug2("|DNS| egg_dns_gotanswer: status=%d adns_answer=%x", status, (int)aw);
    status = 0;
    if ((aw->type == adns_r_addr)
#ifdef IPV6
             || (aw->type == adns_r_addr6)
#endif
             ) {
	if ((aw->status == adns_s_ok) && (aw->nrrs > 0)) {
	    adns_rr_addr *rrp = aw->rrs.untyped;
	    if (rrp->addr.sa.sa_family == AF_INET) {
		egg_inet_ntop(AF_INET, &(rrp->addr.inet.sin_addr), name, UHOSTLEN-1);
		status = 1;
#ifdef IPV6
	    } else if (rrp->addr.sa.sa_family == AF_INET6) {
		egg_inet_ntop(AF_INET6, &(rrp->addr.inet6.sin6_addr), name, UHOSTLEN-1);
		status = 1;
#endif
	    }
#ifdef IPV6
	} else if ((aw->type == adns_r_addr /* af_preferred */) &&
		    egg_strncasecmp(origname, "ipv6%", 5) &&
		    egg_strncasecmp(origname, "ipv4%", 5)) {
	    adns_query q6;
	    char *orign2 = (char*) nmalloc(strlen(origname) + 1);
	    strcpy(orign2, origname);
	    /* ...it may be AAAA */
debug1("|DNS| egg_dns_gotanswer: A failed, checking for AAAA (%s)", origname);
	    adns_submit(ads, origname, adns_r_addr6, 0, orign2, &q6);
	    status = -1;
#endif
	}
	if (status == 0)
	    strcpy(name, "0.0.0.0");
	if (status >= 0) {
debug3("|DNS| egg_dns_gotanswer: (ipbyhost) host: %s ip: %s status: %d", origname, name, status);
	    call_ipbyhost(origname, name, status);
	}
    } else if ((aw->type == adns_r_ptr_ip6) || (aw->type == adns_r_ptr)) {
	if ((aw->status == adns_s_ok) && (aw->nrrs > 0)) {
	    if (aw->rrs.str) {
		strncpyz(name, *(aw->rrs.str), UHOSTLEN);
		status = 1;
	    }
	}
	if (!status) {
	    if (origname)
		strncpyz(name, origname, UHOSTLEN);
	    else
		strcpy(name, "error");
	}
debug3("|DNS| egg_dns_gotanswer: (hostbyip) ip: %s host: %s status: %d", origname, name, status);
	call_hostbyip(origname, name, status);
    } else
	debug0("|DNS| egg_dns_gotanswer: got unknow type of answer ?!");
    nfree(origname);
}

void egg_dns_checkall()
{
    adns_query q, r;
    adns_answer *answer;
    char *origname;

    adns_forallqueries_begin(ads);
    while ((q = adns_forallqueries_next(ads, (void **)&r)) != NULL) {
	switch (adns_check(ads, &q, &answer, (void **)&origname)) {
	    case 0: /* ok */
		egg_dns_gotanswer(1, answer, origname);
	        break;
	    case EAGAIN:  /* Go into the queue again */
	        break;
	    default: /* failed */
		egg_dns_gotanswer(0, answer, origname);
		break;
	} 
    }
}

/* Attempts to read from all the sockets in socklist
 * fills s with up to 511 bytes if available, and returns the array index
 * 
 * 		on EOF:  returns -1, with socket in len
 *     on socket error:  returns -2
 * if nothing is ready:  returns -3
 */
static int sockread(char *s, int *len)
{
  fd_set fd, fdw, fde;
  int fds, i, x;
  struct timeval t, tnow;
  struct timeval *pt = &t;
  int grab = 511;

  fds = getdtablesize();
#ifdef FD_SETSIZE
  if (fds > FD_SETSIZE)
    fds = FD_SETSIZE;		/* Fixes YET ANOTHER freebsd bug!!! */
#endif
  /* timeout: 1 sec */
  t.tv_sec = 1;
  t.tv_usec = 0;
  FD_ZERO(&fd);
  
  for (i = 0; i < MAXSOCKS; i++)
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_VIRTUAL))) {
      if ((socklist[i].sock == STDOUT) && !backgrd)
	FD_SET(STDIN, &fd);
      else
	FD_SET(socklist[i].sock, &fd);
    }
  tnow.tv_sec = time(NULL);
  tnow.tv_usec = 0;
  adns_beforeselect(ads, &fds, &fd, &fdw, &fde, &pt, 0, &tnow);
#ifdef HPUX_HACKS
#ifndef HPUX10_HACKS
  x = select(fds, (int *) &fd, (int *) NULL, (int *) NULL, &t);
#else
  x = select(fds, &fd, NULL, NULL, &t);
#endif
#else
  x = select(fds, &fd, NULL, NULL, &t);
#endif
  tnow.tv_sec = time(NULL);
  tnow.tv_usec = 0;
  adns_afterselect(ads, fds, &fd, &fdw, &fde, &tnow);
  
  /* dns stuff */
  egg_dns_checkall();

  if (x > 0) {
    /* Something happened */
    for (i = 0; i < MAXSOCKS; i++) {
      if ((!(socklist[i].flags & SOCK_UNUSED)) &&
	  ((FD_ISSET(socklist[i].sock, &fd)) ||
	   ((socklist[i].sock == STDOUT) && (!backgrd) &&
	    (FD_ISSET(STDIN, &fd))))) {
	if (socklist[i].flags & (SOCK_LISTEN | SOCK_CONNECT)) {
	  /* Listening socket -- don't read, just return activity */
	  /* Same for connection attempt */
	  /* (for strong connections, require a read to succeed first) */
	  if (socklist[i].flags & SOCK_PROXYWAIT) { /* drummer */
	    /* Hang around to get the return code from proxy */
	    grab = 10;
	  } else if (!(socklist[i].flags & SOCK_STRONGCONN)) {
	    debug1("net: connect! sock %d", socklist[i].sock);
	    s[0] = 0;
	    *len = 0;
	    return i;
	  }
	} else if (socklist[i].flags & SOCK_PASS) {
	  s[0] = 0;
	  *len = 0;
	  return i;
	}
	if ((socklist[i].sock == STDOUT) && !backgrd)
	  x = read(STDIN, s, grab);
	else
	  x = read(socklist[i].sock, s, grab);
	if (x <= 0) {		/* eof */
	  if (errno == EAGAIN) {
	    s[0] = 0;
	    *len = 0;
	    return -3;
	  }
	  *len = socklist[i].sock;
	  socklist[i].flags &= ~SOCK_CONNECT;
	  debug1("net: eof!(read) socket %d", socklist[i].sock);
	  return -1;
	}
	s[x] = 0;
	*len = x;
	if (socklist[i].flags & SOCK_PROXYWAIT) {
	  debug2("net: socket: %d proxy errno: %d", socklist[i].sock, s[1]);
	  socklist[i].flags &= ~(SOCK_CONNECT | SOCK_PROXYWAIT);
	  switch (s[1]) {
	  case 90:		/* Success */
	    s[0] = 0;
	    *len = 0;
	    return i;
	  case 91:		/* Failed */
	    errno = ECONNREFUSED;
	    break;
	  case 92:		/* No identd */
	  case 93:		/* Identd said wrong username */
	    /* A better error message would be "socks misconfigured"
	     * or "identd not working" but this is simplest.
	     */
	    errno = ENETUNREACH;
	    break;
	  }
	  *len = socklist[i].sock;
	  return -1;
	}
	return i;
      }
    }
  } else if (x == -1)
    return -2;			/* socket error */
  else {
    s[0] = 0;
    *len = 0;
  }
  return -3;
}

/* sockgets: buffer and read from sockets
 * 
 * Attempts to read from all registered sockets for up to one second.  if
 * after one second, no complete data has been received from any of the
 * sockets, 's' will be empty, 'len' will be 0, and sockgets will return -3.
 * if there is returnable data received from a socket, the data will be
 * in 's' (null-terminated if non-binary), the length will be returned
 * in len, and the socket number will be returned.
 * normal sockets have their input buffered, and each call to sockgets
 * will return one line terminated with a '\n'.  binary sockets are not
 * buffered and return whatever coems in as soon as it arrives.
 * listening sockets will return an empty string when a connection comes in.
 * connecting sockets will return an empty string on a successful connect,
 * or EOF on a failed connect.
 * if an EOF is detected from any of the sockets, that socket number will be
 * put in len, and -1 will be returned.
 * the maximum length of the string returned is 512 (including null)
 *
 * Returns -4 if we handled something that shouldn't be handled by the
 * dcc functions. Simply ignore it.
 */

int sockgets(char *s, int *len)
{
  char xx[514], *p, *px;
  int ret, i, data = 0;

  for (i = 0; i < MAXSOCKS; i++) {
    /* Check for stored-up data waiting to be processed */
    if (!(socklist[i].flags & SOCK_UNUSED) &&
	!(socklist[i].flags & SOCK_BUFFER) && (socklist[i].inbuf != NULL)) {
      if (!(socklist[i].flags & SOCK_BINARY)) {
	/* look for \r too cos windows can't follow RFCs */
	p = strchr(socklist[i].inbuf, '\n');
	if (p == NULL)
	  p = strchr(socklist[i].inbuf, '\r');
	if (p != NULL) {
	  *p = 0;
	  if (strlen(socklist[i].inbuf) > 510)
	    socklist[i].inbuf[510] = 0;
	  strcpy(s, socklist[i].inbuf);
	  px = (char *) nmalloc(strlen(p + 1) + 1);
	  strcpy(px, p + 1);
	  nfree(socklist[i].inbuf);
	  if (px[0])
	    socklist[i].inbuf = px;
	  else {
	    nfree(px);
	    socklist[i].inbuf = NULL;
	  }
	  /* Strip CR if this was CR/LF combo */
	  if (s[strlen(s) - 1] == '\r')
	    s[strlen(s) - 1] = 0;
	  *len = strlen(s);
	  return socklist[i].sock;
	}
      } else {
	/* Handling buffered binary data (must have been SOCK_BUFFER before). */
	if (socklist[i].inbuflen <= 510) {
	  *len = socklist[i].inbuflen;
	  egg_memcpy(s, socklist[i].inbuf, socklist[i].inbuflen);
	  nfree(socklist[i].inbuf);
          socklist[i].inbuf = NULL;
	  socklist[i].inbuflen = 0;
	} else {
	  /* Split up into chunks of 510 bytes. */
	  *len = 510;
	  egg_memcpy(s, socklist[i].inbuf, *len);
	  egg_memcpy(socklist[i].inbuf, socklist[i].inbuf + *len, *len);
	  socklist[i].inbuflen -= *len;
	  socklist[i].inbuf = nrealloc(socklist[i].inbuf,
				       socklist[i].inbuflen);
	}
	return socklist[i].sock;
      }
    }
    /* Also check any sockets that might have EOF'd during write */
    if (!(socklist[i].flags & SOCK_UNUSED)
	&& (socklist[i].flags & SOCK_EOFD)) {
      s[0] = 0;
      *len = socklist[i].sock;
      return -1;
    }
  }
  /* No pent-up data of any worth -- down to business */
  *len = 0;
  ret = sockread(xx, len);
  if (ret < 0) {
    s[0] = 0;
    return ret;
  }
  /* Binary, listening and passed on sockets don't get buffered. */
  if (socklist[ret].flags & SOCK_CONNECT) {
    if (socklist[ret].flags & SOCK_STRONGCONN) {
      socklist[ret].flags &= ~SOCK_STRONGCONN;
      /* Buffer any data that came in, for future read. */
      socklist[ret].inbuflen = *len;
      socklist[ret].inbuf = (char *) nmalloc(*len + 1);
      /* It might be binary data. You never know. */
      egg_memcpy(socklist[ret].inbuf, xx, *len);
      socklist[ret].inbuf[*len] = 0;
    }
    socklist[ret].flags &= ~SOCK_CONNECT;
    s[0] = 0;
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & SOCK_BINARY) {
    egg_memcpy(s, xx, *len);
    return socklist[ret].sock;
  }
  if ((socklist[ret].flags & SOCK_LISTEN) ||
      (socklist[ret].flags & SOCK_PASS))
    return socklist[ret].sock;
  if (socklist[ret].flags & SOCK_BUFFER) {
    socklist[ret].inbuf = (char *) nrealloc(socklist[ret].inbuf,
		    			    socklist[ret].inbuflen + *len + 1);
    egg_memcpy(socklist[ret].inbuf + socklist[ret].inbuflen, xx, *len);
    socklist[ret].inbuflen += *len;
    /* We don't know whether it's binary data. Make sure normal strings
       will be handled properly later on too. */
    socklist[ret].inbuf[socklist[ret].inbuflen] = 0;
    return -4;	/* Ignore this one. */
  }
  /* Might be necessary to prepend stored-up data! */
  if (socklist[ret].inbuf != NULL) {
    p = socklist[ret].inbuf;
    socklist[ret].inbuf = (char *) nmalloc(strlen(p) + strlen(xx) + 1);
    strcpy(socklist[ret].inbuf, p);
    strcat(socklist[ret].inbuf, xx);
    nfree(p);
    if (strlen(socklist[ret].inbuf) < 512) {
      strcpy(xx, socklist[ret].inbuf);
      nfree(socklist[ret].inbuf);
      socklist[ret].inbuf = NULL;
      socklist[ret].inbuflen = 0;
    } else {
      p = socklist[ret].inbuf;
      socklist[ret].inbuflen = strlen(p) - 510;
      socklist[ret].inbuf = (char *) nmalloc(socklist[ret].inbuflen + 1);
      strcpy(socklist[ret].inbuf, p + 510);
      *(p + 510) = 0;
      strcpy(xx, p);
      nfree(p);
      /* (leave the rest to be post-pended later) */
    }
  }
  /* Look for EOL marker; if it's there, i have something to show */
  p = strchr(xx, '\n');
  if (p == NULL)
    p = strchr(xx, '\r');
  if (p != NULL) {
    *p = 0;
    strcpy(s, xx);
    strcpy(xx, p + 1);
    if (s[strlen(s) - 1] == '\r')
      s[strlen(s) - 1] = 0;
    data = 1;			/* DCC_CHAT may now need to process a
				   blank line */
/* NO! */
/* if (!s[0]) strcpy(s," ");  */
  } else {
    s[0] = 0;
    if (strlen(xx) >= 510) {
      /* String is too long, so just insert fake \n */
      strcpy(s, xx);
      xx[0] = 0;
      data = 1;
    }
  }
  *len = strlen(s);
  /* Anything left that needs to be saved? */
  if (!xx[0]) {
    if (data)
      return socklist[ret].sock;
    else
      return -3;
  }
  /* Prepend old data back */
  if (socklist[ret].inbuf != NULL) {
    p = socklist[ret].inbuf;
    socklist[ret].inbuflen = strlen(p) + strlen(xx);
    socklist[ret].inbuf = (char *) nmalloc(socklist[ret].inbuflen + 1);
    strcpy(socklist[ret].inbuf, xx);
    strcat(socklist[ret].inbuf, p);
    nfree(p);
  } else {
    socklist[ret].inbuflen = strlen(xx);
    socklist[ret].inbuf = (char *) nmalloc(socklist[ret].inbuflen + 1);
    strcpy(socklist[ret].inbuf, xx);
  }
  if (data) {
    return socklist[ret].sock;
  } else {
    return -3;
  }
}

/* Dump something to a socket
 * 
 * NOTE: Do NOT put Contexts in here if you want DEBUG to be meaningful!!
 */
void tputs(register int z, char *s, unsigned int len)
{
  register int i, x, idx;
  char *p;
  static int inhere = 0;

  if (z < 0)
    return;			/* um... HELLO?!  sanity check please! */
  if (((z == STDOUT) || (z == STDERR)) && (!backgrd || use_stderr)) {
    write(z, s, len);
    return;
  }
  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == z)) {
      for (idx = 0; idx < dcc_total; idx++) {
        if (dcc[idx].sock == z) {
          if (dcc[idx].type) {
            if (dcc[idx].type->name) {
              if (!strncmp(dcc[idx].type->name, "BOT", 3)) {
                otraffic_bn_today += len;
                break;
              } else if (!strcmp(dcc[idx].type->name, "SERVER")) {
                otraffic_irc_today += len;
                break;
              } else if (!strncmp(dcc[idx].type->name, "CHAT", 4)) {
                otraffic_dcc_today += len;
                break;
              } else if (!strncmp(dcc[idx].type->name, "FILES", 5)) {
                otraffic_filesys_today += len;
                break;
              } else if (!strcmp(dcc[idx].type->name, "SEND")) {
                otraffic_trans_today += len;
                break;
              } else if (!strncmp(dcc[idx].type->name, "GET", 3)) {
                otraffic_trans_today += len;
                break;
              } else {
                otraffic_unknown_today += len;
                break;
              }
            }
          }
        }
      }
      
      if (socklist[i].outbuf != NULL) {
	/* Already queueing: just add it */
	p = (char *) nrealloc(socklist[i].outbuf, socklist[i].outbuflen + len);
	egg_memcpy(p + socklist[i].outbuflen, s, len);
	socklist[i].outbuf = p;
	socklist[i].outbuflen += len;
	return;
      }
      /* Try. */
      x = write(z, s, len);
      if (x == (-1))
	x = 0;
      if (x < len) {
	/* Socket is full, queue it */
	socklist[i].outbuf = (char *) nmalloc(len - x);
	egg_memcpy(socklist[i].outbuf, &s[x], len - x);
	socklist[i].outbuflen = len - x;
      }
      return;
    }
  }
  /* Make sure we don't cause a crash by looping here */
  if (!inhere) {
    inhere = 1;

    putlog(LOG_MISC, "*", "!!! writing to nonexistent socket: %d", z);
    s[strlen(s) - 1] = 0;
    putlog(LOG_MISC, "*", "!-> '%s'", s);

    inhere = 0;
  }
}

/* tputs might queue data for sockets, let's dump as much of it as
 * possible.
 */
void dequeue_sockets()
{
  int i, x;

  for (i = 0; i < MAXSOCKS; i++) { 
    if (!(socklist[i].flags & SOCK_UNUSED) &&
	socklist[i].outbuf != NULL) {
      /* Trick tputs into doing the work */
      x = write(socklist[i].sock, socklist[i].outbuf,
		socklist[i].outbuflen);
      if ((x < 0) && (errno != EAGAIN)
#ifdef EBADSLT
	  && (errno != EBADSLT)
#endif
#ifdef ENOTCONN
	  && (errno != ENOTCONN)
#endif
	) {
	/* This detects an EOF during writing */
	debug3("net: eof!(write) socket %d (%s,%d)", socklist[i].sock,
	       strerror(errno), errno);
	socklist[i].flags |= SOCK_EOFD;
      } else if (x == socklist[i].outbuflen) {
	/* If the whole buffer was sent, nuke it */
	nfree(socklist[i].outbuf);
	socklist[i].outbuf = NULL;
	socklist[i].outbuflen = 0;
      } else if (x > 0) {
	char *p = socklist[i].outbuf;

	/* This removes any sent bytes from the beginning of the buffer */
	socklist[i].outbuf = (char *) nmalloc(socklist[i].outbuflen - x);
	egg_memcpy(socklist[i].outbuf, p + x, socklist[i].outbuflen - x);
	socklist[i].outbuflen -= x;
	nfree(p);
      }
     
      /* All queued data was sent. Call handler if one exists and the
       * dcc entry wants it.
       */
      if (!socklist[i].outbuf) {
	int idx = findanyidx(socklist[i].sock);

	if (idx > 0 && dcc[idx].type && dcc[idx].type->outdone)
	  dcc[idx].type->outdone(idx);
      }
    }
  }
}
 

/*
 *      Debugging stuff
 */

void tell_netdebug(int idx)
{
  int i;
  char s[80];

  dprintf(idx, "Open sockets:");
  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED)) {
      sprintf(s, " %d", socklist[i].sock);
      if (socklist[i].flags & SOCK_BINARY)
	strcat(s, " (binary)");
      if (socklist[i].flags & SOCK_LISTEN)
	strcat(s, " (listen)");
      if (socklist[i].flags & SOCK_PASS)
	strcat(s, " (passed on)");
      if (socklist[i].flags & SOCK_CONNECT)
	strcat(s, " (connecting)");
      if (socklist[i].flags & SOCK_STRONGCONN)
	strcat(s, " (strong)");
      if (socklist[i].flags & SOCK_NONSOCK)
	strcat(s, " (file)");
      if (socklist[i].inbuf != NULL)
	sprintf(&s[strlen(s)], " (inbuf: %04X)", strlen(socklist[i].inbuf));
      if (socklist[i].outbuf != NULL)
	sprintf(&s[strlen(s)], " (outbuf: %06lX)", socklist[i].outbuflen);
      strcat(s, ",");
      dprintf(idx, "%s", s);
    }
  }
  dprintf(idx, " done.\n");
}

/* Checks wether the referenced socket has data queued.
 *
 * Returns true if the incoming/outgoing (depending on 'type') queues
 * contain data, otherwise false.
 */
int sock_has_data(int type, int sock)
{
  int ret = 0, i;

  for (i = 0; i < MAXSOCKS; i++)
    if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
      break;
  if (i < MAXSOCKS) {
    switch (type) {
      case SOCK_DATA_OUTGOING:
	ret = (socklist[i].outbuf != NULL);
	break;
      case SOCK_DATA_INCOMING:
	ret = (socklist[i].inbuf != NULL);
	break;
    }
  } else
    debug1("sock_has_data: could not find socket #%d, returning false.", sock);
  return ret;
}

/* flush_inbuf():
 * checks if there's data in the incoming buffer of an connection
 * and flushs the buffer if possible
 *
 * returns: -1 if the dcc entry wasn't found
 *          -2 if dcc[idx].type->activity doesn't exist and the data couldn't
 *             be handled
 *          0 if buffer was empty
 *          otherwise length of flushed buffer
 */
int flush_inbuf(int idx)
{
  int i, len;
  char *inbuf;

  Assert((idx >= 0) && (idx < dcc_total));
  for (i = 0; i < MAXSOCKS; i++) {
    if ((dcc[idx].sock == socklist[i].sock)
        && !(socklist[i].flags & SOCK_UNUSED)) {
      len = socklist[i].inbuflen;
      if ((len > 0) && socklist[i].inbuf) {
        if (dcc[idx].type && dcc[idx].type->activity) {
          inbuf = socklist[i].inbuf;
          socklist[i].inbuf = NULL;
          dcc[idx].type->activity(idx, inbuf, len);
          nfree(inbuf);
          return len;
        } else
          return -2;
      } else
        return 0;
    }
  }
  return -1;
}
