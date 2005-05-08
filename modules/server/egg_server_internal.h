#ifndef _EGG_SERVER_INTERNAL_H_
#define _EGG_SERVER_INTERNAL_H_

/* Things from binds.c */
#define BT_wall server_LTX_BT_wall
#define BT_raw server_LTX_BT_raw
#define BT_server_input server_LTX_BT_server_input
#define BT_server_output server_LTX_BT_server_output
#define BT_notice server_LTX_BT_notice
#define BT_msg server_LTX_BT_msg
#define BT_msgm server_LTX_BT_msgm
#define BT_pub server_LTX_BT_pub
#define BT_pubm server_LTX_BT_pubm
#define BT_ctcp server_LTX_BT_ctcp
#define BT_ctcr server_LTX_BT_ctcr
#define BT_dcc_chat server_LTX_BT_dcc_chat
#define BT_dcc_recv server_LTX_BT_dcc_recv
#define BT_nick server_LTX_BT_nick
#define BT_join server_LTX_BT_join
#define BT_part server_LTX_BT_part
#define BT_quit server_LTX_BT_quit
#define BT_kick server_LTX_BT_kick
#define BT_leave server_LTX_BT_leave
#define BT_mode server_LTX_BT_mode
#define BT_chanset server_LTX_BT_chanset

/* Things from channels.c */
#define channel_head server_LTX_channel_head
#define nchannels server_LTX_nchannels
#define channel_init server_LTX_server_channel_init
#define channel_destroy server_LTX_server_channel_destroy
#define channel_reset server_LTX_channel_reset
#define channel_probe server_LTX_channel_probe
#define uhost_cache_lookup server_LTX_uhost_cache_lookup
#define uhost_cache_addref server_LTX_uhost_cache_addref
#define uhost_cache_decref server_LTX_uhost_cache_decref
#define channel_on_join server_LTX_channel_on_join
#define channel_on_leave server_LTX_channel_on_leave
#define channel_on_quit server_LTX_channel_on_quit
#define channel_on_nick server_LTX_channel_on_nick
#define channel_get_arg server_LTX_channel_get_arg
#define channel_get_mask_list server_LTX_channel_get_mask_list
#define channel_add_mask server_LTX_channel_add_mask
#define channel_del_mask server_LTX_channel_del_mask
#define channel_clear_masks server_LTX_channel_clear_masks
#define channel_mode server_LTX_channel_mode
#define channel_lookup_member server_LTX_channel_lookup_member

/* Things from dcc.c */
#define dcc_dns_set server_LTX_dcc_dns_set
#define dcc_start_chat server_LTX_dcc_start_chat
#define dcc_start_send server_LTX_dcc_start_send
#define dcc_send_info server_LTX_dcc_send_info
#define dcc_accept_send server_LTX_dcc_accept_send

/* Things from input.c */
#define global_input_string server_LTX_global_input_string
#define server_parse_input server_LTX_server_parse_input
#define server_raw_binds server_LTX_server_raw_binds

/* Things from nicklist.c */
#define nick_list server_LTX_nick_list
#define nick_list_index server_LTX_nick_list_index
#define nick_list_len server_LTX_nick_list_len
#define nick_list_cycled server_LTX_nick_list_cycled
#define try_next_nick server_LTX_try_next_nick
#define try_random_nick server_LTX_try_random_nick
#define nick_list_on_connect server_LTX_nick_list_on_connect
#define nick_get_next server_LTX_nick_get_next
#define nick_add server_LTX_nick_add
#define nick_del server_LTX_nick_del
#define nick_clear server_LTX_nick_clear
#define nick_find server_LTX_nick_find

/* Things from output.c */
#define global_output_string server_LTX_global_output_string
#define printserv server_LTX_printserv
#define queue_new server_LTX_queue_new
#define queue_append server_LTX_queue_append
#define queue_unlink server_LTX_queue_unlink
#define queue_entry_from_text server_LTX_queue_entry_from_text
#define queue_entry_cleanup server_LTX_queue_entry_cleanup
#define queue_entry_to_text server_LTX_queue_entry_to_text
#define queue_get_by_priority server_LTX_queue_get_by_priority
#define dequeue_messages server_LTX_dequeue_messages

/* Things from party_commands.c */
#define server_party_commands server_LTX_server_party_commands

/* Things from scriptcmds.c */
#define server_script_init server_LTX_server_script_init
#define server_script_destroy server_LTX_server_script_destroy

/* Things from server.c */
#define current_server server_LTX_current_server
#define server_config server_LTX_server_config
#define cycle_delay server_LTX_cycle_delay
#define server_support server_LTX_server_support
#define server_start server_LTX_start

/* Things from serverlist.c */
#define server_list server_LTX_server_list
#define server_list_index server_LTX_server_list_index
#define server_list_len server_LTX_server_list_len
#define server_get_next server_LTX_server_get_next
#define server_set_next server_LTX_server_set_next
#define server_add server_LTX_server_add
#define server_del server_LTX_server_del
#define server_find server_LTX_server_find
#define server_clear server_LTX_server_clear

/* Things from servsock.c */
#define connect_to_next_server server_LTX_connect_to_next_server
#define kill_server server_LTX_kill_server

#endif
