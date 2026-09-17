#!/usr/bin/env python3
"""Execute production group/combat removal for valid and inconsistent lists."""

import os
from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
struct char_data;
using P_char = char_data *;
struct group_list { P_char ch; group_list *next; };
struct affected_type { int modifier = 9; };
struct char_data {
    group_list *group = nullptr;
    struct { int act2 = 3; int affected_by3 = 3; P_char was_fighting = nullptr;
             P_char next_fighting = nullptr; P_char opponent = nullptr; } specials;
    struct { const char *short_descr = "synthetic mobile"; } player;
    void *desc = nullptr;
    const char *name = "synthetic character";
    int in_room = 5;
    int updates = 0, telemetry = 0, combat_ends = 0, positions = 0, cooldowns = 0;
    int purges = 0, aura_adds = 0, aura_removes = 0, rank_fixes = 0;
    int skill = 1;
    bool sane = true, spell = true, aura = true;
    affected_type effect;
    std::vector<std::string> messages;
};
#define TRUE true
#define FALSE false
#define MAX_STRING_LENGTH 1024
#define GET_NAME(ch) ((ch)->name)
#define IS_NPC(ch) false
#define IS_PC(ch) (!(IS_NPC(ch)))
#define PLR2_BACK_RANK 1
#define REMOVE_BIT(bits, flag) ((bits) &= ~(flag))
#define IS_SET(bits, flag) ((bits) & (flag))
#define AFF3_TRACKING 1
#define GET_OPPONENT(ch) ((ch)->specials.opponent)
#define IS_FIGHTING(ch) (GET_OPPONENT(ch) != nullptr)
#define GET_CHAR_SKILL(ch, unused_skill) ((ch)->skill)
#define SPELL_CEGILUNE_BLADE 1
#define SKILL_LANCE_CHARGE 2
#define PULSE_VIOLENCE 8
#define LOG_EXIT 0
#define LOG_DEBUG 1
int dead_group_pool = 0, group_logs = 0, combat_logs = 0, room_marks = 0;
std::vector<group_list *> released;
P_char combat_list = nullptr, combat_next_ch = nullptr;
void wizlog(int, const char *format, const char *) {
    assert(std::string(format).find("claims to be a member") != std::string::npos);
    ++group_logs;
}
void logit(int, const char *format, const char * = nullptr) {
    assert(std::string(format).find("not found in combat_list") != std::string::npos);
    ++combat_logs;
}
void purge_linked_auras(P_char ch) { ++ch->purges; }
P_char in_command_aura(P_char ch) {
    return ch->aura && ch->group ? ch->group->ch : nullptr;
}
void remove_aura_message(P_char ch, P_char) { ++ch->aura_removes; }
void add_aura_message(P_char ch, P_char) { ++ch->aura_adds; }
void mm_release(int, group_list *node) { released.push_back(node); delete node; }
void send_to_char(const char *msg, P_char ch) { ch->messages.emplace_back(msg); }
void update_groupies(P_char ch, bool = false) { ++ch->updates; }
void telemetry_runtime_game_context(P_char ch, void *) { ++ch->telemetry; }
enum class telemetry_encounter_outcome { withdrawal = 5 };
void telemetry_runtime_game_encounter_leave(P_char, telemetry_encounter_outcome) {}
void telemetry_group_context_changed(group_list *gl) {
    for (; gl; gl = gl->next) ++gl->ch->telemetry;
}
int free_back_slots(P_char) { return -1; }
void fix_group_ranks(P_char ch) { ++ch->rank_fixes; }
bool SanityCheck(P_char ch, const char *) { return ch && ch->sane; }
void telemetry_combat_context_changed(P_char ch) { ++ch->telemetry; }
bool affected_by_spell(P_char ch, int) { return ch->spell; }
affected_type *get_spell_from_char(P_char ch, int) { return &ch->effect; }
void set_short_affected_by(P_char ch, int skill, int duration) {
    assert(skill == SKILL_LANCE_CHARGE && duration == PULSE_VIOLENCE / 2);
    ++ch->cooldowns;
}
void gmcp_mark_room_dirty(int room) { assert(room == 5); ++room_marks; }
void gmcp_combat_end(P_char ch) { ++ch->combat_ends; }
void update_pos(P_char ch) { ++ch->positions; }
bool group_remove_member(P_char);
'''

DRIVER = r'''
group_list *make_group(std::vector<P_char> members) {
    group_list *head = nullptr;
    for (auto it = members.rbegin(); it != members.rend(); ++it)
        head = new group_list{*it, head};
    for (auto ch : members) ch->group = head;
    return head;
}
void destroy_group(group_list *head) {
    while (head) { auto next = head->next; delete head; head = next; }
}
void reset_counts() { released.clear(); group_logs = combat_logs = room_marks = 0; }
void test_group_absent(int size) {
    reset_counts();
    char_data leader, member, outsider;
    auto head = make_group(size == 1 ? std::vector<P_char>{&leader}
                                    : std::vector<P_char>{&leader, &member});
    auto tail = head->next;
    outsider.group = head;
    assert(!group_remove_member(&outsider));
    assert(group_logs == 1 && released.empty());
    assert(outsider.group == head && outsider.specials.act2 == 3);
    assert(outsider.purges == 0 && outsider.updates == 0 && outsider.telemetry == 0);
    assert(head->ch == &leader && head->next == tail && leader.group == head);
    assert(leader.messages.empty() && leader.rank_fixes == 0 && leader.aura_removes == 0);
    if (size == 2) assert(member.group == head && !tail->next && tail->ch == &member);
    destroy_group(head);
}
void test_group_remove(int index, int size) {
    reset_counts();
    char_data chars[4];
    std::vector<P_char> members;
    for (int i = 0; i < size; ++i) members.push_back(&chars[i]);
    auto head = make_group(members);
    auto removed = &chars[index];
    auto expected_head = index == 0 ? head->next : head;
    assert(group_remove_member(removed));
    assert(!removed->group && removed->specials.act2 == 2 && removed->purges == 1);
    assert(removed->updates == 1 && removed->telemetry == 1 && group_logs == 0);
    if (size <= 2) {
        assert(released.size() == static_cast<size_t>(size));
        for (auto ch : members) assert(!ch->group);
        if (size == 2) {
            auto survivor = &chars[1 - index];
            assert(survivor->messages.size() == 1);
            assert(survivor->messages[0] == "Your group has been disbanded.\n");
        }
        return;
    }
    assert(released.size() == 1);
    auto node = expected_head;
    for (int i = 0; i < size; ++i) {
        if (i == index) continue;
        assert(node && node->ch == &chars[i] && chars[i].group == expected_head);
        node = node->next;
    }
    assert(!node && expected_head->ch->rank_fixes == 1);
    if (index == 0) {
        assert(expected_head->ch->messages[0] == "You are now the leader of your group!\n");
        for (int i = 1; i < size; ++i) assert(chars[i].aura_adds == 1 && chars[i].aura_removes == 1);
    } else assert(removed->aura_removes == 1);
    destroy_group(expected_head);
}
void test_disband() {
    reset_counts();
    char_data leader, middle, tail;
    make_group({&leader, &middle, &tail});
    do_disband(&leader, nullptr, 0);
    assert(!leader.group && !middle.group && !tail.group && released.size() == 3);
    assert(group_logs == 0);
}
void check_combat_cleanup(P_char ch, P_char opponent) {
    assert(ch->specials.was_fighting == opponent);
    assert(!ch->specials.next_fighting && !GET_OPPONENT(ch));
    assert(ch->specials.affected_by3 == 2 && ch->effect.modifier == 0);
    assert(ch->telemetry == 1 && ch->cooldowns == 1 && ch->combat_ends == 1);
    assert(ch->positions == 1 && room_marks == 1);
}
void test_combat_remove(int index, bool is_next) {
    reset_counts();
    char_data chars[3], opponent, unrelated_next;
    for (int i = 0; i < 3; ++i) {
        chars[i].specials.next_fighting = i < 2 ? &chars[i + 1] : nullptr;
        GET_OPPONENT(&chars[i]) = &opponent;
    }
    combat_list = &chars[0];
    auto removed = &chars[index];
    auto next = removed->specials.next_fighting;
    combat_next_ch = is_next ? removed : &unrelated_next;
    stop_fighting(removed);
    check_combat_cleanup(removed, &opponent);
    assert(combat_logs == 0 && combat_next_ch == (is_next ? next : &unrelated_next));
    auto node = combat_list;
    for (int i = 0; i < 3; ++i) {
        if (i == index) continue;
        assert(node == &chars[i] && GET_OPPONENT(node) == &opponent);
        assert(node->telemetry == 0 && node->combat_ends == 0);
        node = node->specials.next_fighting;
    }
    assert(!node);
}
void test_combat_absent(bool empty, bool is_next) {
    reset_counts();
    char_data leader, tail, outsider, opponent, stale_next;
    leader.specials.next_fighting = &tail;
    GET_OPPONENT(&leader) = GET_OPPONENT(&tail) = GET_OPPONENT(&outsider) = &opponent;
    outsider.specials.next_fighting = &stale_next;
    combat_list = empty ? nullptr : &leader;
    combat_next_ch = is_next ? &outsider : &leader;
    stop_fighting(&outsider);
    check_combat_cleanup(&outsider, &opponent);
    assert(combat_logs == 1 && combat_list == (empty ? nullptr : &leader));
    assert(leader.specials.next_fighting == &tail && !tail.specials.next_fighting);
    assert(GET_OPPONENT(&leader) == &opponent && GET_OPPONENT(&tail) == &opponent);
    assert(combat_next_ch == (is_next ? &stale_next : &leader));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    if (mode == "group-absent") { test_group_absent(1); test_group_absent(2); }
    else if (mode == "combat-absent") {
        for (bool empty : {false, true}) for (bool is_next : {false, true})
            test_combat_absent(empty, is_next);
    } else if (mode == "group-singleton") {
        test_group_remove(0, 1);
    } else {
        char_data ungrouped;
        assert(group_remove_member(&ungrouped) && ungrouped.purges == 0);
        for (int i = 0; i < 4; ++i) test_group_remove(i, 4);
        for (int i = 0; i < 2; ++i) test_group_remove(i, 2);
        test_disband();
        for (int i = 0; i < 3; ++i) for (bool is_next : {false, true})
            test_combat_remove(i, is_next);
        char_data idle, invalid, opponent;
        invalid.sane = false; GET_OPPONENT(&invalid) = &opponent;
        stop_fighting(nullptr); stop_fighting(&idle); stop_fighting(&invalid);
        assert(idle.positions == 0 && invalid.positions == 0);
        reset_counts();
        char_data plain;
        plain.spell = false; plain.skill = 0; plain.in_room = -1;
        plain.specials.affected_by3 = 2; GET_OPPONENT(&plain) = &opponent;
        combat_list = combat_next_ch = &plain;
        stop_fighting(&plain);
        assert(!combat_list && !combat_next_ch && !GET_OPPONENT(&plain));
        assert(plain.cooldowns == 0 && room_marks == 0 && plain.effect.modifier == 9);
        assert(plain.telemetry == 1 && plain.positions == 1 && plain.combat_ends == 1);
    }
}
'''

with tempfile.TemporaryDirectory(prefix="member-removal-") as directory:
    source = Path(directory) / "harness.cc"
    binary = Path(directory) / "harness"
    source.write_text("\n".join([
        PRELUDE,
        extract_function("group.c", "bool group_remove_member(P_char ch)"),
        extract_function("group.c", "void do_disband(P_char ch,"),
        extract_function("fight.c", "void stop_fighting(P_char ch)"),
        DRIVER,
    ]))
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        str(source), "-o", str(binary),
    ], check=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    failures = []
    for case in ("group-absent", "combat-absent", "group-singleton", "normal"):
        result = subprocess.run([str(binary), case], env=env, timeout=30)
        if result.returncode:
            failures.append(case)
        print(f"{case}: {'FAIL' if result.returncode else 'PASS'}", flush=True)
    assert not failures, f"Failed scenarios: {failures}"
