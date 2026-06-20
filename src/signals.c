/*
 ***************************************************************************
 *  File: signals.c                                          Part of Duris *
 *  Usage: Signal Trapping.                                                  *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

extern void exit(int);

/*
   external variables
 */

extern int  tics;
extern bool game_booted;
extern int  shutdownflag;
// signal-initiated shutdown: 0=none, 1=shutdown, 2=reboot, 3=copyover
extern volatile sig_atomic_t signal_shutdown_pending;

// extern pid_t lookup_host_process;
extern pid_t lookup_ident_process;
void         reap(int sig);

void shutdown_request(int);
void shutdown_notice(int);
void reboot_request(int);
void hupsig(int);
void logsig(int);
void reap(int);
void checkpointing(void);
static void checkpointing_signal(int);

void signal_setup(void)
{
	struct itimerval itime;
	struct timeval   interval;

	signal(SIGUSR2, shutdown_request); // shutdown (no restart)
	signal(SIGUSR1, shutdown_notice);  // copyover
	signal(SIGRTMIN, reboot_request);  // reboot

	/*
	   just to be on the safe side:
	 */

	signal(SIGHUP, hupsig);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, hupsig);
	signal(SIGALRM, logsig);
	signal(SIGTERM, hupsig);
	/* new by fafhrd 11/28/99 */
	signal(SIGCHLD, reap);

	/*
	   set up the deadlock-protection
	 */

	// Start timer 900 sec after boot starts (15 min).
	interval.tv_sec  = 900;
	interval.tv_usec = 0;
	itime.it_value   = interval;
	// And have timer check every 15 minutes.
	itime.it_interval = interval;
	// Changing this to 5 min since we don't need to hang for 15 min to know we're stuck.
	itime.it_interval.tv_sec = 300;
	setitimer(ITIMER_VIRTUAL, &itime, 0);
	signal(SIGVTALRM, checkpointing_signal);
}

static volatile sig_atomic_t checkpoint_strikes = 0;
static volatile sig_atomic_t checkpoint_pending = 0;

void checkpointing(void)
{
	if (!checkpoint_pending)
	{
		return;
	}

	if (checkpoint_strikes < 2)
	{
		logit(LOG_EXIT, "CHECKPOINT warning: tics not updated (strike %d)", (int)checkpoint_strikes);
		checkpoint_pending = 0;
		return;
	}

	logit(LOG_EXIT, "CHECKPOINT shutdown: tics not updated (%d strikes)", (int)checkpoint_strikes);

	void *bt[64];
	int   n  = backtrace(bt, 64);
	int   fd = open(LOG_EXIT, O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (fd >= 0)
	{
		char msg[64];
		int  len = snprintf(msg, sizeof(msg), "\n--- hung backtrace #%d ---\n", (int)checkpoint_strikes);
		write(fd, msg, len);
		backtrace_symbols_fd(bt, n, fd);
		close(fd);
	}

	// The reason for this, is that we don't want to reboot into a hung-during-boot situation.
	// In other words, if the mud hangs during a boot, we just want to die completely until it's fixed.
	if (game_booted)
	{
		exit(56);
	}
	else
	{
		exit(-1);
	}
}

static void checkpointing_signal(int signum)
{
	(void)signum;

	if (!tics)
	{
		checkpoint_strikes = checkpoint_strikes + 1;
		checkpoint_pending = 1;
	}
	else
	{
		tics              = 0;
		checkpoint_strikes = 0;
		checkpoint_pending  = 0;
	}

	signal(SIGVTALRM, checkpointing_signal);
}

// sigusr1 - copyover request from launcher
void shutdown_notice(int signum)
{
	signal_shutdown_pending = 3; // copyover
	signal(SIGUSR1, shutdown_notice);
}

// sigusr2 - clean shutdown (no restart)
void shutdown_request(int signum)
{
	signal_shutdown_pending = 1; // shutdown
	signal(SIGUSR2, shutdown_request);
}

// sigrtmin - reboot request from launcher
void reboot_request(int signum)
{
	signal_shutdown_pending = 2; // reboot
	signal(SIGRTMIN, reboot_request);
}

/*
   kick out players etc
 */
void hupsig(int signum)
{
	signal_shutdown_pending = 1;
	signal(signum, hupsig);
}

void logsig(int signum)
{
	(void)signum;
}

/* This should do the trick... fafhrd 11/28/99 */

/* clean up our zombie kids to avoid defunct processes */
void reap(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;

	signal(SIGCHLD, reap);
}

void reaper(int signum) {}
