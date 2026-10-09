#include "flatfile/flatfile_authority_transaction.h"

#include "flatfile/flatfile_store.h"
#include "flatfile/flatfile_accounting_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <sys/stat.h>
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
	case flatfile_authority_store::item_accounting_references:
		return root + "/accounting/item_references";
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

flatfile_authority_lock::flatfile_authority_lock(flatfile_scratch_reserve_fn reserve_scratch_peak,
						 void *context, size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
#else
	constexpr size_t own_bytes = sizeof(flatfile_authority_lock) + sizeof(state);
	if (!reserve_scratch_peak || outer_live_scratch > SIZE_MAX - own_bytes ||
	    !reserve_scratch_peak(outer_live_scratch + own_bytes, context))
	{
		errno = ENOBUFS;
		return;
	}
	state_.reset(new (std::nothrow) state);
	if (!state_)
		errno = ENOMEM;
#endif
}

bool flatfile_authority_lock::retained_bytes(size_t *output) const noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	if (!output || !state_)
		return false;
	size_t bytes = sizeof(*this) + sizeof(state);
	// SSO storage is inline in state and must not be counted a second time.
	if (state_->root.capacity() > 15)
	{
		if (state_->root.capacity() == SIZE_MAX ||
		    state_->root.capacity() + 1 > SIZE_MAX - bytes)
			return false;
		bytes += state_->root.capacity() + 1;
	}
	*output = bytes;
	return true;
#endif
}

bool flatfile_authority_lock::acquire_bounded(const std::string &root,
					      flatfile_scratch_reserve_fn reserve_scratch_peak,
					      void *context, size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return false;
#else
	if (!state_ || state_->process_lock.owns_lock() || root.empty() || !reserve_scratch_peak)
	{
		errno = !state_ ? ENOMEM : EINVAL;
		return false;
	}
	const auto add = [](size_t &bytes, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - bytes)
			return false;
		bytes += amount;
		return true;
	};
	size_t directory_size = root.size();
	size_t live = outer_live_scratch;
	// Both helper metadata objects remain live through flock and return.
	if (!add(live, sizeof(struct stat)) || !add(live, sizeof(struct stat)) ||
	    !add(directory_size, sizeof("/domains") - 1) ||
	    !add(live, 3 * sizeof(std::string)) ||
	    (root.size() > 15 && (root.size() == SIZE_MAX || !add(live, root.size() + 1))) ||
	    (directory_size > 15 &&
	     (directory_size == SIZE_MAX || !add(live, directory_size + 1))) ||
	    !add(live, sizeof(".critical-authority.lock")) || !reserve_scratch_peak(live, context))
	{
		errno = ENOBUFS;
		return false;
	}
	try
	{
		std::string owned_root(root);
		// Fresh length construction avoids the old root-copy/append growth.
		std::string directory(directory_size, '\0');
		std::copy(root.begin(), root.end(), directory.begin());
		std::copy_n("/domains", sizeof("/domains") - 1, directory.begin() + root.size());
		const std::string filename(lock_filename);
		state_->process_lock.lock();
		if (flatfile_lock_acquire(directory, filename, &state_->fd, nullptr))
		{
			state_->root.swap(owned_root);
			return true;
		}
		state_->process_lock.unlock();
		return false;
	}
	catch (const std::bad_alloc &)
	{
		if (state_->process_lock.owns_lock())
			state_->process_lock.unlock();
		errno = ENOMEM;
		return false;
	}
	catch (...)
	{
		if (state_->process_lock.owns_lock())
			state_->process_lock.unlock();
		errno = EIO;
		return false;
	}
#endif
}

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
{
	return commit_with_outcome(root, lock, operations, error, nullptr);
}

flatfile_authority_transaction_result flatfile_accounting_storage::commit_with_outcome(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations, std::string *error,
	flatfile_authority_commit_outcome *outcome)
try
{
	if (outcome)
		*outcome = flatfile_authority_commit_outcome::not_published;
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
	bool published = false;
	bool durable = false;
	try
	{
		durable = flatfile_atomic_write_with_publication(
			domains_directory(root), transaction_filename, bytes, error, &published);
	}
	catch (const std::bad_alloc &)
	{
		if (outcome && published)
			*outcome = flatfile_authority_commit_outcome::publication_uncertain;
		throw;
	}
	if (!durable)
	{
		if (outcome && published)
			*outcome = flatfile_authority_commit_outcome::publication_uncertain;
		return flatfile_authority_transaction_result::io_error;
	}
	if (outcome)
		*outcome = flatfile_authority_commit_outcome::committed;
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
	return flatfile_authority_transaction_commit_operations_with_outcome(root, lock, operations,
									     error, nullptr);
}

flatfile_authority_transaction_result flatfile_authority_transaction_commit_operations_with_outcome(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations, std::string *error,
	flatfile_authority_commit_outcome *outcome)
{
	if (outcome)
		*outcome = flatfile_authority_commit_outcome::not_published;
	for (const auto &operation : operations)
		if (operation.store == flatfile_authority_store::economic_evidence)
			return flatfile_authority_transaction_result::invalid;
	return flatfile_accounting_storage::commit_with_outcome(root, lock, operations, error,
								outcome);
}

namespace
{
bool bounded_add(size_t &bytes, size_t added) noexcept
{
	if (added > SIZE_MAX - bytes)
		return false;
	bytes += added;
	return true;
}

const char *bounded_store_suffix(flatfile_authority_store store) noexcept
{
	switch (store)
	{
	case flatfile_authority_store::domains:
		return "/domains";
	case flatfile_authority_store::players:
		return "/players";
	case flatfile_authority_store::identities:
		return "/identities/names";
	case flatfile_authority_store::accounts:
		return "/identities/accounts";
	case flatfile_authority_store::metadata:
		return "/metadata";
	case flatfile_authority_store::player_deaths:
		return "/player-deaths";
	case flatfile_authority_store::economic_evidence:
		return "/economic-evidence";
	case flatfile_authority_store::item_accounting_references:
		return "/accounting/item_references";
	}
	return nullptr;
}

// Fresh C++11-ABI libstdc++13 string reserve/assign grows from actual SSO
// capacity using _M_create's checked exponential policy. Count the terminator;
// SSO capacity is conservative alongside sizeof(string), as in original budgets.
bool bounded_string_capacity(size_t length, size_t *output) noexcept
{
	const std::string empty;
	size_t capacity = empty.capacity();
	if (length > capacity)
	{
		if (capacity > SIZE_MAX / 2)
			return false;
		capacity = length < capacity * 2 ? capacity * 2 : length;
	}
	if (capacity == SIZE_MAX)
		return false;
	*output = capacity + 1;
	return true;
}

struct bounded_wire_operation
{
	flatfile_authority_store store = flatfile_authority_store::domains;
	flatfile_authority_operation_kind kind = flatfile_authority_operation_kind::write;
	const uint8_t *name = nullptr;
	size_t name_size = 0, image_size = 0;
};

bool bounded_name_equal(const bounded_wire_operation &a, const bounded_wire_operation &b) noexcept
{
	return a.name_size == b.name_size && !memcmp(a.name, b.name, a.name_size);
}

bool bounded_wire_read(decoder &payload, uint32_t version, bounded_wire_operation *out) noexcept
{
	bounded_wire_operation candidate;
	uint16_t name_size = 0;
	uint32_t image_size = 0;
	if (version == transaction_version &&
	    (!payload.number(&candidate.store) || !payload.number(&candidate.kind)))
		return false;
	const char *suffix = bounded_store_suffix(candidate.store);
	if (!suffix || !payload.number(&name_size) || !name_size ||
	    name_size > transaction_maximum_filename || payload.offset > payload.size ||
	    name_size > payload.size - payload.offset)
		return false;
	candidate.name = payload.data + payload.offset;
	candidate.name_size = name_size;
	for (size_t i = 0; i < name_size; ++i)
	{
		const unsigned char ch = candidate.name[i];
		if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
		      (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-'))
			return false;
	}
	const auto reserved = [&](const char *name) noexcept
	{ return strlen(name) == name_size && !memcmp(candidate.name, name, name_size); };
	if (reserved(".") || reserved("..") || reserved(transaction_filename) ||
	    reserved(lock_filename))
		return false;
	payload.offset += name_size;
	if (!payload.number(&image_size) || image_size > transaction_maximum_bytes ||
	    payload.offset > payload.size || image_size > payload.size - payload.offset ||
	    (candidate.kind == flatfile_authority_operation_kind::write	 ? !image_size :
	     candidate.kind == flatfile_authority_operation_kind::remove ? image_size != 0 :
									   true))
		return false;
	candidate.image_size = image_size;
	payload.offset += image_size;
	*out = candidate;
	return true;
}

struct bounded_recovery_workspace
{
	std::vector<uint8_t> journal;
	std::vector<flatfile_authority_operation> operations;
	std::string directory, apply_directory, journal_name;
	uint16_t count = 0;
	size_t decoded_bytes = 0, apply_path_bytes = 0, validation_path_bytes = 0;
};

bool bounded_recovery_scan(bounded_recovery_workspace &work, const std::string &root) noexcept
{
	constexpr size_t header_size =
		transaction_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
	const auto &bytes = work.journal;
	if (bytes.size() < header_size || bytes.size() > transaction_maximum_bytes ||
	    memcmp(bytes.data(), transaction_magic.data(), transaction_magic.size()))
		return false;
	decoder header{ bytes.data() + transaction_magic.size(),
			bytes.size() - transaction_magic.size() };
	uint32_t version = 0, payload_size = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    (version != transaction_version && version != transaction_legacy_version) ||
	    payload_size != bytes.size() - header_size)
		return false;
	decoder payload{ bytes.data() + header_size, payload_size };
	if (!payload.number(&work.count) || !work.count ||
	    work.count > transaction_maximum_images ||
	    work.count > SIZE_MAX / sizeof(flatfile_authority_operation))
		return false;
	work.decoded_bytes = work.count * sizeof(flatfile_authority_operation);
	const size_t first = payload.offset;
	for (size_t index = 0; index < work.count; ++index)
	{
		bounded_wire_operation current;
		if (!bounded_wire_read(payload, version, &current))
			return false;
		decoder previous{ bytes.data() + header_size, payload_size, first };
		for (size_t prior = 0; prior < index; ++prior)
		{
			bounded_wire_operation earlier;
			if (!bounded_wire_read(previous, version, &earlier) ||
			    (current.store == earlier.store &&
			     bounded_name_equal(current, earlier)))
				return false;
		}
		size_t name_bytes = 0;
		if (!bounded_string_capacity(current.name_size, &name_bytes) ||
		    !bounded_add(work.decoded_bytes, name_bytes) ||
		    !bounded_add(work.decoded_bytes, current.image_size))
			return false;
		const size_t suffix_size = strlen(bounded_store_suffix(current.store));
		size_t path_size = root.size(), path_bytes = 0, validation_bytes = 0;
		if (!bounded_add(path_size, suffix_size) ||
		    !bounded_string_capacity(path_size, &path_bytes) ||
		    !bounded_string_capacity(4 + suffix_size, &validation_bytes))
			return false;
		work.apply_path_bytes = std::max(work.apply_path_bytes, path_bytes);
		work.validation_path_bytes = std::max(work.validation_path_bytes, validation_bytes);
	}
	return payload.offset == payload.size;
}
} // namespace

flatfile_authority_transaction_result
flatfile_authority_transaction_recover_bounded(const std::string &root,
					       const flatfile_authority_lock &lock,
					       flatfile_scratch_reserve_fn reserve_scratch_peak,
					       void *context, size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return flatfile_authority_transaction_result::io_error;
#else
	if (!reserve_scratch_peak || !lock.matches(root))
	{
		errno = EINVAL;
		return flatfile_authority_transaction_result::invalid;
	}
	try
	{
		size_t base = outer_live_scratch, path_size = root.size(), directory_bytes = 0,
		       name_bytes = 0;
		if (!bounded_add(path_size, strlen("/domains")) ||
		    !bounded_string_capacity(path_size, &directory_bytes) ||
		    !bounded_string_capacity(strlen(transaction_filename), &name_bytes) ||
		    !bounded_add(base, sizeof(bounded_recovery_workspace)) ||
		    !bounded_add(base, directory_bytes) || !bounded_add(base, name_bytes) ||
		    !bounded_add(base, 3 * sizeof(decoder)) ||
		    !bounded_add(base, 2 * sizeof(bounded_wire_operation)) ||
		    !bounded_add(base, sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>)) ||
		    !bounded_add(base, sizeof(std::string)) || !reserve_scratch_peak(base, context))
		{
			errno = ENOBUFS;
			return flatfile_authority_transaction_result::io_error;
		}
		bounded_recovery_workspace work;
		work.directory.reserve(path_size);
		work.directory.assign(root);
		work.directory.append("/domains");
		work.journal_name.reserve(strlen(transaction_filename));
		work.journal_name.assign(transaction_filename);
		const auto read = flatfile_read_bounded(work.directory, work.journal_name,
							transaction_maximum_bytes, &work.journal,
							reserve_scratch_peak, context, base);
		if (read == flatfile_read_result::not_found)
			return flatfile_authority_transaction_result::ok;
		if (read == flatfile_read_result::invalid)
			return flatfile_authority_transaction_result::invalid;
		if (read != flatfile_read_result::ok)
			return flatfile_authority_transaction_result::io_error;
		if (!bounded_recovery_scan(work, root))
		{
			errno = EBADMSG;
			return flatfile_authority_transaction_result::invalid;
		}
		size_t peak = base;
		// All original journal bytes and ALL decoded operations coexist. The
		// reusable real apply path is reserved before decode and before any IO.
		// Validation uses the unchanged original valid_operation path temporary.
		if (!bounded_add(peak, work.journal.capacity()) ||
		    !bounded_add(peak, work.decoded_bytes) ||
		    !bounded_add(peak, work.apply_path_bytes) ||
		    !bounded_add(peak, 2 * sizeof(std::string)) ||
		    !bounded_add(peak, work.validation_path_bytes) ||
		    !reserve_scratch_peak(peak, context))
		{
			errno = ENOBUFS;
			return flatfile_authority_transaction_result::io_error;
		}
		work.operations.reserve(work.count);
		// Reserve a real reusable same-root directory for the largest actual
		// store suffix. Capacity stays live through every apply/removal.
		work.apply_directory.reserve(work.apply_path_bytes - 1);
		const auto decoded = decode_transaction(work.journal, &work.operations);
		if (decoded != flatfile_authority_transaction_result::ok)
			return decoded;
		for (const auto &operation : work.operations)
		{
			work.apply_directory.assign(root);
			work.apply_directory.append(bounded_store_suffix(operation.store));
			const bool applied =
				operation.kind == flatfile_authority_operation_kind::write ?
					flatfile_atomic_write(work.apply_directory,
							      operation.filename, operation.bytes,
							      nullptr) :
					flatfile_atomic_remove(work.apply_directory,
							       operation.filename, true, nullptr);
			if (!applied)
				return flatfile_authority_transaction_result::io_error;
		}
		return flatfile_atomic_remove(work.directory, work.journal_name, false, nullptr) ?
			       flatfile_authority_transaction_result::ok :
			       flatfile_authority_transaction_result::io_error;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_authority_transaction_result::io_error;
	}
	catch (...)
	{
		errno = EOVERFLOW;
		return flatfile_authority_transaction_result::io_error;
	}
#endif
}

namespace
{
// Exact pinned libstdc++13 fresh reserve request, excluding inline SSO storage.
bool commit_string_request(size_t length, size_t &bytes) noexcept
{
	bytes = 0;
	if (length <= 15)
		return true;
	const size_t capacity = std::max(size_t{ 30 }, length);
	if (capacity == SIZE_MAX)
		return false;
	bytes = capacity + 1;
	return true;
}

struct commit_vector_growth
{
	size_t size = 0, capacity = 0, peak = 0;
	bool insert(size_t count) noexcept
	{
		if (count > SIZE_MAX - size)
			return false;
		if (count > capacity - size)
		{
			const size_t growth = std::max(size, count);
			if (growth > SIZE_MAX - size || size + growth > SIZE_MAX - capacity)
				return false;
			const size_t replacement = size + growth;
			peak = std::max(peak, capacity + replacement);
			capacity = replacement;
		}
		size += count;
		peak = std::max(peak, capacity);
		return true;
	}
	bool number(size_t width) noexcept
	{
		for (size_t i = 0; i < width; ++i)
			if (!insert(1))
				return false;
		return true;
	}
};

struct bounded_commit_workspace
{
	std::vector<uint8_t> pending, journal;
	std::string directory, journal_name, apply_directory;
	commit_vector_growth payload, file;
	size_t encoder_peak = 0, apply_length = 0;
};

bool commit_admit(size_t base, size_t extra, flatfile_scratch_reserve_fn reserve,
		  void *context) noexcept
{
	if (extra > SIZE_MAX - base || !reserve || !reserve(base + extra, context))
	{
		errno = ENOBUFS;
		return false;
	}
	return true;
}

flatfile_authority_transaction_result
bounded_commit_profile(bounded_commit_workspace &work, const std::string &root,
		       const std::vector<flatfile_authority_operation> &operations, size_t base,
		       flatfile_scratch_reserve_fn reserve, void *context)
{
	if (operations.empty() || operations.size() > transaction_maximum_images)
		return flatfile_authority_transaction_result::invalid;
	// Validate in the original order, after the caller's pending-journal check.
	// The real original validator's literal input and returned path objects are
	// admitted before invoking it, including its actual append allocation.
	size_t payload_length = sizeof(uint16_t);
	for (size_t index = 0; index < operations.size(); ++index)
	{
		const auto &operation = operations[index];
		const char *suffix = bounded_store_suffix(operation.store);
		size_t validation_heap = 0;
		if (suffix && !commit_string_request(4 + strlen(suffix), validation_heap))
		{
			errno = ENOBUFS;
			return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
		}
		size_t validation = 2 * sizeof(std::string);
		if (!bounded_add(validation, validation_heap) ||
		    !commit_admit(base, validation, reserve, context))
			return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
		if (!valid_operation(operation))
			return flatfile_authority_transaction_result::invalid;
		for (size_t prior = 0; prior < index; ++prior)
			if (operations[prior].store == operation.store &&
			    operations[prior].filename == operation.filename)
				return flatfile_authority_transaction_result::invalid;
		if (!bounded_add(payload_length, sizeof(operation.store) + sizeof(operation.kind) +
							 sizeof(uint16_t) + sizeof(uint32_t)) ||
		    !bounded_add(payload_length, operation.filename.size()) ||
		    !bounded_add(payload_length, operation.bytes.size()))
		{
			errno = ENOBUFS;
			return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
		}
		size_t length = root.size();
		if (!bounded_add(length, strlen(suffix)))
		{
			errno = ENOBUFS;
			return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
		}
		work.apply_length = std::max(work.apply_length, length);
	}
	constexpr size_t header =
		transaction_magic.size() + 2 * sizeof(uint32_t) + SHA256_DIGEST_LENGTH;
	if (payload_length > transaction_maximum_bytes ||
	    header > transaction_maximum_bytes - payload_length)
		return flatfile_authority_transaction_result::invalid;
	// Replay actual encoder number() single-byte pushes and raw() forward-range
	// insert requests. Old/new backing storage coexists only during growth.
	if (!work.payload.number(sizeof(uint16_t)))
		return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
	for (const auto &operation : operations)
	{
		size_t validation_heap = 0, validation = sizeof(encoder);
		if (!commit_string_request(4 + strlen(bounded_store_suffix(operation.store)),
					   validation_heap) ||
		    !bounded_add(validation, work.payload.capacity) ||
		    !bounded_add(validation, 2 * sizeof(std::string)) ||
		    !bounded_add(validation, validation_heap))
			return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
		work.encoder_peak = std::max(work.encoder_peak, validation);
		if (!work.payload.number(sizeof(operation.store)) ||
		    !work.payload.number(sizeof(operation.kind)) ||
		    !work.payload.number(sizeof(uint16_t)) ||
		    !work.payload.insert(operation.filename.size()) ||
		    !work.payload.number(sizeof(uint32_t)) ||
		    !work.payload.insert(operation.bytes.size()))
			return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
	}
	size_t payload_peak = sizeof(encoder);
	if (!bounded_add(payload_peak, work.payload.peak))
		return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
	work.encoder_peak = std::max(work.encoder_peak, payload_peak);
	if (!work.file.insert(transaction_magic.size()) ||
	    !work.file.number(sizeof(transaction_version)) || !work.file.number(sizeof(uint32_t)) ||
	    !work.file.insert(SHA256_DIGEST_LENGTH) || !work.file.insert(work.payload.size))
		return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
	size_t file_peak = 2 * sizeof(encoder) + sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>);
	if (!bounded_add(file_peak, work.payload.capacity) ||
	    !bounded_add(file_peak, work.file.peak))
		return (errno = ENOBUFS, flatfile_authority_transaction_result::io_error);
	work.encoder_peak = std::max(work.encoder_peak, file_peak);
	return flatfile_authority_transaction_result::ok;
}
} // namespace

flatfile_authority_transaction_result flatfile_accounting_storage::commit_with_outcome_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations,
	flatfile_authority_commit_outcome *outcome,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept
{
	if (outcome)
		*outcome = flatfile_authority_commit_outcome::not_published;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)operations;
	(void)reserve_scratch_peak;
	(void)context;
	(void)outer_live_scratch;
	errno = ENOTSUP;
	return flatfile_authority_transaction_result::io_error;
#else
	if (!lock.owns(root))
		return flatfile_authority_transaction_result::invalid;
	try
	{
		size_t base = outer_live_scratch, directory_length = root.size(),
		       directory_bytes = 0, name_bytes = 0;
		if (!bounded_add(directory_length, strlen("/domains")) ||
		    !commit_string_request(directory_length, directory_bytes) ||
		    !commit_string_request(strlen(transaction_filename), name_bytes) ||
		    !bounded_add(base, sizeof(bounded_commit_workspace)) ||
		    !bounded_add(base, directory_bytes) || !bounded_add(base, name_bytes) ||
		    !commit_admit(base, 0, reserve_scratch_peak, context))
		{
			errno = ENOBUFS;
			return flatfile_authority_transaction_result::io_error;
		}
		bounded_commit_workspace work;
		work.directory.reserve(directory_length);
		work.directory.assign(root);
		work.directory.append("/domains");
		work.journal_name.reserve(strlen(transaction_filename));
		work.journal_name.assign(transaction_filename);
		// Same secure complete read: any present/corrupt pending journal refuses.
		// Never recover here after the caller already staged after-images.
		const auto read = flatfile_read_bounded(work.directory, work.journal_name,
							transaction_maximum_bytes, &work.pending,
							reserve_scratch_peak, context, base);
		if (read != flatfile_read_result::not_found)
			return read == flatfile_read_result::io_error ?
				       flatfile_authority_transaction_result::io_error :
				       flatfile_authority_transaction_result::invalid;
		errno = 0;
		const auto profile = bounded_commit_profile(work, root, operations, base,
							    reserve_scratch_peak, context);
		if (profile != flatfile_authority_transaction_result::ok)
			return profile;
		if (!commit_admit(base, work.encoder_peak, reserve_scratch_peak, context))
			return flatfile_authority_transaction_result::io_error;
		errno = 0;
		if (!encode_transaction(operations, &work.journal))
			return errno == ENOMEM ? flatfile_authority_transaction_result::io_error :
						 flatfile_authority_transaction_result::invalid;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
		if (getenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT"))
			return flatfile_authority_transaction_result::io_error;
#endif
		// Reserve the actual reusable largest apply path and every atomic leaf
		// frame BEFORE publication. No subsequent apply can fail budget admission.
		size_t apply_bytes = 0, apply_base = base;
		if (!commit_string_request(work.apply_length, apply_bytes) ||
		    !bounded_add(apply_base, work.journal.capacity()) ||
		    !bounded_add(apply_base, apply_bytes) ||
		    !commit_admit(apply_base,
				  std::max(flatfile_atomic_write_working_bytes(),
					   flatfile_atomic_remove_working_bytes()),
				  reserve_scratch_peak, context))
		{
			errno = ENOBUFS;
			return flatfile_authority_transaction_result::io_error;
		}
		work.apply_directory.reserve(work.apply_length);
		bool published = false, durable = false;
		try
		{
			durable = flatfile_atomic_write_with_publication(work.directory,
									 work.journal_name,
									 work.journal, nullptr,
									 &published);
		}
		catch (const std::bad_alloc &)
		{
			if (outcome && published)
				*outcome = flatfile_authority_commit_outcome::publication_uncertain;
			throw;
		}
		if (!durable)
		{
			if (outcome && published)
				*outcome = flatfile_authority_commit_outcome::publication_uncertain;
			return flatfile_authority_transaction_result::io_error;
		}
		if (outcome)
			*outcome = flatfile_authority_commit_outcome::committed;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
		if (getenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL"))
			return flatfile_authority_transaction_result::io_error;
#endif
		for (size_t index = 0; index < operations.size(); ++index)
		{
			const auto &operation = operations[index];
			work.apply_directory.assign(root);
			work.apply_directory.append(bounded_store_suffix(operation.store));
			const bool applied =
				operation.kind == flatfile_authority_operation_kind::write ?
					flatfile_atomic_write(work.apply_directory,
							      operation.filename, operation.bytes,
							      nullptr) :
					flatfile_atomic_remove(work.apply_directory,
							       operation.filename, true, nullptr);
			if (!applied)
				return flatfile_authority_transaction_result::io_error;
#ifdef DURIS_FLATFILE_AUTHORITY_FAULT_TEST
			if (const char *stop = getenv(
				    "DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION"))
			{
				char *end = nullptr;
				const auto boundary = strtoul(stop, &end, 10);
				if (end && !*end && boundary == index + 1)
					return flatfile_authority_transaction_result::io_error;
			}
			if (getenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE") &&
			    index == 0)
				return flatfile_authority_transaction_result::io_error;
#endif
		}
		return flatfile_atomic_remove(work.directory, work.journal_name, false, nullptr) ?
			       flatfile_authority_transaction_result::ok :
			       flatfile_authority_transaction_result::io_error;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_authority_transaction_result::io_error;
	}
	catch (...)
	{
		errno = EOVERFLOW;
		return flatfile_authority_transaction_result::io_error;
	}
#endif
}

flatfile_authority_transaction_result
flatfile_authority_transaction_commit_operations_with_outcome_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations,
	flatfile_authority_commit_outcome *outcome,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept
{
	if (outcome)
		*outcome = flatfile_authority_commit_outcome::not_published;
	for (const auto &operation : operations)
		if (operation.store == flatfile_authority_store::economic_evidence)
			return flatfile_authority_transaction_result::invalid;
	return flatfile_accounting_storage::commit_with_outcome_bounded(
		root, lock, operations, outcome, reserve_scratch_peak, context, outer_live_scratch);
}