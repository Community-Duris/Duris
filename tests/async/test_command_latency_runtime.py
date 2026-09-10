#!/usr/bin/env python3
"""Runtime regression for bounded, privacy-safe command latency attribution."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC
from contract_text import contains, index


interp = (SRC / "cmd" / "interp.c").read_text()
def function(name):
    start = interp.index(name)
    opening = interp.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (interp[end] == "{") - (interp[end] == "}")
        end += 1
    return interp[start:end]
lookup = "\n".join([
    '#include <ctype.h>\n#include <string.h>\n#include <sys/types.h>\n#include "cmd/interp.h"',
    '#define MAX_INPUT_LENGTH 1024\n#define MAX_CMD 1024\n#define LOWER(c) tolower((unsigned char)(c))',
    interp[interp.index("const char *command[MAX_CMD]"):interp.index("};", interp.index("const char *command[MAX_CMD]")) + 2],
    function("int old_search_block("),
    function("static int input_command_number("),
    function("const char *input_command_label("),
])

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
	assert(!strcmp(command_latency_kind_name(COMMAND_LATENCY_SSL), "ssl"));
	assert(!strcmp(command_latency_kind_name(COMMAND_LATENCY_DESCRIPTOR), "descriptor"));

	command_latency_tracker thresholds = {};
	command_latency_event playing = {};
	char *player = strdup("PlayerOne\nInjected");
	char *input = strdup("  Ca$t ultra_secret password");
	assert(player && input);
	command_latency_event_prepare(&playing, COMMAND_LATENCY_PLAYING, 0, 4242, player, input_command_label(input));
	free(player);
	free(input);
	assert(!strcmp(playing.player_name, "PlayerOne?Injected"));
	assert(!strcmp(playing.operation, "unknown"));
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
	assert(any_contains(threshold_output, "operation=unknown"));
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
		COMMAND_LATENCY_DESCRIPTOR,
	};
	for (command_latency_kind kind : kinds)
	{
		command_latency_event event = {};
		command_latency_event_prepare(&event, kind, (int)kind, -1, nullptr, "look");
		for (int count = 0; count < 10; ++count)
			command_latency_record(&aggregate, &event, 1000);
	}
	captured_lines aggregate_output;
	command_latency_report(&aggregate, 200000, "boot-b", 77, 123, capture,
			       &aggregate_output);
	assert(aggregate.slow_count == 0);
	assert(aggregate.measured_us == 60000);
	assert(any_contains(aggregate_output, "unattributed_sweep_us=140000"));
	assert(count_contains(aggregate_output, "COMMAND SWEEP KIND:") == 6);
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
					      "Tester", input_command_label(command));
		command_latency_record(&capped, &event, 50000 + index);
	}
	assert(capped.slow_count == 12);
	assert(capped.retained_slow_count == COMMAND_LATENCY_MAX_REPORTS);
	assert(capped.slowest[0].duration_us == 50011);
	captured_lines capped_output;
	command_latency_report(&capped, 700000, "boot-d", 99, 789, capture, &capped_output);
	assert(count_contains(capped_output, "COMMAND OP SLOW:") == COMMAND_LATENCY_MAX_REPORTS);
	assert(any_contains(capped_output, "duration_us=50011"));
	assert(any_contains(capped_output, "unreported_slow_operations=4"));
	assert(capped_output.lines.size() <= COMMAND_LATENCY_MAX_REPORTS + 2);

	command_latency_report_state report_state = {};
	captured_lines first_full;
	command_latency_report_throttled(&report_state, &thresholds, 260000, "boot-f", 1000,
					  100000, capture, &first_full);
	assert(any_contains(first_full, "COMMAND OP SLOW:"));
	assert(!any_contains(first_full, "COMMAND REPORT THROTTLED:"));

	captured_lines first_throttled;
	command_latency_report_throttled(&report_state, &capped, 700000, "boot-f", 1001,
					  101000, capture, &first_throttled);
	assert(first_throttled.lines.empty());

	captured_lines second_throttled;
	command_latency_report_throttled(&report_state, &delayed_ssl, 50000, "boot-f", 1002,
					  102000, capture, &second_throttled);
	assert(second_throttled.lines.empty());

	captured_lines next_full;
	command_latency_report_throttled(&report_state, &thresholds, 260000, "boot-f", 1004,
					  104000, capture, &next_full);
	assert(any_contains(next_full, "COMMAND REPORT THROTTLE:"));
	assert(any_contains(next_full, "suppressed_reports=2"));
	assert(any_contains(next_full, "suppressed_slow_operations=13"));
	assert(any_contains(next_full, "suppressed_worst_tick=1001"));
	assert(any_contains(next_full, "suppressed_worst_pulse_start_mono_us=101000"));
	assert(any_contains(next_full, "suppressed_worst_operation=unknown"));
	assert(any_contains(next_full, "suppressed_worst_duration_us=50011"));
	assert(any_contains(next_full, "COMMAND OP SLOW:"));

	command_latency_log_buffer report_buffer;
	command_latency_log_buffer_reset(&report_buffer, "stamp::");
	command_latency_log_buffer_collect("first", &report_buffer);
	command_latency_log_buffer_collect("second", &report_buffer);
	assert(!strcmp(report_buffer.text, "first\nstamp::second"));

	command_latency_log_buffer_reset(&report_buffer, nullptr);
	assert(report_buffer.length == 0);
	assert(report_buffer.text[0] == '\0');
	assert(!strcmp(report_buffer.continuation_prefix, "timestamp-unavailable::"));

	const std::string nearly_full(COMMAND_LATENCY_REPORT_BUFFER_SIZE - 3, 'x');
	command_latency_log_buffer_collect(nearly_full.c_str(), &report_buffer);
	const size_t nearly_full_length = report_buffer.length;
	command_latency_log_buffer_collect("cannot-fit-after-prefix", &report_buffer);
	assert(report_buffer.length == nearly_full_length);
	assert(report_buffer.text[report_buffer.length] == '\0');

	command_latency_log_buffer_reset(&report_buffer, "stamp::");
	const std::string oversized(COMMAND_LATENCY_REPORT_BUFFER_SIZE + 100, 'y');
	command_latency_log_buffer_collect(oversized.c_str(), &report_buffer);
	assert(report_buffer.length == COMMAND_LATENCY_REPORT_BUFFER_SIZE - 1);
	assert(report_buffer.text[report_buffer.length] == '\0');
	command_latency_log_buffer_collect("ignored", &report_buffer);
	assert(report_buffer.length == COMMAND_LATENCY_REPORT_BUFFER_SIZE - 1);

	command_latency_tracker quiet = {};
	command_latency_record(&quiet, &playing, 49999);
	captured_lines quiet_output;
	command_latency_report(&quiet, 49999, "boot-e", 100, 900, capture, &quiet_output);
	assert(quiet_output.lines.empty());


 assert(!strcmp(input_command_label("  LoO secret"), "look"));
 assert(!strcmp(input_command_label("PASSWORD-typed-at-wrong-prompt"), "unknown"));
 assert(!strcmp(input_command_label("Ca?t"), "unknown"));
 assert(!strcmp(input_command_label(""), "unknown"));
 assert(!strcmp(input_command_label(nullptr), "unknown"));
 assert(!strcmp(input_command_label("' hello"), "say"));

 // Even after recovery, flush pending incident evidence on the next due pulse.
 command_latency_report_state recovered = {};
 captured_lines recovery_output;
 command_latency_report_throttled(&recovered, &thresholds, 260000, "boot", 1, 1, capture, &recovery_output);
 recovery_output.lines.clear();
 command_latency_report_throttled(&recovered, &capped, 700000, "boot", 2, 2, capture, &recovery_output);
 assert(recovery_output.lines.empty());
 command_latency_report_throttled(&recovered, &quiet, 1, "boot", 5, 5, capture, &recovery_output);
 assert(any_contains(recovery_output, "suppressed_reports=1"));
 assert(!any_contains(recovery_output, "COMMAND OP SLOW:"));

 // Missing ticks must still bound the number of emitter calls.
 command_latency_report_state unavailable = {};
 for (int pulse = 0; pulse < 12; ++pulse)
 {
  captured_lines out;
  command_latency_report_throttled(&unavailable, &thresholds, 260000, "boot",
    LATENCY_TRACE_TICK_UNAVAILABLE, pulse, capture, &out);
  assert(out.lines.empty() == (pulse % 4 != 0));
 }
 command_latency_tracker invalid = {};
 command_latency_record(&invalid, &playing, LATENCY_TRACE_DURATION_INVALID);
 assert(invalid.measured_us == 0 && invalid.slow_count == 0);
 captured_lines invalid_output;
 command_latency_report(&invalid, LATENCY_TRACE_DURATION_INVALID, "boot", 1, 1, capture, &invalid_output);
 assert(invalid_output.lines.empty());

 // Actual descriptor timer ends at the prologue; queue/gate work remains residual.
 command_latency_tracker prologue = {};
 command_latency_event descriptor = {};
 command_latency_event_prepare(&descriptor, COMMAND_LATENCY_DESCRIPTOR, 0, -1, nullptr, nullptr);
 test_now_us = 1;
 {
  scoped_command_latency measured(&prologue, &descriptor);
  test_now_us += 1000;
  measured.finish();
  test_now_us += 80000; // queue/gate work after the explicit prologue boundary
 }
 assert(prologue.measured_us == 1000);
 assert(prologue.kinds[COMMAND_LATENCY_DESCRIPTOR].count == 1);
 captured_lines residual;
 command_latency_report(&prologue, 81000, "boot", 5, 1, capture, &residual);
 assert(any_contains(residual, "unattributed_sweep_us=80000"));
 for (int i = 0; command[i][0] != '\n'; ++i)
  assert(strlen(command[i]) < COMMAND_LATENCY_OPERATION_LENGTH);
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
assert not contains(dispatch, "descriptor_latency.finish();")
assert index(comm, "descriptor_latency.finish();") < index(comm, "casting_input =", index(comm, "/* process_commands */"))
assert contains(comm, "COMMAND_LATENCY_SSL")
assert contains(comm, "COMMAND_LATENCY_DESCRIPTOR")
assert contains(comm, "command_latency_report_throttled(&command_report_state, &command_latency,")
reporting_start = index(comm, "command_latency_log_buffer command_report")
reporting_end = index(comm, "PROFILE_START(prompts)", reporting_start)
reporting = comm[reporting_start:reporting_end]
assert contains(reporting, 'logit(LOG_STATUS, "%s", command_report.text);')
assert not contains(reporting, "statuslog(")
assert contains(comm, "if (!report->length) prepare_command_latency_log_buffer(report);")
assert contains(comm, "collect_command_latency_log, &command_report")
assert contains(comm, "continuation_prefix")
assert contains(comm, "timestamp-unavailable::")
assert contains(
    (SRC / "net" / "command_latency.h").read_text(),
    "COMMAND_LATENCY_REPORT_INTERVAL_PULSES",
)
assert index(comm, 'latency_trace_record("commands", command_sweep_us') < reporting_start
assert contains(makefile, "net/command_latency.o")
assert not contains((SRC / "net" / "command_latency.c").read_text(), "do_profile")

with tempfile.TemporaryDirectory(prefix="duris-command-latency-") as directory:
    temporary = Path(directory)
    harness = temporary / "command_latency_test.c"
    binary = temporary / "command_latency_test"
    scoped = comm[comm.index("class scoped_command_latency"):comm.index("/** Select normal", comm.index("class scoped_command_latency"))]
    clock_stub = "static uint64_t test_now_us = 1; static uint64_t loop_monotonic_us() { return test_now_us; }\n"
    source = HARNESS.replace("int main()", clock_stub + scoped + "\nint main()")
    harness.write_text(lookup + source)
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
            str(SRC / "persistence" / "latency_trace.c"),
            "-pthread",
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("command loop attribution source contracts passed")
