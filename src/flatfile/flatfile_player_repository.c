#include "flatfile/flatfile_player_repository.h"

#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_observability.h"
#include "persistence/persistence_mode.h"
#include "player/player_snapshot_codec.h"
#include "flatfile/flatfile_craft_progression.h"

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
	return (snapshot.death ?
			player_snapshot_is_death_request_schema(snapshot.schema_version) :
			(snapshot.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
			 snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ||
			 snapshot.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
			 snapshot.schema_version ==
				 PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION)) &&
	       snapshot.pid > 0 && snapshot.revision && snapshot.components &&
	       !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
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

void mark_degraded(player_load_result *result, uint32_t component, const char *stage)
{
	if (!result)
		return;
	result->outcome = player_load_outcome::degraded;
	result->degraded_components |= component;
	if (!result->failed_component)
		result->failed_component = stage;
}

void clear_items_and_pets(player_load_result *result)
{
	if (!result)
		return;
	result->snapshot.items.clear();
	result->snapshot.pets.clear();
	result->item_identities.clear();
	result->pet_identities.clear();
	result->item_owner_revision = 0;
	result->authoritative_item_count = 0;
	result->authoritative_pet_item_count = 0;
	result->stale_item_rows = 0;
	result->missing_payload_rows = 0;
	result->promoted_item_rows = 0;
	result->repaired_item_rows = 0;
	result->snapshot.components = PLAYER_LOAD_SESSION01_COMPONENTS;
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
	size_t pet_owned_count = 0;
	size_t pet_materialized_count = 0;
	if (!build_item_identities(&result->snapshot.items, owned, owner, owner_revision,
				   &next_database_id, &consumed, &result->item_identities, result))
		goto invalid;
	for (size_t index = 0; index < result->snapshot.pets.size(); ++index)
	{
		const uint64_t pet_uid = result->snapshot.pets[index].pet_uid;
		const item_owner_identity pet_owner =
			pet_uid ? item_owner_identity{ item_owner_type::pet, pet_uid,
						       static_cast<uint64_t>(result->pid) } :
				  owner;
		uint64_t pet_revision = owner_revision;
		std::unordered_map<uint64_t, flatfile_item_ownership_record> pet_owned;
		if (pet_uid)
		{
			std::vector<flatfile_item_ownership_record> pet_records;
			const auto read = flatfile_item_repository_load_owner_locked(
				root, authority, pet_owner, &pet_revision, &pet_records, &error);
			if (read != flatfile_item_repository_result::ok)
			{
				result->outcome =
					read == flatfile_item_repository_result::io_error ?
						player_load_outcome::retryable_failure :
						player_load_outcome::component_failure;
				result->error_code =
					read == flatfile_item_repository_result::io_error ? EIO :
											    EILSEQ;
				result->failed_component = "pet_ownership";
				return false;
			}
			pet_owned_count += pet_records.size();
			try
			{
				for (const auto &record : pet_records)
					if (!pet_owned.emplace(record.item_uid, record).second)
						goto invalid;
			}
			catch (const std::bad_alloc &)
			{
				result->outcome = player_load_outcome::retryable_failure;
				result->error_code = ENOMEM;
				result->failed_component = "pet_ownership";
				return false;
			}
		}
		auto &identity = result->pet_identities[index];
		identity.database_id = index + 1;
		identity.pet_uid = pet_uid;
		identity.owner_revision = pet_revision;
		if (!build_item_identities(&result->snapshot.pets[index].items,
					   pet_uid ? pet_owned : owned, pet_owner, pet_revision,
					   &next_database_id, &consumed, &identity.item_identities,
					   result))
			goto invalid;
		if (pet_uid)
			pet_materialized_count += identity.item_identities.size();
	}
	if (consumed.size() > records.size() + pet_owned_count ||
	    pet_materialized_count > consumed.size())
		goto invalid;
	// An ownership record whose payload item is gone cannot be rebuilt, but it must not
	// refuse the load either. Preserve it for explicit operator repair; snapshot saves are
	// not allowed to rewrite authoritative custody.
	result->missing_payload_rows = records.size() + pet_owned_count - consumed.size();
	result->item_owner_revision = owner_revision;
	result->authoritative_item_count = consumed.size() - pet_materialized_count;
	result->authoritative_pet_item_count = pet_materialized_count;
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

static_assert(PLAYER_SPELL_EFFECT_RECEIPT_MAX + 3 <=
	      flatfile_authority_transaction_maximum_operations);

std::string spell_receipt_filename(int32_t pid, const critical_operation_id &operation)
{
	constexpr char hex[] = "0123456789abcdef";
	std::string filename = std::to_string(pid) + "-";
	for (uint8_t byte : operation.bytes)
	{
		filename += hex[byte >> 4];
		filename += hex[byte & 15];
	}
	return filename + ".spell";
}

flatfile_player_load_result read_spell_receipt(const std::string &root, int32_t pid,
					       const critical_operation_id &operation,
					       player_snapshot *receipt, std::string *error)
{
	const auto read = flatfile_player_snapshot_read_file(player_directory(root),
							     spell_receipt_filename(pid, operation),
							     pid, receipt, error);
	if (read != flatfile_player_load_result::ok)
		return read;
	if (receipt->schema_version != PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION ||
	    receipt->death || !receipt->quest_xp_receipts.empty() ||
	    receipt->spell_effect_receipts.size() != 1 ||
	    receipt->spell_effect_receipts.front().operation_id.bytes != operation.bytes)
		return flatfile_player_load_result::invalid;
	return flatfile_player_load_result::ok;
}

flatfile_player_load_result verify_spell_receipts(const std::string &root,
						  const player_snapshot &request,
						  player_revision_t durable_revision,
						  std::string *error)
{
	for (const auto &expected : request.spell_effect_receipts)
	{
		player_snapshot stored;
		const auto read = read_spell_receipt(root, request.pid, expected.operation_id,
						     &stored, error);
		if (read != flatfile_player_load_result::ok)
			return read;
		if (stored.revision > durable_revision ||
		    stored.spell_effect_receipts.front().effect_id != expected.effect_id)
			return flatfile_player_load_result::invalid;
	}
	return flatfile_player_load_result::ok;
}

flatfile_player_load_result verify_death_receipt(const std::string &root,
						 const player_snapshot &request, std::string *error)
{
	if (!request.death)
		return flatfile_player_load_result::ok;
	player_snapshot stored;
	const auto read = flatfile_player_snapshot_read_file(
		death_directory(root), death_filename(request.pid, request.revision), request.pid,
		&stored, error);
	if (read != flatfile_player_load_result::ok)
		return read;
	auto expected = request;
	std::vector<uint8_t> expected_bytes, stored_bytes;
	return encode_file(&expected, &expected_bytes) && encode_file(&stored, &stored_bytes) &&
			       expected_bytes == stored_bytes ?
		       flatfile_player_load_result::ok :
		       flatfile_player_load_result::invalid;
}

flatfile_player_load_result verify_craft_receipts(const std::string &root,
						  const flatfile_authority_lock &authority,
						  const player_snapshot &request,
						  player_revision_t durable_revision,
						  std::string *error)
{
	for (const auto &expected : request.craft_receipts)
	{
		const auto committed = flatfile_item_repository_craft_root_locked(
			root, authority, expected.operation_id, error);
		if (committed != flatfile_item_repository_result::ok)
			return committed == flatfile_item_repository_result::io_error ?
				       flatfile_player_load_result::io_error :
				       flatfile_player_load_result::invalid;
		player_snapshot obligation, stored;
		auto read = flatfile_craft_receipt_read(root, request.pid, expected.operation_id,
							true, &obligation, error);
		if (read != flatfile_player_load_result::ok)
			return read;
		read = flatfile_craft_receipt_read(root, request.pid, expected.operation_id, false,
						   &stored, error);
		if (read != flatfile_player_load_result::ok)
			return read;
		if (!flatfile_craft_receipt_equal(expected, obligation.craft_receipts[0]) ||
		    !flatfile_craft_receipt_equal(expected, stored.craft_receipts[0]) ||
		    stored.revision > durable_revision)
			return flatfile_player_load_result::invalid;
	}
	return flatfile_player_load_result::ok;
}

// Load the player and only requested receipt identities under the same recovered
// authority cut. Historical receipts never consume the snapshot's row budget.
flatfile_player_load_result load_snapshot_with_spell_receipts(
	const std::string &root, int32_t pid, const std::vector<critical_operation_id> &operations,
	player_snapshot *snapshot, std::vector<player_load_spell_effect_receipt> *receipts,
	std::string *error, const std::vector<critical_operation_id> &craft_operations = {},
	std::vector<player_craft_receipt_snapshot> *craft_receipts = nullptr)
{
	if (craft_operations.size() > PLAYER_CRAFT_RECEIPT_MAX ||
	    (!craft_operations.empty() && !craft_receipts) || pid <= 0 || !snapshot || !receipts ||
	    operations.size() > PLAYER_SPELL_EFFECT_RECEIPT_MAX)
		return flatfile_player_load_result::invalid;
	flatfile_player_snapshot_lock snapshot_lock;
	flatfile_authority_lock authority;
	if (!snapshot_lock.acquire(root, pid, error) || !authority.acquire(root, error))
		return flatfile_player_load_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_player_load_result::io_error :
			       flatfile_player_load_result::invalid;
	const auto loaded = flatfile_player_snapshot_read(root, pid, snapshot, error);
	if (loaded != flatfile_player_load_result::ok)
		return loaded;
	try
	{
		std::unordered_set<std::string> seen;
		for (const auto &operation : operations)
		{
			if (critical_operation_id_is_zero(operation) ||
			    !seen.insert(spell_receipt_filename(pid, operation)).second)
				return flatfile_player_load_result::invalid;
			player_snapshot stored;
			const auto read = read_spell_receipt(root, pid, operation, &stored, error);
			if (read == flatfile_player_load_result::not_found)
				continue;
			if (read != flatfile_player_load_result::ok)
				return read;
			if (stored.revision > snapshot->revision)
				return flatfile_player_load_result::invalid;
			receipts->push_back(
				{ operation, stored.spell_effect_receipts.front().effect_id });
		}
		for (const auto &operation : craft_operations)
		{
			if (critical_operation_id_is_zero(operation) ||
			    !seen.insert(flatfile_craft_receipt_filename(pid, operation)).second)
				return flatfile_player_load_result::invalid;
			player_snapshot stored, obligation;
			auto read = flatfile_craft_receipt_read(root, pid, operation, false,
								&stored, error);
			if (read == flatfile_player_load_result::not_found)
				continue;
			if (read != flatfile_player_load_result::ok)
				return read;
			const auto committed = flatfile_item_repository_craft_root_locked(
				root, authority, operation, error);
			if (committed != flatfile_item_repository_result::ok)
				return committed == flatfile_item_repository_result::io_error ?
					       flatfile_player_load_result::io_error :
					       flatfile_player_load_result::invalid;
			read = flatfile_craft_receipt_read(root, pid, operation, true, &obligation,
							   error);
			if (read != flatfile_player_load_result::ok)
				return read;
			if (stored.revision > snapshot->revision ||
			    !flatfile_craft_receipt_equal(stored.craft_receipts[0],
							  obligation.craft_receipts[0]))
				return flatfile_player_load_result::invalid;
			craft_receipts->push_back(stored.craft_receipts[0]);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_player_load_result::io_error;
	}
	return flatfile_player_load_result::ok;
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
		materialized->output_preferences = incoming.output_preferences;
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
			   const item_owner_identity &owner, bool player_equipment,
			   std::unordered_set<uint64_t> *seen,
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
			uint16_t equipment_slot = 0;
			if (player_equipment && !parent_uid)
			{
				if (item.equipment_slot < 0 ||
				    item.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT)
					return false;
				equipment_slot = static_cast<uint16_t>(item.equipment_slot);
			}
			records->push_back({ item.object_uid,
					     root_uid,
					     parent_uid,
					     owner,
					     1,
					     item.vnum,
					     item_custody_state::active,
					     {},
					     equipment_slot });
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
	if (!append_baseline_items(snapshot.items, owner, true, &seen, &records))
		return flatfile_item_baseline_result::invalid;
	for (const player_pet_snapshot &pet : snapshot.pets)
		if (!append_baseline_items(pet.items, owner, false, &seen, &records))
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
	std::vector<player_load_spell_effect_receipt> receipts;
	return load_snapshot_with_spell_receipts(root, pid, {}, snapshot, &receipts, error);
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

	const flatfile_player_load_result snapshot_loaded = load_snapshot_with_spell_receipts(
		root, identity.pid, request.pending_spell_effect_operations, &result.snapshot,
		&result.spell_effect_receipts, &error, request.pending_craft_operations,
		&result.craft_receipts);
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
	{
		clear_items_and_pets(&result);
		mark_degraded(&result, PLAYER_LOAD_DEGRADED_ITEMS, "item_ownership");
		if (request.include_pets)
			result.degraded_components |= PLAYER_LOAD_DEGRADED_PETS;
	}
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
		result.error_code =
			domains_loaded == flatfile_player_domain_result::not_found ? ENOENT :
			domains_loaded == flatfile_player_domain_result::io_error  ? EIO :
										     EILSEQ;
		mark_degraded(&result, PLAYER_LOAD_DEGRADED_BANK | PLAYER_LOAD_DEGRADED_GAMEPLAY,
			      "domains");
		result.read_components = 0;
		result.domains = {};
		result.recent_pvp_deaths.clear();
		result.completed_epic_zones.clear();
	}
	else
	{
		result.domains = domains.domains;
		result.recent_pvp_deaths = std::move(domains.recent_pvp_deaths);
		result.completed_epic_zones = std::move(domains.completed_epic_zones);
		result.read_components = PLAYER_LOAD_SESSION04_READS;
	}
	{
		std::vector<flatfile_quest_reward_obligation> obligations;
		std::vector<flatfile_quest_xp_entitlement> entitlements;
		const auto loaded = flatfile_item_repository_pending_quest_rewards(
			root, static_cast<uint32_t>(identity.pid), &obligations, &error,
			&entitlements, result.snapshot.revision);
		if (loaded == flatfile_item_repository_result::ok)
		{
			bool malformed = false;
			try
			{
				result.pending_quest_rewards.reserve(obligations.size());
				for (auto &obligation : obligations)
				{
					quest_reward_continuation terms;
					if (!quest_reward_continuation_decode(
						    obligation.continuation.data(),
						    obligation.continuation.size(), &terms))
					{
						malformed = true;
						break;
					}
					result.pending_quest_rewards.push_back(
						{ obligation.offering_operation,
						  std::move(obligation.continuation), terms,
						  obligation.xp_applied_mask,
						  obligation.economic_applied_mask,
						  obligation.economic_history_verified });
				}
				result.pending_quest_xp_entitlements.reserve(entitlements.size());
				for (auto &entitlement : entitlements)
					result.pending_quest_xp_entitlements.push_back(
						{ entitlement.offering_operation,
						  std::move(entitlement.terms),
						  entitlement.reward_index, entitlement.amount });
			}
			catch (const std::bad_alloc &)
			{
				result.pending_quest_rewards.clear();
				result.pending_quest_xp_entitlements.clear();
				result.error_code = ENOMEM;
				mark_degraded(&result, PLAYER_LOAD_DEGRADED_RECOVERY,
					      "quest_reward_obligations");
			}
			if (malformed)
			{
				result.pending_quest_rewards.clear();
				result.pending_quest_xp_entitlements.clear();
				result.error_code = EILSEQ;
				mark_degraded(&result, PLAYER_LOAD_DEGRADED_RECOVERY,
					      "quest_reward_obligations");
			}
		}
		else
		{
			result.error_code =
				loaded == flatfile_item_repository_result::io_error ? EIO : EILSEQ;
			mark_degraded(&result, PLAYER_LOAD_DEGRADED_RECOVERY,
				      "quest_reward_obligations");
		}
	}
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
	if (!result.degraded_components)
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
	if (!snapshot.quest_xp_receipts.empty() &&
	    (!(snapshot.components & PLAYER_COMPONENT_STATUS) ||
	     std::none_of(snapshot.status_integers.begin(), snapshot.status_integers.end(),
			  [](const player_snapshot_integer &row)
			  { return row.field == player_status_field::experience; })))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	std::vector<uint8_t> validated;
	if (player_snapshot_encode(snapshot, &validated) != player_snapshot_codec_result::ok)
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	flatfile_player_snapshot_lock snapshot_lock;
	if (!snapshot_lock.acquire(root, snapshot.pid, error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	flatfile_authority_lock authority;
	// Even an ordinary save must recover a prior receipt/death transaction before
	// advancing the player file, or recovery could overwrite a newer checkpoint.
	{
		flatfile_authority_lock recovery;
		if (!recovery.acquire(root, error))
			return { player_save_apply_outcome::retryable_failure, 0, EIO };
		const auto recovered =
			flatfile_authority_transaction_recover(root, recovery, error);
		if (recovered != flatfile_authority_transaction_result::ok)
			return { recovered == flatfile_authority_transaction_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0, EIO };
	}
	player_snapshot materialized = {};
	auto loaded = flatfile_player_snapshot_read(root, snapshot.pid, &materialized, error);
	if (loaded == flatfile_player_load_result::invalid)
		return { player_save_apply_outcome::terminal_failure, 0, EILSEQ };
	if (loaded == flatfile_player_load_result::io_error)
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	const bool baseline_missing = loaded == flatfile_player_load_result::not_found;
	if (loaded == flatfile_player_load_result::not_found)
	{
		if (snapshot.death || !snapshot.quest_xp_receipts.empty() ||
		    snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL)
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
	// Baseline helpers acquire authority themselves. Recover again and reread the
	// current player after those helpers release it, then hold authority to commit.
	if (!authority.acquire(root, error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return { recovered == flatfile_authority_transaction_result::io_error ?
				 player_save_apply_outcome::retryable_failure :
				 player_save_apply_outcome::terminal_failure,
			 0, EIO };
	player_snapshot current;
	loaded = flatfile_player_snapshot_read(root, snapshot.pid, &current, error);
	const player_revision_t prior_revision =
		loaded == flatfile_player_load_result::ok ? current.revision : 0;
	if (loaded == flatfile_player_load_result::not_found && !baseline_missing)
		return { player_save_apply_outcome::terminal_failure, 0, ENOENT };
	if (loaded == flatfile_player_load_result::invalid ||
	    loaded == flatfile_player_load_result::io_error)
		return { loaded == flatfile_player_load_result::io_error ?
				 player_save_apply_outcome::retryable_failure :
				 player_save_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 loaded == flatfile_player_load_result::io_error ? EIO : EILSEQ) };
	if (loaded == flatfile_player_load_result::ok)
	{
		materialized = std::move(current);
		if (materialized.revision >= snapshot.revision)
		{
			auto verified = verify_death_receipt(root, snapshot, error);
			if (verified == flatfile_player_load_result::ok)
				verified = verify_spell_receipts(root, snapshot,
								 materialized.revision, error);
			if (verified == flatfile_player_load_result::ok)
				verified = verify_craft_receipts(root, authority, snapshot,
								 materialized.revision, error);
			if (verified != flatfile_player_load_result::ok)
				return { verified == flatfile_player_load_result::io_error ?
						 player_save_apply_outcome::retryable_failure :
						 player_save_apply_outcome::terminal_failure,
					 materialized.revision,
					 static_cast<unsigned int>(
						 verified == flatfile_player_load_result::io_error ?
							 EIO :
							 EILSEQ) };
			if (!snapshot.quest_xp_receipts.empty())
			{
				flatfile_authority_operation ignored;
				const auto checked =
					flatfile_item_repository_prepare_quest_xp_receipts(
						root, authority, snapshot.pid,
						materialized.revision, snapshot.revision,
						snapshot.quest_xp_receipts, true, &ignored, error);
				if (checked != flatfile_item_repository_result::unchanged)
					return {
						checked == flatfile_item_repository_result::io_error ?
							player_save_apply_outcome::retryable_failure :
							player_save_apply_outcome::terminal_failure,
						materialized.revision,
						static_cast<unsigned int>(
							checked == flatfile_item_repository_result::
										io_error ?
								EIO :
								EILSEQ)
					};
			}
			return { materialized.revision == snapshot.revision ?
					 player_save_apply_outcome::already_applied :
					 player_save_apply_outcome::stale_revision,
				 materialized.revision, 0, player_save_custody_diagnosis::none,
				 !snapshot.death && (!snapshot.quest_xp_receipts.empty() ||
						     !snapshot.spell_effect_receipts.empty() ||
						     !snapshot.craft_receipts.empty()) };
		}
		if (!merge_snapshot(snapshot, &materialized))
			return { player_save_apply_outcome::terminal_failure, materialized.revision,
				 EINVAL };
	}
	std::vector<flatfile_authority_operation> operations;
	if (!snapshot.death && !snapshot.quest_xp_receipts.empty())
	{
		flatfile_authority_operation quest_xp;
		const auto prepared = flatfile_item_repository_prepare_quest_xp_receipts(
			root, authority, snapshot.pid, prior_revision, snapshot.revision,
			snapshot.quest_xp_receipts, false, &quest_xp, error);
		if (prepared == flatfile_item_repository_result::ok)
			operations.push_back(std::move(quest_xp));
		else if (prepared != flatfile_item_repository_result::unchanged)
			return { prepared == flatfile_item_repository_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0,
				 static_cast<unsigned int>(
					 prepared == flatfile_item_repository_result::io_error ?
						 EIO :
						 EILSEQ) };
	}
	for (const auto &receipt : snapshot.spell_effect_receipts)
	{
		player_snapshot stored;
		const auto read = read_spell_receipt(root, snapshot.pid, receipt.operation_id,
						     &stored, error);
		if (read == flatfile_player_load_result::ok)
		{
			if (stored.revision > prior_revision ||
			    stored.spell_effect_receipts.front().effect_id != receipt.effect_id)
				return { player_save_apply_outcome::terminal_failure, 0, EILSEQ };
			continue;
		}
		if (read != flatfile_player_load_result::not_found)
			return { read == flatfile_player_load_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0,
				 static_cast<unsigned int>(
					 read == flatfile_player_load_result::io_error ? EIO :
											 EILSEQ) };
		stored = {};
		stored.schema_version = PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
		stored.pid = snapshot.pid;
		stored.revision = snapshot.revision;
		stored.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
		stored.spell_effect_receipts.push_back(receipt);
		std::vector<uint8_t> receipt_bytes;
		if (!encode_file(&stored, &receipt_bytes))
			return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
		operations.push_back({ flatfile_authority_store::players,
				       flatfile_authority_operation_kind::write,
				       spell_receipt_filename(snapshot.pid, receipt.operation_id),
				       std::move(receipt_bytes) });
	}
	for (const auto &receipt : snapshot.craft_receipts)
	{
		const auto committed = flatfile_item_repository_craft_root_locked(
			root, authority, receipt.operation_id, error);
		if (committed != flatfile_item_repository_result::ok)
			return { committed == flatfile_item_repository_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0, EILSEQ };
		player_snapshot obligation, stored;
		const auto entitlement = flatfile_craft_receipt_read(
			root, snapshot.pid, receipt.operation_id, true, &obligation, error);
		if (entitlement != flatfile_player_load_result::ok ||
		    !flatfile_craft_receipt_equal(receipt, obligation.craft_receipts[0]))
			return { entitlement == flatfile_player_load_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0, EILSEQ };
		const auto read = flatfile_craft_receipt_read(
			root, snapshot.pid, receipt.operation_id, false, &stored, error);
		if (read == flatfile_player_load_result::ok)
		{
			if (stored.revision > prior_revision ||
			    !flatfile_craft_receipt_equal(receipt, stored.craft_receipts[0]))
				return { player_save_apply_outcome::terminal_failure, 0, EILSEQ };
			continue;
		}
		if (read != flatfile_player_load_result::not_found)
			return { read == flatfile_player_load_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0, EILSEQ };
		stored = {};
		stored.schema_version = PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
		stored.pid = snapshot.pid;
		stored.revision = snapshot.revision;
		stored.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
		stored.craft_receipts.push_back(receipt);
		std::vector<uint8_t> bytes;
		if (!flatfile_player_snapshot_encode_file(stored, &bytes))
			return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
		operations.push_back(
			{ flatfile_authority_store::players,
			  flatfile_authority_operation_kind::write,
			  flatfile_craft_receipt_filename(snapshot.pid, receipt.operation_id),
			  std::move(bytes) });
	}
	materialized.craft_receipts.clear();
	materialized.spell_effect_receipts.clear();
	materialized.quest_xp_receipts.clear();
	if (materialized.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
	    materialized.schema_version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION ||
	    materialized.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION)
		materialized.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	std::vector<uint8_t> bytes;
	// Keep the immutable evidence, quarantine and empty player projection in the
	// same recoverable authority transaction. A failed commit leaves no evidence
	// claiming a disposition that never took effect.
	std::vector<uint8_t> death_bytes;
	if (snapshot.death)
	{
		player_snapshot disposition = snapshot;
		if (!encode_file(&disposition, &death_bytes))
			return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
		materialized.death.reset();
		materialized.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	}
	if (!encode_file(&materialized, &bytes))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	if (snapshot.death)
	{
		operations.push_back({ flatfile_authority_store::player_deaths,
				       flatfile_authority_operation_kind::write,
				       death_filename(snapshot.pid, snapshot.revision),
				       std::move(death_bytes) });
		std::vector<uint64_t> custody_uids;
		try
		{
			custody_uids.reserve(snapshot.death->custody.size());
			for (const auto &row : snapshot.death->custody)
				if (row.item.item_uid)
					custody_uids.push_back(row.item.item_uid);
		}
		catch (const std::bad_alloc &)
		{
			return { player_save_apply_outcome::retryable_failure, 0, ENOMEM };
		}
		flatfile_authority_operation quarantine;
		const auto prepared = flatfile_item_repository_prepare_death_quarantine(
			root, authority, snapshot.pid, custody_uids, &quarantine, error,
			snapshot.quest_xp_receipts, prior_revision, snapshot.revision);
		if (prepared == flatfile_item_repository_result::ok)
			operations.push_back(std::move(quarantine));
		else if (prepared != flatfile_item_repository_result::unchanged)
			return { prepared == flatfile_item_repository_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0, EIO };
	}
	if (snapshot.death || !snapshot.spell_effect_receipts.empty() ||
	    !snapshot.quest_xp_receipts.empty() || !snapshot.craft_receipts.empty())
	{
		operations.push_back({ flatfile_authority_store::players,
				       flatfile_authority_operation_kind::write,
				       player_filename(snapshot.pid), std::move(bytes) });
		const auto committed = flatfile_authority_transaction_commit_operations(
			root, authority, operations, error);
		if (committed != flatfile_authority_transaction_result::ok)
			return { committed == flatfile_authority_transaction_result::io_error ?
					 player_save_apply_outcome::retryable_failure :
					 player_save_apply_outcome::terminal_failure,
				 0, EIO };
		return { player_save_apply_outcome::applied, snapshot.revision, 0 };
	}
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
