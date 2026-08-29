#include "flatfile_corpse_repository.h"

#include "corpse_lifecycle_command.h"
#include "flatfile_artifact_repository.h"
#include "flatfile_authority_transaction.h"
#include "flatfile_item_repository.h"
#include "flatfile_store.h"
#include "flatfile_world_item_repository.h"

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
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'C', 'O', 'P', 'S', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 64 * 1024 * 1024;
constexpr size_t operation_maximum = 262144;
constexpr const char *catalog_filename = "corpse_operation_catalog";

struct corpse_operation
{
	critical_operation_id operation_id = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest = {};
	unsigned int result_code = 0;
	uint64_t durable_revision = 0;
	uint16_t result_size = 0;
	std::array<uint8_t, CORPSE_LIFECYCLE_RESULT_BYTES> result_payload = {};
};

struct operation_catalog
{
	uint64_t revision = 1;
	std::vector<corpse_operation> operations;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		if (!valid)
			return;
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
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}

	bool raw(uint8_t *output, size_t count)
	{
		if (!output || offset > size || size - offset < count)
			return false;
		memcpy(output, data + offset, count);
		offset += count;
		return true;
	}
};

std::mutex corpse_mutex;

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool operation_less(const corpse_operation &left, const corpse_operation &right)
{
	return left.operation_id.bytes < right.operation_id.bytes;
}

bool valid_operation(const corpse_operation &operation)
{
	if (critical_operation_id_is_zero(operation.operation_id) || !operation.durable_revision ||
	    operation.result_size > operation.result_payload.size() ||
	    (operation.result_code && operation.result_size) ||
	    (!operation.result_code && !operation.result_size))
		return false;
	if (!operation.result_code)
	{
		corpse_lifecycle_result result = {};
		if (!corpse_lifecycle_command_decode_result(operation.result_payload.data(),
							    operation.result_size, &result))
			return false;
	}
	return true;
}

bool valid_catalog(const operation_catalog &catalog)
{
	if (!catalog.revision || catalog.operations.size() > operation_maximum ||
	    !std::is_sorted(catalog.operations.begin(), catalog.operations.end(), operation_less))
		return false;
	for (size_t index = 0; index < catalog.operations.size(); ++index)
		if (!valid_operation(catalog.operations[index]) ||
		    (index &&
		     !operation_less(catalog.operations[index - 1], catalog.operations[index])))
			return false;
	return true;
}

bool encode_catalog(const operation_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.operations.size());
	for (const auto &operation : catalog.operations)
	{
		payload.raw(operation.operation_id.bytes.data(),
			    operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number<uint32_t>(operation.result_code);
		payload.number(operation.durable_revision);
		payload.number(operation.result_size);
		payload.raw(operation.result_payload.data(), operation.result_size);
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
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || version != catalog_version ||
	    !header.number(&payload_size) || !header.number(&revision) || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > operation_maximum)
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
		if (!payload.raw(operation.operation_id.bytes.data(),
				 operation.operation_id.bytes.size()) ||
		    !payload.raw(operation.command_digest.data(),
				 operation.command_digest.size()) ||
		    !payload.number(&operation.result_code) ||
		    !payload.number(&operation.durable_revision) ||
		    !payload.number(&operation.result_size) ||
		    operation.result_size > operation.result_payload.size() ||
		    !payload.raw(operation.result_payload.data(), operation.result_size))
			return false;
	if (payload.offset != payload.size || !valid_catalog(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

enum class load_result
{
	ok,
	not_found,
	invalid,
	io_error,
};

load_result load_catalog(const std::string &root, operation_catalog *catalog, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return load_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return load_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "corpse operation catalog is corrupt";
		return load_result::invalid;
	}
	return load_result::ok;
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

critical_apply_result stored_result(critical_apply_outcome outcome,
				    const corpse_operation &operation)
{
	critical_apply_result result = { outcome, operation.durable_revision,
					 operation.result_code };
	result.result_size = operation.result_size;
	std::copy_n(operation.result_payload.begin(), operation.result_size,
		    result.result_payload.begin());
	return result;
}

unsigned int result_code(flatfile_world_item_result result)
{
	switch (result)
	{
	case flatfile_world_item_result::conflict:
		return ESTALE;
	case flatfile_world_item_result::not_found:
		return ENOENT;
	case flatfile_world_item_result::not_empty:
		return ENOTEMPTY;
	default:
		return EILSEQ;
	}
}
} // namespace

critical_apply_result flatfile_corpse_repository_apply(const std::string &root,
						       const critical_command &command)
{
	corpse_lifecycle_payload payload = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !corpse_lifecycle_command_decode_payload(command, &payload) ||
	    !command_digest(command, &digest))
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	std::lock_guard<std::mutex> guard(corpse_mutex);
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
	operation_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded == load_result::io_error)
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	if (loaded == load_result::invalid)
		return { critical_apply_outcome::terminal_failure, 0, EILSEQ };
	if (loaded == load_result::not_found)
		catalog = {};
	corpse_operation key = {};
	key.operation_id = command.operation_id;
	auto existing = std::lower_bound(catalog.operations.begin(), catalog.operations.end(), key,
					 operation_less);
	if (existing != catalog.operations.end() &&
	    critical_operation_id_equal(existing->operation_id, command.operation_id))
	{
		if (CRYPTO_memcmp(existing->command_digest.data(), digest.data(), digest.size()))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EEXIST };
		return stored_result(existing->result_code ?
					     critical_apply_outcome::terminal_failure :
					     critical_apply_outcome::already_applied,
				     *existing);
	}
	if (catalog.operations.size() >= operation_maximum || catalog.revision == UINT64_MAX)
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	flatfile_corpse_lifecycle_mutation mutation;
	flatfile_corpse_release_mutation release;
	flatfile_item_corpse_release_mutation release_items;
	flatfile_artifact_transfer_mutation release_artifacts;
	const bool releases_custody = payload.action == corpse_lifecycle_action::release;
	const auto prepared =
		releases_custody ?
			flatfile_world_item_prepare_corpse_release(root, authority, payload,
								   &release, &error) :
			flatfile_world_item_prepare_corpse_lifecycle(root, authority, payload,
								     &mutation, &error);
	if (prepared == flatfile_world_item_result::io_error)
		return { critical_apply_outcome::retryable_failure, catalog.revision, EIO };
	if (prepared == flatfile_world_item_result::invalid ||
	    prepared == flatfile_world_item_result::already_exists ||
	    prepared == flatfile_world_item_result::unchanged ||
	    (prepared == flatfile_world_item_result::ok &&
	     !(releases_custody ? release.catalog_revision : mutation.catalog_revision)))
		return { critical_apply_outcome::terminal_failure, catalog.revision, EILSEQ };
	bool include_release_artifacts = false;
	if (prepared == flatfile_world_item_result::ok && releases_custody)
	{
		const auto item_prepared = flatfile_item_repository_prepare_corpse_release(
			root, authority, payload, release.expected_items, &release_items, &error);
		if (item_prepared != flatfile_item_repository_result::ok)
			return { item_prepared == flatfile_item_repository_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 item_prepared ==
							 flatfile_item_repository_result::io_error ?
						 EIO :
						 EILSEQ) };
		if (release_items.room_owner_revision != release.room_revision)
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		const auto artifact_prepared = flatfile_artifact_prepare_corpse_release(
			root, authority, payload.owner_pid, payload.room_vnum,
			command.accepted_at_usec, release.items, &release_artifacts, &error);
		if (artifact_prepared != flatfile_artifact_result::ok &&
		    artifact_prepared != flatfile_artifact_result::unchanged)
			return { artifact_prepared == flatfile_artifact_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 artifact_prepared == flatfile_artifact_result::io_error ?
						 EIO :
						 EILSEQ) };
		include_release_artifacts = artifact_prepared == flatfile_artifact_result::ok;
	}
	corpse_operation operation = {};
	operation.operation_id = command.operation_id;
	operation.command_digest = digest;
	operation.result_code = prepared == flatfile_world_item_result::ok ? 0 :
									     result_code(prepared);
	operation.durable_revision =
		prepared == flatfile_world_item_result::ok ?
			(releases_custody ? release.catalog_revision : mutation.catalog_revision) :
			catalog.revision + 1;
	if (!operation.result_code)
	{
		corpse_lifecycle_result result = {};
		result.owner_pid = payload.owner_pid;
		result.save_id = payload.save_id;
		result.action = payload.action;
		result.corpse_revision = mutation.corpse_revision;
		result.catalog_revision = releases_custody ? release.catalog_revision :
							     mutation.catalog_revision;
		if (releases_custody)
		{
			result.corpse_owner_revision = release_items.corpse_owner_revision;
			result.room_owner_revision = release_items.room_owner_revision;
			result.max_item_revision = release_items.max_item_revision;
			result.item_count = static_cast<uint32_t>(release_items.item_count);
		}
		if (!corpse_lifecycle_command_encode_result(result, &operation.result_payload))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EILSEQ };
		operation.result_size = CORPSE_LIFECYCLE_RESULT_BYTES;
	}
	try
	{
		catalog.operations.insert(existing, operation);
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return { critical_apply_outcome::terminal_failure, catalog.revision, EILSEQ };
	std::vector<flatfile_authority_after_image> images;
	try
	{
		if (!operation.result_code)
		{
			if (releases_custody)
			{
				images.push_back(std::move(release_items.after_image));
				images.push_back(std::move(release.after_image));
				if (include_release_artifacts)
					images.push_back(std::move(release_artifacts.after_image));
			}
			else
				images.push_back(std::move(mutation.after_image));
		}
		images.push_back({ catalog_filename, std::move(encoded) });
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
	return stored_result(operation.result_code ? critical_apply_outcome::terminal_failure :
						     critical_apply_outcome::applied,
			     operation);
}
