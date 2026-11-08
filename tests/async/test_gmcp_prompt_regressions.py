#!/usr/bin/env python3
"""Executable GMCP preference, room-delivery and prompt regressions."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function, source


ACTOTH = source("actoth.c").read_text(encoding="utf-8")
GMCP = source("gmcp.c").read_text(encoding="utf-8")
PROMPT = source("prompt.c").read_text(encoding="utf-8")


def run_harness(name: str, source_text: str, *, cases=("",), sanitize=False) -> None:
    build_root = ROOT / "bin" / "tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=name, dir=build_root) as directory:
        directory_path = Path(directory)
        source_path = directory_path / "harness.cpp"
        binary_path = directory_path / "harness"
        source_path.write_text(source_text, encoding="utf-8")
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror"]
        if sanitize:
            flags += [
                "-O1", "-g", "-fsanitize=address,undefined",
                "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
            ]
        subprocess.run(
            ["g++", *flags, f"-I{ROOT / 'src'}", str(source_path), "-o", str(binary_path)],
            cwd=ROOT, check=True, timeout=120,
        )
        for case in cases:
            subprocess.run(
                [str(binary_path), *([case] if case else [])],
                cwd=ROOT, check=True, timeout=30,
            )


def test_source_contracts() -> None:
    """Keep each fix on the intended shared boundary."""
    assert '{ "&+WGMCP&N data streaming disabled.\\r\\n", "&+WGMCP&N data streaming enabled.\\r\\n" }' in ACTOTH
    gmcp_case = ACTOTH.split("case 64: /* gmcp */", 1)[1].split("case 65:", 1)[0]
    assert "result = gmcp_tog(PLR3_FLAGS(ch), arg);" in gmcp_case
    assert "static int gmcp_tog(" in ACTOTH
    assert "GMCP_ENABLED(d->character)" in GMCP
    prompt_guard = PROMPT.split("if (!t_ch_p)", 1)[1].split("\n\t}", 1)[0]
    assert "point->prompt_mode = FALSE;" in prompt_guard


def test_gmcp_toggle_state_matrix() -> None:
    """Exercise production GMCP-toggle semantics for every accepted boolean form."""
    prelude = r'''
#include "core/structs.h"
#include <cassert>
#include <cstring>

int yes_no(const char *value)
{
    while (*value == ' ')
        ++value;
    if (!std::strcmp(value, "yes") || !std::strcmp(value, "on") || !std::strcmp(value, "1"))
        return 1;
    if (!std::strcmp(value, "no") || !std::strcmp(value, "off") || !std::strcmp(value, "0"))
        return 0;
    return -1;
}
'''
    production = "\n".join(
        (
            extract_function("actoth.c", "static int plr_tog("),
            extract_function("actoth.c", "static int gmcp_tog("),
        )
    )
    driver = r'''
int main()
{
    for (const char *value : {"on", "yes"})
    {
        unsigned int flags = PLR3_NOGMCP;
        assert(gmcp_tog(flags, value) == 1 && !(flags & PLR3_NOGMCP));
    }
    for (const char *value : {"off", "no"})
    {
        unsigned int flags = 0;
        assert(gmcp_tog(flags, value) == 0 && (flags & PLR3_NOGMCP));
    }

    unsigned int flags = 0;
    assert(gmcp_tog(flags, "") == 0 && (flags & PLR3_NOGMCP));
    assert(gmcp_tog(flags, "") == 1 && !(flags & PLR3_NOGMCP));
    assert(gmcp_tog(flags, "invalid") == -1 && !(flags & PLR3_NOGMCP));
}
'''
    run_harness("gmcp-toggle-", prelude + production + driver)


def test_gmcp_dirty_room_delivery() -> None:
    """Execute real delivery functions with real character/descriptor layouts."""
    prelude = r'''
#include "core/structs.h"
#include "core/utils.h"
#include "net/gmcp.h"
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <vector>

int top_of_world = 619;
std::vector<room_data> rooms(620);
std::vector<char_data> characters(13);
std::vector<descriptor_data> descriptors(12);
room_data *world = rooms.data();
descriptor_data *descriptor_list = descriptors.data();
std::vector<int> deliveries;

char *json_build_room_info(room_data *room, char_data *ch)
{
    assert(world && ch->in_room >= 0 && ch->in_room <= top_of_world);
    assert(room == &world[ch->in_room]);
    return strdup("room_snapshot");
}

void gmcp_send(descriptor_data *d, const char *package, const char *json)
{
    assert(!strcmp(package, GMCP_PKG_ROOM_INFO));
    assert(!strcmp(json, "room_snapshot"));
    deliveries.push_back(static_cast<int>(d->character - characters.data()));
}
'''
    production = extract_function("gmcp.c", "void gmcp_room_info(")
    production += "\n#define MAX_DIRTY_ROOMS" + GMCP.split("#define MAX_DIRTY_ROOMS", 1)[1].split(
        "/*\n * ship contacts auto-update", 1
    )[0]
    # Keep the production queue declarations and functions together.
    driver = r'''
void setup()
{
    const int locations[] = {0, 499, 500, 501, 619, 618, 500, 500, 500, NOWHERE, 620};
    for (int i = 0; i < 12; ++i)
    {
        descriptors[i].gmcp_enabled = true;
        descriptors[i].connected = CON_PLAYING;
        descriptors[i].next = i < 11 ? &descriptors[i + 1] : nullptr;
        if (i < 11)
        {
            descriptors[i].character = &characters[i];
            characters[i].desc = &descriptors[i];
            characters[i].in_room = locations[i];
        }
    }
    characters[6].specials.act = ACT_ISNPC;
    characters[7].specials.act3 = PLR3_NOGMCP;
    descriptors[8].gmcp_enabled = false;
    characters[12].in_room = 500; // Linkdead: no descriptor.
    for (int i = 0; i < 13; ++i)
    {
        if (i == 11) continue; // Descriptor with no character.
        auto &ch = characters[i];
        if (ch.in_room < 0 || ch.in_room > top_of_world) continue;
        ch.next_in_room = world[ch.in_room].people;
        world[ch.in_room].people = &ch;
    }
}

void expect(std::vector<int> wanted)
{
    assert(deliveries == wanted);
    deliveries.clear();
}

void mark_range(int last)
{
    for (int room = 0; room <= last; ++room) gmcp_mark_room_dirty(room);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    setup();
    if (!strcmp(argv[1], "normal"))
    {
        gmcp_mark_room_dirty(619); // Last valid index is inclusive.
        gmcp_mark_room_dirty(0);
        gmcp_mark_room_dirty(619);
        gmcp_flush_dirty_rooms();
        expect({4, 0}); // Preserve queue order and coalesce repeats.
        gmcp_flush_dirty_rooms();
        expect({});
        mark_range(499);
        gmcp_mark_room_dirty(499); // Duplicate in a full queue is not overflow.
        gmcp_flush_dirty_rooms();
        expect({0, 1});
    }
    else if (!strcmp(argv[1], "overflow"))
    {
        mark_range(500);
        gmcp_flush_dirty_rooms();
        expect({0, 1, 2, 3, 4, 5}); // One snapshot per eligible connected player.
        gmcp_flush_dirty_rooms();
        expect({});
        for (auto &d : descriptors) d.gmcp_enabled = false;
        mark_range(619);
        gmcp_flush_dirty_rooms();
        expect({});
        for (auto &d : descriptors) d.gmcp_enabled = true;
        descriptors[8].gmcp_enabled = false;
        gmcp_mark_room_dirty(500);
        gmcp_flush_dirty_rooms();
        expect({2}); // Overflow and queue state reset even without subscribers.
        for (int i = 0; i < 10000; ++i) gmcp_mark_room_dirty(i % 620);
        gmcp_flush_dirty_rooms();
        expect({0, 1, 2, 3, 4, 5});
    }
    else if (!strcmp(argv[1], "invalid"))
    {
        gmcp_mark_room_dirty(NOWHERE);
        gmcp_mark_room_dirty(620);
        gmcp_mark_room_dirty(INT_MAX);
        gmcp_flush_dirty_rooms();
        expect({});
        for (int room : {NOWHERE, 620, INT_MAX})
        {
            characters[0].in_room = room;
            gmcp_room_info(&characters[0]);
        }
        gmcp_room_info(nullptr);
        gmcp_room_info(&characters[12]);
        world = nullptr;
        characters[0].in_room = 0;
        gmcp_room_info(&characters[0]);
        gmcp_mark_room_dirty(0);
        gmcp_flush_dirty_rooms();
        expect({});
    }
    else if (!strcmp(argv[1], "session"))
    {
        // Deletion confirmation retains a loaded character's saved room but
        // does not put that character into the live room population.
        world[618].people = nullptr;
        for (int state : {CON_ACCT_DELETE_CHAR, CON_MAIN_MENU, CON_RMOTD,
                          CON_PLAYER_LOAD, CON_FLUSH, CON_EXIT})
        {
            descriptors[5].connected = state;
            gmcp_mark_room_dirty(618);
            gmcp_flush_dirty_rooms();
            expect({});
            mark_range(500);
            gmcp_flush_dirty_rooms();
            expect({0, 1, 2, 3, 4});
        }
        descriptors[5].connected = CON_PLAYING;
        world[618].people = &characters[5];
        mark_range(500);
        gmcp_flush_dirty_rooms();
        expect({0, 1, 2, 3, 4, 5});
    }
    else if (!strcmp(argv[1], "world"))
    {
        gmcp_mark_room_dirty(619);
        top_of_world = 400; // A queued index no longer belongs to the world.
        gmcp_flush_dirty_rooms();
        expect({});
        top_of_world = 619;
        mark_range(619);
        world = nullptr;
        gmcp_flush_dirty_rooms();
        expect({});
        world = rooms.data();
        gmcp_flush_dirty_rooms();
        expect({});
        gmcp_mark_room_dirty(501);
        gmcp_flush_dirty_rooms();
        expect({3});
    }
    else if (!strcmp(argv[1], "movement"))
    {
        gmcp_mark_room_dirty(500);
        char_data **entry = &world[500].people;
        while (*entry != &characters[2]) entry = &(*entry)->next_in_room;
        *entry = characters[2].next_in_room;
        characters[2].in_room = 501;
        characters[2].next_in_room = world[501].people;
        world[501].people = &characters[2];
        gmcp_flush_dirty_rooms();
        expect({}); // Departed players receive no stale old-room snapshot.
        mark_range(500);
        gmcp_flush_dirty_rooms();
        expect({0, 1, 2, 3, 4, 5}); // Overflow uses the player's current room.
    }
    else assert(false);
}
'''
    run_harness(
        "gmcp-rooms-", prelude + production + driver,
        cases=("normal", "overflow", "invalid", "session", "world", "movement"), sanitize=True,
    )


if __name__ == "__main__":
    test_source_contracts()
    test_gmcp_toggle_state_matrix()
    test_gmcp_dirty_room_delivery()
    print("GMCP toggle, room-delivery and prompt regressions passed")
