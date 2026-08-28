#include "flatfile_authority_transaction.h"

#include "flatfile_store.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>

namespace
{
constexpr std::array<uint8_t, 8> transaction_magic = { 'D', 'U', 'R', 'A', 'U', 'T', 'H', 0 };
constexpr uint32_t transaction_version = 1;
constexpr size_t transaction_maximum_images = 16;
constexpr size_t transaction_maximum_filename = 128;
constexpr size_t transaction_maximum_bytes = 256 * 1024 * 1024;
constexpr const char *transaction_filename = ".critical-authority-transaction";
constexpr const char *lock_filename = ".critical-authority.lock";
std::mutex authority_mutex;

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool safe_filename(const std::string &filename)
{
	if (filename.empty() || filename.size() > transaction_maximum_filename || filename == "." ||
	    filename == "..")
		return false;
	for (unsigned char character : filename)
		if (!((character >= 'a' && character <= 'z') ||
		      (character >= 'A' && character <= 'Z') ||
		      (character >= '0' && character <= '9') || character == '.' ||
		      character == '_' || character == '-'))
			return false;
	return filename != transaction_filename && filename != lock_filename;
}

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
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

bool encode_transaction(const std::vector<flatfile_authority_after_image> &images,
			std::vector<uint8_t> *bytes)
{
	if (!bytes || images.empty() || images.size() > transaction_maximum_images)
		return false;
	encoder payload;
	payload.number<uint16_t>(images.size());
	for (size_t index = 0; index < images.size(); ++index)
	{
		const auto &image = images[index];
		if (!safe_filename(image.filename) || image.bytes.empty() ||
		    image.bytes.size() > transaction_maximum_bytes)
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (images[prior].filename == image.filename)
				return false;
		payload.number<uint16_t>(image.filename.size());
		payload.raw(reinterpret_cast<const uint8_t *>(image.filename.data()),
			    image.filename.size());
		payload.number<uint32_t>(image.bytes.size());
		payload.raw(image.bytes.data(), image.bytes.size());
	}
	if (!payload.valid || payload.bytes.size() > transaction_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(transaction_magic.data(), transaction_magic.size());
	file.number(transaction_version);
	file.number<uint32_t>(payload.bytes.size());
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > transaction_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

flatfile_authority_transaction_result
decode_transaction(const std::vector<uint8_t> &bytes,
		   std::vector<flatfile_authority_after_image> *images)
{
	constexpr size_t header_size =
		transaction_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
	if (!images || bytes.size() < header_size ||
	    memcmp(bytes.data(), transaction_magic.data(), transaction_magic.size()))
		return flatfile_authority_transaction_result::invalid;
	decoder header{ bytes.data() + transaction_magic.size(),
			bytes.size() - transaction_magic.size() };
	uint32_t version = 0, payload_size = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    version != transaction_version || payload_size != bytes.size() - header_size)
		return flatfile_authority_transaction_result::invalid;
	const uint8_t *expected_digest = bytes.data() + transaction_magic.size() + 8;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return flatfile_authority_transaction_result::invalid;
	decoder payload{ payload_bytes, payload_size };
	uint16_t count = 0;
	if (!payload.number(&count) || !count || count > transaction_maximum_images)
		return flatfile_authority_transaction_result::invalid;
	try
	{
		images->clear();
		images->resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_authority_transaction_result::io_error;
	}
	for (size_t index = 0; index < images->size(); ++index)
	{
		uint16_t filename_size = 0;
		uint32_t image_size = 0;
		if (!payload.number(&filename_size) || !filename_size ||
		    filename_size > transaction_maximum_filename ||
		    payload.size - payload.offset < filename_size)
			return flatfile_authority_transaction_result::invalid;
		auto &image = (*images)[index];
		try
		{
			image.filename.assign(
				reinterpret_cast<const char *>(payload.data + payload.offset),
				filename_size);
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_authority_transaction_result::io_error;
		}
		payload.offset += filename_size;
		if (!safe_filename(image.filename) || !payload.number(&image_size) || !image_size ||
		    image_size > transaction_maximum_bytes ||
		    payload.size - payload.offset < image_size)
			return flatfile_authority_transaction_result::invalid;
		for (size_t prior = 0; prior < index; ++prior)
			if ((*images)[prior].filename == image.filename)
				return flatfile_authority_transaction_result::invalid;
		try
		{
			image.bytes.resize(image_size);
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_authority_transaction_result::io_error;
		}
		if (!payload.raw(image.bytes.data(), image.bytes.size()))
			return flatfile_authority_transaction_result::invalid;
	}
	return payload.offset == payload.size ? flatfile_authority_transaction_result::ok :
						flatfile_authority_transaction_result::invalid;
}
} // namespace

struct flatfile_authority_lock::state
{
	std::unique_lock<std::mutex> process_lock;
	int fd = -1;
	std::string root;

	state()
		: process_lock(authority_mutex, std::defer_lock)
	{
	}
	~state() { flatfile_lock_release(fd); }
};

flatfile_authority_lock::flatfile_authority_lock() noexcept
	: state_(new(std::nothrow) state)
{
}
flatfile_authority_lock::~flatfile_authority_lock() = default;

bool flatfile_authority_lock::acquire(const std::string &root, std::string *error)
{
	if (!state_ || state_->process_lock.owns_lock() || root.empty())
		return false;
	state_->process_lock.lock();
	if (flatfile_lock_acquire(domains_directory(root), lock_filename, &state_->fd, error))
	{
		state_->root = root;
		return true;
	}
	state_->process_lock.unlock();
	return false;
}

bool flatfile_authority_lock::owns(const std::string &root) const
{
	return state_ && state_->process_lock.owns_lock() && state_->fd >= 0 &&
	       state_->root == root;
}

bool flatfile_authority_lock::matches(const std::string &root) const
{
	return owns(root);
}

flatfile_authority_transaction_result
flatfile_authority_transaction_recover(const std::string &root, const flatfile_authority_lock &lock,
				       std::string *error)
{
	if (!lock.owns(root))
		return flatfile_authority_transaction_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read = flatfile_read(domains_directory(root),
							transaction_filename,
							transaction_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
		return flatfile_authority_transaction_result::ok;
	if (read == flatfile_read_result::invalid)
		return flatfile_authority_transaction_result::invalid;
	if (read != flatfile_read_result::ok)
		return flatfile_authority_transaction_result::io_error;
	std::vector<flatfile_authority_after_image> images;
	const auto decoded = decode_transaction(bytes, &images);
	if (decoded != flatfile_authority_transaction_result::ok)
		return decoded;
	for (const auto &image : images)
		if (!flatfile_atomic_write(domains_directory(root), image.filename, image.bytes,
					   error))
			return flatfile_authority_transaction_result::io_error;
	return flatfile_atomic_remove(domains_directory(root), transaction_filename, false, error) ?
		       flatfile_authority_transaction_result::ok :
		       flatfile_authority_transaction_result::io_error;
}

flatfile_authority_transaction_result
flatfile_authority_transaction_commit(const std::string &root, const flatfile_authority_lock &lock,
				      const std::vector<flatfile_authority_after_image> &images,
				      std::string *error)
{
	std::vector<uint8_t> bytes;
	if (!lock.owns(root) || !encode_transaction(images, &bytes))
		return flatfile_authority_transaction_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), transaction_filename, bytes, error))
		return flatfile_authority_transaction_result::io_error;
	for (size_t index = 0; index < images.size(); ++index)
	{
		if (!flatfile_atomic_write(domains_directory(root), images[index].filename,
					   images[index].bytes, error))
			return flatfile_authority_transaction_result::io_error;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
		if (getenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE") && index == 0)
			return flatfile_authority_transaction_result::io_error;
#endif
	}
	return flatfile_atomic_remove(domains_directory(root), transaction_filename, false, error) ?
		       flatfile_authority_transaction_result::ok :
		       flatfile_authority_transaction_result::io_error;
}
