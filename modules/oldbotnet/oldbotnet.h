#ifndef _OLDBOTNET_H_
#define _OLDBOTNET_H_

#define BOT_PID_MULT	1000

typedef struct {
	char *host;
	int port;
	char *username, *password;
	int idx;
	user_t *obot;
} oldbotnet_session_t;

typedef struct {
	user_t *obot;
	char *host;
	int port;
	int idx;
	char *name;
	char *password;
	int connected;
	int handlen;
	int oversion;
} oldbotnet_t;

/* From events.c */
extern int oldbotnet_events_init();

/* From oldbotnet.c */
extern oldbotnet_t oldbotnet;

#endif
