#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "my_socket.h"

typedef struct {
	char *host;
	char *ip;
	int expiration;
} dns_entry_t;

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

static int query_id = 1;
static dns_header_t _dns_header = {0};
static dns_entry_t *dns_cache = NULL;
static int dns_cache_len;

static char **nameservers = NULL;

/* Read in ~/.hosts and /etc/hosts and ~/.resolv.conf and /etc/resolv.conf */
int dns_init()
{
	_dns_header.flags = htons(1 << 8 | 1 << 7);
	_dns_header.question_count = htons(1);
	return(0);
}

static char *search_cache_by_host(char *host)
{
	int i;
	for (i = 0; i < dns_cache_len; i++) {
		if (!strcasecmp(dns_cache[i].host, host)) return(dns_cache[i].ip);
	}
	return(NULL);
}

static char *search_cache_by_ip(char *ip)
{
	int i;
	for (i = 0; i < dns_cache_len; i++) {
		if (!strcasecmp(dns_cache[i].ip, ip)) return(dns_cache[i].host);
	}
	return(NULL);
}

static void add_cache_entry(char *host, char *ip, int expires)
{
	dns_cache = (dns_entry_t *)realloc(dns_cache, (dns_cache_len+1) * sizeof(dns_entry_t));
	dns_cache[dns_cache_len].host = strdup(host);
	dns_cache[dns_cache_len].ip = strdup(ip);
	dns_cache[dns_cache_len].expires = expires;
	dns_cache_len++;
}

static int make_header(char *buf, int id)
{
	_dns_header.id = htons(id);
	memcpy(buf, &_dns_header, 12);
	return(12);
}

static int cut_host(char *host, char *query)
{
	char *period, *orig;
	int len;

	orig = query;
	while (period = strchr(host, '.')) {
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

static int make_query(char *host, int type, unsigned char *buf)
{
	int len = 0;
	int ns_type = 0;

	if (type == IPV4_QUERY) ns_type = 1; /* IPv4 */
	else if (type == IPV6_QUERY) ns_type = 28; /* IPv6 */
	else if (type == REVERSE_QUERY) ns_type = 12; /* PTR (reverse lookup) */
	else return(0);

	len = make_header(buf, query_id++);
	len += cut_host(host, buf + len);
	buf[len] = 0; len++; buf[len] = ns_type; len++;
	buf[len] = 0; len++; buf[len] = 1; len++;
	return(len);
}

static int skip_name(unsigned char *ptr, char *end)
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

static int got_answer(int id, char *answer)
{
	int i;

	for (i = 0; i < nquestions; i++) {
		if (questions[i].id == id) break;
	}
	if (i == nquestions) break;

}

static void parse_reply(unsigned char *response, int nbytes)
{
	dns_header_t header;
	dns_rr_t reply;
	char result[512];
	unsigned char *ptr, *end;
	int i;

	ptr = response;
	memcpy(&header, ptr, 12);
	ptr += 12;

	header.id = ntohs(header.id);
	header.question_count = ntohs(header.question_count);
	header.answer_count = ntohs(header.answer_count);

	/* Pass over the question. */
	ptr += skip_name(ptr, end);
	ptr += 4;
	/* End of question. */

	for (i = 0; i < header.answer_count; i++) {
		result[0] = 0;
		/* Read in the answer. */
		ptr += skip_name(ptr, end);
		memcpy(&reply, ptr, 10);
		reply.type = ntohs(reply.type);
		reply.rdlength = ntohs(reply.rdlength);
		ptr += 10;
		if (reply.type == 1) {
			printf("ipv4 reply\n");
			inet_ntop(AF_INET, ptr, result, 512);
			if (got_answer(header.id, result)) break;
		}
		else if (reply.type == 28) {
			printf("ipv6 reply\n");
			inet_ntop(AF_INET6, ptr, result, 512);
			if (got_answer(header.id, result)) break;
		}
		else if (reply.type == 12) {
			char *placeholder;
			int len, dot;

			printf("reverse-lookup reply\n");
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
			}
			ptr = placeholder;
		}
		ptr += reply.rdlength;
	}
	got_answer(header.id, NULL);
}

main (int argc, char *argv[])
{
	unsigned char query[512], response[512], *ptr, buf[512];
	int i, len, sock;
	struct sockaddr_in server;
	dns_header_t header;
	dns_rr_t reply;
	unsigned long addr;

	if (argc != 4) {
		printf("usage: %s <dns ip> <host> <type>\n", argv[0]);
		printf("  <type> can be 1 (ipv4), 2 (ipv4), or 3 (reverse lookup)\n");
		return(0);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(53);
	server.sin_addr.s_addr = inet_addr(argv[1]);

	dns_init();
	len = make_query(argv[2], atoi(argv[3]), query);

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		perror("socket");
		return(1);
	}

	connect(sock, &server, sizeof(server));
	write(sock, query, len);
	len = read(sock, response, 512);
	parse_reply(response, len);
}
