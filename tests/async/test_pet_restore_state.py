#!/usr/bin/env python3
"""Portable pet-state and legacy-checkpoint compatibility regressions."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "player/pet_restore_state.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <iostream>

int main()
{
    pet_restore_state s;
    s.kind = summoned_pet_kind::undead_first;
    s.name = "undead skeleton _owner_";
    s.short_description = "the skeleton of a particular victim";
    s.long_description = "The skeleton of a particular victim stands here.\r\n";
    s.level = 21; s.race = 10; s.sex = 0; s.size = 2; s.alignment = -1000;
    s.base_stats.fill(90);
    s.base_points = {1234, 55, 100, -25, 17, 19, 0};
    s.damage_dice = {4, 8}; s.spell_slots[3] = 7;
    s.intrinsic_affects[2] = UINT64_C(1) << 45;
    s.charm_expires_at = 2000000000; s.death_expires_at = 2000000060;
    std::string encoded;
    assert(pet_restore_state_encode(s, &encoded));
    pet_restore_state decoded;
    assert(pet_restore_state_decode(encoded, &decoded));
    assert(decoded.name == s.name && decoded.base_points == s.base_points);
    assert(decoded.base_stats == s.base_stats && decoded.spell_slots == s.spell_slots);
    assert(decoded.intrinsic_affects == s.intrinsic_affects);
    assert(decoded.charm_expires_at == s.charm_expires_at && decoded.death_expires_at == s.death_expires_at);
    std::string again;
    assert(pet_restore_state_encode(decoded, &again) && again == encoded);
    for (size_t size = 0; size < encoded.size(); ++size)
        assert(!pet_restore_state_decode(encoded.substr(0, size), &decoded));
    auto invalid = encoded; invalid[0] = 'g';
    assert(!pet_restore_state_decode(invalid, &decoded));
    invalid = encoded; invalid[0] = '2'; // unknown payload version
    assert(!pet_restore_state_decode(invalid, &decoded));
    s.charm_expires_at = s.death_expires_at = 0;
    assert(pet_restore_state_encode(s, &again) && pet_restore_state_decode(again, &decoded));
    assert(!decoded.charm_expires_at && !decoded.death_expires_at);
    s.base_points[1] = INT32_MAX;
    assert(!pet_restore_state_encode(s, &again));
    for (uint32_t kind = 1; kind <= 22; ++kind)
        assert(summoned_pet_cost(static_cast<summoned_pet_kind>(kind)) > 0);
    assert(summoned_pet_cost(summoned_pet_kind::dracolich) == 37);
    assert(summoned_pet_cost(summoned_pet_kind::greater_dracolich) == 75);
    assert(summoned_pet_matches_prototype(summoned_pet_kind::titan, 78));
    assert(!summoned_pet_matches_prototype(summoned_pet_kind::titan, 3));
    assert(legacy_summon_prototype(1201) && !legacy_summon_prototype(200));

    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = 42; snapshot.revision = 1; snapshot.components = PLAYER_COMPONENT_PETS;
    snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
    player_pet_snapshot pet = {};
    pet.mob_vnum = 1201; pet.max_hit = pet.hit = 50;
    pet.restore_state = encoded; pet.hold_reason = pet_hold_reason::over_capacity;
    snapshot.pets.push_back(pet);
    std::vector<uint8_t> bytes;
    assert(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok);
    player_snapshot restored;
    assert(player_snapshot_decode(bytes.data(), bytes.size(), &restored) == player_snapshot_codec_result::ok);
    assert(restored.pets[0].restore_state == encoded);
    assert(restored.pets[0].hold_reason == pet_hold_reason::over_capacity);
    snapshot.pets[0].restore_state.clear(); snapshot.pets[0].hold_reason = pet_hold_reason::none;
    assert(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok);
    // v1 ended each pet immediately after its item vector; the trailing shapes,
    // trophies and recipes fields occupy nine bytes in this minimal checkpoint.
    bytes.erase(bytes.end() - 17, bytes.end() - 9);
    bytes[0] = 1;
    assert(player_snapshot_decode(bytes.data(), bytes.size(), &restored) == player_snapshot_codec_result::ok);
    assert(restored.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION);
    assert(restored.pets[0].restore_state.empty());
    assert(restored.pets[0].hold_reason == pet_hold_reason::none);
    std::cout << "pet-state identity/stats/deadlines, malformed input, subtype costs and v1 checkpoint compatibility passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix="duris-pet-state-") as directory:
    source = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    str(source), "src/player/pet_restore_state.c",
                    "src/player/player_snapshot_codec.c", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
