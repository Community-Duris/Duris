#include "flatfile_player_repository.h"

#include "flatfile_identity_repository.h"
#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_store.h"
#include "persistence_observability.h"
#include "persistence_mode.h"
#include "player_snapshot_codec.h"

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
constexpr uint32_t player_file_version = 1;
constexpr std::array<uint8_t, 8> player_magic = { 'D', 'U', 'R', 'P', 'L', 'Y', 'R', 0 };
constexpr size_t player_file_maximum = PLAYER_SNAPSHOT_MAX_BYTES + 128;
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

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;

	template <typename T> bool number(T *value)
	{
		if (!value || size - offset < sizeof(T))
			return false;
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<unsigned_type>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}
};

struct authority_lock
{
	int fd = -1;
	~authority_lock() { flatfile_lock_release(fd); }
};

std::string player_directory(const std::string &root)
{
	return root + "/players";
}

std::string player_filename(int32_t pid)
{
	return std::to_string(pid) + ".snapshot";
}

std::string player_lock_filename(int32_t pid)
{
	return ".player-" + std::to_string(pid) + ".lock";
}

bool valid_snapshot(const player_snapshot &snapshot)
{
	return snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION && snapshot.pid > 0 &&
	       snapshot.revision && snapshot.components &&
	       !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
	       snapshot.encoded_size_bound &&
	       snapshot.encoded_size_bound <= PLAYER_SNAPSHOT_MAX_BYTES;
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
	case flatfile_identity_result::invalid:
	case flatfile_identity_result::exhausted:
		result.outcome = player_load_outcome::component_failure;
		result.error_code = EILSEQ;
		break;
	}
	return result;
}

bool build_item_identities(const std::vector<player_item_snapshot> &items,
			   const std::unordered_map<uint64_t, flatfile_item_ownership_record> &owned,
			   const item_owner_identity &owner, uint64_t owner_revision,
			   uint64_t *next_database_id, std::unordered_set<uint64_t> *consumed,
			   std::vector<player_load_item_identity> *identities)
{
	if (!next_database_id || !consumed || !identities)
		return false;
	std::vector<uint64_t> database_ids;
	try
	{
		database_ids.reserve(items.size());
		identities->reserve(items.size());
		for (size_t index = 0; index < items.size(); ++index)
		{
			const player_item_snapshot &item = items[index];
			if (!item.object_uid || item.vnum <= 0 ||
			    *next_database_id > static_cast<uint64_t>(INT_MAX) ||
			    !consumed->insert(item.object_uid).second)
				return false;
			const auto found = owned.find(item.object_uid);
			if (found == owned.end())
				return false;
			const flatfile_item_ownership_record &record = found->second;
			uint64_t parent_uid = 0, serialized_parent = 0;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return false;
				const size_t parent_index = static_cast<size_t>(item.parent_index);
				parent_uid = items[parent_index].object_uid;
				serialized_parent = database_ids[parent_index];
			}
			if (record.vnum != item.vnum || record.parent_item_uid != parent_uid ||
			    !item_owner_identity_equal(record.owner, owner) ||
			    record.state != item_custody_state::active ||
			    (!parent_uid && record.root_item_uid != record.item_uid))
				return false;
			const uint64_t database_id = (*next_database_id)++;
			database_ids.push_back(database_id);
			identities->push_back({ database_id, serialized_parent, 1,
						PLAYER_LOAD_ITEM_OVERRIDE_ALL, record.item_uid,
						record.root_item_uid, record.parent_item_uid,
						record.owner, record.item_revision, owner_revision,
						record.state });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
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
	const flatfile_item_repository_result loaded =
		flatfile_item_repository_load_owner(root, owner, &owner_revision, &records, &error);
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
	if (!build_item_identities(result->snapshot.items, owned, owner, owner_revision,
				   &next_database_id, &consumed, &result->item_identities))
		goto invalid;
	for (size_t index = 0; index < result->snapshot.pets.size(); ++index)
	{
		result->pet_identities[index].database_id = index + 1;
		if (!build_item_identities(result->snapshot.pets[index].items, owned, owner,
					   owner_revision, &next_database_id, &consumed,
					   &result->pet_identities[index].item_identities))
			goto invalid;
	}
	if (consumed.size() != records.size())
		goto invalid;
	result->item_owner_revision = owner_revision;
	result->authoritative_item_count = records.size();
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

flatfile_player_load_result load_unlocked(const std::string &root, int32_t pid,
					  player_snapshot *snapshot, std::string *error)
{
	if (pid <= 0 || !snapshot)
		return flatfile_player_load_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read = flatfile_read(
		player_directory(root), player_filename(pid), player_file_maximum, &bytes, error);
	if (read == flatfile_read_result::not_found)
		return flatfile_player_load_result::not_found;
	if (read == flatfile_read_result::invalid)
		return flatfile_player_load_result::invalid;
	if (read != flatfile_read_result::ok)
		return flatfile_player_load_result::io_error;
	constexpr size_t header_size = player_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(int32_t) + sizeof(uint64_t) * 2 +
				       SHA256_DIGEST_LENGTH;
	if (bytes.size() < header_size ||
	    memcmp(bytes.data(), player_magic.data(), player_magic.size()))
		return flatfile_player_load_result::invalid;
	decoder header{ bytes.data() + player_magic.size(), bytes.size() - player_magic.size() };
	uint32_t version = 0, payload_size = 0;
	int32_t stored_pid = 0;
	uint64_t stored_revision = 0, stored_components = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&stored_pid) || !header.number(&stored_revision) ||
	    !header.number(&stored_components) || version != player_file_version ||
	    stored_pid != pid || !stored_revision ||
	    stored_components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    payload_size != bytes.size() - header_size)
		return flatfile_player_load_result::invalid;
	const uint8_t *stored_digest = bytes.data() + player_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(int32_t) + sizeof(uint64_t) * 2;
	const uint8_t *payload = bytes.data() + header_size;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_player_load_result::invalid;
	player_snapshot decoded = {};
	if (player_snapshot_decode(payload, payload_size, &decoded) !=
		    player_snapshot_codec_result::ok ||
	    decoded.pid != stored_pid || decoded.revision != stored_revision ||
	    decoded.components != stored_components)
		return flatfile_player_load_result::invalid;
	*snapshot = std::move(decoded);
	return flatfile_player_load_result::ok;
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
	record.racewar = static_cast<int8_t>(racewar);
	return flatfile_player_domain_establish_initial_player(root, record, error);
}
} // namespace

flatfile_player_load_result flatfile_player_snapshot_load(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error)
{
	std::lock_guard<std::mutex> guard(player_mutex);
	return load_unlocked(root, pid, snapshot, error);
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
	result.outcome = player_load_outcome::component_failure;
	result.error_code = ENOTSUP;
	result.failed_component = "trophies";
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

player_save_apply_result flatfile_player_snapshot_apply(const std::string &root,
							const player_snapshot &snapshot,
							std::string *error)
{
	if (!valid_snapshot(snapshot) || !replace_items_together(snapshot.components))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	std::lock_guard<std::mutex> guard(player_mutex);
	authority_lock authority;
	if (!flatfile_lock_acquire(player_directory(root), player_lock_filename(snapshot.pid),
				   &authority.fd, error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	player_snapshot materialized = {};
	const flatfile_player_load_result loaded =
		load_unlocked(root, snapshot.pid, &materialized, error);
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
