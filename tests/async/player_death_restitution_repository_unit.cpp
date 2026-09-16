#include "persistence/player_death_restitution_repository.h"

#include <cassert>
#include <cerrno>
#include <openssl/sha.h>

// Pure repository contract test: no database connection is opened.  These
// local hooks satisfy the transaction-finalization references in the repository
// object without starting the critical coordinator.
bool critical_command_repository_insert_outbox_event(MYSQL *, const critical_operation_id &,
						     uint16_t, uint16_t, uint16_t, uint16_t,
						     const uint8_t *, size_t)
{
	return false;
}

bool critical_command_repository_finish_inbox(MYSQL *, const critical_command &, uint64_t,
					      unsigned int, const uint8_t *, size_t)
{
	return false;
}

namespace
{
critical_operation_id operation_id(uint8_t seed)
{
	critical_operation_id value = {};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
}

player_death_restitution_plan make_plan()
{
	player_death_restitution_item_state state = {};
	state.item_uid = 1;
	state.vnum = 2;
	state.quantity = 1;
	std::vector<uint8_t> state_payload;
	assert(player_death_restitution_item_state_encode(state, &state_payload));
	player_death_restitution_item item = {};
	item.item_uid = 1;
	item.source_root_item_uid = 1;
	item.delivered_root_item_uid = 1;
	item.source_item_revision = 2;
	item.custody_item_revision = 1;
	item.expected_item_revision = 2;
	item.expected_owner_revision = 3;
	item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
	item.custody_state = 1;
	item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
	item.custody_owner_id = 10;
	item.custody_owner_context_id = 0;
	item.custody_owner_revision = 3;
	item.vnum = 2;
	item.disposition = player_death_restitution_disposition::deliver;
	item.classification = "ordinary_item";
	item.note = "contract";
	item.metadata_payload = state_payload;
	item.original_payload = { 9 };
	SHA256(state_payload.data(), state_payload.size(), item.metadata_digest.data());
	player_death_restitution_plan value = {};
	value.source_pid = 10;
	value.death_revision = 11;
	value.recipient_pid = 12;
	value.restitution_id = operation_id(1);
	value.death_operation_id = operation_id(2);
	value.evidence_digest.fill(1);
	value.plan_digest.fill(2);
	value.expected_recipient_save_revision = 4;
	value.expected_source_owner_revision = 3;
	value.expected_recipient_owner_revision = 5;
	value.loss_epoch = 100;
	value.accepted_at_usec = 1000;
	value.actor = "test";
	value.reason = "test";
	value.items.push_back(item);
	return value;
}
}

int main()
{
	player_death_restitution_plan value = make_plan();
	unsigned int error_code = 0;
	assert(player_death_restitution_repository_validate_plan(value, &error_code));
	assert(error_code == 0);
	assert(!player_death_restitution_repository_execute(nullptr, critical_command{}, nullptr,
							    nullptr, nullptr));
	value.items[0].artifact_vnum = value.items[0].vnum;
	value.items[0].artifact_timing_uid_approved = true;
	value.items[0].artifact_approval_uid = value.items[0].item_uid;
	value.items[0].artifact_source_location_type =
		PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER;
	value.items[0].artifact_source_location = static_cast<int32_t>(value.source_pid);
	value.items[0].artifact_type = PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_UNIQUE;
	value.items[0].artifact_source_timer_epoch = 1000;
	value.items[0].artifact_usable_lifetime_seconds = 5;
	value.items[0].artifact_domain_present = true;
	value.items[0].artifact_domain_item_uid_present = true;
	value.items[0].artifact_domain_item_uid = value.items[0].item_uid;
	value.items[0].artifact_domain_item_revision = value.items[0].expected_item_revision;
	value.items[0].artifact_domain_revision = 3;
	value.items[0].artifact_legacy_projection_mask =
		PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL;
	assert(player_death_restitution_repository_validate_plan(value, &error_code));
	assert(error_code == 0);
	uint64_t delivered_timer = 0;
	assert(player_death_restitution_artifact_delivery_timer(value.items[0], 100,
								&delivered_timer));
	assert(delivered_timer == 105);
	value.items[0].artifact_timing_uid_approved = false;
	value.items[0].artifact_approval_uid = 0;
	assert(!player_death_restitution_repository_validate_plan(value, &error_code));
	assert(error_code == EINVAL);
	return 0;
}
