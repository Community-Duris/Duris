#include "player/player_snapshot_codec.h"
#include "net/output_preference_state.h"
#include "core/files.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <new>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace
{
constexpr uint32_t item_properties_magic = UINT32_C(0x31525049);

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

	void string(const std::string &value, size_t maximum = PLAYER_SNAPSHOT_MAX_STRING_BYTES)
	{
		if (value.size() > maximum)
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

	bool string(std::string &value, size_t maximum = PLAYER_SNAPSHOT_MAX_STRING_BYTES)
	{
		uint32_t length = 0;
		if (!number(length))
			return false;
		if (length > maximum)
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
	return (snapshot.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
		snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ||
		snapshot.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
		snapshot.schema_version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION ||
		player_snapshot_is_death_request_schema(snapshot.schema_version) ||
		player_snapshot_is_death_evidence_schema(snapshot.schema_version)) &&
	       snapshot.pid > 0 && snapshot.revision && snapshot.components &&
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

bool nonzero_operation(const critical_operation_id &id)
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t byte) { return byte != 0; });
}

bool valid_death(const player_snapshot &snapshot)
{
	if (snapshot.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
	    snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ||
	    snapshot.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
	    snapshot.schema_version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION)
		return !snapshot.death;
	if (!snapshot.death || snapshot.save_intent != RENT_DEATH ||
	    snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL || !snapshot.items.empty() ||
	    std::any_of(snapshot.pets.begin(), snapshot.pets.end(),
			[](const auto &pet) { return pet.hold_reason == pet_hold_reason::none; }))
		return false;
	const auto &death = *snapshot.death;
	if (player_snapshot_is_death_evidence_schema(snapshot.schema_version) !=
	    death.conflict_evidence.has_value())
		return false;
	if (!nonzero_operation(death.operation_id) || death.corpse_room_vnum <= 0 ||
	    !death.wallet_revision || death.corpse.empty() ||
	    !valid_item_relationships(death.corpse))
		return false;
	const auto &corpse = death.corpse.front();
	if (corpse.vnum != VOBJ_CORPSE || corpse.type != ITEM_CORPSE ||
	    corpse.values[CORPSE_PID] != snapshot.pid || corpse.values[CORPSE_SAVEID] <= 0 ||
	    !(corpse.values[CORPSE_FLAGS] & PC_CORPSE))
		return false;
	bool has_wallet = false;
	for (int32_t amount : death.wallet_before)
	{
		if (amount < 0)
			return false;
		has_wallet = has_wallet || amount != 0;
	}
	if (has_wallet != (death.wallet_pile_uid != 0))
		return false;
	std::unordered_set<uint64_t> captured;
	for (size_t index = 0; index < death.corpse.size(); ++index)
	{
		const auto &item = death.corpse[index];
		if (!item.object_uid || !captured.insert(item.object_uid).second ||
		    (index && item.parent_index == PLAYER_SNAPSHOT_NO_PARENT))
			return false;
		if (item.object_uid == death.wallet_pile_uid)
		{
			if (item.vnum != VOBJ_COINS || item.type != ITEM_MONEY ||
			    item.parent_index != 0)
				return false;
			for (size_t denomination = 0; denomination < death.wallet_before.size();
			     ++denomination)
				if (item.values[denomination] != death.wallet_before[denomination])
					return false;
		}
	}
	if (has_wallet && (death.wallet_pile_uid == death.corpse.front().object_uid ||
			   !captured.count(death.wallet_pile_uid)))
		return false;
	captured.erase(death.corpse.front().object_uid); // Lifecycle owns the corpse itself.
	std::unordered_set<uint64_t> observed;
	for (const auto &row : death.custody)
	{
		if (!row.item.item_uid || !observed.insert(row.item.item_uid).second ||
		    row.item.vnum <= 0 ||
		    row.item.expected_state > item_custody_state::quarantined ||
		    row.owner.type > item_owner_type::native_mobile ||
		    (row.owner.type == item_owner_type::native_mobile &&
		     (!row.owner.id || row.owner.id == UINT64_MAX || row.owner.context_id)))
			return false;
		if (row.item.expected_state == item_custody_state::absent)
		{
			if (row.item.expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION ||
			    row.owner.type != item_owner_type::unknown || row.owner.id ||
			    row.owner.context_id || row.owner_revision)
				return false;
		}
		else if (!row.item.expected_item_revision ||
			 row.item.expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
			 !row.item.root_item_uid || row.owner.type == item_owner_type::unknown)
			return false;
		captured.erase(row.item.item_uid);
	}
	if (!captured.empty())
		return false;
	std::unordered_set<std::string> operations;
	for (const auto &id : death.unresolved_operations)
		if (!nonzero_operation(id) || id.bytes == death.operation_id.bytes ||
		    !operations
			     .insert(std::string(reinterpret_cast<const char *>(id.bytes.data()),
						 id.bytes.size()))
			     .second)
			return false;
	return true;
}

bool valid_quest_xp_receipts(const player_snapshot &snapshot)
{
	if ((snapshot.schema_version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION ||
	     player_snapshot_has_craft_receipt_schema(snapshot.schema_version)) &&
	    snapshot.quest_xp_receipts.empty())
		return true;
	if (!player_snapshot_has_quest_receipt_schema(snapshot.schema_version))
		return snapshot.quest_xp_receipts.empty();
	if (snapshot.quest_xp_receipts.empty() || snapshot.quest_xp_receipts.size() > 64)
		return false;
	for (size_t index = 0; index < snapshot.quest_xp_receipts.size(); ++index)
	{
		const auto &receipt = snapshot.quest_xp_receipts[index];
		if (!nonzero_operation(receipt.offering_operation) || receipt.reward_index >= 64 ||
		    !receipt.amount)
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (snapshot.quest_xp_receipts[prior].offering_operation.bytes ==
				    receipt.offering_operation.bytes &&
			    snapshot.quest_xp_receipts[prior].reward_index == receipt.reward_index)
				return false;
	}
	return true;
}

bool valid_spell_effect_receipts(const player_snapshot &snapshot)
{
	if (!player_snapshot_has_spell_receipt_schema(snapshot.schema_version))
		return snapshot.spell_effect_receipts.empty();
	if ((player_snapshot_has_craft_receipt_schema(snapshot.schema_version) ||
	     snapshot.schema_version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION ||
	     snapshot.schema_version == PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION) &&
	    snapshot.spell_effect_receipts.empty())
		return true;
	if (snapshot.spell_effect_receipts.empty() ||
	    !(snapshot.components & PLAYER_COMPONENT_AFFECTS) ||
	    snapshot.spell_effect_receipts.size() > PLAYER_SPELL_EFFECT_RECEIPT_MAX)
		return false;
	for (size_t index = 0; index < snapshot.spell_effect_receipts.size(); ++index)
	{
		const auto &receipt = snapshot.spell_effect_receipts[index];
		if (!nonzero_operation(receipt.operation_id) || !receipt.effect_id ||
		    receipt.effect_id > PLAYER_SPELL_EFFECT_RECEIPT_EFFECT_MAX)
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (snapshot.spell_effect_receipts[prior].operation_id.bytes ==
			    receipt.operation_id.bytes)
				return false;
	}
	return true;
}

bool valid_craft_receipts(const player_snapshot &snapshot)
{
	if (!player_snapshot_has_craft_receipt_schema(snapshot.schema_version))
		return snapshot.craft_receipts.empty();
	constexpr auto required = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS |
				  PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
	if (snapshot.craft_receipts.empty() ||
	    snapshot.craft_receipts.size() > PLAYER_CRAFT_RECEIPT_MAX ||
	    (snapshot.components & required) != required)
		return false;
	for (size_t index = 0; index < snapshot.craft_receipts.size(); ++index)
	{
		const auto &receipt = snapshot.craft_receipts[index];
		if (!nonzero_operation(receipt.operation_id) ||
		    (receipt.discipline < 1 || receipt.discipline > 6) ||
		    (receipt.discipline > 2 && receipt.experience) ||
		    receipt.experience > INT32_MAX)
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (snapshot.craft_receipts[prior].operation_id.bytes ==
			    receipt.operation_id.bytes)
				return false;
	}
	return true;
}

void encode_death(encoder &out, const player_death_snapshot &death)
{
	for (uint8_t byte : death.operation_id.bytes)
		out.number<uint8_t>(byte);
	out.number<int32_t>(death.corpse_room_vnum);
	out.number<uint64_t>(death.wallet_revision);
	for (int32_t amount : death.wallet_before)
		out.number<int32_t>(amount);
	out.number<uint64_t>(death.wallet_pile_uid);
	encode_items(out, death.corpse);
	out.vector(death.custody,
		   [&](const auto &row)
		   {
			   out.number<uint64_t>(row.item.item_uid);
			   out.number<uint64_t>(row.item.root_item_uid);
			   out.number<uint64_t>(row.item.parent_item_uid);
			   out.number<uint64_t>(row.item.expected_item_revision);
			   out.number<int32_t>(row.item.vnum);
			   out.number<uint8_t>(static_cast<uint8_t>(row.item.expected_state));
			   out.number<uint8_t>(static_cast<uint8_t>(row.owner.type));
			   out.number<uint64_t>(row.owner.id);
			   out.number<uint64_t>(row.owner.context_id);
			   out.number<uint64_t>(row.owner_revision);
		   });
	out.vector(death.unresolved_operations,
		   [&](const auto &id)
		   {
			   for (uint8_t byte : id.bytes)
				   out.number<uint8_t>(byte);
		   });
}

bool decode_death(decoder &in, player_death_snapshot &death)
{
	for (uint8_t &byte : death.operation_id.bytes)
		if (!in.number(byte))
			return false;
	if (!in.number(death.corpse_room_vnum) || !in.number(death.wallet_revision))
		return false;
	for (int32_t &amount : death.wallet_before)
		if (!in.number(amount))
			return false;
	return in.number(death.wallet_pile_uid) && decode_items(in, death.corpse) &&
	       in.vector(death.custody,
			 [&](auto &row)
			 {
				 uint8_t state = 0, owner = 0;
				 if (!in.number(row.item.item_uid) ||
				     !in.number(row.item.root_item_uid) ||
				     !in.number(row.item.parent_item_uid) ||
				     !in.number(row.item.expected_item_revision) ||
				     !in.number(row.item.vnum) || !in.number(state) ||
				     !in.number(owner) || !in.number(row.owner.id) ||
				     !in.number(row.owner.context_id) ||
				     !in.number(row.owner_revision))
					 return false;
				 row.item.expected_state = static_cast<item_custody_state>(state);
				 row.owner.type = static_cast<item_owner_type>(owner);
				 return true;
			 }) &&
	       in.vector(death.unresolved_operations,
			 [&](auto &id)
			 {
				 for (uint8_t &byte : id.bytes)
					 if (!in.number(byte))
						 return false;
				 return true;
			 });
}

player_snapshot_codec_result validate_evidence(const player_death_conflict_evidence &evidence)
{
	size_t budget = PLAYER_SNAPSHOT_MAX_BYTES;
	size_t rows = 0;
	const auto consume = [&](size_t count)
	{
		if (count > budget)
			return false;
		budget -= count;
		return true;
	};
	for (const auto *table : { &evidence.player_items, &evidence.player_item_affects,
				   &evidence.player_item_extra_descr, &evidence.item_current_owner,
				   &evidence.item_owner_revision })
	{
		if (table->columns.empty())
			return player_snapshot_codec_result::invalid_value;
		if (table->columns.size() > PLAYER_DEATH_EVIDENCE_MAX_COLUMNS ||
		    table->rows.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows ||
		    !consume(2 * sizeof(uint32_t)))
			return player_snapshot_codec_result::limit_exceeded;
		rows += table->rows.size();
		for (auto column = table->columns.begin(); column != table->columns.end(); ++column)
		{
			if (column->size() > PLAYER_DEATH_EVIDENCE_MAX_COLUMN_NAME_BYTES ||
			    !consume(sizeof(uint32_t)) || !consume(column->size()))
				return player_snapshot_codec_result::limit_exceeded;
			if (column->empty() ||
			    std::find(table->columns.begin(), column, *column) != column ||
			    !std::all_of(column->begin(), column->end(),
					 [](unsigned char value)
					 {
						 return (value >= 'a' && value <= 'z') ||
							(value >= 'A' && value <= 'Z') ||
							(value >= '0' && value <= '9') ||
							value == '_';
					 }))
				return player_snapshot_codec_result::invalid_value;
		}
		for (const auto &row : table->rows)
		{
			if (row.size() != table->columns.size())
				return player_snapshot_codec_result::invalid_value;
			for (const auto &cell : row)
				if (!consume(sizeof(uint8_t)) ||
				    (cell &&
				     (!consume(sizeof(uint32_t)) || !consume(cell->size()))))
					return player_snapshot_codec_result::limit_exceeded;
		}
	}
	const auto has_columns = [](const auto &table, std::initializer_list<const char *> required)
	{
		return std::all_of(required.begin(), required.end(),
				   [&](const char *name) {
					   return std::find(table.columns.begin(),
							    table.columns.end(),
							    name) != table.columns.end();
				   });
	};
	if (!rows ||
	    !has_columns(evidence.player_items,
			 { "id", "pid", "obj_uid", "vnum", "container_id" }) ||
	    !has_columns(evidence.player_item_affects,
			 { "id", "item_id", "location", "modifier" }) ||
	    !has_columns(evidence.player_item_extra_descr,
			 { "id", "item_id", "keyword", "description" }) ||
	    !has_columns(evidence.item_current_owner,
			 { "item_uid", "root_item_uid", "parent_item_uid", "item_revision", "vnum",
			   "state", "owner_type", "owner_id", "owner_context_id" }) ||
	    !has_columns(evidence.item_owner_revision,
			 { "owner_type", "owner_id", "owner_context_id", "revision" }))
		return player_snapshot_codec_result::invalid_value;
	return player_snapshot_codec_result::ok;
}

void encode_evidence(encoder &out, const player_death_conflict_evidence &evidence)
{
	// Fixed table order is part of version 10; these are observations, not inventory.
	for (const auto *table : { &evidence.player_items, &evidence.player_item_affects,
				   &evidence.player_item_extra_descr, &evidence.item_current_owner,
				   &evidence.item_owner_revision })
	{
		out.vector(table->columns, [&](const auto &column)
			   { out.string(column, PLAYER_DEATH_EVIDENCE_MAX_COLUMN_NAME_BYTES); });
		out.vector(table->rows,
			   [&](const auto &row)
			   {
				   for (const auto &cell : row)
				   {
					   out.boolean(cell.has_value());
					   if (cell)
						   out.string(*cell, PLAYER_SNAPSHOT_MAX_BYTES);
				   }
			   });
	}
}

bool decode_evidence(decoder &in, player_death_conflict_evidence &evidence)
{
	for (auto *table : { &evidence.player_items, &evidence.player_item_affects,
			     &evidence.player_item_extra_descr, &evidence.item_current_owner,
			     &evidence.item_owner_revision })
	{
		uint32_t columns = 0, rows = 0;
		if (!in.number(columns))
			return false;
		if (!columns || columns > PLAYER_DEATH_EVIDENCE_MAX_COLUMNS)
		{
			in.result = columns ? player_snapshot_codec_result::limit_exceeded :
					      player_snapshot_codec_result::invalid_value;
			return false;
		}
		table->columns.resize(columns);
		for (auto &column : table->columns)
			if (!in.string(column, PLAYER_DEATH_EVIDENCE_MAX_COLUMN_NAME_BYTES))
				return false;
		if (!in.number(rows))
			return false;
		if (rows > PLAYER_SNAPSHOT_MAX_ROWS || in.rows > PLAYER_SNAPSHOT_MAX_ROWS - rows)
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		// Each cell needs at least a NULL/present byte. Reject impossible lengths
		// before allocating the rectangular row vectors.
		if (static_cast<size_t>(rows) * columns > in.size - in.offset)
		{
			in.result = player_snapshot_codec_result::truncated;
			return false;
		}
		in.rows += rows;
		table->rows.resize(rows);
		for (auto &row : table->rows)
		{
			row.resize(columns);
			for (auto &cell : row)
			{
				bool present = false;
				if (!in.boolean(present))
					return false;
				if (present)
				{
					cell.emplace();
					if (!in.string(*cell, PLAYER_SNAPSHOT_MAX_BYTES))
						return false;
				}
			}
		}
	}
	in.result = validate_evidence(evidence);
	return in.result == player_snapshot_codec_result::ok;
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

namespace
{
// The named allocation-free scan DTO is charged by sizeof, including its profile;
// scalar call frames, allocator metadata and library-private internals are outside
// the existing explicit object/request policy.
struct live_item_encoder_scan
{
	player_item_snapshot_list_allocation_profile profile;
	size_t rows = 0, encoded = 0;
	player_snapshot_codec_result result = player_snapshot_codec_result::ok;

	bool add(size_t &total, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - total)
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		total += amount;
		return true;
	}
	bool append(size_t count) noexcept
	{
		if (count > PLAYER_SNAPSHOT_MAX_BYTES - encoded)
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		const size_t next = encoded + count;
		if (profile.canonical_encoder_storage_policy_supported &&
		    next > profile.canonical_encoded_capacity_bytes)
		{
			size_t capacity = encoded;
			if (!add(capacity, std::max(encoded, count)))
				return false;
			size_t peak = profile.canonical_encoded_capacity_bytes;
			if (!add(peak, capacity))
				return false;
			profile.canonical_encoded_capacity_bytes = capacity;
			profile.canonical_encoded_reallocation_peak_bytes =
				std::max(profile.canonical_encoded_reallocation_peak_bytes, peak);
		}
		encoded = next;
		return true;
	}
	bool numbers(size_t bytes) noexcept
	{
		for (size_t byte = 0; byte < bytes; ++byte)
			if (!append(1))
				return false;
		return true;
	}
	bool count(size_t value, size_t width, size_t &total, bool objects = false) noexcept
	{
		if (value > PLAYER_SNAPSHOT_MAX_ROWS || value > PLAYER_SNAPSHOT_MAX_ROWS - rows ||
		    (objects && value > PLAYER_SNAPSHOT_MAX_OBJECTS) ||
		    (width && value > SIZE_MAX / width))
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		rows += value;
		return add(total, value) && add(profile.decoded_row_storage_bytes, value * width) &&
		       numbers(sizeof(uint32_t));
	}
	bool string(const std::string &value) noexcept
	{
		const size_t length = value.size();
		if (length > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		if (!numbers(sizeof(uint32_t)) || !append(length) ||
		    !add(profile.string_count, 1) || !add(profile.string_content_bytes, length))
			return false;
		return !profile.fresh_decode_storage_policy_supported || length <= 15 ||
		       add(profile.decoded_string_storage_bytes, (length < 30 ? 30 : length) + 1);
	}
	bool scan(const std::vector<player_item_snapshot> &items) noexcept
	{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
		profile.fresh_decode_storage_policy_supported = true;
		profile.canonical_encoder_storage_policy_supported = true;
		profile.canonical_encoder_object_bytes = sizeof(encoder);
		profile.item_codec_decoder_object_bytes = sizeof(decoder);
#endif
		if (!count(items.size(), sizeof(player_item_snapshot), profile.item_count, true))
			return false;
		for (size_t index = 0; index < items.size(); ++index)
		{
			const auto &row = items[index];
			// Same original parent-before-child/depth rule, without its temporary
			// depth vector. No UID/relationship authority is granted by this scan.
			size_t depth = 1, ancestor = index;
			while (true)
			{
				const int32_t parent = items[ancestor].parent_index;
				if (parent < PLAYER_SNAPSHOT_NO_PARENT ||
				    parent >= static_cast<int32_t>(ancestor) ||
				    depth > PLAYER_SNAPSHOT_MAX_DEPTH)
				{
					result = player_snapshot_codec_result::invalid_value;
					return false;
				}
				if (parent == PLAYER_SNAPSHOT_NO_PARENT)
					break;
				ancestor = static_cast<size_t>(parent);
				++depth;
			}
			if (!numbers(sizeof(int32_t) + sizeof(int16_t) + sizeof(uint64_t) +
				     sizeof(int64_t) + sizeof(int32_t) + sizeof(int8_t) + sizeof(uint8_t)) ||
			    !string(row.name) || !string(row.short_description) ||
			    !string(row.description) || !string(row.action_description) ||
			    !numbers(row.values.size() * sizeof(int32_t) +
				     row.timers.size() * sizeof(int64_t) + 5 * sizeof(uint32_t) +
				     sizeof(int32_t) + sizeof(int8_t) + sizeof(int32_t) +
				     2 * sizeof(int16_t) + row.bitvectors.size() * sizeof(uint64_t)))
				return false;
			for (const auto &affect : row.affects)
				if (!numbers(affect.size() * sizeof(int16_t)))
					return false;
			if (!count(row.dynamic_affects.size(), sizeof(player_item_dynamic_affect_snapshot),
				   profile.dynamic_affect_count) ||
			    !numbers(row.dynamic_affects.size() * (2 * sizeof(int16_t) + sizeof(uint64_t))) ||
			    !count(row.extra_descriptions.size(), sizeof(player_item_extra_description_snapshot),
				   profile.extra_description_count))
				return false;
			for (const auto &description : row.extra_descriptions)
				if (!string(description.keyword) || !string(description.description) || !numbers(1) ||
				    !count(description.spell_ids.size(), sizeof(int32_t), profile.spell_id_count) ||
				    !numbers(description.spell_ids.size() * sizeof(int32_t)))
					return false;
		}
		if (profile.fresh_decode_storage_policy_supported)
		{
			profile.decoded_payload_bytes = sizeof(std::vector<player_item_snapshot>);
			if (!add(profile.decoded_payload_bytes, profile.decoded_row_storage_bytes) ||
			    !add(profile.decoded_payload_bytes, profile.decoded_string_storage_bytes))
				return false;
		}
		profile.relationship_scratch_bytes = sizeof(std::vector<size_t>);
		if (items.size() > SIZE_MAX / sizeof(size_t) ||
		    !add(profile.relationship_scratch_bytes, items.size() * sizeof(size_t)))
			return false;
		profile.canonical_encoded_bytes = encoded;
		return true;
	}
	bool scan(const std::span<const player_item_snapshot> &items) noexcept
	{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
		profile.fresh_decode_storage_policy_supported = true;
		profile.canonical_encoder_storage_policy_supported = true;
		profile.canonical_encoder_object_bytes = sizeof(encoder);
		profile.item_codec_decoder_object_bytes = sizeof(decoder);
#endif
		if (!count(items.size(), sizeof(player_item_snapshot), profile.item_count, true))
			return false;
		for (size_t index = 0; index < items.size(); ++index)
		{
			const auto &row = items[index];
			// Same original parent-before-child/depth rule, without its temporary
			// depth vector. No UID/relationship authority is granted by this scan.
			size_t depth = 1, ancestor = index;
			while (true)
			{
				const int32_t parent = items[ancestor].parent_index;
				if (parent < PLAYER_SNAPSHOT_NO_PARENT ||
				    parent >= static_cast<int32_t>(ancestor) ||
				    depth > PLAYER_SNAPSHOT_MAX_DEPTH)
				{
					result = player_snapshot_codec_result::invalid_value;
					return false;
				}
				if (parent == PLAYER_SNAPSHOT_NO_PARENT)
					break;
				ancestor = static_cast<size_t>(parent);
				++depth;
			}
			if (!numbers(sizeof(int32_t) + sizeof(int16_t) + sizeof(uint64_t) +
				     sizeof(int64_t) + sizeof(int32_t) + sizeof(int8_t) +
				     sizeof(uint8_t)) ||
			    !string(row.name) || !string(row.short_description) ||
			    !string(row.description) || !string(row.action_description) ||
			    !numbers(row.values.size() * sizeof(int32_t) +
				     row.timers.size() * sizeof(int64_t) + 5 * sizeof(uint32_t) +
				     sizeof(int32_t) + sizeof(int8_t) + sizeof(int32_t) +
				     2 * sizeof(int16_t) +
				     row.bitvectors.size() * sizeof(uint64_t)))
				return false;
			for (const auto &affect : row.affects)
				if (!numbers(affect.size() * sizeof(int16_t)))
					return false;
			if (!count(row.dynamic_affects.size(),
				   sizeof(player_item_dynamic_affect_snapshot),
				   profile.dynamic_affect_count) ||
			    !numbers(row.dynamic_affects.size() *
				     (2 * sizeof(int16_t) + sizeof(uint64_t))) ||
			    !count(row.extra_descriptions.size(),
				   sizeof(player_item_extra_description_snapshot),
				   profile.extra_description_count))
				return false;
			for (const auto &description : row.extra_descriptions)
				if (!string(description.keyword) ||
				    !string(description.description) || !numbers(1) ||
				    !count(description.spell_ids.size(), sizeof(int32_t),
					   profile.spell_id_count) ||
				    !numbers(description.spell_ids.size() * sizeof(int32_t)))
					return false;
		}
		if (profile.fresh_decode_storage_policy_supported)
		{
			profile.decoded_payload_bytes = sizeof(std::vector<player_item_snapshot>);
			if (!add(profile.decoded_payload_bytes,
				 profile.decoded_row_storage_bytes) ||
			    !add(profile.decoded_payload_bytes,
				 profile.decoded_string_storage_bytes))
				return false;
		}
		profile.relationship_scratch_bytes = sizeof(std::vector<size_t>);
		if (items.size() > SIZE_MAX / sizeof(size_t) ||
		    !add(profile.relationship_scratch_bytes, items.size() * sizeof(size_t)))
			return false;
		profile.canonical_encoded_bytes = encoded;
		return true;
	}
};
} // namespace

size_t player_item_snapshot_list_encoder_preflight_object_bytes() noexcept
{
	return sizeof(live_item_encoder_scan);
}

player_snapshot_codec_result player_item_snapshot_list_encoder_preflight(
	const std::vector<player_item_snapshot> &items,
	player_item_snapshot_list_allocation_profile *output) noexcept
{
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	live_item_encoder_scan scan;
	if (!scan.scan(items))
		return scan.result;
	*output = scan.profile;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result player_item_snapshot_list_encoder_preflight(
	const std::span<const player_item_snapshot> &items,
	player_item_snapshot_list_allocation_profile *output) noexcept
{
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	live_item_encoder_scan scan;
	if (!scan.scan(items))
		return scan.result;
	*output = scan.profile;
	return player_snapshot_codec_result::ok;
}

bool player_item_snapshot_list_encoder_working_bytes(
	const player_item_snapshot_list_allocation_profile &profile, size_t *output) noexcept
{
	if (!output || !profile.fresh_decode_storage_policy_supported ||
	    !profile.canonical_encoder_storage_policy_supported)
		return false;
	const auto add = [](size_t &total, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - total)
			return false;
		total += amount;
		return true;
	};
	size_t growth = profile.canonical_encoder_object_bytes;
	size_t validation = profile.canonical_encoder_object_bytes;
	if (!add(growth, profile.canonical_encoded_reallocation_peak_bytes) ||
	    !add(validation, profile.canonical_encoded_capacity_bytes) ||
	    !add(validation, sizeof(std::vector<player_item_snapshot>)) ||
	    !add(validation, profile.item_codec_decoder_object_bytes) ||
	    !add(validation, profile.decoded_payload_bytes) ||
	    !add(validation, profile.relationship_scratch_bytes))
		return false;
	*output = std::max({ growth, validation, profile.relationship_scratch_bytes });
	return true;
}

player_snapshot_codec_result player_item_snapshot_list_encode_bounded(
	const std::vector<player_item_snapshot> &items, std::vector<uint8_t> *output,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
	if (!output || !reserve_scratch_peak)
		return player_snapshot_codec_result::invalid_value;
	const auto admit = [&](size_t extra) noexcept
	{
		return extra <= SIZE_MAX - outer_live_scratch &&
		       reserve_scratch_peak(outer_live_scratch + extra, context);
	};
	if (sizeof(player_item_snapshot_list_allocation_profile) >
	    SIZE_MAX - sizeof(live_item_encoder_scan) ||
	    !admit(sizeof(player_item_snapshot_list_allocation_profile) + sizeof(live_item_encoder_scan)))
		return player_snapshot_codec_result::limit_exceeded;
	player_item_snapshot_list_allocation_profile profile;
	const auto status = player_item_snapshot_list_encoder_preflight(items, &profile);
	if (status != player_snapshot_codec_result::ok)
		return status;
	if (!profile.fresh_decode_storage_policy_supported ||
	    !profile.canonical_encoder_storage_policy_supported)
		return player_snapshot_codec_result::unsupported_version;
	size_t peak = 0;
	if (!player_item_snapshot_list_encoder_working_bytes(profile, &peak))
		return player_snapshot_codec_result::limit_exceeded;
	const auto add = [](size_t &total, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - total)
			return false;
		total += amount;
		return true;
	};
	if (!add(peak, sizeof(profile)) || !admit(peak))
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		return player_item_snapshot_list_encode(items, output);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	catch (...)
	{
		return player_snapshot_codec_result::invalid_value;
	}
}

size_t player_item_snapshot_list_decoder_object_bytes() noexcept
{
	return sizeof(decoder);
}

size_t player_item_snapshot_list_preflight_object_bytes() noexcept
{
	return sizeof(decoder) + sizeof(player_item_snapshot_list_allocation_profile);
}

player_snapshot_codec_result player_item_snapshot_list_preflight(
	const uint8_t *encoded, size_t encoded_size,
	player_item_snapshot_list_allocation_profile *profile_out) noexcept
{
	if (!encoded || !encoded_size || !profile_out)
		return player_snapshot_codec_result::invalid_value;
	if (encoded_size > PLAYER_SNAPSHOT_MAX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	decoder in = { encoded, encoded_size };
	player_item_snapshot_list_allocation_profile profile;
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
	profile.fresh_decode_storage_policy_supported = true;
	profile.canonical_encoder_storage_policy_supported = true;
	profile.canonical_encoder_object_bytes = sizeof(encoder);
	profile.item_codec_decoder_object_bytes = sizeof(decoder);
#endif
	// Only checked scalar arithmetic and decoder::number/boolean are used here.
	// Never call decoder::vector/string: both allocate before full-span validation.
	auto add = [&](size_t &total, size_t value)
	{
		if (value > std::numeric_limits<size_t>::max() - total)
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		total += value;
		return true;
	};
	size_t encoded_append_size = 0;
	auto append = [&](size_t count)
	{
		size_t next_size = encoded_append_size;
		if (!add(next_size, count))
			return false;
		if (profile.canonical_encoder_storage_policy_supported &&
		    next_size > profile.canonical_encoded_capacity_bytes)
		{
			// libstdc++13 vector<uint8_t>::_M_check_len(count):
			// size + max(size,count). The old buffer remains allocated until
			// the new buffer is populated. Zero-byte insert does not grow.
			size_t next_capacity = encoded_append_size;
			if (!add(next_capacity, std::max(encoded_append_size, count)))
				return false;
			// The original span limit keeps all supported requests far below
			// the pinned allocator/vector max_size, so no clipping is reachable.
			size_t reallocation_peak = profile.canonical_encoded_capacity_bytes;
			if (!add(reallocation_peak, next_capacity))
				return false;
			profile.canonical_encoded_capacity_bytes = next_capacity;
			profile.canonical_encoded_reallocation_peak_bytes =
				std::max(profile.canonical_encoded_reallocation_peak_bytes,
					 reallocation_peak);
		}
		encoded_append_size = next_size;
		return true;
	};
	auto number_bytes = [&](size_t bytes)
	{
		// encoder::number uses one push_back per little-endian byte.
		for (size_t byte = 0; byte < bytes; ++byte)
			if (!append(1))
				return false;
		return true;
	};
	auto storage = [&](size_t count, size_t width)
	{
		if (count && width > std::numeric_limits<size_t>::max() / count)
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		return add(profile.decoded_row_storage_bytes, count * width);
	};
	auto skip = [&](size_t bytes)
	{
		if (bytes > in.size - in.offset)
		{
			in.result = player_snapshot_codec_result::truncated;
			return false;
		}
		in.offset += bytes;
		return number_bytes(bytes);
	};
	auto skip_product = [&](size_t count, size_t width)
	{
		if (count && width > std::numeric_limits<size_t>::max() / count)
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		return skip(count * width);
	};
	auto count = [&](uint32_t &value, size_t width, size_t &total, bool objects = false)
	{
		if (!in.number(value) || !number_bytes(sizeof(value)))
			return false;
		if (value > PLAYER_SNAPSHOT_MAX_ROWS ||
		    in.rows > PLAYER_SNAPSHOT_MAX_ROWS - value ||
		    (objects && value > PLAYER_SNAPSHOT_MAX_OBJECTS))
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		in.rows += value;
		return add(total, value) && storage(value, width);
	};
	auto string = [&]()
	{
		uint32_t length = 0;
		if (!in.number(length) || !number_bytes(sizeof(length)))
			return false;
		if (length > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		if (length > in.size - in.offset)
		{
			in.result = player_snapshot_codec_result::truncated;
			return false;
		}
		in.offset += length;
		// encoder::string inserts the complete string body in one operation.
		if (!append(length) || !add(profile.string_count, 1) ||
		    !add(profile.string_content_bytes, length))
			return false;
		// libstdc++ 13 basic_string<char>: local capacity 15. Fresh assign
		// uses _M_create(new_length, 15), doubling to 30 for lengths 16..29,
		// then requests capacity+1 chars. SSO chars are already in sizeof(row).
		if (profile.fresh_decode_storage_policy_supported && length > 15)
			return add(profile.decoded_string_storage_bytes,
				   (length < 30 ? 30 : static_cast<size_t>(length)) + 1);
		return true;
	};
	uint32_t items = 0;
	if (!count(items, sizeof(player_item_snapshot), profile.item_count, true))
		return in.result;
	for (uint32_t item = 0; item < items; ++item)
	{
		if (!skip(sizeof(int32_t) + sizeof(int16_t) + sizeof(uint64_t) + sizeof(int64_t) +
			  sizeof(int32_t) + sizeof(int8_t) + sizeof(uint8_t)) ||
		    !string() || !string() || !string() || !string() ||
		    !skip(8 * sizeof(int32_t) + 6 * sizeof(int64_t) + 5 * sizeof(uint32_t) +
			  sizeof(int32_t) + sizeof(int8_t) + sizeof(int32_t) + 2 * sizeof(int16_t) +
			  5 * sizeof(uint64_t) + 8 * sizeof(int16_t)))
			return in.result;
		uint32_t affects = 0;
		if (!count(affects, sizeof(player_item_dynamic_affect_snapshot),
			   profile.dynamic_affect_count) ||
		    !skip_product(affects, 2 * sizeof(int16_t) + sizeof(uint64_t)))
			return in.result;
		uint32_t descriptions = 0;
		if (!count(descriptions, sizeof(player_item_extra_description_snapshot),
			   profile.extra_description_count))
			return in.result;
		for (uint32_t description = 0; description < descriptions; ++description)
		{
			bool spellbook = false;
			uint32_t spells = 0;
			if (!string() || !string() || !in.boolean(spellbook) || !number_bytes(1) ||
			    !count(spells, sizeof(int32_t), profile.spell_id_count) ||
			    !skip_product(spells, sizeof(int32_t)))
				return in.result;
		}
	}
	if (in.offset != in.size || encoded_append_size != encoded_size)
		return player_snapshot_codec_result::invalid_value;
	if (profile.fresh_decode_storage_policy_supported)
	{
		profile.decoded_payload_bytes = sizeof(std::vector<player_item_snapshot>);
		if (!add(profile.decoded_payload_bytes, profile.decoded_row_storage_bytes) ||
		    !add(profile.decoded_payload_bytes, profile.decoded_string_storage_bytes))
			return in.result;
	}
	if (profile.item_count > std::numeric_limits<size_t>::max() / sizeof(size_t))
		return player_snapshot_codec_result::limit_exceeded;
	profile.relationship_scratch_bytes = sizeof(std::vector<size_t>);
	if (!add(profile.relationship_scratch_bytes, profile.item_count * sizeof(size_t)))
		return in.result;
	profile.canonical_encoded_bytes = encoded_size;
	*profile_out = profile;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result player_item_properties_encode(
	uint32_t extra2_flags,
	const std::vector<player_item_dynamic_affect_snapshot> &dynamic_affects,
	std::string *encoded_hex_out)
{
	if (!encoded_hex_out)
		return player_snapshot_codec_result::invalid_value;
	if (dynamic_affects.size() > PLAYER_SNAPSHOT_MAX_ROWS)
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		encoder out;
		out.number<uint32_t>(item_properties_magic);
		out.number<uint32_t>(1);
		out.number<uint32_t>(extra2_flags);
		out.vector(dynamic_affects,
			   [&](const auto &affect)
			   {
				   out.number<int16_t>(affect.type);
				   out.number<int16_t>(affect.data);
				   out.number<uint64_t>(affect.extra2);
			   });
		if (!out.valid || out.bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
			return player_snapshot_codec_result::limit_exceeded;
		constexpr char digits[] = "0123456789abcdef";
		std::string encoded;
		encoded.reserve(out.bytes.size() * 2);
		for (uint8_t byte : out.bytes)
		{
			encoded.push_back(digits[byte >> 4]);
			encoded.push_back(digits[byte & 0x0f]);
		}
		*encoded_hex_out = std::move(encoded);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
player_item_properties_decode(const std::string &encoded_hex, uint32_t *extra2_flags_out,
			      std::vector<player_item_dynamic_affect_snapshot> *dynamic_affects_out)
{
	if (!extra2_flags_out || !dynamic_affects_out || encoded_hex.empty() ||
	    (encoded_hex.size() & 1))
		return player_snapshot_codec_result::invalid_value;
	if (encoded_hex.size() > PLAYER_ITEM_PROPERTIES_MAX_HEX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	auto nibble = [](char value) -> int
	{
		if (value >= '0' && value <= '9')
			return value - '0';
		if (value >= 'a' && value <= 'f')
			return value - 'a' + 10;
		if (value >= 'A' && value <= 'F')
			return value - 'A' + 10;
		return -1;
	};
	try
	{
		std::vector<uint8_t> bytes;
		bytes.reserve(encoded_hex.size() / 2);
		for (size_t index = 0; index < encoded_hex.size(); index += 2)
		{
			const int high = nibble(encoded_hex[index]);
			const int low = nibble(encoded_hex[index + 1]);
			if (high < 0 || low < 0)
				return player_snapshot_codec_result::invalid_value;
			bytes.push_back(static_cast<uint8_t>((high << 4) | low));
		}
		decoder in = { bytes.data(), bytes.size() };
		uint32_t magic = 0;
		uint32_t version = 0;
		uint32_t extra2_flags = 0;
		std::vector<player_item_dynamic_affect_snapshot> dynamic_affects;
		if (!in.number(magic) || !in.number(version) || !in.number(extra2_flags))
			return in.result;
		if (magic != item_properties_magic)
			return player_snapshot_codec_result::invalid_value;
		if (version != 1)
			return player_snapshot_codec_result::unsupported_version;
		if (!in.vector(dynamic_affects,
			       [&](auto &affect) {
				       return in.number(affect.type) && in.number(affect.data) &&
					      in.number(affect.extra2);
			       }))
			return in.result;
		if (in.offset != in.size)
			return player_snapshot_codec_result::invalid_value;
		*extra2_flags_out = extra2_flags;
		*dynamic_affects_out = std::move(dynamic_affects);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

const char *player_item_properties_sql_column_suffix()
{
	return ",item_properties";
}

player_snapshot_codec_result player_item_properties_sql_value_suffix(
	uint32_t extra2_flags,
	const std::vector<player_item_dynamic_affect_snapshot> &dynamic_affects,
	std::string *value_suffix_out)
{
	if (!value_suffix_out)
		return player_snapshot_codec_result::invalid_value;
	std::string encoded_hex;
	const player_snapshot_codec_result encoded =
		player_item_properties_encode(extra2_flags, dynamic_affects, &encoded_hex);
	if (encoded != player_snapshot_codec_result::ok)
		return encoded;
	try
	{
		std::string value_suffix;
		value_suffix.reserve(encoded_hex.size() + 3);
		value_suffix.append(",'");
		value_suffix.append(encoded_hex);
		value_suffix.push_back('\'');
		*value_suffix_out = std::move(value_suffix);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result player_item_properties_decode_sql_row(
	const char *encoded_hex, const char *octet_length, uint32_t *extra2_flags_out,
	std::vector<player_item_dynamic_affect_snapshot> *dynamic_affects_out,
	bool *has_payload_out)
{
	if (!extra2_flags_out || !dynamic_affects_out || !has_payload_out)
		return player_snapshot_codec_result::invalid_value;
	if (!encoded_hex)
	{
		if (octet_length)
			return player_snapshot_codec_result::invalid_value;
		*has_payload_out = false;
		return player_snapshot_codec_result::ok;
	}
	if (!octet_length || !*octet_length || *octet_length == '-')
		return player_snapshot_codec_result::invalid_value;
	errno = 0;
	char *end = nullptr;
	const unsigned long long parsed_length = std::strtoull(octet_length, &end, 10);
	if (errno == ERANGE || end == octet_length || *end)
		return player_snapshot_codec_result::invalid_value;
	if (parsed_length > PLAYER_ITEM_PROPERTIES_MAX_HEX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		const std::string encoded(encoded_hex, static_cast<size_t>(parsed_length));
		uint32_t decoded_flags = 0;
		std::vector<player_item_dynamic_affect_snapshot> decoded_affects;
		const player_snapshot_codec_result decoded =
			player_item_properties_decode(encoded, &decoded_flags, &decoded_affects);
		if (decoded != player_snapshot_codec_result::ok)
			return decoded;
		*extra2_flags_out = decoded_flags;
		*dynamic_affects_out = std::move(decoded_affects);
		*has_payload_out = true;
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

player_snapshot_codec_result
player_item_snapshot_extract_forest(const std::vector<player_item_snapshot> &items,
				    const std::vector<uint64_t> &selected_root_uids,
				    std::vector<player_item_snapshot> *selected_out,
				    std::vector<player_item_snapshot> *remaining_out)
{
	if (selected_root_uids.empty() || !selected_out || !remaining_out ||
	    std::any_of(selected_root_uids.begin(), selected_root_uids.end(),
			[](uint64_t uid) { return uid == 0; }) ||
	    std::adjacent_find(selected_root_uids.begin(), selected_root_uids.end(),
			       [](uint64_t left, uint64_t right)
			       { return left >= right; }) != selected_root_uids.end())
		return player_snapshot_codec_result::invalid_value;
	try
	{
		std::vector<player_item_snapshot> selected;
		std::vector<player_item_snapshot> remaining = items;
		for (uint64_t selected_root_uid : selected_root_uids)
		{
			std::vector<player_item_snapshot> tree;
			std::vector<player_item_snapshot> next_remaining;
			const auto extracted = player_item_snapshot_extract_subtree(
				remaining, selected_root_uid, &tree, &next_remaining);
			if (extracted != player_snapshot_codec_result::ok)
				return extracted;
			const int32_t offset = static_cast<int32_t>(selected.size());
			for (auto &item : tree)
			{
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += offset;
				selected.push_back(std::move(item));
			}
			remaining = std::move(next_remaining);
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
	if (!encoded_out || !valid_metadata(snapshot) ||
	    !valid_item_relationships(snapshot.items) || !valid_quest_xp_receipts(snapshot) ||
	    !valid_spell_effect_receipts(snapshot) || !valid_craft_receipts(snapshot))
		return player_snapshot_codec_result::invalid_value;
	for (const player_pet_snapshot &pet : snapshot.pets)
		if (!valid_item_relationships(pet.items))
			return player_snapshot_codec_result::invalid_value;
	try
	{
		encoder out;
		if (!valid_death(snapshot))
			return player_snapshot_codec_result::invalid_value;
		if (snapshot.death && snapshot.death->conflict_evidence)
		{
			const auto valid = validate_evidence(*snapshot.death->conflict_evidence);
			if (valid != player_snapshot_codec_result::ok)
				return valid;
		}
		out.bytes.reserve(snapshot.encoded_size_bound);
		const bool ward_fields =
			std::any_of(snapshot.affects.begin(), snapshot.affects.end(),
				    [](const auto &af) { return af.ward_source_type != 0; });
		out.number<uint32_t>(snapshot.schema_version +
				     (ward_fields ? PLAYER_SNAPSHOT_WARD_WIRE_OFFSET : 0));
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
				   if (ward_fields)
				   {
					   out.number<uint64_t>(row.ward_source_uid);
					   out.number<int32_t>(row.ward_full_duration);
					   out.number<int64_t>(row.ward_capacity);
					   out.number<int64_t>(row.ward_capacity_max);
					   out.number<int32_t>(row.ward_refresh_remaining);
					   out.number<uint8_t>(row.ward_source_type);
					   out.number<uint8_t>(row.ward_source_worn);
					   out.number<uint8_t>(row.ward_active);
				   }
				   out.string(row.wear_off_character);
				   out.string(row.wear_off_room);
			   });
		encode_items(out, snapshot.items);
		out.vector(snapshot.pets,
			   [&](const auto &pet)
			   {
				   out.number<uint64_t>(pet.pet_uid);
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
				   out.string(pet.restore_state, PET_RESTORE_STATE_MAX_BYTES);
				   out.number<uint32_t>(static_cast<uint32_t>(pet.hold_reason));
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
		out.string(snapshot.output_preferences, OUTPUT_PREFERENCE_MAX_BYTES);
		if (player_snapshot_has_quest_receipt_schema(snapshot.schema_version))
			out.vector(snapshot.quest_xp_receipts,
				   [&](const auto &receipt)
				   {
					   out.bytes.insert(
						   out.bytes.end(),
						   receipt.offering_operation.bytes.begin(),
						   receipt.offering_operation.bytes.end());
					   out.number<uint32_t>(receipt.reward_index);
					   out.number<uint32_t>(receipt.amount);
				   });
		if (player_snapshot_has_spell_receipt_schema(snapshot.schema_version))
			out.vector(snapshot.spell_effect_receipts,
				   [&](const auto &receipt)
				   {
					   for (uint8_t byte : receipt.operation_id.bytes)
						   out.number<uint8_t>(byte);
					   out.number<uint32_t>(receipt.effect_id);
				   });
		if (player_snapshot_has_craft_receipt_schema(snapshot.schema_version))
			out.vector(snapshot.craft_receipts,
				   [&](const auto &receipt)
				   {
					   for (uint8_t byte : receipt.operation_id.bytes)
						   out.number<uint8_t>(byte);
					   out.number<uint32_t>(receipt.discipline);
					   out.number<uint32_t>(receipt.experience);
				   });
		if (snapshot.death)
		{
			encode_death(out, *snapshot.death);
			if (snapshot.death->conflict_evidence)
				encode_evidence(out, *snapshot.death->conflict_evidence);
		}
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
		const uint32_t raw_wire_version = snapshot.schema_version;
		const bool ward_fields = raw_wire_version >= 20 && raw_wire_version <= 32;
		const uint32_t wire_version =
			ward_fields ? raw_wire_version - PLAYER_SNAPSHOT_WARD_WIRE_OFFSET :
				      raw_wire_version;
		snapshot.schema_version = wire_version;
		if (wire_version == 1 || wire_version == 3 || wire_version == 5 ||
		    wire_version == 7)
			snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		if (wire_version == 2 || wire_version == 4 || wire_version == 6 ||
		    wire_version == 8)
			snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
		if (snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
		    snapshot.schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION &&
		    snapshot.schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
		    snapshot.schema_version !=
			    PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
		    !player_snapshot_is_death_request_schema(snapshot.schema_version) &&
		    !player_snapshot_is_death_evidence_schema(snapshot.schema_version))
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
				       if (!in.number(field))
					       return false;
				       if (field >
					   static_cast<uint16_t>(player_status_field::last_ip))
				       {
					       in.result =
						       player_snapshot_codec_result::invalid_value;
					       return false;
				       }
				       if (!in.number(row.signed_value) ||
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
				       if (!in.number(field))
					       return false;
				       if (field > static_cast<uint8_t>(
							   player_status_string_field::poof_out))
				       {
					       in.result =
						       player_snapshot_codec_result::invalid_value;
					       return false;
				       }
				       if (!in.string(row.value))
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
				       if (ward_fields && (!in.number(row.ward_source_uid) ||
							   !in.number(row.ward_full_duration) ||
							   !in.number(row.ward_capacity) ||
							   !in.number(row.ward_capacity_max) ||
							   !in.number(row.ward_refresh_remaining) ||
							   !in.number(row.ward_source_type) ||
							   !in.number(row.ward_source_worn) ||
							   !in.number(row.ward_active)))
					       return false;
				       return in.string(row.wear_off_character) &&
					      in.string(row.wear_off_room);
			       }) ||
		    !decode_items(in, snapshot.items) ||
		    !in.vector(snapshot.pets,
			       [&](auto &pet)
			       {
				       if (wire_version >= 7 && !in.number(pet.pet_uid))
					       return false;
				       const bool base =
					       in.number(pet.mob_vnum) && in.number(pet.order) &&
					       in.number(pet.hit) && in.number(pet.max_hit) &&
					       in.number(pet.mana) && in.number(pet.max_mana) &&
					       in.number(pet.vitality) &&
					       in.number(pet.max_vitality) &&
					       in.number(pet.charm_duration) &&
					       in.number(pet.room_vnum) &&
					       decode_items(in, pet.items);
				       if (!base || wire_version < 3)
					       return base;
				       uint32_t reason = 0;
				       if (!in.string(pet.restore_state,
						      PET_RESTORE_STATE_MAX_BYTES) ||
					   !in.number(reason))
					       return false;
				       pet.hold_reason = static_cast<pet_hold_reason>(reason);
				       return true;
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
		if (wire_version >= 5 &&
		    !in.string(snapshot.output_preferences, OUTPUT_PREFERENCE_MAX_BYTES))
			return in.result;
		if (player_snapshot_has_quest_receipt_schema(wire_version) &&
		    !in.vector(snapshot.quest_xp_receipts,
			       [&](auto &receipt)
			       {
				       if (in.size - in.offset <
					   receipt.offering_operation.bytes.size())
				       {
					       in.result = player_snapshot_codec_result::truncated;
					       return false;
				       }
				       std::copy_n(in.data + in.offset,
						   receipt.offering_operation.bytes.size(),
						   receipt.offering_operation.bytes.begin());
				       in.offset += receipt.offering_operation.bytes.size();
				       return in.number(receipt.reward_index) &&
					      in.number(receipt.amount);
			       }))
			return in.result;
		if (player_snapshot_has_spell_receipt_schema(wire_version) &&
		    !in.vector(snapshot.spell_effect_receipts,
			       [&](auto &receipt)
			       {
				       if (in.size - in.offset < receipt.operation_id.bytes.size())
				       {
					       in.result = player_snapshot_codec_result::truncated;
					       return false;
				       }
				       std::copy_n(in.data + in.offset,
						   receipt.operation_id.bytes.size(),
						   receipt.operation_id.bytes.begin());
				       in.offset += receipt.operation_id.bytes.size();
				       return in.number(receipt.effect_id);
			       }))
			return in.result;
		if (player_snapshot_has_craft_receipt_schema(wire_version) &&
		    !in.vector(snapshot.craft_receipts,
			       [&](auto &receipt)
			       {
				       if (in.size - in.offset < receipt.operation_id.bytes.size())
				       {
					       in.result = player_snapshot_codec_result::truncated;
					       return false;
				       }
				       std::copy_n(in.data + in.offset,
						   receipt.operation_id.bytes.size(),
						   receipt.operation_id.bytes.begin());
				       in.offset += receipt.operation_id.bytes.size();
				       return in.number(receipt.discipline) &&
					      in.number(receipt.experience);
			       }))
			return in.result;
		if (player_snapshot_is_death_request_schema(snapshot.schema_version) ||
		    player_snapshot_is_death_evidence_schema(snapshot.schema_version))
		{
			snapshot.death.emplace();
			if (!decode_death(in, *snapshot.death))
				return in.result;
			if (player_snapshot_is_death_evidence_schema(snapshot.schema_version))
			{
				snapshot.death->conflict_evidence.emplace();
				if (!decode_evidence(in, *snapshot.death->conflict_evidence))
					return in.result;
			}
		}
		if (!valid_death(snapshot) || !valid_quest_xp_receipts(snapshot) ||
		    !valid_spell_effect_receipts(snapshot) || !valid_craft_receipts(snapshot))
			return player_snapshot_codec_result::invalid_value;
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

namespace
{
using item_list_reserve_fn = bool (*)(size_t, void *) noexcept;
bool item_list_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
template <typename T>
bool item_list_vector_heap(const std::vector<T> &value, size_t &bytes) noexcept
{
	return value.capacity() <= SIZE_MAX / sizeof(T) &&
	       item_list_add(bytes, value.capacity() * sizeof(T));
}
bool item_list_string_heap(const std::string &value, size_t &bytes) noexcept
{
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && item_list_add(bytes, value.capacity() + 1));
}
bool item_list_heap(const std::vector<player_item_snapshot> &items, size_t &bytes) noexcept
{
	if (!item_list_vector_heap(items, bytes))
		return false;
	for (const auto &row : items)
	{
		if (!item_list_string_heap(row.name, bytes) ||
		    !item_list_string_heap(row.short_description, bytes) ||
		    !item_list_string_heap(row.description, bytes) ||
		    !item_list_string_heap(row.action_description, bytes) ||
		    !item_list_vector_heap(row.dynamic_affects, bytes) ||
		    !item_list_vector_heap(row.extra_descriptions, bytes))
			return false;
		for (const auto &description : row.extra_descriptions)
			if (!item_list_string_heap(description.keyword, bytes) ||
			    !item_list_string_heap(description.description, bytes) ||
			    !item_list_vector_heap(description.spell_ids, bytes))
				return false;
	}
	return true;
}
constexpr size_t item_list_allocator_frames =
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
constexpr size_t item_list_copy_frames =
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
constexpr size_t item_list_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_list_default_frames =
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
constexpr size_t item_list_vector_frames =
	item_list_allocator_frames + item_list_copy_frames + item_list_relocate_frames +
	item_list_default_frames +
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
constexpr size_t item_list_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + item_list_allocator_frames;

// Nontrivial row/description construction and destruction are source scopes,
// not heap metadata. The nested member objects already live in sizeof(row).
constexpr size_t item_list_nontrivial_frames =
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
	3 * item_list_allocator_frames;
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t item_list_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t item_list_string_frames =
	item_list_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	item_list_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);
// Genuine vector(n,value,allocator) constructor scopes, before fill:
// vector this/n/value-reference/allocator-reference and default allocator;
// _S_check_init_len n/a/result and its _Tp allocator copy; _Vector_base
// this/n/a, _Vector_impl this/a and allocator copy, _Vector_impl_data this;
// _M_create_storage this/n. Existing allocator profile owns _S_max_size.
constexpr size_t item_list_size_constructor_frames =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<size_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<size_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);
struct item_list_decode_workspace;
struct item_list_live_frame
{
	item_list_decode_workspace &owner;
	item_list_live_frame *previous;
	size_t bytes;
	item_list_live_frame(item_list_decode_workspace &, size_t) noexcept;
	~item_list_live_frame();
};
struct item_list_decode_workspace
{
	decoder in;
	std::vector<player_item_snapshot> items;
	std::vector<size_t> depths;
	item_list_reserve_fn reserve;
	void *context;
	size_t outer;
	size_t transferred_heap = 0;
	item_list_live_frame *frames = nullptr;
	bool peak(size_t extra) noexcept
	{
		size_t bytes = outer;
		if (!item_list_add(bytes, sizeof(*this)) || !item_list_heap(items, bytes) ||
		    !item_list_vector_heap(depths, bytes))
			return false;
		for (auto *frame = frames; frame; frame = frame->previous)
			if (!item_list_add(bytes, frame->bytes))
				return false;
		// Actual peak/current-heap observer parameter/return/iteration scopes.
		constexpr size_t observation =
			7 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
			3 * (sizeof(void *) + sizeof(size_t)) + 4 * (2 * sizeof(void *));
		return item_list_add(bytes, observation) && item_list_add(bytes, extra) &&
		       reserve && reserve(bytes, context);
	}
};
item_list_live_frame::item_list_live_frame(item_list_decode_workspace &value, size_t count) noexcept
	: owner(value)
	, previous(value.frames)
	, bytes(count)
{
	owner.frames = this;
}
item_list_live_frame::~item_list_live_frame()
{
	owner.frames = previous;
}
struct item_list_bounded_decoder
{
	item_list_decode_workspace &owner;
	template <typename T> bool number(T &value) { return owner.in.number(value); }
	bool boolean(bool &value) { return owner.in.boolean(value); }
	bool string(std::string &value, size_t maximum = PLAYER_SNAPSHOT_MAX_STRING_BYTES)
	{
		constexpr size_t own = sizeof(item_list_live_frame) + 3 * sizeof(void *) +
				       4 * sizeof(size_t) + sizeof(uint32_t) + sizeof(bool);
		if (!owner.peak(own))
		{
			owner.in.result = player_snapshot_codec_result::allocation_failure;
			return false;
		}
		item_list_live_frame frame(owner, own);
		uint32_t length = 0;
		if (!number(length))
			return false;
		if (length > maximum)
		{
			owner.in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		if (owner.in.size - owner.in.offset < length)
		{
			owner.in.result = player_snapshot_codec_result::truncated;
			return false;
		}
		size_t request = 0;
		if (length > value.capacity())
		{
			// GCC13 _M_create(capacity,oldcapacity), actual doubling/clipping.
			size_t next = length;
			if (next < 2 * value.capacity())
				next = 2 * value.capacity();
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX)
			{
				owner.in.result = player_snapshot_codec_result::limit_exceeded;
				return false;
			}
			request = next + 1;
		}
		if (!item_list_add(request, item_list_string_frames) || !owner.peak(request))
		{
			owner.in.result = player_snapshot_codec_result::allocation_failure;
			return false;
		}
		value.assign(reinterpret_cast<const char *>(owner.in.data + owner.in.offset),
			     length);
		owner.in.offset += length;
		return true;
	}
	template <typename T, typename Read>
	bool vector(std::vector<T> &values, Read read, bool object_rows = false)
	{
		constexpr size_t own = sizeof(item_list_live_frame) + 4 * sizeof(void *) +
				       2 * sizeof(Read) + sizeof(uint32_t) + sizeof(bool) +
				       2 * sizeof(size_t);
		if (!owner.peak(own))
		{
			owner.in.result = player_snapshot_codec_result::allocation_failure;
			return false;
		}
		item_list_live_frame frame(owner, own);
		uint32_t count = 0;
		if (!number(count))
			return false;
		if (count > PLAYER_SNAPSHOT_MAX_ROWS ||
		    owner.in.rows > PLAYER_SNAPSHOT_MAX_ROWS - count ||
		    (object_rows && (count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
				     owner.in.objects > PLAYER_SNAPSHOT_MAX_OBJECTS - count)))
		{
			owner.in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		owner.in.rows += count;
		if (object_rows)
			owner.in.objects += count;
		size_t request = item_list_vector_frames + item_list_nontrivial_frames;
		if (count > values.capacity())
		{
			const size_t added = count - values.size();
			size_t next = values.size();
			if (!item_list_add(next, std::max(values.size(), added)) ||
			    next > values.max_size())
				next = values.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !item_list_add(request, next * sizeof(T)))
			{
				owner.in.result = player_snapshot_codec_result::limit_exceeded;
				return false;
			}
		}
		if (!owner.peak(request))
		{
			owner.in.result = player_snapshot_codec_result::allocation_failure;
			return false;
		}
		values.resize(count);
		for (T &value : values)
			if (!read(value))
				return false;
		return true;
	}
};

bool item_list_decode_items(item_list_bounded_decoder &in, std::vector<player_item_snapshot> &items)
{
	constexpr size_t own = sizeof(item_list_live_frame) + 3 * sizeof(void *) +
			       2 * sizeof(void *) + sizeof(int32_t *) + sizeof(int64_t *) +
			       sizeof(uint64_t *) + sizeof(std::array<int16_t, 2> *) +
			       sizeof(int16_t *) + sizeof(bool);
	if (!in.owner.peak(own))
	{
		in.owner.in.result = player_snapshot_codec_result::allocation_failure;
		return false;
	}
	item_list_live_frame frame(in.owner, own);
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
bool item_list_relationships(item_list_decode_workspace &owner)
{
	const auto &items = owner.items;
	constexpr size_t own = sizeof(item_list_live_frame) + 3 * sizeof(void *) + sizeof(size_t) +
			       sizeof(int32_t) + sizeof(bool);
	if (!owner.peak(own))
	{
		owner.in.result = player_snapshot_codec_result::allocation_failure;
		return false;
	}
	item_list_live_frame frame(owner, own);
	size_t request = item_list_vector_frames + item_list_move_frames +
			 item_list_size_constructor_frames +
			 // Actual returned vector carrier for the fill constructor and
			 // _Fill_initialize/uninit_fill_n_a/uninit_fill_n<true>, first/n/value,
			 // val-copy, return iterator, tag and real fill_n source scopes.
			 sizeof(std::vector<size_t>) + sizeof(size_t) +
			 4 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(size_t) + sizeof(bool) +
			 sizeof(char);
	if (items.size() > SIZE_MAX / sizeof(size_t) ||
	    !item_list_add(request, items.size() * sizeof(size_t)) || !owner.peak(request))
	{
		owner.in.result = player_snapshot_codec_result::allocation_failure;
		return false;
	}
	owner.depths = std::vector<size_t>(items.size(), 1);
	auto &depths = owner.depths;
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

// Complete original item-list wire decoder and relationship validation. The
// authentic caller outer owns input, prior output and all other live owners;
// this leaf owns each prospective request and all real simultaneous capacities.
player_snapshot_codec_result player_item_snapshot_list_decode_bounded(
	const uint8_t *encoded, size_t encoded_size, std::vector<player_item_snapshot> *items_out,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_item_heap_bytes) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)encoded;
	(void)encoded_size;
	(void)items_out;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_item_heap_bytes;
	return player_snapshot_codec_result::limit_exceeded;
#else
	if (!encoded || !encoded_size || !items_out)
		return player_snapshot_codec_result::invalid_value;
	if (encoded_size > PLAYER_SNAPSHOT_MAX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	constexpr size_t own = sizeof(item_list_live_frame) + sizeof(item_list_bounded_decoder) +
			       5 * sizeof(void *) + 3 * sizeof(size_t) +
			       sizeof(player_snapshot_codec_result) + sizeof(bool);
	size_t initial = outer_live;
	if (!reserve || !item_list_add(initial, sizeof(item_list_decode_workspace)) ||
	    !item_list_add(initial, own) || !item_list_add(initial, item_list_nontrivial_frames) ||
	    !reserve(initial, context))
		return player_snapshot_codec_result::allocation_failure;
	try
	{
		item_list_decode_workspace work{
			{ encoded, encoded_size }, {}, {}, reserve, context, outer_live
		};
		item_list_live_frame frame(work, own);
		item_list_bounded_decoder in{ work };
		if (!item_list_decode_items(in, work.items))
			return work.in.result;
		if (work.in.offset != work.in.size)
			return player_snapshot_codec_result::invalid_value;
		if (!item_list_relationships(work))
			return work.in.result == player_snapshot_codec_result::ok ?
				       player_snapshot_codec_result::invalid_value :
				       work.in.result;
		if (!item_list_heap(work.items, work.transferred_heap) ||
		    !work.peak(item_list_move_frames + item_list_nontrivial_frames))
			return player_snapshot_codec_result::allocation_failure;
		// Final genuine constant-time vector transfer is nonthrowing. No
		// callback or allocation follows this strong-output commit.
		*items_out = std::move(work.items);
		if (retained_item_heap_bytes)
			*retained_item_heap_bytes = work.transferred_heap;
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
#endif
}

namespace
{
// Source-declared carriers for the actual observer functions below, including
// range iterator/query scopes and all checked arithmetic calls. No emitted
// machine-stack or allocator metadata claim is made by this profile.
constexpr size_t snapshot_clone_observation_frames =
	13 * sizeof(void *) + 8 * sizeof(size_t) + 6 * sizeof(bool) +
	8 * (sizeof(void *) + sizeof(size_t)) + 8 * (2 * sizeof(void *));
// basic_string copy constructor: this/source, allocator select/copy result,
// allocator hider/local-data, _M_construct forward this/beg/end/tag, dnew,
// its real one-pointer _Guard and constructor/destructor this parameters;
// distance/__distance and returned difference, _S_copy_chars arguments.
// Existing string/allocator profiles own _M_create( n,0 ), data/capacity/
// set-length, traits copy, runtime memcpy and unwind disposal.
constexpr size_t snapshot_clone_string_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<char>) + 6 * sizeof(void *) +
	3 * sizeof(void *) + sizeof(std::forward_iterator_tag) + sizeof(size_t) + sizeof(void *) +
	3 * sizeof(void *) + 2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	sizeof(std::random_access_iterator_tag) + 4 * sizeof(void *) + item_list_string_frames;
// vector copy constructor calls _Vector_base(size,selected_allocator), then
// __uninitialized_copy_a; the allocated capacity is exactly source.size().
// Actual constructor/base/impl/data/create-storage/select/query carriers.
// Existing vector allocator/copy profiles own the trivial-element path.
constexpr size_t snapshot_clone_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + item_list_vector_frames;
// Nontrivial description copy reaches __do_uninit_copy: first/last/result,
// actual __cur, returned iterator, _Construct(location,source), addressof,
// forward and placement-new carriers. The generated description copy has
// this/source parameters and copies both strings and its spell vector.
constexpr size_t snapshot_clone_description_frames =
	5 * sizeof(void *) + 2 * sizeof(void *) + 4 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + 2 * snapshot_clone_string_constructor_frames +
	snapshot_clone_vector_constructor_frames;
// String move assignment/operator and _M_assign path are allocation-free
// for the standard equal allocator, including the actual _M_is_local tests,
// old-pointer/old-capacity temporaries, memcpy args and source reset. The
// existing string profile conservatively also retains all disposal scopes.
constexpr size_t snapshot_clone_string_move_frames =
	2 * sizeof(void *) + sizeof(char) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	12 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(bool) + 2 * sizeof(void *) +
	sizeof(char) + item_list_string_frames;
// The outer generated row copy/move has this/source parameters. Copies of
// fixed arrays are inline members, not new carriers or requests. Four row
// string constructors and the dynamic-affect/description vector constructors
// coexist with at most one description copy. Sum the source scopes rather
// than rely on spare bytes from a different STL path. All unwind/destructor
// closures and real vector move __tmp are prospectively retained as well.
constexpr size_t snapshot_clone_copy_frames =
	snapshot_clone_observation_frames + 4 * sizeof(void *) +
	4 * snapshot_clone_string_constructor_frames +
	2 * snapshot_clone_vector_constructor_frames + snapshot_clone_description_frames +
	4 * snapshot_clone_string_move_frames + 2 * item_list_move_frames +
	item_list_nontrivial_frames;

template <typename T>
bool snapshot_clone_vector_request(const std::vector<T> &value, bool fresh, size_t &bytes) noexcept
{
	const size_t count = fresh ? value.size() : value.capacity();
	return count <= SIZE_MAX / sizeof(T) && item_list_add(bytes, count * sizeof(T));
}
bool snapshot_clone_string_request(const std::string &value, bool fresh, size_t &bytes) noexcept
{
	const size_t count = fresh ? value.size() : value.capacity();
	return count <= 15 || (count < SIZE_MAX && item_list_add(bytes, count + 1));
}
bool snapshot_clone_row_request(const player_item_snapshot &row, bool fresh, size_t &bytes) noexcept
{
	if (!snapshot_clone_string_request(row.name, fresh, bytes) ||
	    !snapshot_clone_string_request(row.short_description, fresh, bytes) ||
	    !snapshot_clone_string_request(row.description, fresh, bytes) ||
	    !snapshot_clone_string_request(row.action_description, fresh, bytes) ||
	    !snapshot_clone_vector_request(row.dynamic_affects, fresh, bytes) ||
	    !snapshot_clone_vector_request(row.extra_descriptions, fresh, bytes))
		return false;
	for (const auto &description : row.extra_descriptions)
		if (!snapshot_clone_string_request(description.keyword, fresh, bytes) ||
		    !snapshot_clone_string_request(description.description, fresh, bytes) ||
		    !snapshot_clone_vector_request(description.spell_ids, fresh, bytes))
			return false;
	return true;
}
bool snapshot_clone_policy_supported() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return true;
#else
	return false;
#endif
}
} // namespace

bool player_item_snapshot_current_heap_bytes(const player_item_snapshot &row,
					     size_t *output) noexcept
{
	if (!output || !snapshot_clone_policy_supported())
		return false;
	size_t bytes = 0;
	if (!snapshot_clone_row_request(row, false, bytes))
		return false;
	*output = bytes;
	return true;
}
bool player_item_snapshot_fresh_copy_request_bytes(const player_item_snapshot &row,
						   size_t *output) noexcept
{
	if (!output || !snapshot_clone_policy_supported())
		return false;
	size_t bytes = 0;
	if (!snapshot_clone_row_request(row, true, bytes))
		return false;
	*output = bytes;
	return true;
}
size_t player_item_snapshot_copy_frame_bytes() noexcept
{
	return snapshot_clone_copy_frames;
}
player_snapshot_codec_result
player_item_snapshot_clone_bounded(const player_item_snapshot &source, player_item_snapshot *output,
				   bool (*reserve)(size_t, void *) noexcept, void *context,
				   size_t outer) noexcept
{
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	if (!snapshot_clone_policy_supported())
		return player_snapshot_codec_result::limit_exceeded;
	// Initial admission owns the genuine prospective private row and all
	// scalar/function/observation carriers before even profiling requests.
	constexpr size_t own = sizeof(player_item_snapshot) + 4 * sizeof(void *) +
			       3 * sizeof(size_t) + sizeof(bool) +
			       sizeof(player_snapshot_codec_result) +
			       snapshot_clone_observation_frames;
	size_t peak = outer;
	if (!reserve || !item_list_add(peak, own) || !reserve(peak, context))
		return player_snapshot_codec_result::allocation_failure;
	size_t request = 0;
	// The full original generated row copy is closed: fresh strings use
	// _M_create(size,0), fresh vectors allocate exactly size elements, and
	// nested descriptions/spells own the same corresponding constructors.
	// Its entire maximum heap is admitted before the first constructor;
	// partial-copy failure retains a subset until admitted unwind completes.
	if (!player_item_snapshot_fresh_copy_request_bytes(source, &request) ||
	    !item_list_add(peak, request) || !item_list_add(peak, snapshot_clone_copy_frames) ||
	    !reserve(peak, context))
		return player_snapshot_codec_result::allocation_failure;
	try
	{
		player_item_snapshot candidate(source);
		static_assert(std::is_nothrow_move_assignable_v<player_item_snapshot>);
		// Caller retains the entire prior destination in outer. Heap swapping
		// of its strings can leave that old storage in candidate; it remains
		// counted in outer until the preadmitted destructor tail finishes.
		// No callback/allocation/fallible operation follows this commit.
		*output = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

namespace
{
bool snapshot_current_items(const std::vector<player_item_snapshot> &items, size_t &bytes) noexcept
{
	if (!snapshot_clone_vector_request(items, false, bytes))
		return false;
	for (const auto &row : items)
		if (!snapshot_clone_row_request(row, false, bytes))
			return false;
	return true;
}
bool snapshot_current_evidence(const player_death_evidence_table &table, size_t &bytes) noexcept
{
	if (!snapshot_clone_vector_request(table.columns, false, bytes) ||
	    !snapshot_clone_vector_request(table.rows, false, bytes))
		return false;
	for (const auto &column : table.columns)
		if (!snapshot_clone_string_request(column, false, bytes))
			return false;
	for (const auto &row : table.rows)
	{
		if (!snapshot_clone_vector_request(row, false, bytes))
			return false;
		for (const auto &cell : row)
			if (cell && !snapshot_clone_string_request(*cell, false, bytes))
				return false;
	}
	return true;
}
} // namespace

bool player_snapshot_current_heap_bytes(const player_snapshot &snapshot, size_t *output) noexcept
{
	if (!output || !snapshot_clone_policy_supported() || sizeof(void *) != 8 ||
	    sizeof(size_t) != 8 || sizeof(std::string) != 32 || sizeof(std::vector<uint8_t>) != 24)
		return false;
	size_t bytes = 0;
	if (!snapshot_clone_vector_request(snapshot.status_integers, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.status_strings, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.languages, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.introductions, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.timers, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.undead_slots, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.forged_items, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.granted_commands, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.skills, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.affects, false, bytes) ||
	    !snapshot_current_items(snapshot.items, bytes) ||
	    !snapshot_clone_vector_request(snapshot.pets, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.shapes, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.trophies, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.quest_xp_receipts, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.spell_effect_receipts, false, bytes) ||
	    !snapshot_clone_vector_request(snapshot.craft_receipts, false, bytes) ||
	    !snapshot_clone_string_request(snapshot.output_preferences, false, bytes))
		return false;
	for (const auto &status : snapshot.status_strings)
		if (!snapshot_clone_string_request(status.value, false, bytes))
			return false;
	for (const auto &affect : snapshot.affects)
		if (!snapshot_clone_string_request(affect.wear_off_character, false, bytes) ||
		    !snapshot_clone_string_request(affect.wear_off_room, false, bytes))
			return false;
	for (const auto &pet : snapshot.pets)
		if (!snapshot_current_items(pet.items, bytes) ||
		    !snapshot_clone_string_request(pet.restore_state, false, bytes))
			return false;
	if (snapshot.death)
	{
		const auto &death = *snapshot.death;
		if (!snapshot_current_items(death.corpse, bytes) ||
		    !snapshot_clone_vector_request(death.custody, false, bytes) ||
		    !snapshot_clone_vector_request(death.unresolved_operations, false, bytes))
			return false;
		if (death.conflict_evidence)
		{
			const auto &evidence = *death.conflict_evidence;
			if (!snapshot_current_evidence(evidence.player_items, bytes) ||
			    !snapshot_current_evidence(evidence.player_item_affects, bytes) ||
			    !snapshot_current_evidence(evidence.player_item_extra_descr, bytes) ||
			    !snapshot_current_evidence(evidence.item_current_owner, bytes) ||
			    !snapshot_current_evidence(evidence.item_owner_revision, bytes))
				return false;
		}
	}
	*output = bytes;
	return true;
}

namespace
{
// These source-owned carriers complement the existing exact GCC13 vector and
// string request profiles. They do not claim emitted stack or allocator metadata.
struct snapshot_bounded_workspace;
struct snapshot_bounded_frame
{
	snapshot_bounded_workspace &owner;
	snapshot_bounded_frame *previous;
	size_t bytes;
	snapshot_bounded_frame(snapshot_bounded_workspace &, size_t) noexcept;
	~snapshot_bounded_frame();
};
struct snapshot_bounded_workspace
{
	decoder raw;
	player_snapshot snapshot = {};
	std::vector<size_t> depths;
	std::unordered_set<uint64_t> captured, observed;
	std::unordered_set<std::string> operations;
	std::__detail::_Prime_rehash_policy captured_policy, observed_policy, operations_policy;
	std::string operation;
	item_list_reserve_fn reserve;
	void *context;
	size_t outer;
	snapshot_bounded_frame *frames = nullptr;
	size_t transferred_heap = 0;
	template <typename K>
	bool set_heap(const std::unordered_set<K> &set, size_t &bytes) const noexcept
	{
		constexpr bool cached = std::is_same_v<K, std::string>;
		using node = std::__detail::_Hash_node<K, cached>;
		if (set.size() > SIZE_MAX / sizeof(node) ||
		    !item_list_add(bytes, set.size() * sizeof(node)))
			return false;
		if (set.bucket_count() > 1 &&
		    (set.bucket_count() > SIZE_MAX / sizeof(void *) ||
		     !item_list_add(bytes, set.bucket_count() * sizeof(void *))))
			return false;
		if constexpr (cached)
			for (const auto &key : set)
				if (!item_list_string_heap(key, bytes))
					return false;
		return true;
	}
	bool peak(size_t extra) noexcept
	{
		size_t bytes = outer, heap = 0;
		if (!player_snapshot_current_heap_bytes(snapshot, &heap) ||
		    !item_list_add(bytes, sizeof(*this)) || !item_list_add(bytes, heap) ||
		    !item_list_vector_heap(depths, bytes) || !set_heap(captured, bytes) ||
		    !set_heap(observed, bytes) || !set_heap(operations, bytes) ||
		    !item_list_string_heap(operation, bytes))
		{
			errno = ENOBUFS;
			return false;
		}
		for (auto *frame = frames; frame; frame = frame->previous)
			if (!item_list_add(bytes, frame->bytes))
			{
				errno = ENOBUFS;
				return false;
			}
		// Actual observer loops, arithmetic results, query returns and arguments.
		constexpr size_t observer = snapshot_clone_observation_frames + 9 * sizeof(void *) +
					    6 * sizeof(size_t) + 5 * sizeof(bool) +
					    3 * (sizeof(void *) + sizeof(size_t));
		const bool accepted = item_list_add(bytes, observer) &&
				      item_list_add(bytes, extra) && reserve &&
				      reserve(bytes, context);
		if (!accepted)
			errno = ENOBUFS;
		return accepted;
	}
	bool refuse() noexcept
	{
		errno = ENOBUFS;
		raw.result = player_snapshot_codec_result::allocation_failure;
		return false;
	}
};
snapshot_bounded_frame::snapshot_bounded_frame(snapshot_bounded_workspace &work,
					       size_t count) noexcept
	: owner(work)
	, previous(work.frames)
	, bytes(count)
{
	owner.frames = this;
}
snapshot_bounded_frame::~snapshot_bounded_frame()
{
	owner.frames = previous;
}
struct snapshot_bounded_decoder
{
	snapshot_bounded_workspace &owner;
	const uint8_t *&data;
	size_t &size, &offset, &rows;
	player_snapshot_codec_result &result;
	explicit snapshot_bounded_decoder(snapshot_bounded_workspace &work) noexcept
		: owner(work)
		, data(work.raw.data)
		, size(work.raw.size)
		, offset(work.raw.offset)
		, rows(work.raw.rows)
		, result(work.raw.result)
	{
	}
	template <typename T> bool number(T &value)
	{
		// Original number's this/value, unsigned bits, index, offset/query result.
		constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
				       sizeof(std::make_unsigned_t<T>) + 3 * sizeof(size_t) +
				       sizeof(bool);
		if (!owner.peak(own))
			return owner.refuse();
		snapshot_bounded_frame frame(owner, own);
		return owner.raw.number(value);
	}
	bool boolean(bool &value)
	{
		constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
				       sizeof(uint8_t) + sizeof(bool);
		if (!owner.peak(own))
			return owner.refuse();
		snapshot_bounded_frame frame(owner, own);
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
	template <typename T> bool emplace(std::optional<T> &value)
	{
		// optional::emplace/_M_reset/_M_construct/_Construct/construct_at,
		// actual this/value/location/return carriers and placement-new. All
		// contained default string/vector construction is allocation-free.
		constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
				       7 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
				       item_list_nontrivial_frames;
		if (!owner.peak(own))
			return owner.refuse();
		snapshot_bounded_frame frame(owner, own);
		value.emplace();
		return true;
	}
	template <typename T> bool resize(std::vector<T> &values, size_t count)
	{
		constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
				       4 * sizeof(size_t) + sizeof(bool);
		if (!owner.peak(own))
			return owner.refuse();
		snapshot_bounded_frame frame(owner, own);
		size_t request = item_list_vector_frames + item_list_nontrivial_frames;
		if (count > values.capacity())
		{
			const size_t added = count - values.size();
			size_t next = values.size();
			if (!item_list_add(next, std::max(values.size(), added)) ||
			    next > values.max_size())
				next = values.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !item_list_add(request, next * sizeof(T)))
			{
				return owner.refuse();
			}
		}
		if (!owner.peak(request))
			return owner.refuse();
		values.resize(count);
		return true;
	}
	bool string(std::string &value, size_t maximum = PLAYER_SNAPSHOT_MAX_STRING_BYTES)
	{
		constexpr size_t own = sizeof(snapshot_bounded_frame) + 3 * sizeof(void *) +
				       4 * sizeof(size_t) + sizeof(uint32_t) + sizeof(bool);
		if (!owner.peak(own))
		{
			return owner.refuse();
		}
		snapshot_bounded_frame frame(owner, own);
		uint32_t length = 0;
		if (!number(length))
			return false;
		if (length > maximum)
		{
			owner.raw.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		if (owner.raw.size - owner.raw.offset < length)
		{
			owner.raw.result = player_snapshot_codec_result::truncated;
			return false;
		}
		size_t request = 0;
		if (length > value.capacity())
		{
			// GCC13 _M_create(capacity,oldcapacity), actual doubling/clipping.
			size_t next = length;
			if (next < 2 * value.capacity())
				next = 2 * value.capacity();
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX)
			{
				return owner.refuse();
			}
			request = next + 1;
		}
		if (!item_list_add(request, item_list_string_frames) || !owner.peak(request))
		{
			return owner.refuse();
		}
		value.assign(reinterpret_cast<const char *>(owner.raw.data + owner.raw.offset),
			     length);
		owner.raw.offset += length;
		return true;
	}

	template <typename T, typename Read>
	bool vector(std::vector<T> &values, Read read, bool object_rows = false)
	{
		constexpr size_t own = sizeof(snapshot_bounded_frame) + 4 * sizeof(void *) +
				       2 * sizeof(Read) + sizeof(uint32_t) + sizeof(bool) +
				       2 * sizeof(size_t);
		if (!owner.peak(own))
			return owner.refuse();
		snapshot_bounded_frame frame(owner, own);
		uint32_t count = 0;
		if (!number(count))
			return false;
		if (count > PLAYER_SNAPSHOT_MAX_ROWS || rows > PLAYER_SNAPSHOT_MAX_ROWS - count ||
		    (object_rows && (count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
				     owner.raw.objects > PLAYER_SNAPSHOT_MAX_OBJECTS - count)))
		{
			result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		rows += count;
		if (object_rows)
			owner.raw.objects += count;
		if (!resize(values, count))
			return false;
		for (T &value : values)
			if (!read(value))
				return false;
		return true;
	}
};
bool snapshot_bounded_decode_index_rows(snapshot_bounded_decoder &in,
					std::vector<player_index_value_snapshot> &rows)
{
	constexpr size_t own =
		sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) + sizeof(void *) + sizeof(bool);
	if (!in.owner.peak(own))
		return in.owner.refuse();
	snapshot_bounded_frame frame(in.owner, own);

	return in.vector(rows,
			 [&](auto &row) {
				 return in.number(row.index) && in.number(row.value) &&
					in.number(row.auxiliary);
			 });
}
bool snapshot_bounded_decode_items(snapshot_bounded_decoder &in,
				   std::vector<player_item_snapshot> &items)
{
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 3 * sizeof(void *) +
			       2 * sizeof(void *) + sizeof(int32_t *) + sizeof(int64_t *) +
			       sizeof(uint64_t *) + sizeof(std::array<int16_t, 2> *) +
			       sizeof(int16_t *) + sizeof(bool);
	if (!in.owner.peak(own))
	{
		in.owner.raw.result = player_snapshot_codec_result::allocation_failure;
		return false;
	}
	snapshot_bounded_frame frame(in.owner, own);
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
bool snapshot_bounded_decode_death(snapshot_bounded_decoder &in, player_death_snapshot &death)
{
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
			       2 * sizeof(void *) + 2 * sizeof(uint8_t) + 2 * sizeof(void *) +
			       sizeof(bool);
	if (!in.owner.peak(own))
		return in.owner.refuse();
	snapshot_bounded_frame frame(in.owner, own);

	for (uint8_t &byte : death.operation_id.bytes)
		if (!in.number(byte))
			return false;
	if (!in.number(death.corpse_room_vnum) || !in.number(death.wallet_revision))
		return false;
	for (int32_t &amount : death.wallet_before)
		if (!in.number(amount))
			return false;
	return in.number(death.wallet_pile_uid) &&
	       snapshot_bounded_decode_items(in, death.corpse) &&
	       in.vector(death.custody,
			 [&](auto &row)
			 {
				 uint8_t state = 0, owner = 0;
				 if (!in.number(row.item.item_uid) ||
				     !in.number(row.item.root_item_uid) ||
				     !in.number(row.item.parent_item_uid) ||
				     !in.number(row.item.expected_item_revision) ||
				     !in.number(row.item.vnum) || !in.number(state) ||
				     !in.number(owner) || !in.number(row.owner.id) ||
				     !in.number(row.owner.context_id) ||
				     !in.number(row.owner_revision))
					 return false;
				 row.item.expected_state = static_cast<item_custody_state>(state);
				 row.owner.type = static_cast<item_owner_type>(owner);
				 return true;
			 }) &&
	       in.vector(death.unresolved_operations,
			 [&](auto &id)
			 {
				 for (uint8_t &byte : id.bytes)
					 if (!in.number(byte))
						 return false;
				 return true;
			 });
}
bool snapshot_bounded_decode_evidence(snapshot_bounded_decoder &in,
				      player_death_conflict_evidence &evidence)
{
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
			       5 * sizeof(void *) + 4 * sizeof(void *) + 2 * sizeof(uint32_t) +
			       sizeof(bool);
	if (!in.owner.peak(own))
		return in.owner.refuse();
	snapshot_bounded_frame frame(in.owner, own);

	for (auto *table : { &evidence.player_items, &evidence.player_item_affects,
			     &evidence.player_item_extra_descr, &evidence.item_current_owner,
			     &evidence.item_owner_revision })
	{
		uint32_t columns = 0, rows = 0;
		if (!in.number(columns))
			return false;
		if (!columns || columns > PLAYER_DEATH_EVIDENCE_MAX_COLUMNS)
		{
			in.result = columns ? player_snapshot_codec_result::limit_exceeded :
					      player_snapshot_codec_result::invalid_value;
			return false;
		}
		if (!in.resize(table->columns, columns))
			return false;
		for (auto &column : table->columns)
			if (!in.string(column, PLAYER_DEATH_EVIDENCE_MAX_COLUMN_NAME_BYTES))
				return false;
		if (!in.number(rows))
			return false;
		if (rows > PLAYER_SNAPSHOT_MAX_ROWS || in.rows > PLAYER_SNAPSHOT_MAX_ROWS - rows)
		{
			in.result = player_snapshot_codec_result::limit_exceeded;
			return false;
		}
		// Each cell needs at least a NULL/present byte. Reject impossible lengths
		// before allocating the rectangular row vectors.
		if (static_cast<size_t>(rows) * columns > in.size - in.offset)
		{
			in.result = player_snapshot_codec_result::truncated;
			return false;
		}
		in.rows += rows;
		if (!in.resize(table->rows, rows))
			return false;
		for (auto &row : table->rows)
		{
			if (!in.resize(row, columns))
				return false;
			for (auto &cell : row)
			{
				bool present = false;
				if (!in.boolean(present))
					return false;
				if (present)
				{
					if (!in.emplace(cell))
						return false;
					if (!in.string(*cell, PLAYER_SNAPSHOT_MAX_BYTES))
						return false;
				}
			}
		}
	}
	// Complete original allocation-free evidence validation; retain its
	// budget/row scalars, five-table initializer, consume/has-columns
	// closures, required-name arrays and iterator/cell arguments.
	if (!in.owner.peak(2 * sizeof(size_t) + 5 * sizeof(void *) + 2 * sizeof(void *) +
			   9 * sizeof(void *) + 9 * sizeof(void *) + 4 * sizeof(size_t) +
			   3 * sizeof(bool)))
		return in.owner.refuse();
	in.result = validate_evidence(evidence);
	return in.result == player_snapshot_codec_result::ok;
}

bool snapshot_bounded_relationships(snapshot_bounded_workspace &work,
				    const std::vector<player_item_snapshot> &items)
{
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 3 * sizeof(void *) +
			       sizeof(size_t) + sizeof(int32_t) + sizeof(bool);
	if (!work.peak(own))
		return work.refuse();
	snapshot_bounded_frame frame(work, own);
	size_t request = item_list_vector_frames + item_list_move_frames +
			 item_list_size_constructor_frames + sizeof(std::vector<size_t>) +
			 sizeof(size_t) + 4 * (3 * sizeof(void *) + sizeof(size_t)) +
			 sizeof(size_t) + sizeof(bool) + sizeof(char);
	if (items.size() > SIZE_MAX / sizeof(size_t) ||
	    !item_list_add(request, items.size() * sizeof(size_t)) || !work.peak(request))
		return work.refuse();
	work.depths = std::vector<size_t>(items.size(), 1);
	auto &depths = work.depths;
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
	// Original scratch dies on return. No retained depth allocation is output.
	std::vector<size_t>().swap(depths);
	return true;
}
template <typename K>
bool snapshot_bounded_insert(snapshot_bounded_workspace &work, std::unordered_set<K> &set,
			     std::__detail::_Prime_rehash_policy &policy, K &&key)
{
	using node = std::__detail::_Hash_node<K, std::is_same_v<K, std::string>>;
	// Full concrete source path from captured libstdc++13 hashtable.h/policy.h.
	// Each line names declared parameter/local/return carriers; these source
	// scopes conservatively coexist, without claiming emitted stack bytes.
	constexpr size_t library =
		// unordered_set::find/insert, _Insert::insert/_M_insert/_M_insert_unique_aux:
		5 * (2 * sizeof(void *) + sizeof(void *)) +
		// _M_insert_unique: this/k/v/node_gen, loop iterator, hash/bucket,
		// _Scoped_node's genuine two pointers, pos and pair<iterator,bool>:
		4 * sizeof(void *) + sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
		sizeof(void *) + sizeof(std::pair<void *, bool>) +
		// _M_insert_unique_node: this/node, bkt/code/n_elt, saved-state ref,
		// rehash pair, returned iterator. _M_rehash this/count/state/tag:
		3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::pair<bool, size_t>) +
		sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(char) +
		// _M_rehash_aux unique: this/count/tag,new_buckets,p,next,bbegin,bkt:
		4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(char) +
		// bucket allocate/deallocate: this/count, allocator, ptr,p,return,
		// old buckets ptr/count and deallocate allocator/ptr; actual memset:
		9 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(std::allocator<void *>) +
		3 * sizeof(void *) + sizeof(size_t) +
		// node allocate: this/forward key,nptr,n,return; node generator,
		// builder, forward/addressof/construct and placement-new carriers:
		5 * sizeof(void *) + 10 * sizeof(void *) + sizeof(size_t) +
		// find/find_before_node: this/key,code,bucket,iterator/prev/p,
		// node next/key extraction/hash/equal/bucket modulus/returned values:
		6 * sizeof(void *) + 3 * sizeof(size_t) +
		10 * (2 * sizeof(void *) + sizeof(void *)) + 2 * sizeof(bool) +
		// erase/erase(iterator)/_M_erase: this,key,code,bkt,prev,node,next,
		// node destructor/deallocate incl pointer_traits and erase result:
		10 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(void *) +
		item_list_allocator_frames + snapshot_clone_string_move_frames;
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 4 * sizeof(void *) +
			       sizeof(std::__detail::_Prime_rehash_policy) +
			       sizeof(std::pair<bool, size_t>) + 6 * sizeof(size_t) +
			       6 * sizeof(bool) + library;
	if (!work.peak(own))
		return work.refuse();
	snapshot_bounded_frame frame(work, own);
	if (set.find(key) != set.end())
		return false;
	auto proposed = policy;
	const auto rehash = proposed._M_need_rehash(set.bucket_count(), set.size(), 1);
	size_t request = sizeof(node);
	if (rehash.first && (rehash.second > SIZE_MAX / sizeof(void *) ||
			     !item_list_add(request, rehash.second * sizeof(void *))))
		return work.refuse();
	if (!work.peak(request))
		return work.refuse();
	const bool inserted = set.insert(std::move(key)).second;
	if (inserted)
		policy = proposed;
	return inserted;
}
bool snapshot_bounded_operation(snapshot_bounded_workspace &work, const critical_operation_id &id)
{
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 2 * sizeof(void *) +
			       sizeof(size_t) + sizeof(bool) +
			       snapshot_clone_string_constructor_frames + sizeof(std::string) +
			       snapshot_clone_string_move_frames;
	size_t request = own;
	if (id.bytes.size() > 15 && !item_list_add(request, id.bytes.size() + 1))
		return work.refuse();
	if (!work.peak(request))
		return work.refuse();
	snapshot_bounded_frame frame(work, own);
	work.operation =
		std::string(reinterpret_cast<const char *>(id.bytes.data()), id.bytes.size());
	return snapshot_bounded_insert(work, work.operations, work.operations_policy,
				       std::move(work.operation));
}

bool snapshot_bounded_valid_death(snapshot_bounded_workspace &work, const player_snapshot &snapshot)
{
	constexpr size_t own = sizeof(snapshot_bounded_frame) + 3 * sizeof(void *) +
			       7 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(int32_t) +
			       2 * sizeof(bool);
	if (!work.peak(own))
		return work.refuse();
	snapshot_bounded_frame frame(work, own);

	if (snapshot.schema_version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
	    snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ||
	    snapshot.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
	    snapshot.schema_version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION)
		return !snapshot.death;
	if (!snapshot.death || snapshot.save_intent != RENT_DEATH ||
	    snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL || !snapshot.items.empty() ||
	    std::any_of(snapshot.pets.begin(), snapshot.pets.end(),
			[](const auto &pet) { return pet.hold_reason == pet_hold_reason::none; }))
		return false;
	const auto &death = *snapshot.death;
	if (player_snapshot_is_death_evidence_schema(snapshot.schema_version) !=
	    death.conflict_evidence.has_value())
		return false;
	if (!nonzero_operation(death.operation_id) || death.corpse_room_vnum <= 0 ||
	    !death.wallet_revision || death.corpse.empty() ||
	    !snapshot_bounded_relationships(work, death.corpse))
		return false;
	const auto &corpse = death.corpse.front();
	if (corpse.vnum != VOBJ_CORPSE || corpse.type != ITEM_CORPSE ||
	    corpse.values[CORPSE_PID] != snapshot.pid || corpse.values[CORPSE_SAVEID] <= 0 ||
	    !(corpse.values[CORPSE_FLAGS] & PC_CORPSE))
		return false;
	bool has_wallet = false;
	for (int32_t amount : death.wallet_before)
	{
		if (amount < 0)
			return false;
		has_wallet = has_wallet || amount != 0;
	}
	if (has_wallet != (death.wallet_pile_uid != 0))
		return false;
	auto &captured = work.captured;
	for (size_t index = 0; index < death.corpse.size(); ++index)
	{
		const auto &item = death.corpse[index];
		if (!item.object_uid ||
		    !snapshot_bounded_insert(work, captured, work.captured_policy,
					     uint64_t(item.object_uid)) ||
		    (index && item.parent_index == PLAYER_SNAPSHOT_NO_PARENT))
			return false;
		if (item.object_uid == death.wallet_pile_uid)
		{
			if (item.vnum != VOBJ_COINS || item.type != ITEM_MONEY ||
			    item.parent_index != 0)
				return false;
			for (size_t denomination = 0; denomination < death.wallet_before.size();
			     ++denomination)
				if (item.values[denomination] != death.wallet_before[denomination])
					return false;
		}
	}
	if (has_wallet && (death.wallet_pile_uid == death.corpse.front().object_uid ||
			   !captured.count(death.wallet_pile_uid)))
		return false;
	captured.erase(death.corpse.front().object_uid); // Lifecycle owns the corpse itself.
	auto &observed = work.observed;
	for (const auto &row : death.custody)
	{
		if (!row.item.item_uid ||
		    !snapshot_bounded_insert(work, observed, work.observed_policy,
					     uint64_t(row.item.item_uid)) ||
		    row.item.vnum <= 0 ||
		    row.item.expected_state > item_custody_state::quarantined ||
		    row.owner.type > item_owner_type::native_mobile ||
		    (row.owner.type == item_owner_type::native_mobile &&
		     (!row.owner.id || row.owner.id == UINT64_MAX || row.owner.context_id)))
			return false;
		if (row.item.expected_state == item_custody_state::absent)
		{
			if (row.item.expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION ||
			    row.owner.type != item_owner_type::unknown || row.owner.id ||
			    row.owner.context_id || row.owner_revision)
				return false;
		}
		else if (!row.item.expected_item_revision ||
			 row.item.expected_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
			 !row.item.root_item_uid || row.owner.type == item_owner_type::unknown)
			return false;
		captured.erase(row.item.item_uid);
	}
	if (!captured.empty())
		return false;

	for (const auto &id : death.unresolved_operations)
		if (!nonzero_operation(id) || id.bytes == death.operation_id.bytes ||
		    !snapshot_bounded_operation(work, id))
			return false;
	return true;
}
} // namespace

player_snapshot_codec_result
player_snapshot_decode_bounded(const uint8_t *encoded, size_t encoded_size,
			       player_snapshot *snapshot_out,
			       bool (*reserve)(size_t, void *) noexcept, void *context,
			       size_t outer_live, size_t *retained_snapshot_heap) noexcept
{
	if (!encoded || !encoded_size || !snapshot_out)
		return player_snapshot_codec_result::invalid_value;
	if (encoded_size > PLAYER_SNAPSHOT_MAX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;

	if (!snapshot_clone_policy_supported() || sizeof(void *) != 8 || sizeof(size_t) != 8 ||
	    sizeof(std::string) != 32 || sizeof(std::vector<uint8_t>) != 24)
	{
		errno = ENOTSUP;
		return player_snapshot_codec_result::limit_exceeded;
	}
	// All concrete source owners and main's metadata/range/return carriers are
	// admitted before construction; per-call closures are owned by vector().
	constexpr size_t own =
		sizeof(snapshot_bounded_frame) + sizeof(snapshot_bounded_decoder) +
		6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(uint64_t) + 2 * sizeof(uint32_t) +
		sizeof(bool) + 5 * sizeof(void *) + sizeof(player_snapshot_codec_result) +
		// Actual callback field/reason/base/bitvector and fixed receipt-copy
		// iterator/count source scopes (no heap allocation or guessed slack).
		sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(bool) +
		sizeof(uint64_t *) + 4 * sizeof(void *) + 2 * sizeof(size_t);
	constexpr size_t construction = item_list_nontrivial_frames + 17 * item_list_move_frames +
					3 * snapshot_clone_string_move_frames;
	size_t initial = outer_live;
	if (!reserve || !item_list_add(initial, sizeof(snapshot_bounded_workspace)) ||
	    !item_list_add(initial, own) || !item_list_add(initial, construction) ||
	    !reserve(initial, context))
	{
		errno = ENOBUFS;
		return player_snapshot_codec_result::allocation_failure;
	}
	try
	{
		snapshot_bounded_workspace work{ { encoded, encoded_size },
						 {},
						 {},
						 {},
						 {},
						 {},
						 {},
						 {},
						 {},
						 {},
						 reserve,
						 context,
						 outer_live };
		snapshot_bounded_frame frame(work, own);
		snapshot_bounded_decoder in(work);
		auto &snapshot = work.snapshot;
		uint64_t encoded_bound = 0;
		if (!in.number(snapshot.schema_version))
			return in.result;
		const uint32_t raw_wire_version = snapshot.schema_version;
		const bool ward_fields = raw_wire_version >= 20 && raw_wire_version <= 32;
		const uint32_t wire_version =
			ward_fields ? raw_wire_version - PLAYER_SNAPSHOT_WARD_WIRE_OFFSET :
				      raw_wire_version;
		snapshot.schema_version = wire_version;
		if (wire_version == 1 || wire_version == 3 || wire_version == 5 ||
		    wire_version == 7)
			snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		if (wire_version == 2 || wire_version == 4 || wire_version == 6 ||
		    wire_version == 8)
			snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
		if (snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
		    snapshot.schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION &&
		    snapshot.schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
		    snapshot.schema_version !=
			    PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
		    !player_snapshot_is_death_request_schema(snapshot.schema_version) &&
		    !player_snapshot_is_death_evidence_schema(snapshot.schema_version))
			return player_snapshot_codec_result::unsupported_version;
		if (!in.number(snapshot.pid) || !in.number(snapshot.revision) ||
		    !in.number(snapshot.components) || !in.number(snapshot.save_intent) ||
		    !in.number(snapshot.room_vnum) || !in.number(encoded_bound))
			return in.result;
		snapshot.encoded_size_bound = encoded_bound;
		if (!work.peak(sizeof(void *) + sizeof(bool) + sizeof(uint32_t)))
			return player_snapshot_codec_result::allocation_failure;
		if (!valid_metadata(snapshot))
			return player_snapshot_codec_result::invalid_value;
		if (!in.vector(snapshot.status_integers,
			       [&](auto &row)
			       {
				       uint16_t field = 0;
				       if (!in.number(field))
					       return false;
				       if (field >
					   static_cast<uint16_t>(player_status_field::last_ip))
				       {
					       in.result =
						       player_snapshot_codec_result::invalid_value;
					       return false;
				       }
				       if (!in.number(row.signed_value) ||
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
				       if (!in.number(field))
					       return false;
				       if (field > static_cast<uint8_t>(
							   player_status_string_field::poof_out))
				       {
					       in.result =
						       player_snapshot_codec_result::invalid_value;
					       return false;
				       }
				       if (!in.string(row.value))
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
		if (!snapshot_bounded_decode_index_rows(in, snapshot.languages) ||
		    !snapshot_bounded_decode_index_rows(in, snapshot.introductions) ||
		    !snapshot_bounded_decode_index_rows(in, snapshot.timers) ||
		    !snapshot_bounded_decode_index_rows(in, snapshot.undead_slots) ||
		    !snapshot_bounded_decode_index_rows(in, snapshot.forged_items) ||
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
				       if (ward_fields && (!in.number(row.ward_source_uid) ||
							   !in.number(row.ward_full_duration) ||
							   !in.number(row.ward_capacity) ||
							   !in.number(row.ward_capacity_max) ||
							   !in.number(row.ward_refresh_remaining) ||
							   !in.number(row.ward_source_type) ||
							   !in.number(row.ward_source_worn) ||
							   !in.number(row.ward_active)))
					       return false;
				       return in.string(row.wear_off_character) &&
					      in.string(row.wear_off_room);
			       }) ||
		    !snapshot_bounded_decode_items(in, snapshot.items) ||
		    !in.vector(snapshot.pets,
			       [&](auto &pet)
			       {
				       if (wire_version >= 7 && !in.number(pet.pet_uid))
					       return false;
				       const bool base =
					       in.number(pet.mob_vnum) && in.number(pet.order) &&
					       in.number(pet.hit) && in.number(pet.max_hit) &&
					       in.number(pet.mana) && in.number(pet.max_mana) &&
					       in.number(pet.vitality) &&
					       in.number(pet.max_vitality) &&
					       in.number(pet.charm_duration) &&
					       in.number(pet.room_vnum) &&
					       snapshot_bounded_decode_items(in, pet.items);
				       if (!base || wire_version < 3)
					       return base;
				       uint32_t reason = 0;
				       if (!in.string(pet.restore_state,
						      PET_RESTORE_STATE_MAX_BYTES) ||
					   !in.number(reason))
					       return false;
				       pet.hold_reason = static_cast<pet_hold_reason>(reason);
				       return true;
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
		if (wire_version >= 5 &&
		    !in.string(snapshot.output_preferences, OUTPUT_PREFERENCE_MAX_BYTES))
			return in.result;
		if (player_snapshot_has_quest_receipt_schema(wire_version) &&
		    !in.vector(snapshot.quest_xp_receipts,
			       [&](auto &receipt)
			       {
				       if (in.size - in.offset <
					   receipt.offering_operation.bytes.size())
				       {
					       in.result = player_snapshot_codec_result::truncated;
					       return false;
				       }
				       std::copy_n(in.data + in.offset,
						   receipt.offering_operation.bytes.size(),
						   receipt.offering_operation.bytes.begin());
				       in.offset += receipt.offering_operation.bytes.size();
				       return in.number(receipt.reward_index) &&
					      in.number(receipt.amount);
			       }))
			return in.result;
		if (player_snapshot_has_spell_receipt_schema(wire_version) &&
		    !in.vector(snapshot.spell_effect_receipts,
			       [&](auto &receipt)
			       {
				       if (in.size - in.offset < receipt.operation_id.bytes.size())
				       {
					       in.result = player_snapshot_codec_result::truncated;
					       return false;
				       }
				       std::copy_n(in.data + in.offset,
						   receipt.operation_id.bytes.size(),
						   receipt.operation_id.bytes.begin());
				       in.offset += receipt.operation_id.bytes.size();
				       return in.number(receipt.effect_id);
			       }))
			return in.result;
		if (player_snapshot_has_craft_receipt_schema(wire_version) &&
		    !in.vector(snapshot.craft_receipts,
			       [&](auto &receipt)
			       {
				       if (in.size - in.offset < receipt.operation_id.bytes.size())
				       {
					       in.result = player_snapshot_codec_result::truncated;
					       return false;
				       }
				       std::copy_n(in.data + in.offset,
						   receipt.operation_id.bytes.size(),
						   receipt.operation_id.bytes.begin());
				       in.offset += receipt.operation_id.bytes.size();
				       return in.number(receipt.discipline) &&
					      in.number(receipt.experience);
			       }))
			return in.result;
		if (player_snapshot_is_death_request_schema(snapshot.schema_version) ||
		    player_snapshot_is_death_evidence_schema(snapshot.schema_version))
		{
			if (!in.emplace(snapshot.death))
				return in.result;
			if (!snapshot_bounded_decode_death(in, *snapshot.death))
				return in.result;
			if (player_snapshot_is_death_evidence_schema(snapshot.schema_version))
			{
				if (!in.emplace(snapshot.death->conflict_evidence))
					return in.result;
				if (!snapshot_bounded_decode_evidence(
					    in, *snapshot.death->conflict_evidence))
					return in.result;
			}
		}
		// Original allocation-free metadata/receipt validators retain their
		// actual snapshot/receipt references, nested prior/index counters,
		// nonzero-operation iterators and algorithm predicate return scopes.
		if (!work.peak(10 * sizeof(void *) + 7 * sizeof(size_t) +
			       sizeof(player_component_mask_t) + 5 * sizeof(bool)))
			return player_snapshot_codec_result::allocation_failure;
		if (!snapshot_bounded_valid_death(work, snapshot) ||
		    !valid_quest_xp_receipts(snapshot) || !valid_spell_effect_receipts(snapshot) ||
		    !valid_craft_receipts(snapshot))
			return in.result == player_snapshot_codec_result::allocation_failure ?
				       in.result :
				       player_snapshot_codec_result::invalid_value;
		if (in.offset != in.size)
			return player_snapshot_codec_result::invalid_value;
		if (!snapshot_bounded_relationships(work, snapshot.items))
			return in.result == player_snapshot_codec_result::allocation_failure ?
				       in.result :
				       player_snapshot_codec_result::invalid_value;
		for (const player_pet_snapshot &pet : snapshot.pets)
			if (!snapshot_bounded_relationships(work, pet.items))
				return in.result == player_snapshot_codec_result::allocation_failure ?
					       in.result :
					       player_snapshot_codec_result::invalid_value;

		if (!player_snapshot_current_heap_bytes(snapshot, &work.transferred_heap) ||
		    !work.peak(construction))
		{
			errno = ENOBUFS;
			return player_snapshot_codec_result::allocation_failure;
		}
		static_assert(std::is_nothrow_move_assignable_v<player_snapshot>);
		// Strong-output commit: no callback, allocation or fallible operation
		// follows. Prior destination remains included in caller outer through
		// the admitted move/destruction tail.
		*snapshot_out = std::move(snapshot);
		if (retained_snapshot_heap)
			*retained_snapshot_heap = work.transferred_heap;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return player_snapshot_codec_result::allocation_failure;
	}
	return player_snapshot_codec_result::ok;
}

namespace
{
// These source scopes augment the settled typed vector/string/row closures.
// No machine-stack, heap metadata or replacement gameplay rule is assumed.
constexpr size_t subtree_bit_frames =
	// vector(n,value,a): this+n+value/a references, default bool allocator;
	// _Bvector_base(a), conversion bool->word allocator, impl(a), data ctor.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<bool>) + 2 * sizeof(void *) +
	sizeof(std::allocator<unsigned long>) + 2 * sizeof(void *) + sizeof(void *) +
	// _M_initialize: this/n, real q/start. _M_allocate: this/n/p/return;
	// _S_nword: n/result; runtime is_constant_evaluated result (loop inactive).
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::vector<bool>::iterator) +
	3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(size_t) + sizeof(bool) +
	// __addressof(reference): arg/result. _M_initialize_value: this/x/p;
	// _M_end_addr: this/result+__addressof arg/result. Actual fill_n calls
	// __builtin_memset directly at runtime (no iterator fill loop).
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) +
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 3 * sizeof(void *) + sizeof(int) +
	sizeof(size_t) +
	// vector[]: this/n/ref return, begin: this/iterator return; iterator[]
	// this/i/ref return, + reference/n/real tmp/returned iterator, +=,
	// _M_incr this/i/real n, unary* this/returned reference;
	// ref ctor this/x/mask; bool conversion and both actual assignment overloads.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::vector<bool>::reference) +
	sizeof(void *) + sizeof(std::vector<bool>::iterator) + sizeof(void *) +
	sizeof(std::ptrdiff_t) + sizeof(std::vector<bool>::reference) + sizeof(void *) +
	sizeof(std::ptrdiff_t) + 2 * sizeof(std::vector<bool>::iterator) + 2 * sizeof(void *) +
	sizeof(std::ptrdiff_t) + sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::vector<bool>::reference) + 2 * sizeof(void *) + sizeof(unsigned long) +
	sizeof(void *) + sizeof(bool) + 3 * sizeof(void *) + sizeof(bool) +
	// Actual iterator/base constructors and generated copies at start/finish:
	// this+x+offset, base same; three real constructor uses summed, not max.
	3 * (4 * sizeof(void *) + 2 * sizeof(unsigned int)) +
	// ~vector/~base/_M_deallocate this scopes, real n, allocatorref/p/n;
	// reset constructs real impl-data temporary with two default iterators.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::vector<bool>) + 5 * sizeof(void *) +
	2 * (3 * sizeof(void *) + sizeof(unsigned int)) + item_list_allocator_frames;
constexpr size_t subtree_frames =
	6 * sizeof(std::vector<player_item_snapshot>) + sizeof(std::vector<bool>) +
	2 * sizeof(std::vector<int32_t>) + sizeof(player_item_snapshot) + 14 * sizeof(void *) +
	15 * sizeof(size_t) + 4 * sizeof(int32_t) + 8 * sizeof(bool) + sizeof(uint64_t) +
	3 * sizeof(player_snapshot_codec_result) +
	6 * sizeof(std::vector<player_item_snapshot>::const_iterator) + subtree_bit_frames +
	item_list_size_constructor_frames + snapshot_clone_copy_frames + item_list_vector_frames +
	item_list_move_frames + item_list_nontrivial_frames;
struct subtree_budget
{
	item_list_reserve_fn reserve;
	void *context;
	size_t outer;
	const std::vector<player_item_snapshot> *a = nullptr, *b = nullptr, *c = nullptr,
						*d = nullptr;
	const player_item_snapshot *row = nullptr;
	size_t auxiliary = 0;
	bool prefix(size_t &bytes, size_t extra = 0) const noexcept
	{
		bytes = outer;
		if (!item_list_add(bytes, sizeof(*this)) || !item_list_add(bytes, subtree_frames) ||
		    !item_list_add(bytes, auxiliary) || !item_list_add(bytes, extra))
			return false;
		for (const auto *value : { a, b, c, d })
			if (value && !item_list_heap(*value, bytes))
				return false;
		return !row || snapshot_clone_row_request(*row, false, bytes);
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t bytes = 0;
		return prefix(bytes, extra) && reserve && reserve(bytes, context);
	}
	bool copy_rows(std::span<const player_item_snapshot> src,
		       std::vector<player_item_snapshot> &dst) const
	{
		size_t request = 0;
		if (src.size() > SIZE_MAX / sizeof(player_item_snapshot))
			return false;
		request = src.size() * sizeof(player_item_snapshot);
		for (const auto &r : src)
			if (!snapshot_clone_row_request(r, true, request))
				return false;
		if (!item_list_add(request, snapshot_clone_copy_frames) || !peak(request))
			return false;
		dst.assign(src.begin(), src.end());
		return true;
	}
	bool grow(std::vector<player_item_snapshot> &dst) const noexcept
	{
		if (dst.size() < dst.capacity())
			return peak();
		const size_t n = dst.size();
		if (n > SIZE_MAX / 2)
			return false;
		const size_t capacity = n ? n * 2 : 1;
		return capacity <= SIZE_MAX / sizeof(player_item_snapshot) &&
		       peak(capacity * sizeof(player_item_snapshot));
	}
};
}
size_t player_item_snapshot_extract_frame_bytes() noexcept
{
	return sizeof(subtree_budget) + subtree_frames;
}
bool player_item_snapshot_list_current_heap_bytes(const std::vector<player_item_snapshot> &value,
						  size_t *out) noexcept
{
	if (!out || !snapshot_clone_policy_supported())
		return false;
	size_t bytes = 0;
	if (!item_list_heap(value, bytes))
		return false;
	*out = bytes;
	return true;
}

player_snapshot_codec_result player_item_snapshot_extract_subtree_bounded(
	const std::vector<player_item_snapshot> &items, uint64_t selected_uid,
	std::vector<player_item_snapshot> *selected_out,
	std::vector<player_item_snapshot> *remaining_out, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!selected_uid || !selected_out || !remaining_out)
		return player_snapshot_codec_result::invalid_value;
	if (!snapshot_clone_policy_supported())
		return player_snapshot_codec_result::unsupported_version;
	subtree_budget budget{ reserve, context, outer_live };
	if (!budget.peak())
		return player_snapshot_codec_result::allocation_failure;
	try
	{
		if (items.size() > SIZE_MAX / sizeof(size_t) ||
		    !budget.peak(items.size() * sizeof(size_t)))
			return player_snapshot_codec_result::allocation_failure;
		if (!valid_item_relationships(items))
			return player_snapshot_codec_result::invalid_value;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
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
		budget.a = &selected;
		budget.b = &remaining;
		constexpr size_t word_bits = std::numeric_limits<unsigned long>::digits;
		const size_t words = items.size() / word_bits + (items.size() % word_bits != 0);
		if (words > SIZE_MAX / sizeof(unsigned long) ||
		    items.size() > SIZE_MAX / (2 * sizeof(int32_t)))
			return player_snapshot_codec_result::allocation_failure;
		budget.auxiliary = words * sizeof(unsigned long);
		if (!item_list_add(budget.auxiliary, 2 * items.size() * sizeof(int32_t)))
			return player_snapshot_codec_result::allocation_failure;
		if (!budget.peak())
			return player_snapshot_codec_result::allocation_failure;
		std::vector<bool> included(items.size(), false);
		std::vector<int32_t> selected_positions(items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		std::vector<int32_t> remaining_positions(items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		if (items.size() > SIZE_MAX / sizeof(player_item_snapshot) ||
		    !budget.peak(items.size() * sizeof(player_item_snapshot)))
			return player_snapshot_codec_result::allocation_failure;
		selected.reserve(items.size());
		if (!budget.peak(items.size() * sizeof(player_item_snapshot)))
			return player_snapshot_codec_result::allocation_failure;
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
				player_item_snapshot item;
				size_t nested = 0;
				if (!budget.prefix(nested))
					return player_snapshot_codec_result::allocation_failure;
				auto copy_code = player_item_snapshot_clone_bounded(
					items[index], &item, reserve, context, nested);
				if (copy_code != player_snapshot_codec_result::ok)
					return copy_code;
				budget.row = &item;
				item.parent_index = index == selected_index ?
							    PLAYER_SNAPSHOT_NO_PARENT :
							    selected_positions[static_cast<size_t>(
								    items[index].parent_index)];
				if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
				    index != selected_index)
					return player_snapshot_codec_result::invalid_value;
				if (!budget.peak())
					return player_snapshot_codec_result::allocation_failure;
				selected.push_back(std::move(item));
				budget.row = nullptr;
			}
			else
			{
				remaining_positions[index] = static_cast<int32_t>(remaining.size());
				player_item_snapshot item;
				size_t nested = 0;
				if (!budget.prefix(nested))
					return player_snapshot_codec_result::allocation_failure;
				auto copy_code = player_item_snapshot_clone_bounded(
					items[index], &item, reserve, context, nested);
				if (copy_code != player_snapshot_codec_result::ok)
					return copy_code;
				budget.row = &item;
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				{
					item.parent_index = remaining_positions[static_cast<size_t>(
						items[index].parent_index)];
					if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
						return player_snapshot_codec_result::invalid_value;
				}
				if (!budget.peak())
					return player_snapshot_codec_result::allocation_failure;
				remaining.push_back(std::move(item));
				budget.row = nullptr;
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

player_snapshot_codec_result
player_item_snapshot_extract_forest_bounded(const std::vector<player_item_snapshot> &items,
					    const std::vector<uint64_t> &selected_root_uids,
					    std::vector<player_item_snapshot> *selected_out,
					    std::vector<player_item_snapshot> *remaining_out,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer_live) noexcept
{
	if (selected_root_uids.empty() || !selected_out || !remaining_out ||
	    std::any_of(selected_root_uids.begin(), selected_root_uids.end(),
			[](uint64_t uid) { return uid == 0; }) ||
	    std::adjacent_find(selected_root_uids.begin(), selected_root_uids.end(),
			       [](uint64_t left, uint64_t right)
			       { return left >= right; }) != selected_root_uids.end())
		return player_snapshot_codec_result::invalid_value;
	if (!snapshot_clone_policy_supported())
		return player_snapshot_codec_result::unsupported_version;
	subtree_budget budget{ reserve, context, outer_live };
	if (!budget.peak())
		return player_snapshot_codec_result::allocation_failure;
	try
	{
		std::vector<player_item_snapshot> selected;
		std::vector<player_item_snapshot> remaining;
		budget.a = &selected;
		budget.b = &remaining;
		if (!budget.copy_rows(items, remaining))
			return player_snapshot_codec_result::allocation_failure;
		for (uint64_t selected_root_uid : selected_root_uids)
		{
			std::vector<player_item_snapshot> tree;
			std::vector<player_item_snapshot> next_remaining;
			budget.c = &tree;
			budget.d = &next_remaining;
			size_t nested = 0;
			if (!budget.prefix(nested))
				return player_snapshot_codec_result::allocation_failure;
			const auto extracted = player_item_snapshot_extract_subtree_bounded(
				remaining, selected_root_uid, &tree, &next_remaining, reserve,
				context, nested);
			if (extracted != player_snapshot_codec_result::ok)
				return extracted;
			const int32_t offset = static_cast<int32_t>(selected.size());
			for (auto &item : tree)
			{
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += offset;
				if (!budget.grow(selected))
					return player_snapshot_codec_result::allocation_failure;
				selected.push_back(std::move(item));
			}
			if (!budget.peak())
				return player_snapshot_codec_result::allocation_failure;
			remaining = std::move(next_remaining);
			budget.c = nullptr;
			budget.d = nullptr;
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

size_t player_item_snapshot_current_heap_observer_frame_bytes() noexcept
{
	// Actual shared observer public/helper/range/query scopes are owned by the
	// settled observation inventory, not by allocating copy constructors.
	// capacity()'s declared this/result is already one query in that inventory.
	// Its genuine const path reaches _M_is_local -> _M_data + _M_local_data ->
	// pointer_traits<const char*>::pointer_to -> addressof -> __addressof:
	// (this+bool)+(this+ptr)+(this+ptr)+(reference+ptr)*3 = 11P+B.
	// Extra public CURRENT params/value/policy/strong-result + profile return.
	return snapshot_clone_observation_frames + 11 * sizeof(void *) + sizeof(bool) +
	       2 * sizeof(void *) + 2 * sizeof(size_t) + 3 * sizeof(bool);
}

size_t player_item_snapshot_vector_operation_frame_bytes() noexcept
{
	// Only the actual outer vector source paths selected by native stock:
	// range construction/assign, reserve and full-capacity forward insert,
	// grow/push with nonthrowing row relocation, size/value index construction,
	// equal-allocator move assignment and nontrivial cleanup. Row/nested member
	// copies and CURRENT observers retain their separate profiles. This owns
	// neither subtree bit masks nor any unrelated inline output workspace.
	static_assert(std::is_nothrow_move_constructible_v<player_item_snapshot>);
	static_assert(std::is_nothrow_move_assignable_v<player_item_snapshot>);
	return item_list_vector_frames + item_list_move_frames + item_list_size_constructor_frames +
	       item_list_nontrivial_frames + snapshot_clone_vector_constructor_frames +
	       sizeof(size_t);
}
