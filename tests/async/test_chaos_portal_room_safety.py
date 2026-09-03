#!/usr/bin/env python3
"""Exercise the real Chaos portal function across room-safety failure paths."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import SRC, source

ROOT = Path(__file__).resolve().parents[2]
CHAOS = source("chaos.c")
portal_source = CHAOS.read_text(encoding="utf-8", errors="replace")

HARNESS = r"""
#include "core/prototypes.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

const int top_of_world = 1000;
static bool tracked = true;
static bool destination_available = true;
static bool remove_succeeds = true;
static bool remove_extracts = false;
static bool fail_first_arrival = false;
static int remove_calls = 0;
static int arrival_calls = 0;
static int act_calls = 0;
static std::string last_message;
static std::vector<std::string> act_messages;

#include "combat/chaos.c"

bool isname(const char *argument, const char *namelist)
{
    return argument && namelist && std::strcmp(argument, namelist) == 0;
}

int char_in_list(const P_char ch)
{
    return ch && tracked;
}

void send_to_char(const char *message, P_char)
{
    last_message = message ? message : "";
}

void act(const char *message, int, P_char, P_obj, void *, int)
{
    ++act_calls;
    act_messages.emplace_back(message ? message : "");
}

void logit(const char *, const char *, ...)
{
}

int real_room(const int)
{
    return destination_available ? 42 : NOWHERE;
}

void char_from_room(P_char ch)
{
    ++remove_calls;
    if (remove_succeeds)
    {
        ch->in_room = NOWHERE;
        if (remove_extracts)
            tracked = false;
    }
}

bool char_to_room(P_char ch, int room, int)
{
    ++arrival_calls;
    if (!IS_ALIVE(ch) || room < 0)
        return false;
    if (fail_first_arrival && arrival_calls == 1)
        return false;
    ch->in_room = room;
    return true;
}

static void reset_state()
{
    tracked = true;
    destination_available = true;
    remove_succeeds = true;
    remove_extracts = false;
    fail_first_arrival = false;
    remove_calls = 0;
    arrival_calls = 0;
    act_calls = 0;
    last_message.clear();
    act_messages.clear();
}

static char_data make_player(int room, int position)
{
    char_data player{};
    player.in_room = room;
    player.specials.position = position;
    return player;
}

static void check(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void check_no_movement(const char_data &player, int original_room, const char *message)
{
    check(player.in_room == original_room, message);
    check(remove_calls == 0, "rejected portal path removed the actor");
    check(arrival_calls == 0, "rejected portal path attempted arrival");
    check(act_calls == 0, "rejected portal path emitted a room side effect");
}

int main()
{
    {
        reset_state();
        char_data player = make_player(10, STAT_DEAD + POS_PRONE);
        chaos_port(&player, "flann");
        check_no_movement(player, 10, "dead actor changed room state");
    }

    {
        reset_state();
        tracked = false;
        char_data player = make_player(10, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check(remove_calls == 0, "unlinked actor reached room removal");
        check(arrival_calls == 0, "unlinked actor reached room arrival");
    }

    {
        reset_state();
        char_data player = make_player(NOWHERE, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check_no_movement(player, NOWHERE, "NOWHERE actor changed room state");
    }

    {
        reset_state();
        char_data player = make_player(top_of_world + 1, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check_no_movement(player, top_of_world + 1, "out-of-range actor changed room state");
    }

    {
        reset_state();
        destination_available = false;
        char_data player = make_player(10, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check_no_movement(player, 10, "unresolved destination changed room state");
    }

    {
        reset_state();
        remove_succeeds = false;
        char_data player = make_player(10, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check(player.in_room == 10, "leave veto changed the source room");
        check(arrival_calls == 0, "leave veto reached room arrival");
        check(act_calls == 1, "leave veto did not preserve only the opening effect");
        check(act_messages.back().find("spew") == std::string::npos,
              "leave veto emitted the arrival effect");
    }

    {
        reset_state();
        remove_extracts = true;
        char_data player = make_player(10, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check(!tracked, "room-hook extraction was not reflected in list state");
        check(arrival_calls == 0, "extracted actor reached room arrival");
        check(act_calls == 1, "extracted actor emitted an arrival effect");
    }

    {
        reset_state();
        fail_first_arrival = true;
        char_data player = make_player(10, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check(player.in_room == 10, "failed arrival was not restored");
        check(arrival_calls == 2, "failed arrival did not attempt one restoration");
        check(act_messages.back().find("spew") == std::string::npos,
              "failed arrival emitted the arrival effect");
    }

    {
        reset_state();
        char_data player = make_player(10, STAT_NORMAL + POS_STANDING);
        chaos_port(&player, "flann");
        check(player.in_room == 42, "valid alive actor did not reach destination");
        check(arrival_calls == 1, "valid portal attempted an unexpected extra move");
        check(act_calls == 3, "valid portal did not emit the expected room effects");
        check(act_messages.back().find("spew") != std::string::npos,
              "valid portal omitted the arrival effect");
    }

    std::puts("chaos portal room-safety cases passed");
    return 0;
}
"""

# These source-order checks are deliberately evaluated only after the real-function harness passes.
with tempfile.TemporaryDirectory(prefix="duris-chaos-portal-safety-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O0",
            "-ffunction-sections",
            "-fdata-sections",
            f"-I{SRC}",
            str(harness),
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

portal_start = portal_source.index("static void chaos_port")
portal_end = portal_source.index("\nstatic struct", portal_start)
portal = portal_source[portal_start:portal_end]
assert portal.index("IS_ALIVE") < portal.index("char_from_room")
assert portal.index("real_room(portdata[i].vnum)") < portal.index("char_from_room")
assert "if (!char_to_room" in portal
assert portal.index("if (!char_to_room") < portal.index('"A chaos portal briefly appears to spew out $n."')
assert "char_in_list" in portal

print("chaos portal room-safety cases and source-order contract passed")
