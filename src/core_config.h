#ifndef _CORE_CONFIG_H_
#define _CORE_CONFIG_H_

typedef struct {
	/* General bot stuff. */
	char *botname;	/* Name of the bot as seen by user. */
	char *userfile;	/* File we store users in. */

	/* Telnet stuff. */
	char *telnet_vhost;
	int telnet_port;
	int telnet_stealth;
	int telnet_max_retries;

	/* Logfile. */
	char *logfile_suffix;
	int max_logsize;
	int switch_logfiles_at;
	int keep_all_logs;
	int quick_logs;
} core_config_t;

extern core_config_t core_config;

void core_config_init();
void core_config_save();

#endif
