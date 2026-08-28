#include "flatfile_auction_repository.h"

#include "auction_command.h"
#include "flatfile_authority_transaction.h"
#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'A', 'U', 'C', 'T', 0 };
constexpr uint32_t catalog_version = 2;
constexpr uint32_t catalog_legacy_version = 1;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t catalog_maximum_listings = 262144;
constexpr size_t catalog_maximum_money = 262144;
constexpr size_t catalog_maximum_operations = 1048576;
constexpr const char *catalog_filename = "auction_catalog";
constexpr uint32_t auction_status_open = 1;
constexpr uint32_t auction_status_closed = 2;
constexpr uint32_t auction_status_removed = 3;

struct auction_item
{
	uint64_t uid = 0;
	uint64_t revision = 0;
	int32_t vnum = 0;
	uint32_t claim_pid = 0;
	bool claimed = false;
};

struct auction_listing
{
	uint32_t id = 0;
	uint32_t seller_pid = 0;
	uint32_t winner_pid = 0;
	uint32_t status = 0;
	int64_t current_price = 0;
	int64_t buy_price = 0;
	uint64_t revision = 0;
	uint64_t end_time = 0;
	std::string seller_account;
	std::string seller_name;
	std::string winner_name;
	std::string object_short;
	std::string id_keywords;
	std::string object_info;
	std::vector<uint8_t> object_blob;
	std::vector<auction_item> items;
};

struct money_pickup
{
	uint32_t pid = 0;
	int64_t amount = 0;
	uint64_t revision = 0;
};

struct auction_operation
{
	critical_operation_id operation_id;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest = {};
	unsigned int result_code = 0;
	auction_command_result result = {};
	bool event_published = false;
};

struct auction_catalog
{
	uint64_t revision = 0;
	std::vector<auction_listing> listings;
	std::vector<money_pickup> money;
	std::vector<auction_operation> operations;
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
		using U = std::make_unsigned_t<T>;
		U bits = static_cast<U>(value);
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

	void string(const std::string &value)
	{
		number<uint32_t>(value.size());
		raw(reinterpret_cast<const uint8_t *>(value.data()), value.size());
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
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
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

	bool string(std::string *value, size_t maximum)
	{
		uint32_t count = 0;
		if (!value || !number(&count) || count > maximum || size - offset < count)
			return false;
		try
		{
			value->assign(reinterpret_cast<const char *>(data + offset), count);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		offset += count;
		return value->find('\0') == std::string::npos;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool canonical_account(const std::string &input, std::string *output)
{
	if (!output || input.empty() || input.size() > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	output->clear();
	for (unsigned char character : input)
	{
		if (character >= 'A' && character <= 'Z')
			output->push_back(static_cast<char>(character - 'A' + 'a'));
		else if ((character >= 'a' && character <= 'z') ||
			 (character >= '0' && character <= '9') || character == '_' ||
			 character == '-')
			output->push_back(static_cast<char>(character));
		else
			return false;
	}
	return true;
}

auction_listing *find_listing(auction_catalog *catalog, uint32_t id)
{
	auto found = std::lower_bound(catalog->listings.begin(), catalog->listings.end(), id,
				      [](const auction_listing &entry, uint32_t candidate)
				      { return entry.id < candidate; });
	return found != catalog->listings.end() && found->id == id ? &*found : nullptr;
}

money_pickup *find_money(auction_catalog *catalog, uint32_t pid)
{
	auto found = std::lower_bound(catalog->money.begin(), catalog->money.end(), pid,
				      [](const money_pickup &entry, uint32_t candidate)
				      { return entry.pid < candidate; });
	return found != catalog->money.end() && found->pid == pid ? &*found : nullptr;
}

bool stage_money(auction_catalog *catalog, uint32_t pid, int64_t amount)
{
	if (!pid || amount < 0 || amount > UINT_MAX)
		return false;
	money_pickup *pickup = find_money(catalog, pid);
	if (!pickup)
	{
		try
		{
			catalog->money.push_back({ pid, 0, 0 });
			std::sort(catalog->money.begin(), catalog->money.end(),
				  [](const auto &left, const auto &right)
				  { return left.pid < right.pid; });
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		pickup = find_money(catalog, pid);
	}
	if (!pickup || pickup->amount > INT64_MAX - amount ||
	    pickup->revision == std::numeric_limits<uint64_t>::max())
		return false;
	pickup->amount += amount;
	++pickup->revision;
	return true;
}

bool encode_catalog(const auction_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || catalog.listings.size() > catalog_maximum_listings ||
	    catalog.money.size() > catalog_maximum_money ||
	    catalog.operations.size() > catalog_maximum_operations)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.listings.size());
	for (const auto &listing : catalog.listings)
	{
		payload.number(listing.id);
		payload.number(listing.seller_pid);
		payload.number(listing.winner_pid);
		payload.number(listing.status);
		payload.number(listing.current_price);
		payload.number(listing.buy_price);
		payload.number(listing.revision);
		payload.number(listing.end_time);
		payload.string(listing.seller_account);
		payload.string(listing.seller_name);
		payload.string(listing.winner_name);
		payload.string(listing.object_short);
		payload.string(listing.id_keywords);
		payload.string(listing.object_info);
		payload.number<uint32_t>(listing.object_blob.size());
		payload.raw(listing.object_blob.data(), listing.object_blob.size());
		payload.number<uint16_t>(listing.items.size());
		for (const auto &item : listing.items)
		{
			payload.number(item.uid);
			payload.number(item.revision);
			payload.number(item.vnum);
			payload.number(item.claim_pid);
			payload.number<uint8_t>(item.claimed);
		}
	}
	payload.number<uint32_t>(catalog.money.size());
	for (const auto &pickup : catalog.money)
	{
		payload.number(pickup.pid);
		payload.number(pickup.amount);
		payload.number(pickup.revision);
	}
	payload.number<uint32_t>(catalog.operations.size());
	for (const auto &operation : catalog.operations)
	{
		payload.raw(operation.operation_id.bytes.data(),
			    operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number(operation.result_code);
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> result = {};
		if (!auction_command_encode_result(operation.result, &result))
			return false;
		payload.raw(result.data(), result.size());
		payload.number<uint8_t>(operation.event_published);
	}
	if (!payload.valid || payload.bytes.size() > catalog_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(catalog_magic.data(), catalog_magic.size());
	file.number(catalog_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(std::max(catalog.revision, UINT64_C(1)));
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > catalog_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, auction_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) ||
	    (version != catalog_version && version != catalog_legacy_version) || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *expected_digest = bytes.data() + 24;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	auction_catalog decoded;
	decoded.revision = revision;
	uint32_t listing_count = 0, money_count = 0, operation_count = 0;
	if (!payload.number(&listing_count) || listing_count > catalog_maximum_listings)
		return false;
	try
	{
		decoded.listings.resize(listing_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &listing : decoded.listings)
	{
		uint32_t blob_size = 0;
		uint16_t item_count = 0;
		if (!payload.number(&listing.id) || !payload.number(&listing.seller_pid) ||
		    !payload.number(&listing.winner_pid) || !payload.number(&listing.status) ||
		    !payload.number(&listing.current_price) ||
		    !payload.number(&listing.buy_price) || !payload.number(&listing.revision) ||
		    !payload.number(&listing.end_time) ||
		    !payload.string(&listing.seller_account, CURRENCY_ACCOUNT_NAME_MAX_BYTES) ||
		    !payload.string(&listing.seller_name, AUCTION_NAME_MAX_BYTES) ||
		    !payload.string(&listing.winner_name, AUCTION_NAME_MAX_BYTES) ||
		    !payload.string(&listing.object_short, AUCTION_SHORT_MAX_BYTES) ||
		    !payload.string(&listing.id_keywords, AUCTION_KEYWORDS_MAX_BYTES) ||
		    !payload.string(&listing.object_info, AUCTION_INFO_MAX_BYTES) ||
		    !payload.number(&blob_size) || blob_size > AUCTION_BLOB_MAX_BYTES ||
		    payload.size - payload.offset < blob_size)
			return false;
		try
		{
			listing.object_blob.resize(blob_size);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		if (!payload.raw(listing.object_blob.data(), blob_size) ||
		    !payload.number(&item_count) || !item_count ||
		    item_count > AUCTION_COMMAND_MAX_ITEMS)
			return false;
		try
		{
			listing.items.resize(item_count);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		for (auto &item : listing.items)
		{
			uint8_t claimed = 0;
			if (!payload.number(&item.uid) || !payload.number(&item.revision) ||
			    !payload.number(&item.vnum) || !payload.number(&item.claim_pid) ||
			    !payload.number(&claimed) || claimed > 1)
				return false;
			item.claimed = claimed;
		}
		for (size_t index = 0; index < listing.items.size(); ++index)
		{
			if (!listing.items[index].uid || !listing.items[index].revision ||
			    listing.items[index].vnum < 0)
				return false;
			for (size_t other = index + 1; other < listing.items.size(); ++other)
				if (listing.items[index].uid == listing.items[other].uid)
					return false;
		}
		std::string canonical;
		if (!listing.id || !listing.seller_pid || !listing.revision ||
		    listing.status < auction_status_open ||
		    listing.status > auction_status_removed ||
		    !canonical_account(listing.seller_account, &canonical) ||
		    canonical != listing.seller_account)
			return false;
	}
	if (!std::is_sorted(decoded.listings.begin(), decoded.listings.end(),
			    [](const auto &left, const auto &right) { return left.id < right.id; }))
		return false;
	for (size_t index = 1; index < decoded.listings.size(); ++index)
		if (decoded.listings[index - 1].id == decoded.listings[index].id)
			return false;
	if (!payload.number(&money_count) || money_count > catalog_maximum_money)
		return false;
	try
	{
		decoded.money.resize(money_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &pickup : decoded.money)
		if (!payload.number(&pickup.pid) || !payload.number(&pickup.amount) ||
		    !payload.number(&pickup.revision) || !pickup.pid || pickup.amount < 0 ||
		    !pickup.revision)
			return false;
	if (!std::is_sorted(decoded.money.begin(), decoded.money.end(),
			    [](const auto &left, const auto &right)
			    { return left.pid < right.pid; }))
		return false;
	for (size_t index = 1; index < decoded.money.size(); ++index)
		if (decoded.money[index - 1].pid == decoded.money[index].pid)
			return false;
	if (!payload.number(&operation_count) || operation_count > catalog_maximum_operations)
		return false;
	try
	{
		decoded.operations.resize(operation_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> result = {};
	std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, operation_id_hash>
		operation_ids;
	try
	{
		operation_ids.reserve(operation_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	try
	{
		for (auto &operation : decoded.operations)
		{
			uint8_t event_published = version == catalog_legacy_version;
			if (!payload.raw(operation.operation_id.bytes.data(),
					 operation.operation_id.bytes.size()) ||
			    !payload.raw(operation.command_digest.data(),
					 operation.command_digest.size()) ||
			    !payload.number(&operation.result_code) ||
			    !payload.raw(result.data(), result.size()) ||
			    (version == catalog_version && !payload.number(&event_published)) ||
			    event_published > 1 ||
			    critical_operation_id_is_zero(operation.operation_id) ||
			    !auction_command_decode_result(result.data(), result.size(),
							   &operation.result) ||
			    !operation_ids.insert(operation.operation_id.bytes).second)
				return false;
			operation.event_published = event_published;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size)
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_read_result load_catalog(const std::string &root, auction_catalog *catalog,
				  std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto read = flatfile_read(domains_directory(root), catalog_filename,
					catalog_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
	{
		*catalog = {};
		return read;
	}
	if (read != flatfile_read_result::ok)
		return read;
	return decode_catalog(bytes, catalog) ? flatfile_read_result::ok :
						flatfile_read_result::invalid;
}

flatfile_auction_query_result query_catalog(const std::string &root, auction_catalog *catalog,
					    std::string *error)
{
	if (root.empty() || !catalog)
		return flatfile_auction_query_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_auction_query_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_auction_query_result::io_error :
			       flatfile_auction_query_result::invalid;
	const auto loaded = load_catalog(root, catalog, error);
	if (loaded == flatfile_read_result::not_found)
	{
		*catalog = {};
		return flatfile_auction_query_result::ok;
	}
	if (loaded == flatfile_read_result::ok)
		return flatfile_auction_query_result::ok;
	if (loaded == flatfile_read_result::invalid && error && error->empty())
		*error = "auction catalog is corrupt";
	return loaded == flatfile_read_result::io_error ? flatfile_auction_query_result::io_error :
							  flatfile_auction_query_result::invalid;
}

bool project_listing(const auction_listing &source, uint32_t claim_pid,
		     flatfile_auction_listing_projection *target)
{
	if (!target)
		return false;
	flatfile_auction_listing_projection projected;
	projected.auction_id = source.id;
	projected.seller_pid = source.seller_pid;
	projected.winner_pid = source.winner_pid;
	projected.current_price = source.current_price;
	projected.buy_price = source.buy_price;
	projected.revision = source.revision;
	projected.end_time = source.end_time;
	try
	{
		projected.seller_name = source.seller_name;
		projected.winner_name = source.winner_name;
		projected.object_short = source.object_short;
		projected.id_keywords = source.id_keywords;
		projected.object_info = source.object_info;
		projected.object_blob = source.object_blob;
		for (const auto &item : source.items)
			if (!claim_pid || (item.claim_pid == claim_pid && !item.claimed))
				projected.items.push_back({ item.uid, item.revision, item.vnum });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*target = std::move(projected);
	return true;
}

uint32_t listing_id(const critical_operation_id &operation_id)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(operation_id.bytes.data(), operation_id.bytes.size(), digest.data());
	uint32_t id = 0;
	for (size_t byte = 0; byte < sizeof(id); ++byte)
		id |= static_cast<uint32_t>(digest[byte]) << (byte * 8);
	return id ? id : 1;
}

bool sale_proceeds(int64_t price, uint32_t basis_points, int64_t *proceeds)
{
	if (!proceeds || price < 0 || basis_points > 10000)
		return false;
	const int64_t fee =
		(price / 10000) * basis_points + ((price % 10000) * basis_points) / 10000;
	*proceeds = price - fee;
	return true;
}

uint64_t durable_revision(const auction_command_result &result, uint64_t catalog_revision)
{
	return std::max({ catalog_revision, result.wallet_revision, result.bank_revision,
			  result.auction_revision, result.player_owner_revision,
			  result.auction_owner_revision });
}

critical_apply_result make_result(const auction_operation &operation, uint64_t revision,
				  critical_apply_outcome success)
{
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded = {};
	if (!auction_command_encode_result(operation.result, &encoded))
		return { critical_apply_outcome::terminal_failure, revision, EILSEQ };
	critical_apply_result result = {
		operation.result_code ? critical_apply_outcome::terminal_failure : success,
		durable_revision(operation.result, revision), operation.result_code
	};
	result.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), result.result_payload.begin());
	return result;
}
} // namespace

flatfile_auction_query_result
flatfile_auction_list_open(const std::string &root,
			   std::vector<flatfile_auction_listing_projection> *listings,
			   std::string *error)
{
	if (!listings)
		return flatfile_auction_query_result::invalid;
	auction_catalog catalog;
	const auto loaded = query_catalog(root, &catalog, error);
	if (loaded != flatfile_auction_query_result::ok)
		return loaded;
	std::vector<flatfile_auction_listing_projection> projected;
	try
	{
		projected.reserve(catalog.listings.size());
		for (const auto &listing : catalog.listings)
		{
			if (listing.status != auction_status_open)
				continue;
			projected.emplace_back();
			if (!project_listing(listing, 0, &projected.back()))
				return flatfile_auction_query_result::io_error;
		}
		std::sort(projected.begin(), projected.end(),
			  [](const auto &left, const auto &right)
			  {
				  return left.end_time != right.end_time ?
						 left.end_time < right.end_time :
						 left.auction_id < right.auction_id;
			  });
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_auction_query_result::io_error;
	}
	*listings = std::move(projected);
	return flatfile_auction_query_result::ok;
}

flatfile_auction_query_result
flatfile_auction_find_open(const std::string &root, uint32_t auction_id,
			   flatfile_auction_listing_projection *listing, std::string *error)
{
	if (!auction_id || !listing)
		return flatfile_auction_query_result::invalid;
	auction_catalog catalog;
	const auto loaded = query_catalog(root, &catalog, error);
	if (loaded != flatfile_auction_query_result::ok)
		return loaded;
	const auction_listing *found = find_listing(&catalog, auction_id);
	if (!found || found->status != auction_status_open)
		return flatfile_auction_query_result::not_found;
	return project_listing(*found, 0, listing) ? flatfile_auction_query_result::ok :
						     flatfile_auction_query_result::io_error;
}

flatfile_auction_query_result
flatfile_auction_find_pickup(const std::string &root, uint32_t pid,
			     flatfile_auction_pickup_projection *pickup, std::string *error)
{
	if (!pid || !pickup)
		return flatfile_auction_query_result::invalid;
	auction_catalog catalog;
	const auto loaded = query_catalog(root, &catalog, error);
	if (loaded != flatfile_auction_query_result::ok)
		return loaded;
	flatfile_auction_pickup_projection projected;
	if (const money_pickup *money = find_money(&catalog, pid))
	{
		projected.money = money->amount;
		projected.money_revision = money->revision;
	}
	for (const auto &listing : catalog.listings)
	{
		const bool pending = std::any_of(
			listing.items.begin(), listing.items.end(), [&](const auction_item &item)
			{ return item.claim_pid == pid && !item.claimed; });
		if (!pending)
			continue;
		if (!project_listing(listing, pid, &projected.item_claim))
			return flatfile_auction_query_result::io_error;
		projected.has_item_claim = true;
		break;
	}
	*pickup = std::move(projected);
	return flatfile_auction_query_result::ok;
}

flatfile_auction_query_result
flatfile_auction_find_pending_event(const std::string &root,
				    flatfile_auction_event_projection *event, std::string *error)
{
	if (!event)
		return flatfile_auction_query_result::invalid;
	auction_catalog catalog;
	const auto loaded = query_catalog(root, &catalog, error);
	if (loaded != flatfile_auction_query_result::ok)
		return loaded;
	const auto operation = std::find_if(catalog.operations.begin(), catalog.operations.end(),
					    [](const auction_operation &candidate)
					    { return !candidate.event_published; });
	if (operation == catalog.operations.end())
		return flatfile_auction_query_result::not_found;
	const auction_listing *listing = find_listing(&catalog, operation->result.auction_id);
	if (!listing)
	{
		if (error)
			*error = "auction event references a missing listing";
		return flatfile_auction_query_result::invalid;
	}
	flatfile_auction_event_projection projected;
	projected.operation_id = operation->operation_id;
	projected.result = operation->result;
	if (!project_listing(*listing, 0, &projected.listing))
		return flatfile_auction_query_result::io_error;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(projected.operation_id.bytes.data(), projected.operation_id.bytes.size(),
	       digest.data());
	for (size_t index = 0; index < sizeof(projected.outbox_id); ++index)
		projected.outbox_id |= static_cast<uint64_t>(digest[index]) << (index * 8);
	if (!projected.outbox_id)
		projected.outbox_id = 1;
	*event = std::move(projected);
	return flatfile_auction_query_result::ok;
}

flatfile_auction_query_result
flatfile_auction_acknowledge_event(const std::string &root,
				   const critical_operation_id &operation_id, std::string *error)
{
	if (root.empty() || critical_operation_id_is_zero(operation_id))
		return flatfile_auction_query_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_auction_query_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_auction_query_result::io_error :
			       flatfile_auction_query_result::invalid;
	auction_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_auction_query_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ?
			       flatfile_auction_query_result::io_error :
			       flatfile_auction_query_result::invalid;
	auto operation = std::find_if(
		catalog.operations.begin(), catalog.operations.end(),
		[&](const auction_operation &candidate)
		{ return critical_operation_id_equal(candidate.operation_id, operation_id); });
	if (operation == catalog.operations.end())
		return flatfile_auction_query_result::not_found;
	if (operation->event_published)
		return flatfile_auction_query_result::ok;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_auction_query_result::io_error;
	operation->event_published = true;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_auction_query_result::io_error;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_auction_query_result::ok :
		       flatfile_auction_query_result::io_error;
}

flatfile_auction_player_reference_result
flatfile_auction_check_player_unreferenced(const std::string &root,
					   const flatfile_authority_lock &lock, uint32_t pid,
					   std::string *error)
{
	if (!pid || !lock.matches(root))
		return flatfile_auction_player_reference_result::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_auction_player_reference_result::io_error :
			       flatfile_auction_player_reference_result::invalid;
	auction_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_read_result::ok && loaded != flatfile_read_result::not_found)
		return loaded == flatfile_read_result::io_error ?
			       flatfile_auction_player_reference_result::io_error :
			       flatfile_auction_player_reference_result::invalid;
	for (const auto &listing : catalog.listings)
	{
		if (listing.seller_pid == pid || listing.winner_pid == pid)
			return flatfile_auction_player_reference_result::referenced;
		for (const auto &item : listing.items)
			if (item.claim_pid == pid)
				return flatfile_auction_player_reference_result::referenced;
	}
	if (std::any_of(catalog.money.begin(), catalog.money.end(),
			[pid](const auto &entry) { return entry.pid == pid; }))
		return flatfile_auction_player_reference_result::referenced;
	return flatfile_auction_player_reference_result::clear;
}

critical_apply_result flatfile_auction_repository_apply(const std::string &root,
							const critical_command &command)
{
	auction_command_payload payload = {};
	std::vector<uint8_t> encoded_command;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !auction_command_decode_payload(command, &payload) ||
	    critical_command_encode(command, &encoded_command) != critical_command_codec_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	SHA256(encoded_command.data(), encoded_command.size(), digest.data());
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
	auction_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_read_result::ok && loaded != flatfile_read_result::not_found)
		return { loaded == flatfile_read_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 loaded == flatfile_read_result::io_error ? EIO : EILSEQ) };
	for (const auto &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EEXIST };
			return make_result(operation, catalog.revision,
					   critical_apply_outcome::already_applied);
		}
	if (catalog.operations.size() >= catalog_maximum_operations ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };

	auction_command_result result = {};
	result.action = payload.action;
	unsigned int result_code = 0;
	flatfile_wallet_mutation wallet;
	if (payload.actor_pid)
	{
		const auto prepared = flatfile_player_domain_prepare_wallet(
			root, lock, payload.actor_pid, payload.account_name.data(), payload.racewar,
			payload.expected_wallet_revision, payload.expected_bank_revision, 0, false,
			&wallet, &result_code, &error);
		if (prepared != flatfile_player_domain_result::ok)
		{
			if (prepared == flatfile_player_domain_result::not_found)
				result_code = ENOENT;
			else
				return { prepared == flatfile_player_domain_result::io_error ?
						 critical_apply_outcome::retryable_failure :
						 critical_apply_outcome::terminal_failure,
					 0,
					 static_cast<unsigned int>(
						 prepared == flatfile_player_domain_result::io_error ?
							 EIO :
							 EILSEQ) };
		}
		result.wallet = wallet.wallet;
		result.bank = wallet.bank;
		result.wallet_revision = wallet.wallet_revision;
		result.bank_revision = wallet.bank_revision;
	}
	flatfile_item_auction_mutation item_mutation;
	auction_catalog original_catalog;
	try
	{
		original_catalog = catalog;
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	bool mutation_applied = false;
	int64_t wallet_delta = 0;
	bool mutate_wallet = false;
	if (!result_code && payload.action == auction_action::list)
	{
		if (payload.listing_fee < 0 || payload.start_price < 0 ||
		    (payload.buy_price && payload.buy_price < payload.start_price) ||
		    !payload.object_blob_size)
			result_code = EINVAL;
		const uint32_t id = listing_id(command.operation_id);
		if (!result_code && find_listing(&catalog, id))
			result_code = EEXIST;
		if (!result_code)
		{
			const auto prepared = flatfile_item_repository_prepare_auction_transfer(
				root, lock, payload, id, true, &item_mutation, &result_code,
				&error);
			if (prepared != flatfile_item_repository_result::ok)
				return {
					prepared == flatfile_item_repository_result::io_error ?
						critical_apply_outcome::retryable_failure :
						critical_apply_outcome::terminal_failure,
					0,
					static_cast<unsigned int>(
						prepared == flatfile_item_repository_result::io_error ?
							EIO :
							EILSEQ)
				};
		}
		if (!result_code)
		{
			auction_listing listing;
			listing.id = id;
			listing.seller_pid = payload.actor_pid;
			listing.status = auction_status_open;
			listing.current_price = payload.start_price;
			listing.buy_price = payload.buy_price;
			listing.revision = 1;
			listing.end_time = payload.end_time;
			canonical_account(payload.account_name.data(), &listing.seller_account);
			listing.seller_name = payload.actor_name.data();
			listing.object_short = payload.object_short.data();
			listing.id_keywords = payload.id_keywords.data();
			listing.object_info = payload.object_info.data();
			try
			{
				listing.object_blob.assign(payload.object_blob.begin(),
							   payload.object_blob.begin() +
								   payload.object_blob_size);
				for (size_t index = 0; index < payload.item_count; ++index)
					listing.items.push_back(
						{ payload.items[index].item_uid,
						  item_mutation.item_revisions[index],
						  payload.items[index].vnum, 0, false });
				catalog.listings.push_back(std::move(listing));
				std::sort(catalog.listings.begin(), catalog.listings.end(),
					  [](const auto &left, const auto &right)
					  { return left.id < right.id; });
			}
			catch (const std::bad_alloc &)
			{
				return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
			}
			wallet_delta = -payload.listing_fee;
			mutate_wallet = true;
			result.auction_id = id;
			result.status = auction_status_open;
			result.seller_pid = payload.actor_pid;
			result.auction_revision = 1;
			result.player_owner_revision = item_mutation.player_owner_revision;
			result.auction_owner_revision = item_mutation.auction_owner_revision;
			result.item_count = item_mutation.item_count;
			result.item_uids = item_mutation.item_uids;
			result.item_revisions = item_mutation.item_revisions;
			result.event_type = auction_event_type::listed;
			mutation_applied = true;
		}
	}
	else if (!result_code && payload.action == auction_action::bid)
	{
		auto *listing = find_listing(&catalog, payload.auction_id);
		if (!listing)
			result_code = ENOENT;
		else
		{
			std::string actor_account;
			if (!canonical_account(payload.account_name.data(), &actor_account) ||
			    listing->status != auction_status_open ||
			    listing->seller_pid == payload.actor_pid ||
			    listing->seller_account == actor_account)
				result_code = EACCES;
		}
		if (listing && !result_code)
		{
			if (listing->revision == std::numeric_limits<uint64_t>::max())
				result_code = ERANGE;
		}
		if (listing && !result_code)
		{
			int64_t bid = payload.value;
			if (listing->buy_price > 0 && bid >= listing->buy_price)
				bid = listing->buy_price;
			if (bid <= 0 || (!listing->winner_pid && bid < listing->current_price) ||
			    (listing->winner_pid && bid <= listing->current_price))
				result_code = EINVAL;
			else
			{
				const int64_t to_pay = listing->winner_pid == payload.actor_pid ?
							       bid - listing->current_price :
							       bid;
				const uint32_t previous = listing->winner_pid;
				if (previous && previous != payload.actor_pid &&
				    !stage_money(&catalog, previous, listing->current_price))
					return { critical_apply_outcome::retryable_failure, 0,
						 ENOMEM };
				listing->winner_pid = payload.actor_pid;
				listing->winner_name = payload.actor_name.data();
				listing->current_price = bid;
				++listing->revision;
				const bool sold = listing->buy_price > 0 &&
						  bid >= listing->buy_price;
				if (sold)
				{
					listing->status = auction_status_closed;
					int64_t proceeds = 0;
					if (!sale_proceeds(bid, payload.closing_fee_basis_points,
							   &proceeds))
						result_code = EINVAL;
					else if (!stage_money(&catalog, listing->seller_pid,
							      proceeds))
						return { critical_apply_outcome::retryable_failure,
							 0, ENOMEM };
					if (!result_code)
						for (auto &item : listing->items)
							if (!item.claimed && !item.claim_pid)
								item.claim_pid = payload.actor_pid;
				}
				else if (previous != payload.actor_pid &&
					 payload.bid_extension_seconds)
				{
					if (listing->end_time >
					    UINT64_MAX - payload.bid_extension_seconds)
						result_code = ERANGE;
					else
						listing->end_time += payload.bid_extension_seconds;
				}
				wallet_delta = -to_pay;
				mutate_wallet = true;
				result.auction_id = listing->id;
				result.status = listing->status;
				result.seller_pid = listing->seller_pid;
				result.winner_pid = payload.actor_pid;
				result.previous_bidder_pid = previous;
				result.final_price = bid;
				result.auction_revision = listing->revision;
				result.event_type = sold ? auction_event_type::sold :
							   auction_event_type::bid_placed;
				mutation_applied = true;
			}
		}
	}
	else if (!result_code && (payload.action == auction_action::finalize ||
				  payload.action == auction_action::remove))
	{
		auto *listing = find_listing(&catalog, payload.auction_id);
		if (!listing)
			result_code = ENOENT;
		else if (listing->status != auction_status_open)
			result_code = EALREADY;
		else if (payload.action == auction_action::finalize &&
			 listing->end_time > static_cast<uint64_t>(time(nullptr)))
			result_code = EAGAIN;
		else if (listing->revision == std::numeric_limits<uint64_t>::max())
			result_code = ERANGE;
		else
		{
			++listing->revision;
			listing->status = payload.action == auction_action::remove ?
						  auction_status_removed :
						  auction_status_closed;
			const uint32_t claimant =
				(!listing->winner_pid || payload.action == auction_action::remove) ?
					listing->seller_pid :
					listing->winner_pid;
			for (auto &item : listing->items)
				if (!item.claimed && !item.claim_pid)
					item.claim_pid = claimant;
			if (listing->winner_pid && payload.action != auction_action::remove)
			{
				int64_t proceeds = 0;
				if (!sale_proceeds(listing->current_price,
						   payload.closing_fee_basis_points, &proceeds))
					result_code = EINVAL;
				else if (!stage_money(&catalog, listing->seller_pid, proceeds))
					return { critical_apply_outcome::retryable_failure, 0,
						 ENOMEM };
			}
			result.auction_id = listing->id;
			result.status = listing->status;
			result.seller_pid = listing->seller_pid;
			result.winner_pid = listing->winner_pid;
			result.final_price = listing->current_price;
			result.auction_revision = listing->revision;
			result.event_type = payload.action == auction_action::remove ?
						    auction_event_type::removed :
					    listing->winner_pid ? auction_event_type::sold :
								  auction_event_type::expired;
			mutation_applied = true;
		}
	}
	else if (!result_code && payload.action == auction_action::claim_money)
	{
		money_pickup *pickup = find_money(&catalog, payload.actor_pid);
		if (!pickup || pickup->amount <= 0 || pickup->amount > INT_MAX)
			result_code = ENOENT;
		else if (pickup->revision == std::numeric_limits<uint64_t>::max())
			result_code = ERANGE;
		else
		{
			wallet_delta = pickup->amount;
			mutate_wallet = true;
			pickup->amount = 0;
			++pickup->revision;
			result.event_type = auction_event_type::money_claimed;
			result.auction_revision = wallet.wallet_revision + 1;
			mutation_applied = true;
		}
	}
	else if (!result_code && payload.action == auction_action::claim_item)
	{
		auto *listing = find_listing(&catalog, payload.auction_id);
		if (!listing)
			result_code = ENOENT;
		for (size_t index = 0; listing && !result_code && index < payload.item_count;
		     ++index)
		{
			auto found =
				std::find_if(listing->items.begin(), listing->items.end(),
					     [&](const auction_item &item)
					     { return item.uid == payload.items[index].item_uid; });
			if (found == listing->items.end() || found->claimed ||
			    found->claim_pid != payload.actor_pid ||
			    found->revision != payload.items[index].expected_item_revision)
				result_code = ESTALE;
		}
		if (listing && !result_code)
		{
			if (listing->revision == std::numeric_limits<uint64_t>::max())
				result_code = ERANGE;
		}
		if (listing && !result_code)
		{
			const auto prepared = flatfile_item_repository_prepare_auction_transfer(
				root, lock, payload, listing->id, false, &item_mutation,
				&result_code, &error);
			if (prepared != flatfile_item_repository_result::ok)
				return {
					prepared == flatfile_item_repository_result::io_error ?
						critical_apply_outcome::retryable_failure :
						critical_apply_outcome::terminal_failure,
					0,
					static_cast<unsigned int>(
						prepared == flatfile_item_repository_result::io_error ?
							EIO :
							EILSEQ)
				};
		}
		if (listing && !result_code)
		{
			for (size_t index = 0; index < payload.item_count; ++index)
			{
				auto found = std::find_if(
					listing->items.begin(), listing->items.end(),
					[&](const auction_item &item)
					{ return item.uid == payload.items[index].item_uid; });
				found->revision = item_mutation.item_revisions[index];
				found->claimed = true;
			}
			++listing->revision;
			result.auction_id = listing->id;
			result.status = listing->status;
			result.seller_pid = listing->seller_pid;
			result.winner_pid = payload.actor_pid;
			result.auction_revision = listing->revision;
			result.player_owner_revision = item_mutation.player_owner_revision;
			result.auction_owner_revision = item_mutation.auction_owner_revision;
			result.item_count = item_mutation.item_count;
			result.item_uids = item_mutation.item_uids;
			result.item_revisions = item_mutation.item_revisions;
			result.event_type = auction_event_type::item_claimed;
			mutation_applied = true;
		}
	}
	else if (!result_code)
		result_code = EINVAL;

	if (!result_code && mutate_wallet)
	{
		const auto prepared = flatfile_player_domain_prepare_wallet(
			root, lock, payload.actor_pid, payload.account_name.data(), payload.racewar,
			payload.expected_wallet_revision, payload.expected_bank_revision,
			wallet_delta, true, &wallet, &result_code, &error);
		if (prepared != flatfile_player_domain_result::ok)
			return { prepared == flatfile_player_domain_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 0,
				 static_cast<unsigned int>(
					 prepared == flatfile_player_domain_result::io_error ?
						 EIO :
						 EILSEQ) };
		if (!result_code)
		{
			result.wallet_value_delta = wallet_delta;
			result.wallet = wallet.wallet;
			result.bank = wallet.bank;
			result.wallet_revision = wallet.wallet_revision;
			result.bank_revision = wallet.bank_revision;
		}
	}
	if (result_code)
	{
		mutation_applied = false;
		try
		{
			catalog = original_catalog;
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision,
				 ENOMEM };
		}
		result = {};
		result.action = payload.action;
		result.wallet = wallet.wallet;
		result.bank = wallet.bank;
		result.wallet_revision = wallet.wallet_revision;
		result.bank_revision = wallet.bank_revision;
	}
	try
	{
		const bool needs_publication =
			!result_code && result.event_type != auction_event_type::none &&
			result.event_type != auction_event_type::money_claimed &&
			result.event_type != auction_event_type::item_claimed;
		catalog.operations.push_back(
			{ command.operation_id, digest, result_code, result, !needs_publication });
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	++catalog.revision;
	std::vector<uint8_t> catalog_bytes;
	if (!encode_catalog(catalog, &catalog_bytes))
		return { critical_apply_outcome::terminal_failure, catalog.revision - 1, ENOSPC };
	std::vector<flatfile_authority_after_image> images;
	try
	{
		images.push_back({ catalog_filename, std::move(catalog_bytes) });
		if (mutation_applied && item_mutation.after_image.bytes.size())
			images.push_back(std::move(item_mutation.after_image));
		if (mutation_applied)
			for (auto &image : wallet.after_images)
				images.push_back(std::move(image));
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision - 1, ENOMEM };
	}
	const auto committed = flatfile_authority_transaction_commit(root, lock, images, &error);
	if (committed != flatfile_authority_transaction_result::ok)
		return { committed == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 catalog.revision - 1,
			 static_cast<unsigned int>(
				 committed == flatfile_authority_transaction_result::io_error ?
					 EIO :
					 EILSEQ) };
	return make_result(catalog.operations.back(), catalog.revision,
			   critical_apply_outcome::applied);
}
