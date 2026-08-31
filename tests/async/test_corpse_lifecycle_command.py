#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "persistence/corpse_lifecycle_command.h"

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
	legacy.payload.erase(legacy.payload.begin() + 80, legacy.payload.begin() + 136);
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
	const auto release_command = command;
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
	auto previous_release = release_command;
	previous_release.payload.erase(previous_release.payload.begin() + 112,
				       previous_release.payload.begin() + 136);
	previous_release.payload_version = CORPSE_LIFECYCLE_PREVIOUS_PAYLOAD_VERSION;
	assert(corpse_lifecycle_command_decode_payload(previous_release, &decoded));
	corpse_lifecycle_payload destroy = release;
	destroy.action = corpse_lifecycle_action::destroy;
	destroy.expected_room_revision = 2;
	assert(corpse_lifecycle_command_build(&command, operation(6), destroy,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	assert(command.keys.size() == 2 && command.expected_revisions.size() == 2);
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	assert(decoded.action == corpse_lifecycle_action::destroy && decoded.room_vnum == 500 &&
	       decoded.expected_room_revision == 2);
	auto previous_destroy = command;
	previous_destroy.payload.erase(previous_destroy.payload.begin() + 112,
				       previous_destroy.payload.begin() + 136);
	previous_destroy.payload_version = CORPSE_LIFECYCLE_PREVIOUS_PAYLOAD_VERSION;
	assert(corpse_lifecycle_command_decode_payload(previous_destroy, &decoded));
	auto unsupported_release_version_destroy = previous_destroy;
	unsupported_release_version_destroy.payload_version = CORPSE_LIFECYCLE_RELEASE_PAYLOAD_VERSION;
	assert(!corpse_lifecycle_command_decode_payload(unsupported_release_version_destroy,
						       &decoded));
	corpse_lifecycle_result destroy_result = release_result;
	destroy_result.action = corpse_lifecycle_action::destroy;
	assert(corpse_lifecycle_command_encode_result(destroy_result, &encoded_result));
	assert(corpse_lifecycle_command_decode_result(encoded_result.data(), encoded_result.size(),
						      &decoded_result));
	assert(decoded_result.action == corpse_lifecycle_action::destroy &&
	       decoded_result.corpse_owner_revision == 5 && decoded_result.item_count == 2);
	corpse_lifecycle_payload resurrect = {};
	resurrect.action = corpse_lifecycle_action::resurrect;
	resurrect.owner_pid = 42;
	resurrect.save_id = 20;
	resurrect.expected_corpse_revision = 2;
	resurrect.expected_room_revision = 7;
	resurrect.destination_player_pid = 77;
	resurrect.old_room_vnum = 600;
	resurrect.expected_player_revision = 9;
	resurrect.expected_wallet_revision = 11;
	resurrect.room_vnum = 500;
	resurrect.money = { 4, 3, 2, 1 };
	resurrect.owner_name = "Hero";
	assert(corpse_lifecycle_command_build(&command, operation(7), resurrect,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	assert(command.keys.size() == 3 && command.expected_revisions.size() == 3);
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	assert(decoded.action == corpse_lifecycle_action::resurrect &&
	       decoded.destination_player_pid == 77 && decoded.old_room_vnum == 600 &&
	       decoded.expected_wallet_revision == 11 && decoded.money[0] == 4);
	auto previous_resurrect = command;
	previous_resurrect.payload.erase(previous_resurrect.payload.begin() + 112,
					 previous_resurrect.payload.begin() + 136);
	previous_resurrect.payload_version = CORPSE_LIFECYCLE_PREVIOUS_PAYLOAD_VERSION;
	assert(corpse_lifecycle_command_decode_payload(previous_resurrect, &decoded));
	auto unsupported_previous_resurrect = command;
	unsupported_previous_resurrect.payload.erase(
		unsupported_previous_resurrect.payload.begin() + 88,
		unsupported_previous_resurrect.payload.begin() + 136);
	unsupported_previous_resurrect.payload_version =
		CORPSE_LIFECYCLE_INTERMEDIATE_PAYLOAD_VERSION;
	assert(!corpse_lifecycle_command_decode_payload(unsupported_previous_resurrect, &decoded));
	corpse_lifecycle_result resurrect_result = {};
	resurrect_result.owner_pid = 42;
	resurrect_result.save_id = 20;
	resurrect_result.action = corpse_lifecycle_action::resurrect;
	resurrect_result.catalog_revision = 9;
	resurrect_result.corpse_owner_revision = 6;
	resurrect_result.room_owner_revision = 8;
	resurrect_result.player_owner_revision = 10;
	resurrect_result.wallet_revision = 12;
	resurrect_result.max_item_revision = 13;
	resurrect_result.item_count = 2;
	resurrect_result.wallet = { 1, 2, 3, 4 };
	assert(corpse_lifecycle_command_encode_result(resurrect_result, &encoded_result));
	assert(corpse_lifecycle_command_decode_result(encoded_result.data(), encoded_result.size(),
						      &decoded_result));
	assert(decoded_result.action == corpse_lifecycle_action::resurrect &&
	       decoded_result.player_owner_revision == 10 && decoded_result.wallet_revision == 12 &&
	       decoded_result.wallet[3] == 4);
	corpse_lifecycle_payload raise = resurrect;
	raise.action = corpse_lifecycle_action::raise_follower;
	raise.expected_room_revision = 0;
	raise.old_room_vnum = 0;
	assert(corpse_lifecycle_command_build(&command, operation(8), raise,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	assert(command.keys.size() == 2 && command.expected_revisions.size() == 2);
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	assert(decoded.action == corpse_lifecycle_action::raise_follower &&
	       decoded.destination_player_pid == 77 && decoded.expected_player_revision == 9 &&
	       decoded.expected_wallet_revision == 11 && decoded.room_vnum == 500);
	auto previous_raise = command;
	previous_raise.payload.erase(previous_raise.payload.begin() + 112,
				     previous_raise.payload.begin() + 136);
	previous_raise.payload_version = CORPSE_LIFECYCLE_PREVIOUS_PAYLOAD_VERSION;
	assert(corpse_lifecycle_command_decode_payload(previous_raise, &decoded));
	auto unsupported_previous_raise = command;
	unsupported_previous_raise.payload.erase(unsupported_previous_raise.payload.begin() + 88,
					 unsupported_previous_raise.payload.begin() + 136);
	unsupported_previous_raise.payload_version =
		CORPSE_LIFECYCLE_INTERMEDIATE_PAYLOAD_VERSION;
	assert(!corpse_lifecycle_command_decode_payload(unsupported_previous_raise, &decoded));
	corpse_lifecycle_result raise_result = resurrect_result;
	raise_result.action = corpse_lifecycle_action::raise_follower;
	raise_result.room_owner_revision = 0;
	assert(corpse_lifecycle_command_encode_result(raise_result, &encoded_result));
	assert(corpse_lifecycle_command_decode_result(encoded_result.data(), encoded_result.size(),
					      &decoded_result));
	assert(decoded_result.action == corpse_lifecycle_action::raise_follower &&
	       decoded_result.room_owner_revision == 0 &&
	       decoded_result.player_owner_revision == 10 && decoded_result.wallet[0] == 1);
	corpse_lifecycle_payload nested = release;
	nested.action = corpse_lifecycle_action::release_nested;
	nested.target_root_item_uid = 100;
	nested.target_parent_item_uid = 101;
	nested.expected_target_parent_revision = 12;
	assert(corpse_lifecycle_command_build(&command, operation(9), nested,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	assert(command.keys.size() == 2 && command.expected_revisions.size() == 2);
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	assert(decoded.action == corpse_lifecycle_action::release_nested &&
	       decoded.target_root_item_uid == 100 && decoded.target_parent_item_uid == 101 &&
	       decoded.expected_target_parent_revision == 12);
	auto unsupported_previous_nested = command;
	unsupported_previous_nested.payload.erase(unsupported_previous_nested.payload.begin() + 112,
					  unsupported_previous_nested.payload.begin() + 136);
	unsupported_previous_nested.payload_version = CORPSE_LIFECYCLE_PREVIOUS_PAYLOAD_VERSION;
	assert(!corpse_lifecycle_command_decode_payload(unsupported_previous_nested, &decoded));
	corpse_lifecycle_result nested_result = release_result;
	nested_result.action = corpse_lifecycle_action::release_nested;
	assert(corpse_lifecycle_command_encode_result(nested_result, &encoded_result));
	nested.destination_player_pid = 77;
	nested.expected_room_revision = 0;
	nested.expected_player_revision = 9;
	nested.expected_wallet_revision = 11;
	nested.money = { 4, 3, 2, 1 };
	assert(corpse_lifecycle_command_build(&command, operation(10), nested,
					      critical_source_site::command,
					      critical_deadline_class::terminal));
	assert(corpse_lifecycle_command_decode_payload(command, &decoded));
	nested_result.room_owner_revision = 0;
	nested_result.player_owner_revision = 10;
	nested_result.wallet_revision = 12;
	nested_result.wallet = { 5, 6, 7, 8 };
	assert(corpse_lifecycle_command_encode_result(nested_result, &encoded_result));
	release.money[0] = 1;
	assert(!corpse_lifecycle_command_build(&command, operation(11), release,
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
            rel("corpse_lifecycle_command.c"),
            rel("item_transfer_command.c"),
            rel("critical_command.c"),
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("[PASS] bounded corpse lifecycle command codec")
