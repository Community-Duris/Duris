#include "player/player_death_restitution_runtime.h"

#include <cassert>
#include <openssl/sha.h>

// Submit is not exercised here because the coordinator is deliberately not
// initialized by this pure boundary test.  This stub keeps the test independent
// of the coordinator journal and its worker thread.
critical_submit_result critical_command_coordinator_submit(critical_command)
{
	return critical_submit_result::unavailable;
}

namespace
{
critical_operation_id test_id(uint8_t seed)
{
	critical_operation_id value = {};
	for (size_t i = 0; i < value.bytes.size(); ++i)
		value.bytes[i] = static_cast<uint8_t>(seed + i);
	return value;
}

player_death_restitution_plan test_plan()
{
	player_death_restitution_item_state state = {};
	state.item_uid = 99;
	state.vnum = 7001;
	state.quantity = 1;
	state.timer = -1;
	std::vector<uint8_t> metadata;
	assert(player_death_restitution_item_state_encode(state, &metadata));
	player_death_restitution_item item = {};
	item.item_uid = 99;
	item.source_root_item_uid = 99;
	item.delivered_root_item_uid = 99;
	item.source_item_revision = 2;
	item.custody_item_revision = 1;
	item.expected_item_revision = 2;
	item.expected_owner_revision = 3;
	item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
	item.custody_state = 1;
	item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
	item.custody_owner_id = 1;
	item.custody_owner_context_id = 0;
	item.custody_owner_revision = 3;
	item.vnum = 7001;
	item.disposition = player_death_restitution_disposition::deliver;
	item.classification = "ordinary_item";
	item.note = "test";
	item.metadata_payload = metadata;
	item.original_payload = { 1 };
	SHA256(metadata.data(), metadata.size(), item.metadata_digest.data());
	player_death_restitution_plan plan = {};
	plan.source_pid = 1;
	plan.death_revision = 1;
	plan.recipient_pid = 2;
	plan.restitution_id = test_id(1);
	plan.death_operation_id = test_id(2);
	plan.evidence_digest.fill(1);
	plan.plan_digest.fill(2);
	plan.expected_recipient_save_revision = 7;
	plan.expected_source_owner_revision = 3;
	plan.expected_recipient_owner_revision = 4;
	plan.loss_epoch = 100;
	plan.accepted_at_usec = 1000;
	plan.actor = "test";
	plan.reason = "test";
	plan.items.push_back(item);
	return plan;
}

struct callback_state
{
	bool offline;
	bool pending_save;
};

bool offline(uint32_t, void *context)
{
	return static_cast<callback_state *>(context)->offline;
}

bool pending(uint32_t, uint64_t, void *context)
{
	return !static_cast<callback_state *>(context)->pending_save;
}

bool acquire(uint32_t, uint64_t, void *)
{
	return true;
}

void release(uint32_t, uint64_t, void *) {}
}

int main()
{
	const player_death_restitution_plan plan = test_plan();
	const player_death_restitution_runtime_callbacks callbacks = { offline, pending, acquire,
								       release };
	callback_state state = { false, false };
	assert(player_death_restitution_runtime_preflight(plan, callbacks, &state) ==
	       player_death_restitution_runtime_result::recipient_online);
	state.offline = true;
	state.pending_save = true;
	assert(player_death_restitution_runtime_preflight(plan, callbacks, &state) ==
	       player_death_restitution_runtime_result::pending_save);
	state.pending_save = false;
	assert(player_death_restitution_runtime_preflight(plan, callbacks, &state) ==
	       player_death_restitution_runtime_result::accepted);
	const player_death_restitution_runtime_callbacks incomplete = { offline, pending, acquire,
									nullptr };
	assert(player_death_restitution_runtime_preflight(plan, incomplete, &state) ==
	       player_death_restitution_runtime_result::invalid_plan);
	return 0;
}
