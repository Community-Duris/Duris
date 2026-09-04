#include "flatfile/flatfile_association_repository.h"

#include "flatfile/flatfile_store.h"

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
constexpr std::array<uint8_t, 8> ledger_magic = { 'D', 'U', 'R', 'G', 'L', 'D', 'G', 0 };
constexpr std::array<uint8_t, 8> alliance_magic = { 'D', 'U', 'R', 'A', 'L', 'L', 'Y', 0 };
constexpr std::array<uint8_t, 8> guildhall_magic = { 'D', 'U', 'R', 'G', 'H', 'A', 'L', 'L' };
constexpr std::array<uint8_t, 8> outpost_magic = { 'D', 'U', 'R', 'O', 'U', 'T', 'P', 0 };
constexpr uint32_t catalog_version = 1;
constexpr uint32_t ledger_version = 1;
constexpr uint32_t alliance_version = 1;
constexpr uint32_t guildhall_version = 1;
constexpr uint32_t outpost_version = 1;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t ledger_maximum_bytes = 128 * 1024;
constexpr size_t alliance_maximum_bytes = 1024 * 1024;
constexpr size_t guildhall_maximum_bytes = 4 * 1024 * 1024;
constexpr size_t outpost_maximum_bytes = 4096;
constexpr size_t association_maximum = 65536;
constexpr size_t member_maximum = 1048576;
constexpr size_t association_name_maximum = 80;
constexpr size_t member_name_maximum = 64;
constexpr size_t rank_name_maximum = 80;
constexpr size_t top_fragger_maximum = 64;
constexpr size_t ledger_message_maximum = 255;
constexpr size_t ledger_kind_maximum = 100;
constexpr size_t guildhall_maximum = 4096;
constexpr size_t guildhall_room_maximum = 50;
constexpr size_t guildhall_room_name_maximum = 255;
constexpr const char *catalog_filename = "association_catalog";
constexpr const char *alliance_filename = "association_alliances";
constexpr const char *guildhall_filename = "association_guildhalls";
constexpr const char *outpost_filename = "association_outposts";

struct association_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_association_record> associations;
};

struct ledger_entry
{
	bool system = false;
	std::string message;
};

struct association_ledger
{
	uint64_t revision = 1;
	std::vector<ledger_entry> entries;
};

struct alliance_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_alliance_record> alliances;
};

struct guildhall_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_guildhall_record> guildhalls;
};

struct outpost_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_outpost_record> outposts;
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

bool member_content_equal(const flatfile_association_member_record &left,
			  const flatfile_association_member_record &right)
{
	return left.pid == right.pid && left.name == right.name && left.bits == right.bits &&
	       left.debt == right.debt && left.online_status == right.online_status &&
	       left.contributed_frags == right.contributed_frags;
}

bool association_content_equal(const flatfile_association_record &left,
			       const flatfile_association_record &right)
{
	if (left.association_id != right.association_id || left.name != right.name ||
	    left.racewar != right.racewar || left.bits != right.bits ||
	    left.prestige != right.prestige || left.construction != right.construction ||
	    left.platinum != right.platinum || left.gold != right.gold ||
	    left.silver != right.silver || left.copper != right.copper ||
	    left.frags != right.frags || left.top_frags != right.top_frags ||
	    left.top_fragger != right.top_fragger || left.ranks != right.ranks ||
	    left.members.size() != right.members.size())
		return false;
	for (size_t index = 0; index < left.members.size(); ++index)
		if (!member_content_equal(left.members[index], right.members[index]))
			return false;
	return true;
}

bool normalize_record(flatfile_association_record *record)
{
	if (!record)
		return false;
	try
	{
		record->top_fragger = canonical_name(record->top_fragger);
		for (auto &member : record->members)
			member.name = canonical_name(member.name);
		std::sort(record->members.begin(), record->members.end(), member_less);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

std::string ledger_filename(uint32_t association_id)
{
	return "association_ledger_" + std::to_string(association_id);
}

bool valid_ledger(const association_ledger &ledger)
{
	if (!ledger.revision || ledger.entries.size() > ledger_kind_maximum * 2)
		return false;
	size_t player_count = 0, system_count = 0;
	for (const auto &entry : ledger.entries)
	{
		if (!valid_text(entry.message, ledger_message_maximum, true, true))
			return false;
		size_t &count = entry.system ? system_count : player_count;
		if (++count > ledger_kind_maximum)
			return false;
	}
	return true;
}

bool encode_ledger(const association_ledger &ledger, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_ledger(ledger))
		return false;
	encoder payload;
	payload.number<uint32_t>(ledger.entries.size());
	for (const auto &entry : ledger.entries)
	{
		payload.number<uint8_t>(entry.system ? 1 : 0);
		payload.text(entry.message, ledger_message_maximum);
	}
	if (!payload.valid || payload.bytes.size() > ledger_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(ledger_magic.data(), ledger_magic.size());
	file.number(ledger_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(ledger.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > ledger_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_ledger(const std::vector<uint8_t> &bytes, association_ledger *ledger)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!ledger || bytes.size() < header_size ||
	    memcmp(bytes.data(), ledger_magic.data(), ledger_magic.size()))
		return false;
	decoder header{ bytes.data() + ledger_magic.size(), bytes.size() - ledger_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != ledger_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > ledger_kind_maximum * 2)
		return false;
	association_ledger decoded;
	decoded.revision = revision;
	try
	{
		decoded.entries.resize(count);
		for (auto &entry : decoded.entries)
		{
			uint8_t system = 0;
			if (!payload.number(&system) || system > 1 ||
			    !payload.text(&entry.message, ledger_message_maximum))
				return false;
			entry.system = system != 0;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_ledger(decoded))
		return false;
	*ledger = std::move(decoded);
	return true;
}

flatfile_association_result load_ledger(const std::string &root, uint32_t association_id,
					association_ledger *ledger, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), ledger_filename(association_id),
					  ledger_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_association_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_association_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_ledger(bytes, ledger))
	{
		if (error && error->empty())
			*error = "association ledger is corrupt";
		return flatfile_association_result::invalid;
	}
	return flatfile_association_result::ok;
}

bool alliance_less(const flatfile_alliance_record &left, const flatfile_alliance_record &right)
{
	if (left.forging_association_id != right.forging_association_id)
		return left.forging_association_id < right.forging_association_id;
	return left.joining_association_id < right.joining_association_id;
}

bool alliance_equal(const flatfile_alliance_record &left, const flatfile_alliance_record &right)
{
	return left.forging_association_id == right.forging_association_id &&
	       left.joining_association_id == right.joining_association_id &&
	       left.tribute_owed == right.tribute_owed;
}

bool valid_alliances(const alliance_catalog &catalog)
{
	if (!catalog.revision || catalog.alliances.size() > association_maximum / 2 ||
	    !std::is_sorted(catalog.alliances.begin(), catalog.alliances.end(), alliance_less))
		return false;
	std::unordered_set<uint32_t> guild_ids;
	try
	{
		for (const auto &alliance : catalog.alliances)
			if (!alliance.forging_association_id || !alliance.joining_association_id ||
			    alliance.forging_association_id == alliance.joining_association_id ||
			    !guild_ids.insert(alliance.forging_association_id).second ||
			    !guild_ids.insert(alliance.joining_association_id).second)
				return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_alliances(const alliance_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_alliances(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.alliances.size());
	for (const auto &alliance : catalog.alliances)
	{
		payload.number(alliance.forging_association_id);
		payload.number(alliance.joining_association_id);
		payload.number(alliance.tribute_owed);
	}
	if (!payload.valid || payload.bytes.size() > alliance_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(alliance_magic.data(), alliance_magic.size());
	file.number(alliance_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > alliance_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_alliances(const std::vector<uint8_t> &bytes, alliance_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), alliance_magic.data(), alliance_magic.size()))
		return false;
	decoder header{ bytes.data() + alliance_magic.size(),
			bytes.size() - alliance_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != alliance_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > association_maximum / 2)
		return false;
	alliance_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.alliances.resize(count);
		for (auto &alliance : decoded.alliances)
			if (!payload.number(&alliance.forging_association_id) ||
			    !payload.number(&alliance.joining_association_id) ||
			    !payload.number(&alliance.tribute_owed))
				return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_alliances(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_association_result load_alliance_catalog(const std::string &root,
						  alliance_catalog *catalog, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), alliance_filename,
					  alliance_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_association_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_association_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_alliances(bytes, catalog))
	{
		if (error && error->empty())
			*error = "alliance catalog is corrupt";
		return flatfile_association_result::invalid;
	}
	return flatfile_association_result::ok;
}

bool guildhall_less(const flatfile_guildhall_record &left, const flatfile_guildhall_record &right)
{
	return left.guildhall_id < right.guildhall_id;
}

bool guildhall_room_less(const flatfile_guildhall_room_record &left,
			 const flatfile_guildhall_room_record &right)
{
	return left.room_id < right.room_id;
}

bool valid_guildhall_room_name(const std::string &name)
{
	if (name.size() > guildhall_room_name_maximum)
		return false;
	for (const unsigned char character : name)
		if (character < 0x20 || character == 0x7f)
			return false;
	return true;
}

bool guildhall_room_equal(const flatfile_guildhall_room_record &left,
			  const flatfile_guildhall_room_record &right)
{
	return left.room_id == right.room_id && left.vnum == right.vnum &&
	       left.name == right.name && left.type == right.type && left.values == right.values &&
	       left.exits == right.exits;
}

bool guildhall_equal(const flatfile_guildhall_record &left, const flatfile_guildhall_record &right)
{
	if (left.guildhall_id != right.guildhall_id ||
	    left.association_id != right.association_id || left.type != right.type ||
	    left.outside_vnum != right.outside_vnum || left.racewar != right.racewar ||
	    left.rooms.size() != right.rooms.size())
		return false;
	for (size_t index = 0; index < left.rooms.size(); ++index)
		if (!guildhall_room_equal(left.rooms[index], right.rooms[index]))
			return false;
	return true;
}

bool normalize_guildhall(flatfile_guildhall_record *guildhall)
{
	if (!guildhall)
		return false;
	try
	{
		std::sort(guildhall->rooms.begin(), guildhall->rooms.end(), guildhall_room_less);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool valid_guildhalls(const guildhall_catalog &catalog)
{
	if (!catalog.revision || catalog.guildhalls.size() > guildhall_maximum ||
	    !std::is_sorted(catalog.guildhalls.begin(), catalog.guildhalls.end(), guildhall_less))
		return false;
	std::unordered_set<int32_t> hall_ids, room_ids, room_vnums;
	try
	{
		for (const auto &guildhall : catalog.guildhalls)
		{
			if (guildhall.guildhall_id <= 0 || !guildhall.association_id ||
			    guildhall.association_id >
				    static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
			    guildhall.outside_vnum < 0 ||
			    guildhall.rooms.size() > guildhall_room_maximum ||
			    !hall_ids.insert(guildhall.guildhall_id).second ||
			    !std::is_sorted(guildhall.rooms.begin(), guildhall.rooms.end(),
					    guildhall_room_less))
				return false;
			for (const auto &room : guildhall.rooms)
				if (room.room_id <= 0 || room.vnum <= 0 || room.type < 0 ||
				    room.type > 10 || !valid_guildhall_room_name(room.name) ||
				    !room_ids.insert(room.room_id).second ||
				    !room_vnums.insert(room.vnum).second)
					return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_guildhalls(const guildhall_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_guildhalls(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.guildhalls.size());
	for (const auto &guildhall : catalog.guildhalls)
	{
		payload.number(guildhall.guildhall_id);
		payload.number(guildhall.association_id);
		payload.number(guildhall.type);
		payload.number(guildhall.outside_vnum);
		payload.number(guildhall.racewar);
		payload.number<uint32_t>(guildhall.rooms.size());
		for (const auto &room : guildhall.rooms)
		{
			payload.number(room.room_id);
			payload.number(room.vnum);
			payload.text(room.name, guildhall_room_name_maximum);
			payload.number(room.type);
			for (const auto value : room.values)
				payload.number(value);
			for (const auto exit : room.exits)
				payload.number(exit);
		}
	}
	if (!payload.valid || payload.bytes.size() > guildhall_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(guildhall_magic.data(), guildhall_magic.size());
	file.number(guildhall_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > guildhall_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_guildhalls(const std::vector<uint8_t> &bytes, guildhall_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), guildhall_magic.data(), guildhall_magic.size()))
		return false;
	decoder header{ bytes.data() + guildhall_magic.size(),
			bytes.size() - guildhall_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != guildhall_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > guildhall_maximum)
		return false;
	guildhall_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.guildhalls.resize(count);
		for (auto &guildhall : decoded.guildhalls)
		{
			uint32_t room_count = 0;
			if (!payload.number(&guildhall.guildhall_id) ||
			    !payload.number(&guildhall.association_id) ||
			    !payload.number(&guildhall.type) ||
			    !payload.number(&guildhall.outside_vnum) ||
			    !payload.number(&guildhall.racewar) || !payload.number(&room_count) ||
			    room_count > guildhall_room_maximum)
				return false;
			guildhall.rooms.resize(room_count);
			for (auto &room : guildhall.rooms)
			{
				if (!payload.number(&room.room_id) || !payload.number(&room.vnum) ||
				    !payload.text(&room.name, guildhall_room_name_maximum) ||
				    !payload.number(&room.type))
					return false;
				for (auto &value : room.values)
					if (!payload.number(&value))
						return false;
				for (auto &exit : room.exits)
					if (!payload.number(&exit))
						return false;
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_guildhalls(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_association_result load_guildhall_catalog(const std::string &root,
						   guildhall_catalog *catalog, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), guildhall_filename,
					  guildhall_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_association_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_association_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_guildhalls(bytes, catalog))
	{
		if (error && error->empty())
			*error = "guildhall catalog is corrupt";
		return flatfile_association_result::invalid;
	}
	return flatfile_association_result::ok;
}

bool outpost_less(const flatfile_outpost_record &left, const flatfile_outpost_record &right)
{
	return left.outpost_id < right.outpost_id;
}

bool outpost_equal(const flatfile_outpost_record &left, const flatfile_outpost_record &right)
{
	return left.outpost_id == right.outpost_id &&
	       left.owner_association_id == right.owner_association_id &&
	       left.level == right.level && left.walls == right.walls &&
	       left.archers == right.archers && left.resources == right.resources &&
	       left.applied_resources == right.applied_resources &&
	       left.hitpoints == right.hitpoints && left.territory == right.territory &&
	       left.portal_room == right.portal_room && left.golems == right.golems &&
	       left.meurtriere == right.meurtriere && left.scouts == right.scouts;
}

bool valid_outposts(const outpost_catalog &catalog)
{
	if (!catalog.revision || catalog.outposts.size() != 3 ||
	    !std::is_sorted(catalog.outposts.begin(), catalog.outposts.end(), outpost_less))
		return false;
	for (size_t index = 0; index < catalog.outposts.size(); ++index)
	{
		const auto &outpost = catalog.outposts[index];
		if (outpost.outpost_id != static_cast<int32_t>(index) ||
		    outpost.owner_association_id >
			    static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
		    outpost.level <= 0 || outpost.walls < 0 || outpost.archers < 0 ||
		    outpost.archers > 1 || outpost.resources < 0 || outpost.applied_resources < 0 ||
		    outpost.hitpoints < 0 || outpost.territory < 0 || outpost.portal_room < 0 ||
		    outpost.portal_room > 1 || outpost.golems < 0 || outpost.meurtriere < 0 ||
		    outpost.meurtriere > 1 || outpost.scouts < 0)
			return false;
	}
	return true;
}

bool encode_outposts(const outpost_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_outposts(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.outposts.size());
	for (const auto &outpost : catalog.outposts)
	{
		payload.number(outpost.outpost_id);
		payload.number(outpost.owner_association_id);
		payload.number(outpost.level);
		payload.number(outpost.walls);
		payload.number(outpost.archers);
		payload.number(outpost.resources);
		payload.number(outpost.applied_resources);
		payload.number(outpost.hitpoints);
		payload.number(outpost.territory);
		payload.number(outpost.portal_room);
		payload.number(outpost.golems);
		payload.number(outpost.meurtriere);
		payload.number(outpost.scouts);
	}
	if (!payload.valid || payload.bytes.size() > outpost_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(outpost_magic.data(), outpost_magic.size());
	file.number(outpost_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > outpost_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_outposts(const std::vector<uint8_t> &bytes, outpost_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), outpost_magic.data(), outpost_magic.size()))
		return false;
	decoder header{ bytes.data() + outpost_magic.size(), bytes.size() - outpost_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != outpost_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count != 3)
		return false;
	outpost_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.outposts.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &outpost : decoded.outposts)
		if (!payload.number(&outpost.outpost_id) ||
		    !payload.number(&outpost.owner_association_id) ||
		    !payload.number(&outpost.level) || !payload.number(&outpost.walls) ||
		    !payload.number(&outpost.archers) || !payload.number(&outpost.resources) ||
		    !payload.number(&outpost.applied_resources) ||
		    !payload.number(&outpost.hitpoints) || !payload.number(&outpost.territory) ||
		    !payload.number(&outpost.portal_room) || !payload.number(&outpost.golems) ||
		    !payload.number(&outpost.meurtriere) || !payload.number(&outpost.scouts))
			return false;
	if (payload.offset != payload.size || !valid_outposts(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_association_result load_outpost_catalog(const std::string &root, outpost_catalog *catalog,
						 std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), outpost_filename,
					  outpost_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_association_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_association_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_outposts(bytes, catalog))
	{
		if (error && error->empty())
			*error = "outpost catalog is corrupt";
		return flatfile_association_result::invalid;
	}
	return flatfile_association_result::ok;
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

/* Build the association catalogue after-image while the shared authority lock
 * remains with the caller. This is the seam used when a guild change and a
 * second authority (such as a paid kingdom roster) must share one recoverable
 * commit. Nothing is published here. */
flatfile_association_result
flatfile_association_prepare_save(const std::string &root, const flatfile_authority_lock &lock,
				  const flatfile_association_record &association,
				  flatfile_authority_operation *operation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !operation)
		return flatfile_association_result::invalid;
	*operation = {};
	flatfile_association_record desired;
	try
	{
		desired = association;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!normalize_record(&desired))
		return flatfile_association_result::io_error;
	desired.revision = 1;
	for (auto &member : desired.members)
		member.revision = 1;
	association_catalog validation;
	try
	{
		validation.associations = { desired };
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!valid_catalog(validation))
		return flatfile_association_result::invalid;

	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;

	association_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	const bool new_catalog = loaded == flatfile_association_result::not_found;
	if (!new_catalog && loaded != flatfile_association_result::ok)
		return loaded;

	auto existing = std::lower_bound(catalog.associations.begin(), catalog.associations.end(),
					 desired, association_less);
	if (existing != catalog.associations.end() &&
	    existing->association_id == desired.association_id)
	{
		for (auto &member : desired.members)
		{
			auto previous = std::lower_bound(existing->members.begin(),
							 existing->members.end(), member.pid,
							 [](const auto &entry, uint32_t pid)
							 { return entry.pid < pid; });
			if (previous == existing->members.end() || previous->pid != member.pid)
				continue;
			member.revision = previous->revision;
			if (!member_content_equal(member, *previous))
			{
				if (member.revision == std::numeric_limits<uint64_t>::max())
					return flatfile_association_result::conflict;
				++member.revision;
			}
		}
		if (association_content_equal(desired, *existing))
			return flatfile_association_result::unchanged;
		if (existing->revision == std::numeric_limits<uint64_t>::max() ||
		    catalog.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_association_result::conflict;
		desired.revision = existing->revision + 1;
		*existing = std::move(desired);
		++catalog.revision;
	}
	else
	{
		try
		{
			catalog.associations.insert(existing, std::move(desired));
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_association_result::io_error;
		}
		if (!new_catalog)
		{
			if (catalog.revision == std::numeric_limits<uint64_t>::max())
				return flatfile_association_result::conflict;
			++catalog.revision;
		}
	}

	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_association_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(encoded);
	return flatfile_association_result::ok;
}

flatfile_association_result
flatfile_association_save(const std::string &root, const flatfile_association_record &association,
			  std::string *error)
{
	if (root.empty())
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	flatfile_authority_operation operation;
	const auto prepared =
		flatfile_association_prepare_save(root, lock, association, &operation, error);
	if (prepared != flatfile_association_result::ok)
		return prepared;
	const auto committed =
		flatfile_authority_transaction_commit_operations(root, lock, { operation }, error);
	if (committed == flatfile_authority_transaction_result::ok)
		return flatfile_association_result::ok;
	return committed == flatfile_authority_transaction_result::io_error ?
		       flatfile_association_result::io_error :
		       flatfile_association_result::invalid;
}

flatfile_association_result flatfile_association_erase(const std::string &root,
						       uint32_t association_id, std::string *error)
{
	if (root.empty() || !association_id)
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
	auto found = std::lower_bound(catalog.associations.begin(), catalog.associations.end(),
				      association_id, [](const auto &entry, uint32_t candidate)
				      { return entry.association_id < candidate; });
	if (found == catalog.associations.end() || found->association_id != association_id)
		return flatfile_association_result::unchanged;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_association_result::conflict;
	catalog.associations.erase(found);
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result flatfile_association_ledger_append(const std::string &root,
							       uint32_t association_id,
							       bool system_entry,
							       const std::string &message,
							       std::string *error)
{
	if (root.empty() || !association_id ||
	    !valid_text(message, ledger_message_maximum, true, true))
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	association_ledger ledger;
	const auto loaded = load_ledger(root, association_id, &ledger, error);
	if (loaded != flatfile_association_result::ok &&
	    loaded != flatfile_association_result::not_found)
		return loaded;
	if (loaded == flatfile_association_result::ok &&
	    ledger.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_association_result::conflict;
	try
	{
		ledger.entries.push_back({ system_entry, message });
		std::vector<ledger_entry> retained;
		retained.reserve(ledger_kind_maximum * 2);
		size_t player_count = 0, system_count = 0;
		for (auto entry = ledger.entries.rbegin(); entry != ledger.entries.rend(); ++entry)
		{
			size_t &count = entry->system ? system_count : player_count;
			if (count++ < ledger_kind_maximum)
				retained.push_back(*entry);
		}
		std::reverse(retained.begin(), retained.end());
		ledger.entries = std::move(retained);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (loaded == flatfile_association_result::ok)
		++ledger.revision;
	std::vector<uint8_t> encoded;
	if (!encode_ledger(ledger, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), ledger_filename(association_id),
				     encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result flatfile_association_ledger_list(const std::string &root,
							     uint32_t association_id,
							     bool system_entries,
							     std::vector<std::string> *messages,
							     std::string *error)
{
	if (root.empty() || !association_id || !messages)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	association_ledger ledger;
	const auto loaded = load_ledger(root, association_id, &ledger, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	std::vector<std::string> selected;
	try
	{
		selected.reserve(ledger_kind_maximum);
		for (auto entry = ledger.entries.rbegin(); entry != ledger.entries.rend(); ++entry)
			if (entry->system == system_entries)
				selected.push_back(entry->message);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	*messages = std::move(selected);
	return flatfile_association_result::ok;
}

flatfile_association_result flatfile_alliance_list(const std::string &root,
						   std::vector<flatfile_alliance_record> *alliances,
						   std::string *error)
{
	if (root.empty() || !alliances)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	alliance_catalog catalog;
	const auto loaded = load_alliance_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	*alliances = std::move(catalog.alliances);
	return flatfile_association_result::ok;
}

flatfile_association_result
flatfile_alliance_replace(const std::string &root,
			  const std::vector<flatfile_alliance_record> &alliances,
			  std::string *error)
{
	if (root.empty())
		return flatfile_association_result::invalid;
	alliance_catalog desired;
	try
	{
		desired.alliances = alliances;
		std::sort(desired.alliances.begin(), desired.alliances.end(), alliance_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!valid_alliances(desired))
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	alliance_catalog existing;
	const auto loaded = load_alliance_catalog(root, &existing, error);
	if (loaded == flatfile_association_result::ok)
	{
		bool equal = existing.alliances.size() == desired.alliances.size();
		for (size_t index = 0; equal && index < existing.alliances.size(); ++index)
			equal = alliance_equal(existing.alliances[index], desired.alliances[index]);
		if (equal)
			return flatfile_association_result::unchanged;
		if (existing.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_association_result::conflict;
		desired.revision = existing.revision + 1;
	}
	else if (loaded != flatfile_association_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_alliances(desired, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), alliance_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result
flatfile_guildhall_list(const std::string &root, std::vector<flatfile_guildhall_record> *guildhalls,
			std::string *error)
{
	if (root.empty() || !guildhalls)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	guildhall_catalog catalog;
	const auto loaded = load_guildhall_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	*guildhalls = std::move(catalog.guildhalls);
	return flatfile_association_result::ok;
}

flatfile_association_result flatfile_guildhall_save(const std::string &root,
						    const flatfile_guildhall_record &guildhall,
						    std::string *error)
{
	if (root.empty())
		return flatfile_association_result::invalid;
	flatfile_guildhall_record desired;
	try
	{
		desired = guildhall;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!normalize_guildhall(&desired))
		return flatfile_association_result::io_error;
	guildhall_catalog probe;
	try
	{
		probe.guildhalls.push_back(desired);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!valid_guildhalls(probe))
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	guildhall_catalog catalog;
	const auto loaded = load_guildhall_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok &&
	    loaded != flatfile_association_result::not_found)
		return loaded;
	auto existing = std::lower_bound(catalog.guildhalls.begin(), catalog.guildhalls.end(),
					 desired.guildhall_id, [](const auto &entry, int32_t id)
					 { return entry.guildhall_id < id; });
	if (existing != catalog.guildhalls.end() && existing->guildhall_id == desired.guildhall_id)
	{
		if (guildhall_equal(*existing, desired))
			return flatfile_association_result::unchanged;
		*existing = std::move(desired);
	}
	else
	{
		try
		{
			catalog.guildhalls.insert(existing, std::move(desired));
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_association_result::io_error;
		}
	}
	if (loaded == flatfile_association_result::ok)
	{
		if (catalog.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_association_result::conflict;
		++catalog.revision;
	}
	if (!valid_guildhalls(catalog))
		return flatfile_association_result::invalid;
	std::vector<uint8_t> encoded;
	if (!encode_guildhalls(catalog, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), guildhall_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result flatfile_guildhall_erase(const std::string &root, int32_t guildhall_id,
						     std::string *error)
{
	if (root.empty() || guildhall_id <= 0)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	guildhall_catalog catalog;
	const auto loaded = load_guildhall_catalog(root, &catalog, error);
	if (loaded == flatfile_association_result::not_found)
		return flatfile_association_result::unchanged;
	if (loaded != flatfile_association_result::ok)
		return loaded;
	auto existing = std::lower_bound(catalog.guildhalls.begin(), catalog.guildhalls.end(),
					 guildhall_id, [](const auto &entry, int32_t id)
					 { return entry.guildhall_id < id; });
	if (existing == catalog.guildhalls.end() || existing->guildhall_id != guildhall_id)
		return flatfile_association_result::unchanged;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_association_result::conflict;
	catalog.guildhalls.erase(existing);
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_guildhalls(catalog, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), guildhall_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result flatfile_guildhall_room_erase(const std::string &root,
							  int32_t guildhall_id, int32_t room_id,
							  std::string *error)
{
	if (root.empty() || guildhall_id <= 0 || room_id <= 0)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	guildhall_catalog catalog;
	const auto loaded = load_guildhall_catalog(root, &catalog, error);
	if (loaded == flatfile_association_result::not_found)
		return flatfile_association_result::unchanged;
	if (loaded != flatfile_association_result::ok)
		return loaded;
	auto guildhall = std::lower_bound(catalog.guildhalls.begin(), catalog.guildhalls.end(),
					  guildhall_id, [](const auto &entry, int32_t id)
					  { return entry.guildhall_id < id; });
	if (guildhall == catalog.guildhalls.end() || guildhall->guildhall_id != guildhall_id)
		return flatfile_association_result::unchanged;
	auto room = std::lower_bound(guildhall->rooms.begin(), guildhall->rooms.end(), room_id,
				     [](const auto &entry, int32_t id)
				     { return entry.room_id < id; });
	if (room == guildhall->rooms.end() || room->room_id != room_id)
		return flatfile_association_result::unchanged;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_association_result::conflict;
	guildhall->rooms.erase(room);
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_guildhalls(catalog, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), guildhall_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result
flatfile_outpost_establish(const std::string &root,
			   const std::vector<flatfile_outpost_record> &outposts, std::string *error)
{
	if (root.empty())
		return flatfile_association_result::invalid;
	outpost_catalog candidate;
	try
	{
		candidate.outposts = outposts;
		std::sort(candidate.outposts.begin(), candidate.outposts.end(), outpost_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_association_result::io_error;
	}
	if (!valid_outposts(candidate))
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	outpost_catalog existing;
	const auto loaded = load_outpost_catalog(root, &existing, error);
	if (loaded == flatfile_association_result::ok)
	{
		bool equal = existing.outposts.size() == candidate.outposts.size();
		for (size_t index = 0; equal && index < existing.outposts.size(); ++index)
			equal = outpost_equal(existing.outposts[index], candidate.outposts[index]);
		return equal ? flatfile_association_result::already_exists :
			       flatfile_association_result::invalid;
	}
	if (loaded != flatfile_association_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_outposts(candidate, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), outpost_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
}

flatfile_association_result flatfile_outpost_list(const std::string &root,
						  std::vector<flatfile_outpost_record> *outposts,
						  std::string *error)
{
	if (root.empty() || !outposts)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	outpost_catalog catalog;
	const auto loaded = load_outpost_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	*outposts = std::move(catalog.outposts);
	return flatfile_association_result::ok;
}

flatfile_association_result flatfile_outpost_save(const std::string &root,
						  const flatfile_outpost_record &outpost,
						  std::string *error)
{
	if (root.empty() || outpost.outpost_id < 0 || outpost.outpost_id >= 3)
		return flatfile_association_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_association_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_association_result::ok)
		return recovered;
	outpost_catalog catalog;
	const auto loaded = load_outpost_catalog(root, &catalog, error);
	if (loaded != flatfile_association_result::ok)
		return loaded;
	auto &existing = catalog.outposts[outpost.outpost_id];
	if (outpost_equal(existing, outpost))
		return flatfile_association_result::unchanged;
	existing = outpost;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_association_result::conflict;
	++catalog.revision;
	if (!valid_outposts(catalog))
		return flatfile_association_result::invalid;
	std::vector<uint8_t> encoded;
	if (!encode_outposts(catalog, &encoded))
		return flatfile_association_result::invalid;
	return flatfile_atomic_write(domains_directory(root), outpost_filename, encoded, error) ?
		       flatfile_association_result::ok :
		       flatfile_association_result::io_error;
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
