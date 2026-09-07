#!/usr/bin/env python3
"""Source and message-audience regressions for ship-audit review fixes."""

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, SRC
from contract_text import contains


source = (SRC / "ships/ship_auto.c").read_text()
start = source.index("int shipgroupremove(")
end = source.index("int shipgroupadd(", start)
body = source[start:end]
member = body[body.rindex("\ttmpgroup2 = autopilot->group;") :]

# A member node must be located through its predecessor and unlinked before it
# is freed.  This also prevents the old loop from advancing through a null or
# freed cursor after autopilot->group is cleared.
assert contains(member, "while (tmpgroup && tmpgroup->next != tmpgroup2)")
splice = member.index("tmpgroup->next = tmpgroup2->next;")
release = member.index("FREE(tmpgroup2);")
clear = member.index("autopilot->group = NULL;", release)
assert splice < release < clear
assert not contains(member, "if ((tmpgroup = autopilot->group))")

# Projection must clamp to getmap()'s populated 0..99 range without changing
# the map's established 100-y coordinate inversion.
assert contains(source, "#define AUTOPILOT_MAP_MAX 99")
assert contains(source, "int map_y = 100 - (int)(ydist + ship->y);")

npc_source = (SRC / "ships/ship_npc.c").read_text()
start = npc_source.index("void setup_npc_caravel_03(")
end = npc_source.index("void setup_npc_caravel_04(", start)
caravel = npc_source[start:end]
for slot in (1, 2, 3):
    assert contains(caravel, f"set_weapon(ship, {slot}, W_MEDIUM_BAL, SIDE_PORT);")
assert caravel.count("W_MEDIUM_BAL, SIDE_PORT") == 3
assert not contains(npc_source, "Selection is uniform over the matching")
assert contains(npc_source, "it is not strictly uniform")

shop_source = (SRC / "ships/ship_shop.c").read_text()
assert not contains(shop_source, "a rejected transaction refunds through")
assert not contains(shop_source, "you cannot re-hull")
assert contains(shop_source, "wartime hull changes take five times longer")

# Execute the production hull/weapon functions with deterministic randomness and
# captured output.  Both attack paths must report damage/destruction to the
# correct audience; absent or already-destroyed weapons produce no such report.
combat_source = (SRC / "ships/ship_combat.c").read_text()
start = combat_source.index("int damage_hull(")
end = combat_source.index("\nvoid force_anchor(", start)
harness = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "ships/ships.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

static std::map<const void *, std::vector<std::string>> captured_messages;
int number(int low, int) { return low; }
const char *get_arc_name(int) { return "fore"; }
char *ShipSlot::get_description() { return desc; }
void stun_all_in_ship(P_ship, int) {}
void act_to_outside_ships(P_ship, P_ship, int, const char *, ...) {}
void act_to_all_in_ship(P_ship ship, const char *message)
{
    captured_messages[ship].emplace_back(message);
}
void act_to_all_in_ship_f(P_ship ship, const char *format, ...)
{
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    captured_messages[ship].emplace_back(message);
}
void send_to_char_f(P_char ch, const char *format, ...)
{
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    captured_messages[ch].emplace_back(message);
}
void act(const char *message, int, P_char ch, P_obj, void *, int audience)
{
    assert(audience == TO_CHAR);
    captured_messages[ch].emplace_back(message);
}
''' + combat_source[start:end] + r'''
int main()
{
    char_data character{};
    ShipData attacker{};
    for (int mode = 0; mode < 3; ++mode) // character, ship, no attacker
    {
        for (int initial : {0, 95, 100, -1, -2})
        {
            captured_messages.clear();
            ShipData target{};
            char id[] = "AB", name[] = "victim";
            target.id = id;
            target.name = name;
            target.internal[SIDE_FORE] = 100;
            auto &weapon = target.slot[0];
            weapon.type = initial == -1 ? SLOT_EMPTY : SLOT_WEAPON;
            weapon.position = initial == -2 ? SIDE_PORT : SIDE_FORE;
            weapon.val2 = initial < 0 ? 0 : initial;
            snprintf(weapon.desc, sizeof(weapon.desc), "ballista");
            if (mode == 0)
                assert(ch_damage_hull(&character, &target, 1, SIDE_FORE, 0));
            else
                assert(damage_hull(mode == 1 ? &attacker : nullptr,
                                   &target, 1, SIDE_FORE, 0));

            const bool hit = initial == 0 || initial == 95;
            assert(target.internal[SIDE_FORE] == 99);
            assert(weapon.val2 == (initial < 0 ? 0 : initial) + (hit ? 5 : 0));
            const auto &victim_messages = captured_messages[&target];
            assert(victim_messages.size() == (hit ? 2 : 1));
            for (const auto &message : victim_messages)
            {
                assert(message.find("You damage") == std::string::npos);
                assert(message.find("You destroy") == std::string::npos);
            }
            if (hit)
                assert(victim_messages.back() == (initial == 95
                    ? " &+RYour &+Wballista &+Rhas been destroyed!&N"
                    : " &+WYour ballista has been damaged!&N"));
            if (mode < 2)
            {
                const void *recipient = mode == 0 ? static_cast<void *>(&character)
                                                 : static_cast<void *>(&attacker);
                const auto &attacker_messages = captured_messages[recipient];
                assert(attacker_messages.size() == (hit ? 2 : 1));
                if (hit)
                {
                    std::string expected = initial == 95
                        ? " &+GYou destroy a &+Wballista&+G!&N"
                        : " &+GYou damage a &+Wballista&+G!&N";
                    if (mode == 0) expected += "\r\n";
                    assert(attacker_messages.back() == expected);
                }
            }
            assert(captured_messages.size() == (mode == 2 ? 1 : 2));
        }
    }
    puts("ship weapon damage audiences: 15 cases passed");
}
'''
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="ship-audit-", dir=build) as directory:
    source_path = Path(directory) / "messages.cpp"
    binary = Path(directory) / "messages"
    source_path.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
        str(source_path), "-o", str(binary),
    ], check=True, cwd=ROOT, timeout=60)
    subprocess.run([str(binary)], check=True, cwd=ROOT, timeout=10)

print("ship audit review regressions passed")
