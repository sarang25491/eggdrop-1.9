/*
 * binds.c -- init/destroy bind tables
 */

#define MODULE_NAME "server"
#define MAKING_SERVER
#include "lib/eggdrop/module.h"
#include "dcc.h"

bind_table_t *BT_wall = NULL,
	*BT_raw = NULL,
	*BT_new_raw = NULL,
	*BT_notice = NULL,
	*BT_msg = NULL,
	*BT_msgm = NULL,
	*BT_pub = NULL,
	*BT_pubm = NULL,
	*BT_ctcp = NULL,
	*BT_ctcr = NULL,
	*BT_dcc_chat = NULL,
	*BT_dcc_recv = NULL;

void server_binds_destroy()
{
	bind_table_del(BT_wall);
	bind_table_del(BT_raw);
	bind_table_del(BT_new_raw);
	bind_table_del(BT_notice);
	bind_table_del(BT_msgm);
	bind_table_del(BT_msg);
	bind_table_del(BT_pubm);
	bind_table_del(BT_pub);
	bind_table_del(BT_ctcr);
	bind_table_del(BT_ctcp);
	bind_table_del(BT_dcc_chat);
}

void server_binds_init()
{
	/* Create our bind tables. */
	BT_wall = bind_table_add("wall", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_raw = bind_table_add("raw", 3, "sss", MATCH_MASK, BIND_STACKABLE);
	BT_new_raw = bind_table_add("newraw", 6, "ssUsiS", MATCH_MASK, BIND_STACKABLE);
	BT_notice = bind_table_add("notice", 5, "ssUss", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_msg = bind_table_add("msg", 4, "ssUs", 0, BIND_USE_ATTR);
	BT_msgm = bind_table_add("msgm", 4, "ssUs", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_pub = bind_table_add("pub", 5, "ssUss", 0, BIND_USE_ATTR);
	BT_pubm = bind_table_add("pubm", 5, "ssUss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
	BT_ctcr = bind_table_add("ctcr", 6, "ssUsss", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_ctcp = bind_table_add("ctcp", 6, "ssUsss", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_dcc_chat = bind_table_add("dcc_chat", 6, "ssUssi", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_dcc_recv = bind_table_add("dcc_recv", 7, "ssUssii", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);

	add_builtins("ctcp", ctcp_dcc_binds);
}

void check_tcl_notc(char *nick, char *uhost, struct userrec *u, char *dest, char *arg)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  get_user_flagrec(u, &fr, NULL);
  check_bind(BT_notice, arg, &fr, nick, uhost, u, arg, dest);
}

static int check_tcl_ctcpr(char *nick, char *uhost, struct userrec *u,
			   char *dest, char *keyword, char *args,
			   bind_table_t *table)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  get_user_flagrec(u, &fr, NULL);

  return check_bind(table, keyword, &fr, nick, uhost, u, dest, keyword, args);
}

static int check_tcl_wall(char *from, char *msg)
{
  int x;

  x = check_bind(BT_wall, msg, NULL, from, msg);
  if (x & BIND_RET_LOG) {
    putlog(LOG_WALL, "*", "!%s! %s", from, msg);
    return 1;
  }
  else return 0;
}
