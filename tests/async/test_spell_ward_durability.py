#!/usr/bin/env python3
"""Execute production wards, Dispel Magic target policies, and snapshot codecs."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

with tempfile.TemporaryDirectory(prefix="duris-wards-") as directory:
    native = Path(directory) / "wards"
    spirit = Path(directory) / "spirit.cpp"
    spirit.write_text('\n'.join([
        '#include "combat/spell_wards.h"', '#include "core/prototypes.h"',
        '#include "core/utils.h"', '#include "net/comm.h"', '#include "magic/spells.h"',
        '#include "core/utility.h"',
        '#include "cmd/interp.h"',
        '#include "classes/disguise.h"', '#include "world/falling.h"',
        '#include "world/events.h"',
        '#include "telemetry/telemetry_runtime.h"', '#include <limits>',
        # Exercise the maintained mutation scope with capture isolated from
        # this ward/spell/codec fixture. Enabled capture is qualified separately
        # by test_telemetry_gameplay_adapters.py --native-affects.
        'void telemetry_runtime_game_control_changed(P_char) noexcept {}',
        extract_function("telemetry/telemetry_runtime.c", "std::uint16_t telemetry_runtime_game_control_mask("),
        extract_function("telemetry/telemetry_runtime.c", "telemetry_control_mutation_scope::telemetry_control_mutation_scope("),
        extract_function("telemetry/telemetry_runtime.c", "telemetry_control_mutation_scope::~telemetry_control_mutation_scope("),
        extract_function("telemetry/telemetry_runtime.c", "void telemetry_control_mutation_scope::finish("),
        'extern P_char character_list;', 'extern const racial_data_type racial_data[];',
        'extern P_index obj_index;',
        '#include <strings.h>', 'void do_point(P_char, P_char);',
        extract_function("magic/smagic.c", "void spell_spirit_ward("),
        extract_function("magic/smagic.c", "void spell_greater_spirit_ward("),
        extract_function("magic/affects.c", "void affect_update("),
        extract_function("magic/affects.c", "void affect_from_char("),
        extract_function("magic/affects.c", "struct affected_type *get_spell_from_char("),
        'int portal_general_internal(P_obj, P_char, int, char *, portal_action_messages *);',
        extract_function("specs/specs.heavens.c", "int portal_door("),
        extract_function("specs/specs.heavens.c", "int portal_wormhole("),
        extract_function("specs/specs.heavens.c", "int portal_etherportal("),
        extract_function("specs/specs.heavens.c", "int moonstone("),
        extract_function("magic/spell_portals.c", "struct portal_data\n") + ';',
        extract_function("magic/spell_portals.c", "static void event_portal_owner_check("),
        'void run_portal_owner_check(P_char owner, P_obj obj, bool oneway) {',
        'portal_data data{GET_PID(owner), oneway};',
        'event_portal_owner_check(nullptr, nullptr, obj, &data);', '}',
    ]))
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "tests/async/spell_ward_durability_harness.cpp", "src/combat/spell_wards.c",
        "src/magic/spell_globes.c", "src/magic/spell_dispel_magic.c", str(spirit),
        "src/player/player_snapshot_codec.c", "-o", str(native),
    ], cwd=ROOT, check=True)
    subprocess.run([str(native)], check=True)

fight = extract_function("combat/fight.c", "int spell_damage(")
ward = fight.index("spell_ward_absorb(ch, victim, dam, flags)")
assert fight.index("check_damage_ward") < ward
assert "dam = ward.remaining" in fight[ward:]
assert "IS_NPC(victim)" in fight[ward:]

# Object callbacks retain travel and normal decay cleanup. The spell owns
# custom dispel policy, rather than a second CMD_DISPEL implementation.
for signature in ("int portal_general_internal(", "int portal_door(",
                  "int portal_wormhole(", "int portal_etherportal(", "int moonstone("):
    assert "CMD_DISPEL" not in extract_function("specs/specs.heavens.c", signature)
