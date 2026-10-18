#!/usr/bin/env python3
"""The embrace death event follows the character's wounds.

The event's first check was inverted when its nested ifs were flattened: it returned
whenever the character had the affect, so the bonus never changed or ended, and it went
on to use the missing affect when there was none. The real event_embrace_death() runs
against a character with and without the affect.
"""
from _paths import SRC
import subprocess
import tempfile
from pathlib import Path

source = (SRC / "innates.c").read_text()
start = source.index("void event_embrace_death(")
function = source[start:source.index("\n}\n", start) + 3]

HARNESS = r'''
#include <cassert>
#include <cstdio>

struct affected_type
{
	int modifier;
};
struct char_data
{
	struct
	{
		int hit, max_hit;
	} points;
	affected_type *affect;
};
typedef char_data *P_char;
typedef void *P_obj;
#define GET_HIT(ch) ((ch)->points.hit)
#define GET_MAX_HIT(ch) ((ch)->points.max_hit)
#define WAIT_SEC 4
#define TAG_EMBRACE_DEATH 1

int removed, balanced, scheduled;
affected_type *get_spell_from_char(P_char ch, int)
{
	return ch->affect;
}
void affect_remove(P_char ch, affected_type *affect)
{
	assert(affect);
	ch->affect = nullptr;
	++removed;
}
void send_to_char(const char *, P_char)
{
}
void balance_affects(P_char)
{
	++balanced;
}
void add_event(void (*)(P_char, P_char, P_obj, void *), int, P_char, P_char, P_obj, int, void *,
	       int)
{
	++scheduled;
}
''' + function + r'''
int main()
{
	// No affect: nothing to follow, and nothing is touched.
	char_data ch = { { 50, 100 }, nullptr };
	event_embrace_death(&ch, 0, 0, 0);
	assert(!removed && !balanced && !scheduled);

	// Wounded: the bonus follows the wounds and the event runs again.
	affected_type affect = { 15 };
	ch.affect = &affect;
	event_embrace_death(&ch, 0, 0, 0);
	assert(affect.modifier == 5 && balanced == 1 && scheduled == 1);
	ch.points.hit = 30;
	event_embrace_death(&ch, 0, 0, 0);
	assert(affect.modifier == 10 && balanced == 2 && scheduled == 2);

	// Healed past three quarters: the affect ends and the event stops.
	ch.points.hit = 80;
	event_embrace_death(&ch, 0, 0, 0);
	assert(removed == 1 && !ch.affect && scheduled == 2);
	puts("the embrace death event follows the character's wounds");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-embrace-death-") as directory:
    harness = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    harness.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    str(harness), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
