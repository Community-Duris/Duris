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


def compile_gameplay(executable: Path, *, sanitize: bool = False, native_sql: bool = False) -> None:
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


def main(*, sanitize: bool = False) -> None:
    verify_build_mutation_hooks()
    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-gameplay-", dir=artifacts) as directory:
        executable = Path(directory) / "telemetry-gameplay-adapters"
        compile_gameplay(executable, sanitize=sanitize)
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
    main(sanitize=parser.parse_args().sanitize)
