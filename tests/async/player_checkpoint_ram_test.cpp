#include "player/player_checkpoint_ram.h"
#include "player/player_checkpoint_command.h"
#include "core/structs.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"
#include <cassert>
constexpr int PROBE_PID = 1;
player_snapshot make_death(player_revision_t revision)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.pid = PROBE_PID;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 4; // RENT_DEATH
	snapshot.room_vnum = 1201;
	snapshot.encoded_size_bound = 8192;
	snapshot.status_integers.push_back({ player_status_field::level, 50, 0, false });
	snapshot.status_integers.push_back({ player_status_field::copper, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::silver, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::gold, 0, 0, false });
	snapshot.status_integers.push_back({ player_status_field::platinum, 0, 0, false });
	snapshot.status_strings.push_back({ player_status_string_field::name, "Probe" });
	snapshot.recipes_are_external = true;

	snapshot.death.emplace();
	player_death_snapshot &death = *snapshot.death;
	death.operation_id.bytes.fill(0);
	death.operation_id.bytes[0] = 0xa5;
	death.operation_id.bytes[15] = 0x5a;
	death.corpse_room_vnum = 1201;
	death.wallet_revision = 7;
	death.wallet_before = { 11, 12, 13, 14 };
	death.wallet_pile_uid = 202;

	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 200;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	corpse.values[CORPSE_PID] = PROBE_PID;
	corpse.values[CORPSE_SAVEID] = 9001;
	death.corpse.push_back(corpse);

	player_item_snapshot refused = {};
	refused.parent_index = 0;
	refused.object_uid = 201;
	refused.vnum = 501;
	death.corpse.push_back(refused);

	player_item_snapshot captured_child = {};
	captured_child.parent_index = 0;
	captured_child.object_uid = 203;
	captured_child.vnum = 501;
	death.corpse.push_back(captured_child);

	player_item_snapshot wallet = {};
	wallet.parent_index = 0;
	wallet.object_uid = death.wallet_pile_uid;
	wallet.vnum = VOBJ_COINS;
	wallet.type = ITEM_MONEY;
	for (size_t denomination = 0; denomination < death.wallet_before.size(); ++denomination)
		wallet.values[denomination] = death.wallet_before[denomination];
	death.corpse.push_back(wallet);

	death.custody.push_back({ { 201, 201, 0, 3, 501, item_custody_state::active },
				  { item_owner_type::player, PROBE_PID, 0 },
				  5 });
	death.custody.push_back({ { 203, 201, 201, 1, 501, item_custody_state::active },
				  { item_owner_type::player, PROBE_PID, 0 },
				  5 });
	death.custody.push_back(
		{ { death.wallet_pile_uid, death.wallet_pile_uid, 0, ITEM_TRANSFER_ABSENT_REVISION,
		    VOBJ_COINS, item_custody_state::absent },
		  {},
		  0 });
	return snapshot;
}

int main()
{
	const item_owner_identity owner{ item_owner_type::player, 1, 0 };
	assert(player_checkpoint_ram_seed_revision(1, 1));
	assert(item_ownership_runtime_hydrate(
		{ 201, 201, 0, owner, 3, 5, 501, item_custody_state::active }));
	assert(item_ownership_runtime_hydrate(
		{ 203, 201, 201, owner, 1, 5, 501, item_custody_state::active }));
	// Durable-only descendant is retained even though capture could not see it.
	assert(item_ownership_runtime_hydrate(
		{ 204, 201, 201, owner, 9, 5, 501, item_custody_state::active }));
	assert(item_ownership_runtime_hydrate(
		{ 205, 205, 0, owner, 2, 5, 501, item_custody_state::active }));
	auto snapshot = make_death(2);
	player_checkpoint_ram_change abandoned;
	assert(player_checkpoint_ram_prepare(snapshot, 123000000, false, &abandoned));
	item_ownership_runtime_entry item;
	assert(item_ownership_runtime_lookup(201, &item) && item.item_revision == 3);
	assert(!player_checkpoint_ram_death(1, 2));
	player_checkpoint_ram_change accepted;
	assert(player_checkpoint_ram_prepare(snapshot, 124000000, false, &accepted));
	assert(accepted.custody.after.size() == 3);
	assert(player_checkpoint_ram_publish(&accepted));
	assert(!player_checkpoint_ram_publish(&abandoned));
	assert(item_ownership_runtime_lookup(201, &item) && item.item_revision == 4 &&
	       item.state == item_custody_state::quarantined);
	assert(item_ownership_runtime_lookup(204, &item) && item.item_revision == 10 &&
	       item.state == item_custody_state::quarantined);
	assert(item_ownership_runtime_lookup(205, &item) && item.item_revision == 2 &&
	       item.state == item_custody_state::active);
	uint64_t revision = 0;
	assert(item_ownership_runtime_peek_owner_revision(owner, &revision) && revision == 6);
	assert(player_checkpoint_ram_revision(1, &revision) && revision == 2);
	const auto proof = player_checkpoint_ram_death(1, 2);
	assert(proof && proof->epoch == 124 &&
	       proof->custody.at(204).item.expected_item_revision == 9);
	assert(!player_checkpoint_ram_prepare(snapshot, 125000000, false, &abandoned));
	critical_command command;
	assert(player_checkpoint_command_build(snapshot, &command));
	command.accepted_at_usec = 124000000;
	auto project = [](const player_snapshot &value, void *)
	{
		assert(player_checkpoint_projection_epoch() == 124);
		return player_save_apply_result{ player_save_apply_outcome::applied, value.revision,
						 0 };
	};
	assert(player_checkpoint_command_project(command, project, nullptr).outcome ==
	       critical_apply_outcome::applied);
	assert(player_checkpoint_projection_epoch() == 0);
}
