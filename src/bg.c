/* bg.c: handles moving the process to the background and forking, while
 *       keeping threads happy.
 *
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: bg.c,v 1.19 2004/06/22 10:54:42 wingman Exp $";
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>	/* fork(), setpgid() */
#include <stdio.h>	/* printf() */
#include <stdlib.h>	/* exit() */
#include <sys/types.h>	/* pid_t, kill() */
#include <sys/wait.h>	/* waitpid() */
#include <signal.h>	/* kill() */
#include <eggdrop/eggdrop.h>
#include "main.h"	/* fatal()*/
#include "bg.h"

/* When threads are started during eggdrop's init phase, we can't simply
 * fork() later on, because that only copies the VM space over and
 * doesn't actually duplicate the threads.
 *
 * To work around this, we fork() very early and let the parent process
 * wait in an event loop. As soon as the init phase is completed and we
 * would normally move to the background, the child process simply
 * signals its parent that it may now quit. This allows us to control
 * the terminal long enough to, e.g. properly feed error messages to
 * cron scripts and let the user abort the loading process by hitting
 * CTRL+C.
 *
 */

/* The child has to keep the parent's pid so it can send it a signal when it's
 * time to exit. */
static pid_t parent_pid = -1, child_pid = -1;

void wait_for_child(int sig)
{
	printf("Eggdrop launched successfully into the background, pid = %d.\n", child_pid);
	exit(0);
}

void bg_begin_split()
{
	int result;

	parent_pid = getpid();

	child_pid = fork();
	if (child_pid == -1) fatal("CANNOT FORK PROCESS.");

	/* Are we the child? */
	if (child_pid == 0) {
		/* Yes. Continue as normal. */
		return;
	}

	/* We are the parent. Just hang around until the child is done. When
	 * the child wants us to exit, it will send us the signal and trigger
	 * wait_for_child. */
	signal(SIGUSR1, wait_for_child);
	waitpid(child_pid, &result, 0);

	/* If we reach this point, that means the child process exited, so
	 * there was an error. */
	printf("Eggdrop exited abnormally!\n");
	exit(1);
}

void bg_close()
{
	/* Send parent the USR1 signal, which tells it to print a message
	 * and exit. */
	kill(parent_pid, SIGUSR1);
}

void bg_finish_split()
{
	bg_close();

#if HAVE_SETPGID && !defined(CYGWIN_HACKS)
	setpgid(0, 0);
#endif
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);

#ifdef CYGWIN_HACKS
	FreeConsole();
#endif
}
