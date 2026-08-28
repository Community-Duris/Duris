#include "flatfile_association_repository.h"

#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'A', 'S', 'S', 'C', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t association_maximum = 65536;
constexpr size_t member_maximum = 1048576;
constexpr size_t association_name_maximum = 80;
constexpr size_t member_name_maximum = 64;
constexpr size_t rank_name_maximum = 80;
constexpr size_t top_fragger_maximum = 64;
constexpr const char *catalog_filename = "association_catalog";

struct association_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_association_record> associations;
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

	void text(const std::string &value, size_t maximum)
	{
		if (value.size() > maximum || value.size() > UINT32_MAX)
		{
			valid = false;
			return;
		}
		number<uint32_t>(value.size());
		raw(reinterpret_cast<const uint8_t *>(value.data()), value.size());
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

	bool text(std::string *value, size_t maximum)
	{
		uint32_t length = 0;
		if (!value || !number(&length) || length > maximum || offset > size ||
		    size - offset < length)
			return false;
		try
		{
			value->assign(reinterpret_cast<const char *>(data + offset), length);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		offset += length;
		return value->find('\0') == std::string::npos;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

std::string canonical_name(const std::string &name)
{
	std::string canonical = name;
	for (char &character : canonical)
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	return canonical;
}

bool valid_text(const std::string &value, size_t maximum, bool required, bool allow_space)
{
	if ((required && value.empty()) || value.size() > maximum)
		return false;
	for (unsigned char character : value)
		if (character < (allow_space ? 0x20 : 0x21) || character > 0x7e)
			return false;
	return true;
}

bool association_less(const flatfile_association_record &left,
		      const flatfile_association_record &right)
{
	return left.association_id < right.association_id;
}

bool member_less(const flatfile_association_member_record &left,
		 const flatfile_association_member_record &right)
{
	return left.pid < right.pid;
}

bool valid_catalog(const association_catalog &catalog)
{
	if (!catalog.revision || catalog.associations.size() > association_maximum ||
	    !std::is_sorted(catalog.associations.begin(), catalog.associations.end(),
			    association_less))
		return false;
	std::unordered_set<uint32_t> member_pids;
	size_t member_count = 0;
	try
	{
		for (size_t guild_index = 0; guild_index < catalog.associations.size();
		     ++guild_index)
		{
			const auto &guild = catalog.associations[guild_index];
			if (!guild.association_id || !guild.revision ||
			    !valid_text(guild.name, association_name_maximum, true, true) ||
			    !valid_text(guild.top_fragger, top_fragger_maximum, false, false) ||
			    guild.top_fragger != canonical_name(guild.top_fragger) ||
			    (guild.top_fragger.empty() != (guild.top_frags == 0)) ||
			    (guild_index && catalog.associations[guild_index - 1].association_id ==
						    guild.association_id) ||
			    !std::is_sorted(guild.members.begin(), guild.members.end(),
					    member_less))
				return false;
			for (const auto &rank : guild.ranks)
				if (!valid_text(rank, rank_name_maximum, false, true))
					return false;
			if (guild.members.size() > member_maximum - member_count)
				return false;
			member_count += guild.members.size();
			for (size_t member_index = 0; member_index < guild.members.size();
			     ++member_index)
			{
				const auto &member = guild.members[member_index];
				if (!member.pid || !member.revision || member.online_status > 2 ||
				    !valid_text(member.name, member_name_maximum, true, false) ||
				    member.name != canonical_name(member.name) ||
				    (member_index &&
				     guild.members[member_index - 1].pid == member.pid) ||
				    !member_pids.insert(member.pid).second)
					return false;
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_catalog(const association_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.associations.size());
	for (const auto &guild : catalog.associations)
	{
		payload.number(guild.association_id);
		payload.text(guild.name, association_name_maximum);
		payload.number(guild.racewar);
		payload.number(guild.bits);
		payload.number(guild.prestige);
		payload.number(guild.construction);
		payload.number(guild.platinum);
		payload.number(guild.gold);
		payload.number(guild.silver);
		payload.number(guild.copper);
		payload.number(guild.frags);
		payload.number(guild.top_frags);
		payload.text(guild.top_fragger, top_fragger_maximum);
		for (const auto &rank : guild.ranks)
			payload.text(rank, rank_name_maximum);
		payload.number(guild.revision);
		payload.number<uint32_t>(guild.members.size());
		for (const auto &member : guild.members)
		{
			payload.number(member.pid);
			payload.text(member.name, member_name_maximum);
			payload.number(member.bits);
			payload.number(member.debt);
			payload.number(member.online_status);
			payload.number(member.contributed_frags);
			payload.number(member.revision);
		}
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

bool decode_catalog(const std::vector<uint8_t> &bytes, association_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
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
	uint32_t association_count = 0;
	if (!payload.number(&association_count) || association_count > association_maximum)
		return false;
	association_catalog decoded;
	decoded.revision = revision;
	size_t member_count = 0;
	try
	{
		decoded.associations.resize(association_count);
		for (auto &guild : decoded.associations)
		{
			uint32_t count = 0;
			if (!payload.number(&guild.association_id) ||
			    !payload.text(&guild.name, association_name_maximum) ||
			    !payload.number(&guild.racewar) || !payload.number(&guild.bits) ||
			    !payload.number(&guild.prestige) ||
			    !payload.number(&guild.construction) ||
			    !payload.number(&guild.platinum) || !payload.number(&guild.gold) ||
			    !payload.number(&guild.silver) || !payload.number(&guild.copper) ||
			    !payload.number(&guild.frags) || !payload.number(&guild.top_frags) ||
			    !payload.text(&guild.top_fragger, top_fragger_maximum))
				return false;
			for (auto &rank : guild.ranks)
				if (!payload.text(&rank, rank_name_maximum))
					return false;
			if (!payload.number(&guild.revision) || !payload.number(&count) ||
			    count > member_maximum - member_count)
				return false;
			member_count += count;
			guild.members.resize(count);
			for (auto &member : guild.members)
				if (!payload.number(&member.pid) ||
				    !payload.text(&member.name, member_name_maximum) ||
				    !payload.number(&member.bits) ||
				    !payload.number(&member.debt) ||
				    !payload.number(&member.online_status) ||
				    !payload.number(&member.contributed_frags) ||
				    !payload.number(&member.revision))
					return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_catalog(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_association_result recover(const std::string &root, const flatfile_authority_lock &lock,
				    std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_association_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_association_result::io_error :
		       flatfile_association_result::invalid;
}

flatfile_association_result load_catalog(const std::string &root, association_catalog *catalog,
					 std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_association_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_association_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "association catalog is corrupt";
		return flatfile_association_result::invalid;
	}
	return flatfile_association_result::ok;
}

bool catalog_equal(association_catalog left, association_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}
} // namespace

flatfile_association_result
flatfile_association_establish(const std::string &root,
			       const std::vector<flatfile_association_record> &associations,
			       std::string *error)
{
	if (root.empty())
		return flatfile_association_result::invalid;
	association_catalog candidate;
	try
	{
		candidate.associations = associations;
		for (auto &guild : candidate.associations)
		{
			guild.top_fragger = canonical_name(guild.top_fragger);
			for (auto &member : guild.members)
				member.name = canonical_name(member.name);
			std::sort(guild.members.begin(), guild.members.end(), member_less);
		}
		std::sort(candidate.associations.begin(), candidate.associations.end(),
			  association_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	association_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_association_result::ok)
		return catalog_equal(existing, candidate) ?
			       flatfile_association_result::already_exists :
			       flatfile_association_result::invalid;
	if (loaded != flatfile_association_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_association_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_association_result::io_error;
	return flatfile_association_result::ok;
}

flatfile_association_result
flatfile_association_list(const std::string &root,
			  std::vector<flatfile_association_record> *associations,
			  std::string *error)
{
	if (root.empty() || !associations)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	association_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	*associations = std::move(catalog.associations);
	return flatfile_association_result::ok;
}

flatfile_association_result flatfile_association_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::string &expected_name, flatfile_authority_operation *operation,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !pid || !operation ||
	    !valid_text(expected_name, member_name_maximum, true, false))
		return flatfile_association_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	association_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	const std::string canonical_expected = canonical_name(expected_name);
	for (auto &guild : catalog.associations)
	{
		auto member = std::lower_bound(guild.members.begin(), guild.members.end(), pid,
					       [](const auto &entry, uint32_t candidate)
					       { return entry.pid < candidate; });
		if (member == guild.members.end() || member->pid != pid)
			continue;
		if (member->name != canonical_expected ||
		    guild.revision == std::numeric_limits<uint64_t>::max() ||
		    catalog.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_association_result::conflict;
		if ((member->contributed_frags > 0 &&
		     guild.frags <
			     std::numeric_limits<int64_t>::min() + member->contributed_frags) ||
		    (member->contributed_frags < 0 &&
		     guild.frags > std::numeric_limits<int64_t>::max() + member->contributed_frags))
			return flatfile_association_result::conflict;
		guild.frags -= member->contributed_frags;
		if (guild.top_fragger == canonical_expected)
		{
			guild.top_fragger.clear();
			guild.top_frags = 0;
		}
		guild.members.erase(member);
		++guild.revision;
		++catalog.revision;
		std::vector<uint8_t> encoded;
		if (!encode_catalog(catalog, &encoded))
			return flatfile_association_result::invalid;
		operation->store = flatfile_authority_store::domains;
		operation->kind = flatfile_authority_operation_kind::write;
		operation->filename = catalog_filename;
		operation->bytes = std::move(encoded);
		return flatfile_association_result::ok;
	}
	return flatfile_association_result::unchanged;
}
