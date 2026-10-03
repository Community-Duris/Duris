#!/usr/bin/env python3
"""Typed nevent payload, stable identity, and mob-hunt migration regressions."""

from _paths import SRC, extract_function
import os
import re
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OWNED_PAYLOAD_HARNESS = r'''
#include "core/prototypes.h"

#include <cstdlib>
#include <vector>

static void *captured_payload = nullptr;
static nevent_payload_destroy_type captured_destroy = nullptr;

nevent_schedule_result add_event_owned_payload(event_func, int, P_char, P_char, P_obj, int,
					       void *payload,
					       nevent_payload_destroy_type destroy)
{
	captured_payload = payload;
	captured_destroy = destroy;
	return { nevent_schedule_status::scheduled, { nullptr, 0 } };
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
#include "account/character_identity.c"
#include "core/utils.h"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <thread>

P_char character_list = nullptr;
static std::thread::id nevent_game_thread;
static bool nevent_game_thread_bound = false;
struct panic_signal {};

void panic_corruption(const char *, const char *, ...)
{
	throw panic_signal{};
}

void logit(const char *, const char *, ...) {}

// INSERT_PRODUCTION_THREAD_GUARDS
// INSERT_PRODUCTION_CLEAR_CHAR
// INSERT_PRODUCTION_POINTER_LOOKUP

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

int main()
{
	nevent_bind_game_thread();
	require(character_runtime_index_is_consistent(), 1);
	char_data first = {};
	char_data second = {};
	clear_char(&first);
	clear_char(&second);
	require(first.runtime_id && second.runtime_id > first.runtime_id, 2);
	require(find_character_by_runtime_id(first.runtime_id) == nullptr, 3);
	first.next = &second;
	character_list = &first;
	require(!character_runtime_index_is_consistent(), 4);
	register_character_runtime_id(&first);
	register_character_runtime_id(&second);
	register_character_runtime_id(&second); // Idempotent publication.
	require(character_runtime_index_is_consistent(), 5);

	require(find_character_by_runtime_id(first.runtime_id) == &first, 6);
	require(find_character_by_runtime_id(second.runtime_id) == &second, 7);
	require(find_character_by_runtime_id(0) == nullptr, 8);
	require(find_character_by_runtime_id(UINT64_MAX) == nullptr, 9);
	require(find_live_character(&second, first.runtime_id) == nullptr, 10);
	require(find_live_character(&first, first.runtime_id) == &first, 11);
	require(first.next == &second && second.next == nullptr, 12);

	const uint64_t stale_id = first.runtime_id;
	unregister_character_runtime_id(&first);
	unregister_character_runtime_id(&first); // extract_char then free_char.
	require(find_character_by_runtime_id(stale_id) == nullptr, 13);
	character_list = &second;
	require(character_runtime_index_is_consistent(), 14);
	clear_char(&first); // Deterministic pool reuse at the same address.
	require(first.runtime_id > second.runtime_id, 15);
	require(find_character_by_runtime_id(first.runtime_id) == nullptr, 16);
	first.next = character_list;
	character_list = &first;
	register_character_runtime_id(&first);
	require(find_live_character(&first, stale_id) == nullptr, 17);
	require(find_character_by_runtime_id(first.runtime_id) == &first, 18);
	require(character_runtime_index_is_consistent(), 19);

	// Check missing entries, orphan entries, mutated identities and list cycles.
	character_list = &second;
	require(!character_runtime_index_is_consistent(), 20);
	character_list = &first;
	const uint64_t current_id = first.runtime_id;
	first.runtime_id = 0;
	require(!character_runtime_index_is_consistent(), 21);
	first.runtime_id = current_id;
	second.next = &first;
	require(!character_runtime_index_is_consistent(), 22);
	second.next = nullptr;
	require(character_runtime_index_is_consistent(), 23);

	// An unrelated pointer with a duplicate ID cannot erase the live entry.
	char_data collision = {};
	collision.runtime_id = first.runtime_id;
	unregister_character_runtime_id(&collision);
	bool duplicate_rejected = false;
	try { register_character_runtime_id(&collision); }
	catch (const panic_signal &) { duplicate_rejected = true; }
	require(duplicate_rejected && find_character_by_runtime_id(current_id) == &first, 24);
	bool zero_rejected = false;
	char_data unpublished = {};
	try { register_character_runtime_id(&unpublished); }
	catch (const panic_signal &) { zero_rejected = true; }
	require(zero_rejected, 25);
	unregister_character_runtime_id(&unpublished); // Failed load cleanup.
	unregister_character_runtime_id(nullptr);

	// The registry never owns storage or dereferences stale callback pointers.
	P_char freed = new char_data{};
	clear_char(freed);
	const uint64_t freed_id = freed->runtime_id;
	freed->next = character_list;
	character_list = freed;
	register_character_runtime_id(freed);
	unregister_character_runtime_id(freed);
	character_list = freed->next;
	delete freed;
	require(find_character_by_runtime_id(freed_id) == nullptr, 26);
	require(character_runtime_index_is_consistent(), 27);

	bool wrong_thread_rejected = false;
	std::thread worker([&] {
		try {
			require(find_character_by_runtime_id(current_id) == nullptr, 28);
			register_character_runtime_id(&unpublished);
			unregister_character_runtime_id(&first);
			require(!character_runtime_index_is_consistent(), 29);
			wrong_thread_rejected = true;
		} catch (const panic_signal &) { wrong_thread_rejected = true; }
	});
	worker.join();
	require(wrong_thread_rejected && find_character_by_runtime_id(current_id) == &first, 30);

	unregister_character_runtime_id(&first);
	unregister_character_runtime_id(&second);
	character_list = nullptr;
	// Repeat the shared reconstruction primitives; constructor hooks are checked below.
	for (int reconstruction = 0; reconstruction < 12; ++reconstruction) {
		clear_char(&first);
		clear_char(&second);
		require(first.runtime_id > current_id && second.runtime_id > first.runtime_id, 31);
		require(find_character_by_runtime_id(first.runtime_id) == nullptr, 32);
		second.next = &first;
		character_list = &second;
		register_character_runtime_id(&first);
		register_character_runtime_id(&second);
		require(character_runtime_index_is_consistent(), 33);
		require(find_character_by_runtime_id(stale_id) == nullptr &&
			find_character_by_runtime_id(current_id) == nullptr, 34);
		unregister_character_runtime_id(&first);
		unregister_character_runtime_id(&second);
		character_list = nullptr;
	}
	require(character_runtime_index_is_consistent(), 35);
	// Exhaustion remains a fail-stop, never wrapping into a reused ID.
	next_runtime_id = UINT64_MAX;
	bool exhaustion_rejected = false;
	try { allocate_character_runtime_id(); }
	catch (const panic_signal &) { exhaustion_rejected = true; }
	require(exhaustion_rejected, 36);
	return 0;
}
'''

IDENTITY_HARNESS = IDENTITY_HARNESS.replace(
    "// INSERT_PRODUCTION_THREAD_GUARDS",
    "\n".join(extract_function("new_events.c", signature) for signature in (
        "void nevent_bind_game_thread()", "bool nevent_is_game_thread()",
        "bool nevent_require_game_thread(const char *operation)")),
).replace("// INSERT_PRODUCTION_CLEAR_CHAR", extract_function("db.c", "void clear_char(P_char ch)"))
IDENTITY_HARNESS = IDENTITY_HARNESS.replace(
    "// INSERT_PRODUCTION_POINTER_LOOKUP",
    extract_function("handler.c", "P_char find_live_character(P_char expected, uint64_t runtime_id)"),
)

RAW_TRIVIAL_SOURCE = r'''
#include "core/prototypes.h"

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
#include "core/prototypes.h"

static void callback(P_char, P_char, P_obj, void *)
{
}

void schedule_nontrivial()
{
	hunt_data data = {};
	add_event(callback, 1, nullptr, nullptr, nullptr, 0, &data, sizeof(data));
}
'''


def compile_source(source: str, output: Path, *, link: bool,
                   release: bool = False) -> subprocess.CompletedProcess[str]:
    source_file = output.with_suffix(".cpp")
    source_file.write_text(source, encoding="ascii")
    command = [
        "g++",
        "-std=c++20",
        "-O1",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pthread",
        f"-I{SRC}",
        str(source_file),
    ]
    if release:
        command.append("-DNDEBUG")
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


scratch = ROOT / "bin/tests"
scratch.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="duris-nevent-payload-", dir=scratch) as directory:
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

    release_identity_binary = temp / "character_identity_release"
    result = compile_source(IDENTITY_HARNESS, release_identity_binary, link=True, release=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(release_identity_binary)], check=True, env=environment)

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
mob_hunt = mobact.split("void event_mob_hunt(", 1)[1].split(
    "/* given a protector flagged mob", 1
)[0]

assert "add_event(event_mob_hunt" not in sources
assert ".targ.victim" not in sources
assert ".targ.room" not in sources
assert "target_runtime_id" in structs
assert "allocate_character_runtime_id()" in database
# Every publication path must register an initialized character. NPC construction
# intentionally links the legacy list early, but cannot expose a partial body.
publication_sites = (
    ("nanny.c", "void enter_game(P_desc d)", "ch"),
    ("copyover.c", "int copyover_recover(", "ch"),
    ("actwiz.c", "void do_read_player(", "vict"),
    ("artifact.c", "P_char load_dummy_char(", "owner"),
    ("storage_lockers.c", "static P_char load_locker_char(", "vict"),
)
actual_publications = []
for path in SRC.rglob("*.c"):
    for match in re.finditer(r"\bcharacter_list\s*=\s*(\w+)\s*;", path.read_text()):
        if match.group(1) not in {"0", "NULL", "nullptr"}:
            actual_publications.append((path.name, match.group(1)))
assert sorted(actual_publications) == sorted(
    [(filename, variable) for filename, _, variable in publication_sites] + [("db.c", "mob")])
for filename, signature, variable in publication_sites:
    body = extract_function(filename, signature)
    assert body.index(f"character_list = {variable};") < body.index(
        f"register_character_runtime_id({variable});")
mobile = extract_function("db.c", "P_char read_mobile(int nr, int type, bool apply_mob_gold)")
assert mobile.index("convertMob(mob, apply_mob_gold);") < mobile.index(
    "register_character_runtime_id(mob);") < mobile.index("return (mob);")
assert mobile.index("CMD_SET_PERIODIC") < mobile.index("register_character_runtime_id(mob);")
extract = extract_function("handler.c", "void extract_char(P_char ch)")
assert extract.index("if (!IS_MORPH(ch))") < extract.index(
    "unregister_character_runtime_id(ch);") < extract.index("#if defined(CTF_MUD)")
assert extract.index("item_actions_character_leaving(ch);") < extract.index(
    "unregister_character_runtime_id(ch);") < extract.index("die_follower(ch);")
free = extract_function("db.c", "void free_char(P_char ch)")
assert free.index("unregister_character_runtime_id(ch);") < free.index("affect_remove(ch, af);")
assert free.index("disarm_char_nevents(ch, NULL);") < free.index("add_event(release_mob_mem,")
assert mobact.count("get_scheduled_excluding_current(ch, event_mob_hunt)") == 4
assert mob_hunt.count("if (!get_scheduled_excluding_current(ch, event_mob_hunt))") == 2
assert mob_hunt.count("if (get_scheduled_excluding_current(ch, event_mob_hunt))") == 2
assert "schedule_mob_hunt(ch, *data);" in mobact
assert "&data,\n\t\t\t\t  sizeof(hunt_data)" not in mobact

print("typed nevent payload and stable hunt identity tests passed under ASan/UBSan")
