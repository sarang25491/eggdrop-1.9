#include <eggdrop/eggdrop.h>

static int script_putlog(char *text)
{
	return putlog(-1, "*", "%s", text);
}

static int script_putloglev(char *level, char *chan, char *text)
{
	return putlog(-1, chan, "%s", text);
}

script_command_t script_log_cmds[] = {
	{"", "putlog", script_putlog, NULL, 1, "s", "text", SCRIPT_INTEGER, 0},
	{"", "putloglev", script_putloglev, NULL, 3, "sss", "level channel text", SCRIPT_INTEGER, 0},
	{0}
};
