#include "flatfile_item_repository.h"

#include "flatfile_auction_repository.h"
#include "flatfile_artifact_repository.h"
#include "flatfile_corpse_repository.h"
#include "flatfile_boon_repository.h"
#include "flatfile_authority_transaction.h"
#include "flatfile_locker_repository.h"
#include "flatfile_store.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_shop_trade_materialization.h"
#include "flatfile_shop_trade_repository.h"
#include "persistence_mode.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace
{
constexpr uint32_t ownership_format_version = 2;
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
	       left.state == right.state;
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
	}
	std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, operation_id_hash>
		operation_ids;
	try
	{
		operation_ids.reserve(catalog.operations.size());
		for (const operation_state &entry : catalog.operations)
			if (critical_operation_id_is_zero(entry.operation_id) ||
			    !entry.result.root_item_uid || !entry.result.item_count ||
			    entry.result.item_count > ITEM_TRANSFER_MAX_ITEMS ||
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
	    (version != ownership_format_version && version != ownership_legacy_format_version) ||
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
		    (version >= ownership_format_version &&
		     !input.number(&entry.result.corpse_revision)))
			return flatfile_item_repository_result::invalid;
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
			     ((payload.reason == item_transfer_reason::player_drop &&
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

bool generic_transfer_supported(const item_transfer_payload &payload, uint16_t payload_version)
{
	return (generic_materialization_owner(payload.from_owner.type) &&
		generic_materialization_owner(payload.to_owner.type)) ||
	       locker_transfer(payload) || corpse_loot_transfer(payload) ||
	       (payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION && room_transfer(payload)) ||
	       (payload_version == ITEM_TRANSFER_PAYLOAD_VERSION &&
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
	*result = { payload.selected_item_uid,
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
		for (auto &entry : catalog->items)
			if (entry.root_item_uid == payload.items[0].root_item_uid)
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
		for (auto *entry : root_items)
			if (descendant_of(root_items, *entry, payload.selected_item_uid))
				selected.push_back(entry);
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
				catalog->items.push_back(
					{ entry.item_uid, payload.target_root_item_uid,
					  entry.item_uid == payload.selected_item_uid ?
						  payload.target_parent_item_uid :
						  entry.parent_item_uid,
					  payload.to_owner, 1, entry.vnum,
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
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			auto &entry = *selected[index];
			if (entry.item_revision == std::numeric_limits<uint64_t>::max())
				return ERANGE;
			++entry.item_revision;
			entry.root_item_uid = payload.target_root_item_uid;
			entry.parent_item_uid = entry.item_uid == payload.selected_item_uid ?
							payload.target_parent_item_uid :
							payload.items[index].parent_item_uid;
			entry.owner = payload.to_owner;
			entry.state = payload.to_owner.type == item_owner_type::destruction ?
					      item_custody_state::destroyed :
					      item_custody_state::active;
			result->max_item_revision =
				std::max(result->max_item_revision, entry.item_revision);
		}
	++from_owner->revision;
	if (!same_owner)
		++to_owner->revision;
	result->from_owner_revision = from_owner->revision;
	result->to_owner_revision = same_owner ? from_owner->revision : to_owner->revision;
	return 0;
}
} // namespace

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
	flatfile_item_auction_mutation *mutation, unsigned int *result_code, std::string *error)
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
			item_owner_identity{ item_owner_type::player,
					     static_cast<uint64_t>(payload.destination_player_pid),
					     0 };
	const item_owner_identity old_room_owner = { item_owner_type::room,
						     static_cast<uint64_t>(payload.old_room_vnum),
						     0 };
	owner_state *corpse = find_owner(&catalog, corpse_owner);
	owner_state *destination = find_owner(&catalog, destination_owner);
	owner_state *old_room = resurrect ? find_owner(&catalog, old_room_owner) : nullptr;
	if ((!corpse && !expected_items.empty()) ||
	    (destination &&
	     destination->revision != ((resurrect || raise_follower || nested_player) ?
					       payload.expected_player_revision :
					       payload.expected_room_revision)) ||
	    (!destination &&
	     ((resurrect || raise_follower || nested_player) ? payload.expected_player_revision :
							       payload.expected_room_revision)) ||
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
	mutation->player_owner_revision =
		resurrect || raise_follower || nested_player ? destination->revision : 0;
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
		mutation->max_item_revision =
			std::max(mutation->max_item_revision, item.item_revision);
	}
	mutation->after_image.filename = ownership_filename;
	if (!encode_catalog(catalog, catalog.revision + 1, &mutation->after_image.bytes))
		return flatfile_item_repository_result::invalid;
	return flatfile_item_repository_result::ok;
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

flatfile_item_repository_result flatfile_item_repository_prepare_player_and_custody_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	const std::vector<flatfile_corpse_custody_owner> &corpse_custody,
	flatfile_authority_operation *operation, std::string *error)
{
	if (!operation || !pid || !lock.matches(root) ||
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
		owners.reserve(locker_custody.size() + corpse_custody.size() + 1);
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

flatfile_item_repository_result flatfile_item_repository_prepare_player_and_locker_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::vector<flatfile_locker_custody_owner> &locker_custody,
	flatfile_authority_operation *operation, std::string *error)
{
	return flatfile_item_repository_prepare_player_and_custody_remove(
		root, lock, pid, locker_custody, {}, operation, error);
}

critical_apply_result flatfile_item_repository_apply(const std::string &root,
						     const critical_command &command)
{
	item_transfer_payload payload = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
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
			return make_result(entry.result_code ?
						   critical_apply_outcome::terminal_failure :
						   critical_apply_outcome::already_applied,
					   entry.result_code, entry.result);
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
	unsigned int result_code = apply_transfer(&candidate, payload, &result);
	if (result_code == ENOMEM || result_code == EILSEQ)
		return { critical_apply_outcome::retryable_failure, catalog.revision, result_code };
	if (!result_code && command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	    !generic_transfer_supported(payload, command.payload_version))
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
		result = { payload.selected_item_uid,
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
			result = { payload.selected_item_uid,
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
	try
	{
		candidate.operations.push_back(
			{ command.operation_id, digest, result_code, result });
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
	if (!result_code && command.payload_version == ITEM_TRANSFER_PAYLOAD_VERSION &&
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
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
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
	return make_result(result_code ? critical_apply_outcome::terminal_failure :
					 critical_apply_outcome::applied,
			   result_code, result);
}

critical_apply_result
flatfile_critical_command_repository_apply_selected(const critical_command &command, void *context)
{
	const char *root = context ? static_cast<const char *>(context) :
				     persistence_mode_flatfile_root();
	if (!root || !*root)
		return { critical_apply_outcome::terminal_failure, 0, ENOENT };
	if (command.type == critical_command_type::item_transfer)
		return flatfile_item_repository_apply(root, command);
	if (command.type == critical_command_type::auction)
		return flatfile_auction_repository_apply(root, command);
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
