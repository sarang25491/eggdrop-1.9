#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "my_socket.h"
#include "sockbuf.h"
#include "eggnet.h"
#include "eggdns.h"

static void get_dns_idx();
static int dns_on_read(void *client_data, int idx, char *buf, int len);
static int dns_on_eof(void *client_data, int idx, int err, const char *errmsg);
static const char *dns_next_server();
static void parse_reply(char *response, int nbytes);
static int dns_make_query(const char *host, int type, char **buf, int *query_len, int (*callback)(), void *client_data);

static sockbuf_handler_t dns_handler = {
	"dns",
	NULL, dns_on_eof, NULL,
	dns_on_read, NULL
};

static int dns_idx = -1;
static const char *dns_ip = NULL;

static void get_dns_idx()
{
	int i, sock;

	sock = -1;
	for (i = 0; i < 5; i++) {
		if (!dns_ip) dns_ip = dns_next_server();
		sock = socket_create(dns_ip, DNS_PORT, NULL, 0, SOCKET_CLIENT | SOCKET_NONBLOCK | SOCKET_UDP);
		if (sock < 0) {
			/* Try the next server. */
			dns_ip = NULL;
		}
		else break;
	}
	if (i == 5) return;
	dns_idx = sockbuf_new();
	sockbuf_set_handler(dns_idx, &dns_handler, NULL);
	sockbuf_set_sock(dns_idx, sock, 0);
}

void egg_dns_send(char *query, int len)
{
	if (dns_idx < 0) {
		get_dns_idx();
		if (dns_idx < 0) return;
	}
	sockbuf_write(dns_idx, query, len);
}

/* Perform an async dns lookup. This is host -> ip. For ip -> host, use
 * egg_dns_reverse(). We return a dns id that you can use to cancel the
 * lookup. */
int egg_dns_lookup(const char *host, int timeout, int (*callback)(), void *client_data)
{
	char *query;
	int len, id;

	if (socket_valid_ip(host)) {
		/* If it's already an ip, we're done. */
		callback(client_data, host, host);
		return(-1);
	}
	/* Nope, we have to actually do a lookup. */
	id = dns_make_query(host, DNS_IPV4, &query, &len, callback, client_data);
	if (id != -1) {
		egg_dns_send(query, len);
		free(query);
	}
	return(id);
}

/* Perform an async dns reverse lookup. This does ip -> host. For host -> ip
 * use egg_dns_lookup(). We return a dns id that you can use to cancel the
 * lookup. */
int egg_dns_reverse(const char *ip, int timeout, int (*callback)(), void *client_data)
{
	char *query;
	int len, id;

	if (!socket_valid_ip(ip)) {
		/* If it's not a valid ip, don't even make the request. */
		callback(client_data, ip, NULL);
		return(-1);
	}
	/* Nope, we have to actually do a lookup. */
	id = dns_make_query(ip, DNS_REVERSE, &query, &len, callback, client_data);
	if (id != -1) {
		egg_dns_send(query, len);
		free(query);
	}
	return(id);
}

static int dns_on_read(void *client_data, int idx, char *buf, int len)
{
	parse_reply(buf, len);
	return(0);
}

static int dns_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	sockbuf_delete(idx);
	dns_idx = -1;
	dns_ip = NULL;
	return(0);
}

typedef struct dns_query {
	struct dns_query *next;
	char *query;
	int id;
	int (*callback)(void *client_data, const char *query, const char *result);
	void *client_data;
} dns_query_t;

typedef struct {
	unsigned short id;
	unsigned short flags;
	unsigned short question_count;
	unsigned short answer_count;
	unsigned short ns_count;
	unsigned short ar_count;
} dns_header_t;

typedef struct {
	/* char name[]; */
	unsigned short type;
	unsigned short class;
	int ttl;
	unsigned short rdlength;
	/* char rdata[]; */
} dns_rr_t;

/* Entries from resolv.conf */
typedef struct dns_server {
	char *ip;
	int idx;
} dns_server_t;

/* Entries from hosts */
typedef struct {
	char *host, *ip;
} dns_host_t;

static int query_id = 1;
static dns_header_t _dns_header = {0};
static dns_query_t *query_head = NULL;
static dns_host_t *hosts = NULL;
static int nhosts = 0;
static dns_server_t *servers = NULL;
static int nservers = 0;
static int cur_server = -1;

static char separators[] = " ,\t\r\n";

static void read_resolv(char *fname);
static void read_hosts(char *fname);

/* Read in .hosts and /etc/hosts and .resolv.conf and /etc/resolv.conf */
int egg_dns_init()
{
	_dns_header.flags = htons(1 << 8 | 1 << 7);
	_dns_header.question_count = htons(1);
	read_resolv("/etc/resolv.conf");
	read_resolv(".resolv.conf");
	read_hosts("/etc/hosts");
	read_hosts(".hosts");
	return(0);
}

static const char *dns_next_server()
{
	if (!servers || nservers < 1) return("127.0.0.1");
	cur_server++;
	if (cur_server >= nservers) cur_server = 0;
	return(servers[cur_server].ip);
}

static void add_server(char *ip)
{
	servers = (dns_server_t *)realloc(servers, (nservers+1)*sizeof(*servers));
	servers[nservers].ip = strdup(ip);
	nservers++;
}

static void add_host(char *host, char *ip)
{
	hosts = (dns_host_t *)realloc(hosts, (nhosts+1)*sizeof(*hosts));
	hosts[nhosts].host = strdup(host);
	hosts[nhosts].ip = strdup(ip);
	nhosts++;
}

static int read_thing(char *buf, char *ip)
{
	int skip, len;

	skip = strspn(buf, separators);
	buf += skip;
	len = strcspn(buf, separators);
	memcpy(ip, buf, len);
	ip[len] = 0;
	return(skip + len);
}

static void read_resolv(char *fname)
{
	FILE *fp;
	char buf[512], ip[512];

	fp = fopen(fname, "r");
	if (!fp) return;
	while (fgets(buf, sizeof(buf), fp)) {
		if (!strncasecmp(buf, "nameserver", 10)) {
			read_thing(buf+10, ip);
			if (strlen(ip)) add_server(ip);
		}
	}
	fclose(fp);
}

static void read_hosts(char *fname)
{
	FILE *fp;
	char buf[512], ip[512], host[512];
	int skip, n;

	fp = fopen(fname, "r");
	if (!fp) return;
	while (fgets(buf, sizeof(buf), fp)) {
		if (strchr(buf, '#')) continue;
		skip = read_thing(buf, ip);
		if (!strlen(ip)) continue;
		while ((n = read_thing(buf+skip, host))) {
			skip += n;
			if (strlen(host)) add_host(host, ip);
		}
	}
	fclose(fp);
}

static int make_header(char *buf, int id)
{
	_dns_header.id = htons(id);
	memcpy(buf, &_dns_header, 12);
	return(12);
}

static int cut_host(const char *host, char *query)
{
	char *period, *orig;
	int len;

	orig = query;
	while ((period = strchr(host, '.'))) {
		len = period - host;
		if (len > 63) return(-1);
		*query++ = len;
		memcpy(query, host, len);
		query += len;
		host = period+1;
	}
	len = strlen(host);
	if (len) {
		*query++ = len;
		memcpy(query, host, len);
		query += len;
	}
	*query++ = 0;
	return(query-orig);
}

static int reverse_ip(const char *host, char *reverse)
{
	char *period;
	int offset, len;

	period = strchr(host, '.');
	if (!period) {
		len = strlen(host);
		memcpy(reverse, host, len);
		return(len);
	}
	else {
		len = period - host;
		offset = reverse_ip(host+len+1, reverse);
		reverse[offset++] = '.';
		memcpy(reverse+offset, host, len);
		reverse[offset+len] = 0;
		return(offset+len);
	}
}

static int dns_make_query(const char *host, int type, char **buf, int *query_len, int (*callback)(), void *client_data)
{
	char *newhost;
	int len = 0;
	int ns_type = 0;
	int i;
	dns_query_t *q;

	*buf = NULL;
	*query_len = 0;
	if (type == DNS_REVERSE) {
		/* First see if we have it in our host cache. */
		for (i = 0; i < nhosts; i++) {
			if (!strcasecmp(hosts[i].ip, host)) {
				callback(client_data, host, hosts[i].host);
				return(-1);
			}
		}

		/* We need to transform the ip address into the proper form
		 * for reverse lookup. */
		if (strchr(host, ':')) {
			char temp[65];

			socket_ipv6_to_dots(host, temp);
			newhost = malloc(strlen(temp) + 10);
			reverse_ip(temp, newhost);
			strcat(newhost, ".in6.arpa");
		}
		else {
			newhost = (char *)malloc(strlen(host) + 14);
			reverse_ip(host, newhost);
			strcat(newhost, ".in-addr.arpa");
		}
		ns_type = 12; /* PTR (reverse lookup) */
	}
	else {
		/* First see if it's in our host cache. */
		for (i = 0; i < nhosts; i++) {
			if (!strcasecmp(host, hosts[i].host)) {
				callback(client_data, host, hosts[i].ip);
				return(-1);
			}
		}
		if (type == DNS_IPV4) ns_type = 1; /* IPv4 */
		else if (type == DNS_IPV6) ns_type = 28; /* IPv6 */
		else return(-1);
		newhost = host;
	}

	*buf = (char *)malloc(strlen(newhost) + 512);
	len = make_header(*buf, query_id);
	len += cut_host(newhost, *buf + len);
	(*buf)[len] = 0; len++; (*buf)[len] = ns_type; len++;
	(*buf)[len] = 0; len++; (*buf)[len] = 1; len++;
	*query_len = len;

	if (newhost != host) free(newhost);

	q = calloc(1, sizeof(*q));
	q->id = query_id;
	query_id++;
	q->query = strdup(host);
	q->callback = callback;
	q->client_data = client_data;
	if (query_head) q->next = query_head->next;
	query_head = q;
	return(q->id);
}

int egg_dns_cancel(int id, int issue_callback)
{
	dns_query_t *q, *prev;

	prev = NULL;
	for (q = query_head; q; q = q->next) {
		if (q->id == id) break;
		prev = q;
	}
	if (!q) return(-1);
	if (prev) prev->next = q->next;
	else query_head = q->next;

	if (issue_callback) q->callback(q->client_data, q->query, NULL);
	free(q);
	return(0);
}

static int skip_name(unsigned char *ptr)
{
	int len;
	unsigned char *start = ptr;

	while ((len = *ptr++) > 0) {
		if (len > 63) {
			ptr++;
			break;
		}
		else {
			ptr += len;
		}
	}
	return(ptr - start);
}

static void got_answer(int id, char *answer)
{
	dns_query_t *q, *prev;

	prev = NULL;
	for (q = query_head; q; q = q->next) {
		if (q->id == id) break;
		prev = q;
	}
	if (!q) return;

	if (prev) prev->next = q->next;
	else query_head = q->next;

	q->callback(q->client_data, q->query, answer);
	free(q->query);
	free(q);
}

static void parse_reply(char *response, int nbytes)
{
	dns_header_t header;
	dns_rr_t reply;
	char result[512];
	unsigned char *ptr;
	int i;

	ptr = (unsigned char *)response;
	memcpy(&header, ptr, 12);
	ptr += 12;

	header.id = ntohs(header.id);
	header.question_count = ntohs(header.question_count);
	header.answer_count = ntohs(header.answer_count);

	/* Pass over the question. */
	ptr += skip_name(ptr);
	ptr += 4;
	/* End of question. */

	for (i = 0; i < header.answer_count; i++) {
		result[0] = 0;
		/* Read in the answer. */
		ptr += skip_name(ptr);
		memcpy(&reply, ptr, 10);
		reply.type = ntohs(reply.type);
		reply.rdlength = ntohs(reply.rdlength);
		ptr += 10;
		if (reply.type == 1) {
			//fprintf(fp, "ipv4 reply\n");
			inet_ntop(AF_INET, ptr, result, 512);
			got_answer(header.id, result);
			return;
		}
		else if (reply.type == 28) {
			//fprintf(fp, "ipv6 reply\n");
			inet_ntop(AF_INET6, ptr, result, 512);
			got_answer(header.id, result);
			return;
		}
		else if (reply.type == 12) {
			char *placeholder;
			int len, dot;

			//fprintf(fp, "reverse-lookup reply\n");
			placeholder = ptr;
			result[0] = 0;
			while ((len = *ptr++) != 0) {
				if (len > 63) {
					ptr++;
					break;
				}
				else {
					dot = ptr[len];
					ptr[len] = 0;
					strcat(result, ptr);
					strcat(result, ".");
					ptr[len] = dot;
					ptr += len;
				}
			}
			if (strlen(result)) {
				result[strlen(result)-1] = 0;
				got_answer(header.id, result);
				return;
			}
			ptr = placeholder;
		}
		ptr += reply.rdlength;
	}
	got_answer(header.id, NULL);
}
