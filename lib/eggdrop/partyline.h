#ifndef _EGG_PARTYLINE_H_
#define _EGG_PARTYLINE_H_

/* Flags for partyline. */
#define PARTY_DELETED	1

struct partyline_event;
struct partychan;

typedef struct partymember {
	struct partymember *next, *prev;
	int pid;
	char *nick, *ident, *host;
	user_t *user;
	int flags;

	struct partychan **channels;
	int nchannels;

	struct partyline_event *handler;
	void *client_data;
} partymember_t;

typedef struct partychan_member {
	partymember_t *p;
	int flags;
} partychan_member_t;

typedef struct partychan_handler {
	struct partyline_event *handler;
	void *client_data;
	int flags;
} partychan_handler_t;

typedef struct partychan {
	struct partychan *next, *prev;
	int cid;
	char *name;
	int flags;

	partychan_member_t *members;
	int nmembers;
	partychan_handler_t *handlers;
	int nhandlers;
} partychan_t;

typedef struct partyline_event {
	int (*on_write)(void *client_data, partymember_t *dest, const char *text, int len);
	int (*on_privmsg)(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
	int (*on_chanwrite)(void *client_data, partychan_t *chan, const char *text, int len);
	int (*on_chanmsg)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
	int (*on_join)(void *client_data, partychan_t *chan, partymember_t *src);
	int (*on_part)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

	int (*on_delete)(void *client_data);
} partyline_event_t;

int partyline_init();
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
//int partyline_disconnect(int pid, const char *msg);
int partyline_is_command(const char *text);
int partyline_on_input(partychan_t *chan, partymember_t *p, const char *text, int len);
int partyline_on_command(partychan_t *chan, partymember_t *p, const char *cmd, const char *text);
int partyline_update_info(partymember_t *p, const char *ident, const char *host);

/* Channel functions. */
partychan_t *partychan_new(int cid, const char *name);
partychan_t *partychan_lookup_cid(int cid);
partychan_t *partychan_lookup_name(const char *name);
int partychan_join_name(const char *chan, partymember_t *p);
int partychan_join_cid(int cid, partymember_t *p);
int partychan_join(partychan_t *chan, partymember_t *p);
int partychan_part_name(const char *chan, partymember_t *p, const char *text);
int partychan_part_cid(int cid, partymember_t *p, const char *text);
int partychan_part(partychan_t *chan, partymember_t *p, const char *text);
int partychan_msg_name(const char *name, partymember_t *src, const char *text, int len);
int partychan_msg_cid(int cid, partymember_t *src, const char *text, int len);
int partychan_msg(partychan_t *chan, partymember_t *src, const char *text, int len);


partymember_t *partymember_lookup_pid(int pid);
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partymember_update_info(partymember_t *p, const char *ident, const char *host);
int partymember_write_pid(int pid, const char *text, int len);
int partymember_write(partymember_t *p, const char *text, int len);
int partymember_printf_pid(int pid, const char *fmt, ...);
int partymember_printf(partymember_t *p, const char *fmt, ...);

#endif
