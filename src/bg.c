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
static const char rcsid[] = "$Id: bg.c,v 1.22 2004/10/17 05:14:07 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

#ifdef CYGWIN_HACKS
#  include <windows.h>
#endif

#include <unistd.h>	/* fork(), setpgid() */
#include <sys/wait.h>	/* waitpid() */
#include <signal.h>	/* kill() */
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
static int pipefd[2];

void bg_begin_split()
{
	int parent_pid = -1, child_pid = -1;
	int result;
	char temp = 0;

	parent_pid = getpid();

	pipe(pipefd);

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
	close(pipefd[1]);
	result = read(pipefd[0], &temp, 1);

	if (result <= 0) {
		printf("Eggdrop exited abnormally!\n");
		exit(1);
	}
	else {
		printf("Eggdrop launched successfully into the background, pid = %d.\n", child_pid);
		exit(0);
	}
}

void bg_finish_split()
{
	char temp = 0;
	write(pipefd[1], &temp, 1);
	close(pipefd[1]);
	close(pipefd[0]);

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
