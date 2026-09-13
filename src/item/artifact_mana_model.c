#include "item/artifact_mana_model.h"

#include <algorithm>
#include <limits>

bool artifact_mana_valid(const artifact_mana_profile &profile)
{
	return profile.id && profile.revision && profile.capacity &&
	       profile.capacity <= ARTIFACT_MANA_MAXIMUM &&
	       profile.regeneration <= ARTIFACT_MANA_MAX_RATE &&
	       profile.passive_floor <= profile.capacity;
}

bool artifact_mana_valid(const artifact_mana_record &record)
{
	return record.uid && record.profile_id && record.profile_revision && record.version &&
	       record.capacity && record.capacity <= ARTIFACT_MANA_MAXIMUM &&
	       record.regeneration <= ARTIFACT_MANA_MAX_RATE && record.reserve <= record.capacity;
}

artifact_mana_record artifact_mana_empty(uint64_t uid, const artifact_mana_profile &profile,
					 uint64_t now)
{
	if (!uid || !artifact_mana_valid(profile))
		return {};
	return { uid, profile.id, profile.revision, 1, profile.capacity, profile.regeneration,
		 0,   now };
}

bool artifact_mana_project(artifact_mana_record &record, const artifact_mana_profile &profile,
			   uint64_t now)
{
	if (!artifact_mana_valid(record) || !artifact_mana_valid(profile) ||
	    record.profile_id != profile.id || profile.revision < record.profile_revision ||
	    (profile.revision == record.profile_revision &&
	     (record.capacity != profile.capacity || record.regeneration != profile.regeneration)))
		return false;
	const uint64_t elapsed =
		now > record.settled_at ?
			std::min(now - record.settled_at, ARTIFACT_MANA_MAX_OFFLINE_SECONDS) :
			0;
	record.reserve += std::min(record.capacity - record.reserve, elapsed * record.regeneration);
	record.settled_at = std::max(record.settled_at, now);
	record.reserve = std::min(record.reserve, profile.capacity);
	record.capacity = profile.capacity;
	record.regeneration = profile.regeneration;
	record.profile_revision = profile.revision;
	return true;
}

bool artifact_mana_spend(artifact_mana_record &record, const artifact_mana_profile &profile,
			 uint64_t now, uint64_t cost, bool passive)
{
	auto next = record;
	if (!cost || cost > ARTIFACT_MANA_MAXIMUM ||
	    next.version == std::numeric_limits<uint64_t>::max() ||
	    !artifact_mana_project(next, profile, now) || cost > next.reserve ||
	    (passive && next.reserve - cost < profile.passive_floor))
		return false;
	next.reserve -= cost;
	++next.version;
	record = next;
	return true;
}
