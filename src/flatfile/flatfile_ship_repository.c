#include "flatfile/flatfile_ship_repository.h"

#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <initializer_list>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>
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
constexpr size_t legacy_index_maximum_bytes = 5 * 1024 * 1024;
constexpr size_t legacy_ship_maximum_bytes = 64 * 1024;
constexpr size_t legacy_ship_slot_count = 16;
constexpr size_t legacy_ship_line_count = 14 + legacy_ship_slot_count * 3;
constexpr int32_t legacy_ship_class_count = 13;
constexpr int32_t legacy_weapon_count = 12;
constexpr int32_t legacy_equipment_count = 3;
constexpr int32_t legacy_port_count = 10;

enum class legacy_read_result
{
	ok,
	not_found,
	invalid,
	io_error
};

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

void legacy_error(std::string *error, const std::string &message)
{
	if (error)
		*error = message;
}

legacy_read_result open_legacy_directory(const std::string &directory, int *directory_fd,
					 std::string *error)
{
	if (!directory_fd || directory.empty())
		return legacy_read_result::invalid;
	*directory_fd = open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (*directory_fd < 0)
	{
		if (errno == ENOENT)
			return legacy_read_result::not_found;
		legacy_error(error, errno == ELOOP ?
					    "legacy ship directory is a symlink" :
					    std::string("could not open legacy ship directory: ") +
						    strerror(errno));
		return errno == ELOOP ? legacy_read_result::invalid : legacy_read_result::io_error;
	}
	struct stat info = {};
	if (fstat(*directory_fd, &info) < 0)
	{
		legacy_error(error, std::string("could not inspect legacy ship directory: ") +
					    strerror(errno));
		close(*directory_fd);
		*directory_fd = -1;
		return legacy_read_result::io_error;
	}
	if (!S_ISDIR(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 0022))
	{
		legacy_error(error, "legacy ship directory has unsafe metadata");
		close(*directory_fd);
		*directory_fd = -1;
		return legacy_read_result::invalid;
	}
	return legacy_read_result::ok;
}

legacy_read_result read_legacy_file(int directory_fd, const std::string &name, size_t maximum_size,
				    std::vector<uint8_t> *bytes, std::string *error)
{
	if (directory_fd < 0 || !bytes || name.empty() || name == "." || name == ".." ||
	    name.find('/') != std::string::npos)
		return legacy_read_result::invalid;
	bytes->clear();
	const int file_fd = openat(directory_fd, name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (file_fd < 0)
	{
		if (errno == ENOENT)
			return legacy_read_result::not_found;
		legacy_error(error, errno == ELOOP ?
					    "legacy ship file is a symlink" :
					    std::string("could not open legacy ship file: ") +
						    strerror(errno));
		return errno == ELOOP ? legacy_read_result::invalid : legacy_read_result::io_error;
	}
	struct stat info = {};
	if (fstat(file_fd, &info) < 0)
	{
		legacy_error(error,
			     std::string("could not inspect legacy ship file: ") + strerror(errno));
		close(file_fd);
		return legacy_read_result::io_error;
	}
	if (!S_ISREG(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 0022) ||
	    info.st_size < 0 || static_cast<uintmax_t>(info.st_size) > maximum_size)
	{
		legacy_error(error, "legacy ship file has unsafe metadata or size");
		close(file_fd);
		return legacy_read_result::invalid;
	}
	try
	{
		bytes->resize(static_cast<size_t>(info.st_size));
	}
	catch (const std::bad_alloc &)
	{
		close(file_fd);
		return legacy_read_result::io_error;
	}
	size_t offset = 0;
	while (offset < bytes->size())
	{
		const ssize_t count = read(file_fd, bytes->data() + offset, bytes->size() - offset);
		if (count < 0)
		{
			if (errno == EINTR)
				continue;
			legacy_error(error, std::string("could not read legacy ship file: ") +
						    strerror(errno));
			bytes->clear();
			close(file_fd);
			return legacy_read_result::io_error;
		}
		if (!count)
		{
			legacy_error(error, "legacy ship file changed while being read");
			bytes->clear();
			close(file_fd);
			return legacy_read_result::invalid;
		}
		offset += static_cast<size_t>(count);
	}
	if (close(file_fd) < 0)
	{
		legacy_error(error,
			     std::string("could not close legacy ship file: ") + strerror(errno));
		bytes->clear();
		return legacy_read_result::io_error;
	}
	return legacy_read_result::ok;
}

bool legacy_lines(const std::vector<uint8_t> &bytes, size_t maximum_lines, size_t maximum_line,
		  std::vector<std::string> *lines)
{
	if (!lines)
		return false;
	lines->clear();
	size_t start = 0;
	try
	{
		for (size_t offset = 0; offset <= bytes.size(); ++offset)
		{
			if (offset != bytes.size() && bytes[offset] != '\n')
				continue;
			if (offset == bytes.size() && start == offset)
				break;
			size_t end = offset;
			if (end > start && bytes[end - 1] == '\r')
				--end;
			if (end - start > maximum_line || lines->size() >= maximum_lines)
				return false;
			for (size_t index = start; index < end; ++index)
				if (bytes[index] < 0x20 || bytes[index] > 0x7e)
					return false;
			lines->emplace_back(reinterpret_cast<const char *>(bytes.data() + start),
					    end - start);
			start = offset + 1;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool legacy_numbers(const std::string &line, std::initializer_list<int32_t *> values)
{
	const char *cursor = line.data();
	const char *end = cursor + line.size();
	for (int32_t *value : values)
	{
		while (cursor != end && *cursor == ' ')
			++cursor;
		if (!value || cursor == end)
			return false;
		auto parsed = std::from_chars(cursor, end, *value);
		if (parsed.ec != std::errc() || parsed.ptr == cursor)
			return false;
		cursor = parsed.ptr;
	}
	while (cursor != end && *cursor == ' ')
		++cursor;
	return cursor == end;
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

bool valid_ship_payload(const flatfile_ship_record &ship)
{
	if (!ship.owner_pid || !valid_text(ship.owner_name, owner_name_maximum, true, false) ||
	    ship.owner_name != canonical_name(ship.owner_name) ||
	    !valid_text(ship.ship_name, ship_name_maximum, false, true) ||
	    ship.slots.size() > slot_maximum ||
	    !std::is_sorted(ship.slots.begin(), ship.slots.end(), slot_less))
		return false;
	for (size_t index = 1; index < ship.slots.size(); ++index)
		if (ship.slots[index - 1].slot_index == ship.slots[index].slot_index)
			return false;
	return true;
}

bool valid_legacy_slot(const flatfile_ship_slot_record &slot)
{
	if (slot.position < -1 || slot.position > 5)
		return false;
	switch (slot.slot_type)
	{
	case 0:
		return slot.item_index == -1;
	case 1:
	case 5:
		return slot.item_index >= 0 && slot.item_index < legacy_weapon_count;
	case 2:
	case 3:
		return slot.item_index >= 0 && slot.item_index < legacy_port_count;
	case 4:
		return slot.item_index >= 0 && slot.item_index < legacy_equipment_count;
	default:
		return false;
	}
}

bool parse_legacy_index(const std::vector<uint8_t> &bytes, std::vector<std::string> *owners,
			std::string *error)
{
	std::vector<std::string> lines;
	if (!owners || !legacy_lines(bytes, ship_maximum + 1, owner_name_maximum + 1, &lines) ||
	    lines.empty())
	{
		legacy_error(error, "legacy ship index is malformed");
		return false;
	}
	owners->clear();
	std::unordered_set<std::string> seen;
	try
	{
		seen.reserve(lines.size());
		for (size_t index = 0; index < lines.size(); ++index)
		{
			const std::string &line = lines[index];
			if (line.size() < 2 || line.back() != '~')
			{
				legacy_error(error, "legacy ship index entry is malformed");
				return false;
			}
			const std::string owner = line.substr(0, line.size() - 1);
			if (owner == "$")
			{
				if (index + 1 != lines.size())
				{
					legacy_error(
						error,
						"legacy ship index has data after its sentinel");
					return false;
				}
				return true;
			}
			if (!valid_text(owner, owner_name_maximum, true, false) ||
			    !seen.insert(canonical_name(owner)).second)
			{
				legacy_error(error,
					     "legacy ship index has an invalid or duplicate owner");
				return false;
			}
			owners->push_back(owner);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	legacy_error(error, "legacy ship index has no sentinel");
	return false;
}

bool parse_legacy_ship(const std::vector<uint8_t> &bytes, const std::string &indexed_owner,
		       uint32_t ship_id, uint32_t owner_pid, flatfile_ship_record *ship,
		       std::string *error)
{
	std::vector<std::string> lines;
	if (!ship || !ship_id || !owner_pid ||
	    !legacy_lines(bytes, legacy_ship_line_count, ship_name_maximum, &lines) ||
	    lines.size() != legacy_ship_line_count || lines[0] != "version:3")
	{
		legacy_error(error, "legacy ship file is not a complete version-3 record");
		return false;
	}
	int32_t ship_class = 0;
	if (!legacy_numbers(lines[1], { &ship_class }) || ship_class < 0 ||
	    ship_class >= legacy_ship_class_count ||
	    !valid_text(lines[2], owner_name_maximum, true, false) ||
	    canonical_name(lines[2]) != canonical_name(indexed_owner) ||
	    !valid_text(lines[3], ship_name_maximum, false, true))
	{
		legacy_error(error, "legacy ship identity fields are invalid");
		return false;
	}

	flatfile_ship_record parsed = {};
	parsed.ship_id = ship_id;
	parsed.owner_pid = owner_pid;
	parsed.owner_name = canonical_name(indexed_owner);
	parsed.ship_name = lines[3];
	parsed.ship_class = static_cast<uint8_t>(ship_class);
	if (!legacy_numbers(lines[4], { &parsed.frags }) ||
	    !legacy_numbers(lines[5], { &parsed.anchor_room, &parsed.time_played }))
	{
		legacy_error(error, "legacy ship scalar fields are invalid");
		return false;
	}
	size_t line = 6;
	for (size_t side = 0; side < parsed.armor.size(); ++side, ++line)
		if (!legacy_numbers(lines[line], { &parsed.armor[side], &parsed.internal[side] }))
		{
			legacy_error(error, "legacy ship armor fields are invalid");
			return false;
		}
	if (!legacy_numbers(lines[line++], { &parsed.mainsail }) ||
	    !legacy_numbers(lines[line++], { &parsed.crew.crew_index }))
	{
		legacy_error(error, "legacy ship sail or crew fields are invalid");
		return false;
	}
	int32_t ignored[7] = {};
	if (!legacy_numbers(lines[line++],
			    { &parsed.crew.sail_skill_milli, &parsed.crew.guns_skill_milli,
			      &parsed.crew.repair_skill_milli, &ignored[0], &ignored[1],
			      &ignored[2] }) ||
	    !legacy_numbers(lines[line++],
			    { &parsed.crew.sail_chief, &parsed.crew.guns_chief,
			      &parsed.crew.repair_chief, &ignored[0], &ignored[1], &ignored[2],
			      &ignored[3], &ignored[4], &ignored[5], &ignored[6] }))
	{
		legacy_error(error, "legacy ship crew fields are invalid");
		return false;
	}
	try
	{
		parsed.slots.resize(legacy_ship_slot_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (size_t index = 0; index < parsed.slots.size(); ++index)
	{
		auto &slot = parsed.slots[index];
		slot.slot_index = static_cast<uint8_t>(index);
		if (!legacy_numbers(lines[line++], { &slot.slot_type, &slot.item_index }) ||
		    !legacy_numbers(lines[line++], { &slot.position, &slot.timer }) ||
		    !legacy_numbers(lines[line++],
				    { &slot.values[0], &slot.values[1], &slot.values[2],
				      &slot.values[3], &slot.values[4] }) ||
		    !valid_legacy_slot(slot))
		{
			legacy_error(error, "legacy ship slot fields are invalid");
			return false;
		}
		if (slot.slot_type == 1)
		{
			if (slot.timer < 0)
				slot.timer = 0;
			slot.values[3] = -1;
			slot.values[4] = -1;
		}
		else if (slot.slot_type == 2 || slot.slot_type == 3)
		{
			slot.values[2] = -1;
			slot.values[3] = -1;
			slot.values[4] = -1;
		}
	}
	parsed.revision = 1;
	if (line != lines.size() || !valid_ship_payload(parsed))
	{
		legacy_error(error, "legacy ship record is invalid");
		return false;
	}
	*ship = std::move(parsed);
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

flatfile_ship_result publish(const std::string &root, ship_catalog *catalog, std::string *error)
{
	if (!catalog || catalog->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_ship_result::invalid;
	++catalog->revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(*catalog, &encoded))
		return flatfile_ship_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error) ?
		       flatfile_ship_result::ok :
		       flatfile_ship_result::io_error;
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

flatfile_ship_result flatfile_ship_upsert(const std::string &root, flatfile_ship_record *ship,
					  std::string *error)
{
	if (root.empty() || !ship)
		return flatfile_ship_result::invalid;
	flatfile_ship_record candidate;
	try
	{
		candidate = *ship;
		candidate.owner_name = canonical_name(candidate.owner_name);
		std::sort(candidate.slots.begin(), candidate.slots.end(), slot_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_ship_result::io_error;
	}
	if (!valid_ship_payload(candidate))
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

	auto stored = candidate.ship_id ?
			      std::lower_bound(catalog.ships.begin(), catalog.ships.end(),
					       candidate.ship_id, [](const auto &entry, uint32_t id)
					       { return entry.ship_id < id; }) :
			      catalog.ships.end();
	if (candidate.ship_id &&
	    (stored == catalog.ships.end() || stored->ship_id != candidate.ship_id))
		return flatfile_ship_result::conflict;
	for (auto existing = catalog.ships.begin(); existing != catalog.ships.end(); ++existing)
	{
		if (existing == stored)
			continue;
		if (existing->owner_pid == candidate.owner_pid ||
		    existing->owner_name == candidate.owner_name)
			return flatfile_ship_result::conflict;
	}

	if (stored == catalog.ships.end())
	{
		if (catalog.ships.size() >= ship_maximum)
			return flatfile_ship_result::invalid;
		const uint32_t last_id = catalog.ships.empty() ? 0 : catalog.ships.back().ship_id;
		if (last_id >= static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
			return flatfile_ship_result::invalid;
		candidate.ship_id = last_id + 1;
		candidate.revision = 1;
		try
		{
			catalog.ships.push_back(candidate);
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_ship_result::io_error;
		}
	}
	else
	{
		if (stored->revision == std::numeric_limits<uint64_t>::max())
			return flatfile_ship_result::invalid;
		candidate.revision = stored->revision + 1;
		try
		{
			*stored = candidate;
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_ship_result::io_error;
		}
	}

	const auto result = publish(root, &catalog, error);
	if (result == flatfile_ship_result::ok)
	{
		ship->ship_id = candidate.ship_id;
		ship->revision = candidate.revision;
	}
	return result;
}

flatfile_ship_result flatfile_ship_remove(const std::string &root, uint32_t ship_id,
					  const std::string &expected_owner, std::string *error)
{
	if (root.empty() || !ship_id ||
	    !valid_text(expected_owner, owner_name_maximum, true, false))
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
	auto stored = std::lower_bound(catalog.ships.begin(), catalog.ships.end(), ship_id,
				       [](const auto &entry, uint32_t id)
				       { return entry.ship_id < id; });
	if (stored == catalog.ships.end() || stored->ship_id != ship_id)
		return flatfile_ship_result::unchanged;
	if (stored->owner_name != canonical_name(expected_owner))
		return flatfile_ship_result::conflict;
	catalog.ships.erase(stored);
	return publish(root, &catalog, error);
}

flatfile_ship_result flatfile_ship_import_legacy(const std::string &root,
						 const std::string &legacy_directory,
						 flatfile_ship_owner_pid_resolver resolve_owner,
						 std::string *error)
{
	if (root.empty() || legacy_directory.empty() || !resolve_owner)
		return flatfile_ship_result::invalid;
	int directory_fd = -1;
	const auto opened = open_legacy_directory(legacy_directory, &directory_fd, error);
	if (opened == legacy_read_result::not_found)
		return flatfile_ship_result::not_found;
	if (opened != legacy_read_result::ok)
		return opened == legacy_read_result::io_error ? flatfile_ship_result::io_error :
								flatfile_ship_result::invalid;

	std::vector<uint8_t> bytes;
	const auto index_read = read_legacy_file(directory_fd, "ship_index",
						 legacy_index_maximum_bytes, &bytes, error);
	if (index_read != legacy_read_result::ok)
	{
		close(directory_fd);
		if (index_read == legacy_read_result::not_found)
			return flatfile_ship_result::not_found;
		return index_read == legacy_read_result::io_error ? flatfile_ship_result::io_error :
								    flatfile_ship_result::invalid;
	}
	std::vector<std::string> owners;
	if (!parse_legacy_index(bytes, &owners, error))
	{
		close(directory_fd);
		return flatfile_ship_result::invalid;
	}
	std::vector<flatfile_ship_record> ships;
	try
	{
		ships.reserve(owners.size());
	}
	catch (const std::bad_alloc &)
	{
		close(directory_fd);
		return flatfile_ship_result::io_error;
	}
	for (size_t index = 0; index < owners.size(); ++index)
	{
		uint32_t owner_pid = 0;
		if (!resolve_owner(owners[index].c_str(), &owner_pid, error) || !owner_pid)
		{
			if (error && error->empty())
				*error = "legacy ship owner identity is unavailable";
			close(directory_fd);
			return flatfile_ship_result::invalid;
		}
		const auto ship_read = read_legacy_file(directory_fd, owners[index],
							legacy_ship_maximum_bytes, &bytes, error);
		if (ship_read != legacy_read_result::ok)
		{
			if (ship_read == legacy_read_result::not_found)
				legacy_error(error,
					     "legacy ship index references a missing owner file");
			close(directory_fd);
			return ship_read == legacy_read_result::io_error ?
				       flatfile_ship_result::io_error :
				       flatfile_ship_result::invalid;
		}
		flatfile_ship_record ship;
		if (!parse_legacy_ship(bytes, owners[index], static_cast<uint32_t>(index + 1),
				       owner_pid, &ship, error))
		{
			close(directory_fd);
			return flatfile_ship_result::invalid;
		}
		try
		{
			ships.push_back(std::move(ship));
		}
		catch (const std::bad_alloc &)
		{
			close(directory_fd);
			return flatfile_ship_result::io_error;
		}
	}
	if (close(directory_fd) < 0)
	{
		legacy_error(error, std::string("could not close legacy ship directory: ") +
					    strerror(errno));
		return flatfile_ship_result::io_error;
	}
	return flatfile_ship_establish(root, ships, error);
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
