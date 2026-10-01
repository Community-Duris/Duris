#!/usr/bin/env python3
"""Exercise the production movement-special dispatch for ridden NPC mounts."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function


SPECIAL_ACTOR = extract_function(
    "actmove.c", "static P_char movement_special_actor(P_char mover)"
)
SIMPLE_MOVE = extract_function(
    "actmove.c", "int do_simple_move(P_char ch, int exitnumb, unsigned int flags)"
)

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"

#include <cassert>
#include <cstdio>

static P_char ridden_mount;
static P_char linked_rider;
static P_char special_actor;
static P_char grease_actor;
static P_char moved_actor;
static int special_calls;
static int grease_calls;
static int move_calls;
static int special_command;
static bool gatekeeper_blocks_players;
static bool block_grease;

P_char get_linking_char(P_char ch, ush_int type)
{
    return ch == ridden_mount && type == LNK_RIDING ? linked_rider : nullptr;
}

bool special(P_char ch, int cmd, char *)
{
    ++special_calls;
    special_actor = ch;
    special_command = cmd;
    return gatekeeper_blocks_players && IS_PC(ch);
}

bool grease_check(P_char ch)
{
    ++grease_calls;
    grease_actor = ch;
    return block_grease;
}

int exitnumb_to_cmd(int exitnumb)
{
    return 100 + exitnumb;
}

int do_simple_move_skipping_procs(P_char ch, int, unsigned int)
{
    ++move_calls;
    moved_actor = ch;
    return TRUE;
}
'''

DRIVER = r'''
static void reset()
{
    ridden_mount = linked_rider = nullptr;
    special_actor = grease_actor = moved_actor = nullptr;
    special_calls = grease_calls = move_calls = special_command = 0;
    gatekeeper_blocks_players = block_grease = false;
}

static void expect_completed_move(P_char mover, P_char expected_special_actor)
{
    assert(do_simple_move(mover, 2, 0) == TRUE);
    assert(special_calls == 1);
    assert(special_actor == expected_special_actor);
    assert(special_command == 102);
    assert(grease_calls == 1 && grease_actor == mover);
    assert(move_calls == 1 && moved_actor == mover);
}

int main()
{
    char_data player{};
    char_data mount{};
    char_data npc_rider{};
    char_data autonomous_npc{};

    player.in_room = mount.in_room = npc_rider.in_room = autonomous_npc.in_room = 1;
    mount.specials.act = ACT_ISNPC | ACT_MOUNT;
    npc_rider.specials.act = ACT_ISNPC;
    autonomous_npc.specials.act = ACT_ISNPC | ACT_MOUNT;

    reset();
    expect_completed_move(&player, &player);

    // This is the reported exploit: the pet is the physical mover, while its
    // player rider must be the one presented to every movement special.
    reset();
    ridden_mount = &mount;
    linked_rider = &player;
    expect_completed_move(&mount, &player);

    // A blocking gate runs exactly once and prevents the mount and rider pair
    // from reaching the physical movement implementation.
    reset();
    ridden_mount = &mount;
    linked_rider = &player;
    gatekeeper_blocks_players = true;
    assert(do_simple_move(&mount, 2, 0) == FALSE);
    assert(special_calls == 1 && special_actor == &player);
    assert(grease_calls == 0 && move_calls == 0);

    // Autonomous NPC movement and NPC-on-NPC riding retain their old actor and
    // therefore pass a guard that intentionally permits NPCs.
    reset();
    gatekeeper_blocks_players = true;
    expect_completed_move(&autonomous_npc, &autonomous_npc);

    reset();
    ridden_mount = &mount;
    linked_rider = &npc_rider;
    gatekeeper_blocks_players = true;
    expect_completed_move(&mount, &mount);

    // The riding link is authoritative even if old area data omitted ACT_MOUNT.
    reset();
    ridden_mount = &autonomous_npc;
    linked_rider = &player;
    autonomous_npc.specials.act = ACT_ISNPC;
    expect_completed_move(&autonomous_npc, &player);

    // Invalid movement never dispatches a special.
    reset();
    assert(do_simple_move(&player, -1, 0) == FALSE);
    assert(special_calls == 0 && grease_calls == 0 && move_calls == 0);

    reset();
    player.in_room = NOWHERE;
    assert(do_simple_move(&player, 0, 0) == FALSE);
    assert(special_calls == 0 && grease_calls == 0 && move_calls == 0);

    std::puts("mounted gatekeeper movement-special regressions passed");
    return 0;
}
'''


# Keep compiled artifacts under bin/, matching repository conventions.
build_root = ROOT / "bin" / "tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="mounted-gatekeeper-", dir=build_root) as directory:
    harness = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    harness.write_text(
        "\n".join((PRELUDE, SPECIAL_ACTOR, SIMPLE_MOVE, DRIVER)), encoding="utf-8"
    )
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            f"-I{ROOT / 'src'}",
            str(harness),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

print("mounted gatekeeper runtime regression passed")
