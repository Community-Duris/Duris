#!/usr/bin/env python3
"""Execute production healing spell bodies and hit_regen with isolated game services."""
from pathlib import Path
import subprocess
from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "magic/spells.h"
#include "world/epic_bonus.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <strings.h>

static room_data rooms[1]{};
P_room world = rooms;
static int innate_gain = 100;
bool affected_by_spell(P_char ch, int type) {
    for (auto *af = ch->affected; af; af = af->next)
        if (af->type == type) return true;
    return false;
}
void send_to_char(const char *, P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
float get_property(const char *, double) { return 9.0f; }
bool has_innate(P_char, int) { return false; }
int get_innate_regeneration(P_char) { return innate_gain; }
float get_epic_bonus(P_char, int) { return 0; }
room_affect *get_spell_from_room(P_room, int) { return nullptr; }
bool IS_TWILIGHT_ROOM(int) { return false; }
bool IS_OUTDOORS(int) { return false; }
#undef IS_SUNLIT
#define IS_SUNLIT(r) false
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af);
    copy->next = ch->affected;
    ch->affected = copy;
    if (copy->location == APPLY_HIT_REG) ch->points.hit_reg += copy->modifier;
    ch->specials.affected_by4 |= copy->bitvector4;
    return copy;
}
static void remove_spell(P_char ch, int type) {
    for (auto **p = &ch->affected; *p;) {
        auto *af = *p;
        if (af->type != type) { p = &af->next; continue; }
        *p = af->next;
        if (af->location == APPLY_HIT_REG) ch->points.hit_reg -= af->modifier;
        delete af;
    }
    ch->specials.affected_by4 = 0;
    for (auto *af = ch->affected; af; af = af->next)
        ch->specials.affected_by4 |= af->bitvector4;
}
static int count(P_char ch, int type) {
    int n = 0;
    for (auto *af = ch->affected; af; af = af->next) n += af->type == type;
    return n;
}
'''
DRIVER = r'''
int main() {
    for (int order : {0, 1}) {
        for (int state : {STAT_NORMAL, STAT_RESTING, STAT_SLEEPING}) {
            for (int base : {-20, 0, 16, 17, 400}) {
                for (int strength : {20, 100, 500}) {
                    char_data ch{};
                    ch.specials.position = state;
                    ch.specials.conditions[FULL] = ch.specials.conditions[THIRST] = 24;
                    ch.points.hit = 10; ch.points.max_hit = 1000;
                    ch.points.hit_reg = base;
                    innate_gain = strength;
                    spell_regeneration(50, &ch, nullptr, 0, &ch, nullptr);
                    int regen = hit_regen(&ch, true);
                    ch.specials.fighting = &ch;
                    int combat = hit_regen(&ch, true);
                    ch.specials.fighting = nullptr;
                    remove_spell(&ch, SPELL_REGENERATION);
                    spell_accel_healing(50, &ch, nullptr, 0, &ch, nullptr);
                    int accel = hit_regen(&ch, true);
                    remove_spell(&ch, SPELL_ACCEL_HEALING);
                    if (order == 0) spell_regeneration(50, &ch, nullptr, 0, &ch, nullptr);
                    spell_accel_healing(50, &ch, nullptr, 0, &ch, nullptr);
                    spell_regeneration(50, &ch, nullptr, 0, &ch, nullptr);
                    assert(count(&ch, SPELL_REGENERATION) == 1);
                    assert(count(&ch, SPELL_ACCEL_HEALING) == 1);
                    assert(hit_regen(&ch, true) == std::max(regen, accel));
                    assert(hit_regen(&ch, false) == hit_regen(&ch, true));
                    for (auto *af = ch.affected; af; af = af->next) af->duration = 1;
                    int modifier = ch.points.hit_reg;
                    spell_regeneration(50, &ch, nullptr, 0, &ch, nullptr);
                    spell_accel_healing(50, &ch, nullptr, 0, &ch, nullptr);
                    assert(count(&ch, SPELL_REGENERATION) == 1 && count(&ch, SPELL_ACCEL_HEALING) == 1);
                    assert(ch.points.hit_reg == modifier);
                    for (auto *af = ch.affected; af; af = af->next)
                        assert(af->duration == (af->type == SPELL_REGENERATION ? 5 : 42));
                    ch.points.hit = ch.points.max_hit;
                    assert(hit_regen(&ch, false) == 0);
                    ch.points.hit = 10;
                    ch.specials.affected_by3 |= AFF3_SWIMMING;
                    assert(hit_regen(&ch, true) == 0);
                    ch.specials.affected_by3 &= ~AFF3_SWIMMING;
                    ch.specials.fighting = &ch;
                    assert(hit_regen(&ch, true) == combat);
                    ch.specials.fighting = nullptr;
                    if (order == 0) {
                        remove_spell(&ch, SPELL_REGENERATION);
                        assert(hit_regen(&ch, true) == accel);
                    } else {
                        remove_spell(&ch, SPELL_ACCEL_HEALING);
                        assert(hit_regen(&ch, true) == regen);
                    }
                    remove_spell(&ch, SPELL_REGENERATION);
                    remove_spell(&ch, SPELL_ACCEL_HEALING);
                    assert(ch.points.hit_reg == base);
                }
            }
        }
    }
    for (int blocker : {SKILL_REGENERATE, SPELL_PACTUM_SERPENTIS}) {
        char_data ch{};
        ch.specials.position = STAT_NORMAL;
        affected_type af{};
        af.type = blocker;
        affect_to_char(&ch, &af);
        spell_regeneration(50, &ch, nullptr, 0, &ch, nullptr);
        spell_accel_healing(50, &ch, nullptr, 0, &ch, nullptr);
        assert(count(&ch, SPELL_REGENERATION) == 0);
        assert(count(&ch, SPELL_ACCEL_HEALING) == 0);
        remove_spell(&ch, blocker);
    }
    std::puts("healing coexistence, precedence, refresh, combat and removal regressions passed");
}
'''
output = ROOT / 'bin' / 'tests' / 'healing-spell-precedence'
output.mkdir(parents=True, exist_ok=True)
harness = output / 'harness.cpp'
harness.write_text(PRELUDE + '\n'.join([
    extract_function('magic.c', 'void spell_regeneration('),
    extract_function('magic.c', 'void spell_accel_healing('),
    extract_function('limits.c', 'int hit_regen('),
]) + DRIVER)
binary = output / 'harness'
subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I' + str(ROOT / 'src'),
                str(harness), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)

