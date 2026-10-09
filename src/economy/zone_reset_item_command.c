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
