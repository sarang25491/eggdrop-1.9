/* This file contains the stuff you need to use the scripting modules. */

#ifndef _SCRIPT_API_H_
#define _SCRIPT_API_H_

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
/* SCRIPT_PASS_CDATA means your callback wants its (void *)client_data passed
   as its *first* arg.
   SCRIPT_PASS_RETVAL will pass a (scriptvar_t *)retval so that you can return
   complex types.
   SCRIPT_PASS_ARRAY will pass all the args from the script in an array. It
   actually causes 2 arguments: int argc and void *argv[].
   SCRIPT_PASS_COUNT will pass the number of script arguments you're getting.
   SCRIPT_VAR_ARGS means you accept variable number of args. The nargs field
   of the command struct is the minimum number required, and the strlen of your
   syntax field is the max number (unless it ends in * of course).
   SCRIPT_VAR_FRONT means the variable args come from the front instead of the
   back. This is useful for flags and stuff.
*/
#define SCRIPT_PASS_CDATA	1
#define SCRIPT_PASS_RETVAL	2
#define SCRIPT_PASS_ARRAY	4
#define SCRIPT_PASS_COUNT	8
#define SCRIPT_VAR_ARGS	16
#define SCRIPT_VAR_FRONT	32

/* Flags for callbacks. */
/* SCRIPT_CALLBACK_ONCE means the callback will be automatically deleted after
   its first use.
*/
#define SCRIPT_CALLBACK_ONCE	1

/* Flags for linked variables. */
#define SCRIPT_READ_ONLY	1

/* Flags for variables. */
/* SCRIPT_FREE means we will call free(retval->value) (i.e. free a malloc'd
   string or array base ptr.)
   SCRIPT_FREE_VAR means we will call free(retval) (you may need this if you
   build a complex list structure of (script_var_t *)'s).
   SCRIPT_ARRAY means we interpret retval->value as an array of length
   retval->len, of whatever basic type is specified in retval->type (flags
   are ignored).
   SCRIPT_ERROR means the interpreter will raise an error exception if it knows
   how, with retval->value as the error information. Otherwise, it is ignored.
*/
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
