#!/usr/bin/env python3
"""Regressions for coalesced, revision-aware live corpse persistence."""

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "corpse_lifecycle_transaction.h"

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

static std::vector<critical_command> submitted;
static bool externally_fenced = false;

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	submitted.push_back(std::move(command));
	return critical_submit_result::accepted;
}

bool critical_command_coordinator_is_fenced(const critical_entity_key &,
					    critical_operation_id *)
{
	return externally_fenced;
}

static corpse_lifecycle_payload upsert(uint32_t owner_pid, uint32_t save_id,
					int room, int money)
{
	corpse_lifecycle_payload payload = {};
	payload.owner_pid = owner_pid;
	payload.save_id = save_id;
	payload.room_vnum = room;
	payload.weight = 25;
	payload.values[3] = static_cast<int32_t>(owner_pid);
	payload.values[5] = 1;
	payload.values[6] = static_cast<int32_t>(save_id);
	payload.money[0] = money;
	payload.owner_name = "Hero";
	payload.short_description = "the corpse of Hero";
	payload.description = "The corpse of Hero is lying here.";
	payload.keywords = "hero corpse _pcorpse_";
	return payload;
}

static corpse_lifecycle_payload remove(uint32_t owner_pid, uint32_t save_id)
{
	corpse_lifecycle_payload payload = {};
	payload.action = corpse_lifecycle_action::remove;
	payload.owner_pid = owner_pid;
	payload.save_id = save_id;
	payload.owner_name = "Hero";
	return payload;
}

static corpse_lifecycle_payload decode(size_t index)
{
	corpse_lifecycle_payload payload = {};
	assert(index < submitted.size());
	assert(corpse_lifecycle_command_decode_payload(submitted[index], &payload));
	return payload;
}

static critical_completion completion(size_t index, corpse_lifecycle_action action,
				      uint32_t owner_pid, uint32_t save_id,
				      uint64_t corpse_revision, uint64_t catalog_revision)
{
	critical_completion value = {};
	value.operation_id = submitted[index].operation_id;
	value.outcome = critical_apply_outcome::applied;
	const corpse_lifecycle_result result = { owner_pid, save_id, action,
						 corpse_revision, catalog_revision };
	std::array<uint8_t, CORPSE_LIFECYCLE_RESULT_BYTES> encoded = {};
	assert(corpse_lifecycle_command_encode_result(result, &encoded));
	value.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), value.result_payload.begin());
	return value;
}

int main()
{
	corpse_lifecycle_transaction_reset_for_tests();
	assert(corpse_lifecycle_transaction_stage(upsert(42, 20, 500, 1)));
	assert(submitted.size() == 1 && decode(0).expected_corpse_revision == 0);
	assert(corpse_lifecycle_transaction_stage(upsert(42, 20, 501, 2)));
	assert(submitted.size() == 1);

	auto done = completion(0, corpse_lifecycle_action::upsert, 42, 20, 1, 10);
	corpse_lifecycle_transaction_handle_completions(&done, 1);
	assert(submitted.size() == 2);
	auto decoded = decode(1);
	assert(decoded.expected_corpse_revision == 1 && decoded.room_vnum == 501 &&
	       decoded.money[0] == 2);
	done = completion(1, corpse_lifecycle_action::upsert, 42, 20, 2, 11);
	corpse_lifecycle_transaction_handle_completions(&done, 1);
	assert(!corpse_lifecycle_transaction_busy(42, 20));

	assert(corpse_lifecycle_transaction_stage(remove(42, 20)));
	assert(submitted.size() == 2);
	corpse_lifecycle_transaction_pulse();
	assert(submitted.size() == 3);
	decoded = decode(2);
	assert(decoded.action == corpse_lifecycle_action::remove &&
	       decoded.expected_corpse_revision == 2);
	assert(corpse_lifecycle_transaction_stage(upsert(42, 20, 777, 3)));
	done = completion(2, corpse_lifecycle_action::remove, 42, 20, 0, 12);
	corpse_lifecycle_transaction_handle_completions(&done, 1);
	assert(submitted.size() == 4);
	decoded = decode(3);
	assert(decoded.action == corpse_lifecycle_action::upsert &&
	       decoded.expected_corpse_revision == 0 && decoded.room_vnum == 777);
	done = completion(3, corpse_lifecycle_action::upsert, 42, 20, 1, 13);
	corpse_lifecycle_transaction_handle_completions(&done, 1);

	externally_fenced = true;
	assert(corpse_lifecycle_transaction_stage(upsert(43, 21, 900, 4)));
	assert(submitted.size() == 4);
	assert(corpse_lifecycle_transaction_note_item_transfer(43, 21, 5));
	assert(submitted.size() == 4);
	externally_fenced = false;
	corpse_lifecycle_transaction_pulse();
	assert(submitted.size() == 5 && decode(4).expected_corpse_revision == 5);
	done = completion(4, corpse_lifecycle_action::upsert, 43, 21, 6, 14);
	corpse_lifecycle_transaction_handle_completions(&done, 1);

	const auto health = corpse_lifecycle_transaction_health_copy();
	assert(health.submitted == 5 && health.committed == 5 && health.rejected == 0 &&
	       health.pending == 0 && health.dirty == 0);
	assert(corpse_lifecycle_transaction_forget(42, 20));
	assert(corpse_lifecycle_transaction_forget(43, 21));
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-corpse-lifecycle-transaction-") as temporary:
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
            "src/corpse_lifecycle_transaction.c",
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

print("[PASS] live corpse lifecycle transactions coalesce and track revisions")
