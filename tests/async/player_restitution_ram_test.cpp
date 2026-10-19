#include "player/player_restitution_ram.h"
#include "player/player_checkpoint_ram.h"
#include "player/artifact_restitution_ram.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <openssl/sha.h>

namespace
{
critical_operation_id id(uint8_t seed)
{
	critical_operation_id value = {};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(seed + index);
	return value;
}

player_death_restitution_item make_item(uint64_t uid, uint64_t parent = 0)
{
	player_death_restitution_item_state state = {};
	state.item_uid = uid;
	state.vnum = 7000 + static_cast<uint32_t>(uid);
	state.quantity = 1;
	state.weight = 4;
	state.cost = 99;
	state.timer = -1;
	state.item_type = 1;
	state.material = 2;
	state.condition = 100;
	state.string_present[0] = true;
	state.strings[0] = { 'r', 'e', 's', 't', 'o', 'r', 'e' };
	state.bitvector_present[0] = true;
	state.bitvectors[0] = 0x22;
	state.affects.push_back({ 1, 2 });
	state.extra_descriptions.push_back({ { 'k', 'e', 'y' }, { 'v', 'a', 'l' } });
	std::vector<uint8_t> state_payload;
	assert(player_death_restitution_item_state_encode(state, &state_payload));

	player_death_restitution_item item = {};
	item.item_uid = uid;
	item.source_root_item_uid = parent ? 100 : uid;
	item.source_parent_item_uid = parent;
	item.delivered_root_item_uid = parent ? 100 : uid;
	item.delivered_parent_item_uid = parent;
	item.source_item_revision = 12;
	item.custody_item_revision = 9;
	item.expected_item_revision = 12;
	item.expected_owner_revision = 4;
	item.expected_owner_state = PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE;
	item.custody_state = 1;
	item.custody_owner_type = PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE;
	item.custody_owner_id = 10;
	item.custody_owner_context_id = 0;
	item.custody_owner_revision = 4;
	item.vnum = state.vnum;
	item.disposition = player_death_restitution_disposition::deliver;
	item.classification = "ordinary_item";
	item.note = "native first slice";
	item.metadata_payload = state_payload;
	item.original_payload = { 0xaa, 0xbb, 0xcc };
	SHA256(state_payload.data(), state_payload.size(), item.metadata_digest.data());
	return item;
}

player_death_restitution_plan make_plan()
{
	player_death_restitution_plan plan = {};
	plan.source_pid = 10;
	plan.death_revision = 77;
	plan.recipient_pid = 20;
	plan.restitution_id = id(1);
	plan.death_operation_id = id(33);
	plan.evidence_digest.fill(0x11);
	plan.payload_digest.fill(0x33);
	plan.plan_digest.fill(0x22);
	plan.expected_recipient_save_revision = 8;
	plan.expected_source_owner_revision = 4;
	plan.expected_recipient_owner_revision = 6;
	plan.loss_epoch = 1700000000;
	plan.accepted_at_usec = 1700000000000000ULL;
	plan.actor = "native-test";
	plan.reason = "death restitution";
	plan.items.push_back(make_item(101));
	return plan;
}
}

int main()
{
	auto plan = make_plan();
	plan.items.push_back(make_item(102));
	player_death_ram_evidence evidence;
	evidence.operation_id = plan.death_operation_id;
	evidence.digest = plan.payload_digest;
	evidence.epoch = plan.loss_epoch;
	const item_owner_identity source{ item_owner_type::player, 10, 0 };
	const item_owner_identity recipient{ item_owner_type::player, 20, 0 };
	assert(player_checkpoint_ram_seed_revision(20, 8));
	assert(item_ownership_runtime_hydrate_owner(recipient, 6));
	for (const auto &item : plan.items)
	{
		evidence.custody.emplace(
			item.item_uid,
			player_death_custody_snapshot{
				{ item.item_uid, item.source_root_item_uid,
				  item.source_parent_item_uid, item.custody_item_revision,
				  static_cast<int32_t>(item.vnum), item_custody_state::active },
				source,
				item.custody_owner_revision });
		assert(item_ownership_runtime_hydrate(
			{ item.item_uid, item.source_root_item_uid, item.source_parent_item_uid,
			  source, item.expected_item_revision, 4, static_cast<int32_t>(item.vnum),
			  item_custody_state::quarantined }));
	}
	assert(player_checkpoint_ram_seed_death(10, 77, std::move(evidence)));
	critical_command command;
	auto invalid = plan;
	invalid.items[1].custody_item_revision++;
	assert(player_death_restitution_command_build(invalid, &command));
	assert(player_restitution_ram_prepare(command).error_code == ESTALE);
	assert(player_death_restitution_command_build(plan, &command));
	auto prepared = player_restitution_ram_prepare(command);
	assert(prepared.outcome == critical_apply_outcome::applied);
	item_ownership_runtime_entry current;
	assert(item_ownership_runtime_lookup(101, &current) && current.owner.id == 10);
	assert(!player_restitution_ram_publish(command)); // no sealed outcome
	command.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
	command.outcome_revision = prepared.durable_revision;
	command.outcome_payload.assign(prepared.result_payload.begin(),
				       prepared.result_payload.begin() + prepared.result_size);
	auto damaged = command;
	damaged.outcome_payload[0] ^= 1;
	assert(!player_restitution_ram_publish(damaged));
	assert(player_restitution_ram_can_publish(command));
	assert(player_restitution_ram_publish(command));
	for (uint64_t uid : { 101, 102 })
		assert(item_ownership_runtime_lookup(uid, &current) && current.owner.id == 20 &&
		       current.item_revision == 13 && current.state == item_custody_state::active);
	uint64_t clock = 0;
	assert(item_ownership_runtime_peek_owner_revision(source, &clock) && clock == 5);
	assert(item_ownership_runtime_peek_owner_revision(recipient, &clock) && clock == 7);
	assert(!player_restitution_ram_publish(command));
	assert(player_death_restitution_command_build(plan, &command));
	assert(player_restitution_ram_prepare(command).error_code == ESTALE);

	// Exact artifact evidence, including absent legacy/baseline rows, is checked
	// against RAM. Preparing or rejecting a plan cannot publish any timer change.
	plan = make_plan();
	auto &artifact = plan.items.front();
	artifact.artifact_vnum = artifact.vnum;
	artifact.artifact_timing_evidence_present = true;
	artifact.artifact_loss_epoch = plan.loss_epoch;
	artifact.artifact_source_timer_epoch = plan.loss_epoch + 456;
	artifact.artifact_usable_lifetime_seconds = 456;
	artifact.artifact_source_location_type =
		PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_CORPSE;
	artifact.artifact_source_location = plan.source_pid;
	artifact.artifact_type = PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_UNIQUE;
	artifact.artifact_domain_present = true;
	artifact.artifact_domain_item_uid_present = true;
	artifact.artifact_domain_item_uid = artifact.item_uid;
	artifact.artifact_domain_item_revision = artifact.expected_item_revision;
	artifact.artifact_domain_revision = 4;
	artifact.artifact_bind_present = true;
	artifact.artifact_bind_owner_pid = plan.source_pid;
	artifact.artifact_bind_timer_epoch = 654321;
	artifact.artifact_legacy_projection_mask = PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL;
	artifact_restitution_state state{};
	state.vnum = artifact.vnum;
	state.domain = { true,
			 true,
			 artifact.artifact_type,
			 artifact.artifact_source_location_type,
			 artifact.artifact_source_location,
			 static_cast<int64_t>(artifact.artifact_source_timer_epoch) };
	state.mortal = state.domain;
	state.uid_present = state.bind_present = true;
	state.uid = artifact.item_uid;
	state.item_revision = artifact.expected_item_revision;
	state.revision = 4;
	state.bind_owner = state.bind_row_owner = artifact.artifact_bind_owner_pid;
	state.bind_timer = state.bind_row_timer = artifact.artifact_bind_timer_epoch;
	assert(artifact_restitution_ram_seed(state));
	const uint64_t delivery = plan.loss_epoch + 10000;
	auto changed = plan;
	changed.items.front().artifact_domain_item_uid++;
	assert(!artifact_restitution_ram_prepare(changed, delivery));
	changed = plan;
	changed.items.front().artifact_legacy_projection_mask |=
		PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_GOD;
	assert(!artifact_restitution_ram_prepare(changed, delivery));
	changed = plan;
	changed.items.front().artifact_bind_timer_epoch++;
	assert(!artifact_restitution_ram_prepare(changed, delivery));
	assert(artifact_restitution_ram_prepare(plan, delivery));
	assert(artifact_restitution_ram_lookup(state.vnum)->domain.timer == state.domain.timer);
	artifact_restitution_ram_publish();
	const auto restored = artifact_restitution_ram_lookup(state.vnum);
	assert(restored->domain.timer == static_cast<int64_t>(delivery + 456));
	assert(restored->domain.location == static_cast<int32_t>(plan.recipient_pid));
	assert(restored->revision == 5 &&
	       restored->item_revision == artifact.expected_item_revision + 1);
	assert(!artifact_restitution_ram_prepare(plan, delivery));
}
