/* This file contains the stuff you need to use the scripting modules. */

#ifndef _SCRIPT_API_H_
#define _SCRIPT_API_H_

/* How about some macros for setting return values? */
#define SCRIPT_RETURN_INT(x) retval->type = SCRIPT_INTEGER, retval->intval = x
#define SCRIPT_RETURN_STR(s) retval->type = SCRIPT_STRING, retval->str = s

/* Script events that get recorded in the script journal. */
enum {
	SCRIPT_EVENT_LOAD_SCRIPT = 0,
	SCRIPT_EVENT_SET_INT,
	SCRIPT_EVENT_SET_STR,
	SCRIPT_EVENT_LINK_INT,
	SCRIPT_EVENT_UNLINK_INT,
	SCRIPT_EVENT_LINK_STR,
	SCRIPT_EVENT_UNLINK_STR,
	SCRIPT_EVENT_CREATE_CMD,
	SCRIPT_EVENT_DELETE_CMD,
	SCRIPT_EVENT_MAX
};

/* Flags for commands. */
#define SCRIPT_PASS_CDATA	1
#define SCRIPT_PASS_RETVAL	2
#define SCRIPT_PASS_ARRAY	4
#define SCRIPT_PASS_COUNT	8
#define SCRIPT_VAR_ARGS	16
#define SCRIPT_VAR_FRONT	32

/* Flags for callbacks. */
#define SCRIPT_CALLBACK_ONCE	1

/* Flags for linked variables. */
#define SCRIPT_READ_ONLY	1

/* Flags for variables. */
#define SCRIPT_FREE	256
#define SCRIPT_FREE_VAR	512
#define SCRIPT_ARRAY	1024
#define SCRIPT_ERROR	2048

/* Types for variables. */
#define SCRIPT_STRING	((int)'s')
#define SCRIPT_INTEGER	((int)'i')
#define SCRIPT_POINTER	((int)'p')
#define SCRIPT_CALLBACK	((int)'c')
#define SCRIPT_USER	((int)'U')
#define SCRIPT_BYTES	((int)'b')
#define SCRIPT_VAR	((int)'v')
#define SCRIPT_TYPE_MASK	255

typedef struct script_callback_b {
	int (*callback)();
	void *callback_data;
	int (*delete)();
	void *delete_data;
	char *syntax;
	char *name;
	int flags;
} script_callback_t;

typedef struct script_var_b {
	int type;	/* Type of variable (int, str, etc). */
	void *value;	/* Value (needs to be cast to right type). */
	int len;	/* Length of string or array (when appropriate). */
} script_var_t;

typedef struct script_int_b {
	char *class;
	char *name;
	int *ptr;
} script_int_t;

typedef struct script_str_b {
	char *class;
	char *name;
	char **ptr;
} script_str_t;

typedef struct script_command_b {
	char *class;
	char *name;
	Function callback;
	void *client_data;
	int nargs; /* Number of arguments the script wants. */
	char *syntax; /* Argument types. */
	char *syntax_error; /* Error to print when called incorrectly. */
	int retval_type; /* Limited return value type, for simple stuff. */
	int flags;
} script_command_t;

#endif
