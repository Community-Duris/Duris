#!/usr/bin/env python3
"""Run the production single-character purge path with immediate frees under ASan/UBSan."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

# Execute the complete character branch, including permission checks and NPC dispatch.
branch = extract_function("actwiz.c", "if ((vict = get_char_room_vis(ch, name)))")
harness = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/files.h"
#include "sql/sql.h"
#include "net/comm.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
static room_data rooms[2]{};
P_room world = rooms;
static P_char victim;
static bool save_ok, item_busy, coin_busy, collector_busy, raise_busy, recovered;
static int saves, extracted, announcements, faction_delta, logged_out, offline;
static std::string output;
static obj_data bag{}, child{}, worn{};
P_char get_char_room_vis(P_char, const char *) { return victim; }
int IS_MORPH(P_char) { return false; }
void send_to_char(const char *text, P_char) { output += text; }
void act(const char *, int, P_char, P_obj, void *, int) { ++announcements; }
void wizlog(int, const char *, ...) {}
void logit(const char *, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
bool item_movement_transaction_player_busy(P_char) { return item_busy; }
bool currency_transaction_player_busy(P_char) { return coin_busy; }
bool collector_transaction_player_busy(P_char) { return collector_busy; }
bool corpse_raise_player_save_fenced(P_char) { return raise_busy; }
bool collector_service_recover_player(P_char) { return recovered; }
void graph_unchanged(P_char ch) {
    assert(ch->carrying == &bag && bag.contains == &child && ch->equipment[0] == &worn);
    assert(ch->only.pc->prestige == 50 && !rooms[1].contents);
}
bool persistence_save_character_terminal(P_char ch, int type) {
    assert(type == RENT_QUIT && !announcements && !faction_delta);
    assert(!item_busy && !coin_busy && !collector_busy && !raise_busy && recovered);
    graph_unchanged(ch); ++saves; return save_ok;
}
void update_ingame_racewar(int delta) { assert(saves == 1 && save_ok); faction_delta += delta; }
void sql_log_player_login(P_char, const char *event) {
    assert(saves == 1 && save_ok && !strcmp(event, "logout")); ++logged_out;
}
void redis_player_offline(P_char) { assert(saves == 1 && save_ok); ++offline; }
void dispose(P_char ch) {
    if (ch->desc) ch->desc->character = nullptr;
    if (IS_PC(ch)) delete ch->only.pc; else delete ch->only.npc;
    delete ch; victim = nullptr; ++extracted;
}
void extract_char_after_terminal_save(P_char ch) {
    assert(saves == 1 && save_ok && logged_out == 1 && offline == 1);
    graph_unchanged(ch); dispose(ch);
}
void extract_char(P_char ch) { assert(IS_NPC(ch)); dispose(ch); }
void free_char(P_char) { abort(); }
int writeCharacter(P_char, int, int) { abort(); }
void close_socket(P_desc) { abort(); }
void purge(P_char ch) {
    P_char vict;
    const int level = MIN(62, GET_LEVEL(ch));
    char name[] = "Victim";
''' + branch + r'''
}
int main() {
    for (int scenario = 0; scenario < 13; ++scenario) {
        char_data staff{}; pc_only_data staff_pc{}; descriptor_data descriptor{};
        staff.only.pc = &staff_pc; staff.player.level = OVERLORD; staff.in_room = 1;
        staff.player.name = const_cast<char *>("Staff"); staff.player.racewar = 1;
        victim = new char_data{}; victim->only.pc = new pc_only_data{};
        victim->player.name = const_cast<char *>("Victim"); victim->player.level = 30;
        victim->player.racewar = 2; victim->in_room = 1; victim->only.pc->prestige = 50;
        bag = {}; child = {}; worn = {}; rooms[1] = {}; rooms[1].number = 100;
        bag.extra_flags = ITEM_TRANSIENT; bag.contains = &child;
        victim->carrying = &bag; victim->equipment[0] = &worn;
        if (scenario != 1) { victim->desc = &descriptor; descriptor.character = victim; }
        saves = extracted = announcements = faction_delta = logged_out = offline = 0;
        output.clear(); save_ok = recovered = true;
        item_busy = coin_busy = collector_busy = raise_busy = false;
        if (scenario == 2) save_ok = false;
        if (scenario == 3) rooms[1].room_flags = ROOM_LOCKER;
        if (scenario == 4) item_busy = true;
        if (scenario == 5) coin_busy = true;
        if (scenario == 6) collector_busy = true;
        if (scenario == 7) raise_busy = true;
        if (scenario == 8) recovered = false;
        if (scenario == 9) victim->player.level = OVERLORD;
        if (scenario == 10) staff.player.level = GREATER_G - 1;
        if (scenario == 11) victim->player.level = MINLVLIMMORTAL;
        if (scenario == 12) {
            delete victim->only.pc;
            victim->only.npc = new npc_only_data{};
            victim->specials.act = ACT_ISNPC;
        }
        purge(&staff);
        if (scenario == 12) {
            assert(!victim && extracted == 1 && !descriptor.character);
            assert(!saves && !faction_delta && !logged_out && !offline);
        } else if (scenario < 2 || scenario == 11) {
            assert(!victim && extracted == 1 && !descriptor.character);
            assert(faction_delta == (scenario == 11 ? 0 : -2));
        } else {
            assert(victim && !extracted && !announcements && !faction_delta);
            assert(saves == (scenario == 2 ? 1 : 0));
            graph_unchanged(victim); dispose(victim);
        }
    }
    puts("staff purge terminal-save ordering, refusal, belongings and lifetime passed");
}
'''
with tempfile.TemporaryDirectory(prefix="staff-purge-") as tmp:
    test, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    test.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=all", "-fno-pie", "-no-pie", "-Isrc",
                    "-I/usr/include/mysql",
                    str(test), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
