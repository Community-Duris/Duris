#include "player/player_checkpoint_ram.h"
#include "player/player_snapshot_codec.h"
#include <openssl/sha.h>
#include <set>

namespace
{
std::map<uint32_t, uint64_t> revisions;
std::map<std::pair<uint32_t, uint64_t>, player_death_ram_evidence> deaths;
}

bool player_checkpoint_ram_prepare(const player_snapshot &snapshot, uint64_t accepted_at_usec,
				   bool flatfile, player_checkpoint_ram_change *change)
{
	if (!change || snapshot.pid <= 0 || !snapshot.revision || !accepted_at_usec)
		return false;
	*change = {};
	change->pid = snapshot.pid;
	change->revision = snapshot.revision;
	const auto current = revisions.try_emplace(snapshot.pid, 0).first;
	if (snapshot.revision <= current->second)
		return false;
	if (!snapshot.death)
		return true;
	change->has_death = true;
	auto &proof = change->death;
	proof.operation_id = snapshot.death->operation_id;
	proof.epoch = accepted_at_usec / 1000000;
	std::vector<uint8_t> encoded;
	if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
		return false;
	SHA256(encoded.data(), encoded.size(), proof.digest.data());
	std::set<uint64_t> captured, roots;
	for (const auto &row : snapshot.death->custody)
	{
		if (!proof.custody.emplace(row.item.item_uid, row).second)
			return false;
		captured.insert(row.item.item_uid);
		if (!flatfile)
			roots.insert(row.item.root_item_uid);
	}
	const item_owner_identity owner{ item_owner_type::player,
					 static_cast<uint64_t>(snapshot.pid), 0 };
	uint64_t revision = 0;
	std::vector<item_ownership_runtime_entry> items;
	if (!item_ownership_runtime_owner_revision(owner, &revision) ||
	    !item_ownership_runtime_snapshot_owner(owner, SIZE_MAX, &items))
		return false;
	if (flatfile)
		for (const auto &item : items)
			if (item.state == item_custody_state::active &&
			    captured.contains(item.item_uid))
				roots.insert(item.root_item_uid);
	for (const auto &item : items)
	{
		if (item.state != item_custody_state::active ||
		    (!captured.contains(item.item_uid) && !roots.contains(item.root_item_uid)))
			continue;
		if (item.item_revision == UINT64_MAX || revision == UINT64_MAX)
			return false;
		proof.custody.try_emplace(
			item.item_uid,
			player_death_custody_snapshot{ { item.item_uid, item.root_item_uid,
							 item.parent_item_uid, item.item_revision,
							 item.vnum, item.state },
						       owner,
						       revision });
		change->custody.before.push_back(item);
		auto next = item;
		++next.item_revision;
		next.owner_revision = revision + 1;
		next.state = item_custody_state::quarantined;
		change->custody.after.push_back(next);
	}
	if (!change->custody.before.empty())
		change->custody.owners.emplace_back(owner, revision + 1);
	// Allocate the destination node before append; an uncommitted placeholder has
	// no epoch and is never exposed as recovery evidence.
	if (!deaths.try_emplace({ snapshot.pid, snapshot.revision }).second)
		if (deaths.at({ snapshot.pid, snapshot.revision }).epoch)
			return false;
	return item_ownership_runtime_patch_ready(change->custody);
}

bool player_checkpoint_ram_publish(player_checkpoint_ram_change *change)
{
	if (!change)
		return false;
	const auto found = revisions.find(change->pid);
	const auto death = deaths.find({ change->pid, change->revision });
	if (found == revisions.end() || (change->has_death && death == deaths.end()) ||
	    !item_ownership_runtime_patch_publish(change->custody))
		return false;
	found->second = change->revision;
	if (change->has_death)
	{
		std::swap(death->second, change->death);
	}
	return true;
}

bool player_checkpoint_ram_artifact_unique(int32_t vnum, uint64_t uid)
{
	for (const auto &[key, death] : deaths)
		for (const auto &[other_uid, evidence] : death.custody)
			if (other_uid != uid && evidence.item.vnum == vnum &&
			    evidence.item.expected_state != item_custody_state::destroyed)
			{
				item_ownership_runtime_entry current{};
				if (!item_ownership_runtime_lookup(other_uid, &current) ||
				    current.state != item_custody_state::destroyed)
					return false;
			}
	return true;
}

bool player_checkpoint_ram_seed_revision(uint32_t pid, uint64_t revision)
{
	return pid && revisions.emplace(pid, revision).second;
}
bool player_checkpoint_ram_seed_death(uint32_t pid, uint64_t revision,
				      player_death_ram_evidence evidence)
{
	return pid && revision && evidence.epoch &&
	       deaths.emplace(std::make_pair(pid, revision), std::move(evidence)).second;
}
bool player_checkpoint_ram_revision(uint32_t pid, uint64_t *revision)
{
	const auto found = revisions.find(pid);
	if (!revision || found == revisions.end())
		return false;
	*revision = found->second;
	return true;
}
const player_death_ram_evidence *player_checkpoint_ram_death(uint32_t pid, uint64_t revision)
{
	const auto found = deaths.find({ pid, revision });
	return found == deaths.end() || !found->second.epoch ? nullptr : &found->second;
}
void player_checkpoint_ram_reset()
{
	revisions.clear();
	deaths.clear();
}
