#include "economy/collector_death_enrollment.h"

#include "classes/necromancy.h"
#include "core/defines.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_config.h"
#include "economy/collector_runtime.h"

#include <algorithm>
#include <new>
#include <unordered_map>
#include <utility>

namespace
{
constexpr size_t PENDING_DEATH_MAX = 1024;

struct pending_death
{
	uint32_t beneficiary_pid = 0;
	uint32_t corpse_save_id = 0;
	collector::rules policy = {};
	critical_operation_id death_operation = {};
};

std::unordered_map<uint64_t, pending_death> pending;

uint64_t death_key(P_char character, P_obj corpse)
{
	if (!character || !corpse || !IS_PC(character) || GET_PID(character) <= 0 ||
	    GET_ITEM_TYPE(corpse) != ITEM_CORPSE ||
	    !IS_SET(corpse->value[CORPSE_FLAGS], PC_CORPSE) ||
	    corpse->value[CORPSE_PID] != GET_PID(character) || corpse->value[CORPSE_SAVEID] <= 0)
		return 0;
	return item_corpse_owner_id(static_cast<uint32_t>(GET_PID(character)),
				    static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]));
}

uint64_t death_key(P_obj corpse)
{
	if (!corpse || GET_ITEM_TYPE(corpse) != ITEM_CORPSE ||
	    !IS_SET(corpse->value[CORPSE_FLAGS], PC_CORPSE) || corpse->value[CORPSE_PID] <= 0 ||
	    corpse->value[CORPSE_SAVEID] <= 0)
		return 0;
	return item_corpse_owner_id(static_cast<uint32_t>(corpse->value[CORPSE_PID]),
				    static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]));
}

item_collector_death_policy captured_policy(const collector::rules &policy)
{
	return { policy.collection_delay, policy.sale_delay, policy.holding_duration,
		 policy.price_percent, policy.minimum_value };
}
}

void collector_death_enrollment_begin(P_char character, P_obj corpse)
{
	const uint64_t key = death_key(character, corpse);
	if (!key || !corpse->obj_uid)
		return;
	const collector_feature_config *config = collector_config_get();
	if (!config->policy.enabled)
		return;
	if (pending.size() >= PENDING_DEATH_MAX && !pending.contains(key))
	{
		logit(LOG_STATUS,
		      "Collector death intake saturated; corpse %llu will not be enrolled.",
		      static_cast<unsigned long long>(corpse->obj_uid));
		return;
	}
	try
	{
		pending[key] = { static_cast<uint32_t>(GET_PID(character)),
				 static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]),
				 config->policy,
				 {} };
	}
	catch (const std::bad_alloc &)
	{
		logit(LOG_STATUS,
		      "Collector death intake allocation failed; corpse %llu will not be enrolled.",
		      static_cast<unsigned long long>(corpse->obj_uid));
	}
}

void collector_death_enrollment_end(P_obj corpse)
{
	const uint64_t key = death_key(corpse);
	if (key)
		pending.erase(key);
}

collector_death_enrollment_resume_result collector_death_enrollment_resume(P_char character,
									   P_obj corpse)
{
	const uint64_t key = death_key(character, corpse);
	if (!key)
		return collector_death_enrollment_resume_result::invalid;
	if (pending.contains(key))
		return collector_death_enrollment_resume_result::ready;
	if (!collector_config_enabled())
		return collector_death_enrollment_resume_result::outside;
	if (!collector_catalog_cache_ready())
		return collector_death_enrollment_resume_result::unavailable;
	collector_death_snapshot durable;
	if (!collector_runtime_find_death(static_cast<uint32_t>(GET_PID(character)),
					  static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]),
					  &durable))
		return collector_death_enrollment_resume_result::outside;
	if (durable.beneficiary_pid != static_cast<uint32_t>(GET_PID(character)) ||
	    durable.death_time != static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]) ||
	    !durable.policy.enabled || !collector::valid_rules(durable.policy) ||
	    critical_operation_id_is_zero(durable.operation_id))
		return collector_death_enrollment_resume_result::invalid;
	if (pending.size() >= PENDING_DEATH_MAX)
		return collector_death_enrollment_resume_result::unavailable;
	try
	{
		pending.emplace(key, pending_death{ durable.beneficiary_pid,
						    static_cast<uint32_t>(durable.death_time),
						    durable.policy, durable.operation_id });
	}
	catch (const std::bad_alloc &)
	{
		return collector_death_enrollment_resume_result::unavailable;
	}
	return collector_death_enrollment_resume_result::ready;
}

bool collector_death_enrollment_attach(P_char character, P_obj corpse,
				       const critical_operation_id &proposed_operation,
				       const std::vector<player_item_snapshot> &snapshots,
				       item_transfer_payload *payload)
{
	if (!payload || payload->reason != item_transfer_reason::corpse_create)
		return true;
	if (!character || !corpse || critical_operation_id_is_zero(proposed_operation))
		return false;
	if (!collector_config_enabled())
		return true;
	const uint64_t key = death_key(character, corpse);
	if (!key)
		return false;
	const auto found = pending.find(key);
	if (found == pending.end())
		return true;
	const pending_death &death = found->second;
	if (death.beneficiary_pid != static_cast<uint32_t>(GET_PID(character)) ||
	    death.corpse_save_id != static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]) ||
	    payload->to_owner.type != item_owner_type::corpse ||
	    payload->to_owner.id !=
		    item_corpse_owner_id(death.beneficiary_pid, death.corpse_save_id) ||
	    snapshots.size() != payload->item_count)
		return false;
	item_collector_death_enrollment enrollment;
	try
	{
		enrollment.eligible_item_uids.reserve(snapshots.size());
		for (const player_item_snapshot &snapshot : snapshots)
		{
			const auto authority = std::lower_bound(
				payload->items.begin(),
				payload->items.begin() + payload->item_count, snapshot.object_uid,
				[](const item_transfer_entry &item, uint64_t uid)
				{ return item.item_uid < uid; });
			if (authority == payload->items.begin() + payload->item_count ||
			    authority->item_uid != snapshot.object_uid)
				return false;
			if (collector_death_item_snapshot_eligible(snapshot))
				enrollment.eligible_item_uids.push_back(snapshot.object_uid);
		}
		std::sort(enrollment.eligible_item_uids.begin(),
			  enrollment.eligible_item_uids.end());
		if (std::adjacent_find(enrollment.eligible_item_uids.begin(),
				       enrollment.eligible_item_uids.end()) !=
		    enrollment.eligible_item_uids.end())
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	enrollment.present = true;
	enrollment.death_operation = critical_operation_id_is_zero(death.death_operation) ?
					     proposed_operation :
					     death.death_operation;
	enrollment.beneficiary_pid = death.beneficiary_pid;
	enrollment.death_time = death.corpse_save_id;
	enrollment.policy = captured_policy(death.policy);
	payload->collector = std::move(enrollment);
	return true;
}

void collector_death_enrollment_note_committed(P_obj corpse, const item_transfer_payload &payload)
{
	if (!corpse || !payload.collector.present)
		return;
	const uint64_t key = death_key(corpse);
	if (!key)
		return;
	const auto found = pending.find(key);
	if (found == pending.end())
		return;
	if (critical_operation_id_is_zero(found->second.death_operation))
		found->second.death_operation = payload.collector.death_operation;
}

void collector_death_enrollment_reset_for_tests(void)
{
	pending.clear();
}
