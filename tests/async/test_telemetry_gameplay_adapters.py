#!/usr/bin/env python3
"""Compile and run the focused #265 gameplay-adapter journey."""

from pathlib import Path
import argparse
import hashlib
import json
import shlex
import struct
import subprocess
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


def compile_gameplay(executable: Path, *, sanitize: bool = False, native_sql: bool = False,
                     native_affects: bool = False) -> None:
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
                       "void song_broken(") + "\n",
            encoding="utf-8",
        )
        command.extend(["-DTELEMETRY_TEST_NATIVE_AFFECTS", str(native_helpers)])
    if native_sql:
        command.extend(["-DTELEMETRY_TEST_NATIVE_BATTLE_SQL", "-DTELEMETRY_TEST_STUB_REPOSITORY",
                        str(ROOT / "tests/async/telemetry_gameplay_sql.cc")])
        command.extend(shlex.split(subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)))
    else:
        command.append("-D__NO_MYSQL__")
    if sanitize:
        command.extend(["-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"])
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


def main(*, sanitize: bool = False, native_affects: bool = False) -> None:
    verify_build_mutation_hooks()
    verify_control_mutation_hooks()
    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-gameplay-", dir=artifacts) as directory:
        executable = Path(directory) / "telemetry-gameplay-adapters"
        compile_gameplay(executable, sanitize=sanitize, native_affects=native_affects)
        if native_affects:
            subprocess.run([str(executable), "--native-affects"], cwd=ROOT, check=True, timeout=30)
            return
        verify_native_build_context(executable)
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
    arguments = parser.parse_args()
    main(sanitize=arguments.sanitize, native_affects=arguments.native_affects)
