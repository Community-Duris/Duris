#include "flatfile/flatfile_item_repository.h"

#include "flatfile/flatfile_auction_repository.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_corpse_repository.h"
#include "flatfile/flatfile_boon_repository.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_collector_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_store.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_shop_trade_repository.h"
#include "persistence/persistence_mode.h"
#include "economy/coin_transfer_command.h"
#include "economy/item_transfer_accounting.h"
#include "economy/economic_accounting_intent.h"
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
		if (!entry.item_uid || !entry.root_item_uid || entry.vnum <= 0 ||
		    !item_owner_identity_valid(entry.owner) ||
		    entry.state == item_custody_state::absent ||
		    entry.state > item_custody_state::quarantined ||
		    (index && catalog.items[index - 1].item_uid == entry.item_uid) ||
		    !find_owner(const_cast<ownership_catalog *>(&catalog), entry.owner))
			return false;
		if (entry.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT ||
		    (entry.equipment_slot &&
		     (entry.owner.type != item_owner_type::player || entry.parent_item_uid ||
		      entry.state != item_custody_state::active)))
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
	const bool craft = !accounted && payload.reason == item_transfer_reason::craft;
	return craft || (generic_materialization_owner(payload.from_owner.type) &&
		generic_materialization_owner(payload.to_owner.type)) ||
	       mobile_claim || pet_give || pet_return || accounted_container_destruction ||
	       accounted_player_quest_turnin ||
	       locker_transfer(payload) ||
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
			return EMSGSIZE;
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

const flatfile_item_ownership_record *catalog_item(const ownership_catalog &catalog,
							  uint64_t uid)
{
	const auto found = std::lower_bound(catalog.items.begin(), catalog.items.end(), uid,
					     [](const auto &item, uint64_t key)
					     { return item.item_uid < key; });
	return found != catalog.items.end() && found->item_uid == uid ? &*found : nullptr;
}

bool build_item_accounting_references(
	const critical_command &command, const economic_accounting_plan &plan,
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
				const ownership_catalog &before,
				const ownership_catalog &after,
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
	const bool creation = payload.reason == item_transfer_reason::creation;
	if (creation)
	{
		plan.items_before.reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
			plan.items_before.push_back(
				{ payload.items[index].item_uid,
				  { { item_owner_type::unknown, 0, 0 }, 0, 0, 0,
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
				plan.items_before.push_back({ item.item_uid, accounting_position(item) });
		std::sort(plan.items_before.begin(), plan.items_before.end(),
			  [](const auto &left, const auto &right)
			  { return left.uid < right.uid; });
	}
	plan.items_after = plan.items_before;
	plan.item_events.reserve(payload.item_count);
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		auto prior = std::lower_bound(plan.items_before.begin(), plan.items_before.end(),
					      entry.item_uid,
					      [](const auto &item, uint64_t uid)
					      { return item.uid < uid; });
		auto current = std::lower_bound(plan.items_after.begin(), plan.items_after.end(),
						entry.item_uid,
						[](const auto &item, uint64_t uid)
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
		    after_position.state !=
			    (payload.to_owner.type == item_owner_type::destruction ?
				     item_custody_state::destroyed : item_custody_state::active))
			return false;
		current->position = after_position;
		plan.item_events.push_back({ static_cast<uint32_t>(index), 0, entry.item_uid,
					     before_position, after_position });
	}
	if (economic_plan_normalize(&plan) != economic_accounting_error::ok ||
	    economic_plan_validate_structure(plan) != economic_accounting_error::ok ||
	    economic_item_effects_validate(plan.items_before, plan.items_after,
					   plan.item_events, 0) !=
		economic_accounting_error::ok ||
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
		  actual.source_event->generation.bytes == expected.source_event->generation.bytes &&
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

bool verify_accounted_item_record(
	const critical_command &command, const item_transfer_payload &payload,
	const flatfile_accounting_record &record, unsigned int result_code,
	const item_transfer_result &result,
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
	    !plan.accounts.empty() || !plan.postings.empty() || !plan.children.empty() ||
	    plan.item_events.size() != payload.item_count ||
	    plan.item_events.size() != result.item_count ||
	    economic_item_effects_validate(plan.items_before, plan.items_after,
					   plan.item_events, 0) !=
		economic_accounting_error::ok)
		return false;
	economic_plan_metadata expected_metadata;
	if (economic_intent_plan_metadata(command, intent, &expected_metadata) !=
		    economic_accounting_error::ok ||
	    !plan_metadata_matches(plan.metadata, expected_metadata))
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
		    event.after.state !=
			    (payload.to_owner.type == item_owner_type::destruction ?
				     item_custody_state::destroyed : item_custody_state::active) ||
		    event.after.revision != event.before.revision + 1)
			return false;
		if (payload.reason == item_transfer_reason::creation)
		{
			if (event.before.owner.type != item_owner_type::unknown ||
			    event.before.state != item_custody_state::absent ||
			    event.before.revision || event.before.root_uid || event.before.parent_uid)
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
			source_roots.push_back(payload.items[index].root_item_uid);
		std::sort(source_roots.begin(), source_roots.end());
		source_roots.erase(std::unique(source_roots.begin(), source_roots.end()),
				   source_roots.end());
		for (auto &entry : catalog->items)
			if (entry.state == item_custody_state::active &&
			    item_owner_identity_equal(entry.owner, payload.from_owner) &&
			    std::binary_search(source_roots.begin(), source_roots.end(),
					       entry.root_item_uid))
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
		entry->owner = { item_owner_type::destruction, 0, 0 };
		entry->state = item_custody_state::destroyed;
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
	static flatfile_accounting_status stage(
		const std::string &root, const flatfile_authority_lock &lock,
		const flatfile_accounting_record &record,
		std::vector<flatfile_authority_operation> *operations, std::string *error)
	{
		return flatfile_accounting_storage::stage(root, lock, record, operations, error);
	}
	static flatfile_accounting_status stage_source_claim(
		const std::string &root, const flatfile_authority_lock &lock,
		const flatfile_accounting_record &record,
		std::vector<flatfile_authority_operation> *operations, std::string *error)
	{
		return flatfile_accounting_storage::stage_source_claim(root, lock, record,
								       operations, error);
	}
	static flatfile_accounting_status verify_source_claim(
		const std::string &root, const flatfile_authority_lock &lock,
		const flatfile_accounting_record &record, std::string *error)
	{
		return flatfile_accounting_storage::verify_source_claim(root, lock, record, error);
	}
	static flatfile_authority_transaction_result commit(
		const std::string &root, const flatfile_authority_lock &lock,
		const std::vector<flatfile_authority_operation> &operations, std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};

namespace
{
critical_apply_result item_accounting_failure(flatfile_accounting_status status,
						      uint64_t revision)
{
	if (status == flatfile_accounting_status::io_error ||
	    status == flatfile_accounting_status::capacity)
		return { critical_apply_outcome::retryable_failure, revision,
			 static_cast<unsigned int>(status == flatfile_accounting_status::io_error ?
							   EIO : ENOSPC) };
	return { critical_apply_outcome::terminal_failure, revision,
		 static_cast<unsigned int>(status == flatfile_accounting_status::already_exists ||
					   status == flatfile_accounting_status::conflict ?
					   EEXIST : EILSEQ) };
}

critical_apply_result item_reference_failure(flatfile_item_accounting_status status,
						      uint64_t revision)
{
	if (status == flatfile_item_accounting_status::io_error ||
	    status == flatfile_item_accounting_status::capacity)
		return { critical_apply_outcome::retryable_failure, revision,
			 static_cast<unsigned int>(status == flatfile_item_accounting_status::io_error ?
							   EIO : ENOSPC) };
	return { critical_apply_outcome::terminal_failure, revision,
		 static_cast<unsigned int>(status == flatfile_item_accounting_status::already_exists ?
						   EEXIST : EILSEQ) };
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

critical_apply_result flatfile_item_repository_apply(const std::string &root,
						     const critical_command &command)
{
	item_transfer_payload payload = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	const bool accounted =
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	if (root.empty() ||
	    (accounted ? !item_transfer_accounting_command_supported(command) :
			 !critical_command_valid(command)) ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0,
			 static_cast<unsigned int>(accounted ? ENOTSUP : EINVAL) };
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
		if (accounted)
		{
			flatfile_accounting_record retained;
			const auto status =
				flatfile_accounting_lookup(root, authority, command, &retained, &error);
			if (status != flatfile_accounting_status::ok)
				return item_accounting_failure(status, catalog.revision);
			std::vector<economic_accounting_item_reference> references;
			if (!verify_accounted_item_record(command, payload, retained,
							  entry.result_code, entry.result,
							  &references))
				return { critical_apply_outcome::terminal_failure,
					 catalog.revision, EILSEQ };
			const auto refs = flatfile_item_accounting_reference_verify_operation(
				root, command.operation_id, references, &error);
			if (refs != flatfile_item_accounting_status::ok)
				return item_reference_failure(refs, catalog.revision);
			const auto claim =
				flatfile_accounting_item_transfer_transaction::verify_source_claim(
					root, authority, retained, &error);
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
			return { critical_apply_outcome::terminal_failure, catalog.revision, EEXIST };
		if (status != flatfile_accounting_status::not_found)
			return item_accounting_failure(status, catalog.revision);
	}
	if (catalog.operations.size() >= ownership_maximum_operations ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
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
	unsigned int result_code = payload.reason == item_transfer_reason::craft ?
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
	    payload.item_blob_size)
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
		if (!result_code &&
		    !build_item_accounting_plan(command, payload, catalog, candidate, &record,
						&references))
			return { errno == ENOMEM ? critical_apply_outcome::retryable_failure :
						critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(errno == ENOMEM ? ENOMEM : EILSEQ) };
		std::vector<flatfile_authority_operation> operations;
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
			return { committed == flatfile_authority_transaction_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 committed == flatfile_authority_transaction_result::io_error ?
						 EIO :
						 EILSEQ) };
		flatfile_accounting_record retained;
		const auto looked_up =
			flatfile_accounting_lookup(root, authority, command, &retained, &error);
		if (looked_up != flatfile_accounting_status::ok)
			return item_accounting_failure(looked_up, catalog.revision);
		if (!verify_accounted_item_record(command, payload, retained, result_code,
						  result, &references))
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
		const auto committed =
			flatfile_authority_transaction_commit(root, authority, images, &error);
		if (committed != flatfile_authority_transaction_result::ok)
			return { committed == flatfile_authority_transaction_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 committed == flatfile_authority_transaction_result::io_error ?
						 EIO :
						 EILSEQ) };
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
