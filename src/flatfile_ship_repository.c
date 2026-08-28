#include "flatfile_ship_repository.h"

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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'S', 'H', 'I', 'P', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 64 * 1024 * 1024;
constexpr size_t ship_maximum = 65536;
constexpr size_t slot_maximum = 256;
constexpr size_t owner_name_maximum = 64;
constexpr size_t ship_name_maximum = 128;
constexpr const char *catalog_filename = "ship_catalog";

struct ship_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_ship_record> ships;
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

bool ship_less(const flatfile_ship_record &left, const flatfile_ship_record &right)
{
	return left.ship_id < right.ship_id;
}

bool slot_less(const flatfile_ship_slot_record &left, const flatfile_ship_slot_record &right)
{
	return left.slot_index < right.slot_index;
}

bool valid_catalog(const ship_catalog &catalog)
{
	if (!catalog.revision || catalog.ships.size() > ship_maximum ||
	    !std::is_sorted(catalog.ships.begin(), catalog.ships.end(), ship_less))
		return false;
	std::unordered_set<uint32_t> owner_pids;
	std::unordered_set<std::string> owner_names;
	try
	{
		owner_pids.reserve(catalog.ships.size());
		owner_names.reserve(catalog.ships.size());
		for (size_t ship_index = 0; ship_index < catalog.ships.size(); ++ship_index)
		{
			const auto &ship = catalog.ships[ship_index];
			if (!ship.ship_id || !ship.owner_pid || !ship.revision ||
			    !valid_text(ship.owner_name, owner_name_maximum, true, false) ||
			    ship.owner_name != canonical_name(ship.owner_name) ||
			    !valid_text(ship.ship_name, ship_name_maximum, false, true) ||
			    (ship_index && catalog.ships[ship_index - 1].ship_id == ship.ship_id) ||
			    !owner_pids.insert(ship.owner_pid).second ||
			    !owner_names.insert(ship.owner_name).second ||
			    ship.slots.size() > slot_maximum ||
			    !std::is_sorted(ship.slots.begin(), ship.slots.end(), slot_less))
				return false;
			for (size_t slot_index = 1; slot_index < ship.slots.size(); ++slot_index)
				if (ship.slots[slot_index - 1].slot_index ==
				    ship.slots[slot_index].slot_index)
					return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

void encode_ship(encoder &out, const flatfile_ship_record &ship)
{
	out.number(ship.ship_id);
	out.number(ship.owner_pid);
	out.text(ship.owner_name, owner_name_maximum);
	out.text(ship.ship_name, ship_name_maximum);
	out.number(ship.ship_class);
	out.number(ship.frags);
	out.number(ship.anchor_room);
	out.number(ship.time_played);
	out.number(ship.mainsail);
	out.number(ship.race);
	out.number(ship.money);
	out.number(ship.flags);
	for (int32_t value : ship.armor)
		out.number(value);
	for (int32_t value : ship.internal)
		out.number(value);
	out.number(ship.crew.crew_index);
	out.number(ship.crew.sail_skill_milli);
	out.number(ship.crew.guns_skill_milli);
	out.number(ship.crew.repair_skill_milli);
	out.number(ship.crew.sail_chief);
	out.number(ship.crew.guns_chief);
	out.number(ship.crew.repair_chief);
	out.number<uint32_t>(ship.slots.size());
	for (const auto &slot : ship.slots)
	{
		out.number(slot.slot_index);
		out.number(slot.slot_type);
		out.number(slot.item_index);
		out.number(slot.position);
		out.number(slot.timer);
		for (int32_t value : slot.values)
			out.number(value);
	}
	out.number(ship.revision);
}

bool decode_ship(decoder &in, flatfile_ship_record *ship)
{
	if (!ship || !in.number(&ship->ship_id) || !in.number(&ship->owner_pid) ||
	    !in.text(&ship->owner_name, owner_name_maximum) ||
	    !in.text(&ship->ship_name, ship_name_maximum) || !in.number(&ship->ship_class) ||
	    !in.number(&ship->frags) || !in.number(&ship->anchor_room) ||
	    !in.number(&ship->time_played) || !in.number(&ship->mainsail) ||
	    !in.number(&ship->race) || !in.number(&ship->money) || !in.number(&ship->flags))
		return false;
	for (int32_t &value : ship->armor)
		if (!in.number(&value))
			return false;
	for (int32_t &value : ship->internal)
		if (!in.number(&value))
			return false;
	if (!in.number(&ship->crew.crew_index) || !in.number(&ship->crew.sail_skill_milli) ||
	    !in.number(&ship->crew.guns_skill_milli) ||
	    !in.number(&ship->crew.repair_skill_milli) || !in.number(&ship->crew.sail_chief) ||
	    !in.number(&ship->crew.guns_chief) || !in.number(&ship->crew.repair_chief))
		return false;
	uint32_t slot_count = 0;
	if (!in.number(&slot_count) || slot_count > slot_maximum)
		return false;
	try
	{
		ship->slots.resize(slot_count);
		for (auto &slot : ship->slots)
		{
			if (!in.number(&slot.slot_index) || !in.number(&slot.slot_type) ||
			    !in.number(&slot.item_index) || !in.number(&slot.position) ||
			    !in.number(&slot.timer))
				return false;
			for (int32_t &value : slot.values)
				if (!in.number(&value))
					return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return in.number(&ship->revision);
}

bool encode_catalog(const ship_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.ships.size());
	for (const auto &ship : catalog.ships)
		encode_ship(payload, ship);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, ship_catalog *catalog)
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
	uint32_t count = 0;
	if (!payload.number(&count) || count > ship_maximum)
		return false;
	ship_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.ships.resize(count);
		for (auto &ship : decoded.ships)
			if (!decode_ship(payload, &ship))
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

flatfile_ship_result recover(const std::string &root, const flatfile_authority_lock &lock,
			     std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_ship_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_ship_result::io_error :
		       flatfile_ship_result::invalid;
}

flatfile_ship_result load_catalog(const std::string &root, ship_catalog *catalog,
				  std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_ship_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_ship_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "ship catalog is corrupt";
		return flatfile_ship_result::invalid;
	}
	return flatfile_ship_result::ok;
}

bool catalog_equal(ship_catalog left, ship_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}
} // namespace

flatfile_ship_result flatfile_ship_establish(const std::string &root,
					     const std::vector<flatfile_ship_record> &ships,
					     std::string *error)
{
	if (root.empty())
		return flatfile_ship_result::invalid;
	ship_catalog candidate;
	try
	{
		candidate.ships = ships;
		for (auto &ship : candidate.ships)
		{
			ship.owner_name = canonical_name(ship.owner_name);
			std::sort(ship.slots.begin(), ship.slots.end(), slot_less);
		}
		std::sort(candidate.ships.begin(), candidate.ships.end(), ship_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_ship_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_ship_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_ship_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_ship_result::ok)
		return recovered;
	ship_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_ship_result::ok)
		return catalog_equal(existing, candidate) ? flatfile_ship_result::already_exists :
							    flatfile_ship_result::invalid;
	if (loaded != flatfile_ship_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_ship_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_ship_result::io_error;
	return flatfile_ship_result::ok;
}

flatfile_ship_result flatfile_ship_list(const std::string &root,
					std::vector<flatfile_ship_record> *ships,
					std::string *error)
{
	if (root.empty() || !ships)
		return flatfile_ship_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_ship_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_ship_result::ok)
		return recovered;
	ship_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_ship_result::ok)
		return loaded;
	*ships = std::move(catalog.ships);
	return flatfile_ship_result::ok;
}

flatfile_ship_result
flatfile_ship_prepare_player_remove(const std::string &root, const flatfile_authority_lock &lock,
				    uint32_t pid, const std::string &expected_name,
				    flatfile_authority_operation *operation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !pid || !operation ||
	    !valid_text(expected_name, owner_name_maximum, true, false))
		return flatfile_ship_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_ship_result::ok)
		return recovered;
	ship_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_ship_result::ok)
		return loaded;
	const std::string canonical_expected = canonical_name(expected_name);
	auto ship = std::find_if(catalog.ships.begin(), catalog.ships.end(),
				 [pid](const auto &entry) { return entry.owner_pid == pid; });
	if (ship == catalog.ships.end())
		return flatfile_ship_result::unchanged;
	if (ship->owner_name != canonical_expected ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_ship_result::conflict;
	catalog.ships.erase(ship);
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_ship_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(encoded);
	return flatfile_ship_result::ok;
}
