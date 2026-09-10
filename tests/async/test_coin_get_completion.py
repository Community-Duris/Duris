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
assert "$n gets some coins from $P." in COMPLETION
assert "$n gets some coins." in COMPLETION

HARNESS = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <string>
#include <unordered_map>
#include <vector>

#define MAX_STRING_LENGTH 65536
#define TRUE 1
#define TO_ROOM 0
#define ITEM_CORPSE 24
#define CORPSE_FLAGS 1
#define PC_CORPSE 1
#define IS_SET(flag, bit) ((flag) & (bit))
typedef uint64_t player_component_mask_t;
#define PLAYER_COMPONENT_STATUS (UINT64_C(1) << 0)
#define PLAYER_COMPONENT_INVENTORY (UINT64_C(1) << 10)
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
	bool got_coins = false;
	uint64_t container_uid = 0;
	bool failed = false;
};

static std::string actor_text;
static std::string room_text;
static std::vector<std::string> room_args;
static int room_acts = 0;
static P_obj room_container = nullptr;
static std::unordered_map<uint32_t, bulk_get_state> bulk_gets;
static std::unordered_map<uint64_t, obj_data> live_items;
static uint64_t next_uid = 1;
static bool publish_ok = true;
static int corpse_writes = 0;

void send_to_char(const char *txt, P_char)
{
	if (txt)
		actor_text += txt;
}
void act(const char *str, int, P_char, P_obj, void *vict_obj, int type)
{
	if (type == TO_ROOM && str)
	{
		room_text += str;
		room_args.push_back(str ? str : "");
		if (vict_obj)
			room_container = static_cast<P_obj>(vict_obj);
		++room_acts;
	}
}
P_obj find_live_item_uid(uint64_t uid)
{
	auto found = live_items.find(uid);
	return found == live_items.end() ? nullptr : &found->second;
}
static P_obj make_container(int type = 0)
{
	obj_data stored = {};
	stored.type = type;
	uint64_t uid = next_uid++;
	live_items[uid] = stored;
	return &live_items[uid];
}
static bool publish_coin_pile(const coin_transfer_endpoint &, const item_transfer_result &, uint64_t)
{
	return publish_ok;
}
void mark_player_dirty_components(int, player_component_mask_t) {}
void writeCorpse(P_obj) { ++corpse_writes; }
static bool finish_bulk_get_after_commit(P_char, bulk_get_state &, P_obj) { return true; }
static void report_bulk_get(P_char actor, const bulk_get_state &state);
static void finish_bulk_get(P_char actor, uint32_t pid) {
    report_bulk_get(actor, bulk_gets.at(pid));
    bulk_gets.erase(pid);
}
void debug(const char *, ...) {}
''' + extract_function("utility.c", "char *coins_to_string(int platinum, int gold, int silver, int copper, const char *color_string)") + "\n" + extract_function("actobj.c", "static void report_bulk_get(P_char actor, const bulk_get_state &state)") + "\n" + extract_function("actobj.c", "struct coin_pickup_context") + ";\n" + extract_function("actobj.c", "static bool coin_get_completion(P_char actor, bool committed, const coin_transfer_payload &payload,") + r'''

// A zero-denomination omission failure must name the denomination token
// itself (e.g. "0g" as a standalone count), not a trailing digit of a
// larger quantity: "10g" and "100c" are legal pickup amounts.
static bool has_zero_denomination(const std::string &text, char unit)
{
	for (size_t pos = 0; (pos = text.find(unit, pos)) != std::string::npos; ++pos)
	{
		size_t start = pos;
		while (start > 0 && text[start - 1] >= '0' && text[start - 1] <= '9')
			--start;
		if (start == pos)
			continue;
		bool all_zero = true;
		for (size_t i = start; i < pos; ++i)
		{
			if (text[i] != '0')
			{
				all_zero = false;
				break;
			}
		}
		if (!all_zero)
			continue;
		// Only a count whose digits are all zero is an omitted-denomination
		// failure; require a non-digit boundary so "10g" does not match.
		if (start > 0 && text[start - 1] >= '0' && text[start - 1] <= '9')
			continue;
		return true;
	}
	return false;
}

static void expect_zero_denominations_omitted(const std::string &text)
{
	assert(!has_zero_denomination(text, 'p'));
	assert(!has_zero_denomination(text, 'g'));
	assert(!has_zero_denomination(text, 's'));
	assert(!has_zero_denomination(text, 'c'));
}

static coin_transfer_payload make_payload(const std::array<int32_t, 4> &before,
					  const std::array<int32_t, 4> &after = {})
{
	coin_transfer_payload payload = {};
	payload.source.before = before;
	payload.source.after = after;
	return payload;
}

static void run_completion(P_char actor, bool committed, const coin_transfer_payload &payload,
			   P_obj container, int showit)
{
	coin_pickup_context context = {};
	context.container_uid = 0;
	for (const auto &[uid, stored] : live_items)
	{
		if (&stored == container)
		{
			context.container_uid = uid;
			break;
		}
	}
	context.showit = showit;
	coin_transfer_result result = {};
	assert(coin_get_completion(actor, committed, payload, result, 0,
				   reinterpret_cast<const uint8_t *>(&context), sizeof(context)));
}

static void expect_get(const coin_transfer_payload &payload, int showit, const char *want_line,
		       P_obj container = nullptr, bool expect_container_message = false)
{
	actor_text.clear();
	room_text.clear();
	room_args.clear();
	room_acts = 0;
	room_container = nullptr;
	pc_only_data player = {};
	player.pid = 42;
	char_data actor = {};
	actor.only_pc = &player;
	run_completion(&actor, true, payload, container, showit);
	assert(actor_text == want_line);
	expect_zero_denominations_omitted(actor_text);
	if (showit)
	{
		assert(room_acts == 1);
		if (expect_container_message)
		{
			assert(room_text.find("$n gets some coins from $P.") != std::string::npos);
			assert(room_container == container);
		}
		else
		{
			assert(room_text.find("$n gets some coins.") != std::string::npos);
			assert(room_container == nullptr);
		}
	}
	else
		assert(room_acts == 0);
}

int main()
{
	char want[MAX_STRING_LENGTH];

	coin_transfer_payload gold_only = make_payload({0, 0, 15, 0});
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(0, 15, 0, 0, "&+y"));
	expect_get(gold_only, 1, want);
	assert(std::string(want).find("&+Y") != std::string::npos);
	assert(std::string(want).find("&+W") == std::string::npos);

	// Two-digit quantities end in zero without being zero denominations.
	coin_transfer_payload double_digit_gold = make_payload({0, 0, 10, 0});
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(0, 10, 0, 0, "&+y"));
	expect_get(double_digit_gold, 0, want);

	coin_transfer_payload mixed = make_payload({0, 3, 0, 1});
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(1, 0, 3, 0, "&+y"));
	expect_get(mixed, 1, want);
	assert(std::string(want).find("&+W") != std::string::npos);
	assert(std::string(want).find("&+w") != std::string::npos);
	assert(std::string(want).find("&+Y") == std::string::npos);

	coin_transfer_payload copper_only = make_payload({4, 0, 0, 0});
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(0, 0, 0, 4, "&+y"));
	expect_get(copper_only, 0, want);
	assert(std::string(want).find("&+y") != std::string::npos);

	coin_transfer_payload platinum_only = make_payload({0, 0, 0, 5});
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(5, 0, 0, 0, "&+y"));
	expect_get(platinum_only, 1, want);
	assert(std::string(want).find("&+W") != std::string::npos);

	coin_transfer_payload all_four = make_payload({4, 3, 2, 1});
	snprintf(want, sizeof(want), "You get %s.\r\n", coins_to_string(1, 2, 3, 4, "&+y"));
	expect_get(all_four, 1, want);
	assert(std::string(want).find("and") != std::string::npos);

	// The room message names the container when coins come out of one.
	P_obj chest = make_container();
	expect_get(gold_only, 1, want[0] ? ({
		snprintf(want, sizeof(want), "You get %s.\r\n",
			 coins_to_string(0, 15, 0, 0, "&+y"));
		want;
	}) : want, chest, true);
	expect_get(gold_only, 0, want, chest, false);

	// A partial pickup leaves coins behind and says so.
	actor_text.clear();
	room_text.clear();
	room_acts = 0;
	{
		pc_only_data player = {};
		player.pid = 42;
		char_data actor = {};
		actor.only_pc = &player;
		coin_transfer_payload leftover = make_payload({0, 0, 15, 0}, {0, 0, 5, 0});
		snprintf(want, sizeof(want), "You get %s.\r\n",
			 coins_to_string(0, 10, 0, 0, "&+y"));
		run_completion(&actor, true, leftover, nullptr, 0);
		assert(actor_text.find(want) == 0);
		assert(actor_text.find("You couldn't carry all the coins.\r\n") !=
		       std::string::npos);
	}

	// An uncommitted completion changes nothing and says so.
	{
		pc_only_data player = {};
		player.pid = 42;
		char_data actor = {};
		actor.only_pc = &player;
		actor_text.clear();
		room_text.clear();
		room_acts = 0;
		coin_transfer_payload payload = make_payload({0, 0, 15, 0});
		coin_pickup_context context = {};
		coin_transfer_result result = {};
		assert(coin_get_completion(&actor, false, payload, result, 0,
					   reinterpret_cast<const uint8_t *>(&context),
					   sizeof(context)));
		assert(actor_text == "The coin transfer did not commit; nothing changed.\r\n");
		assert(room_acts == 0);
	}

	// Zero coins render as "nothing", never as four zero denominations.
	{
		pc_only_data player = {};
		player.pid = 42;
		char_data actor = {};
		actor.only_pc = &player;
		actor_text.clear();
		snprintf(want, sizeof(want), "You get %s.\r\n",
			 coins_to_string(0, 0, 0, 0, "&+y"));
		run_completion(&actor, true, make_payload({0, 0, 0, 0}), nullptr, 0);
		assert(actor_text == want);
		assert(actor_text.find("nothing") != std::string::npos);
	}
    // Coins never inflate equipment counts or produce a false empty-container message.
    for (int items : {0, 1, 2})
    for (bool committed : {false, true})
    {
        pc_only_data player{42};
        char_data actor{&player};
        coin_pickup_context context{};
        context.actor_pid = 42;
        context.bulk = true;
        bulk_gets[42] = {};
        bulk_gets[42].total = items;
        actor_text.clear();
        assert(coin_get_completion(&actor, committed, gold_only, {}, 0,
            reinterpret_cast<const uint8_t *>(&context), sizeof(context)));
        assert(bulk_gets.empty());
        assert(actor_text.find("nothing here") == std::string::npos);
        assert(actor_text.find("nothing in it") == std::string::npos);
        if (items == 2)
            assert(actor_text.find("You got 2 items.") != std::string::npos);
        else
            assert(actor_text.find("You got ") == std::string::npos);
    }
    // Publication failures, detached actors, and fenced completions release bulk state.
    for (int failure : {0, 1, 2})
    {
        pc_only_data player{42};
        char_data actor{&player};
        coin_pickup_context context{};
        context.actor_pid = 42;
        context.bulk = true;
        bulk_gets[42] = {};
        publish_ok = failure != 0;
        assert(coin_get_completion(failure == 1 ? nullptr : &actor, true, gold_only,
            {}, failure == 2 ? EOWNERDEAD : 0,
            reinterpret_cast<const uint8_t *>(&context), sizeof(context)) == publish_ok);
        assert(bulk_gets.empty());
    }
    publish_ok = true;
    // A coin commit changes the pile after any earlier equipment-phase corpse save.
    {
        pc_only_data player{42};
        char_data actor{&player};
        P_obj corpse = make_container(ITEM_CORPSE);
        corpse->value[CORPSE_FLAGS] = PC_CORPSE;
        corpse_writes = 0;
        run_completion(&actor, true, gold_only, corpse, 0);
        assert(corpse_writes == 1);
        run_completion(&actor, false, gold_only, corpse, 0);
        assert(corpse_writes == 1);
    }
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
