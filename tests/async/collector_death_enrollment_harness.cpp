#include "economy/collector_config.h"
#include "economy/collector_death_enrollment.h"
#include "economy/collector_storage.h"

#include "classes/necromancy.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <vector>

namespace
{
collector_feature_config config;
bool cache_ready = false;
collector_death_snapshot durable_death;
bool durable_death_present = false;

critical_operation_id operation(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0xa5;
	id.bytes.back() = discriminator;
	return id;
}

player_item_snapshot item(uint64_t uid)
{
	player_item_snapshot value = {};
	value.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	value.object_uid = uid;
	value.vnum = static_cast<int32_t>(500 + uid);
	value.type = ITEM_WEAPON;
	value.name = "ancient blade";
	value.wear_flags = ITEM_TAKE;
	return value;
}

item_transfer_payload transfer(uint32_t save_id, const std::vector<player_item_snapshot> &snapshots)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::player, 42, 0 };
	payload.to_owner = { item_owner_type::corpse, item_corpse_owner_id(42, save_id), 0 };
	payload.reason = item_transfer_reason::corpse_create;
	payload.multi_root = true;
	payload.item_count = static_cast<uint16_t>(snapshots.size());
	for (size_t index = 0; index < snapshots.size(); ++index)
		payload.items[index] = {
			snapshots[index].object_uid, snapshots[index].object_uid, 0, 1,
			snapshots[index].vnum,	     item_custody_state::active
		};
	return payload;
}
}

const collector_feature_config *collector_config_get(void)
{
	return &config;
}

bool collector_config_enabled(void)
{
	return config.policy.enabled;
}

bool collector_catalog_cache_ready(void)
{
	return cache_ready;
}

bool collector_runtime_find_death(uint32_t beneficiary_pid, uint64_t death_time,
				  collector_death_snapshot *death)
{
	if (!death || !durable_death_present || durable_death.beneficiary_pid != beneficiary_pid ||
	    durable_death.death_time != death_time)
		return false;
	*death = durable_death;
	return true;
}

void logit(const char *, const char *, ...) {}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main()
{
	char_data character = {};
	pc_only_data player = {};
	obj_data corpse = {};
	character.only.pc = &player;
	player.pid = 42;
	corpse.obj_uid = 900;
	corpse.type = ITEM_CORPSE;
	corpse.value[CORPSE_FLAGS] = PC_CORPSE;
	corpse.value[CORPSE_PID] = 42;
	corpse.value[CORPSE_SAVEID] = 1700000000;

	config.policy.enabled = true;
	config.policy.collection_delay = 10;
	config.policy.sale_delay = 20;
	config.policy.holding_duration = 30;
	config.policy.price_percent = 250;
	config.policy.minimum_value = 7;

	std::vector<player_item_snapshot> snapshots;
	for (uint64_t uid = 1; uid <= 10; ++uid)
		snapshots.push_back(item(uid));
	snapshots[1].type = ITEM_MONEY;
	snapshots[2].type = ITEM_CORPSE;
	snapshots[3].extra_flags = ITEM_ARTIFACT;
	snapshots[4].name = "ancient unique blade";
	snapshots[5].extra_flags = ITEM_TRANSIENT;
	snapshots[6].extra_flags = ITEM_NORENT;
	snapshots[7].extra_flags = ITEM_NOSELL;
	snapshots[8].extra2_flags = ITEM2_ACCOUNT_BOUND;
	snapshots[9].wear_flags = 0;
	assert(collector_death_item_snapshot_eligible(snapshots[0]));
	for (size_t index = 1; index < snapshots.size(); ++index)
		assert(!collector_death_item_snapshot_eligible(snapshots[index]));
	auto permitted_unique = item(11);
	permitted_unique.name = "ancient powerunique blade";
	assert(collector_death_item_snapshot_eligible(permitted_unique));

	collector_death_enrollment_reset_for_tests();
	collector_death_enrollment_begin(&character, &corpse);
	config.policy.collection_delay = 999;
	item_transfer_payload payload = transfer(corpse.value[CORPSE_SAVEID], snapshots);
	assert(collector_death_enrollment_attach(&character, &corpse, operation(1), snapshots,
						 &payload));
	assert(payload.collector.present && payload.collector.beneficiary_pid == 42 &&
	       payload.collector.death_time == 1700000000 &&
	       payload.collector.policy.collection_delay == 10 &&
	       payload.collector.policy.sale_delay == 20 &&
	       payload.collector.eligible_item_uids == std::vector<uint64_t>{ 1 });
	collector_death_enrollment_note_submitted(&corpse, payload);

	std::vector<player_item_snapshot> next_snapshots = { permitted_unique };
	item_transfer_payload next = transfer(corpse.value[CORPSE_SAVEID], next_snapshots);
	assert(collector_death_enrollment_attach(&character, &corpse, operation(2), next_snapshots,
						 &next));
	assert(next.collector.present &&
	       critical_operation_id_equal(next.collector.death_operation,
					   payload.collector.death_operation));

	// A process restart loses the transient map. Once the authoritative catalog
	// is ready, the stable corpse identity restores the frozen policy and first
	// accepted operation instead of treating the remaining batch as a new death.
	durable_death.operation_id = payload.collector.death_operation;
	durable_death.beneficiary_pid = payload.collector.beneficiary_pid;
	durable_death.death_time = payload.collector.death_time;
	durable_death.policy = config.policy;
	durable_death.policy.collection_delay = payload.collector.policy.collection_delay;
	durable_death.policy.sale_delay = payload.collector.policy.sale_delay;
	durable_death.policy.holding_duration = payload.collector.policy.holding_duration;
	durable_death.policy.price_percent = payload.collector.policy.price_percent;
	durable_death.policy.minimum_value = payload.collector.policy.minimum_value;
	durable_death_present = true;
	cache_ready = true;
	collector_death_enrollment_reset_for_tests();
	assert(collector_death_enrollment_resume(&character, &corpse) ==
	       collector_death_enrollment_resume_result::ready);
	item_transfer_payload resumed = transfer(corpse.value[CORPSE_SAVEID], next_snapshots);
	assert(collector_death_enrollment_attach(&character, &corpse, operation(7), next_snapshots,
						 &resumed));
	assert(resumed.collector.present &&
	       critical_operation_id_equal(resumed.collector.death_operation,
					   payload.collector.death_operation) &&
	       resumed.collector.policy.collection_delay == 10);

	// Disabling is an immediate intake fence, while a death that began disabled
	// cannot be admitted merely because the feature is later enabled.
	config.policy.enabled = false;
	item_transfer_payload disabled = transfer(corpse.value[CORPSE_SAVEID], next_snapshots);
	assert(collector_death_enrollment_attach(&character, &corpse, operation(3), next_snapshots,
						 &disabled));
	assert(!disabled.collector.present);
	collector_death_enrollment_end(&corpse);
	config.policy.enabled = true;
	assert(collector_death_enrollment_attach(&character, &corpse, operation(4), next_snapshots,
						 &disabled));
	assert(!disabled.collector.present);

	collector_death_enrollment_reset_for_tests();
	durable_death_present = false;
	config.policy.enabled = false;
	collector_death_enrollment_begin(&character, &corpse);
	config.policy.enabled = true;
	item_transfer_payload late = transfer(corpse.value[CORPSE_SAVEID], next_snapshots);
	assert(collector_death_enrollment_attach(&character, &corpse, operation(5), next_snapshots,
						 &late));
	assert(!late.collector.present);

	collector_death_enrollment_reset_for_tests();
	config.policy.enabled = true;
	collector_death_enrollment_begin(&character, &corpse);
	std::vector<player_item_snapshot> excluded_only = { snapshots[1] };
	item_transfer_payload empty = transfer(corpse.value[CORPSE_SAVEID], excluded_only);
	assert(collector_death_enrollment_attach(&character, &corpse, operation(8), excluded_only,
						 &empty));
	assert(empty.collector.present && empty.collector.eligible_item_uids.empty());
	auto mismatched = next_snapshots;
	mismatched[0].object_uid = 99;
	item_transfer_payload invalid = transfer(corpse.value[CORPSE_SAVEID], next_snapshots);
	assert(!collector_death_enrollment_attach(&character, &corpse, operation(6), mismatched,
						  &invalid));
	return 0;
}
