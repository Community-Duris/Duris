#include "persistence/corpse_ram.h"
#include "economy/collector_ram.h"
#include "economy/collector_runtime.h"
#include "economy/economic_state.h"
#include "item/item_ownership_runtime.h"
#include "player/artifact_restitution_ram.h"
#include "core/structs.h"
#include "classes/necromancy.h"

#include <cassert>
#include <cerrno>

P_obj object_list = nullptr;

static critical_command command(const corpse_lifecycle_payload &payload)
{
	critical_command result;
	critical_operation_id id{};
	id.bytes[0] = 1;
	assert(corpse_lifecycle_command_build(&result, id, payload, critical_source_site::command,
					      critical_deadline_class::terminal));
	result.accepted_at_usec = 1000000000;
	return result;
}
static corpse_lifecycle_result prepare(const corpse_lifecycle_payload &payload)
{
	corpse_ram_begin();
	collector_ram_begin();
	const auto prepared = corpse_ram_prepare(command(payload));
	assert(prepared.outcome == critical_apply_outcome::applied && !prepared.error_code);
	corpse_lifecycle_result result;
	assert(corpse_lifecycle_command_decode_result(prepared.result_payload.data(),
						      prepared.result_size, &result));
	return result;
}

int main()
{
	economic_state_reset();
	item_ownership_runtime_reset();
	collector_runtime_reset();
	assert(collector_runtime_rebuild({ 1, 1, {} }));
	corpse_ram_reset(5, false);
	corpse_lifecycle_payload corpse;
	corpse.owner_pid = 42;
	corpse.save_id = 100;
	corpse.owner_name = "Tester";
	corpse.room_vnum = 300;
	corpse.values[CORPSE_PID] = 42;
	corpse.values[CORPSE_SAVEID] = 100;
	corpse.values[CORPSE_RACEWAR] = 1;
	corpse.short_description = "a corpse";
	corpse.description = "A corpse lies here.";
	corpse.keywords = "corpse";
	auto created = prepare(corpse);
	assert(created.corpse_revision == 1 && created.catalog_revision == 6);
	// Abandoning admission changes neither the metadata revision nor the catalog.
	auto retried = prepare(corpse);
	assert(retried.corpse_revision == 1 && retried.catalog_revision == 6);
	corpse_ram_publish();
	corpse_ram_begin();
	collector_ram_begin();
	assert(corpse_ram_prepare(command(corpse)).error_code == ESTALE);

	const uint64_t corpse_key = item_corpse_owner_id(42, 100);
	const item_owner_identity owner{ item_owner_type::corpse, corpse_key, 0 };
	const item_owner_identity player{ item_owner_type::player, 42, 0 };
	const item_owner_identity room{ item_owner_type::room, 300, 0 };
	const item_owner_identity destruction{ item_owner_type::destruction, 0, 0 };
	assert(item_ownership_runtime_hydrate_owner(player, 8));
	assert(item_ownership_runtime_hydrate_owner(room, 9));
	assert(item_ownership_runtime_hydrate_owner(destruction, 10));
	assert(item_ownership_runtime_hydrate(
		{ 1001, 1001, 0, owner, 3, 4, 2001, item_custody_state::active }));
	assert(item_ownership_runtime_hydrate(
		{ 1002, 1002, 0, owner, 4, 4, 2002, item_custody_state::active }));
	obj_data body{}, item{}, coins{};
	body.type = ITEM_CORPSE;
	body.value[CORPSE_PID] = 42;
	body.value[CORPSE_SAVEID] = 100;
	item.obj_uid = 1001;
	item.type = ITEM_OTHER;
	coins.obj_uid = 1002;
	coins.type = ITEM_MONEY;
	coins.value[2] = 5;
	body.contains = &item;
	item.next_content = &coins;
	object_list = &body;
	body.next = &item;
	item.next = &coins;
	assert(economic_state_seed(
		{ 42, "tester", "Tester", 1, { { { 0, 0, 1, 0 } }, {}, 7, 2 } }));
	corpse.values = {};
	corpse.short_description.clear();
	corpse.description.clear();
	corpse.keywords.clear();
	corpse.action = corpse_lifecycle_action::resurrect;
	corpse.expected_corpse_revision = 1;
	corpse.expected_room_revision = 9;
	corpse.old_room_vnum = 300;
	corpse.destination_player_pid = 42;
	corpse.expected_player_revision = 8;
	corpse.expected_wallet_revision = 7;
	corpse.money[2] = 1;
	// Reject a corpse artifact without matching durable evidence before append.
	item.extra_flags |= ITEM_ARTIFACT;
	corpse_ram_begin();
	collector_ram_begin();
	assert(corpse_ram_prepare(command(corpse)).error_code == ESTALE);
	artifact_restitution_state artifact;
	artifact.vnum = 2001;
	artifact.domain = artifact.god = artifact.mortal = { true, true, 1, 5, 42, 9000 };
	artifact.uid_present = artifact.item_revision_present = true;
	artifact.uid = 1001;
	artifact.item_revision = 3;
	artifact.revision = 12;
	assert(artifact_restitution_ram_seed(artifact));
	auto restored = prepare(corpse);
	assert(artifact_restitution_ram_lookup(2001)->revision == 12);
	// An abandoned preparation cannot publish the new location.
	restored = prepare(corpse);
	assert(restored.corpse_owner_revision == 6 && restored.player_owner_revision == 9 &&
	       restored.room_owner_revision == 10);
	assert(restored.item_count == 1 && restored.max_item_revision == 4);
	assert(restored.discarded_item_count == 1 && restored.max_discarded_item_revision == 5 &&
	       restored.destruction_owner_revision == 11);
	assert(restored.wallet[2] == 5 && restored.wallet_revision == 8 &&
	       restored.bank_revision == 3);
	assert(item_ownership_runtime_apply_corpse_discarded(42, 100, { 1002 }, restored));
	assert(item_ownership_runtime_apply_corpse_resurrection(42, 100, 42, 300, restored));
	corpse_ram_publish();
	const auto recovered_artifact = artifact_restitution_ram_lookup(2001);
	assert(recovered_artifact->revision == 13 && recovered_artifact->item_revision == 4 &&
	       recovered_artifact->god.location_type == 3 &&
	       recovered_artifact->mortal.location_type == 3 &&
	       recovered_artifact->domain.location == 42);
	corpse_ram_begin();
	collector_ram_begin();
	assert(corpse_ram_prepare(command(corpse)).error_code == ESTALE);
	item_ownership_runtime_entry final;
	assert(item_ownership_runtime_lookup(1001, &final) &&
	       final.owner.type == item_owner_type::player);
	assert(item_ownership_runtime_lookup(1002, &final) &&
	       final.state == item_custody_state::destroyed);
}
