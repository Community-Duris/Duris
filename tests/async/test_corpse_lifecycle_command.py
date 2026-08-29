#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "corpse_lifecycle_command.h"

#include <cassert>

static critical_operation_id operation(uint8_t seed)
{
	critical_operation_id value = {};
	value.bytes[0] = seed;
	return value;
}

static corpse_lifecycle_payload payload()
{
	corpse_lifecycle_payload value = {};
	value.owner_pid = 42;
	value.save_id = 20;
	value.room_vnum = 500;
	value.weight = 90;
	value.values[3] = 42;
	value.values[5] = 1;
	value.values[6] = 20;
	value.money = { 1, 2, 3, 4 };
	value.owner_name = "Hero";
	value.short_description = "the corpse of Hero";
	value.description = "The corpse of Hero is lying here.";
	value.keywords = "hero corpse _pcorpse_";
	return value;
}

int main()
{
	auto source = payload();
	critical_command command = {};
	assert(corpse_lifecycle_command_build(&command, operation(1), source,
					      critical_source_site::combat,
					      critical_deadline_class::terminal));
	command.accepted_at_usec = 100;
	assert(critical_command_valid(command));
	const auto upsert_command = command;
	corpse_lifecycle_payload decoded = {};
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	assert(decoded.owner_pid == 42 && decoded.save_id == 20 && decoded.money[3] == 4 &&
	       decoded.description == source.description);
	auto truncated = command;
	truncated.payload.pop_back();
	assert(!corpse_lifecycle_command_decode_payload(truncated, &decoded));
	auto wrong_revision = command;
	wrong_revision.expected_revisions[0].revision = 1;
	assert(!corpse_lifecycle_command_decode_payload(wrong_revision, &decoded));
	auto invalid = source;
	invalid.money[0] = -1;
	assert(!corpse_lifecycle_command_build(&command, operation(2), invalid,
					       critical_source_site::command,
					       critical_deadline_class::interactive));
	invalid = source;
	invalid.values[3] = 9;
	assert(!corpse_lifecycle_command_build(&command, operation(2), invalid,
					       critical_source_site::command,
					       critical_deadline_class::interactive));
	corpse_lifecycle_result result = { 42, 20, corpse_lifecycle_action::upsert, 2, 7 };
	std::array<uint8_t, CORPSE_LIFECYCLE_RESULT_BYTES> encoded_result = {};
	assert(corpse_lifecycle_command_encode_result(result, &encoded_result));
	corpse_lifecycle_result decoded_result = {};
	assert(corpse_lifecycle_command_decode_result(encoded_result.data(), encoded_result.size(),
						      &decoded_result));
	assert(decoded_result.corpse_revision == 2 && decoded_result.catalog_revision == 7);
	assert(corpse_lifecycle_command_decode_result(encoded_result.data(),
						      CORPSE_LIFECYCLE_LEGACY_RESULT_BYTES,
						      &decoded_result));
	auto legacy = upsert_command;
	legacy.payload.erase(legacy.payload.begin() + 80, legacy.payload.begin() + 88);
	legacy.payload_version = CORPSE_LIFECYCLE_LEGACY_PAYLOAD_VERSION;
	assert(corpse_lifecycle_command_decode_payload(legacy, &decoded));
	assert(decoded.expected_room_revision == 0);
	corpse_lifecycle_payload remove = {};
	remove.action = corpse_lifecycle_action::remove;
	remove.owner_pid = 42;
	remove.save_id = 20;
	remove.expected_corpse_revision = 2;
	remove.owner_name = "Hero";
	assert(corpse_lifecycle_command_build(&command, operation(3), remove,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	remove.room_vnum = 500;
	assert(!corpse_lifecycle_command_build(&command, operation(4), remove,
					       critical_source_site::command,
					       critical_deadline_class::terminal));
	corpse_lifecycle_payload release = {};
	release.action = corpse_lifecycle_action::release;
	release.owner_pid = 42;
	release.save_id = 20;
	release.expected_corpse_revision = 2;
	release.expected_room_revision = 4;
	release.room_vnum = 500;
	release.owner_name = "Hero";
	assert(corpse_lifecycle_command_build(&command, operation(5), release,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	assert(command.keys.size() == 2 && command.expected_revisions.size() == 2);
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	assert(decoded.action == corpse_lifecycle_action::release &&
	       decoded.expected_room_revision == 4 && decoded.room_vnum == 500);
	corpse_lifecycle_result release_result = {};
	release_result.owner_pid = 42;
	release_result.save_id = 20;
	release_result.action = corpse_lifecycle_action::release;
	release_result.catalog_revision = 8;
	release_result.corpse_owner_revision = 5;
	release_result.room_owner_revision = 5;
	release_result.max_item_revision = 9;
	release_result.item_count = 2;
	assert(corpse_lifecycle_command_encode_result(release_result, &encoded_result));
	assert(corpse_lifecycle_command_decode_result(encoded_result.data(), encoded_result.size(),
						      &decoded_result));
	assert(decoded_result.action == corpse_lifecycle_action::release &&
	       decoded_result.room_owner_revision == 5 && decoded_result.item_count == 2);
	release.money[0] = 1;
	assert(!corpse_lifecycle_command_build(&command, operation(6), release,
					       critical_source_site::command,
					       critical_deadline_class::terminal));
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-corpse-lifecycle-command-") as temporary:
    temporary_path = pathlib.Path(temporary)
    source = temporary_path / "harness.cpp"
    binary = temporary_path / "harness"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "src/corpse_lifecycle_command.c",
            "src/item_transfer_command.c",
            "src/critical_command.c",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("[PASS] bounded corpse lifecycle command codec")
