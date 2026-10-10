#include "flatfile/flatfile_auction_repository.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_physical.h"
#include "economy/native_mobile_birth_cash_role_command.h"

#include "economy/auction_command.h"
#include "economy/auction_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_store.h"

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
constexpr std::array<uint8_t, 8> source_magic = { 'D', 'U', 'R', 'A', 'U', 'S', 'R', 0 };
constexpr uint32_t source_version = 1;
constexpr size_t source_maximum_bytes = 32 * 1024 * 1024;
constexpr size_t source_maximum_rows = 262144;
constexpr const char *source_filename = "auction_claim_sources";
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

struct auction_claim_source_row
{
	critical_operation_id operation = {};
	uint16_t slot = 0;
	critical_operation_id lineage = {};
	uint32_t beneficiary_pid = 0;
	uint64_t claim_mapping_id = 0;
	uint64_t amount = 0;
	critical_operation_id consumed_by = {};
};

struct auction_claim_source_catalog
{
	uint64_t revision = 0;
	std::vector<auction_claim_source_row> rows;
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

bool spend_money(auction_catalog *catalog, uint32_t pid, int64_t amount)
{
	if (!catalog || !pid || amount <= 0)
		return false;
	const auto *existing = find_money(catalog, pid);
	if (!existing || existing->amount < amount ||
	    existing->revision == std::numeric_limits<uint64_t>::max())
		return false;
	auto *pickup = find_money(catalog, pid);
	pickup->amount -= amount;
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

bool source_less(const auction_claim_source_row &left, const auction_claim_source_row &right)
{
	return left.operation.bytes < right.operation.bytes ||
	       (left.operation.bytes == right.operation.bytes && left.slot < right.slot);
}

bool encode_sources(const auction_claim_source_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || catalog.rows.size() > source_maximum_rows)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.rows.size());
	for (const auto &row : catalog.rows)
	{
		payload.raw(row.operation.bytes.data(), row.operation.bytes.size());
		payload.number(row.slot);
		payload.raw(row.lineage.bytes.data(), row.lineage.bytes.size());
		payload.number(row.beneficiary_pid);
		payload.number(row.claim_mapping_id);
		payload.number(row.amount);
		payload.raw(row.consumed_by.bytes.data(), row.consumed_by.bytes.size());
	}
	if (!payload.valid || payload.bytes.size() > source_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(source_magic.data(), source_magic.size());
	file.number(source_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > source_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_sources(const std::vector<uint8_t> &bytes, auction_claim_source_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), source_magic.data(), source_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != source_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data() + header_size, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ bytes.data() + header_size, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > source_maximum_rows ||
	    payload_size != 4 + static_cast<size_t>(count) * 70)
		return false;
	auction_claim_source_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.rows.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &row : decoded.rows)
		if (!payload.raw(row.operation.bytes.data(), row.operation.bytes.size()) ||
		    !payload.number(&row.slot) ||
		    !payload.raw(row.lineage.bytes.data(), row.lineage.bytes.size()) ||
		    !payload.number(&row.beneficiary_pid) ||
		    !payload.number(&row.claim_mapping_id) || !payload.number(&row.amount) ||
		    !payload.raw(row.consumed_by.bytes.data(), row.consumed_by.bytes.size()) ||
		    critical_operation_id_is_zero(row.operation) || !row.slot ||
		    critical_operation_id_is_zero(row.lineage) || !row.beneficiary_pid ||
		    !row.claim_mapping_id || !row.amount || row.amount > INT_MAX ||
		    critical_operation_id_equal(row.operation, row.consumed_by))
			return false;
	for (size_t index = 1; index < decoded.rows.size(); ++index)
		if (!source_less(decoded.rows[index - 1], decoded.rows[index]))
			return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_read_result load_sources(const std::string &root, auction_claim_source_catalog *catalog,
				  std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto read = flatfile_read(domains_directory(root), source_filename,
					source_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
	{
		*catalog = {};
		return read;
	}
	if (read != flatfile_read_result::ok)
		return read;
	return decode_sources(bytes, catalog) ? flatfile_read_result::ok :
						flatfile_read_result::invalid;
}

bool add_source(auction_claim_source_catalog *catalog, const critical_command &command,
		const economic_account_key &claim_account, uint32_t beneficiary_pid, uint16_t slot,
		uint64_t amount)
{
	if (!catalog || !beneficiary_pid || !slot || !amount || amount > INT_MAX ||
	    catalog->rows.size() >= source_maximum_rows || catalog->revision == UINT64_MAX ||
	    claim_account.kind != economic_account_kind::pending_claim ||
	    !economic_account_key_valid(claim_account))
		return false;
	auction_claim_source_row row{ command.operation_id,
				      slot,
				      claim_account.lineage,
				      beneficiary_pid,
				      claim_account.authority_id,
				      amount,
				      {} };
	auto found = std::lower_bound(catalog->rows.begin(), catalog->rows.end(), row, source_less);
	if (found != catalog->rows.end() && !source_less(row, *found))
		return false;
	catalog->rows.insert(found, row);
	++catalog->revision;
	return true;
}

bool claim_source_balance_matches(const auction_claim_source_catalog &catalog,
				  const economic_account_key &claim_account,
				  uint32_t beneficiary_pid, int64_t balance)
{
	if (balance < 0 || (balance && !economic_account_key_valid(claim_account)))
		return false;
	uint64_t total = 0;
	for (const auto &row : catalog.rows)
		if (critical_operation_id_is_zero(row.consumed_by) &&
		    row.lineage.bytes == claim_account.lineage.bytes &&
		    row.claim_mapping_id == claim_account.authority_id &&
		    row.beneficiary_pid == beneficiary_pid)
		{
			if (row.amount > static_cast<uint64_t>(INT64_MAX) - total)
				return false;
			total += row.amount;
		}
	return total == static_cast<uint64_t>(balance);
}

bool consume_whole_claim_sources(auction_claim_source_catalog *catalog,
				 const critical_command &command,
				 const economic_account_key &claim_account,
				 uint32_t beneficiary_pid, int64_t claim_balance, int64_t amount)
{
	if (!catalog || amount <= 0 || catalog->revision == UINT64_MAX ||
	    !claim_source_balance_matches(*catalog, claim_account, beneficiary_pid, claim_balance))
		return false;
	uint64_t remaining = static_cast<uint64_t>(amount);
	std::vector<size_t> selected;
	try
	{
		for (size_t index = 0; index < catalog->rows.size() && remaining; ++index)
		{
			const auto &row = catalog->rows[index];
			if (!critical_operation_id_is_zero(row.consumed_by) ||
			    row.lineage.bytes != claim_account.lineage.bytes ||
			    row.claim_mapping_id != claim_account.authority_id ||
			    row.beneficiary_pid != beneficiary_pid)
				continue;
			if (row.amount > remaining)
				return false;
			selected.push_back(index);
			remaining -= row.amount;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (remaining || selected.empty())
		return false;
	for (const size_t index : selected)
		catalog->rows[index].consumed_by = command.operation_id;
	++catalog->revision;
	return true;
}

bool plan_claim_credit(const economic_accounting_plan &plan,
		       const economic_account_key &claim_account, int64_t before,
		       uint64_t before_revision, int64_t amount)
{
	if (amount <= 0 || before < 0 || before > INT64_MAX - amount ||
	    before_revision == UINT64_MAX)
		return false;
	for (const auto &effect : plan.accounts)
	{
		if (!economic_account_key_equal(effect.key, claim_account))
			continue;
		int64_t effect_before = 0, effect_after = 0;
		return economic_coin_value(effect.before, &effect_before) ==
			       economic_accounting_error::ok &&
		       economic_coin_value(effect.after, &effect_after) ==
			       economic_accounting_error::ok &&
		       effect.before_revision == before_revision &&
		       effect.after_revision == before_revision + 1 && effect_before == before &&
		       effect_after == before + amount;
	}
	return false;
}

bool plan_claim_debit(const economic_accounting_plan &plan,
		      const economic_account_key &claim_account, int64_t before,
		      uint64_t before_revision, int64_t amount)
{
	if (amount <= 0 || before < amount || before_revision == UINT64_MAX)
		return false;
	for (const auto &effect : plan.accounts)
	{
		if (!economic_account_key_equal(effect.key, claim_account))
			continue;
		int64_t effect_before = 0, effect_after = 0;
		return economic_coin_value(effect.before, &effect_before) ==
			       economic_accounting_error::ok &&
		       economic_coin_value(effect.after, &effect_after) ==
			       economic_accounting_error::ok &&
		       effect.before_revision == before_revision &&
		       effect.after_revision == before_revision + 1 && effect_before == before &&
		       effect_after == before - amount;
	}
	return false;
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

bool claim_state(const auction_catalog &catalog, const auction_command_payload &payload,
		 auction_item_claim_state *claim)
{
	if (!claim || payload.action != auction_action::claim_item)
		return false;
	const auto found_listing = std::find_if(catalog.listings.begin(), catalog.listings.end(),
						[&](const auction_listing &listing)
						{ return listing.id == payload.auction_id; });
	if (found_listing == catalog.listings.end())
		return false;
	const auction_listing *listing = &*found_listing;
	auction_item_claim_state value;
	value.auction_id = listing->id;
	value.seller_pid = listing->seller_pid;
	value.winner_pid = listing->winner_pid;
	value.claimant_pid = payload.actor_pid;
	value.status = listing->status;
	value.custody_state = 1;
	value.auction_revision = listing->revision;
	value.item_count = payload.item_count;
	for (const auto &operation : catalog.operations)
	{
		if (operation.result_code || operation.result.auction_id != listing->id)
			continue;
		if (operation.result.event_type == auction_event_type::listed)
		{
			if (!critical_operation_id_is_zero(value.listing_operation))
				return false;
			value.listing_operation = operation.operation_id;
		}
		else if (operation.result.event_type == auction_event_type::sold ||
			 operation.result.event_type == auction_event_type::expired ||
			 operation.result.event_type == auction_event_type::removed)
		{
			if (!critical_operation_id_is_zero(value.claim_source_operation))
				return false;
			value.claim_source_operation = operation.operation_id;
		}
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto found = std::find_if(
			listing->items.begin(), listing->items.end(), [&](const auction_item &item)
			{ return item.uid == payload.items[index].item_uid; });
		if (found == listing->items.end())
			return false;
		value.rows[index] = { found->uid,
				      found->revision,
				      static_cast<uint16_t>(found - listing->items.begin()),
				      found->vnum,
				      found->claim_pid,
				      found->claimed };
	}
	*claim = value;
	return true;
}

bool open_listing_state(const auction_catalog &catalog, uint32_t auction_id,
			auction_bid_accounting_listing *state)
{
	if (!state)
		return false;
	const auto found = std::find_if(catalog.listings.begin(), catalog.listings.end(),
					[&](const auction_listing &listing)
					{ return listing.id == auction_id; });
	if (found == catalog.listings.end() || found->status != auction_status_open ||
	    found->items.empty())
		return false;
	for (const auto &item : found->items)
		if (item.claimed || item.claim_pid)
			return false;
	auction_bid_accounting_listing value;
	value.auction_id = found->id;
	value.seller_pid = found->seller_pid;
	value.winning_bidder_pid = found->winner_pid;
	value.status = found->status;
	value.custody_state = 1;
	value.current_price = found->current_price;
	value.buy_price = found->buy_price;
	value.revision = found->revision;
	for (const auto &operation : catalog.operations)
	{
		if (operation.result_code || operation.result.auction_id != found->id)
			continue;
		if (operation.result.event_type == auction_event_type::listed)
		{
			if (!critical_operation_id_is_zero(value.listing_operation) ||
			    operation.result.auction_revision != 1 ||
			    operation.result.seller_pid != found->seller_pid)
				return false;
			value.listing_operation = operation.operation_id;
		}
		else if (operation.result.event_type == auction_event_type::bid_placed &&
			 operation.result.auction_revision == found->revision)
		{
			if (!critical_operation_id_is_zero(value.previous_bid_operation) ||
			    operation.result.winner_pid != found->winner_pid ||
			    operation.result.final_price != found->current_price)
				return false;
			value.previous_bid_operation = operation.operation_id;
		}
	}
	if (critical_operation_id_is_zero(value.listing_operation) ||
	    (found->winner_pid != 0) !=
		    !critical_operation_id_is_zero(value.previous_bid_operation))
		return false;
	*state = value;
	return true;
}

bool settlement_state(const auction_catalog &catalog, uint32_t auction_id,
		      auction_settlement_listing *state)
{
	if (!state)
		return false;
	auction_bid_accounting_listing open;
	if (!open_listing_state(catalog, auction_id, &open))
		return false;
	const auto found = std::find_if(catalog.listings.begin(), catalog.listings.end(),
					[&](const auction_listing &listing)
					{ return listing.id == auction_id; });
	if (found == catalog.listings.end() || found->items.size() > AUCTION_COMMAND_MAX_ITEMS)
		return false;
	auction_settlement_listing value;
	value.auction_id = open.auction_id;
	value.seller_pid = open.seller_pid;
	value.winner_pid = open.winning_bidder_pid;
	value.status = open.status;
	value.custody_state = open.custody_state;
	value.quantity = found->items.size();
	value.current_price = open.current_price;
	value.buy_price = open.buy_price;
	value.revision = open.revision;
	value.end_time = found->end_time;
	value.listing_operation = open.listing_operation;
	value.winning_bid_operation = open.previous_bid_operation;
	value.item_count = found->items.size();
	for (size_t index = 0; index < found->items.size(); ++index)
	{
		const auto &item = found->items[index];
		value.items[index] = { item.uid,  item.revision,  static_cast<uint16_t>(index),
				       item.vnum, item.claim_pid, item.claimed };
	}
	*state = value;
	return true;
}

critical_apply_result accounted_completion(const flatfile_accounting_record &record)
{
	critical_apply_result result = { record.result_code ?
						 critical_apply_outcome::terminal_failure :
						 critical_apply_outcome::already_applied,
					 record.durable_revision, record.result_code };
	result.failure_stage = record.failure_stage;
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
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

class flatfile_accounting_auction_item_claim_transaction
{
    public:
	static critical_apply_result apply(const std::string &root, const critical_command &command,
					   bool accounted, auction_action accounted_action);
};

critical_apply_result flatfile_accounting_auction_item_claim_transaction::apply(
	const std::string &root, const critical_command &command, bool accounted,
	auction_action accounted_action)
try
{
	auction_command_payload payload = {};
	std::vector<uint8_t> encoded_command;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() ||
	    !(accounted ? critical_command_envelope_valid(command) :
			  critical_command_valid(command)) ||
	    !auction_command_decode_payload(command, &payload) ||
	    critical_command_encode(command, &encoded_command) != critical_command_codec_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	economic_frozen_intent intent;
	auction_item_claim_state frozen_claim;
	auction_bid_accounting_listing frozen_bid;
	auction_bid_accounting_accounts bid_accounts;
	auction_settlement_listing frozen_settlement;
	auction_settlement_accounts settlement_accounts;
	economic_account_key wallet_account, bank_account, claim_account;
	if (accounted &&
	    (payload.action != accounted_action ||
	     (accounted_action == auction_action::list ?
		      auction_listing_accounting_decode(command, &intent, &payload, &wallet_account,
							&bank_account) :
	      accounted_action == auction_action::bid ?
		      auction_bid_accounting_decode(command, &intent, &payload, &frozen_bid,
						    &bid_accounts) :
	      accounted_action == auction_action::finalize ||
			      accounted_action == auction_action::remove ?
		      auction_settlement_accounting_decode(command, &intent, &payload,
							   &frozen_settlement,
							   &settlement_accounts) :
	      accounted_action == auction_action::claim_money ?
		      auction_money_claim_accounting_decode(command, &intent, &payload,
							    &wallet_account, &bank_account,
							    &claim_account) :
		      auction_item_claim_accounting_decode(
			      command, &intent, &payload, &frozen_claim, &wallet_account,
			      &bank_account)) != economic_accounting_error::ok))
		return { critical_apply_outcome::terminal_failure, 0, EPROTONOSUPPORT };
	if (accounted && accounted_action == auction_action::bid)
	{
		wallet_account = bid_accounts.wallet;
		bank_account = bid_accounts.bank;
	}
	if (accounted && (accounted_action == auction_action::finalize ||
			  accounted_action == auction_action::remove))
	{
		wallet_account = settlement_accounts.actor_wallet;
		bank_account = settlement_accounts.actor_bank;
	}
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
	if (accounted)
	{
		flatfile_accounting_record retained;
		const auto found =
			flatfile_accounting_lookup(root, lock, command, &retained, &error);
		if (found == flatfile_accounting_status::ok)
		{
			const auto native = std::find_if(
				catalog.operations.begin(), catalog.operations.end(),
				[&](const auction_operation &operation) {
					return critical_operation_id_equal(operation.operation_id,
									   command.operation_id);
				});
			std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded = {};
			if (native == catalog.operations.end() ||
			    CRYPTO_memcmp(native->command_digest.data(), digest.data(),
					  digest.size()) ||
			    !auction_command_encode_result(native->result, &encoded) ||
			    retained.result_code != native->result_code ||
			    retained.result.size() != encoded.size() ||
			    !std::equal(retained.result.begin(), retained.result.end(),
					encoded.begin()) ||
			    retained.durable_revision < durable_revision(native->result, 0) ||
			    retained.durable_revision >
				    durable_revision(native->result, catalog.revision) ||
			    (retained.result_code == 0) != !retained.plan.empty())
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EILSEQ };
			if (accounted_action == auction_action::claim_money &&
			    !retained.result_code)
			{
				auction_claim_source_catalog retained_sources;
				const auto read = load_sources(root, &retained_sources, &error);
				if (read != flatfile_read_result::ok)
					return { read == flatfile_read_result::io_error ?
							 critical_apply_outcome::retryable_failure :
							 critical_apply_outcome::terminal_failure,
						 catalog.revision,
						 static_cast<unsigned int>(
							 read == flatfile_read_result::io_error ?
								 EIO :
								 EILSEQ) };
				decoder facts{ intent.admission.facts.data(),
					       intent.admission.facts.size(), 24 };
				uint32_t beneficiary = 0;
				uint64_t amount = 0, revision = 0;
				if (!facts.number(&beneficiary) || !facts.number(&amount) ||
				    !facts.number(&revision))
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				auction_money_claim_state retained_claim;
				retained_claim.beneficiary_pid = beneficiary;
				retained_claim.money = static_cast<int64_t>(amount);
				retained_claim.revision = revision;
				for (const auto &row : retained_sources.rows)
					if (critical_operation_id_equal(row.consumed_by,
									command.operation_id))
					{
						if (row.lineage.bytes !=
							    claim_account.lineage.bytes ||
						    row.beneficiary_pid != beneficiary ||
						    row.claim_mapping_id !=
							    claim_account.authority_id)
							return { critical_apply_outcome::
									 terminal_failure,
								 catalog.revision, EILSEQ };
						retained_claim.sources.push_back(
							{ row.operation, row.slot,
							  row.beneficiary_pid, row.claim_mapping_id,
							  row.amount });
					}
				critical_command projected = command;
				projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
				projected.accounting_intent.clear();
				projected.publication_required = false;
				std::vector<uint8_t> expected;
				if (auction_money_claim_accounting_intent(
					    projected, intent.admission.metadata.epoch,
					    wallet_account, bank_account, claim_account,
					    retained_claim,
					    &expected) != economic_accounting_error::ok ||
				    expected != command.accounting_intent)
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				const auto linked =
					flatfile_accounting_storage::verify_source_claim(
						root, lock, retained, &error);
				if (linked != flatfile_accounting_status::ok)
					return { linked == flatfile_accounting_status::io_error ?
							 critical_apply_outcome::retryable_failure :
							 critical_apply_outcome::terminal_failure,
						 catalog.revision,
						 static_cast<unsigned int>(
							 linked == flatfile_accounting_status::
										 io_error ?
								 EIO :
								 EILSEQ) };
			}
			return accounted_completion(retained);
		}
		if (found != flatfile_accounting_status::not_found)
			return { found == flatfile_accounting_status::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 found == flatfile_accounting_status::io_error ? EIO :
					 found == flatfile_accounting_status::conflict ? EEXIST :
											 EILSEQ) };
	}
	for (const auto &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EEXIST };
			if (accounted)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EEXIST };
			return make_result(operation, catalog.revision,
					   critical_apply_outcome::already_applied);
		}
	if (const auto gate = accounted ? 0 :
					  flatfile_economic_legacy_domain_gate(root, lock, &error))
		return { critical_apply_outcome::retryable_failure, catalog.revision, gate };
	if (catalog.operations.size() >= catalog_maximum_operations ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	const bool listing_accounted = accounted && payload.action == auction_action::list;
	const bool bid_accounted = accounted && payload.action == auction_action::bid;
	const bool settlement_accounted = accounted &&
					  (payload.action == auction_action::finalize ||
					   payload.action == auction_action::remove);
	const bool money_accounted = accounted && payload.action == auction_action::claim_money;
	auction_item_claim_accounting_authority accounting_before;
	auction_money_claim_authority money_before;
	auction_listing_accounting_authority listing_before;
	auction_bid_accounting_authority bid_before;
	auction_settlement_authority settlement_before;
	auction_claim_source_catalog sources;
	bool sources_changed = false;
	if (bid_accounted || settlement_accounted || money_accounted)
	{
		const auto read = load_sources(root, &sources, &error);
		if (read != flatfile_read_result::ok && read != flatfile_read_result::not_found)
			return { read == flatfile_read_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 read == flatfile_read_result::io_error ? EIO : EILSEQ) };
	}
	flatfile_economic_authority_snapshot authority;
	if (accounted)
	{
		std::vector<flatfile_economic_mapping_request> requests;
		if (!settlement_accounted || payload.actor_pid)
		{
			std::string account;
			if (!canonical_account(payload.account_name.data(), &account))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EINVAL };
			requests.push_back({ wallet_account, { 1, payload.actor_pid, {} } });
			requests.push_back(
				{ bank_account, { 2, bank_account.authority_id, account } });
		}
		if (bid_accounted)
		{
			requests.push_back({ bid_accounts.escrow, { 4, payload.auction_id, {} } });
			requests.push_back(
				{ bid_accounts.bidder_claim, { 5, payload.actor_pid, {} } });
			if (bid_accounts.previous_claim.authority_id)
				requests.push_back({ bid_accounts.previous_claim,
						     { 5, frozen_bid.winning_bidder_pid, {} } });
			if (bid_accounts.seller_claim.authority_id)
				requests.push_back({ bid_accounts.seller_claim,
						     { 5, frozen_bid.seller_pid, {} } });
		}
		if (settlement_accounted)
		{
			requests.push_back(
				{ settlement_accounts.escrow, { 4, payload.auction_id, {} } });
			if (settlement_accounts.seller_claim.authority_id)
				requests.push_back({ settlement_accounts.seller_claim,
						     { 5, frozen_settlement.seller_pid, {} } });
		}
		if (money_accounted)
			requests.push_back({ claim_account, { 5, payload.actor_pid, {} } });
		const unsigned int gate = economic_flatfile_lock_authority(
			root, lock,
			settlement_accounted ? settlement_accounts.escrow.lineage :
					       wallet_account.lineage,
			intent.admission.metadata.epoch, requests, &authority, &error);
		if (gate)
			return { gate == EIO || gate == ENOMEM ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision, gate };
		if (listing_accounted)
		{
			listing_before.epoch = intent.admission.metadata.epoch;
			listing_before.wallet = wallet_account;
			listing_before.bank = bank_account;
		}
		else if (bid_accounted)
		{
			bid_before.epoch = intent.admission.metadata.epoch;
			bid_before.accounts = bid_accounts;
			if (!open_listing_state(catalog, payload.auction_id, &bid_before.listing))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			bid_before.bidder_claim_before = {};
			if (const auto *pickup = find_money(&catalog, payload.actor_pid))
				bid_before.bidder_claim_before = { pickup->amount,
								   pickup->revision };
			if (!claim_source_balance_matches(sources, bid_accounts.bidder_claim,
							  payload.actor_pid,
							  bid_before.bidder_claim_before.money))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			critical_command projected = command;
			projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			projected.accounting_intent.clear();
			projected.publication_required = false;
			std::vector<uint8_t> expected;
			if (auction_bid_accounting_intent(
				    projected, bid_before.epoch, bid_before.listing, bid_accounts,
				    &expected) != economic_accounting_error::ok ||
			    expected != command.accounting_intent)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			const auto prior = bid_before.listing.winning_bidder_pid;
			if (prior && prior != payload.actor_pid)
				if (const auto *pickup = find_money(&catalog, prior))
					bid_before.previous_claim_before = { pickup->amount,
									     pickup->revision };
			if (bid_before.listing.buy_price > 0 &&
			    payload.value >= bid_before.listing.buy_price)
				if (const auto *pickup =
					    find_money(&catalog, bid_before.listing.seller_pid))
					bid_before.seller_claim_before = { pickup->amount,
									   pickup->revision };
		}
		else if (settlement_accounted)
		{
			settlement_before.epoch = intent.admission.metadata.epoch;
			settlement_before.accounts = settlement_accounts;
			if (!settlement_state(catalog, payload.auction_id,
					      &settlement_before.listing))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			critical_command projected = command;
			projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			projected.accounting_intent.clear();
			projected.publication_required = false;
			std::vector<uint8_t> expected;
			if (auction_settlement_accounting_intent(projected, settlement_before.epoch,
								 settlement_before.listing,
								 settlement_accounts, &expected) !=
				    economic_accounting_error::ok ||
			    expected != command.accounting_intent)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			if (payload.action == auction_action::finalize &&
			    settlement_before.listing.winner_pid)
				if (const auto *pickup = find_money(
					    &catalog, settlement_before.listing.seller_pid))
				{
					settlement_before.seller_claim_before = pickup->amount;
					settlement_before.seller_claim_revision_before =
						pickup->revision;
				}
		}
		else if (money_accounted)
		{
			money_before.epoch = intent.admission.metadata.epoch;
			money_before.wallet = wallet_account;
			money_before.bank = bank_account;
			money_before.claim_account = claim_account;
			money_before.claim.beneficiary_pid = payload.actor_pid;
			if (const auto *pickup = find_money(&catalog, payload.actor_pid))
			{
				money_before.claim.money = pickup->amount;
				money_before.claim.revision = pickup->revision;
			}
			for (const auto &row : sources.rows)
				if (critical_operation_id_is_zero(row.consumed_by) &&
				    row.lineage.bytes == claim_account.lineage.bytes &&
				    row.beneficiary_pid == payload.actor_pid &&
				    row.claim_mapping_id == claim_account.authority_id)
				{
					if (money_before.claim.sources.size() >=
					    ECONOMIC_AUCTION_CLAIM_MAX_SOURCES)
						return { critical_apply_outcome::terminal_failure,
							 catalog.revision, EILSEQ };
					money_before.claim.sources.push_back(
						{ row.operation, row.slot, row.beneficiary_pid,
						  row.claim_mapping_id, row.amount });
				}
			critical_command projected = command;
			projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			projected.accounting_intent.clear();
			projected.publication_required = false;
			std::vector<uint8_t> expected;
			if (auction_money_claim_accounting_intent(
				    projected, money_before.epoch, wallet_account, bank_account,
				    claim_account, money_before.claim,
				    &expected) != economic_accounting_error::ok ||
			    expected != command.accounting_intent)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
		}
		else
		{
			accounting_before.epoch = intent.admission.metadata.epoch;
			accounting_before.wallet_account = wallet_account;
			accounting_before.bank_account = bank_account;
			if (!claim_state(catalog, payload, &accounting_before.claim))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			critical_command projected = command;
			projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			projected.accounting_intent.clear();
			projected.publication_required = false;
			std::vector<uint8_t> expected;
			if (auction_item_claim_accounting_intent(
				    projected, accounting_before.epoch, wallet_account,
				    bank_account, accounting_before.claim,
				    &expected) != economic_accounting_error::ok ||
			    expected != command.accounting_intent)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
		}
	}

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
	if (accounted && !result_code)
	{
		auto &balances = listing_accounted    ? listing_before.balances_before :
				 bid_accounted	      ? bid_before.balances_before :
				 settlement_accounted ? settlement_before.actor_balances_before :
				 money_accounted      ? money_before.balances_before :
							accounting_before.balances_before;
		balances.wallet = wallet.wallet;
		balances.bank = wallet.bank;
		balances.wallet_revision = wallet.wallet_revision;
		balances.bank_revision = wallet.bank_revision;
		const item_owner_identity auction_owner = { item_owner_type::auction,
							    payload.auction_id, 0 };
		const item_owner_identity player_owner = { item_owner_type::player,
							   payload.actor_pid, 0 };
		uint64_t auction_revision = 0, player_revision = 0;
		std::vector<flatfile_item_ownership_record> auction_items, player_items;
		const auto from = listing_accounted || money_accounted ?
					  flatfile_item_repository_result::not_found :
					  flatfile_item_repository_load_owner_locked(
						  root, lock, auction_owner, &auction_revision,
						  &auction_items, &error);
		const auto to = bid_accounted || settlement_accounted || money_accounted ?
					flatfile_item_repository_result::not_found :
					flatfile_item_repository_load_owner_locked(
						root, lock, player_owner, &player_revision,
						&player_items, &error);
		if ((from != flatfile_item_repository_result::ok &&
		     from != flatfile_item_repository_result::not_found) ||
		    (to != flatfile_item_repository_result::ok &&
		     to != flatfile_item_repository_result::not_found))
			return { from == flatfile_item_repository_result::io_error ||
						 to == flatfile_item_repository_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 from == flatfile_item_repository_result::io_error ||
							 to == flatfile_item_repository_result::
									 io_error ?
						 EIO :
						 EILSEQ) };
		if (listing_accounted)
			listing_before.player_owner_revision_before = player_revision;
		else if (!bid_accounted && !settlement_accounted && !money_accounted)
		{
			accounting_before.auction_owner_revision_before = auction_revision;
			accounting_before.player_owner_revision_before = player_revision;
		}
		if (bid_accounted || settlement_accounted)
		{
			const auto *listing = find_listing(&catalog, payload.auction_id);
			if (!listing || auction_items.size() != listing->items.size())
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 ESTALE };
			for (const auto &row : listing->items)
			{
				const auto found = std::find_if(
					auction_items.begin(), auction_items.end(),
					[&](const auto &item) { return item.item_uid == row.uid; });
				if (found == auction_items.end() ||
				    found->root_item_uid != row.uid || found->parent_item_uid ||
				    found->item_revision != row.revision ||
				    found->vnum != row.vnum ||
				    found->state != item_custody_state::active)
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, ESTALE };
				if (settlement_accounted)
					settlement_before.items_before.push_back(
						{ found->item_uid,
						  { found->owner, found->root_item_uid,
						    found->parent_item_uid, found->item_revision,
						    found->state } });
			}
		}
		if (!money_accounted)
		{
			const auto &source_items = listing_accounted ? player_items : auction_items;
			auto &items_before = listing_accounted ? listing_before.items_before :
								 accounting_before.items_before;
			for (size_t index = 0; index < payload.item_count; ++index)
			{
				const auto selected = std::find_if(
					source_items.begin(), source_items.end(),
					[&](const auto &item)
					{ return item.item_uid == payload.items[index].item_uid; });
				if (selected == source_items.end())
					break;
				items_before.push_back(
					{ selected->item_uid,
					  { selected->owner, selected->root_item_uid,
					    selected->parent_item_uid, selected->item_revision,
					    selected->state } });
			}
		}
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
				root, lock, payload, id, true, listing_accounted, &item_mutation,
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
				const auto *claim = find_money(&catalog, payload.actor_pid);
				const int64_t claim_balance = claim ? claim->amount : 0;
				const int64_t claim_credit_used = std::min(to_pay, claim_balance);
				const int64_t wallet_to_pay = to_pay - claim_credit_used;
				if (claim_credit_used &&
				    !spend_money(&catalog, payload.actor_pid, claim_credit_used))
					result_code = ERANGE;
				if (!result_code)
				{
					const uint32_t previous = listing->winner_pid;
					if (previous && previous != payload.actor_pid &&
					    !stage_money(&catalog, previous,
							 listing->current_price))
						return { critical_apply_outcome::retryable_failure,
							 0, ENOMEM };
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
						if (!sale_proceeds(bid,
								   payload.closing_fee_basis_points,
								   &proceeds))
							result_code = EINVAL;
						else if (!stage_money(&catalog, listing->seller_pid,
								      proceeds))
							return { critical_apply_outcome::
									 retryable_failure,
								 0, ENOMEM };
						if (!result_code)
							for (auto &item : listing->items)
								if (!item.claimed &&
								    !item.claim_pid)
									item.claim_pid =
										payload.actor_pid;
					}
					else if (previous != payload.actor_pid &&
						 payload.bid_extension_seconds)
					{
						if (listing->end_time >
						    UINT64_MAX - payload.bid_extension_seconds)
							result_code = ERANGE;
						else
							listing->end_time +=
								payload.bid_extension_seconds;
					}
					wallet_delta = -wallet_to_pay;
					mutate_wallet = true;
					result.auction_id = listing->id;
					result.status = listing->status;
					result.seller_pid = listing->seller_pid;
					result.winner_pid = payload.actor_pid;
					result.previous_bidder_pid = previous;
					result.final_price = bid;
					result.claim_credit_used = claim_credit_used;
					result.auction_revision = listing->revision;
					result.event_type = sold ? auction_event_type::sold :
								   auction_event_type::bid_placed;
					mutation_applied = true;
				}
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
				root, lock, payload, listing->id, false, accounted, &item_mutation,
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
	if (settlement_accounted && mutation_applied)
	{
		const auto *updated = find_listing(&catalog, payload.auction_id);
		if (!updated || updated->items.size() != settlement_before.listing.item_count)
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		for (size_t index = 0; index < updated->items.size(); ++index)
		{
			const auto &item = updated->items[index];
			if (item.uid != settlement_before.listing.items[index].uid ||
			    item.revision != settlement_before.listing.items[index].revision ||
			    item.claimed)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EILSEQ };
			settlement_before.claim_pids_after.push_back(item.claim_pid);
		}
		if (payload.action == auction_action::finalize &&
		    settlement_before.listing.winner_pid)
		{
			const auto *pickup =
				find_money(&catalog, settlement_before.listing.seller_pid);
			if (!pickup)
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EILSEQ };
			settlement_before.seller_claim_after = pickup->amount;
			settlement_before.seller_claim_revision_after = pickup->revision;
		}
	}
	flatfile_accounting_record accounting_record;
	std::vector<flatfile_authority_operation> mapping_operations;
	if (accounted)
	{
		accounting_record.command = command;
		accounting_record.result_code = result_code;
		if (mutation_applied)
		{
			economic_accounting_plan plan;
			if (listing_accounted)
			{
				flatfile_economic_mapping escrow;
				const unsigned int created =
					flatfile_accounting_authority_storage::create_mapping(
						root, lock, authority.lineage_revision,
						economic_account_kind::auction_escrow, 0,
						{ 4, result.auction_id, {} }, command.operation_id,
						&escrow, &mapping_operations, &error);
				if (created)
					return { created == EIO || created == ENOMEM ?
							 critical_apply_outcome::retryable_failure :
							 critical_apply_outcome::terminal_failure,
						 catalog.revision, created };
				listing_before.escrow = escrow.account;
			}
			const auto planned =
				listing_accounted ?
					auction_listing_accounting_plan(
						command, intent, listing_before, result, &plan) :
				bid_accounted ?
					auction_bid_accounting_plan(command, intent, bid_before,
								    result, &plan) :
				settlement_accounted ?
					auction_settlement_accounting_plan(
						command, intent, settlement_before, result, &plan) :
				money_accounted ?
					auction_money_claim_accounting_plan(
						command, intent, money_before, result, &plan) :
					auction_item_claim_accounting_plan(
						command, intent, accounting_before, result, &plan);
			if (planned != economic_accounting_error::ok ||
			    (!listing_accounted && !bid_accounted && !settlement_accounted &&
			     !money_accounted &&
			     (!plan.accounts.empty() || !plan.postings.empty())) ||
			    !plan.children.empty() || plan.item_events.size() != payload.item_count)
				return { planned == economic_accounting_error::capacity ?
						 critical_apply_outcome::retryable_failure :
						 critical_apply_outcome::terminal_failure,
					 catalog.revision,
					 static_cast<unsigned int>(
						 planned == economic_accounting_error::capacity ?
							 ENOMEM :
							 EBADMSG) };
			if (bid_accounted)
			{
				const auto &listing = bid_before.listing;
				if (result.claim_credit_used > 0)
				{
					if (!plan_claim_debit(
						    plan, bid_accounts.bidder_claim,
						    bid_before.bidder_claim_before.money,
						    bid_before.bidder_claim_before.revision,
						    result.claim_credit_used) ||
					    !consume_whole_claim_sources(
						    &sources, command, bid_accounts.bidder_claim,
						    payload.actor_pid,
						    bid_before.bidder_claim_before.money,
						    result.claim_credit_used))
						return { critical_apply_outcome::terminal_failure,
							 catalog.revision, ENOSPC };
					sources_changed = true;
				}
				if (listing.winning_bidder_pid &&
				    listing.winning_bidder_pid != payload.actor_pid)
				{
					const auto *pickup =
						find_money(&catalog, listing.winning_bidder_pid);
					if (!pickup ||
					    pickup->amount !=
						    bid_before.previous_claim_before.money +
							    listing.current_price ||
					    pickup->revision !=
						    bid_before.previous_claim_before.revision + 1 ||
					    !plan_claim_credit(
						    plan, bid_accounts.previous_claim,
						    bid_before.previous_claim_before.money,
						    bid_before.previous_claim_before.revision,
						    listing.current_price) ||
					    !add_source(&sources, command,
							bid_accounts.previous_claim,
							listing.winning_bidder_pid, 1,
							listing.current_price))
						return { critical_apply_outcome::terminal_failure,
							 catalog.revision, EILSEQ };
					sources_changed = true;
				}
				if (result.event_type == auction_event_type::sold)
				{
					const int64_t fee = static_cast<int64_t>(
						static_cast<__int128_t>(result.final_price) *
						payload.closing_fee_basis_points / 10000);
					const int64_t proceeds = result.final_price - fee;
					const auto *pickup =
						find_money(&catalog, listing.seller_pid);
					if (!pickup ||
					    pickup->amount != bid_before.seller_claim_before.money +
								      proceeds ||
					    pickup->revision !=
						    bid_before.seller_claim_before.revision + 1)
						return { critical_apply_outcome::terminal_failure,
							 catalog.revision, EILSEQ };
					if (proceeds)
					{
						if (!plan_claim_credit(
							    plan, bid_accounts.seller_claim,
							    bid_before.seller_claim_before.money,
							    bid_before.seller_claim_before.revision,
							    proceeds) ||
						    !add_source(&sources, command,
								bid_accounts.seller_claim,
								listing.seller_pid, 2, proceeds))
							return { critical_apply_outcome::
									 terminal_failure,
								 catalog.revision, EILSEQ };
						sources_changed = true;
					}
				}
			}
			if (settlement_accounted && payload.action == auction_action::finalize &&
			    settlement_before.listing.winner_pid)
			{
				const int64_t fee = static_cast<int64_t>(
					static_cast<__int128_t>(result.final_price) *
					payload.closing_fee_basis_points / 10000);
				const int64_t proceeds = result.final_price - fee;
				if (proceeds &&
				    (!plan_claim_credit(
					     plan, settlement_accounts.seller_claim,
					     settlement_before.seller_claim_before,
					     settlement_before.seller_claim_revision_before,
					     proceeds) ||
				     !add_source(
					     &sources, command, settlement_accounts.seller_claim,
					     settlement_before.listing.seller_pid, 2, proceeds)))
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				sources_changed = proceeds > 0;
			}
			if (money_accounted)
			{
				const auto *pickup = find_money(&catalog, payload.actor_pid);
				if (!pickup || pickup->amount ||
				    pickup->revision != money_before.claim.revision + 1 ||
				    sources.revision == UINT64_MAX)
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				for (const auto &source : money_before.claim.sources)
				{
					auction_claim_source_row key{ source.operation,
								      source.slot };
					auto found = std::lower_bound(sources.rows.begin(),
								      sources.rows.end(), key,
								      source_less);
					if (found == sources.rows.end() ||
					    source_less(key, *found) ||
					    !critical_operation_id_is_zero(found->consumed_by) ||
					    found->lineage.bytes != claim_account.lineage.bytes ||
					    found->beneficiary_pid != payload.actor_pid ||
					    found->claim_mapping_id != claim_account.authority_id ||
					    found->amount != source.amount)
						return { critical_apply_outcome::terminal_failure,
							 catalog.revision, EILSEQ };
					found->consumed_by = command.operation_id;
				}
				++sources.revision;
				sources_changed = true;
			}
			if ((bid_accounted && result.event_type == auction_event_type::sold) ||
			    (settlement_accounted && payload.action == auction_action::finalize))
			{
				const auto &escrow = bid_accounted ? bid_accounts.escrow :
								     settlement_accounts.escrow;
				const auto effect = std::find_if(
					plan.accounts.begin(), plan.accounts.end(),
					[&](const auto &entry)
					{ return economic_account_key_equal(entry.key, escrow); });
				const auto mapping = std::find_if(
					authority.mappings.begin(), authority.mappings.end(),
					[&](const auto &entry) {
						return economic_account_key_equal(entry.account,
										  escrow);
					});
				if (effect == plan.accounts.end() ||
				    effect->after != economic_coin_vector{} ||
				    mapping == authority.mappings.end())
					return { critical_apply_outcome::terminal_failure,
						 catalog.revision, EILSEQ };
				const auto retired =
					flatfile_accounting_authority_storage::retire_mapping(
						root, lock, authority.lineage_revision, escrow,
						mapping->revision, command.operation_id,
						&mapping_operations, &error);
				if (retired)
					return { retired == EIO || retired == ENOMEM ?
							 critical_apply_outcome::retryable_failure :
							 critical_apply_outcome::terminal_failure,
						 catalog.revision, retired };
			}
			const auto encoded = economic_plan_encode(plan, &accounting_record.plan);
			if (encoded != economic_accounting_error::ok)
				return { encoded == economic_accounting_error::capacity ?
						 critical_apply_outcome::retryable_failure :
						 critical_apply_outcome::terminal_failure,
					 catalog.revision,
					 static_cast<unsigned int>(
						 encoded == economic_accounting_error::capacity ?
							 ENOMEM :
							 EILSEQ) };
		}
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
	if (accounted)
	{
		accounting_record.durable_revision = durable_revision(result, catalog.revision);
		std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded = {};
		if (!auction_command_encode_result(result, &encoded))
			return { critical_apply_outcome::terminal_failure, catalog.revision - 1,
				 EILSEQ };
		accounting_record.result.assign(encoded.begin(), encoded.end());
	}
	std::vector<uint8_t> catalog_bytes;
	if (!encode_catalog(catalog, &catalog_bytes))
		return { critical_apply_outcome::terminal_failure, catalog.revision - 1, ENOSPC };
	std::vector<flatfile_authority_after_image> images;
	try
	{
		images.push_back({ catalog_filename, std::move(catalog_bytes) });
		if (sources_changed)
		{
			std::vector<uint8_t> encoded_sources;
			if (!encode_sources(sources, &encoded_sources))
				return { critical_apply_outcome::terminal_failure,
					 catalog.revision - 1, ENOSPC };
			images.push_back({ source_filename, std::move(encoded_sources) });
		}
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
	std::vector<flatfile_authority_operation> operations;
	try
	{
		operations.reserve(images.size() + mapping_operations.size() + 3);
		for (auto &image : images)
			operations.push_back({ flatfile_authority_store::domains,
					       flatfile_authority_operation_kind::write,
					       std::move(image.filename), std::move(image.bytes) });
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision - 1, ENOMEM };
	}
	if (mutation_applied && item_mutation.item_count > 0)
	{
		std::vector<economic_accounting_item_reference> references;
		try
		{
			references.reserve(item_mutation.item_count);
			for (size_t index = 0; index < item_mutation.item_count; ++index)
			{
				economic_accounting_item_reference ref = {};
				ref.operation_id = command.operation_id;
				ref.line_index = static_cast<uint16_t>(index);
				ref.event_index = ref.line_index;
				ref.child_index = accounted ? 0 : 1;
				ref.item_uid = item_mutation.item_uids[index];
				ref.before_revision =
					index < payload.item_count ?
						payload.items[index].expected_item_revision :
						(item_mutation.item_revisions[index] > 0 ?
							 item_mutation.item_revisions[index] - 1 :
							 0);
				ref.after_revision = item_mutation.item_revisions[index];
				ref.legacy_operation_id = command.operation_id;
				ref.legacy_event_index = static_cast<uint16_t>(index);
				references.push_back(ref);
			}
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision - 1,
				 ENOMEM };
		}
		const auto staged = flatfile_item_accounting_reference_stage(
			root, lock, command.operation_id, references, &operations, &error);
		if (staged != flatfile_item_accounting_status::ok)
			return { staged == flatfile_item_accounting_status::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision - 1,
				 static_cast<unsigned int>(
					 staged == flatfile_item_accounting_status::io_error ?
						 EIO :
					 staged == flatfile_item_accounting_status::capacity ?
						 ENOSPC :
						 EILSEQ) };
	}
	if (accounted)
	{
		const auto staged = flatfile_accounting_storage::stage(
			root, lock, accounting_record, &operations, &error);
		if (staged != flatfile_accounting_status::ok)
			return { staged == flatfile_accounting_status::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision - 1,
				 static_cast<unsigned int>(
					 staged == flatfile_accounting_status::io_error ? EIO :
					 staged == flatfile_accounting_status::capacity ? ENOSPC :
					 staged == flatfile_accounting_status::already_exists ?
											  EEXIST :
											  EILSEQ) };
		if (money_accounted && !result_code)
		{
			const auto linked = flatfile_accounting_storage::stage_source_claim(
				root, lock, accounting_record, &operations, &error);
			if (linked != flatfile_accounting_status::ok)
				return {
					linked == flatfile_accounting_status::io_error ||
							linked ==
								flatfile_accounting_status::capacity ?
						critical_apply_outcome::retryable_failure :
						critical_apply_outcome::terminal_failure,
					catalog.revision - 1,
					static_cast<unsigned int>(
						linked == flatfile_accounting_status::io_error ?
							EIO :
						linked == flatfile_accounting_status::capacity ?
							ENOSPC :
							EILSEQ)
				};
		}
	}
	// The EAP stage accepts native images only; add the new mapping images
	// afterwards so all evidence still commits in the same authority journal.
	for (auto &mapping_operation : mapping_operations)
		operations.push_back(std::move(mapping_operation));
	const auto committed =
		accounted ? flatfile_accounting_storage::commit(root, lock, operations, &error) :
			    flatfile_authority_transaction_commit_operations(root, lock, operations,
									     &error);
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
catch (const std::bad_alloc &)
{
	return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
}

critical_apply_result flatfile_auction_repository_apply(const std::string &root,
							const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(root, command, false,
									 auction_action::list);
}

critical_apply_result
flatfile_auction_repository_apply_accounted_item_claim(const std::string &root,
						       const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(
		root, command, true, auction_action::claim_item);
}

critical_apply_result
flatfile_auction_repository_apply_accounted_listing(const std::string &root,
						    const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(root, command, true,
									 auction_action::list);
}

critical_apply_result
flatfile_auction_repository_apply_accounted_bid(const std::string &root,
						const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(root, command, true,
									 auction_action::bid);
}

critical_apply_result
flatfile_auction_repository_apply_accounted_finalize(const std::string &root,
						     const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(root, command, true,
									 auction_action::finalize);
}

critical_apply_result
flatfile_auction_repository_apply_accounted_remove(const std::string &root,
						   const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(root, command, true,
									 auction_action::remove);
}

critical_apply_result
flatfile_auction_repository_apply_accounted_money_claim(const std::string &root,
							const critical_command &command)
{
	return flatfile_accounting_auction_item_claim_transaction::apply(
		root, command, true, auction_action::claim_money);
}

unsigned int flatfile_native_mobile_birth_ordinary_auction_physical_storage::verify_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_catalog_physical_absence *output,
	std::string *error) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return EINVAL;
	try
	{
		if (!native_mobile_birth_cash_role_recovery_valid(original))
			return EINVAL;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		if (native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    !intent.admission.metadata.source_event)
			return EINVAL;
		std::vector<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return EINVAL;
		auction_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_read_result::ok && loaded != flatfile_read_result::not_found)
			return loaded == flatfile_read_result::io_error ? EIO : EILSEQ;
		flatfile_native_mobile_birth_ordinary_catalog_physical_absence observed;
		observed.present = loaded == flatfile_read_result::ok;
		observed.file_revision = catalog.revision;
		observed.catalog_revision = catalog.revision;
		observed.listings = catalog.listings.size();
		for (const auto &listing : catalog.listings)
			for (const auto &item : listing.items)
			{
				// Original SQL auction_item_custody absence is not restricted by
				// listing status, claim PID or claimed state. Retained rows count.
				if (std::binary_search(born.begin(), born.end(), item.uid))
					return EEXIST;
				++observed.item_rows;
			}
		if (!lock.matches(root))
			return EINVAL;
		*output = observed;
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
}

// Private prospective CURRENT catalog proof beside the original allocating
// readers. This is explicit C++ request accounting, not native stack/library
// allocator/OpenSSL/system or whole-process 32MiB qualification.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
struct physical_failure
{
	unsigned int code;
};
size_t physical_sum(size_t a, size_t b)
{
	if (b > SIZE_MAX - a)
		throw physical_failure{ ENOBUFS };
	return a + b;
}
size_t physical_product(size_t a, size_t b)
{
	if (b && a > SIZE_MAX / b)
		throw physical_failure{ ENOBUFS };
	return a * b;
}
struct physical_reservation
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	bool refused = false;
	static bool callback(size_t bytes, void *context) noexcept
	{
		auto &owner = *static_cast<physical_reservation *>(context);
		const bool accepted = owner.reserve(bytes, owner.context);
		if (!accepted)
			owner.refused = true;
		return accepted;
	}
};
// Pure status mapping; caller prospectively owns these actual source frames.
constexpr size_t physical_codec_status_frames = sizeof(economic_accounting_error) + sizeof(void *) +
						sizeof(unsigned int) + sizeof(bool) * 4;
unsigned int physical_codec_status(economic_accounting_error code,
				   const physical_reservation &reservation) noexcept
{
	if (reservation.refused || code == economic_accounting_error::capacity ||
	    code == economic_accounting_error::overflow)
		return ENOBUFS;
	if (code == economic_accounting_error::unresolved)
		return ENOTSUP;
	return code == economic_accounting_error::ok ? 0 : EINVAL;
}
struct physical_scope
{
	physical_reservation *owner;
	size_t outer;
	void admit(size_t request) const
	{
		if (!physical_reservation::callback(physical_sum(outer, request), owner))
			throw physical_failure{ ENOBUFS };
	}
	physical_scope nested(size_t request) const
	{
		return { owner, physical_sum(outer, request) };
	}
};
constexpr size_t physical_callback_frames = sizeof(physical_reservation) +
					    sizeof(physical_scope) * 2 + sizeof(void *) * 4 +
					    sizeof(size_t) * 10 + sizeof(bool) * 3;
size_t physical_string_heap(const std::string &value)
{
	return value.capacity() > 15 ? physical_sum(value.capacity(), 1) : 0;
}
template <class Set> size_t physical_hash_heap(const Set &values)
{
	using node = std::__detail::_Hash_node<
		typename Set::value_type,
		std::__cache_default<typename Set::key_type, typename Set::hasher>::value>;
	size_t total = physical_product(values.size(), sizeof(node));
	if (values.bucket_count() != 1)
		total = physical_sum(total,
				     physical_product(values.bucket_count(),
						      sizeof(std::__detail::_Hash_node_base *)));
	return total;
}
template <class Set>
void physical_hash_reserve(Set &values, size_t count, physical_scope scope, size_t live)
{
	struct work_frame
	{
		std::__detail::_Prime_rehash_policy policy;
		size_t buckets;
	};
	scope.admit(physical_sum(live, sizeof(work_frame)));
	work_frame work{};
	work.buckets = work.policy._M_next_bkt(work.policy._M_bkt_for_elements(count));
	scope.admit(physical_sum(physical_sum(live, sizeof(work_frame)),
				 physical_product(work.buckets,
						  sizeof(std::__detail::_Hash_node_base *))));
	values.reserve(count);
}
template <class Set> void physical_hash_insert_admit([[maybe_unused]] const Set &values,
						     physical_scope scope, size_t live)
{
	using node = std::__detail::_Hash_node<
		typename Set::value_type,
		std::__cache_default<typename Set::key_type, typename Set::hasher>::value>;
	// All these new local sets reserved their full count before insertion.
	// The pinned default-load-factor-one bucket table therefore never grows.
	scope.admit(physical_sum(live, sizeof(node)));
}
size_t physical_catalog_heap(const auction_catalog &catalog)
{
	size_t total = 0;
	total = physical_sum(total, physical_product(catalog.listings.capacity(),
						     sizeof(auction_listing)));
	total = physical_sum(total,
			     physical_product(catalog.money.capacity(), sizeof(money_pickup)));
	total = physical_sum(total, physical_product(catalog.operations.capacity(),
						     sizeof(auction_operation)));
	for (const auto &listing : catalog.listings)
	{
		for (const auto *str :
		     { &listing.seller_account, &listing.seller_name, &listing.winner_name,
		       &listing.object_short, &listing.id_keywords, &listing.object_info })
			total = physical_sum(total, physical_string_heap(*str));
		total = physical_sum(total, listing.object_blob.capacity());
		total = physical_sum(total, physical_product(listing.items.capacity(),
							     sizeof(auction_item)));
	}
	return total;
}

struct physical_decoder : decoder
{
	physical_scope scope;
	size_t *retained_heap;
	physical_decoder(const uint8_t *data, size_t size, physical_scope scope,
			 size_t *retained_heap)
		: decoder{ data, size }
		, scope(scope)
		, retained_heap(retained_heap)
	{
	}
	bool string(std::string *value, size_t maximum)
	{
		uint32_t count = 0;
		if (!value || !number(&count) || count > maximum || offset > size ||
		    size - offset < count)
			return false;
		const size_t inline_frame = sizeof(std::string) + sizeof(uint32_t) +
					    sizeof(size_t) * 2 + sizeof(void *) * 3 +
					    physical_callback_frames;
		const size_t live = physical_sum(inline_frame, *retained_heap);
		scope.admit(physical_sum(live, count > 15 ? physical_sum(count, 1) : 0));
		std::string candidate(count, '\0');
		scope.admit(physical_sum(live, physical_string_heap(candidate)));
		std::copy_n(reinterpret_cast<const char *>(data + offset), count,
			    candidate.begin());
		const size_t previous_heap = physical_string_heap(*value);
		value->swap(candidate);
		if (previous_heap > *retained_heap)
			throw physical_failure{ ENOBUFS };
		*retained_heap =
			physical_sum(*retained_heap - previous_heap, physical_string_heap(*value));
		offset += count;
		return value->find('\0') == std::string::npos;
	}
};
bool physical_decode_catalog(const std::vector<uint8_t> &bytes, auction_catalog *catalog,
			     physical_scope scope)
{
	using operation_keys = std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>,
						  operation_id_hash>;
	const size_t frame =
		sizeof(auction_catalog) + sizeof(decoder) + sizeof(physical_decoder) +
		sizeof(operation_keys) + sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
		sizeof(std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES>) + sizeof(std::string) +
		sizeof(uint64_t) + sizeof(uint32_t) * 6 + sizeof(uint16_t) + sizeof(uint8_t) * 2 +
		sizeof(size_t) * 6 + sizeof(void *) * 8 + physical_callback_frames;
	scope.admit(frame);

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
	auction_catalog decoded;
	size_t decoded_heap = 0;
	physical_decoder payload{ payload_bytes, payload_size, scope.nested(frame), &decoded_heap };
	decoded.revision = revision;
	uint32_t listing_count = 0, money_count = 0, operation_count = 0;
	if (!payload.number(&listing_count) || listing_count > catalog_maximum_listings)
		return false;
	try
	{
		scope.admit(physical_sum(physical_sum(frame, decoded_heap),
					 physical_product(listing_count, sizeof(auction_listing))));
		decoded.listings.resize(listing_count);
		decoded_heap =
			physical_sum(decoded_heap, physical_product(decoded.listings.capacity(),
								    sizeof(auction_listing)));
		scope.admit(physical_sum(frame, decoded_heap));
	}
	catch (const std::bad_alloc &)
	{
		throw;
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
			scope.admit(physical_sum(physical_sum(frame, decoded_heap),
						 physical_product(blob_size, sizeof(uint8_t))));
			listing.object_blob.resize(blob_size);
			decoded_heap = physical_sum(decoded_heap,
						    physical_product(listing.object_blob.capacity(),
								     sizeof(uint8_t)));
			scope.admit(physical_sum(frame, decoded_heap));
		}
		catch (const std::bad_alloc &)
		{
			throw;
		}
		if (!payload.raw(listing.object_blob.data(), blob_size) ||
		    !payload.number(&item_count) || !item_count ||
		    item_count > AUCTION_COMMAND_MAX_ITEMS)
			return false;
		try
		{
			scope.admit(
				physical_sum(physical_sum(frame, decoded_heap),
					     physical_product(item_count, sizeof(auction_item))));
			listing.items.resize(item_count);
			decoded_heap = physical_sum(decoded_heap,
						    physical_product(listing.items.capacity(),
								     sizeof(auction_item)));
			scope.admit(physical_sum(frame, decoded_heap));
		}
		catch (const std::bad_alloc &)
		{
			throw;
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
		scope.admit(physical_sum(physical_sum(frame, decoded_heap),
					 listing.seller_account.size() > 15 ?
						 physical_sum(listing.seller_account.size(), 1) :
						 0));
		std::string canonical(listing.seller_account.size(), char{});
		scope.admit(physical_sum(physical_sum(frame, decoded_heap),
					 physical_string_heap(canonical)));
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
		scope.admit(physical_sum(physical_sum(frame, decoded_heap),
					 physical_product(money_count, sizeof(money_pickup))));
		decoded.money.resize(money_count);
		decoded_heap = physical_sum(decoded_heap, physical_product(decoded.money.capacity(),
									   sizeof(money_pickup)));
		scope.admit(physical_sum(frame, decoded_heap));
	}
	catch (const std::bad_alloc &)
	{
		throw;
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
		scope.admit(
			physical_sum(physical_sum(frame, decoded_heap),
				     physical_product(operation_count, sizeof(auction_operation))));
		decoded.operations.resize(operation_count);
		decoded_heap =
			physical_sum(decoded_heap, physical_product(decoded.operations.capacity(),
								    sizeof(auction_operation)));
		scope.admit(physical_sum(frame, decoded_heap));
	}
	catch (const std::bad_alloc &)
	{
		throw;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> result = {};
	std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, operation_id_hash>
		operation_ids;
	try
	{
		physical_hash_reserve(operation_ids, operation_count, scope,
				      physical_sum(frame, decoded_heap));
	}
	catch (const std::bad_alloc &)
	{
		throw;
	}
	try
	{
		for (auto &operation : decoded.operations)
		{
			physical_hash_insert_admit(operation_ids, scope,
						   physical_sum(physical_sum(frame, decoded_heap),
								physical_hash_heap(operation_ids)));
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
		throw;
	}
	if (payload.offset != payload.size)
		return false;
	if (physical_catalog_heap(decoded) != decoded_heap)
		throw physical_failure{ ENOBUFS };
	scope.admit(physical_sum(frame, decoded_heap));
	*catalog = std::move(decoded);
	return true;
}

flatfile_read_result physical_load_catalog(const std::string &root, auction_catalog *catalog,
					   physical_scope scope)
{
	const size_t frame = sizeof(std::string) * 2 + sizeof(std::vector<uint8_t>) +
			     sizeof(size_t) * 3 + sizeof(flatfile_read_result) + sizeof(int) +
			     sizeof(void *) * 2 + physical_callback_frames;
	const size_t length = physical_sum(root.size(), sizeof("/domains") - 1);
	const size_t name_length = std::strlen(catalog_filename);
	size_t request = frame;
	if (length > 15)
		request = physical_sum(request, physical_sum(length, 1));
	if (name_length > 15)
		request = physical_sum(request, physical_sum(name_length, 1));
	scope.admit(request);
	std::string directory(length, '\0'), name(catalog_filename);
	std::copy(root.begin(), root.end(), directory.begin());
	std::copy_n("/domains", sizeof("/domains") - 1, directory.begin() + root.size());
	std::vector<uint8_t> bytes;
	const size_t live = physical_sum(frame, physical_sum(physical_string_heap(directory),
							     physical_string_heap(name)));
	scope.admit(live);
	errno = 0;
	const auto result = flatfile_read_bounded(directory, name, catalog_maximum_bytes, &bytes,
						  physical_reservation::callback, scope.owner,
						  scope.nested(live).outer);
	const int read_error = errno;
	if (result != flatfile_read_result::ok && result != flatfile_read_result::not_found)
	{
		if (read_error == ENOBUFS || read_error == EOVERFLOW || read_error == ENOSPC)
			throw physical_failure{ ENOBUFS };
		if (read_error == ENOMEM)
			throw physical_failure{ ENOMEM };
		if (read_error == ENOTSUP)
			throw physical_failure{ ENOTSUP };
	}
	if (scope.owner->refused)
		throw physical_failure{ ENOBUFS };
	if (result == flatfile_read_result::not_found)
	{
		*catalog = {};
		return result;
	}
	if (result != flatfile_read_result::ok)
		return result;
	scope.admit(physical_sum(live, bytes.capacity()));
	return physical_decode_catalog(bytes, catalog,
				       scope.nested(physical_sum(live, bytes.capacity()))) ?
		       flatfile_read_result::ok :
		       flatfile_read_result::invalid;
}
#endif
}

unsigned int flatfile_native_mobile_birth_ordinary_auction_physical_storage::verify_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_catalog_physical_absence *output,
	flatfile_scratch_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (root.empty() || !output || !reserve || !lock.matches(root))
		return EINVAL;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)original;
	(void)context;
	(void)outer;
	return ENOTSUP;
#else
	physical_reservation reservation{ reserve, context };
	physical_scope scope{ &reservation, outer };
	try
	{
		const size_t frame =
			sizeof(quest_mobile_native_image) +
			sizeof(std::vector<native_mobile_birth_item_recipe>) +
			sizeof(native_mobile_birth_cash_role_recipe) +
			sizeof(economic_frozen_intent) + sizeof(std::vector<uint64_t>) +
			sizeof(auction_catalog) +
			sizeof(flatfile_native_mobile_birth_ordinary_catalog_physical_absence) +
			sizeof(std::span<const uint8_t>) + sizeof(size_t) * 7 + sizeof(void *) * 6 +
			sizeof(economic_accounting_error) * 4 + sizeof(unsigned int) * 4 +
			sizeof(flatfile_read_result) + physical_callback_frames +
			physical_codec_status_frames;
		scope.admit(frame);
		const auto recovery_code = native_mobile_birth_cash_role_recovery_validate_bounded(
			original, physical_reservation::callback, &reservation,
			scope.nested(frame).outer);
		const auto recovery_status = physical_codec_status(recovery_code, reservation);
		if (recovery_status)
			return recovery_status;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		size_t image_heap = 0, recipe_heap = 0;
		const auto decoded = native_mobile_birth_cash_role_command_decode_bounded(
			original.command, &image, &recipes, &role, physical_reservation::callback,
			&reservation, scope.nested(frame).outer, &image_heap, &recipe_heap);
		const auto command_status = physical_codec_status(decoded, reservation);
		if (command_status)
			return command_status;
		if (role.role != native_mobile_birth_cash_role::ordinary_wallet)
			return EINVAL;
		size_t retained = physical_sum(frame, physical_sum(image_heap, recipe_heap));
		scope.admit(retained);
		const std::span<const uint8_t> intent_wire{ original.command.accounting_intent };
		const auto intent_code = economic_intent_decode_bounded(
			intent_wire, &intent, physical_reservation::callback, &reservation,
			scope.nested(retained).outer);
		const auto intent_status = physical_codec_status(intent_code, reservation);
		if (intent_status)
			return intent_status;
		retained = physical_sum(retained, intent.admission.facts.capacity());
		scope.admit(retained);
		const auto binding_code = economic_intent_verify_binding_bounded(
			original.command, intent, physical_reservation::callback, &reservation,
			scope.nested(retained).outer);
		const auto binding_status = physical_codec_status(binding_code, reservation);
		if (binding_status)
			return binding_status;
		if (!intent.admission.metadata.source_event)
			return EINVAL;
		std::vector<uint64_t> born;
		scope.admit(physical_sum(retained,
					 physical_product(image.items.size(), sizeof(uint64_t))));
		born.reserve(image.items.size());
		retained =
			physical_sum(retained, physical_product(born.capacity(), sizeof(uint64_t)));
		scope.admit(retained);
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return EINVAL;
		auction_catalog catalog;
		const auto loaded = physical_load_catalog(root, &catalog, scope.nested(retained));
		scope.admit(physical_sum(retained, physical_catalog_heap(catalog)));
		if (loaded != flatfile_read_result::ok && loaded != flatfile_read_result::not_found)
			return loaded == flatfile_read_result::io_error ? EIO : EILSEQ;
		flatfile_native_mobile_birth_ordinary_catalog_physical_absence observed;
		observed.present = loaded == flatfile_read_result::ok;
		observed.file_revision = catalog.revision;
		observed.catalog_revision = catalog.revision;
		observed.listings = catalog.listings.size();
		for (const auto &listing : catalog.listings)
			for (const auto &item : listing.items)
			{
				// Original SQL auction_item_custody absence is not restricted by
				// listing status, claim PID or claimed state. Retained rows count.
				if (std::binary_search(born.begin(), born.end(), item.uid))
					return EEXIST;
				++observed.item_rows;
			}
		if (!lock.matches(root))
			return EINVAL;
		*output = observed;
		return 0;
	}
	catch (const physical_failure &failure)
	{
		return failure.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
#endif
}
