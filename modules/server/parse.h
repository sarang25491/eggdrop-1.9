#ifndef _PARSE_H_
#define _PARSE_H_

/* Structure to hold the parts of an irc message. NSTATIC_ARGS is how much
 * space we allocate statically for arguments. If we run out, more is
 * allocated dynamically. */
#define IRC_MSG_NSTATIC_ARGS	10
typedef struct irc_msg {
	struct irc_msg *next;
	char *prefix, *cmd, **args, *trailing;
	char *static_args[IRC_MSG_NSTATIC_ARGS];
	int nargs;
} irc_msg_t;

void irc_msg_parse(char *text, irc_msg_t *msg);
void irc_msg_restore(irc_msg_t *msg);
void irc_msg_cleanup(irc_msg_t *msg);

#endif /* _PARSE_H_ */
