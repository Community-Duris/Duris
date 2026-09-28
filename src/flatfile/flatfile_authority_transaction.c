#include "flatfile/flatfile_authority_transaction.h"

#include "flatfile/flatfile_store.h"
#include "flatfile/flatfile_accounting_store.h"

#include <array>
#include <cerrno>
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
constexpr uint32_t transaction_version = 2;
constexpr uint32_t transaction_legacy_version = 1;
constexpr size_t transaction_maximum_images = flatfile_authority_transaction_maximum_operations;
constexpr size_t transaction_maximum_filename = 192;
constexpr size_t transaction_maximum_bytes = flatfile_authority_transaction_maximum_bytes;
constexpr const char *transaction_filename = ".critical-authority-transaction";
constexpr const char *lock_filename = ".critical-authority.lock";
std::mutex authority_mutex;

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

std::string operation_directory(const std::string &root, flatfile_authority_store store)
{
	switch (store)
	{
	case flatfile_authority_store::domains:
		return domains_directory(root);
	case flatfile_authority_store::players:
		return root + "/players";
	case flatfile_authority_store::identities:
		return root + "/identities/names";
	case flatfile_authority_store::accounts:
		return root + "/identities/accounts";
	case flatfile_authority_store::metadata:
		return root + "/metadata";
	case flatfile_authority_store::player_deaths:
		return root + "/player-deaths";
	case flatfile_authority_store::economic_evidence:
		return root + "/economic-evidence";
	}
	return {};
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
			errno = ENOMEM;
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
			errno = ENOMEM;
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
		if (!output || offset > size || size - offset < count)
			return false;
		memcpy(output, data + offset, count);
		offset += count;
		return true;
	}
};

bool valid_operation(const flatfile_authority_operation &operation)
{
	if (operation_directory("root", operation.store).empty() ||
	    !safe_filename(operation.filename))
		return false;
	switch (operation.kind)
	{
	case flatfile_authority_operation_kind::write:
		return !operation.bytes.empty() &&
		       operation.bytes.size() <= transaction_maximum_bytes;
	case flatfile_authority_operation_kind::remove:
		return operation.bytes.empty();
	}
	return false;
}

bool encode_transaction(const std::vector<flatfile_authority_operation> &operations,
			std::vector<uint8_t> *bytes)
{
	if (!bytes || operations.empty() || operations.size() > transaction_maximum_images)
		return false;
	encoder payload;
	payload.number<uint16_t>(operations.size());
	for (size_t index = 0; index < operations.size(); ++index)
	{
		const auto &operation = operations[index];
		if (!valid_operation(operation))
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (operations[prior].store == operation.store &&
			    operations[prior].filename == operation.filename)
				return false;
		payload.number(operation.store);
		payload.number(operation.kind);
		payload.number<uint16_t>(operation.filename.size());
		payload.raw(reinterpret_cast<const uint8_t *>(operation.filename.data()),
			    operation.filename.size());
		payload.number<uint32_t>(operation.bytes.size());
		if (!operation.bytes.empty())
			payload.raw(operation.bytes.data(), operation.bytes.size());
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
		   std::vector<flatfile_authority_operation> *operations)
{
	constexpr size_t header_size =
		transaction_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
	if (!operations || bytes.size() < header_size ||
	    memcmp(bytes.data(), transaction_magic.data(), transaction_magic.size()))
		return flatfile_authority_transaction_result::invalid;
	decoder header{ bytes.data() + transaction_magic.size(),
			bytes.size() - transaction_magic.size() };
	uint32_t version = 0, payload_size = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    (version != transaction_version && version != transaction_legacy_version) ||
	    payload_size != bytes.size() - header_size)
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
		operations->clear();
		operations->resize(count);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_authority_transaction_result::io_error;
	}
	for (size_t index = 0; index < operations->size(); ++index)
	{
		uint16_t filename_size = 0;
		uint32_t image_size = 0;
		auto &operation = (*operations)[index];
		if (version == transaction_version)
		{
			if (!payload.number(&operation.store) || !payload.number(&operation.kind))
				return flatfile_authority_transaction_result::invalid;
		}
		else
		{
			operation.store = flatfile_authority_store::domains;
			operation.kind = flatfile_authority_operation_kind::write;
		}
		if (!payload.number(&filename_size) || !filename_size ||
		    filename_size > transaction_maximum_filename ||
		    payload.size - payload.offset < filename_size)
			return flatfile_authority_transaction_result::invalid;
		try
		{
			operation.filename.assign(
				reinterpret_cast<const char *>(payload.data + payload.offset),
				filename_size);
		}
		catch (const std::bad_alloc &)
		{
			errno = ENOMEM;
			return flatfile_authority_transaction_result::io_error;
		}
		payload.offset += filename_size;
		if (!payload.number(&image_size) || image_size > transaction_maximum_bytes ||
		    payload.size - payload.offset < image_size)
			return flatfile_authority_transaction_result::invalid;
		for (size_t prior = 0; prior < index; ++prior)
			if ((*operations)[prior].store == operation.store &&
			    (*operations)[prior].filename == operation.filename)
				return flatfile_authority_transaction_result::invalid;
		try
		{
			operation.bytes.resize(image_size);
		}
		catch (const std::bad_alloc &)
		{
			errno = ENOMEM;
			return flatfile_authority_transaction_result::io_error;
		}
		if (image_size && !payload.raw(operation.bytes.data(), operation.bytes.size()))
			return flatfile_authority_transaction_result::invalid;
		if (!valid_operation(operation))
			return flatfile_authority_transaction_result::invalid;
	}
	return payload.offset == payload.size ? flatfile_authority_transaction_result::ok :
						flatfile_authority_transaction_result::invalid;
}

bool apply_operation(const std::string &root, const flatfile_authority_operation &operation,
		     std::string *error)
{
	const std::string directory = operation_directory(root, operation.store);
	if (directory.empty())
		return false;
	if (operation.kind == flatfile_authority_operation_kind::write)
		return flatfile_atomic_write(directory, operation.filename, operation.bytes, error);
	if (operation.kind == flatfile_authority_operation_kind::remove)
		return flatfile_atomic_remove(directory, operation.filename, true, error);
	return false;
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
try
{
	if (!state_ || state_->process_lock.owns_lock() || root.empty())
	{
		errno = !state_ ? ENOMEM : EINVAL;
		return false;
	}
	// Allocate before taking either lock; failures leave the object reusable.
	std::string owned_root(root);
	const std::string directory = domains_directory(root);
	state_->process_lock.lock();
	if (flatfile_lock_acquire(directory, lock_filename, &state_->fd, error))
	{
		state_->root.swap(owned_root);
		return true;
	}
	state_->process_lock.unlock();
	return false;
}
catch (const std::bad_alloc &)
{
	if (state_ && state_->process_lock.owns_lock())
		state_->process_lock.unlock();
	errno = ENOMEM;
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
try
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
	std::vector<flatfile_authority_operation> operations;
	const auto decoded = decode_transaction(bytes, &operations);
	if (decoded != flatfile_authority_transaction_result::ok)
		return decoded;
	for (const auto &operation : operations)
		if (!apply_operation(root, operation, error))
			return flatfile_authority_transaction_result::io_error;
	return flatfile_atomic_remove(domains_directory(root), transaction_filename, false, error) ?
		       flatfile_authority_transaction_result::ok :
		       flatfile_authority_transaction_result::io_error;
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_authority_transaction_result::io_error;
}

flatfile_authority_transaction_result
flatfile_authority_transaction_commit(const std::string &root, const flatfile_authority_lock &lock,
				      const std::vector<flatfile_authority_after_image> &images,
				      std::string *error)
{
	std::vector<flatfile_authority_operation> operations;
	try
	{
		operations.reserve(images.size());
		for (const auto &image : images)
			operations.push_back({ flatfile_authority_store::domains,
					       flatfile_authority_operation_kind::write,
					       image.filename, image.bytes });
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_authority_transaction_result::io_error;
	}
	return flatfile_authority_transaction_commit_operations(root, lock, operations, error);
}

flatfile_authority_transaction_result
flatfile_accounting_storage::commit(const std::string &root, const flatfile_authority_lock &lock,
				    const std::vector<flatfile_authority_operation> &operations,
				    std::string *error)
try
{
	std::vector<uint8_t> bytes;
	if (!lock.owns(root))
		return flatfile_authority_transaction_result::invalid;
	// Never overwrite a pending journal or recover after the caller staged images.
	// The caller must recover and resolve fresh state before retrying.
	std::vector<uint8_t> pending;
	const auto read = flatfile_read(domains_directory(root), transaction_filename,
					transaction_maximum_bytes, &pending, error);
	if (read != flatfile_read_result::not_found)
		return read == flatfile_read_result::io_error ?
			       flatfile_authority_transaction_result::io_error :
			       flatfile_authority_transaction_result::invalid;
	errno = 0;
	if (!encode_transaction(operations, &bytes))
		return errno == ENOMEM ? flatfile_authority_transaction_result::io_error :
					 flatfile_authority_transaction_result::invalid;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
	if (getenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT"))
		return flatfile_authority_transaction_result::io_error;
#endif
	if (!flatfile_atomic_write(domains_directory(root), transaction_filename, bytes, error))
		return flatfile_authority_transaction_result::io_error;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
	if (getenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL"))
		return flatfile_authority_transaction_result::io_error;
#endif
	for (size_t index = 0; index < operations.size(); ++index)
	{
		if (!apply_operation(root, operations[index], error))
			return flatfile_authority_transaction_result::io_error;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
		if (const char *stop =
			    getenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION"))
		{
			char *end = nullptr;
			const auto boundary = strtoul(stop, &end, 10);
			if (end && !*end && boundary == index + 1)
				return flatfile_authority_transaction_result::io_error;
		}
		if (getenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE") && index == 0)
			return flatfile_authority_transaction_result::io_error;
#endif
	}
	return flatfile_atomic_remove(domains_directory(root), transaction_filename, false, error) ?
		       flatfile_authority_transaction_result::ok :
		       flatfile_authority_transaction_result::io_error;
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_authority_transaction_result::io_error;
}

flatfile_authority_transaction_result flatfile_authority_transaction_commit_operations(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations, std::string *error)
{
	for (const auto &operation : operations)
		if (operation.store == flatfile_authority_store::economic_evidence)
			return flatfile_authority_transaction_result::invalid;
	return flatfile_accounting_storage::commit(root, lock, operations, error);
}
