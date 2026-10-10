#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_physical.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <dirent.h>
#include <fcntl.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

#include <cstring>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <utility>

namespace
{
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

}

namespace flatfile_player_snapshot_file
{
std::string player_directory(const std::string &root)
{
	return root + "/players";
}

std::string player_filename(int32_t pid)
{
	return std::to_string(pid) + ".snapshot";
}

std::string death_directory(const std::string &root)
{
	return root + "/player-deaths";
}

std::string death_filename(int32_t pid, uint64_t revision)
{
	return std::to_string(pid) + "-" + std::to_string(revision) + ".death";
}
}

using namespace flatfile_player_snapshot_file;

bool flatfile_player_snapshot_encode_file(const player_snapshot &snapshot,
					  std::vector<uint8_t> *bytes)
{
	if (!bytes || snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL)
		return false;
	try
	{
		player_snapshot normalized = snapshot;
		normalized.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		std::vector<uint8_t> payload;
		if (player_snapshot_encode(normalized, &payload) !=
		    player_snapshot_codec_result::ok)
			return false;
		normalized.encoded_size_bound = payload.size();
		if (player_snapshot_encode(normalized, &payload) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::vector<uint8_t> candidate;
		candidate.reserve(payload.size() + 68);
		candidate.insert(candidate.end(), player_magic.begin(), player_magic.end());
		auto number = [&](uint64_t value, size_t size)
		{
			for (size_t index = 0; index < size; ++index)
				candidate.push_back(static_cast<uint8_t>(value >> (index * 8)));
		};
		number(player_file_version, 4);
		number(payload.size(), 4);
		number(static_cast<uint32_t>(normalized.pid), 4);
		number(normalized.revision, 8);
		number(normalized.components, 8);
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(payload.data(), payload.size(), digest.data());
		candidate.insert(candidate.end(), digest.begin(), digest.end());
		candidate.insert(candidate.end(), payload.begin(), payload.end());
		if (candidate.size() > player_file_maximum)
			return false;
		*bytes = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

flatfile_player_load_result flatfile_player_snapshot_read(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error)
{
	return flatfile_player_snapshot_read_file(player_directory(root), player_filename(pid), pid,
						  snapshot, error);
}

flatfile_player_load_result
flatfile_player_snapshot_read_file(const std::string &directory, const std::string &filename,
				   int32_t pid, player_snapshot *snapshot, std::string *error)
{
	if (pid <= 0 || !snapshot)
		return flatfile_player_load_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read =
		flatfile_read(directory, filename, player_file_maximum, &bytes, error);
	if (read == flatfile_read_result::not_found)
		return flatfile_player_load_result::not_found;
	if (read == flatfile_read_result::invalid)
		return flatfile_player_load_result::invalid;
	if (read != flatfile_read_result::ok)
		return flatfile_player_load_result::io_error;
	constexpr size_t header_size = player_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(int32_t) + sizeof(uint64_t) * 2 +
				       SHA256_DIGEST_LENGTH;
	if (bytes.size() < header_size ||
	    memcmp(bytes.data(), player_magic.data(), player_magic.size()))
		return flatfile_player_load_result::invalid;
	decoder header{ bytes.data() + player_magic.size(), bytes.size() - player_magic.size() };
	uint32_t version = 0, payload_size = 0;
	int32_t stored_pid = 0;
	uint64_t stored_revision = 0, stored_components = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&stored_pid) || !header.number(&stored_revision) ||
	    !header.number(&stored_components) || version != player_file_version ||
	    stored_pid != pid || !stored_revision ||
	    stored_components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    payload_size != bytes.size() - header_size)
		return flatfile_player_load_result::invalid;
	const uint8_t *stored_digest = bytes.data() + player_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(int32_t) + sizeof(uint64_t) * 2;
	const uint8_t *payload = bytes.data() + header_size;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_player_load_result::invalid;
	player_snapshot decoded = {};
	if (player_snapshot_decode(payload, payload_size, &decoded) !=
		    player_snapshot_codec_result::ok ||
	    decoded.pid != stored_pid || decoded.revision != stored_revision ||
	    decoded.components != stored_components)
		return flatfile_player_load_result::invalid;
	*snapshot = std::move(decoded);
	return flatfile_player_load_result::ok;
}

unsigned int flatfile_native_mobile_birth_ordinary_player_physical_storage::verify_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_player_physical_absence *output,
	std::string *error) noexcept
{
	if (root.empty() || !output || !identity_lock.matches(root) ||
	    !authority_lock.matches(root))
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
		flatfile_native_mobile_birth_ordinary_identity_current identities;
		const auto identity_status =
			flatfile_native_mobile_birth_ordinary_identity_storage::read_locked(
				root, identity_lock, authority_lock, &identities, error);
		if (identity_status != flatfile_identity_result::ok)
			return identity_status == flatfile_identity_result::io_error ? EIO : EILSEQ;
		flatfile_native_mobile_birth_ordinary_player_physical_absence observed;
		observed.identity_catalog_revision = identities.catalog_revision;
		observed.identities = identities.records.size();
		for (const auto &identity : identities.records)
			if (!identity.active)
				++observed.retired_identities;

		const std::string directory = flatfile_player_snapshot_file::player_directory(root);
		std::vector<int32_t> pids;
		{
			// Enumerate actual canonical files, not only current/active identities.
			// The original writer holds authority through snapshot rename; do not
			// acquire player locks under authority and reverse its lock order.
			const int descriptor = open(
				directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
			if (descriptor < 0)
				return errno ? static_cast<unsigned int>(errno) : EIO;
			struct stat metadata
			{
			};
			if (fstat(descriptor, &metadata) < 0)
			{
				const int saved = errno;
				close(descriptor);
				return saved ? static_cast<unsigned int>(saved) : EIO;
			}
			if (!S_ISDIR(metadata.st_mode) || metadata.st_uid != geteuid() ||
			    (metadata.st_mode & 0077))
			{
				close(descriptor);
				return EILSEQ;
			}
			DIR *stream = fdopendir(descriptor);
			if (!stream)
			{
				const int saved = errno;
				close(descriptor);
				return saved ? static_cast<unsigned int>(saved) : EIO;
			}
			struct directory_owner
			{
				DIR *stream;
				~directory_owner() { closedir(stream); }
			} owned{ stream };
			for (;;)
			{
				errno = 0;
				const auto *entry = readdir(stream);
				if (!entry)
				{
					if (errno)
						return static_cast<unsigned int>(errno);
					break;
				}
				const std::string_view name(entry->d_name);
				constexpr std::string_view suffix = ".snapshot";
				if (!name.ends_with(suffix))
					continue; // Original lock/atomic temporary names are not snapshots.
				const auto numeric = name.substr(0, name.size() - suffix.size());
				int32_t pid = 0;
				const auto parsed = std::from_chars(
					numeric.data(), numeric.data() + numeric.size(), pid);
				if (parsed.ec != std::errc{} ||
				    parsed.ptr != numeric.data() + numeric.size() || pid <= 0 ||
				    flatfile_player_snapshot_file::player_filename(pid) != name)
					return EILSEQ;
				pids.push_back(pid);
			}
		}
		std::sort(pids.begin(), pids.end());
		if (std::adjacent_find(pids.begin(), pids.end()) != pids.end())
			return EILSEQ;
		for (int32_t pid : pids)
		{
			player_snapshot snapshot{};
			const auto loaded = flatfile_player_snapshot_read_file(
				directory, flatfile_player_snapshot_file::player_filename(pid), pid,
				&snapshot, error);
			// An enumerated canonical file disappearing under this genuine freeze
			// is not a harmless missing identity or an accepted empty inventory.
			if (loaded != flatfile_player_load_result::ok)
				return loaded == flatfile_player_load_result::io_error ?
					       EIO :
					       (loaded == flatfile_player_load_result::not_found ?
							ENOENT :
							EILSEQ);
			++observed.snapshots;
			const auto identity = std::lower_bound(identities.records.begin(),
							       identities.records.end(), pid,
							       [](const auto &value, int32_t sought)
							       { return value.pid < sought; });
			if (identity == identities.records.end() || identity->pid != pid)
				++observed.unindexed_snapshots;
			for (const auto &item : snapshot.items)
			{
				if (std::binary_search(born.begin(), born.end(), item.object_uid))
					return EEXIST;
				++observed.inventory_rows;
			}
			for (const auto &pet : snapshot.pets)
				for (const auto &item : pet.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.pet_item_rows;
				}
		}
		if (!identity_lock.matches(root) || !authority_lock.matches(root))
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

#include <sys/syscall.h>
namespace
{
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && defined(SYS_getdents64) &&  \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
struct ordinary_player_refusal
{
	unsigned int error;
};
size_t ordinary_player_add(size_t a, size_t b)
{
	if (b > SIZE_MAX - a)
		throw ordinary_player_refusal{ ENOBUFS };
	return a + b;
}
size_t ordinary_player_product(size_t count, size_t width)
{
	if (width && count > SIZE_MAX / width)
		throw ordinary_player_refusal{ ENOBUFS };
	return count * width;
}
size_t ordinary_player_string(const std::string &value)
{
	return value.capacity() > 15 ? ordinary_player_add(value.capacity(), 1) : 0;
}
constexpr size_t ordinary_player_callback_frames =
	sizeof(void *) * 10 + sizeof(size_t) * 10 + sizeof(bool) * 4;
struct ordinary_player_budget
{
	size_t outer;
	flatfile_scratch_reserve_fn reserve;
	void *context;
	void admit(size_t local) const
	{
		if (!reserve ||
		    !reserve(ordinary_player_add(
				     outer,
				     ordinary_player_add(local, ordinary_player_callback_frames)),
			     context))
			throw ordinary_player_refusal{ ENOBUFS };
	}
	ordinary_player_budget nested(size_t local) const
	{
		return { ordinary_player_add(outer, local), reserve, context };
	}
};
unsigned int ordinary_player_codec_error(economic_accounting_error value)
{
	return value == economic_accounting_error::capacity ||
			       value == economic_accounting_error::overflow ?
		       ENOBUFS :
	       value == economic_accounting_error::unresolved ? ENOTSUP :
								EINVAL;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool ordinary_player_digest(const uint8_t *bytes, size_t size, const uint8_t *expected,
			    ordinary_player_budget budget)
{
	const size_t frame = sizeof(SHA256_CTX) + SHA256_DIGEST_LENGTH + sizeof(void *) * 5 +
			     sizeof(size_t) * 2 + sizeof(int) * 3;
	if (sizeof(SHA_LONG) != 4 || OPENSSL_VERSION_MAJOR != 3 || OPENSSL_VERSION_MINOR != 0 ||
	    OPENSSL_VERSION_PATCH != 13)
		throw ordinary_player_refusal{ ENOTSUP };
	budget.admit(ordinary_player_add(frame, 1631 + 56));
	SHA256_CTX hash{};
	unsigned char digest[SHA256_DIGEST_LENGTH]{};
	return SHA256_Init(&hash) == 1 && SHA256_Update(&hash, bytes, size) == 1 &&
	       SHA256_Final(digest, &hash) == 1 &&
	       CRYPTO_memcmp(digest, expected, sizeof(digest)) == 0;
}
#pragma GCC diagnostic pop
struct ordinary_player_file_workspace
{
	std::vector<uint8_t> bytes;
	player_snapshot decoded{};
};
struct ordinary_player_fd
{
	int descriptor = -1;
	~ordinary_player_fd()
	{
		if (descriptor >= 0)
			close(descriptor);
	}
};
// Linux getdents64 wire prefix, read using memcpy instead of an unaligned
// type cast. Its name begins at byte19, after ino/off/reclen/type. No opaque
// DIR object/buffer or libc allocator is substituted for an admitted object.
struct ordinary_player_directory_workspace
{
	ordinary_player_fd owned;
	struct stat metadata
	{
	};
	std::array<uint8_t, 8192> bytes{};
	long received = 0;
	size_t offset = 0, name_size = 0;
	uint16_t record_size = 0;
	uint8_t record_type = 0;
	const char *name_data = nullptr, *terminator = nullptr;
	std::string_view name, numeric;
	std::from_chars_result parsed{};
	int32_t pid = 0;
};
// Complete same-FD namespace enumeration; all original .snapshot suffix and
// canonical positive PID rules apply. A missing/unsafe directory always errors.
// Linux getdents64 shares the retained d_name/type values with original readdir;
// record bounds/NUL checks secure this distinct byte-buffer interface.
void ordinary_player_enumerate(const std::string &directory, std::vector<int32_t> &pids,
			       ordinary_player_budget budget)
{
	if (!pids.empty() || pids.capacity())
		throw ordinary_player_refusal{ EINVAL };
	if (sizeof(ino_t) != 8 || sizeof(off_t) != 8 || offsetof(struct dirent, d_reclen) != 16 ||
	    offsetof(struct dirent, d_type) != 18 || offsetof(struct dirent, d_name) != 19)
		throw ordinary_player_refusal{ ENOTSUP };
	const size_t frame = sizeof(ordinary_player_directory_workspace) + sizeof(void *) * 5 +
			     sizeof(size_t) * 5 + sizeof(int) * 2 + sizeof(long);
	budget.admit(frame);
	ordinary_player_directory_workspace work;
	work.owned.descriptor =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (work.owned.descriptor < 0)
		throw ordinary_player_refusal{ errno ? static_cast<unsigned int>(errno) : EIO };
	if (fstat(work.owned.descriptor, &work.metadata) < 0)
		throw ordinary_player_refusal{ errno ? static_cast<unsigned int>(errno) : EIO };
	if (!S_ISDIR(work.metadata.st_mode) || work.metadata.st_uid != geteuid() ||
	    (work.metadata.st_mode & 0077))
		throw ordinary_player_refusal{ EILSEQ };
	for (;;)
	{
		errno = 0;
		work.received = syscall(SYS_getdents64, work.owned.descriptor, work.bytes.data(),
					work.bytes.size());
		if (work.received < 0)
		{
			// Original glibc readdir treats dead-directory ENOENT as EOF and
			// propagates EINTR rather than retrying. Keep that existing policy.
			if (errno == ENOENT)
				break;
			throw ordinary_player_refusal{ errno ? static_cast<unsigned int>(errno) :
							       EIO };
		}
		if (!work.received)
			break;
		if (static_cast<unsigned long>(work.received) > work.bytes.size())
			throw ordinary_player_refusal{ EILSEQ };
		for (work.offset = 0; work.offset < static_cast<size_t>(work.received);
		     work.offset += work.record_size)
		{
			if (static_cast<size_t>(work.received) - work.offset < 20)
				throw ordinary_player_refusal{ EILSEQ };
			std::memcpy(&work.record_size, work.bytes.data() + work.offset + 16,
				    sizeof(work.record_size));
			work.record_type = work.bytes[work.offset + 18];
			if (work.record_size < 20 ||
			    work.record_size > static_cast<size_t>(work.received) - work.offset ||
			    work.record_size % 8)
				throw ordinary_player_refusal{ EILSEQ };
			// Original readdir did not select by d_type: DT_UNKNOWN, symlink,
			// directory and regular entries are all read by the secure file leaf.
			// Only a legal Linux d_type value is accepted as a wire record.
			if (work.record_type != DT_UNKNOWN && work.record_type != DT_FIFO &&
			    work.record_type != DT_CHR && work.record_type != DT_DIR &&
			    work.record_type != DT_BLK && work.record_type != DT_REG &&
			    work.record_type != DT_LNK && work.record_type != DT_SOCK &&
			    work.record_type != DT_WHT)
				throw ordinary_player_refusal{ EILSEQ };
			work.name_data = reinterpret_cast<const char *>(work.bytes.data() +
									work.offset + 19);
			work.terminator = static_cast<const char *>(
				std::memchr(work.name_data, 0, work.record_size - 19));
			if (!work.terminator)
				throw ordinary_player_refusal{ EILSEQ };
			work.name_size = static_cast<size_t>(work.terminator - work.name_data);
			work.name = std::string_view(work.name_data, work.name_size);
			if (!work.name.ends_with(".snapshot"))
				continue;
			work.numeric =
				work.name.substr(0, work.name.size() - (sizeof(".snapshot") - 1));
			work.pid = 0;
			work.parsed = std::from_chars(work.numeric.data(),
						      work.numeric.data() + work.numeric.size(),
						      work.pid);
			if (work.parsed.ec != std::errc{} ||
			    work.parsed.ptr != work.numeric.data() + work.numeric.size() ||
			    work.pid <= 0)
				throw ordinary_player_refusal{ EILSEQ };
			// Positive int32 decimal plus suffix is <=19 characters. Construct
			// exact canonical bytes in real inline storage, not to_string temporaries.
			budget.admit(ordinary_player_add(
				frame,
				ordinary_player_add(ordinary_player_product(pids.capacity(),
									    sizeof(int32_t)),
						    sizeof(char[sizeof("2147483647.snapshot")]) +
							    sizeof(std::to_chars_result) +
							    sizeof(void *) * 2)));
			{
				char canonical[sizeof("2147483647.snapshot")]{};
				const auto converted = std::to_chars(
					canonical, canonical + sizeof(canonical), work.pid);
				if (converted.ec != std::errc{})
					throw ordinary_player_refusal{ EILSEQ };
				std::copy_n(".snapshot", sizeof(".snapshot") - 1, converted.ptr);
				if (std::string_view(canonical, static_cast<size_t>(converted.ptr -
										    canonical) +
									sizeof(".snapshot") - 1) !=
				    work.name)
					throw ordinary_player_refusal{ EILSEQ };
			}
			if (pids.size() == pids.capacity())
			{
				if (pids.size() > SIZE_MAX / 2)
					throw ordinary_player_refusal{ ENOBUFS };
				const size_t next = pids.size() ? pids.size() * 2 : 1;
				budget.admit(ordinary_player_add(
					frame,
					ordinary_player_add(
						ordinary_player_product(pids.capacity(),
									sizeof(int32_t)),
						ordinary_player_product(next, sizeof(int32_t)))));
			}
			pids.push_back(work.pid);
		}
	}
}
struct ordinary_player_physical_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	std::vector<uint64_t> born;
	flatfile_native_mobile_birth_ordinary_identity_current identities;
	flatfile_native_mobile_birth_ordinary_player_physical_absence observed;
	std::string directory;
	std::vector<int32_t> pids;
	size_t image_heap = 0, recipe_heap = 0, identity_heap = 0;
};
size_t ordinary_player_physical_heap(const ordinary_player_physical_workspace &work)
{
	return ordinary_player_add(
		work.image_heap,
		ordinary_player_add(
			work.recipe_heap,
			ordinary_player_add(
				work.intent.admission.facts.capacity(),
				ordinary_player_add(
					work.identity_heap,
					ordinary_player_add(
						ordinary_player_string(work.directory),
						ordinary_player_add(ordinary_player_product(
									    work.born.capacity(),
									    sizeof(uint64_t)),
								    ordinary_player_product(
									    work.pids.capacity(),
									    sizeof(int32_t))))))));
}
#endif
}

flatfile_player_load_result flatfile_player_snapshot_read_file_bounded(
	const std::string &directory, const std::string &filename, int32_t pid,
	player_snapshot *output, flatfile_scratch_reserve_fn reserve, void *context, size_t outer,
	size_t *retained_snapshot_heap) noexcept
{
	if (pid <= 0 || !output || !reserve)
		return flatfile_player_load_result::invalid;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) ||                    \
	!defined(SYS_getdents64) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)directory;
	(void)filename;
	(void)context;
	(void)outer;
	(void)retained_snapshot_heap;
	errno = ENOTSUP;
	return flatfile_player_load_result::io_error;
#else
	try
	{
		const size_t frame = sizeof(ordinary_player_file_workspace) + sizeof(decoder) +
				     sizeof(void *) * 10 + sizeof(size_t) * 5 +
				     sizeof(uint32_t) * 2 + sizeof(int32_t) + sizeof(uint64_t) * 2 +
				     sizeof(flatfile_read_result) +
				     sizeof(player_snapshot_codec_result);
		ordinary_player_budget budget{ outer, reserve, context };
		budget.admit(frame);
		ordinary_player_file_workspace work;
		const auto loaded = flatfile_read_bounded(directory, filename, player_file_maximum,
							  &work.bytes, reserve, context,
							  budget.nested(frame).outer);
		if (loaded == flatfile_read_result::not_found)
			return flatfile_player_load_result::not_found;
		if (loaded == flatfile_read_result::invalid)
			return flatfile_player_load_result::invalid;
		if (loaded != flatfile_read_result::ok)
			return flatfile_player_load_result::io_error;
		const size_t live = ordinary_player_add(frame, work.bytes.capacity());
		constexpr size_t header_size = player_magic.size() + sizeof(uint32_t) * 2 +
					       sizeof(int32_t) + sizeof(uint64_t) * 2 +
					       SHA256_DIGEST_LENGTH;
		if (work.bytes.size() < header_size ||
		    memcmp(work.bytes.data(), player_magic.data(), player_magic.size()))
			return flatfile_player_load_result::invalid;
		decoder header{ work.bytes.data() + player_magic.size(),
				work.bytes.size() - player_magic.size() };
		uint32_t version = 0, payload_size = 0;
		int32_t stored_pid = 0;
		uint64_t stored_revision = 0, stored_components = 0;
		if (!header.number(&version) || !header.number(&payload_size) ||
		    !header.number(&stored_pid) || !header.number(&stored_revision) ||
		    !header.number(&stored_components) || version != player_file_version ||
		    stored_pid != pid || !stored_revision ||
		    stored_components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
		    payload_size != work.bytes.size() - header_size)
			return flatfile_player_load_result::invalid;
		if (!ordinary_player_digest(work.bytes.data() + header_size, payload_size,
					    work.bytes.data() + player_magic.size() +
						    sizeof(uint32_t) * 2 + sizeof(int32_t) +
						    sizeof(uint64_t) * 2,
					    budget.nested(live)))
			return flatfile_player_load_result::invalid;
		size_t heap = 0;
		errno = 0;
		const auto decoded = player_snapshot_decode_bounded(
			work.bytes.data() + header_size, payload_size, &work.decoded, reserve,
			context, budget.nested(live).outer, &heap);
		if (decoded != player_snapshot_codec_result::ok)
		{
			if (errno == ENOTSUP)
				return flatfile_player_load_result::io_error;
			if (decoded == player_snapshot_codec_result::allocation_failure ||
			    errno == ENOBUFS)
			{
				errno = errno == ENOBUFS ? ENOBUFS : ENOMEM;
				return flatfile_player_load_result::io_error;
			}
			return flatfile_player_load_result::invalid;
		}
		if (work.decoded.pid != stored_pid || work.decoded.revision != stored_revision ||
		    work.decoded.components != stored_components)
			return flatfile_player_load_result::invalid;
		*output = std::move(work.decoded);
		if (retained_snapshot_heap)
			*retained_snapshot_heap = heap;
		return flatfile_player_load_result::ok;
	}
	catch (const ordinary_player_refusal &failure)
	{
		errno = failure.error;
		return flatfile_player_load_result::io_error;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_player_load_result::io_error;
	}
	catch (...)
	{
		errno = EIO;
		return flatfile_player_load_result::io_error;
	}
#endif
}

unsigned int flatfile_native_mobile_birth_ordinary_player_physical_storage::verify_locked_bounded(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_player_physical_absence *output,
	flatfile_scratch_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (root.empty() || !output || !reserve || !identity_lock.matches(root) ||
	    !authority_lock.matches(root))
		return EINVAL;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) ||                    \
	!defined(SYS_getdents64) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)original;
	(void)context;
	(void)outer;
	return ENOTSUP;
#else
	try
	{
		const size_t frame = sizeof(ordinary_player_physical_workspace) +
				     sizeof(ordinary_player_budget) + sizeof(void *) * 12 +
				     sizeof(size_t) * 6 + sizeof(economic_accounting_error) * 4 +
				     sizeof(flatfile_identity_result) + sizeof(unsigned int) +
				     sizeof(void *);
		ordinary_player_budget budget{ outer, reserve, context };
		budget.admit(frame);
		const auto recovery_status =
			native_mobile_birth_cash_role_recovery_validate_bounded(
				original, reserve, context, budget.nested(frame).outer);
		if (recovery_status != economic_accounting_error::ok)
			return ordinary_player_codec_error(recovery_status);
		ordinary_player_physical_workspace work;
		auto result = native_mobile_birth_cash_role_command_decode_bounded(
			original.command, &work.image, &work.recipes, &work.role, reserve, context,
			budget.nested(frame).outer, &work.image_heap, &work.recipe_heap);
		if (result != economic_accounting_error::ok)
			return ordinary_player_codec_error(result);
		if (work.role.role != native_mobile_birth_cash_role::ordinary_wallet)
			return EINVAL;
		work.intent_wire = original.command.accounting_intent;
		result = economic_intent_decode_bounded(
			work.intent_wire, &work.intent, reserve, context,
			budget.nested(ordinary_player_add(frame,
							  ordinary_player_physical_heap(work)))
				.outer);
		if (result != economic_accounting_error::ok)
			return ordinary_player_codec_error(result);
		result = economic_intent_verify_binding_bounded(
			original.command, work.intent, reserve, context,
			budget.nested(ordinary_player_add(frame,
							  ordinary_player_physical_heap(work)))
				.outer);
		if (result != economic_accounting_error::ok)
			return ordinary_player_codec_error(result);
		if (!work.intent.admission.metadata.source_event)
			return EINVAL;
		budget.admit(ordinary_player_add(
			frame, ordinary_player_add(ordinary_player_physical_heap(work),
						   ordinary_player_product(work.image.items.size(),
									   sizeof(uint64_t)))));
		work.born.reserve(work.image.items.size());
		for (const auto &literal : work.image.items)
			work.born.push_back(literal.object_uid);
		budget.admit(ordinary_player_add(frame, ordinary_player_physical_heap(work)));
		std::sort(work.born.begin(), work.born.end());
		if (std::adjacent_find(work.born.begin(), work.born.end()) != work.born.end() ||
		    (!work.born.empty() && !work.born.front()))
			return EINVAL;
		errno = 0;
		const auto identity_status =
			flatfile_native_mobile_birth_ordinary_identity_storage::read_locked_bounded(
				root, identity_lock, authority_lock, &work.identities, reserve,
				context,
				budget.nested(ordinary_player_add(
						      frame, ordinary_player_physical_heap(work)))
					.outer,
				&work.identity_heap);
		if (identity_status != flatfile_identity_result::ok)
			return identity_status == flatfile_identity_result::io_error ?
				       (errno == ENOBUFS ? ENOBUFS :
					errno == ENOMEM	 ? ENOMEM :
					errno == ENOTSUP ? ENOTSUP :
							   EIO) :
				       EILSEQ;
		work.observed.identity_catalog_revision = work.identities.catalog_revision;
		work.observed.identities = work.identities.records.size();
		for (const auto &identity : work.identities.records)
			if (!identity.active)
				++work.observed.retired_identities;
		{
			const size_t length =
				ordinary_player_add(root.size(), sizeof("/players") - 1);
			budget.admit(ordinary_player_add(
				frame,
				ordinary_player_add(
					ordinary_player_physical_heap(work),
					sizeof(std::string) +
						(length > 15 ? ordinary_player_add(length, 1) :
							       0))));
			std::string directory(length, '\0');
			std::copy(root.begin(), root.end(), directory.begin());
			std::copy_n("/players", sizeof("/players") - 1,
				    directory.begin() + root.size());
			work.directory = std::move(directory);
		}
		ordinary_player_enumerate(work.directory, work.pids,
					  budget.nested(ordinary_player_add(
						  frame, ordinary_player_physical_heap(work))));
		budget.admit(ordinary_player_add(frame, ordinary_player_physical_heap(work)));
		std::sort(work.pids.begin(), work.pids.end());
		if (std::adjacent_find(work.pids.begin(), work.pids.end()) != work.pids.end())
			return EILSEQ;
		for (int32_t pid : work.pids)
		{
			const size_t child_frame = sizeof(player_snapshot) + sizeof(std::string) +
						   sizeof(size_t) * 2 + sizeof(int32_t) +
						   sizeof(flatfile_player_load_result) +
						   sizeof(char[20]) + sizeof(std::to_chars_result) +
						   sizeof(void *) * 5;
			// Original decimal int32 filename is <=19 bytes. Its new exact-size
			// constructor allocates length+1; no to_string concatenation scratch.
			budget.admit(ordinary_player_add(
				frame, ordinary_player_add(ordinary_player_physical_heap(work),
							   child_frame + 20)));
			char name[20]{};
			const auto numeric = std::to_chars(name, name + sizeof(name), pid);
			if (numeric.ec != std::errc{})
				return EILSEQ;
			std::copy_n(".snapshot", sizeof(".snapshot") - 1, numeric.ptr);
			std::string filename(name, static_cast<size_t>(numeric.ptr - name) +
							   sizeof(".snapshot") - 1);
			player_snapshot snapshot{};
			size_t snapshot_heap = 0;
			errno = 0;
			const auto loaded = flatfile_player_snapshot_read_file_bounded(
				work.directory, filename, pid, &snapshot, reserve, context,
				budget
					.nested(ordinary_player_add(
						frame,
						ordinary_player_add(
							ordinary_player_physical_heap(work),
							ordinary_player_add(
								child_frame,
								ordinary_player_string(filename)))))
					.outer,
				&snapshot_heap);
			if (loaded != flatfile_player_load_result::ok)
				return loaded == flatfile_player_load_result::io_error ?
					       (errno == ENOBUFS ? ENOBUFS :
						errno == ENOMEM	 ? ENOMEM :
						errno == ENOTSUP ? ENOTSUP :
								   EIO) :
					       (loaded == flatfile_player_load_result::not_found ?
							ENOENT :
							EILSEQ);
			budget.admit(ordinary_player_add(
				frame, ordinary_player_add(
					       ordinary_player_physical_heap(work),
					       ordinary_player_add(
						       child_frame,
						       ordinary_player_add(
							       ordinary_player_string(filename),
							       snapshot_heap)))));
			++work.observed.snapshots;
			const auto identity = std::lower_bound(work.identities.records.begin(),
							       work.identities.records.end(), pid,
							       [](const auto &value, int32_t sought)
							       { return value.pid < sought; });
			if (identity == work.identities.records.end() || identity->pid != pid)
				++work.observed.unindexed_snapshots;
			for (const auto &item : snapshot.items)
			{
				if (std::binary_search(work.born.begin(), work.born.end(),
						       item.object_uid))
					return EEXIST;
				++work.observed.inventory_rows;
			}
			for (const auto &pet : snapshot.pets)
				for (const auto &item : pet.items)
				{
					if (std::binary_search(work.born.begin(), work.born.end(),
							       item.object_uid))
						return EEXIST;
					++work.observed.pet_item_rows;
				}
		}
		if (!identity_lock.matches(root) || !authority_lock.matches(root))
			return EINVAL;
		*output = work.observed;
		return 0;
	}
	catch (const ordinary_player_refusal &failure)
	{
		return failure.error;
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
