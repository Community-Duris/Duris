#include "flatfile/flatfile_player_repository.h"

#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_observability.h"
#include "persistence/persistence_mode.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <numeric>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
using namespace flatfile_player_snapshot_file;
std::mutex player_mutex;

struct encoder
{
	std::vector<uint8_t> bytes;

	template <typename T> void number(T value)
	{
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		for (size_t index = 0; index < sizeof(T); ++index)
		{
			bytes.push_back(static_cast<uint8_t>(bits & 0xff));
			bits >>= 8;
		}
	}
};

std::string player_lock_filename(int32_t pid)
{
	return ".player-" + std::to_string(pid) + ".lock";
}

bool valid_snapshot(const player_snapshot &snapshot)
{
	const uint32_t required = snapshot.death ? PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION :
						   PLAYER_SNAPSHOT_SCHEMA_VERSION;
	return snapshot.schema_version == required && snapshot.pid > 0 && snapshot.revision &&
	       snapshot.components && !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
	       snapshot.encoded_size_bound &&
	       snapshot.encoded_size_bound <= PLAYER_SNAPSHOT_MAX_BYTES &&
	       (!snapshot.death || !snapshot.death->corpse.empty());
}

bool same_authority_key(const std::string &left, const std::string &right)
{
	if (left.size() != right.size())
		return false;
	for (size_t index = 0; index < left.size(); ++index)
	{
		unsigned char left_character = left[index];
		unsigned char right_character = right[index];
		if (left_character >= 'A' && left_character <= 'Z')
			left_character = static_cast<unsigned char>(left_character - 'A' + 'a');
		if (right_character >= 'A' && right_character <= 'Z')
			right_character = static_cast<unsigned char>(right_character - 'A' + 'a');
		if (left_character != right_character)
			return false;
	}
	return true;
}

const std::string *snapshot_player_name(const player_snapshot &snapshot)
{
	const std::string *name = nullptr;
	for (const player_snapshot_string &entry : snapshot.status_strings)
		if (entry.field == player_status_string_field::name)
		{
			if (name)
				return nullptr;
			name = &entry.value;
		}
	return name;
}

bool snapshot_unsigned(const player_snapshot &snapshot, player_status_field field, uint64_t *value)
{
	bool found = false;
	for (const player_snapshot_integer &entry : snapshot.status_integers)
		if (entry.field == field)
		{
			if (found || (!entry.is_unsigned && entry.signed_value < 0))
				return false;
			*value = entry.is_unsigned ? entry.unsigned_value :
						     static_cast<uint64_t>(entry.signed_value);
			found = true;
		}
	return found;
}

bool snapshot_signed(const player_snapshot &snapshot, player_status_field field, int64_t *value)
{
	bool found = false;
	for (const player_snapshot_integer &entry : snapshot.status_integers)
		if (entry.field == field)
		{
			if (found || (entry.is_unsigned && entry.unsigned_value > INT64_MAX))
				return false;
			*value = entry.is_unsigned ? static_cast<int64_t>(entry.unsigned_value) :
						     entry.signed_value;
			found = true;
		}
	return found;
}

player_load_result identity_failure(const player_load_request &request,
				    flatfile_identity_result failure)
{
	player_load_result result = {};
	result.request_id = request.request_id;
	result.pid = request.pid;
	result.failed_component = "identity";
	switch (failure)
	{
	case flatfile_identity_result::not_found:
		result.outcome = player_load_outcome::not_found;
		result.error_code = ENOENT;
		break;
	case flatfile_identity_result::io_error:
		result.outcome = player_load_outcome::retryable_failure;
		result.error_code = EIO;
		break;
	case flatfile_identity_result::ok:
	case flatfile_identity_result::conflict:
	case flatfile_identity_result::unchanged:
	case flatfile_identity_result::invalid:
	case flatfile_identity_result::exhausted:
		result.outcome = player_load_outcome::component_failure;
		result.error_code = EILSEQ;
		break;
	}
	return result;
}

// The ownership file is authoritative. A payload item it does not list, or lists as
// somebody else's or as inactive, is one skippable row: refusing it here would make the
// character permanently unloadable over a single inconsistent entry. Skipped rows are
// compacted out and the contents of a skipped container move to the top level.
bool build_item_identities(std::vector<player_item_snapshot> *items,
			   const std::unordered_map<uint64_t, flatfile_item_ownership_record> &owned,
			   const item_owner_identity &owner, uint64_t owner_revision,
			   uint64_t *next_database_id, std::unordered_set<uint64_t> *consumed,
			   std::vector<player_load_item_identity> *identities,
			   player_load_result *result)
{
	if (!items || !next_database_id || !consumed || !identities || !result)
		return false;
	constexpr size_t skipped_index = static_cast<size_t>(-1);
	std::vector<uint64_t> database_ids;
	std::vector<size_t> remap;
	std::vector<player_item_snapshot> kept;
	try
	{
		database_ids.reserve(items->size());
		remap.reserve(items->size());
		kept.reserve(items->size());
		identities->reserve(items->size());
		for (size_t index = 0; index < items->size(); ++index)
		{
			player_item_snapshot item = std::move((*items)[index]);
			if (!item.object_uid || item.vnum <= 0 ||
			    *next_database_id > static_cast<uint64_t>(INT_MAX))
				return false;
			size_t parent_new = skipped_index;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return false;
				parent_new = remap[static_cast<size_t>(item.parent_index)];
			}
			const auto found = owned.find(item.object_uid);
			if (found == owned.end() ||
			    !item_owner_identity_equal(found->second.owner, owner) ||
			    found->second.state != item_custody_state::active)
			{
				remap.push_back(skipped_index);
				++result->stale_item_rows;
				continue;
			}
			if (!consumed->insert(item.object_uid).second)
				return false;
			const flatfile_item_ownership_record &record = found->second;
			uint64_t serialized_parent = 0;
			if (parent_new != skipped_index)
				serialized_parent = database_ids[parent_new];
			else if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
				 !record.parent_item_uid)
				++result->promoted_item_rows;
			if (record.vnum != item.vnum)
				return false;
			item.parent_index = parent_new == skipped_index ?
						    PLAYER_SNAPSHOT_NO_PARENT :
						    static_cast<int32_t>(parent_new);
			const uint64_t database_id = (*next_database_id)++;
			database_ids.push_back(database_id);
			remap.push_back(kept.size());
			identities->push_back({ database_id, serialized_parent, 1,
						PLAYER_LOAD_ITEM_OVERRIDE_ALL, record.item_uid,
						record.root_item_uid, record.parent_item_uid,
						record.owner, record.item_revision, owner_revision,
						record.state });
			kept.push_back(std::move(item));
		}
		*items = std::move(kept);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return player_load_reconcile_item_topology(items, identities, &result->promoted_item_rows,
						   &result->repaired_item_rows);
}

bool reconcile_item_ownership(const std::string &root, player_load_result *result)
{
	if (!result || result->pid <= 0)
		return false;
	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint64_t>(result->pid), 0 };
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> records;
	std::string error;
	flatfile_authority_lock authority;
	if (!authority.acquire(root, &error))
	{
		result->outcome = player_load_outcome::retryable_failure;
		result->error_code = EIO;
		result->failed_component = "item_ownership";
		return false;
	}
	const auto recovered = flatfile_authority_transaction_recover(root, authority, &error);
	if (recovered != flatfile_authority_transaction_result::ok)
	{
		result->outcome = recovered == flatfile_authority_transaction_result::io_error ?
					  player_load_outcome::retryable_failure :
					  player_load_outcome::component_failure;
		result->error_code =
			recovered == flatfile_authority_transaction_result::io_error ? EIO : EILSEQ;
		result->failed_component = "item_ownership";
		return false;
	}
	const flatfile_item_repository_result loaded = flatfile_item_repository_load_owner_locked(
		root, authority, owner, &owner_revision, &records, &error);
	if (loaded != flatfile_item_repository_result::ok)
	{
		result->outcome = loaded == flatfile_item_repository_result::io_error ?
					  player_load_outcome::retryable_failure :
					  player_load_outcome::component_failure;
		result->error_code = loaded == flatfile_item_repository_result::not_found ? ENOENT :
				     loaded == flatfile_item_repository_result::io_error  ? EIO :
											    EILSEQ;
		result->failed_component = "item_ownership";
		return false;
	}
	const auto materialized = flatfile_shop_trade_materialization_reconcile(
		root, authority, static_cast<uint32_t>(result->pid), records, &result->snapshot,
		&error);
	if (materialized != flatfile_shop_trade_materialization_result::ok)
	{
		result->outcome =
			materialized == flatfile_shop_trade_materialization_result::io_error ?
				player_load_outcome::retryable_failure :
				player_load_outcome::component_failure;
		result->error_code =
			materialized == flatfile_shop_trade_materialization_result::io_error ?
				EIO :
				EILSEQ;
		result->failed_component = "shop_trade_materialization";
		return false;
	}
	std::unordered_map<uint64_t, flatfile_item_ownership_record> owned;
	std::unordered_set<uint64_t> consumed;
	try
	{
		owned.reserve(records.size());
		consumed.reserve(records.size());
		for (const auto &record : records)
			if (!owned.emplace(record.item_uid, record).second)
				return false;
		result->pet_identities.resize(result->snapshot.pets.size());
	}
	catch (const std::bad_alloc &)
	{
		result->outcome = player_load_outcome::retryable_failure;
		result->error_code = ENOMEM;
		result->failed_component = "item_ownership";
		return false;
	}
	uint64_t next_database_id = 1;
	if (!build_item_identities(&result->snapshot.items, owned, owner, owner_revision,
				   &next_database_id, &consumed, &result->item_identities, result))
		goto invalid;
	for (size_t index = 0; index < result->snapshot.pets.size(); ++index)
	{
		result->pet_identities[index].database_id = index + 1;
		if (!build_item_identities(&result->snapshot.pets[index].items, owned, owner,
					   owner_revision, &next_database_id, &consumed,
					   &result->pet_identities[index].item_identities, result))
			goto invalid;
	}
	if (consumed.size() > records.size())
		goto invalid;
	// An ownership record whose payload item is gone cannot be rebuilt, but it must not
	// refuse the load either. Preserve it for explicit operator repair; snapshot saves are
	// not allowed to rewrite authoritative custody.
	result->missing_payload_rows = records.size() - consumed.size();
	result->item_owner_revision = owner_revision;
	result->authoritative_item_count = consumed.size();
	return true;

invalid:
	result->outcome = player_load_outcome::component_failure;
	result->error_code = EILSEQ;
	result->failed_component = "item_ownership";
	return false;
}

bool normalize_size(player_snapshot *snapshot, std::vector<uint8_t> *payload)
{
	if (!snapshot || !payload)
		return false;
	snapshot->encoded_size_bound = 1;
	if (player_snapshot_encode(*snapshot, payload) != player_snapshot_codec_result::ok)
		return false;
	snapshot->encoded_size_bound = payload->size();
	return player_snapshot_encode(*snapshot, payload) == player_snapshot_codec_result::ok;
}

bool encode_file(player_snapshot *snapshot, std::vector<uint8_t> *bytes)
{
	std::vector<uint8_t> payload;
	if (!bytes || !normalize_size(snapshot, &payload))
		return false;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.data(), payload.size(), digest);
	encoder out;
	out.bytes.insert(out.bytes.end(), player_magic.begin(), player_magic.end());
	out.number<uint32_t>(player_file_version);
	out.number<uint32_t>(payload.size());
	out.number<int32_t>(snapshot->pid);
	out.number<uint64_t>(snapshot->revision);
	out.number<uint64_t>(snapshot->components);
	out.bytes.insert(out.bytes.end(), digest, digest + sizeof(digest));
	out.bytes.insert(out.bytes.end(), payload.begin(), payload.end());
	if (out.bytes.size() > player_file_maximum)
		return false;
	*bytes = std::move(out.bytes);
	return true;
}

bool replace_items_together(player_component_mask_t components)
{
	const player_component_mask_t items = PLAYER_COMPONENT_EQUIPMENT |
					      PLAYER_COMPONENT_INVENTORY;
	return !(components & items) || (components & items) == items;
}

bool merge_snapshot(const player_snapshot &incoming, player_snapshot *materialized)
{
	if (!materialized || materialized->pid != incoming.pid ||
	    materialized->components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    !replace_items_together(incoming.components))
		return false;
	materialized->revision = incoming.revision;
	materialized->save_intent = incoming.save_intent;
	materialized->room_vnum = incoming.room_vnum;
	materialized->recipes_are_external = incoming.recipes_are_external;
	if (incoming.components & PLAYER_COMPONENT_STATUS)
	{
		materialized->status_integers = incoming.status_integers;
		materialized->status_strings = incoming.status_strings;
		materialized->conditions = incoming.conditions;
		materialized->quest_values = incoming.quest_values;
	}
	if (incoming.components & PLAYER_COMPONENT_LANGUAGES)
		materialized->languages = incoming.languages;
	if (incoming.components & PLAYER_COMPONENT_INTRODUCTIONS)
		materialized->introductions = incoming.introductions;
	if (incoming.components & PLAYER_COMPONENT_TIMERS)
		materialized->timers = incoming.timers;
	if (incoming.components & PLAYER_COMPONENT_UNDEAD_SLOTS)
		materialized->undead_slots = incoming.undead_slots;
	if (incoming.components & PLAYER_COMPONENT_FORGED_ITEMS)
		materialized->forged_items = incoming.forged_items;
	if (incoming.components & PLAYER_COMPONENT_GRANTED_COMMANDS)
		materialized->granted_commands = incoming.granted_commands;
	if (incoming.components & PLAYER_COMPONENT_SKILLS)
		materialized->skills = incoming.skills;
	if (incoming.components & PLAYER_COMPONENT_AFFECTS)
		materialized->affects = incoming.affects;
	if (incoming.components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		materialized->items = incoming.items;
	if (incoming.components & PLAYER_COMPONENT_PETS)
		materialized->pets = incoming.pets;
	if (incoming.components & PLAYER_COMPONENT_SHAPECHANGES)
		materialized->shapes = incoming.shapes;
	if (incoming.components & PLAYER_COMPONENT_TROPHIES)
		materialized->trophies = incoming.trophies;
	materialized->components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	return true;
}

bool append_baseline_items(const std::vector<player_item_snapshot> &items,
			   const item_owner_identity &owner, std::unordered_set<uint64_t> *seen,
			   std::vector<flatfile_item_ownership_record> *records)
{
	if (!seen || !records)
		return false;
	std::vector<uint64_t> roots;
	try
	{
		roots.reserve(items.size());
		for (size_t index = 0; index < items.size(); ++index)
		{
			const player_item_snapshot &item = items[index];
			if (!item.object_uid || item.vnum <= 0 ||
			    !seen->insert(item.object_uid).second)
				return false;
			uint64_t parent_uid = 0, root_uid = item.object_uid;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return false;
				const size_t parent = static_cast<size_t>(item.parent_index);
				parent_uid = items[parent].object_uid;
				root_uid = roots[parent];
			}
			roots.push_back(root_uid);
			records->push_back({ item.object_uid, root_uid, parent_uid, owner, 1,
					     item.vnum, item_custody_state::active });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

flatfile_item_baseline_result establish_item_baseline(const std::string &root,
						      const player_snapshot &snapshot,
						      std::string *error)
{
	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint64_t>(snapshot.pid), 0 };
	std::unordered_set<uint64_t> seen;
	std::vector<flatfile_item_ownership_record> records;
	try
	{
		const size_t pet_items =
			std::accumulate(snapshot.pets.begin(), snapshot.pets.end(), size_t{ 0 },
					[](size_t count, const player_pet_snapshot &pet)
					{ return count + pet.items.size(); });
		if (snapshot.items.size() > PLAYER_LOAD_ITEM_MAX ||
		    pet_items > PLAYER_LOAD_ITEM_MAX - snapshot.items.size())
			return flatfile_item_baseline_result::invalid;
		seen.reserve(snapshot.items.size() + pet_items);
		records.reserve(snapshot.items.size() + pet_items);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_baseline_result::io_error;
	}
	if (!append_baseline_items(snapshot.items, owner, &seen, &records))
		return flatfile_item_baseline_result::invalid;
	for (const player_pet_snapshot &pet : snapshot.pets)
		if (!append_baseline_items(pet.items, owner, &seen, &records))
			return flatfile_item_baseline_result::invalid;
	std::sort(records.begin(), records.end(), [](const auto &left, const auto &right)
		  { return left.item_uid < right.item_uid; });
	return flatfile_item_repository_establish_owner(root, owner, records, error);
}

flatfile_player_domain_result establish_domain_baseline(const std::string &root,
							const player_snapshot &snapshot,
							std::string *error)
{
	flatfile_identity_record identity;
	const flatfile_identity_result identity_loaded =
		flatfile_identity_lookup_pid(root, snapshot.pid, &identity, error);
	if (identity_loaded != flatfile_identity_result::ok)
		return identity_loaded == flatfile_identity_result::io_error ?
			       flatfile_player_domain_result::io_error :
		       identity_loaded == flatfile_identity_result::not_found ?
			       flatfile_player_domain_result::not_found :
			       flatfile_player_domain_result::invalid;
	const std::string *name = snapshot_player_name(snapshot);
	int64_t racewar = 0;
	flatfile_player_domain_record record;
	record.pid = snapshot.pid;
	record.account_name = identity.account;
	if (!identity.active || !name || !same_authority_key(*name, identity.name) ||
	    !snapshot_signed(snapshot, player_status_field::racewar, &racewar) ||
	    racewar < INT8_MIN || racewar > INT8_MAX || identity.racewar != racewar ||
	    !snapshot_unsigned(snapshot, player_status_field::copper, &record.domains.wallet[0]) ||
	    !snapshot_unsigned(snapshot, player_status_field::silver, &record.domains.wallet[1]) ||
	    !snapshot_unsigned(snapshot, player_status_field::gold, &record.domains.wallet[2]) ||
	    !snapshot_unsigned(snapshot, player_status_field::platinum,
			       &record.domains.wallet[3]) ||
	    !snapshot_signed(snapshot, player_status_field::epics, &record.domains.epics) ||
	    !snapshot_signed(snapshot, player_status_field::frags, &record.domains.frags) ||
	    !snapshot_signed(snapshot, player_status_field::old_frags, &record.domains.old_frags))
		return flatfile_player_domain_result::invalid;
	static constexpr std::array<player_status_field, 10> base_stat_fields = {
		player_status_field::base_strength, player_status_field::base_dexterity,
		player_status_field::base_agility,  player_status_field::base_constitution,
		player_status_field::base_power,    player_status_field::base_intelligence,
		player_status_field::base_wisdom,   player_status_field::base_charisma,
		player_status_field::base_karma,    player_status_field::base_luck,
	};
	for (size_t index = 0; index < base_stat_fields.size(); ++index)
	{
		int64_t stat = 0;
		if (!snapshot_signed(snapshot, base_stat_fields[index], &stat) || stat < 0 ||
		    stat > 100)
			return flatfile_player_domain_result::invalid;
		record.domains.base_stats[index] = static_cast<int16_t>(stat);
	}
	record.domains.base_stat_revision = 1;
	record.racewar = static_cast<int8_t>(racewar);
	return flatfile_player_domain_establish_initial_player(root, record, error);
}
} // namespace

struct flatfile_player_snapshot_lock::state
{
	std::unique_lock<std::mutex> process_lock;
	int fd = -1;
	std::string root;
	int32_t pid = 0;

	state()
		: process_lock(player_mutex, std::defer_lock)
	{
	}
	~state() { flatfile_lock_release(fd); }
};

flatfile_player_snapshot_lock::flatfile_player_snapshot_lock() noexcept
	: state_(new(std::nothrow) state)
{
}
flatfile_player_snapshot_lock::~flatfile_player_snapshot_lock() = default;

bool flatfile_player_snapshot_lock::acquire(const std::string &root, int32_t pid,
					    std::string *error)
{
	if (!state_ || state_->process_lock.owns_lock() || root.empty() || pid <= 0)
		return false;
	state_->process_lock.lock();
	if (flatfile_lock_acquire(player_directory(root), player_lock_filename(pid), &state_->fd,
				  error))
	{
		state_->root = root;
		state_->pid = pid;
		return true;
	}
	state_->process_lock.unlock();
	return false;
}

bool flatfile_player_snapshot_lock::owns(const std::string &root, int32_t pid) const
{
	return state_ && state_->process_lock.owns_lock() && state_->fd >= 0 &&
	       state_->root == root && state_->pid == pid;
}

bool flatfile_player_snapshot_lock::matches(const std::string &root, int32_t pid) const
{
	return owns(root, pid);
}

flatfile_player_load_result flatfile_player_snapshot_load(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error)
{
	std::lock_guard<std::mutex> guard(player_mutex);
	return flatfile_player_snapshot_read(root, pid, snapshot, error);
}

player_load_result flatfile_player_load_repository_execute(const std::string &root,
							   const player_load_request &request)
{
	const uint64_t started = persistence_observability_now_usec();
	player_load_result result = {};
	result.request_id = request.request_id;
	result.pid = request.pid;
	if (!player_load_request_valid(request, started))
	{
		result.outcome = request.deadline_usec <= started ?
					 player_load_outcome::timed_out :
					 player_load_outcome::component_failure;
		result.error_code = request.deadline_usec <= started ? ETIMEDOUT : EINVAL;
		result.failed_component = "request";
		return result;
	}

	flatfile_identity_record identity = {};
	std::string error;
	const flatfile_identity_result identity_loaded =
		request.pid > 0 ?
			flatfile_identity_lookup_pid(root, request.pid, &identity, &error) :
			flatfile_identity_lookup_name(root, request.player_name, &identity, &error);
	if (identity_loaded != flatfile_identity_result::ok)
		return identity_failure(request, identity_loaded);
	result.pid = identity.pid;
	result.account_name = identity.account;
	result.saved_at = identity.last_save;
	if (!identity.active)
	{
		result.outcome = player_load_outcome::not_found;
		result.error_code = ENOENT;
		result.failed_component = "identity";
		return result;
	}
	if (identity.blocked ||
	    (request.pid > 0 && !same_authority_key(request.account_name, identity.account)))
	{
		result.outcome = player_load_outcome::component_failure;
		result.error_code = EACCES;
		result.failed_component = "identity";
		return result;
	}

	const flatfile_player_load_result snapshot_loaded =
		flatfile_player_snapshot_load(root, identity.pid, &result.snapshot, &error);
	if (snapshot_loaded != flatfile_player_load_result::ok)
	{
		result.failed_component = "snapshot";
		result.error_code =
			snapshot_loaded == flatfile_player_load_result::not_found ? ENOENT :
			snapshot_loaded == flatfile_player_load_result::io_error  ? EIO :
										    EILSEQ;
		result.outcome = snapshot_loaded == flatfile_player_load_result::not_found ?
					 player_load_outcome::not_found :
				 snapshot_loaded == flatfile_player_load_result::io_error ?
					 player_load_outcome::retryable_failure :
					 player_load_outcome::component_failure;
		return result;
	}
	const std::string *snapshot_name = snapshot_player_name(result.snapshot);
	if (!snapshot_name || !same_authority_key(*snapshot_name, identity.name))
	{
		result.outcome = player_load_outcome::component_failure;
		result.error_code = EILSEQ;
		result.failed_component = "snapshot_identity";
		return result;
	}
	if (request.include_items && !reconcile_item_ownership(root, &result))
		return result;
	int64_t snapshot_racewar = 0;
	if (!snapshot_signed(result.snapshot, player_status_field::racewar, &snapshot_racewar) ||
	    snapshot_racewar != identity.racewar)
	{
		result.outcome = player_load_outcome::component_failure;
		result.error_code = EILSEQ;
		result.failed_component = "domain_identity";
		return result;
	}
	flatfile_player_domain_record domains;
	const flatfile_player_domain_result domains_loaded = flatfile_player_domain_load(
		root, identity.pid, identity.account, identity.racewar, &domains, &error);
	if (domains_loaded != flatfile_player_domain_result::ok)
	{
		result.outcome = domains_loaded == flatfile_player_domain_result::io_error ?
					 player_load_outcome::retryable_failure :
					 player_load_outcome::component_failure;
		result.error_code =
			domains_loaded == flatfile_player_domain_result::not_found ? ENOENT :
			domains_loaded == flatfile_player_domain_result::io_error  ? EIO :
										     EILSEQ;
		result.failed_component = "domains";
		return result;
	}
	result.domains = domains.domains;
	result.recent_pvp_deaths = std::move(domains.recent_pvp_deaths);
	result.completed_epic_zones = std::move(domains.completed_epic_zones);
	result.read_components = PLAYER_LOAD_SESSION04_READS;
	if (!request.include_pets)
	{
		result.snapshot.pets.clear();
		result.pet_identities.clear();
	}
	if (!request.include_items)
	{
		result.snapshot.items.clear();
		result.item_identities.clear();
		result.item_owner_revision = 0;
		result.authoritative_item_count = 0;
	}
	result.snapshot.components = request.include_pets  ? PLAYER_LOAD_SESSION03_COMPONENTS :
				     request.include_items ? PLAYER_LOAD_SESSION02_COMPONENTS :
							     PLAYER_LOAD_SESSION01_COMPONENTS;

	result.metrics.byte_count = result.snapshot.encoded_size_bound;
	result.metrics.row_count = 1;
	result.metrics.transaction_usec = persistence_observability_now_usec() - started;
	result.outcome = player_load_outcome::applied;
	return result;
}

player_load_result
flatfile_player_load_repository_execute_selected(const player_load_request &request, void *context)
{
	const char *root = context ? static_cast<const char *>(context) :
				     persistence_mode_flatfile_root();
	if (root && *root)
		return flatfile_player_load_repository_execute(root, request);
	player_load_result result = {};
	result.request_id = request.request_id;
	result.pid = request.pid;
	result.outcome = player_load_outcome::component_failure;
	result.error_code = ENOENT;
	result.failed_component = "state_root";
	return result;
}

flatfile_player_load_result
flatfile_player_snapshot_prepare_remove(const std::string &root,
					const flatfile_player_snapshot_lock &snapshot_lock,
					const flatfile_authority_lock &authority_lock, int32_t pid,
					flatfile_authority_operation *operation, std::string *error)
{
	if (!operation || !snapshot_lock.matches(root, pid) || !authority_lock.matches(root))
		return flatfile_player_load_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, authority_lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_player_load_result::io_error :
			       flatfile_player_load_result::invalid;
	player_snapshot snapshot = {};
	const auto loaded = flatfile_player_snapshot_read(root, pid, &snapshot, error);
	if (loaded != flatfile_player_load_result::ok)
		return loaded;
	operation->store = flatfile_authority_store::players;
	operation->kind = flatfile_authority_operation_kind::remove;
	operation->filename = player_filename(pid);
	return flatfile_player_load_result::ok;
}

player_save_apply_result flatfile_player_snapshot_apply(const std::string &root,
							const player_snapshot &snapshot,
							std::string *error)
{
	if (!valid_snapshot(snapshot) || !replace_items_together(snapshot.components))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	flatfile_player_snapshot_lock snapshot_lock;
	if (!snapshot_lock.acquire(root, snapshot.pid, error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	player_snapshot materialized = {};
	const flatfile_player_load_result loaded =
		flatfile_player_snapshot_read(root, snapshot.pid, &materialized, error);
	if (loaded == flatfile_player_load_result::invalid)
		return { player_save_apply_outcome::terminal_failure, 0, EILSEQ };
	if (loaded == flatfile_player_load_result::io_error)
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	if (loaded == flatfile_player_load_result::not_found)
	{
		if (snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL)
			return { player_save_apply_outcome::terminal_failure, 0, ENOENT };
		const flatfile_item_baseline_result item_baseline =
			establish_item_baseline(root, snapshot, error);
		if (item_baseline == flatfile_item_baseline_result::io_error)
			return { player_save_apply_outcome::retryable_failure, 0, EIO };
		if (item_baseline != flatfile_item_baseline_result::applied &&
		    item_baseline != flatfile_item_baseline_result::already_applied)
			return { player_save_apply_outcome::terminal_failure, 0,
				 static_cast<unsigned int>(
					 item_baseline == flatfile_item_baseline_result::conflict ?
						 EEXIST :
						 EINVAL) };
		const flatfile_player_domain_result domain_baseline =
			establish_domain_baseline(root, snapshot, error);
		if (domain_baseline == flatfile_player_domain_result::io_error)
			return { player_save_apply_outcome::retryable_failure, 0, EIO };
		if (domain_baseline != flatfile_player_domain_result::ok)
			return { player_save_apply_outcome::terminal_failure, 0,
				 static_cast<unsigned int>(
					 domain_baseline ==
							 flatfile_player_domain_result::conflict ?
						 EEXIST :
					 domain_baseline ==
							 flatfile_player_domain_result::not_found ?
						 ENOENT :
						 EINVAL) };
		materialized = snapshot;
	}
	else
	{
		if (materialized.revision >= snapshot.revision)
			return { materialized.revision == snapshot.revision ?
					 player_save_apply_outcome::already_applied :
					 player_save_apply_outcome::stale_revision,
				 materialized.revision, 0 };
		if (!merge_snapshot(snapshot, &materialized))
			return { player_save_apply_outcome::terminal_failure, materialized.revision,
				 EINVAL };
	}
	std::vector<uint8_t> bytes;
	// The refused assets exist nowhere else once the character is released. Publish
	// the disposition before the player file, and keep it out of that file so a
	// later ordinary save cannot overwrite the record.
	if (snapshot.death)
	{
		player_snapshot disposition = snapshot;
		std::vector<uint8_t> death_bytes;
		if (!encode_file(&disposition, &death_bytes))
			return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
		if (!flatfile_atomic_write(death_directory(root),
					   death_filename(snapshot.pid, snapshot.revision),
					   death_bytes, error))
			return { player_save_apply_outcome::retryable_failure, 0, EIO };
		materialized.death.reset();
		materialized.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	}
	if (!encode_file(&materialized, &bytes))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	if (!flatfile_atomic_write(player_directory(root), player_filename(snapshot.pid), bytes,
				   error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}

player_save_apply_result flatfile_player_snapshot_apply_selected(const player_snapshot &snapshot,
								 void *context)
{
	(void)context;
	const char *root = persistence_mode_flatfile_root();
	if (!root)
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	std::string error;
	return flatfile_player_snapshot_apply(root, snapshot, &error);
}
