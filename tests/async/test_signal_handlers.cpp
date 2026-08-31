#include <signal.h>
#include <stddef.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

// Standalone coverage for src/signals.c signal handlers and tics safety.
// This test compiles the production file directly with minimal stubs.
bool game_booted = false;
int shutdownflag = 0;
volatile sig_atomic_t tics = 0;
volatile sig_atomic_t signal_shutdown_pending = 0;
pid_t lookup_ident_process = 0;

void logit(const char *, const char *, ...) {}
void fatal_boot_error(const char *, const char *, ...)
{
	std::abort();
}

#include "../../src/core/signals.c"

namespace
{
int g_failures = 0;

void expect(bool cond, const char *message)
{
	if (!cond)
	{
		std::fprintf(stderr, "[signal-test] FAIL: %s\n", message);
		++g_failures;
	}
}

void reset_state()
{
	signal_shutdown_pending = 0;
	shutdownflag = 0;
	tics = 0;
}
} // namespace

static_assert(std::is_same_v<decltype(tics), volatile sig_atomic_t>,
	      "tics must be sig_atomic_t for signal safety");
static_assert(std::is_same_v<decltype(signal_shutdown_pending), volatile sig_atomic_t>,
	      "signal_shutdown_pending must be sig_atomic_t for signal safety");

int main()
{
	reset_state();
	shutdown_request(SIGUSR2);
	expect(signal_shutdown_pending == 1, "SIGUSR2 should set pending shutdown");
	expect(tics == 0, "shutdown_request must not touch tics");

	reset_state();
	shutdown_notice(SIGUSR1);
	expect(signal_shutdown_pending == 3, "SIGUSR1 should request copyover");
	expect(tics == 0, "shutdown_notice must not touch tics");

	reset_state();
	reboot_request(SIGRTMIN);
	expect(signal_shutdown_pending == 2, "SIGRTMIN should request reboot");
	expect(tics == 0, "reboot_request must not touch tics");

	reset_state();
	hupsig(SIGHUP);
	expect(signal_shutdown_pending == 1, "SIGHUP should route to graceful shutdown");
	expect(tics == 0, "hupsig must not touch tics");

	reset_state();
	logsig(SIGALRM);
	expect(signal_shutdown_pending == 0, "logsig should be a no-op on shutdown state");
	expect(tics == 0, "logsig must not touch tics");

	return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
