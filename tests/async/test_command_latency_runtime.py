#!/usr/bin/env python3
"""Runtime regression for bounded, privacy-safe command latency attribution."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC
from contract_text import contains, index


HARNESS = r'''
#include "net/command_latency.h"
#include "persistence/latency_trace.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

struct captured_lines
{
	std::vector<std::string> lines;
};

static void capture(const char *line, void *context)
{
	static_cast<captured_lines *>(context)->lines.emplace_back(line);
}

static bool any_contains(const captured_lines &captured, const char *needle)
{
	for (const std::string &line : captured.lines)
		if (line.find(needle) != std::string::npos)
			return true;
	return false;
}

static int count_contains(const captured_lines &captured, const char *needle)
{
	int count = 0;
	for (const std::string &line : captured.lines)
		if (line.find(needle) != std::string::npos)
			++count;
	return count;
}

int main()
{
	assert(command_latency_elapsed_us(100, 149) == 49);
	assert(command_latency_elapsed_us(100, 99) == 0);
	assert(command_latency_elapsed_us(0, 149) == 0);
	assert(!strcmp(command_latency_kind_name(COMMAND_LATENCY_SSL), "ssl"));

	command_latency_tracker thresholds = {};
	command_latency_event playing = {};
	char *player = strdup("PlayerOne\nInjected");
	char *input = strdup("  Ca$t ultra_secret password");
	assert(player && input);
	command_latency_event_prepare(&playing, COMMAND_LATENCY_PLAYING, 0, 4242, player, input);
	free(player);
	free(input);
	assert(!strcmp(playing.player_name, "PlayerOne?Injected"));
	assert(!strcmp(playing.operation, "Ca?t"));
	command_latency_record(&thresholds, &playing, 49999);
	assert(thresholds.slow_count == 0);
	command_latency_record(&thresholds, &playing, 50000);
	command_latency_record(&thresholds, &playing, 200000);
	assert(thresholds.slow_count == 2);
	assert(thresholds.kinds[COMMAND_LATENCY_PLAYING].count == 3);
	assert(thresholds.kinds[COMMAND_LATENCY_PLAYING].max_us == 200000);

	captured_lines threshold_output;
	command_latency_report(&thresholds, 260000, "boot-a", UINT64_C(1) << 40,
			       998877, capture, &threshold_output);
	assert(any_contains(threshold_output, "duration_us=50000"));
	assert(any_contains(threshold_output, "duration_us=200000"));
	assert(any_contains(threshold_output, "tick=1099511627776"));
	assert(any_contains(threshold_output, "pulse_start_mono_us=998877"));
	assert(any_contains(threshold_output, "player=PlayerOne?Injected"));
	assert(any_contains(threshold_output, "operation=Ca?t"));
	assert(!any_contains(threshold_output, "ultra_secret"));
	assert(!any_contains(threshold_output, "password"));
	for (const std::string &line : threshold_output.lines)
		assert(line.find('\n') == std::string::npos);
	for (const std::string &line : threshold_output.lines)
		if (line.find("COMMAND SWEEP KIND:") != std::string::npos)
			assert(line.find("pulse_start_mono_us=998877") != std::string::npos);

	captured_lines unavailable_output;
	command_latency_report(&thresholds, 260000, "boot-a", LATENCY_TRACE_TICK_UNAVAILABLE,
			       998877, capture, &unavailable_output);
	assert(any_contains(unavailable_output, "tick=-"));
	assert(!any_contains(unavailable_output, "18446744073709551615"));

	command_latency_event nanny = {};
	command_latency_event_prepare(&nanny, COMMAND_LATENCY_NANNY, 3, -1, nullptr,
				      "account-password-must-not-appear");
	assert(!strcmp(nanny.operation, "nanny"));
	command_latency_tracker aggregate = {};
	const command_latency_kind kinds[] = {
		COMMAND_LATENCY_PLAYING,
		COMMAND_LATENCY_NANNY,
		COMMAND_LATENCY_PAGER,
		COMMAND_LATENCY_EDITOR,
		COMMAND_LATENCY_SSL,
	};
	for (command_latency_kind kind : kinds)
	{
		command_latency_event event = {};
		command_latency_event_prepare(&event, kind, (int)kind, -1, nullptr, "look ignored");
		for (int count = 0; count < 10; ++count)
			command_latency_record(&aggregate, &event, 1000);
	}
	captured_lines aggregate_output;
	command_latency_report(&aggregate, 200000, "boot-b", 77, 123, capture,
			       &aggregate_output);
	assert(aggregate.slow_count == 0);
	assert(aggregate.measured_us == 50000);
	assert(any_contains(aggregate_output, "maintenance_residual_us=150000"));
	assert(count_contains(aggregate_output, "COMMAND SWEEP KIND:") == 5);
	for (command_latency_kind kind : kinds)
	{
		const std::string expected =
			std::string("kind=") + command_latency_kind_name(kind) +
			" count=10 total_us=10000 max_us=1000";
		assert(any_contains(aggregate_output, expected.c_str()));
	}
	assert(!any_contains(aggregate_output, "account-password"));

	command_latency_tracker delayed_ssl = {};
	command_latency_event ssl = {};
	command_latency_event_prepare(&ssl, COMMAND_LATENCY_SSL, 87, -1, nullptr, nullptr);
	command_latency_record(&delayed_ssl, &ssl, 50000);
	captured_lines ssl_output;
	command_latency_report(&delayed_ssl, 50000, "boot-c", 88, 456, capture, &ssl_output);
	assert(any_contains(ssl_output, "kind=ssl state=87"));

	command_latency_tracker capped = {};
	for (uint64_t index = 0; index < 12; ++index)
	{
		command_latency_event event = {};
		char command[32];
		snprintf(command, sizeof command, "command%" PRIu64 " secret", index);
		command_latency_event_prepare(&event, COMMAND_LATENCY_PLAYING, 0, (long)index,
					      "Tester", command);
		command_latency_record(&capped, &event, 50000 + index);
	}
	assert(capped.slow_count == 12);
	assert(capped.retained_slow_count == COMMAND_LATENCY_MAX_REPORTS);
	assert(capped.slowest[0].duration_us == 50011);
	captured_lines capped_output;
	command_latency_report(&capped, 700000, "boot-d", 99, 789, capture, &capped_output);
	assert(count_contains(capped_output, "COMMAND OP SLOW:") == COMMAND_LATENCY_MAX_REPORTS);
	assert(any_contains(capped_output, "duration_us=50011"));
	assert(any_contains(capped_output, "suppressed=4"));
	assert(capped_output.lines.size() <= COMMAND_LATENCY_MAX_REPORTS + 2);

	command_latency_tracker quiet = {};
	command_latency_record(&quiet, &playing, 49999);
	captured_lines quiet_output;
	command_latency_report(&quiet, 49999, "boot-e", 100, 900, capture, &quiet_output);
	assert(quiet_output.lines.empty());

	puts("command latency runtime checks passed");
	return 0;
}
'''


comm = (SRC / "net" / "comm.c").read_text()
makefile = (SRC / "Makefile").read_text()
dispatch_start = index(comm, "if (point->showstr_count)")
dispatch_end = index(comm, "const uint64_t command_sweep_us", dispatch_start)
dispatch = comm[dispatch_start:dispatch_end]

assert contains(dispatch, "show_string(point, comm);")
assert contains(dispatch, "string_add(point, comm);")
assert contains(dispatch, "dispatch_playing_command(t_ch, comm);")
assert contains(dispatch, "nanny(point, comm);")
assert contains(dispatch, "COMMAND_LATENCY_PAGER")
assert contains(dispatch, "COMMAND_LATENCY_EDITOR")
assert contains(dispatch, "COMMAND_LATENCY_PLAYING")
assert contains(dispatch, "COMMAND_LATENCY_NANNY")
assert contains(comm, "COMMAND_LATENCY_SSL")
assert contains(
    comm,
    "command_latency_report(&command_latency, command_sweep_us, "
    "latency_trace_boot_id(), loop_tick, loop_start_mono_us",
)
reporting_start = index(comm, "command_latency_log_buffer command_report")
reporting_end = index(comm, "PROFILE_START(prompts)", reporting_start)
reporting = comm[reporting_start:reporting_end]
assert contains(reporting, 'logit(LOG_STATUS, "%s", command_report.text);')
assert not contains(reporting, "statuslog(")
assert contains(comm, "COMMAND_LATENCY_REPORT_INTERVAL_PULSES")
assert index(comm, 'latency_trace_record("commands", command_sweep_us') < reporting_start
assert contains(makefile, "net/command_latency.o")
assert not contains((SRC / "net" / "command_latency.c").read_text(), "do_profile")

with tempfile.TemporaryDirectory(prefix="duris-command-latency-") as directory:
    temporary = Path(directory)
    harness = temporary / "command_latency_test.c"
    binary = temporary / "command_latency_test"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-g",
            "-O1",
            "-fsanitize=address,undefined",
            "-I",
            str(SRC),
            str(harness),
            str(SRC / "net" / "command_latency.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("command loop attribution source contracts passed")
