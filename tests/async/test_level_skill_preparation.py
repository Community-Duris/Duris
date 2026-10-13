#!/usr/bin/env python3
"""Original-body differential skill preparation with complete policy controls.

Uses actual native types, frozen original SKILL_DATA_ALL/GET_LVL_FOR_SKILL,
unchanged original flag2idx, actual shared selectors and native wrappers, and
the exact frozen original update_skills body. The full matrix is initialized
with controlled nonzero/zero/boundary class/spec values, not live server boot
evidence. No original configuration/source lifetime or native AFTER is proved.
"""

from _paths import extract_function
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
original = (ROOT / "tests/async/level_skill_original_update.inc").read_text()
original = original.replace("GET_LVL_FOR_SKILL(", "original_GET_LVL_FOR_SKILL(")
original = original.replace("SKILL_DATA_ALL(", "original_SKILL_DATA_ALL(")
original_getters = (ROOT / "tests/async/skill_selector_original_getters.inc").read_text()
selector = (ROOT / "src/player/skill_selection_preparation.c").read_text()
utility = "\n".join(extract_function("utility.c", signature) for signature in (
    "int flag2idx(", "ClassSkillInfo SKILL_DATA_ALL(", "int GET_LVL_FOR_SKILL(",
))
actual = "\n".join(extract_function("level_skill_preparation.c", signature) for signature in (
    "bool player_level_skill_observe_policy(",
    "bool player_level_skill_prepare_policy_values(",
    "void player_level_skill_apply_regular(", "void player_level_skill_apply_song_with_reader(",
    "void player_level_skill_apply_song(",
    "bool player_level_skill_prepare(",
))
native = extract_function("guild.c", "void update_skills(")

HARNESS = r'''
#include "player/level_skill_preparation.h"
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "combat/chaos_config.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>
#include <iterator>
#include <vector>
#include <string>
#include <cstdio>
#include <unistd.h>

Skill skills[MAX_SKILLS];
bool chaos = false, starter_epics = false;
float primary_modifier = 100.f, secondary_modifier = 95.f;
std::vector<std::string> property_reads;
bool chaos_mud_enabled() { return chaos; }
bool chaos_starter_epic_skills_enabled() { return starter_epics; }
float get_property(const char *name, double fallback) {
    property_reads.emplace_back(name);
    if (std::strcmp(name, "skill.cap.multi.mod.primarySkill") == 0) return primary_modifier;
    if (std::strcmp(name, "skill.cap.multi.mod.secondarySkill") == 0) return secondary_modifier;
    return static_cast<float>(fallback);
}
'''

CASES = r'''
bool equal_class_policy(const ClassSkillInfo &a, const ClassSkillInfo &b) {
    if (a.costmult != b.costmult) return false;
    for (int spec = 0; spec <= MAX_SPEC; ++spec)
        if (a.rlevel[spec] != b.rlevel[spec] || a.maxlearn[spec] != b.maxlearn[spec]) return false;
    return true;
}
void compare_selector(const char_data &input_actor, int skill) {
    char_data old_actor = input_actor, new_actor = input_actor;
    property_reads.clear();
    const auto old_policy = original_SKILL_DATA_ALL(&old_actor, skill);
    const auto original_reads = property_reads;
    property_reads.clear();
    const auto new_policy = SKILL_DATA_ALL(&new_actor, skill);
    assert(equal_class_policy(old_policy, new_policy));
    assert(property_reads == original_reads);
    assert(original_reads == (std::vector<std::string>{"skill.cap.multi.mod.primarySkill", "skill.cap.multi.mod.secondarySkill"}));
    property_reads.clear();
    const int old_level = original_GET_LVL_FOR_SKILL(&old_actor, skill);
    const auto required_reads = property_reads;
    property_reads.clear();
    assert(GET_LVL_FOR_SKILL(&new_actor, skill) == old_level);
    assert(property_reads == required_reads);
    assert(old_actor.player.m_class == input_actor.player.m_class && new_actor.player.m_class == input_actor.player.m_class);
    const player_skill_selection_input input{input_actor.player.m_class, input_actor.player.secondary_class,
                                            input_actor.player.spec, IS_NPC(&input_actor) != 0, IS_MULTICLASS_NPC(&input_actor)};
    ClassSkillInfo prepared;
    int required = -777;
    property_reads.clear();
    assert(player_skill_select_prepared_values(input, skills[skill].m_class,
                                               {primary_modifier, secondary_modifier}, &prepared));
    assert(player_skill_required_level_prepared_values(input, skills[skill].m_class,
                                                       {primary_modifier, secondary_modifier}, &required));
    assert(equal_class_policy(old_policy, prepared) && required == old_level);
    assert(property_reads.empty());
}
std::string capture_invalid_diagnostic(bool original, char_data &actor, int skill) {
    FILE *capture = std::tmpfile();
    assert(capture);
    std::fflush(stdout);
    const int saved = dup(fileno(stdout));
    assert(saved >= 0 && dup2(fileno(capture), fileno(stdout)) >= 0);
    const auto policy = original ? original_SKILL_DATA_ALL(&actor, skill) : SKILL_DATA_ALL(&actor, skill);
    assert(equal_class_policy(policy, skills[skill].m_class[0]));
    std::fflush(stdout);
    assert(dup2(saved, fileno(stdout)) >= 0);
    close(saved);
    std::rewind(capture);
    char buffer[256]{};
    const auto count = std::fread(buffer, 1, sizeof(buffer), capture);
    std::fclose(capture);
    return std::string(buffer, count);
}
void check_selectors() {
    char name[] = "skill-selector-control";
    char_data actor{};
    actor.player.name = name;
    actor.player.short_descr = name;
    for (int mode = 0; mode < 4; ++mode)
        for (int primary = 0; primary < CLASS_COUNT; ++primary)
            for (int spec = 0; spec <= MAX_SPEC; ++spec)
                for (int secondary = -1; secondary < CLASS_COUNT; ++secondary) {
                    actor.specials.act = mode >= 2 ? ACT_ISNPC : 0;
                    actor.specials.affected_by4 = mode == 3 ? AFF4_MULTI_CLASS : 0;
                    actor.player.m_class = 1U << primary;
                    if (mode == 3) actor.player.m_class |= 1U << ((primary + 7) % CLASS_COUNT);
                    actor.player.secondary_class = mode == 1 && secondary >= 0 ? 1U << secondary : 0;
                    actor.player.spec = spec;
                    for (const int skill : {FIRST_SKILL, FIRST_INSTRUMENT, LAST_SONG, MAX_SKILLS - 1})
                        compare_selector(actor, skill);
                }
    // Genuine original highest-order PC class selection and secondary BIT_32 absence.
    actor.specials.act = actor.specials.affected_by4 = 0;
    actor.player.m_class = (1U << 2) | (1U << 5);
    actor.player.secondary_class = BIT_32;
    compare_selector(actor, FIRST_SKILL);
    actor.player.secondary_class = 1U << 7;
    compare_selector(actor, FIRST_SKILL);
    // All complete rows remain initialized. Equal caps retain the earliest
    // flagged class; nonpositive caps preserve the native class-zero default.
    std::array<ClassSkillInfo, CLASS_COUNT> saved;
    std::copy(std::begin(skills[FIRST_SKILL].m_class), std::end(skills[FIRST_SKILL].m_class), saved.begin());
    // All original PC branches, signed-byte narrowing and actual float
    // modifiers are compared directly with the complete frozen getter body.
    actor.player.m_class = 1U << 2;
    actor.player.secondary_class = 1U << 5;
    for (int spec = 0; spec <= MAX_SPEC; ++spec)
        for (const int primary_level : {-1, 0, 1, 127})
            for (const int secondary_level : {-1, 0, 1, 127})
                for (const auto modifiers : {player_skill_selection_modifiers{100.f, 95.f},
                                             player_skill_selection_modifiers{1.5f, 113.25f},
                                             player_skill_selection_modifiers{-25.f, 0.f},
                                             player_skill_selection_modifiers{321.125f, 2300.f}}) {
                    actor.player.spec = spec;
                    auto &primary = skills[FIRST_SKILL].m_class[2];
                    auto &secondary = skills[FIRST_SKILL].m_class[5];
                    primary.rlevel[spec] = primary_level; secondary.rlevel[spec] = secondary_level;
                    primary.maxlearn[spec] = -128; secondary.maxlearn[spec] = 127;
                    primary_modifier = modifiers.primary; secondary_modifier = modifiers.secondary;
                    compare_selector(actor, FIRST_SKILL);
                }
    primary_modifier = 100.f; secondary_modifier = 95.f;
    std::copy(saved.begin(), saved.end(), std::begin(skills[FIRST_SKILL].m_class));
    for (int cls = 0; cls < CLASS_COUNT; ++cls)
        skills[FIRST_SKILL].m_class[cls].maxlearn[0] = 70;
    actor.specials.act = ACT_ISNPC;
    actor.specials.affected_by4 = AFF4_MULTI_CLASS;
    actor.player.m_class = (1U << 2) | (1U << 5);
    actor.player.spec = MAX_SPEC;
    compare_selector(actor, FIRST_SKILL);
    assert(equal_class_policy(SKILL_DATA_ALL(&actor, FIRST_SKILL), skills[FIRST_SKILL].m_class[2]));
    for (int cls = 0; cls < CLASS_COUNT; ++cls)
        skills[FIRST_SKILL].m_class[cls].maxlearn[0] = 0;
    compare_selector(actor, FIRST_SKILL);
    assert(equal_class_policy(SKILL_DATA_ALL(&actor, FIRST_SKILL), skills[FIRST_SKILL].m_class[0]));
    for (int cls = 0; cls < CLASS_COUNT; ++cls)
        skills[FIRST_SKILL].m_class[cls].maxlearn[0] = -127;
    compare_selector(actor, FIRST_SKILL);
    actor.player.m_class = 0;
    compare_selector(actor, FIRST_SKILL);
    actor.player.m_class = BIT_32;
    compare_selector(actor, FIRST_SKILL);
    std::copy(saved.begin(), saved.end(), std::begin(skills[FIRST_SKILL].m_class));
    // The genuine native diagnostic/fallback mutates the live class exactly as
    // before. Preparation refuses that single-class input without any fallback.
    actor.specials.affected_by4 = 0;
    for (const unsigned int invalid : {0U, BIT_32}) {
        char_data old_actor = actor, new_actor = actor;
        old_actor.player.m_class = new_actor.player.m_class = invalid;
        const auto old_diagnostic = capture_invalid_diagnostic(true, old_actor, FIRST_SKILL);
        const auto new_diagnostic = capture_invalid_diagnostic(false, new_actor, FIRST_SKILL);
        assert(old_diagnostic == new_diagnostic && !old_diagnostic.empty());
        assert(old_actor.player.m_class == 1 && new_actor.player.m_class == 1);
        ClassSkillInfo refused = saved[7];
        const auto sentinel = refused;
        const player_skill_selection_input invalid_input{invalid, 0, 0, true, false};
        assert(!player_skill_select_prepared_values(invalid_input, saved, {}, &refused));
        assert(equal_class_policy(refused, sentinel));
    }
    ClassSkillInfo refused = saved[7];
    const auto sentinel = refused;
    player_skill_selection_input prepared{1U << 2, 1U << 5, 0, false, false};
    for (const int invalid_spec : {-1, MAX_SPEC + 1}) {
        prepared.specialization = invalid_spec;
        assert(!player_skill_select_prepared_values(prepared, saved, {}, &refused));
        assert(equal_class_policy(refused, sentinel));
    }
    prepared.specialization = 0;
    assert(!player_skill_select_prepared_values(prepared, std::span<const ClassSkillInfo>(saved).first(CLASS_COUNT - 1), {}, &refused));
    assert(!player_skill_select_prepared_values(prepared, saved, {}, nullptr));
    prepared.multiclass_npc = true;
    assert(!player_skill_select_prepared_values(prepared, saved, {}, &refused));
    prepared.multiclass_npc = false;
    prepared.primary_classes = 0;
    assert(!player_skill_select_prepared_values(prepared, saved, {}, &refused));
    prepared.primary_classes = 1U << 2;
    prepared.secondary_classes = 1U << CLASS_COUNT;
    assert(!player_skill_select_prepared_values(prepared, saved, {}, &refused));
    prepared.secondary_classes = 1U << 5;
    auto controlled = saved;
    controlled[2].rlevel[0] = controlled[5].rlevel[0] = 1;
    controlled[2].maxlearn[0] = controlled[5].maxlearn[0] = 1;
    for (const float invalid : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), 2147483648.f}) {
        assert(!player_skill_select_prepared_values(prepared, controlled, {invalid, 95.f}, &refused));
        assert(!player_skill_select_prepared_values(prepared, controlled, {100.f, invalid}, &refused));
        assert(equal_class_policy(refused, sentinel));
    }
    ClassSkillInfo boundary;
    assert(player_skill_select_prepared_values(prepared, controlled, {-2147483648.f, 95.f}, &boundary));
    assert(player_skill_select_prepared_values(prepared, controlled, {std::nextafter(2147483648.f, 0.f), 95.f}, &boundary));
    int required = -777;
    prepared.specialization = MAX_SPEC + 1;
    assert(!player_skill_required_level_prepared_values(prepared, controlled, {}, &required) && required == -777);
    assert(!player_skill_required_level_prepared_values(prepared, controlled, {}, nullptr));
    property_reads.clear();
    assert(GET_LVL_FOR_SKILL(nullptr, FIRST_SKILL) == 0);
    assert(original_GET_LVL_FOR_SKILL(nullptr, FIRST_SKILL) == 0 && property_reads.empty());
}
bool equal_skills(const char_skill_data *a, const char_skill_data *b) {
    for (int i = 0; i < MAX_SKILLS; ++i)
        if (a[i].learned != b[i].learned || a[i].taught != b[i].taught) return false;
    return true;
}
int main() {
    // Every class/spec row is initialized, exercising the real policy lookup.
    for (int skill = 0; skill < MAX_SKILLS; ++skill) {
        skills[skill].targets = skill % 7 == 0 ? TAR_EPIC : 0;
        for (int cls = 0; cls < CLASS_COUNT; ++cls) {
            skills[skill].m_class[cls].costmult = cls;
            for (int spec = 0; spec <= MAX_SPEC; ++spec) {
                skills[skill].m_class[cls].rlevel[spec] = (skill + cls + spec) % 9 == 0 ? 0 : 1 + (skill + cls * 3 + spec) % 60;
                skills[skill].m_class[cls].maxlearn[spec] = 1 + (skill * 3 + cls * 5 + spec) % 110;
            }
        }
    }
    check_selectors();
    char name[] = "level-skill-control";
    for (const int level : {1, 5, 20, 26, 35, 50, 56, 64})
        for (int spec = 0; spec <= MAX_SPEC; ++spec)
            for (const bool multi : {false, true})
                for (int mode = 0; mode < 3; ++mode) {
                    chaos = mode != 0; starter_epics = mode == 2;
                    pc_only_data old_pc{}, new_pc{}, observed_pc{};
                    char_data old_actor{}, new_actor{}, observed_actor{};
                    old_actor.only.pc = &old_pc; new_actor.only.pc = &new_pc;
                    observed_actor.only.pc = &observed_pc;
                    for (P_char actor : {&old_actor, &new_actor, &observed_actor}) {
                        actor->player.name = name; actor->player.level = level;
                        actor->player.m_class = 1U << 2;
                        actor->player.secondary_class = multi ? 1U << 5 : 0;
                        actor->player.spec = spec;
                    }
                    std::array<char_skill_data, MAX_SKILLS> before, after;
                    for (int skill = 0; skill < MAX_SKILLS; ++skill) {
                        before[skill].learned = static_cast<::byte>(-20 + skill % 148);
                        before[skill].taught = static_cast<::byte>(-10 + skill % 138);
                        old_pc.skills[skill] = new_pc.skills[skill] = observed_pc.skills[skill] = before[skill];
                    }
                    property_reads.clear();
                    original_update_skills(&old_actor);
                    const auto original_reads = property_reads;
                    property_reads.clear();
                    update_skills(&new_actor);
                    assert(equal_skills(old_pc.skills, new_pc.skills));
                    assert(property_reads == original_reads);
                    player_level_skill_policy policy;
                    assert(player_level_skill_observe_policy(&observed_actor, &policy));
                    assert(policy.primary_class == observed_actor.player.m_class && policy.secondary_class == observed_actor.player.secondary_class && policy.specialization == spec);
                    assert(player_level_skill_prepare(policy, level, before, &after));
                    assert(equal_skills(old_pc.skills, after.data()));
                    assert(equal_skills(observed_pc.skills, before.data()));
                    assert(before[0].learned == -20 && observed_actor.player.level == level);
                }
    // Preserve the original distinct song behavior: taught may be capped below
    // learned; ordinary regular-skill reconciliation must not be substituted.
    player_level_skill_policy policy;
    policy.rows[FIRST_INSTRUMENT] = {1, 10, 10, false};
    std::array<char_skill_data, MAX_SKILLS> before{}, after{};
    before[FIRST_INSTRUMENT] = {100, 100};
    assert(player_level_skill_prepare(policy, 30, before, &after));
    assert(after[FIRST_INSTRUMENT].taught == 10 && after[FIRST_INSTRUMENT].learned == 100);
    after[0] = {77, 78};
    assert(!player_level_skill_prepare(policy, -1, before, &after));
    assert(after[0].learned == 77 && after[0].taught == 78);
    assert(!player_level_skill_prepare(policy, 256, before, &after));
    assert(!player_level_skill_prepare(policy, 1, before, nullptr));
    // Explicit future PC class/spec policy uses the same initialized table,
    // rather than a resolved policy for a different BEFORE class/spec.
    pc_only_data future_pc{};
    char_data future_actor{};
    future_actor.only.pc = &future_pc; future_actor.player.name = name;
    future_actor.player.m_class = 1U << 4; future_actor.player.secondary_class = 1U << 8;
    future_actor.player.spec = 3; future_actor.player.level = 35;
    for (int skill = 0; skill < MAX_SKILLS; ++skill)
        future_pc.skills[skill] = before[skill];
    chaos = starter_epics = false;
    original_update_skills(&future_actor);
    const player_skill_selection_input future{1U << 4, 1U << 8, 3, false, false};
    assert(player_level_skill_prepare_policy_values(future, skills, {100.f, 95.f}, false, false, &policy));
    assert(policy.primary_class == future.primary_classes && policy.secondary_class == future.secondary_classes && policy.specialization == future.specialization);
    assert(player_level_skill_prepare(policy, 35, before, &after));
    assert(equal_skills(future_pc.skills, after.data()));
    policy.primary_class = 123;
    assert(!player_level_skill_prepare_policy_values(future, std::span<const Skill>(skills).first(MAX_SKILLS - 1), {}, false, false, &policy));
    assert(policy.primary_class == 123);
    auto invalid_future = future; invalid_future.npc = true;
    assert(!player_level_skill_prepare_policy_values(invalid_future, skills, {}, false, false, &policy));
    invalid_future = future; invalid_future.specialization = MAX_SPEC + 1;
    assert(!player_level_skill_prepare_policy_values(invalid_future, skills, {}, false, false, &policy));
    assert(policy.primary_class == 123);
    assert(!player_level_skill_prepare_policy_values(future, skills, {}, false, false, nullptr));
    pc_only_data pc{}; char_data actor{};
    actor.only.pc = &pc; actor.player.name = name; actor.player.m_class = 1;
    policy.primary_class = 123;
    actor.player.spec = MAX_SPEC + 1;
    assert(!player_level_skill_observe_policy(&actor, &policy) && policy.primary_class == 123);
    actor.player.spec = 0; actor.player.m_class = 0;
    assert(!player_level_skill_observe_policy(&actor, &policy) && actor.player.m_class == 0);
    actor.player.m_class = 1; actor.player.secondary_class = 1U << 5;
    primary_modifier = std::numeric_limits<float>::infinity();
    assert(!player_level_skill_observe_policy(&actor, &policy) && policy.primary_class == 123);
    assert(!player_level_skill_observe_policy(nullptr, &policy));
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-level-skills-") as directory:
    scratch = Path(directory)
    (scratch / "bin").mkdir()
    source, binary = scratch / "skills.cpp", scratch / "bin/skills"
    source.write_text(HARNESS + selector + utility + actual + native + original_getters + original + CASES)
    sanitizers = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if os.environ.get("DURIS_TEST_SANITIZERS") == "1" else []
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *sanitizers,
                    "-Isrc", str(source), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)

print("[PASS] exact original-body level-skill differential with complete policy controls; native source/AFTER unproven")
