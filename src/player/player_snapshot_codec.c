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
		    row.owner.type > item_owner_type::pet)
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
		const uint32_t wire_version = snapshot.schema_version;
		if (wire_version == 1 || wire_version == 3 || wire_version == 5)
			snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		if (wire_version == 2 || wire_version == 4 || wire_version == 6)
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
