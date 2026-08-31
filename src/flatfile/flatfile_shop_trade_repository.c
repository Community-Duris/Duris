#include "flatfile/flatfile_shop_trade_repository.h"

#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_store.h"
#include "shop_trade_command.h"

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
#include <utility>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'S', 'H', 'T', 'X', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_operations = 262144;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr const char *catalog_filename = "shop_trade_operations";
std::mutex repository_mutex;

struct operation_record
{
	critical_operation_id operation_id = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest = {};
	unsigned int result_code = 0;
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> result = {};
};

struct operation_catalog
{
	uint64_t revision = 0;
	std::vector<operation_record> operations;
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
		if (!output || offset > size || count > size - offset)
			return false;
		memcpy(output, data + offset, count);
		offset += count;
		return true;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool encode_catalog(const operation_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || catalog.operations.size() > catalog_maximum_operations)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.operations.size());
	for (const auto &operation : catalog.operations)
	{
		shop_trade_result decoded = {};
		if (critical_operation_id_is_zero(operation.operation_id) ||
		    !shop_trade_command_decode_result(operation.result.data(),
						      operation.result.size(), &decoded))
			return false;
		payload.raw(operation.operation_id.bytes.data(),
			    operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number<uint32_t>(operation.result_code);
		payload.raw(operation.result.data(), operation.result.size());
	}
	if (!payload.valid || payload.bytes.size() > catalog_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(catalog_magic.data(), catalog_magic.size());
	file.number(catalog_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > catalog_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, operation_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + catalog_magic.size(), bytes.size() - catalog_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != catalog_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > catalog_maximum_operations)
		return false;
	operation_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.operations.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &operation : decoded.operations)
	{
		shop_trade_result result = {};
		uint32_t result_code = 0;
		if (!payload.raw(operation.operation_id.bytes.data(),
				 operation.operation_id.bytes.size()) ||
		    !payload.raw(operation.command_digest.data(),
				 operation.command_digest.size()) ||
		    !payload.number(&result_code) ||
		    !payload.raw(operation.result.data(), operation.result.size()) ||
		    critical_operation_id_is_zero(operation.operation_id) ||
		    !shop_trade_command_decode_result(operation.result.data(),
						      operation.result.size(), &result))
			return false;
		operation.result_code = result_code;
	}
	if (payload.offset != payload.size)
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_read_result load_catalog(const std::string &root, operation_catalog *catalog,
				  std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
	{
		*catalog = {};
		return flatfile_read_result::ok;
	}
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "shop trade operation catalog is corrupt";
		return loaded == flatfile_read_result::io_error ? flatfile_read_result::io_error :
								  flatfile_read_result::invalid;
	}
	return flatfile_read_result::ok;
}

critical_apply_result make_result(const operation_record &operation, uint64_t revision,
				  critical_apply_outcome success)
{
	critical_apply_result applied = { operation.result_code ?
						  critical_apply_outcome::terminal_failure :
						  success,
					  revision, operation.result_code };
	applied.result_size = operation.result.size();
	std::copy(operation.result.begin(), operation.result.end(), applied.result_payload.begin());
	return applied;
}

critical_apply_result repository_failure(bool io_error, unsigned int terminal_code)
{
	return { io_error ? critical_apply_outcome::retryable_failure :
			    critical_apply_outcome::terminal_failure,
		 0, io_error ? static_cast<unsigned int>(EIO) : terminal_code };
}
} // namespace

critical_apply_result flatfile_shop_trade_repository_apply(const std::string &root,
							   const critical_command &command)
{
	shop_trade_payload payload = {};
	std::vector<uint8_t> encoded_command;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !shop_trade_command_decode_payload(command, &payload) ||
	    critical_command_encode(command, &encoded_command) != critical_command_codec_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	SHA256(encoded_command.data(), encoded_command.size(), digest.data());
	std::lock_guard<std::mutex> guard(repository_mutex);
	flatfile_authority_lock lock;
	std::string error;
	if (!lock.acquire(root, &error))
		return repository_failure(true, EIO);
	const auto recovered = flatfile_authority_transaction_recover(root, lock, &error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return repository_failure(
			recovered == flatfile_authority_transaction_result::io_error, EILSEQ);
	operation_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_read_result::ok)
		return repository_failure(loaded == flatfile_read_result::io_error, EILSEQ);
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

	flatfile_wallet_mutation wallet;
	flatfile_item_shop_trade_mutation items;
	flatfile_shopkeeper_trade_mutation shop;
	flatfile_shop_trade_materialization_mutation materialization;
	unsigned int result_code = 0;
	const auto wallet_read = flatfile_player_domain_prepare_wallet(
		root, lock, payload.player_pid, payload.account_name.data(), payload.racewar,
		payload.expected_wallet_revision, payload.expected_bank_revision, 0, false, &wallet,
		&result_code, &error);
	if (wallet_read != flatfile_player_domain_result::ok)
		return repository_failure(
			wallet_read == flatfile_player_domain_result::io_error,
			wallet_read == flatfile_player_domain_result::not_found ? ENOENT : EILSEQ);
	if (!result_code)
	{
		const auto item_prepared = flatfile_item_repository_prepare_shop_trade(
			root, lock, payload, &items, &result_code, &error);
		if (item_prepared != flatfile_item_repository_result::ok)
			return repository_failure(
				item_prepared == flatfile_item_repository_result::io_error,
				item_prepared == flatfile_item_repository_result::not_found ?
					ENOENT :
					EILSEQ);
	}
	if (!result_code)
	{
		const auto shop_prepared = flatfile_shopkeeper_prepare_trade(
			root, lock, payload, &shop, &result_code, &error);
		if (shop_prepared != flatfile_shopkeeper_result::ok)
			return repository_failure(
				shop_prepared == flatfile_shopkeeper_result::io_error,
				shop_prepared == flatfile_shopkeeper_result::not_found ? ENOENT :
											 EILSEQ);
	}
	if (!result_code && payload.action != shop_trade_action::discard_invalid)
	{
		const auto prepared = flatfile_shop_trade_materialization_prepare(
			root, lock, command.operation_id, payload, &materialization, &error);
		if (prepared != flatfile_shop_trade_materialization_result::ok)
			return repository_failure(
				prepared == flatfile_shop_trade_materialization_result::io_error,
				EILSEQ);
	}
	if (!result_code && payload.action != shop_trade_action::discard_invalid)
	{
		const int64_t value_delta =
			payload.action == shop_trade_action::buy_existing ||
					payload.action == shop_trade_action::buy_produced ?
				-payload.price :
				payload.price;
		const auto wallet_prepared = flatfile_player_domain_prepare_wallet(
			root, lock, payload.player_pid, payload.account_name.data(),
			payload.racewar, payload.expected_wallet_revision,
			payload.expected_bank_revision, value_delta, true, &wallet, &result_code,
			&error);
		if (wallet_prepared != flatfile_player_domain_result::ok)
			return repository_failure(
				wallet_prepared == flatfile_player_domain_result::io_error,
				wallet_prepared == flatfile_player_domain_result::not_found ?
					ENOENT :
					EILSEQ);
	}
	shop_trade_result result = {};
	result.action = payload.action;
	result.wallet = wallet.wallet;
	result.bank = wallet.bank;
	result.wallet_revision = wallet.wallet_revision;
	result.bank_revision = wallet.bank_revision;
	if (!result_code)
	{
		result.shop_revision = shop.shop_revision;
		result.player_owner_revision = items.player_owner_revision;
		result.counterparty_owner_revision = items.counterparty_owner_revision;
		result.item_count = items.item_count;
		result.item_uids = items.item_uids;
		result.item_revisions = items.item_revisions;
	}
	operation_record operation = {};
	operation.operation_id = command.operation_id;
	operation.command_digest = digest;
	operation.result_code = result_code;
	if (!shop_trade_command_encode_result(result, &operation.result))
		return { critical_apply_outcome::terminal_failure, catalog.revision, EBADMSG };
	try
	{
		catalog.operations.push_back(operation);
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
		if (!result_code)
		{
			images.push_back(std::move(shop.after_image));
			images.push_back(std::move(items.after_image));
			if (payload.action != shop_trade_action::discard_invalid)
			{
				images.push_back(std::move(materialization.after_image));
				for (auto &image : wallet.after_images)
					images.push_back(std::move(image));
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision - 1, ENOMEM };
	}
	const auto committed = flatfile_authority_transaction_commit(root, lock, images, &error);
	if (committed != flatfile_authority_transaction_result::ok)
		return repository_failure(
			committed == flatfile_authority_transaction_result::io_error, EILSEQ);
	return make_result(catalog.operations.back(), catalog.revision,
			   critical_apply_outcome::applied);
}
