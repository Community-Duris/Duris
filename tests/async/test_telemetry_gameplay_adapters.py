#!/usr/bin/env python3
"""Compile and run the focused #265 gameplay-adapter journey."""

from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import struct
import subprocess
import sys
import tempfile

from test_telemetry_combat_hooks import function

ROOT = Path(__file__).resolve().parents[2]


def verify_build_mutation_hooks() -> None:
    handler = (ROOT / "src/world/handler.c").read_text()
    for signature in ("void equip_char(", "P_obj unequip_char("):
        body = function(handler, signature)
        assert body.count("telemetry_runtime_game_battle_build_changed(ch);") == 1
        assert body.index("telemetry_runtime_game_battle_build_changed(ch);") > body.index(
            "SET_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT)")
    affects = (ROOT / "src/magic/affects.c").read_text()
    body = function(affects, "char affect_total(")
    assert body.index("telemetry_runtime_game_battle_build_changed(ch);") > body.index(
        "all_affects(ch, TRUE)")
    assert "telemetry_runtime_game_battle_build_changed" not in function(affects, "void balance_affects(")
    assert "affect_total(ch, TRUE);" in function(affects, "void event_balance_affects(")


def verify_control_mutation_hooks() -> None:
    affects = (ROOT / "src/magic/affects.c").read_text(encoding="utf-8")
    assert "telemetry_control_mutation_scope control_state(ch, mode != FALSE);" in function(
        affects, "void all_affects(")
    for signature in ("struct affected_type *affect_to_char(", "void affect_remove(",
                      "void affect_from_char(", "void affect_join("):
        assert "telemetry_control_mutation_scope control_state(ch);" in function(affects, signature)
    total = function(affects, "char affect_total(")
    assert total.index("all_affects(ch, TRUE)") < total.index("control_state.finish();")
    assert total.index("control_state.finish();") < total.index("die(ch,")
    handler = function((ROOT / "src/world/handler.c").read_text(encoding="utf-8"),
                       "P_obj unequip_char(")
    assert handler.index("telemetry_control_mutation_scope control_state(ch);") < handler.index(
        "all_affects(ch, FALSE)")
    assert handler.index("all_affects(ch, TRUE)") < handler.index("control_state.finish();")
    files = (ROOT / "src/core/files.c").read_text(encoding="utf-8")
    character_save = function(files, "int writeCharacter(")
    assert character_save.count("telemetry_control_mutation_scope control_state(ch);") == 2
    pet_save = function(files, "int writePet(")
    assert pet_save.index("telemetry_control_mutation_scope control_state(ch);") < pet_save.index(
        "save_equip[i] = unequip_char(ch, i)")
    assert pet_save.index("all_affects(ch, TRUE)") < pet_save.index("control_state.finish();")
    wards = (ROOT / "src/combat/spell_wards.c").read_text(encoding="utf-8")
    assert "telemetry_control_mutation_scope control_state(ch);" in function(wards, "void set_ward_bits(")
    position = function((ROOT / "src/combat/fight_state.c").read_text(encoding="utf-8"),
                        "void update_pos(")
    assert position.count("telemetry_control_mutation_scope control_state(ch);") == 2
    assert position.index("control_state.finish();") < position.index("do_wake(ch,")
    ranged = function((ROOT / "src/combat/range.c").read_text(encoding="utf-8"), "void do_fire(")
    assert ranged.index("telemetry_control_mutation_scope control_state(victim);") < ranged.index(
        "REMOVE_BIT(victim->specials.affected_by, AFF_SLEEP)")
    extraction = function((ROOT / "src/world/handler.c").read_text(encoding="utf-8"), "void extract_char(")
    assert extraction.index("telemetry_runtime_game_battle_leave(ch)") < extraction.index("affect_remove(ch, af)")
    staff = (ROOT / "src/cmd/actset.c").read_text(encoding="utf-8")
    assert "telemetry_runtime_game_control_changed(static_cast<P_char>(ptr));" in function(staff, "static void setbit_parseTable(P_char ch,")
    attribute = (ROOT / "src/cmd/staff_setattr.c").read_text(encoding="utf-8")
    assert "telemetry_runtime_game_control_changed(ch);" in function(attribute, "static void sa_intCopy(")


def verify_result_hooks() -> None:
    fight = function((ROOT / "src/combat/fight.c").read_text(encoding="utf-8"), "void die(")
    assert fight.index("training_dummy_is(ch)") < fight.index("telemetry_runtime_game_battle_result_begin(")
    assert fight.index("telemetry_runtime_game_battle_result_begin(") < fight.index("telemetry_runtime_game_encounter_leave(")
    accepted = fight.index("telemetry_battle_result_kind::death_observed")
    assert fight.index("check_outpost_death(ch, killer)") < accepted
    assert fight.index("check_reincarnate(ch)") < accepted < fight.index("kill_gain(killer, ch)")
    assert "telemetry_battle_result_reason::reincarnated" in fight
    actoff = (ROOT / "src/cmd/actoff.c").read_text(encoding="utf-8")
    flee = function(actoff, "void do_flee(")
    assert flee.index("telemetry_runtime_game_battle_result_begin(") < flee.index("do_simple_move(ch, attempted_dir,")
    assert flee.index("start_room == ch->in_room") < flee.index("telemetry_battle_result_kind::flee_movement")
    assert flee.index("if (atts)") < flee.index("telemetry_battle_result_kind::flee_movement")
    retreat = function(actoff, "void do_retreat(")
    assert retreat.index("if (number(1, 100) <= chance)") < retreat.index("telemetry_runtime_game_battle_result_begin(")
    assert retreat.index("telemetry_runtime_game_battle_result_begin(") < retreat.index("do_simple_move(ch, dir, 0)")
    assert retreat.index("do_simple_move(ch, dir, 0)") < retreat.index("telemetry_battle_result_kind::withdrawal")
    assert "find_character_by_runtime_id(withdrawal_evidence.target_runtime_id)" in retreat
    disengage = function(actoff, "void do_disengage(")
    assert disengage.index("if (found && !IS_TRUSTED(ch))") < disengage.index("telemetry_runtime_game_battle_result_begin(")
    assert disengage.index("stop_fighting(ch)") < disengage.index("telemetry_battle_result_kind::withdrawal")
    # Reviewed non-null combat-target assignments must report their accepted edge.
    for path, signature, assignment, capture in (
        ("src/combat/fight_state.c", "void set_fighting(", "GET_OPPONENT(ch) = victim;",
         "telemetry_runtime_game_combat_engage(ch, victim)"),
        ("src/combat/justice.c", "int shout_and_hunt(", "GET_OPPONENT(ch) = GET_MASTER(GET_OPPONENT(ch));",
         "telemetry_runtime_game_combat_engage(ch, GET_OPPONENT(ch))"),
        ("src/classes/paladins.c", "void event_righteous_aura_check(", "GET_OPPONENT(opponent) = ch;",
         "telemetry_runtime_game_combat_engage(opponent, ch)"),
    ):
        body = function((ROOT / path).read_text(encoding="utf-8"), signature)
        assert body.index(assignment) < body.index(capture)
    zone = (ROOT / "src/world/zone_touch_transaction.c").read_text(encoding="utf-8")
    submit = function(zone, "bool zone_touch_transaction_submit(")
    assert submit.index("critical_submit_result_keeps_operation(submitted)") < submit.index("telemetry_runtime_game_zone_objective(")
    completion = function(zone, "void zone_touch_transaction_handle_completions(")
    assert "completion.operation_id, entry.result" in completion
    assert "entry.result.recovered_claim" in completion
    # Outbox delivery runs on its SQL worker; it cannot mutate game-thread telemetry.
    assert "telemetry_runtime_game_zone_objective" not in function(zone, "zone_touch_transaction_outbox_delivery(")


def compile_gameplay(executable: Path, *, sanitize: bool = False, native_sql: bool = False,
                     native_affects: bool = False, optimize: bool = False) -> None:
    # Execute the maintained helper bodies with the game-service seams in the
    # existing harness; the actual runtime/worker/writer remain linked below.
    source = (ROOT / "src/magic/affects.c").read_text()
    wards = (ROOT / "src/combat/spell_wards.c").read_text()
    status_control = (ROOT / "src/magic/spell_status_control.c").read_text()
    control_helpers = executable.parent / "telemetry-control-helpers.cc"
    control_helpers.write_text(
        '#include "core/prototypes.h"\n#include "core/utils.h"\n'
        '#include "magic/spells.h"\n#include "combat/spell_wards.h"\n'
        '#include "world/graph.h"\n#include "telemetry/telemetry_runtime.h"\n'
        + function((ROOT / "src/core/utility.c").read_text(), "int BOUNDED(int a, int b, int c)") + "\n"
        + function((ROOT / "src/core/utility.c").read_text(), "int GET_CLASS(P_char ch, uint m_class)") + "\n"
        + wards[wards.index("enum ward_kind\n"):wards.index("\n\nstruct ward_source")] + "\n"
        + wards[wards.index("const int ward_spells["):wards.index("\n\nconst unsigned int ward_flags")] + "\n"
        + function(wards, "bool is_ward_spell(int spell)") + "\n"
        + function(wards, "bool spell_ward_is_managed(") + "\n"
        + function(wards, "bool spell_ward_is_active(") + "\n"
        + function(source, "bool affected_by_spell(P_char ch, int skill)") + "\n"
        + function(source, "bool blind(P_char ch, P_char victim, int duration)") + "\n"
        + function(source, "void Stun(P_char stunnee, P_char stunner, int duration, bool Fear_Check)") + "\n"
        + "\n".join(function(status_control, "void spell_" + name + "(")
            for name in ("major_paralysis", "minor_paralysis", "slow", "sleep", "silence", "entangle")) + "\n",
        encoding="utf-8",
    )
    command = [
        "g++",
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pthread",
        "-I",
        str(ROOT / "src"),
        str(ROOT / "tests/async/telemetry_gameplay_adapters.cc"),
        str(control_helpers),
        str(ROOT / "src/account/character_identity.c"),
        *[
            str(ROOT / "src/telemetry" / name)
            for name in (
                "telemetry_activity.c",
                "telemetry_battle.c",
                "telemetry_battle_contribution.c",
                "telemetry_battle_build_observation.c",
                "telemetry_control.c",
                "telemetry_battle_contract.c",
                "telemetry_combat_summary.c",
                "telemetry_config.c",
                "telemetry_encounter.c",
                "telemetry_failure.c",
                "telemetry_health.c",
                "telemetry_outage.c",
                "telemetry_queue.c",
                "telemetry_progression.c",
                "telemetry_repository.c",
                "telemetry_runtime.c",
                "telemetry_session.c",
                "telemetry_transport.c",
            )
        ],
        "-lcrypto",
        "-o",
        str(executable),
    ]
    if native_affects:
        fight = (ROOT / "src/combat/fight.c").read_text(encoding="utf-8")
        release_at = fight.index("if (GET_STAT(victim) == STAT_SLEEPING && new_stat != STAT_DEAD)")
        release = fight[fight.rfind("\n\t\t{", 0, release_at):fight.index("/* make mirror images disappear */", release_at)]
        native_helpers = executable.parent / "telemetry-native-affects.cc"
        native_helpers.write_text(
            '#include "core/prototypes.h"\n#include "core/utils.h"\n#include "core/mm.h"\n'
            '#include "core/profile.h"\n#include "magic/spells.h"\n#include "net/gmcp.h"\n'
            '#include "combat/spell_wards.h"\n#include "combat/racewar_stat_mods.h"\n'
            '#include "classes/paladins.h"\n#include "classes/epic_skills.h"\n'
            '#include "classes/reavers.h"\n#include "item/objmisc.h"\n'
            '#include "cmd/interp.h"\n'
            '#include "kingdom/kingdom_store_piece.h"\n'
            '#include "world/events.h"\n#include "world/rested.h"\n'
            '#include "telemetry/telemetry_runtime.h"\n#include "world/db.h"\n'
            '#include "world/character_maintenance.h"\n'
            'extern P_char character_list;\nextern P_room world;\nextern int top_of_world;\n'
            'extern const struct stat_data stat_factor[];\n'
            'extern float combat_by_race[LAST_RACE + 1][3];\n'
            'extern float combat_by_class[CLASS_COUNT + 1][2];\n'
            'extern const struct race_names race_names_table[];\n'
            'extern float pulse_all, shield_combat_mult, shield_combat_tank_mult;\n'
            'extern int damroll_cap, hitroll_cap;\nextern unsigned long long ne_event_tick;\n'
            'extern Skill skills[];\nbool innate_two_daggers(P_char);\n'
            'int calculate_hitpoints2(P_char);\nint calculate_mana(P_char);\n'
            'void unlink_char_affect(P_char, struct affected_type *);\n'
            'void unlink_char_obj_affect(P_char, struct affected_type *);\n'
            'void get_epic_stat_affects(P_char);\nvoid get_aura_affects(P_char);\n'
            'struct mm_ds *dead_affect_pool = nullptr;\nstruct hold_data TmpAffs{};\n'
            + function((ROOT / "src/core/utility.c").read_text(encoding="utf-8"),
                       "int flag2idx(int flag)") + "\n"
            + function(wards, "bool spell_ward_is_equipment(") + "\n"
            + function(wards, "void set_ward_bits(") + "\n"
            + "\n".join(function(source, signature) for signature in (
                "int apply_ac(", "void add_racial_stat_bonus(", "void apply_affs(",
                "void affect_modify(", "void all_affects(", "char affect_total(",
                "void event_balance_affects(", "void balance_affects(",
                "void event_short_affect(", "struct affected_type *affect_to_char(",
                "void affect_remove(", "void affect_from_char(", "void affect_join(",
                "bool rested_bonus_effect_active(", "void wear_off_message(")) + "\n"
            + function((ROOT / "src/magic/spell_healing.c").read_text(encoding="utf-8"),
                       "void spell_cure_blind(") + "\n"
            + function((ROOT / "src/classes/bard.c").read_text(encoding="utf-8"),
                       "void song_broken(") + "\n"
            + "\n".join(function((ROOT / "src/combat/fight_state.c").read_text(encoding="utf-8"),
                                  signature) for signature in
                          ("unsigned int calculate_ch_state(", "void update_pos(")) + "\n"
            + "void fixture_damage_control_release(P_char ch, P_char victim, int dam, int new_stat) {\n"
            + "affected_type *af, *next_af;\n" + release + "\n}\n",
            encoding="utf-8",
        )
        command.extend(["-DTELEMETRY_TEST_NATIVE_AFFECTS", str(native_helpers)])
        staff = (ROOT / "src/cmd/actset.c").read_text(encoding="utf-8")
        staff_helpers = executable.parent / "telemetry-native-staff.cc"
        staff_helpers.write_text(
            '#include "core/prototypes.h"\n#include "core/utils.h"\n#include "core/safe_format.h"\n'
            '#include "telemetry/telemetry_runtime.h"\n#include <cstddef>\n#include <cstdlib>\n#include <cstring>\n#include <cctype>\n'
            '#define SETBIT_CHAR 1\n#define SAME_STRING(A,B) ac_strcasecmp(A,B)\n'
            '#define LOWER_CASE(C) (isupper(C) ? tolower(C) : (C))\n'
            + staff[staff.index("struct setBitTable\n"):staff.index("/* Private Interface */")] + "\n"
            + 'char bad_on_off[MAX_INPUT_LENGTH]{};\n'
            + 'static void setbit_syntax(P_char, int) {}\n'
            + 'static void setbit_printOutTable(P_char, SetBitTable *, int) {}\n'
            + 'static void setbit_printOutSubTable(P_char, const char **, int) {}\n'
            + 'static void ac_tongueCopy(void *, int, char *, int, int) { std::abort(); }\n'
            + 'static void ac_skillCopy(void *, int, char *, int, int) { std::abort(); }\n'
            + function(staff, "static int ac_strcasecmp(const char *str1,") + "\n"
            + function(staff, "static void ac_bitCopy(void *where,") + "\n"
            + function((ROOT / "src/cmd/interp.c").read_text(encoding="utf-8"), "bool is_number(") + "\n"
            + function(staff, "static void setbit_parseTable(P_char ch,") + "\n"
            + function((ROOT / "src/cmd/staff_setattr.c").read_text(encoding="utf-8"), "static void sa_intCopy(") + "\n"
            + 'void fixture_staff_control_bit(P_char target, bool second, unsigned bit, bool enabled, bool character) {\n'
            + 'SetBitTable table[] = {{"aff", offsetof(char_data, specials.affected_by), nullptr, ac_bitCopy},'
            + '{"aff2", offsetof(char_data, specials.affected_by2), nullptr, ac_bitCopy}};\n'
            + 'char first_flag[]="aff", second_flag[]="aff2", value[32]; std::snprintf(value,sizeof(value),"%u",bit);\n'
            + 'setbit_parseTable(target,target,table,2,second?second_flag:first_flag,value,enabled,character?SETBIT_CHAR:2);\n}\n'
            + 'void fixture_staff_control_bank(P_char target, int value) { sa_intCopy(target,offsetof(char_data,specials.affected_by),value); }\n',
            encoding="utf-8")
        command.extend([str(staff_helpers), str(ROOT / "src/core/safe_format.c")])
    if native_sql:
        command.extend(["-DTELEMETRY_TEST_NATIVE_BATTLE_SQL", "-DTELEMETRY_TEST_STUB_REPOSITORY",
                        str(ROOT / "tests/async/telemetry_gameplay_sql.cc")])
        command.extend(shlex.split(subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)))
    else:
        command.append("-D__NO_MYSQL__")
    if sanitize:
        command.extend(["-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"])
    if optimize:
        command.append("-O2")
    subprocess.run(command, cwd=ROOT, check=True, timeout=120)


def verify_native_build_context(executable: Path) -> None:
    completed = subprocess.run(
        [str(executable), "--native-build-context"], cwd=ROOT, check=False,
        text=True, capture_output=True, timeout=30,
    )
    print(completed.stdout, end="")
    print(completed.stderr, end="")
    completed.check_returncode()
    exported = [line.removeprefix("BUILD_CONTEXT_JSON ") for line in completed.stdout.splitlines()
                if line.startswith("BUILD_CONTEXT_JSON ")]
    assert len(exported) == 1
    context = json.loads(exported[0])
    assert context["snapshot_bytes"] <= 448 and context["crypto_heap_calls"] == 0
    assert context["content_version"] == 11

    # Independent network-order reference for the reviewed fixed features.
    # The three fixture items occupy slots 0, 1 and the final maintained slot.
    objects = {
        0: (5, 4, 97, -7, (0, 2, 6, 0, 0, 0, 0, 0), (0x1234, 0, 0, 0, 0),
            ((17, -5), (18, 7), (19, 11), (13, 13)), 0),
        1: (37, 0, 0, 0, (0, 0, 0, 25, 0, 0, 0, 0), (0, 0x55, 0, 0, 0),
            ((12, -2), (0, 0), (0, 0), (0, 0)), 1),
        42: (9, 0, 0, 0, (0,) * 8, (0, 0, 0, 0, 0x10000000),
             ((18, -3), (0, 0), (0, 0), (0, 0)), 0),
    }
    equipment = bytearray(struct.pack(">HIH", 1, 11, 43))
    for slot in range(43):
        equipment.extend(struct.pack(">BB", slot, int(slot in objects)))
        if slot not in objects:
            continue
        item_type, material, condition, craftsmanship, values, flags, affects, dynamic = objects[slot]
        equipment.extend(struct.pack(">BBhh", item_type, material, condition, craftsmanship))
        equipment.extend(struct.pack(">8i", *values))
        equipment.extend(struct.pack(">5Q", *flags))
        equipment.extend(struct.pack(">5I", 0, 0, 0, 0, 0))
        for location, modifier in affects:
            equipment.extend(struct.pack(">Bb", location, modifier))
        equipment.extend(struct.pack(">B", dynamic))
    assert context["equipment_digest"] == hashlib.sha256(equipment).hexdigest()
    epics = struct.pack(">HIHH", 1, 11, 1000, 1308)
    epics += struct.pack(">HBHB", 1230, 50, 1253, 0)
    assert context["epic_digest"] == hashlib.sha256(epics).hexdigest()
    print("native equipment and learned-epic SHA-256 canonical references passed")


def verify_native_result_capture(executable: Path) -> None:
    sys.path.insert(0, str(ROOT / "scripts/telemetry"))
    import battle_result_contract as results
    exported = executable.parent / "native-result-adapters.jsonl"
    subprocess.run([str(executable), "--native-result-capture"], cwd=ROOT,
        env=dict(os.environ, TELEMETRY_RESULT_CAPTURE_EXPORT=str(exported)), check=True, timeout=30)
    rows = [json.loads(line) for line in exported.read_text(encoding="utf-8").splitlines()]
    assert len(rows) == 18
    values = {}
    for row in rows:
        row["bout_operation_id"] = bytes.fromhex(row["bout_operation_id"])
        results.validate_raw_observation(row)
        key = results.observation_key(row)
        assert key not in values
        values[key] = row
    for key, row in values.items():
        if row["bout_kind"] in (4, 8):
            parent = values[(*key[:2], row["bout_parent_sequence"])]
            assert parent["bout_kind"] == 2 or (parent["bout_kind"] == 3 and parent["bout_authority"] == 3)
            assert row["bout_start_usec"] == parent["bout_at_usec"]
            assert row["bout_target_actor_id"] == parent["bout_target_actor_id"]
            assert row["bout_target_battle_seq"] == parent["bout_target_battle_seq"]
    objectives = [row for row in rows if row["bout_authority"] == 7]
    assert len(objectives) == 3
    assert all(row["bout_operation_id"] == bytes(range(1, 17)) for row in objectives)
    assert all(row["bout_source_object_uid"] == 810101 for row in objectives)
    assert all(row["bout_target_session_seq"] == row["bout_target_battle_seq"] == 0 for row in objectives)
    print("native result adapters: 18 exact independently validated rows; synthetic pulse-clock fixture")
    subprocess.run([str(executable), "--native-result-budget-fixture"], cwd=ROOT, check=True, timeout=30)


def result_performance(output: Path) -> None:
    """Measure native callbacks and the actual bounded pulse with live watches."""
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="result-performance-", dir=ROOT / "bin/tests") as directory:
        executable = Path(directory) / "native-results"
        compile_gameplay(executable, optimize=True)
        profiles = []
        for repetition in range(5):
            completed = subprocess.check_output([str(executable), "--native-result-performance"], cwd=ROOT,
                text=True, timeout=90)
            profiles.extend(dict(json.loads(line), repetition=repetition) for line in completed.splitlines())
        assert len(profiles) == 30
        for profile in profiles:
            assert profile["event_heap_calls"] == profile["event_crypto_heap_calls"] == 0
            assert profile["samples"] == 4096 and profile["world_nodes"] == 4096
            assert profile["p99_ns"] <= 1_000_000 and profile["p999_ns"] <= 5_000_000
            assert profile["watched_players"] in (50, 200, 256)
        report = dict(status="passed", profiles=profiles, compiler=subprocess.check_output(
            ["g++", "--version"], text=True).splitlines()[0], compile_optimization="-O2",
            measurement="native begin/finish or full game-thread pulse; fixture world and private fake writer; SQL and Telnet excluded",
            watched_player_workloads=[50, 200, 256], session_capacity=256, fixed_watch_capacity=512,
            fixed_watch_state_upper_bytes=256 * 1024, world_node_limit=4096,
            combined_selection_limit=16, build_read_limit_per_second=16,
            capture_p99_budget_ns=1_000_000, capture_p999_budget_ns=5_000_000,
            event_heap_calls=0, running_server=False, positive_gameplay_qualification=False,
            synthetic_escape_clock_fixture=True, session_capacity_refusal=True,
            bounded_pending_selection=True, world_capacity_censoring=True, lifetime_censoring=True)
        output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({key: report[key] for key in ("status", "watched_player_workloads", "fixed_watch_capacity", "event_heap_calls")}))


def main(*, sanitize: bool = False, native_affects: bool = False) -> None:
    verify_build_mutation_hooks()
    verify_control_mutation_hooks()
    verify_result_hooks()
    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-gameplay-", dir=artifacts) as directory:
        executable = Path(directory) / "telemetry-gameplay-adapters"
        compile_gameplay(executable, sanitize=sanitize, native_affects=native_affects)
        if native_affects:
            subprocess.run([str(executable), "--native-affects"], cwd=ROOT, check=True, timeout=30)
            return
        verify_native_build_context(executable)
        verify_native_result_capture(executable)
        completed = subprocess.run(
            [str(executable)], cwd=ROOT, check=False, text=True, capture_output=True, timeout=30
        )
        print(completed.stdout, end="")
        print(completed.stderr, end="")
        completed.check_returncode()
        assert "telemetry gameplay adapter paths passed" in completed.stdout


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true", help="run AddressSanitizer and UndefinedBehaviorSanitizer")
    parser.add_argument("--native-affects", action="store_true",
                        help="execute maintained affect apply/rebuild/removal/expiry functions")
    parser.add_argument("--result-performance-output", type=Path,
                        help="measure optimized native result capture and pending-watch pulse")
    arguments = parser.parse_args()
    if arguments.result_performance_output:
        result_performance(arguments.result_performance_output)
    else:
        main(sanitize=arguments.sanitize, native_affects=arguments.native_affects)
