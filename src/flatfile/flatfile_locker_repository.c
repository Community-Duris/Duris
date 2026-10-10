#include "flatfile/flatfile_locker_repository.h"

#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'L', 'O', 'C', 'K', 0 };
constexpr uint32_t catalog_version = 2;
constexpr uint32_t catalog_legacy_version = 1;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t locker_maximum = 65536;
constexpr size_t chest_maximum = 262144;
constexpr size_t access_maximum = 1048576;
constexpr size_t locker_name_maximum = 100;
constexpr size_t account_name_maximum = 50;
constexpr size_t chest_name_maximum = 32;
constexpr size_t password_hash_maximum = 64;
constexpr size_t access_name_maximum = 255;
constexpr size_t sort_config_maximum = 4096;
constexpr const char *catalog_filename = "locker_catalog";

struct locker_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> access;
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

	void string(const std::string &value, size_t maximum)
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

	bool string(std::string *value, size_t maximum)
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
		return true;
	}

	bool bytes(std::vector<uint8_t> *value, size_t maximum)
	{
		uint32_t length = 0;
		if (!value || !number(&length) || !length || length > maximum || offset > size ||
		    size - offset < length)
			return false;
		try
		{
			value->assign(data + offset, data + offset + length);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		offset += length;
		return true;
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

bool valid_name(const std::string &name, size_t maximum)
{
	if (name.empty() || name.size() > maximum)
		return false;
	for (unsigned char character : name)
		if (character < 0x21 || character > 0x7e)
			return false;
	return true;
}

bool locker_less(const flatfile_locker_record &left, const flatfile_locker_record &right)
{
	return left.locker_id < right.locker_id;
}

bool chest_less(const flatfile_locker_chest_record &left, const flatfile_locker_chest_record &right)
{
	return left.chest_id < right.chest_id;
}

bool access_less(const flatfile_locker_access_record &left,
		 const flatfile_locker_access_record &right)
{
	if (left.owner_name != right.owner_name)
		return left.owner_name < right.owner_name;
	return left.visitor_name < right.visitor_name;
}

/* Validate canonical locker identities, nested chests, items, and access policy. */
bool valid_catalog(const locker_catalog &catalog)
{
	if (!catalog.revision || catalog.lockers.size() > locker_maximum ||
	    catalog.access.size() > access_maximum ||
	    !std::is_sorted(catalog.lockers.begin(), catalog.lockers.end(), locker_less) ||
	    !std::is_sorted(catalog.access.begin(), catalog.access.end(), access_less))
		return false;
	std::unordered_set<std::string> locker_names;
	std::unordered_set<int32_t> player_owners;
	std::unordered_set<int32_t> association_owners;
	std::unordered_set<std::string> account_owners;
	std::unordered_set<uint32_t> chest_ids;
	std::unordered_set<uint64_t> item_uids;
	size_t chest_count = 0;
	try
	{
		locker_names.reserve(catalog.lockers.size());
		player_owners.reserve(catalog.lockers.size());
		association_owners.reserve(catalog.lockers.size());
		account_owners.reserve(catalog.lockers.size());
		chest_ids.reserve(std::min(chest_maximum, catalog.lockers.size() * 2));
		for (size_t locker_index = 0; locker_index < catalog.lockers.size(); ++locker_index)
		{
			const auto &locker = catalog.lockers[locker_index];
			const bool has_account_owner = locker.account_owner.has_value();
			const unsigned int owner_count = (locker.owner_pid > 0 ? 1U : 0U) +
							 (locker.owner_assoc_id > 0 ? 1U : 0U) +
							 (has_account_owner ? 1U : 0U);
			if (!locker.locker_id || !locker.revision ||
			    !valid_name(locker.locker_name, locker_name_maximum) ||
			    locker.locker_name != canonical_name(locker.locker_name) ||
			    owner_count != 1 || locker.owner_pid < 0 || locker.owner_assoc_id < 0 ||
			    (locker.owner_pid > 0 &&
			     !player_owners.insert(locker.owner_pid).second) ||
			    (locker.owner_assoc_id > 0 &&
			     !association_owners.insert(locker.owner_assoc_id).second) ||
			    (locker_index &&
			     catalog.lockers[locker_index - 1].locker_id == locker.locker_id) ||
			    !locker_names.insert(locker.locker_name).second ||
			    !std::is_sorted(locker.chests.begin(), locker.chests.end(), chest_less))
				return false;
			if (has_account_owner)
			{
				const auto &owner = *locker.account_owner;
				const std::string expected_name =
					"account." + owner.account_name + "." +
					std::to_string(owner.racewar_side) + ".locker";
				std::string owner_key = owner.account_name;
				owner_key.push_back('\0');
				owner_key.push_back(static_cast<char>(owner.racewar_side));
				if (!valid_name(owner.account_name, account_name_maximum) ||
				    owner.account_name != canonical_name(owner.account_name) ||
				    owner.racewar_side < 0 || owner.racewar_side > 4 ||
				    locker.racewar != owner.racewar_side ||
				    locker.locker_name != expected_name ||
				    !account_owners.insert(std::move(owner_key)).second)
					return false;
			}
			else if (locker.locker_name.rfind("account.", 0) == 0)
			{
				return false;
			}
			if (locker.chests.empty() ||
			    locker.chests.size() > chest_maximum - chest_count)
				return false;
			chest_count += locker.chests.size();
			size_t public_count = 0;
			std::unordered_set<std::string> chest_names;
			chest_names.reserve(locker.chests.size());
			for (size_t chest_index = 0; chest_index < locker.chests.size();
			     ++chest_index)
			{
				const auto &chest = locker.chests[chest_index];
				if (!chest.chest_id || !chest.revision ||
				    !valid_name(chest.chest_name, chest_name_maximum) ||
				    chest.chest_name != canonical_name(chest.chest_name) ||
				    chest.password_hash.size() > password_hash_maximum ||
				    chest.sort_config.size() > sort_config_maximum ||
				    (chest.is_public && !chest.password_hash.empty()) ||
				    (chest_index &&
				     locker.chests[chest_index - 1].chest_id == chest.chest_id) ||
				    !chest_ids.insert(chest.chest_id).second ||
				    !chest_names.insert(chest.chest_name).second)
					return false;
				public_count += chest.is_public ? 1 : 0;
				std::vector<uint8_t> encoded_items;
				if (player_item_snapshot_list_encode(chest.items, &encoded_items) !=
				    player_snapshot_codec_result::ok)
					return false;
				for (const auto &item : chest.items)
					if (!item.object_uid || item.vnum <= 0 ||
					    item.equipment_slot != -1 ||
					    !item_uids.insert(item.object_uid).second)
						return false;
			}
			if (public_count != 1)
				return false;
		}
		for (size_t index = 0; index < catalog.access.size(); ++index)
		{
			const auto &entry = catalog.access[index];
			if (!entry.revision || !valid_name(entry.owner_name, access_name_maximum) ||
			    !valid_name(entry.visitor_name, access_name_maximum) ||
			    entry.owner_name != canonical_name(entry.owner_name) ||
			    entry.visitor_name != canonical_name(entry.visitor_name) ||
			    !locker_names.count(entry.owner_name) ||
			    (index && entry.owner_name == catalog.access[index - 1].owner_name &&
			     entry.visitor_name == catalog.access[index - 1].visitor_name))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

/* Encode the canonical catalog using the current version-two owner format. */
bool encode_catalog(const locker_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.lockers.size());
	payload.number<uint32_t>(catalog.access.size());
	for (const auto &locker : catalog.lockers)
	{
		payload.number(locker.locker_id);
		payload.string(locker.locker_name, locker_name_maximum);
		payload.number(locker.owner_pid);
		payload.number(locker.owner_assoc_id);
		payload.number<uint8_t>(locker.account_owner.has_value() ? 1 : 0);
		if (locker.account_owner)
		{
			payload.string(locker.account_owner->account_name, account_name_maximum);
			payload.number(locker.account_owner->racewar_side);
		}
		payload.number(locker.racewar);
		payload.number(locker.race);
		payload.number(locker.revision);
		payload.number<uint32_t>(locker.chests.size());
		for (const auto &chest : locker.chests)
		{
			std::vector<uint8_t> encoded_items;
			if (player_item_snapshot_list_encode(chest.items, &encoded_items) !=
			    player_snapshot_codec_result::ok)
				return false;
			payload.number(chest.chest_id);
			payload.string(chest.chest_name, chest_name_maximum);
			payload.string(chest.password_hash, password_hash_maximum);
			payload.number<uint8_t>(chest.is_public ? 1 : 0);
			payload.string(chest.sort_config, sort_config_maximum);
			payload.number(chest.revision);
			payload.number<uint32_t>(encoded_items.size());
			payload.raw(encoded_items.data(), encoded_items.size());
		}
	}
	for (const auto &entry : catalog.access)
	{
		payload.string(entry.owner_name, access_name_maximum);
		payload.string(entry.visitor_name, access_name_maximum);
		payload.number(entry.revision);
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

/* Decode current catalogs while retaining version-one player and guild compatibility. */
bool decode_catalog(const std::vector<uint8_t> &bytes, locker_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) ||
	    (version != catalog_version && version != catalog_legacy_version) || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t locker_count = 0, access_count = 0;
	if (!payload.number(&locker_count) || !payload.number(&access_count) ||
	    locker_count > locker_maximum || access_count > access_maximum)
		return false;
	locker_catalog decoded;
	decoded.revision = revision;
	size_t chest_count = 0;
	try
	{
		decoded.lockers.resize(locker_count);
		decoded.access.resize(access_count);
		for (auto &locker : decoded.lockers)
		{
			uint32_t count = 0;
			if (!payload.number(&locker.locker_id) ||
			    !payload.string(&locker.locker_name, locker_name_maximum) ||
			    !payload.number(&locker.owner_pid) ||
			    !payload.number(&locker.owner_assoc_id))
				return false;
			if (version == catalog_version)
			{
				uint8_t has_account_owner = 0;
				flatfile_account_locker_identity owner;
				if (!payload.number(&has_account_owner) || has_account_owner > 1 ||
				    (has_account_owner &&
				     (!payload.string(&owner.account_name, account_name_maximum) ||
				      !payload.number(&owner.racewar_side))))
					return false;
				if (has_account_owner)
					locker.account_owner = std::move(owner);
			}
			if (!payload.number(&locker.racewar) || !payload.number(&locker.race) ||
			    !payload.number(&locker.revision) || !payload.number(&count) ||
			    count > chest_maximum - chest_count)
				return false;
			chest_count += count;
			locker.chests.resize(count);
			for (auto &chest : locker.chests)
			{
				uint8_t is_public = 0;
				std::vector<uint8_t> encoded_items;
				if (!payload.number(&chest.chest_id) ||
				    !payload.string(&chest.chest_name, chest_name_maximum) ||
				    !payload.string(&chest.password_hash, password_hash_maximum) ||
				    !payload.number(&is_public) || is_public > 1 ||
				    !payload.string(&chest.sort_config, sort_config_maximum) ||
				    !payload.number(&chest.revision) ||
				    !payload.bytes(&encoded_items, PLAYER_SNAPSHOT_MAX_BYTES) ||
				    player_item_snapshot_list_decode(
					    encoded_items.data(), encoded_items.size(),
					    &chest.items) != player_snapshot_codec_result::ok)
					return false;
				chest.is_public = is_public != 0;
			}
		}
		for (auto &entry : decoded.access)
			if (!payload.string(&entry.owner_name, access_name_maximum) ||
			    !payload.string(&entry.visitor_name, access_name_maximum) ||
			    !payload.number(&entry.revision))
				return false;
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

flatfile_locker_result recover(const std::string &root, const flatfile_authority_lock &lock,
			       std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_locker_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_locker_result::io_error :
		       flatfile_locker_result::invalid;
}

flatfile_locker_result load_catalog(const std::string &root, locker_catalog *catalog,
				    std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_locker_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_locker_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "locker catalog is corrupt";
		return flatfile_locker_result::invalid;
	}
	return flatfile_locker_result::ok;
}

bool catalog_equal(locker_catalog left, locker_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}

bool payload_items_match(const item_transfer_payload &payload,
			 const std::vector<player_item_snapshot> &items)
{
	if (items.empty() || items.size() != payload.item_count ||
	    items.front().object_uid != item_transfer_result_root(payload))
		return false;
	std::unordered_set<uint64_t> expected;
	try
	{
		expected.reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
			expected.insert(payload.items[index].item_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return expected.size() == items.size() &&
	       std::all_of(items.begin(), items.end(),
			   [&](const auto &item) { return expected.contains(item.object_uid); });
}

/* Collect sorted item-custody evidence for every chest in a locker. */
bool collect_locker_custody(const flatfile_locker_record &locker,
			    std::vector<flatfile_locker_custody_owner> *custody)
{
	if (!custody)
		return false;
	try
	{
		for (const auto &chest : locker.chests)
		{
			flatfile_locker_custody_owner owner;
			owner.owner = { item_owner_type::locker, locker.locker_id,
					static_cast<uint64_t>(chest.chest_id) };
			owner.items.reserve(chest.items.size());
			for (const auto &item : chest.items)
				owner.items.push_back({ item.object_uid, item.vnum });
			std::sort(owner.items.begin(), owner.items.end(),
				  [](const auto &left, const auto &right)
				  { return left.item_uid < right.item_uid; });
			custody->push_back(std::move(owner));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

} // namespace

flatfile_locker_result flatfile_locker_establish(
	const std::string &root, const std::vector<flatfile_locker_record> &lockers,
	const std::vector<flatfile_locker_access_record> &access, std::string *error)
{
	if (root.empty())
		return flatfile_locker_result::invalid;
	locker_catalog candidate;
	try
	{
		candidate.lockers = lockers;
		candidate.access = access;
		for (auto &locker : candidate.lockers)
		{
			locker.locker_name = canonical_name(locker.locker_name);
			std::sort(locker.chests.begin(), locker.chests.end(), chest_less);
			for (auto &chest : locker.chests)
				chest.chest_name = canonical_name(chest.chest_name);
		}
		for (auto &entry : candidate.access)
		{
			entry.owner_name = canonical_name(entry.owner_name);
			entry.visitor_name = canonical_name(entry.visitor_name);
		}
		std::sort(candidate.lockers.begin(), candidate.lockers.end(), locker_less);
		std::sort(candidate.access.begin(), candidate.access.end(), access_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_locker_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_locker_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_locker_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_locker_result::ok)
		return recovered;
	locker_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_locker_result::ok)
		return catalog_equal(existing, candidate) ? flatfile_locker_result::already_exists :
							    flatfile_locker_result::invalid;
	if (loaded != flatfile_locker_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_locker_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_locker_result::io_error;
	return flatfile_locker_result::ok;
}

flatfile_locker_result flatfile_locker_list(const std::string &root,
					    std::vector<flatfile_locker_record> *lockers,
					    std::vector<flatfile_locker_access_record> *access,
					    std::string *error)
{
	if (root.empty() || !lockers || !access)
		return flatfile_locker_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_locker_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_locker_result::ok)
		return recovered;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	*lockers = std::move(catalog.lockers);
	*access = std::move(catalog.access);
	return flatfile_locker_result::ok;
}

flatfile_locker_result
flatfile_locker_recovery_list_locked(const std::string &root, const flatfile_authority_lock &lock,
				     std::vector<flatfile_locker_record> *lockers,
				     std::string *error)
{
	if (!lock.matches(root) || !lockers)
		return flatfile_locker_result::invalid;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	*lockers = std::move(catalog.lockers);
	return flatfile_locker_result::ok;
}

flatfile_locker_result flatfile_locker_read_coin(const std::string &root,
						 const flatfile_authority_lock &lock,
						 const item_owner_identity &owner, uint64_t uid,
						 player_item_snapshot *item, std::string *error)
{
	if (!lock.matches(root) || !uid || !item || owner.type != item_owner_type::locker)
		return flatfile_locker_result::invalid;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	size_t matches = 0;
	for (const auto &locker : catalog.lockers)
		if (locker.locker_id == owner.id)
			for (const auto &chest : locker.chests)
				if (chest.chest_id == owner.context_id)
					for (const auto &candidate : chest.items)
						if (candidate.object_uid == uid)
						{
							*item = candidate;
							++matches;
						}
	return matches == 1 ? flatfile_locker_result::ok :
	       matches	    ? flatfile_locker_result::conflict :
			      flatfile_locker_result::not_found;
}

/* Prepare player-owned locker and visitor-grant removal with exact custody evidence. */
flatfile_locker_result
flatfile_locker_prepare_player_remove(const std::string &root, const flatfile_authority_lock &lock,
				      uint32_t pid, const std::string &player_name,
				      flatfile_locker_player_removal *removal, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !pid || pid > static_cast<uint32_t>(INT32_MAX) ||
	    !removal || !valid_name(player_name, access_name_maximum))
		return flatfile_locker_result::invalid;
	*removal = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_locker_result::ok)
		return recovered;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	const std::string canonical_player = canonical_name(player_name);
	const int32_t owner_pid = static_cast<int32_t>(pid);
	auto locker = std::find_if(catalog.lockers.begin(), catalog.lockers.end(),
				   [owner_pid](const auto &entry)
				   { return entry.owner_pid == owner_pid; });
	const bool has_locker = locker != catalog.lockers.end();
	const std::string removed_name = has_locker ? locker->locker_name : std::string();
	try
	{
		if (has_locker)
		{
			removal->custody.reserve(locker->chests.size());
			if (!collect_locker_custody(*locker, &removal->custody))
				return flatfile_locker_result::io_error;
			catalog.lockers.erase(locker);
		}
		const size_t old_access_size = catalog.access.size();
		catalog.access.erase(
			std::remove_if(catalog.access.begin(), catalog.access.end(),
				       [&](const auto &entry)
				       {
					       return entry.visitor_name == canonical_player ||
						      (has_locker &&
						       entry.owner_name == removed_name);
				       }),
			catalog.access.end());
		if (!has_locker && old_access_size == catalog.access.size())
			return flatfile_locker_result::unchanged;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_locker_result::io_error;
	}
	if (catalog.revision == UINT64_MAX)
		return flatfile_locker_result::conflict;
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_locker_result::invalid;
	removal->operation.store = flatfile_authority_store::domains;
	removal->operation.kind = flatfile_authority_operation_kind::write;
	removal->operation.filename = catalog_filename;
	removal->operation.bytes = std::move(encoded);
	return flatfile_locker_result::ok;
}

/* Prepare removal of every locker owned by an account and every grant it visits. */
flatfile_locker_result
flatfile_locker_prepare_account_remove(const std::string &root, const flatfile_authority_lock &lock,
				       const std::string &account_name,
				       flatfile_locker_player_removal *removal, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !removal ||
	    !valid_name(account_name, account_name_maximum))
		return flatfile_locker_result::invalid;
	*removal = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_locker_result::ok)
		return recovered;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	const std::string canonical_account = canonical_name(account_name);
	std::unordered_set<std::string> removed_names;
	try
	{
		for (const auto &locker : catalog.lockers)
		{
			if (!locker.account_owner ||
			    locker.account_owner->account_name != canonical_account)
				continue;
			removed_names.insert(locker.locker_name);
			if (!collect_locker_custody(locker, &removal->custody))
				return flatfile_locker_result::io_error;
		}
		catalog.lockers.erase(
			std::remove_if(catalog.lockers.begin(), catalog.lockers.end(),
				       [&](const auto &locker) {
					       return locker.account_owner &&
						      locker.account_owner->account_name ==
							      canonical_account;
				       }),
			catalog.lockers.end());
		const size_t old_access_size = catalog.access.size();
		catalog.access.erase(
			std::remove_if(catalog.access.begin(), catalog.access.end(),
				       [&](const auto &entry) {
					       return entry.visitor_name == canonical_account ||
						      removed_names.contains(entry.owner_name);
				       }),
			catalog.access.end());
		if (removed_names.empty() && old_access_size == catalog.access.size())
			return flatfile_locker_result::unchanged;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_locker_result::io_error;
	}
	if (catalog.revision == UINT64_MAX)
		return flatfile_locker_result::conflict;
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_locker_result::invalid;
	removal->operation.store = flatfile_authority_store::domains;
	removal->operation.kind = flatfile_authority_operation_kind::write;
	removal->operation.filename = catalog_filename;
	removal->operation.bytes = std::move(encoded);
	return flatfile_locker_result::ok;
}

flatfile_locker_result
flatfile_locker_prepare_item_transfer(const std::string &root, const flatfile_authority_lock &lock,
				      const item_transfer_payload &payload,
				      flatfile_locker_transfer_mutation *mutation,
				      std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return flatfile_locker_result::invalid;
	*mutation = {};
	const bool deposit = payload.to_owner.type == item_owner_type::locker;
	const bool withdraw = payload.from_owner.type == item_owner_type::locker;
	if (deposit == withdraw ||
	    (deposit && (payload.reason != item_transfer_reason::locker_deposit ||
			 payload.from_owner.type != item_owner_type::player)) ||
	    (withdraw && (payload.reason != item_transfer_reason::locker_withdraw ||
			  payload.to_owner.type != item_owner_type::player)))
		return flatfile_locker_result::invalid;
	const item_owner_identity locker_owner = deposit ? payload.to_owner : payload.from_owner;
	if (!locker_owner.id || locker_owner.id > UINT32_MAX || !locker_owner.context_id ||
	    locker_owner.context_id > UINT32_MAX || payload.target_parent_item_uid)
		return flatfile_locker_result::invalid;
	std::vector<player_item_snapshot> exact_items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &exact_items) != player_snapshot_codec_result::ok ||
	    !payload_items_match(payload, exact_items))
		return flatfile_locker_result::invalid;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	auto locker = std::find_if(catalog.lockers.begin(), catalog.lockers.end(),
				   [&](const auto &entry)
				   { return entry.locker_id == locker_owner.id; });
	if (locker == catalog.lockers.end())
		return flatfile_locker_result::not_found;
	auto chest = std::find_if(locker->chests.begin(), locker->chests.end(),
				  [&](const auto &entry)
				  { return entry.chest_id == locker_owner.context_id; });
	if (chest == locker->chests.end())
		return flatfile_locker_result::not_found;
	if (catalog.revision == UINT64_MAX || locker->revision == UINT64_MAX ||
	    chest->revision == UINT64_MAX)
		return flatfile_locker_result::conflict;
	try
	{
		mutation->expected_items.reserve(chest->items.size());
		for (const auto &item : chest->items)
			mutation->expected_items.push_back({ item.object_uid, item.vnum });
		std::sort(mutation->expected_items.begin(), mutation->expected_items.end(),
			  [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		if (deposit)
		{
			std::unordered_set<uint64_t> existing;
			for (const auto &stored_locker : catalog.lockers)
				for (const auto &stored_chest : stored_locker.chests)
					for (const auto &item : stored_chest.items)
						existing.insert(item.object_uid);
			for (const auto &item : exact_items)
				if (existing.contains(item.object_uid))
					return flatfile_locker_result::conflict;
			const int32_t offset = static_cast<int32_t>(chest->items.size());
			for (auto item : exact_items)
			{
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += offset;
				chest->items.push_back(std::move(item));
			}
		}
		else
		{
			std::vector<uint64_t> selected_roots;
			std::vector<player_item_snapshot> selected;
			std::vector<player_item_snapshot> remaining;
			if (!item_transfer_selected_roots(payload, &selected_roots) ||
			    player_item_snapshot_extract_forest(chest->items, selected_roots,
								&selected, &remaining) !=
				    player_snapshot_codec_result::ok)
				return flatfile_locker_result::conflict;
			std::vector<uint8_t> selected_blob;
			if (player_item_snapshot_list_encode(selected, &selected_blob) !=
				    player_snapshot_codec_result::ok ||
			    selected_blob.size() != payload.item_blob_size ||
			    !std::equal(selected_blob.begin(), selected_blob.end(),
					payload.item_blob.begin()))
				return flatfile_locker_result::conflict;
			chest->items = std::move(remaining);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_locker_result::io_error;
	}
	++catalog.revision;
	++locker->revision;
	++chest->revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_locker_result::invalid;
	mutation->after_image = { catalog_filename, std::move(encoded) };
	mutation->locker_revision = locker->revision;
	mutation->chest_revision = chest->revision;
	return flatfile_locker_result::ok;
}

// Full borrowed-lock ordinary CURRENT companion. Original methods above are
// untouched. No execution/recovery/publication or activation authority is added.
#include <cerrno>
#include <iterator>
#include <limits>
#include <string_view>
#include <utility>

namespace
{
constexpr size_t locker_bounded_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t locker_bounded_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t locker_bounded_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t locker_bounded_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t locker_bounded_vector_frames =
	locker_bounded_allocator_frames + locker_bounded_copy_frames +
	locker_bounded_relocate_frames + locker_bounded_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t locker_bounded_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + locker_bounded_allocator_frames;

// Nontrivial row/description construction and destruction are source scopes,
// not heap metadata. The nested member objects already live in sizeof(row).
constexpr size_t locker_bounded_nontrivial_frames =
	// default_n_1<false>: first/n/cur/return; _Construct/addressof/placement.
	3 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) + sizeof(size_t) +
	// Actual aggregate row and description this, four row strings and two
	// description strings: string()/allocator hider/use-local-data/set-length.
	2 * sizeof(void *) +
	6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	// row/description nested vector()/Vector_base()/Vector_impl()/data() and
	// allocator return carriers. Three source member vector types.
	3 * (5 * sizeof(void *) + sizeof(std::allocator<int32_t>)) +
	// Nontrivial _Destroy range/aux::__destroy/destroy_at/__addressof; actual
	// row/description destructor this then six string destructors/dispose/
	// _M_is_local/_M_destroy and three nested vector destroy/deallocate scopes.
	8 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	3 * locker_bounded_allocator_frames;
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t locker_bounded_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t locker_bounded_string_frames =
	locker_bounded_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	locker_bounded_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);

bool locker_bounded_add(size_t &bytes, size_t count) noexcept
{
	if (count > SIZE_MAX - bytes)
		return false;
	bytes += count;
	return true;
}
template <typename T>
bool locker_bounded_vector_heap(const std::vector<T> &rows, size_t &bytes) noexcept
{
	return rows.capacity() <= SIZE_MAX / sizeof(T) &&
	       locker_bounded_add(bytes, rows.capacity() * sizeof(T));
}
bool locker_bounded_string_heap(const std::string &value, size_t &bytes) noexcept
{
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && locker_bounded_add(bytes, value.capacity() + 1));
}
struct locker_bounded_account_key
{
	std::string_view name;
	int8_t side;
	bool operator<(const locker_bounded_account_key &other) const noexcept
	{
		return name != other.name ? name < other.name : side < other.side;
	}
	bool operator==(const locker_bounded_account_key &other) const noexcept
	{
		return name == other.name && side == other.side;
	}
};
struct locker_bounded_chest_key
{
	size_t locker;
	std::string_view name;
	bool operator<(const locker_bounded_chest_key &other) const noexcept
	{
		return locker != other.locker ? locker < other.locker : name < other.name;
	}
	bool operator==(const locker_bounded_chest_key &other) const noexcept
	{
		return locker == other.locker && name == other.name;
	}
};
struct locker_bounded_workspace;
struct locker_bounded_frame
{
	locker_bounded_workspace &owner;
	locker_bounded_frame *previous;
	size_t bytes;
	locker_bounded_frame(locker_bounded_workspace &, size_t) noexcept;
	~locker_bounded_frame();
};
struct locker_bounded_workspace
{
	locker_catalog catalog;
	std::vector<uint8_t> file, encoded_items;
	std::string directory, filename;
	flatfile_account_locker_identity account;
	std::vector<std::string_view> names;
	std::vector<int32_t> players, associations;
	std::vector<locker_bounded_account_key> accounts;
	std::vector<uint32_t> chest_ids;
	std::vector<locker_bounded_chest_key> chest_names;
	std::vector<uint64_t> item_uids;
	flatfile_scratch_reserve_fn reserve;
	void *context;
	size_t outer;
	// Actual decoder heap capacities, updated only after successful allocation
	// or complete bounded item transfer. Includes the temporary account string
	// until that same allocation moves into its retained account owner.
	size_t decoded_heap = 0;
	size_t transferred_heap = 0;
	bool rejected = false;
	bool allocation_failed = false;
	locker_bounded_frame *frames = nullptr;
	bool refuse() noexcept
	{
		rejected = true;
		errno = ENOBUFS;
		return false;
	}
	bool current(size_t *out) noexcept
	{
		size_t bytes = outer;
		if (!locker_bounded_add(bytes, sizeof(*this)) ||
		    !locker_bounded_add(bytes, decoded_heap) ||
		    !locker_bounded_vector_heap(file, bytes) ||
		    !locker_bounded_vector_heap(encoded_items, bytes) ||
		    !locker_bounded_string_heap(directory, bytes) ||
		    !locker_bounded_string_heap(filename, bytes) ||
		    !locker_bounded_vector_heap(names, bytes) ||
		    !locker_bounded_vector_heap(players, bytes) ||
		    !locker_bounded_vector_heap(associations, bytes) ||
		    !locker_bounded_vector_heap(accounts, bytes) ||
		    !locker_bounded_vector_heap(chest_ids, bytes) ||
		    !locker_bounded_vector_heap(chest_names, bytes) ||
		    !locker_bounded_vector_heap(item_uids, bytes))
			return refuse();
		for (auto *frame = frames; frame; frame = frame->previous)
			if (!locker_bounded_add(bytes, frame->bytes))
				return refuse();
		// Actual arithmetic/capacity/string observer parameters and returns.
		constexpr size_t observer = 9 * sizeof(void *) + 5 * sizeof(size_t) +
					    4 * sizeof(bool) +
					    9 * (sizeof(void *) + sizeof(size_t));
		if (!locker_bounded_add(bytes, observer))
			return refuse();
		*out = bytes;
		return true;
	}
	bool admit(size_t extra) noexcept
	{
		size_t bytes = 0;
		if (!current(&bytes) || !locker_bounded_add(bytes, extra) || !reserve ||
		    !reserve(bytes, context))
			return refuse();
		return true;
	}
	static bool relay(size_t bytes, void *context) noexcept
	{
		auto &work = *static_cast<locker_bounded_workspace *>(context);
		if (!work.reserve || !work.reserve(bytes, work.context))
			return work.refuse();
		return true;
	}
	template <typename T> bool decoded_resize(std::vector<T> &rows, size_t count)
	{
		constexpr size_t own = sizeof(locker_bounded_frame) + 2 * sizeof(void *) +
				       2 * sizeof(size_t) + sizeof(bool);
		if (!admit(own))
			return false;
		locker_bounded_frame frame(*this, own);
		// Every decoder destination is fresh, matching original resize's exact
		// GCC13 request. Do not reinterpret a reused/nonempty container as fresh.
		if (!rows.empty() || rows.capacity())
			return false;
		size_t request = locker_bounded_vector_frames + locker_bounded_nontrivial_frames;
		if (count > rows.max_size() || count > SIZE_MAX / sizeof(T) ||
		    !locker_bounded_add(request, count * sizeof(T)) || !admit(request))
			return refuse();
		rows.resize(count);
		return locker_bounded_add(decoded_heap, rows.capacity() * sizeof(T)) || refuse();
	}
	bool decoded_string(std::string &value, const uint8_t *data, size_t length)
	{
		constexpr size_t own = sizeof(locker_bounded_frame) + 3 * sizeof(void *) +
				       5 * sizeof(size_t) + sizeof(bool);
		if (!admit(own))
			return false;
		locker_bounded_frame frame(*this, own);
		size_t request = locker_bounded_string_frames;
		const size_t old_heap = value.capacity() <= 15 ? 0 : value.capacity() + 1;
		if (length > value.capacity())
		{
			size_t next = length;
			if (value.capacity() <= SIZE_MAX / 2 && next < 2 * value.capacity())
				next = 2 * value.capacity();
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX || !locker_bounded_add(request, next + 1))
				return refuse();
		}
		if (!admit(request))
			return false;
		value.assign(reinterpret_cast<const char *>(data), length);
		const size_t new_heap = value.capacity() <= 15 ? 0 : value.capacity() + 1;
		if (old_heap > decoded_heap)
			return refuse();
		decoded_heap -= old_heap;
		return locker_bounded_add(decoded_heap, new_heap) || refuse();
	}
	template <typename T> bool reserve_keys(std::vector<T> &rows, size_t count)
	{
		constexpr size_t own = sizeof(locker_bounded_frame) + 2 * sizeof(void *) +
				       2 * sizeof(size_t) + sizeof(bool);
		if (!admit(own))
			return false;
		locker_bounded_frame frame(*this, own);
		size_t request = locker_bounded_vector_frames;
		if (rows.capacity() || count > rows.max_size() || count > SIZE_MAX / sizeof(T) ||
		    !locker_bounded_add(request, count * sizeof(T)) || !admit(request))
			return refuse();
		rows.reserve(count);
		return true;
	}
};
locker_bounded_frame::locker_bounded_frame(locker_bounded_workspace &work, size_t count) noexcept
	: owner(work)
	, previous(work.frames)
	, bytes(count)
{
	owner.frames = this;
}
locker_bounded_frame::~locker_bounded_frame()
{
	owner.frames = previous;
}
struct locker_bounded_decoder
{
	locker_bounded_workspace &work;
	const uint8_t *data;
	size_t size, offset = 0;
	template <typename T> bool number(T *value)
	{
		constexpr size_t own = sizeof(locker_bounded_frame) + 2 * sizeof(void *) +
				       sizeof(std::make_unsigned_t<T>) + sizeof(size_t) +
				       sizeof(bool);
		if (!work.admit(own))
			return false;
		locker_bounded_frame frame(work, own);
		if (!value || offset > size || size - offset < sizeof(T))
			return false;
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}
	bool string(std::string *value, size_t maximum)
	{
		constexpr size_t own = sizeof(locker_bounded_frame) + 3 * sizeof(void *) +
				       4 * sizeof(size_t) + sizeof(uint32_t) + sizeof(bool);
		if (!work.admit(own))
			return false;
		locker_bounded_frame frame(work, own);
		uint32_t length = 0;
		if (!value || !number(&length) || length > maximum || offset > size ||
		    size - offset < length)
			return false;
		if (!work.decoded_string(*value, data + offset, length))
			return false;
		offset += length;
		return true;
	}
	bool items(std::vector<player_item_snapshot> *value)
	{
		constexpr size_t own = sizeof(locker_bounded_frame) + 3 * sizeof(void *) +
				       4 * sizeof(size_t) + sizeof(uint32_t) + sizeof(bool) +
				       sizeof(player_snapshot_codec_result);
		if (!work.admit(own))
			return false;
		locker_bounded_frame frame(work, own);
		uint32_t length = 0;
		if (!value || !number(&length) || !length || length > PLAYER_SNAPSHOT_MAX_BYTES ||
		    offset > size || size - offset < length || !value->empty() || value->capacity())
			return false;
		size_t outer = 0, heap = 0;
		if (!work.current(&outer))
			return false;
		// The full original item span remains in the authentic SAME-FD file
		// buffer. Avoid an otherwise redundant byte-vector copy; every item
		// row/string, forest and semantic decoder check remains authoritative.
		errno = 0;
		const auto result = player_item_snapshot_list_decode_bounded(
			data + offset, length, value, locker_bounded_workspace::relay, &work, outer,
			&heap);
		if (result != player_snapshot_codec_result::ok)
		{
			if (errno == ENOBUFS)
				work.refuse();
			else if (result == player_snapshot_codec_result::allocation_failure)
				work.allocation_failed = true;
			return false;
		}
		if (!locker_bounded_add(work.decoded_heap, heap))
			return work.refuse();
		offset += length;
		return true;
	}
};

bool locker_bounded_canonical(const std::string &name, size_t maximum) noexcept
{
	if (!valid_name(name, maximum))
		return false;
	for (const char character : name)
		if (character !=
		    static_cast<char>(std::tolower(static_cast<unsigned char>(character))))
			return false;
	return true;
}
bool locker_bounded_account_name(const flatfile_locker_record &locker) noexcept
{
	const auto &owner = *locker.account_owner;
	if (!locker_bounded_canonical(owner.account_name, account_name_maximum) ||
	    owner.racewar_side < 0 || owner.racewar_side > 4 ||
	    locker.racewar != owner.racewar_side)
		return false;
	const char side = static_cast<char>('0' + owner.racewar_side);
	const std::array<std::string_view, 5> pieces = { "account.", owner.account_name, ".",
							 std::string_view(&side, 1), ".locker" };
	size_t offset = 0;
	for (const auto piece : pieces)
	{
		if (offset > locker.locker_name.size() ||
		    piece.size() > locker.locker_name.size() - offset ||
		    std::string_view(locker.locker_name).substr(offset, piece.size()) != piece)
			return false;
		offset += piece.size();
	}
	return offset == locker.locker_name.size();
}
template <typename T>
bool locker_bounded_unique(locker_bounded_workspace &work, std::vector<T> &values)
{
	// Allocation-free iterative heapsort. Actual callback/loop indices, three
	// captured references/size, swap's inline T temporary and comparison frames
	// are admitted prospectively; no guessed recursive std::sort depth.
	constexpr size_t own = sizeof(locker_bounded_frame) + 3 * sizeof(void *) +
			       9 * sizeof(size_t) + sizeof(T) + 4 * sizeof(void *) +
			       3 * sizeof(bool) + 4 * sizeof(std::string_view) +
			       4 * sizeof(size_t) + 4 * sizeof(void *) + sizeof(int);
	if (!work.admit(own))
		return false;
	locker_bounded_frame frame(work, own);
	const auto sift = [&](size_t root, size_t count)
	{
		while (root < count / 2)
		{
			size_t child = root * 2 + 1;
			if (child + 1 < count && values[child] < values[child + 1])
				++child;
			if (!(values[root] < values[child]))
				break;
			std::swap(values[root], values[child]);
			root = child;
		}
	};
	for (size_t start = values.size() / 2; start; --start)
		sift(start - 1, values.size());
	for (size_t count = values.size(); count > 1; --count)
	{
		std::swap(values[0], values[count - 1]);
		sift(0, count - 1);
	}
	for (size_t index = 1; index < values.size(); ++index)
		if (values[index] == values[index - 1])
			return false;
	return true;
}

bool locker_bounded_valid_catalog(locker_bounded_workspace &work)
{
	constexpr size_t own = sizeof(locker_bounded_frame) + 12 * sizeof(void *) +
			       8 * sizeof(size_t) + 3 * sizeof(bool) + sizeof(unsigned int) +
			       sizeof(std::array<std::string_view, 5>) + sizeof(char) +
			       locker_bounded_vector_frames + locker_bounded_nontrivial_frames +
			       4 * sizeof(std::string_view) + 8 * sizeof(void *) +
			       sizeof(player_snapshot_codec_result);
	if (!work.admit(own))
		return false;
	locker_bounded_frame frame(work, own);
	const auto &catalog = work.catalog;
	if (!catalog.revision || catalog.lockers.size() > locker_maximum ||
	    catalog.access.size() > access_maximum ||
	    !std::is_sorted(catalog.lockers.begin(), catalog.lockers.end(), locker_less) ||
	    !std::is_sorted(catalog.access.begin(), catalog.access.end(), access_less))
		return false;
	size_t chest_count = 0, item_count = 0;
	for (const auto &locker : catalog.lockers)
	{
		if (locker.chests.empty() || locker.chests.size() > chest_maximum - chest_count)
			return false;
		chest_count += locker.chests.size();
		for (const auto &chest : locker.chests)
			if (!locker_bounded_add(item_count, chest.items.size()))
				return work.refuse();
	}
	if (!work.reserve_keys(work.names, catalog.lockers.size()) ||
	    !work.reserve_keys(work.players, catalog.lockers.size()) ||
	    !work.reserve_keys(work.associations, catalog.lockers.size()) ||
	    !work.reserve_keys(work.accounts, catalog.lockers.size()) ||
	    !work.reserve_keys(work.chest_ids, chest_count) ||
	    !work.reserve_keys(work.chest_names, chest_count) ||
	    !work.reserve_keys(work.item_uids, item_count))
		return false;
	for (size_t locker_index = 0; locker_index < catalog.lockers.size(); ++locker_index)
	{
		const auto &locker = catalog.lockers[locker_index];
		const bool has_account_owner = locker.account_owner.has_value();
		const unsigned int owner_count = (locker.owner_pid > 0 ? 1U : 0U) +
						 (locker.owner_assoc_id > 0 ? 1U : 0U) +
						 (has_account_owner ? 1U : 0U);
		if (!locker.locker_id || !locker.revision ||
		    !locker_bounded_canonical(locker.locker_name, locker_name_maximum) ||
		    owner_count != 1 || locker.owner_pid < 0 || locker.owner_assoc_id < 0 ||
		    (locker_index &&
		     catalog.lockers[locker_index - 1].locker_id == locker.locker_id) ||
		    !std::is_sorted(locker.chests.begin(), locker.chests.end(), chest_less))
			return false;
		work.names.emplace_back(locker.locker_name);
		if (locker.owner_pid > 0)
			work.players.push_back(locker.owner_pid);
		if (locker.owner_assoc_id > 0)
			work.associations.push_back(locker.owner_assoc_id);
		if (has_account_owner)
		{
			if (!locker_bounded_account_name(locker))
				return false;
			// The original key is canonical account bytes + NUL + side. Valid
			// printable account names contain no NUL, so this complete tuple
			// has exactly the same equality law without string allocations.
			work.accounts.push_back({ locker.account_owner->account_name,
						  locker.account_owner->racewar_side });
		}
		else if (locker.locker_name.rfind("account.", 0) == 0)
			return false;
		size_t public_count = 0;
		for (size_t chest_index = 0; chest_index < locker.chests.size(); ++chest_index)
		{
			const auto &chest = locker.chests[chest_index];
			if (!chest.chest_id || !chest.revision ||
			    !locker_bounded_canonical(chest.chest_name, chest_name_maximum) ||
			    chest.password_hash.size() > password_hash_maximum ||
			    chest.sort_config.size() > sort_config_maximum ||
			    (chest.is_public && !chest.password_hash.empty()) ||
			    (chest_index &&
			     locker.chests[chest_index - 1].chest_id == chest.chest_id))
				return false;
			work.chest_ids.push_back(chest.chest_id);
			work.chest_names.push_back({ locker_index, chest.chest_name });
			public_count += chest.is_public ? 1 : 0;
			size_t outer = 0;
			if (!work.current(&outer))
				return false;
			errno = 0;
			const auto encoded_result = player_item_snapshot_list_encode_bounded(
				chest.items, &work.encoded_items, locker_bounded_workspace::relay,
				&work, outer);
			if (encoded_result != player_snapshot_codec_result::ok)
			{
				if (errno == ENOBUFS)
					work.refuse();
				else if (encoded_result ==
					 player_snapshot_codec_result::allocation_failure)
					work.allocation_failed = true;
				return false;
			}
			for (const auto &item : chest.items)
			{
				if (!item.object_uid || item.vnum <= 0 || item.equipment_slot != -1)
					return false;
				work.item_uids.push_back(item.object_uid);
			}
			if (!work.admit(locker_bounded_move_frames + locker_bounded_vector_frames))
				return false;
			std::vector<uint8_t>().swap(work.encoded_items);
		}
		if (public_count != 1)
			return false;
	}
	if (!locker_bounded_unique(work, work.names) ||
	    !locker_bounded_unique(work, work.players) ||
	    !locker_bounded_unique(work, work.associations) ||
	    !locker_bounded_unique(work, work.accounts) ||
	    !locker_bounded_unique(work, work.chest_ids) ||
	    !locker_bounded_unique(work, work.chest_names) ||
	    !locker_bounded_unique(work, work.item_uids))
		return false;
	for (size_t index = 0; index < catalog.access.size(); ++index)
	{
		const auto &entry = catalog.access[index];
		if (!entry.revision ||
		    !locker_bounded_canonical(entry.owner_name, access_name_maximum) ||
		    !locker_bounded_canonical(entry.visitor_name, access_name_maximum) ||
		    !std::binary_search(work.names.begin(), work.names.end(),
					std::string_view(entry.owner_name)) ||
		    (index && entry.owner_name == catalog.access[index - 1].owner_name &&
		     entry.visitor_name == catalog.access[index - 1].visitor_name))
			return false;
	}
	return true;
}

bool locker_bounded_decode_catalog(locker_bounded_workspace &work)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	constexpr size_t own =
		sizeof(locker_bounded_frame) + 2 * sizeof(locker_bounded_decoder) +
		9 * sizeof(void *) + 3 * sizeof(size_t) + 5 * sizeof(uint32_t) + sizeof(uint64_t) +
		2 * sizeof(uint8_t) + sizeof(bool) +
		sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
		// Genuine digest call arguments. No guessed SHA256_CTX is substituted
		// for OpenSSL3's provider/private allocator or emitted call frames;
		// those native/library scopes remain qualification obligations.
		3 * sizeof(void *) + 2 * sizeof(size_t) + locker_bounded_string_frames +
		locker_bounded_nontrivial_frames;
	if (!work.admit(own))
		return false;
	locker_bounded_frame frame(work, own);
	const auto &bytes = work.file;
	if (bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	locker_bounded_decoder header{ work, bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) ||
	    (version != catalog_version && version != catalog_legacy_version) || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	locker_bounded_decoder payload{ work, payload_bytes, payload_size };
	uint32_t locker_count = 0, access_count = 0;
	if (!payload.number(&locker_count) || !payload.number(&access_count) ||
	    locker_count > locker_maximum || access_count > access_maximum)
		return false;
	auto &decoded = work.catalog;
	decoded.revision = revision;
	size_t chest_count = 0;
	if (!work.decoded_resize(decoded.lockers, locker_count) ||
	    !work.decoded_resize(decoded.access, access_count))
		return false;
	for (auto &locker : decoded.lockers)
	{
		uint32_t count = 0;
		if (!payload.number(&locker.locker_id) ||
		    !payload.string(&locker.locker_name, locker_name_maximum) ||
		    !payload.number(&locker.owner_pid) || !payload.number(&locker.owner_assoc_id))
			return false;
		if (version == catalog_version)
		{
			uint8_t has_account_owner = 0;
			auto &owner = work.account;
			if (!payload.number(&has_account_owner) || has_account_owner > 1 ||
			    (has_account_owner &&
			     (!payload.string(&owner.account_name, account_name_maximum) ||
			      !payload.number(&owner.racewar_side))))
				return false;
			if (has_account_owner)
			{
				// Optional/contained string move is nonallocating; the already
				// counted string allocation changes owner exactly once.
				if (!work.admit(locker_bounded_string_frames +
						locker_bounded_nontrivial_frames +
						8 * sizeof(void *) + sizeof(bool)))
					return false;
				locker.account_owner = std::move(owner);
			}
		}
		if (!payload.number(&locker.racewar) || !payload.number(&locker.race) ||
		    !payload.number(&locker.revision) || !payload.number(&count) ||
		    count > chest_maximum - chest_count)
			return false;
		chest_count += count;
		if (!work.decoded_resize(locker.chests, count))
			return false;
		for (auto &chest : locker.chests)
		{
			uint8_t is_public = 0;
			if (!payload.number(&chest.chest_id) ||
			    !payload.string(&chest.chest_name, chest_name_maximum) ||
			    !payload.string(&chest.password_hash, password_hash_maximum) ||
			    !payload.number(&is_public) || is_public > 1 ||
			    !payload.string(&chest.sort_config, sort_config_maximum) ||
			    !payload.number(&chest.revision) || !payload.items(&chest.items))
				return false;
			chest.is_public = is_public != 0;
		}
	}
	for (auto &entry : decoded.access)
		if (!payload.string(&entry.owner_name, access_name_maximum) ||
		    !payload.string(&entry.visitor_name, access_name_maximum) ||
		    !payload.number(&entry.revision))
			return false;
	return payload.offset == payload.size && locker_bounded_valid_catalog(work);
}

bool locker_bounded_retained_heap(const std::vector<flatfile_locker_record> &lockers,
				  size_t *out) noexcept
{
	size_t bytes = 0;
	if (!locker_bounded_vector_heap(lockers, bytes))
		return false;
	for (const auto &locker : lockers)
	{
		if (!locker_bounded_string_heap(locker.locker_name, bytes) ||
		    !locker_bounded_vector_heap(locker.chests, bytes) ||
		    (locker.account_owner &&
		     !locker_bounded_string_heap(locker.account_owner->account_name, bytes)))
			return false;
		for (const auto &chest : locker.chests)
		{
			if (!locker_bounded_string_heap(chest.chest_name, bytes) ||
			    !locker_bounded_string_heap(chest.password_hash, bytes) ||
			    !locker_bounded_string_heap(chest.sort_config, bytes) ||
			    !locker_bounded_vector_heap(chest.items, bytes))
				return false;
			for (const auto &item : chest.items)
			{
				size_t heap = 0;
				if (!player_item_snapshot_current_heap_bytes(item, &heap) ||
				    !locker_bounded_add(bytes, heap))
					return false;
			}
		}
	}
	*out = bytes;
	return true;
}
} // namespace

flatfile_locker_result flatfile_locker_recovery_list_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_locker_record> *lockers, flatfile_scratch_reserve_fn reserve,
	void *context, size_t outer_live, size_t *retained_output_heap) noexcept
{
	if (!lock.matches(root) || !lockers)
		return flatfile_locker_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	errno = ENOTSUP;
	return flatfile_locker_result::io_error;
#else
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::string) != 32 ||
	    sizeof(std::vector<uint8_t>) != 24)
	{
		errno = ENOTSUP;
		return flatfile_locker_result::io_error;
	}
	constexpr size_t own = sizeof(locker_bounded_frame) + 6 * sizeof(void *) +
			       5 * sizeof(size_t) + sizeof(flatfile_read_result) + sizeof(bool);
	constexpr size_t tail =
		locker_bounded_move_frames + locker_bounded_nontrivial_frames +
		locker_bounded_string_frames + 14 * sizeof(void *) + 6 * sizeof(size_t) +
		5 * sizeof(bool) +
		// Actual complete row-current-heap observer's source carriers, captured
		// from the maintained codec; no presumed spare bytes from another path.
		13 * sizeof(void *) + 8 * sizeof(size_t) + 6 * sizeof(bool) +
		8 * (sizeof(void *) + sizeof(size_t)) + 8 * (2 * sizeof(void *));
	size_t initial = outer_live;
	if (!reserve || !locker_bounded_add(initial, sizeof(locker_bounded_workspace)) ||
	    !locker_bounded_add(initial, own) || !locker_bounded_add(initial, tail) ||
	    !reserve(initial, context))
	{
		errno = ENOBUFS;
		return flatfile_locker_result::io_error;
	}
	try
	{
		locker_bounded_workspace work{ {}, {}, {}, {}, {}, {},	    {},	     {},
					       {}, {}, {}, {}, {}, reserve, context, outer_live };
		locker_bounded_frame frame(work, own);
		// Fresh string::reserve has a genuine minimum doubled SSO request.
		size_t length = root.size();
		if (!locker_bounded_add(length, 8))
		{
			work.refuse();
			return flatfile_locker_result::io_error;
		}
		size_t request = locker_bounded_string_frames;
		if (length > 15 && !locker_bounded_add(request, std::max(length, size_t(30)) + 1))
		{
			work.refuse();
			return flatfile_locker_result::io_error;
		}
		if (!work.admit(request))
			return flatfile_locker_result::io_error;
		work.directory.reserve(length);
		work.directory.assign(root);
		work.directory.append("/domains");
		work.filename.assign(catalog_filename);
		// Secure original SAME-FD reader owns its stat/candidate vector. Here
		// admit its real arguments/scalars, directory-helper/read_all/libc
		// source frames and vector/string STL closures before invocation.
		constexpr size_t file_frames =
			7 * sizeof(void *) + 10 * sizeof(int) + 9 * sizeof(size_t) +
			6 * sizeof(bool) + 3 * sizeof(void *) + sizeof(std::ptrdiff_t) +
			locker_bounded_vector_frames + locker_bounded_nontrivial_frames;
		size_t outer = 0;
		if (!work.current(&outer) || !locker_bounded_add(outer, file_frames) ||
		    !work.admit(file_frames))
			return flatfile_locker_result::io_error;
		const auto loaded = flatfile_read_bounded(work.directory, work.filename,
							  catalog_maximum_bytes, &work.file,
							  locker_bounded_workspace::relay, &work,
							  outer);
		if (work.rejected)
		{
			errno = ENOBUFS;
			return flatfile_locker_result::io_error;
		}
		if (loaded == flatfile_read_result::not_found)
			return flatfile_locker_result::not_found;
		if (loaded == flatfile_read_result::io_error)
			return flatfile_locker_result::io_error;
		if (loaded != flatfile_read_result::ok || !locker_bounded_decode_catalog(work))
		{
			if (work.rejected)
			{
				errno = ENOBUFS;
				return flatfile_locker_result::io_error;
			}
			if (work.allocation_failed)
			{
				errno = ENOMEM;
				return flatfile_locker_result::io_error;
			}
			return flatfile_locker_result::invalid;
		}
		if (!work.admit(tail) ||
		    !locker_bounded_retained_heap(work.catalog.lockers, &work.transferred_heap))
		{
			work.refuse();
			return flatfile_locker_result::io_error;
		}
		static_assert(
			std::is_nothrow_move_assignable_v<std::vector<flatfile_locker_record>>);
		// Complete original borrowed-lock reader checks the lock at entry.
		// Whole validated output and scalar commit after final admission;
		// no callback or fallible operation follows this transfer.
		*lockers = std::move(work.catalog.lockers);
		if (retained_output_heap)
			*retained_output_heap = work.transferred_heap;
		return flatfile_locker_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_locker_result::io_error;
	}
	catch (...)
	{
		errno = EOVERFLOW;
		return flatfile_locker_result::io_error;
	}
#endif
}
