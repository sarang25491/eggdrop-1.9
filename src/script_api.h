/* This file contains the stuff you need to use the scripting modules. */

#ifndef _SCRIPT_API_H_
#define _SCRIPT_API_H_

/* How about some macros for setting return values? */
#define SCRIPT_RETURN_INT(x) retval->type = SCRIPT_INTEGER, retval->intval = x
#define SCRIPT_RETURN_STR(s) retval->type = SCRIPT_STRING, retval->str = s

/* Flags for commands. */
#define SCRIPT_WANTS_CD	1
#define SCRIPT_COMPLEX	2

/* Flags for linked variables. */
#define SCRIPT_READ_ONLY	1

/* Flags for variables (check out struct script_var_t) */
#define SCRIPT_STATIC	1
#define SCRIPT_STRING	2
#define SCRIPT_INTEGER	4
#define SCRIPT_LIST	8
#define SCRIPT_ARRAY	16
#define SCRIPT_VARRAY	32
#define SCRIPT_POINTER	64
#define SCRIPT_CALLBACK	128

/* Eggdrop specific types. */
#define SCRIPT_USER	256

/* Error bit. */
#define SCRIPT_ERROR	512

typedef struct script_callback_b {
	int (*callback)();
	void *callback_data;
	int (*delete)();
	void *delete_data;
} script_callback_t;

typedef struct script_var_b {
	int type;
	union {
		int intval;
		char *str;
		unsigned char *bytes;
		void *ptr;
		void *list;
		void **ptrarray;
		struct script_var_b *varray;
		struct script_var_b **vptrarray;
	};
	int len;
	int flags;
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
	int pass_array; /* Want an array of stuff? */
	char *syntax; /* Argument types. */
	char *syntax_error; /* Error to print when called incorrectly. */
	int retval_type; /* Limited return value type, for simple stuff. */
	int flags;
} script_command_t;

typedef struct script_simple_command_b {
	char *name;
	Function callback;
	char *syntax;
	char *syntax_error;
	int retval_type;
} script_simple_command_t;

#endif
