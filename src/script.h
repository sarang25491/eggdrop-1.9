#ifndef _EGG_SCRIPT_H
#define _EGG_SCRIPT_H

#include <eggdrop/eggdrop.h>
#include "script_api.h"

typedef struct {
	char *name;
	Function callback;
	char *syntax;
	char *syntax_error;
	int retval_type;
} script_simple_command_t;

extern int script_init(eggdrop_t *);

extern int script_link_int_table(script_int_t *table);
extern int script_unlink_int_table(script_int_t *table);
extern int script_link_str_table(script_str_t *table);
extern int script_unlink_str_table(script_str_t *table);
extern int script_create_cmd_table(script_command_t *table);
extern int script_delete_cmd_table(script_command_t *table);
extern int script_create_simple_cmd_table(script_simple_command_t *table);

extern script_var_t *script_string(char *str, int len);
extern script_var_t *script_int(int val);
extern script_var_t *script_list(int nitems, ...);
extern int script_list_append(script_var_t *list, script_var_t *item);


#endif /* _EGG_SCRIPT_H */
