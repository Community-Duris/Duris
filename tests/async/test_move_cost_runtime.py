#!/usr/bin/env python3
"""Execute the production movement-cost calculation against sector fixtures.

The harness extracts the real move_cost() body and supplies only the world,
movement table, and unrelated game services needed by that function.  It keeps
terrain costs, Freedom of Movement, flight, altitude, ice, and invalid-sector
fallback behavior under one executable regression test.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function


MOVE_COST = extract_function("utility.c", "int move_cost(P_char ch, int dir)")

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/grapple.h"
#include "magic/spells.h"

#include <cstdio>
#include <cstdlib>

static room_data rooms[2]{};
P_room world = rooms;
static bool ice;

extern const int movement_loss[NUM_SECT_TYPES] = {
    1, 1, 3, 3, 4, 5, 6, 6, 6, 7, 7, 3, 75, 3, 2, 2, 5, 5, 5, 2,
    5, 4, 2, 2, 7, 7, 7, 7, 5, 3, 5, 4, 10, 2, 2, 4, 4, 1, 7, 10,
};

int load_modifier(P_char) { return 75; }
int is_ice(P_char, int) { return ice ? 1 : 0; }
bool affected_by_spell(P_char, int) { return false; }
'''

DRIVER = r'''
static int run_move(char_data &actor, int from, int to, ulong affects = 0,
                    int altitude = 0, bool on_ice = false)
{
    rooms[0].sector_type = from;
    rooms[1].sector_type = to;
    actor.specials.affected_by = affects;
    actor.specials.z_cord = altitude;
    ice = on_ice;
    return move_cost(&actor, 0);
}

static void require_cost(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        std::fprintf(stderr, "%s: expected %d, got %d\n", name, expected, actual);
        std::exit(1);
    }
}

int main()
{
    room_direction_data north{};
    north.to_room = 1;
    rooms[0].dir_option[0] = &north;

    char_data actor{};
    actor.in_room = 0;
    actor.specials.act = 0; // mortal, not a mount

    // The reported regression: ocean (75) + ocean (75), load 75 -> 56.
    require_cost("ocean-to-ocean", run_move(actor, SECT_OCEAN, SECT_OCEAN), 56);
    require_cost("ocean-to-land", run_move(actor, SECT_OCEAN, SECT_FIELD), 29);

    // Spell and movement modifiers remain distinct from terrain lookup.
    require_cost("freedom-of-movement",
                 run_move(actor, SECT_OCEAN, SECT_OCEAN, AFF_FREEDOM_OF_MVMNT), 1);
    require_cost("ordinary-flight-at-zero-altitude",
                 run_move(actor, SECT_OCEAN, SECT_OCEAN, AFF_FLY), 56);
    require_cost("positive-altitude",
                 run_move(actor, SECT_OCEAN, SECT_OCEAN, 0, 1), 18);
    require_cost("ice-ocean",
                 run_move(actor, SECT_OCEAN, SECT_OCEAN, 0, 0, true), 14);

    // A valid sector above the old hardcoded boundary keeps its table entry.
    require_cost("desert-to-desert",
                 run_move(actor, SECT_DESERT, SECT_DESERT), 5);

    // Truly invalid sector values still use the safe field fallback.
    require_cost("invalid-sector-fallback", run_move(actor, 127, 127), 2);

    std::puts("move_cost sector-boundary runtime regressions passed");
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-move-cost-runtime-") as directory:
    temporary = Path(directory)
    harness = temporary / "harness.cpp"
    binary = temporary / "harness"
    harness.write_text("\n".join((PRELUDE, MOVE_COST, DRIVER)), encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-ffunction-sections",
            "-fdata-sections",
            f"-I{ROOT / 'src'}",
            str(harness),
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

print("move_cost sector-boundary runtime regression passed")
