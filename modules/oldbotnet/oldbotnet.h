#ifndef _OLDBOTNET_H_
#define _OLDBOTNET_H_

typedef struct {
	char *host;
	int port;
	char *username, *password;
	int idx;
	user_t *obot;
} oldbotnet_session_t;

typedef struct {
	char *name;
	int connected;
	int idx;
} oldbotnet_t;

/* From events.c */
extern int oldbotnet_events_init();

/* From oldbotnet.c */
extern oldbotnet_t oldbotnet;

#endif
