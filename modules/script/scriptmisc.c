/*
 * scriptmisc.c --
 *
 *	Misc script commands
 */

#include <eggdrop/eggdrop.h>

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#include <sys/stat.h>

#ifdef HAVE_UNAME
#  include <sys/utsname.h>
#endif

static char *script_duration(unsigned int sec)
{
  char s[70];
  int tmp;

  s[0] = 0;
  if (sec >= 31536000) {
    tmp = (sec / 31536000);
    sprintf(s, "%d year%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 31536000);
  }
  if (sec >= 604800) {
    tmp = (sec / 604800);
    sprintf(&s[strlen(s)], "%d week%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 604800);
  }
  if (sec >= 86400) {
    tmp = (sec / 86400);
    sprintf(&s[strlen(s)], "%d day%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 86400);
  }
  if (sec >= 3600) {
    tmp = (sec / 3600);
    sprintf(&s[strlen(s)], "%d hour%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 3600);
  }
  if (sec >= 60) {
    tmp = (sec / 60);
    sprintf(&s[strlen(s)], "%d minute%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 60);
  }
  if (sec > 0 || !s[0]) {
    tmp = (sec);
    sprintf(&s[strlen(s)], "%d second%s", tmp, (tmp == 1) ? "" : "s");
  }
  if (strlen(s) > 0 && s[strlen(s)-1] == ' ') s[strlen(s)-1] = 0;
  return strdup(s);
}

static unsigned int script_unixtime()
{
	return(time(NULL));
}

static char *script_ctime(unsigned int sec)
{
	time_t tt;
	char *str;

	tt = (time_t) sec;
	str = ctime(&tt);
	str[strlen(str)-1] = 0;
	return(str);
}

static char *script_strftime(int nargs, char *format, unsigned int sec)
{
  char buf[512];
  struct tm *tm1;
  time_t t;

  if (nargs > 1) t = sec;
  else t = time(NULL);

  tm1 = localtime(&t);
  if (strftime(buf, sizeof(buf), format, tm1)) return strdup(buf);
  return strdup("error with strftime");
}

static int script_rand(int nargs, int min, int max)
{
	if (nargs == 1) {
		max = min;
		min = 0;
	}
	if (max <= min) return(max);

	return(random() % (max - min) + min);
}

static int script_die(const char *reason)
{
	eggdrop_event("die");
	return(0);
}

static char *script_unames()
{
	char buf[512];
#ifdef HAVE_UNAME
	struct utsname un;

	if (!uname(&un) < 0) snprintf(buf, sizeof buf, "unknown");
	else snprintf(buf, sizeof buf, "%s %s", un.sysname, un.release);
#else
	snprintf(buf, sizeof buf, "unknown");
#endif
	return strdup(buf);
}

static char *script_md5(char *data)
{
  MD5_CTX md5context;
  char *hex;
  unsigned char digest[16];

  MD5_Init(&md5context);
  MD5_Update(&md5context, data, strlen(data));
  MD5_Final(digest, &md5context);
  hex = malloc(33);
  MD5_Hex(digest, hex);
  return(hex);
}

int script_export(char *name, char *syntax, script_callback_t *callback)
{
	script_command_t new_command = {
		"", strdup(name), callback->callback, callback, strlen(syntax), strdup(syntax), strdup(syntax), SCRIPT_INTEGER, SCRIPT_PASS_CDATA
	};
	script_command_t *copy;

	callback->syntax = strdup(syntax);
	copy = calloc(2, sizeof(*copy));
	memcpy(copy, &new_command, sizeof(new_command));
	script_create_commands(copy);
	return(0);
}

script_command_t script_misc_cmds[] = {
	{"", "duration", (Function) script_duration, NULL, 1, "u", "seconds", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "unixtime", (Function) script_unixtime, NULL, 0, "", "", SCRIPT_UNSIGNED, 0},
	{"", "ctime", (Function) script_ctime, NULL, 1, "u", "seconds", SCRIPT_STRING, 0},
	{"", "strftime", (Function) script_strftime, NULL, 1, "su", "format ?seconds?", SCRIPT_STRING | SCRIPT_FREE, SCRIPT_PASS_COUNT | SCRIPT_VAR_ARGS},
	{"", "rand", (Function) script_rand, NULL, 1, "ii", "?min? max", SCRIPT_INTEGER, SCRIPT_PASS_COUNT | SCRIPT_VAR_ARGS},
	{"", "die", (Function) script_die, NULL, 0, "s", "?reason?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "unames", (Function) script_unames, NULL, 0, "", "", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "md5", (Function) script_md5, NULL, 1, "s", "data", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "eggdrop_event", (Function) eggdrop_event, NULL, 1, "s", "event", SCRIPT_INTEGER, 0},
	{"", "script_export", script_export, NULL, 3, "ssc", "export-name syntax callback", SCRIPT_INTEGER, 0},
	{0}
};
