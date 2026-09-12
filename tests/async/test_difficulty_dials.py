#!/usr/bin/env python3
"""Contracts for the server-wide difficulty dials.

Every dial is 1..10 with 5 meaning the game as it was, so the most important property is
that a neutral dial changes nothing: the curve pins 5 to exactly 1.0 and every scaling
helper returns its input untouched at 1.0. The rest pins each dial to its hook.
"""

from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, source


DIFFICULTY = source("world/difficulty.c").read_text()
HEADER = source("world/difficulty.h").read_text()
PROPERTIES = (ROOT / "lib" / "duris.properties").read_text()


def _run_harness(body: str, prefix: str) -> None:
    with tempfile.TemporaryDirectory(prefix=prefix) as directory:
        root = Path(directory)
        source_path = root / "harness.cpp"
        binary = root / "harness"
        source_path.write_text(body)
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-Isrc", str(source_path), "-o", str(binary)],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], cwd=ROOT, check=True)


def _body(name: str, signature: str) -> str:
    """Return a function definition: from its signature to the closing brace in column 0.

    Occurrences that reach a semicolon before an opening brace are declarations and are
    skipped.
    """
    text = source(name).read_text()
    at = text.find(signature)
    while at >= 0:
        brace, semicolon = text.find("{", at), text.find(";", at)
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            return text[at:text.index("\n}\n", brace) + 2]
        at = text.find(signature, at + 1)
    raise AssertionError(f"{name}: no definition for {signature}")


def test_neutral_setting_is_exactly_the_old_game() -> None:
    _run_harness(r'''
#include "world/difficulty_math.h"
#include <climits>
int main() {
    // 5 is 1.0 even if an administrator writes something else into the curve.
    const double hostile[DIFFICULTY_MAX] = {9, 9, 9, 9, 9, 9, 9, 9, 9, 9};
    if (difficulty_curve_multiplier(hostile, DIFFICULTY_NEUTRAL) != 1.0) return 1;
    if (difficulty_curve_multiplier(DIFFICULTY_DEFAULT_CURVE, DIFFICULTY_NEUTRAL) != 1.0) return 2;
    if (difficulty_effective_multiplier(1.0, true) != 1.0) return 3;
    if (difficulty_effective_multiplier(1.0, false) != 1.0) return 4;
    // At 1.0 every helper hands back exactly what it was given.
    const int ints[] = {INT_MIN, -7, 0, 1, 3, 99, 100, 101, 123456789, INT_MAX};
    for (int value : ints) {
        if (difficulty_scale_int(value, 1.0) != value) return 5;
        if (difficulty_scale_percent(value, 1.0) != value) return 6;
    }
    const long longs[] = {-5L, 0L, 2000L, 250000000L, 9000000000000L};
    for (long value : longs)
        if (difficulty_scale_long(value, 1.0) != value) return 7;
    return 0;
}
''', "duris-difficulty-neutral-")


def test_curve_settings_and_direction() -> None:
    _run_harness(r'''
#include "world/difficulty_math.h"
#include <cmath>
int main() {
    // Settings round to the nearest whole number and clamp to 1..10.
    if (difficulty_clamp_setting(0.0) != 1) return 1;
    if (difficulty_clamp_setting(11.0) != 10) return 2;
    if (difficulty_clamp_setting(4.6) != 5) return 3;
    if (difficulty_clamp_setting(std::nan("")) != DIFFICULTY_NEUTRAL) return 4;
    // The default curve's ends.
    if (difficulty_curve_multiplier(DIFFICULTY_DEFAULT_CURVE, 1) != 0.5) return 5;
    if (difficulty_curve_multiplier(DIFFICULTY_DEFAULT_CURVE, 10) != 2.0) return 6;
    // An unusable curve entry falls back to 1.0 rather than zeroing the game.
    const double broken[DIFFICULTY_MAX] = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0};
    if (difficulty_curve_multiplier(broken, 2) != 1.0) return 7;
    // Dials on something players want run the other way: 10 gives them half.
    if (difficulty_effective_multiplier(2.0, true) != 0.5) return 8;
    if (difficulty_effective_multiplier(2.0, false) != 2.0) return 9;
    // Scaling rounds, saturates and keeps percentages inside 1..100.
    if (difficulty_scale_int(7, 1.5) != 11) return 10;
    if (difficulty_scale_int(2000000000, 2.0) != 2147483647) return 11;
    if (difficulty_scale_percent(80, 2.0) != 100) return 12;
    if (difficulty_scale_percent(1, 0.5) != 1) return 13;
    if (difficulty_scale_long(250000000L, 2.0) != 500000000L) return 14;
    return 0;
}
''', "duris-difficulty-curve-")


def _dial_keys() -> list[str]:
    table = DIFFICULTY[DIFFICULTY.index("DIFFICULTY_DIALS[DIFFICULTY_DIAL_COUNT] = {"):]
    table = table[: table.index("};")]
    return re.findall(r'\{ "([a-z.]+)", "', table)


def test_table_enum_and_properties_agree() -> None:
    keys = _dial_keys()
    enum = HEADER[HEADER.index("enum difficulty_dial"):HEADER.index("DIFFICULTY_DIAL_COUNT")]
    members = re.findall(r"\bDIFFICULTY_[A-Z_]+\b", enum)
    assert len(keys) == len(members) == 18, (keys, members)
    for key, member in zip(keys, members):
        assert member == "DIFFICULTY_" + key.upper().replace(".", "_"), (key, member)
    section = PROPERTIES[PROPERTIES.index("[difficulty]"):].splitlines()
    for key in keys:
        assert f"difficulty.dial.{key}=5.000" in section, key
    for setting in (1, 2, 3, 4, 6, 7, 8, 9, 10):
        assert any(line.startswith(f"difficulty.curve.{setting:02d}=") for line in section)
    # 5 has no key: it is always 1.0.
    assert not any(line.startswith("difficulty.curve.05=") for line in section)


def test_dials_load_before_the_experience_table() -> None:
    apply = _body("world/properties.c", "void apply_properties(")
    assert apply.index("update_difficulty_dials();") < apply.index("update_exp_table();")
    table = _body("world/limits.c", "void update_exp_table(")
    assert "new_exp_table[i - 1]" not in table  # the scaled entry is never the fallback
    assert "difficulty_multiplier(DIFFICULTY_EXP_REQUIRED)" in table


def _flat(text: str) -> str:
    return "".join(text.split())


def test_every_dial_reaches_its_hook() -> None:
    """Each dial is applied by its own expression, not merely named near its hook."""
    hooks = [
        ("mob/mobconv.c", "void apply_zone_modifier(",
         "MAX(1, difficulty_scale_int(ch->points.base_hit, hitpoint_dial))"),
        ("mob/mobconv.c", "void convertMob(",
         "difficulty_scale_coins(&GET_COPPER(ch), &GET_SILVER(ch), &GET_GOLD(ch), "
         "&GET_PLATINUM(ch));"),
        ("combat/fight.c", "bool hit(P_char ch, P_char victim, P_obj weapon",
         "dam *= ch->specials.damage_mod; if (difficulty_world_npc(ch)) "
         "dam *= difficulty_multiplier(DIFFICULTY_MOB_MELEE);"),
        ("combat/fight.c", "int chance_to_hit(",
         "return difficulty_scale_percent(hit_chance, "
         "difficulty_multiplier(DIFFICULTY_MOB_ACCURACY));"),
        ("combat/breath_weapons.c", "static float breath_damage_mod(",
         "return static_cast<float>(breath_dam_mod * dial);"),
        ("net/sparser.c", "bool NewSaves(",
         "save = difficulty_scale_int(save, 1.0 / resistance_dial);"),
        ("world/limits.c", "int gain_exp(",
         "if (XP > 0 && !pvp && type != EXP_RESURRECT) "
         "XP *= difficulty_multiplier(DIFFICULTY_EXP_EARNED);"),
        ("world/limits.c", "int gain_exp(",
         "XP *= difficulty_multiplier(DIFFICULTY_DEATH_PENALTY);"),
        ("world/limits.c", "void update_exp_table(",
         "difficulty_scale_long(propVal, difficulty_multiplier(DIFFICULTY_EXP_REQUIRED));"),
        ("item/randomeq.c", "bool check_random_drop(",
         "chance *= static_cast<float>(difficulty_multiplier(DIFFICULTY_LOOT_DROPS));"),
        ("item/randomeq.c", "P_obj create_random_eq_new(",
         "howgood = difficulty_scale_int(howgood, "
         "difficulty_multiplier(DIFFICULTY_LOOT_QUALITY));"),
        ("world/db.c", "void reset_zone(",
         "MAX(1, difficulty_scale_int(zone_table[zone].lifespan, 1.0 / repop_dial));"),
        ("guild/artifact_guild_state.c", "int artifact_feed_seconds(",
         "seconds = difficulty_scale_int(seconds, "
         "difficulty_multiplier(DIFFICULTY_ARTIFACT_FEEDING));"),
        ("world_quest.c", "bool createQuest(",
         "MIN(difficulty_scale_world_quest_kills(number(7, 9)), mob_index[rnum].number - 1);"),
    ]
    for name, signature, expression in hooks:
        assert _flat(expression) in _flat(_body(name, signature)), (name, expression)

    # The bartender fee is scaled after it is priced and before it is taken or refunded.
    bartender = _flat(source("specs.mobile.c").read_text())
    priced = bartender.index(_flat('get_property("world.quest.cost.per.level", 20.000)'))
    scaled = bartender.index(_flat("temp = difficulty_scale_world_quest_fee(temp);"), priced)
    assert scaled < bartender.index(_flat("SUB_MONEY(pl, temp, 0);"), priced)

    # Both backends' daily allowance takes the dial before today's quests are counted off.
    sql = _flat(source("sql/sql.c").read_text())
    assert _flat("maximum = difficulty_scale_world_quest_allowance(maximum); "
                 "return std::max(maximum - completed_today, 0);") in sql
    assert _flat("returning_value = difficulty_scale_world_quest_allowance(returning_value); "
                 "returning_value -= atoi(row[0]);") in sql

    # The spell dial multiplies the final damage, after the profile's 2.0 "more" cap.
    spell = _flat(_body("combat/fight.c", "int spell_damage("))
    cap = spell.index(_flat("BOUNDEDF(0.1, damProf.moreMod, 2.0)"))
    assert cap < spell.index(_flat(
        "const double spell_dial = difficulty_multiplier(DIFFICULTY_MOB_SPELL);"))
    assert cap < spell.index(_flat("dam = MAX(1, dam * spell_dial);"))

    # get_circle_memtime() defeats body extraction, so check the NPC memorisation line and
    # what immediately follows it.
    memorize = _flat(source("classes/memorize.c").read_text())
    npc = memorize.index(_flat('time = time * get_property("memorize.factor.npc", 1.0);'))
    assert _flat("if (recovery_dial != 1.0 && difficulty_world_npc(ch)) "
                 "time = static_cast<float>(time / recovery_dial);") in memorize[npc:npc + 400]

    epic = _flat(source("world/epic.c").read_text())
    assert _flat("if (type != EPIC_PVP && type != EPIC_SHIP_PVP) amount = "
                 "difficulty_scale_int(amount, difficulty_multiplier(DIFFICULTY_EPIC_GAIN));") in epic


def test_mob_dials_skip_player_pets_and_morphs() -> None:
    world_npc = _body("world/difficulty.c", "bool difficulty_world_npc(")
    assert "IS_NPC(ch)" in world_npc and "!IS_PC_PET(ch)" in world_npc and "!IS_MORPH(ch)" in world_npc
    convert = _body("mob/mobconv.c", "void convertMob(")
    assert "apply_mob_gold && difficulty_world_npc(ch)" in convert
    assert convert.index("difficulty_scale_coins(") > convert.index("isname(\"_nomoney_\"")
    for name, signature in (("combat/fight.c", "int chance_to_hit("),
                            ("combat/fight.c", "int spell_damage("),
                            ("net/sparser.c", "bool NewSaves("),
                            ("combat/breath_weapons.c", "static float breath_damage_mod(")):
        assert "difficulty_world_npc(ch)" in _body(name, signature), name
    db = source("world/db.c").read_text()
    assert "read_mobile(int nr, int type, bool apply_mob_gold)" in db
    assert "convertMob(mob, apply_mob_gold);" in db


def test_pet_loaders_do_not_apply_pre_link_gold() -> None:
    checks = {
        "player/player_load_pets.c": "read_mobile(mobile_number, REAL, false)",
        "sql/sql_player.c": "read_mobile(pet_rnum, REAL, false)",
        "classes/innates.c": "read_mobile(DEVIL_IMP, VIRTUAL, false)",
        "specs/specs.mobile.c": "read_mobile(real_mobile(mobnumb), REAL, false)",
        "combat/mobcombat.c": "read_mobile(1006, VIRTUAL, false)",
        "core/files.c": "convertMob(ch, false)",
    }
    for name, expression in checks.items():
        assert expression in source(name).read_text(), (name, expression)


def test_breath_money_regen_and_corpse_hooks_are_complete() -> None:
    breath = source("combat/breath_weapons.c").read_text()
    assert "breath_dam_mod * (" not in breath
    assert breath.count("breath_damage_mod(ch) * (") == 12
    db = source("world/db.c").read_text()
    assert "difficulty_scale_coins(&tmp1, &tmp2, &tmp3, &tmp4);" not in db
    assert "ADD_MONEY(mob, difficulty_scale_money(tmp1));" not in db
    assert db.count("ADD_MONEY(mob, tmp1);") == 2
    # The legacy 20-platinum bonus is decided on the file's value.  The final
    # converted wallet is scaled later, so this predicate never changes with the dial.
    flat_db = _flat(db)
    assert _flat("const bool platinum_bonus = tmp4 > 20;") in flat_db
    assert _flat("if (platinum_bonus)") in flat_db
    assert _flat("if (GET_PLATINUM(mob) > 20)") not in flat_db
    limits = source("world/limits.c").read_text()
    for signature in ("int hit_regen(", "int mana_regen(", "int move_regen("):
        assert "difficulty_scale_player_regen(ch," in _body("world/limits.c", signature)
    earned = _body("world/limits.c", "int gain_exp(")
    assert "if (XP > 0 && !pvp && type != EXP_RESURRECT)" in earned
    for name in ("combat/fight.c", "core/files.c", "magic/smagic.c"):
        assert 'get_property("timer.decay.corpse.pc"' not in source(name).read_text(), name
    assert "difficulty_pc_corpse_decay_minutes() * WAIT_MIN" in source("combat/fight.c").read_text()


def test_command_is_registered() -> None:
    interp = source("cmd/interp.c").read_text()
    assert '"difficulty",\n\t"\\n" /* MAX_CMD = 860, MAX_CMD_LIST = 1000 */' in interp
    assert "CMD_GRT(CMD_DIFFICULTY, STAT_DEAD + POS_PRONE, do_difficulty, LESSER_G);" in interp
    assert "#define CMD_DIFFICULTY 859" in source("cmd/interp.h").read_text()
    # The command-name table is sized by MAX_CMD, which counts its terminating entry.
    headers = "".join(path.read_text() for path in (ROOT / "src").rglob("*.h"))
    assert "#define MAX_CMD 860 " in headers
    assert "void do_difficulty(P_char, char *, int);" in source("core/prototypes.h").read_text()
    command = _body("world/difficulty.c", "void do_difficulty(")
    assert "GET_LEVEL(ch) < FORGER" in command
    assert command.index("GET_LEVEL(ch) < FORGER") < command.index("do_properties(ch, request, 0);")
    assert "set difficulty.dial.* %d" in command


def test_module_runtime() -> None:
    """Run the production module against stubbed properties and output."""
    _run_harness(r"""
#include "core/prototypes.h"
#include "core/utils.h"
#include <cmath>
#include <map>
#include <string>
#include <vector>

static std::map<std::string, double> props;
float get_property(const char *key, double fallback) {
    const auto found = props.find(key);
    return static_cast<float>(found == props.end() ? fallback : found->second);
}
int get_property(const char *key, int fallback) {
    const auto found = props.find(key);
    return found == props.end() ? fallback : static_cast<int>(found->second);
}
P_char get_linked_char(P_char, ush_int) { return nullptr; }
int IS_MORPH(P_char) { return 0; }
static std::vector<std::string> requests;
static std::string shown;
void do_properties(P_char, char *args, int) { requests.push_back(args); }
void send_to_char(const char *text, P_char) { shown += text; }

#include "world/difficulty.c"

static bool close_to(double a, double b) { return std::fabs(a - b) < 1e-6; }
static void run(P_char ch, const char *text) {
    char buffer[128];
    snprintf(buffer, sizeof buffer, "%s", text);
    do_difficulty(ch, buffer, 0);
}

int main() {
    // No properties at all: every dial is exactly neutral.
    update_difficulty_dials();
    for (int dial = 0; dial < DIFFICULTY_DIAL_COUNT; ++dial)
        if (difficulty_multiplier(static_cast<difficulty_dial>(dial)) != 1.0) return 1;

    props["difficulty.dial.mob.melee"] = 7;
    props["difficulty.dial.exp.earned"] = 10;
    props["difficulty.dial.exp.required"] = 1;
    props["difficulty.dial.mob.gold"] = 10;
    props["difficulty.dial.death.penalty"] = 10;
    props["difficulty.dial.player.recovery"] = 10;
    props["difficulty.dial.mob.hitpoints"] = 42;  // out of range: clamps to 10
    props["difficulty.dial.zone.repop"] = 5;
    props["difficulty.curve.05"] = 3.0;           // ignored: 5 is always 1.0
    props["difficulty.curve.07"] = 1.4;           // curve overrides are honoured
    update_difficulty_dials();
    if (!close_to(difficulty_multiplier(DIFFICULTY_MOB_MELEE), 1.4)) return 2;
    if (!close_to(difficulty_multiplier(DIFFICULTY_EXP_EARNED), 0.5)) return 3;
    if (!close_to(difficulty_multiplier(DIFFICULTY_EXP_REQUIRED), 0.5)) return 4;
    if (!close_to(difficulty_multiplier(DIFFICULTY_MOB_HITPOINTS), 2.0)) return 5;
    if (difficulty_multiplier(DIFFICULTY_ZONE_REPOP) != 1.0) return 6;
    if (difficulty_multiplier(DIFFICULTY_MOB_SPELL) != 1.0) return 7;

    // Mob gold at 10 halves every denomination, rounding half away from zero.
    int copper = 10, silver = 3, gold = 1, platinum = 7;
    difficulty_scale_coins(&copper, &silver, &gold, &platinum);
    if (copper != 5 || silver != 2 || gold != 1 || platinum != 4) return 8;
    if (difficulty_scale_money(1001) != 501) return 9;

    // Death penalty at 10 halves the time before a player's corpse decays.
    props["timer.decay.corpse.pc"] = 120;
    if (difficulty_pc_corpse_decay_minutes() != 60) return 10;

    // Player recovery at 10 halves positive regeneration, never below 1, and
    // leaves losses alone.
    char_data player{};
    player.player.level = 50;
    if (difficulty_scale_player_regen(&player, 10) != 5) return 11;
    if (difficulty_scale_player_regen(&player, 1) != 1) return 12;
    if (difficulty_scale_player_regen(&player, -3) != -3) return 13;
    if (difficulty_world_npc(&player)) return 14;

    // The command: anyone who can use it can look; only a Forger can change anything.
    run(&player, "");
    if (shown.find("mob.hitpoints") == std::string::npos) return 15;
    run(&player, "set mob.melee 7");
    if (!requests.empty()) return 16;
    player.player.level = FORGER;
    run(&player, "set mob.melee 7");
    run(&player, "preset 3");
    run(&player, "set mob.melee 11");
    run(&player, "set nonsense 3");
    run(&player, "preset 0");
    run(&player, "save");
    if (requests.size() != 3) return 17;
    if (requests[0] != "set difficulty.dial.mob.melee 7") return 18;
    if (requests[1] != "set difficulty.dial.* 3") return 19;
    if (requests[2] != "save") return 20;

    // Bartender quests at 10: double the fee, half the daily allowance (never below one)
    // and double the kills.
    props["difficulty.dial.world.quest"] = 10;
    update_difficulty_dials();
    if (difficulty_scale_world_quest_fee(1000) != 2000) return 21;
    if (difficulty_scale_world_quest_allowance(8) != 4) return 22;
    if (difficulty_scale_world_quest_allowance(1) != 1) return 23;
    if (difficulty_scale_world_quest_kills(7) != 14) return 24;
    // A steeper curve cannot take the daily allowance below one quest.
    props["difficulty.curve.10"] = 4.0;
    update_difficulty_dials();
    if (difficulty_scale_world_quest_allowance(1) != 1) return 28;
    props.erase("difficulty.curve.10");
    props["difficulty.dial.world.quest"] = 5;
    update_difficulty_dials();
    if (difficulty_scale_world_quest_fee(1000) != 1000) return 25;
    if (difficulty_scale_world_quest_allowance(8) != 8) return 26;
    if (difficulty_scale_world_quest_kills(7) != 7) return 27;
    return 0;
}
""", "duris-difficulty-runtime-")


if __name__ == "__main__":
    tests = [
        test_neutral_setting_is_exactly_the_old_game,
        test_curve_settings_and_direction,
        test_table_enum_and_properties_agree,
        test_dials_load_before_the_experience_table,
        test_every_dial_reaches_its_hook,
        test_mob_dials_skip_player_pets_and_morphs,
        test_pet_loaders_do_not_apply_pre_link_gold,
        test_breath_money_regen_and_corpse_hooks_are_complete,
        test_command_is_registered,
        test_module_runtime,
    ]
    for test in tests:
        test()
    print("difficulty dial contracts passed")
