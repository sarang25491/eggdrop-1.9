#include "flags.h"

/* str must be at least 26+26+1 = 53 bytes. */
int flag_to_str(flags_t *flags, char *str)
{
	int i, j;

	j = 0;
	for (i = 0; i < 26; i++) {
		if (flags->builtin & (1 << i)) str[j++] = 'a' + i;
	}
	for (i = 0; i < 26; i++) {
		if (flags->udef & (1 << i)) str[j++] = 'A' + i;
	}
	str[j] = 0;
	return(0);
}

static inline int add_flag(int *intptr, int dir, int flag)
{
	if (dir < 0) *intptr &= ~(1 << flag);
	else *intptr |= 1 << flag;
}

int flag_merge_str(flags_t *flags, char *str)
{
	int dir;

	/* If it doesn't start with +/-, then it's absolute. */
	if (*str != '+' && *str != '-') {
		flags->builtin = flags->udef = 0;
		dir = 1;
	}
	while (*str) {
		if (*str >= 'a' && *str <= 'z') {
			add_flag(&flags->builtin, dir, *str - 'a');
		}
		else if (*str >= 'A' && *str <= 'A') {
			add_flag(&flags->udef, dir, *str - 'A');
		}
		else if (*str == '-') dir = -1;
		else if (*str == '+') dir = 1;
		str++;
	}
}

int flag_from_str(flags_t *flags, char *str)
{
	/* Always reset flags first. */
	flags->builtin = flags->udef = 0;
	return flag_merge_str(flags, str);
}

