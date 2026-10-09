#include "flatfile/flatfile_world_item_repository.h"

#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"
#include "economy/coin_transfer_command.h"
#include "economy/collector_command.h"
#include "core/defines.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <climits>
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
		     std::unordered_set<uint64_t> *item_uids, bool allow_room_zero = false)
{
	std::vector<uint8_t> encoded;
	if (!item_uids ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok)
		return false;
	size_t roots = 0;
	for (const auto &item : items)
	{
		roots += item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ? 1 : 0;
		if (!item.object_uid || item.vnum < 0 || (!allow_room_zero && !item.vnum) ||
		    item.equipment_slot != -1 || !item_uids->insert(item.object_uid).second)
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
			    !valid_item_list(saved.items, false, &item_uids))
				return false;
		}
		for (size_t index = 0; index < catalog.rooms.size(); ++index)
		{
			const auto &room = catalog.rooms[index];
			if (room.room_vnum <= 0 || !room.revision ||
			    (index && !room_less(catalog.rooms[index - 1], room)) ||
			    !std::all_of(room.money.begin(), room.money.end(),
					 [](int32_t value) { return value >= 0; }) ||
			    !valid_item_list(room.items, false, &item_uids, true))
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

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
    _GLIBCXX_USE_CXX11_ABI
bool world_storage_add(size_t left, size_t right, size_t *out) noexcept
{
	if (!out || right > SIZE_MAX - left)
	{
		errno = ENOBUFS;
		return false;
	}
	*out = left + right;
	return true;
}

bool world_storage_product(size_t count, size_t width, size_t *out) noexcept
{
	if (!out || (width && count > SIZE_MAX / width))
	{
		errno = ENOBUFS;
		return false;
	}
	*out = count * width;
	return true;
}

// Fresh string assign grows the empty C++11 ABI SSO capacity (15) to at
// least twice that capacity. Copy construction instead requests length+1.
size_t world_assigned_text_request(size_t length) noexcept
{
	return length <= 15 ? 0 : std::max(length, size_t{30}) + 1;
}

size_t world_copied_text_request(size_t length) noexcept
{
	return length <= 15 ? 0 : length + 1;
}

struct world_catalog_storage_profile
{
	size_t retained_payload_bytes = 0;
	size_t decode_prefix_peak = 0;
	size_t validation_peak = 0;
};

// These objects really coexist during the allocation-free two-pass scan.
// Scalar/closure call frames and library-private internals use the existing
// codec request policy; nested codec/prime-policy objects are charged separately.
struct world_catalog_storage_scan
{
	decoder header{nullptr, 0}, payload{nullptr, 0};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	player_item_snapshot_list_allocation_profile items;
	std::__detail::_Prime_rehash_policy uid_policy;
	std::pair<bool, size_t> uid_growth{};
	world_catalog_storage_profile result;
};

bool world_skip(decoder &in, size_t length) noexcept
{
	if (in.offset > in.size || length > in.size - in.offset)
		return false;
	in.offset += length;
	return true;
}

bool world_text_length(decoder &in, size_t maximum, size_t *length) noexcept
{
	uint32_t count = 0;
	if (!length || !in.number(&count) || count > maximum || !world_skip(in, count))
		return false;
	*length = count;
	return true;
}

bool world_reserved_string_buckets(size_t count, size_t *bytes) noexcept
{
	std::__detail::_Prime_rehash_policy policy;
	const size_t buckets = policy._M_next_bkt(
	    std::max(policy._M_bkt_for_elements(count), policy._M_bkt_for_elements(1)));
	return world_storage_product(buckets, sizeof(std::__detail::_Hash_node_base *), bytes);
}

constexpr size_t world_string_node_bytes = sizeof(std::__detail::_Hash_node<
    std::string, std::__cache_default<std::string, std::hash<std::string>>::value>);
constexpr size_t world_uid_node_bytes = sizeof(std::__detail::_Hash_node<
    uint64_t, std::__cache_default<uint64_t, std::hash<uint64_t>>::value>);

bool world_catalog_storage_preflight(const std::vector<uint8_t> &bytes,
				     world_catalog_storage_profile *output) noexcept
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!output || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	world_catalog_storage_scan scan;
	scan.header = decoder{bytes.data() + 8, bytes.size() - 8};
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!scan.header.number(&version) || !scan.header.number(&payload_size) ||
	    !scan.header.number(&revision) || !revision ||
	    (version != catalog_version && version != catalog_money_version &&
	     version != catalog_legacy_version) || payload_size != bytes.size() - header_size)
		return false;
	SHA256(bytes.data() + header_size, payload_size, scan.digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, scan.digest.data(), scan.digest.size()))
		return false;
	// First pass models the decoder's actual prefix; second pass models the
	// catalog validator while the complete decoded catalog remains live.
	for (unsigned int pass = 0; pass != 2; ++pass)
	{
		scan.payload = decoder{bytes.data() + header_size, payload_size};
		uint32_t corpse_count = 0, saved_count = 0, room_count = 0;
		if (!scan.payload.number(&corpse_count) || !scan.payload.number(&saved_count) ||
		    (version >= catalog_version && !scan.payload.number(&room_count)) ||
		    corpse_count > corpse_maximum || saved_count > saved_item_maximum ||
		    room_count > room_maximum)
			return false;
		size_t retained = 0, validator = 0, owner_buckets = 0, key_buckets = 0;
		size_t uid_count = 0, uid_buckets = 1, uid_bucket_bytes = 0;
		uint32_t previous_owner = 0;
		if (pass &&
		    (!world_reserved_string_buckets(corpse_count, &owner_buckets) ||
		     !world_reserved_string_buckets(saved_count, &key_buckets) ||
		     !world_storage_add(2 * sizeof(std::unordered_set<std::string>) +
					sizeof(std::unordered_set<uint64_t>),
					owner_buckets, &validator) ||
		     !world_storage_add(validator, key_buckets, &validator)))
			return false;
		if (pass)
		{
			scan.result.validation_peak = validator;
		}
		for (unsigned int group = 0; group != 3; ++group)
		{
			const size_t count = group == 0 ? corpse_count :
					     group == 1 ? saved_count : room_count;
			const size_t row_width = group == 0 ? sizeof(flatfile_corpse_record) :
						group == 1 ? sizeof(flatfile_saved_world_item_record) :
							     sizeof(flatfile_room_item_record);
			size_t rows = 0;
			if (!pass && (!world_storage_product(count, row_width, &rows) ||
				      !world_storage_add(retained, rows, &retained)))
				return false;
			if (!pass)
				scan.result.decode_prefix_peak =
				    std::max(scan.result.decode_prefix_peak, retained);
			for (size_t index = 0; index != count; ++index)
			{
				size_t name_length = 0, text_length = 0, canonical_live = 0;
				uint32_t owner = 0;
				if (group == 0)
				{
					if (!scan.payload.number(&owner) ||
					    !world_text_length(scan.payload, name_maximum, &name_length) ||
					    !world_skip(scan.payload, sizeof(uint32_t) + sizeof(int32_t)))
						return false;
					if (!pass && !world_storage_add(retained,
						world_assigned_text_request(name_length), &retained))
						return false;
					for (unsigned int text = 0; text != 3; ++text)
					{
						const size_t maximum = text == 0 ? short_description_maximum :
							text == 1 ? description_maximum : keywords_maximum;
						if (!world_text_length(scan.payload, maximum, &text_length) ||
						    (!pass && !world_storage_add(retained,
							world_assigned_text_request(text_length), &retained)))
							return false;
					}
					if (!world_skip(scan.payload, 9 * sizeof(int32_t) +
						(version >= catalog_money_version ? 4 * sizeof(int32_t) : 0) +
						sizeof(uint64_t)))
						return false;
					if (pass)
					{
						canonical_live = sizeof(std::string) +
							world_copied_text_request(name_length);
						size_t comparison = 0;
						if (!world_storage_add(validator, canonical_live, &comparison) ||
						    !world_storage_add(comparison, sizeof(std::string), &comparison))
							return false;
						scan.result.validation_peak = std::max(
							scan.result.validation_peak, comparison);
						if ((!index || previous_owner != owner) &&
						    (!world_storage_add(validator, world_string_node_bytes, &validator) ||
						     !world_storage_add(validator, world_copied_text_request(name_length),
									&validator)))
							return false;
					}
					previous_owner = owner;
				}
				else if (group == 1)
				{
					if (!world_text_length(scan.payload, key_maximum, &name_length) ||
					    !world_skip(scan.payload, sizeof(int32_t) + sizeof(uint64_t)))
						return false;
					if (!pass && !world_storage_add(retained,
						world_assigned_text_request(name_length), &retained))
						return false;
					if (pass &&
					    (!world_storage_add(validator, world_string_node_bytes, &validator) ||
					     !world_storage_add(validator, world_copied_text_request(name_length),
								&validator)))
						return false;
				}
				else if (!world_skip(scan.payload, sizeof(int32_t) + sizeof(uint64_t) +
								  4 * sizeof(int32_t)))
					return false;
				uint32_t wire_length = 0;
				if (!scan.payload.number(&wire_length) || !wire_length ||
				    wire_length > PLAYER_SNAPSHOT_MAX_BYTES ||
				    scan.payload.offset > scan.payload.size ||
				    wire_length > scan.payload.size - scan.payload.offset ||
				    player_item_snapshot_list_preflight(
					scan.payload.data + scan.payload.offset, wire_length, &scan.items) !=
					player_snapshot_codec_result::ok ||
				    !scan.items.fresh_decode_storage_policy_supported ||
				    !scan.items.canonical_encoder_storage_policy_supported ||
				    !world_skip(scan.payload, wire_length))
					return false;
				size_t peak = 0;
				if (!pass)
				{
					if (scan.items.decoded_payload_bytes < sizeof(std::vector<player_item_snapshot>) ||
					    !world_storage_add(retained, sizeof(std::vector<uint8_t>), &peak) ||
					    !world_storage_add(peak, wire_length, &peak) ||
					    !world_storage_add(peak, scan.items.item_codec_decoder_object_bytes, &peak) ||
					    !world_storage_add(peak, scan.items.decoded_payload_bytes, &peak) ||
					    !world_storage_add(peak, scan.items.relationship_scratch_bytes, &peak) ||
					    !world_storage_add(retained, scan.items.decoded_payload_bytes -
							    sizeof(std::vector<player_item_snapshot>), &retained))
						return false;
					scan.result.decode_prefix_peak = std::max(scan.result.decode_prefix_peak, peak);
				}
				else
				{
					size_t encoding = 0, forest_live = 0;
					if (!player_item_snapshot_list_encoder_working_bytes(scan.items, &encoding) ||
					    !world_storage_add(validator, canonical_live, &peak) ||
					    !world_storage_add(peak, sizeof(std::vector<uint8_t>), &peak) ||
					    !world_storage_add(peak, encoding, &peak) ||
					    !world_storage_add(canonical_live, sizeof(std::vector<uint8_t>), &forest_live) ||
					    !world_storage_add(forest_live, scan.items.canonical_encoded_capacity_bytes,
								&forest_live))
						return false;
					scan.result.validation_peak = std::max(scan.result.validation_peak, peak);
					for (size_t item = 0; item != scan.items.item_count; ++item)
					{
						if (!world_storage_add(validator, world_uid_node_bytes, &validator))
							return false;
						scan.uid_growth = scan.uid_policy._M_need_rehash(uid_buckets, uid_count, 1);
						size_t new_buckets = 0;
						if (scan.uid_growth.first &&
						    !world_storage_product(scan.uid_growth.second,
							sizeof(std::__detail::_Hash_node_base *), &new_buckets))
							return false;
						if (!world_storage_add(validator, forest_live, &peak) ||
						    !world_storage_add(peak, new_buckets, &peak))
							return false;
						scan.result.validation_peak = std::max(scan.result.validation_peak, peak);
						if (scan.uid_growth.first)
						{
							validator -= uid_bucket_bytes;
							if (!world_storage_add(validator, new_buckets, &validator))
								return false;
							uid_bucket_bytes = new_buckets;
							uid_buckets = scan.uid_growth.second;
						}
						++uid_count;
					}
				}
			}
		}
		if (scan.payload.offset != scan.payload.size)
			return false;
		if (!pass)
			scan.result.retained_payload_bytes = retained;
	}
	*output = scan.result;
	return true;
}

struct world_bounded_read_workspace
{
	explicit world_bounded_read_workspace(size_t directory_size)
	    : directory(directory_size, '\0'), filename(catalog_filename) {}
	world_item_catalog catalog;
	std::vector<uint8_t> bytes;
	std::string directory, filename;
	world_catalog_storage_profile profile;
};
#endif

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
	       (((payload.reason == item_transfer_reason::player_drop ||
		  item_transfer_forced_weapon_drop(payload.reason)) &&
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

flatfile_world_item_result flatfile_world_item_recovery_list_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_corpse_record> *corpses, std::vector<flatfile_room_item_record> *rooms,
	std::string *error)
{
	if (!lock.matches(root) || !corpses || !rooms)
		return flatfile_world_item_result::invalid;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	*corpses = std::move(catalog.corpses);
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

flatfile_world_item_result flatfile_world_item_recovery_list_all_locked(
    const std::string &root, const flatfile_authority_lock &lock,
    std::vector<flatfile_corpse_record> *corpses,
    std::vector<flatfile_room_item_record> *rooms,
    std::vector<flatfile_saved_world_item_record> *saved_items, std::string *error)
{
    if (root.empty() || !lock.matches(root) || !corpses || !rooms || !saved_items)
        return flatfile_world_item_result::invalid;
    world_item_catalog catalog;
    const auto loaded = load_catalog(root, &catalog, error);
    if (loaded != flatfile_world_item_result::ok)
        return loaded;
    *corpses = std::move(catalog.corpses);
    *rooms = std::move(catalog.rooms);
    *saved_items = std::move(catalog.saved_items);
    return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_world_item_recovery_list_all_locked_bounded(
    const std::string &root, const flatfile_authority_lock &lock,
    std::vector<flatfile_corpse_record> *corpses,
    std::vector<flatfile_room_item_record> *rooms,
    std::vector<flatfile_saved_world_item_record> *saved_items,
    flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
    size_t outer_live_scratch, size_t *retained_output_payload_bytes) noexcept
{
	if (root.empty() || !lock.matches(root) || !corpses || !rooms || !saved_items ||
	    !reserve_scratch_peak)
		return flatfile_world_item_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
    !_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live_scratch;
	(void)retained_output_payload_bytes;
	errno = ENOTSUP;
	return flatfile_world_item_result::io_error;
#else
	size_t directory_size = 0, live = 0;
	// The workspace constructor builds fresh count-character and copied-name
	// strings directly, avoiding an unmodeled root-copy/append growth phase.
	if (!world_storage_add(root.size(), sizeof("/domains") - 1, &directory_size) ||
	    directory_size == SIZE_MAX ||
	    !world_storage_add(outer_live_scratch, sizeof(world_bounded_read_workspace), &live) ||
	    !world_storage_add(live, directory_size <= 15 ? 0 : directory_size + 1, &live) ||
	    !world_storage_add(live, world_copied_text_request(sizeof("world_item_catalog") - 1),
			       &live) || !reserve_scratch_peak(live, context))
	{
		errno = ENOBUFS;
		return flatfile_world_item_result::io_error;
	}
	try
	{
		world_bounded_read_workspace work(directory_size);
		memcpy(work.directory.data(), root.data(), root.size());
		memcpy(work.directory.data() + root.size(), "/domains", sizeof("/domains") - 1);
		const auto loaded = flatfile_read_bounded(work.directory, work.filename,
		    catalog_maximum_bytes, &work.bytes, reserve_scratch_peak, context, live);
		if (loaded == flatfile_read_result::not_found)
			return flatfile_world_item_result::not_found;
		if (loaded == flatfile_read_result::io_error)
			return flatfile_world_item_result::io_error;
		if (loaded != flatfile_read_result::ok)
			return flatfile_world_item_result::invalid;
		size_t scan_peak = 0;
		const size_t nested_scan = std::max(player_item_snapshot_list_preflight_object_bytes(),
			std::max(sizeof(std::__detail::_Prime_rehash_policy), sizeof(decoder)));
		if (!world_storage_add(live, work.bytes.capacity(), &live) ||
		    !world_storage_add(live, sizeof(world_catalog_storage_scan), &scan_peak) ||
		    !world_storage_add(scan_peak, nested_scan, &scan_peak) ||
		    !reserve_scratch_peak(scan_peak, context))
		{
			errno = ENOBUFS;
			return flatfile_world_item_result::io_error;
		}
		errno = 0;
		if (!world_catalog_storage_preflight(work.bytes, &work.profile))
			return errno == ENOBUFS ? flatfile_world_item_result::io_error :
						flatfile_world_item_result::invalid;
		size_t validation = 0, peak = 0;
		if (!world_storage_add(work.profile.retained_payload_bytes,
			work.profile.validation_peak, &validation) ||
		    !world_storage_add(live, sizeof(world_item_catalog) + 2 * sizeof(decoder) +
					 sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>), &peak) ||
		    !world_storage_add(peak, std::max(work.profile.decode_prefix_peak, validation), &peak) ||
		    !reserve_scratch_peak(peak, context))
		{
			errno = ENOBUFS;
			return flatfile_world_item_result::io_error;
		}
		// The original semantic decoder remains authoritative for every field,
		// ordering rule, topology, UID, money value and canonical item encoding.
		if (!decode_catalog(work.bytes, &work.catalog))
			return flatfile_world_item_result::invalid;
		static_assert(std::is_nothrow_move_assignable_v<std::vector<flatfile_corpse_record>>);
		static_assert(std::is_nothrow_move_assignable_v<std::vector<flatfile_room_item_record>>);
		static_assert(std::is_nothrow_move_assignable_v<std::vector<flatfile_saved_world_item_record>>);
		*corpses = std::move(work.catalog.corpses);
		*rooms = std::move(work.catalog.rooms);
		*saved_items = std::move(work.catalog.saved_items);
		if (retained_output_payload_bytes)
			*retained_output_payload_bytes = work.profile.retained_payload_bytes;
		return flatfile_world_item_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_world_item_result::io_error;
	}
	catch (...)
	{
		return flatfile_world_item_result::invalid;
	}
#endif
}

flatfile_world_item_result flatfile_world_item_read_coin(const std::string &root,
							 const flatfile_authority_lock &lock,
							 const item_owner_identity &owner,
							 uint64_t uid, player_item_snapshot *item,
							 std::string *error)
{
	if (!lock.matches(root) || !uid || !item ||
	    (owner.type != item_owner_type::room && owner.type != item_owner_type::corpse))
		return flatfile_world_item_result::invalid;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	size_t matches = 0;
	auto inspect = [&](const std::vector<player_item_snapshot> &items)
	{
		for (const auto &candidate : items)
			if (candidate.object_uid == uid)
			{
				*item = candidate;
				++matches;
			}
	};
	if (owner.type == item_owner_type::corpse)
	{
		for (const auto &corpse : catalog.corpses)
			if (item_corpse_owner_id(corpse.owner_pid, corpse.save_id) == owner.id)
				inspect(corpse.items);
	}
	else
	{
		for (const auto &room : catalog.rooms)
			if (static_cast<uint64_t>(room.room_vnum) == owner.id)
				inspect(room.items);
		for (const auto &saved : catalog.saved_items)
			if (static_cast<uint64_t>(saved.room_vnum) == owner.id)
				inspect(saved.items);
	}
	return matches == 1 ? flatfile_world_item_result::ok :
	       matches	    ? flatfile_world_item_result::conflict :
			      flatfile_world_item_result::not_found;
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
	if (loaded == flatfile_world_item_result::not_found &&
	    payload.action == corpse_lifecycle_action::upsert && !payload.expected_corpse_revision)
		catalog = {};
	else if (loaded != flatfile_world_item_result::ok)
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
	if (loaded == flatfile_world_item_result::not_found && create &&
	    !payload.expected_to_revision)
		catalog = {};
	else if (loaded != flatfile_world_item_result::ok)
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
				std::vector<uint64_t> selected_roots;
				std::vector<player_item_snapshot> selected;
				std::vector<player_item_snapshot> remaining;
				if (!item_transfer_selected_roots(payload, &selected_roots) ||
				    player_item_snapshot_extract_forest(
					    corpse->items, selected_roots, &selected, &remaining) !=
					    player_snapshot_codec_result::ok)
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
	if (loaded == flatfile_world_item_result::not_found)
	{
		if (!append)
			return flatfile_world_item_result::conflict;
		catalog = {};
	}
	else if (loaded != flatfile_world_item_result::ok)
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
			if (payload.target_parent_item_uid)
			{
				int64_t selected_weight = 0;
				for (const auto &item : exact_items)
					if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
					{
						if (item.weight < 0 ||
						    selected_weight > INT64_MAX - item.weight)
							return flatfile_world_item_result::conflict;
						selected_weight += item.weight;
					}
				if (parent_index == room->items.size() ||
				    !room_item_root_matches(room->items, parent_index,
							    payload.target_root_item_uid) ||
				    !apply_room_weight_delta(&room->items, parent_index,
							     selected_weight))
					return flatfile_world_item_result::conflict;
			}
			const int32_t offset = static_cast<int32_t>(room->items.size());
			for (size_t index = 0; index < exact_items.size(); ++index)
			{
				auto item = exact_items[index];
				if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
				    payload.target_parent_item_uid)
					item.parent_index = static_cast<int32_t>(parent_index);
				else if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += offset;
				room->items.push_back(std::move(item));
			}
		}
		else
		{
			std::vector<uint64_t> selected_roots;
			if (!item_transfer_selected_roots(payload, &selected_roots))
				return flatfile_world_item_result::conflict;
			for (uint64_t selected_root : selected_roots)
			{
				const size_t selected_index =
					room_item_index(room->items, selected_root);
				auto exact_root =
					std::find_if(exact_items.begin(), exact_items.end(),
						     [&](const auto &item)
						     { return item.object_uid == selected_root; });
				if (selected_index == room->items.size() ||
				    exact_root == exact_items.end())
					return flatfile_world_item_result::conflict;
				const int32_t parent_index =
					room->items[selected_index].parent_index;
				if (parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
				    !apply_room_weight_delta(
					    &room->items, static_cast<size_t>(parent_index),
					    -static_cast<int64_t>(exact_root->weight)))
					return flatfile_world_item_result::conflict;
			}
			std::vector<player_item_snapshot> selected;
			std::vector<player_item_snapshot> remaining;
			if (player_item_snapshot_extract_forest(room->items, selected_roots,
								&selected, &remaining) !=
			    player_snapshot_codec_result::ok)
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

namespace
{
uint64_t collector_snapshot_parent(const std::vector<player_item_snapshot> &items, size_t index)
{
	if (index >= items.size())
		return 0;
	const int32_t parent = items[index].parent_index;
	return parent == PLAYER_SNAPSHOT_NO_PARENT || parent < 0 ||
			       static_cast<size_t>(parent) >= index ?
		       0 :
		       items[static_cast<size_t>(parent)].object_uid;
}

uint64_t collector_snapshot_root(const std::vector<player_item_snapshot> &items, size_t index)
{
	if (index >= items.size())
		return 0;
	for (size_t depth = 0; depth <= items.size(); ++depth)
	{
		const int32_t parent = items[index].parent_index;
		if (parent == PLAYER_SNAPSHOT_NO_PARENT)
			return items[index].object_uid;
		if (parent < 0 || static_cast<size_t>(parent) >= index)
			return 0;
		index = static_cast<size_t>(parent);
	}
	return 0;
}

bool collector_detach_snapshot(std::vector<player_item_snapshot> *items,
			       const collector_command_payload &payload,
			       const player_item_snapshot &exact, unsigned int *result_code)
{
	if (!items || !result_code)
		return false;
	auto selected = std::find_if(items->begin(), items->end(), [&](const auto &item)
				     { return item.object_uid == payload.selected_item_uid; });
	if (selected == items->end())
	{
		*result_code = ENOENT;
		return true;
	}
	const size_t selected_index = static_cast<size_t>(selected - items->begin());
	const uint64_t root_uid = collector_snapshot_root(*items, selected_index);
	if (!root_uid || root_uid != payload.items[0].root_item_uid)
	{
		*result_code = ESTALE;
		return true;
	}
	std::vector<size_t> tree;
	try
	{
		for (size_t index = 0; index < items->size(); ++index)
			if (collector_snapshot_root(*items, index) == root_uid)
				tree.push_back(index);
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	if (tree.size() != payload.item_count)
	{
		*result_code = EMSGSIZE;
		return true;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry &expected = payload.items[index];
		auto actual = std::find_if(
			tree.begin(), tree.end(), [&](size_t position)
			{ return (*items)[position].object_uid == expected.item_uid; });
		if (actual == tree.end())
		{
			*result_code = ESTALE;
			return true;
		}
		const player_item_snapshot &snapshot = (*items)[*actual];
		if (snapshot.vnum != expected.vnum ||
		    collector_snapshot_parent(*items, *actual) != expected.parent_item_uid)
		{
			*result_code = ESTALE;
			return true;
		}
	}
	if (selected->vnum != exact.vnum || selected->cost != exact.cost ||
	    selected->condition != exact.condition)
	{
		*result_code = ESTALE;
		return true;
	}
	int64_t child_weight = 0;
	for (size_t index = 0; index < items->size(); ++index)
		if ((*items)[index].parent_index == static_cast<int32_t>(selected_index))
		{
			if ((*items)[index].weight < 0 ||
			    child_weight > INT64_MAX - (*items)[index].weight)
			{
				*result_code = ERANGE;
				return true;
			}
			child_weight += (*items)[index].weight;
		}
	if (selected->weight < child_weight || exact.weight != selected->weight - child_weight)
	{
		*result_code = ESTALE;
		return true;
	}
	const int64_t own_weight = exact.weight;
	int32_t ancestor = selected->parent_index;
	for (size_t depth = 0; ancestor != PLAYER_SNAPSHOT_NO_PARENT; ++depth)
	{
		if (depth >= items->size() || ancestor < 0 ||
		    static_cast<size_t>(ancestor) >= selected_index ||
		    (*items)[static_cast<size_t>(ancestor)].weight < own_weight)
		{
			*result_code = EBADMSG;
			return true;
		}
		auto &parent = (*items)[static_cast<size_t>(ancestor)];
		parent.weight -= static_cast<int32_t>(own_weight);
		ancestor = parent.parent_index;
	}
	const uint64_t selected_parent = collector_snapshot_parent(*items, selected_index);
	std::vector<uint64_t> parents;
	std::vector<player_item_snapshot> remaining;
	try
	{
		parents.reserve(items->size() - 1);
		remaining.reserve(items->size() - 1);
		for (size_t index = 0; index < items->size(); ++index)
		{
			if (index == selected_index)
				continue;
			uint64_t parent = collector_snapshot_parent(*items, index);
			if (parent == payload.selected_item_uid)
				parent = selected_parent;
			parents.push_back(parent);
			remaining.push_back(std::move((*items)[index]));
		}
		for (size_t index = 0; index < remaining.size(); ++index)
		{
			if (!parents[index])
			{
				remaining[index].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				continue;
			}
			auto parent =
				std::find_if(remaining.begin(),
					     remaining.begin() + static_cast<ptrdiff_t>(index),
					     [&](const auto &candidate)
					     { return candidate.object_uid == parents[index]; });
			if (parent == remaining.begin() + static_cast<ptrdiff_t>(index))
			{
				*result_code = EBADMSG;
				return true;
			}
			remaining[index].parent_index =
				static_cast<int32_t>(parent - remaining.begin());
		}
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	*items = std::move(remaining);
	return true;
}
} // namespace

flatfile_world_item_result flatfile_world_item_prepare_collector_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const collector_command_payload &payload, flatfile_collector_world_mutation *mutation,
	unsigned int *result_code, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !result_code ||
	    payload.action != collector_action::collect || !payload.item_count ||
	    !payload.item_blob_size || payload.from_owner.context_id ||
	    (payload.from_owner.type != item_owner_type::corpse &&
	     payload.from_owner.type != item_owner_type::room))
		return flatfile_world_item_result::invalid;
	*mutation = {};
	*result_code = 0;
	std::vector<player_item_snapshot> decoded;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &decoded) != player_snapshot_codec_result::ok ||
	    decoded.size() != 1 || decoded[0].object_uid != payload.selected_item_uid ||
	    decoded[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT || decoded[0].equipment_slot != 0)
	{
		*result_code = EBADMSG;
		return flatfile_world_item_result::ok;
	}
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_world_item_result::not_found)
	{
		if (payload.from_owner.type == item_owner_type::room)
			return flatfile_world_item_result::unchanged;
		*result_code = ENOENT;
		return flatfile_world_item_result::ok;
	}
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	if (catalog.revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return flatfile_world_item_result::ok;
	}
	std::vector<player_item_snapshot> *items = nullptr;
	uint64_t *aggregate_revision = nullptr;
	bool erase_saved = false;
	if (payload.from_owner.type == item_owner_type::corpse)
	{
		const uint32_t owner_pid = static_cast<uint32_t>(payload.from_owner.id >> 32);
		const uint32_t save_id = static_cast<uint32_t>(payload.from_owner.id);
		flatfile_corpse_record key = {};
		key.owner_pid = owner_pid;
		key.save_id = save_id;
		auto corpse = std::lower_bound(catalog.corpses.begin(), catalog.corpses.end(), key,
					       corpse_less);
		if (!owner_pid || !save_id || corpse == catalog.corpses.end() ||
		    corpse->owner_pid != owner_pid || corpse->save_id != save_id)
		{
			*result_code = ENOENT;
			return flatfile_world_item_result::ok;
		}
		items = &corpse->items;
		aggregate_revision = &corpse->revision;
	}
	else
	{
		if (!payload.from_owner.id || payload.from_owner.id > INT32_MAX)
			return flatfile_world_item_result::invalid;
		const int32_t room_vnum = static_cast<int32_t>(payload.from_owner.id);
		size_t matches = 0;
		for (auto &saved : catalog.saved_items)
			if (saved.room_vnum == room_vnum &&
			    std::any_of(saved.items.begin(), saved.items.end(),
					[&](const auto &item)
					{ return item.object_uid == payload.selected_item_uid; }))
			{
				items = &saved.items;
				aggregate_revision = &saved.revision;
				erase_saved = true;
				++matches;
			}
		flatfile_room_item_record key = {};
		key.room_vnum = room_vnum;
		auto room = std::lower_bound(catalog.rooms.begin(), catalog.rooms.end(), key,
					     room_less);
		if (room != catalog.rooms.end() && room->room_vnum == room_vnum &&
		    std::any_of(room->items.begin(), room->items.end(), [&](const auto &item)
				{ return item.object_uid == payload.selected_item_uid; }))
		{
			items = &room->items;
			aggregate_revision = &room->revision;
			erase_saved = false;
			++matches;
		}
		if (!matches)
			return flatfile_world_item_result::unchanged;
		if (matches != 1)
		{
			*result_code = EBADMSG;
			return flatfile_world_item_result::ok;
		}
	}
	if (!items || !aggregate_revision || *aggregate_revision == UINT64_MAX)
	{
		*result_code = ERANGE;
		return flatfile_world_item_result::ok;
	}
	if (*aggregate_revision != payload.expected_from_owner_revision)
	{
		*result_code = ESTALE;
		return flatfile_world_item_result::ok;
	}
	if (!collector_detach_snapshot(items, payload, decoded[0], result_code))
		return errno == ENOMEM ? flatfile_world_item_result::io_error :
					 flatfile_world_item_result::invalid;
	if (*result_code)
		return flatfile_world_item_result::ok;
	++*aggregate_revision;
	if (erase_saved && items->empty())
	{
		auto empty = std::find_if(catalog.saved_items.begin(), catalog.saved_items.end(),
					  [&](const auto &saved) { return &saved.items == items; });
		if (empty == catalog.saved_items.end())
			return flatfile_world_item_result::invalid;
		catalog.saved_items.erase(empty);
	}
	++catalog.revision;
	mutation->after_image.filename = catalog_filename;
	if (!encode_catalog(catalog, &mutation->after_image.bytes))
		return flatfile_world_item_result::invalid;
	mutation->changed = true;
	return flatfile_world_item_result::ok;
}

// Coin custody is authoritative, but an existing room-transfer projection must
// advance in the same transaction or the next ordinary loot command goes stale.
flatfile_world_item_result
flatfile_world_item_prepare_coin_rooms(const std::string &root, const flatfile_authority_lock &lock,
				       const coin_transfer_payload &payload,
				       const coin_transfer_result &result,
				       flatfile_authority_after_image *image, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !image)
		return flatfile_world_item_result::invalid;
	*image = {};
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_world_item_result::not_found)
		return flatfile_world_item_result::unchanged;
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	bool changed = false;
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		if (endpoint.change.type != critical_command_type::item_transfer)
			continue;
		item_transfer_payload transfer;
		if (!item_transfer_command_decode_payload(endpoint.change, &transfer) ||
		    transfer.item_count != 1)
			return flatfile_world_item_result::invalid;
		const bool creation = transfer.from_owner.type == item_owner_type::system;
		const auto &owner = creation ? transfer.to_owner : transfer.from_owner;
		if (owner.type != item_owner_type::room)
			continue;
		auto room = std::find_if(
			catalog.rooms.begin(), catalog.rooms.end(), [&](const auto &entry)
			{ return static_cast<uint64_t>(entry.room_vnum) == owner.id; });
		// Imported saved-item piles have no room-transfer projection. They retain
		// the existing custody/coin-payload loading path.
		if (room == catalog.rooms.end())
			continue;
		const uint64_t revision = creation ? result.piles[index].to_owner_revision :
						     result.piles[index].from_owner_revision;
		if (!revision || room->revision != revision - 1)
			return flatfile_world_item_result::conflict;
		std::vector<player_item_snapshot> exact;
		if (player_item_snapshot_list_decode(transfer.item_blob.data(),
						     transfer.item_blob_size,
						     &exact) != player_snapshot_codec_result::ok ||
		    exact.size() != 1 || !payload_items_match(transfer, exact) ||
		    !canonicalize_detached_items(&exact))
			return flatfile_world_item_result::invalid;
		const uint64_t uid = transfer.selected_item_uid;
		const size_t position = room_item_index(room->items, uid);
		const bool consumed = transfer.to_owner.type == item_owner_type::destruction;
		if (creation ? position != room->items.size() : position == room->items.size())
			return flatfile_world_item_result::conflict;
		int32_t parent = PLAYER_SNAPSHOT_NO_PARENT;
		int64_t old_weight = 0;
		if (!creation)
		{
			const auto &stored = room->items[position];
			parent = stored.parent_index;
			old_weight = stored.weight;
			if (!std::equal(endpoint.before.begin(), endpoint.before.end(),
					stored.values.begin()) ||
			    !room_item_root_matches(room->items, position,
						    transfer.items[0].root_item_uid) ||
			    (parent == PLAYER_SNAPSHOT_NO_PARENT ?
				     0 :
				     room->items[parent].object_uid) !=
				    transfer.items[0].parent_item_uid)
				return flatfile_world_item_result::conflict;
		}
		else if (transfer.target_parent_item_uid)
		{
			const size_t found =
				room_item_index(room->items, transfer.target_parent_item_uid);
			if (found == room->items.size() ||
			    !room_item_root_matches(room->items, found,
						    transfer.target_root_item_uid))
				return flatfile_world_item_result::conflict;
			parent = static_cast<int32_t>(found);
		}
		const int64_t new_weight = consumed ? 0 : exact[0].weight;
		if (parent != PLAYER_SNAPSHOT_NO_PARENT &&
		    !apply_room_weight_delta(&room->items, static_cast<size_t>(parent),
					     new_weight - old_weight))
			return flatfile_world_item_result::conflict;
		if (consumed)
		{
			std::vector<player_item_snapshot> selected, remaining;
			if (player_item_snapshot_extract_forest(room->items, { uid }, &selected,
								&remaining) !=
				    player_snapshot_codec_result::ok ||
			    selected.size() != 1)
				return flatfile_world_item_result::conflict;
			room->items = std::move(remaining);
		}
		else
		{
			exact[0].parent_index = parent;
			if (creation)
				room->items.push_back(std::move(exact[0]));
			else
				room->items[position] = std::move(exact[0]);
		}
		room->revision = revision;
		changed = true;
	}
	if (!changed)
		return flatfile_world_item_result::unchanged;
	if (catalog.revision == UINT64_MAX)
		return flatfile_world_item_result::conflict;
	++catalog.revision;
	image->filename = catalog_filename;
	return encode_catalog(catalog, &image->bytes) ? flatfile_world_item_result::ok :
							flatfile_world_item_result::invalid;
}

flatfile_world_item_result flatfile_world_item_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_corpse_release_mutation *mutation,
	std::string *error)
{
	const bool release = payload.action == corpse_lifecycle_action::release;
	const bool destroy = payload.action == corpse_lifecycle_action::destroy;
	const bool resurrect = payload.action == corpse_lifecycle_action::resurrect;
	const bool raise_follower = payload.action == corpse_lifecycle_action::raise_follower;
	const bool release_nested = payload.action == corpse_lifecycle_action::release_nested;
	const bool nested_room = release_nested && !payload.destination_player_pid;
	if (root.empty() || !lock.matches(root) || !mutation ||
	    static_cast<unsigned int>(release) + static_cast<unsigned int>(destroy) +
			    static_cast<unsigned int>(resurrect) +
			    static_cast<unsigned int>(raise_follower) +
			    static_cast<unsigned int>(release_nested) !=
		    1 ||
	    !payload.owner_pid || !payload.save_id || !payload.expected_corpse_revision ||
	    payload.room_vnum <= 0 ||
	    (resurrect && (!payload.destination_player_pid || payload.old_room_vnum <= 0 ||
			   !payload.expected_player_revision ||
			   std::any_of(payload.money.begin(), payload.money.end(),
				       [](int32_t amount) { return amount < 0; }))) ||
	    (raise_follower &&
	     (!payload.destination_player_pid || payload.old_room_vnum ||
	      !payload.expected_player_revision || payload.expected_room_revision ||
	      std::any_of(payload.money.begin(), payload.money.end(),
			  [](int32_t amount) { return amount < 0; }))) ||
	    (release_nested &&
	     (!payload.target_root_item_uid || !payload.target_parent_item_uid ||
	      !payload.expected_target_parent_revision ||
	      (payload.destination_player_pid ?
		       (!payload.expected_player_revision || payload.expected_room_revision) :
		       !payload.expected_room_revision))) ||
	    !valid_printable(payload.owner_name, name_maximum, true))
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
	room_key.room_vnum = resurrect ? payload.old_room_vnum : payload.room_vnum;
	auto room =
		std::lower_bound(catalog.rooms.begin(), catalog.rooms.end(), room_key, room_less);
	const bool room_found = room != catalog.rooms.end() &&
				room->room_vnum == room_key.room_vnum;
	if ((release || resurrect || nested_room) &&
	    ((room_found && room->revision != payload.expected_room_revision) ||
	     (!room_found && payload.expected_room_revision) ||
	     (room_found && room->revision == UINT64_MAX) ||
	     (!room_found && catalog.rooms.size() >= room_maximum)))
		return flatfile_world_item_result::conflict;
	try
	{
		mutation->items = corpse->items;
		mutation->money = corpse->money;
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
		if (release || resurrect || nested_room)
		{
			if (!room_found)
			{
				if (nested_room)
					return flatfile_world_item_result::conflict;
				flatfile_room_item_record created = {};
				created.room_vnum = room_key.room_vnum;
				created.revision = 1;
				room = catalog.rooms.insert(room, std::move(created));
			}
			else
				++room->revision;
			if (release)
			{
				const int32_t offset = static_cast<int32_t>(room->items.size());
				for (auto item : corpse->items)
				{
					if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
						item.parent_index += offset;
					room->items.push_back(std::move(item));
				}
			}
			else if (nested_room)
			{
				const size_t parent_index = room_item_index(
					room->items, payload.target_parent_item_uid);
				if (parent_index == room->items.size() ||
				    !room_item_root_matches(room->items, parent_index,
							    payload.target_root_item_uid))
					return flatfile_world_item_result::conflict;
				for (const auto &item : corpse->items)
					if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
					    !apply_room_weight_delta(&room->items, parent_index,
								     item.weight))
						return flatfile_world_item_result::conflict;
				const int32_t offset = static_cast<int32_t>(room->items.size());
				for (auto item : corpse->items)
				{
					if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
						item.parent_index =
							static_cast<int32_t>(parent_index);
					else
						item.parent_index += offset;
					room->items.push_back(std::move(item));
				}
			}
			// Floor money is published to the live room after this commit, but is not
			// part of the durable room aggregate.  Pickup uses the currency authority
			// directly and cannot atomically withdraw this world-domain snapshot; if it
			// were persisted, every reboot after pickup would recreate the same coins.
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

flatfile_world_item_result flatfile_world_item_prepare_world_corpse_raise(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload, flatfile_world_corpse_raise_mutation *mutation,
	std::string *error)
{
	const uint64_t source_uid = (static_cast<uint64_t>(payload.owner_pid) << 32) |
				    static_cast<uint64_t>(payload.save_id);
	const bool hostile = payload.pet_uid == 0;
	if (root.empty() || !lock.matches(root) || !mutation ||
	    payload.action != corpse_lifecycle_action::raise_world_follower || !source_uid ||
	    !payload.expected_corpse_revision || !payload.expected_room_revision ||
	    !payload.destination_player_pid || !payload.expected_player_revision ||
	    payload.room_vnum <= 0 || (!hostile && payload.pet_uid != source_uid))
		return flatfile_world_item_result::invalid;
	*mutation = {};
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	auto selected = catalog.saved_items.end();
	size_t selected_root = 0;
	for (auto saved = catalog.saved_items.begin(); saved != catalog.saved_items.end(); ++saved)
	{
		for (size_t index = 0; index < saved->items.size(); ++index)
		{
			if (saved->items[index].object_uid != source_uid)
				continue;
			if (selected != catalog.saved_items.end() ||
			    saved->items[index].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				return flatfile_world_item_result::conflict;
			selected = saved;
			selected_root = index;
		}
	}
	if (selected == catalog.saved_items.end())
		return flatfile_world_item_result::not_found;
	if (selected->room_vnum != payload.room_vnum ||
	    selected->items[selected_root].type != ITEM_CORPSE ||
	    selected->items.size() > ITEM_TRANSFER_MAX_ITEMS || catalog.revision == UINT64_MAX)
		return flatfile_world_item_result::conflict;

	std::vector<bool> discarded;
	std::vector<int32_t> pet_indexes;
	try
	{
		discarded.resize(selected->items.size());
		pet_indexes.assign(selected->items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		mutation->expected_items.reserve(selected->items.size());
		mutation->pet_items.reserve(selected->items.size());
		mutation->durable_uids.reserve(selected->items.size());
		mutation->discarded_uids.reserve(selected->items.size());
		for (size_t index = 0; index < selected->items.size(); ++index)
		{
			const auto &item = selected->items[index];
			uint64_t parent_uid = 0;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >=
					    selected->items.size())
					return flatfile_world_item_result::invalid;
				parent_uid = selected->items[static_cast<size_t>(item.parent_index)]
						     .object_uid;
			}
			bool reaches_source = index == selected_root;
			int32_t parent_index = item.parent_index;
			for (size_t depth = 0;
			     !reaches_source && parent_index != PLAYER_SNAPSHOT_NO_PARENT; ++depth)
			{
				if (depth >= selected->items.size() || parent_index < 0 ||
				    static_cast<size_t>(parent_index) >= selected->items.size())
					return flatfile_world_item_result::invalid;
				const size_t parent = static_cast<size_t>(parent_index);
				reaches_source = parent == selected_root;
				parent_index = selected->items[parent].parent_index;
			}
			if (!reaches_source)
				return flatfile_world_item_result::conflict;
			mutation->expected_items.push_back(
				{ item.object_uid, item.vnum, source_uid, parent_uid });
			discarded[index] = index == selected_root ||
					   (item.extra_flags & ITEM_TRANSIENT) != 0;
			if ((item.extra_flags & ITEM_ARTIFACT) != 0)
				return flatfile_world_item_result::conflict;
			(discarded[index] ? mutation->discarded_uids : mutation->durable_uids)
				.push_back(item.object_uid);
		}
		for (size_t index = 0; index < selected->items.size(); ++index)
		{
			if (discarded[index])
				continue;
			int32_t parent = selected->items[index].parent_index;
			for (size_t depth = 0; parent != PLAYER_SNAPSHOT_NO_PARENT; ++depth)
			{
				if (depth >= selected->items.size() || parent < 0 ||
				    static_cast<size_t>(parent) >= selected->items.size())
					return flatfile_world_item_result::invalid;
				const size_t parent_index = static_cast<size_t>(parent);
				if (discarded[parent_index] && parent_index != selected_root)
					return flatfile_world_item_result::conflict;
				parent = selected->items[parent_index].parent_index;
			}
			pet_indexes[index] = static_cast<int32_t>(mutation->pet_items.size());
			auto item = selected->items[index];
			item.equipment_slot = -1;
			mutation->pet_items.push_back(std::move(item));
		}
		for (size_t index = 0; index < selected->items.size(); ++index)
		{
			if (discarded[index])
				continue;
			auto &pet_item =
				mutation->pet_items[static_cast<size_t>(pet_indexes[index])];
			const int32_t parent = selected->items[index].parent_index;
			pet_item.parent_index =
				parent == PLAYER_SNAPSHOT_NO_PARENT ||
						discarded[static_cast<size_t>(parent)] ?
					PLAYER_SNAPSHOT_NO_PARENT :
					pet_indexes[static_cast<size_t>(parent)];
		}
		for (size_t skipped = 0; skipped < selected->items.size(); ++skipped)
		{
			if (!discarded[skipped] || skipped == selected_root)
				continue;
			int32_t parent = selected->items[skipped].parent_index;
			for (size_t depth = 0; parent != PLAYER_SNAPSHOT_NO_PARENT; ++depth)
			{
				if (depth >= selected->items.size() || parent < 0 ||
				    static_cast<size_t>(parent) >= selected->items.size())
					return flatfile_world_item_result::invalid;
				const size_t parent_index = static_cast<size_t>(parent);
				if (!discarded[parent_index])
				{
					auto &pet_parent = mutation->pet_items[static_cast<size_t>(
						pet_indexes[parent_index])];
					const int64_t adjusted =
						static_cast<int64_t>(pet_parent.weight) -
						selected->items[skipped].weight;
					if (adjusted < INT32_MIN || adjusted > INT32_MAX)
						return flatfile_world_item_result::conflict;
					pet_parent.weight = static_cast<int32_t>(adjusted);
				}
				parent = selected->items[parent_index].parent_index;
			}
		}
		std::sort(mutation->expected_items.begin(), mutation->expected_items.end(),
			  [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		std::sort(mutation->durable_uids.begin(), mutation->durable_uids.end());
		std::sort(mutation->discarded_uids.begin(), mutation->discarded_uids.end());
		if (hostile && !mutation->pet_items.empty())
		{
			const size_t boundary_count = static_cast<size_t>(std::count_if(
				mutation->pet_items.begin(), mutation->pet_items.end(),
				[](const player_item_snapshot &item)
				{ return item.parent_index == PLAYER_SNAPSHOT_NO_PARENT; }));
			if (payload.expected_room_revision > UINT64_MAX - boundary_count - 1)
				return flatfile_world_item_result::conflict;
			selected->revision = payload.expected_room_revision + boundary_count + 1;
			selected->items = mutation->pet_items;
		}
		else
			catalog.saved_items.erase(selected);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	if (mutation->discarded_uids.empty() || mutation->discarded_uids.front() > source_uid ||
	    !std::binary_search(mutation->discarded_uids.begin(), mutation->discarded_uids.end(),
				source_uid))
		return flatfile_world_item_result::invalid;
	++catalog.revision;
	mutation->after_image.filename = catalog_filename;
	if (!encode_catalog(catalog, &mutation->after_image.bytes))
		return flatfile_world_item_result::invalid;
	mutation->catalog_revision = catalog.revision;
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result flatfile_initial_room_reset_world_storage::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_initial_room_reset_world_stage *output) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_world_item_result::invalid;
	try
	{
		zone_reset_item_image image;
		if (!zone_reset_item_recovery_initial(original) ||
		    zone_reset_item_command_decode(original.command, &image) !=
			    economic_accounting_error::ok ||
		    !image.placement || image.placement->fall_selected)
			return flatfile_world_item_result::invalid;
		for (const auto &item : image.items)
			if (item.type < ITEM_LOWEST || item.type > ITEM_LAST ||
			    item.type == ITEM_CORPSE || (item.extra_flags & ITEM_ARTIFACT))
				return flatfile_world_item_result::invalid;
		world_item_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, nullptr);
		if (loaded != flatfile_world_item_result::ok &&
		    loaded != flatfile_world_item_result::not_found)
			return loaded;
		if (!valid_catalog(catalog) || catalog.revision == UINT64_MAX)
			return flatfile_world_item_result::invalid;
		std::unordered_set<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &item : image.items)
			if (!born.insert(item.object_uid).second)
				return flatfile_world_item_result::invalid;
		const auto absent = [&](const std::vector<player_item_snapshot> &items)
		{
			return std::none_of(items.begin(), items.end(), [&](const auto &item)
					    { return born.contains(item.object_uid); });
		};
		for (const auto &corpse : catalog.corpses)
			if (!absent(corpse.items))
				return flatfile_world_item_result::conflict;
		for (const auto &saved : catalog.saved_items)
			if (!absent(saved.items))
				return flatfile_world_item_result::conflict;
		for (const auto &room : catalog.rooms)
			if (!absent(room.items))
				return flatfile_world_item_result::conflict;
		flatfile_room_item_record key{};
		key.room_vnum = image.room_vnum;
		auto room = std::lower_bound(catalog.rooms.begin(), catalog.rooms.end(), key,
					     room_less);
		const bool present = room != catalog.rooms.end() &&
				     room->room_vnum == image.room_vnum;
		const uint64_t before = present ? room->revision : 0;
		if (before != image.expected_room_revision || before == UINT64_MAX ||
		    (!present && catalog.rooms.size() >= room_maximum))
			return flatfile_world_item_result::conflict;
		flatfile_initial_room_reset_world_stage stage;
		stage.original_command = original.command;
		stage.catalog_before_present = loaded == flatfile_world_item_result::ok;
		stage.catalog_before_revision = stage.catalog_before_present ? catalog.revision : 0;
		stage.room_before_present = present;
		stage.room_revision_before = before;
		stage.room_revision_after = before + 1;
		if (present)
			stage.room_before_items = room->items;
		else
		{
			flatfile_room_item_record created{};
			created.room_vnum = image.room_vnum;
			created.revision = 1;
			room = catalog.rooms.insert(room, std::move(created));
		}
		if (room->items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - image.items.size() ||
		    room->items.size() > static_cast<size_t>(INT32_MAX) - image.items.size())
			return flatfile_world_item_result::conflict;
		auto added = image.items;
		// Original DURWRLD stores detached objects at slot -1, while the
		// canonical constructor command and custody use the original slot0.
		if (!canonicalize_detached_items(&added))
			return flatfile_world_item_result::invalid;
		const int32_t offset = static_cast<int32_t>(room->items.size());
		room->items.reserve(room->items.size() + added.size());
		for (auto &item : added)
		{
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				item.parent_index += offset;
			room->items.push_back(std::move(item));
		}
		room->revision = stage.room_revision_after;
		++catalog.revision;
		stage.catalog_revision_after = catalog.revision;
		stage.operation.store = flatfile_authority_store::domains;
		stage.operation.kind = flatfile_authority_operation_kind::write;
		stage.operation.filename = catalog_filename;
		if (!encode_catalog(catalog, &stage.operation.bytes) || !lock.matches(root))
			return flatfile_world_item_result::invalid;
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_initial_room_reset_world_stage>);
		*output = std::move(stage);
		return flatfile_world_item_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	catch (...)
	{
		return flatfile_world_item_result::invalid;
	}
}
