#ifndef _FLAGS_H_
#define _FLAGS_H_

typedef struct {
	int builtin;
	int udef;
} flags_t;

/* str should be at least 26+26+1 = 53 bytes. */
int flag_to_str(flags_t *flags, char *str);
int flag_merge_str(flags_t *flags, char *str);
int flag_from_str(flags_t *flags, char *str);

#endif
