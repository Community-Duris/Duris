#!/usr/bin/env python3
"""Exercise the PvP credit decision without running death side effects."""

from _paths import SRC, extract_function
from pathlib import Path
import subprocess
import tempfile


decision = extract_function("combat/fight.c", "static P_char credited_player_killer(")
fight = (SRC / "fight.c").read_text()
death = fight[fight.index("void die(P_char ch, P_char killer)"):]
assert death.index("credited_player_killer(ch, killer)") < death.index("setHeavenTime(ch)")
assert death.index("setHeavenTime(ch)") < death.index("submit_pvp_outcome(")

harness = r'''
#include <cassert>
#include <cstdio>
struct char_data {
    bool npc = false;
    int in_room = 0;
    int group = 0;
    char_data *master = nullptr;
};
using P_char = char_data *;
#define LNK_PET 1
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!IS_NPC(ch))
static P_char get_linked_char(P_char ch, int) { return ch->master; }
#define IS_PC_PET(mob) (IS_NPC(mob) && get_linked_char(mob, LNK_PET) && IS_PC(get_linked_char(mob, LNK_PET)))
#define GET_MASTER(ch) (get_linked_char(ch, LNK_PET))
''' + decision + r'''
int main()
{
    char_data victim, player, pet, other_npc;
    victim.in_room = player.in_room = pet.in_room = 3;
    player.group = pet.group = 7;
    pet.npc = other_npc.npc = true;
    assert(credited_player_killer(&victim, &player) == &player);
    assert(credited_player_killer(&victim, &victim) == nullptr);
    assert(credited_player_killer(&victim, &other_npc) == nullptr);
    assert(credited_player_killer(&victim, &pet) == nullptr);
    pet.master = &player;
    assert(credited_player_killer(&victim, &pet) == &player);
    player.in_room = 4;
    assert(credited_player_killer(&victim, &pet) == nullptr);
    player.in_room = 3;
    player.group = 8;
    assert(credited_player_killer(&victim, &pet) == nullptr);
    std::puts("PvP kill credit decision passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-pvp-credit-") as directory:
    source = Path(directory) / "credit.cpp"
    binary = Path(directory) / "credit"
    source.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
