#ifndef _SERVER_BINDS_H_
#define _SERVER_BINDS_H_

extern bind_table_t *BT_wall,
	*BT_raw,
	*BT_notice,
	*BT_msg,
	*BT_msgm,
	*BT_pub,
	*BT_pubm,
	*BT_ctcp,
	*BT_ctcr,
	*BT_dcc_chat,
	*BT_dcc_recv,
	*BT_nick,
	*BT_join,
	*BT_part,
	*BT_quit,
	*BT_kick,
	*BT_leave;

extern void server_binds_destroy();
extern void server_binds_init();

#endif
