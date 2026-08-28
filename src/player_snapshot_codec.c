#include "player_snapshot_codec.h"

#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		for (size_t index = 0; index < sizeof(T); ++index)
		{
			bytes.push_back(static_cast<uint8_t>(bits & 0xff));
			bits >>= 8;
		}
	}

	void boolean(bool value) { number<uint8_t>(value ? 1 : 0); }

	void string(const std::string &value)
	{
		if (value.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
		{
			valid = false;
			return;
		}
		number<uint32_t>(value.size());
		bytes.insert(bytes.end(), value.begin(), value.end());
	}

	template <typename T, typename Write> void vector(const std::vector<T> &values, Write write)
	{
		if (values.size() > PLAYER_SNAPSHOT_MAX_ROWS)
		{
			valid = false;
			return;
		}
		number<uint32_t>(values.size());
		for (const T &value : values)
			write(value);
	}
};

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;
	size_t rows = 0;
	size_t objects = 0;
	player_snapshot_codec_result result = player_snapshot_codec_result::ok;

	template <typename T> bool number(T &value)
	{
		if (result != player_snapshot_codec_result::ok || size - offset < sizeof(T))
		{
			result = player_snapshot_codec_result::truncated;
			return false;
		}
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<unsigned_type>(data[offset++]) << (index * 8);
		value = static_cast<T>(bits);
		return true;
	}

	bool boolean(bool &value)
	{
		uint8_t encoded = 0;
		if (!number(encoded))
			return false;
		if (encoded > 1)
		{
			result = player_snapshot_codec_result::invalid_value;
			return false;
		}
		value = encoded != 0;
		return true;
	}

	bool string(std::string &value)
	{
		uint32_t length = 0;
		if (!number(length))
			return false;
		if (length > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		if (size - offset < length)
		{
			result = player_snapshot_codec_result::truncated;
			return false;
		}
		value.assign(reinterpret_cast<const char *>(data + offset), length);
		offset += length;
		return true;
	}

	template <typename T, typename Read>
	bool vector(std::vector<T> &values, Read read, bool object_rows = false)
	{
		uint32_t count = 0;
		if (!number(count))
			return false;
		if (count > PLAYER_SNAPSHOT_MAX_ROWS || rows > PLAYER_SNAPSHOT_MAX_ROWS - count ||
		    (object_rows && (count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
				     objects > PLAYER_SNAPSHOT_MAX_OBJECTS - count)))
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		rows += count;
		if (object_rows)
			objects += count;
		values.resize(count);
		for (T &value : values)
			if (!read(value))
				return false;
		return true;
	}
};

void encode_index_rows(encoder &out, const std::vector<player_index_value_snapshot> &rows)
{
	out.vector(rows,
		   [&](const auto &row)
		   {
			   out.number<int32_t>(row.index);
			   out.number<int64_t>(row.value);
			   out.number<uint64_t>(row.auxiliary);
		   });
}

bool decode_index_rows(decoder &in, std::vector<player_index_value_snapshot> &rows)
{
	return in.vector(rows,
			 [&](auto &row) {
				 return in.number(row.index) && in.number(row.value) &&
					in.number(row.auxiliary);
			 });
}

void encode_items(encoder &out, const std::vector<player_item_snapshot> &items)
{
	out.vector(items,
		   [&](const player_item_snapshot &row)
		   {
			   out.number<int32_t>(row.parent_index);
			   out.number<int16_t>(row.equipment_slot);
			   out.number<uint64_t>(row.object_uid);
			   out.number<int64_t>(row.generated_key);
			   out.number<int32_t>(row.vnum);
			   out.number<int8_t>(row.type);
			   out.number<uint8_t>(row.string_mask);
			   out.string(row.name);
			   out.string(row.short_description);
			   out.string(row.description);
			   out.string(row.action_description);
			   for (int32_t value : row.values)
				   out.number<int32_t>(value);
			   for (int64_t timer : row.timers)
				   out.number<int64_t>(timer);
			   out.number<uint32_t>(row.wear_flags);
			   out.number<uint32_t>(row.extra_flags);
			   out.number<uint32_t>(row.anti_flags);
			   out.number<uint32_t>(row.anti2_flags);
			   out.number<uint32_t>(row.extra2_flags);
			   out.number<int32_t>(row.weight);
			   out.number<int8_t>(row.material);
			   out.number<int32_t>(row.cost);
			   out.number<int16_t>(row.condition);
			   out.number<int16_t>(row.craftsmanship);
			   for (uint64_t bitvector : row.bitvectors)
				   out.number<uint64_t>(bitvector);
			   for (const auto &affect : row.affects)
				   for (int16_t value : affect)
					   out.number<int16_t>(value);
			   out.vector(row.dynamic_affects,
				      [&](const auto &affect)
				      {
					      out.number<int16_t>(affect.type);
					      out.number<int16_t>(affect.data);
					      out.number<uint64_t>(affect.extra2);
				      });
			   out.vector(row.extra_descriptions,
				      [&](const auto &description)
				      {
					      out.string(description.keyword);
					      out.string(description.description);
					      out.boolean(description.spellbook);
					      out.vector(description.spell_ids,
							 [&](int32_t skill_id)
							 { out.number<int32_t>(skill_id); });
				      });
		   });
}

bool decode_items(decoder &in, std::vector<player_item_snapshot> &items)
{
	return in.vector(
		items,
		[&](player_item_snapshot &row)
		{
			if (!in.number(row.parent_index) || !in.number(row.equipment_slot) ||
			    !in.number(row.object_uid) || !in.number(row.generated_key) ||
			    !in.number(row.vnum) || !in.number(row.type) ||
			    !in.number(row.string_mask) || !in.string(row.name) ||
			    !in.string(row.short_description) || !in.string(row.description) ||
			    !in.string(row.action_description))
				return false;
			for (int32_t &value : row.values)
				if (!in.number(value))
					return false;
			for (int64_t &timer : row.timers)
				if (!in.number(timer))
					return false;
			if (!in.number(row.wear_flags) || !in.number(row.extra_flags) ||
			    !in.number(row.anti_flags) || !in.number(row.anti2_flags) ||
			    !in.number(row.extra2_flags) || !in.number(row.weight) ||
			    !in.number(row.material) || !in.number(row.cost) ||
			    !in.number(row.condition) || !in.number(row.craftsmanship))
				return false;
			for (uint64_t &bitvector : row.bitvectors)
				if (!in.number(bitvector))
					return false;
			for (auto &affect : row.affects)
				for (int16_t &value : affect)
					if (!in.number(value))
						return false;
			if (!in.vector(row.dynamic_affects,
				       [&](auto &affect) {
					       return in.number(affect.type) &&
						      in.number(affect.data) &&
						      in.number(affect.extra2);
				       }))
				return false;
			return in.vector(row.extra_descriptions,
					 [&](auto &description)
					 {
						 return in.string(description.keyword) &&
							in.string(description.description) &&
							in.boolean(description.spellbook) &&
							in.vector(description.spell_ids,
								  [&](int32_t &skill_id)
								  { return in.number(skill_id); });
					 });
		},
		true);
}

bool valid_metadata(const player_snapshot &snapshot)
{
	return snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION && snapshot.pid > 0 &&
	       snapshot.revision && snapshot.components &&
	       !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
	       snapshot.encoded_size_bound &&
	       snapshot.encoded_size_bound <= PLAYER_SNAPSHOT_MAX_BYTES;
}

bool valid_item_relationships(const std::vector<player_item_snapshot> &items)
{
	std::vector<size_t> depths(items.size(), 1);
	for (size_t index = 0; index < items.size(); ++index)
	{
		const int32_t parent = items[index].parent_index;
		if (parent < PLAYER_SNAPSHOT_NO_PARENT || parent >= static_cast<int32_t>(index))
			return false;
		if (parent >= 0)
		{
			depths[index] = depths[parent] + 1;
			if (depths[index] > PLAYER_SNAPSHOT_MAX_DEPTH)
				return false;
		}
	}
	return true;
}
} // namespace

player_snapshot_codec_result
player_item_snapshot_list_encode(const std::vector<player_item_snapshot> &items,
				 std::vector<uint8_t> *encoded_out)
{
	if (!encoded_out || !valid_item_relationships(items))
		return player_snapshot_codec_result::invalid_value;
	try
	{
		encoder out;
		encode_items(out, items);
		if (!out.valid || out.bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
			return player_snapshot_codec_result::limit_exceeded;
		std::vector<player_item_snapshot> validated;
		const auto validation = player_item_snapshot_list_decode(
			out.bytes.data(), out.bytes.size(), &validated);
		if (validation != player_snapshot_codec_result::ok)
			return validation;
		*encoded_out = std::move(out.bytes);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
player_item_snapshot_list_decode(const uint8_t *encoded, size_t encoded_size,
				 std::vector<player_item_snapshot> *items_out)
{
	if (!encoded || !encoded_size || !items_out)
		return player_snapshot_codec_result::invalid_value;
	if (encoded_size > PLAYER_SNAPSHOT_MAX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		decoder in = { encoded, encoded_size };
		std::vector<player_item_snapshot> items;
		if (!decode_items(in, items))
			return in.result;
		if (in.offset != in.size || !valid_item_relationships(items))
			return player_snapshot_codec_result::invalid_value;
		*items_out = std::move(items);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
player_item_snapshot_extract_subtree(const std::vector<player_item_snapshot> &items,
				     uint64_t selected_uid,
				     std::vector<player_item_snapshot> *selected_out,
				     std::vector<player_item_snapshot> *remaining_out)
{
	if (!selected_uid || !selected_out || !remaining_out || !valid_item_relationships(items))
		return player_snapshot_codec_result::invalid_value;
	size_t selected_index = items.size();
	for (size_t index = 0; index < items.size(); ++index)
		if (items[index].object_uid == selected_uid)
		{
			selected_index = index;
			break;
		}
	if (selected_index == items.size())
		return player_snapshot_codec_result::invalid_value;
	try
	{
		std::vector<player_item_snapshot> selected;
		std::vector<player_item_snapshot> remaining;
		std::vector<bool> included(items.size(), false);
		std::vector<int32_t> selected_positions(items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		std::vector<int32_t> remaining_positions(items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		selected.reserve(items.size());
		remaining.reserve(items.size());
		for (size_t index = 0; index < items.size(); ++index)
		{
			if (index == selected_index)
				included[index] = true;
			else if (items[index].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				included[index] =
					included[static_cast<size_t>(items[index].parent_index)];
			if (included[index])
			{
				selected_positions[index] = static_cast<int32_t>(selected.size());
				auto item = items[index];
				item.parent_index = index == selected_index ?
							    PLAYER_SNAPSHOT_NO_PARENT :
							    selected_positions[static_cast<size_t>(
								    items[index].parent_index)];
				if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
				    index != selected_index)
					return player_snapshot_codec_result::invalid_value;
				selected.push_back(std::move(item));
			}
			else
			{
				remaining_positions[index] = static_cast<int32_t>(remaining.size());
				auto item = items[index];
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				{
					item.parent_index = remaining_positions[static_cast<size_t>(
						items[index].parent_index)];
					if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
						return player_snapshot_codec_result::invalid_value;
				}
				remaining.push_back(std::move(item));
			}
		}
		*selected_out = std::move(selected);
		*remaining_out = std::move(remaining);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result player_snapshot_encode(const player_snapshot &snapshot,
						    std::vector<uint8_t> *encoded_out)
{
	if (!encoded_out || !valid_metadata(snapshot) || !valid_item_relationships(snapshot.items))
		return player_snapshot_codec_result::invalid_value;
	for (const player_pet_snapshot &pet : snapshot.pets)
		if (!valid_item_relationships(pet.items))
			return player_snapshot_codec_result::invalid_value;
	try
	{
		encoder out;
		out.bytes.reserve(snapshot.encoded_size_bound);
		out.number<uint32_t>(snapshot.schema_version);
		out.number<int32_t>(snapshot.pid);
		out.number<player_revision_t>(snapshot.revision);
		out.number<player_component_mask_t>(snapshot.components);
		out.number<int32_t>(snapshot.save_intent);
		out.number<int32_t>(snapshot.room_vnum);
		out.number<uint64_t>(snapshot.encoded_size_bound);
		out.vector(snapshot.status_integers,
			   [&](const auto &row)
			   {
				   out.number<uint16_t>(static_cast<uint16_t>(row.field));
				   out.number<int64_t>(row.signed_value);
				   out.number<uint64_t>(row.unsigned_value);
				   out.boolean(row.is_unsigned);
			   });
		out.vector(snapshot.status_strings,
			   [&](const auto &row)
			   {
				   out.number<uint8_t>(static_cast<uint8_t>(row.field));
				   out.string(row.value);
			   });
		for (int32_t value : snapshot.conditions)
			out.number<int32_t>(value);
		for (int32_t value : snapshot.quest_values)
			out.number<int32_t>(value);
		encode_index_rows(out, snapshot.languages);
		encode_index_rows(out, snapshot.introductions);
		encode_index_rows(out, snapshot.timers);
		encode_index_rows(out, snapshot.undead_slots);
		encode_index_rows(out, snapshot.forged_items);
		out.vector(snapshot.granted_commands,
			   [&](int32_t command) { out.number<int32_t>(command); });
		out.vector(snapshot.skills,
			   [&](const auto &row)
			   {
				   out.number<int32_t>(row.skill_id);
				   out.number<uint8_t>(row.learned);
				   out.number<uint8_t>(row.taught);
			   });
		out.vector(snapshot.affects,
			   [&](const auto &row)
			   {
				   out.number<int16_t>(row.type);
				   out.number<int32_t>(row.duration);
				   out.number<uint32_t>(row.flags);
				   out.number<int32_t>(row.modifier);
				   out.number<uint8_t>(row.location);
				   out.number<uint16_t>(row.level);
				   for (uint64_t bitvector : row.bitvectors)
					   out.number<uint64_t>(bitvector);
				   out.string(row.wear_off_character);
				   out.string(row.wear_off_room);
			   });
		encode_items(out, snapshot.items);
		out.vector(snapshot.pets,
			   [&](const auto &pet)
			   {
				   out.number<int32_t>(pet.mob_vnum);
				   out.number<int32_t>(pet.order);
				   out.number<int32_t>(pet.hit);
				   out.number<int32_t>(pet.max_hit);
				   out.number<int32_t>(pet.mana);
				   out.number<int32_t>(pet.max_mana);
				   out.number<int32_t>(pet.vitality);
				   out.number<int32_t>(pet.max_vitality);
				   out.number<int32_t>(pet.charm_duration);
				   out.number<int32_t>(pet.room_vnum);
				   encode_items(out, pet.items);
			   });
		out.vector(snapshot.shapes,
			   [&](const auto &row)
			   {
				   out.number<int32_t>(row.mob_vnum);
				   out.number<int32_t>(row.times_researched);
				   out.number<int64_t>(row.last_researched);
				   out.number<int64_t>(row.last_shapechanged);
			   });
		out.vector(snapshot.trophies,
			   [&](const auto &row)
			   {
				   out.number<int32_t>(row.zone_number);
				   out.number<int32_t>(row.experience);
			   });
		out.boolean(snapshot.recipes_are_external);
		if (!out.valid || out.bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
			return player_snapshot_codec_result::limit_exceeded;
		player_snapshot validated = {};
		const player_snapshot_codec_result validation =
			player_snapshot_decode(out.bytes.data(), out.bytes.size(), &validated);
		if (validation != player_snapshot_codec_result::ok)
			return validation;
		*encoded_out = std::move(out.bytes);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result player_snapshot_decode(const uint8_t *encoded, size_t encoded_size,
						    player_snapshot *snapshot_out)
{
	if (!encoded || !encoded_size || !snapshot_out)
		return player_snapshot_codec_result::invalid_value;
	if (encoded_size > PLAYER_SNAPSHOT_MAX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		decoder in = { encoded, encoded_size };
		player_snapshot snapshot = {};
		uint64_t encoded_bound = 0;
		if (!in.number(snapshot.schema_version))
			return in.result;
		if (snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION)
			return player_snapshot_codec_result::unsupported_version;
		if (!in.number(snapshot.pid) || !in.number(snapshot.revision) ||
		    !in.number(snapshot.components) || !in.number(snapshot.save_intent) ||
		    !in.number(snapshot.room_vnum) || !in.number(encoded_bound))
			return in.result;
		snapshot.encoded_size_bound = encoded_bound;
		if (!valid_metadata(snapshot))
			return player_snapshot_codec_result::invalid_value;
		if (!in.vector(snapshot.status_integers,
			       [&](auto &row)
			       {
				       uint16_t field = 0;
				       if (!in.number(field) ||
					   field > static_cast<uint16_t>(
							   player_status_field::last_ip) ||
					   !in.number(row.signed_value) ||
					   !in.number(row.unsigned_value) ||
					   !in.boolean(row.is_unsigned))
					       return false;
				       row.field = static_cast<player_status_field>(field);
				       return true;
			       }) ||
		    !in.vector(snapshot.status_strings,
			       [&](auto &row)
			       {
				       uint8_t field = 0;
				       if (!in.number(field) ||
					   field > static_cast<uint8_t>(
							   player_status_string_field::poof_out) ||
					   !in.string(row.value))
					       return false;
				       row.field = static_cast<player_status_string_field>(field);
				       return true;
			       }))
			return in.result;
		for (int32_t &value : snapshot.conditions)
			if (!in.number(value))
				return in.result;
		for (int32_t &value : snapshot.quest_values)
			if (!in.number(value))
				return in.result;
		if (!decode_index_rows(in, snapshot.languages) ||
		    !decode_index_rows(in, snapshot.introductions) ||
		    !decode_index_rows(in, snapshot.timers) ||
		    !decode_index_rows(in, snapshot.undead_slots) ||
		    !decode_index_rows(in, snapshot.forged_items) ||
		    !in.vector(snapshot.granted_commands,
			       [&](int32_t &command) { return in.number(command); }) ||
		    !in.vector(snapshot.skills,
			       [&](auto &row) {
				       return in.number(row.skill_id) && in.number(row.learned) &&
					      in.number(row.taught);
			       }) ||
		    !in.vector(snapshot.affects,
			       [&](auto &row)
			       {
				       if (!in.number(row.type) || !in.number(row.duration) ||
					   !in.number(row.flags) || !in.number(row.modifier) ||
					   !in.number(row.location) || !in.number(row.level))
					       return false;
				       for (uint64_t &bitvector : row.bitvectors)
					       if (!in.number(bitvector))
						       return false;
				       return in.string(row.wear_off_character) &&
					      in.string(row.wear_off_room);
			       }) ||
		    !decode_items(in, snapshot.items) ||
		    !in.vector(snapshot.pets,
			       [&](auto &pet)
			       {
				       return in.number(pet.mob_vnum) && in.number(pet.order) &&
					      in.number(pet.hit) && in.number(pet.max_hit) &&
					      in.number(pet.mana) && in.number(pet.max_mana) &&
					      in.number(pet.vitality) &&
					      in.number(pet.max_vitality) &&
					      in.number(pet.charm_duration) &&
					      in.number(pet.room_vnum) &&
					      decode_items(in, pet.items);
			       }) ||
		    !in.vector(snapshot.shapes,
			       [&](auto &row)
			       {
				       return in.number(row.mob_vnum) &&
					      in.number(row.times_researched) &&
					      in.number(row.last_researched) &&
					      in.number(row.last_shapechanged);
			       }) ||
		    !in.vector(
			    snapshot.trophies, [&](auto &row)
			    { return in.number(row.zone_number) && in.number(row.experience); }) ||
		    !in.boolean(snapshot.recipes_are_external))
			return in.result;
		if (in.offset != in.size)
			return player_snapshot_codec_result::invalid_value;
		if (!valid_item_relationships(snapshot.items))
			return player_snapshot_codec_result::invalid_value;
		for (const player_pet_snapshot &pet : snapshot.pets)
			if (!valid_item_relationships(pet.items))
				return player_snapshot_codec_result::invalid_value;
		*snapshot_out = std::move(snapshot);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}
