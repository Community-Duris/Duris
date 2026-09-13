#!/usr/bin/env python3
"""Run cleric utility spell bodies with real protection/cure logic and isolated effects."""

import subprocess
from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdio>
#include <strings.h>

void send_to_char(const char *, P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
bool affected_by_spell(P_char ch, int type) {
    for (auto *af = ch->affected; af; af = af->next)
        if (af->type == type) return true;
    return false;
}
static void rebuild_bits(P_char ch) {
    ch->specials.affected_by = ch->specials.affected_by2 = 0;
    for (auto *af = ch->affected; af; af = af->next) {
        ch->specials.affected_by |= af->bitvector;
        ch->specials.affected_by2 |= af->bitvector2;
    }
}
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af);
    copy->next = ch->affected;
    ch->affected = copy;
    rebuild_bits(ch);
    return copy;
}
void affect_remove(P_char ch, affected_type *target) {
    for (auto **p = &ch->affected; *p; p = &(*p)->next) {
        if (*p != target) continue;
        *p = target->next;
        delete target;
        rebuild_bits(ch);
        return;
    }
}
void affect_from_char(P_char ch, int type) {
    for (auto *af = ch->affected; af;) {
        auto *next = af->next;
        if (af->type == type) affect_remove(ch, af);
        af = next;
    }
}
static void add(P_char ch, int type, unsigned int bits = 0, unsigned int bits2 = 0) {
    affected_type af{};
    af.type = type;
    af.duration = 7;
    af.bitvector = bits;
    af.bitvector2 = bits2;
    affect_to_char(ch, &af);
}
static int count(P_char ch) {
    int n = 0;
    for (auto *af = ch->affected; af; af = af->next) ++n;
    return n;
}
static void clear(P_char ch) {
    while (ch->affected) affect_remove(ch, ch->affected);
}
static void initialize(P_char ch) {
    ch->specials.position = STAT_NORMAL;
    ch->in_room = 5;
    ch->points.hit = 41;
    ch->points.max_hit = 100;
}
static void ailments(P_char ch) {
    add(ch, SPELL_POISON, 0, AFF2_POISONED);
    add(ch, POISON_LIFELEAK, 0, AFF2_POISONED);
    add(ch, SPELL_BLINDNESS, AFF_BLIND);
    add(ch, SPELL_DISEASE);
    add(ch, SPELL_CONTAGION);
    add(ch, SPELL_CURSE);
    add(ch, SPELL_PLAGUE);
    add(ch, SPELL_BLESS);
}
static void check_clean(P_char ch) {
    assert(count(ch) == 3);
    assert(!IS_AFFECTED(ch, AFF_BLIND));
    assert(!IS_AFFECTED2(ch, AFF2_POISONED));
    assert(affected_by_spell(ch, SPELL_CURSE));
    assert(affected_by_spell(ch, SPELL_PLAGUE));
    assert(affected_by_spell(ch, SPELL_BLESS));
    assert(ch->points.hit == 41 && ch->points.max_hit == 100);
}
'''

DRIVER = r'''
int main() {
    char_data caster{}, ally{}, remote{}, stranger{}, dead{}, pet{};
    P_char characters[] = {&caster, &ally, &remote, &stranger, &dead, &pet};
    for (auto *ch : characters) initialize(ch);
    remote.in_room = 6;
    dead.specials.position = STAT_DEAD;

    // Warding affects only the selected living target in the room.
    add(&ally, SPELL_BLESS);
    spell_divine_warding(30, &caster, nullptr, 0, &ally, nullptr);
    const int protections[] = {SPELL_PROTECT_FROM_FIRE, SPELL_PROTECT_FROM_COLD,
        SPELL_PROTECT_FROM_ACID, SPELL_PROTECT_FROM_GAS, SPELL_PROTECT_FROM_LIGHTNING,
        SPELL_PROTECT_FROM_GOOD, SPELL_PROTECT_FROM_EVIL};
    assert(count(&ally) == 8 && count(&caster) == 0 && count(&stranger) == 0);
    for (int type : protections) assert(affected_by_spell(&ally, type));
    spell_divine_warding(56, &caster, nullptr, 0, &ally, nullptr);
    assert(count(&ally) == 8);
    for (auto *af = ally.affected; af; af = af->next)
        assert(af->duration == (af->type == SPELL_BLESS ? 7 : 56));
    affect_from_char(&ally, SPELL_PROTECT_FROM_FIRE);
    assert(count(&ally) == 7 && affected_by_spell(&ally, SPELL_PROTECT_FROM_COLD));
    spell_divine_warding(56, &caster, nullptr, 0, &ally, nullptr);
    assert(count(&ally) == 8 && affected_by_spell(&ally, SPELL_PROTECT_FROM_FIRE));
    spell_divine_warding(56, &caster, nullptr, 0, &caster, nullptr);
    assert(count(&caster) == 7);
    spell_divine_warding(56, &caster, nullptr, 0, &remote, nullptr);
    spell_divine_warding(56, &caster, nullptr, 0, &dead, nullptr);
    spell_divine_warding(56, nullptr, nullptr, 0, &stranger, nullptr);
    spell_divine_warding(56, &dead, nullptr, 0, &stranger, nullptr);
    spell_divine_warding(56, &caster, nullptr, 0, nullptr, nullptr);
    assert(count(&remote) == 0 && count(&dead) == 0 && count(&stranger) == 0);
    for (auto *ch : characters) clear(ch);
    caster.in_room = stranger.in_room = NOWHERE;
    spell_divine_warding(56, &caster, nullptr, 0, &stranger, nullptr);
    assert(count(&stranger) == 0);
    caster.in_room = stranger.in_room = 5;

    // Solo purification works, preserves unrelated effects and never heals.
    ailments(&caster);
    spell_mass_purification(56, &caster, nullptr, 0, nullptr, nullptr);
    check_clean(&caster);
    spell_mass_purification(56, &caster, nullptr, 0, nullptr, nullptr);
    check_clean(&caster);
    clear(&caster);

    // A nonleader caster cleans the leader and local members, never bystanders,
    // dead members or members elsewhere. Null entries are harmless.
    for (auto *ch : characters) ailments(ch);
    group_list pet_entry{&pet, nullptr};
    group_list null_entry{nullptr, &pet_entry};
    group_list dead_entry{&dead, &null_entry};
    group_list remote_entry{&remote, &dead_entry};
    group_list caster_entry{&caster, &remote_entry};
    group_list leader_entry{&ally, &caster_entry};
    caster.group = &leader_entry;
    spell_mass_purification(56, &caster, nullptr, 0, nullptr, nullptr);
    check_clean(&caster);
    check_clean(&ally);
    check_clean(&pet);
    assert(count(&remote) == 8 && count(&stranger) == 8 && count(&dead) == 8);
    spell_mass_purification(56, nullptr, nullptr, 0, nullptr, nullptr);
    dead.group = &leader_entry;
    spell_mass_purification(56, &dead, nullptr, 0, nullptr, nullptr);
    assert(count(&dead) == 8);
    for (auto *ch : characters) clear(ch);

    // Both flag-only ailments and spell effects follow the normal cure paths.
    caster.group = nullptr;
    caster.specials.affected_by |= AFF_BLIND;
    caster.specials.affected_by2 |= AFF2_POISONED;
    spell_mass_purification(56, &caster, nullptr, 0, nullptr, nullptr);
    assert(!IS_AFFECTED(&caster, AFF_BLIND) && !IS_AFFECTED2(&caster, AFF2_POISONED));
    ailments(&caster);
    caster.in_room = NOWHERE;
    spell_mass_purification(56, &caster, nullptr, 0, nullptr, nullptr);
    assert(count(&caster) == 8);
    clear(&caster);
    std::puts("Cleric warding targets, refresh, independent effects, and group cleansing passed.");
}
'''

functions = [extract_function("magic.c", "void spell_protection_from_" + name + "(")
             for name in ("fire", "cold", "acid", "gas", "lightning", "good", "evil")]
functions += [extract_function("handler.c", "int poison_common_remove(")]
functions += [extract_function("magic.c", signature) for signature in (
    "void spell_cure_blind(", "void spell_cure_disease(", "void spell_divine_warding(",
    "static void purify_group_member(", "void spell_mass_purification(")]
output = ROOT / "bin/tests/cleric-utilities"
output.mkdir(parents=True, exist_ok=True)
harness = output / "harness.cpp"
harness.write_text(PRELUDE + "\n".join(functions) + DRIVER)
binary = output / "harness"
subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-I" + str(ROOT / "src"),
                str(harness), "-o", str(binary)], check=True)
subprocess.run([str(binary)], check=True)
