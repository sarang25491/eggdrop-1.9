#ifndef _SCRIPT_HELPER_H_
#define _SCRIPT_HELPER_H_

int script_link_int_table(script_int_t *table);
int script_unlink_int_table(script_int_t *table);
int script_link_str_table(script_str_t *table);
int script_unlink_str_table(script_str_t *table);
int script_create_cmd_table(script_command_t *table);
int script_delete_cmd_table(script_command_t *table);
int script_create_simple_cmd_table(script_simple_command_t *table);

#endif /* _SCRIPT_H_ */
