// Independent, read-only retained authority validation. Do not call storage
// readers here: those readers recover journals and can change the candidate.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_AUTHORITY_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_AUTHORITY_H

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <deque>
#include <dirent.h>
#include <filesystem>
#include <fcntl.h>
#include <openssl/sha.h>
#include <set>
#include <span>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <vector>

namespace restore_economic_authority
{
using bytes = std::vector<uint8_t>;
using identity = std::array<uint8_t, 16>;
using digest = std::array<uint8_t, 32>;
constexpr size_t buckets = 256, bucket_capacity = 4096, maximum_bytes = 2 * 1024 * 1024;

inline void need(bool valid)
{
	if (!valid)
		throw std::runtime_error("native_restore_qualification_failed");
}
// Only the online operator audit installs this cooperative admission budget.
// Offline candidate qualification keeps the existing format limits. Count
// repeated physical reads and directory visits, including ignored filenames.
struct audit_budget_refused : std::runtime_error
{
	audit_budget_refused()
		: std::runtime_error("native_restore_qualification_failed")
	{
	}
};
struct audit_budget
{
	// A valid 3071-holding lifecycle fixture uses 9574 physical reads through
	// the bounded eight-bucket caches. Keep byte/time bounds while admitting it.
	size_t remaining_bytes = 128 * 1024 * 1024, remaining_files = 16384;
	size_t remaining_entries = 8192;
	std::chrono::steady_clock::time_point deadline =
		std::chrono::steady_clock::now() + std::chrono::seconds(30);
	void checkpoint() const
	{
		if (std::chrono::steady_clock::now() >= deadline)
			throw audit_budget_refused();
	}
	void file(size_t size)
	{
		checkpoint();
		if (!remaining_files || size > remaining_bytes)
			throw audit_budget_refused();
		--remaining_files;
		remaining_bytes -= size;
	}
	void entry()
	{
		checkpoint();
		if (!remaining_entries)
			throw audit_budget_refused();
		--remaining_entries;
	}
};
inline thread_local audit_budget *current_audit_budget = nullptr;
struct scoped_audit_budget
{
	audit_budget *previous;
	explicit scoped_audit_budget(audit_budget &budget)
		: previous(current_audit_budget)
	{
		current_audit_budget = &budget;
	}
	~scoped_audit_budget() { current_audit_budget = previous; }
	scoped_audit_budget(const scoped_audit_budget &) = delete;
	scoped_audit_budget &operator=(const scoped_audit_budget &) = delete;
};
inline void audit_checkpoint()
{
	if (current_audit_budget)
		current_audit_budget->checkpoint();
}
inline void audit_directory_entry()
{
	if (current_audit_budget)
		current_audit_budget->entry();
}

// Independent SELECT-equivalent lock acquisition. Never creates a lock, opens
// native storage, or recovers a journal. Cooperating native writers take the
// same inode exclusively; readers fail immediately if a writer holds it.
class authority_read_lock
{
	struct fd_owner
	{
		int value = -1;
		~fd_owner()
		{
			if (value >= 0)
				close(value);
		}
	};
	std::filesystem::path root;
	fd_owner root_fd, domains_fd, evidence_fd, lock_fd;
	static void directory(int fd)
	{
		struct stat info = {};
		need(fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) && info.st_uid == geteuid() &&
		     !(info.st_mode & 0077));
	}
	static void unchanged(int fd, int parent, const char *name)
	{
		struct stat held = {}, named = {};
		need(fstat(fd, &held) == 0 &&
		     fstatat(parent, name, &named, AT_SYMLINK_NOFOLLOW) == 0 &&
		     held.st_dev == named.st_dev && held.st_ino == named.st_ino);
	}
	bool empty_evidence() const
	{
		if (evidence_fd.value < 0)
			return true;
		const int duplicate = dup(evidence_fd.value);
		need(duplicate >= 0);
		DIR *stream = fdopendir(duplicate);
		if (!stream)
			close(duplicate);
		need(stream);
		bool empty = true;
		int error = 0;
		while (true)
		{
			errno = 0;
			auto entry = readdir(stream);
			if (!entry)
			{
				error = errno;
				break;
			}
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
			{
				empty = false;
				break;
			}
		}
		closedir(stream);
		need(error == 0);
		return empty;
	}
	void no_pending() const
	{
		if (domains_fd.value >= 0)
		{
			struct stat info = {};
			need(fstatat(domains_fd.value, ".critical-authority-transaction", &info,
				     AT_SYMLINK_NOFOLLOW) == -1 &&
			     errno == ENOENT);
		}
	}

    public:
	explicit authority_read_lock(const std::filesystem::path &path)
		: root(path)
	{
		need(path.is_absolute());
		root_fd.value = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
		directory(root_fd.value);
		for (auto [name, descriptor] : { std::pair{ "domains", &domains_fd },
						 std::pair{ "economic-evidence", &evidence_fd } })
		{
			descriptor->value = openat(root_fd.value, name,
						   O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
			if (descriptor->value < 0)
				need(errno == ENOENT);
			else
				directory(descriptor->value);
		}
		if (domains_fd.value >= 0)
		{
			lock_fd.value = openat(domains_fd.value, ".critical-authority.lock",
					       O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
			if (lock_fd.value < 0)
				need(errno == ENOENT);
			else
			{
				struct stat info = {};
				need(fstat(lock_fd.value, &info) == 0 && S_ISREG(info.st_mode) &&
				     info.st_uid == geteuid() && info.st_nlink == 1 &&
				     !(info.st_mode & 0077));
				need(flock(lock_fd.value, LOCK_SH | LOCK_NB) == 0);
			}
		}
		no_pending();
		// Legacy absent/empty accounting needs no initialization. Return the
		// empty observation directly; never start an unlocked multi-file scan.
		need(locked() || empty_evidence());
		finish();
	}
	bool locked() const { return lock_fd.value >= 0; }
	void finish() const
	{
		unchanged(root_fd.value, AT_FDCWD, root.c_str());
		if (domains_fd.value >= 0)
			unchanged(domains_fd.value, root_fd.value, "domains");
		if (evidence_fd.value >= 0)
			unchanged(evidence_fd.value, root_fd.value, "economic-evidence");
		if (locked())
			unchanged(lock_fd.value, domains_fd.value, ".critical-authority.lock");
		no_pending();
	}
	authority_read_lock(const authority_read_lock &) = delete;
	authority_read_lock &operator=(const authority_read_lock &) = delete;
};
inline bool nonzero(std::span<const uint8_t> value)
{
	return std::any_of(value.begin(), value.end(), [](auto byte) { return byte != 0; });
}
inline digest hash(std::span<const uint8_t> value)
{
	digest result;
	SHA256(value.data(), value.size(), result.data());
	return result;
}
inline bool same(std::span<const uint8_t> first, std::span<const uint8_t> second)
{
	return first.size() == second.size() &&
	       std::equal(first.begin(), first.end(), second.begin());
}
struct reader
{
	std::span<const uint8_t> value;
	size_t offset = 0;
	std::span<const uint8_t> take(size_t count)
	{
		audit_checkpoint();
		need(offset <= value.size() && count <= value.size() - offset);
		auto part = value.subspan(offset, count);
		offset += count;
		return part;
	}
	uint64_t number(size_t width)
	{
		need(width <= 8);
		uint64_t result = 0;
		auto part = take(width);
		for (size_t i = 0; i < width; ++i)
			result |= uint64_t(part[i]) << (8 * i);
		return result;
	}
	template <size_t N> std::array<uint8_t, N> fixed()
	{
		std::array<uint8_t, N> result;
		auto part = take(N);
		std::copy(part.begin(), part.end(), result.begin());
		return result;
	}
	void done() { need(offset == value.size()); }
};
inline uint64_t number(std::span<const uint8_t> value, size_t offset, size_t width)
{
	reader in{ value };
	(void)in.take(offset);
	return in.number(width);
}
inline std::string filename(const char *prefix, size_t bucket, const char *suffix)
{
	constexpr char hex[] = "0123456789abcdef";
	return std::string(prefix) + hex[bucket >> 4] + hex[bucket & 15] + suffix;
}
inline void put(bytes &out, uint64_t value, size_t width)
{
	for (size_t i = 0; i < width; ++i)
		out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
// Validate the native locator independently of the production account codec.
inline void locator(uint64_t kind, uint64_t context, uint64_t type, uint64_t native,
		    std::span<const uint8_t> name, bool key = false)
{
	need(type == kind);
	if (kind == 2)
	{
		need(context <= 127 && (key || native > 0) && !name.empty() && name.size() <= 50);
		for (auto c : name)
			need((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
			     c == '-');
	}
	else
	{
		need(kind == 1 || kind == 4 || kind == 5 || kind == 6);
		const uint64_t limit = kind == 6 ? uint64_t{ UINT32_MAX } + 1 :
				       kind == 4 ? UINT32_MAX :
						   INT32_MAX;
		need(context == 0 && native > 0 && native <= limit && name.empty());
	}
}
inline bytes file_bytes(const std::filesystem::path &directory, const std::string &name,
			size_t limit, const digest &expected = {})
{
	audit_checkpoint();
	const int fd =
		open((directory / name).c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	need(fd >= 0);
	struct closer
	{
		int fd;
		~closer() { close(fd); }
	} close_file{ fd };
	struct stat info = {};
	need(fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_uid == geteuid() &&
	     info.st_nlink == 1 && !(info.st_mode & 0077) && info.st_size >= 48 &&
	     uint64_t(info.st_size) <= limit);
	if (current_audit_budget)
		current_audit_budget->file(static_cast<size_t>(info.st_size));
	bytes encoded(static_cast<size_t>(info.st_size));
	size_t offset = 0;
	while (offset < encoded.size())
	{
		audit_checkpoint();
		auto count = read(fd, encoded.data() + offset, encoded.size() - offset);
		if (count < 0 && errno == EINTR)
			continue;
		need(count > 0);
		offset += static_cast<size_t>(count);
	}
	uint8_t extra;
	need(read(fd, &extra, 1) == 0);
	need(!nonzero(expected) || hash(encoded) == expected);
	return encoded;
}
inline bytes frame(const std::filesystem::path &directory, const std::string &name,
		   const char *magic, size_t limit = maximum_bytes, const digest &expected = {},
		   uint32_t *catalog_version = nullptr)
{
	auto encoded = file_bytes(directory, name, limit, expected);
	reader in{ encoded };
	auto prefix = in.take(8);
	auto version = in.number(4);
	need(memcmp(prefix.data(), magic, 8) == 0 &&
	     (version == 1 || (catalog_version && memcmp(magic, "DURECE1", 8) == 0 &&
			       (version == 2 || version == 3))) &&
	     in.number(4) == encoded.size() - 48);
	auto body_digest = in.fixed<32>();
	auto body = in.take(encoded.size() - 48);
	need(body_digest == hash(body));
	if (catalog_version)
		*catalog_version = version;
	return { body.begin(), body.end() };
}
enum class baseline_initialization : uint8_t
{
	legacy_unknown = 0,
	never_initialized = 1,
	initialized = 2
};
enum class initialization_origin : uint8_t
{
	legacy_unknown = 0,
	baseline_participant = 1,
	lifecycle_owner = 2
};
struct epoch_marker
{
	identity epoch = {}, initializing_operation = {};
	identity predecessor = {}, creating_operation = {};
	uint64_t ordinal = 0;
	uint16_t transition_kind = 0;
	digest transition_digest = {};
	baseline_initialization initialization = baseline_initialization::legacy_unknown;
	initialization_origin origin = initialization_origin::legacy_unknown;
	std::array<uint8_t, 40> opening = {};
};
struct epoch_catalog
{
	identity lineage = {}, last_epoch = {};
	std::vector<epoch_marker> entries;
};
// One independent bounded decoder serves authority cross-links and baseline
// discovery. No storage/recovery codec participates in this read-only gate.
inline epoch_catalog catalog(const std::filesystem::path &directory, const digest &expected)
{
	uint32_t version = 0;
	auto body = frame(directory, "epochs.eae", "DURECE1", maximum_bytes, expected, &version);
	reader in{ body };
	epoch_catalog result;
	result.lineage = in.fixed<16>();
	auto count = in.number(4);
	need(nonzero(result.lineage) && count <= 4096 && in.number(4) == 0);
	std::set<identity> seen;
	std::set<identity> lifecycle_operations;
	for (size_t i = 0; i < count; ++i)
	{
		epoch_marker entry;
		entry.epoch = in.fixed<16>();
		entry.ordinal = in.number(8);
		entry.predecessor = in.fixed<16>();
		entry.transition_kind = in.number(2);
		need(in.number(6) == 0);
		entry.transition_digest = in.fixed<32>();
		entry.creating_operation = in.fixed<16>();
		need(nonzero(entry.epoch) && seen.insert(entry.epoch).second &&
		     entry.ordinal == i + 1 && entry.predecessor == result.last_epoch &&
		     entry.transition_kind && nonzero(entry.transition_digest) &&
		     nonzero(entry.creating_operation));
		if (version >= 2)
		{
			auto state = in.number(1);
			need(state <= 2);
			entry.initialization = static_cast<baseline_initialization>(state);
			if (version == 3)
			{
				auto origin = in.number(1);
				need(origin <= 2 && in.number(6) == 0 && (!origin || state == 2));
				entry.origin = static_cast<initialization_origin>(origin);
			}
			else
				need(in.number(7) == 0);
			entry.initializing_operation = in.fixed<16>();
			entry.opening = in.fixed<40>();
			if (entry.initialization == baseline_initialization::initialized)
			{
				reader key{ entry.opening };
				need(nonzero(entry.initializing_operation) &&
				     key.fixed<16>() == result.lineage && key.number(2) == 1 &&
				     key.number(2) == 9 && key.number(8) != 0);
				(void)key.number(
					8); // Preserve the exact opening context, including zero.
				need(key.number(4) == 0);
				key.done();
			}
			else
				need(!nonzero(entry.initializing_operation) &&
				     !nonzero(entry.opening));
			if (entry.origin == initialization_origin::lifecycle_owner)
				need(entry.creating_operation == entry.initializing_operation &&
				     entry.transition_kind == 1 &&
				     lifecycle_operations.insert(entry.initializing_operation)
					     .second);
		}
		result.last_epoch = entry.epoch;
		result.entries.push_back(entry);
	}
	in.done();
	return result;
}
struct mapping
{
	uint64_t authority;
	bytes key;
	bool retired;
};
struct native_entry
{
	bytes key;
	uint64_t active, last;
};
struct authority_page
{
	identity lineage = {};
	digest authority_body = {};
	bytes cursor, ceiling;
	size_t rows = 0, verified = 0, bucket_rows = 0;
	bool exhausted = false;
	std::vector<bytes> invalid_links;
};
// At most eight decoded buckets are cached during either cross-link pass.
// Each bucket is bounded by 4096 entries and a 2 MiB frame, independent of the
// number of retained mapping lifetimes in the store.
template <typename T> struct cache
{
	std::deque<std::pair<size_t, std::vector<T>>> entries;
	template <typename Load> const std::vector<T> &get(size_t bucket, Load load)
	{
		for (const auto &entry : entries)
			if (entry.first == bucket)
				return entry.second;
		if (entries.size() == 8)
			entries.pop_front();
		entries.emplace_back(bucket, load(bucket));
		return entries.back().second;
	}
};
class checker
{
	std::filesystem::path directory;
	identity lineage = {}, last_epoch = {};
	uint64_t next_mapping = 0, epoch_count = 0;
	digest epochs_digest = {}, control_digest = {};
	std::array<digest, buckets> native_digests = {}, mapping_digests = {};
	mutable cache<mapping> historic_accounts;

	void absent(const std::string &name) const
	{
		struct stat info = {};
		need(lstat((directory / name).c_str(), &info) != 0 && errno == ENOENT);
	}
	bytes frame(const std::string &name, const char *magic, const digest &expected = {}) const
	{
		return restore_economic_authority::frame(directory, name, magic, maximum_bytes,
							 expected);
	}
	size_t mapping_count(size_t bucket) const
	{
		const uint64_t first = bucket ? bucket : 256;
		return next_mapping <= first ? 0 : 1 + (next_mapping - 1 - first) / 256;
	}
	void control()
	{
		auto body = frame("authority.eal", "DURECA1");
		control_digest = hash(body);
		reader in{ body };
		lineage = in.fixed<16>();
		need(nonzero(lineage) && nonzero(in.take(16)) && nonzero(in.take(16)));
		last_epoch = in.fixed<16>();
		auto active = in.fixed<16>();
		(void)in.number(
			8); // Revision is an unsigned durable counter, including UINT64_MAX.
		next_mapping = in.number(8);
		epoch_count = in.number(4);
		need(in.number(4) == 0 && next_mapping > 0 &&
		     next_mapping <= buckets * bucket_capacity + 1 && epoch_count <= 4096 &&
		     nonzero(last_epoch) == (epoch_count != 0) &&
		     (!nonzero(active) || active == last_epoch));
		epochs_digest = in.fixed<32>();
		need(nonzero(epochs_digest));
		for (auto &value : native_digests)
			value = in.fixed<32>();
		for (size_t bucket = 0; bucket < buckets; ++bucket)
		{
			mapping_digests[bucket] = in.fixed<32>();
			need(nonzero(mapping_digests[bucket]) == (mapping_count(bucket) != 0));
		}
		(void)in.take(32); // Evidence initialization belongs to the retained-record check.
		in.done();
	}
	void epochs() const
	{
		auto decoded = catalog(directory, epochs_digest);
		need(decoded.lineage == lineage && decoded.entries.size() == epoch_count &&
		     decoded.last_epoch == last_epoch);
	}
	std::vector<mapping> mappings(size_t bucket) const
	{
		auto name = filename("mapping-", bucket, ".eam");
		if (!nonzero(mapping_digests[bucket]))
		{
			absent(name);
			return {};
		}
		auto body = frame(name, "DURECM1", mapping_digests[bucket]);
		reader in{ body };
		need(in.fixed<16>() == lineage && in.number(4) == bucket);
		auto count = in.number(4);
		need(count == mapping_count(bucket) && count <= bucket_capacity);
		std::vector<mapping> result;
		for (size_t i = 0; i < count; ++i)
		{
			reader row{ in.take(in.number(4)) };
			need(row.fixed<16>() == lineage && row.number(2) == 1);
			auto kind = row.number(2), authority = row.number(8),
			     context = row.number(8);
			need(row.number(4) == 0 && authority == (bucket ? bucket : 256) + 256 * i);
			auto type = row.number(2), length = row.number(2), native = row.number(8);
			auto name_bytes = row.take(length);
			locator(kind, context, type, native, name_bytes);
			need(kind != 2 || native == authority);
			auto creating = row.fixed<16>(), retiring = row.fixed<16>(),
			     last = row.fixed<16>();
			auto revision = row.number(8);
			row.done();
			need(nonzero(creating) && nonzero(last) &&
			     (revision || (last == creating && !nonzero(retiring))) &&
			     (!nonzero(retiring) || (last == retiring && revision)));
			bytes key;
			put(key, kind, 2);
			put(key, context, 8);
			put(key, type, 2);
			if (kind == 2)
				key.insert(key.end(), name_bytes.begin(), name_bytes.end());
			else
				put(key, native, 8);
			result.push_back({ authority, std::move(key), nonzero(retiring) });
		}
		in.done();
		return result;
	}
	std::vector<native_entry> natives(size_t bucket) const
	{
		auto name = filename("native-", bucket, ".ean");
		if (!nonzero(native_digests[bucket]))
		{
			absent(name);
			return {};
		}
		auto body = frame(name, "DURECN1", native_digests[bucket]);
		reader in{ body };
		need(in.fixed<16>() == lineage && in.number(4) == bucket);
		auto count = in.number(4);
		need(count <= bucket_capacity);
		std::vector<native_entry> result;
		for (size_t i = 0; i < count; ++i)
		{
			auto length = in.number(2);
			need(length <= 62 && in.number(2) == 0);
			auto active = in.number(8), last = in.number(8);
			auto key = in.take(length);
			reader fields{ key };
			auto kind = fields.number(2), context = fields.number(8),
			     type = fields.number(2);
			auto native = kind == 2 ? 0 : fields.number(8);
			auto name_bytes = fields.take(key.size() - fields.offset);
			locator(kind, context, type, native, name_bytes, true);
			need(hash(key)[0] == bucket && last > 0 && last < next_mapping &&
			     (!active || active == last));
			bytes owned{ key.begin(), key.end() };
			need(result.empty() || result.back().key < owned);
			result.push_back({ std::move(owned), active, last });
		}
		in.done();
		return result;
	}
	void mapping_link(const mapping &row, cache<native_entry> &native_cache) const
	{
		if (row.retired)
			return;
		const auto &index =
			native_cache.get(hash(row.key)[0], [&](auto b) { return natives(b); });
		auto found = std::lower_bound(index.begin(), index.end(), row.key,
					      [](const auto &entry, const auto &key)
					      { return entry.key < key; });
		need(found != index.end() && found->key == row.key &&
		     found->active == row.authority);
	}
	void native_link(const native_entry &entry, cache<mapping> &mapping_cache) const
	{
		const auto &values =
			mapping_cache.get(entry.last % 256, [&](auto b) { return mappings(b); });
		const auto position = (entry.last - 1) / 256;
		need(position < values.size());
		const auto &row = values[position];
		if (entry.active)
			need(!row.retired && row.key == entry.key);
		else
		{
			need(row.key.size() >= 12 && entry.key.size() >= 12 &&
			     std::equal(row.key.begin(), row.key.begin() + 12, entry.key.begin()));
			const bool bank = row.key[0] == 2 && row.key[1] == 0;
			need(bank || row.key == entry.key);
			need(row.retired || (bank && row.key != entry.key));
		}
	}

    public:
	explicit checker(const std::filesystem::path &root)
		: directory(root / "economic-evidence")
	{
	}
	// Page callers establish only the control/catalog context. Whole-store
	// mapping/native closure remains in run(); it is not repeated per root page.
	void begin_page()
	{
		control();
		epochs();
	}
	// Two selected cross-links, with bounded independent frames for their
	// counterpart buckets. Native key order is a traversal fence, not commit order.
	authority_page page(bool mapping_direction, size_t bucket, const bytes &after,
			    const bytes &ceiling, bool ceiling_known)
	{
		need(bucket < buckets && (after.empty() || ceiling_known) &&
		     (!ceiling_known || after <= ceiling));
		begin_page();
		authority_page result;
		result.lineage = lineage;
		result.authority_body = control_digest;
		result.cursor = after;
		auto visit = [&](const auto &values, auto key, auto verify)
		{
			result.bucket_rows = values.size();
			auto retained = [&](const bytes &saved)
			{
				return std::any_of(values.begin(), values.end(),
						   [&](const auto &row)
						   { return key(row) == saved; });
			};
			need((after.empty() || retained(after)) &&
			     (!ceiling_known || ceiling.empty() || retained(ceiling)));
			result.ceiling = ceiling_known	? ceiling :
					 values.empty() ? bytes{} :
							  key(values.back());
			result.exhausted = true;
			for (const auto &row : values)
			{
				auto current = key(row);
				if (current <= after || current > result.ceiling)
					continue;
				if (result.rows == 2)
				{
					result.exhausted = false;
					break;
				}
				try
				{
					verify(row);
					++result.verified;
				}
				catch (const audit_budget_refused &)
				{
					throw;
				}
				catch (const std::runtime_error &)
				{
					result.invalid_links.push_back(current);
				}
				result.cursor = std::move(current);
				++result.rows;
			}
		};
		if (mapping_direction)
		{
			cache<native_entry> native_cache;
			visit(
				mappings(bucket),
				[](const mapping &row)
				{
					bytes result;
					put(result, row.authority, 8);
					std::reverse(result.begin(), result.end());
					return result;
				},
				[&](const auto &row) { mapping_link(row, native_cache); });
		}
		else
		{
			cache<mapping> mapping_cache;
			visit(
				natives(bucket), [](const native_entry &row) { return row.key; },
				[&](const auto &row) { native_link(row, mapping_cache); });
		}
		return result;
	}
	// Historical account identity is immutable even when locator aliases and
	// mapping revision/operation metadata have subsequently changed.
	bool mapped_account(std::span<const uint8_t> account) const
	{
		if (account.size() != 40)
			return false;
		reader key{ account };
		if (key.fixed<16>() != lineage || key.number(2) != 1)
			return false;
		auto kind = key.number(2), id = key.number(8), context = key.number(8);
		if (key.number(4) != 0 || !id || id >= next_mapping)
			return false;
		const auto &rows = historic_accounts.get(id % 256, [&](auto bucket)
							 { return mappings(bucket); });
		const auto position = (id - 1) / 256;
		return position < rows.size() && rows[position].authority == id &&
		       number(rows[position].key, 0, 2) == kind &&
		       number(rows[position].key, 2, 8) == context;
	}
	void run()
	{
		struct stat info = {};
		if (lstat(directory.c_str(), &info) != 0)
		{
			need(errno == ENOENT);
			return; // Preserve installations where accounting has never been initialized.
		}
		need(S_ISDIR(info.st_mode) && info.st_uid == geteuid() && !(info.st_mode & 0077));
		if (std::filesystem::is_empty(directory))
			return;
		std::set<std::string> metadata = { "authority.eal", "epochs.eae" };
		for (size_t bucket = 0; bucket < buckets; ++bucket)
		{
			metadata.insert(filename("mapping-", bucket, ".eam"));
			metadata.insert(filename("native-", bucket, ".ean"));
		}
		for (const auto &entry : std::filesystem::directory_iterator(directory))
		{
			audit_directory_entry();
			auto name = entry.path().filename().string();
			if (name.starts_with("mapping-") || name.starts_with("native-"))
				need(metadata.contains(name));
		}
		control();
		epochs();
		cache<native_entry> native_cache;
		for (size_t bucket = 0; bucket < buckets; ++bucket)
			for (const auto &row : mappings(bucket))
				mapping_link(row, native_cache);
		native_cache.entries.clear();
		cache<mapping> mapping_cache;
		for (size_t bucket = 0; bucket < buckets; ++bucket)
			for (const auto &entry : natives(bucket))
				native_link(entry, mapping_cache);
	}
};
} // namespace restore_economic_authority
#endif
