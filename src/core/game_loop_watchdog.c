#include "core/game_loop_watchdog.h"
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace
{
int progress_fd = -1;
pid_t world_pid = 0;
uint64_t completed_loops = 0;

bool on_world_thread()
{
	return progress_fd >= 0 && getpid() == world_pid && syscall(SYS_gettid) == world_pid;
}

void publish(char phase)
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return;
	const uint64_t monotonic_us = static_cast<uint64_t>(now.tv_sec) * 1000000 +
				      static_cast<uint64_t>(now.tv_nsec) / 1000;
	char record[128];
	const int length = snprintf(record, sizeof(record), "1 %ld %llu %llu %c\n",
				    static_cast<long>(world_pid),
				    static_cast<unsigned long long>(completed_loops),
				    static_cast<unsigned long long>(monotonic_us), phase);
	// A slow/dead observer must never block the world or raise SIGPIPE. The
	// observer's deadline also detects a broken/full progress channel.
	const ssize_t sent = send(progress_fd, record, length, MSG_DONTWAIT | MSG_NOSIGNAL);
	(void)sent;
}
} // namespace

bool game_loop_watchdog_init()
{
	// exec preserves interval timers. Disarm a checkpoint timer inherited
	// from an older binary before its now-absent SIGVTALRM handler can fire.
	struct itimerval disabled = {};
	if (setitimer(ITIMER_VIRTUAL, &disabled, nullptr) != 0)
		return false;
	const char *fd_text = getenv("DURIS_WATCHDOG_FD");
	if (!fd_text)
		return true;
	char *end = nullptr;
	errno = 0;
	const long fd = strtol(fd_text, &end, 10);
	if (errno || end == fd_text || *end || fd < 3 || fd > INT_MAX)
		return false;
	int type = 0;
	socklen_t size = sizeof(type);
	if (getsockopt(static_cast<int>(fd), SOL_SOCKET, SO_TYPE, &type, &size) != 0 ||
	    type != SOCK_DGRAM)
		return false;
	const char *sequence = getenv("DURIS_WATCHDOG_SEQUENCE");
	if (sequence)
	{
		errno = 0;
		const unsigned long long value = strtoull(sequence, &end, 10);
		if (errno || end == sequence || *end || *sequence == '-')
			return false;
		completed_loops = static_cast<uint64_t>(value);
	}
	progress_fd = static_cast<int>(fd);
	world_pid = getpid();
	publish('B');
	return true;
}

void game_loop_watchdog_completed()
{
	if (!on_world_thread())
		return;
	++completed_loops;
	publish('P');
}

void game_loop_watchdog_lifecycle(char phase)
{
	if (on_world_thread())
		publish(phase);
}

bool game_loop_watchdog_prepare_copyover()
{
	if (!on_world_thread())
		return progress_fd < 0;
	char sequence[32];
	snprintf(sequence, sizeof(sequence), "%llu",
		 static_cast<unsigned long long>(completed_loops));
	const int flags = fcntl(progress_fd, F_GETFD);
	return flags >= 0 && fcntl(progress_fd, F_SETFD, flags & ~FD_CLOEXEC) == 0 &&
	       setenv("DURIS_WATCHDOG_SEQUENCE", sequence, 1) == 0;
}
