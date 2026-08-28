#include "flatfile_offline_message_repository.h"

#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>

namespace
{
constexpr std::array<uint8_t, 8> message_magic = { 'D', 'U', 'R', 'O', 'F', 'F', 'L', 0 };
constexpr uint32_t message_version = 1;
constexpr size_t message_file_maximum_bytes = 32 * 1024 * 1024;
constexpr size_t message_count_maximum = 4096;

struct message_catalog
{
	uint64_t revision = 0;
	std::vector<flatfile_offline_message_record> messages;
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
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

std::string message_filename(uint32_t pid)
{
	return "offline_messages_" + std::to_string(pid);
}

bool id_is_zero(const flatfile_offline_message_id &id)
{
	return std::all_of(id.begin(), id.end(), [](uint8_t byte) { return byte == 0; });
}

bool encode_catalog(const message_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || catalog.messages.size() > message_count_maximum)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.messages.size());
	for (const auto &message : catalog.messages)
	{
		if (id_is_zero(message.id) || !message.created_at || message.text.empty() ||
		    message.text.size() > FLATFILE_OFFLINE_MESSAGE_MAX_BYTES ||
		    message.text.find('\0') != std::string::npos)
			return false;
		payload.raw(message.id.data(), message.id.size());
		payload.number(message.created_at);
		payload.number<uint32_t>(message.text.size());
		payload.raw(reinterpret_cast<const uint8_t *>(message.text.data()),
			    message.text.size());
	}
	if (!payload.valid || payload.bytes.size() > message_file_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(message_magic.data(), message_magic.size());
	file.number(message_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(std::max(catalog.revision, UINT64_C(1)));
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > message_file_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, message_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), message_magic.data(), message_magic.size()))
		return false;
	decoder header{ bytes.data() + message_magic.size(), bytes.size() - message_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != message_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *expected_digest = bytes.data() + 24;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > message_count_maximum)
		return false;
	message_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.messages.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &message : decoded.messages)
	{
		uint32_t text_size = 0;
		if (!payload.raw(message.id.data(), message.id.size()) ||
		    !payload.number(&message.created_at) || !payload.number(&text_size) ||
		    id_is_zero(message.id) || !message.created_at || !text_size ||
		    text_size > FLATFILE_OFFLINE_MESSAGE_MAX_BYTES ||
		    payload.size - payload.offset < text_size)
			return false;
		try
		{
			message.text.assign(
				reinterpret_cast<const char *>(payload.data + payload.offset),
				text_size);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		payload.offset += text_size;
		if (message.text.find('\0') != std::string::npos)
			return false;
	}
	if (payload.offset != payload.size ||
	    !std::is_sorted(decoded.messages.begin(), decoded.messages.end(),
			    [](const auto &left, const auto &right) { return left.id < right.id; }))
		return false;
	for (size_t index = 1; index < decoded.messages.size(); ++index)
		if (decoded.messages[index - 1].id == decoded.messages[index].id)
			return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_offline_message_result load_catalog(const std::string &root, uint32_t pid,
					     message_catalog *catalog, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto read = flatfile_read(domains_directory(root), message_filename(pid),
					message_file_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
	{
		*catalog = {};
		return flatfile_offline_message_result::not_found;
	}
	if (read == flatfile_read_result::io_error)
		return flatfile_offline_message_result::io_error;
	if (read != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "offline message catalog is corrupt";
		return flatfile_offline_message_result::invalid;
	}
	return flatfile_offline_message_result::ok;
}

flatfile_offline_message_result
acquire_and_recover(const std::string &root, flatfile_authority_lock *lock, std::string *error)
{
	if (root.empty() || !lock || !lock->acquire(root, error))
		return flatfile_offline_message_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, *lock, error);
	if (recovered == flatfile_authority_transaction_result::ok)
		return flatfile_offline_message_result::ok;
	return recovered == flatfile_authority_transaction_result::io_error ?
		       flatfile_offline_message_result::io_error :
		       flatfile_offline_message_result::invalid;
}

flatfile_offline_message_result publish(const std::string &root, uint32_t pid,
					message_catalog *catalog, std::string *error)
{
	if (catalog->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_offline_message_result::full;
	++catalog->revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(*catalog, &bytes))
		return flatfile_offline_message_result::full;
	return flatfile_atomic_write(domains_directory(root), message_filename(pid), bytes, error) ?
		       flatfile_offline_message_result::ok :
		       flatfile_offline_message_result::io_error;
}
} // namespace

flatfile_offline_message_result
flatfile_offline_message_enqueue(const std::string &root, uint32_t pid,
				 const flatfile_offline_message_id &id, const std::string &text,
				 std::string *error)
{
	if (!pid || id_is_zero(id) || text.empty() ||
	    text.size() > FLATFILE_OFFLINE_MESSAGE_MAX_BYTES ||
	    text.find('\0') != std::string::npos)
		return flatfile_offline_message_result::invalid;
	flatfile_authority_lock lock;
	const auto acquired = acquire_and_recover(root, &lock, error);
	if (acquired != flatfile_offline_message_result::ok)
		return acquired;
	message_catalog catalog;
	const auto loaded = load_catalog(root, pid, &catalog, error);
	if (loaded != flatfile_offline_message_result::ok &&
	    loaded != flatfile_offline_message_result::not_found)
		return loaded;
	auto found = std::lower_bound(catalog.messages.begin(), catalog.messages.end(), id,
				      [](const auto &message, const auto &candidate)
				      { return message.id < candidate; });
	if (found != catalog.messages.end() && found->id == id)
		return found->text == text ? flatfile_offline_message_result::ok :
					     flatfile_offline_message_result::conflict;
	if (catalog.messages.size() >= message_count_maximum)
		return flatfile_offline_message_result::full;
	try
	{
		catalog.messages.insert(found, { id, static_cast<uint64_t>(time(nullptr)), text });
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_offline_message_result::io_error;
	}
	return publish(root, pid, &catalog, error);
}

flatfile_offline_message_result
flatfile_offline_message_list(const std::string &root, uint32_t pid,
			      std::vector<flatfile_offline_message_record> *messages,
			      std::string *error)
{
	if (!pid || !messages)
		return flatfile_offline_message_result::invalid;
	flatfile_authority_lock lock;
	const auto acquired = acquire_and_recover(root, &lock, error);
	if (acquired != flatfile_offline_message_result::ok)
		return acquired;
	message_catalog catalog;
	const auto loaded = load_catalog(root, pid, &catalog, error);
	if (loaded == flatfile_offline_message_result::not_found)
	{
		messages->clear();
		return flatfile_offline_message_result::ok;
	}
	if (loaded != flatfile_offline_message_result::ok)
		return loaded;
	try
	{
		*messages = catalog.messages;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_offline_message_result::io_error;
	}
	return flatfile_offline_message_result::ok;
}

flatfile_offline_message_result
flatfile_offline_message_acknowledge(const std::string &root, uint32_t pid,
				     const flatfile_offline_message_id &id, std::string *error)
{
	if (!pid || id_is_zero(id))
		return flatfile_offline_message_result::invalid;
	flatfile_authority_lock lock;
	const auto acquired = acquire_and_recover(root, &lock, error);
	if (acquired != flatfile_offline_message_result::ok)
		return acquired;
	message_catalog catalog;
	const auto loaded = load_catalog(root, pid, &catalog, error);
	if (loaded == flatfile_offline_message_result::not_found)
		return flatfile_offline_message_result::not_found;
	if (loaded != flatfile_offline_message_result::ok)
		return loaded;
	auto found = std::lower_bound(catalog.messages.begin(), catalog.messages.end(), id,
				      [](const auto &message, const auto &candidate)
				      { return message.id < candidate; });
	if (found == catalog.messages.end() || found->id != id)
		return flatfile_offline_message_result::not_found;
	catalog.messages.erase(found);
	return publish(root, pid, &catalog, error);
}
