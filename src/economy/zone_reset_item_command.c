#include "economy/zone_reset_item_command.h"

#include "core/defines.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <bit>
#include <new>
#include <utility>

namespace
{
using error = economic_accounting_error;
constexpr std::array<uint8_t, 4> MAGIC{ 'Z', 'R', 'I', '1' };
// Magic/version/reserved, operation, source48, zone/room, season/room clocks,
// canonical item/recipe lengths and coin row count. No native struct padding.
constexpr size_t HEADER_BYTES = 108;
constexpr size_t COIN_BYTES = 40;
constexpr size_t PLACEMENT_BYTES = 32;
constexpr uint8_t LITERAL_MASK = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;

void put(uint8_t *output, uint64_t value, size_t count) noexcept
{
	for (size_t i = 0; i < count; ++i)
		output[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(const uint8_t *input, size_t count) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < count; ++i)
		value |= static_cast<uint64_t>(input[i]) << (8 * i);
	return value;
}
error codec_error(player_snapshot_codec_result result) noexcept
{
	if (result == player_snapshot_codec_result::ok)
		return error::ok;
	if (result == player_snapshot_codec_result::allocation_failure ||
	    result == player_snapshot_codec_result::limit_exceeded)
		return error::capacity;
	return error::corrupt_evidence;
}
bool add_bytes(size_t count, size_t *total) noexcept
{
	if (count > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - *total)
		return false;
	*total += count;
	return true;
}
bool add_string(const std::string &value, size_t *total) noexcept
{
	return value.size() <= PLAYER_SNAPSHOT_MAX_STRING_BYTES &&
	       value.find('\0') == std::string::npos && add_bytes(value.size(), total);
}

// Preflight the existing item/recipe wire sizes without first copying strings,
// allocating keys or encoding a potentially oversized four-MiB item list.
error image_preflight(const zone_reset_item_image &image) noexcept
{
	if (critical_operation_id_is_zero(image.operation_id) ||
	    !economic_source_event_valid(image.reset_source) ||
	    image.reset_source.kind != economic_source_kind::world_generation ||
	    image.reset_source.source.bytes != image.reset_source.generation.bytes ||
	    image.reset_source.source.bytes == image.operation_id.bytes ||
	    image.reset_source.sequence || image.zone_vnum < 0 || image.room_vnum <= 0 ||
	    !image.season_epoch || image.season_epoch == UINT64_MAX ||
	    image.expected_room_revision == UINT64_MAX || image.items.empty())
		return error::invalid_identity;
	if (image.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    image.items.size() > CRITICAL_COMMAND_MAX_KEYS - 2 ||
	    image.recipes.size() != image.items.size() || image.coins.size() > image.items.size())
		return error::capacity;
	if (image.placement && (!zone_reset_room_placement_recipe_valid(*image.placement) ||
				image.placement->root_uid != image.items.front().object_uid ||
				image.placement->room_vnum != image.room_vnum))
		return error::payload_conflict;
	size_t total = HEADER_BYTES + 4 + 12;
	if (image.placement && !add_bytes(PLACEMENT_BYTES, &total))
		return error::capacity;
	size_t rows = image.items.size(), recipe_libraries = 0;
	for (size_t i = 0; i < image.items.size(); ++i)
	{
		const auto &item = image.items[i];
		if (!item.object_uid || item.object_uid == UINT64_MAX || item.vnum < 0 ||
		    item.equipment_slot || item.string_mask != LITERAL_MASK ||
		    (i == 0 ?
			     item.parent_index != PLAYER_SNAPSHOT_NO_PARENT :
			     item.parent_index < 0 || item.parent_index >= static_cast<int32_t>(i)))
			return error::topology;
		for (size_t j = 0; j < i; ++j)
			if (image.items[j].object_uid == item.object_uid)
				return error::duplicate_event;
		if (item.dynamic_affects.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
			return error::capacity;
		rows += item.dynamic_affects.size();
		if (item.extra_descriptions.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
			return error::capacity;
		rows += item.extra_descriptions.size();
		// 221 fixed bytes includes the four string and two vector prefixes.
		if (!add_bytes(221, &total) || !add_string(item.name, &total) ||
		    !add_string(item.short_description, &total) ||
		    !add_string(item.description, &total) ||
		    !add_string(item.action_description, &total) ||
		    !add_bytes(item.dynamic_affects.size() * 12, &total))
			return error::capacity;
		for (const auto &description : item.extra_descriptions)
		{
			if (description.spell_ids.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
				return error::capacity;
			rows += description.spell_ids.size();
			if (!add_bytes(13, &total) || !add_string(description.keyword, &total) ||
			    !add_string(description.description, &total) ||
			    !add_bytes(description.spell_ids.size() * 4, &total))
				return error::capacity;
		}
		const auto &recipe = image.recipes[i];
		if (recipe.libraries.size() > PLAYER_SNAPSHOT_MAX_ROWS - recipe_libraries)
			return error::capacity;
		recipe_libraries += recipe.libraries.size();
		if (!add_bytes(28, &total) || !add_bytes(recipe.libraries.size() * 12, &total))
			return error::capacity;
		const bool money = item.type == ITEM_MONEY;
		if (money != (item.vnum == VOBJ_COINS))
			return error::payload_conflict;
		size_t matches = 0;
		for (const auto &coin : image.coins)
			if (coin.item_uid == item.object_uid)
			{
				++matches;
				if (!money)
					return error::payload_conflict;
				for (size_t denomination = 0; denomination < 4; ++denomination)
					if (item.values[denomination] < 0 ||
					    coin.denominations[denomination] !=
						    item.values[denomination])
						return error::payload_conflict;
				economic_coin_vector delta{};
				const auto status =
					economic_coin_delta({}, coin.denominations, &delta);
				if (status != error::ok)
					return status;
			}
		if (matches != static_cast<size_t>(money))
			return error::payload_conflict;
	}
	for (const auto &coin : image.coins)
		if (std::none_of(image.items.begin(), image.items.end(), [&](const auto &item)
				 { return item.object_uid == coin.item_uid; }))
			return error::payload_conflict;
	if (!add_bytes(image.coins.size() * COIN_BYTES, &total))
		return error::capacity;
	return native_mobile_birth_recipe_valid(image.items, image.recipes) ?
		       error::ok :
		       error::corrupt_evidence;
}

error metadata_check(const economic_operation_metadata &metadata,
		     const zone_reset_item_image &image)
{
	auto status = economic_operation_metadata_validate(metadata);
	if (status != error::ok)
		return status;
	if (metadata.operation_id.bytes != image.operation_id.bytes ||
	    !critical_operation_id_is_zero(metadata.original_operation_id) ||
	    metadata.actor_kind != economic_actor_kind::domain ||
	    metadata.actor_id != image.items.front().object_uid ||
	    metadata.writer_id != ECONOMIC_WRITER_ZONE_RESET_ITEM_BIRTH ||
	    metadata.reason != economic_reason::item_create || metadata.policy_version != 1 ||
	    metadata.compiler_version != 1 || !metadata.source_event)
		return error::unauthorized;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> actual{}, expected{};
	if (economic_source_event_encode(*metadata.source_event, &actual) != error::ok ||
	    economic_source_event_encode(image.reset_source, &expected) != error::ok ||
	    actual != expected)
		return error::payload_conflict;
	return error::ok;
}

error payload_encode(const zone_reset_item_image &image, std::vector<uint8_t> *output)
{
	std::vector<uint8_t> items, recipes;
	auto status = codec_error(player_item_snapshot_list_encode(image.items, &items));
	if (status != error::ok)
		return status;
	status = native_mobile_birth_recipe_encode(image.items, image.recipes, &recipes);
	if (status != error::ok)
		return status;
	size_t size = HEADER_BYTES;
	if (!add_bytes(items.size(), &size) || !add_bytes(recipes.size(), &size) ||
	    !add_bytes(image.coins.size() * COIN_BYTES, &size) ||
	    (image.placement && !add_bytes(PLACEMENT_BYTES, &size)))
		return error::capacity;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	status = economic_source_event_encode(image.reset_source, &source);
	if (status != error::ok)
		return status;
	std::vector<uint8_t> candidate(size, 0);
	std::copy(MAGIC.begin(), MAGIC.end(), candidate.begin());
	put(candidate.data() + 4,
	    image.placement ? ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION :
			      ZONE_RESET_ITEM_PAYLOAD_VERSION,
	    2);
	std::copy(image.operation_id.bytes.begin(), image.operation_id.bytes.end(),
		  candidate.begin() + 8);
	std::copy(source.begin(), source.end(), candidate.begin() + 24);
	put(candidate.data() + 72, static_cast<uint32_t>(image.zone_vnum), 4);
	put(candidate.data() + 76, static_cast<uint32_t>(image.room_vnum), 4);
	put(candidate.data() + 80, image.season_epoch, 8);
	put(candidate.data() + 88, image.expected_room_revision, 8);
	put(candidate.data() + 96, items.size(), 4);
	put(candidate.data() + 100, recipes.size(), 4);
	put(candidate.data() + 104, image.coins.size(), 4);
	std::copy(items.begin(), items.end(), candidate.begin() + HEADER_BYTES);
	std::copy(recipes.begin(), recipes.end(), candidate.begin() + HEADER_BYTES + items.size());
	size_t offset = HEADER_BYTES + items.size() + recipes.size();
	for (const auto &coin : image.coins)
	{
		put(candidate.data() + offset, coin.item_uid, 8);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			put(candidate.data() + offset + 8 + denomination * 8,
			    static_cast<uint64_t>(coin.denominations[denomination]), 8);
		offset += COIN_BYTES;
	}
	if (image.placement)
	{
		const auto &p = *image.placement;
		put(candidate.data() + offset, p.root_uid, 8);
		put(candidate.data() + offset + 8, static_cast<uint32_t>(p.room_vnum), 4);
		put(candidate.data() + offset + 12, static_cast<uint32_t>(p.original_sector_type),
		    4);
		put(candidate.data() + offset + 16, static_cast<uint32_t>(p.original_chance_fall),
		    4);
		put(candidate.data() + offset + 20, static_cast<uint32_t>(p.original_z_cord), 4);
		put(candidate.data() + offset + 24, p.fall_roll, 4);
		candidate[offset + 28] = p.original_levitates;
		candidate[offset + 29] = p.fall_roll_drawn;
		candidate[offset + 30] = p.fall_selected;
	}
	*output = std::move(candidate);
	return error::ok;
}

error payload_decode(std::span<const uint8_t> bytes, zone_reset_item_image *output)
{
	if (bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return error::capacity;
	if (bytes.size() < HEADER_BYTES || !std::equal(MAGIC.begin(), MAGIC.end(), bytes.begin()) ||
	    (get(bytes.data() + 4, 2) != ZONE_RESET_ITEM_PAYLOAD_VERSION &&
	     get(bytes.data() + 4, 2) != ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION) ||
	    get(bytes.data() + 6, 2))
		return error::corrupt_evidence;
	const bool has_placement = get(bytes.data() + 4, 2) ==
				   ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION;
	const size_t placement_size = has_placement ? PLACEMENT_BYTES : 0;
	if (bytes.size() - HEADER_BYTES < placement_size)
		return error::corrupt_evidence;
	const size_t item_size = get(bytes.data() + 96, 4),
		     recipe_size = get(bytes.data() + 100, 4);
	const size_t coin_count = get(bytes.data() + 104, 4),
		     body = bytes.size() - HEADER_BYTES - placement_size;
	if (!item_size || !recipe_size || item_size > body || recipe_size > body - item_size ||
	    coin_count > CRITICAL_COMMAND_MAX_KEYS - 2 ||
	    coin_count > (body - item_size - recipe_size) / COIN_BYTES ||
	    coin_count * COIN_BYTES != body - item_size - recipe_size)
		return error::corrupt_evidence;
	// The original item decoder is bounded itself; additionally enforce this
	// command's smaller key ceiling and minimum literal size before it allocates.
	if (item_size < 4 || recipe_size < 12)
		return error::corrupt_evidence;
	const size_t item_count = get(bytes.data() + HEADER_BYTES, 4);
	if (!item_count || item_count > CRITICAL_COMMAND_MAX_KEYS - 2 ||
	    item_count > PLAYER_SNAPSHOT_MAX_OBJECTS || item_count > (item_size - 4) / 221 ||
	    coin_count > item_count)
		return error::corrupt_evidence;
	zone_reset_item_image candidate;
	std::copy_n(bytes.begin() + 8, candidate.operation_id.bytes.size(),
		    candidate.operation_id.bytes.begin());
	auto status = economic_source_event_decode(bytes.subspan(24, ECONOMIC_SOURCE_EVENT_BYTES),
						   &candidate.reset_source);
	if (status != error::ok)
		return status;
	candidate.zone_vnum =
		std::bit_cast<int32_t>(static_cast<uint32_t>(get(bytes.data() + 72, 4)));
	candidate.room_vnum =
		std::bit_cast<int32_t>(static_cast<uint32_t>(get(bytes.data() + 76, 4)));
	candidate.season_epoch = get(bytes.data() + 80, 8);
	candidate.expected_room_revision = get(bytes.data() + 88, 8);
	status = codec_error(player_item_snapshot_list_decode(bytes.data() + HEADER_BYTES,
							      item_size, &candidate.items));
	if (status != error::ok)
		return status;
	status = native_mobile_birth_recipe_decode(bytes.subspan(HEADER_BYTES + item_size,
								 recipe_size),
						   candidate.items, &candidate.recipes);
	if (status != error::ok)
		return status;
	candidate.coins.reserve(coin_count);
	size_t offset = HEADER_BYTES + item_size + recipe_size;
	for (size_t i = 0; i < coin_count; ++i)
	{
		zone_reset_coin_output coin;
		coin.item_uid = get(bytes.data() + offset, 8);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			coin.denominations[denomination] = std::bit_cast<int64_t>(
				get(bytes.data() + offset + 8 + denomination * 8, 8));
		candidate.coins.push_back(coin);
		offset += COIN_BYTES;
	}
	if (has_placement)
	{
		if (bytes[offset + 28] > 1 || bytes[offset + 29] > 1 || bytes[offset + 30] > 1 ||
		    bytes[offset + 31])
			return error::corrupt_evidence;
		zone_reset_room_placement_recipe p;
		p.root_uid = get(bytes.data() + offset, 8);
		p.room_vnum = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 8, 4)));
		p.original_sector_type = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 12, 4)));
		p.original_chance_fall = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 16, 4)));
		p.original_z_cord = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 20, 4)));
		p.fall_roll = static_cast<uint32_t>(get(bytes.data() + offset + 24, 4));
		p.original_levitates = bytes[offset + 28];
		p.fall_roll_drawn = bytes[offset + 29];
		p.fall_selected = bytes[offset + 30];
		candidate.placement = p;
	}
	status = image_preflight(candidate);
	if (status != error::ok)
		return status;
	*output = std::move(candidate);
	return error::ok;
}
}

bool zone_reset_room_placement_recipe_valid(const zone_reset_room_placement_recipe &p) noexcept
{
	if (!p.root_uid || p.root_uid == UINT64_MAX || p.room_vnum <= 0 ||
	    p.original_sector_type < 0 || p.original_sector_type >= NUM_SECT_TYPES ||
	    p.original_chance_fall < INT8_MIN || p.original_chance_fall > INT8_MAX ||
	    p.original_z_cord < INT16_MIN || p.original_z_cord > INT16_MAX)
		return false;
	const bool no_ground = p.original_sector_type == SECT_NO_GROUND ||
			       p.original_sector_type == SECT_UNDRWLD_NOGROUND;
	const bool drawn = !p.original_levitates && !no_ground;
	if (p.fall_roll_drawn != drawn ||
	    (drawn ? p.fall_roll < 1 || p.fall_roll > 100 : p.fall_roll != 0))
		return false;
	const bool selected = !p.original_levitates &&
			      (no_ground ||
			       p.original_chance_fall >= static_cast<int32_t>(p.fall_roll) ||
			       p.original_z_cord > 0);
	return p.fall_selected == selected;
}

economic_accounting_error zone_reset_item_command_build(const economic_operation_metadata &metadata,
							const zone_reset_item_image &image,
							uint64_t accepted_at_usec,
							critical_command *output) noexcept
{
	if (!output || !accepted_at_usec)
		return error::invalid_identity;
	try
	{
		auto status = image_preflight(image);
		if (status != error::ok)
			return status;
		status = metadata_check(metadata, image);
		if (status != error::ok)
			return status;
		critical_command candidate{};
		candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		candidate.operation_id = image.operation_id;
		candidate.type = critical_command_type::zone_reset_item_birth;
		candidate.payload_version = image.placement ?
						    ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION :
						    ZONE_RESET_ITEM_PAYLOAD_VERSION;
		candidate.source_site = critical_source_site::zone_event;
		candidate.deadline_class = critical_deadline_class::background;
		candidate.accepted_at_usec = accepted_at_usec;
		status = payload_encode(image, &candidate.payload);
		if (status != error::ok)
			return status;
		candidate.keys.reserve(image.items.size() + 2);
		candidate.expected_revisions.reserve(image.items.size() + 1);
		candidate.keys.push_back(
			{ critical_entity_type::zone, static_cast<uint64_t>(image.zone_vnum) + 1 });
		candidate.keys.push_back(
			{ critical_entity_type::room, static_cast<uint64_t>(image.room_vnum) });
		candidate.expected_revisions.push_back(
			{ candidate.keys.back(), image.expected_room_revision });
		for (const auto &item : image.items)
		{
			candidate.keys.push_back({ critical_entity_type::item, item.object_uid });
			candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		}
		std::sort(candidate.keys.begin(), candidate.keys.end(), critical_entity_key_less);
		std::sort(candidate.expected_revisions.begin(), candidate.expected_revisions.end(),
			  [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		economic_admission_facts facts;
		facts.metadata = metadata;
		status = economic_intent_freeze(candidate, facts, &candidate.accounting_intent);
		if (status != error::ok)
			return status;
		candidate.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		candidate.publication_required = true;
		if (!critical_command_envelope_valid(candidate))
			return error::corrupt_evidence;
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error zone_reset_item_command_decode(const critical_command &command,
							 zone_reset_item_image *output) noexcept
{
	if (!output)
		return error::invalid_identity;
	try
	{
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    command.type != critical_command_type::zone_reset_item_birth ||
		    (command.payload_version != ZONE_RESET_ITEM_PAYLOAD_VERSION &&
		     command.payload_version != ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION) ||
		    !command.publication_required || !critical_command_envelope_valid(command))
			return error::corrupt_evidence;
		zone_reset_item_image candidate;
		auto status = payload_decode(command.payload, &candidate);
		if (status != error::ok)
			return status;
		economic_frozen_intent intent;
		status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		if (intent.admission.facts_version != 1 || !intent.admission.facts.empty())
			return error::payload_conflict;
		status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		critical_command expected;
		status = zone_reset_item_command_build(intent.admission.metadata, candidate,
						       command.accepted_at_usec, &expected);
		if (status != error::ok)
			return status;
		std::vector<uint8_t> actual_bytes, expected_bytes;
		if (critical_command_encode(command, &actual_bytes) !=
			    critical_command_codec_result::ok ||
		    critical_command_encode(expected, &expected_bytes) !=
			    critical_command_codec_result::ok)
			return error::capacity;
		if (actual_bytes != expected_bytes)
			return error::payload_conflict;
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}


namespace
{
struct room_command_bound_workspace
{
	player_item_snapshot_list_allocation_profile items;
	native_mobile_birth_recipe_allocation_profile recipes;
	size_t peak = 0, phase = 0, payload = 0, key_bytes = 0, revision_bytes = 0;
	size_t command_heap = 0, command_wire = 0, item_working = 0;
	size_t binding_capacity = 0, binding_peak = 0, domain_bytes = 0;
	size_t domain_capacity = 0, domain_peak = 0;
};
[[maybe_unused]] bool room_bound_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
[[maybe_unused]] bool room_bound_array(size_t count, size_t unit, size_t *out) noexcept
{
	if (!out || (unit && count > SIZE_MAX / unit))
		return false;
	*out = count * unit;
	return true;
}
// Original vectors are fresh with capacity==size. libstdc++13 insertion's
// _M_check_len grows by max(size, inserted_count), with old+new coexistence.
[[maybe_unused]] bool room_bound_prepend(size_t size, size_t tag, size_t *capacity, size_t *peak) noexcept
{
	*capacity = size;
	if (!room_bound_add(*capacity, std::max(size, tag)))
		return false;
	*peak = size;
	return room_bound_add(*peak, *capacity);
}
} // namespace

economic_accounting_error zone_reset_item_command_build_bounded(
	const economic_operation_metadata &metadata, const zone_reset_item_image &image,
	uint64_t accepted_at_usec, critical_command *output,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
	if (!output || !accepted_at_usec)
		return error::invalid_identity;
	if (!reserve_scratch_peak)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)metadata;
	(void)image;
	(void)context;
	(void)outer_live_scratch;
	return error::capacity;
#else
	// Wrapper profiles stay live across the original compiler. Each allocation-
	// free validation/scan phase is admitted before entering its callee.
	size_t base = outer_live_scratch;
	if (!room_bound_add(base, sizeof(room_command_bound_workspace)))
		return error::capacity;
	size_t validation = std::max(
		player_item_snapshot_list_encoder_preflight_object_bytes(),
		native_mobile_birth_recipe_profile_inline_storage_bytes());
	validation = std::max(validation,
		 sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) +
		 sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>));
	size_t preliminary = base;
	if (!room_bound_add(preliminary, validation) ||
	    !reserve_scratch_peak(preliminary, context))
		return error::capacity;
	auto status = image_preflight(image);
	if (status != error::ok)
		return status;
	status = metadata_check(metadata, image);
	if (status != error::ok)
		return status;
	room_command_bound_workspace work;
	const auto item_status = player_item_snapshot_list_encoder_preflight(image.items, &work.items);
	if (item_status != player_snapshot_codec_result::ok)
		return codec_error(item_status);
	status = native_mobile_birth_recipe_encode_profile(image.items, image.recipes, &work.recipes);
	if (status != error::ok)
		return status;
	if (!work.recipes.fresh_encode_storage_policy_supported ||
	    !player_item_snapshot_list_encoder_working_bytes(work.items, &work.item_working))
		return error::capacity;
	work.payload = HEADER_BYTES;
	size_t coin_bytes = 0;
	if (!room_bound_array(image.coins.size(), COIN_BYTES, &coin_bytes) ||
	    !room_bound_add(work.payload, work.items.canonical_encoded_bytes) ||
	    !room_bound_add(work.payload, work.recipes.wire_bytes) ||
	    !room_bound_add(work.payload, coin_bytes) ||
	    (image.placement && !room_bound_add(work.payload, PLACEMENT_BYTES)) ||
	    work.payload > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    !room_bound_array(image.items.size() + 2, sizeof(critical_entity_key), &work.key_bytes) ||
	    !room_bound_array(image.items.size() + 1, sizeof(critical_expected_revision),
			      &work.revision_bytes))
		return error::capacity;
	// Every following phase includes the original command candidate. The two
	// payload vectors survive item encoding, recipe encoding and payload assembly.
	if (!room_bound_add(base, sizeof(critical_command)))
		return error::capacity;
	work.peak = preliminary;
	work.phase = base;
	if (!room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, work.item_working))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	work.phase = base;
	size_t recipe_working = work.recipes.encoder_inline_storage_bytes;
	if (!room_bound_add(recipe_working, work.recipes.encoded_capacity_bytes))
		return error::capacity;
	recipe_working = std::max(recipe_working, work.recipes.validation_inline_storage_bytes);
	if (!room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, work.items.canonical_encoded_capacity_bytes) ||
	    !room_bound_add(work.phase, recipe_working))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	work.phase = base;
	if (!room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)) ||
	    !room_bound_add(work.phase, work.items.canonical_encoded_capacity_bytes) ||
	    !room_bound_add(work.phase, work.recipes.encoded_capacity_bytes) ||
	    !room_bound_add(work.phase, work.payload))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	// Payload temporary vectors are now destroyed. Keys/revisions reserve once.
	work.command_heap = work.payload;
	if (!room_bound_add(work.command_heap, work.key_bytes) ||
	    !room_bound_add(work.command_heap, work.revision_bytes))
		return error::capacity;
	work.command_wire = CRITICAL_COMMAND_HEADER_BYTES;
	size_t key_wire = 0, revision_wire = 0;
	if (!room_bound_array(image.items.size() + 2, CRITICAL_COMMAND_ENTITY_KEY_BYTES, &key_wire) ||
	    !room_bound_array(image.items.size() + 1, CRITICAL_COMMAND_EXPECTED_REVISION_BYTES,
			      &revision_wire) ||
	    !room_bound_add(work.command_wire, key_wire) ||
	    !room_bound_add(work.command_wire, revision_wire) ||
	    !room_bound_add(work.command_wire, work.payload) ||
	    work.command_wire > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    !room_bound_prepend(work.command_wire, sizeof("DURIS-ECONOMIC-COMMAND-V1"),
				&work.binding_capacity, &work.binding_peak))
		return error::capacity;
	// Freeze's facts and empty frozen intent coexist with the original candidate.
	if (!room_bound_add(base, sizeof(economic_admission_facts)) ||
	    !room_bound_add(base, sizeof(economic_frozen_intent)) ||
	    !room_bound_add(base, work.command_heap))
		return error::capacity;
	// Binding projection owns exact copies of all candidate vector requests.
	work.phase = base;
	if (!room_bound_add(work.phase, sizeof(critical_command)) ||
	    !room_bound_add(work.phase, work.command_heap) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, work.command_wire))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	size_t binding_digest_live = work.binding_capacity;
	if (!room_bound_add(binding_digest_live, sizeof(economic_digest)))
		return error::capacity;
	work.phase = base;
	if (!room_bound_add(work.phase, sizeof(critical_command)) ||
	    !room_bound_add(work.phase, work.command_heap) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, std::max(work.binding_peak, binding_digest_live)))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	// Binding projection is gone before domain hashing. The source byte vector
	// and moved hash parameter objects coexist; old+new storage only at prepend.
	work.domain_bytes = work.payload;
	if (!room_bound_add(work.domain_bytes, 8) ||
	    !room_bound_prepend(work.domain_bytes, sizeof("DURIS-ECONOMIC-DOMAIN-V1"),
				&work.domain_capacity, &work.domain_peak))
		return error::capacity;
	// Hash local result and returned digest may be distinct without NRVO.
	size_t domain_digest_live = work.domain_capacity;
	if (!room_bound_add(domain_digest_live, sizeof(economic_digest)) ||
	    !room_bound_add(domain_digest_live, sizeof(economic_digest)))
		return error::capacity;
	work.phase = base;
	if (!room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, std::max(work.domain_peak, domain_digest_live)))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	// Empty facts produce the exact 256-byte intent. Source encoding occurs
	// while its fresh bytes, caller source array and encoder result remain live.
	work.phase = base;
	if (!room_bound_add(work.phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(work.phase, ECONOMIC_INTENT_HEADER_BYTES) ||
	    !room_bound_add(work.phase,
			    2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)))
		return error::capacity;
	work.peak = std::max(work.peak, work.phase);
	if (!reserve_scratch_peak(work.peak, context))
		return error::capacity;
	// All original semantics, wire tags, timestamps and strong output behavior
	// stay in the original compiler. Reservation is not source/admission proof.
	return zone_reset_item_command_build(metadata, image, accepted_at_usec, output);
#endif
}


namespace
{
struct room_decode_workspace
{
	zone_reset_item_image image;
	economic_frozen_intent intent;
	critical_command expected;
	std::vector<uint8_t> actual_bytes, expected_bytes;
	std::span<const uint8_t> payload_bytes;
	size_t image_heap = 0, command_heap = 0, projection_wire = 0;
	size_t binding_capacity = 0, binding_peak = 0;
	size_t domain_capacity = 0, domain_peak = 0, expected_heap = 0;
};

// Empty facts are rejected only after the original decoder validates the full
// intent. Reserve its exact potential suffix and named source/result DTOs first.
[[maybe_unused]] bool room_intent_decode_peak(const critical_command &command,
	size_t live, size_t *out) noexcept
{
	if (!room_bound_add(live, sizeof(economic_frozen_intent)) ||
	    !room_bound_add(live, sizeof(economic_source_event)) ||
	    !room_bound_add(live, economic_source_event_decode_object_bytes()) ||
	    !room_bound_add(live, 2 * sizeof(std::span<const uint8_t>)) ||
	    (command.accounting_intent.size() > ECONOMIC_INTENT_HEADER_BYTES &&
	     !room_bound_add(live,
			     command.accounting_intent.size() - ECONOMIC_INTENT_HEADER_BYTES)))
		return false;
	*out = live;
	return true;
}

[[maybe_unused]] bool room_intent_verify_peak(const critical_command &command,
	room_decode_workspace &work, size_t live, size_t *out) noexcept
{
	size_t keys = 0, revisions = 0;
	if (!room_bound_array(command.keys.size(), sizeof(critical_entity_key), &keys) ||
	    !room_bound_array(command.expected_revisions.size(), sizeof(critical_expected_revision),
			      &revisions))
		return false;
	work.command_heap = command.payload.size();
	if (!room_bound_add(work.command_heap, command.accounting_intent.size()) ||
	    !room_bound_add(work.command_heap, keys) || !room_bound_add(work.command_heap, revisions))
		return false;
	// Projection.clear() retains the copied accounting-intent allocation.
	work.projection_wire = CRITICAL_COMMAND_HEADER_BYTES;
	if (!room_bound_array(command.keys.size(), CRITICAL_COMMAND_ENTITY_KEY_BYTES, &keys) ||
	    !room_bound_array(command.expected_revisions.size(), CRITICAL_COMMAND_EXPECTED_REVISION_BYTES,
			      &revisions) ||
	    !room_bound_add(work.projection_wire, keys) ||
	    !room_bound_add(work.projection_wire, revisions) ||
	    !room_bound_add(work.projection_wire, command.payload.size()) ||
	    !room_bound_prepend(work.projection_wire, sizeof("DURIS-ECONOMIC-COMMAND-V1"),
				&work.binding_capacity, &work.binding_peak))
		return false;
	// Canonical intent comparison's bytes die before binding/domain hashing.
	size_t peak = live, phase = live;
	if (!room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, ECONOMIC_INTENT_HEADER_BYTES) ||
	    !room_bound_add(phase, 2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)))
		return false;
	peak = std::max(peak, phase);
	// verify_binding's binding result remains live across both hash helpers.
	if (!room_bound_add(live, sizeof(economic_digest)))
		return false;
	phase = live;
	if (!room_bound_add(phase, sizeof(critical_command)) ||
	    !room_bound_add(phase, work.command_heap) ||
	    !room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, work.projection_wire))
		return false;
	peak = std::max(peak, phase);
	size_t digest_live = work.binding_capacity;
	if (!room_bound_add(digest_live, sizeof(economic_digest)))
		return false;
	phase = live;
	if (!room_bound_add(phase, sizeof(critical_command)) ||
	    !room_bound_add(phase, work.command_heap) ||
	    !room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, std::max(work.binding_peak, digest_live)))
		return false;
	peak = std::max(peak, phase);
	size_t domain = command.payload.size();
	if (!room_bound_add(domain, 8) ||
	    !room_bound_prepend(domain, sizeof("DURIS-ECONOMIC-DOMAIN-V1"),
				&work.domain_capacity, &work.domain_peak))
		return false;
	digest_live = work.domain_capacity;
	if (!room_bound_add(digest_live, sizeof(economic_digest)) ||
	    !room_bound_add(digest_live, sizeof(economic_digest)))
		return false;
	phase = live;
	if (!room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(phase, std::max(work.domain_peak, digest_live)))
		return false;
	*out = std::max(peak, phase);
	return true;
}

[[maybe_unused]] error payload_decode_bounded(const std::span<const uint8_t> &bytes,
					      zone_reset_item_image *output,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer_live,
					      size_t *retained_heap)
{
	if (bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return error::capacity;
	if (bytes.size() < HEADER_BYTES || !std::equal(MAGIC.begin(), MAGIC.end(), bytes.begin()) ||
	    (get(bytes.data() + 4, 2) != ZONE_RESET_ITEM_PAYLOAD_VERSION &&
	     get(bytes.data() + 4, 2) != ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION) ||
	    get(bytes.data() + 6, 2))
		return error::corrupt_evidence;
	const bool has_placement = get(bytes.data() + 4, 2) ==
				   ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION;
	const size_t placement_size = has_placement ? PLACEMENT_BYTES : 0;
	if (bytes.size() - HEADER_BYTES < placement_size)
		return error::corrupt_evidence;
	const size_t item_size = get(bytes.data() + 96, 4),
		     recipe_size = get(bytes.data() + 100, 4);
	const size_t coin_count = get(bytes.data() + 104, 4),
		     body = bytes.size() - HEADER_BYTES - placement_size;
	if (!item_size || !recipe_size || item_size > body || recipe_size > body - item_size ||
	    coin_count > CRITICAL_COMMAND_MAX_KEYS - 2 ||
	    coin_count > (body - item_size - recipe_size) / COIN_BYTES ||
	    coin_count * COIN_BYTES != body - item_size - recipe_size)
		return error::corrupt_evidence;
	// The original item decoder is bounded itself; additionally enforce this
	// command's smaller key ceiling and minimum literal size before it allocates.
	if (item_size < 4 || recipe_size < 12)
		return error::corrupt_evidence;
	const size_t item_count = get(bytes.data() + HEADER_BYTES, 4);
	if (!item_count || item_count > CRITICAL_COMMAND_MAX_KEYS - 2 ||
	    item_count > PLAYER_SNAPSHOT_MAX_OBJECTS || item_count > (item_size - 4) / 221 ||
	    coin_count > item_count)
		return error::corrupt_evidence;

	size_t live = outer_live;
	if (!room_bound_add(live, sizeof(zone_reset_item_image)) ||
	    !room_bound_add(live, sizeof(player_item_snapshot_list_allocation_profile)) ||
	    !room_bound_add(live, sizeof(native_mobile_birth_recipe_allocation_profile)))
		return error::capacity;
	size_t preliminary = live;
	const size_t scan = std::max(player_item_snapshot_list_preflight_object_bytes(),
				     economic_source_event_decode_object_bytes() +
					     sizeof(std::span<const uint8_t>));
	if (!room_bound_add(preliminary, scan) || !reserve(preliminary, context))
		return error::capacity;
	zone_reset_item_image candidate;
	player_item_snapshot_list_allocation_profile items;
	native_mobile_birth_recipe_allocation_profile recipes;
	std::copy_n(bytes.begin() + 8, candidate.operation_id.bytes.size(),
		    candidate.operation_id.bytes.begin());
	auto status = economic_source_event_decode(bytes.subspan(24, ECONOMIC_SOURCE_EVENT_BYTES),
						   &candidate.reset_source);
	if (status != error::ok)
		return status;
	candidate.zone_vnum =
		std::bit_cast<int32_t>(static_cast<uint32_t>(get(bytes.data() + 72, 4)));
	candidate.room_vnum =
		std::bit_cast<int32_t>(static_cast<uint32_t>(get(bytes.data() + 76, 4)));
	candidate.season_epoch = get(bytes.data() + 80, 8);
	candidate.expected_room_revision = get(bytes.data() + 88, 8);

	const auto scanned = player_item_snapshot_list_preflight(
		bytes.data() + HEADER_BYTES, item_size, &items);
	if (scanned != player_snapshot_codec_result::ok)
		return codec_error(scanned);
	if (!items.fresh_decode_storage_policy_supported ||
	    items.decoded_payload_bytes < sizeof(std::vector<player_item_snapshot>))
		return error::capacity;
	size_t peak = live;
	if (!room_bound_add(peak, items.item_codec_decoder_object_bytes) ||
	    !room_bound_add(peak, items.decoded_payload_bytes) ||
	    !room_bound_add(peak, items.relationship_scratch_bytes) || !reserve(peak, context))
		return error::capacity;
	status = codec_error(player_item_snapshot_list_decode(bytes.data() + HEADER_BYTES,
							      item_size, &candidate.items));
	if (status != error::ok)
		return status;

	// The literal decoder is admitted first. Full recipe validation needs those
	// actual descriptors and runs before any recipe/library allocation.
	const size_t item_heap = items.decoded_payload_bytes -
		sizeof(std::vector<player_item_snapshot>);
	peak = live;
	if (!room_bound_add(peak, item_heap) ||
	    !room_bound_add(peak, native_mobile_birth_recipe_profile_inline_storage_bytes()) ||
	    !reserve(peak, context))
		return error::capacity;
	status = native_mobile_birth_recipe_decode_profile(
		bytes.subspan(HEADER_BYTES + item_size, recipe_size), candidate.items, &recipes);
	if (status != error::ok)
		return status;
	if (!recipes.fresh_decode_storage_policy_supported)
		return error::capacity;
	size_t recipe_working = recipes.decoder_inline_storage_bytes;
	if (!room_bound_add(recipe_working, recipes.decoded_payload_bytes))
		return error::capacity;
	recipe_working = std::max(recipe_working, recipes.preflight_inline_storage_bytes);
	peak = live;
	if (!room_bound_add(peak, item_heap) || !room_bound_add(peak, recipe_working) ||
	    !reserve(peak, context))
		return error::capacity;
	status = native_mobile_birth_recipe_decode(bytes.subspan(HEADER_BYTES + item_size,
								 recipe_size),
						   candidate.items, &candidate.recipes);
	if (status != error::ok)
		return status;

	size_t coins_heap = 0, result_heap = item_heap;
	if (!room_bound_array(coin_count, sizeof(zone_reset_coin_output), &coins_heap) ||
	    !room_bound_add(result_heap, recipes.decoded_payload_bytes) ||
	    !room_bound_add(result_heap, coins_heap))
		return error::capacity;
	peak = live;
	const size_t value_working = std::max(sizeof(zone_reset_coin_output),
		sizeof(zone_reset_room_placement_recipe));
	if (!room_bound_add(peak, result_heap) ||
	    !room_bound_add(peak, std::max(value_working, recipes.validation_inline_storage_bytes)) ||
	    !reserve(peak, context))
		return error::capacity;
	candidate.coins.reserve(coin_count);
	size_t offset = HEADER_BYTES + item_size + recipe_size;
	for (size_t i = 0; i < coin_count; ++i)
	{
		zone_reset_coin_output coin;
		coin.item_uid = get(bytes.data() + offset, 8);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			coin.denominations[denomination] = std::bit_cast<int64_t>(
				get(bytes.data() + offset + 8 + denomination * 8, 8));
		candidate.coins.push_back(coin);
		offset += COIN_BYTES;
	}
	if (has_placement)
	{
		if (bytes[offset + 28] > 1 || bytes[offset + 29] > 1 || bytes[offset + 30] > 1 ||
		    bytes[offset + 31])
			return error::corrupt_evidence;
		zone_reset_room_placement_recipe p;
		p.root_uid = get(bytes.data() + offset, 8);
		p.room_vnum = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 8, 4)));
		p.original_sector_type = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 12, 4)));
		p.original_chance_fall = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 16, 4)));
		p.original_z_cord = std::bit_cast<int32_t>(
			static_cast<uint32_t>(get(bytes.data() + offset + 20, 4)));
		p.fall_roll = static_cast<uint32_t>(get(bytes.data() + offset + 24, 4));
		p.original_levitates = bytes[offset + 28];
		p.fall_roll_drawn = bytes[offset + 29];
		p.fall_selected = bytes[offset + 30];
		candidate.placement = p;
	}
	status = image_preflight(candidate);
	if (status != error::ok)
		return status;
	*retained_heap = result_heap;
	*output = std::move(candidate);
	return error::ok;
}

} // namespace

economic_accounting_error zone_reset_item_command_decode_bounded(
	const critical_command &command, zone_reset_item_image *output,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
	if (!output)
		return error::invalid_identity;
	if (!reserve_scratch_peak)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)context;
	(void)outer_live_scratch;
	return error::capacity;
#else
	try
	{
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    command.type != critical_command_type::zone_reset_item_birth ||
		    (command.payload_version != ZONE_RESET_ITEM_PAYLOAD_VERSION &&
		     command.payload_version != ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION) ||
		    !command.publication_required || !critical_command_envelope_valid(command))
			return error::corrupt_evidence;
		size_t live = outer_live_scratch;
		if (!room_bound_add(live, sizeof(room_decode_workspace)) ||
		    !reserve_scratch_peak(live, context))
			return error::capacity;
		room_decode_workspace work;
		work.payload_bytes = command.payload;
		auto status = payload_decode_bounded(work.payload_bytes, &work.image,
						     reserve_scratch_peak, context, live,
						     &work.image_heap);
		if (status != error::ok)
			return status;
		if (!room_bound_add(live, work.image_heap))
			return error::capacity;
		size_t peak = 0;
		if (!room_intent_decode_peak(command, live, &peak) ||
		    !reserve_scratch_peak(peak, context))
			return error::capacity;
		status = economic_intent_decode(command.accounting_intent, &work.intent);
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 || !work.intent.admission.facts.empty())
			return error::payload_conflict;
		if (!room_intent_verify_peak(command, work, live, &peak) ||
		    !reserve_scratch_peak(peak, context))
			return error::capacity;
		status = economic_intent_verify_binding(command, work.intent);
		if (status != error::ok)
			return status;
		// Reuse the genuine bounded compiler with the decoded image/intent still
		// live; do not duplicate its allocation arithmetic or execute a probe build.
		status = zone_reset_item_command_build_bounded(work.intent.admission.metadata,
			work.image, command.accepted_at_usec, &work.expected,
			reserve_scratch_peak, context, live);
		if (status != error::ok)
			return status;
		size_t keys = 0, revisions = 0;
		if (!room_bound_array(work.expected.keys.capacity(), sizeof(critical_entity_key), &keys) ||
		    !room_bound_array(work.expected.expected_revisions.capacity(),
			 sizeof(critical_expected_revision), &revisions))
			return error::capacity;
		work.expected_heap = work.expected.payload.capacity();
		if (!room_bound_add(work.expected_heap, work.expected.accounting_intent.capacity()) ||
		    !room_bound_add(work.expected_heap, keys) ||
		    !room_bound_add(work.expected_heap, revisions) ||
		    !room_bound_add(live, work.expected_heap))
			return error::capacity;
		if (critical_command_encode_bounded(command, &work.actual_bytes,
			reserve_scratch_peak, context, live) != critical_command_codec_result::ok)
			return error::capacity;
		if (!room_bound_add(live, work.actual_bytes.capacity()))
			return error::capacity;
		if (critical_command_encode_bounded(work.expected, &work.expected_bytes,
			reserve_scratch_peak, context, live) != critical_command_codec_result::ok)
			return error::capacity;
		if (work.actual_bytes != work.expected_bytes)
			return error::payload_conflict;
		*output = std::move(work.image);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}

#include <type_traits>

namespace
{
constexpr size_t room_value_P = sizeof(void *);
constexpr size_t room_value_N = sizeof(size_t);
template <class T> constexpr size_t room_value_vector_cleanup_source() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	constexpr size_t destruction = sizeof(V *) + sizeof(V *) + sizeof(A *) + 2 * sizeof(T *) +
				       sizeof(A *) + 2 * sizeof(T *) + 2 * sizeof(T *) +
				       sizeof(bool);
	constexpr size_t element = std::is_trivially_destructible_v<T> ? 0 : 4 * sizeof(T *);
	constexpr size_t deallocation =
		sizeof(void *) + sizeof(V *) + sizeof(T *) + sizeof(size_t) + sizeof(A *) +
		sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) + sizeof(size_t) +
		sizeof(A *) + sizeof(T *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
		sizeof(bool) + sizeof(A *);
	// _Vector_base's member/base cleanup really reaches implicit ~_Vector_impl,
	// ~_Vector_impl_data and ~__new_allocator, each with its own this carrier.
	return destruction + element + deallocation + 3 * room_value_P;
}

template <class T> constexpr size_t room_value_vector_default_source() noexcept
{
	// vector, _Vector_base, _Vector_impl, allocator, __new_allocator and
	// _Vector_impl_data default constructors: six genuine this carriers.
	return 6 * room_value_P;
}
template <class T> constexpr size_t room_value_vector_get_allocator_source() noexcept
{
	// _Vector_base::get_allocator(this); _M_get_Tp_allocator(this,returned-ref);
	// allocator(const&) and __new_allocator(const&): this/source for each.
	// The returned allocator value is not a member of the caller vector.
	return 7 * room_value_P + sizeof(std::allocator<T>);
}
template <class T> constexpr size_t room_value_vector_const_allocator_ctor_source() noexcept
{
	// vector(alloc), _Vector_base(alloc), _Vector_impl(alloc), allocator copy,
	// new_allocator copy each this/source; _Vector_impl_data default this.
	return 11 * room_value_P;
}
template <class T> constexpr size_t room_value_vector_data_swap_source() noexcept
{
	// _M_swap_data(this,source), its real three-pointer __tmp, data default
	// ctor(this), three _M_copy_data(this,source) calls, implicit tmp dtor(this).
	return 2 * room_value_P + 3 * sizeof(T *) + room_value_P + 3 * (2 * room_value_P) +
	       room_value_P;
}
template <class T> constexpr size_t room_value_vector_move_constructor_source() noexcept
{
	// Defaulted vector/base moves, impl move, allocator/new_allocator const
	// copies, data move: six this/source pairs. Impl makes two std::move calls
	// (reference/result each); data move's pointer() null-reset result is real.
	return 6 * (2 * room_value_P) + 2 * (2 * room_value_P) + sizeof(T *);
}
template <class T> constexpr size_t room_value_vector_move_assignment_source() noexcept
{
	// operator=(this,source,returned-ref), named constexpr __move_storage,
	// _S_propagate_on_move_assign bool result (true short-circuits _S_always_equal),
	// std::move(ref,result), _M_move_assign(this,source,actual true_type value),
	// generated true_type ctor/dtor this. The actual __tmp vector is separate
	// from input/output vectors. get_allocator's value dies via allocator and
	// new_allocator dtors after __tmp's const-allocator construction.
	constexpr size_t entry = 3 * room_value_P + 2 * sizeof(bool) + 2 * room_value_P +
				 2 * room_value_P + sizeof(std::true_type) + 2 * room_value_P;
	// C++20 __alloc_on_move(one,two), std::move(ref,result), generated allocator
	// assignment(this,source,returned-ref), generated new_allocator assignment
	// (this,source,returned-ref):10P, plus both _M_get_Tp_allocator(this,ref):4P.
	constexpr size_t allocator_move = 10 * room_value_P + 2 * (2 * room_value_P);
	return entry + sizeof(std::vector<T>) + room_value_vector_get_allocator_source<T>() +
	       2 * room_value_P + room_value_vector_const_allocator_ctor_source<T>() +
	       2 * room_value_vector_data_swap_source<T>() + allocator_move +
	       room_value_vector_cleanup_source<T>();
}

bool room_value_source_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
} // namespace

bool zone_reset_item_image_lifetime_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	// Exact genuine selected GNU13 value closure. Heap/capacities and actual
	// image/old-output inline objects are separately owned by the caller.
	static_assert(std::is_trivially_destructible_v<zone_reset_coin_output>);
	static_assert(std::is_trivially_copy_constructible_v<zone_reset_room_placement_recipe>);
	static_assert(std::is_trivially_move_assignable_v<zone_reset_room_placement_recipe>);
	static_assert(std::is_trivially_destructible_v<zone_reset_room_placement_recipe>);
	size_t items = 0, recipes = 0,
	       total =
		       // Generated image/scalar aggregate and trivial optional receiver graph:
	       // default9P, move-assignment45P, cleanup15P; no row moves or active
	       // Recipe/_Empty_byte destructor from the trivial optional union.
	       69 * room_value_P + room_value_vector_default_source<zone_reset_coin_output>() +
	       room_value_vector_move_assignment_source<zone_reset_coin_output>() +
	       room_value_vector_cleanup_source<zone_reset_coin_output>();
	if (!player_item_snapshot_list_lifetime_source_frame_bytes(&items) ||
	    !native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&recipes) ||
	    sizeof(std::allocator<zone_reset_coin_output>) != 1 ||
	    sizeof(std::vector<zone_reset_coin_output>) != 3 * sizeof(void *) ||
	    !room_value_source_add(total, items) || !room_value_source_add(total, recipes))
		return false;
	*output = total;
	return true;
}

namespace
{
constexpr size_t room_fixed_P = sizeof(void *);
constexpr size_t room_fixed_N = sizeof(size_t);
constexpr size_t room_fixed_B = sizeof(bool);
constexpr size_t room_fixed_E = sizeof(economic_accounting_error);
using room_fixed_source_query = bool (*)(size_t *) noexcept;
constexpr size_t room_fixed_child_preflight_frames =
	// Its actual parameters/locals/returned error only. The shared checked
	// add/admit leaves already belong to the retained ROOM control envelope.
	6 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(economic_accounting_error);
constexpr size_t room_fixed_freeze_wrapper_source =
	5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(economic_accounting_error);
constexpr size_t room_fixed_verify_wrapper_source =
	4 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(economic_accounting_error);
template <class T> constexpr size_t room_fixed_reserved_push_source() noexcept
{
	// push_back(this,rvalue-ref) -> std::move(ref,result); emplace_back(this,
	// arg-ref,returned-ref) -> forward(ref,result); allocator_traits::construct
	// (allocator-ref,location,arg-ref) -> forward; construct_at(location,arg-ref,
	// returned-pointer) -> forward; actual placement-new(size,where,result).
	constexpr size_t construction = 2 * room_fixed_P + 2 * room_fixed_P + 3 * room_fixed_P +
					2 * room_fixed_P + 3 * room_fixed_P + 2 * room_fixed_P +
					3 * room_fixed_P + 2 * room_fixed_P + room_fixed_N +
					2 * room_fixed_P;
	// C++20 emplace return -> back(this,returned-ref) -> end(this,iterator)
	// + normal ctor(this,pointer-ref), iterator::operator-(this,difference,
	// returned-iterator) + normal ctor; dereference(this,returned-ref). Both
	// actual iterator temporaries have generated destructors(this). Capacity
	// was genuinely reserved for all rows, so _M_realloc_insert is not selected.
	constexpr size_t back =
		2 * room_fixed_P + 2 * room_fixed_P + 2 * room_fixed_P + 2 * room_fixed_P +
		sizeof(std::ptrdiff_t) + 2 * room_fixed_P + 2 * room_fixed_P + 2 * room_fixed_P +
		// operator-(n) constructs the returned iterator from pointer subtraction:
		// a distinct pointer temporary binds its const-reference ctor argument.
		room_fixed_P;
	return construction + back;
}

constexpr size_t room_fixed_byte_fill_source =
	// allocator-specialized __uninitialized_fill_n_a(first,n,value-ref,alloc-ref,
	// returned-first), genuine is_constant_evaluated result; public
	// uninitialized_fill_n(first,n,value-ref,returned-first,__can_fill).
	(4 * room_fixed_P + room_fixed_N + sizeof(bool)) +
	(3 * room_fixed_P + room_fixed_N + sizeof(bool)) +
	// __uninitialized_fill_n<true>::__uninit_fill_n and fill_n each
	// first/n/value-ref/result. fill_n owns both returned size integer and tag.
	2 * (3 * room_fixed_P + room_fixed_N) +
	2 * room_fixed_N + // __size_to_integer(unsigned long input,returned size)
	room_fixed_P + sizeof(std::random_access_iterator_tag) + // category input/result
	// random_access -> bidirectional -> forward -> input tag ctor/dtor chains.
	8 * room_fixed_P +
	// __fill_n_a(first,n,value-ref,random tag,returned-first), then
	// __fill_a(first,last,value-ref), then byte __fill_a1(first,last,value-ref,
	// __tmp,__len,is_constant_evaluated result), actual memset arguments/result.
	3 * room_fixed_P + room_fixed_N + sizeof(std::random_access_iterator_tag) +
	3 * room_fixed_P + 3 * room_fixed_P + sizeof(uint8_t) + room_fixed_N + sizeof(bool) +
	2 * room_fixed_P + sizeof(int) + room_fixed_N;

template <class T> constexpr size_t room_fixed_count_value_vector_ctor_source() noexcept
{
	// Real default allocator argument: allocator/new_allocator ctor and dtor.
	// vector(this,count,value-ref,allocator-ref); _Vector_base(this,n,alloc-ref)
	// plus its full impl/allocator/data const-copy chain; _M_create_storage(this,n).
	// _S_check_init_len(n,alloc-ref,returned-n) owns actual temporary allocator
	// copy (allocator/new_allocator this/source), value, and both destructors;
	// _S_max_size's exact descendant scalar graph stays room_fixed_allocation_source.
	return sizeof(std::allocator<T>) + 4 * room_fixed_P + 3 * room_fixed_P + room_fixed_N +
	       2 * room_fixed_P + room_fixed_N +
	       (2 * room_fixed_P + 2 * room_fixed_P + 2 * room_fixed_P + room_fixed_P) +
	       room_fixed_P + room_fixed_N + room_fixed_P + 2 * room_fixed_N + 4 * room_fixed_P +
	       sizeof(std::allocator<T>) + 2 * room_fixed_P +
	       // _M_fill_initialize(this,n,value-ref) and _M_get_Tp_allocator(this,ref).
	       2 * room_fixed_P + room_fixed_N + 2 * room_fixed_P;
}

constexpr size_t room_fixed_max_size_source =
	// _S_max_size(allocator-ref,__diffmax,__allocmax,returned size), allocator
	// traits max_size(allocator-ref,returned size), min(two refs,returned ref,bool).
	(room_fixed_P + 3 * room_fixed_N) + (room_fixed_P + room_fixed_N) + 3 * room_fixed_P +
	sizeof(bool);
constexpr size_t room_fixed_allocation_source =
	// _M_allocate(this,n,returned pointer); allocator_traits::allocate(alloc-ref,
	// n,returned pointer); allocator::allocate(this,n,returned pointer,actual
	// constant-evaluation result); new_allocator::allocate(this,n,hint,result),
	// its _M_max_size(this,result), then ordinary operator new(n,result).
	3 * (2 * room_fixed_P + room_fixed_N) + sizeof(bool) + (3 * room_fixed_P + room_fixed_N) +
	(room_fixed_P + room_fixed_N) + (room_fixed_N + room_fixed_P);

template <typename T, typename Comparator>
[[maybe_unused]] constexpr size_t room_fixed_sort_leaf_frames()
{
	// Same real GCC13 sort/partition/insertion/heap/copy/adjacent call scopes
	// as UID sorting. Values and comparator carriers use their genuine types.
	// Original key less/equal this-free argument/result scopes and revision
	// lambda this/left/right/result plus its nested key less call.
	return 3 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	       18 * sizeof(void *) + 7 * sizeof(Comparator) + sizeof(T) + 16 * sizeof(void *) +
	       6 * sizeof(Comparator) + 2 * sizeof(T) + 23 * sizeof(void *) +
	       11 * sizeof(std::ptrdiff_t) + 7 * sizeof(Comparator) + 4 * sizeof(T) +
	       8 * sizeof(void *) + 5 * sizeof(Comparator) + 4 * sizeof(bool) +
	       5 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) +
	       2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::ptrdiff_t) + 9 * sizeof(void *) +
	       2 * sizeof(Comparator) + sizeof(bool);
}

using room_fixed_revision_comparator =
	decltype([](const critical_expected_revision &a, const critical_expected_revision &b)
		 { return critical_entity_key_less(a.key, b.key); });
[[maybe_unused]] constexpr size_t room_fixed_sort_depth(size_t count) noexcept
{
	size_t depth = 0;
	while (count > 1)
	{
		count >>= 1;
		++depth;
	}
	return 2 * depth + 1;
}
template <typename T, typename C>
[[maybe_unused]] constexpr size_t room_fixed_sort_complete() noexcept
{
	// The actual sort is reached only after both key vectors pass MAX_KEYS.
	// __introsort_loop preserves iterator/length/comparator at every nested
	// level; heap/insertion/partition source is the identical typed leaf map.
	return room_fixed_sort_leaf_frames<T, C>() +
	       room_fixed_sort_depth(CRITICAL_COMMAND_MAX_KEYS) *
		       (3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(C));
}
[[maybe_unused]] constexpr size_t room_fixed_sorts =
	room_fixed_sort_complete<critical_entity_key, decltype(&critical_entity_key_less)>() +
	room_fixed_sort_complete<critical_expected_revision, room_fixed_revision_comparator>();

template <class T> constexpr size_t room_fixed_fresh_reserve_source() noexcept
{
	// Actual three reserves are fresh and empty: coin (nontrivial default),
	// key and revision (trivial). No per-row relocation or memmove executes
	// on that empty domain. Every selected runtime controller is still real.
	constexpr size_t relocate = 5 * room_fixed_P + 5 * room_fixed_P + 3 * (2 * room_fixed_P) +
				    (std::is_trivial_v<T> ?
					     5 * room_fixed_P + sizeof(std::ptrdiff_t) :
					     6 * room_fixed_P) +
				    2 * room_fixed_P;
	// reserve(this,n,old_size,tmp), max_size/get_allocator/S_max/traits/min;
	// capacity/size; constexpr _S_use_relocate/_S_nothrow_relocate results;
	// exact allocation call chain; null _M_deallocate(this,p,n) is entered,
	// but no delete/deallocator body is called for the old null allocation.
	return 2 * room_fixed_P + 2 * room_fixed_N + room_fixed_P + room_fixed_N +
	       2 * room_fixed_P + room_fixed_max_size_source + 2 * (room_fixed_P + room_fixed_N) +
	       2 * room_fixed_B + room_fixed_allocation_source + relocate + 2 * room_fixed_P +
	       room_fixed_N;
}

constexpr size_t room_fixed_coin_const_push_source =
	// push_back(this,const value&), then direct traits::construct; no
	// emplace/back/iterator/std::move or allocating reallocation branch.
	2 * room_fixed_P + 3 * room_fixed_P + 2 * room_fixed_P + 3 * room_fixed_P +
	2 * room_fixed_P + room_fixed_N + 2 * room_fixed_P +
	// Trivial coin copy constructor and its std::array copy constructor.
	4 * room_fixed_P;

constexpr size_t room_fixed_owned_vector_source =
	room_fixed_fresh_reserve_source<zone_reset_coin_output>() +
	room_fixed_fresh_reserve_source<critical_entity_key>() +
	room_fixed_fresh_reserve_source<critical_expected_revision>() +
	room_fixed_coin_const_push_source + room_fixed_reserved_push_source<critical_entity_key>() +
	room_fixed_reserved_push_source<critical_expected_revision>() +
	// Key and revision genuine trivial move construction; revision's key
	// member and its temporary aggregate key copy/cleanup also execute.
	2 * room_fixed_P + 4 * room_fixed_P + 2 * room_fixed_P + 3 * room_fixed_P +
	// Actual payload byte constructor, value argument and final output move.
	sizeof(uint8_t) + room_fixed_count_value_vector_ctor_source<uint8_t>() +
	room_fixed_max_size_source + room_fixed_allocation_source + room_fixed_byte_fill_source +
	room_value_vector_cleanup_source<uint8_t>() +
	room_value_vector_move_assignment_source<uint8_t>();

// Each function retains its actual scalar/call/reference/result carriers;
// complete existing named DTO objects and fresh capacity requests stay with
// genuine original/bounded request owners. These are source declarations,
// never an emitted-stack or cached workspace proxy.
constexpr size_t room_fixed_entry_source =
	// Public command/output/callback/context, outer/full/retained/initial/live,
	// returned error; typed bad_alloc catch reference and both codec returns.
	4 * room_fixed_P + 5 * room_fixed_N + room_fixed_E + room_fixed_P +
	2 * sizeof(critical_command_codec_result);
constexpr size_t room_fixed_decode_remaining_source =
	// Successful top decoder: keys/revisions and status; work's named scalar
	// members belong to the actual work DTO. No old EVP peak helper is called.
	2 * room_fixed_N + room_fixed_E + 2 * (2 * room_fixed_P);
constexpr size_t room_fixed_payload_decode_scalar_source =
	// Five pointer/ref parameters, outer; has_placement; six wire sizes;
	// live/preliminary/scan/peak/item_heap/recipe_working/coins_heap/result_heap/
	// value_working/offset/i/denomination. Named image/profile/coin/placement
	// inline values stay in original request phases, not this source sum.
	5 * room_fixed_P + 22 * room_fixed_N + room_fixed_B + room_fixed_E +
	sizeof(player_snapshot_codec_result) +
	// Real coin and array default/destruction, local placement default/dtor.
	3 * room_fixed_P + 2 * room_fixed_P;
constexpr size_t room_fixed_payload_encode_scalar_source =
	// Four pointer/ref parameters; outer/live/size/offset/denomination;
	// returned+status, hidden coin range/begin/end and coin+placement refs.
	4 * room_fixed_P + 5 * room_fixed_N + 2 * room_fixed_E + 5 * room_fixed_P;
constexpr size_t room_fixed_build_scalar_source =
	// Six pointer/ref parameters (metadata,image,out,reserve,context), actual
	// timestamp/outer/live/preliminary/keys/revisions/request/heap and status.
	5 * room_fixed_P + sizeof(uint64_t) + 7 * room_fixed_N + 2 * room_fixed_E + room_fixed_P +
	4 * room_fixed_P +
	// Genuine key/revision aggregate temporary values. Sequential initial
	// and loop occurrences are one typed family; no item-count multiplier.
	sizeof(critical_entity_key) + sizeof(critical_expected_revision);
constexpr size_t room_fixed_recipe_wrapper_scalar_source =
	// items/recipes/out/callback/context; outer, all three named constexpr
	// size carriers, scan/validation/encoded/live and max result/query result;
	// checked/returned error and actual profile ctor/cleanup receiver.
	5 * room_fixed_P + 9 * room_fixed_N + 2 * room_fixed_E + 2 * room_fixed_P;
constexpr size_t room_fixed_controls_source =
	// add(ref,amount)->bool; array(count,width,out)->bool;
	// admit(base,extra,reserve,context)->bool; heap(command,out,keys,revs,heap).
	room_fixed_P + room_fixed_N + room_fixed_B + room_fixed_P + 2 * room_fixed_N +
	room_fixed_B + 2 * room_fixed_P + 2 * room_fixed_N + room_fixed_B + 2 * room_fixed_P +
	3 * room_fixed_N + room_fixed_B +
	// Genuine max(two references,returned ref,comparison) and codec mapper.
	3 * room_fixed_P + room_fixed_B + sizeof(player_snapshot_codec_result) + room_fixed_E;
constexpr size_t room_fixed_wire_scalar_source =
	// put(output,value,count,i), get(input,count,value,i,returned value).
	2 * room_fixed_P + 4 * room_fixed_N + 3 * sizeof(uint64_t) +
	// Actual two bit_cast instantiations: const source-ref and returned scalar.
	2 * room_fixed_P + sizeof(int32_t) + sizeof(int64_t) +
	// Original decoder's intermediate uint32_t binds bit_cast const ref.
	sizeof(uint32_t);

constexpr size_t room_fixed_optional_placement_assign_source =
	// Actual disengaged value assignment optional=(Recipe&). Default image
	// already owns empty optional; no optional copy/move construction here.
	3 * room_fixed_P + // this/value/returned reference
	room_fixed_P + room_fixed_B + // _M_is_engaged
	2 * room_fixed_P + // outer forward
	2 * room_fixed_P + 2 * room_fixed_P + // base_impl construct + forward
	2 * room_fixed_P + 2 * room_fixed_P +
	2 * room_fixed_P + // payload construct/addressof/forward
	2 * room_fixed_P + room_fixed_B + 2 * room_fixed_P + // runtime _Construct + forward
	room_fixed_N + 2 * room_fixed_P + // placement new(size,where,returned pointer)
	2 * room_fixed_P; // trivial Recipe copy construction

constexpr size_t room_fixed_metadata_default_source = 9 * room_fixed_P;
constexpr size_t room_fixed_metadata_cleanup_source = 16 * room_fixed_P;
constexpr size_t room_fixed_metadata_assignment_source = 48 * room_fixed_P;
// Metadata default: own receiver + eight disengaged optional receivers. Four
// IDs/arrays use aggregate brace initialization. Cleanup: metadata, four IDs,
// four arrays and seven trivial optional receiver destructors. Assignment:
// metadata/four IDs/four arrays (9*3P) plus seven optional assignments (21P).
constexpr size_t room_fixed_facts_value_source = room_fixed_P + room_fixed_metadata_default_source +
						 room_value_vector_default_source<uint8_t>() +
						 room_fixed_P + room_fixed_metadata_cleanup_source +
						 room_value_vector_cleanup_source<uint8_t>();
constexpr size_t room_fixed_intent_value_source =
	// Generated frozen intent constructor/destructor and both digest-array
	// destructor receivers; digest brace initialization has no ctor call.
	4 * room_fixed_P + room_fixed_facts_value_source;
constexpr size_t room_fixed_command_value_source =
	// Actual command default/cleanup and operation-id/array cleanup plus four
	// vector empty defaults/cleanup. Its selected output move assignment owns
	// command/operation/array three receiver/source/returned-ref scopes.
	6 * room_fixed_P +
	2 * (room_value_vector_default_source<uint8_t>() +
	     room_value_vector_cleanup_source<uint8_t>()) +
	room_value_vector_default_source<critical_entity_key>() +
	room_value_vector_cleanup_source<critical_entity_key>() +
	room_value_vector_default_source<critical_expected_revision>() +
	room_value_vector_cleanup_source<critical_expected_revision>() + 9 * room_fixed_P +
	2 * room_value_vector_move_assignment_source<uint8_t>() +
	room_value_vector_move_assignment_source<critical_entity_key>() +
	room_value_vector_move_assignment_source<critical_expected_revision>() +
	// candidate.operation_id=image.operation_id: ID and array assignment.
	6 * room_fixed_P;
constexpr size_t room_fixed_value_lifetime_source =
	// Actual room workspace receiver default/cleanup and span default/cleanup.
	2 * room_fixed_P + 4 * room_fixed_P + room_fixed_intent_value_source +
	room_fixed_facts_value_source + room_fixed_command_value_source +
	// Parent’s expected command is default/cleaned but never moved as a
	// whole command by the codec; same typed command closure covers its value.
	2 * (room_value_vector_default_source<uint8_t>() +
	     room_value_vector_cleanup_source<uint8_t>()) +
	// Two payload locals are empty byte vectors; count/value candidate has
	// its separate constructor/cleanup above. std::array source cleanup this.
	2 * (room_value_vector_default_source<uint8_t>() +
	     room_value_vector_cleanup_source<uint8_t>()) +
	room_fixed_P + room_fixed_metadata_assignment_source +
	// Four genuine caller std::move instantiations at the original assignment
	// cuts: decoded candidate/image, final workspace/image, payload byte
	// candidate and rebuilt command candidate. Ref/result2P per call, outside
	// the selected vector/generated assignment lifetimes they invoke.
	8 * room_fixed_P;

template <bool Data, bool Index, bool Empty>
constexpr size_t room_fixed_vector_access_source() noexcept
{
	// size/capacity/empty/data:_M_data_ptr, const [] and begin/end. Four actual
	// public data/index/begin/end pointer/ref returns are distinct from the
	// owning vector, along with the generated normal-iterator lifetimes.
	return 2 * (room_fixed_P + room_fixed_N) + (Empty ? room_fixed_P + room_fixed_B : 0) +
	       (Data ? 4 * room_fixed_P : 0) + (Index ? 2 * room_fixed_P + room_fixed_N : 0) +
	       2 * (4 * room_fixed_P + room_fixed_P) +
	       // Ordinary range iterator comparison/base/increment/dereference.
	       // rvalue push's back graph is already in the exact push controller;
	       // direct key.back uses that same selected closure. Byte/coin push
	       // reaches no unrelated front/back overload in this private graph.
	       2 * room_fixed_P + room_fixed_B + 2 * (2 * room_fixed_P) + 2 * room_fixed_P +
	       2 * room_fixed_P;
}

constexpr size_t room_fixed_direct_pointer_copy_source =
	// Public copy, __copy_move_a/a1/a2/copy_m: three iterators and returned
	// iterator each. Runtime constant-evaluation bool, actual _Num, memmove
	// formal/result scopes, and scalar __assign_one on one-byte input.
	5 * (4 * room_fixed_P) + room_fixed_B + sizeof(std::ptrdiff_t) + 3 * room_fixed_P +
	room_fixed_N + 2 * room_fixed_P +
	// __miter_base input/returned iterator twice; __niter_base three times;
	// raw __niter_wrap(from,result,return). The normal-output overload needs
	// difference/operator+ and its genuine pointer temporary/ctor lifetime.
	2 * (2 * room_fixed_P) + 3 * (2 * room_fixed_P) + 3 * room_fixed_P + 2 * room_fixed_P +
	sizeof(std::ptrdiff_t) + 3 * room_fixed_P + room_fixed_P +
	// Genuine two normal byte iterator types (vector mutable, span const):
	// generated copy(this,source), move(this,source), cleanup(this). Formal
	// and returned iterator values themselves were counted above.
	2 * (5 * room_fixed_P) +
	// copy_n(first,n,result,__n2,return), size_to_integer(n,result), actual
	// category(first,returned tag), __copy_n(first,n,result,tag,return), normal
	// input+count’s pointer temp/ctor and its returned iterator destructor.
	3 * room_fixed_P + 2 * room_fixed_N + 2 * room_fixed_N + room_fixed_P +
	sizeof(std::random_access_iterator_tag) + 3 * room_fixed_P + room_fixed_N +
	sizeof(std::random_access_iterator_tag) + 2 * room_fixed_P + sizeof(std::ptrdiff_t) +
	4 * room_fixed_P;

constexpr size_t room_fixed_byte_vector_compare_source =
	// vector==(two refs,returned bool), size(this,returnedN) twice, four
	// begin/end normal iterator constructor/destructor paths. std::equal,
	// equal_aux/aux1/equal<true> each3 params/returned bool, actual __simple,
	// length/niter/memcmp first/second/count/returned int. No three-way compare.
	2 * room_fixed_P + room_fixed_B + 2 * (room_fixed_P + room_fixed_N) +
	4 * (5 * room_fixed_P) + 4 * (3 * room_fixed_P + room_fixed_B) + room_fixed_B +
	room_fixed_N + 3 * (2 * room_fixed_P) +
	// __memcmp owns its pointer/count/int result, then the one bool
	// std::is_constant_evaluated result (compiler intrinsic has no
	// second bool carrier), then a distinct builtin memcmp boundary.
	2 * room_fixed_P + room_fixed_N + sizeof(int) + room_fixed_B + 2 * room_fixed_P +
	room_fixed_N + sizeof(int) +
	// Actual const-vector-byte normal iterator generated copy/move/cleanup
	// type family. Mutable iterator/source comparisons have no three-way path.
	5 * room_fixed_P;

constexpr size_t room_fixed_dynamic_span_source =
	// range constructor(this,range); data/size CPO(this,range,result),
	// vector data/size, extent(this,n); actual subspan(this,offset,count)
	// creates span(this,pointer,count) and extent(this,count); data/size/index
	// wrappers and normal begin/end constructor/destructor identities.
	2 * room_fixed_P + 3 * room_fixed_P + 2 * room_fixed_P + 2 * room_fixed_P + room_fixed_N +
	room_fixed_P + room_fixed_N + room_fixed_P + room_fixed_N + room_fixed_P +
	2 * room_fixed_N + 2 * room_fixed_P + room_fixed_N + room_fixed_P + room_fixed_N +
	// Pointer/count constructor selects raw-pointer to_address and
	// __to_address: each actual pointer formal and returned pointer.
	2 * (2 * room_fixed_P) + 2 * room_fixed_P + 2 * (room_fixed_P + room_fixed_N) +
	2 * room_fixed_P + room_fixed_N + 2 * (5 * room_fixed_P) +
	// Returned subspans and implicit vector-to-span actual value storage;
	// the work payload span is already named inside its actual workspace.
	2 * sizeof(std::span<const uint8_t>) +
	sizeof(std::span<const native_mobile_birth_item_recipe>) +
	// Three actual const-byte/player-item/recipe span families each
	// select generated span+dynamic-extent copy constructors (4P)
	// and cleanup receivers (2P); these are not pointer forwarding.
	3 * (4 * room_fixed_P + 2 * room_fixed_P) +
	// work.payload_bytes assignment selects defaulted span and
	// dynamic-extent copy assignment: receiver/source/returned-ref
	// 3P each. This distinct closure is not ctor/cleanup credit.
	2 * (3 * room_fixed_P);

// Direct iterator/span/byte-copy/compare and generated aggregate scope families
// are owned here; validation leaves and pure profile composition follow below.
// Native/emitted/host qualification is separate from this Source inventory.
} // namespace

namespace
{
constexpr size_t room_fixed_image_validation_scalar_source =
	// Original image-ref; total/rows/recipe_libraries/i/j/matches/denomination;
	// item/recipe/description aliases, money, returned+local status.
	room_fixed_P + 7 * room_fixed_N + 3 * room_fixed_P + room_fixed_B + 2 * room_fixed_E +
	// Three genuine range families: extra-descriptions, coin-match loop,
	// final coin existence loop. Each range/begin/end/current alias is real.
	3 * (4 * room_fixed_P) +
	// delta{} and economic_coin_delta's temporary zero BEFORE are distinct
	// physical arrays not present in the old recipe/item inline credit.
	2 * sizeof(economic_coin_vector) +
	// Both actual caller array cleanup receivers, separate from the nested
	// delta helper's local result-array cleanup already owned below.
	2 * room_fixed_P;
constexpr size_t room_fixed_placement_validation_source = room_fixed_P + 4 * room_fixed_B;
constexpr size_t room_fixed_string_nul_source =
	// add_bytes(count,total-ref,result), add_string(value,total-ref,result),
	// actual basic_string::size(this,returned N), find(char,default-pos):
	// this,c,pos,__ret,__size,__n,returnedN,__data,__p; char_traits::find
	// pointer/count/character-ref/returned-pointer, constant-eval bool and
	// actual memchr(input,c,n,returned-pointer). No substring or strlen.
	room_fixed_N + room_fixed_P + room_fixed_B + 2 * room_fixed_P + room_fixed_B +
	room_fixed_P + room_fixed_N + room_fixed_P + sizeof(char) + 5 * room_fixed_N +
	2 * room_fixed_P + 2 * room_fixed_P + // _M_data(this,returned pointer)
	3 * room_fixed_P + room_fixed_N + room_fixed_B + 2 * room_fixed_P + sizeof(int) +
	room_fixed_N;

constexpr size_t room_fixed_coin_nonnegative_source =
	// nonnegative(value-ref,returned bool), empty lambda temporary value.
	room_fixed_P + room_fixed_B + sizeof(char) +
	// all_of,find_if_not,__find_if_not,RA __find_if: actual first/last/
	// return pointers, predicate values, trip count and RA category value.
	11 * room_fixed_P + 4 * sizeof(char) + room_fixed_B + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) +
	// __pred_iter(returned/passed predicate), _Iter_pred(this,pred), move;
	// __negate(returned/passed adapter), _Iter_negate(this,pred), move.
	6 * sizeof(char) + 6 * room_fixed_P +
	// Negation callback(this,iterator,result) and empty predicate's genuine
	// operator(this,int64,result); actual category(first,returned tag).
	3 * room_fixed_P + sizeof(int64_t) + 2 * room_fixed_B + room_fixed_P +
	sizeof(std::random_access_iterator_tag) +
	// RA tag default/destroy four-base receiver chain. Lambda, positive
	// adapter and negating adapter generated copy/move/cleanup type families.
	8 * room_fixed_P + 5 * room_fixed_P + 10 * room_fixed_P + 10 * room_fixed_P;
constexpr size_t room_fixed_item_none_of_source =
	// Captured coin-reference lambda temporary: a real pointer, never an
	// empty lambda. none_of/find_if/3arg_find_if/RA_find_if use this exact
	// positive predicate; no negation or find_if_not branch executes.
	room_fixed_P + 11 * room_fixed_P + 4 * room_fixed_P + room_fixed_B +
	sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) +
	// __pred_iter+_Iter_pred constructor/move and real callback's operator
	// this/iterator/result, lambda this/item-reference/result.
	3 * room_fixed_P + 3 * room_fixed_P + 4 * room_fixed_P + 2 * room_fixed_B + room_fixed_P +
	sizeof(std::random_access_iterator_tag) + 8 * room_fixed_P + 5 * room_fixed_P +
	10 * room_fixed_P +
	// Normal const-item iterator generated copy/move/cleanup, subtract+two
	// base calls, equality+two base calls, increment and dereference.
	5 * room_fixed_P + 2 * room_fixed_P + sizeof(std::ptrdiff_t) + 4 * room_fixed_P +
	2 * room_fixed_P + room_fixed_B + 4 * room_fixed_P + 2 * room_fixed_P + 2 * room_fixed_P;
constexpr size_t room_fixed_coin_delta_source =
	// delta(before,after,out), ignored, actual result, index, error return.
	3 * room_fixed_P + sizeof(int64_t) + sizeof(economic_coin_vector) + room_fixed_N +
	room_fixed_E +
	// coin_value(vector,out,total128,index,error), narrow(wide,out,bool),
	// actual numeric_limits int64 min/max returned values.
	2 * room_fixed_P + sizeof(__int128_t) + room_fixed_N + room_fixed_E + sizeof(__int128_t) +
	room_fixed_P + room_fixed_B + 2 * sizeof(int64_t) + room_fixed_coin_nonnegative_source +
	// Actual coin array begin/end each call direct data; size and both
	// const/mutable subscripts are direct (this,index,returned reference).
	// Generated result assignment and local result cleanup are separate.
	2 * (4 * room_fixed_P) + room_fixed_P + room_fixed_N +
	2 * (2 * room_fixed_P + room_fixed_N) + 4 * room_fixed_P;

constexpr size_t room_fixed_byte_array_equal_source =
	// array==(left,right,result), exactly three begin/end direct-data wrappers,
	// equal/equal_aux/equal_aux1/equal<true>, __simple, actual length,
	// raw niter three input/results, __memcmp(first,second,n,returned int).
	2 * room_fixed_P + room_fixed_B + 3 * (4 * room_fixed_P) +
	4 * (3 * room_fixed_P + room_fixed_B) + room_fixed_B + room_fixed_N +
	3 * (2 * room_fixed_P) +
	// Own __memcmp scope, its single constant-evaluation query bool,
	// and the distinct builtin memcmp boundary. This conservative
	// family includes its own leaf; no vector margin is borrowed.
	2 * room_fixed_P + room_fixed_N + sizeof(int) + room_fixed_B + 2 * room_fixed_P +
	room_fixed_N + sizeof(int);
constexpr size_t room_fixed_source_encode_scalar_source =
	// event/output, actual local result array48, kind, two lexical byte-loop
	// values, returned error. Source validity/zero-ID graph is in the genuine
	// metadata validator Source export, which is actually called by this codec.
	2 * room_fixed_P + sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) +
	sizeof(uint16_t) + 2 * room_fixed_N + room_fixed_E +
	// Actual array result assignment(this,source,returned-ref), cleanup(this).
	4 * room_fixed_P;
constexpr size_t room_fixed_source_decode_scalar_source =
	// Original source decoder's by-value formal span is in the old actual
	// decode-object+span phase. Reader/result/two IDs/two spans are real old
	// DTO credit, not source substitutes. These remaining call/results are new.
	room_fixed_P + room_fixed_E +
	// take(this,count), integer<uint16,uint32,uint64>(this,value,i,return);
	// id(this), block16(this,array-ref). The unsigned returned T carriers are
	// real, distinct from uint64 value and returned identity/span objects.
	room_fixed_P + room_fixed_N + 3 * (room_fixed_P + sizeof(uint64_t) + room_fixed_N) +
	sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint64_t) + room_fixed_P + 2 * room_fixed_P +
	// Reader's span copy/extent-copy and generated reader/span/extent cleanup;
	// ID return generated move/copy and local cleanup; source event result
	// final copy assignment and result cleanup, both IDs/arrays included.
	7 * room_fixed_P + 6 * room_fixed_P + 6 * room_fixed_P + 15 * room_fixed_P +
	5 * room_fixed_P;
constexpr size_t room_fixed_metadata_check_scalar_source =
	2 * room_fixed_P + 2 * room_fixed_E +
	// Actual local actual/expected source48 arrays are already pre-admitted
	// by the builder; their trivial generated cleanup receiver scopes are not.
	2 * room_fixed_P;
constexpr size_t room_fixed_optional_access_source =
	// Both actual optional types: bool(this,result)->is_engaged(this,result),
	// const dereference(this,resultref)->base_get(this,resultref)->payload_get
	// (this,resultref). Source/placement caller values remain input-owned.
	2 * (2 * (room_fixed_P + room_fixed_B) + 3 * (2 * room_fixed_P)) +
	// Placement's actual const arrow(this,returned-pointer) -> addressof,
	// whose _M_get chain is already the same genuine dereference leaf above.
	4 * room_fixed_P;
constexpr size_t room_fixed_item_vector_access_source =
	// Actual const item vector: size, index, empty, front, and data/_M_data_ptr
	// for the real vector-to-span recipe call. Its begin/end constructors and
	// range comparison/base/inc/deref generated families really execute.
	room_fixed_P + room_fixed_N + 2 * room_fixed_P + room_fixed_N + room_fixed_P +
	room_fixed_B + 2 * room_fixed_P + 4 * room_fixed_P + 2 * (5 * room_fixed_P) +
	2 * room_fixed_P + room_fixed_B + 2 * (2 * room_fixed_P) + 2 * room_fixed_P +
	2 * room_fixed_P + 5 * room_fixed_P;
constexpr size_t room_fixed_recipe_vector_access_source =
	// Real recipe vector size/index and vector-to-span data/_M_data_ptr. No
	// recipe-vector range iteration/empty/front is reached by the parent.
	room_fixed_P + room_fixed_N + 2 * room_fixed_P + room_fixed_N + 4 * room_fixed_P;
constexpr size_t room_fixed_description_vector_access_source =
	// Actual extra-description size/range, with const normal iterator family.
	// No data/index/empty/front branch is selected for this type.
	room_fixed_P + room_fixed_N + 2 * (5 * room_fixed_P) + 2 * room_fixed_P + room_fixed_B +
	2 * (2 * room_fixed_P) + 2 * room_fixed_P + 2 * room_fixed_P + 5 * room_fixed_P;
constexpr size_t room_fixed_leaf_validation_source =
	room_fixed_image_validation_scalar_source + room_fixed_placement_validation_source +
	room_fixed_string_nul_source + room_fixed_item_none_of_source +
	room_fixed_coin_delta_source + room_fixed_byte_array_equal_source +
	room_fixed_source_encode_scalar_source + room_fixed_source_decode_scalar_source +
	room_fixed_metadata_check_scalar_source + room_fixed_optional_access_source +
	room_fixed_item_vector_access_source + room_fixed_recipe_vector_access_source +
	room_fixed_description_vector_access_source +
	// Distinct genuine dynamic-affect, spell-ID and recipe-library vector
	// types only reach size in the parent. Their lower codec internals keep
	// their own complete Source profiles; no uncalled range/index is lent.
	3 * (room_fixed_P + room_fixed_N) +
	// Actual const item.values int32 and mutable byte-array direct
	// subscripts:2P+N each. Operation-ID byte size owns this/result P+N.
	// Byte-array begin/end/direct-data descendants are already the same
	// selected wrappers above; no nonexistent _S_ref/_S_ptr is charged.
	// Coin int64 array access is separately owned above.
	2 * (2 * room_fixed_P + room_fixed_N) + room_fixed_P + room_fixed_N;
} // namespace

namespace
{
bool room_fixed_source_supported() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 && \
	!defined(__clang__) && __cplusplus == 202002L && defined(_GLIBCXX_RELEASE) &&   \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                    \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                 \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&              \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(unsigned long) == 8 && sizeof(std::allocator<zone_reset_coin_output>) == 1 &&
	       sizeof(std::vector<zone_reset_coin_output>) == 3 * sizeof(void *);
#else
	return false;
#endif
}

// Actual selected ROOM graph and genuine named lower pure Source laws. This
// private candidate requires Source review and native qualification separately.
constexpr size_t room_fixed_decoder_owned_source =
	room_fixed_decode_remaining_source + room_fixed_payload_decode_scalar_source +
	room_fixed_payload_encode_scalar_source + room_fixed_build_scalar_source +
	room_fixed_recipe_wrapper_scalar_source + room_fixed_controls_source +
	room_fixed_wire_scalar_source + room_fixed_optional_placement_assign_source +
	room_fixed_owned_vector_source + room_fixed_value_lifetime_source + room_fixed_sorts +
	room_fixed_direct_pointer_copy_source + room_fixed_byte_vector_compare_source +
	room_fixed_dynamic_span_source +
	room_fixed_vector_access_source<true, true, true>() + // byte vectors
	room_fixed_vector_access_source<false, false, false>() + // key vectors
	room_fixed_vector_access_source<false, false, false>() + // revision vectors
	room_fixed_vector_access_source<false, false, false>() + // coin vectors
	room_fixed_leaf_validation_source;

bool room_fixed_decode_profile(size_t *full_output, size_t *retained_output) noexcept
{
	if (!full_output || !retained_output || !room_fixed_source_supported())
		return false;
	size_t item_encode = 0, item_decode = 0, item_preflight = 0, recipe_encode = 0,
	       recipe_decode = 0, recipe_valid = 0, image_lifetime = 0, metadata = 0,
	       intent_decode = 0, intent_supplement = 0, freeze = 0, verify = 0, codec = 0,
	       full = room_fixed_decoder_owned_source, retained = room_fixed_decoder_owned_source;
	if (!player_item_snapshot_list_encode_source_frame_bytes(&item_encode) ||
	    !player_item_snapshot_list_decode_source_frame_bytes(&item_decode) ||
	    !player_item_snapshot_list_preflight_source_frame_bytes(&item_preflight) ||
	    !native_mobile_birth_recipe_encode_source_frame_bytes(&recipe_encode) ||
	    !native_mobile_birth_recipe_decode_source_frame_bytes(&recipe_decode) ||
	    !native_mobile_birth_recipe_valid_source_frame_bytes(&recipe_valid) ||
	    !zone_reset_item_image_lifetime_source_frame_bytes(&image_lifetime) ||
	    !economic_operation_metadata_validate_source_frame_bytes(&metadata) ||
	    !economic_intent_decode_source_frame_bytes(&intent_decode) ||
	    !economic_intent_decode_source_supplement_frame_bytes(&intent_supplement) ||
	    !economic_intent_freeze_fixed_source_frame_bytes(&freeze) ||
	    !economic_intent_verify_binding_fixed_source_frame_bytes(&verify) ||
	    !critical_command_startup_codec_complete_source_frame_bytes(&codec))
		return false;
	// Original item/recipe stages run sequentially. These named complete lower
	// Source laws use a genuine maximum, with physical DTO/request lifetimes
	// remaining in the real child/old payload controller; no cached admission.
	if (!room_bound_add(full, std::max(item_encode, std::max(item_decode, item_preflight))) ||
	    !room_bound_add(full, std::max(recipe_encode, std::max(recipe_decode, recipe_valid))) ||
	    !room_bound_add(full, image_lifetime) || !room_bound_add(full, metadata) ||
	    !room_bound_add(full, codec) ||
	    !room_bound_add(full, critical_command_valid_frame_bytes()))
		return false;
	retained = full;
	// Decode's original bounded child lacks only its named supplement. Fixed
	// verify owns its Source. The freeze wrapper retains only freeze's named
	// supplement returned by the genuine lower getter. Both wrappers and the
	// child preflight own/admit their actual scope independently; include them
	// in the full query envelope, never a second retained baseline.
	if (!room_bound_add(retained, intent_supplement) || !room_bound_add(full, intent_decode) ||
	    !room_bound_add(full, freeze) || !room_bound_add(full, verify) ||
	    !room_bound_add(full, room_fixed_child_preflight_frames) ||
	    !room_bound_add(full, room_fixed_freeze_wrapper_source) ||
	    !room_bound_add(full, room_fixed_verify_wrapper_source) ||
	    !room_bound_add(full, room_fixed_entry_source) ||
	    !room_bound_add(full, zone_reset_item_command_decode_source_query_frame_bytes()))
		return false;
	*full_output = full;
	*retained_output = retained;
	return true;
}
} // namespace

bool zone_reset_item_command_decode_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	size_t full = 0, retained = 0;
	if (!room_fixed_decode_profile(&full, &retained))
		return false;
	*output = full;
	return true;
}
bool zone_reset_item_command_decode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !room_fixed_source_supported())
		return false;
	*output = 0;
	return true;
}
bool zone_reset_item_command_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !room_fixed_source_supported())
		return false;
	// Actual private workspace is constructed after the admitted source query.
	// Caller image/output/prior capacities remain caller-owned.
	*output = sizeof(room_decode_workspace);
	return true;
}

namespace
{

using room_fixed_reserve_fn = bool (*)(size_t, void *) noexcept;
bool room_fixed_admit(size_t base, size_t extra, room_fixed_reserve_fn reserve,
		      void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}
bool room_fixed_command_heap(const critical_command &command, size_t *output) noexcept
{
	if (!output)
		return false;
	size_t keys = 0, revisions = 0, heap = command.payload.capacity();
	if (!room_bound_array(command.keys.capacity(), sizeof(critical_entity_key), &keys) ||
	    !room_bound_array(command.expected_revisions.capacity(),
			      sizeof(critical_expected_revision), &revisions) ||
	    !room_bound_add(heap, command.accounting_intent.capacity()) ||
	    !room_bound_add(heap, keys) || !room_bound_add(heap, revisions))
		return false;
	*output = heap;
	return true;
}

error room_fixed_child_preflight(room_fixed_source_query full, room_fixed_source_query initial,
				 room_fixed_source_query supplement, size_t query_frames,
				 size_t outer, bool (*reserve)(size_t, void *) noexcept,
				 void *context, size_t *child_outer) noexcept
{
	size_t source = 0, entry = 0, retained = 0, request = outer;
	if (!room_bound_add(request, room_fixed_child_preflight_frames) ||
	    !room_bound_add(request, query_frames) || !room_bound_add(request, sizeof(size_t)) ||
	    !room_fixed_admit(request, 0, reserve, context))
		return error::capacity;
	if (!full || !child_outer || !full(&source) || (initial && !initial(&entry)) ||
	    (supplement && !supplement(&retained)))
		return error::unresolved;
	request = outer;
	if (!room_bound_add(request, room_fixed_child_preflight_frames) ||
	    !room_bound_add(request, source) || !room_bound_add(request, entry) ||
	    !room_fixed_admit(request, 0, reserve, context))
		return error::capacity;
	request = outer;
	if (!room_bound_add(request, retained))
		return error::capacity;
	*child_outer = request;
	return error::ok;
}

error room_fixed_intent_freeze(const critical_command &command,
			       const economic_admission_facts &facts, std::vector<uint8_t> *output,
			       bool (*reserve)(size_t, void *) noexcept, void *context,
			       size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.

	size_t base = outer, child = 0;
	if (!room_bound_add(base, room_fixed_freeze_wrapper_source))
		return error::capacity;
	const auto checked = room_fixed_child_preflight(
		economic_intent_freeze_fixed_source_frame_bytes,
		economic_intent_freeze_fixed_initial_inline_bytes,
		economic_intent_freeze_fixed_source_supplement_frame_bytes,
		economic_intent_freeze_fixed_source_query_frame_bytes(), base, reserve, context,
		&child);
	if (checked != error::ok)
		return checked;
	return economic_intent_freeze_fixed_bounded(command, facts, output, reserve, context,
						    child);
}

error room_fixed_intent_verify(const critical_command &command,
			       const economic_frozen_intent &intent,
			       bool (*reserve)(size_t, void *) noexcept, void *context,
			       size_t outer) noexcept
{
	// Actual pointer arguments, outer/base/child/query size_t values and
	// checked/result, plus add/return boolean source. Inputs/prior outputs
	// remain caller-owned. The wrapper holds no private codec candidate.

	size_t base = outer, child = 0;
	if (!room_bound_add(base, room_fixed_verify_wrapper_source))
		return error::capacity;
	const auto checked = room_fixed_child_preflight(
		economic_intent_verify_binding_fixed_source_frame_bytes,
		economic_intent_verify_binding_fixed_initial_inline_bytes, nullptr,
		economic_intent_verify_binding_fixed_source_query_frame_bytes(), base, reserve,
		context, &child);
	if (checked != error::ok)
		return checked;
	return economic_intent_verify_binding_fixed_bounded(command, intent, reserve, context,
							    child);
}

error room_fixed_recipe_encode(const std::vector<player_item_snapshot> &items,
			       const std::span<const native_mobile_birth_item_recipe> &recipes,
			       std::vector<uint8_t> *output, room_fixed_reserve_fn reserve,
			       void *context, size_t outer) noexcept
{
	if (!output || !reserve)
		return error::corrupt_evidence;
	constexpr size_t profile_object = sizeof(native_mobile_birth_recipe_allocation_profile);
	constexpr size_t item_span = sizeof(std::span<const player_item_snapshot>);
	constexpr size_t recipe_span = sizeof(std::span<const native_mobile_birth_item_recipe>);
	size_t scan = outer;
	if (!room_bound_add(scan, profile_object) ||
	    !room_bound_add(scan, native_mobile_birth_recipe_profile_inline_storage_bytes()) ||
	    !room_bound_add(scan, 2 * item_span + 2 * recipe_span) ||
	    !room_fixed_admit(scan, 0, reserve, context))
		return error::capacity;
	native_mobile_birth_recipe_allocation_profile profile;
	const auto checked = native_mobile_birth_recipe_encode_profile(items, recipes, &profile);
	if (checked != error::ok)
		return checked;
	if (!profile.fresh_encode_storage_policy_supported)
		return error::unresolved;
	size_t validation = item_span + recipe_span, encoded = profile.encoder_inline_storage_bytes;
	if (!room_bound_add(validation, profile.validation_inline_storage_bytes) ||
	    !room_bound_add(encoded, profile.encoded_capacity_bytes))
		return error::capacity;
	size_t live = outer;
	if (!room_bound_add(live, profile_object) ||
	    !room_bound_add(live, item_span + recipe_span) ||
	    !room_bound_add(live, std::max(validation, encoded)) ||
	    !room_fixed_admit(live, 0, reserve, context))
		return error::capacity;
	return native_mobile_birth_recipe_encode(items, recipes, output);
}

error room_fixed_payload_encode(const zone_reset_item_image &image, std::vector<uint8_t> *output,
				room_fixed_reserve_fn reserve, void *context, size_t outer)
{
	size_t live = outer;
	// Real local vectors/source-array exist in this selected original payload.
	// Count them before construction; prior output belongs to the caller.
	if (!room_bound_add(live, 3 * sizeof(std::vector<uint8_t>)) ||
	    !room_bound_add(live, sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)) ||
	    !room_fixed_admit(live, 0, reserve, context))
		return error::capacity;
	std::vector<uint8_t> items, recipes;
	auto status = codec_error(player_item_snapshot_list_encode_bounded(image.items, &items,
									   reserve, context, live));
	if (status != error::ok)
		return status;
	if (!room_bound_add(live, items.capacity()))
		return error::capacity;
	status = room_fixed_recipe_encode(image.items, image.recipes, &recipes, reserve, context,
					  live);
	if (status != error::ok)
		return status;
	size_t size = HEADER_BYTES;
	if (!add_bytes(items.size(), &size) || !add_bytes(recipes.size(), &size) ||
	    !add_bytes(image.coins.size() * COIN_BYTES, &size) ||
	    (image.placement && !add_bytes(PLACEMENT_BYTES, &size)))
		return error::capacity;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	status = economic_source_event_encode(image.reset_source, &source);
	if (status != error::ok)
		return status;
	if (!room_bound_add(live, recipes.capacity()) ||
	    !room_fixed_admit(live, size, reserve, context))
		return error::capacity;
	std::vector<uint8_t> candidate(size, 0);
	std::copy(MAGIC.begin(), MAGIC.end(), candidate.begin());
	put(candidate.data() + 4,
	    image.placement ? ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION :
			      ZONE_RESET_ITEM_PAYLOAD_VERSION,
	    2);
	std::copy(image.operation_id.bytes.begin(), image.operation_id.bytes.end(),
		  candidate.begin() + 8);
	std::copy(source.begin(), source.end(), candidate.begin() + 24);
	put(candidate.data() + 72, static_cast<uint32_t>(image.zone_vnum), 4);
	put(candidate.data() + 76, static_cast<uint32_t>(image.room_vnum), 4);
	put(candidate.data() + 80, image.season_epoch, 8);
	put(candidate.data() + 88, image.expected_room_revision, 8);
	put(candidate.data() + 96, items.size(), 4);
	put(candidate.data() + 100, recipes.size(), 4);
	put(candidate.data() + 104, image.coins.size(), 4);
	std::copy(items.begin(), items.end(), candidate.begin() + HEADER_BYTES);
	std::copy(recipes.begin(), recipes.end(), candidate.begin() + HEADER_BYTES + items.size());
	size_t offset = HEADER_BYTES + items.size() + recipes.size();
	for (const auto &coin : image.coins)
	{
		put(candidate.data() + offset, coin.item_uid, 8);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			put(candidate.data() + offset + 8 + denomination * 8,
			    static_cast<uint64_t>(coin.denominations[denomination]), 8);
		offset += COIN_BYTES;
	}
	if (image.placement)
	{
		const auto &p = *image.placement;
		put(candidate.data() + offset, p.root_uid, 8);
		put(candidate.data() + offset + 8, static_cast<uint32_t>(p.room_vnum), 4);
		put(candidate.data() + offset + 12, static_cast<uint32_t>(p.original_sector_type),
		    4);
		put(candidate.data() + offset + 16, static_cast<uint32_t>(p.original_chance_fall),
		    4);
		put(candidate.data() + offset + 20, static_cast<uint32_t>(p.original_z_cord), 4);
		put(candidate.data() + offset + 24, p.fall_roll, 4);
		candidate[offset + 28] = p.original_levitates;
		candidate[offset + 29] = p.fall_roll_drawn;
		candidate[offset + 30] = p.fall_selected;
	}
	*output = std::move(candidate);
	return error::ok;
}

economic_accounting_error room_fixed_build(const economic_operation_metadata &metadata,
					   const zone_reset_item_image &image,
					   uint64_t accepted_at_usec, critical_command *output,
					   room_fixed_reserve_fn reserve, void *context,
					   size_t outer) noexcept
{
	if (!output || !accepted_at_usec)
		return error::invalid_identity;
	try
	{
		size_t live = outer, preliminary = outer;
		// Genuine local candidate/facts and native validation objects only;
		// all input/image/old-output storage remains in outer.
		if (!room_bound_add(live, sizeof(critical_command)) ||
		    !room_bound_add(live, sizeof(economic_admission_facts)) ||
		    !room_bound_add(preliminary,
				    native_mobile_birth_recipe_profile_inline_storage_bytes()) ||
		    !room_bound_add(preliminary,
				    2 * sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>)) ||
		    !room_fixed_admit(std::max(live, preliminary), 0, reserve, context))
			return error::capacity;
		auto status = image_preflight(image);
		if (status != error::ok)
			return status;
		status = metadata_check(metadata, image);
		if (status != error::ok)
			return status;
		critical_command candidate{};
		candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		candidate.operation_id = image.operation_id;
		candidate.type = critical_command_type::zone_reset_item_birth;
		candidate.payload_version = image.placement ?
						    ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION :
						    ZONE_RESET_ITEM_PAYLOAD_VERSION;
		candidate.source_site = critical_source_site::zone_event;
		candidate.deadline_class = critical_deadline_class::background;
		candidate.accepted_at_usec = accepted_at_usec;
		status = room_fixed_payload_encode(image, &candidate.payload, reserve, context,
						   live);
		if (status != error::ok)
			return status;
		size_t keys = 0, revisions = 0, request = live;
		if (!room_bound_array(image.items.size() + 2, sizeof(critical_entity_key), &keys) ||
		    !room_bound_array(image.items.size() + 1, sizeof(critical_expected_revision),
				      &revisions) ||
		    !room_bound_add(request, candidate.payload.capacity()) ||
		    !room_bound_add(request, keys) || !room_bound_add(request, revisions) ||
		    !room_fixed_admit(request, 0, reserve, context))
			return error::capacity;
		candidate.keys.reserve(image.items.size() + 2);
		candidate.expected_revisions.reserve(image.items.size() + 1);
		candidate.keys.push_back(
			{ critical_entity_type::zone, static_cast<uint64_t>(image.zone_vnum) + 1 });
		candidate.keys.push_back(
			{ critical_entity_type::room, static_cast<uint64_t>(image.room_vnum) });
		candidate.expected_revisions.push_back(
			{ candidate.keys.back(), image.expected_room_revision });
		for (const auto &item : image.items)
		{
			candidate.keys.push_back({ critical_entity_type::item, item.object_uid });
			candidate.expected_revisions.push_back({ candidate.keys.back(), 0 });
		}
		std::sort(candidate.keys.begin(), candidate.keys.end(), critical_entity_key_less);
		std::sort(candidate.expected_revisions.begin(), candidate.expected_revisions.end(),
			  [](const auto &a, const auto &b)
			  { return critical_entity_key_less(a.key, b.key); });
		economic_admission_facts facts;
		facts.metadata = metadata;
		size_t heap = 0;
		if (!room_fixed_command_heap(candidate, &heap) || !room_bound_add(live, heap))
			return error::capacity;
		status = room_fixed_intent_freeze(candidate, facts, &candidate.accounting_intent,
						  reserve, context, live);
		if (status != error::ok)
			return status;
		candidate.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		candidate.publication_required = true;
		if (!critical_command_envelope_valid(candidate))
			return error::corrupt_evidence;
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}
} // namespace

economic_accounting_error
zone_reset_item_command_decode_fixed_bounded(const critical_command &command,
					     zone_reset_item_image *output,
					     bool (*reserve_scratch_peak)(size_t, void *) noexcept,
					     void *context, size_t outer_live_scratch) noexcept
{
	if (!output)
		return error::invalid_identity;
	if (!reserve_scratch_peak)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)context;
	(void)outer_live_scratch;
	return error::capacity;
#else
	size_t full = 0, retained = 0, initial = 0, live = outer_live_scratch;
	// Query and actual entry carriers are admitted before pure getter entry.
	if (!room_bound_add(live, room_fixed_entry_source) ||
	    !room_fixed_admit(live,
			      zone_reset_item_command_decode_source_query_frame_bytes() +
				      sizeof(size_t),
			      reserve_scratch_peak, context))
		return error::capacity;
	if (!room_fixed_decode_profile(&full, &retained) ||
	    !zone_reset_item_command_decode_initial_inline_bytes(&initial) ||
	    !room_bound_add(full, initial) ||
	    !room_fixed_admit(live, full, reserve_scratch_peak, context) ||
	    !room_bound_add(live, retained) || !room_bound_add(live, initial) ||
	    !room_fixed_admit(live, 0, reserve_scratch_peak, context))
		return error::capacity;
	try
	{
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    command.type != critical_command_type::zone_reset_item_birth ||
		    (command.payload_version != ZONE_RESET_ITEM_PAYLOAD_VERSION &&
		     command.payload_version != ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION) ||
		    !command.publication_required || !critical_command_envelope_valid(command))
			return error::corrupt_evidence;
		room_decode_workspace work;
		work.payload_bytes = command.payload;
		auto status = payload_decode_bounded(work.payload_bytes, &work.image,
						     reserve_scratch_peak, context, live,
						     &work.image_heap);
		if (status != error::ok)
			return status;
		if (!room_bound_add(live, work.image_heap))
			return error::capacity;
		work.payload_bytes = command.accounting_intent;
		status = economic_intent_decode_bounded(work.payload_bytes, &work.intent,
							reserve_scratch_peak, context, live);
		work.payload_bytes = command.payload;
		if (status != error::ok)
			return status;
		if (work.intent.admission.facts_version != 1 ||
		    !work.intent.admission.facts.empty())
			return error::payload_conflict;
		status = room_fixed_intent_verify(command, work.intent, reserve_scratch_peak,
						  context, live);
		if (status != error::ok)
			return status;
		// Canonical original compiler order and wire with genuine fixed
		// binding/freezing at the original cuts; no EVP body receives a stamp.
		status = room_fixed_build(work.intent.admission.metadata, work.image,
					  command.accepted_at_usec, &work.expected,
					  reserve_scratch_peak, context, live);
		if (status != error::ok)
			return status;
		size_t keys = 0, revisions = 0;
		if (!room_bound_array(work.expected.keys.capacity(), sizeof(critical_entity_key),
				      &keys) ||
		    !room_bound_array(work.expected.expected_revisions.capacity(),
				      sizeof(critical_expected_revision), &revisions))
			return error::capacity;
		work.expected_heap = work.expected.payload.capacity();
		if (!room_bound_add(work.expected_heap,
				    work.expected.accounting_intent.capacity()) ||
		    !room_bound_add(work.expected_heap, keys) ||
		    !room_bound_add(work.expected_heap, revisions) ||
		    !room_bound_add(live, work.expected_heap))
			return error::capacity;
		if (critical_command_encode_bounded(command, &work.actual_bytes,
						    reserve_scratch_peak, context,
						    live) != critical_command_codec_result::ok)
			return error::capacity;
		if (!room_bound_add(live, work.actual_bytes.capacity()))
			return error::capacity;
		if (critical_command_encode_bounded(work.expected, &work.expected_bytes,
						    reserve_scratch_peak, context,
						    live) != critical_command_codec_result::ok)
			return error::capacity;
		if (work.actual_bytes != work.expected_bytes)
			return error::payload_conflict;
		*output = std::move(work.image);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}
