/*
 ***************************************************************************
 *  File: signals.c                                          Part of Duris *
 *  Usage: Signal Trapping.                                                  *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern void exit(int);

/*
   external variables
 */

extern volatile sig_atomic_t tics;
extern int shutdownflag;
// signal-initiated shutdown: 0=none, 1=shutdown, 2=reboot, 3=copyover
extern volatile sig_atomic_t signal_shutdown_pending;

// extern pid_t lookup_host_process;
void reap(int sig);

#define REAPED_CHILD_STATUS_CAPACITY 64
static volatile sig_atomic_t reaped_child_pids[REAPED_CHILD_STATUS_CAPACITY];
static volatile sig_atomic_t reaped_child_statuses[REAPED_CHILD_STATUS_CAPACITY];
static volatile sig_atomic_t reaped_child_cursor = 0;

void shutdown_request(int);
void shutdown_notice(int);
void reboot_request(int);
void hupsig(int);
void logsig(int);
void reap(int);

static void install_signal_handler(int signo, void (*handler)(int), int flags)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags = flags;
	sigemptyset(&sa.sa_mask);
	if (sigaction(signo, &sa, NULL) < 0)
	{
		fatal_boot_error("signals", "sigaction(%d) failed: %s", signo, strerror(errno));
	}
}

void signal_setup(void)
{
	install_signal_handler(SIGUSR2, shutdown_request, SA_RESTART); // shutdown (no restart)
	install_signal_handler(SIGUSR1, shutdown_notice, SA_RESTART); // copyover
	install_signal_handler(SIGRTMIN, reboot_request, SA_RESTART); // reboot

	/*
	   just to be on the safe side:
	 */

	install_signal_handler(SIGHUP, hupsig, SA_RESTART);
	signal(SIGPIPE, SIG_IGN);
	install_signal_handler(SIGINT, hupsig, SA_RESTART);
	install_signal_handler(SIGALRM, logsig, SA_RESTART);
	install_signal_handler(SIGTERM, hupsig, SA_RESTART);
	/* new by fafhrd 11/28/99 */
	install_signal_handler(SIGCHLD, reap, SA_RESTART | SA_NOCLDSTOP);

	// Completed-loop deadlines are enforced by the launcher's external
	// observer. A signal handler cannot recover a callback that never returns.
}

// sigusr1 - copyover request from launcher
void shutdown_notice(int signum)
{
	(void)signum;
	signal_shutdown_pending = 3; // copyover
}

// sigusr2 - clean shutdown (no restart)
void shutdown_request(int signum)
{
	(void)signum;
	signal_shutdown_pending = 1; // shutdown
}

// sigrtmin - reboot request from launcher
void reboot_request(int signum)
{
	(void)signum;
	signal_shutdown_pending = 2; // reboot
}

/*
   kick out players etc
 */
void hupsig(int signum)
{
	(void)signum;
	signal_shutdown_pending = 1;
}

void logsig(int signum)
{
	(void)signum;
}

/* This should do the trick... fafhrd 11/28/99 */

/* clean up our zombie kids to avoid defunct processes */
void reap(int sig)
{
	(void)sig;
	int status = 0;
	pid_t pid;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		sig_atomic_t slot = reaped_child_cursor % REAPED_CHILD_STATUS_CAPACITY;
		reaped_child_statuses[slot] = status;
		reaped_child_pids[slot] = (sig_atomic_t)pid;
		reaped_child_cursor = reaped_child_cursor + 1;
	}
}

bool take_reaped_child_status(pid_t pid, int *status)
{
	if (pid <= 0 || !status)
		return false;

	sigset_t block_set;
	sigset_t old_set;
	sigemptyset(&block_set);
	sigaddset(&block_set, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &block_set, &old_set) < 0)
		return false;

	bool found = false;
	for (int i = 0; i < REAPED_CHILD_STATUS_CAPACITY; ++i)
	{
		if (reaped_child_pids[i] != (sig_atomic_t)pid)
			continue;
		*status = (int)reaped_child_statuses[i];
		reaped_child_pids[i] = 0;
		found = true;
		break;
	}
	sigprocmask(SIG_SETMASK, &old_set, NULL);
	return found;
}

void reaper(int /*signum*/) {}
