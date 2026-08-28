#include "flatfile_item_repository.h"

#include "flatfile_auction_repository.h"
#include "flatfile_boon_repository.h"
#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"
#include "flatfile_player_domain_repository.h"
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
constexpr uint32_t ownership_format_version = 1;
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
	    !header.number(&revision) || version != ownership_format_version || !revision ||
	    payload_size != bytes.size() - header_size)
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
		    !input.number(&entry.result.max_item_revision))
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
	critical_apply_result applied = { outcome,
					  std::max({ result.from_owner_revision,
						     result.to_owner_revision,
						     result.max_item_revision }),
					  error_code };
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
	*result = { payload.selected_item_uid, payload.item_count, from_owner->revision,
		    to_owner->revision, 0 };
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
	const unsigned int result_code = apply_transfer(&candidate, payload, &result);
	if (result_code == ENOMEM || result_code == EILSEQ)
		return { critical_apply_outcome::retryable_failure, catalog.revision, result_code };
	try
	{
		candidate.operations.push_back(
			{ command.operation_id, digest, result_code, result });
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, catalog.revision + 1, &encoded))
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	if (!flatfile_atomic_write(domains_directory(root), ownership_filename, encoded, &error))
		return { critical_apply_outcome::retryable_failure, catalog.revision, EIO };
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
	if (command.type == critical_command_type::epic ||
	    command.type == critical_command_type::account_bank ||
	    command.type == critical_command_type::combat_outcome)
		return flatfile_player_domain_apply(root, command);
	return { critical_apply_outcome::terminal_failure, 0, ENOTSUP };
}
