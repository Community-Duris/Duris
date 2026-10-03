#include "core/game_loop_watchdog.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <signal.h>
#include <string>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <thread>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;

static void stop(int)
{
	stop_requested = 1;
}

static void progressing(int count = 12)
{
	for (int i = 0; i < count; ++i)
	{
		usleep(50000);
		game_loop_watchdog_completed();
	}
}

static void independent_worker()
{
	std::thread(
		[]
		{
			for (;;)
			{
				game_loop_watchdog_completed(); // Must be ignored off the world thread.
				usleep(10000);
			}
		})
		.detach();
}

static void cpu_hang()
{
	for (;;)
		asm volatile("" ::: "memory");
}

static void legacy_timer_handler(int) {}

int main(int argc, char **argv)
{
	// Tests never leave cores or invoke a configured real server/database.
	struct rlimit no_core = { 0, 0 };
	assert(setrlimit(RLIMIT_CORE, &no_core) == 0);
	assert(game_loop_watchdog_init());
	std::string mode = getenv("WATCHDOG_TEST_MODE") ? getenv("WATCHDOG_TEST_MODE") : "normal";
	if (argc > 1 && argv[1][0] != '-')
		mode = argv[1];
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	assert(listener >= 0);
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	assert(bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
	assert(listen(listener, 4) == 0);
	socklen_t length = sizeof(address);
	assert(getsockname(listener, reinterpret_cast<sockaddr *>(&address), &length) == 0);
	printf("world pid=%ld mode=%s port=%u\n", static_cast<long>(getpid()), mode.c_str(),
	       static_cast<unsigned int>(ntohs(address.sin_port)));
	fflush(stdout);
	if (mode == "startup")
		usleep(450000);
	if (mode == "startup-hang")
		cpu_hang();
	if (mode == "replacement" || mode == "replacement-hang" || mode == "timer-replacement")
	{
		assert(getenv("DURIS_WATCHDOG_SEQUENCE"));
		assert(strcmp(getenv("DURIS_WATCHDOG_SEQUENCE"), "1") == 0);
		if (mode == "timer-replacement")
		{
			struct itimerval inherited;
			assert(getitimer(ITIMER_VIRTUAL, &inherited) == 0);
			assert(inherited.it_value.tv_sec == 0 && inherited.it_value.tv_usec == 0);
		}
		usleep(450000);
		if (mode == "replacement-hang")
			cpu_hang();
	}
	game_loop_watchdog_completed();
	if (mode == "copyover" || mode == "copyover-hang" || mode == "legacy-copyover")
	{
		game_loop_watchdog_lifecycle('C');
		assert(game_loop_watchdog_prepare_copyover());
		close(listener);
		if (mode == "legacy-copyover")
		{
			signal(SIGVTALRM, legacy_timer_handler);
			struct itimerval timer = {};
			timer.it_value.tv_sec = 1;
			assert(setitimer(ITIMER_VIRTUAL, &timer, nullptr) == 0);
			execl(argv[0], argv[0], "timer-replacement", static_cast<char *>(nullptr));
			std::abort();
		}
		execl(argv[0], argv[0], mode == "copyover" ? "replacement" : "replacement-hang",
		      static_cast<char *>(nullptr));
		std::abort();
	}
	if (mode == "recover")
	{
		const char *marker = getenv("WATCHDOG_TEST_RESTART_FILE");
		assert(marker);
		const bool first = access(marker, F_OK) != 0;
		std::ofstream(marker, std::ios::app) << getpid() << '\n';
		if (first)
			cpu_hang();
	}
	if (mode == "cpu" || mode == "blocking" || mode == "abort-ignored" || mode == "worker-only")
	{
		independent_worker();
		if (mode == "abort-ignored")
			signal(SIGABRT, SIG_IGN);
		if (mode == "blocking" || mode == "worker-only")
		{
			int pipe_fds[2];
			assert(pipe(pipe_fds) == 0);
			char byte;
			assert(read(pipe_fds[0], &byte, 1) == 1);
			std::abort();
		}
		cpu_hang();
	}
	if (mode == "duplicate" || mode == "foreign")
	{
		const pid_t owner = getpid();
		if (mode == "foreign" && fork() != 0)
			cpu_hang();
		unsigned long long sequence = 1;
		for (;;)
		{
			struct timespec now;
			assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
			char record[128];
			const int size =
				snprintf(record, sizeof(record), "1 %ld %llu %llu P\n",
					 static_cast<long>(owner), sequence,
					 static_cast<unsigned long long>(now.tv_sec) * 1000000 +
						 now.tv_nsec / 1000);
			send(atoi(getenv("DURIS_WATCHDOG_FD")), record, size,
			     MSG_DONTWAIT | MSG_NOSIGNAL);
			if (mode == "foreign")
				++sequence; // A fork cannot impersonate the game's kernel credentials.
			usleep(10000);
		}
	}
	if (mode == "drain-retry")
	{
		for (;;)
		{
			game_loop_watchdog_lifecycle('C');
			usleep(10000);
			game_loop_watchdog_lifecycle('R');
			usleep(10000);
		}
	}
	if (mode == "drain" || mode == "drain-hang")
	{
		game_loop_watchdog_lifecycle('C');
		if (mode == "drain-hang")
			for (;;)
			{
				game_loop_watchdog_lifecycle('C'); // Cannot renew copyover grace.
				usleep(10000);
			}
		usleep(450000);
		game_loop_watchdog_lifecycle('R');
	}
	if (mode == "shutdown" || mode == "shutdown-hang" || mode == "reboot")
	{
		game_loop_watchdog_lifecycle(mode == "reboot" ? 'D' : 'S');
		if (mode == "shutdown-hang")
			cpu_hang();
		usleep(450000);
		return mode == "reboot" ? 52 : 0;
	}
	if (mode == "signal-stop")
	{
		signal(SIGTERM, stop);
		while (!stop_requested)
			progressing(1);
		game_loop_watchdog_lifecycle('S');
		usleep(450000);
		return 0;
	}
	progressing(mode == "normal-long" ? 40 : 12);
	return 0;
}
