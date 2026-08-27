#!/usr/bin/env python3
"""Typed nevent payload, stable identity, and mob-hunt migration regressions."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

OWNED_PAYLOAD_HARNESS = r'''
#include "prototypes.h"

#include <cstdlib>
#include <vector>

static void *captured_payload = nullptr;
static nevent_payload_destroy_type captured_destroy = nullptr;

void add_event_owned_payload(event_func, int, P_char, P_char, P_obj, int, void *payload,
			     nevent_payload_destroy_type destroy)
{
	captured_payload = payload;
	captured_destroy = destroy;
}

static void callback(P_char, P_char, P_obj, void *)
{
}

struct tracked_payload
{
	static int copies;
	static int moves;
	static int destroys;
	static int live;
	std::vector<int> path;

	tracked_payload()
	{
		live++;
	}

	tracked_payload(const tracked_payload &other) : path(other.path)
	{
		copies++;
		live++;
	}

	tracked_payload(tracked_payload &&other) noexcept : path(std::move(other.path))
	{
		moves++;
		live++;
	}

	~tracked_payload()
	{
		destroys++;
		live--;
	}
};

int tracked_payload::copies = 0;
int tracked_payload::moves = 0;
int tracked_payload::destroys = 0;
int tracked_payload::live = 0;

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

int main()
{
	tracked_payload original;
	original.path = { 4, 8, 15, 16, 23, 42 };
	add_event_owned(callback, 1, nullptr, nullptr, nullptr, 0, original);

	auto *stored = static_cast<tracked_payload *>(captured_payload);
	require(stored != nullptr && captured_destroy != nullptr, 1);
	require(tracked_payload::copies == 1 && tracked_payload::moves == 1, 2);
	require(tracked_payload::destroys == 1 && tracked_payload::live == 2, 3);
	original.path[0] = 99;
	require(stored->path.size() == 6 && stored->path[0] == 4, 4);

	captured_destroy(captured_payload);
	captured_payload = nullptr;
	require(tracked_payload::destroys == 2 && tracked_payload::live == 1, 5);
	return 0;
}
'''

IDENTITY_HARNESS = r'''
#include "character_identity.c"

#include <cstdarg>
#include <cstdlib>

P_char character_list = nullptr;

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

int main()
{
	char_data first = {};
	char_data second = {};
	first.runtime_id = allocate_character_runtime_id();
	second.runtime_id = allocate_character_runtime_id();
	first.next = &second;
	character_list = &first;

	require(first.runtime_id != 0 && second.runtime_id != 0, 1);
	require(first.runtime_id != second.runtime_id, 2);
	require(find_character_by_runtime_id(first.runtime_id) == &first, 3);
	require(find_character_by_runtime_id(second.runtime_id) == &second, 4);
	require(find_character_by_runtime_id(0) == nullptr, 5);

	const uint64_t stale_id = first.runtime_id;
	first.runtime_id = allocate_character_runtime_id();
	require(find_character_by_runtime_id(stale_id) == nullptr, 6);
	require(find_character_by_runtime_id(first.runtime_id) == &first, 7);
	return 0;
}
'''

RAW_TRIVIAL_SOURCE = r'''
#include "prototypes.h"

static void callback(P_char, P_char, P_obj, void *)
{
}

void schedule_trivial()
{
	int value = 7;
	add_event(callback, 1, nullptr, nullptr, nullptr, 0, &value, sizeof(value));
}
'''

RAW_NONTRIVIAL_SOURCE = r'''
#include "prototypes.h"

static void callback(P_char, P_char, P_obj, void *)
{
}

void schedule_nontrivial()
{
	hunt_data data = {};
	add_event(callback, 1, nullptr, nullptr, nullptr, 0, &data, sizeof(data));
}
'''


def compile_source(source: str, output: Path, *, link: bool) -> subprocess.CompletedProcess[str]:
    source_file = output.with_suffix(".cpp")
    source_file.write_text(source, encoding="ascii")
    command = [
        "g++",
        "-std=c++20",
        "-O1",
        "-Wall",
        "-Wextra",
        "-Werror",
        f"-I{SRC}",
        str(source_file),
    ]
    if link:
        command.extend(
            [
                "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer",
                "-o",
                str(output),
            ]
        )
    else:
        command.extend(["-c", "-o", str(output)])
    return subprocess.run(command, capture_output=True, text=True, check=False)


with tempfile.TemporaryDirectory(prefix="duris-nevent-payload-") as directory:
    temp = Path(directory)
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"

    owned_binary = temp / "owned_payload"
    result = compile_source(OWNED_PAYLOAD_HARNESS, owned_binary, link=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(owned_binary)], check=True, env=environment)

    identity_binary = temp / "character_identity"
    result = compile_source(IDENTITY_HARNESS, identity_binary, link=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(identity_binary)], check=True, env=environment)

    trivial_object = temp / "trivial.o"
    result = compile_source(RAW_TRIVIAL_SOURCE, trivial_object, link=False)
    assert result.returncode == 0, result.stderr

    nontrivial_object = temp / "nontrivial.o"
    result = compile_source(RAW_NONTRIVIAL_SOURCE, nontrivial_object, link=False)
    assert result.returncode != 0, "non-trivial raw payload unexpectedly compiled"
    assert "raw event payloads must be trivially copyable" in result.stderr

sources = "\n".join(
    path.read_text(encoding="utf-8") for path in SRC.rglob("*") if path.suffix in {".c", ".h"}
)
mobact = (SRC / "mobact.c").read_text(encoding="ascii")
structs = (SRC / "structs.h").read_text(encoding="ascii")
database = (SRC / "db.c").read_text(encoding="ascii")

assert "add_event(event_mob_hunt" not in sources
assert ".targ.victim" not in sources
assert ".targ.room" not in sources
assert "target_runtime_id" in structs
assert "allocate_character_runtime_id()" in database
assert mobact.count("get_scheduled_excluding_current(ch, event_mob_hunt)") == 4
assert "schedule_mob_hunt(ch, *data);" in mobact
assert "&data,\n\t\t\t\t  sizeof(hunt_data)" not in mobact

print("typed nevent payload and stable hunt identity tests passed under ASan/UBSan")
