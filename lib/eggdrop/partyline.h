#ifndef _EGG_PARTYLINE_H_
#define _EGG_PARTYLINE_H_

/* Flags for partyline. */
#define PARTY_DELETED	1
#define PARTY_SELECTED	2

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

typedef struct partychan {
	struct partychan *next, *prev;
	int cid;
	char *name;
	int flags;

	partychan_member_t *members;
	int nmembers;
} partychan_t;

typedef struct partymember_common {
	struct partymember_common *next;
	partymember_t **members;
	int len;
	int max;
} partymember_common_t;

typedef struct partyline_event {
	/* Events that don't depend on a single chan. */
	int (*on_privmsg)(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
	int (*on_nick)(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
	int (*on_quit)(void *client_data, partymember_t *src, const char *text, int len);

	/* Channel events. */
	int (*on_chanmsg)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
	int (*on_join)(void *client_data, partychan_t *chan, partymember_t *src);
	int (*on_part)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
} partyline_event_t;

int partyline_init();
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partyline_delete(partymember_t *p, const char *text);
int partyline_is_command(const char *text);
int partyline_on_input(partychan_t *chan, partymember_t *p, const char *text, int len);
int partyline_on_command(partymember_t *p, const char *cmd, const char *text);
int partyline_update_info(partymember_t *p, const char *ident, const char *host);

/* Channel functions. */
partychan_t *partychan_new(int cid, const char *name);
partychan_t *partychan_lookup_cid(int cid);
partychan_t *partychan_lookup_name(const char *name);
partychan_t *partychan_get_default(partymember_t *p);
int partychan_join_name(const char *chan, partymember_t *p);
int partychan_join_cid(int cid, partymember_t *p);
int partychan_join(partychan_t *chan, partymember_t *p);
int partychan_part_name(const char *chan, partymember_t *p, const char *text);
int partychan_part_cid(int cid, partymember_t *p, const char *text);
int partychan_part(partychan_t *chan, partymember_t *p, const char *text);
int partychan_msg_name(const char *name, partymember_t *src, const char *text, int len);
int partychan_msg_cid(int cid, partymember_t *src, const char *text, int len);
int partychan_msg(partychan_t *chan, partymember_t *src, const char *text, int len);
partymember_common_t *partychan_get_common(partymember_t *p);
int partychan_free_common(partymember_common_t *common);


partymember_t *partymember_lookup_pid(int pid);
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partymember_update_info(partymember_t *p, const char *ident, const char *host);
int partymember_who(int **pids, int *len);
int partymember_write_pid(int pid, const char *text, int len);
int partymember_write(partymember_t *p, const char *text, int len);
int partymember_printf_pid(int pid, const char *fmt, ...);
int partymember_printf(partymember_t *p, const char *fmt, ...);

#endif
