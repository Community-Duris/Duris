#include "flatfile_world_item_repository.h"

#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"
#include "player_snapshot_codec.h"

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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'W', 'R', 'L', 'D', 0 };
constexpr uint32_t catalog_version = 3;
constexpr uint32_t catalog_money_version = 2;
constexpr uint32_t catalog_legacy_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t corpse_maximum = 262144;
constexpr size_t saved_item_maximum = 262144;
constexpr size_t room_maximum = 262144;
constexpr size_t name_maximum = 255;
constexpr size_t key_maximum = 100;
constexpr size_t short_description_maximum = 512;
constexpr size_t description_maximum = 64 * 1024;
constexpr size_t keywords_maximum = 512;
constexpr const char *catalog_filename = "world_item_catalog";

struct world_item_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	std::vector<flatfile_room_item_record> rooms;
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
		if (!size)
			return;
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
		return true;
	}

	bool byte_vector(std::vector<uint8_t> *value, size_t maximum)
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

bool valid_printable(const std::string &value, size_t maximum, bool required)
{
	if ((required && value.empty()) || value.size() > maximum)
		return false;
	for (unsigned char character : value)
		if (character < 0x20 || character == 0x7f)
			return false;
	return true;
}

bool corpse_less(const flatfile_corpse_record &left, const flatfile_corpse_record &right)
{
	if (left.owner_pid != right.owner_pid)
		return left.owner_pid < right.owner_pid;
	return left.save_id < right.save_id;
}

bool saved_item_less(const flatfile_saved_world_item_record &left,
		     const flatfile_saved_world_item_record &right)
{
	return left.item_key < right.item_key;
}

bool room_less(const flatfile_room_item_record &left, const flatfile_room_item_record &right)
{
	return left.room_vnum < right.room_vnum;
}

bool valid_item_list(const std::vector<player_item_snapshot> &items, bool require_one_root,
		     std::unordered_set<uint64_t> *item_uids)
{
	std::vector<uint8_t> encoded;
	if (!item_uids ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok)
		return false;
	size_t roots = 0;
	for (const auto &item : items)
	{
		roots += item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ? 1 : 0;
		if (!item.object_uid || item.vnum <= 0 || item.equipment_slot != -1 ||
		    !item_uids->insert(item.object_uid).second)
			return false;
	}
	return !require_one_root || roots == 1;
}

bool valid_catalog(const world_item_catalog &catalog)
{
	if (!catalog.revision || catalog.corpses.size() > corpse_maximum ||
	    catalog.saved_items.size() > saved_item_maximum ||
	    catalog.rooms.size() > room_maximum ||
	    !std::is_sorted(catalog.corpses.begin(), catalog.corpses.end(), corpse_less) ||
	    !std::is_sorted(catalog.saved_items.begin(), catalog.saved_items.end(),
			    saved_item_less) ||
	    !std::is_sorted(catalog.rooms.begin(), catalog.rooms.end(), room_less))
		return false;
	std::unordered_set<std::string> owner_names;
	std::unordered_set<std::string> item_keys;
	std::unordered_set<uint64_t> item_uids;
	try
	{
		owner_names.reserve(catalog.corpses.size());
		item_keys.reserve(catalog.saved_items.size());
		for (size_t index = 0; index < catalog.corpses.size(); ++index)
		{
			const auto &corpse = catalog.corpses[index];
			const bool same_owner = index && catalog.corpses[index - 1].owner_pid ==
								 corpse.owner_pid;
			if (!corpse.owner_pid || !corpse.save_id || !corpse.revision ||
			    corpse.room_vnum < 0 ||
			    !std::all_of(corpse.money.begin(), corpse.money.end(),
					 [](int32_t value) { return value >= 0; }) ||
			    !valid_printable(corpse.owner_name, name_maximum, true) ||
			    corpse.owner_name != canonical_name(corpse.owner_name) ||
			    !valid_printable(corpse.short_description, short_description_maximum,
					     false) ||
			    !valid_printable(corpse.description, description_maximum, false) ||
			    !valid_printable(corpse.keywords, keywords_maximum, false) ||
			    (index && !corpse_less(catalog.corpses[index - 1], corpse)) ||
			    (same_owner &&
			     catalog.corpses[index - 1].owner_name != corpse.owner_name) ||
			    (!same_owner && !owner_names.insert(corpse.owner_name).second) ||
			    !valid_item_list(corpse.items, false, &item_uids))
				return false;
		}
		for (size_t index = 0; index < catalog.saved_items.size(); ++index)
		{
			const auto &saved = catalog.saved_items[index];
			if (!saved.revision || saved.room_vnum <= 0 ||
			    !valid_printable(saved.item_key, key_maximum, true) ||
			    saved.items.empty() ||
			    (index && !saved_item_less(catalog.saved_items[index - 1], saved)) ||
			    !item_keys.insert(saved.item_key).second ||
			    !valid_item_list(saved.items, true, &item_uids))
				return false;
		}
		for (size_t index = 0; index < catalog.rooms.size(); ++index)
		{
			const auto &room = catalog.rooms[index];
			if (room.room_vnum <= 0 || !room.revision ||
			    (index && !room_less(catalog.rooms[index - 1], room)) ||
			    !std::all_of(room.money.begin(), room.money.end(),
					 [](int32_t value) { return value >= 0; }) ||
			    !valid_item_list(room.items, false, &item_uids))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_items(encoder &out, const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> encoded;
	if (player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() > UINT32_MAX)
		return false;
	out.number<uint32_t>(encoded.size());
	out.raw(encoded.data(), encoded.size());
	return out.valid;
}

bool decode_items(decoder &in, std::vector<player_item_snapshot> *items)
{
	std::vector<uint8_t> encoded;
	return in.byte_vector(&encoded, PLAYER_SNAPSHOT_MAX_BYTES) &&
	       player_item_snapshot_list_decode(encoded.data(), encoded.size(), items) ==
		       player_snapshot_codec_result::ok;
}

bool encode_catalog(const world_item_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.corpses.size());
	payload.number<uint32_t>(catalog.saved_items.size());
	payload.number<uint32_t>(catalog.rooms.size());
	for (const auto &corpse : catalog.corpses)
	{
		payload.number(corpse.owner_pid);
		payload.text(corpse.owner_name, name_maximum);
		payload.number(corpse.save_id);
		payload.number(corpse.room_vnum);
		payload.text(corpse.short_description, short_description_maximum);
		payload.text(corpse.description, description_maximum);
		payload.text(corpse.keywords, keywords_maximum);
		payload.number(corpse.weight);
		for (int32_t value : corpse.values)
			payload.number(value);
		for (int32_t value : corpse.money)
			payload.number(value);
		payload.number(corpse.revision);
		if (!encode_items(payload, corpse.items))
			return false;
	}
	for (const auto &saved : catalog.saved_items)
	{
		payload.text(saved.item_key, key_maximum);
		payload.number(saved.room_vnum);
		payload.number(saved.revision);
		if (!encode_items(payload, saved.items))
			return false;
	}
	for (const auto &room : catalog.rooms)
	{
		payload.number(room.room_vnum);
		payload.number(room.revision);
		for (int32_t value : room.money)
			payload.number(value);
		if (!encode_items(payload, room.items))
			return false;
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

bool decode_catalog(const std::vector<uint8_t> &bytes, world_item_catalog *catalog)
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
	    (version != catalog_version && version != catalog_money_version &&
	     version != catalog_legacy_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t corpse_count = 0, saved_count = 0, room_count = 0;
	if (!payload.number(&corpse_count) || !payload.number(&saved_count) ||
	    (version >= catalog_version && !payload.number(&room_count)) ||
	    corpse_count > corpse_maximum || saved_count > saved_item_maximum ||
	    room_count > room_maximum)
		return false;
	world_item_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.corpses.resize(corpse_count);
		for (auto &corpse : decoded.corpses)
		{
			if (!payload.number(&corpse.owner_pid) ||
			    !payload.text(&corpse.owner_name, name_maximum) ||
			    !payload.number(&corpse.save_id) ||
			    !payload.number(&corpse.room_vnum) ||
			    !payload.text(&corpse.short_description, short_description_maximum) ||
			    !payload.text(&corpse.description, description_maximum) ||
			    !payload.text(&corpse.keywords, keywords_maximum) ||
			    !payload.number(&corpse.weight))
				return false;
			for (int32_t &value : corpse.values)
				if (!payload.number(&value))
					return false;
			if (version >= catalog_money_version)
				for (int32_t &value : corpse.money)
					if (!payload.number(&value))
						return false;
			if (!payload.number(&corpse.revision) ||
			    !decode_items(payload, &corpse.items))
				return false;
		}
		decoded.saved_items.resize(saved_count);
		for (auto &saved : decoded.saved_items)
			if (!payload.text(&saved.item_key, key_maximum) ||
			    !payload.number(&saved.room_vnum) || !payload.number(&saved.revision) ||
			    !decode_items(payload, &saved.items))
				return false;
		decoded.rooms.resize(room_count);
		for (auto &room : decoded.rooms)
		{
			if (!payload.number(&room.room_vnum) || !payload.number(&room.revision))
				return false;
			for (int32_t &value : room.money)
				if (!payload.number(&value))
					return false;
			if (!decode_items(payload, &room.items))
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

flatfile_world_item_result recover(const std::string &root, const flatfile_authority_lock &lock,
				   std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_world_item_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_world_item_result::io_error :
		       flatfile_world_item_result::invalid;
}

flatfile_world_item_result load_catalog(const std::string &root, world_item_catalog *catalog,
					std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_world_item_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_world_item_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "world item catalog is corrupt";
		return flatfile_world_item_result::invalid;
	}
	return flatfile_world_item_result::ok;
}

bool catalog_equal(world_item_catalog left, world_item_catalog right)
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
	    items.front().object_uid != payload.selected_item_uid)
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

bool canonicalize_detached_items(std::vector<player_item_snapshot> *items)
{
	if (!items)
		return false;
	for (auto &item : *items)
	{
		if (item.equipment_slot != 0 && item.equipment_slot != -1)
			return false;
		item.equipment_slot = -1;
	}
	return true;
}

bool room_transfer_deposit(const item_transfer_payload &payload)
{
	return payload.from_owner.type == item_owner_type::player &&
	       payload.to_owner.type == item_owner_type::room &&
	       ((payload.reason == item_transfer_reason::player_drop &&
		 !payload.target_parent_item_uid) ||
		(payload.reason == item_transfer_reason::player_put &&
		 payload.target_parent_item_uid));
}

bool room_transfer_withdraw(const item_transfer_payload &payload)
{
	return payload.from_owner.type == item_owner_type::room &&
	       payload.to_owner.type == item_owner_type::player &&
	       payload.reason == item_transfer_reason::player_get &&
	       !payload.target_parent_item_uid;
}

bool room_transfer_create(const item_transfer_payload &payload)
{
	return payload.from_owner.type == item_owner_type::system &&
	       payload.to_owner.type == item_owner_type::room &&
	       payload.reason == item_transfer_reason::creation && !payload.target_parent_item_uid;
}

bool room_transfer_destroy(const item_transfer_payload &payload)
{
	return payload.from_owner.type == item_owner_type::room &&
	       payload.to_owner.type == item_owner_type::destruction &&
	       payload.reason == item_transfer_reason::destruction &&
	       !payload.target_parent_item_uid;
}

bool room_transfer_reparent(const item_transfer_payload &payload)
{
	return item_owner_identity_equal(payload.from_owner, payload.to_owner) &&
	       payload.from_owner.type == item_owner_type::room &&
	       payload.reason == item_transfer_reason::operator_repair &&
	       !payload.target_parent_item_uid;
}

bool room_custody(const std::vector<player_item_snapshot> &items,
		  std::vector<flatfile_corpse_custody_item> *custody)
{
	if (!custody)
		return false;
	custody->clear();
	try
	{
		custody->reserve(items.size());
		for (size_t index = 0; index < items.size(); ++index)
		{
			const auto &item = items[index];
			uint64_t root_uid = item.object_uid;
			int32_t parent_index = item.parent_index;
			for (size_t depth = 0; parent_index != PLAYER_SNAPSHOT_NO_PARENT; ++depth)
			{
				if (depth >= items.size() || parent_index < 0 ||
				    static_cast<size_t>(parent_index) >= items.size())
					return false;
				const auto &parent = items[static_cast<size_t>(parent_index)];
				root_uid = parent.object_uid;
				parent_index = parent.parent_index;
			}
			custody->push_back({ item.object_uid, item.vnum, root_uid,
					     item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
						     0 :
						     items[static_cast<size_t>(item.parent_index)]
							     .object_uid });
		}
		std::sort(custody->begin(), custody->end(), [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

size_t room_item_index(const std::vector<player_item_snapshot> &items, uint64_t item_uid)
{
	const auto found = std::find_if(items.begin(), items.end(), [&](const auto &item)
					{ return item.object_uid == item_uid; });
	return found == items.end() ? items.size() : static_cast<size_t>(found - items.begin());
}

bool room_item_root_matches(const std::vector<player_item_snapshot> &items, size_t index,
			    uint64_t expected_root_uid)
{
	if (index >= items.size())
		return false;
	for (size_t depth = 0; depth <= items.size(); ++depth)
	{
		const auto &item = items[index];
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			return item.object_uid == expected_root_uid;
		if (item.parent_index < 0 || static_cast<size_t>(item.parent_index) >= items.size())
			return false;
		index = static_cast<size_t>(item.parent_index);
	}
	return false;
}

bool apply_room_weight_delta(std::vector<player_item_snapshot> *items, size_t index, int64_t delta)
{
	if (!items || index >= items->size())
		return false;
	for (size_t depth = 0; delta && depth <= items->size(); ++depth)
	{
		auto &item = (*items)[index];
		const int64_t previous = item.weight;
		const int64_t next = previous + delta;
		if (next < INT32_MIN || next > INT32_MAX)
			return false;
		item.weight = static_cast<int32_t>(next);
		delta = std::max<int64_t>(next, 0) - std::max<int64_t>(previous, 0);
		if (!delta || item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			return true;
		if (item.parent_index < 0 ||
		    static_cast<size_t>(item.parent_index) >= items->size())
			return false;
		index = static_cast<size_t>(item.parent_index);
	}
	return !delta;
}

void apply_corpse_metadata(flatfile_corpse_record *corpse, const item_corpse_metadata &metadata)
{
	corpse->owner_name = canonical_name(metadata.owner_name);
	corpse->room_vnum = metadata.room_vnum;
	corpse->short_description = metadata.short_description;
	corpse->description = metadata.description;
	corpse->keywords = metadata.keywords;
	corpse->weight = metadata.weight;
	corpse->values = metadata.values;
}

void apply_corpse_lifecycle(flatfile_corpse_record *corpse, const corpse_lifecycle_payload &payload)
{
	corpse->owner_pid = payload.owner_pid;
	corpse->owner_name = canonical_name(payload.owner_name);
	corpse->save_id = payload.save_id;
	corpse->room_vnum = payload.room_vnum;
	corpse->short_description = payload.short_description;
	corpse->description = payload.description;
	corpse->keywords = payload.keywords;
	corpse->weight = payload.weight;
	corpse->values = payload.values;
	corpse->money = payload.money;
}

bool valid_corpse_lifecycle(const corpse_lifecycle_payload &payload)
{
	if (!payload.owner_pid || payload.owner_pid > INT32_MAX || !payload.save_id ||
	    payload.save_id > INT32_MAX ||
	    !valid_printable(payload.owner_name, CORPSE_LIFECYCLE_OWNER_NAME_MAX_BYTES, true))
		return false;
	if (payload.action == corpse_lifecycle_action::remove)
		return payload.expected_corpse_revision && !payload.room_vnum && !payload.weight &&
		       std::all_of(payload.values.begin(), payload.values.end(),
				   [](int32_t value) { return value == 0; }) &&
		       std::all_of(payload.money.begin(), payload.money.end(),
				   [](int32_t value) { return value == 0; }) &&
		       payload.short_description.empty() && payload.description.empty() &&
		       payload.keywords.empty();
	return payload.action == corpse_lifecycle_action::upsert && payload.room_vnum >= 0 &&
	       payload.values[3] == static_cast<int32_t>(payload.owner_pid) &&
	       payload.values[5] >= 0 && payload.values[5] <= 4 &&
	       payload.values[6] == static_cast<int32_t>(payload.save_id) &&
	       std::all_of(payload.money.begin(), payload.money.end(),
			   [](int32_t value) { return value >= 0; }) &&
	       valid_printable(payload.short_description,
			       CORPSE_LIFECYCLE_SHORT_DESCRIPTION_MAX_BYTES, false) &&
	       valid_printable(payload.description, CORPSE_LIFECYCLE_DESCRIPTION_MAX_BYTES,
			       false) &&
	       valid_printable(payload.keywords, CORPSE_LIFECYCLE_KEYWORDS_MAX_BYTES, false);
}
} // namespace

flatfile_world_item_result flatfile_world_item_establish(
	const std::string &root, const std::vector<flatfile_corpse_record> &corpses,
	const std::vector<flatfile_saved_world_item_record> &saved_items, std::string *error)
{
	if (root.empty())
		return flatfile_world_item_result::invalid;
	world_item_catalog candidate;
	try
	{
		candidate.corpses = corpses;
		candidate.saved_items = saved_items;
		for (auto &corpse : candidate.corpses)
			corpse.owner_name = canonical_name(corpse.owner_name);
		std::sort(candidate.corpses.begin(), candidate.corpses.end(), corpse_less);
		std::sort(candidate.saved_items.begin(), candidate.saved_items.end(),
			  saved_item_less);
		std::sort(candidate.rooms.begin(), candidate.rooms.end(), room_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_world_item_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_world_item_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_world_item_result::ok)
		return recovered;
	world_item_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_world_item_result::ok)
		return catalog_equal(existing, candidate) ?
			       flatfile_world_item_result::already_exists :
			       flatfile_world_item_result::invalid;
	if (loaded != flatfile_world_item_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_world_item_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_world_item_result::io_error;
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result
flatfile_world_item_list_rooms(const std::string &root,
			       std::vector<flatfile_room_item_record> *rooms, std::string *error)
{
	if (root.empty() || !rooms)
		return flatfile_world_item_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_world_item_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_world_item_result::ok)
		return recovered;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	*rooms = std::move(catalog.rooms);
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result
flatfile_world_item_list(const std::string &root, std::vector<flatfile_corpse_record> *corpses,
			 std::vector<flatfile_saved_world_item_record> *saved_items,
			 std::string *error)
{
	if (root.empty() || !corpses || !saved_items)
		return flatfile_world_item_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_world_item_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_world_item_result::ok)
		return recovered;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	*corpses = std::move(catalog.corpses);
	*saved_items = std::move(catalog.saved_items);
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_world_item_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::string &expected_name, flatfile_world_item_player_removal *removal,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !pid || !removal ||
	    !valid_printable(expected_name, name_maximum, true))
		return flatfile_world_item_result::invalid;
	*removal = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_world_item_result::ok)
		return recovered;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	const std::string canonical_expected = canonical_name(expected_name);
	auto first = std::lower_bound(catalog.corpses.begin(), catalog.corpses.end(), pid,
				      [](const auto &corpse, uint32_t owner_pid)
				      { return corpse.owner_pid < owner_pid; });
	auto last = first;
	while (last != catalog.corpses.end() && last->owner_pid == pid)
		++last;
	if (first == last)
		return flatfile_world_item_result::unchanged;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_world_item_result::invalid;
	try
	{
		removal->custody.reserve(static_cast<size_t>(last - first));
		for (auto corpse = first; corpse != last; ++corpse)
		{
			if (corpse->owner_name != canonical_expected)
				return flatfile_world_item_result::conflict;
			flatfile_corpse_custody_owner expected;
			expected.owner = { item_owner_type::corpse,
					   item_corpse_owner_id(pid, corpse->save_id), 0 };
			expected.items.reserve(corpse->items.size());
			for (const auto &item : corpse->items)
				expected.items.push_back({ item.object_uid, item.vnum });
			std::sort(expected.items.begin(), expected.items.end(),
				  [](const auto &left, const auto &right)
				  { return left.item_uid < right.item_uid; });
			if (!expected.items.empty())
				removal->custody.push_back(std::move(expected));
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	catalog.corpses.erase(first, last);
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_world_item_result::invalid;
	removal->operation.store = flatfile_authority_store::domains;
	removal->operation.kind = flatfile_authority_operation_kind::write;
	removal->operation.filename = catalog_filename;
	removal->operation.bytes = std::move(encoded);
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_world_item_prepare_corpse_lifecycle(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_corpse_lifecycle_mutation *mutation,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !valid_corpse_lifecycle(payload))
		return flatfile_world_item_result::invalid;
	*mutation = {};
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	flatfile_corpse_record key = {};
	key.owner_pid = payload.owner_pid;
	key.save_id = payload.save_id;
	auto corpse =
		std::lower_bound(catalog.corpses.begin(), catalog.corpses.end(), key, corpse_less);
	const bool found = corpse != catalog.corpses.end() &&
			   corpse->owner_pid == payload.owner_pid &&
			   corpse->save_id == payload.save_id;
	const std::string canonical_owner = canonical_name(payload.owner_name);
	if (catalog.revision == UINT64_MAX)
		return flatfile_world_item_result::conflict;
	if (payload.action == corpse_lifecycle_action::upsert)
	{
		if ((!payload.expected_corpse_revision && found) ||
		    (payload.expected_corpse_revision && !found))
			return flatfile_world_item_result::conflict;
		if ((!found && std::any_of(catalog.corpses.begin(), catalog.corpses.end(),
					   [&](const auto &candidate) {
						   return candidate.owner_pid !=
								  payload.owner_pid &&
							  candidate.owner_name == canonical_owner;
					   })) ||
		    (found &&
		     (corpse->revision != payload.expected_corpse_revision ||
		      corpse->revision == UINT64_MAX || corpse->owner_name != canonical_owner ||
		      corpse->values[3] != payload.values[3] ||
		      corpse->values[5] != payload.values[5] ||
		      corpse->values[6] != payload.values[6])))
			return flatfile_world_item_result::conflict;
		if (!found && catalog.corpses.size() >= corpse_maximum)
			return flatfile_world_item_result::conflict;
		try
		{
			if (!found)
			{
				flatfile_corpse_record created = {};
				apply_corpse_lifecycle(&created, payload);
				created.revision = 1;
				corpse = catalog.corpses.insert(corpse, std::move(created));
			}
			else
			{
				apply_corpse_lifecycle(&*corpse, payload);
				++corpse->revision;
			}
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_world_item_result::io_error;
		}
		mutation->corpse_revision = corpse->revision;
	}
	else
	{
		if (!found)
			return flatfile_world_item_result::not_found;
		if (corpse->revision != payload.expected_corpse_revision ||
		    corpse->owner_name != canonical_owner)
			return flatfile_world_item_result::conflict;
		if (!corpse->items.empty() ||
		    std::any_of(corpse->money.begin(), corpse->money.end(),
				[](int32_t value) { return value != 0; }))
			return flatfile_world_item_result::not_empty;
		catalog.corpses.erase(corpse);
	}
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_world_item_result::invalid;
	mutation->after_image = { catalog_filename, std::move(encoded) };
	mutation->catalog_revision = catalog.revision;
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_world_item_prepare_corpse_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, flatfile_corpse_transfer_mutation *mutation,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size() || payload.target_parent_item_uid)
		return flatfile_world_item_result::invalid;
	*mutation = {};
	const bool create = payload.from_owner.type == item_owner_type::player &&
			    payload.to_owner.type == item_owner_type::corpse &&
			    payload.reason == item_transfer_reason::corpse_create;
	const bool loot = payload.from_owner.type == item_owner_type::corpse &&
			  payload.to_owner.type == item_owner_type::player &&
			  payload.reason == item_transfer_reason::corpse_loot;
	if (create == loot || (create && !payload.corpse.present))
		return flatfile_world_item_result::invalid;
	const item_owner_identity &corpse_owner = create ? payload.to_owner : payload.from_owner;
	const uint32_t owner_pid = static_cast<uint32_t>(corpse_owner.id >> 32);
	const uint32_t save_id = static_cast<uint32_t>(corpse_owner.id);
	if (!owner_pid || !save_id || corpse_owner.id != item_corpse_owner_id(owner_pid, save_id) ||
	    corpse_owner.context_id)
		return flatfile_world_item_result::invalid;
	std::vector<player_item_snapshot> exact_items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &exact_items) != player_snapshot_codec_result::ok ||
	    !payload_items_match(payload, exact_items))
		return flatfile_world_item_result::invalid;
	std::vector<uint8_t> transport_blob;
	if (player_item_snapshot_list_encode(exact_items, &transport_blob) !=
		    player_snapshot_codec_result::ok ||
	    transport_blob.size() != payload.item_blob_size ||
	    !std::equal(transport_blob.begin(), transport_blob.end(), payload.item_blob.begin()) ||
	    !canonicalize_detached_items(&exact_items))
		return flatfile_world_item_result::invalid;
	std::vector<uint8_t> exact_blob;
	if (player_item_snapshot_list_encode(exact_items, &exact_blob) !=
	    player_snapshot_codec_result::ok)
		return flatfile_world_item_result::invalid;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	flatfile_corpse_record key = {};
	key.owner_pid = owner_pid;
	key.save_id = save_id;
	auto corpse =
		std::lower_bound(catalog.corpses.begin(), catalog.corpses.end(), key, corpse_less);
	const bool found = corpse != catalog.corpses.end() && corpse->owner_pid == owner_pid &&
			   corpse->save_id == save_id;
	if (!found && loot)
		return flatfile_world_item_result::not_found;
	if (found && payload.corpse.present &&
	    (corpse->owner_name != canonical_name(payload.corpse.owner_name) ||
	     corpse->values[3] != payload.corpse.values[3] ||
	     corpse->values[5] != payload.corpse.values[5] ||
	     corpse->values[6] != payload.corpse.values[6]))
		return flatfile_world_item_result::conflict;
	if (catalog.revision == UINT64_MAX || (found && corpse->revision == UINT64_MAX) ||
	    (!found && catalog.corpses.size() >= corpse_maximum))
		return flatfile_world_item_result::conflict;
	try
	{
		if (!found)
		{
			flatfile_corpse_record created = {};
			created.owner_pid = owner_pid;
			created.save_id = save_id;
			created.revision = 1;
			created.items = exact_items;
			apply_corpse_metadata(&created, payload.corpse);
			corpse = catalog.corpses.insert(corpse, std::move(created));
			mutation->created = true;
		}
		else
		{
			mutation->expected_items.reserve(corpse->items.size());
			for (const auto &item : corpse->items)
				mutation->expected_items.push_back({ item.object_uid, item.vnum });
			std::sort(mutation->expected_items.begin(), mutation->expected_items.end(),
				  [](const auto &left, const auto &right)
				  { return left.item_uid < right.item_uid; });
			if (create)
			{
				const int32_t offset = static_cast<int32_t>(corpse->items.size());
				for (auto item : exact_items)
				{
					if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
						item.parent_index += offset;
					corpse->items.push_back(std::move(item));
				}
			}
			else
			{
				std::vector<player_item_snapshot> selected;
				std::vector<player_item_snapshot> remaining;
				if (player_item_snapshot_extract_subtree(
					    corpse->items, payload.selected_item_uid, &selected,
					    &remaining) != player_snapshot_codec_result::ok)
					return flatfile_world_item_result::conflict;
				std::vector<uint8_t> selected_blob;
				if (player_item_snapshot_list_encode(selected, &selected_blob) !=
					    player_snapshot_codec_result::ok ||
				    selected_blob != exact_blob)
					return flatfile_world_item_result::conflict;
				corpse->items = std::move(remaining);
			}
			if (payload.corpse.present)
				apply_corpse_metadata(&*corpse, payload.corpse);
			++corpse->revision;
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_world_item_result::invalid;
	mutation->after_image = { catalog_filename, std::move(encoded) };
	mutation->corpse_revision = corpse->revision;
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_world_item_prepare_room_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, flatfile_room_transfer_mutation *mutation,
	std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return flatfile_world_item_result::invalid;
	*mutation = {};
	const bool deposit = room_transfer_deposit(payload);
	const bool withdraw = room_transfer_withdraw(payload);
	const bool create = room_transfer_create(payload);
	const bool destroy = room_transfer_destroy(payload);
	const bool reparent = room_transfer_reparent(payload);
	if (static_cast<unsigned int>(deposit) + static_cast<unsigned int>(withdraw) +
		    static_cast<unsigned int>(create) + static_cast<unsigned int>(destroy) +
		    static_cast<unsigned int>(reparent) !=
	    1)
		return flatfile_world_item_result::invalid;
	const bool append = deposit || create;
	const item_owner_identity &room_owner = append ? payload.to_owner : payload.from_owner;
	if (!room_owner.id || room_owner.id > INT32_MAX || room_owner.context_id)
		return flatfile_world_item_result::invalid;
	std::vector<player_item_snapshot> exact_items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &exact_items) != player_snapshot_codec_result::ok ||
	    !payload_items_match(payload, exact_items) ||
	    exact_items.front().parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return flatfile_world_item_result::invalid;
	std::vector<uint8_t> transport_blob;
	if (player_item_snapshot_list_encode(exact_items, &transport_blob) !=
		    player_snapshot_codec_result::ok ||
	    transport_blob.size() != payload.item_blob_size ||
	    !std::equal(transport_blob.begin(), transport_blob.end(), payload.item_blob.begin()) ||
	    !canonicalize_detached_items(&exact_items))
		return flatfile_world_item_result::invalid;
	std::vector<uint8_t> exact_blob;
	if (player_item_snapshot_list_encode(exact_items, &exact_blob) !=
	    player_snapshot_codec_result::ok)
		return flatfile_world_item_result::invalid;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	flatfile_room_item_record key = {};
	key.room_vnum = static_cast<int32_t>(room_owner.id);
	auto room = std::lower_bound(catalog.rooms.begin(), catalog.rooms.end(), key, room_less);
	const bool found = room != catalog.rooms.end() && room->room_vnum == key.room_vnum;
	const uint64_t expected_revision = append ? payload.expected_to_revision :
						    payload.expected_from_revision;
	if ((!found && (!append || expected_revision)) ||
	    (found && room->revision != expected_revision) || catalog.revision == UINT64_MAX ||
	    (found && room->revision == UINT64_MAX) ||
	    (!found && catalog.rooms.size() >= room_maximum))
		return flatfile_world_item_result::conflict;
	if (found && !room_custody(room->items, &mutation->expected_items))
		return flatfile_world_item_result::invalid;
	try
	{
		if (!found)
		{
			flatfile_room_item_record created = {};
			created.room_vnum = key.room_vnum;
			created.revision = 1;
			room = catalog.rooms.insert(room, std::move(created));
			mutation->created = true;
		}
		else
			++room->revision;
		if (append)
		{
			const size_t parent_index =
				payload.target_parent_item_uid ?
					room_item_index(room->items,
							payload.target_parent_item_uid) :
					room->items.size();
			if (payload.target_parent_item_uid &&
			    (parent_index == room->items.size() ||
			     !room_item_root_matches(room->items, parent_index,
						     payload.target_root_item_uid) ||
			     !apply_room_weight_delta(&room->items, parent_index,
						      exact_items.front().weight)))
				return flatfile_world_item_result::conflict;
			const int32_t offset = static_cast<int32_t>(room->items.size());
			for (size_t index = 0; index < exact_items.size(); ++index)
			{
				auto item = exact_items[index];
				if (!index && payload.target_parent_item_uid)
					item.parent_index = static_cast<int32_t>(parent_index);
				else if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += offset;
				room->items.push_back(std::move(item));
			}
		}
		else
		{
			const size_t selected_index =
				room_item_index(room->items, payload.selected_item_uid);
			if (selected_index == room->items.size())
				return flatfile_world_item_result::conflict;
			const int32_t parent_index = room->items[selected_index].parent_index;
			if (parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
			    !apply_room_weight_delta(
				    &room->items, static_cast<size_t>(parent_index),
				    -static_cast<int64_t>(exact_items.front().weight)))
				return flatfile_world_item_result::conflict;
			std::vector<player_item_snapshot> selected;
			std::vector<player_item_snapshot> remaining;
			if (player_item_snapshot_extract_subtree(
				    room->items, payload.selected_item_uid, &selected,
				    &remaining) != player_snapshot_codec_result::ok)
				return flatfile_world_item_result::conflict;
			std::vector<uint8_t> selected_blob;
			if (player_item_snapshot_list_encode(selected, &selected_blob) !=
				    player_snapshot_codec_result::ok ||
			    selected_blob != exact_blob)
				return flatfile_world_item_result::conflict;
			if (reparent)
			{
				const int32_t offset = static_cast<int32_t>(remaining.size());
				for (auto item : selected)
				{
					if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
						item.parent_index += offset;
					remaining.push_back(std::move(item));
				}
			}
			room->items = std::move(remaining);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_world_item_result::invalid;
	mutation->after_image = { catalog_filename, std::move(encoded) };
	mutation->room_revision = room->revision;
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_world_item_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_corpse_release_mutation *mutation,
	std::string *error)
{
	const bool release = payload.action == corpse_lifecycle_action::release;
	const bool destroy = payload.action == corpse_lifecycle_action::destroy;
	if (root.empty() || !lock.matches(root) || !mutation || release == destroy ||
	    !payload.owner_pid || !payload.save_id || !payload.expected_corpse_revision ||
	    payload.room_vnum <= 0 || !valid_printable(payload.owner_name, name_maximum, true))
		return flatfile_world_item_result::invalid;
	*mutation = {};
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	flatfile_corpse_record corpse_key = {};
	corpse_key.owner_pid = payload.owner_pid;
	corpse_key.save_id = payload.save_id;
	auto corpse = std::lower_bound(catalog.corpses.begin(), catalog.corpses.end(), corpse_key,
				       corpse_less);
	if (corpse == catalog.corpses.end() || corpse->owner_pid != payload.owner_pid ||
	    corpse->save_id != payload.save_id)
		return flatfile_world_item_result::not_found;
	if (corpse->revision != payload.expected_corpse_revision ||
	    corpse->owner_name != canonical_name(payload.owner_name) ||
	    corpse->room_vnum != payload.room_vnum || catalog.revision == UINT64_MAX)
		return flatfile_world_item_result::conflict;
	flatfile_room_item_record room_key = {};
	room_key.room_vnum = payload.room_vnum;
	auto room =
		std::lower_bound(catalog.rooms.begin(), catalog.rooms.end(), room_key, room_less);
	const bool room_found = room != catalog.rooms.end() && room->room_vnum == payload.room_vnum;
	if (release && ((room_found && room->revision != payload.expected_room_revision) ||
			(!room_found && payload.expected_room_revision) ||
			(room_found && room->revision == UINT64_MAX) ||
			(!room_found && catalog.rooms.size() >= room_maximum)))
		return flatfile_world_item_result::conflict;
	try
	{
		mutation->items = corpse->items;
		mutation->expected_items.reserve(corpse->items.size());
		for (size_t index = 0; index < corpse->items.size(); ++index)
		{
			const auto &item = corpse->items[index];
			uint64_t actual_root_uid = item.object_uid;
			int32_t parent_index = item.parent_index;
			while (parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				const auto &parent =
					corpse->items[static_cast<size_t>(parent_index)];
				actual_root_uid = parent.object_uid;
				parent_index = parent.parent_index;
			}
			mutation->expected_items.push_back(
				{ item.object_uid, item.vnum, actual_root_uid,
				  item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					  0 :
					  corpse->items[static_cast<size_t>(item.parent_index)]
						  .object_uid });
		}
		std::sort(mutation->expected_items.begin(), mutation->expected_items.end(),
			  [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		if (release)
		{
			if (!room_found)
			{
				flatfile_room_item_record created = {};
				created.room_vnum = payload.room_vnum;
				created.revision = 1;
				room = catalog.rooms.insert(room, std::move(created));
			}
			else
				++room->revision;
			const int32_t offset = static_cast<int32_t>(room->items.size());
			for (auto item : corpse->items)
			{
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += offset;
				room->items.push_back(std::move(item));
			}
			for (size_t index = 0; index < room->money.size(); ++index)
			{
				if (corpse->money[index] > INT32_MAX - room->money[index])
					return flatfile_world_item_result::conflict;
				room->money[index] += corpse->money[index];
			}
			mutation->room_revision = room->revision;
		}
		catalog.corpses.erase(corpse);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_world_item_result::invalid;
	mutation->after_image = { catalog_filename, std::move(encoded) };
	mutation->catalog_revision = catalog.revision;
	return flatfile_world_item_result::ok;
}
