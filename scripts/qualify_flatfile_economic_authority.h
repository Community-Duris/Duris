// Independent, read-only retained authority validation. Do not call storage
// readers here: those readers recover journals and can change the candidate.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_AUTHORITY_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_AUTHORITY_H

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fcntl.h>
#include <openssl/sha.h>
#include <set>
#include <span>
#include <stdexcept>
#include <sys/stat.h>
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
struct reader
{
	std::span<const uint8_t> value;
	size_t offset = 0;
	std::span<const uint8_t> take(size_t count)
	{
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
	digest epochs_digest = {};
	std::array<digest, buckets> native_digests = {}, mapping_digests = {};

	void absent(const std::string &name) const
	{
		struct stat info = {};
		need(lstat((directory / name).c_str(), &info) != 0 && errno == ENOENT);
	}
	bytes frame(const std::string &name, const char *magic, const digest &expected = {}) const
	{
		const int fd = open((directory / name).c_str(),
				    O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
		need(fd >= 0);
		struct closer
		{
			int fd;
			~closer() { close(fd); }
		} close_file{ fd };
		struct stat info = {};
		need(fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_uid == geteuid() &&
		     info.st_nlink == 1 && !(info.st_mode & 0077) && info.st_size >= 48 &&
		     uint64_t(info.st_size) <= maximum_bytes);
		bytes encoded(static_cast<size_t>(info.st_size));
		size_t offset = 0;
		while (offset < encoded.size())
		{
			auto count = read(fd, encoded.data() + offset, encoded.size() - offset);
			if (count < 0 && errno == EINTR)
				continue;
			need(count > 0);
			offset += static_cast<size_t>(count);
		}
		uint8_t extra;
		need(read(fd, &extra, 1) == 0);
		need(!nonzero(expected) || hash(encoded) == expected);
		reader in{ encoded };
		auto prefix = in.take(8);
		need(memcmp(prefix.data(), magic, 8) == 0 && in.number(4) == 1 &&
		     in.number(4) == encoded.size() - 48);
		auto body_digest = in.fixed<32>();
		auto body = in.take(encoded.size() - 48);
		need(body_digest == hash(body));
		return { body.begin(), body.end() };
	}
	size_t mapping_count(size_t bucket) const
	{
		const uint64_t first = bucket ? bucket : 256;
		return next_mapping <= first ? 0 : 1 + (next_mapping - 1 - first) / 256;
	}
	void control()
	{
		auto body = frame("authority.eal", "DURECA1");
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
		auto body = frame("epochs.eae", "DURECE1", epochs_digest);
		reader in{ body };
		need(in.fixed<16>() == lineage && in.number(4) == epoch_count && in.number(4) == 0);
		identity previous = {};
		std::set<identity> seen;
		for (size_t i = 0; i < epoch_count; ++i)
		{
			auto id = in.fixed<16>();
			need(nonzero(id) && seen.insert(id).second && in.number(8) == i + 1 &&
			     in.fixed<16>() == previous && in.number(2) != 0 && in.number(6) == 0 &&
			     nonzero(in.take(32)) && nonzero(in.take(16)));
			previous = id;
		}
		in.done();
		need(previous == last_epoch);
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

    public:
	explicit checker(const std::filesystem::path &root)
		: directory(root / "economic-evidence")
	{
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
			auto name = entry.path().filename().string();
			if (name.starts_with("mapping-") || name.starts_with("native-"))
				need(metadata.contains(name));
		}
		control();
		epochs();
		cache<native_entry> native_cache;
		for (size_t bucket = 0; bucket < buckets; ++bucket)
			for (const auto &row : mappings(bucket))
			{
				if (row.retired)
					continue;
				const auto &index = native_cache.get(hash(row.key)[0], [&](auto b)
								     { return natives(b); });
				auto found = std::lower_bound(index.begin(), index.end(), row.key,
							      [](const auto &entry, const auto &key)
							      { return entry.key < key; });
				need(found != index.end() && found->key == row.key &&
				     found->active == row.authority);
			}
		native_cache.entries.clear();
		cache<mapping> mapping_cache;
		for (size_t bucket = 0; bucket < buckets; ++bucket)
			for (const auto &entry : natives(bucket))
			{
				const auto &values = mapping_cache.get(entry.last % 256, [&](auto b)
								       { return mappings(b); });
				const auto position = (entry.last - 1) / 256;
				need(position < values.size());
				const auto &row = values[position];
				if (entry.active)
					need(!row.retired && row.key == entry.key);
				else
				{
					need(row.key.size() >= 12 && entry.key.size() >= 12 &&
					     std::equal(row.key.begin(), row.key.begin() + 12,
							entry.key.begin()));
					const bool bank = row.key[0] == 2 && row.key[1] == 0;
					need(bank || row.key == entry.key);
					need(row.retired || (bank && row.key != entry.key));
				}
			}
	}
};
} // namespace restore_economic_authority
#endif
