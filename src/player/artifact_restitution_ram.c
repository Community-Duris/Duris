#include "player/artifact_restitution_ram.h"
#include <map>

namespace
{
std::map<uint32_t, artifact_restitution_state> states, staged;
bool matches(const artifact_restitution_location &location,
	     const player_death_restitution_item &item)
{
	return location.owned && location.type == item.artifact_type &&
	       location.location_type == item.artifact_source_location_type &&
	       location.location == item.artifact_source_location && location.timer >= 0 &&
	       static_cast<uint64_t>(location.timer) == item.artifact_source_timer_epoch;
}
}
bool artifact_restitution_ram_seed(const artifact_restitution_state &state)
{
	return state.vnum && states.emplace(state.vnum, state).second;
}
const artifact_restitution_state *artifact_restitution_ram_lookup(uint32_t vnum)
{
	const auto found = states.find(vnum);
	return found == states.end() ? nullptr : &found->second;
}
bool artifact_restitution_ram_prepare(const player_death_restitution_plan &plan, uint64_t epoch)
{
	staged.clear();
	for (const auto &item : plan.items)
	{
		if (!item.artifact_vnum ||
		    item.disposition != player_death_restitution_disposition::deliver)
			continue;
		const auto current = artifact_restitution_ram_lookup(item.artifact_vnum);
		// Reserve an absent domain before append, without making it present. The
		// SQL transition can adopt legacy-only artifacts as well as canonical ones.
		artifact_restitution_state empty{};
		empty.vnum = item.artifact_vnum;
		const auto &s = current ? *current : empty;
		const uint8_t mask =
			(s.god.present ? PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_GOD : 0) |
			(s.mortal.present ? PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL : 0);
		if (s.domain.present != item.artifact_domain_present ||
		    (s.domain.present &&
		     (!matches(s.domain, item) ||
		      s.uid_present != item.artifact_domain_item_uid_present ||
		      (s.uid_present && s.uid != item.artifact_domain_item_uid) ||
		      s.item_revision != item.artifact_domain_item_revision ||
		      s.revision != item.artifact_domain_revision ||
		      s.bind_owner != item.artifact_bind_owner_pid ||
		      s.bind_timer != item.artifact_bind_timer_epoch)) ||
		    s.revision == UINT64_MAX ||
		    s.baseline_present != item.artifact_baseline_present ||
		    (s.baseline_present &&
		     (s.opening_timer < 0 ||
		      static_cast<uint64_t>(s.opening_timer) !=
			      item.artifact_baseline_opening_timer_epoch ||
		      s.opening_bind_owner != item.artifact_baseline_opening_bind_owner_pid ||
		      s.opening_bind_timer != item.artifact_baseline_opening_bind_timer_epoch ||
		      s.opening_revision != item.artifact_baseline_opening_revision)) ||
		    s.bind_present != item.artifact_bind_present ||
		    (s.bind_present && (s.bind_row_owner != item.artifact_bind_owner_pid ||
					s.bind_row_timer != item.artifact_bind_timer_epoch)) ||
		    mask != item.artifact_legacy_projection_mask ||
		    (s.god.present && !matches(s.god, item)) ||
		    (s.mortal.present && !matches(s.mortal, item)))
			return false;
		uint64_t timer = 0;
		if (!player_death_restitution_artifact_delivery_timer(item, epoch, &timer) ||
		    timer > INT64_MAX)
			return false;
		auto next = s;
		if (!next.baseline_present && !(s.domain.present && s.uid_present))
		{
			next.baseline_present = true;
			next.opening_timer = item.artifact_source_timer_epoch;
			next.opening_bind_owner = item.artifact_bind_owner_pid;
			next.opening_bind_timer = item.artifact_bind_timer_epoch;
			next.opening_revision = 0;
		}
		next.domain = { true,
				true,
				item.artifact_type,
				PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER,
				static_cast<int32_t>(plan.recipient_pid),
				static_cast<int64_t>(timer) };
		if (next.god.present)
			next.god = next.domain;
		if (next.mortal.present)
			next.mortal = next.domain;
		next.uid_present = true;
		next.uid = item.item_uid;
		next.item_revision = item.expected_item_revision + 1;
		next.item_revision_present = true;
		next.revision = s.domain.present ? s.revision + 1 : 1;
		next.bind_owner = item.artifact_bind_owner_pid;
		next.bind_timer = item.artifact_bind_timer_epoch;
		if (!staged.emplace(item.artifact_vnum, next).second)
			return false;
		states.try_emplace(item.artifact_vnum, empty);
	}
	return true;
}
void artifact_restitution_ram_publish()
{
	for (const auto &[vnum, next] : staged)
		states.find(vnum)->second = next;
	staged.clear();
}
void artifact_restitution_ram_feed(uint32_t vnum, int64_t timer, int32_t owner, int64_t bind_timer,
				   uint64_t revision)
{
	const auto found = states.find(vnum);
	if (found == states.end())
		return;
	auto &s = found->second;
	s.domain.timer = timer;
	s.bind_owner = owner;
	s.bind_timer = bind_timer;
	s.revision = revision;
	if (s.god.present)
		s.god.timer = timer;
	s.bind_present = true;
	s.bind_row_owner = owner;
	s.bind_row_timer = bind_timer;
}
void artifact_restitution_ram_reset()
{
	states.clear();
	staged.clear();
}

void artifact_restitution_ram_update(const artifact_restitution_state &state) noexcept
{
	const auto found = states.find(state.vnum);
	if (found != states.end())
		found->second = state;
}
