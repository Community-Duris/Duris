#include "flatfile/flatfile_collector_repository.h"

#include "economy/collector_custody_boundary.h"
#include "economy/collector_eligibility.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_store.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'C', 'O', 'L', 'L', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t catalog_maximum_operations = 1048576;
constexpr const char *catalog_filename = "collector_catalog";

struct death_state
{
	collector::death_operation_id operation = {};
	uint32_t beneficiary = 0;
	uint64_t death_time = 0;
	collector::rules policy = {};
};

struct listing_state
{
	collector::record entry = {};
	std::vector<uint8_t> item_blob;
};

struct operation_state
{
	critical_operation_id operation_id = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest = {};
	unsigned int result_code = 0;
	collector_command_result result = {};
};

struct collector_catalog
{
	uint64_t file_revision = 0;
	uint64_t catalog_revision = 0;
	uint64_t next_listing = 1;
	std::vector<death_state> deaths;
	std::vector<listing_state> listings;
	std::vector<operation_state> operations;
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
		if (!valid || (!data && size) || bytes.size() > catalog_maximum_bytes ||
		    size > catalog_maximum_bytes - bytes.size())
		{
			valid = false;
			return;
		}
		if (!size)
			return;
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
		if (!value || offset > size || size - offset < sizeof(T))
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
		if ((!output && count) || offset > size || size - offset < count)
			return false;
		if (count)
			memcpy(output, data + offset, count);
		offset += count;
		return true;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

int hexadecimal(unsigned char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	return -1;
}

bool encode_operation_id(const collector::death_operation_id &operation,
			 encoder *output)
{
	if (!output || operation[collector::death_operation_hex_size] != '\0')
		return false;
	std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> bytes = {};
	for (size_t index = 0; index < bytes.size(); ++index)
	{
		const int high = hexadecimal(operation[index * 2]);
		const int low = hexadecimal(operation[index * 2 + 1]);
		if (high < 0 || low < 0)
			return false;
		bytes[index] = static_cast<uint8_t>((high << 4) | low);
	}
	output->raw(bytes.data(), bytes.size());
	return output->valid;
}

bool decode_operation_id(decoder *input, collector::death_operation_id *operation)
{
	if (!input || !operation)
		return false;
	std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> bytes = {};
	if (!input->raw(bytes.data(), bytes.size()))
		return false;
	constexpr char digits[] = "0123456789abcdef";
	for (size_t index = 0; index < bytes.size(); ++index)
	{
		(*operation)[index * 2] = digits[bytes[index] >> 4];
		(*operation)[index * 2 + 1] = digits[bytes[index] & 0xf];
	}
	(*operation)[collector::death_operation_hex_size] = '\0';
	return !std::all_of(bytes.begin(), bytes.end(), [](uint8_t byte) { return !byte; });
}

collector::death_operation_id operation_text(const critical_operation_id &operation)
{
	collector::death_operation_id result = {};
	constexpr char digits[] = "0123456789abcdef";
	for (size_t index = 0; index < operation.bytes.size(); ++index)
	{
		result[index * 2] = digits[operation.bytes[index] >> 4];
		result[index * 2 + 1] = digits[operation.bytes[index] & 0xf];
	}
	return result;
}

void encode_rules(encoder *output, const collector::rules &policy)
{
	output->number<uint8_t>(policy.enabled ? 1 : 0);
	output->number(policy.collection_delay);
	output->number(policy.sale_delay);
	output->number(policy.holding_duration);
	output->number(policy.price_percent);
	output->number(policy.minimum_value);
}

bool decode_rules(decoder *input, collector::rules *policy)
{
	uint8_t enabled = 0;
	if (!input || !policy || !input->number(&enabled) || enabled > 1 ||
	    !input->number(&policy->collection_delay) || !input->number(&policy->sale_delay) ||
	    !input->number(&policy->holding_duration) ||
	    !input->number(&policy->price_percent) || !input->number(&policy->minimum_value))
		return false;
	policy->enabled = enabled != 0;
	return collector::valid_rules(*policy);
}

bool same_rules(const collector::rules &left, const collector::rules &right)
{
	return left.enabled == right.enabled && left.collection_delay == right.collection_delay &&
	       left.sale_delay == right.sale_delay &&
	       left.holding_duration == right.holding_duration &&
	       left.price_percent == right.price_percent &&
	       left.minimum_value == right.minimum_value;
}

bool death_less(const death_state &left, const death_state &right)
{
	return std::lexicographical_compare(left.operation.begin(),
					    left.operation.begin() + collector::death_operation_hex_size,
					    right.operation.begin(),
					    right.operation.begin() + collector::death_operation_hex_size);
}

bool death_equal(const collector::death_operation_id &left,
		 const collector::death_operation_id &right)
{
	return std::equal(left.begin(), left.begin() + collector::death_operation_hex_size,
			  right.begin());
}

const death_state *find_death(const collector_catalog &catalog,
			      const collector::death_operation_id &operation)
{
	auto found = std::lower_bound(catalog.deaths.begin(), catalog.deaths.end(), operation,
				      [](const death_state &entry,
					 const collector::death_operation_id &candidate) {
					      return std::lexicographical_compare(
						      entry.operation.begin(),
						      entry.operation.begin() +
							      collector::death_operation_hex_size,
						      candidate.begin(),
						      candidate.begin() +
							      collector::death_operation_hex_size);
				      });
	return found != catalog.deaths.end() && death_equal(found->operation, operation) ?
		       &*found :
		       nullptr;
}

listing_state *find_listing(collector_catalog *catalog, uint64_t listing)
{
	if (!catalog)
		return nullptr;
	auto found = std::lower_bound(catalog->listings.begin(), catalog->listings.end(), listing,
				      [](const listing_state &entry, uint64_t candidate)
				      { return entry.entry.listing < candidate; });
	return found != catalog->listings.end() && found->entry.listing == listing ? &*found :
										       nullptr;
}

const listing_state *find_death_item(const collector_catalog &catalog,
				     const collector::death_operation_id &operation, uint64_t uid)
{
	auto found = std::find_if(catalog.listings.begin(), catalog.listings.end(),
				  [&](const listing_state &entry) {
					  return entry.entry.uid == uid &&
						 death_equal(entry.entry.death_operation, operation);
				  });
	return found == catalog.listings.end() ? nullptr : &*found;
}

bool valid_blob(const listing_state &listing)
{
	if (listing.item_blob.empty())
		return listing.entry.status == collector::state::candidate ||
		       (listing.entry.status == collector::state::cancelled &&
			listing.entry.closed_reason != collector::reason::holding_elapsed);
	if (listing.item_blob.size() > COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES)
		return false;
	std::vector<player_item_snapshot> items;
	return player_item_snapshot_list_decode(listing.item_blob.data(), listing.item_blob.size(),
						&items) == player_snapshot_codec_result::ok &&
	       items.size() == 1 && items[0].object_uid == listing.entry.uid &&
	       items[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
	       items[0].equipment_slot == 0;
}

bool valid_catalog(const collector_catalog &catalog)
{
	if (!catalog.next_listing || catalog.deaths.size() > collector::catalog_max_records ||
	    catalog.listings.size() > collector::catalog_max_records ||
	    catalog.operations.size() > catalog_maximum_operations ||
	    !std::is_sorted(catalog.deaths.begin(), catalog.deaths.end(), death_less) ||
	    !std::is_sorted(catalog.listings.begin(), catalog.listings.end(),
			    [](const listing_state &left, const listing_state &right) {
				    return left.entry.listing < right.entry.listing;
			    }))
		return false;
	for (size_t index = 0; index < catalog.deaths.size(); ++index)
	{
		const death_state &death = catalog.deaths[index];
		if (!death.beneficiary || !death.death_time || !death.policy.enabled ||
		    !collector::valid_rules(death.policy) ||
		    (index && death_equal(catalog.deaths[index - 1].operation, death.operation)))
			return false;
	}
	std::set<std::pair<uint32_t, uint64_t>> death_identities;
	for (const death_state &death : catalog.deaths)
		if (!death_identities.emplace(death.beneficiary, death.death_time).second)
			return false;
	std::set<std::pair<collector::death_operation_id, uint64_t>> death_items;
	try
	{
		for (size_t index = 0; index < catalog.listings.size(); ++index)
		{
			const listing_state &listing = catalog.listings[index];
			const death_state *death = find_death(catalog, listing.entry.death_operation);
			if (!collector::valid_record(listing.entry) || !valid_blob(listing) ||
			    listing.entry.listing >= catalog.next_listing ||
			    (index && catalog.listings[index - 1].entry.listing ==
					      listing.entry.listing) ||
			    !death_items.emplace(listing.entry.death_operation,
						 listing.entry.uid).second ||
			    !death ||
			    death->beneficiary != listing.entry.beneficiary ||
			    death->death_time != listing.entry.death_time ||
			    !same_rules(death->policy, listing.entry.policy))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, operation_id_hash>
		operation_ids;
	try
	{
		operation_ids.reserve(catalog.operations.size());
		for (const operation_state &operation : catalog.operations)
		{
			std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded = {};
			if (critical_operation_id_is_zero(operation.operation_id) ||
			    !collector_command_encode_result(operation.result, &encoded) ||
			    !operation_ids.insert(operation.operation_id.bytes).second)
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_catalog(const collector_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.file_revision || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number(catalog.catalog_revision);
	payload.number(catalog.next_listing);
	payload.number<uint32_t>(catalog.deaths.size());
	for (const death_state &death : catalog.deaths)
	{
		if (!encode_operation_id(death.operation, &payload))
			return false;
		payload.number(death.beneficiary);
		payload.number(death.death_time);
		encode_rules(&payload, death.policy);
	}
	payload.number<uint32_t>(catalog.listings.size());
	for (const listing_state &listing : catalog.listings)
	{
		std::array<uint8_t, collector::encoded_record_bytes> record = {};
		if (collector::record_encode(listing.entry, &record) != collector::codec_result::ok)
			return false;
		payload.raw(record.data(), record.size());
		payload.number<uint32_t>(listing.item_blob.size());
		payload.raw(listing.item_blob.data(), listing.item_blob.size());
	}
	payload.number<uint32_t>(catalog.operations.size());
	for (const operation_state &operation : catalog.operations)
	{
		payload.raw(operation.operation_id.bytes.data(), operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number<uint32_t>(operation.result_code);
		std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> result = {};
		if (!collector_command_encode_result(operation.result, &result))
			return false;
		payload.raw(result.data(), result.size());
	}
	if (!payload.valid || payload.bytes.size() > catalog_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(catalog_magic.data(), catalog_magic.size());
	file.number(catalog_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.file_revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > catalog_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, collector_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size || bytes.size() > catalog_maximum_bytes ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + catalog_magic.size(),
			bytes.size() - catalog_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t file_revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&file_revision) || version != catalog_version || !file_revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	collector_catalog decoded;
	decoded.file_revision = file_revision;
	uint32_t death_count = 0, listing_count = 0, operation_count = 0;
	if (!payload.number(&decoded.catalog_revision) || !payload.number(&decoded.next_listing) ||
	    !payload.number(&death_count) || death_count > collector::catalog_max_records)
		return false;
	try
	{
		decoded.deaths.resize(death_count);
		for (death_state &death : decoded.deaths)
			if (!decode_operation_id(&payload, &death.operation) ||
			    !payload.number(&death.beneficiary) ||
			    !payload.number(&death.death_time) || !decode_rules(&payload, &death.policy))
				return false;
		if (!payload.number(&listing_count) || listing_count > collector::catalog_max_records)
			return false;
		decoded.listings.resize(listing_count);
		for (listing_state &listing : decoded.listings)
		{
			std::array<uint8_t, collector::encoded_record_bytes> record = {};
			uint32_t blob_size = 0;
			if (!payload.raw(record.data(), record.size()) ||
			    collector::record_decode(record.data(), record.size(), &listing.entry) !=
				    collector::codec_result::ok ||
			    !payload.number(&blob_size) ||
			    blob_size > COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES ||
			    payload.offset > payload.size || payload.size - payload.offset < blob_size)
				return false;
			listing.item_blob.resize(blob_size);
			if (!payload.raw(listing.item_blob.data(), listing.item_blob.size()))
				return false;
		}
		if (!payload.number(&operation_count) ||
		    operation_count > catalog_maximum_operations)
			return false;
		decoded.operations.resize(operation_count);
		for (operation_state &operation : decoded.operations)
		{
			std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> result = {};
			if (!payload.raw(operation.operation_id.bytes.data(),
					 operation.operation_id.bytes.size()) ||
			    !payload.raw(operation.command_digest.data(),
					 operation.command_digest.size()) ||
			    !payload.number(&operation.result_code) ||
			    !payload.raw(result.data(), result.size()) ||
			    !collector_command_decode_result(result.data(), result.size(),
						     &operation.result))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_catalog(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_collector_repository_result load_catalog(const std::string &root,
						  collector_catalog *catalog,
						  std::string *error)
{
	if (!catalog)
		return flatfile_collector_repository_result::invalid;
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					 catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
	{
		*catalog = {};
		return flatfile_collector_repository_result::not_found;
	}
	if (loaded == flatfile_read_result::io_error)
		return flatfile_collector_repository_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "collector catalog is corrupt";
		return flatfile_collector_repository_result::invalid;
	}
	return flatfile_collector_repository_result::ok;
}

flatfile_collector_repository_result recover(const std::string &root,
					      const flatfile_authority_lock &lock,
					      std::string *error)
{
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered == flatfile_authority_transaction_result::ok)
		return flatfile_collector_repository_result::ok;
	return recovered == flatfile_authority_transaction_result::io_error ?
		       flatfile_collector_repository_result::io_error :
		       flatfile_collector_repository_result::invalid;
}

collector::rules enrollment_rules(const item_collector_death_policy &source)
{
	return { true, source.collection_delay, source.sale_delay, source.holding_duration,
		 source.price_percent, source.minimum_value };
}

unsigned int policy_code(collector::outcome outcome)
{
	switch (outcome)
	{
	case collector::outcome::applied:
		return 0;
	case collector::outcome::not_due:
		return EAGAIN;
	case collector::outcome::conflict:
		return ESTALE;
	case collector::outcome::invalid:
		return EINVAL;
	case collector::outcome::overflow:
		return ERANGE;
	case collector::outcome::forbidden:
		return EACCES;
	case collector::outcome::insufficient_funds:
		return ENOSPC;
	case collector::outcome::capacity:
		return ENOBUFS;
	}
	return EINVAL;
}
} // namespace

flatfile_collector_repository_result flatfile_collector_repository_read_bootstrap(
	const std::string &root, collector_bootstrap_snapshot *snapshot, std::string *error)
{
	if (root.empty() || !snapshot)
		return flatfile_collector_repository_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_collector_repository_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_collector_repository_result::ok)
		return recovered;
	collector_catalog stored;
	const auto loaded = load_catalog(root, &stored, error);
	if (loaded != flatfile_collector_repository_result::ok &&
	    loaded != flatfile_collector_repository_result::not_found)
		return loaded;
	collector_bootstrap_snapshot candidate;
	candidate.catalog.revision = stored.catalog_revision;
	candidate.catalog.next_listing = stored.next_listing;
	try
	{
		candidate.deaths.reserve(stored.deaths.size());
		for (const death_state &death : stored.deaths)
		{
			collector_death_snapshot snapshot_death;
			if (!critical_operation_id_from_hex(death.operation.data(),
						   &snapshot_death.operation_id))
				return flatfile_collector_repository_result::invalid;
			snapshot_death.beneficiary_pid = death.beneficiary;
			snapshot_death.death_time = death.death_time;
			snapshot_death.policy = death.policy;
			candidate.deaths.push_back(snapshot_death);
		}
		candidate.catalog.records.reserve(stored.listings.size());
		for (const listing_state &listing : stored.listings)
			candidate.catalog.records.push_back(listing.entry);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_collector_repository_result::io_error;
	}
	if (!collector::valid_catalog(candidate.catalog))
		return flatfile_collector_repository_result::invalid;
	const auto held = flatfile_item_repository_list_collector_items_locked(
		root, lock, &candidate.held_items, error);
	if (held != flatfile_item_repository_result::ok &&
	    held != flatfile_item_repository_result::not_found)
		return held == flatfile_item_repository_result::io_error ?
			       flatfile_collector_repository_result::io_error :
			       flatfile_collector_repository_result::invalid;
	for (const listing_state &listing : stored.listings)
	{
		const bool should_be_held = listing.entry.status == collector::state::collected ||
					    listing.entry.status == collector::state::available;
		auto item = std::find_if(candidate.held_items.begin(), candidate.held_items.end(),
					 [&](const item_ownership_runtime_entry &entry) {
						 return entry.item_uid == listing.entry.uid;
					 });
		if (should_be_held != (item != candidate.held_items.end()))
			return flatfile_collector_repository_result::invalid;
		if (item != candidate.held_items.end() &&
		    (item->root_item_uid != listing.entry.uid || item->parent_item_uid ||
		     item->owner.type != item_owner_type::collector ||
		     item->owner.id != item_collector_owner_id(listing.entry.listing) ||
		     item->owner.context_id || item->item_revision != listing.entry.item_revision ||
		     item->vnum <= 0 || item->state != item_custody_state::active ||
		     !item->owner_revision))
			return flatfile_collector_repository_result::invalid;
	}
	if (candidate.held_items.size() !=
	    static_cast<size_t>(std::count_if(stored.listings.begin(), stored.listings.end(),
					      [](const listing_state &listing) {
						      return listing.entry.status ==
								     collector::state::collected ||
							     listing.entry.status ==
								     collector::state::available;
					      })))
		return flatfile_collector_repository_result::invalid;
	*snapshot = std::move(candidate);
	return flatfile_collector_repository_result::ok;
}

flatfile_collector_repository_result flatfile_collector_repository_read_listing(
	const std::string &root, uint64_t listing, collector_listing_detail *detail, bool *found,
	std::string *error)
{
	if (root.empty() || !listing || !detail || !found)
		return flatfile_collector_repository_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_collector_repository_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_collector_repository_result::ok)
		return recovered;
	collector_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_collector_repository_result::not_found)
	{
		*found = false;
		return flatfile_collector_repository_result::ok;
	}
	if (loaded != flatfile_collector_repository_result::ok)
		return loaded;
	const listing_state *stored = find_listing(&catalog, listing);
	if (!stored)
	{
		*found = false;
		return flatfile_collector_repository_result::ok;
	}
	try
	{
		detail->entry = stored->entry;
		detail->item_blob = stored->item_blob;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_collector_repository_result::io_error;
	}
	*found = true;
	return flatfile_collector_repository_result::ok;
}

flatfile_collector_repository_result flatfile_collector_prepare_death_enrollment(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, const item_transfer_result &transfer,
	flatfile_collector_enrollment_mutation *mutation, unsigned int *result_code,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !result_code)
		return flatfile_collector_repository_result::invalid;
	*mutation = {};
	*result_code = 0;
	if (!payload.collector.present)
		return flatfile_collector_repository_result::unchanged;
	if (payload.reason != item_transfer_reason::corpse_create ||
	    payload.to_owner.type != item_owner_type::corpse ||
	    transfer.item_count != payload.item_count ||
	    transfer.root_item_uid != item_transfer_result_root(payload) ||
	    critical_operation_id_is_zero(payload.collector.death_operation))
	{
		*result_code = EBADMSG;
		return flatfile_collector_repository_result::ok;
	}
	std::vector<player_item_snapshot> snapshots;
	std::vector<uint64_t> eligible;
	try
	{
		if (!payload.item_blob_size ||
		    player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
						     &snapshots) !=
			    player_snapshot_codec_result::ok ||
		    snapshots.size() != payload.item_count)
		{
			*result_code = EBADMSG;
			return flatfile_collector_repository_result::ok;
		}
		eligible.reserve(snapshots.size());
		for (const player_item_snapshot &snapshot : snapshots)
			if (collector_death_item_snapshot_eligible(snapshot))
				eligible.push_back(snapshot.object_uid);
		std::sort(eligible.begin(), eligible.end());
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_collector_repository_result::io_error;
	}
	if (eligible != payload.collector.eligible_item_uids)
	{
		*result_code = EBADMSG;
		return flatfile_collector_repository_result::ok;
	}
	const collector::rules policy = enrollment_rules(payload.collector.policy);
	if (!collector::valid_rules(policy))
	{
		*result_code = EINVAL;
		return flatfile_collector_repository_result::ok;
	}
	collector_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_collector_repository_result::ok &&
	    loaded != flatfile_collector_repository_result::not_found)
		return loaded;
	const collector::death_operation_id operation =
		operation_text(payload.collector.death_operation);
	const death_state *existing_death = find_death(catalog, operation);
	if (existing_death &&
	    (existing_death->beneficiary != payload.collector.beneficiary_pid ||
	     existing_death->death_time != payload.collector.death_time ||
	     !same_rules(existing_death->policy, policy)))
	{
		*result_code = EEXIST;
		return flatfile_collector_repository_result::ok;
	}
	std::vector<const item_transfer_entry *> additions;
	try
	{
		additions.reserve(eligible.size());
		for (uint64_t uid : eligible)
		{
			auto expected = std::lower_bound(
				payload.items.begin(), payload.items.begin() + payload.item_count, uid,
				[](const item_transfer_entry &entry, uint64_t candidate)
				{ return entry.item_uid < candidate; });
			if (expected == payload.items.begin() + payload.item_count ||
			    expected->item_uid != uid || expected->expected_state !=
								item_custody_state::active ||
			    expected->expected_item_revision == UINT64_MAX)
			{
				*result_code = EBADMSG;
				return flatfile_collector_repository_result::ok;
			}
			if (find_death_item(catalog, operation, uid))
			{
				continue;
			}
			additions.push_back(&*expected);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_collector_repository_result::io_error;
	}
	if (additions.empty() && existing_death)
	{
		mutation->catalog_revision = catalog.catalog_revision;
		return flatfile_collector_repository_result::unchanged;
	}
	if (!payload.collector.beneficiary_pid || !payload.collector.death_time ||
	    (!existing_death && catalog.deaths.size() >= collector::catalog_max_records) ||
	    catalog.listings.size() > collector::catalog_max_records - additions.size() ||
	    catalog.next_listing > UINT64_MAX - additions.size() ||
	    catalog.file_revision == UINT64_MAX || catalog.catalog_revision == UINT64_MAX)
	{
		*result_code = ENOBUFS;
		return flatfile_collector_repository_result::ok;
	}
	try
	{
		if (!existing_death)
		{
			catalog.deaths.push_back({ operation, payload.collector.beneficiary_pid,
						   payload.collector.death_time, policy });
			std::sort(catalog.deaths.begin(), catalog.deaths.end(), death_less);
		}
		for (const item_transfer_entry *item : additions)
		{
			collector::record entry;
			const collector::outcome enrolled = collector::enroll(
				catalog.next_listing, operation.data(),
				payload.collector.beneficiary_pid, item->item_uid,
				item->expected_item_revision + 1, payload.collector.death_time,
				policy, &entry);
			if (enrolled != collector::outcome::applied)
			{
				*result_code = policy_code(enrolled);
				return flatfile_collector_repository_result::ok;
			}
			catalog.listings.push_back({ entry, {} });
			++catalog.next_listing;
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_collector_repository_result::io_error;
	}
	if (!additions.empty())
		++catalog.catalog_revision;
	catalog.file_revision = catalog.file_revision ? catalog.file_revision + 1 : 1;
	mutation->after_image.filename = catalog_filename;
	if (!encode_catalog(catalog, &mutation->after_image.bytes))
		return flatfile_collector_repository_result::invalid;
	mutation->catalog_revision = catalog.catalog_revision;
	mutation->enrolled = additions.size();
	return flatfile_collector_repository_result::ok;
}

flatfile_collector_repository_result flatfile_collector_prepare_item_boundary(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, const item_transfer_result &transfer,
	flatfile_collector_enrollment_mutation *mutation, unsigned int *result_code,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !result_code ||
	    transfer.item_count != payload.item_count ||
	    transfer.root_item_uid != item_transfer_result_root(payload))
		return flatfile_collector_repository_result::invalid;
	*mutation = {};
	*result_code = 0;

	flatfile_collector_enrollment_mutation enrollment;
	flatfile_collector_repository_result enrollment_result =
		flatfile_collector_repository_result::unchanged;
	if (payload.collector.present)
	{
		enrollment_result = flatfile_collector_prepare_death_enrollment(
			root, lock, payload, transfer, &enrollment, result_code, error);
		if (enrollment_result != flatfile_collector_repository_result::ok &&
		    enrollment_result != flatfile_collector_repository_result::unchanged)
			return enrollment_result;
		if (*result_code)
			return flatfile_collector_repository_result::ok;
	}

	const collector::reason reason = collector_item_transfer_boundary_reason(payload);
	if (reason == collector::reason::none)
	{
		*mutation = std::move(enrollment);
		return enrollment_result;
	}

	collector_catalog catalog;
	if (!enrollment.after_image.bytes.empty())
	{
		if (!decode_catalog(enrollment.after_image.bytes, &catalog))
			return flatfile_collector_repository_result::invalid;
	}
	else
	{
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded == flatfile_collector_repository_result::not_found)
		{
			*mutation = std::move(enrollment);
			return enrollment_result;
		}
		if (loaded != flatfile_collector_repository_result::ok)
			return loaded;
	}

	size_t cancelled_count = 0;
	for (listing_state &listing : catalog.listings)
	{
		if (listing.entry.status != collector::state::candidate)
			continue;
		const auto item = std::lower_bound(
			payload.items.begin(), payload.items.begin() + payload.item_count,
			listing.entry.uid,
			[](const item_transfer_entry &entry, uint64_t uid)
			{ return entry.item_uid < uid; });
		if (item == payload.items.begin() + payload.item_count ||
		    item->item_uid != listing.entry.uid)
			continue;
		if (payload.collector.present)
		{
			const auto current_death = operation_text(payload.collector.death_operation);
			if (death_equal(listing.entry.death_operation, current_death))
				continue;
		}
		uint64_t post_item_revision = 0;
		if (item->expected_state == item_custody_state::absent)
		{
			if (item->expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION)
			{
				*result_code = EBADMSG;
				return flatfile_collector_repository_result::ok;
			}
			post_item_revision = 1;
		}
		else
		{
			if (item->expected_item_revision == UINT64_MAX)
			{
				*result_code = ERANGE;
				return flatfile_collector_repository_result::ok;
			}
			post_item_revision = item->expected_item_revision + 1;
		}
		if (post_item_revision < listing.entry.item_revision)
		{
			*result_code = ESTALE;
			return flatfile_collector_repository_result::ok;
		}
		const collector::outcome outcome =
			collector::cancel(&listing.entry, listing.entry.revision, reason);
		if (outcome != collector::outcome::applied)
		{
			*result_code = policy_code(outcome);
			return flatfile_collector_repository_result::ok;
		}
		listing.entry.item_revision = post_item_revision;
		++cancelled_count;
	}
	if (!cancelled_count)
	{
		*mutation = std::move(enrollment);
		return enrollment_result;
	}
	if (catalog.catalog_revision == UINT64_MAX || catalog.file_revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return flatfile_collector_repository_result::ok;
	}
	++catalog.catalog_revision;
	++catalog.file_revision;
	mutation->after_image.filename = catalog_filename;
	if (!encode_catalog(catalog, &mutation->after_image.bytes))
		return flatfile_collector_repository_result::invalid;
	mutation->catalog_revision = catalog.catalog_revision;
	mutation->enrolled = enrollment.enrolled;
	mutation->cancelled = cancelled_count;
	return flatfile_collector_repository_result::ok;
}

namespace
{
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

int64_t wallet_value(const currency_vector &wallet)
{
	constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> coin_values = { 1, 10, 100,
									    1000 };
	int64_t value = 0;
	for (size_t index = 0; index < wallet.amount.size(); ++index)
	{
		if (wallet.amount[index] < 0 ||
		    wallet.amount[index] > (INT64_MAX - value) / coin_values[index])
			return -1;
		value += wallet.amount[index] * coin_values[index];
	}
	return value;
}

uint64_t durable_revision(const collector_command_result &result, uint64_t catalog_revision)
{
	return std::max({ catalog_revision, result.catalog_revision,
			  result.from_owner_revision, result.to_owner_revision,
			  result.wallet_revision, result.bank_revision,
			  result.record_present ? result.entry.revision : 0,
			  result.record_present ? result.entry.item_revision : 0 });
}

critical_apply_result make_result(const operation_state &operation, uint64_t catalog_revision,
				  critical_apply_outcome success)
{
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded = {};
	if (!collector_command_encode_result(operation.result, &encoded))
		return { critical_apply_outcome::terminal_failure, catalog_revision, EILSEQ };
	critical_apply_result result = {
		operation.result_code ? critical_apply_outcome::terminal_failure : success,
		durable_revision(operation.result, catalog_revision), operation.result_code
	};
	result.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), result.result_payload.begin());
	return result;
}

bool decode_singleton(const collector_command_payload &payload, player_item_snapshot *item,
		      unsigned int *result_code)
{
	if (!item || !result_code)
		return false;
	std::vector<player_item_snapshot> decoded;
	const auto parsed = player_item_snapshot_list_decode(payload.item_blob.data(),
						    payload.item_blob_size, &decoded);
	if (parsed == player_snapshot_codec_result::allocation_failure)
	{
		errno = ENOMEM;
		return false;
	}
	if (parsed != player_snapshot_codec_result::ok || decoded.size() != 1 ||
	    decoded[0].object_uid != payload.selected_item_uid ||
	    decoded[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    decoded[0].equipment_slot != 0)
	{
		*result_code = EBADMSG;
		return true;
	}
	*item = std::move(decoded[0]);
	return true;
}

item_transfer_payload materialization_payload(const collector_command_payload &payload)
{
	item_transfer_payload result = {};
	result.reason = payload.action == collector_action::purchase ?
				item_transfer_reason::collector_buyback :
				item_transfer_reason::collector_expire;
	result.from_owner = payload.from_owner;
	result.to_owner = payload.to_owner;
	result.expected_from_revision = payload.expected_from_owner_revision;
	result.expected_to_revision = payload.expected_to_owner_revision;
	result.selected_item_uid = payload.selected_item_uid;
	result.target_root_item_uid = payload.selected_item_uid;
	result.item_count = payload.item_count;
	std::copy(payload.items.begin(), payload.items.begin() + payload.item_count,
		  result.items.begin());
	result.item_blob_size = payload.item_blob_size;
	std::copy(payload.item_blob.begin(), payload.item_blob.begin() + payload.item_blob_size,
		  result.item_blob.begin());
	return result;
}

flatfile_collector_repository_result item_prepare_failure(
	flatfile_item_repository_result result)
{
	return result == flatfile_item_repository_result::io_error ?
		       flatfile_collector_repository_result::io_error :
		       flatfile_collector_repository_result::invalid;
}

flatfile_collector_repository_result world_prepare_failure(flatfile_world_item_result result)
{
	return result == flatfile_world_item_result::io_error ?
		       flatfile_collector_repository_result::io_error :
		       flatfile_collector_repository_result::invalid;
}
} // namespace

critical_apply_result flatfile_collector_repository_apply(const std::string &root,
							   const critical_command &command)
{
	collector_command_payload payload = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !collector_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	flatfile_authority_lock lock;
	std::string error;
	if (!lock.acquire(root, &error))
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	const auto recovered = recover(root, lock, &error);
	if (recovered != flatfile_collector_repository_result::ok)
		return { recovered == flatfile_collector_repository_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 recovered == flatfile_collector_repository_result::io_error ? EIO :
										       EILSEQ) };
	collector_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_collector_repository_result::ok &&
	    loaded != flatfile_collector_repository_result::not_found)
		return { loaded == flatfile_collector_repository_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 loaded == flatfile_collector_repository_result::io_error ? EIO :
										    EILSEQ) };
	for (const operation_state &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure,
					 catalog.catalog_revision, EEXIST };
			return make_result(operation, catalog.catalog_revision,
					   critical_apply_outcome::already_applied);
		}
	if (catalog.operations.size() >= catalog_maximum_operations ||
	    catalog.file_revision == UINT64_MAX)
		return { critical_apply_outcome::terminal_failure, catalog.catalog_revision,
			 ENOSPC };

	collector_catalog candidate;
	try
	{
		candidate = catalog;
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.catalog_revision,
			 ENOMEM };
	}
	collector_command_result result = {};
	result.action = payload.action;
	unsigned int result_code = 0;
	listing_state *listing = find_listing(&candidate, payload.listing);
	if (!listing)
		result_code = ENOENT;
	else if (listing->entry.revision != payload.expected_listing_revision)
		result_code = ESTALE;

	flatfile_wallet_mutation wallet_read;
	if (!result_code && payload.action == collector_action::purchase)
	{
		const auto prepared = flatfile_player_domain_prepare_wallet(
			root, lock, payload.actor_pid, payload.account_name.data(), payload.racewar,
			payload.expected_wallet_revision, payload.expected_bank_revision, 0, false,
			&wallet_read, &result_code, &error);
		if (prepared != flatfile_player_domain_result::ok)
		{
			if (prepared == flatfile_player_domain_result::not_found)
				result_code = ENOENT;
			else
				return { prepared == flatfile_player_domain_result::io_error ?
						 critical_apply_outcome::retryable_failure :
						 critical_apply_outcome::terminal_failure,
					 catalog.catalog_revision,
					 static_cast<unsigned int>(
						 prepared == flatfile_player_domain_result::io_error ?
							 EIO :
							 EILSEQ) };
		}
		result.wallet = wallet_read.wallet;
		result.bank = wallet_read.bank;
		result.wallet_revision = wallet_read.wallet_revision;
		result.bank_revision = wallet_read.bank_revision;
	}

	player_item_snapshot exact;
	if (!result_code && payload.item_count && !decode_singleton(payload, &exact, &result_code))
		return { critical_apply_outcome::retryable_failure, catalog.catalog_revision,
			 static_cast<unsigned int>(errno == ENOMEM ? ENOMEM : EIO) };

	collector::record updated = listing ? listing->entry : collector::record{};
	collector::outcome policy = collector::outcome::invalid;
	if (!result_code)
	{
		if (payload.action == collector_action::collect)
		{
			auto selected = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const item_transfer_entry &item)
				{ return item.item_uid == payload.selected_item_uid; });
			if (selected == payload.items.begin() + payload.item_count)
				result_code = ESTALE;
			else
				policy = collector::collect(
					&updated, payload.expected_listing_revision,
					selected->expected_item_revision,
					selected->expected_item_revision, true, exact.cost,
					payload.observed_at);
		}
		else if (payload.action == collector_action::activate)
			policy = collector::activate(&updated, payload.expected_listing_revision,
						     payload.observed_at);
		else if (payload.action == collector_action::purchase)
		{
			const int64_t carried = wallet_value(wallet_read.wallet);
			if (carried < 0)
				result_code = ERANGE;
			else
				policy = collector::purchase(
					&updated, payload.expected_listing_revision, payload.actor_pid,
					static_cast<uint64_t>(carried), payload.capacity_admitted,
					payload.observed_at);
		}
		else if (payload.action == collector_action::expire)
			policy = collector::expire(&updated, payload.expected_listing_revision,
						   payload.observed_at);
		else if (payload.action == collector_action::cancel)
			policy = collector::cancel(&updated, payload.expected_listing_revision,
						   payload.cancel_reason);
		else if (payload.action == collector_action::pause)
			policy = collector::pause(&updated, payload.expected_listing_revision,
						  payload.observed_at);
		else if (payload.action == collector_action::resume)
			policy = collector::resume(&updated, payload.expected_listing_revision,
						   payload.observed_at);
		if (!result_code)
			result_code = policy_code(policy);
	}

	flatfile_item_collector_mutation item;
	flatfile_collector_world_mutation world;
	flatfile_wallet_mutation wallet_apply;
	flatfile_shop_trade_materialization_mutation materialization;
	bool include_item = false, include_world = false, include_wallet = false,
	     include_materialization = false;
	if (!result_code && payload.item_count)
	{
		const auto prepared = flatfile_item_repository_prepare_collector_transfer(
			root, lock, payload, &item, &result_code, &error);
		if (prepared != flatfile_item_repository_result::ok)
		{
			const auto failure = item_prepare_failure(prepared);
			return { failure == flatfile_collector_repository_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.catalog_revision,
				 static_cast<unsigned int>(
					 failure == flatfile_collector_repository_result::io_error ?
						 EIO :
						 EILSEQ) };
		}
		include_item = !result_code && !item.after_image.bytes.empty();
		if (!result_code && item.item_revision != updated.item_revision)
			result_code = ESTALE;
	}
	if (!result_code && payload.action == collector_action::collect)
	{
		const auto prepared = flatfile_world_item_prepare_collector_transfer(
			root, lock, payload, &world, &result_code, &error);
		if (prepared != flatfile_world_item_result::ok &&
		    prepared != flatfile_world_item_result::unchanged)
		{
			const auto failure = world_prepare_failure(prepared);
			return { failure == flatfile_collector_repository_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.catalog_revision,
				 static_cast<unsigned int>(
					 failure == flatfile_collector_repository_result::io_error ?
						 EIO :
						 EILSEQ) };
		}
		include_world = !result_code && prepared == flatfile_world_item_result::ok &&
				world.changed;
		if (!result_code && payload.from_owner.type == item_owner_type::corpse &&
		    !include_world)
			result_code = ENOENT;
	}
	if (!result_code && payload.action == collector_action::purchase)
	{
		if (updated.price_value > static_cast<uint64_t>(INT64_MAX))
			result_code = ERANGE;
		else
		{
			const auto prepared = flatfile_player_domain_prepare_wallet(
				root, lock, payload.actor_pid, payload.account_name.data(),
				payload.racewar, payload.expected_wallet_revision,
				payload.expected_bank_revision,
				-static_cast<int64_t>(updated.price_value), true, &wallet_apply,
				&result_code, &error);
			if (prepared != flatfile_player_domain_result::ok)
			{
				if (prepared == flatfile_player_domain_result::not_found)
					result_code = ENOENT;
				else
					return {
						prepared == flatfile_player_domain_result::io_error ?
							critical_apply_outcome::retryable_failure :
							critical_apply_outcome::terminal_failure,
						catalog.catalog_revision,
						static_cast<unsigned int>(
							prepared ==
									flatfile_player_domain_result::io_error ?
								EIO :
								EILSEQ)
					};
			}
			include_wallet = !result_code;
			if (!result_code)
			{
				result.wallet = wallet_apply.wallet;
				result.bank = wallet_apply.bank;
				result.wallet_revision = wallet_apply.wallet_revision;
				result.bank_revision = wallet_apply.bank_revision;
			}
		}
		if (!result_code)
		{
			const item_transfer_payload transfer = materialization_payload(payload);
			const auto prepared = flatfile_item_transfer_materialization_prepare(
				root, lock, command.operation_id, transfer, &materialization,
				&error);
			if (prepared != flatfile_shop_trade_materialization_result::ok &&
			    prepared != flatfile_shop_trade_materialization_result::unchanged)
				return {
					prepared == flatfile_shop_trade_materialization_result::io_error ?
						critical_apply_outcome::retryable_failure :
						critical_apply_outcome::terminal_failure,
					catalog.catalog_revision,
					static_cast<unsigned int>(
						prepared ==
								flatfile_shop_trade_materialization_result::
									io_error ?
							EIO :
							EILSEQ)
				};
			include_materialization =
				prepared == flatfile_shop_trade_materialization_result::ok;
		}
	}

	const bool mutation_applied = !result_code;
	if (mutation_applied)
	{
		listing = find_listing(&candidate, payload.listing);
		if (!listing || candidate.catalog_revision == UINT64_MAX)
			return { critical_apply_outcome::terminal_failure,
				 catalog.catalog_revision, ERANGE };
		listing->entry = updated;
		if (payload.action == collector_action::collect)
		{
			try
			{
				listing->item_blob.assign(
					payload.item_blob.begin(),
					payload.item_blob.begin() + payload.item_blob_size);
			}
			catch (const std::bad_alloc &)
			{
				return { critical_apply_outcome::retryable_failure,
					 catalog.catalog_revision, ENOMEM };
			}
		}
		++candidate.catalog_revision;
		result.record_present = true;
		result.catalog_revision = candidate.catalog_revision;
		result.from_owner_revision = item.from_owner_revision;
		result.to_owner_revision = item.to_owner_revision;
		result.entry = updated;
	}
	else
	{
		try
		{
			candidate = catalog;
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure,
				 catalog.catalog_revision, ENOMEM };
		}
		include_item = include_world = include_wallet = include_materialization = false;
		const currency_vector wallet = result.wallet;
		const currency_vector bank = result.bank;
		const uint64_t wallet_revision = result.wallet_revision;
		const uint64_t bank_revision = result.bank_revision;
		result = {};
		result.action = payload.action;
		result.wallet = wallet;
		result.bank = bank;
		result.wallet_revision = wallet_revision;
		result.bank_revision = bank_revision;
	}
	try
	{
		candidate.operations.push_back({ command.operation_id, digest, result_code, result });
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.catalog_revision,
			 ENOMEM };
	}
	candidate.file_revision = candidate.file_revision ? candidate.file_revision + 1 : 1;
	std::vector<uint8_t> catalog_bytes;
	if (!encode_catalog(candidate, &catalog_bytes))
		return { critical_apply_outcome::terminal_failure, catalog.catalog_revision, ENOSPC };
	std::vector<flatfile_authority_after_image> images;
	try
	{
		images.push_back({ catalog_filename, std::move(catalog_bytes) });
		if (include_item)
			images.push_back(std::move(item.after_image));
		if (include_world)
			images.push_back(std::move(world.after_image));
		if (include_wallet)
			for (auto &image : wallet_apply.after_images)
				images.push_back(std::move(image));
		if (include_materialization)
			images.push_back(std::move(materialization.after_image));
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.catalog_revision,
			 ENOMEM };
	}
	const auto committed = flatfile_authority_transaction_commit(root, lock, images, &error);
	if (committed != flatfile_authority_transaction_result::ok)
		return { committed == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 catalog.catalog_revision,
			 static_cast<unsigned int>(
				 committed == flatfile_authority_transaction_result::io_error ? EIO :
										  EILSEQ) };
	return make_result(candidate.operations.back(), candidate.catalog_revision,
			   critical_apply_outcome::applied);
}
