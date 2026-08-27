#!/usr/bin/env python3
"""Periodic nevent rearm behavior and maintenance call-site contracts."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "prototypes.h"

#include <cstdlib>
#include <deque>
#include <vector>

struct scheduled_call
{
	event_func callback;
	int delay;
};

static std::deque<scheduled_call> queue;
static std::vector<int> scheduled_delays;
static bool redis_enabled_for_test = false;
static int local_checkpoints = 0;
static int redis_flushes = 0;
static int artifact_run = 0;
static int artifact_successes = 0;

void add_event(event_func callback, int delay, P_char, P_char, P_obj, int, const void *, int)
{
	queue.push_back({ callback, delay });
	scheduled_delays.push_back(delay);
}

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

static void dirty_checkpoint(P_char, P_char, P_obj, void *)
{
	nevent_rearm_guard rearm(dirty_checkpoint, 30);
	local_checkpoints++;
	if (redis_enabled_for_test)
		redis_flushes++;
}

static void artifact_maintenance(P_char, P_char, P_obj, void *)
{
	nevent_rearm_guard rearm(artifact_maintenance, 100);
	artifact_run++;
	if (artifact_run <= 2)
	{
		rearm.retry_after(5);
		return;
	}
	if (artifact_run == 3)
		return;
	artifact_successes++;
}

static void advance_one()
{
	require(!queue.empty(), 90);
	const scheduled_call call = queue.front();
	queue.pop_front();
	call.callback(nullptr, nullptr, nullptr, nullptr);
}

static void reset_queue()
{
	queue.clear();
	scheduled_delays.clear();
}

static void test_dirty_checkpoint(bool redis_enabled)
{
	reset_queue();
	redis_enabled_for_test = redis_enabled;
	local_checkpoints = 0;
	redis_flushes = 0;
	add_event(dirty_checkpoint, 1, nullptr, nullptr, nullptr, 0, nullptr, 0);

	for (int interval = 0; interval < 3; interval++)
	{
		advance_one();
		require(queue.size() == 1, 1 + interval);
		require(queue.front().delay == 30, 4 + interval);
	}
	require(local_checkpoints == 3, 7);
	require(redis_flushes == (redis_enabled ? 3 : 0), 8);
}

static void test_artifact_failures_and_empty_result_rearm()
{
	reset_queue();
	artifact_run = 0;
	artifact_successes = 0;
	add_event(artifact_maintenance, 1, nullptr, nullptr, nullptr, 0, nullptr, 0);

	for (int interval = 0; interval < 4; interval++)
	{
		advance_one();
		require(queue.size() == 1, 20 + interval);
	}
	require(artifact_run == 4 && artifact_successes == 1, 24);
	require(scheduled_delays.size() == 5, 25);
	require(scheduled_delays[1] == 5 && scheduled_delays[2] == 5, 26);
	require(scheduled_delays[3] == 100 && scheduled_delays[4] == 100, 27);
}

int main()
{
	test_dirty_checkpoint(false);
	test_dirty_checkpoint(true);
	test_artifact_failures_and_empty_result_rearm();
	return 0;
}
'''


def function_body(source: str, signature: str) -> str:
    start = source.rindex(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


with tempfile.TemporaryDirectory(prefix="duris-nevent-periodic-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            f"-I{SRC}",
            str(harness),
            "-o",
            str(binary),
        ],
        check=True,
    )
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(binary)], check=True, env=environment)

redis = (SRC / "redis.c").read_text(encoding="ascii")
artifact = (SRC / "artifact.c").read_text(encoding="ascii")
events = (SRC / "new_events.c").read_text(encoding="ascii")

dirty = function_body(redis, "void event_flush_dirty_players")
assert "nevent_rearm_guard rearm(event_flush_dirty_players" in dirty
assert dirty.index("flush_dirty_players();") < dirty.index("if (redis_enabled)")
assert "if (redis_enabled)\n\t\tredis_flush_floor_drops();" in dirty
assert "add_event(event_flush_dirty_players" not in dirty
assert "add_event(event_flush_dirty_players, 5 * WAIT_SEC" in events

poof = function_body(artifact, "void event_artifact_check_poof_sql")
wars = function_body(artifact, "void event_artifact_wars_sql")
binding = function_body(artifact, "void event_artifact_check_bind_sql")

assert "nevent_rearm_guard rearm(event_artifact_check_poof_sql" in poof
assert "nevent_rearm_guard rearm(event_artifact_wars_sql" in wars
assert "nevent_rearm_guard rearm(event_artifact_check_bind_sql" in binding
for body in (poof, wars, binding):
    assert body.index("nevent_rearm_guard rearm(") < body.index("return;")
assert "add_event(event_artifact_check_poof_sql" not in poof
assert "add_event(event_artifact_wars_sql" not in wars
assert "add_event(event_artifact_check_bind_sql" not in binding
assert wars.count("rearm.retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY);") == 2
assert binding.count("rearm.retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY);") == 2

print("periodic nevent rearm tests passed under ASan/UBSan")
