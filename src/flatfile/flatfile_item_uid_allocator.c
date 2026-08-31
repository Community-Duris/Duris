#include "flatfile/flatfile_item_uid_allocator.h"

#include "flatfile/flatfile_store.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <mutex>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <vector>

namespace
{
constexpr uint32_t allocator_format_version = 1;
constexpr std::array<uint8_t, 8> allocator_magic = { 'D', 'U', 'R', 'U', 'I', 'D', 0, 0 };
constexpr size_t allocator_file_size =
	allocator_magic.size() + sizeof(uint32_t) * 2 + sizeof(uint64_t) * 2 + SHA256_DIGEST_LENGTH;
constexpr const char *allocator_filename = "item_uid_allocator";
constexpr const char *allocator_lock_filename = ".item-uid-allocator.lock";
std::mutex allocator_mutex;

struct authority_lock
{
	int fd = -1;
	~authority_lock() { flatfile_lock_release(fd); }
};

struct encoder
{
	std::vector<uint8_t> bytes;

	template <typename T> void number(T value)
	{
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		for (size_t index = 0; index < sizeof(T); ++index)
		{
			bytes.push_back(static_cast<uint8_t>(bits & 0xff));
			bits >>= 8;
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
};

std::string metadata_directory(const std::string &root)
{
	return root + "/metadata";
}

flatfile_item_uid_result load_allocator(const std::string &root, uint64_t *next_uid,
					uint64_t *revision, std::string *error)
{
	if (!next_uid || !revision)
		return flatfile_item_uid_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read = flatfile_read(
		metadata_directory(root), allocator_filename, allocator_file_size, &bytes, error);
	if (read == flatfile_read_result::not_found)
	{
		*next_uid = 1;
		*revision = 0;
		return flatfile_item_uid_result::ok;
	}
	if (read == flatfile_read_result::invalid)
		return flatfile_item_uid_result::invalid;
	if (read != flatfile_read_result::ok)
		return flatfile_item_uid_result::io_error;
	if (bytes.size() != allocator_file_size ||
	    memcmp(bytes.data(), allocator_magic.data(), allocator_magic.size()))
		return flatfile_item_uid_result::invalid;
	decoder header{ bytes.data() + allocator_magic.size(),
			bytes.size() - allocator_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t stored_revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&stored_revision) || version != allocator_format_version ||
	    payload_size != sizeof(uint64_t) || !stored_revision)
		return flatfile_item_uid_result::invalid;
	const uint8_t *stored_digest =
		bytes.data() + allocator_magic.size() + sizeof(uint32_t) * 2 + sizeof(uint64_t);
	const uint8_t *payload = stored_digest + SHA256_DIGEST_LENGTH;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_item_uid_result::invalid;
	decoder value{ payload, payload_size };
	uint64_t stored_next = 0;
	if (!value.number(&stored_next) || !stored_next || value.offset != value.size)
		return flatfile_item_uid_result::invalid;
	*next_uid = stored_next;
	*revision = stored_revision;
	return flatfile_item_uid_result::ok;
}

bool publish_allocator(const std::string &root, uint64_t next_uid, uint64_t revision,
		       std::string *error)
{
	encoder payload;
	payload.number(next_uid);
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.bytes.data(), payload.bytes.size(), digest);
	encoder file;
	file.bytes.insert(file.bytes.end(), allocator_magic.begin(), allocator_magic.end());
	file.number<uint32_t>(allocator_format_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(revision);
	file.bytes.insert(file.bytes.end(), digest, digest + sizeof(digest));
	file.bytes.insert(file.bytes.end(), payload.bytes.begin(), payload.bytes.end());
	return file.bytes.size() == allocator_file_size &&
	       flatfile_atomic_write(metadata_directory(root), allocator_filename, file.bytes,
				     error);
}
} // namespace

flatfile_item_uid_result flatfile_item_uid_reserve(const std::string &root, uint64_t count,
						   uint64_t *first, std::string *error)
{
	if (root.empty() || !count || !first)
		return flatfile_item_uid_result::invalid;
	std::lock_guard<std::mutex> guard(allocator_mutex);
	authority_lock authority;
	if (!flatfile_lock_acquire(metadata_directory(root), allocator_lock_filename, &authority.fd,
				   error))
		return flatfile_item_uid_result::io_error;
	uint64_t next_uid = 0, revision = 0;
	const flatfile_item_uid_result loaded = load_allocator(root, &next_uid, &revision, error);
	if (loaded != flatfile_item_uid_result::ok)
		return loaded;
	if (next_uid > std::numeric_limits<uint64_t>::max() - count ||
	    revision == std::numeric_limits<uint64_t>::max())
		return flatfile_item_uid_result::exhausted;
	const uint64_t end = next_uid + count;
	if (!publish_allocator(root, end, revision + 1, error))
		return flatfile_item_uid_result::io_error;
	*first = next_uid;
	return flatfile_item_uid_result::ok;
}

flatfile_item_uid_result flatfile_item_uid_current(const std::string &root, uint64_t *next_uid,
						   uint64_t *revision, std::string *error)
{
	if (root.empty())
		return flatfile_item_uid_result::invalid;
	std::lock_guard<std::mutex> guard(allocator_mutex);
	return load_allocator(root, next_uid, revision, error);
}
