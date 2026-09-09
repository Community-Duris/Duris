#!/usr/bin/env python3
"""Coin get completion omits zero denominations, uses colored coin formatting, and notifies the room."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function
from _source_contract import function_body


ACTOBJ = (SRC / "actobj.c").read_text(encoding="utf-8", errors="replace")
COMPLETION = function_body(ACTOBJ, r"static bool\s+coin_get_completion\s*\(")
assert COMPLETION, "coin_get_completion definition not found"

assert "coins_to_string(" in COMPLETION
assert "You get %d platinum, %d gold, %d silver, and %d copper coins." not in COMPLETION
assert "act(" in COMPLETION and "TO_ROOM" in COMPLETION

HARNESS = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <string>
#include <unordered_map>

#define MAX_STRING_LENGTH 65536
#define TRUE 1
#define TO_ROOM 0
#define ITEM_CORPSE 23
#define CORPSE_FLAGS 1
#define PC_CORPSE 1
#define IS_SET(flag, bit) ((flag) & (bit))
#define PLAYER_COMPONENT_STATUS 1
#define PLAYER_COMPONENT_INVENTORY 2
#define BIT_1 1U
#define BIT_2 2U
#define BIT_3 4U
#define BIT_4 8U

struct obj_data
{
	int type;
	int value[8];
};
using P_obj = obj_data *;

struct pc_only_data
{
	int pid;
};
struct char_data
{
	pc_only_data *only_pc;
};
using P_char = char_data *;
#define GET_PID(ch) ((ch)->only_pc->pid)

struct coin_transfer_endpoint
{
	std::array<int32_t, 4> before = {};
	std::array<int32_t, 4> after = {};
};
struct coin_transfer_payload
{
	coin_transfer_endpoint source;
	coin_transfer_endpoint destination;
};
struct item_transfer_result
{
	int unused = 0;
};
struct coin_transfer_result
{
	std::array<item_transfer_result, 2> piles = {};
};
struct bulk_get_state
{
	int total = 0;
	bool failed = false;
};

static std::string actor_text;
static std::string room_text;
static int room_acts = 0;
static std::unordered_map<uint32_t, bulk_get_state> bulk_gets;

void send_to_char(const char *txt, P_char)
{
	if (txt)
		actor_text += txt;
}
void act(const char *str, int, P_char, P_obj, void *, int type)
{
	if (type == TO_ROOM && str)
	{
		room_text += str;
		++room_acts;
	}
}
P_obj find_live_item_uid(uint64_t) { return nullptr; }
static bool publish_coin_pile(const coin_transfer_endpoint &, const item_transfer_result &, uint64_t)
{
	return true;
}
void mark_player_dirty_components(int, int) {}
void writeCorpse(P_obj) {}
static bool finish_bulk_get_after_commit(P_char, bulk_get_state &, P_obj) { return true; }
static void finish_bulk_get(P_char, uint32_t) {}
void debug(const char *, ...) {}
''' + extract_function("utility.c", "char *coins_to_string(int platinum, int gold, int silver, int copper, const char *color_string)") + "\n" + extract_function("actobj.c", "struct coin_pickup_context") + ";\n" + extract_function("actobj.c", "static bool coin_get_completion(P_char actor, bool committed, const coin_transfer_payload &payload,") + r'''

static void expect_get(const coin_transfer_payload &payload, int showit, const char *want_line)
{
	actor_text.clear();
	room_text.clear();
	room_acts = 0;
	pc_only_data player = {};
	player.pid = 42;
	char_data actor = {};
	actor.only_pc = &player;
	coin_pickup_context context = {};
	context.showit = showit;
	coin_transfer_result result = {};
	assert(coin_get_completion(&actor, true, payload, result, 0,
				   reinterpret_cast<const uint8_t *>(&context), sizeof(context)));
	assert(actor_text == want_line);
	assert(actor_text.find("0 platinum") == std::string::npos);
	assert(actor_text.find("0 gold") == std::string::npos);
	assert(actor_text.find("0 silver") == std::string::npos);
	assert(actor_text.find("0 copper") == std::string::npos);
	if (showit)
	{
		assert(room_acts == 1);
		assert(room_text.find("$n gets some coins.") != std::string::npos);
	}
	else
		assert(room_acts == 0);
}

int main()
{
	char want[MAX_STRING_LENGTH];

	coin_transfer_payload gold_only;
	gold_only.source.before = {0, 0, 15, 0};
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(0, 15, 0, 0, "&n"));
	expect_get(gold_only, 1, want);
	assert(std::string(want).find("&+Y") != std::string::npos);
	assert(std::string(want).find("&+W") == std::string::npos);

	coin_transfer_payload mixed;
	mixed.source.before = {0, 3, 0, 1};
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(1, 0, 3, 0, "&n"));
	expect_get(mixed, 1, want);
	assert(std::string(want).find("&+W") != std::string::npos);
	assert(std::string(want).find("&+w") != std::string::npos);
	assert(std::string(want).find("&+Y") == std::string::npos);

	coin_transfer_payload copper_only;
	copper_only.source.before = {4, 0, 0, 0};
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(0, 0, 0, 4, "&n"));
	expect_get(copper_only, 0, want);
	assert(std::string(want).find("&+y") != std::string::npos);
	return 0;
}
'''

def main() -> int:
    """Compile and run coin_get_completion against the real shipped formatter."""
    tests_dir = ROOT / "bin/tests"
    tests_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="duris-coin-get-completion-", dir=str(tests_dir)) as directory:
        build = Path(directory)
        build.mkdir(parents=True, exist_ok=True)
        source = build / "harness.cpp"
        binary = build / "harness"
        source.write_text(HARNESS, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-o",
                str(binary),
                str(source),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("coin_get_completion omits zeros, uses coins_to_string colors, and notifies the room")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
