#include "persistence/death_recovery_visibility.h"
#include "flatfile/quest_mobile_native_flatfile.h"
#include "item/item_transfer_repository.h"
#include "flatfile/flatfile_item_repository.h"

#include "flatfile/flatfile_auction_repository.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_corpse_repository.h"
#include "flatfile/flatfile_boon_repository.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_collector_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_store.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "item/craft_pouch_mutation.h"
#include "flatfile/flatfile_craft_progression.h"
#include "flatfile/flatfile_shop_trade_repository.h"
#include "persistence/persistence_mode.h"
#include "economy/coin_transfer_command.h"
#include "economy/item_transfer_accounting.h"
#include "economy/smith_native_accounting.h"
#include "item/smith_native_compound_images.h"
#include "economy/economic_accounting_intent.h"
#include "economy/native_mobile_birth_recovery.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "item/quest_reward_continuation.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"
#include "core/defines.h"

#include <algorithm>
#include <array>
#include <cerrno>

__attribute__((weak)) flatfile_item_accounting_status flatfile_item_accounting_reference_append(
	const std::string &root, const economic_accounting_item_reference &ref, std::string *error)
{
	(void)root;
	(void)ref;
	if (error)
		error->clear();
	return flatfile_item_accounting_status::ok;
}
#include <cstring>
#include <iterator>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <bit>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13
#include <bits/hashtable_policy.h>
#endif
#include <tuple>
#include <utility>

namespace
{
constexpr uint32_t ownership_format_version = 8;
constexpr uint32_t ownership_legacy_format_version = 1;
constexpr std::array<uint8_t, 8> ownership_magic = { 'D', 'U', 'R', 'O', 'W', 'N', 0, 0 };
constexpr size_t ownership_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t ownership_maximum_entries = 262144;
constexpr size_t ownership_maximum_operations = 1048576;
constexpr const char *ownership_filename = "item_ownership";
std::mutex ownership_mutex;

struct owner_state
{
	item_owner_identity owner;
	uint64_t revision;
};

struct operation_state
{
	critical_operation_id operation_id;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest;
	unsigned int result_code;
	item_transfer_result result;
	bool coin_operation = false;
	std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> coin_result = {};
	std::vector<uint8_t> quest_continuation = {};
	bool quest_reward_acknowledged = false;
	uint64_t quest_xp_applied_mask = 0;
	// Revisions follow the set bits in increasing slot order. Version 5 slots
	// identify frozen recipient awards; version 4 slots identify solo rewards.
	std::vector<player_revision_t> quest_xp_revisions = {};
	uint64_t creation_source_id = 0;
	uint32_t creation_recipient_pid = 0;
	int32_t creation_vnum = 0;
	bool quest_legacy_economic_history = false;
};

struct ownership_catalog
{
	uint64_t revision = 0;
	std::vector<owner_state> owners;
	std::vector<flatfile_item_ownership_record> items;
	std::vector<operation_state> operations;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		if (!valid)
			return;
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		try
		{
			for (size_t index = 0; index < sizeof(T); ++index)
			{
				bytes.push_back(static_cast<uint8_t>(bits & 0xff));
				bits >>= 8;
			}
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	void raw(const uint8_t *data, size_t size)
	{
		if (!valid || (!data && size))
		{
			valid = false;
			return;
		}
		try
		{
			bytes.insert(bytes.end(), data, data + size);
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
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

	bool raw(uint8_t *output, size_t count)
	{
		if (!output || size - offset < count)
			return false;
		memcpy(output, data + offset, count);
		offset += count;
		return true;
	}
};

struct operation_id_hash
{
	size_t operator()(const std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> &value) const
	{
		size_t result = 0;
		for (uint8_t byte : value)
			result = result * 131 + byte;
		return result;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool owner_less(const item_owner_identity &left, const item_owner_identity &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	if (left.id != right.id)
		return left.id < right.id;
	return left.context_id < right.context_id;
}

bool item_less(const flatfile_item_ownership_record &left,
	       const flatfile_item_ownership_record &right)
{
	return left.item_uid < right.item_uid;
}

bool item_equal(const flatfile_item_ownership_record &left,
		const flatfile_item_ownership_record &right)
{
	return left.item_uid == right.item_uid && left.root_item_uid == right.root_item_uid &&
	       left.parent_item_uid == right.parent_item_uid &&
	       item_owner_identity_equal(left.owner, right.owner) &&
	       left.item_revision == right.item_revision && left.vnum == right.vnum &&
	       left.state == right.state && left.equipment_slot == right.equipment_slot;
}

owner_state *find_owner(ownership_catalog *catalog, const item_owner_identity &owner)
{
	if (!catalog)
		return nullptr;
	auto found =
		std::lower_bound(catalog->owners.begin(), catalog->owners.end(), owner,
				 [](const owner_state &entry, const item_owner_identity &candidate)
				 { return owner_less(entry.owner, candidate); });
	return found != catalog->owners.end() && item_owner_identity_equal(found->owner, owner) ?
		       &*found :
		       nullptr;
}

owner_state *ensure_owner(ownership_catalog *catalog, const item_owner_identity &owner)
{
	if (owner_state *existing = find_owner(catalog, owner))
		return existing;
	if (!catalog || catalog->owners.size() >= ownership_maximum_entries)
		return nullptr;
	auto at =
		std::lower_bound(catalog->owners.begin(), catalog->owners.end(), owner,
				 [](const owner_state &entry, const item_owner_identity &candidate)
				 { return owner_less(entry.owner, candidate); });
	try
	{
		at = catalog->owners.insert(at, { owner, 0 });
	}
	catch (const std::bad_alloc &)
	{
		return nullptr;
	}
	return &*at;
}

flatfile_item_ownership_record *find_item(ownership_catalog *catalog, uint64_t item_uid)
{
	if (!catalog)
		return nullptr;
	auto found =
		std::lower_bound(catalog->items.begin(), catalog->items.end(), item_uid,
				 [](const flatfile_item_ownership_record &entry, uint64_t candidate)
				 { return entry.item_uid < candidate; });
	return found != catalog->items.end() && found->item_uid == item_uid ? &*found : nullptr;
}

bool encode_catalog(const ownership_catalog &catalog, uint64_t revision,
		    std::vector<uint8_t> *bytes)
{
	if (!bytes || !revision || catalog.owners.size() > ownership_maximum_entries ||
	    catalog.items.size() > ownership_maximum_entries ||
	    catalog.operations.size() > ownership_maximum_operations)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.owners.size());
	payload.number<uint32_t>(catalog.items.size());
	payload.number<uint32_t>(catalog.operations.size());
	for (const owner_state &entry : catalog.owners)
	{
		payload.number<uint8_t>(static_cast<uint8_t>(entry.owner.type));
		payload.number(entry.owner.id);
		payload.number(entry.owner.context_id);
		payload.number(entry.revision);
	}
	for (const flatfile_item_ownership_record &entry : catalog.items)
	{
		payload.number(entry.item_uid);
		payload.number(entry.root_item_uid);
		payload.number(entry.parent_item_uid);
		payload.number<uint8_t>(static_cast<uint8_t>(entry.owner.type));
		payload.number(entry.owner.id);
		payload.number(entry.owner.context_id);
		payload.number(entry.item_revision);
		payload.number(entry.vnum);
		payload.number<uint8_t>(static_cast<uint8_t>(entry.state));
		if (entry.coin_payload.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
			return false;
		payload.number<uint32_t>(entry.coin_payload.size());
		if (!entry.coin_payload.empty())
			payload.raw(entry.coin_payload.data(), entry.coin_payload.size());
		payload.number(entry.equipment_slot);
	}
	for (const operation_state &entry : catalog.operations)
	{
		payload.raw(entry.operation_id.bytes.data(), entry.operation_id.bytes.size());
		payload.raw(entry.command_digest.data(), entry.command_digest.size());
		payload.number<uint32_t>(entry.result_code);
		payload.number(entry.result.root_item_uid);
		payload.number(entry.result.item_count);
		payload.number(entry.result.from_owner_revision);
		payload.number(entry.result.to_owner_revision);
		payload.number(entry.result.max_item_revision);
		payload.number(entry.result.corpse_revision);
		payload.number<uint8_t>(entry.result.collector_catalog_changed ? 1 : 0);
		payload.number<uint8_t>(entry.coin_operation ? 1 : 0);
		if (entry.coin_operation)
			payload.raw(entry.coin_result.data(), entry.coin_result.size());
		payload.number<uint32_t>(entry.quest_continuation.size());
		if (!entry.quest_continuation.empty())
			payload.raw(entry.quest_continuation.data(),
				    entry.quest_continuation.size());
		payload.number<uint8_t>(entry.quest_reward_acknowledged ? 1 : 0);
		payload.number(entry.quest_xp_applied_mask);
		for (auto revision : entry.quest_xp_revisions)
			payload.number(revision);
		payload.number(entry.creation_source_id);
		payload.number(entry.creation_recipient_pid);
		payload.number(entry.creation_vnum);
		payload.number<uint8_t>(entry.quest_legacy_economic_history ? 1 : 0);
	}
	if (!payload.valid || payload.bytes.size() > ownership_maximum_bytes)
		return false;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.bytes.data(), payload.bytes.size(), digest);
	encoder file;
	file.raw(ownership_magic.data(), ownership_magic.size());
	file.number<uint32_t>(ownership_format_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(revision);
	file.raw(digest, sizeof(digest));
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > ownership_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

uint64_t quest_xp_slots(const quest_reward_continuation &terms, uint32_t pid)
{
	uint64_t mask = 0;
	if (terms.version >= 5)
	{
		for (size_t index = 0; index < terms.xp_award_count; ++index)
			if (!pid || terms.xp_awards[index].recipient_pid == pid)
				mask |= UINT64_C(1) << index;
	}
	else if (terms.version == 4 && terms.credited_count == 1 &&
		 (!pid || terms.player_pid == pid))
		for (size_t index = 0; index < terms.reward_count; ++index)
			if (terms.rewards[index].type == 5U)
				mask |= UINT64_C(1) << index;
	return mask;
}

uint64_t quest_owner_xp_mask(const operation_state &entry, const quest_reward_continuation &terms)
{
	if (terms.version < 5)
		return entry.quest_xp_applied_mask;
	uint64_t mask = 0;
	for (size_t index = 0; index < terms.xp_award_count; ++index)
		if (terms.xp_awards[index].recipient_pid == terms.player_pid &&
		    (entry.quest_xp_applied_mask & (UINT64_C(1) << index)))
			mask |= UINT64_C(1) << terms.xp_awards[index].reward_index;
	return mask;
}

bool valid_quest_operation(const operation_state &entry)
{
	if (entry.quest_continuation.empty())
		return !entry.quest_reward_acknowledged && !entry.quest_xp_applied_mask &&
		       entry.quest_xp_revisions.empty() && !entry.quest_legacy_economic_history;
	quest_reward_continuation terms;
	return !entry.coin_operation && !entry.result_code &&
	       quest_reward_continuation_decode(entry.quest_continuation.data(),
						entry.quest_continuation.size(), &terms) &&
	       !(entry.quest_xp_applied_mask & ~quest_xp_slots(terms, 0)) &&
	       entry.quest_xp_revisions.size() ==
		       static_cast<size_t>(std::popcount(entry.quest_xp_applied_mask)) &&
	       std::all_of(entry.quest_xp_revisions.begin(), entry.quest_xp_revisions.end(),
			   [](player_revision_t revision) { return revision != 0; });
}

bool valid_catalog(const ownership_catalog &catalog)
{
	if (!std::is_sorted(catalog.owners.begin(), catalog.owners.end(),
			    [](const owner_state &left, const owner_state &right)
			    { return owner_less(left.owner, right.owner); }) ||
	    !std::is_sorted(catalog.items.begin(), catalog.items.end(), item_less))
		return false;
	for (size_t index = 0; index < catalog.owners.size(); ++index)
		if (!item_owner_identity_valid(catalog.owners[index].owner) ||
		    (index && item_owner_identity_equal(catalog.owners[index - 1].owner,
							catalog.owners[index].owner)))
			return false;
	for (size_t index = 0; index < catalog.items.size(); ++index)
	{
		const auto &entry = catalog.items[index];
		if (!entry.item_uid || !entry.root_item_uid || entry.vnum < 0 ||
		    (!entry.vnum && entry.owner.type != item_owner_type::room) ||
		    !item_owner_identity_valid(entry.owner) ||
		    entry.state == item_custody_state::absent ||
		    entry.state > item_custody_state::quarantined ||
		    (index && catalog.items[index - 1].item_uid == entry.item_uid) ||
		    !find_owner(const_cast<ownership_catalog *>(&catalog), entry.owner))
			return false;
		if (entry.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT ||
		    (entry.equipment_slot &&
		     ((entry.owner.type != item_owner_type::player &&
		       entry.owner.type != item_owner_type::native_mobile &&
		       entry.owner.type != item_owner_type::shopkeeper) ||
		      entry.parent_item_uid || entry.state != item_custody_state::active ||
		      ((entry.owner.type == item_owner_type::native_mobile ||
			entry.owner.type == item_owner_type::shopkeeper) &&
		       entry.root_item_uid != entry.item_uid))))
			return false;
		if (!entry.coin_payload.empty())
		{
			std::vector<player_item_snapshot> items;
			if (entry.coin_payload.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
			    player_item_snapshot_list_decode(entry.coin_payload.data(),
							     entry.coin_payload.size(), &items) !=
				    player_snapshot_codec_result::ok ||
			    items.size() != 1 || items[0].object_uid != entry.item_uid ||
			    items[0].vnum != entry.vnum || items[0].type != ITEM_MONEY)
				return false;
		}
	}
	std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, operation_id_hash>
		operation_ids;
	try
	{
		operation_ids.reserve(catalog.operations.size());
		for (const operation_state &entry : catalog.operations)
			if (critical_operation_id_is_zero(entry.operation_id) ||
			    entry.quest_continuation.size() >
				    ITEM_TRANSFER_CONTINUATION_MAX_BYTES ||
			    !valid_quest_operation(entry) ||
			    ((entry.creation_source_id == 0) !=
			     (entry.creation_recipient_pid == 0)) ||
			    (!entry.creation_source_id && entry.creation_vnum) ||
			    (entry.creation_source_id &&
			     (entry.coin_operation || entry.result_code ||
			      entry.creation_vnum <= 0 || entry.result.max_item_revision != 1)) ||
			    (!entry.coin_operation &&
			     (!entry.result.root_item_uid || !entry.result.item_count ||
			      entry.result.item_count > ITEM_TRANSFER_MAX_ITEMS)) ||
			    !operation_ids.insert(entry.operation_id.bytes).second)
				return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

flatfile_item_repository_result decode_catalog(const std::vector<uint8_t> &bytes,
					       ownership_catalog *catalog)
{
	constexpr size_t header_size = ownership_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(uint64_t) + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), ownership_magic.data(), ownership_magic.size()))
		return flatfile_item_repository_result::invalid;
	decoder header{ bytes.data() + ownership_magic.size(),
			bytes.size() - ownership_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) ||
	    (version < ownership_legacy_format_version || version > ownership_format_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return flatfile_item_repository_result::invalid;
	const uint8_t *stored_digest =
		bytes.data() + ownership_magic.size() + sizeof(uint32_t) * 2 + sizeof(uint64_t);
	const uint8_t *payload = bytes.data() + header_size;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_item_repository_result::invalid;
	decoder input{ payload, payload_size };
	uint32_t owner_count = 0, item_count = 0, operation_count = 0;
	if (!input.number(&owner_count) || !input.number(&item_count) ||
	    !input.number(&operation_count) || owner_count > ownership_maximum_entries ||
	    item_count > ownership_maximum_entries ||
	    operation_count > ownership_maximum_operations)
		return flatfile_item_repository_result::invalid;
	ownership_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.owners.resize(owner_count);
		decoded.items.resize(item_count);
		decoded.operations.resize(operation_count);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	for (owner_state &entry : decoded.owners)
	{
		uint8_t type = 0;
		if (!input.number(&type) || !input.number(&entry.owner.id) ||
		    !input.number(&entry.owner.context_id) || !input.number(&entry.revision))
			return flatfile_item_repository_result::invalid;
		entry.owner.type = static_cast<item_owner_type>(type);
	}
	for (flatfile_item_ownership_record &entry : decoded.items)
	{
		uint8_t type = 0, state = 0;
		if (!input.number(&entry.item_uid) || !input.number(&entry.root_item_uid) ||
		    !input.number(&entry.parent_item_uid) || !input.number(&type) ||
		    !input.number(&entry.owner.id) || !input.number(&entry.owner.context_id) ||
		    !input.number(&entry.item_revision) || !input.number(&entry.vnum) ||
		    !input.number(&state))
			return flatfile_item_repository_result::invalid;
		entry.owner.type = static_cast<item_owner_type>(type);
		entry.state = static_cast<item_custody_state>(state);
		if (version >= 3)
		{
			uint32_t size = 0;
			if (!input.number(&size) || size > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
			    size > input.size - input.offset)
				return flatfile_item_repository_result::invalid;
			try
			{
				entry.coin_payload.resize(size);
			}
			catch (const std::bad_alloc &)
			{
				return flatfile_item_repository_result::io_error;
			}
			if (size && !input.raw(entry.coin_payload.data(), size))
				return flatfile_item_repository_result::invalid;
		}
		if (version >= 5 && !input.number(&entry.equipment_slot))
			return flatfile_item_repository_result::invalid;
	}
	for (operation_state &entry : decoded.operations)
	{
		if (!input.raw(entry.operation_id.bytes.data(), entry.operation_id.bytes.size()) ||
		    !input.raw(entry.command_digest.data(), entry.command_digest.size()) ||
		    !input.number(&entry.result_code) ||
		    !input.number(&entry.result.root_item_uid) ||
		    !input.number(&entry.result.item_count) ||
		    !input.number(&entry.result.from_owner_revision) ||
		    !input.number(&entry.result.to_owner_revision) ||
		    !input.number(&entry.result.max_item_revision) ||
		    (version >= 2 && !input.number(&entry.result.corpse_revision)))
			return flatfile_item_repository_result::invalid;
		if (version >= 4)
		{
			uint8_t collector_changed = 0;
			if (!input.number(&collector_changed) || collector_changed > 1)
				return flatfile_item_repository_result::invalid;
			entry.result.collector_catalog_changed = collector_changed != 0;
		}
		if (version >= 3)
		{
			uint8_t coin = 0;
			if (!input.number(&coin) || coin > 1 ||
			    (coin &&
			     !input.raw(entry.coin_result.data(), entry.coin_result.size())))
				return flatfile_item_repository_result::invalid;
			entry.coin_operation = coin != 0;
		}
		if (version >= 6)
		{
			uint32_t size = 0;
			uint8_t acknowledged = 0;
			if (!input.number(&size) || size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES ||
			    size > input.size - input.offset)
				return flatfile_item_repository_result::invalid;
			try
			{
				entry.quest_continuation.resize(size);
			}
			catch (const std::bad_alloc &)
			{
				return flatfile_item_repository_result::io_error;
			}
			if ((size && !input.raw(entry.quest_continuation.data(), size)) ||
			    !input.number(&acknowledged) || acknowledged > 1)
				return flatfile_item_repository_result::invalid;
			entry.quest_reward_acknowledged = acknowledged != 0;
		}
		if (version >= 7)
		{
			if (!input.number(&entry.quest_xp_applied_mask))
				return flatfile_item_repository_result::invalid;
			try
			{
				entry.quest_xp_revisions.resize(
					std::popcount(entry.quest_xp_applied_mask));
			}
			catch (const std::bad_alloc &)
			{
				return flatfile_item_repository_result::io_error;
			}
			for (auto &revision : entry.quest_xp_revisions)
				if (!input.number(&revision))
					return flatfile_item_repository_result::invalid;
		}
		if (version >= 8)
		{
			uint8_t legacy = 0;
			if (!input.number(&entry.creation_source_id) ||
			    !input.number(&entry.creation_recipient_pid) ||
			    !input.number(&entry.creation_vnum) || !input.number(&legacy) ||
			    legacy > 1)
				return flatfile_item_repository_result::invalid;
			entry.quest_legacy_economic_history = legacy != 0;
		}
		else
			entry.quest_legacy_economic_history = !entry.quest_continuation.empty();
	}
	if (input.offset != input.size || !valid_catalog(decoded))
		return flatfile_item_repository_result::invalid;
	*catalog = std::move(decoded);
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result load_catalog(const std::string &root, ownership_catalog *catalog,
					     std::string *error)
{
	if (!catalog)
		return flatfile_item_repository_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read = flatfile_read(domains_directory(root), ownership_filename,
							ownership_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
	{
		*catalog = {};
		return flatfile_item_repository_result::not_found;
	}
	if (read == flatfile_read_result::invalid)
		return flatfile_item_repository_result::invalid;
	if (read != flatfile_read_result::ok)
		return flatfile_item_repository_result::io_error;
	return decode_catalog(bytes, catalog);
}

critical_apply_result make_result(critical_apply_outcome outcome, unsigned int error_code,
				  const item_transfer_result &result)
{
	critical_apply_result applied = {
		outcome,
		std::max({ result.from_owner_revision, result.to_owner_revision,
			   result.max_item_revision, result.corpse_revision }),
		error_code
	};
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded = {};
	if (!item_transfer_command_encode_result(result, &encoded))
		return { critical_apply_outcome::terminal_failure, 0, EBADMSG };
	applied.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), applied.result_payload.begin());
	return applied;
}

bool command_digest(const critical_command &command,
		    std::array<uint8_t, SHA256_DIGEST_LENGTH> *digest)
{
	std::vector<uint8_t> encoded;
	if (!digest ||
	    critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return false;
	SHA256(encoded.data(), encoded.size(), digest->data());
	return true;
}

bool generic_materialization_owner(item_owner_type type)
{
	return type == item_owner_type::player || type == item_owner_type::system ||
	       type == item_owner_type::destruction;
}

bool locker_transfer(const item_transfer_payload &payload)
{
	return (payload.from_owner.type == item_owner_type::player &&
		payload.to_owner.type == item_owner_type::locker &&
		payload.reason == item_transfer_reason::locker_deposit) ||
	       (payload.from_owner.type == item_owner_type::locker &&
		payload.to_owner.type == item_owner_type::player &&
		payload.reason == item_transfer_reason::locker_withdraw);
}

bool corpse_loot_transfer(const item_transfer_payload &payload)
{
	return payload.from_owner.type == item_owner_type::corpse &&
	       payload.to_owner.type == item_owner_type::player &&
	       payload.reason == item_transfer_reason::corpse_loot;
}

bool corpse_create_transfer(const item_transfer_payload &payload)
{
	return payload.from_owner.type == item_owner_type::player &&
	       payload.to_owner.type == item_owner_type::corpse &&
	       payload.reason == item_transfer_reason::corpse_create;
}

bool room_transfer(const item_transfer_payload &payload)
{
	const bool deposit = payload.from_owner.type == item_owner_type::player &&
			     payload.to_owner.type == item_owner_type::room &&
			     (((payload.reason == item_transfer_reason::player_drop ||
				item_transfer_forced_weapon_drop(payload.reason)) &&
			       !payload.target_parent_item_uid) ||
			      (payload.reason == item_transfer_reason::player_put &&
			       payload.target_parent_item_uid));
	const bool withdraw = payload.from_owner.type == item_owner_type::room &&
			      payload.to_owner.type == item_owner_type::player &&
			      payload.reason == item_transfer_reason::player_get &&
			      !payload.target_parent_item_uid;
	const bool create = payload.from_owner.type == item_owner_type::system &&
			    payload.to_owner.type == item_owner_type::room &&
			    payload.reason == item_transfer_reason::creation &&
			    !payload.target_parent_item_uid;
	const bool destroy = payload.from_owner.type == item_owner_type::room &&
			     payload.to_owner.type == item_owner_type::destruction &&
			     payload.reason == item_transfer_reason::destruction &&
			     !payload.target_parent_item_uid;
	const bool reparent = item_owner_identity_equal(payload.from_owner, payload.to_owner) &&
			      payload.from_owner.type == item_owner_type::room &&
			      payload.reason == item_transfer_reason::operator_repair &&
			      !payload.target_parent_item_uid;
	return static_cast<unsigned int>(deposit) + static_cast<unsigned int>(withdraw) +
		       static_cast<unsigned int>(create) + static_cast<unsigned int>(destroy) +
		       static_cast<unsigned int>(reparent) ==
	       1;
}

bool generic_transfer_supported(const item_transfer_payload &payload, uint16_t payload_version,
				bool accounted)
{
	// Read compatibility is not native NPC stock mutation/admission authority.
	if (payload.from_owner.type == item_owner_type::native_mobile ||
	    payload.to_owner.type == item_owner_type::native_mobile)
		return false;
	const bool mobile_claim = payload.reason == item_transfer_reason::mobile_claim &&
				  item_owner_identity_equal(payload.from_owner, payload.to_owner) &&
				  !payload.multi_root && !payload.target_parent_item_uid;
	const bool pet_give = payload.reason == item_transfer_reason::pet_give &&
			      payload.from_owner.type == item_owner_type::player &&
			      payload.to_owner.type == item_owner_type::pet &&
			      payload.from_owner.id == payload.to_owner.context_id &&
			      !payload.multi_root && !payload.target_parent_item_uid;
	const bool pet_return = payload.reason == item_transfer_reason::pet_return &&
				payload.from_owner.type == item_owner_type::pet &&
				payload.to_owner.type == item_owner_type::player &&
				payload.from_owner.context_id == payload.to_owner.id &&
				!payload.multi_root && !payload.target_parent_item_uid;
	const bool accounted_container_destruction =
		accounted && payload.from_owner.type == item_owner_type::container &&
		payload.to_owner.type == item_owner_type::destruction &&
		payload.reason == item_transfer_reason::destruction && !payload.multi_root &&
		!payload.target_parent_item_uid;
	const bool accounted_player_quest_turnin =
		accounted && payload.from_owner.type == item_owner_type::player &&
		payload.to_owner.type == item_owner_type::destruction &&
		payload.reason == item_transfer_reason::quest_turnin && payload.multi_root &&
		payload.continuation.kind == item_transfer_continuation_kind::quest_offering;
	const bool craft = payload.reason == item_transfer_reason::craft;
	return craft ||
	       (generic_materialization_owner(payload.from_owner.type) &&
		generic_materialization_owner(payload.to_owner.type)) ||
	       mobile_claim || pet_give || pet_return || accounted_container_destruction ||
	       accounted_player_quest_turnin || locker_transfer(payload) ||
	       corpse_loot_transfer(payload) ||
	       (payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION && room_transfer(payload)) ||
	       (payload_version >= ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION &&
		corpse_create_transfer(payload));
}

bool locker_custody_matches(ownership_catalog &catalog, const item_owner_identity &owner,
			    const std::vector<flatfile_locker_custody_item> &expected)
{
	if (!find_owner(&catalog, owner))
		return false;
	size_t index = 0;
	for (const auto &item : catalog.items)
	{
		if (item.state != item_custody_state::active ||
		    !item_owner_identity_equal(item.owner, owner))
			continue;
		if (index >= expected.size() || expected[index].item_uid != item.item_uid ||
		    expected[index].vnum != item.vnum)
			return false;
		++index;
	}
	return index == expected.size();
}

bool corpse_custody_matches(ownership_catalog &catalog, const item_owner_identity &owner,
			    const std::vector<flatfile_corpse_custody_item> &expected, bool created)
{
	const owner_state *stored_owner = find_owner(&catalog, owner);
	if (!stored_owner)
		// writeCorpse() publishes the empty aggregate before the first item handoff.
		// In that ordering the world corpse exists while its custody owner does not;
		// the first transfer is what creates that owner.  A non-empty aggregate still
		// requires matching custody and therefore remains fail-closed here.
		return expected.empty();
	if (created && (stored_owner->revision || !expected.empty()))
		return false;
	size_t index = 0;
	for (const auto &item : catalog.items)
	{
		if (item.state != item_custody_state::active ||
		    !item_owner_identity_equal(item.owner, owner))
			continue;
		if (index >= expected.size() || expected[index].item_uid != item.item_uid ||
		    expected[index].vnum != item.vnum)
			return false;
		++index;
	}
	return index == expected.size();
}

bool room_custody_matches(ownership_catalog &catalog, const item_owner_identity &owner,
			  const std::vector<flatfile_corpse_custody_item> &expected, bool created)
{
	const owner_state *stored_owner = find_owner(&catalog, owner);
	if (!stored_owner)
		return created && expected.empty();
	if (created && (stored_owner->revision || !expected.empty()))
		return false;
	size_t index = 0;
	for (const auto &item : catalog.items)
	{
		if (item.state != item_custody_state::active ||
		    !item_owner_identity_equal(item.owner, owner))
			continue;
		if (index >= expected.size() || expected[index].item_uid != item.item_uid ||
		    expected[index].vnum != item.vnum ||
		    expected[index].root_item_uid != item.root_item_uid ||
		    expected[index].parent_item_uid != item.parent_item_uid)
			return false;
		++index;
	}
	return index == expected.size();
}

bool descendant_of(const std::vector<flatfile_item_ownership_record *> &root_items,
		   const flatfile_item_ownership_record &candidate, uint64_t selected_uid)
{
	uint64_t ancestor = candidate.item_uid;
	for (size_t depth = 0; depth <= root_items.size(); ++depth)
	{
		if (ancestor == selected_uid)
			return true;
		auto parent = std::find_if(root_items.begin(), root_items.end(),
					   [ancestor](const auto *entry)
					   { return entry->item_uid == ancestor; });
		if (parent == root_items.end() || !(*parent)->parent_item_uid)
			return false;
		ancestor = (*parent)->parent_item_uid;
	}
	return false;
}

unsigned int apply_transfer(ownership_catalog *catalog, const item_transfer_payload &payload,
			    item_transfer_result *result)
{
	if (!catalog || !result)
		return EINVAL;
	if (!ensure_owner(catalog, payload.from_owner) || !ensure_owner(catalog, payload.to_owner))
		return ENOSPC;
	owner_state *from_owner = find_owner(catalog, payload.from_owner);
	owner_state *to_owner = find_owner(catalog, payload.to_owner);
	if (!from_owner || !to_owner)
		return EILSEQ;
	const bool same_owner = item_owner_identity_equal(payload.from_owner, payload.to_owner);
	*result = { item_transfer_result_root(payload),
		    payload.item_count,
		    from_owner->revision,
		    to_owner->revision,
		    0,
		    0 };
	if (from_owner->revision != payload.expected_from_revision ||
	    to_owner->revision != payload.expected_to_revision)
		return ESTALE;
	const bool creation = payload.from_owner.type == item_owner_type::system;
	std::vector<flatfile_item_ownership_record *> root_items;
	std::vector<flatfile_item_ownership_record *> selected;
	try
	{
		std::vector<uint64_t> source_roots;
		for (size_t index = 0; index < payload.item_count; ++index)
			source_roots.push_back(payload.items[index].root_item_uid);
		std::sort(source_roots.begin(), source_roots.end());
		source_roots.erase(std::unique(source_roots.begin(), source_roots.end()),
				   source_roots.end());
		for (auto &entry : catalog->items)
			if (std::binary_search(source_roots.begin(), source_roots.end(),
					       entry.root_item_uid))
				root_items.push_back(&entry);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	if ((creation && !root_items.empty()) || (!creation && root_items.empty()))
		return creation ? EEXIST : ENOENT;
	if (creation)
	{
		for (size_t index = 0; index < payload.item_count; ++index)
			if (find_item(catalog, payload.items[index].item_uid))
				return EEXIST;
		if (catalog->items.size() > ownership_maximum_entries - payload.item_count)
			return ENOSPC;
	}
	else
	{
		std::vector<uint64_t> selected_roots;
		try
		{
			for (size_t index = 0; index < payload.item_count; ++index)
				if (item_transfer_selected_root(payload,
								payload.items[index].item_uid) ==
				    payload.items[index].item_uid)
					selected_roots.push_back(payload.items[index].item_uid);
		}
		catch (const std::bad_alloc &)
		{
			return ENOMEM;
		}
		for (auto *entry : root_items)
			for (uint64_t selected_root : selected_roots)
				if (descendant_of(root_items, *entry, selected_root))
				{
					selected.push_back(entry);
					break;
				}
		if (selected.size() != payload.item_count)
			return ITEM_TRANSFER_TOPOLOGY_CARDINALITY;
		std::sort(selected.begin(), selected.end(), [](const auto *left, const auto *right)
			  { return left->item_uid < right->item_uid; });
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &stored = *selected[index];
			const auto &expected = payload.items[index];
			result->max_item_revision =
				std::max(result->max_item_revision, stored.item_revision);
			if (stored.item_uid != expected.item_uid ||
			    stored.root_item_uid != expected.root_item_uid ||
			    stored.parent_item_uid != expected.parent_item_uid ||
			    !item_owner_identity_equal(stored.owner, payload.from_owner) ||
			    stored.item_revision != expected.expected_item_revision ||
			    stored.vnum != expected.vnum || stored.state != expected.expected_state)
				return ESTALE;
		}
	}
	if (payload.reason == item_transfer_reason::player_wear ||
	    payload.reason == item_transfer_reason::player_remove)
	{
		const auto root = find_item(catalog, payload.selected_item_uid);
		const uint16_t before_slot = payload.reason == item_transfer_reason::player_remove ?
						     static_cast<uint16_t>(payload.reason_id) :
						     0;
		const uint16_t after_slot = payload.reason == item_transfer_reason::player_wear ?
						    static_cast<uint16_t>(payload.reason_id) :
						    0;
		if (!root || root->owner.type != item_owner_type::player ||
		    !item_owner_identity_equal(root->owner, payload.from_owner) ||
		    root->equipment_slot != before_slot || root->parent_item_uid ||
		    std::any_of(selected.begin(), selected.end(),
				[&](const auto *item) {
					return item->item_uid != payload.selected_item_uid &&
					       item->equipment_slot != 0;
				}))
			return ESTALE;
		if (after_slot &&
		    std::any_of(catalog->items.begin(), catalog->items.end(),
				[&](const auto &item)
				{
					return item.item_uid != payload.selected_item_uid &&
					       item_owner_identity_equal(item.owner,
									 payload.to_owner) &&
					       item.equipment_slot == after_slot;
				}))
			return ESTALE;
		std::vector<player_item_snapshot> snapshots;
		if (!payload.item_blob_size ||
		    player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size, &snapshots) !=
			    player_snapshot_codec_result::ok ||
		    snapshots.size() != payload.item_count ||
		    snapshots[0].object_uid != payload.selected_item_uid ||
		    snapshots[0].equipment_slot != after_slot)
			return EBADMSG;
	}
	if (item_transfer_forced_weapon_drop(payload.reason))
	{
		const auto root = find_item(catalog, payload.selected_item_uid);
		if (!root || root->owner.type != item_owner_type::player ||
		    !item_owner_identity_equal(root->owner, payload.from_owner) ||
		    root->parent_item_uid ||
		    root->equipment_slot != static_cast<uint16_t>(payload.reason_id))
			return ESTALE;
	}
	if (payload.target_parent_item_uid)
	{
		const auto *parent = find_item(catalog, payload.target_parent_item_uid);
		if (!parent || parent->root_item_uid != payload.target_root_item_uid ||
		    !item_owner_identity_equal(parent->owner, payload.to_owner) ||
		    parent->item_revision != payload.expected_target_parent_revision ||
		    parent->state != item_custody_state::active)
			return ESTALE;
	}
	if (from_owner->revision == std::numeric_limits<uint64_t>::max() ||
	    (!same_owner && to_owner->revision == std::numeric_limits<uint64_t>::max()))
		return ERANGE;
	if (creation)
	{
		try
		{
			for (size_t index = 0; index < payload.item_count; ++index)
			{
				const auto &entry = payload.items[index];
				uint64_t target_root = 0, target_parent = 0;
				if (!item_transfer_target_topology(payload, entry.item_uid,
								   &target_root, &target_parent))
					return EINVAL;
				catalog->items.push_back({ entry.item_uid, target_root,
							   target_parent, payload.to_owner, 1,
							   entry.vnum,
							   item_custody_state::active });
				result->max_item_revision = 1;
			}
			std::sort(catalog->items.begin(), catalog->items.end(), item_less);
		}
		catch (const std::bad_alloc &)
		{
			return ENOMEM;
		}
	}
	else
	{
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			uint64_t target_root = 0, target_parent = 0;
			if (selected[index]->item_revision ==
				    std::numeric_limits<uint64_t>::max() ||
			    !item_transfer_target_topology(payload, selected[index]->item_uid,
							   &target_root, &target_parent))
				return selected[index]->item_revision ==
						       std::numeric_limits<uint64_t>::max() ?
					       ERANGE :
					       EINVAL;
		}
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			auto &entry = *selected[index];
			uint64_t target_root = 0, target_parent = 0;
			if (!item_transfer_target_topology(payload, entry.item_uid, &target_root,
							   &target_parent))
				return EINVAL;
			++entry.item_revision;
			entry.root_item_uid = target_root;
			entry.parent_item_uid = target_parent;
			entry.owner = payload.to_owner;
			entry.state = payload.to_owner.type == item_owner_type::destruction ?
					      item_custody_state::destroyed :
					      item_custody_state::active;
			entry.equipment_slot =
				payload.reason == item_transfer_reason::player_wear &&
						entry.item_uid == payload.selected_item_uid ?
					static_cast<uint16_t>(payload.reason_id) :
					0;
			result->max_item_revision =
				std::max(result->max_item_revision, entry.item_revision);
		}
	}
	++from_owner->revision;
	if (!same_owner)
		++to_owner->revision;
	result->from_owner_revision = from_owner->revision;
	result->to_owner_revision = same_owner ? from_owner->revision : to_owner->revision;
	return 0;
}

economic_item_position accounting_position(const flatfile_item_ownership_record &item)
{
	return { item.owner,	     item.root_item_uid, item.parent_item_uid,
		 item.item_revision, item.state,	 item.equipment_slot };
}

const flatfile_item_ownership_record *catalog_item(const ownership_catalog &catalog, uint64_t uid)
{
	const auto found = std::lower_bound(catalog.items.begin(), catalog.items.end(), uid,
					    [](const auto &item, uint64_t key)
					    { return item.item_uid < key; });
	return found != catalog.items.end() && found->item_uid == uid ? &*found : nullptr;
}

bool build_item_accounting_references(const critical_command &command,
				      const economic_accounting_plan &plan,
				      std::vector<economic_accounting_item_reference> *references)
{
	if (!references)
		return false;
	std::vector<economic_accounting_item_reference> candidate;
	try
	{
		candidate.reserve(plan.item_events.size());
		for (const auto &event : plan.item_events)
		{
			if (event.event_index >= 3000 || event.event_index > UINT16_MAX)
				return false;
			economic_accounting_item_reference reference = {};
			reference.operation_id = command.operation_id;
			reference.line_index = static_cast<uint16_t>(event.event_index);
			reference.event_index = event.event_index;
			reference.child_index = event.child_index;
			reference.item_uid = event.uid;
			reference.before_revision = event.before.revision;
			reference.after_revision = event.after.revision;
			reference.legacy_operation_id = command.operation_id;
			reference.legacy_event_index = static_cast<uint16_t>(event.event_index);
			if (!economic_accounting_item_reference_validate(reference))
				return false;
			candidate.push_back(reference);
		}
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	*references = std::move(candidate);
	return true;
}

bool build_item_accounting_plan(const critical_command &command,
				const item_transfer_payload &payload,
				const ownership_catalog &before, const ownership_catalog &after,
				flatfile_accounting_record *record,
				std::vector<economic_accounting_item_reference> *references)
try
{
	if (!record || !references || record->result_code)
		return false;
	economic_frozen_intent intent;
	if (economic_intent_decode(command.accounting_intent, &intent) !=
	    economic_accounting_error::ok)
		return false;
	economic_accounting_plan plan;
	if (economic_intent_plan_metadata(command, intent, &plan.metadata) !=
	    economic_accounting_error::ok)
		return false;
	if (payload.reason == item_transfer_reason::craft)
	{
		std::vector<economic_item_snapshot> inputs;
		inputs.reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto *item = catalog_item(before, payload.items[index].item_uid);
			if (!item)
				return false;
			inputs.push_back({ item->item_uid, accounting_position(*item) });
		}
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			uint64_t parent = payload.items[index].parent_item_uid;
			for (size_t depth = 0; parent && depth <= PLAYER_SNAPSHOT_MAX_DEPTH;
			     ++depth)
			{
				const auto *item = catalog_item(before, parent);
				if (!item)
					return false;
				if (std::none_of(inputs.begin(), inputs.end(),
						 [parent](const auto &candidate)
						 { return candidate.uid == parent; }))
					inputs.push_back(
						{ item->item_uid, accounting_position(*item) });
				parent = item->parent_item_uid;
			}
			if (parent)
				return false;
		}
		if (item_transfer_craft_accounting_effects(payload, inputs, &plan) !=
			    economic_accounting_error::ok ||
		    item_transfer_refine_wallet_accounting_effects(payload, &plan) !=
			    economic_accounting_error::ok)
			return false;
		for (const auto &effect : plan.items_after)
		{
			const auto *item = catalog_item(after, effect.uid);
			if (!item || !economic_item_position_equal(effect.position,
								   accounting_position(*item)))
				return false;
		}
		if (economic_plan_normalize(&plan) != economic_accounting_error::ok ||
		    !build_item_accounting_references(command, plan, references))
			return false;
		return economic_plan_encode(plan, &record->plan) == economic_accounting_error::ok;
	}
	const bool creation = payload.reason == item_transfer_reason::creation;
	if (creation)
	{
		plan.items_before.reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
			plan.items_before.push_back({ payload.items[index].item_uid,
						      { { item_owner_type::unknown, 0, 0 },
							0,
							0,
							0,
							item_custody_state::absent } });
	}
	else
	{
		std::vector<uint64_t> roots;
		roots.reserve(payload.item_count + (payload.target_parent_item_uid ? 1 : 0));
		for (size_t index = 0; index < payload.item_count; ++index)
			roots.push_back(payload.items[index].root_item_uid);
		if (payload.target_parent_item_uid)
			roots.push_back(payload.target_root_item_uid);
		std::sort(roots.begin(), roots.end());
		roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
		for (const auto &item : before.items)
			if (std::binary_search(roots.begin(), roots.end(), item.root_item_uid))
				plan.items_before.push_back(
					{ item.item_uid, accounting_position(item) });
		std::sort(plan.items_before.begin(), plan.items_before.end(),
			  [](const auto &left, const auto &right) { return left.uid < right.uid; });
	}
	plan.items_after = plan.items_before;
	plan.item_events.reserve(payload.item_count);
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		auto prior = std::lower_bound(plan.items_before.begin(), plan.items_before.end(),
					      entry.item_uid, [](const auto &item, uint64_t uid)
					      { return item.uid < uid; });
		auto current = std::lower_bound(plan.items_after.begin(), plan.items_after.end(),
						entry.item_uid, [](const auto &item, uint64_t uid)
						{ return item.uid < uid; });
		if (prior == plan.items_before.end() || prior->uid != entry.item_uid ||
		    current == plan.items_after.end() || current->uid != entry.item_uid)
			return false;
		const economic_item_position before_position = prior->position;
		uint64_t target_root = 0, target_parent = 0;
		if (!item_transfer_target_topology(payload, entry.item_uid, &target_root,
						   &target_parent))
			return false;
		if (creation)
		{
			if (before_position.state != item_custody_state::absent ||
			    before_position.revision || before_position.root_uid ||
			    before_position.parent_uid ||
			    before_position.owner.type != item_owner_type::unknown)
				return false;
		}
		else if (!item_owner_identity_equal(before_position.owner, payload.from_owner) ||
			 before_position.root_uid != entry.root_item_uid ||
			 before_position.parent_uid != entry.parent_item_uid ||
			 before_position.revision != entry.expected_item_revision ||
			 before_position.state != entry.expected_state)
			return false;
		const auto *retained = catalog_item(after, entry.item_uid);
		if (!retained)
			return false;
		const economic_item_position after_position = accounting_position(*retained);
		if (!item_owner_identity_equal(after_position.owner, payload.to_owner) ||
		    after_position.root_uid != target_root ||
		    after_position.parent_uid != target_parent ||
		    after_position.revision != before_position.revision + 1 ||
		    after_position.state != (payload.to_owner.type == item_owner_type::destruction ?
						     item_custody_state::destroyed :
						     item_custody_state::active))
			return false;
		current->position = after_position;
		plan.item_events.push_back({ static_cast<uint32_t>(index), 0, entry.item_uid,
					     before_position, after_position });
	}
	if (economic_plan_normalize(&plan) != economic_accounting_error::ok ||
	    economic_plan_validate_structure(plan) != economic_accounting_error::ok ||
	    economic_item_effects_validate(plan.items_before, plan.items_after, plan.item_events,
					   0) != economic_accounting_error::ok ||
	    !build_item_accounting_references(command, plan, references))
		return false;
	return economic_plan_encode(plan, &record->plan) == economic_accounting_error::ok;
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return false;
}

bool plan_metadata_matches(const economic_plan_metadata &actual,
			   const economic_plan_metadata &expected)
{
	const bool source_matches =
		actual.source_event.has_value() == expected.source_event.has_value() &&
		(!actual.source_event ||
		 (actual.source_event->kind == expected.source_event->kind &&
		  actual.source_event->source.bytes == expected.source_event->source.bytes &&
		  actual.source_event->generation.bytes ==
			  expected.source_event->generation.bytes &&
		  actual.source_event->sequence == expected.source_event->sequence &&
		  actual.source_event->slot == expected.source_event->slot));
	return actual.version == expected.version &&
	       actual.lineage.bytes == expected.lineage.bytes &&
	       actual.epoch.bytes == expected.epoch.bytes &&
	       actual.operation_id.bytes == expected.operation_id.bytes &&
	       actual.original_operation_id.bytes == expected.original_operation_id.bytes &&
	       actual.actor_kind == expected.actor_kind && actual.actor_id == expected.actor_id &&
	       actual.writer_id == expected.writer_id &&
	       actual.policy_version == expected.policy_version &&
	       actual.compiler_version == expected.compiler_version &&
	       actual.reason == expected.reason && source_matches &&
	       actual.intent_digest == expected.intent_digest &&
	       actual.domain_digest == expected.domain_digest;
}

unsigned int refine_wallet_mapping(const std::string &root, const flatfile_authority_lock &lock,
				   const critical_command &command,
				   const item_transfer_payload &payload, bool current,
				   std::string *error)
{
	craft_recipe_continuation terms;
	if (!craft_refine_from_payload(payload, &terms) || terms.refine_ore_count == 1)
		return 0;
	economic_frozen_intent intent;
	if (economic_intent_decode(command.accounting_intent, &intent) !=
	    economic_accounting_error::ok)
		return EILSEQ;
	const economic_account_key wallet{ intent.admission.metadata.lineage,
					   economic_account_kind::wallet,
					   terms.refine_cost.wallet_mapping_id, 0 };
	if (current)
	{
		const flatfile_economic_mapping_request request{ wallet,
								 { 1, terms.player_pid, {} } };
		flatfile_economic_authority_snapshot observed;
		return economic_flatfile_lock_authority(root, lock, wallet.lineage,
							intent.admission.metadata.epoch,
							std::span(&request, 1), &observed, error);
	}
	flatfile_economic_mapping observed;
	const auto read = flatfile_economic_mapping_read(root, lock, wallet, &observed, error);
	if (read)
		return read;
	return economic_account_key_equal(observed.account, wallet) && observed.locator.kind == 1 &&
			       observed.locator.native_id == terms.player_pid &&
			       observed.locator.name.empty() ?
		       0 :
		       EILSEQ;
}

bool verify_accounted_item_record(const critical_command &command,
				  const item_transfer_payload &payload,
				  const flatfile_accounting_record &record,
				  unsigned int result_code, const item_transfer_result &result,
				  std::vector<economic_accounting_item_reference> *references)
{
	if (!references || record.result_code != result_code ||
	    record.failure_stage != critical_failure_stage::none)
		return false;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded_result = {};
	if (!item_transfer_command_encode_result(result, &encoded_result) ||
	    record.result.size() != encoded_result.size() ||
	    !std::equal(record.result.begin(), record.result.end(), encoded_result.begin()) ||
	    record.durable_revision !=
		    std::max({ result.from_owner_revision, result.to_owner_revision,
			       result.max_item_revision, result.corpse_revision }))
		return false;
	references->clear();
	if (result_code)
		return record.plan.empty();
	economic_frozen_intent intent;
	if (economic_intent_decode(command.accounting_intent, &intent) !=
	    economic_accounting_error::ok)
		return false;
	economic_accounting_plan plan;
	if (economic_plan_decode(record.plan, &plan) != economic_accounting_error::ok ||
	    economic_plan_validate_structure(plan) != economic_accounting_error::ok ||
	    !plan.children.empty() ||
	    (payload.reason != item_transfer_reason::craft &&
	     (plan.item_events.size() != payload.item_count ||
	      plan.item_events.size() != result.item_count)) ||
	    economic_item_effects_validate(plan.items_before, plan.items_after, plan.item_events,
					   0) != economic_accounting_error::ok)
		return false;
	economic_plan_metadata expected_metadata;
	if (economic_intent_plan_metadata(command, intent, &expected_metadata) !=
		    economic_accounting_error::ok ||
	    !plan_metadata_matches(plan.metadata, expected_metadata))
		return false;
	if (payload.reason == item_transfer_reason::craft)
	{
		const auto &inputs = plan.items_before;
		economic_accounting_plan expected;
		expected.metadata = expected_metadata;
		std::vector<uint8_t> encoded;
		if (item_transfer_craft_accounting_effects(payload, inputs, &expected) !=
			    economic_accounting_error::ok ||
		    item_transfer_refine_wallet_accounting_effects(payload, &expected) !=
			    economic_accounting_error::ok ||
		    economic_plan_normalize(&expected) != economic_accounting_error::ok ||
		    economic_plan_encode(expected, &encoded) != economic_accounting_error::ok ||
		    encoded != record.plan || result.item_count != payload.item_count)
			return false;
		return build_item_accounting_references(command, expected, references);
	}
	if (!plan.accounts.empty() || !plan.postings.empty())
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		const auto &event = plan.item_events[index];
		uint64_t root_uid = 0, parent_uid = 0;
		if (event.event_index != index || event.child_index != 0 ||
		    event.uid != entry.item_uid ||
		    !item_transfer_target_topology(payload, entry.item_uid, &root_uid,
						   &parent_uid) ||
		    !item_owner_identity_equal(event.after.owner, payload.to_owner) ||
		    event.after.root_uid != root_uid || event.after.parent_uid != parent_uid ||
		    event.after.state != (payload.to_owner.type == item_owner_type::destruction ?
						  item_custody_state::destroyed :
						  item_custody_state::active) ||
		    event.after.revision != event.before.revision + 1)
			return false;
		if (payload.reason == item_transfer_reason::creation)
		{
			if (event.before.owner.type != item_owner_type::unknown ||
			    event.before.state != item_custody_state::absent ||
			    event.before.revision || event.before.root_uid ||
			    event.before.parent_uid)
				return false;
		}
		else if (!item_owner_identity_equal(event.before.owner, payload.from_owner) ||
			 event.before.root_uid != entry.root_item_uid ||
			 event.before.parent_uid != entry.parent_item_uid ||
			 event.before.revision != entry.expected_item_revision ||
			 event.before.state != entry.expected_state)
			return false;
	}
	return build_item_accounting_references(command, plan, references);
}

unsigned int apply_craft(ownership_catalog *catalog, const item_transfer_payload &payload,
			 item_transfer_result *result)
{
	if (!catalog || !result || payload.reason != item_transfer_reason::craft ||
	    !item_owner_identity_equal(payload.from_owner, payload.to_owner))
		return EINVAL;
	if (!ensure_owner(catalog, payload.from_owner))
		return ENOSPC;
	owner_state *owner = find_owner(catalog, payload.from_owner);
	if (!owner)
		return EILSEQ;
	*result = { payload.selected_item_uid,
		    payload.item_count,
		    owner->revision,
		    owner->revision,
		    0,
		    0 };
	if (owner->revision != payload.expected_from_revision ||
	    owner->revision != payload.expected_to_revision)
		return ESTALE;
	if (owner->revision == UINT64_MAX || catalog->revision == UINT64_MAX)
		return ERANGE;
	craft_pouch_mutation pouch;
	if (!craft_pouch_mutation_from_payload(payload, &pouch))
		return EBADMSG;
	std::vector<player_item_snapshot> outputs;
	if (payload.item_blob_size &&
	    player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &outputs) != player_snapshot_codec_result::ok)
		return EBADMSG;
	if (!outputs.empty() && outputs[0].object_uid != payload.selected_item_uid)
		return EBADMSG;
	std::vector<uint64_t> source_roots;
	std::vector<flatfile_item_ownership_record *> source;
	try
	{
		for (size_t index = 0; index < payload.item_count; ++index)
			if (payload.items[index].item_uid != pouch.before.object_uid)
				source_roots.push_back(payload.items[index].root_item_uid);
		std::sort(source_roots.begin(), source_roots.end());
		source_roots.erase(std::unique(source_roots.begin(), source_roots.end()),
				   source_roots.end());
		for (auto &entry : catalog->items)
			if (entry.state == item_custody_state::active &&
			    item_owner_identity_equal(entry.owner, payload.from_owner) &&
			    (entry.item_uid == pouch.before.object_uid ||
			     std::binary_search(source_roots.begin(), source_roots.end(),
						entry.root_item_uid)))
				source.push_back(&entry);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	if (source.size() != payload.item_count)
		return EMSGSIZE;
	std::sort(source.begin(), source.end(), [](const auto *left, const auto *right)
		  { return left->item_uid < right->item_uid; });
	for (size_t index = 0; index < source.size(); ++index)
	{
		const auto &stored = *source[index];
		const auto &expected = payload.items[index];
		if (stored.item_uid != expected.item_uid ||
		    stored.root_item_uid != expected.root_item_uid ||
		    stored.parent_item_uid != expected.parent_item_uid ||
		    stored.item_revision != expected.expected_item_revision ||
		    stored.vnum != expected.vnum || stored.state != item_custody_state::active)
			return ESTALE;
		if (stored.item_revision == UINT64_MAX)
			return ERANGE;
	}
	std::unordered_set<uint64_t> output_uids;
	try
	{
		output_uids.reserve(outputs.size());
		for (size_t index = 0; index < outputs.size(); ++index)
		{
			const auto &output = outputs[index];
			if (!output.object_uid || output.vnum <= 0 ||
			    !output_uids.insert(output.object_uid).second ||
			    find_item(catalog, output.object_uid))
				return EEXIST;
			if (output.parent_index >= static_cast<int32_t>(index) ||
			    output.parent_index < PLAYER_SNAPSHOT_NO_PARENT)
				return EBADMSG;
		}
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	if (outputs.size() > ownership_maximum_entries ||
	    catalog->items.size() > ownership_maximum_entries - outputs.size())
		return ENOSPC;
	owner_state *destruction = ensure_owner(catalog, { item_owner_type::destruction, 0, 0 });
	if (!destruction)
		return ENOSPC;
	owner = find_owner(catalog, payload.from_owner);
	if (!owner)
		return EILSEQ;
	for (auto *entry : source)
	{
		++entry->item_revision;
		if (entry->item_uid != pouch.before.object_uid)
		{
			entry->owner = { item_owner_type::destruction, 0, 0 };
			entry->state = item_custody_state::destroyed;
			entry->equipment_slot = 0;
		}
		result->max_item_revision =
			std::max(result->max_item_revision, entry->item_revision);
	}
	++owner->revision;
	for (size_t index = 0; index < outputs.size(); ++index)
	{
		const auto &output = outputs[index];
		uint64_t root = output.object_uid;
		int32_t parent = output.parent_index;
		for (size_t depth = 0;
		     parent != PLAYER_SNAPSHOT_NO_PARENT && depth < outputs.size(); ++depth)
		{
			if (parent < 0 || static_cast<size_t>(parent) >= outputs.size())
				return EBADMSG;
			root = outputs[static_cast<size_t>(parent)].object_uid;
			parent = outputs[static_cast<size_t>(parent)].parent_index;
		}
		if (parent != PLAYER_SNAPSHOT_NO_PARENT)
			return EBADMSG;
		const uint64_t parent_uid =
			output.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
				0 :
				outputs[static_cast<size_t>(output.parent_index)].object_uid;
		catalog->items.push_back({ output.object_uid, root, parent_uid, payload.to_owner, 1,
					   output.vnum, item_custody_state::active });
		result->max_item_revision = std::max(result->max_item_revision, uint64_t(1));
	}
	std::sort(catalog->items.begin(), catalog->items.end(), item_less);
	result->from_owner_revision = owner->revision;
	result->to_owner_revision = owner->revision;
	return 0;
}
} // namespace

class flatfile_accounting_item_transfer_transaction
{
    public:
	static flatfile_accounting_status
	stage(const std::string &root, const flatfile_authority_lock &lock,
	      const flatfile_accounting_record &record,
	      std::vector<flatfile_authority_operation> *operations, std::string *error)
	{
		return flatfile_accounting_storage::stage(root, lock, record, operations, error);
	}
	static flatfile_accounting_status
	stage_source_claim(const std::string &root, const flatfile_authority_lock &lock,
			   const flatfile_accounting_record &record,
			   std::vector<flatfile_authority_operation> *operations,
			   std::string *error)
	{
		return flatfile_accounting_storage::stage_source_claim(root, lock, record,
								       operations, error);
	}
	static flatfile_accounting_status
	verify_source_claim(const std::string &root, const flatfile_authority_lock &lock,
			    const flatfile_accounting_record &record, std::string *error)
	{
		return flatfile_accounting_storage::verify_source_claim(root, lock, record, error);
	}
	static flatfile_authority_transaction_result
	commit(const std::string &root, const flatfile_authority_lock &lock,
	       const std::vector<flatfile_authority_operation> &operations, std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};

namespace
{
critical_apply_result item_accounting_failure(flatfile_accounting_status status, uint64_t revision)
{
	if (status == flatfile_accounting_status::io_error ||
	    status == flatfile_accounting_status::capacity)
		return { critical_apply_outcome::retryable_failure, revision,
			 static_cast<unsigned int>(
				 status == flatfile_accounting_status::io_error ? EIO : ENOSPC) };
	return { critical_apply_outcome::terminal_failure, revision,
		 static_cast<unsigned int>(
			 status == flatfile_accounting_status::already_exists ||
					 status == flatfile_accounting_status::conflict ?
				 EEXIST :
				 EILSEQ) };
}

critical_apply_result item_reference_failure(flatfile_item_accounting_status status,
					     uint64_t revision)
{
	if (status == flatfile_item_accounting_status::io_error ||
	    status == flatfile_item_accounting_status::capacity)
		return { critical_apply_outcome::retryable_failure, revision,
			 static_cast<unsigned int>(
				 status == flatfile_item_accounting_status::io_error ? EIO :
										       ENOSPC) };
	return { critical_apply_outcome::terminal_failure, revision,
		 static_cast<unsigned int>(
			 status == flatfile_item_accounting_status::already_exists ? EEXIST :
										     EILSEQ) };
}
}

flatfile_item_repository_result flatfile_item_repository_load_owner(
	const std::string &root, const item_owner_identity &owner, uint64_t *owner_revision,
	std::vector<flatfile_item_ownership_record> *items, std::string *error)
{
	if (!item_owner_identity_valid(owner) || !owner_revision || !items)
		return flatfile_item_repository_result::invalid;
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_item_repository_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	return flatfile_item_repository_load_owner_locked(root, authority, owner, owner_revision,
							  items, error);
}

flatfile_item_repository_result flatfile_item_repository_recovery_catalog_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_item_ownership_record> *items, std::string *error)
{
	if (!lock.matches(root) || !items)
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_item_repository_result::ok)
		*items = std::move(catalog.items);
	return loaded;
}

flatfile_item_repository_result flatfile_item_repository_read_trade_after_image(
	const flatfile_authority_after_image &image,
	const std::array<item_owner_identity, 2> &owners, std::array<uint64_t, 2> *owner_revisions,
	std::vector<flatfile_item_ownership_record> *records, std::string * /*error*/)
{
	if (!owner_revisions || !records || image.filename != ownership_filename ||
	    image.bytes.empty() || image.bytes.size() > ownership_maximum_bytes ||
	    !item_owner_identity_valid(owners[0]) || !item_owner_identity_valid(owners[1]) ||
	    item_owner_identity_equal(owners[0], owners[1]))
		return flatfile_item_repository_result::invalid;
	try
	{
		ownership_catalog catalog;
		const auto decoded = decode_catalog(image.bytes, &catalog);
		if (decoded != flatfile_item_repository_result::ok)
			return decoded;
		std::vector<uint8_t> canonical;
		if (!encode_catalog(catalog, catalog.revision, &canonical) ||
		    canonical != image.bytes)
			return flatfile_item_repository_result::invalid;
		std::array<uint64_t, 2> revisions{};
		for (size_t index = 0; index < owners.size(); ++index)
		{
			const auto *owner = find_owner(&catalog, owners[index]);
			if (!owner)
				return flatfile_item_repository_result::not_found;
			revisions[index] = owner->revision;
		}
		*records = std::move(catalog.items);
		*owner_revisions = revisions;
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

flatfile_item_repository_result flatfile_item_repository_load_owner_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_owner_identity &owner, uint64_t *owner_revision,
	std::vector<flatfile_item_ownership_record> *items, std::string *error)
{
	if (!lock.matches(root) || !item_owner_identity_valid(owner) || !owner_revision || !items)
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const flatfile_item_repository_result loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const owner_state *stored_owner = find_owner(&catalog, owner);
	if (!stored_owner)
		return flatfile_item_repository_result::not_found;
	std::vector<flatfile_item_ownership_record> selected;
	try
	{
		for (const auto &entry : catalog.items)
			if (entry.state == item_custody_state::active &&
			    item_owner_identity_equal(entry.owner, owner))
				selected.push_back(entry);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	*owner_revision = stored_owner->revision;
	*items = std::move(selected);
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result
flatfile_item_repository_lookup_uid(const std::string &root, uint64_t uid,
				    flatfile_item_ownership_record *item, std::string *error)
{
	if (!uid || !item)
		return flatfile_item_repository_result::invalid;
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_item_repository_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	return flatfile_item_repository_lookup_uid_locked(root, authority, uid, item, error);
}

flatfile_item_repository_result
flatfile_item_repository_lookup_uid_locked(const std::string &root,
					   const flatfile_authority_lock &lock, uint64_t uid,
					   flatfile_item_ownership_record *item, std::string *error)
{
	if (!lock.matches(root) || !uid || !item)
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const auto *stored = find_item(&catalog, uid);
	if (!stored)
		return flatfile_item_repository_result::not_found;
	try
	{
		flatfile_item_ownership_record candidate = *stored;
		*item = std::move(candidate);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	return flatfile_item_repository_result::ok;
}

// The caller selects ITEM_MONEY identities from the original snapshot. Include
// their tombstones even though consumed records no longer have a coin payload.
flatfile_item_repository_result flatfile_item_repository_load_coins_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<uint64_t> &uids, std::vector<flatfile_item_ownership_record> *coins,
	std::string *error)
{
	if (!lock.matches(root) || !coins || uids.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	try
	{
		coins->clear();
		for (uint64_t uid : uids)
			if (const auto *item = find_item(&catalog, uid); item)
				coins->push_back(*item);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_list_collector_items_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<item_ownership_runtime_entry> *items, std::string *error)
{
	if (!lock.matches(root) || !items)
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	std::vector<item_ownership_runtime_entry> selected;
	try
	{
		for (const auto &entry : catalog.items)
		{
			if (entry.owner.type != item_owner_type::collector)
				continue;
			const owner_state *owner = find_owner(&catalog, entry.owner);
			if (!owner)
				return flatfile_item_repository_result::invalid;
			selected.push_back({ entry.item_uid, entry.root_item_uid,
					     entry.parent_item_uid, entry.owner,
					     entry.item_revision, owner->revision, entry.vnum,
					     entry.state });
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	*items = std::move(selected);
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_prepare_collector_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const collector_command_payload &payload, flatfile_item_collector_mutation *mutation,
	unsigned int *result_code, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !result_code ||
	    !payload.item_count || payload.item_count > payload.items.size())
		return flatfile_item_repository_result::invalid;
	*mutation = {};
	*result_code = 0;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_item_repository_result::not_found)
	{
		*result_code = ENOENT;
		return flatfile_item_repository_result::ok;
	}
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	if (!ensure_owner(&catalog, payload.from_owner) ||
	    !ensure_owner(&catalog, payload.to_owner))
	{
		*result_code = ENOSPC;
		return flatfile_item_repository_result::ok;
	}
	owner_state *from = find_owner(&catalog, payload.from_owner);
	owner_state *to = find_owner(&catalog, payload.to_owner);
	if (!from || !to)
		return flatfile_item_repository_result::invalid;
	mutation->from_owner_revision = from->revision;
	mutation->to_owner_revision = to->revision;
	if (from->revision != payload.expected_from_owner_revision ||
	    to->revision != payload.expected_to_owner_revision)
	{
		*result_code = ESTALE;
		return flatfile_item_repository_result::ok;
	}
	if (from->revision == UINT64_MAX || to->revision == UINT64_MAX ||
	    catalog.revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return flatfile_item_repository_result::ok;
	}
	std::vector<flatfile_item_ownership_record *> source;
	try
	{
		for (auto &item : catalog.items)
			if (item.root_item_uid == payload.items[0].root_item_uid)
				source.push_back(&item);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	if (source.size() != payload.item_count)
	{
		*result_code = EMSGSIZE;
		return flatfile_item_repository_result::ok;
	}
	std::sort(source.begin(), source.end(), [](const auto *left, const auto *right)
		  { return left->item_uid < right->item_uid; });
	for (size_t index = 0; index < source.size(); ++index)
	{
		const auto &stored = *source[index];
		const auto &expected = payload.items[index];
		if (stored.item_uid != expected.item_uid ||
		    stored.root_item_uid != expected.root_item_uid ||
		    stored.parent_item_uid != expected.parent_item_uid ||
		    !item_owner_identity_equal(stored.owner, payload.from_owner) ||
		    stored.item_revision != expected.expected_item_revision ||
		    stored.vnum != expected.vnum || stored.state != expected.expected_state)
		{
			*result_code = ESTALE;
			return flatfile_item_repository_result::ok;
		}
		if (stored.item_revision == UINT64_MAX)
		{
			*result_code = ERANGE;
			return flatfile_item_repository_result::ok;
		}
	}
	auto selected = std::find_if(source.begin(), source.end(), [&](const auto *item)
				     { return item->item_uid == payload.selected_item_uid; });
	if (selected == source.end())
	{
		*result_code = ESTALE;
		return flatfile_item_repository_result::ok;
	}
	const uint64_t selected_parent = (*selected)->parent_item_uid;
	const bool collect = payload.action == collector_action::collect;
	if (!collect && source.size() != 1)
	{
		*result_code = EMSGSIZE;
		return flatfile_item_repository_result::ok;
	}
	for (auto *item : source)
	{
		uint64_t new_root = item->root_item_uid;
		uint64_t new_parent = item->parent_item_uid;
		if (collect && item != *selected && item->root_item_uid == (*selected)->item_uid)
		{
			const flatfile_item_ownership_record *cursor = item;
			for (size_t depth = 0; depth <= source.size(); ++depth)
			{
				if (cursor->parent_item_uid == (*selected)->item_uid)
				{
					new_root = cursor->item_uid;
					break;
				}
				auto parent = std::find_if(
					source.begin(), source.end(), [&](const auto *candidate)
					{ return candidate->item_uid == cursor->parent_item_uid; });
				if (parent == source.end())
				{
					new_root = 0;
					break;
				}
				cursor = *parent;
			}
			if (!new_root)
			{
				*result_code = EBADMSG;
				return flatfile_item_repository_result::ok;
			}
		}
		if (collect && item->parent_item_uid == (*selected)->item_uid)
			new_parent = selected_parent;
		++item->item_revision;
		if (item == *selected)
		{
			item->root_item_uid = item->item_uid;
			item->parent_item_uid = 0;
			item->owner = payload.to_owner;
			item->state = payload.target_state;
			mutation->item_revision = item->item_revision;
		}
		else
		{
			item->root_item_uid = new_root;
			item->parent_item_uid = new_parent;
		}
	}
	++from->revision;
	++to->revision;
	mutation->from_owner_revision = from->revision;
	mutation->to_owner_revision = to->revision;
	mutation->after_image.filename = ownership_filename;
	if (!encode_catalog(catalog, catalog.revision + 1, &mutation->after_image.bytes))
		return flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_list_active_player_items(
	const std::string &root, std::vector<flatfile_item_ownership_record> *items,
	std::string *error)
{
	if (root.empty() || !items)
		return flatfile_item_repository_result::invalid;
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_item_repository_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	std::vector<flatfile_item_ownership_record> selected;
	try
	{
		for (const auto &entry : catalog.items)
			if (entry.state == item_custody_state::active &&
			    entry.owner.type == item_owner_type::player)
				selected.push_back(entry);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	*items = std::move(selected);
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_pending_quest_rewards(
	const std::string &root, uint32_t player_pid,
	std::vector<flatfile_quest_reward_obligation> *obligations, std::string *error,
	std::vector<flatfile_quest_xp_entitlement> *entitlements,
	player_revision_t durable_revision)
{
	if (root.empty() || !player_pid || !obligations)
		return flatfile_item_repository_result::invalid;
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_item_repository_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	return flatfile_item_repository_pending_quest_rewards_locked(
		root, authority, player_pid, obligations, error, entitlements, durable_revision);
}

flatfile_item_repository_result flatfile_item_repository_pending_quest_rewards_locked(
	const std::string &root, const flatfile_authority_lock &authority, uint32_t player_pid,
	std::vector<flatfile_quest_reward_obligation> *obligations, std::string *error,
	std::vector<flatfile_quest_xp_entitlement> *entitlements,
	player_revision_t durable_revision)
{
	if (!authority.matches(root) || !player_pid || !obligations)
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	std::vector<flatfile_quest_reward_obligation> selected;
	std::vector<flatfile_quest_xp_entitlement> pending_xp;
	try
	{
		std::unordered_multimap<uint64_t, const operation_state *> creations;
		for (const auto &entry : catalog.operations)
			if (entry.creation_source_id)
				creations.emplace(entry.creation_source_id, &entry);
		for (const operation_state &entry : catalog.operations)
		{
			if (entry.quest_continuation.empty())
				continue;
			quest_reward_continuation terms;
			if (!quest_reward_continuation_decode(entry.quest_continuation.data(),
							      entry.quest_continuation.size(),
							      &terms))
				return flatfile_item_repository_result::invalid;
			const uint64_t player_slots = quest_xp_slots(terms, player_pid);
			for (size_t slot = 0; slot < 64; ++slot)
			{
				const uint64_t bit = UINT64_C(1) << slot;
				if (!(player_slots & bit))
					continue;
				if (entry.quest_xp_applied_mask & bit)
				{
					const size_t index = std::popcount(
						entry.quest_xp_applied_mask & (bit - 1));
					if (entry.quest_xp_revisions[index] > durable_revision)
						return flatfile_item_repository_result::invalid;
				}
				else if (entitlements && terms.version >= 5 &&
					 terms.player_pid != player_pid)
				{
					const auto &award = terms.xp_awards[slot];
					pending_xp.push_back({ entry.operation_id, terms,
							       award.reward_index, award.amount });
				}
			}
			if (terms.player_pid != player_pid || entry.quest_reward_acknowledged)
				continue;
			flatfile_quest_reward_obligation obligation = {
				entry.operation_id, entry.quest_continuation,
				quest_owner_xp_mask(entry, terms)
			};
			for (size_t index = 0; index < terms.reward_count; ++index)
			{
				const auto &reward = terms.rewards[index];
				if (reward.type != 1U && reward.type != 3U)
					continue;
				if (entry.quest_legacy_economic_history)
				{
					obligation.economic_history_verified = false;
					continue;
				}
				if (reward.type == 1U)
				{
					const auto source =
						quest_item_reward_source_id(terms, index);
					if (!source)
						return flatfile_item_repository_result::invalid;
					const auto [first, last] = creations.equal_range(source);
					if (first == last)
						continue;
					if (std::next(first) != last)
						return flatfile_item_repository_result::invalid;
					const auto &receipt = *first->second;
					const auto item = std::lower_bound(
						catalog.items.begin(), catalog.items.end(),
						receipt.result.root_item_uid,
						[](const flatfile_item_ownership_record &record,
						   uint64_t uid) { return record.item_uid < uid; });
					if (receipt.creation_recipient_pid != player_pid ||
					    receipt.creation_vnum !=
						    static_cast<int32_t>(reward.number) ||
					    item == catalog.items.end() ||
					    item->item_uid != receipt.result.root_item_uid ||
					    item->vnum != receipt.creation_vnum)
						return flatfile_item_repository_result::invalid;
				}
				else
				{
					critical_operation_id child = {};
					if (!critical_operation_id_derive(
						    entry.operation_id,
						    QUEST_REWARD_CURRENCY_OPERATION_DOMAIN,
						    static_cast<uint32_t>(index + 1), &child))
						return flatfile_item_repository_result::invalid;
					std::optional<flatfile_legacy_domain_receipt> receipt;
					const auto read =
						flatfile_player_domain_legacy_receipt_locked(
							root, authority,
							static_cast<int32_t>(player_pid), child,
							&receipt, error);
					if (read != flatfile_player_domain_result::ok)
						return read == flatfile_player_domain_result::
									       io_error ?
							       flatfile_item_repository_result::
								       io_error :
							       flatfile_item_repository_result::
								       invalid;
					if (!receipt)
						continue;
					if (!receipt->quest_reward_index)
					{
						obligation.economic_history_verified = false;
						continue;
					}
					currency_command_result paid = {};
					if (receipt->result_code ||
					    receipt->quest_reward_index != index + 1 ||
					    receipt->quest_reward_amount != reward.number ||
					    !currency_command_decode_result(receipt->result.data(),
									    receipt->result_size,
									    &paid) ||
					    !paid.wallet_revision || !paid.bank_revision)
						return flatfile_item_repository_result::invalid;
				}
				obligation.economic_applied_mask |= UINT64_C(1) << index;
			}
			selected.push_back(std::move(obligation));
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	*obligations = std::move(selected);
	if (entitlements)
		*entitlements = std::move(pending_xp);
	return flatfile_item_repository_result::ok;
}

namespace
{
flatfile_item_repository_result
prepare_quest_xp_markers(ownership_catalog &catalog, uint32_t player_pid,
			 player_revision_t durable_revision, player_revision_t applied_revision,
			 const std::vector<player_quest_xp_receipt_snapshot> &receipts,
			 bool verify_only)
{
	if (!player_pid || receipts.empty() || receipts.size() > 64 ||
	    (!verify_only && applied_revision <= durable_revision))
		return flatfile_item_repository_result::invalid;
	bool changed = false;
	try
	{
		for (size_t index = 0; index < receipts.size(); ++index)
		{
			const auto &receipt = receipts[index];
			if (critical_operation_id_is_zero(receipt.offering_operation) ||
			    receipt.reward_index >= 64 || !receipt.amount)
				return flatfile_item_repository_result::invalid;
			for (size_t prior = 0; prior < index; ++prior)
				if (critical_operation_id_equal(receipts[prior].offering_operation,
								receipt.offering_operation) &&
				    receipts[prior].reward_index == receipt.reward_index)
					return flatfile_item_repository_result::invalid;
			auto found = std::find_if(
				catalog.operations.begin(), catalog.operations.end(),
				[&](const operation_state &entry) {
					return critical_operation_id_equal(
						entry.operation_id, receipt.offering_operation);
				});
			if (found == catalog.operations.end() || found->quest_continuation.empty())
				return flatfile_item_repository_result::invalid;
			quest_reward_continuation terms;
			if (!quest_reward_continuation_decode(found->quest_continuation.data(),
							      found->quest_continuation.size(),
							      &terms))
				return flatfile_item_repository_result::invalid;
			uint64_t bit = 0;
			if (terms.version >= 5)
			{
				for (size_t slot = 0; slot < terms.xp_award_count; ++slot)
					if (terms.xp_awards[slot].recipient_pid == player_pid &&
					    terms.xp_awards[slot].reward_index ==
						    receipt.reward_index &&
					    terms.xp_awards[slot].amount == receipt.amount)
						bit = UINT64_C(1) << slot;
			}
			else if (terms.version == 4 && terms.credited_count == 1 &&
				 terms.player_pid == player_pid &&
				 receipt.reward_index < terms.reward_count &&
				 terms.rewards[receipt.reward_index].type == 5U &&
				 terms.rewards[receipt.reward_index].frozen_amount ==
					 receipt.amount)
				bit = UINT64_C(1) << receipt.reward_index;
			if (!bit)
				return flatfile_item_repository_result::invalid;
			const size_t position =
				std::popcount(found->quest_xp_applied_mask & (bit - 1));
			if (found->quest_xp_applied_mask & bit)
			{
				if (found->quest_xp_revisions[position] > durable_revision)
					return flatfile_item_repository_result::invalid;
				continue;
			}
			if (verify_only ||
			    (found->quest_reward_acknowledged && terms.player_pid == player_pid))
				return flatfile_item_repository_result::invalid;
			found->quest_xp_revisions.insert(
				found->quest_xp_revisions.begin() + position, applied_revision);
			found->quest_xp_applied_mask |= bit;
			changed = true;
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	return changed ? flatfile_item_repository_result::ok :
			 flatfile_item_repository_result::unchanged;
}
} // namespace

flatfile_item_repository_result flatfile_item_repository_prepare_quest_xp_receipts(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t player_pid,
	player_revision_t durable_revision, player_revision_t applied_revision,
	const std::vector<player_quest_xp_receipt_snapshot> &receipts, bool verify_only,
	flatfile_authority_operation *operation, std::string *error)
{
	if (!operation || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const auto prepared = prepare_quest_xp_markers(catalog, player_pid, durable_revision,
						       applied_revision, receipts, verify_only);
	if (prepared != flatfile_item_repository_result::ok)
		return prepared;
	if (catalog.revision == UINT64_MAX)
		return flatfile_item_repository_result::invalid;
	flatfile_authority_operation staged;
	staged.filename = ownership_filename;
	if (!encode_catalog(catalog, catalog.revision + 1, &staged.bytes))
		return flatfile_item_repository_result::invalid;
	*operation = std::move(staged);
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result
flatfile_item_repository_ack_quest_reward(const std::string &root, uint32_t player_pid,
					  const critical_operation_id &offering_operation,
					  std::string *error)
{
	if (root.empty() || !player_pid || critical_operation_id_is_zero(offering_operation))
		return flatfile_item_repository_result::invalid;
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_item_repository_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	for (operation_state &entry : catalog.operations)
	{
		if (!critical_operation_id_equal(entry.operation_id, offering_operation))
			continue;
		if (entry.quest_continuation.empty())
			return flatfile_item_repository_result::invalid;
		quest_reward_continuation terms;
		if (!quest_reward_continuation_decode(entry.quest_continuation.data(),
						      entry.quest_continuation.size(), &terms) ||
		    terms.player_pid != player_pid)
			return flatfile_item_repository_result::invalid;
		if (entry.quest_reward_acknowledged)
			return flatfile_item_repository_result::unchanged;
		const uint64_t required_xp = quest_xp_slots(terms, player_pid);
		if ((entry.quest_xp_applied_mask & required_xp) != required_xp)
			return flatfile_item_repository_result::invalid;
		if (catalog.revision == UINT64_MAX)
			return flatfile_item_repository_result::invalid;
		entry.quest_reward_acknowledged = true;
		std::vector<uint8_t> bytes;
		if (!encode_catalog(catalog, catalog.revision + 1, &bytes))
			return flatfile_item_repository_result::invalid;
		const auto committed = flatfile_authority_transaction_commit(
			root, authority, { { ownership_filename, std::move(bytes) } }, error);
		return committed == flatfile_authority_transaction_result::ok ?
			       flatfile_item_repository_result::ok :
		       committed == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	}
	return flatfile_item_repository_result::not_found;
}

flatfile_item_baseline_result
flatfile_item_repository_establish_owner(const std::string &root, const item_owner_identity &owner,
					 const std::vector<flatfile_item_ownership_record> &items,
					 std::string *error)
{
	if (root.empty() || !item_owner_identity_valid(owner) ||
	    owner.type == item_owner_type::native_mobile ||
	    items.size() > ownership_maximum_entries ||
	    !std::is_sorted(items.begin(), items.end(), item_less))
		return flatfile_item_baseline_result::invalid;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &entry = items[index];
		if (!entry.item_uid || !entry.root_item_uid || entry.vnum <= 0 ||
		    entry.item_revision != 1 || entry.state != item_custody_state::active ||
		    !item_owner_identity_equal(entry.owner, owner) ||
		    (index && items[index - 1].item_uid == entry.item_uid))
			return flatfile_item_baseline_result::invalid;
		if (entry.parent_item_uid)
		{
			auto parent = std::lower_bound(
				items.begin(), items.end(), entry.parent_item_uid,
				[](const flatfile_item_ownership_record &candidate, uint64_t uid)
				{ return candidate.item_uid < uid; });
			if (parent == items.end() || parent->item_uid != entry.parent_item_uid ||
			    parent->root_item_uid != entry.root_item_uid)
				return flatfile_item_baseline_result::invalid;
		}
		else if (entry.root_item_uid != entry.item_uid)
			return flatfile_item_baseline_result::invalid;
	}

	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_item_baseline_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, authority, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_baseline_result::io_error :
			       flatfile_item_baseline_result::invalid;
	ownership_catalog catalog;
	const flatfile_item_repository_result loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok &&
	    loaded != flatfile_item_repository_result::not_found)
		return loaded == flatfile_item_repository_result::io_error ?
			       flatfile_item_baseline_result::io_error :
			       flatfile_item_baseline_result::invalid;
	owner_state *stored_owner = find_owner(&catalog, owner);
	std::vector<flatfile_item_ownership_record> existing;
	try
	{
		for (const auto &entry : catalog.items)
			if (item_owner_identity_equal(entry.owner, owner))
				existing.push_back(entry);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_baseline_result::io_error;
	}
	if (stored_owner && stored_owner->revision == 1 && existing.size() == items.size() &&
	    std::equal(existing.begin(), existing.end(), items.begin(), item_equal))
		return flatfile_item_baseline_result::already_applied;
	if ((stored_owner && stored_owner->revision != 0) || !existing.empty())
		return flatfile_item_baseline_result::conflict;
	if (catalog.items.size() > ownership_maximum_entries - items.size() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_item_baseline_result::conflict;
	if (!stored_owner)
	{
		stored_owner = ensure_owner(&catalog, owner);
		if (!stored_owner)
			return flatfile_item_baseline_result::io_error;
	}
	stored_owner->revision = 1;
	try
	{
		catalog.items.insert(catalog.items.end(), items.begin(), items.end());
		std::sort(catalog.items.begin(), catalog.items.end(), item_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_baseline_result::io_error;
	}
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, catalog.revision + 1, &encoded))
		return flatfile_item_baseline_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), ownership_filename, encoded, error))
		return flatfile_item_baseline_result::io_error;
	return flatfile_item_baseline_result::applied;
}

flatfile_item_repository_result flatfile_item_repository_prepare_auction_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const auction_command_payload &payload, uint32_t auction_id, bool to_auction,
	bool require_isolated_roots, flatfile_item_auction_mutation *mutation,
	unsigned int *result_code, std::string *error)
{
	if (!mutation || !result_code || !auction_id || !payload.actor_pid || !payload.item_count ||
	    payload.item_count > payload.items.size() || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	*mutation = {};
	*result_code = 0;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const item_owner_identity player_owner = { item_owner_type::player, payload.actor_pid, 0 };
	const item_owner_identity auction_owner = { item_owner_type::auction, auction_id, 0 };
	const item_owner_identity &from_owner = to_auction ? player_owner : auction_owner;
	const item_owner_identity &to_owner = to_auction ? auction_owner : player_owner;
	if (!ensure_owner(&catalog, player_owner) || !ensure_owner(&catalog, auction_owner))
		return flatfile_item_repository_result::io_error;
	owner_state *player = find_owner(&catalog, player_owner);
	owner_state *auction = find_owner(&catalog, auction_owner);
	if (!player || !auction)
		return flatfile_item_repository_result::invalid;
	if (player->revision == std::numeric_limits<uint64_t>::max() ||
	    auction->revision == std::numeric_limits<uint64_t>::max())
	{
		*result_code = ERANGE;
		return flatfile_item_repository_result::ok;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &expected = payload.items[index];
		flatfile_item_ownership_record *item = find_item(&catalog, expected.item_uid);
		if (!item || item->root_item_uid != item->item_uid || item->parent_item_uid ||
		    !item_owner_identity_equal(item->owner, from_owner) ||
		    item->item_revision != expected.expected_item_revision ||
		    item->item_revision == std::numeric_limits<uint64_t>::max() ||
		    item->vnum != expected.vnum || item->state != item_custody_state::active)
		{
			*result_code = ESTALE;
			return flatfile_item_repository_result::ok;
		}
		for (size_t prior = 0; prior < index; ++prior)
			if (payload.items[prior].item_uid == expected.item_uid)
			{
				*result_code = ESTALE;
				return flatfile_item_repository_result::ok;
			}
		if (require_isolated_roots)
			for (const auto &candidate : catalog.items)
				if (candidate.root_item_uid == expected.item_uid &&
				    candidate.item_uid != expected.item_uid)
				{
					*result_code = EOPNOTSUPP;
					return flatfile_item_repository_result::ok;
				}
	}
	++player->revision;
	++auction->revision;
	mutation->player_owner_revision = player->revision;
	mutation->auction_owner_revision = auction->revision;
	mutation->item_count = payload.item_count;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		flatfile_item_ownership_record *item =
			find_item(&catalog, payload.items[index].item_uid);
		if (!item)
			return flatfile_item_repository_result::invalid;
		item->owner = to_owner;
		++item->item_revision;
		mutation->item_uids[index] = item->item_uid;
		mutation->item_revisions[index] = item->item_revision;
	}
	mutation->after_image.filename = ownership_filename;
	if (catalog.revision == std::numeric_limits<uint64_t>::max() ||
	    !encode_catalog(catalog, catalog.revision + 1, &mutation->after_image.bytes))
		return flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_prepare_shop_trade(
	const std::string &root, const flatfile_authority_lock &lock,
	const shop_trade_payload &payload, flatfile_item_shop_trade_mutation *mutation,
	unsigned int *result_code, std::string *error)
{
	if (!mutation || !result_code || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	*mutation = {};
	*result_code = 0;
	std::vector<uint8_t> validated_payload;
	if (!shop_trade_command_encode_payload(payload, &validated_payload))
		return flatfile_item_repository_result::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const item_owner_identity player = { item_owner_type::player, payload.player_pid, 0 };
	const item_owner_identity shop = { item_owner_type::shopkeeper,
					   item_shopkeeper_owner_id(payload.shop_id), 0 };
	if (!find_owner(&catalog, player) || !find_owner(&catalog, shop))
	{
		*result_code = ESTALE;
		return flatfile_item_repository_result::ok;
	}
	if (payload.action == shop_trade_action::buy_produced)
	{
		const auto *stock = find_item(&catalog, payload.stock_item_uid);
		if (!stock || stock->root_item_uid != stock->item_uid || stock->parent_item_uid ||
		    !item_owner_identity_equal(stock->owner, shop) ||
		    stock->item_revision != payload.expected_stock_item_revision ||
		    stock->vnum != payload.stock_vnum || stock->state != item_custody_state::active)
		{
			*result_code = ESTALE;
			return flatfile_item_repository_result::ok;
		}
	}
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	item_transfer_payload transfer = {};
	transfer.reason_id = payload.shop_id;
	transfer.selected_item_uid = payload.selected_item_uid;
	transfer.target_root_item_uid = payload.target_root_item_uid ?
						payload.target_root_item_uid :
						payload.selected_item_uid;
	transfer.target_parent_item_uid = payload.target_parent_item_uid;
	transfer.expected_target_parent_revision = payload.expected_target_parent_revision;
	transfer.item_count = payload.item_count;
	if (payload.action == shop_trade_action::buy_existing)
	{
		transfer.from_owner = shop;
		transfer.to_owner = player;
		transfer.reason = item_transfer_reason::shop_buy;
	}
	else if (payload.action == shop_trade_action::buy_produced)
	{
		transfer.from_owner = system;
		transfer.to_owner = player;
		transfer.reason = item_transfer_reason::creation;
		if (!ensure_owner(&catalog, system))
			return flatfile_item_repository_result::io_error;
	}
	else if (payload.action == shop_trade_action::sell_store)
	{
		transfer.from_owner = player;
		transfer.to_owner = shop;
		transfer.reason = item_transfer_reason::shop_sell;
	}
	else if (payload.action == shop_trade_action::sell_destroy)
	{
		transfer.from_owner = player;
		transfer.to_owner = destruction;
		transfer.reason = item_transfer_reason::destruction;
		if (!ensure_owner(&catalog, destruction))
			return flatfile_item_repository_result::io_error;
	}
	else if (payload.action == shop_trade_action::discard_invalid)
	{
		transfer.from_owner = shop;
		transfer.to_owner = destruction;
		transfer.reason = item_transfer_reason::destruction;
		if (!ensure_owner(&catalog, destruction))
			return flatfile_item_repository_result::io_error;
	}
	else
		return flatfile_item_repository_result::invalid;
	owner_state *from = find_owner(&catalog, transfer.from_owner);
	owner_state *to = find_owner(&catalog, transfer.to_owner);
	if (!from || !to)
		return flatfile_item_repository_result::invalid;
	transfer.expected_from_revision = from->revision;
	transfer.expected_to_revision = to->revision;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		transfer.items[index] = { item.item_uid,
					  item.root_item_uid,
					  item.parent_item_uid,
					  item.expected_item_revision,
					  item.vnum,
					  item.expected_state };
	}
	item_transfer_result transfer_result = {};
	const unsigned int applied = apply_transfer(&catalog, transfer, &transfer_result);
	if (applied)
	{
		if (applied == ENOMEM)
			return flatfile_item_repository_result::io_error;
		*result_code = applied;
		return flatfile_item_repository_result::ok;
	}
	const item_owner_identity counterparty =
		payload.action == shop_trade_action::buy_produced ? system :
		payload.action == shop_trade_action::sell_destroy ||
				payload.action == shop_trade_action::discard_invalid ?
								    destruction :
								    shop;
	const owner_state *player_owner = find_owner(
		&catalog, payload.action == shop_trade_action::discard_invalid ? shop : player);
	const owner_state *counterparty_owner = find_owner(&catalog, counterparty);
	if (!player_owner || !counterparty_owner)
		return flatfile_item_repository_result::invalid;
	mutation->player_owner_revision = player_owner->revision;
	mutation->counterparty_owner_revision = counterparty_owner->revision;
	mutation->item_count = payload.item_count;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto *item = find_item(&catalog, payload.items[index].item_uid);
		if (!item)
			return flatfile_item_repository_result::invalid;
		mutation->item_uids[index] = item->item_uid;
		mutation->item_revisions[index] = item->item_revision;
	}
	mutation->after_image.filename = ownership_filename;
	if (catalog.revision == std::numeric_limits<uint64_t>::max() ||
	    !encode_catalog(catalog, catalog.revision + 1, &mutation->after_image.bytes))
		return flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload,
	const std::vector<flatfile_corpse_custody_item> &expected_items,
	flatfile_item_corpse_release_mutation *mutation, std::string *error)
{
	const bool release = payload.action == corpse_lifecycle_action::release;
	const bool destroy = payload.action == corpse_lifecycle_action::destroy;
	const bool resurrect = payload.action == corpse_lifecycle_action::resurrect;
	const bool raise_follower = payload.action == corpse_lifecycle_action::raise_follower;
	const bool pet_raise = raise_follower && payload.pet_uid;
	const bool release_nested = payload.action == corpse_lifecycle_action::release_nested;
	const bool nested_player = release_nested && payload.destination_player_pid;
	if (!mutation || !lock.matches(root) ||
	    static_cast<unsigned int>(release) + static_cast<unsigned int>(destroy) +
			    static_cast<unsigned int>(resurrect) +
			    static_cast<unsigned int>(raise_follower) +
			    static_cast<unsigned int>(release_nested) !=
		    1 ||
	    !payload.owner_pid || !payload.save_id || payload.room_vnum <= 0 ||
	    (resurrect && (!payload.destination_player_pid || payload.old_room_vnum <= 0 ||
			   !payload.expected_player_revision)) ||
	    (raise_follower &&
	     (!payload.destination_player_pid || payload.old_room_vnum ||
	      !payload.expected_player_revision || payload.expected_room_revision)) ||
	    (release_nested && (!payload.target_root_item_uid || !payload.target_parent_item_uid ||
				!payload.expected_target_parent_revision ||
				(nested_player ? (!payload.expected_player_revision ||
						  payload.expected_room_revision) :
						 !payload.expected_room_revision))) ||
	    expected_items.size() > ownership_maximum_entries ||
	    !std::is_sorted(expected_items.begin(), expected_items.end(),
			    [](const auto &left, const auto &right)
			    { return left.item_uid < right.item_uid; }))
		return flatfile_item_repository_result::invalid;
	*mutation = {};
	for (size_t index = 0; index < expected_items.size(); ++index)
	{
		const auto &expected = expected_items[index];
		if (!expected.item_uid || expected.vnum <= 0 || !expected.root_item_uid ||
		    (index && expected_items[index - 1].item_uid == expected.item_uid))
			return flatfile_item_repository_result::invalid;
	}
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const item_owner_identity corpse_owner = {
		item_owner_type::corpse, item_corpse_owner_id(payload.owner_pid, payload.save_id), 0
	};
	const item_owner_identity destination_owner =
		release || (release_nested && !nested_player) ?
			item_owner_identity{ item_owner_type::room,
					     static_cast<uint64_t>(payload.room_vnum), 0 } :
		destroy ?
			item_owner_identity{ item_owner_type::destruction, 0, 0 } :
		pet_raise ?
			item_owner_identity{ item_owner_type::pet, payload.pet_uid,
					     payload.destination_player_pid } :
			item_owner_identity{ item_owner_type::player,
					     static_cast<uint64_t>(payload.destination_player_pid),
					     0 };
	const item_owner_identity old_room_owner = { item_owner_type::room,
						     static_cast<uint64_t>(payload.old_room_vnum),
						     0 };
	owner_state *corpse = find_owner(&catalog, corpse_owner);
	owner_state *destination = find_owner(&catalog, destination_owner);
	const item_owner_identity player_owner = { item_owner_type::player,
						   payload.destination_player_pid, 0 };
	owner_state *player_state = pet_raise ? find_owner(&catalog, player_owner) : nullptr;
	owner_state *old_room = resurrect ? find_owner(&catalog, old_room_owner) : nullptr;
	const uint64_t expected_destination_revision =
		pet_raise				       ? 0 :
		(resurrect || raise_follower || nested_player) ? payload.expected_player_revision :
								 payload.expected_room_revision;
	if ((!corpse && !expected_items.empty()) ||
	    (destination ? destination->revision != expected_destination_revision :
			   expected_destination_revision != 0) ||
	    (pet_raise &&
	     (player_state ? player_state->revision != payload.expected_player_revision :
			     payload.expected_player_revision != 0)) ||
	    (resurrect && ((old_room && old_room->revision != payload.expected_room_revision) ||
			   (!old_room && payload.expected_room_revision))) ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_item_repository_result::invalid;
	size_t item_index = 0;
	for (const auto &item : catalog.items)
	{
		if (!item_owner_identity_equal(item.owner, corpse_owner))
			continue;
		if (item_index >= expected_items.size())
			return flatfile_item_repository_result::invalid;
		const auto &expected = expected_items[item_index++];
		if (item.item_uid != expected.item_uid ||
		    item.root_item_uid != expected.root_item_uid ||
		    item.parent_item_uid != expected.parent_item_uid ||
		    item.vnum != expected.vnum || item.state != item_custody_state::active ||
		    item.item_revision == std::numeric_limits<uint64_t>::max())
			return flatfile_item_repository_result::invalid;
	}
	if (item_index != expected_items.size() || !ensure_owner(&catalog, corpse_owner) ||
	    !ensure_owner(&catalog, destination_owner) ||
	    (resurrect && !ensure_owner(&catalog, old_room_owner)))
		return flatfile_item_repository_result::invalid;
	try
	{
		mutation->collector_items.reserve(expected_items.size());
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	if (release_nested)
	{
		const auto *parent = find_item(&catalog, payload.target_parent_item_uid);
		if (!parent || parent->root_item_uid != payload.target_root_item_uid ||
		    !item_owner_identity_equal(parent->owner, destination_owner) ||
		    parent->item_revision != payload.expected_target_parent_revision ||
		    parent->state != item_custody_state::active)
			return flatfile_item_repository_result::invalid;
	}
	corpse = find_owner(&catalog, corpse_owner);
	destination = find_owner(&catalog, destination_owner);
	old_room = resurrect ? find_owner(&catalog, old_room_owner) : nullptr;
	if (!corpse || !destination || corpse->revision == std::numeric_limits<uint64_t>::max() ||
	    destination->revision == std::numeric_limits<uint64_t>::max() ||
	    (resurrect &&
	     (!old_room || old_room->revision == std::numeric_limits<uint64_t>::max())))
		return flatfile_item_repository_result::invalid;
	++corpse->revision;
	++destination->revision;
	if (resurrect)
		++old_room->revision;
	mutation->corpse_owner_revision = corpse->revision;
	mutation->room_owner_revision = resurrect ? old_room->revision :
					release || destroy || (release_nested && !nested_player) ?
						    destination->revision :
						    0;
	mutation->player_owner_revision = resurrect || (raise_follower && !pet_raise) ||
							  nested_player ?
						  destination->revision :
						  0;
	mutation->pet_owner_revision = pet_raise ? destination->revision : 0;
	mutation->item_count = expected_items.size();
	for (auto &item : catalog.items)
	{
		if (!item_owner_identity_equal(item.owner, corpse_owner))
			continue;
		item.owner = destination_owner;
		if (release_nested)
		{
			item.root_item_uid = payload.target_root_item_uid;
			if (!item.parent_item_uid)
				item.parent_item_uid = payload.target_parent_item_uid;
		}
		if (destroy)
			item.state = item_custody_state::destroyed;
		++item.item_revision;
		mutation->collector_items.push_back({ item.item_uid, item.item_revision });
		mutation->max_item_revision =
			std::max(mutation->max_item_revision, item.item_revision);
	}
	mutation->after_image.filename = ownership_filename;
	if (!encode_catalog(catalog, catalog.revision + 1, &mutation->after_image.bytes))
		return flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_prepare_world_corpse_raise(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload,
	const std::vector<flatfile_corpse_custody_item> &expected_items,
	const std::vector<uint64_t> &durable_uids, const std::vector<uint64_t> &discarded_uids,
	flatfile_item_corpse_release_mutation *mutation, std::string *error)
{
	const uint64_t source_uid = (static_cast<uint64_t>(payload.owner_pid) << 32) |
				    static_cast<uint64_t>(payload.save_id);
	const bool hostile = payload.pet_uid == 0;
	auto sorted_unique_uids = [](const std::vector<uint64_t> &values)
	{
		return std::is_sorted(values.begin(), values.end()) &&
		       std::adjacent_find(values.begin(), values.end()) == values.end();
	};
	const bool expected_sorted =
		std::is_sorted(expected_items.begin(), expected_items.end(),
			       [](const auto &left, const auto &right)
			       { return left.item_uid < right.item_uid; }) &&
		std::adjacent_find(expected_items.begin(), expected_items.end(),
				   [](const auto &left, const auto &right) {
					   return left.item_uid == right.item_uid;
				   }) == expected_items.end();
	if (!mutation || !lock.matches(root) ||
	    payload.action != corpse_lifecycle_action::raise_world_follower || !source_uid ||
	    payload.room_vnum <= 0 || !payload.destination_player_pid ||
	    !payload.expected_corpse_revision || !payload.expected_room_revision ||
	    !payload.expected_player_revision || (!hostile && payload.pet_uid != source_uid) ||
	    expected_items.empty() || expected_items.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    expected_items.size() != durable_uids.size() + discarded_uids.size() ||
	    !expected_sorted || !sorted_unique_uids(durable_uids) ||
	    !sorted_unique_uids(discarded_uids) ||
	    !std::binary_search(discarded_uids.begin(), discarded_uids.end(), source_uid))
		return flatfile_item_repository_result::invalid;
	*mutation = {};
	for (size_t index = 0; index < expected_items.size(); ++index)
	{
		const auto &item = expected_items[index];
		const bool durable =
			std::binary_search(durable_uids.begin(), durable_uids.end(), item.item_uid);
		const bool discarded = std::binary_search(discarded_uids.begin(),
							  discarded_uids.end(), item.item_uid);
		if (!item.item_uid || item.vnum <= 0 || item.root_item_uid != source_uid ||
		    (item.item_uid == source_uid ? item.parent_item_uid != 0 :
						   item.parent_item_uid == 0) ||
		    durable == discarded ||
		    (index && expected_items[index - 1].item_uid == item.item_uid))
			return flatfile_item_repository_result::invalid;
	}
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	if (catalog.revision == UINT64_MAX)
		return flatfile_item_repository_result::invalid;
	const item_owner_identity room = { item_owner_type::room,
					   static_cast<uint64_t>(payload.room_vnum), 0 };
	const item_owner_identity player = { item_owner_type::player,
					     payload.destination_player_pid, 0 };
	const item_owner_identity pet = { item_owner_type::pet, payload.pet_uid,
					  payload.destination_player_pid };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	owner_state *room_owner = find_owner(&catalog, room);
	owner_state *player_owner = find_owner(&catalog, player);
	if (!room_owner || room_owner->revision != payload.expected_room_revision ||
	    !player_owner || player_owner->revision != payload.expected_player_revision ||
	    !ensure_owner(&catalog, destruction) || (!hostile && !ensure_owner(&catalog, pet)))
		return flatfile_item_repository_result::invalid;
	room_owner = find_owner(&catalog, room);
	player_owner = find_owner(&catalog, player);
	owner_state *pet_owner = hostile ? nullptr : find_owner(&catalog, pet);
	if (!room_owner || !player_owner || (!hostile && (!pet_owner || pet_owner->revision)))
		return flatfile_item_repository_result::invalid;

	size_t matched = 0;
	for (const auto &item : catalog.items)
	{
		if (item.state != item_custody_state::active ||
		    !item_owner_identity_equal(item.owner, room) ||
		    item.root_item_uid != source_uid)
			continue;
		if (matched >= expected_items.size())
			return flatfile_item_repository_result::invalid;
		const auto &expected = expected_items[matched++];
		if (item.item_uid != expected.item_uid ||
		    item.root_item_uid != expected.root_item_uid ||
		    item.parent_item_uid != expected.parent_item_uid ||
		    item.vnum != expected.vnum || !item.item_revision ||
		    item.item_revision == UINT64_MAX)
			return flatfile_item_repository_result::invalid;
	}
	const auto *source = find_item(&catalog, source_uid);
	if (matched != expected_items.size() || !source ||
	    source->item_revision != payload.expected_corpse_revision)
		return flatfile_item_repository_result::invalid;

	auto expected_by_uid = [&](uint64_t uid) -> const flatfile_corpse_custody_item *
	{
		auto found = std::lower_bound(expected_items.begin(), expected_items.end(), uid,
					      [](const flatfile_corpse_custody_item &item,
						 uint64_t value) { return item.item_uid < value; });
		return found != expected_items.end() && found->item_uid == uid ? &*found : nullptr;
	};
	std::vector<std::pair<size_t, uint64_t>> boundaries;
	try
	{
		for (const auto &item : expected_items)
		{
			if (!item.parent_item_uid)
				continue;
			const auto *parent = expected_by_uid(item.parent_item_uid);
			if (!parent)
				return flatfile_item_repository_result::invalid;
			const bool item_discarded = std::binary_search(
				discarded_uids.begin(), discarded_uids.end(), item.item_uid);
			const bool parent_discarded = std::binary_search(
				discarded_uids.begin(), discarded_uids.end(), parent->item_uid);
			if (item_discarded == parent_discarded)
				continue;
			size_t depth = 0;
			const auto *ancestor = &item;
			while (ancestor->parent_item_uid)
			{
				if (++depth > expected_items.size() ||
				    !(ancestor = expected_by_uid(ancestor->parent_item_uid)))
					return flatfile_item_repository_result::invalid;
			}
			boundaries.emplace_back(depth, item.item_uid);
		}
		std::sort(boundaries.begin(), boundaries.end(),
			  [](const auto &left, const auto &right)
			  { return left.first > right.first; });
		mutation->collector_items.reserve(durable_uids.size());
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}

	auto descends_from =
		[&](const flatfile_item_ownership_record &candidate, uint64_t ancestor_uid)
	{
		uint64_t current_uid = candidate.item_uid;
		for (size_t depth = 0; depth <= expected_items.size(); ++depth)
		{
			if (current_uid == ancestor_uid)
				return true;
			const auto *current = find_item(&catalog, current_uid);
			if (!current || !current->parent_item_uid)
				return false;
			current_uid = current->parent_item_uid;
		}
		return false;
	};
	auto fill_transfer = [&](item_transfer_payload *transfer,
				 const std::vector<uint64_t> *selected_uids, uint64_t subtree_uid)
	{
		if (!transfer)
			return false;
		size_t count = 0;
		for (const auto &item : catalog.items)
		{
			if (item.state != item_custody_state::active ||
			    !item_owner_identity_equal(item.owner, room))
				continue;
			const bool selected =
				subtree_uid ?
					descends_from(item, subtree_uid) :
					selected_uids && std::binary_search(selected_uids->begin(),
									    selected_uids->end(),
									    item.item_uid);
			if (!selected)
				continue;
			if (count >= transfer->items.size())
				return false;
			transfer->items[count++] = {
				item.item_uid,	    item.root_item_uid, item.parent_item_uid,
				item.item_revision, item.vnum,		item_custody_state::active
			};
		}
		transfer->item_count = static_cast<uint16_t>(count);
		return count != 0;
	};

	for (const auto &[depth, boundary_uid] : boundaries)
	{
		(void)depth;
		item_transfer_payload detach = {};
		detach.from_owner = room;
		detach.to_owner = room;
		detach.reason = item_transfer_reason::player_drop;
		detach.reason_id = static_cast<int64_t>(source_uid);
		detach.expected_from_revision = room_owner->revision;
		detach.expected_to_revision = room_owner->revision;
		detach.selected_item_uid = boundary_uid;
		if (!fill_transfer(&detach, nullptr, boundary_uid))
			return flatfile_item_repository_result::invalid;
		item_transfer_result detached = {};
		const unsigned int applied = apply_transfer(&catalog, detach, &detached);
		if (applied)
			return applied == ENOMEM ? flatfile_item_repository_result::io_error :
						   flatfile_item_repository_result::invalid;
		room_owner = find_owner(&catalog, room);
		if (!room_owner)
			return flatfile_item_repository_result::invalid;
	}

	item_transfer_result durable_result = {};
	if (!hostile && !durable_uids.empty())
	{
		item_transfer_payload durable = {};
		durable.from_owner = room;
		durable.to_owner = pet;
		durable.reason = item_transfer_reason::corpse_raise_pet;
		durable.reason_id = static_cast<int64_t>(source_uid);
		durable.expected_from_revision = room_owner->revision;
		durable.expected_to_revision = pet_owner->revision;
		durable.multi_root = true;
		if (!fill_transfer(&durable, &durable_uids, 0))
			return flatfile_item_repository_result::invalid;
		const unsigned int applied = apply_transfer(&catalog, durable, &durable_result);
		if (applied)
			return applied == ENOMEM ? flatfile_item_repository_result::io_error :
						   flatfile_item_repository_result::invalid;
		room_owner = find_owner(&catalog, room);
		pet_owner = find_owner(&catalog, pet);
		for (uint64_t uid : durable_uids)
		{
			const auto *item = find_item(&catalog, uid);
			if (!item || !item_owner_identity_equal(item->owner, pet))
				return flatfile_item_repository_result::invalid;
			mutation->collector_items.push_back({ uid, item->item_revision });
		}
	}
	else if (!hostile)
	{
		if (room_owner->revision == UINT64_MAX || pet_owner->revision == UINT64_MAX)
			return flatfile_item_repository_result::invalid;
		++room_owner->revision;
		++pet_owner->revision;
		durable_result.from_owner_revision = room_owner->revision;
		durable_result.to_owner_revision = pet_owner->revision;
	}
	else
	{
		durable_result.from_owner_revision = room_owner->revision;
		durable_result.item_count = static_cast<uint16_t>(durable_uids.size());
		for (uint64_t uid : durable_uids)
		{
			const auto *item = find_item(&catalog, uid);
			if (!item || !item_owner_identity_equal(item->owner, room))
				return flatfile_item_repository_result::invalid;
			durable_result.max_item_revision =
				std::max(durable_result.max_item_revision, item->item_revision);
		}
	}

	item_transfer_payload discarded = {};
	discarded.from_owner = room;
	discarded.to_owner = destruction;
	discarded.reason = item_transfer_reason::destruction;
	discarded.reason_id = static_cast<int64_t>(source_uid);
	discarded.expected_from_revision = room_owner->revision;
	discarded.expected_to_revision = find_owner(&catalog, destruction)->revision;
	discarded.multi_root = true;
	if (!fill_transfer(&discarded, &discarded_uids, 0))
		return flatfile_item_repository_result::invalid;
	item_transfer_result discarded_result = {};
	const unsigned int discarded_applied =
		apply_transfer(&catalog, discarded, &discarded_result);
	if (discarded_applied)
		return discarded_applied == ENOMEM ? flatfile_item_repository_result::io_error :
						     flatfile_item_repository_result::invalid;
	room_owner = find_owner(&catalog, room);
	pet_owner = hostile ? nullptr : find_owner(&catalog, pet);
	if (!room_owner || (!hostile && !pet_owner))
		return flatfile_item_repository_result::invalid;
	mutation->corpse_owner_revision = room_owner->revision;
	mutation->pet_owner_revision = hostile ? 0 : pet_owner->revision;
	mutation->max_item_revision = durable_result.max_item_revision;
	mutation->item_count = durable_result.item_count;
	mutation->destruction_owner_revision = discarded_result.to_owner_revision;
	mutation->max_discarded_item_revision = discarded_result.max_item_revision;
	mutation->discarded_item_count = discarded_result.item_count;
	mutation->after_image.filename = ownership_filename;
	if (!encode_catalog(catalog, catalog.revision + 1, &mutation->after_image.bytes))
		return flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_item_repository_prepare_death_quarantine(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<uint64_t> &custody_uids, flatfile_authority_operation *operation,
	std::string *error, const std::vector<player_quest_xp_receipt_snapshot> &receipts,
	player_revision_t durable_revision, player_revision_t applied_revision)
{
	if (!operation || !pid || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	*operation = {};
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	bool xp_changed = false;
	if (!receipts.empty())
	{
		const auto prepared = prepare_quest_xp_markers(catalog, pid, durable_revision,
							       applied_revision, receipts, false);
		if (prepared != flatfile_item_repository_result::ok &&
		    prepared != flatfile_item_repository_result::unchanged)
			return prepared;
		xp_changed = prepared == flatfile_item_repository_result::ok;
	}
	const item_owner_identity player = { item_owner_type::player, pid, 0 };
	owner_state *owner = find_owner(&catalog, player);
	if (!owner)
		return flatfile_item_repository_result::not_found;
	std::unordered_set<uint64_t> retained;
	std::unordered_set<uint64_t> retained_roots;
	try
	{
		retained.reserve(custody_uids.size());
		retained_roots.reserve(custody_uids.size());
		for (uint64_t uid : custody_uids)
			if (uid)
				retained.insert(uid);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	for (const auto &item : catalog.items)
		if (item.state == item_custody_state::active &&
		    item_owner_identity_equal(item.owner, player) &&
		    retained.contains(item.item_uid))
			retained_roots.insert(item.root_item_uid);
	bool changed = false;
	for (auto &item : catalog.items)
	{
		if (item.state != item_custody_state::active ||
		    !item_owner_identity_equal(item.owner, player) ||
		    (!retained.contains(item.item_uid) &&
		     !retained_roots.contains(item.root_item_uid)))
			continue;
		if (item.item_revision == UINT64_MAX)
			return flatfile_item_repository_result::invalid;
		// Keep identity, parentage and payload for the captured death graph and
		// any additional authoritative rows attached to one of its roots. Live
		// player-owned objects under unrelated roots remain active and usable.
		item.state = item_custody_state::quarantined;
		++item.item_revision;
		changed = true;
	}
	if (!changed && !xp_changed)
		return flatfile_item_repository_result::unchanged;
	if ((changed && owner->revision == UINT64_MAX) || catalog.revision == UINT64_MAX)
		return flatfile_item_repository_result::invalid;
	if (changed)
		++owner->revision;
	operation->filename = ownership_filename;
	return encode_catalog(catalog, catalog.revision + 1, &operation->bytes) ?
		       flatfile_item_repository_result::ok :
		       flatfile_item_repository_result::invalid;
}

flatfile_item_repository_result flatfile_item_repository_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	flatfile_authority_operation *operation, std::string *error)
{
	if (!operation || !pid || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const item_owner_identity player = { item_owner_type::player, pid, 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	owner_state *player_owner = find_owner(&catalog, player);
	if (!player_owner)
		return flatfile_item_repository_result::unchanged;
	owner_state *destruction_owner = ensure_owner(&catalog, destruction);
	if (!destruction_owner || catalog.revision == std::numeric_limits<uint64_t>::max() ||
	    destruction_owner->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_item_repository_result::invalid;
	for (const auto &item : catalog.items)
		if (item_owner_identity_equal(item.owner, player) &&
		    item.item_revision == std::numeric_limits<uint64_t>::max())
			return flatfile_item_repository_result::invalid;
	for (auto &item : catalog.items)
	{
		if (!item_owner_identity_equal(item.owner, player))
			continue;
		item.owner = destruction;
		item.state = item_custody_state::destroyed;
		++item.item_revision;
	}
	++destruction_owner->revision;
	auto owner =
		std::lower_bound(catalog.owners.begin(), catalog.owners.end(), player,
				 [](const owner_state &candidate, const item_owner_identity &value)
				 { return owner_less(candidate.owner, value); });
	if (owner == catalog.owners.end() || !item_owner_identity_equal(owner->owner, player))
		return flatfile_item_repository_result::invalid;
	catalog.owners.erase(owner);
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, catalog.revision + 1, &encoded))
		return flatfile_item_repository_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = ownership_filename;
	operation->bytes = std::move(encoded);
	return flatfile_item_repository_result::ok;
}

/* Prepare one authority image that destroys an exact set of player and custody owners. */
static flatfile_item_repository_result
prepare_custody_remove(const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
		       const std::vector<flatfile_locker_custody_owner> &locker_custody,
		       const std::vector<flatfile_corpse_custody_owner> &corpse_custody,
		       flatfile_authority_operation *operation, std::string *error)
{
	if (!operation || (!pid && locker_custody.empty()) || !lock.matches(root) ||
	    locker_custody.size() > ownership_maximum_entries ||
	    corpse_custody.size() > ownership_maximum_entries - locker_custody.size())
		return flatfile_item_repository_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	std::vector<item_owner_identity> owners;
	try
	{
		owners.reserve(locker_custody.size() + corpse_custody.size() + (pid ? 1 : 0));
		if (pid)
			owners.push_back({ item_owner_type::player, pid, 0 });
		for (const auto &expected : locker_custody)
		{
			if (expected.owner.type != item_owner_type::locker ||
			    !item_owner_identity_valid(expected.owner) ||
			    !std::is_sorted(expected.items.begin(), expected.items.end(),
					    [](const auto &left, const auto &right)
					    { return left.item_uid < right.item_uid; }))
				return flatfile_item_repository_result::invalid;
			if (std::find_if(owners.begin(), owners.end(),
					 [&](const auto &owner) {
						 return item_owner_identity_equal(owner,
										  expected.owner);
					 }) != owners.end())
				return flatfile_item_repository_result::invalid;
			const owner_state *stored_owner = find_owner(&catalog, expected.owner);
			if (!stored_owner)
			{
				if (expected.items.empty())
					continue;
				return flatfile_item_repository_result::invalid;
			}
			size_t item_index = 0;
			for (const auto &item : catalog.items)
			{
				if (item.state != item_custody_state::active ||
				    !item_owner_identity_equal(item.owner, expected.owner))
					continue;
				if (item_index >= expected.items.size() ||
				    expected.items[item_index].item_uid != item.item_uid ||
				    expected.items[item_index].vnum != item.vnum)
					return flatfile_item_repository_result::invalid;
				++item_index;
			}
			if (item_index != expected.items.size())
				return flatfile_item_repository_result::invalid;
			owners.push_back(expected.owner);
		}
		for (const auto &expected : corpse_custody)
		{
			if (expected.owner.type != item_owner_type::corpse ||
			    !item_owner_identity_valid(expected.owner) ||
			    !std::is_sorted(expected.items.begin(), expected.items.end(),
					    [](const auto &left, const auto &right)
					    { return left.item_uid < right.item_uid; }))
				return flatfile_item_repository_result::invalid;
			if (std::find_if(owners.begin(), owners.end(),
					 [&](const auto &owner) {
						 return item_owner_identity_equal(owner,
										  expected.owner);
					 }) != owners.end())
				return flatfile_item_repository_result::invalid;
			const owner_state *stored_owner = find_owner(&catalog, expected.owner);
			if (!stored_owner)
				return flatfile_item_repository_result::invalid;
			size_t item_index = 0;
			for (const auto &item : catalog.items)
			{
				if (item.state != item_custody_state::active ||
				    !item_owner_identity_equal(item.owner, expected.owner))
					continue;
				if (item_index >= expected.items.size() ||
				    expected.items[item_index].item_uid != item.item_uid ||
				    expected.items[item_index].vnum != item.vnum)
					return flatfile_item_repository_result::invalid;
				++item_index;
			}
			if (item_index != expected.items.size())
				return flatfile_item_repository_result::invalid;
			owners.push_back(expected.owner);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	owner_state *destruction_owner = ensure_owner(&catalog, destruction);
	if (!destruction_owner || catalog.revision == std::numeric_limits<uint64_t>::max() ||
	    destruction_owner->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_item_repository_result::invalid;
	++destruction_owner->revision;
	bool changed = false;
	for (const auto &owner : owners)
	{
		owner_state *stored_owner = find_owner(&catalog, owner);
		if (!stored_owner)
		{
			if (owner.type == item_owner_type::player)
				continue;
			return flatfile_item_repository_result::invalid;
		}
		for (auto &item : catalog.items)
		{
			if (!item_owner_identity_equal(item.owner, owner))
				continue;
			if (item.item_revision == std::numeric_limits<uint64_t>::max())
				return flatfile_item_repository_result::invalid;
			item.owner = destruction;
			item.state = item_custody_state::destroyed;
			++item.item_revision;
		}
		auto at = std::lower_bound(catalog.owners.begin(), catalog.owners.end(), owner,
					   [](const owner_state &candidate,
					      const item_owner_identity &value)
					   { return owner_less(candidate.owner, value); });
		if (at == catalog.owners.end() || !item_owner_identity_equal(at->owner, owner))
			return flatfile_item_repository_result::invalid;
		catalog.owners.erase(at);
		changed = true;
	}
	if (!changed)
		return flatfile_item_repository_result::unchanged;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, catalog.revision + 1, &encoded))
		return flatfile_item_repository_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = ownership_filename;
	operation->bytes = std::move(encoded);
	return flatfile_item_repository_result::ok;
}

/* Prepare removal of one player plus verified locker and corpse custody. */
flatfile_item_repository_result flatfile_item_repository_prepare_player_and_custody_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	const std::vector<flatfile_corpse_custody_owner> &corpse_custody,
	flatfile_authority_operation *operation, std::string *error)
{
	if (!pid)
		return flatfile_item_repository_result::invalid;
	return prepare_custody_remove(root, lock, pid, locker_custody, corpse_custody, operation,
				      error);
}

/* Prepare removal of one player plus verified locker custody. */
flatfile_item_repository_result flatfile_item_repository_prepare_player_and_locker_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	flatfile_authority_operation *operation, std::string *error)
{
	return flatfile_item_repository_prepare_player_and_custody_remove(
		root, lock, pid, locker_custody, {}, operation, error);
}

/* Prepare removal of verified locker custody without requiring a player owner. */
flatfile_item_repository_result flatfile_item_repository_prepare_locker_remove(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	flatfile_authority_operation *operation, std::string *error)
{
	return prepare_custody_remove(root, lock, 0, locker_custody, {}, operation, error);
}

flatfile_item_repository_result flatfile_item_repository_craft_root_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation, std::string *error)
{
	if (!lock.matches(root) || critical_operation_id_is_zero(operation))
		return flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	for (const auto &entry : catalog.operations)
		if (entry.operation_id.bytes == operation.bytes)
			return !entry.result_code && !entry.coin_operation &&
					       entry.result.root_item_uid &&
					       entry.result.item_count &&
					       entry.result.from_owner_revision &&
					       entry.result.max_item_revision ?
				       flatfile_item_repository_result::ok :
				       flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::not_found;
}

critical_apply_result
flatfile_item_repository_verify_creation_locked(const std::string &root,
						const flatfile_authority_lock &lock,
						const critical_command &command, std::string *error)
{
	item_transfer_payload payload{};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	const bool accounted = command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	if (!lock.matches(root) ||
	    (accounted ? !item_transfer_accounting_command_supported(command) :
			 !critical_command_valid(command)) ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    payload.reason != item_transfer_reason::creation ||
	    payload.from_owner.type != item_owner_type::system ||
	    payload.to_owner.type != item_owner_type::player || payload.to_owner.context_id ||
	    !payload.item_blob_size ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	ownership_catalog catalog;
	if (load_catalog(root, &catalog, error) != flatfile_item_repository_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EILSEQ };
	for (const auto &entry : catalog.operations)
		if (critical_operation_id_equal(entry.operation_id, command.operation_id))
		{
			if (entry.result_code || entry.coin_operation ||
			    entry.result.item_count != payload.item_count ||
			    entry.result.max_item_revision != 1 ||
			    CRYPTO_memcmp(entry.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, 0, EILSEQ };
			if (accounted)
			{
				flatfile_accounting_record retained;
				std::vector<economic_accounting_item_reference> references;
				if (flatfile_accounting_lookup(root, lock, command, &retained,
							       error) !=
					    flatfile_accounting_status::ok ||
				    !verify_accounted_item_record(command, payload, retained, 0,
								  entry.result, &references) ||
				    flatfile_item_accounting_reference_verify_operation(
					    root, command.operation_id, references, error) !=
					    flatfile_item_accounting_status::ok ||
				    flatfile_accounting_item_transfer_transaction::verify_source_claim(
					    root, lock, retained, error) !=
					    flatfile_accounting_status::ok)
					return { critical_apply_outcome::terminal_failure, 0,
						 EILSEQ };
			}
			return make_result(critical_apply_outcome::already_applied, 0,
					   entry.result);
		}
	return { critical_apply_outcome::terminal_failure, 0, ENOENT };
}

critical_apply_result flatfile_item_repository_apply(const std::string &root,
						     const critical_command &command)
{
	item_transfer_payload payload = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	const bool accounted = command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	if (root.empty() ||
	    (accounted ? !item_transfer_accounting_command_supported(command) :
			 !critical_command_valid(command)) ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0,
			 static_cast<unsigned int>(accounted ? ENOTSUP : EINVAL) };
	craft_recipe_continuation refine;
	const bool refining = craft_refine_from_payload(payload, &refine);
	if (refining && !accounted)
		return { critical_apply_outcome::terminal_failure, 0, ENOTSUP };
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock authority;
	std::string error;
	if (!authority.acquire(root, &error))
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	const auto recovered = flatfile_authority_transaction_recover(root, authority, &error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return { recovered == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 recovered == flatfile_authority_transaction_result::io_error ?
					 EIO :
					 EILSEQ) };
	ownership_catalog catalog;
	const flatfile_item_repository_result loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_item_repository_result::ok &&
	    loaded != flatfile_item_repository_result::not_found)
		return { loaded == flatfile_item_repository_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 loaded == flatfile_item_repository_result::io_error ? EIO :
										       EILSEQ) };
	for (const operation_state &entry : catalog.operations)
		if (critical_operation_id_equal(entry.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(entry.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EEXIST };
			if (!entry.result_code &&
			    payload.continuation.kind ==
				    item_transfer_continuation_kind::craft_recipe)
			{
				craft_recipe_continuation terms;
				player_snapshot obligation;
				if (!craft_recipe_continuation_decode(payload.continuation.data,
								      &terms))
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				if (terms.discipline != craft_recipe_discipline::refine)
				{
					const auto read = flatfile_craft_receipt_read(
						root, terms.player_pid, command.operation_id, true,
						&obligation, &error);
					if (read != flatfile_player_load_result::ok ||
					    obligation.craft_receipts[0].discipline !=
						    static_cast<uint32_t>(terms.discipline) ||
					    obligation.craft_receipts[0].experience !=
						    terms.experience)
						return {
							read == flatfile_player_load_result::io_error ?
								critical_apply_outcome::
									retryable_failure :
								critical_apply_outcome::
									terminal_failure,
							catalog.revision,
							static_cast<unsigned int>(
								read == flatfile_player_load_result::
											io_error ?
									EIO :
									EILSEQ)
						};
				}
			}
			if (accounted)
			{
				flatfile_accounting_record retained;
				const auto status = flatfile_accounting_lookup(
					root, authority, command, &retained, &error);
				if (status != flatfile_accounting_status::ok)
					return item_accounting_failure(status, catalog.revision);
				std::vector<economic_accounting_item_reference> references;
				if (!verify_accounted_item_record(command, payload, retained,
								  entry.result_code, entry.result,
								  &references))
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				if (refine_wallet_mapping(root, authority, command, payload, false,
							  &error))
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				const auto refs =
					flatfile_item_accounting_reference_verify_operation(
						root, command.operation_id, references, &error);
				if (refs != flatfile_item_accounting_status::ok)
					return item_reference_failure(refs, catalog.revision);
				const auto claim = flatfile_accounting_item_transfer_transaction::
					verify_source_claim(root, authority, retained, &error);
				if (claim != flatfile_accounting_status::ok)
					return item_accounting_failure(claim, catalog.revision);
			}
			return make_result(entry.result_code ?
						   critical_apply_outcome::terminal_failure :
						   critical_apply_outcome::already_applied,
					   entry.result_code, entry.result);
		}
	if (accounted)
	{
		flatfile_accounting_record retained;
		const auto status =
			flatfile_accounting_lookup(root, authority, command, &retained, &error);
		if (status == flatfile_accounting_status::ok)
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EEXIST };
		if (status != flatfile_accounting_status::not_found)
			return item_accounting_failure(status, catalog.revision);
	}
	if (catalog.operations.size() >= ownership_maximum_operations ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	flatfile_wallet_mutation refine_wallet;
	unsigned int refine_result_code = 0;
	if (refining && refine.refine_ore_count != 1)
	{
		const auto mapping =
			refine_wallet_mapping(root, authority, command, payload, true, &error);
		if (mapping)
			return { mapping == EIO || mapping == ENOMEM ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision, mapping };
		// Reuse the existing checked wallet-only native after-image primitive;
		// its replacement is authorized by this craft root, not resurrection.
		const auto prepared = flatfile_player_domain_prepare_resurrection_wallet(
			root, authority, refine.player_pid, refine.refine_cost.before_revision,
			refine.refine_cost.before, refine.refine_cost.after, &refine_wallet,
			&error);
		if (prepared == flatfile_player_domain_result::conflict)
			refine_result_code = ESTALE;
		else if (prepared != flatfile_player_domain_result::ok ||
			 refine_wallet.wallet_revision != refine.refine_cost.after_revision)
			return { prepared == flatfile_player_domain_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_player_domain_result::io_error ?
						 EIO :
						 EILSEQ) };
		else
			for (size_t i = 0; i < 4; ++i)
				if (refine_wallet.wallet.amount[i] != refine.refine_cost.after[i])
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
	}
	ownership_catalog candidate;
	try
	{
		candidate = catalog;
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	item_transfer_result result = {};
	if (refine_result_code)
	{
		const auto *owner = find_owner(&catalog, payload.from_owner);
		result = { item_transfer_result_root(payload),
			   payload.item_count,
			   owner ? owner->revision : 0,
			   owner ? owner->revision : 0,
			   0,
			   0 };
	}
	unsigned int result_code = refine_result_code ?
					   refine_result_code :
				   payload.reason == item_transfer_reason::craft ?
					   apply_craft(&candidate, payload, &result) :
					   apply_transfer(&candidate, payload, &result);
	if (result_code == ENOMEM || result_code == EILSEQ)
		return { critical_apply_outcome::retryable_failure, catalog.revision, result_code };
	if (accounted && result_code)
	{
		try
		{
			candidate = catalog;
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision,
				 ENOMEM };
		}
	}
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    !generic_transfer_supported(payload, command.payload_version, accounted))
	{
		try
		{
			candidate = catalog;
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision,
				 ENOMEM };
		}
		const owner_state *from = find_owner(&catalog, payload.from_owner);
		const owner_state *to = find_owner(&catalog, payload.to_owner);
		result = { item_transfer_result_root(payload),
			   payload.item_count,
			   from ? from->revision : 0,
			   to ? to->revision : 0,
			   0,
			   0 };
		result_code = EOPNOTSUPP;
	}
	if (!result_code && command.payload_version == ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    corpse_loot_transfer(payload))
	{
		flatfile_artifact_transfer_mutation ignored;
		const auto artifacts = flatfile_artifact_prepare_corpse_transfer(
			root, authority, payload, command.accepted_at_usec, &ignored, &error);
		if (artifacts == flatfile_artifact_result::conflict)
		{
			try
			{
				candidate = catalog;
			}
			catch (const std::bad_alloc &)
			{
				return { critical_apply_outcome::retryable_failure,
					 catalog.revision, ENOMEM };
			}
			const owner_state *from = find_owner(&catalog, payload.from_owner);
			const owner_state *to = find_owner(&catalog, payload.to_owner);
			result = { item_transfer_result_root(payload),
				   payload.item_count,
				   from ? from->revision : 0,
				   to ? to->revision : 0,
				   0,
				   0 };
			result_code = EOPNOTSUPP;
		}
		else if (artifacts != flatfile_artifact_result::ok &&
			 artifacts != flatfile_artifact_result::unchanged)
			return { artifacts == flatfile_artifact_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 artifacts == flatfile_artifact_result::io_error ?
						 EIO :
						 EILSEQ) };
	}
	flatfile_collector_enrollment_mutation collector_mutation;
	bool include_collector_mutation = false;
	if (!result_code)
	{
		const auto prepared = flatfile_collector_prepare_item_boundary(
			root, authority, payload, result, &collector_mutation, &result_code,
			&error);
		if (prepared != flatfile_collector_repository_result::ok &&
		    prepared != flatfile_collector_repository_result::unchanged)
			return { prepared == flatfile_collector_repository_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_collector_repository_result::io_error ?
						 EIO :
						 EILSEQ) };
		include_collector_mutation = prepared == flatfile_collector_repository_result::ok &&
					     !collector_mutation.after_image.bytes.empty();
		if (result_code)
		{
			try
			{
				candidate = catalog;
			}
			catch (const std::bad_alloc &)
			{
				return { critical_apply_outcome::retryable_failure,
					 catalog.revision, ENOMEM };
			}
			const owner_state *from = find_owner(&catalog, payload.from_owner);
			const owner_state *to = find_owner(&catalog, payload.to_owner);
			result = { item_transfer_result_root(payload),
				   payload.item_count,
				   from ? from->revision : 0,
				   to ? to->revision : 0,
				   0,
				   0 };
			include_collector_mutation = false;
		}
		else if (include_collector_mutation)
			result.collector_catalog_changed = true;
	}
	try
	{
		operation_state operation = { command.operation_id, digest, result_code, result };
		if (!result_code &&
		    payload.continuation.kind == item_transfer_continuation_kind::quest_offering)
			operation.quest_continuation = payload.continuation.data;
		if (!result_code && payload.from_owner.type == item_owner_type::system &&
		    payload.to_owner.type == item_owner_type::player &&
		    payload.to_owner.id <= UINT32_MAX &&
		    payload.reason == item_transfer_reason::creation && payload.reason_id > 0 &&
		    !payload.multi_root)
		{
			for (size_t index = 0; index < payload.item_count; ++index)
				if (payload.items[index].item_uid == result.root_item_uid)
				{
					operation.creation_source_id =
						static_cast<uint64_t>(payload.reason_id);
					operation.creation_recipient_pid =
						static_cast<uint32_t>(payload.to_owner.id);
					operation.creation_vnum = payload.items[index].vnum;
					break;
				}
		}
		candidate.operations.push_back(std::move(operation));
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	flatfile_shop_trade_materialization_mutation materialization;
	flatfile_locker_transfer_mutation locker;
	flatfile_corpse_transfer_mutation corpse;
	flatfile_room_transfer_mutation room;
	flatfile_artifact_transfer_mutation corpse_artifacts;
	flatfile_artifact_transfer_mutation room_artifacts;
	bool include_locker = false;
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    locker_transfer(payload))
	{
		const auto prepared = flatfile_locker_prepare_item_transfer(
			root, authority, payload, &locker, &error);
		if (prepared != flatfile_locker_result::ok)
			return { prepared == flatfile_locker_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_locker_result::io_error ? EIO :
											EILSEQ) };
		if (!locker_custody_matches(catalog,
					    payload.from_owner.type == item_owner_type::locker ?
						    payload.from_owner :
						    payload.to_owner,
					    locker.expected_items))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		include_locker = true;
	}
	bool include_corpse = false;
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    (corpse_loot_transfer(payload) || corpse_create_transfer(payload)))
	{
		const auto prepared = flatfile_world_item_prepare_corpse_transfer(
			root, authority, payload, &corpse, &error);
		if (prepared != flatfile_world_item_result::ok)
			return { prepared == flatfile_world_item_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_world_item_result::io_error ?
						 EIO :
						 EILSEQ) };
		const item_owner_identity &corpse_owner =
			corpse_create_transfer(payload) ? payload.to_owner : payload.from_owner;
		if (!corpse_custody_matches(catalog, corpse_owner, corpse.expected_items,
					    corpse.created))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		result.corpse_revision = corpse.corpse_revision;
		candidate.operations.back().result = result;
		include_corpse = true;
	}
	bool include_room = false;
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    room_transfer(payload))
	{
		const auto prepared = flatfile_world_item_prepare_room_transfer(
			root, authority, payload, &room, &error);
		if (prepared != flatfile_world_item_result::ok)
			return { prepared == flatfile_world_item_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_world_item_result::io_error ?
						 EIO :
						 EILSEQ) };
		const item_owner_identity &room_owner =
			payload.from_owner.type == item_owner_type::room ? payload.from_owner :
									   payload.to_owner;
		const uint64_t result_revision = payload.from_owner.type == item_owner_type::room ?
							 result.from_owner_revision :
							 result.to_owner_revision;
		if (!room_custody_matches(catalog, room_owner, room.expected_items, room.created) ||
		    room.room_revision != result_revision)
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		include_room = true;
	}
	bool include_corpse_artifacts = false;
	if (!result_code && command.payload_version >= ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION &&
	    (corpse_loot_transfer(payload) || corpse_create_transfer(payload)))
	{
		const auto prepared = flatfile_artifact_prepare_corpse_transfer(
			root, authority, payload, command.accepted_at_usec, &corpse_artifacts,
			&error);
		if (prepared != flatfile_artifact_result::ok &&
		    prepared != flatfile_artifact_result::unchanged)
			return { prepared == flatfile_artifact_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_artifact_result::io_error ? EIO :
											  EILSEQ) };
		include_corpse_artifacts = prepared == flatfile_artifact_result::ok;
	}
	bool include_room_artifacts = false;
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    room_transfer(payload))
	{
		const auto prepared = flatfile_artifact_prepare_room_transfer(
			root, authority, payload, command.accepted_at_usec, &room_artifacts,
			&error);
		if (prepared != flatfile_artifact_result::ok &&
		    prepared != flatfile_artifact_result::unchanged)
			return { prepared == flatfile_artifact_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_artifact_result::io_error ? EIO :
											  EILSEQ) };
		include_room_artifacts = prepared == flatfile_artifact_result::ok;
	}
	bool include_materialization = false;
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    (payload.item_blob_size ||
	     payload.continuation.kind == item_transfer_continuation_kind::craft_pouch_usage ||
	     payload.continuation.kind == item_transfer_continuation_kind::craft_recipe))
	{
		const auto prepared = flatfile_item_transfer_materialization_prepare(
			root, authority, command.operation_id, payload, &materialization, &error);
		if (prepared != flatfile_shop_trade_materialization_result::ok &&
		    prepared != flatfile_shop_trade_materialization_result::unchanged)
			return { prepared == flatfile_shop_trade_materialization_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_shop_trade_materialization_result::
								 io_error ?
						 EIO :
						 EILSEQ) };
		include_materialization = prepared ==
					  flatfile_shop_trade_materialization_result::ok;
	}
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, catalog.revision + 1, &encoded))
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	std::vector<flatfile_authority_after_image> images;
	try
	{
		images.push_back({ ownership_filename, std::move(encoded) });
		if (!result_code && refining && refine.refine_ore_count != 1)
			for (auto &image : refine_wallet.after_images)
				images.push_back(std::move(image));
		if (include_locker)
			images.push_back(std::move(locker.after_image));
		if (include_corpse)
			images.push_back(std::move(corpse.after_image));
		if (include_room)
			images.push_back(std::move(room.after_image));
		if (include_corpse_artifacts)
			images.push_back(std::move(corpse_artifacts.after_image));
		if (include_room_artifacts)
			images.push_back(std::move(room_artifacts.after_image));
		if (include_materialization)
			images.push_back(std::move(materialization.after_image));
		if (include_collector_mutation)
			images.push_back(std::move(collector_mutation.after_image));
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	std::vector<flatfile_authority_operation> recipe_operations;
	try
	{
		if (!result_code &&
		    payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
		{
			craft_recipe_continuation terms;
			if (!craft_recipe_continuation_decode(payload.continuation.data, &terms) ||
			    !craft_recipe_continuation_matches(terms, payload))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EILSEQ };
			if (terms.discipline != craft_recipe_discipline::refine)
			{
				player_snapshot obligation = {};
				obligation.schema_version =
					PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
				obligation.pid = terms.player_pid;
				obligation.revision = 1;
				obligation.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
				obligation.craft_receipts.push_back(
					{ command.operation_id,
					  static_cast<uint32_t>(terms.discipline),
					  terms.experience });
				std::vector<uint8_t> bytes;
				if (!flatfile_player_snapshot_encode_file(obligation, &bytes))
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				recipe_operations.push_back(
					{ flatfile_authority_store::players,
					  flatfile_authority_operation_kind::write,
					  flatfile_craft_receipt_filename(
						  terms.player_pid, command.operation_id, true),
					  std::move(bytes) });
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	if (accounted)
	{
		flatfile_accounting_record record;
		record.command = command;
		record.result_code = result_code;
		record.durable_revision =
			std::max({ result.from_owner_revision, result.to_owner_revision,
				   result.max_item_revision, result.corpse_revision });
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded_result = {};
		if (!item_transfer_command_encode_result(result, &encoded_result))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		record.result.assign(encoded_result.begin(), encoded_result.end());
		std::vector<economic_accounting_item_reference> references;
		if (!result_code && !build_item_accounting_plan(command, payload, catalog,
								candidate, &record, &references))
			return { errno == ENOMEM ? critical_apply_outcome::retryable_failure :
						   critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(errno == ENOMEM ? ENOMEM : EILSEQ) };
		std::vector<flatfile_authority_operation> operations = std::move(recipe_operations);
		try
		{
			operations.reserve(images.size() + 4);

			for (auto &image : images)
				operations.push_back({ flatfile_authority_store::domains,
						       flatfile_authority_operation_kind::write,
						       std::move(image.filename),
						       std::move(image.bytes) });
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision,
				 ENOMEM };
		}
		const auto staged = flatfile_accounting_item_transfer_transaction::stage(
			root, authority, record, &operations, &error);
		if (staged != flatfile_accounting_status::ok)
			return item_accounting_failure(staged, catalog.revision);
		const auto source_staged =
			flatfile_accounting_item_transfer_transaction::stage_source_claim(
				root, authority, record, &operations, &error);
		if (source_staged != flatfile_accounting_status::ok)
			return item_accounting_failure(source_staged, catalog.revision);
		const auto references_staged = flatfile_item_accounting_reference_stage(
			root, authority, command.operation_id, references, &operations, &error);
		if (references_staged != flatfile_item_accounting_status::ok)
			return item_reference_failure(references_staged, catalog.revision);
		const auto committed = flatfile_accounting_item_transfer_transaction::commit(
			root, authority, operations, &error);
		if (committed != flatfile_authority_transaction_result::ok)
			return {
				committed == flatfile_authority_transaction_result::io_error ?
					critical_apply_outcome::retryable_failure :
					critical_apply_outcome::terminal_failure,
				catalog.revision,
				static_cast<unsigned int>(
					committed == flatfile_authority_transaction_result::io_error ?
						EIO :
						EILSEQ)
			};
		flatfile_accounting_record retained;
		const auto looked_up =
			flatfile_accounting_lookup(root, authority, command, &retained, &error);
		if (looked_up != flatfile_accounting_status::ok)
			return item_accounting_failure(looked_up, catalog.revision);
		if (!verify_accounted_item_record(command, payload, retained, result_code, result,
						  &references))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		if (refine_wallet_mapping(root, authority, command, payload, false, &error))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		const auto refs_verified = flatfile_item_accounting_reference_verify_operation(
			root, command.operation_id, references, &error);
		if (refs_verified != flatfile_item_accounting_status::ok)
			return item_reference_failure(refs_verified, catalog.revision);
		const auto claim_verified =
			flatfile_accounting_item_transfer_transaction::verify_source_claim(
				root, authority, retained, &error);
		if (claim_verified != flatfile_accounting_status::ok)
			return item_accounting_failure(claim_verified, catalog.revision);
	}
	else
	{
		try
		{
			if (!recipe_operations.empty())
				for (auto &image : images)
					recipe_operations.push_back(
						{ flatfile_authority_store::domains,
						  flatfile_authority_operation_kind::write,
						  std::move(image.filename),
						  std::move(image.bytes) });
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision,
				 ENOMEM };
		}
		const auto committed = recipe_operations.empty() ?
					       flatfile_authority_transaction_commit(
						       root, authority, images, &error) :
					       flatfile_authority_transaction_commit_operations(
						       root, authority, recipe_operations, &error);
		if (committed != flatfile_authority_transaction_result::ok)
			return {
				committed == flatfile_authority_transaction_result::io_error ?
					critical_apply_outcome::retryable_failure :
					critical_apply_outcome::terminal_failure,
				catalog.revision,
				static_cast<unsigned int>(
					committed == flatfile_authority_transaction_result::io_error ?
						EIO :
						EILSEQ)
			};
	}
	if (!accounted && !result_code && payload.item_count > 0)
	{
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			economic_accounting_item_reference ref = {};
			ref.operation_id = command.operation_id;
			ref.line_index = static_cast<uint16_t>(index);
			ref.event_index = ref.line_index;
			ref.child_index = 1;
			ref.item_uid = payload.items[index].item_uid;
			ref.before_revision =
				result.from_owner_revision > 0 ? result.from_owner_revision - 1 : 0;
			ref.after_revision = result.from_owner_revision;
			ref.legacy_operation_id = command.operation_id;
			ref.legacy_event_index = static_cast<uint16_t>(index);
			flatfile_item_accounting_reference_append(root, ref);
		}
	}
	return make_result(result_code ? critical_apply_outcome::terminal_failure :
					 critical_apply_outcome::applied,
			   result_code, result);
}

// Legacy piles have custody but still keep their amount in the owner's snapshot.
// Read that exact payload under authority; missing or ambiguous evidence cannot be spent.
static unsigned int read_legacy_coin(const std::string &root, const flatfile_authority_lock &lock,
				     const item_owner_identity &owner, uint64_t uid,
				     player_item_snapshot *item, std::string *error)
{
	if (owner.type == item_owner_type::player || owner.type == item_owner_type::pet)
	{
		const uint64_t player_id = owner.type == item_owner_type::pet ? owner.context_id :
										owner.id;
		if (!player_id || player_id > INT32_MAX ||
		    (owner.type == item_owner_type::pet && !owner.id))
			return EINVAL;
		player_snapshot snapshot;
		const auto loaded =
			flatfile_player_snapshot_read(root, player_id, &snapshot, error);
		if (loaded != flatfile_player_load_result::ok)
			return loaded == flatfile_player_load_result::io_error	? EIO :
			       loaded == flatfile_player_load_result::not_found ? ENOENT :
										  EBADMSG;
		size_t matches = 0;
		size_t owner_matches = 0;
		auto inspect = [&](const std::vector<player_item_snapshot> &items, bool is_owner)
		{
			for (const auto &candidate : items)
				if (candidate.object_uid == uid)
				{
					++matches;
					if (is_owner)
					{
						*item = candidate;
						++owner_matches;
					}
				}
		};
		inspect(snapshot.items, owner.type == item_owner_type::player);
		for (const auto &pet : snapshot.pets)
			inspect(pet.items,
				owner.type == item_owner_type::pet && pet.pet_uid == owner.id);
		return matches > 1 ? EMSGSIZE : owner_matches == 1 ? 0 : ENOENT;
	}
	if (owner.type == item_owner_type::room || owner.type == item_owner_type::corpse)
	{
		const auto loaded =
			flatfile_world_item_read_coin(root, lock, owner, uid, item, error);
		return loaded == flatfile_world_item_result::ok	       ? 0 :
		       loaded == flatfile_world_item_result::io_error  ? EIO :
		       loaded == flatfile_world_item_result::not_found ? ENOENT :
		       loaded == flatfile_world_item_result::conflict  ? EMSGSIZE :
									 EBADMSG;
	}
	if (owner.type == item_owner_type::locker)
	{
		const auto loaded = flatfile_locker_read_coin(root, lock, owner, uid, item, error);
		return loaded == flatfile_locker_result::ok	   ? 0 :
		       loaded == flatfile_locker_result::io_error  ? EIO :
		       loaded == flatfile_locker_result::not_found ? ENOENT :
		       loaded == flatfile_locker_result::conflict  ? EMSGSIZE :
								     EBADMSG;
	}
	return EOPNOTSUPP;
}

static flatfile_item_repository_result
read_coin_pile_source(const std::string &root, const flatfile_authority_lock &lock,
		      const flatfile_item_ownership_record &stored,
		      flatfile_coin_pile_source *source, std::string *error)
{
	if (stored.state != item_custody_state::active)
		return flatfile_item_repository_result::invalid;
	try
	{
		flatfile_coin_pile_source candidate;
		candidate.ownership = stored;
		if (stored.coin_payload.empty())
		{
			const auto code = read_legacy_coin(root, lock, stored.owner,
							   stored.item_uid, &candidate.item, error);
			if (code)
				return code == EIO    ? flatfile_item_repository_result::io_error :
				       code == ENOENT ? flatfile_item_repository_result::not_found :
							flatfile_item_repository_result::invalid;
		}
		else
		{
			std::vector<player_item_snapshot> items;
			if (player_item_snapshot_list_decode(stored.coin_payload.data(),
							     stored.coin_payload.size(), &items) !=
				    player_snapshot_codec_result::ok ||
			    items.size() != 1)
				return flatfile_item_repository_result::invalid;
			candidate.item = std::move(items[0]);
		}
		if (candidate.item.object_uid != stored.item_uid ||
		    candidate.item.vnum != stored.vnum)
			return flatfile_item_repository_result::invalid;
		if (candidate.item.type != ITEM_MONEY)
			return stored.coin_payload.empty() ?
				       flatfile_item_repository_result::unchanged :
				       flatfile_item_repository_result::invalid;
		if (std::any_of(candidate.item.values.begin(), candidate.item.values.begin() + 4,
				[](int32_t value) { return value < 0; }))
			return flatfile_item_repository_result::invalid;
		*source = std::move(candidate);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

flatfile_item_repository_result flatfile_item_repository_read_coin_pile_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	flatfile_coin_pile_source *source, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !uid || !source)
		return flatfile_item_repository_result::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	const auto *stored = find_item(&catalog, uid);
	if (!stored)
		return flatfile_item_repository_result::not_found;
	const auto result = read_coin_pile_source(root, lock, *stored, source, error);
	return result == flatfile_item_repository_result::unchanged ?
		       flatfile_item_repository_result::not_found :
		       result;
}

unsigned int flatfile_item_repository_verify_coin_root_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, std::span<const uint8_t> retained_result,
	std::string *error)
try
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !lock.matches(root) ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::coin_transfer ||
	    !critical_command_envelope_valid(command) || !command_digest(command, &digest) ||
	    retained_result.size() != COIN_TRANSFER_RESULT_BYTES)
		return EINVAL;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ? EIO : EILSEQ;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded == flatfile_item_repository_result::io_error ? EIO : EILSEQ;
	const auto found = std::find_if(
		catalog.operations.begin(), catalog.operations.end(),
		[&](const operation_state &value)
		{ return critical_operation_id_equal(value.operation_id, command.operation_id); });
	if (found == catalog.operations.end())
		return ENOENT;
	if (!found->coin_operation || found->result_code || found->command_digest != digest ||
	    !std::equal(found->coin_result.begin(), found->coin_result.end(),
			retained_result.begin(), retained_result.end()))
		return EILSEQ;
	return 0;
}
catch (const std::bad_alloc &)
{
	return ENOMEM;
}

flatfile_item_repository_result flatfile_item_repository_list_coin_piles_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_coin_pile_source> *sources, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !sources)
		return flatfile_item_repository_result::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded;
	try
	{
		std::vector<flatfile_coin_pile_source> candidate;
		for (const auto &item : catalog.items)
		{
			if (item.state == item_custody_state::destroyed)
				continue;
			flatfile_coin_pile_source source;
			const auto result = read_coin_pile_source(root, lock, item, &source, error);
			if (result == flatfile_item_repository_result::unchanged)
				continue;
			if (result != flatfile_item_repository_result::ok)
				return result == flatfile_item_repository_result::not_found ?
					       flatfile_item_repository_result::invalid :
					       result;
			candidate.push_back(std::move(source));
		}
		*sources = std::move(candidate);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

unsigned int flatfile_item_repository_capture_room_coin_piles_locked(
    const std::string &root, const flatfile_authority_lock &lock,
    std::vector<flatfile_coin_pile_source> *sources, std::string *error) noexcept
try
{
    if (root.empty() || !lock.matches(root) || !sources)
        return EINVAL;
    const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
    if (recovered != flatfile_authority_transaction_result::ok)
        return recovered == flatfile_authority_transaction_result::io_error ? EIO : EILSEQ;
    std::vector<flatfile_corpse_record> corpses;
    std::vector<flatfile_room_item_record> rooms;
    std::vector<flatfile_saved_world_item_record> saved_items;
    const auto world = flatfile_world_item_recovery_list_all_locked(
        root, lock, &corpses, &rooms, &saved_items, error);
    if (world != flatfile_world_item_result::ok && world != flatfile_world_item_result::not_found)
        return world == flatfile_world_item_result::io_error ? EIO : EILSEQ;
    std::vector<flatfile_locker_record> lockers;
    const auto locker = flatfile_locker_recovery_list_locked(root, lock, &lockers, error);
    if (locker != flatfile_locker_result::ok && locker != flatfile_locker_result::not_found)
        return locker == flatfile_locker_result::io_error ? EIO : EILSEQ;
    struct physical_item
    {
        item_owner_identity owner;
        const player_item_snapshot *item;
    };
    std::unordered_map<uint64_t, physical_item> physical;
    size_t physical_money = 0;
    auto inspect = [&](const std::vector<player_item_snapshot> &items,
                       const item_owner_identity &owner) -> unsigned int
    {
        std::unordered_set<int32_t> parents;
        for (const auto &item : items)
            if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
                parents.insert(item.parent_index);
        for (size_t index = 0; index < items.size(); ++index)
        {
            const auto &item = items[index];
            if (item.object_uid && !physical.emplace(item.object_uid, physical_item{owner, &item}).second)
                return EILSEQ;
            if (item.type != ITEM_MONEY)
            {
                if (item.vnum == VOBJ_COINS)
                    return EILSEQ;
                continue;
            }
            if (owner.type != item_owner_type::room)
                return EOPNOTSUPP;
            if (!owner.id || !item.object_uid || item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
                item.equipment_slot != -1 || parents.count(static_cast<int32_t>(index)))
                return EILSEQ;
            ++physical_money;
        }
        return 0;
    };
    for (const auto &corpse : corpses)
        if (const auto code = inspect(corpse.items, {item_owner_type::corpse,
                item_corpse_owner_id(corpse.owner_pid, corpse.save_id), 0}))
            return code;
    for (const auto &room : rooms)
        if (const auto code = inspect(room.items, {item_owner_type::room,
                static_cast<uint64_t>(room.room_vnum), 0}))
            return code;
    for (const auto &saved : saved_items)
        if (const auto code = inspect(saved.items, {item_owner_type::room,
                static_cast<uint64_t>(saved.room_vnum), 0}))
            return code;
    for (const auto &stored_locker : lockers)
        for (const auto &chest : stored_locker.chests)
            if (const auto code = inspect(chest.items, {item_owner_type::locker,
                    stored_locker.locker_id, chest.chest_id}))
                return code;
    ownership_catalog catalog;
    const auto loaded = load_catalog(root, &catalog, error);
    if (loaded == flatfile_item_repository_result::not_found)
    {
        if (physical_money)
            return EILSEQ;
        *sources = {};
        return 0;
    }
    if (loaded != flatfile_item_repository_result::ok)
        return loaded == flatfile_item_repository_result::io_error ? EIO : EILSEQ;
    std::unordered_set<uint64_t> referenced_roots;
    for (const auto &stored : catalog.items)
    {
        if (stored.root_item_uid != stored.item_uid)
            referenced_roots.insert(stored.root_item_uid);
        if (stored.parent_item_uid)
            referenced_roots.insert(stored.parent_item_uid);
    }
    std::vector<flatfile_coin_pile_source> candidate;
    for (const auto &stored : catalog.items)
    {
        if (stored.state == item_custody_state::destroyed)
            continue;
        flatfile_coin_pile_source source;
        const auto found = physical.find(stored.item_uid);
        const bool world_owner = stored.owner.type == item_owner_type::room ||
            stored.owner.type == item_owner_type::corpse || stored.owner.type == item_owner_type::locker;
        if (stored.coin_payload.empty() && world_owner)
        {
            if (stored.state != item_custody_state::active || found == physical.end() ||
                !item_owner_identity_equal(found->second.owner, stored.owner) ||
                found->second.item->vnum != stored.vnum)
                return EILSEQ;
            source.ownership = stored;
            source.item = *found->second.item;
            if (source.item.type != ITEM_MONEY)
            {
                if (stored.vnum == VOBJ_COINS)
                    return EILSEQ;
                continue;
            }
            if (std::any_of(source.item.values.begin(), source.item.values.begin()+4,
                            [](int32_t value){return value<0;}))
                return EILSEQ;
        }
        else
        {
            const auto status = read_coin_pile_source(root, lock, stored, &source, error);
            if (status == flatfile_item_repository_result::unchanged)
            {
                if (stored.vnum == VOBJ_COINS)
                    return EILSEQ;
                continue;
            }
            if (status != flatfile_item_repository_result::ok)
                return status == flatfile_item_repository_result::io_error ? EIO : EILSEQ;
        }
        if (stored.owner.type != item_owner_type::room)
            return EOPNOTSUPP;
        if (!stored.owner.id || stored.owner.context_id || !stored.item_revision ||
            stored.root_item_uid != stored.item_uid || stored.parent_item_uid || stored.equipment_slot ||
            source.item.parent_index != PLAYER_SNAPSHOT_NO_PARENT || source.item.equipment_slot != -1 ||
            referenced_roots.count(stored.item_uid))
            return EILSEQ;
        if (found == physical.end() || !item_owner_identity_equal(found->second.owner, stored.owner))
            return EILSEQ;
        std::vector<uint8_t> actual, retained;
        if (player_item_snapshot_list_encode({*found->second.item}, &actual) != player_snapshot_codec_result::ok ||
            player_item_snapshot_list_encode({source.item}, &retained) != player_snapshot_codec_result::ok ||
            actual != retained)
            return EILSEQ;
        const auto *owner = find_owner(&catalog, stored.owner);
        if (!owner || !owner->revision)
            return EILSEQ;
        candidate.push_back(std::move(source));
    }
    if (candidate.size() != physical_money)
        return EILSEQ;
    std::sort(candidate.begin(), candidate.end(), [](const auto &left, const auto &right)
              { return left.ownership.item_uid < right.ownership.item_uid; });
    *sources = std::move(candidate);
    return 0;
}
catch (const std::bad_alloc &)
{
    return ENOMEM;
}
catch (...)
{
    return EIO;
}

static unsigned int
prepare_coin_pile_changes(const std::string &root, const flatfile_authority_lock &lock,
			  const coin_transfer_payload &payload, ownership_catalog *catalog,
			  coin_transfer_result *result,
			  std::vector<flatfile_authority_after_image> *images, std::string *error)
{
	bool changes_room = false;
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		if (endpoint.change.type != critical_command_type::item_transfer)
			continue;
		critical_command change = endpoint.change;
		if (index &&
		    !coin_transfer_command_destination_after_source(payload, *result, &change))
			return EINVAL;
		item_transfer_payload transfer;
		if (!item_transfer_command_decode_payload(change, &transfer) ||
		    transfer.item_count != 1)
			return EINVAL;
		changes_room = changes_room || transfer.from_owner.type == item_owner_type::room ||
			       transfer.to_owner.type == item_owner_type::room;
		const uint64_t uid = transfer.items[0].item_uid;
		if (transfer.from_owner.type != item_owner_type::system)
		{
			const auto *stored = find_item(catalog, uid);
			if (!stored)
				return ENOENT;
			player_item_snapshot baseline;
			if (stored->coin_payload.empty())
			{
				const auto code = read_legacy_coin(root, lock, transfer.from_owner,
								   uid, &baseline, error);
				if (code)
					return code;
			}
			else
			{
				std::vector<player_item_snapshot> items;
				if (player_item_snapshot_list_decode(stored->coin_payload.data(),
								     stored->coin_payload.size(),
								     &items) !=
					    player_snapshot_codec_result::ok ||
				    items.size() != 1)
					return EBADMSG;
				baseline = std::move(items[0]);
			}
			if (baseline.object_uid != uid || baseline.vnum != transfer.items[0].vnum ||
			    baseline.type != ITEM_MONEY)
				return EBADMSG;
			for (size_t coin = 0; coin < 4; ++coin)
				if (baseline.values[coin] != endpoint.before[coin])
					return ESTALE;
		}
		const auto code = apply_transfer(catalog, transfer, &result->piles[index]);
		if (code)
			return code;
		auto *stored = find_item(catalog, uid);
		if (!stored)
			return EILSEQ;
		if (transfer.to_owner.type == item_owner_type::destruction)
			stored->coin_payload.clear();
		else
			stored->coin_payload.assign(transfer.item_blob.begin(),
						    transfer.item_blob.begin() +
							    transfer.item_blob_size);
	}
	if (changes_room)
	{
		flatfile_authority_after_image room_image;
		const auto rooms = flatfile_world_item_prepare_coin_rooms(
			root, lock, payload, *result, &room_image, error);
		if (rooms == flatfile_world_item_result::ok)
			images->push_back(std::move(room_image));
		else if (rooms != flatfile_world_item_result::unchanged)
			return rooms == flatfile_world_item_result::io_error ? EIO : EILSEQ;
	}
	return 0;
}

flatfile_item_repository_result flatfile_item_repository_prepare_coin_piles(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, coin_transfer_result *result,
	std::vector<flatfile_authority_after_image> *images, unsigned int *result_code,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !result || !images || !result_code ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::coin_transfer ||
	    !critical_command_envelope_valid(command))
		return flatfile_item_repository_result::invalid;
	coin_transfer_payload payload;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (!coin_transfer_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return flatfile_item_repository_result::invalid;
	if (payload.source.change.type != critical_command_type::item_transfer &&
	    payload.destination.change.type != critical_command_type::item_transfer)
		return flatfile_item_repository_result::unchanged;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_item_repository_result::ok &&
	    loaded != flatfile_item_repository_result::not_found)
		return loaded;
	for (const auto &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
			return flatfile_item_repository_result::invalid;
	if (catalog.operations.size() >= ownership_maximum_operations ||
	    catalog.revision == UINT64_MAX)
		return flatfile_item_repository_result::invalid;
	try
	{
		ownership_catalog candidate = catalog;
		coin_transfer_result prepared = *result;
		std::vector<flatfile_authority_after_image> prepared_images;
		const auto code = prepare_coin_pile_changes(root, lock, payload, &candidate,
							    &prepared, &prepared_images, error);
		if (code == ENOMEM || code == EIO)
			return flatfile_item_repository_result::io_error;
		if (code)
		{
			*result_code = code;
			return flatfile_item_repository_result::ok;
		}
		operation_state operation = { command.operation_id, digest, 0, {} };
		operation.coin_operation = true;
		if (!coin_transfer_command_encode_result(payload, prepared, &operation.coin_result))
			return flatfile_item_repository_result::invalid;
		candidate.operations.push_back(std::move(operation));
		prepared_images.push_back({ ownership_filename, {} });
		if (!encode_catalog(candidate, catalog.revision + 1, &prepared_images.back().bytes))
			return flatfile_item_repository_result::invalid;
		auto combined_images = *images;
		combined_images.insert(combined_images.end(),
				       std::make_move_iterator(prepared_images.begin()),
				       std::make_move_iterator(prepared_images.end()));
		*images = std::move(combined_images);
		*result = std::move(prepared);
		*result_code = 0;
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

// One authority transaction owns both coin legs and their replay receipt.
static critical_apply_result flatfile_coin_apply(const std::string &root,
						 const critical_command &command)
{
	coin_transfer_payload payload;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !coin_transfer_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	std::lock_guard<std::mutex> guard(ownership_mutex);
	flatfile_authority_lock lock;
	std::string error;
	if (!lock.acquire(root, &error))
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	const auto recovered = flatfile_authority_transaction_recover(root, lock, &error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return { recovered == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 recovered == flatfile_authority_transaction_result::io_error ?
					 EIO :
					 EILSEQ) };
	ownership_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_item_repository_result::ok &&
	    loaded != flatfile_item_repository_result::not_found)
		return { loaded == flatfile_item_repository_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 loaded == flatfile_item_repository_result::io_error ? EIO :
										       EILSEQ) };
	auto completion = [](const operation_state &operation, critical_apply_outcome outcome)
	{
		critical_apply_result result = { operation.result_code ?
							 critical_apply_outcome::terminal_failure :
							 outcome,
						 0, operation.result_code };
		if (!operation.result_code)
		{
			result.result_size = operation.coin_result.size();
			std::copy(operation.coin_result.begin(), operation.coin_result.end(),
				  result.result_payload.begin());
		}
		return result;
	};
	for (const auto &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (!operation.coin_operation ||
			    CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, 0, EEXIST };
			return completion(operation, critical_apply_outcome::already_applied);
		}
	if (catalog.operations.size() >= ownership_maximum_operations ||
	    catalog.revision == UINT64_MAX)
		return { critical_apply_outcome::terminal_failure, 0, ENOSPC };
	try
	{
		ownership_catalog candidate = catalog;
		coin_transfer_result result;
		unsigned int result_code = 0;
		std::vector<flatfile_authority_after_image> images;
		const auto wallets = flatfile_player_domain_prepare_coin_wallets(
			root, lock, payload, &result, &images, &result_code, &error);
		if (wallets != flatfile_player_domain_result::ok)
			return { wallets == flatfile_player_domain_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 0,
				 static_cast<unsigned int>(
					 wallets == flatfile_player_domain_result::io_error ?
						 EIO :
					 wallets == flatfile_player_domain_result::not_found ?
						 ENOENT :
						 EILSEQ) };
		if (!result_code)
			result_code = prepare_coin_pile_changes(root, lock, payload, &candidate,
								&result, &images, &error);
		const coin_transfer_endpoint *endpoints[] = { &payload.source,
							      &payload.destination };
		if (result_code == ENOMEM || result_code == EIO)
			return { critical_apply_outcome::retryable_failure, 0, result_code };
		operation_state operation = { command.operation_id, digest, result_code, {} };
		operation.coin_operation = true;
		if (result_code)
		{
			candidate = catalog;
			images.clear();
		}
		else if (!coin_transfer_command_encode_result(payload, result,
							      &operation.coin_result))
			return { critical_apply_outcome::terminal_failure, 0, EBADMSG };
		candidate.operations.push_back(operation);
		images.push_back({ ownership_filename, {} });
		if (!encode_catalog(candidate, catalog.revision + 1, &images.back().bytes))
			return { critical_apply_outcome::terminal_failure, 0, ENOSPC };
		const auto committed =
			flatfile_authority_transaction_commit(root, lock, images, &error);
		if (committed != flatfile_authority_transaction_result::ok)
			return {
				committed == flatfile_authority_transaction_result::io_error ?
					critical_apply_outcome::retryable_failure :
					critical_apply_outcome::terminal_failure,
				0,
				static_cast<unsigned int>(
					committed == flatfile_authority_transaction_result::io_error ?
						EIO :
						EILSEQ)
			};
		if (!result_code)
		{
			for (size_t index = 0; index < 2; ++index)
			{
				const auto &endpoint = *endpoints[index];
				if (endpoint.change.type != critical_command_type::item_transfer)
					continue;
				item_transfer_payload transfer;
				if (item_transfer_command_decode_payload(endpoint.change,
									 &transfer) &&
				    transfer.item_count > 0)
				{
					for (size_t item_idx = 0; item_idx < transfer.item_count;
					     ++item_idx)
					{
						economic_accounting_item_reference ref = {};
						ref.operation_id = command.operation_id;
						ref.line_index = static_cast<uint16_t>(
							(index == 0 ? 0 :
								      result.piles[0].item_count) +
							item_idx);
						ref.event_index = ref.line_index;
						ref.child_index = static_cast<uint16_t>(index + 1);
						ref.item_uid = transfer.items[item_idx].item_uid;
						ref.before_revision =
							result.piles[index].max_item_revision > 0 ?
								result.piles[index]
										.max_item_revision -
									1 :
								0;
						ref.after_revision =
							result.piles[index].max_item_revision;
						ref.legacy_operation_id =
							endpoint.change.operation_id;
						ref.legacy_event_index =
							static_cast<uint16_t>(item_idx);
						flatfile_item_accounting_reference_append(root,
											  ref);
					}
				}
			}
		}
		return completion(operation, critical_apply_outcome::applied);
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	}
}

critical_apply_result
flatfile_critical_command_repository_apply_selected(const critical_command &command, void *context)
{
	const char *root = context ? static_cast<const char *>(context) :
				     persistence_mode_flatfile_root();
	if (!root || !*root)
		return { critical_apply_outcome::terminal_failure, 0, ENOENT };
	if (command.type == critical_command_type::coin_transfer)
		return flatfile_coin_apply(root, command);
	if (command.type == critical_command_type::item_transfer)
		return flatfile_item_repository_apply(root, command);
	if (command.type == critical_command_type::auction)
		return flatfile_auction_repository_apply(root, command);
	if (command.type == critical_command_type::collector)
		return flatfile_collector_repository_apply(root, command);
	if (command.type == critical_command_type::boon_reward)
		return flatfile_boon_repository_apply(root, command);
	if (command.type == critical_command_type::boon_shop)
		return flatfile_boon_shop_repository_apply(root, command);
	if (command.type == critical_command_type::shop_trade)
		return flatfile_shop_trade_repository_apply(root, command);
	if (command.type == critical_command_type::corpse_lifecycle)
		return flatfile_corpse_repository_apply(root, command);
	if (command.type == critical_command_type::epic ||
	    command.type == critical_command_type::account_bank ||
	    command.type == critical_command_type::combat_outcome)
		return flatfile_player_domain_apply(root, command);
	return { critical_apply_outcome::terminal_failure, 0, ENOTSUP };
}

namespace
{
bool flat_native_custody_matches(const ownership_catalog &catalog,
				 const quest_mobile_native_image &image)
{
	const item_owner_identity owner{ item_owner_type::native_mobile,
					 image.reference.mobile_instance_id, 0 };
	size_t count = 0;
	for (const auto &row : catalog.items)
		if (item_owner_identity_equal(row.owner, owner))
			++count;
	if (count != image.items.size())
		return false;
	std::vector<uint64_t> roots(image.items.size());
	for (size_t i = 0; i < image.items.size(); ++i)
	{
		const auto &item = image.items[i];
		const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		roots[i] = root ? item.object_uid : roots[item.parent_index];
		const auto *row = catalog_item(catalog, item.object_uid);
		if (!row || !item_owner_identity_equal(row->owner, owner) ||
		    row->root_item_uid != roots[i] ||
		    row->parent_item_uid !=
			    (root ? 0 : image.items[item.parent_index].object_uid) ||
		    !row->item_revision || row->item_revision == UINT64_MAX ||
		    row->vnum != item.vnum || row->state != item_custody_state::active ||
		    row->equipment_slot != item.equipment_slot)
			return false;
	}
	return true;
}
flatfile_item_repository_result
flat_native_player_preimage(const std::string &root, const flatfile_authority_lock &lock,
			    const ownership_catalog &catalog, const item_transfer_payload &payload,
			    std::span<const player_item_snapshot> original, bool *matched,
			    std::string *error)
{
	*matched = false;
	player_snapshot saved;
	const auto loaded = flatfile_player_snapshot_read(
		root, static_cast<int32_t>(payload.from_owner.id), &saved, error);
	if (loaded != flatfile_player_load_result::ok)
		return loaded == flatfile_player_load_result::io_error ?
			       flatfile_item_repository_result::io_error :
		       loaded == flatfile_player_load_result::not_found ?
			       flatfile_item_repository_result::not_found :
			       flatfile_item_repository_result::invalid;
	std::vector<flatfile_item_ownership_record> owned;
	for (const auto &row : catalog.items)
		if (item_owner_identity_equal(row.owner, payload.from_owner) &&
		    row.state == item_custody_state::active)
			owned.push_back(row);
	const auto reconciled = flatfile_shop_trade_materialization_reconcile(
		root, lock, static_cast<uint32_t>(payload.from_owner.id), owned, &saved, error);
	if (reconciled != flatfile_shop_trade_materialization_result::ok)
		return reconciled == flatfile_shop_trade_materialization_result::io_error ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	std::vector<player_item_snapshot> expected(original.begin(), original.end());
	std::vector<uint8_t> a, b;
	auto code = player_item_snapshot_list_encode(saved.items, &a);
	if (code == player_snapshot_codec_result::ok)
		code = player_item_snapshot_list_encode(expected, &b);
	if (code != player_snapshot_codec_result::ok)
		return code == player_snapshot_codec_result::allocation_failure ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	if (a != b)
		return flatfile_item_repository_result::ok;
	std::unordered_set<uint64_t> selected;
	for (size_t i = 0; i < payload.item_count; ++i)
		selected.insert(payload.items[i].item_uid);
	std::vector<player_item_snapshot> observed;
	std::vector<int32_t> remap(saved.items.size(), PLAYER_SNAPSHOT_NO_PARENT);
	for (size_t i = 0; i < saved.items.size(); ++i)
	{
		auto item = saved.items[i];
		if (!selected.count(item.object_uid))
			continue;
		if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		{
			if (item.parent_index < 0 || static_cast<size_t>(item.parent_index) >= i ||
			    remap[item.parent_index] == PLAYER_SNAPSHOT_NO_PARENT)
				return flatfile_item_repository_result::invalid;
			item.parent_index = remap[item.parent_index];
		}
		remap[i] = static_cast<int32_t>(observed.size());
		observed.push_back(std::move(item));
	}
	code = player_item_snapshot_list_encode(observed, &a);
	if (code != player_snapshot_codec_result::ok)
		return code == player_snapshot_codec_result::allocation_failure ?
			       flatfile_item_repository_result::io_error :
			       flatfile_item_repository_result::invalid;
	if (a.size() != payload.item_blob_size ||
	    !std::equal(a.begin(), a.end(), payload.item_blob.begin()))
		return flatfile_item_repository_result::ok;
	// Reconciliation is read-only. Outbound materialization below retains the
	// selected original bytes; the ordinary save generation is never rewritten.
	*matched = true;
	return flatfile_item_repository_result::ok;
}
bool flat_native_custody_delta(const ownership_catalog &before, const ownership_catalog &after,
			       const item_transfer_payload &payload,
			       item_transfer_custody_delta *output)
{
	item_transfer_custody_delta delta;
	for (size_t i = 0; i < payload.item_count; ++i)
	{
		const auto *old = catalog_item(before, payload.items[i].item_uid);
		const auto *now = catalog_item(after, payload.items[i].item_uid);
		if (!old || !now)
			return false;
		delta.before.push_back({ old->item_uid, accounting_position(*old) });
		delta.after.push_back({ now->item_uid, accounting_position(*now) });
		delta.events.push_back({ static_cast<uint32_t>(i), 0, old->item_uid,
					 accounting_position(*old), accounting_position(*now) });
	}
	if (economic_item_effects_validate(delta.before, delta.after, delta.events, 0) !=
	    economic_accounting_error::ok)
		return false;
	*output = std::move(delta);
	return true;
}
}

flatfile_item_repository_result flatfile_item_repository_prepare_native_mobile(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command,
	std::span<const player_item_snapshot> original_player_items, item_transfer_result *result,
	unsigned int *result_code, item_transfer_custody_delta *delta,
	quest_mobile_native_image *after, std::vector<flatfile_authority_operation> *operations,
	std::string *error)
{
	if (!lock.matches(root) || !result || !result_code || !delta || !after || !operations ||
	    command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION ||
	    critical_operation_id_is_zero(command.operation_id))
		return flatfile_item_repository_result::invalid;
	*result_code = 0;
	try
	{
		item_transfer_payload payload{};
		if (!item_transfer_command_decode_payload(command, &payload) ||
		    !item_transfer_native_mobile_shape_valid(payload))
			return flatfile_item_repository_result::invalid;
		const bool acceptance = payload.native_mobile.action ==
					item_native_mobile_action::acceptance;
		if ((!acceptance && !original_player_items.empty()) ||
		    original_player_items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
			return flatfile_item_repository_result::invalid;
		quest_mobile_native_flatfile_row native;
		int code = quest_mobile_native_flatfile_read_locked(
			root, lock, payload.native_mobile.reference.mobile_instance_id, &native);
		if (code)
			return code == ENOMEM || code == EIO ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		quest_mobile_native_image next;
		if (!native.present)
		{
			*result_code = ESTALE;
			return flatfile_item_repository_result::ok;
		}
		const auto transformed = quest_mobile_native_item_transition(
			native.image, payload, command.operation_id, &next);
		if (transformed == player_snapshot_codec_result::allocation_failure)
		{
			return flatfile_item_repository_result::io_error;
		}
		if (transformed != player_snapshot_codec_result::ok)
		{
			*result_code = transformed == player_snapshot_codec_result::limit_exceeded ?
					       E2BIG :
					       ESTALE;
			return flatfile_item_repository_result::ok;
		}
		ownership_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_item_repository_result::ok)
			return loaded;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
		if (!command_digest(command, &digest))
			return flatfile_item_repository_result::invalid;
		for (const auto &entry : catalog.operations)
			if (critical_operation_id_equal(entry.operation_id, command.operation_id))
			{
				*result_code = CRYPTO_memcmp(entry.command_digest.data(),
							     digest.data(), digest.size()) ?
						       EEXIST :
						       EALREADY;
				return flatfile_item_repository_result::ok;
			} // Root owns original receipt replay/proof.
		const auto *from = find_owner(&catalog, payload.from_owner);
		const auto *to = find_owner(&catalog, payload.to_owner);
		if (!from || !to || from->revision != payload.expected_from_revision ||
		    to->revision != payload.expected_to_revision ||
		    (acceptance ? to->revision : from->revision) !=
			    native.image.reference.stock_revision)
		{
			*result_code = ESTALE;
			return flatfile_item_repository_result::ok;
		}
		if (!flat_native_custody_matches(catalog, native.image))
			return flatfile_item_repository_result::invalid;
		if (acceptance)
		{
			bool matched = false;
			const auto preimage =
				flat_native_player_preimage(root, lock, catalog, payload,
							    original_player_items, &matched, error);
			if (preimage != flatfile_item_repository_result::ok)
				return preimage;
			if (!matched)
			{
				*result_code = ESTALE;
				return flatfile_item_repository_result::ok;
			}
		}
		if (catalog.revision == UINT64_MAX ||
		    catalog.operations.size() >= ownership_maximum_operations)
			return flatfile_item_repository_result::invalid;
		ownership_catalog candidate = catalog;
		item_transfer_result applied{};
		code = apply_transfer(&candidate, payload, &applied);
		if (code)
		{
			*result_code = code;
			return code == ENOMEM ? flatfile_item_repository_result::io_error :
						flatfile_item_repository_result::ok;
		}
		if (!flat_native_custody_matches(candidate, next))
			return flatfile_item_repository_result::invalid;
		item_transfer_custody_delta effects;
		if (!flat_native_custody_delta(catalog, candidate, payload, &effects))
			return flatfile_item_repository_result::invalid;
		operation_state operation{ command.operation_id, digest, 0, applied };
		if (!acceptance &&
		    payload.continuation.kind == item_transfer_continuation_kind::quest_offering)
			operation.quest_continuation = payload.continuation.data;
		candidate.operations.push_back(std::move(operation));
		std::vector<flatfile_authority_operation> prepared;
		flatfile_authority_operation native_operation;
		code = quest_mobile_native_flatfile_prepare_locked(root, lock, command.operation_id,
								   native, next, &native_operation);
		if (code)
			return code == ENOMEM || code == EIO ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		prepared.push_back(std::move(native_operation));
		std::vector<uint8_t> catalog_bytes;
		if (!encode_catalog(candidate, catalog.revision + 1, &catalog_bytes))
			return flatfile_item_repository_result::invalid;
		prepared.push_back({ flatfile_authority_store::domains,
				     flatfile_authority_operation_kind::write, ownership_filename,
				     std::move(catalog_bytes) });
		if (acceptance)
		{
			flatfile_shop_trade_materialization_mutation removal;
			const auto status = flatfile_item_transfer_materialization_prepare(
				root, lock, command.operation_id, payload, &removal, error);
			if (status != flatfile_shop_trade_materialization_result::ok)
				return status == flatfile_shop_trade_materialization_result::io_error ?
					       flatfile_item_repository_result::io_error :
					       flatfile_item_repository_result::invalid;
			prepared.push_back({ flatfile_authority_store::domains,
					     flatfile_authority_operation_kind::write,
					     std::move(removal.after_image.filename),
					     std::move(removal.after_image.bytes) });
		}
		*result = applied;
		*delta = std::move(effects);
		*after = std::move(next);
		*operations = std::move(prepared);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

namespace
{
bool flat_smith_same_native(const quest_mobile_native_image &a, const quest_mobile_native_image &b)
{
	std::vector<uint8_t> left, right;
	return quest_mobile_native_image_encode(a, &left) == player_snapshot_codec_result::ok &&
	       quest_mobile_native_image_encode(b, &right) == player_snapshot_codec_result::ok &&
	       left == right;
}
bool flat_smith_same_items(const std::vector<player_item_snapshot> &a,
			   const std::vector<player_item_snapshot> &b)
{
	std::vector<uint8_t> left, right;
	return player_item_snapshot_list_encode(a, &left) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(b, &right) == player_snapshot_codec_result::ok &&
	       left == right;
}
bool flat_smith_persisted_image(const player_snapshot &image, uint32_t pid,
				uint64_t original_observed_ack_revision,
				std::vector<uint8_t> *encoded)
{
	// The maintained full-file writer normalizes the bound to actual encoded
	// size. This applies to the observed persisted image, never the queued body.
	return pid && pid <= uint32_t(INT32_MAX) && original_observed_ack_revision &&
	       image.pid == int32_t(pid) && image.revision == original_observed_ack_revision &&
	       image.components == PLAYER_CHECKPOINT_COMPONENT_ALL && encoded &&
	       player_snapshot_encode(image, encoded) == player_snapshot_codec_result::ok &&
	       image.encoded_size_bound == encoded->size();
}
// Complete owner forest equality, including native slot/root/parent positions.
// Separate unchanged forests are not expanded into the bounded effect witness.
bool flat_smith_player_custody(const ownership_catalog &catalog, const item_owner_identity &owner,
			       const std::vector<player_item_snapshot> &items)
{
	size_t count = 0;
	for (const auto &row : catalog.items)
		if (item_owner_identity_equal(row.owner, owner))
		{
			if (row.state != item_custody_state::active)
				return false;
			++count;
		}
	if (count != items.size())
		return false;
	std::vector<uint64_t> roots(items.size());
	for (size_t i = 0; i < items.size(); ++i)
	{
		const auto &item = items[i];
		const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		if (!root && (item.parent_index < 0 || size_t(item.parent_index) >= i))
			return false;
		roots[i] = root ? item.object_uid : roots[item.parent_index];
		const auto *row = catalog_item(catalog, item.object_uid);
		if (!row || !item_owner_identity_equal(row->owner, owner) ||
		    row->root_item_uid != roots[i] ||
		    row->parent_item_uid != (root ? 0 : items[item.parent_index].object_uid) ||
		    !row->item_revision || row->item_revision == UINT64_MAX ||
		    row->vnum != item.vnum || row->state != item_custody_state::active ||
		    row->equipment_slot != item.equipment_slot)
			return false;
	}
	return true;
}
} // namespace

flatfile_item_repository_result flatfile_smith_native_observation::capture_persisted_before(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	uint64_t original_observed_ack_revision, player_snapshot *full_persisted_before,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !pid || pid > uint32_t(INT32_MAX) ||
	    !original_observed_ack_revision || !full_persisted_before)
		return flatfile_item_repository_result::invalid;
	try
	{
		player_snapshot actual;
		const auto read = flatfile_player_snapshot_read(root, int32_t(pid), &actual, error);
		if (read != flatfile_player_load_result::ok)
			return read == flatfile_player_load_result::io_error ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		std::vector<uint8_t> bytes;
		if (!flat_smith_persisted_image(actual, pid, original_observed_ack_revision,
						&bytes) ||
		    !lock.matches(root))
			return flatfile_item_repository_result::invalid;
		*full_persisted_before = std::move(actual);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

flatfile_item_repository_result flatfile_item_repository_prepare_smith_native(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, const smith_native_compound_images &original_images,
	const smith_native_player_grant_projection &original_grant,
	const player_snapshot &original_acknowledged_filtered_save,
	const player_snapshot &original_full_persisted_before,
	const economic_account_key &original_player_wallet, item_transfer_result *result,
	unsigned int *result_code, item_transfer_custody_delta *delta,
	economic_accounting_plan *expected_effects,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !result || !result_code || !delta ||
	    !expected_effects || !operations ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return flatfile_item_repository_result::invalid;
	try
	{
		smith_native_compound_terms terms;
		economic_frozen_intent intent;
		economic_accounting_plan effects;
		if (smith_native_command_validate(command, &terms) !=
			    economic_accounting_error::ok ||
		    economic_intent_decode(command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_plan_metadata(command, intent, &effects.metadata) !=
			    economic_accounting_error::ok)
			return flatfile_item_repository_result::invalid;
		smith_native_compound_images projected;
		if (smith_native_compound_prepare_images(
			    terms, original_images.native_before, original_images.player_before,
			    original_grant, &projected) != player_snapshot_codec_result::ok ||
		    !flat_smith_same_native(projected.native_after, original_images.native_after) ||
		    !flat_smith_same_items(projected.player_after, original_images.player_after))
			return flatfile_item_repository_result::invalid;
		const item_owner_identity native_owner{
			item_owner_type::native_mobile,
			terms.native_before.reference.mobile_instance_id, 0
		};
		const item_owner_identity player_owner{ item_owner_type::player, terms.player_pid,
							0 };
		quest_mobile_native_flatfile_row native;
		const int native_read = quest_mobile_native_flatfile_read_locked(
			root, lock, native_owner.id, &native);
		if (native_read || !native.present ||
		    !flat_smith_same_native(native.image, original_images.native_before))
			return native_read == ENOMEM || native_read == EIO ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		ownership_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_item_repository_result::ok)
			return loaded;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
		if (!valid_catalog(catalog) || !command_digest(command, &digest) ||
		    catalog.revision == UINT64_MAX ||
		    catalog.operations.size() >= ownership_maximum_operations)
			return flatfile_item_repository_result::invalid;
		for (const auto &entry : catalog.operations)
			if (critical_operation_id_equal(entry.operation_id, command.operation_id))
				return flatfile_item_repository_result::
					invalid; // Original owner replays.
		const auto *npc_clock = find_owner(&catalog, native_owner);
		const auto *pc_clock = find_owner(&catalog, player_owner);
		if (!npc_clock || !pc_clock ||
		    npc_clock->revision != native.image.reference.stock_revision ||
		    pc_clock->revision != terms.expected_player_item_revision ||
		    !flat_native_custody_matches(catalog, native.image) ||
		    !flat_smith_player_custody(catalog, player_owner,
					       original_images.player_before))
			return flatfile_item_repository_result::invalid;

		// Keep the truthful queued/ACK body separate from the genuine full file
		// captured by the owner under this original lock after its observed ACK.
		// The full writer carries uncaptured components, strips transient receipts
		// and normalizes schema/size. Byte equality to a partial queued body is not
		// a save proof. Root authenticates the original ACK chain and lifetime.
		player_snapshot saved;
		const auto saved_read = flatfile_player_snapshot_read(
			root, int32_t(terms.player_pid), &saved, error);
		if (saved_read != flatfile_player_load_result::ok)
			return saved_read == flatfile_player_load_result::io_error ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		std::vector<uint8_t> queued, recorded, current;
		if (!original_acknowledged_filtered_save.revision ||
		    original_acknowledged_filtered_save.pid != int32_t(terms.player_pid) ||
		    player_snapshot_encode(original_acknowledged_filtered_save, &queued) !=
			    player_snapshot_codec_result::ok ||
		    !flat_smith_persisted_image(original_full_persisted_before, terms.player_pid,
						original_acknowledged_filtered_save.revision,
						&recorded) ||
		    !flat_smith_persisted_image(saved, terms.player_pid,
						original_acknowledged_filtered_save.revision,
						&current) ||
		    recorded != current)
			return flatfile_item_repository_result::invalid;
		size_t level_count = 0;
		for (const auto &field : saved.status_integers)
			if (field.field == player_status_field::level)
			{
				++level_count;
				if ((field.is_unsigned ? field.unsigned_value :
							 uint64_t(field.signed_value)) !=
					    original_grant.original_player_level ||
				    (!field.is_unsigned && field.signed_value < 0))
					return flatfile_item_repository_result::invalid;
			}
		if (level_count != 1)
			return flatfile_item_repository_result::invalid;
		std::vector<flatfile_item_ownership_record> owned;
		for (const auto &row : catalog.items)
			if (item_owner_identity_equal(row.owner, player_owner))
				owned.push_back(row);
		const auto reconciled = flatfile_shop_trade_materialization_reconcile(
			root, lock, terms.player_pid, owned, &saved, error);
		if (reconciled != flatfile_shop_trade_materialization_result::ok ||
		    !flat_smith_same_items(saved.items, original_images.player_before))
			return reconciled == flatfile_shop_trade_materialization_result::io_error ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;

		// One complete catalog cut proves absent outputs and excludes live foreign
		// links into either retained owner forest. No PID/null/VNUM absence inference.
		std::unordered_set<uint64_t> npc_uids, pc_uids, output_uids;
		for (const auto &item : native.image.items)
			npc_uids.insert(item.object_uid);
		for (const auto &item : original_images.player_before)
			pc_uids.insert(item.object_uid);
		for (const auto &item : terms.frozen_outputs)
			output_uids.insert(item.object_uid);
		for (const auto &row : catalog.items)
		{
			if (output_uids.contains(row.item_uid) ||
			    output_uids.contains(row.root_item_uid) ||
			    output_uids.contains(row.parent_item_uid))
				return flatfile_item_repository_result::invalid;
			if (row.state != item_custody_state::active &&
			    row.state != item_custody_state::quarantined)
				continue;
			if (((npc_uids.contains(row.root_item_uid) ||
			      npc_uids.contains(row.parent_item_uid)) &&
			     !item_owner_identity_equal(row.owner, native_owner)) ||
			    ((pc_uids.contains(row.root_item_uid) ||
			      pc_uids.contains(row.parent_item_uid)) &&
			     !item_owner_identity_equal(row.owner, player_owner)))
				return flatfile_item_repository_result::invalid;
		}
		std::vector<economic_item_snapshot> witnesses;
		for (const auto &entry : terms.selected_custody)
		{
			const auto *row = catalog_item(catalog, entry.item_uid);
			if (!row)
				return flatfile_item_repository_result::invalid;
			witnesses.push_back({ row->item_uid, accounting_position(*row) });
		}
		for (const auto &item : terms.frozen_outputs)
			witnesses.push_back({ item.object_uid,
					      { { item_owner_type::unknown, 0, 0 },
						0,
						0,
						0,
						item_custody_state::absent,
						0 } });
		if (smith_native_accounting_effects(command, original_player_wallet, witnesses,
						    &effects) != economic_accounting_error::ok)
			return flatfile_item_repository_result::invalid;
		flatfile_economic_authority_snapshot authority;
		const flatfile_economic_mapping_request wallet_request{
			original_player_wallet, { 1, terms.player_pid, {} }
		};
		const auto mapping = economic_flatfile_lock_authority(
			root, lock, effects.metadata.lineage, effects.metadata.epoch,
			std::span<const flatfile_economic_mapping_request>(&wallet_request, 1),
			&authority, error);
		if (mapping)
			return mapping == ENOMEM || mapping == EIO ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;

		ownership_catalog candidate = catalog;
		// Existing ordinary craft bookkeeping: missing destruction identity starts
		// at zero; preserve every field/revision of an existing row. No third clock.
		if (!ensure_owner(&candidate, { item_owner_type::destruction, 0, 0 }))
			return flatfile_item_repository_result::invalid;
		item_transfer_result applied{ terms.frozen_outputs.front().object_uid,
					      uint16_t(terms.selected_custody.size()),
					      npc_clock->revision + 1,
					      pc_clock->revision + 1,
					      0,
					      0 };
		for (const auto &event : effects.item_events)
		{
			if (event.before.state == item_custody_state::absent)
			{
				const auto output = std::find_if(
					terms.frozen_outputs.begin(), terms.frozen_outputs.end(),
					[&](const auto &item)
					{ return item.object_uid == event.uid; });
				if (output == terms.frozen_outputs.end() ||
				    candidate.items.size() >= ownership_maximum_entries)
					return flatfile_item_repository_result::invalid;
				candidate.items.push_back({ event.uid,
							    event.after.root_uid,
							    event.after.parent_uid,
							    event.after.owner,
							    event.after.revision,
							    output->vnum,
							    event.after.state,
							    {},
							    event.after.equipment_slot });
				std::sort(candidate.items.begin(), candidate.items.end(),
					  item_less);
			}
			else
			{
				auto *row = find_item(&candidate, event.uid);
				if (!row || !economic_item_position_equal(accounting_position(*row),
									  event.before))
					return flatfile_item_repository_result::invalid;
				row->root_item_uid = event.after.root_uid;
				row->parent_item_uid = event.after.parent_uid;
				row->owner = event.after.owner;
				row->item_revision = event.after.revision;
				row->state = event.after.state;
				row->equipment_slot = event.after.equipment_slot;
			}
			applied.max_item_revision =
				std::max(applied.max_item_revision, event.after.revision);
		}
		find_owner(&candidate, native_owner)->revision = applied.from_owner_revision;
		find_owner(&candidate, player_owner)->revision = applied.to_owner_revision;
		if (!valid_catalog(candidate) ||
		    !flat_native_custody_matches(candidate, original_images.native_after) ||
		    !flat_smith_player_custody(candidate, player_owner,
					       original_images.player_after))
			return flatfile_item_repository_result::invalid;
		for (const auto &item : effects.items_after)
		{
			const auto *row = catalog_item(candidate, item.uid);
			if (!row ||
			    !economic_item_position_equal(accounting_position(*row), item.position))
				return flatfile_item_repository_result::invalid;
		}
		candidate.operations.push_back({ command.operation_id, digest, 0, applied });
		std::vector<flatfile_authority_operation> prepared;
		flatfile_authority_operation native_operation;
		const int native_prepare = quest_mobile_native_flatfile_prepare_locked(
			root, lock, command.operation_id, native, original_images.native_after,
			&native_operation);
		if (native_prepare)
			return native_prepare == ENOMEM || native_prepare == EIO ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		prepared.push_back(std::move(native_operation));
		std::vector<uint8_t> catalog_bytes;
		if (!encode_catalog(candidate, catalog.revision + 1, &catalog_bytes))
			return flatfile_item_repository_result::invalid;
		prepared.push_back({ flatfile_authority_store::domains,
				     flatfile_authority_operation_kind::write, ownership_filename,
				     std::move(catalog_bytes) });
		flatfile_wallet_mutation wallet;
		const auto wallet_prepared = flatfile_player_domain_prepare_resurrection_wallet(
			root, lock, terms.player_pid, terms.player_wallet.before_revision,
			terms.player_wallet.before, terms.player_wallet.after, &wallet, error);
		if (wallet_prepared != flatfile_player_domain_result::ok ||
		    wallet.wallet_revision != terms.player_wallet.after_revision ||
		    wallet.after_images.size() != 1)
			return wallet_prepared == flatfile_player_domain_result::io_error ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		for (size_t i = 0; i < 4; ++i)
			if (wallet.wallet.amount[i] != terms.player_wallet.after[i])
				return flatfile_item_repository_result::invalid;
		for (auto &image : wallet.after_images)
			prepared.push_back({ flatfile_authority_store::domains,
					     flatfile_authority_operation_kind::write,
					     std::move(image.filename), std::move(image.bytes) });

		// Only genuine granted PC outputs enter player materialization. NPC ore
		// literals remain in the separate original Smith command/native image.
		std::vector<player_item_snapshot> granted, unchanged;
		if (player_item_snapshot_extract_subtree(
			    original_images.player_after, terms.frozen_outputs.front().object_uid,
			    &granted, &unchanged) != player_snapshot_codec_result::ok ||
		    granted.size() != terms.frozen_outputs.size() ||
		    !flat_smith_same_items(unchanged, original_images.player_before))
			return flatfile_item_repository_result::invalid;
		std::vector<uint8_t> blob;
		if (player_item_snapshot_list_encode(granted, &blob) !=
			    player_snapshot_codec_result::ok ||
		    blob.empty() || blob.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
			return flatfile_item_repository_result::invalid;
		item_transfer_payload materialization{};
		materialization.from_owner = { item_owner_type::system, 0, 0 };
		materialization.to_owner = player_owner;
		materialization.reason = item_transfer_reason::creation;
		materialization.multi_root = true;
		materialization.selected_item_uid = granted.front().object_uid;
		materialization.item_count = uint32_t(granted.size());
		materialization.item_blob_size = uint32_t(blob.size());
		std::copy(blob.begin(), blob.end(), materialization.item_blob.begin());
		for (size_t i = 0; i < granted.size(); ++i)
		{
			const auto *row = catalog_item(candidate, granted[i].object_uid);
			if (!row)
				return flatfile_item_repository_result::invalid;
			materialization.items[i] = { row->item_uid,
						     row->root_item_uid,
						     row->parent_item_uid,
						     ITEM_TRANSFER_ABSENT_REVISION,
						     row->vnum,
						     item_custody_state::absent };
		}
		flatfile_shop_trade_materialization_mutation output;
		const auto materialized = flatfile_item_transfer_materialization_prepare(
			root, lock, command.operation_id, materialization, &output, error);
		if (materialized != flatfile_shop_trade_materialization_result::ok)
			return materialized == flatfile_shop_trade_materialization_result::io_error ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		prepared.push_back({ flatfile_authority_store::domains,
				     flatfile_authority_operation_kind::write,
				     std::move(output.after_image.filename),
				     std::move(output.after_image.bytes) });
		item_transfer_custody_delta custody;
		custody.before = effects.items_before;
		custody.after = effects.items_after;
		custody.events = effects.item_events;
		// All fallible work completed. Root supplies the actual accounting/itemref
		// receipt binding and sole shared Smith commit. The root has already
		// recovered shared/legacy journals before calling this participant; the
		// wallet helper repeats that same-lock idempotent recovery, not a Smith write.
		*result = applied;
		*result_code = 0;
		*delta = std::move(custody);
		*expected_effects = std::move(effects);
		*operations = std::move(prepared);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
}

flatfile_item_repository_result flatfile_shared_shop_initial_custody_storage::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_shared_shop_initial_custody_stage *output, std::string *error) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	try
	{
		if (!native_mobile_birth_shared_shop_recovery_initial(original))
			return flatfile_item_repository_result::invalid;
		native_mobile_birth_shared_shop_recovery_context context;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		if (native_mobile_birth_shared_shop_recovery_decode(
			    original.command, original.attachment, &context) !=
			    economic_accounting_error::ok ||
		    native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::shared_shopkeeper || !image.cash)
			return flatfile_item_repository_result::invalid;
		// Full original checkpoint binding is checked by the shared codec.
		// Actual selected keeper/native absence is independently read here;
		// neither a supplied presence bool nor an empty DTO is authority.
		const uint32_t shop_id = static_cast<uint32_t>(role.original.reset_shop_index);
		std::vector<flatfile_shopkeeper_record> keepers;
		const auto keeper_read =
			flatfile_shopkeeper_list_locked(root, lock, &keepers, error);
		if (keeper_read != flatfile_shopkeeper_result::ok &&
		    keeper_read != flatfile_shopkeeper_result::not_found)
			return keeper_read == flatfile_shopkeeper_result::io_error ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		if (std::any_of(keepers.begin(), keepers.end(),
				[&](const auto &keeper) { return keeper.shop_id == shop_id; }))
			return flatfile_item_repository_result::invalid;
		quest_mobile_native_flatfile_row native;
		const int native_read = quest_mobile_native_flatfile_read_locked(
			root, lock, image.reference.mobile_instance_id, &native);
		if (native_read || native.present)
			return native_read == ENOMEM || native_read == EIO ?
				       flatfile_item_repository_result::io_error :
				       flatfile_item_repository_result::invalid;
		ownership_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_item_repository_result::ok &&
		    loaded != flatfile_item_repository_result::not_found)
			return loaded;
		if (!valid_catalog(catalog))
			return flatfile_item_repository_result::invalid;
		const item_owner_identity shop_owner{ item_owner_type::shopkeeper,
						      item_shopkeeper_owner_id(shop_id), 0 };
		const item_owner_identity native_owner{ item_owner_type::native_mobile,
							image.reference.mobile_instance_id, 0 };
		if (!item_owner_identity_valid(shop_owner) || find_owner(&catalog, native_owner))
			return flatfile_item_repository_result::invalid;
		for (const auto &owner : catalog.owners)
			if (owner.owner.type == item_owner_type::shopkeeper &&
			    owner.owner.id == shop_owner.id && owner.owner.context_id)
				return flatfile_item_repository_result::invalid;
		std::unordered_set<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &item : image.items)
			if (!born.insert(item.object_uid).second)
				return flatfile_item_repository_result::invalid;
		// Retired/quarantined records and old foreign links still exclude UID
		// reuse. Preserve them; do not convert their existence into absence.
		for (const auto &row : catalog.items)
			if (born.contains(row.item_uid) || born.contains(row.root_item_uid) ||
			    born.contains(row.parent_item_uid) ||
			    (item_owner_identity_equal(row.owner, shop_owner) &&
			     row.state == item_custody_state::active))
				return flatfile_item_repository_result::invalid;
		for (const auto &operation : catalog.operations)
			if (operation.operation_id.bytes == original.command.operation_id.bytes ||
			    born.contains(operation.result.root_item_uid))
				return flatfile_item_repository_result::invalid;
		flatfile_shared_shop_initial_custody_stage stage;
		stage.catalog_before_present = loaded == flatfile_item_repository_result::ok;
		stage.catalog_after_present = stage.catalog_before_present;
		stage.catalog_before_revision = catalog.revision;
		stage.catalog_after_revision = catalog.revision;
		auto &participant = stage.participant;
		participant.shop_id = shop_id;
		participant.shop_after_present = true;
		participant.shop_revision_after = 1; // Same accepted initial keeper stage.
		const auto *owner_before = find_owner(&catalog, shop_owner);
		participant.owner_before_present = owner_before != nullptr;
		participant.owner_revision_before = owner_before ? owner_before->revision : 0;
		participant.owner_after_present = participant.owner_before_present;
		participant.owner_revision_after = participant.owner_revision_before;
		participant.born_cash = image.cash->denominations.amount;
		if (!image.items.empty())
		{
			if (participant.owner_revision_before == UINT64_MAX ||
			    catalog.revision == UINT64_MAX ||
			    image.items.size() > ownership_maximum_entries ||
			    catalog.items.size() > ownership_maximum_entries - image.items.size())
				return flatfile_item_repository_result::invalid;
			participant.owner_after_present = true;
			participant.owner_revision_after = participant.owner_revision_before + 1;
		}
		if (native_mobile_birth_cash_role_accounting_compile(original.command, participant,
								     &stage.plan) !=
			    economic_accounting_error::ok ||
		    stage.plan.item_events.size() != image.items.size() ||
		    stage.plan.items_before.size() != image.items.size() ||
		    stage.plan.items_after.size() != image.items.size())
			return flatfile_item_repository_result::invalid;
		if (!image.items.empty())
		{
			auto *owner_after = ensure_owner(&catalog, shop_owner);
			if (!owner_after)
				return flatfile_item_repository_result::io_error;
			owner_after->revision = participant.owner_revision_after;
			for (const auto &event : stage.plan.item_events)
			{
				const auto item =
					std::find_if(image.items.begin(), image.items.end(),
						     [&](const auto &literal)
						     { return literal.object_uid == event.uid; });
				if (item == image.items.end() ||
				    event.before.state != item_custody_state::absent ||
				    event.after.state != item_custody_state::active ||
				    event.after.revision != 1 ||
				    !item_owner_identity_equal(event.after.owner, shop_owner) ||
				    event.after.equipment_slot != item->equipment_slot)
					return flatfile_item_repository_result::invalid;
				// Complete literal properties, including money denominations,
				// remain in the SAME native/SHOP checkpoint participants.
				// Reuse existing empty coin_payload compatibility; no new coin
				// mutation, normalized projection or spending route is added.
				catalog.items.push_back({ event.uid,
							  event.after.root_uid,
							  event.after.parent_uid,
							  event.after.owner,
							  event.after.revision,
							  item->vnum,
							  event.after.state,
							  {},
							  event.after.equipment_slot });
			}
			std::sort(catalog.items.begin(), catalog.items.end(), item_less);
			if (!valid_catalog(catalog))
				return flatfile_item_repository_result::invalid;
			stage.catalog_after_present = true;
			stage.catalog_after_revision = catalog.revision + 1;
			stage.operation.store = flatfile_authority_store::domains;
			stage.operation.kind = flatfile_authority_operation_kind::write;
			stage.operation.filename = ownership_filename;
			if (!encode_catalog(catalog, stage.catalog_after_revision,
					    &stage.operation.bytes))
				return flatfile_item_repository_result::invalid;
			stage.has_operation = true;
		}
		// No generic item-transfer receipt is fabricated. The real root binds
		// exact plan/item references/source claim/MBR4 with every native AFTER
		// in one original authority bundle, and owns uncertainty/replay proof.
		if (!lock.matches(root))
			return flatfile_item_repository_result::invalid;
		static_assert(std::is_nothrow_move_assignable_v<
			      flatfile_shared_shop_initial_custody_stage>);
		*output = std::move(stage);
		return flatfile_item_repository_result::ok;
	}
	catch (...)
	{
		return flatfile_item_repository_result::io_error;
	}
}

namespace
{
bool custody_budget_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
bool custody_budget_product(size_t count, size_t width, size_t *bytes) noexcept
{
	if (!bytes || (width && count > SIZE_MAX / width))
		return false;
	*bytes = count * width;
	return true;
}

// Pure scalar shape scan. Never calls quest's allocating decoder (or a hash).
bool custody_quest_wire_working(std::span<const uint8_t> bytes, size_t *working) noexcept
{
	if (!working || bytes.size() < 40 || bytes.size() > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	decoder in{ bytes.data(), bytes.size() };
	const auto skip = [&](size_t n) noexcept
	{
		if (n > in.size - in.offset)
			return false;
		in.offset += n;
		return true;
	};
	uint32_t version = 0, roots = 0, rewards = 0, credited = 0, length = 0, awards = 0;
	if (!in.number(&version) || version < 1 || version > 6 || !skip(28) || !in.number(&roots) ||
	    roots > 14 || (version == 6 && roots) || !skip(size_t{ roots } * sizeof(uint64_t)) ||
	    !in.number(&rewards) || rewards > 64 ||
	    !skip(size_t{ rewards } * (version >= 4 ? 16 :
				       version >= 3 ? 12 :
						      8)))
		return false;
	size_t string_requests = 0;
	if (version >= 2)
	{
		if (!skip(20) || !in.number(&credited) ||
		    credited > QUEST_REWARD_MAX_CREDITED_PIDS ||
		    !skip(size_t{ credited } * sizeof(uint32_t)))
			return false;
		for (size_t index = 0; index < 2; ++index)
		{
			const size_t maximum = index ? QUEST_REWARD_MAX_DEFINITION_ID_BYTES :
						       QUEST_REWARD_MAX_CHARACTER_NAME_BYTES;
			if (!in.number(&length) || !length || length > maximum || !skip(length))
				return false;
			// Fresh assign to SSO capacity 15 uses _M_create(max(n,2*15))+1.
			if (length > 15 &&
			    !custody_budget_add(string_requests,
						std::max(size_t{ 30 }, size_t{ length }) + 1))
				return false;
		}
		if (version >= 5 && (!in.number(&awards) || awards > 64 ||
				     !skip(size_t{ awards } * 3 * sizeof(uint32_t))))
			return false;
	}
	if (version == 6)
	{
		constexpr size_t tail =
			4 + 16 + ECONOMIC_SOURCE_EVENT_BYTES + 8 + 16 + ITEM_TRANSFER_RESULT_BYTES;
		if (in.size - in.offset != tail || memcmp(in.data + in.offset, "QRF6", 4) ||
		    !skip(tail))
			return false;
	}
	if (in.offset != in.size)
		return false;
	// Original v1-v5 ends COPY assignment to the caller's fresh terms; both
	// strings' requests remain live twice. v6 moves. Both actual DTOs coexist.
	size_t result = 2 * sizeof(quest_reward_continuation);
	if (!custody_budget_add(result, string_requests) ||
	    (version < 6 && !custody_budget_add(result, string_requests)))
		return false;
	// Both quest DTOs/strings remain live through sequential v6 tail checks.
	// Source decoding retains an event plus its nested ID DTO; later result
	// decoding retains the actual transfer-result DTO. These phases do not sum.
	if (version == 6 &&
	    !custody_budget_add(
		    result, std::max(sizeof(economic_source_event) + sizeof(critical_operation_id),
				     sizeof(item_transfer_result))))
		return false;
	*working = result;
	return true;
}

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
template <typename Key, typename Hash>
bool custody_reserved_set_requests(size_t count, size_t *bucket_bytes, size_t *node_bytes) noexcept
{
	// unordered_set::reserve on a fresh table calls rehash(max(bkt_for(n),
	// bkt_for(element_count+1))). Even n==0 requests two buckets. This policy
	// method uses only scalar/static prime-table lookup and does not allocate.
	std::__detail::_Prime_rehash_policy policy;
	const size_t buckets = policy._M_next_bkt(
		std::max(policy._M_bkt_for_elements(count), policy._M_bkt_for_elements(1)));
	return custody_budget_product(buckets, sizeof(std::__detail::_Hash_node_base *),
				      bucket_bytes) &&
	       custody_budget_product(
		       count,
		       sizeof(std::__detail::_Hash_node<Key, std::__cache_default<Key, Hash>::value>),
		       node_bytes);
}
#endif
} // namespace

size_t flatfile_item_catalog_preflight_object_bytes() noexcept
{
	// Actual named objects in catalog scan and its sequential nested scans.
	// Pure scalar/lambda call frames are outside this C++ payload policy.
	size_t nested = 2 * sizeof(player_item_snapshot_list_allocation_profile) +
			player_item_snapshot_list_decoder_object_bytes();
	nested = std::max(nested, sizeof(decoder)); // Quest shape scanner.
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	nested = std::max(nested, sizeof(std::__detail::_Prime_rehash_policy));
#endif
	return sizeof(flatfile_item_catalog_allocation_profile) + 2 * sizeof(decoder) + nested;
}

flatfile_item_repository_result
flatfile_item_catalog_preflight(std::span<const uint8_t> bytes,
				flatfile_item_catalog_allocation_profile *output) noexcept
{
	if (!output || bytes.size() < 56 || bytes.size() > ownership_maximum_bytes ||
	    memcmp(bytes.data(), ownership_magic.data(), ownership_magic.size()))
		return flatfile_item_repository_result::invalid;
	decoder header{ bytes.data() + ownership_magic.size(),
			bytes.size() - ownership_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || version < ownership_legacy_format_version ||
	    version > ownership_format_version || !header.number(&payload_size) ||
	    !header.number(&revision) || !revision || payload_size != bytes.size() - 56)
		return flatfile_item_repository_result::invalid;
	flatfile_item_catalog_allocation_profile profile;
	profile.framing_working_object_bytes = flatfile_item_catalog_preflight_object_bytes();
	profile.format_version = version;
	decoder in{ bytes.data() + 56, payload_size };
	const auto skip = [&](size_t n) noexcept
	{
		if (n > in.size - in.offset)
			return false;
		in.offset += n;
		return true;
	};
	if (!in.number(&profile.owner_count) || !in.number(&profile.item_count) ||
	    !in.number(&profile.operation_count) ||
	    profile.owner_count > ownership_maximum_entries ||
	    profile.item_count > ownership_maximum_entries ||
	    profile.operation_count > ownership_maximum_operations)
		return flatfile_item_repository_result::invalid;
	size_t payload = sizeof(ownership_catalog), request = 0, coin_working = 0,
	       quest_hash_working = 0;
	if (!custody_budget_product(profile.owner_count, sizeof(owner_state), &request) ||
	    !custody_budget_add(payload, request) ||
	    !custody_budget_product(profile.item_count, sizeof(flatfile_item_ownership_record),
				    &request) ||
	    !custody_budget_add(payload, request) ||
	    !custody_budget_product(profile.operation_count, sizeof(operation_state), &request) ||
	    !custody_budget_add(payload, request) ||
	    !custody_budget_product(profile.owner_count, 1 + 3 * sizeof(uint64_t), &request) ||
	    !skip(request))
		return flatfile_item_repository_result::invalid;
	size_t op_buckets = 0, op_nodes = 0, op_node_width = 0;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	using op_key = std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>;
	op_node_width = sizeof(
		std::__detail::_Hash_node<op_key,
					  std::__cache_default<op_key, operation_id_hash>::value>);
	if (!custody_reserved_set_requests<op_key, operation_id_hash>(profile.operation_count,
								      &op_buckets, &op_nodes))
		return flatfile_item_repository_result::invalid;
	profile.storage_policy_supported = true;
#endif
	for (size_t index = 0; index < profile.item_count; ++index)
	{
		if (!skip(6 * sizeof(uint64_t) + sizeof(int32_t) + 2 * sizeof(uint8_t)))
			return flatfile_item_repository_result::invalid;
		if (version >= 3)
		{
			uint32_t length = 0;
			if (!in.number(&length) || length > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
			    length > in.size - in.offset || !custody_budget_add(payload, length))
				return flatfile_item_repository_result::invalid;
			if (length)
			{
				player_item_snapshot_list_allocation_profile items;
				if (player_item_snapshot_list_preflight(in.data + in.offset, length,
									&items) !=
				    player_snapshot_codec_result::ok)
					return flatfile_item_repository_result::invalid;
				profile.storage_policy_supported =
					profile.storage_policy_supported &&
					items.fresh_decode_storage_policy_supported;
				size_t working = sizeof(std::vector<player_item_snapshot>);
				if (!custody_budget_add(working, items.decoded_payload_bytes) ||
				    !custody_budget_add(working,
							items.relationship_scratch_bytes) ||
				    !custody_budget_add(working,
							items.item_codec_decoder_object_bytes))
					return flatfile_item_repository_result::invalid;
				coin_working = std::max(coin_working, working);
			}
			if (!skip(length))
				return flatfile_item_repository_result::invalid;
		}
		if (version >= 5 && !skip(sizeof(uint16_t)))
			return flatfile_item_repository_result::invalid;
	}
	for (size_t index = 0; index < profile.operation_count; ++index)
	{
		constexpr size_t original = CRITICAL_COMMAND_ID_BYTES + SHA256_DIGEST_LENGTH +
					    sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint16_t) +
					    3 * sizeof(uint64_t);
		if (!skip(original) || (version >= 2 && !skip(sizeof(uint64_t))))
			return flatfile_item_repository_result::invalid;
		uint8_t flag = 0;
		if (version >= 4 && (!in.number(&flag) || flag > 1))
			return flatfile_item_repository_result::invalid;
		if (version >= 3 &&
		    (!in.number(&flag) || flag > 1 || (flag && !skip(COIN_TRANSFER_RESULT_BYTES))))
			return flatfile_item_repository_result::invalid;
		if (version >= 6)
		{
			uint32_t length = 0;
			if (!in.number(&length) || length > ITEM_TRANSFER_CONTINUATION_MAX_BYTES ||
			    length > in.size - in.offset || !custody_budget_add(payload, length))
				return flatfile_item_repository_result::invalid;
			if (length)
			{
				size_t quest = 0;
				if (!custody_quest_wire_working({ in.data + in.offset, length },
								&quest) ||
				    !custody_budget_product(index, op_node_width, &request) ||
				    !custody_budget_add(quest, request))
					return flatfile_item_repository_result::invalid;
				quest_hash_working = std::max(quest_hash_working, quest);
			}
			if (!skip(length) || !in.number(&flag) || flag > 1)
				return flatfile_item_repository_result::invalid;
		}
		if (version >= 7)
		{
			uint64_t mask = 0;
			if (!in.number(&mask) ||
			    !custody_budget_product(std::popcount(mask), sizeof(player_revision_t),
						    &request) ||
			    !custody_budget_add(payload, request) || !skip(request))
				return flatfile_item_repository_result::invalid;
		}
		if (version >= 8 && (!skip(sizeof(uint64_t) + sizeof(uint32_t) + sizeof(int32_t)) ||
				     !in.number(&flag) || flag > 1))
			return flatfile_item_repository_result::invalid;
	}
	if (in.offset != in.size)
		return flatfile_item_repository_result::invalid;
	profile.decoded_catalog_payload_bytes = payload;
	size_t operation_working =
		sizeof(std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>,
					  operation_id_hash>);
	if (!custody_budget_add(operation_working, op_buckets) ||
	    !custody_budget_add(operation_working, std::max(op_nodes, quest_hash_working)))
		return flatfile_item_repository_result::invalid;
	profile.validation_working_bytes = std::max(coin_working, operation_working);
	profile.authenticated_decode_working_bytes = payload;
	if (!custody_budget_add(profile.authenticated_decode_working_bytes,
				2 * sizeof(decoder) + SHA256_DIGEST_LENGTH) ||
	    !custody_budget_add(profile.authenticated_decode_working_bytes,
				profile.validation_working_bytes))
		return flatfile_item_repository_result::invalid;
	*output = profile;
	return flatfile_item_repository_result::ok;
}

flatfile_item_repository_result flatfile_shared_shop_current_custody_storage::read_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, std::span<const uint8_t> stored_result,
	std::span<const uint8_t> stored_plan, flatfile_shared_shop_current_custody *output,
	std::string *error) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	try
	{
		if (!native_mobile_birth_shared_shop_recovery_valid(original))
			return flatfile_item_repository_result::invalid;
		native_mobile_birth_shared_shop_recovery_context context;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		native_mobile_birth_cash_role_result result;
		if (native_mobile_birth_shared_shop_recovery_decode(
			    original.command, original.attachment, &context) !=
			    economic_accounting_error::ok ||
		    native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    !native_mobile_birth_cash_role_result_decode(stored_result, &result) ||
		    result.role != native_mobile_birth_cash_role::shared_shopkeeper)
			return flatfile_item_repository_result::invalid;
		const auto &p = result.shared;
		if (p.shop_before_present || p.shop_revision_before || !p.shop_after_present ||
		    p.shop_revision_after != 1 ||
		    (!image.items.empty() ?
			     (!p.owner_after_present || p.owner_revision_before == UINT64_MAX ||
			      p.owner_revision_after != p.owner_revision_before + 1) :
			     (p.owner_before_present != p.owner_after_present ||
			      p.owner_revision_before != p.owner_revision_after)))
			return flatfile_item_repository_result::invalid;
		economic_accounting_plan plan;
		std::vector<uint8_t> canonical_plan;
		std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> canonical_result{};
		if (native_mobile_birth_cash_role_accounting_compile(original.command, p, &plan) !=
			    economic_accounting_error::ok ||
		    economic_plan_encode(plan, &canonical_plan) != economic_accounting_error::ok ||
		    canonical_plan.size() != stored_plan.size() ||
		    !std::equal(canonical_plan.begin(), canonical_plan.end(),
				stored_plan.begin()) ||
		    !native_mobile_birth_cash_role_result_matches(original.command, p, plan,
								  result) ||
		    !native_mobile_birth_cash_role_result_encode(result, &canonical_result) ||
		    canonical_result.size() != stored_result.size() ||
		    !std::equal(canonical_result.begin(), canonical_result.end(),
				stored_result.begin()))
			return flatfile_item_repository_result::invalid;
		if (context.progress.receipt_present)
		{
			const auto &receipt = context.progress.receipt;
			// Same immutable economic core policy as original receipt_core_equal:
			// successful applied/already_applied may differ; delivery metadata
			// is independently authenticated by the root. No terminal-stage gate.
			if ((receipt.outcome != critical_apply_outcome::applied &&
			     receipt.outcome != critical_apply_outcome::already_applied) ||
			    receipt.operation_id.bytes != original.command.operation_id.bytes ||
			    receipt.durable_revision !=
				    std::max(uint64_t{ 1 }, p.owner_revision_after) ||
			    receipt.error_code ||
			    receipt.failure_stage != critical_failure_stage::none ||
			    receipt.disposition != critical_completion_disposition::execution ||
			    receipt.result_size != stored_result.size() ||
			    !std::equal(stored_result.begin(), stored_result.end(),
					receipt.result_payload.begin()) ||
			    !std::all_of(receipt.result_payload.begin() + receipt.result_size,
					 receipt.result_payload.end(),
					 [](uint8_t b) { return !b; }))
				return flatfile_item_repository_result::invalid;
		}
		ownership_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_item_repository_result::ok &&
		    loaded != flatfile_item_repository_result::not_found)
			return loaded;
		if (!valid_catalog(catalog))
			return flatfile_item_repository_result::invalid;
		const item_owner_identity selected{ item_owner_type::shopkeeper,
						    item_shopkeeper_owner_id(p.shop_id), 0 };
		for (const auto &owner : catalog.owners)
			if ((owner.owner.type == item_owner_type::shopkeeper &&
			     owner.owner.id == selected.id && owner.owner.context_id) ||
			    (owner.owner.type == item_owner_type::native_mobile &&
			     owner.owner.id == image.reference.mobile_instance_id))
				return flatfile_item_repository_result::invalid;
		flatfile_shared_shop_current_custody observed;
		const auto *owner = find_owner(&catalog, selected);
		observed.owner_present = owner != nullptr;
		observed.owner_revision = owner ? owner->revision : 0;
		if (observed.owner_present != p.owner_after_present ||
		    observed.owner_revision != p.owner_revision_after)
			return flatfile_item_repository_result::invalid;
		std::unordered_set<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			if (!born.insert(literal.object_uid).second)
				return flatfile_item_repository_result::invalid;
		for (const auto &operation : catalog.operations)
			if (operation.operation_id.bytes == original.command.operation_id.bytes ||
			    born.contains(operation.result.root_item_uid))
				return flatfile_item_repository_result::invalid;
		observed.rows.reserve(image.items.size());
		for (const auto &row : catalog.items)
		{
			const bool born_row = born.contains(row.item_uid);
			if (!born_row)
			{
				// A foreign/inactive descendant still claims the original
				// born forest; active-owner filtering must never hide it.
				if (born.contains(row.root_item_uid) ||
				    born.contains(row.parent_item_uid) ||
				    (item_owner_identity_equal(row.owner, selected) &&
				     row.state == item_custody_state::active))
					return flatfile_item_repository_result::invalid;
				continue;
			}
			const auto event = std::find_if(plan.item_events.begin(),
							plan.item_events.end(), [&](const auto &e)
							{ return e.uid == row.item_uid; });
			const auto literal = std::find_if(image.items.begin(), image.items.end(),
							  [&](const auto &i)
							  { return i.object_uid == row.item_uid; });
			if (event == plan.item_events.end() || literal == image.items.end() ||
			    !item_owner_identity_equal(row.owner, selected) ||
			    row.state != item_custody_state::active ||
			    row.root_item_uid != event->after.root_uid ||
			    row.parent_item_uid != event->after.parent_uid ||
			    row.item_revision != event->after.revision ||
			    row.equipment_slot != event->after.equipment_slot ||
			    row.vnum != literal->vnum || !row.coin_payload.empty())
				return flatfile_item_repository_result::invalid;
			observed.rows.push_back(row);
		}
		// Missing or retired born rows refuse even when the active projection
		// would otherwise have looked empty. Catalog UID uniqueness is already
		// authenticated; unrelated inactive selected history stays untouched.
		if (observed.rows.size() != image.items.size() || !lock.matches(root))
			return flatfile_item_repository_result::invalid;
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_shared_shop_current_custody>);
		*output = std::move(observed);
		return flatfile_item_repository_result::ok;
	}
	catch (...)
	{
		return flatfile_item_repository_result::io_error;
	}
}

flatfile_item_repository_result
flatfile_shared_shop_current_custody_storage::read_original_projection_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const quest_mobile_native_image &image,
	const native_mobile_birth_shared_shop_participant &p, const economic_accounting_plan &plan,
	const critical_operation_id &original_operation,
	flatfile_shared_shop_current_custody *output,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return flatfile_item_repository_result::io_error;
#else
	if (root.empty() || !output || !reserve_scratch_peak || !lock.matches(root) ||
	    critical_operation_id_is_zero(original_operation) || p.shop_before_present ||
	    p.shop_revision_before || !p.shop_after_present || p.shop_revision_after != 1 ||
	    (!image.items.empty() ?
		     (!p.owner_after_present || p.owner_revision_before == UINT64_MAX ||
		      p.owner_revision_after != p.owner_revision_before + 1) :
		     (p.owner_before_present != p.owner_after_present ||
		      p.owner_revision_before != p.owner_revision_after)))
		return flatfile_item_repository_result::invalid;
	size_t buckets = 0, nodes = 0, rows = 0;
	if (!custody_reserved_set_requests<uint64_t, std::hash<uint64_t>>(image.items.size(),
									  &buckets, &nodes) ||
	    !custody_budget_product(image.items.size(), sizeof(flatfile_item_ownership_record),
				    &rows))
		return flatfile_item_repository_result::invalid;
	constexpr size_t fixed =
		sizeof(ownership_catalog) + sizeof(flatfile_shared_shop_current_custody) +
		sizeof(std::unordered_set<uint64_t>) +
		sizeof(flatfile_item_catalog_allocation_profile) + sizeof(item_owner_identity);
	constexpr size_t read_fixed = 2 * sizeof(std::string) + sizeof(std::vector<uint8_t>);
	size_t base = outer_live_scratch, directory_size = root.size(), read_live = 0;
	if (!custody_budget_add(base, fixed) ||
	    !custody_budget_add(directory_size, sizeof("/domains") - 1))
		return flatfile_item_repository_result::invalid;
	read_live = base;
	if (!custody_budget_add(read_live, read_fixed) ||
	    (directory_size > 15 &&
	     (directory_size == SIZE_MAX || !custody_budget_add(read_live, directory_size + 1))) ||
	    !reserve_scratch_peak(read_live, context))
	{
		errno = ENOBUFS;
		return flatfile_item_repository_result::io_error;
	}
	try
	{
		ownership_catalog catalog;
		flatfile_shared_shop_current_custody observed;
		std::unordered_set<uint64_t> born;
		flatfile_item_catalog_allocation_profile profile;
		bool present = false;
		{
			// Length constructors avoid root operator+ growth and intermediate copies.
			std::string directory(directory_size, '\0');
			std::copy(root.begin(), root.end(), directory.begin());
			std::copy_n("/domains", sizeof("/domains") - 1,
				    directory.begin() + root.size());
			const std::string file_name(ownership_filename);
			std::vector<uint8_t> bytes;
			errno = 0;
			const auto read = flatfile_read_bounded(directory, file_name,
								ownership_maximum_bytes, &bytes,
								reserve_scratch_peak, context,
								read_live);
			if (read == flatfile_read_result::invalid)
				return flatfile_item_repository_result::invalid;
			if (read != flatfile_read_result::ok &&
			    read != flatfile_read_result::not_found)
				return flatfile_item_repository_result::io_error;
			present = read == flatfile_read_result::ok;
			if (present)
			{
				size_t scan_live = read_live;
				if (!custody_budget_add(scan_live, bytes.capacity()) ||
				    !custody_budget_add(
					    scan_live,
					    flatfile_item_catalog_preflight_object_bytes()) ||
				    !reserve_scratch_peak(scan_live, context))
				{
					errno = ENOBUFS;
					return flatfile_item_repository_result::io_error;
				}
				const auto framed =
					flatfile_item_catalog_preflight(bytes, &profile);
				if (framed != flatfile_item_repository_result::ok)
					return framed;
				if (!profile.storage_policy_supported)
				{
					errno = ENOTSUP;
					return flatfile_item_repository_result::io_error;
				}
				size_t decode_live = read_live;
				if (!custody_budget_add(decode_live, bytes.capacity()) ||
				    !custody_budget_add(
					    decode_live,
					    profile.authenticated_decode_working_bytes) ||
				    !reserve_scratch_peak(decode_live, context))
				{
					errno = ENOBUFS;
					return flatfile_item_repository_result::io_error;
				}
				if (!lock.matches(root))
					return flatfile_item_repository_result::invalid;
				const auto decoded = decode_catalog(bytes, &catalog);
				if (decoded != flatfile_item_repository_result::ok)
					return decoded;
			}
		} // File vector and paths die BEFORE projection admission/allocation.
		// Original CURRENT checks valid_catalog even for absent store and repeats it
		// for present store. Full authenticated decode already ran it when present.
		size_t validation_live = base;
		if (present &&
		    (!custody_budget_add(validation_live, profile.decoded_catalog_payload_bytes -
								  sizeof(ownership_catalog)) ||
		     !custody_budget_add(validation_live, profile.validation_working_bytes)))
			return flatfile_item_repository_result::invalid;
		if (!present)
		{
			size_t empty_buckets = 0, empty_nodes = 0;
			if (!custody_reserved_set_requests<
				    std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>,
				    operation_id_hash>(0, &empty_buckets, &empty_nodes) ||
			    !custody_budget_add(
				    validation_live,
				    sizeof(std::unordered_set<
					    std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>,
					    operation_id_hash>)) ||
			    !custody_budget_add(validation_live, empty_buckets))
				return flatfile_item_repository_result::invalid;
		}
		if (!reserve_scratch_peak(validation_live, context))
		{
			errno = ENOBUFS;
			return flatfile_item_repository_result::io_error;
		}
		if (!valid_catalog(catalog))
			return flatfile_item_repository_result::invalid;
		size_t projection_live = base;
		if ((present &&
		     !custody_budget_add(projection_live, profile.decoded_catalog_payload_bytes -
								  sizeof(ownership_catalog))) ||
		    !custody_budget_add(projection_live, buckets) ||
		    !custody_budget_add(projection_live, nodes) ||
		    !custody_budget_add(projection_live, rows) ||
		    !reserve_scratch_peak(projection_live, context))
		{
			errno = ENOBUFS;
			return flatfile_item_repository_result::io_error;
		}
		const item_owner_identity selected{ item_owner_type::shopkeeper,
						    item_shopkeeper_owner_id(p.shop_id), 0 };
		for (const auto &owner : catalog.owners)
			if ((owner.owner.type == item_owner_type::shopkeeper &&
			     owner.owner.id == selected.id && owner.owner.context_id) ||
			    (owner.owner.type == item_owner_type::native_mobile &&
			     owner.owner.id == image.reference.mobile_instance_id))
				return flatfile_item_repository_result::invalid;
		const auto *owner = find_owner(&catalog, selected);
		observed.owner_present = owner != nullptr;
		observed.owner_revision = owner ? owner->revision : 0;
		if (observed.owner_present != p.owner_after_present ||
		    observed.owner_revision != p.owner_revision_after)
			return flatfile_item_repository_result::invalid;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			if (!born.insert(literal.object_uid).second)
				return flatfile_item_repository_result::invalid;
		for (const auto &operation : catalog.operations)
			if (operation.operation_id.bytes == original_operation.bytes ||
			    born.contains(operation.result.root_item_uid))
				return flatfile_item_repository_result::invalid;
		observed.rows.reserve(image.items.size());
		for (const auto &row : catalog.items)
		{
			const bool born_row = born.contains(row.item_uid);
			if (!born_row)
			{
				// A foreign/inactive descendant still claims the original
				// born forest; active-owner filtering must never hide it.
				if (born.contains(row.root_item_uid) ||
				    born.contains(row.parent_item_uid) ||
				    (item_owner_identity_equal(row.owner, selected) &&
				     row.state == item_custody_state::active))
					return flatfile_item_repository_result::invalid;
				continue;
			}
			const auto event = std::find_if(plan.item_events.begin(),
							plan.item_events.end(), [&](const auto &e)
							{ return e.uid == row.item_uid; });
			const auto literal = std::find_if(image.items.begin(), image.items.end(),
							  [&](const auto &i)
							  { return i.object_uid == row.item_uid; });
			if (event == plan.item_events.end() || literal == image.items.end() ||
			    !item_owner_identity_equal(row.owner, selected) ||
			    row.state != item_custody_state::active ||
			    row.root_item_uid != event->after.root_uid ||
			    row.parent_item_uid != event->after.parent_uid ||
			    row.item_revision != event->after.revision ||
			    row.equipment_slot != event->after.equipment_slot ||
			    row.vnum != literal->vnum || !row.coin_payload.empty())
				return flatfile_item_repository_result::invalid;
			observed.rows.push_back(row);
		}
		// Missing or retired born rows refuse even when the active projection
		// would otherwise have looked empty. Catalog UID uniqueness is already
		// authenticated; unrelated inactive selected history stays untouched.
		if (observed.rows.size() != image.items.size() || !lock.matches(root))
			return flatfile_item_repository_result::invalid;
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_shared_shop_current_custody>);
		*output = std::move(observed);
		return flatfile_item_repository_result::ok;
	}
	catch (...)
	{
		return flatfile_item_repository_result::io_error;
	}
#endif
}

flatfile_item_repository_result flatfile_initial_room_reset_custody_storage::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	const flatfile_initial_room_reset_world_stage &world,
	flatfile_initial_room_reset_custody_stage *output) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_item_repository_result::invalid;
	try
	{
		zone_reset_item_image image;
		if (!zone_reset_item_recovery_initial(original) ||
		    zone_reset_item_command_decode(original.command, &image) !=
			    economic_accounting_error::ok ||
		    !critical_command_equal(world.original_command, original.command) ||
		    world.room_revision_before != image.expected_room_revision ||
		    world.room_revision_before == UINT64_MAX ||
		    world.room_revision_after != world.room_revision_before + 1 ||
		    (!world.room_before_present &&
		     (!world.room_before_items.empty() || world.room_revision_before)) ||
		    (world.room_before_present && !world.room_revision_before) ||
		    !image.placement || image.placement->fall_selected)
			return flatfile_item_repository_result::invalid;
		ownership_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, nullptr);
		if (loaded != flatfile_item_repository_result::ok &&
		    loaded != flatfile_item_repository_result::not_found)
			return loaded;
		if (!valid_catalog(catalog) || catalog.revision == UINT64_MAX ||
		    catalog.items.size() > ownership_maximum_entries - image.items.size())
			return flatfile_item_repository_result::invalid;
		const item_owner_identity selected{ item_owner_type::room,
						    static_cast<uint64_t>(image.room_vnum), 0 };
		const auto *before_owner = find_owner(&catalog, selected);
		const uint64_t before_revision = before_owner ? before_owner->revision : 0;
		if (before_revision != world.room_revision_before)
			return flatfile_item_repository_result::invalid;
		for (const auto &owner : catalog.owners)
			if (owner.owner.type == selected.type && owner.owner.id == selected.id &&
			    owner.owner.context_id)
				return flatfile_item_repository_result::invalid;
		std::unordered_set<uint64_t> born, before_uids;
		born.reserve(image.items.size());
		before_uids.reserve(world.room_before_items.size());
		for (const auto &item : image.items)
			if (item.type < ITEM_LOWEST || item.type > ITEM_LAST ||
			    item.type == ITEM_CORPSE || (item.extra_flags & ITEM_ARTIFACT) ||
			    !born.insert(item.object_uid).second)
				return flatfile_item_repository_result::invalid;
		for (const auto &item : world.room_before_items)
			if (!before_uids.insert(item.object_uid).second ||
			    born.contains(item.object_uid))
				return flatfile_item_repository_result::invalid;
		size_t observed = 0;
		for (const auto &row : catalog.items)
		{
			// All histories and foreign root/parent claims exclude reuse.
			if (born.contains(row.item_uid) || born.contains(row.root_item_uid) ||
			    born.contains(row.parent_item_uid))
				return flatfile_item_repository_result::invalid;
			if (!item_owner_identity_equal(row.owner, selected))
			{
				if ((row.owner.type == selected.type &&
				     row.owner.id == selected.id) ||
				    before_uids.contains(row.item_uid) ||
				    before_uids.contains(row.root_item_uid) ||
				    before_uids.contains(row.parent_item_uid))
					return flatfile_item_repository_result::invalid;
				continue;
			}
			if (row.state != item_custody_state::active)
				continue; // Preserve unrelated same-room inactive history.
			const auto found = std::find_if(
				world.room_before_items.begin(), world.room_before_items.end(),
				[&](const auto &item) { return item.object_uid == row.item_uid; });
			if (found == world.room_before_items.end() || row.vnum != found->vnum ||
			    !row.item_revision || row.equipment_slot)
				return flatfile_item_repository_result::invalid;
			const size_t index =
				static_cast<size_t>(found - world.room_before_items.begin());
			if (found->parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    (found->parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
			     static_cast<size_t>(found->parent_index) >= index))
				return flatfile_item_repository_result::invalid;
			const uint64_t parent =
				found->parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					0 :
					world.room_before_items[static_cast<size_t>(
									found->parent_index)]
						.object_uid;
			size_t root_index = index;
			for (size_t depth = 0; world.room_before_items[root_index].parent_index !=
					       PLAYER_SNAPSHOT_NO_PARENT;
			     ++depth)
			{
				const int32_t p = world.room_before_items[root_index].parent_index;
				if (depth >= world.room_before_items.size() || p < 0 ||
				    static_cast<size_t>(p) >= root_index)
					return flatfile_item_repository_result::invalid;
				root_index = static_cast<size_t>(p);
			}
			if (row.parent_item_uid != parent ||
			    row.root_item_uid != world.room_before_items[root_index].object_uid)
				return flatfile_item_repository_result::invalid;
			if (!row.coin_payload.empty())
			{
				auto literal = *found;
				literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				std::vector<uint8_t> canonical;
				if (literal.type != ITEM_MONEY ||
				    player_item_snapshot_list_encode({ literal }, &canonical) !=
					    player_snapshot_codec_result::ok ||
				    canonical != row.coin_payload)
					return flatfile_item_repository_result::invalid;
			}
			++observed;
		}
		if (observed != world.room_before_items.size())
			return flatfile_item_repository_result::invalid;
		for (const auto &operation : catalog.operations)
			if (operation.operation_id.bytes == original.command.operation_id.bytes ||
			    born.contains(operation.result.root_item_uid))
				return flatfile_item_repository_result::invalid;
		flatfile_initial_room_reset_custody_stage stage;
		if (zone_reset_item_accounting_compile(original.command, &stage.plan) !=
			    economic_accounting_error::ok ||
		    stage.plan.item_events.size() != image.items.size())
			return flatfile_item_repository_result::invalid;
		stage.catalog_before_present = loaded == flatfile_item_repository_result::ok;
		stage.catalog_revision_before = catalog.revision;
		stage.catalog_revision_after = catalog.revision + 1;
		stage.owner_before_present = before_owner != nullptr;
		stage.owner_revision_before = before_revision;
		stage.owner_revision_after = world.room_revision_after;
		auto *owner_after = ensure_owner(&catalog, selected);
		if (!owner_after)
			return flatfile_item_repository_result::io_error;
		owner_after->revision = stage.owner_revision_after;
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			const auto &event = stage.plan.item_events[index];
			if (event.uid != item.object_uid ||
			    event.before.state != item_custody_state::absent ||
			    event.after.state != item_custody_state::active ||
			    event.after.revision != 1 ||
			    !item_owner_identity_equal(event.after.owner, selected) ||
			    event.after.equipment_slot ||
			    event.after.root_uid != image.items.front().object_uid ||
			    event.after.parent_uid !=
				    (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					     0 :
					     image.items[static_cast<size_t>(item.parent_index)]
						     .object_uid))
				return flatfile_item_repository_result::invalid;
			flatfile_item_ownership_record row{ event.uid,
							    event.after.root_uid,
							    event.after.parent_uid,
							    selected,
							    1,
							    item.vnum,
							    item_custody_state::active,
							    {},
							    0 };
			if (item.type == ITEM_MONEY)
			{
				auto literal = item;
				literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				literal.equipment_slot = -1;
				if (player_item_snapshot_list_encode({ literal },
								     &row.coin_payload) !=
				    player_snapshot_codec_result::ok)
					return flatfile_item_repository_result::invalid;
			}
			catalog.items.push_back(std::move(row));
		}
		std::sort(catalog.items.begin(), catalog.items.end(), item_less);
		stage.result.root_item_uid = image.items.front().object_uid;
		stage.result.item_count = static_cast<uint16_t>(image.items.size());
		stage.result.to_owner_revision = stage.owner_revision_after;
		stage.result.max_item_revision = 1;
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> canonical_result{};
		if (!item_transfer_command_encode_result(stage.result, &canonical_result) ||
		    !valid_catalog(catalog))
			return flatfile_item_repository_result::invalid;
		stage.operation.store = flatfile_authority_store::domains;
		stage.operation.kind = flatfile_authority_operation_kind::write;
		stage.operation.filename = ownership_filename;
		if (!encode_catalog(catalog, stage.catalog_revision_after,
				    &stage.operation.bytes) ||
		    !lock.matches(root))
			return flatfile_item_repository_result::invalid;
		// The real ROOM root owns accounting/source/item references, every
		// actual pile head and the typed48 receipt in the same atomic bundle.
		// Do not fabricate a generic item-transfer operation history entry.
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_initial_room_reset_custody_stage>);
		*output = std::move(stage);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	catch (...)
	{
		return flatfile_item_repository_result::invalid;
	}
}

namespace
{
// Complete original item literal equality except the independently checked
// transport parent index and detached DURWRLD slot -1 versus command slot0.
bool room_reset_current_literal_equal(const player_item_snapshot &a,
				      const player_item_snapshot &b) noexcept
{
	if (std::tie(a.object_uid, a.generated_key, a.vnum, a.type, a.string_mask, a.name,
		     a.short_description, a.description, a.action_description, a.values, a.timers,
		     a.wear_flags, a.extra_flags, a.anti_flags, a.anti2_flags, a.extra2_flags,
		     a.weight, a.material, a.cost, a.condition, a.craftsmanship, a.bitvectors,
		     a.affects) !=
		    std::tie(b.object_uid, b.generated_key, b.vnum, b.type, b.string_mask, b.name,
			     b.short_description, b.description, b.action_description, b.values,
			     b.timers, b.wear_flags, b.extra_flags, b.anti_flags, b.anti2_flags,
			     b.extra2_flags, b.weight, b.material, b.cost, b.condition,
			     b.craftsmanship, b.bitvectors, b.affects) ||
	    a.dynamic_affects.size() != b.dynamic_affects.size() ||
	    a.extra_descriptions.size() != b.extra_descriptions.size())
		return false;
	for (size_t i = 0; i < a.dynamic_affects.size(); ++i)
	{
		const auto &x = a.dynamic_affects[i];
		const auto &y = b.dynamic_affects[i];
		if (std::tie(x.type, x.data, x.extra2) != std::tie(y.type, y.data, y.extra2))
			return false;
	}
	for (size_t i = 0; i < a.extra_descriptions.size(); ++i)
	{
		const auto &x = a.extra_descriptions[i];
		const auto &y = b.extra_descriptions[i];
		if (std::tie(x.keyword, x.description, x.spellbook, x.spell_ids) !=
		    std::tie(y.keyword, y.description, y.spellbook, y.spell_ids))
			return false;
	}
	return true;
}
} // namespace

flatfile_item_repository_result flatfile_room_reset_current_custody_storage::read_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, std::span<const uint8_t> stored_typed48,
	std::span<const uint8_t> stored_compiled_plan, const flatfile_room_item_record &room,
	std::vector<flatfile_item_ownership_record> *output) noexcept
{
	if (root.empty() || !output || !lock.matches(root) ||
	    stored_typed48.size() != ITEM_TRANSFER_RESULT_BYTES || stored_compiled_plan.empty())
		return flatfile_item_repository_result::invalid;
	try
	{
		zone_reset_item_image image;
		zone_reset_item_recovery_context context;
		if (!zone_reset_item_recovery_valid(original) ||
		    zone_reset_item_recovery_decode(original.command, original.attachment,
						    &context) != economic_accounting_error::ok ||
		    zone_reset_item_command_decode(original.command, &image) !=
			    economic_accounting_error::ok ||
		    !image.placement || image.placement->fall_selected ||
		    room.room_vnum != image.room_vnum ||
		    room.revision != image.expected_room_revision + 1 ||
		    room.items.size() < image.items.size() ||
		    room.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    std::any_of(room.money.begin(), room.money.end(),
				[](int32_t value) { return value < 0; }))
			return flatfile_item_repository_result::invalid;
		economic_accounting_plan plan;
		std::vector<uint8_t> canonical_plan;
		if (zone_reset_item_accounting_compile(original.command, &plan) !=
			    economic_accounting_error::ok ||
		    economic_plan_encode(plan, &canonical_plan) != economic_accounting_error::ok ||
		    canonical_plan.size() != stored_compiled_plan.size() ||
		    !std::equal(canonical_plan.begin(), canonical_plan.end(),
				stored_compiled_plan.begin()) ||
		    plan.item_events.size() != image.items.size())
			return flatfile_item_repository_result::invalid;
		item_transfer_result actual{}, expected{};
		expected.root_item_uid = image.items.front().object_uid;
		expected.item_count = static_cast<uint16_t>(image.items.size());
		expected.to_owner_revision = room.revision;
		expected.max_item_revision = 1;
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> canonical{}, expected_bytes{};
		if (!item_transfer_command_decode_result(stored_typed48.data(),
							 stored_typed48.size(), &actual) ||
		    !item_transfer_command_encode_result(actual, &canonical) ||
		    !item_transfer_command_encode_result(expected, &expected_bytes) ||
		    canonical != expected_bytes ||
		    !std::equal(canonical.begin(), canonical.end(), stored_typed48.begin()))
			return flatfile_item_repository_result::invalid;
		if (context.receipt_present)
		{
			const auto &r = context.receipt;
			if ((r.outcome != critical_apply_outcome::applied &&
			     r.outcome != critical_apply_outcome::already_applied) ||
			    r.operation_id.bytes != original.command.operation_id.bytes ||
			    r.durable_revision != room.revision || r.error_code ||
			    r.failure_stage != critical_failure_stage::none ||
			    r.disposition != critical_completion_disposition::execution ||
			    r.result_size != canonical.size() ||
			    !std::equal(canonical.begin(), canonical.end(),
					r.result_payload.begin()) ||
			    !std::all_of(r.result_payload.begin() + r.result_size,
					 r.result_payload.end(),
					 [](uint8_t value) { return !value; }))
				return flatfile_item_repository_result::invalid;
		}
		// Original world AFTER appends this complete forest, retaining detached -1
		// slots and shifting each non-root parent index by the BEFORE room length.
		std::vector<uint8_t> room_wire;
		if (player_item_snapshot_list_encode(room.items, &room_wire) !=
		    player_snapshot_codec_result::ok)
			return flatfile_item_repository_result::invalid;
		const size_t born_start = room.items.size() - image.items.size();
		for (size_t i = 0; i < room.items.size(); ++i)
		{
			const auto &item = room.items[i];
			if (!item.object_uid || item.equipment_slot != -1)
				return flatfile_item_repository_result::invalid;
			for (size_t prior = 0; prior < i; ++prior)
				if (room.items[prior].object_uid == item.object_uid)
					return flatfile_item_repository_result::invalid;
		}
		for (size_t i = 0; i < image.items.size(); ++i)
		{
			const auto &literal = image.items[i];
			const auto &observed = room.items[born_start + i];
			const int32_t parent =
				literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					PLAYER_SNAPSHOT_NO_PARENT :
					static_cast<int32_t>(born_start) + literal.parent_index;
			if (literal.type < ITEM_LOWEST || literal.type > ITEM_LAST ||
			    literal.type == ITEM_CORPSE || (literal.extra_flags & ITEM_ARTIFACT) ||
			    observed.parent_index != parent ||
			    !room_reset_current_literal_equal(literal, observed))
				return flatfile_item_repository_result::invalid;
		}
		ownership_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, nullptr);
		if (loaded != flatfile_item_repository_result::ok)
			return loaded;
		if (!valid_catalog(catalog))
			return flatfile_item_repository_result::invalid;
		const item_owner_identity selected{ item_owner_type::room,
						    static_cast<uint64_t>(image.room_vnum), 0 };
		const auto *owner = find_owner(&catalog, selected);
		if (!owner || owner->revision != room.revision)
			return flatfile_item_repository_result::invalid;
		for (const auto &entry : catalog.owners)
			if (entry.owner.type == selected.type && entry.owner.id == selected.id &&
			    entry.owner.context_id)
				return flatfile_item_repository_result::
					invalid; // Includes empty crossed-context owners.
		const auto born = [&](uint64_t uid) noexcept
		{
			return std::any_of(image.items.begin(), image.items.end(),
					   [&](const auto &literal)
					   { return literal.object_uid == uid; });
		};
		for (const auto &operation : catalog.operations)
			if (operation.operation_id.bytes == original.command.operation_id.bytes ||
			    born(operation.result.root_item_uid))
				return flatfile_item_repository_result::invalid;
		std::vector<flatfile_item_ownership_record> observed;
		observed.reserve(image.items.size());
		size_t selected_count = 0;
		for (const auto &row : catalog.items)
		{
			const bool born_row = born(row.item_uid);
			if ((!born_row && (born(row.root_item_uid) || born(row.parent_item_uid))) ||
			    (born_row && (!item_owner_identity_equal(row.owner, selected) ||
					  row.state != item_custody_state::active)))
				return flatfile_item_repository_result::invalid;
			if (!item_owner_identity_equal(row.owner, selected))
			{
				if (row.owner.type == selected.type && row.owner.id == selected.id)
					return flatfile_item_repository_result::invalid;
				continue;
			}
			if (row.state != item_custody_state::active)
				continue; // Preserve unrelated inactive history.
			const auto found = std::find_if(
				room.items.begin(), room.items.end(), [&](const auto &literal)
				{ return literal.object_uid == row.item_uid; });
			if (found == room.items.end() || row.vnum != found->vnum ||
			    !row.item_revision || row.equipment_slot)
				return flatfile_item_repository_result::invalid;
			const size_t index = static_cast<size_t>(found - room.items.begin());
			const uint64_t parent =
				found->parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					0 :
					room.items[static_cast<size_t>(found->parent_index)]
						.object_uid;
			size_t root_index = index;
			while (room.items[root_index].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				root_index =
					static_cast<size_t>(room.items[root_index].parent_index);
			if (row.parent_item_uid != parent ||
			    row.root_item_uid != room.items[root_index].object_uid)
				return flatfile_item_repository_result::invalid;
			if (!row.coin_payload.empty())
			{
				auto literal = *found;
				literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				std::vector<uint8_t> coin;
				if (literal.type != ITEM_MONEY ||
				    player_item_snapshot_list_encode({ literal }, &coin) !=
					    player_snapshot_codec_result::ok ||
				    coin != row.coin_payload)
					return flatfile_item_repository_result::invalid;
			}
			if (born_row)
			{
				const auto event = std::find_if(plan.item_events.begin(),
								plan.item_events.end(),
								[&](const auto &e)
								{ return e.uid == row.item_uid; });
				if (event == plan.item_events.end() ||
				    row.item_revision != event->after.revision ||
				    row.root_item_uid != event->after.root_uid ||
				    row.parent_item_uid != event->after.parent_uid ||
				    row.equipment_slot != event->after.equipment_slot ||
				    !item_owner_identity_equal(row.owner, event->after.owner) ||
				    row.state != event->after.state ||
				    (found->type == ITEM_MONEY && row.coin_payload.empty()))
					return flatfile_item_repository_result::invalid;
				observed.push_back(row);
			}
			++selected_count;
		}
		if (selected_count != room.items.size() || observed.size() != image.items.size() ||
		    !lock.matches(root))
			return flatfile_item_repository_result::invalid;
		output->swap(observed);
		return flatfile_item_repository_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_item_repository_result::io_error;
	}
	catch (...)
	{
		return flatfile_item_repository_result::invalid;
	}
}
