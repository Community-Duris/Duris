#include "player_snapshot_capture.h"

#include "prototypes.h"
#include "structs.h"
#include "utils.h"

#include <algorithm>
#include <new>
#include <type_traits>
#include <unordered_set>

#include "assocs.h"
#include "files.h"
#include "spells.h"
#include "trophy.h"

extern P_index obj_index;
extern struct index_data *mob_index;
extern struct room_data *world;
extern int top_of_objt;
extern int top_of_mobt;
extern int top_of_world;
extern Skill skills[];

namespace
{
struct capture_budget
{
	size_t bytes = sizeof(player_snapshot);
	size_t rows = 0;
	size_t objects = 0;

	bool add(size_t byte_count, size_t row_count = 0)
	{
		if (byte_count > PLAYER_SNAPSHOT_MAX_BYTES -
					 std::min(bytes, PLAYER_SNAPSHOT_MAX_BYTES) ||
		    row_count > PLAYER_SNAPSHOT_MAX_ROWS - std::min(rows, PLAYER_SNAPSHOT_MAX_ROWS))
			return false;
		bytes += byte_count;
		rows += row_count;
		return true;
	}
};

bool copy_string(const char *source, std::string &destination, capture_budget &budget)
{
	const size_t length = source ? strlen(source) : 0;
	if (length > PLAYER_SNAPSHOT_MAX_STRING_BYTES || !budget.add(length + 1))
		return false;
	destination.assign(source ? source : "", length);
	return true;
}

template <typename T> bool add_status_integer(player_snapshot &snapshot, capture_budget &budget,
					      player_status_field field, T value)
{
	if (!budget.add(sizeof(player_snapshot_integer), 1))
		return false;
	player_snapshot_integer row = {};
	row.field = field;
	row.is_unsigned = std::is_unsigned_v<T>;
	if (row.is_unsigned)
		row.unsigned_value = static_cast<uint64_t>(value);
	else
		row.signed_value = static_cast<int64_t>(value);
	snapshot.status_integers.push_back(row);
	return true;
}

bool add_status_string(player_snapshot &snapshot, capture_budget &budget,
		       player_status_string_field field, const char *value)
{
	player_snapshot_string entry = {};
	entry.field = field;
	if (!copy_string(value, entry.value, budget) ||
	    !budget.add(sizeof(player_snapshot_string), 1))
		return false;
	snapshot.status_strings.push_back(std::move(entry));
	return true;
}

bool capture_status(P_char ch, player_snapshot &snapshot, capture_budget &budget)
{
#define ADD_STATUS(field, value)                                                                \
	do                                                                                      \
	{                                                                                       \
		if (!add_status_integer(snapshot, budget, player_status_field::field, (value))) \
			return false;                                                           \
	} while (0)
	if (!add_status_string(snapshot, budget, player_status_string_field::name, GET_NAME(ch)) ||
	    !add_status_string(snapshot, budget, player_status_string_field::short_description,
			       ch->player.short_descr) ||
	    !add_status_string(snapshot, budget, player_status_string_field::long_description,
			       ch->player.long_descr) ||
	    !add_status_string(snapshot, budget, player_status_string_field::description,
			       ch->player.description) ||
	    !add_status_string(snapshot, budget, player_status_string_field::title,
			       GET_TITLE(ch)) ||
	    !add_status_string(snapshot, budget, player_status_string_field::poof_in,
			       ch->only.pc->poofIn) ||
	    !add_status_string(snapshot, budget, player_status_string_field::poof_out,
			       ch->only.pc->poofOut))
		return false;

	ADD_STATUS(class_primary, ch->player.m_class);
	ADD_STATUS(class_secondary, ch->player.secondary_class);
	ADD_STATUS(specialization, ch->player.spec);
	ADD_STATUS(race, GET_RACE(ch));
	ADD_STATUS(racewar, GET_RACEWAR(ch));
	ADD_STATUS(level, GET_LEVEL(ch));
	ADD_STATUS(sex, GET_SEX(ch));
	ADD_STATUS(weight, ch->player.weight);
	ADD_STATUS(height, ch->player.height);
	ADD_STATUS(size, GET_SIZE(ch));
	ADD_STATUS(hometown, GET_HOME(ch));
	ADD_STATUS(birthplace, GET_BIRTHPLACE(ch));
	ADD_STATUS(original_birthplace, GET_ORIG_BIRTHPLACE(ch));
	ADD_STATUS(birth_time, ch->player.time.birth);
	ADD_STATUS(played_time, ch->player.time.played);
	ADD_STATUS(base_strength, ch->base_stats.Str);
	ADD_STATUS(base_dexterity, ch->base_stats.Dex);
	ADD_STATUS(base_agility, ch->base_stats.Agi);
	ADD_STATUS(base_constitution, ch->base_stats.Con);
	ADD_STATUS(base_power, ch->base_stats.Pow);
	ADD_STATUS(base_intelligence, ch->base_stats.Int);
	ADD_STATUS(base_wisdom, ch->base_stats.Wis);
	ADD_STATUS(base_charisma, ch->base_stats.Cha);
	ADD_STATUS(base_karma, ch->base_stats.Kar);
	ADD_STATUS(base_luck, ch->base_stats.Luk);
	ADD_STATUS(mana, GET_MANA(ch));
	ADD_STATUS(base_mana, ch->points.base_mana);
	ADD_STATUS(hit_difference, MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch)));
	ADD_STATUS(base_hit, ch->points.base_hit);
	ADD_STATUS(vitality, GET_VITALITY(ch));
	ADD_STATUS(base_vitality, ch->points.base_vitality);
	ADD_STATUS(extra_memorization, ch->only.pc->spells_memmed[MAX_CIRCLE]);
	ADD_STATUS(copper, GET_COPPER(ch));
	ADD_STATUS(silver, GET_SILVER(ch));
	ADD_STATUS(gold, GET_GOLD(ch));
	ADD_STATUS(platinum, GET_PLATINUM(ch));
	ADD_STATUS(experience, GET_EXP(ch));
	ADD_STATUS(epics, ch->only.pc->epics);
	ADD_STATUS(epic_skill_points, ch->only.pc->epic_skill_points);
	ADD_STATUS(skill_points, ch->only.pc->skillpoints);
	ADD_STATUS(spell_bind_used, ch->only.pc->spell_bind_used);
	ADD_STATUS(action_flags, ch->specials.act);
	ADD_STATUS(action_flags_2, ch->specials.act2);
	ADD_STATUS(action_flags_3, ch->specials.act3);
	ADD_STATUS(vote, ch->only.pc->vote);
	ADD_STATUS(alignment, ch->specials.alignment);
	ADD_STATUS(prestige, ch->only.pc->prestige);
	ADD_STATUS(guild_id, GET_ASSOC_ID(ch));
	ADD_STATUS(guild_status, ch->specials.guild_status);
	ADD_STATUS(time_left_guild, ch->only.pc->time_left_guild);
	ADD_STATUS(times_left_guild, ch->only.pc->nb_left_guild);
	ADD_STATUS(time_unspecialized, ch->only.pc->time_unspecced);
	ADD_STATUS(frags, ch->only.pc->frags);
	ADD_STATUS(old_frags, ch->only.pc->oldfrags);
	ADD_STATUS(deaths, ch->only.pc->numb_deaths);
	ADD_STATUS(echo, ch->only.pc->echo_toggle);
	ADD_STATUS(prompt, ch->only.pc->prompt);
	ADD_STATUS(wizard_invisibility, ch->only.pc->wiz_invis);
	ADD_STATUS(wimpy, ch->only.pc->wimpy);
	ADD_STATUS(aggressive, ch->only.pc->aggressive);
	ADD_STATUS(highest_level, ch->only.pc->highest_level);
	ADD_STATUS(screen_length, ch->only.pc->screen_length);
	ADD_STATUS(last_ip, ch->only.pc->last_ip);
#undef ADD_STATUS

	for (size_t index = 0; index < snapshot.conditions.size(); ++index)
		snapshot.conditions[index] = ch->specials.conditions[index];
	snapshot.quest_values = {
		ch->only.pc->quest_active,	  ch->only.pc->quest_mob_vnum,
		ch->only.pc->quest_type,	  ch->only.pc->quest_accomplished,
		ch->only.pc->quest_started,	  ch->only.pc->quest_zone_number,
		ch->only.pc->quest_giver,	  ch->only.pc->quest_level,
		ch->only.pc->quest_receiver,	  ch->only.pc->quest_shares_left,
		ch->only.pc->quest_kill_how_many, ch->only.pc->quest_kill_original,
		ch->only.pc->quest_map_room,	  ch->only.pc->quest_map_bought,
	};
	return budget.add(sizeof(snapshot.conditions) + sizeof(snapshot.quest_values));
}

template <typename T> bool add_index_value(std::vector<T> &target, capture_budget &budget,
					   int index, int64_t value, uint64_t auxiliary = 0)
{
	if (!budget.add(sizeof(T), 1))
		return false;
	target.push_back({ index, value, auxiliary });
	return true;
}

player_snapshot_capture_result capture_replacement_rows(P_char ch,
							player_component_mask_t components,
							player_snapshot &snapshot,
							capture_budget &budget)
{
	if (components & PLAYER_COMPONENT_LANGUAGES)
		for (int index = 0; index < MAX_TONGUE; ++index)
			if (GET_LANGUAGE(ch, index) &&
			    !add_index_value(snapshot.languages, budget, index,
					     GET_LANGUAGE(ch, index)))
				return player_snapshot_capture_result::limit_exceeded;
	if (components & PLAYER_COMPONENT_INTRODUCTIONS)
		for (int index = 0; index < MAX_INTRO; ++index)
			if ((ch->only.pc->introd_list[index] || ch->only.pc->introd_times[index]) &&
			    !add_index_value(snapshot.introductions, budget, index,
					     ch->only.pc->introd_list[index],
					     ch->only.pc->introd_times[index]))
				return player_snapshot_capture_result::limit_exceeded;
	if (components & PLAYER_COMPONENT_TIMERS)
		for (int index = 0; index < NUMB_PC_TIMERS; ++index)
			if (ch->only.pc->pc_timer[index] &&
			    !add_index_value(snapshot.timers, budget, index,
					     ch->only.pc->pc_timer[index]))
				return player_snapshot_capture_result::limit_exceeded;
	if (components & PLAYER_COMPONENT_UNDEAD_SLOTS)
		for (int index = 0; index <= MAX_CIRCLE; ++index)
			if (ch->specials.undead_spell_slots[index] &&
			    !add_index_value(snapshot.undead_slots, budget, index,
					     ch->specials.undead_spell_slots[index]))
				return player_snapshot_capture_result::limit_exceeded;
	if (components & PLAYER_COMPONENT_FORGED_ITEMS)
		for (int index = 0; index < MAX_FORGE_ITEMS; ++index)
			if (ch->only.pc->learned_forged_list[index] &&
			    !add_index_value(snapshot.forged_items, budget, index,
					     ch->only.pc->learned_forged_list[index]))
				return player_snapshot_capture_result::limit_exceeded;
	if (!(components & PLAYER_COMPONENT_GRANTED_COMMANDS))
		return player_snapshot_capture_result::ok;
	if (ch->only.pc->numb_gcmd < 0 || ch->only.pc->numb_gcmd > (int)PLAYER_SNAPSHOT_MAX_ROWS ||
	    (ch->only.pc->numb_gcmd && !ch->only.pc->gcmd_arr))
		return player_snapshot_capture_result::malformed_source;
	for (int index = 0; index < ch->only.pc->numb_gcmd; ++index)
	{
		if (!budget.add(sizeof(int32_t), 1))
			return player_snapshot_capture_result::limit_exceeded;
		snapshot.granted_commands.push_back(ch->only.pc->gcmd_arr[index]);
	}
	return player_snapshot_capture_result::ok;
}

bool capture_skills(P_char ch, player_snapshot &snapshot, capture_budget &budget)
{
	for (int index = 0; index < MAX_SKILLS; ++index)
	{
		const auto &skill = ch->only.pc->skills[index];
		if (!skill.learned && !skill.taught)
			continue;
		if (!budget.add(sizeof(player_skill_snapshot), 1))
			return false;
		snapshot.skills.push_back({ index, static_cast<uint8_t>(skill.learned),
					    static_cast<uint8_t>(skill.taught) });
	}
	return true;
}

player_snapshot_capture_result capture_affects(P_char ch, player_snapshot &snapshot,
					       capture_budget &budget)
{
	std::unordered_set<const affected_type *> seen;
	for (const affected_type *affect = ch->affected; affect; affect = affect->next)
	{
		if (!seen.insert(affect).second)
			return player_snapshot_capture_result::object_cycle;
		if (IS_SET(affect->flags, AFFTYPE_NOSAVE))
			continue;
		if (!budget.add(sizeof(player_affect_snapshot), 1))
			return player_snapshot_capture_result::limit_exceeded;
		player_affect_snapshot row = {};
		row.type = affect->type;
		row.duration = affect->duration;
		row.flags = affect->flags;
		row.modifier = affect->modifier;
		row.location = affect->location;
		row.level = affect->level;
		row.bitvectors = { affect->bitvector, affect->bitvector2, affect->bitvector3,
				   affect->bitvector4, affect->bitvector5 };
		if (affect->wear_off_message_index > 0 &&
		    affect->wear_off_message_index < MAX_WEAR_OFF_MESSAGES && affect->type >= 0 &&
		    affect->type < MAX_SKILLS)
		{
			if (!copy_string(skills[affect->type]
						 .wear_off_char[affect->wear_off_message_index],
					 row.wear_off_character, budget) ||
			    !copy_string(skills[affect->type]
						 .wear_off_room[affect->wear_off_message_index],
					 row.wear_off_room, budget))
				return player_snapshot_capture_result::limit_exceeded;
		}
		snapshot.affects.push_back(std::move(row));
	}
	return player_snapshot_capture_result::ok;
}

player_snapshot_capture_result
capture_item_tree(const obj_data *object, int parent_index, int equipment_slot,
		  std::vector<player_item_snapshot> &target, capture_budget &budget,
		  std::unordered_set<const obj_data *> &seen, size_t depth)
{
	if (!object || IS_SET(object->extra_flags, ITEM_NORENT))
		return player_snapshot_capture_result::ok;
	if (depth > PLAYER_SNAPSHOT_MAX_DEPTH)
		return player_snapshot_capture_result::limit_exceeded;
	if (!seen.insert(object).second)
		return player_snapshot_capture_result::object_cycle;
	if (object->R_num < 0 || object->R_num > top_of_objt)
		return player_snapshot_capture_result::malformed_source;
	if (budget.objects >= PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    !budget.add(sizeof(player_item_snapshot), 1))
		return player_snapshot_capture_result::limit_exceeded;
	++budget.objects;

	player_item_snapshot row = {};
	row.parent_index = parent_index;
	row.equipment_slot = static_cast<int16_t>(equipment_slot);
	row.object_uid = object->obj_uid;
	row.generated_key = object->g_key;
	row.vnum = obj_index[object->R_num].virtual_number;
	row.type = object->type;
	row.string_mask = object->str_mask;
	row.wear_flags = object->wear_flags;
	row.extra_flags = object->extra_flags;
	row.anti_flags = object->anti_flags;
	row.anti2_flags = object->anti2_flags;
	row.extra2_flags = object->extra2_flags;
	row.weight = object->weight;
	row.material = object->material;
	row.cost = object->cost;
	row.condition = object->condition;
	row.craftsmanship = object->craftsmanship;
	row.bitvectors = { object->bitvector, object->bitvector2, object->bitvector3,
			   object->bitvector4, object->bitvector5 };
	if ((IS_SET(object->str_mask, STRUNG_KEYS) &&
	     !copy_string(object->name, row.name, budget)) ||
	    (IS_SET(object->str_mask, STRUNG_DESC2) &&
	     !copy_string(object->short_description, row.short_description, budget)) ||
	    (IS_SET(object->str_mask, STRUNG_DESC1) &&
	     !copy_string(object->description, row.description, budget)) ||
	    (IS_SET(object->str_mask, STRUNG_DESC3) &&
	     !copy_string(object->action_description, row.action_description, budget)))
		return player_snapshot_capture_result::limit_exceeded;
	for (size_t index = 0; index < row.values.size(); ++index)
		row.values[index] = object->value[index];
	for (size_t index = 0; index < row.timers.size(); ++index)
		row.timers[index] = object->timer[index];
	for (size_t index = 0; index < row.affects.size(); ++index)
		row.affects[index] = { object->affected[index].location,
				       object->affected[index].modifier };

	std::unordered_set<const obj_affect *> dynamic_seen;
	for (const obj_affect *affect = object->affects; affect; affect = affect->next)
	{
		if (!dynamic_seen.insert(affect).second)
			return player_snapshot_capture_result::object_cycle;
		if (!budget.add(sizeof(player_item_dynamic_affect_snapshot), 1))
			return player_snapshot_capture_result::limit_exceeded;
		row.dynamic_affects.push_back({ affect->type, affect->data, affect->extra2 });
	}
	std::unordered_set<const extra_descr_data *> description_seen;
	for (const extra_descr_data *description = object->ex_description; description;
	     description = description->next)
	{
		if (!description_seen.insert(description).second)
			return player_snapshot_capture_result::object_cycle;
		if (!budget.add(sizeof(player_item_extra_description_snapshot), 1))
			return player_snapshot_capture_result::limit_exceeded;
		player_item_extra_description_snapshot extra = {};
		const bool spellbook = description->keyword && strlen(description->keyword) == 3 &&
				       description->keyword[0] == 3 &&
				       description->keyword[1] == 1 && description->keyword[2] == 3;
		if (spellbook)
		{
			extra.spellbook = true;
			if (!copy_string("SPELLBOOK", extra.keyword, budget) ||
			    !description->description)
				return player_snapshot_capture_result::malformed_source;
			for (int skill_id = 0; skill_id < MAX_SKILLS; ++skill_id)
				if (description->description[skill_id / 8] & (1 << (skill_id % 8)))
				{
					if (!budget.add(sizeof(int32_t)))
						return player_snapshot_capture_result::limit_exceeded;
					extra.spell_ids.push_back(skill_id);
				}
		}
		else if (!copy_string(description->keyword, extra.keyword, budget) ||
			 !copy_string(description->description, extra.description, budget))
			return player_snapshot_capture_result::limit_exceeded;
		row.extra_descriptions.push_back(std::move(extra));
	}

	const int row_index = static_cast<int>(target.size());
	target.push_back(std::move(row));
	for (const obj_data *content = object->contains; content; content = content->next_content)
	{
		const auto result =
			capture_item_tree(content, row_index, 0, target, budget, seen, depth + 1);
		if (result != player_snapshot_capture_result::ok)
			return result;
	}
	return player_snapshot_capture_result::ok;
}

player_snapshot_capture_result capture_items(P_char owner,
					     std::vector<player_item_snapshot> &target,
					     capture_budget &budget, bool equipment, bool inventory)
{
	std::unordered_set<const obj_data *> seen;
	if (equipment)
	{
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			const auto result = capture_item_tree(owner->equipment[slot],
							      PLAYER_SNAPSHOT_NO_PARENT, slot + 1,
							      target, budget, seen, 1);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
	}
	if (inventory)
	{
		for (const obj_data *object = owner->carrying; object;
		     object = object->next_content)
		{
			const auto result = capture_item_tree(object, PLAYER_SNAPSHOT_NO_PARENT, 0,
							      target, budget, seen, 1);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
	}
	return player_snapshot_capture_result::ok;
}

player_snapshot_capture_result capture_pets(P_char ch, int save_intent, player_snapshot &snapshot,
					    capture_budget &budget)
{
	if (save_intent != RENT_CRASH && save_intent != RENT_CRASH2)
		return player_snapshot_capture_result::ok;
	std::unordered_set<const follow_type *> followers_seen;
	int order = 0;
	for (const follow_type *follow = ch->followers; follow; follow = follow->next)
	{
		if (!followers_seen.insert(follow).second)
			return player_snapshot_capture_result::object_cycle;
		P_char pet = follow->follower;
		if (!pet || !IS_NPC(pet) || pet->in_room != ch->in_room)
			continue;
		const int rnum = GET_RNUM(pet);
		if (rnum < 0 || rnum > top_of_mobt)
			return player_snapshot_capture_result::malformed_source;
		if (!budget.add(sizeof(player_pet_snapshot), 1))
			return player_snapshot_capture_result::limit_exceeded;
		player_pet_snapshot row = {};
		row.mob_vnum = mob_index[rnum].virtual_number;
		row.order = order++;
		row.hit = GET_HIT(pet);
		row.max_hit = GET_MAX_HIT(pet);
		row.mana = GET_MANA(pet);
		row.max_mana = GET_MAX_MANA(pet);
		row.vitality = GET_VITALITY(pet);
		row.max_vitality = GET_MAX_VITALITY(pet);
		row.charm_duration = -1;
		if (pet->in_room < 0 || pet->in_room > top_of_world)
			return player_snapshot_capture_result::malformed_source;
		row.room_vnum = world[pet->in_room].number;
		std::unordered_set<const affected_type *> pet_affects_seen;
		for (const affected_type *affect = pet->affected; affect; affect = affect->next)
		{
			if (!pet_affects_seen.insert(affect).second)
				return player_snapshot_capture_result::object_cycle;
			if (affect->type == SPELL_CHARM_PERSON)
			{
				row.charm_duration = affect->duration;
				break;
			}
		}
		const auto item_result = capture_items(pet, row.items, budget, true, true);
		if (item_result != player_snapshot_capture_result::ok)
			return item_result;
		snapshot.pets.push_back(std::move(row));
	}
	return player_snapshot_capture_result::ok;
}

player_snapshot_capture_result capture_shapes_and_trophies(P_char ch,
							   player_component_mask_t components,
							   player_snapshot &snapshot,
							   capture_budget &budget)
{
	if ((components & PLAYER_COMPONENT_SHAPECHANGES) && has_innate(ch, INNATE_SHAPECHANGE))
	{
		std::unordered_set<const char_shapechange_data *> seen;
		for (const char_shapechange_data *shape = ch->only.pc->knownShapes; shape;
		     shape = shape->next)
		{
			if (!seen.insert(shape).second)
				return player_snapshot_capture_result::object_cycle;
			if (!budget.add(sizeof(player_shape_snapshot), 1))
				return player_snapshot_capture_result::limit_exceeded;
			snapshot.shapes.push_back({ shape->mobVnum, shape->timesResearched,
						    shape->lastResearched,
						    shape->lastShapechanged });
		}
	}
	if ((components & PLAYER_COMPONENT_TROPHIES) && ZONE_TROPHY(ch))
		for (const zone_trophy_data &trophy : *ZONE_TROPHY(ch))
		{
			if (!budget.add(sizeof(player_trophy_snapshot), 1))
				return player_snapshot_capture_result::limit_exceeded;
			snapshot.trophies.push_back({ trophy.zone_number, trophy.exp });
		}
	return player_snapshot_capture_result::ok;
}
} // namespace

player_snapshot_capture_result player_snapshot_capture(P_char ch, player_revision_t revision,
						       player_component_mask_t components,
						       int save_intent, int room_vnum,
						       player_snapshot *snapshot_out)
{
	if (!ch || !snapshot_out || IS_NPC(ch) || !ch->only.pc || GET_PID(ch) <= 0 || !revision ||
	    !components || (components & ~PLAYER_CHECKPOINT_COMPONENT_ALL))
		return player_snapshot_capture_result::invalid_identity;

	try
	{
		player_snapshot snapshot = {};
		snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
		snapshot.pid = GET_PID(ch);
		snapshot.revision = revision;
		snapshot.components = components;
		snapshot.save_intent = save_intent;
		snapshot.room_vnum = room_vnum;
		snapshot.recipes_are_external = true;
		capture_budget budget;
		if ((components & PLAYER_COMPONENT_STATUS) && !capture_status(ch, snapshot, budget))
			return player_snapshot_capture_result::limit_exceeded;
		{
			const auto result =
				capture_replacement_rows(ch, components, snapshot, budget);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		if ((components & PLAYER_COMPONENT_SKILLS) && !capture_skills(ch, snapshot, budget))
			return player_snapshot_capture_result::limit_exceeded;
		if (components & PLAYER_COMPONENT_AFFECTS)
		{
			const auto result = capture_affects(ch, snapshot, budget);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		if (components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		{
			const auto result = capture_items(ch, snapshot.items, budget,
							  components & PLAYER_COMPONENT_EQUIPMENT,
							  components & PLAYER_COMPONENT_INVENTORY);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		if (components & PLAYER_COMPONENT_PETS)
		{
			const auto result = capture_pets(ch, save_intent, snapshot, budget);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		if (components & (PLAYER_COMPONENT_SHAPECHANGES | PLAYER_COMPONENT_TROPHIES))
		{
			const auto result =
				capture_shapes_and_trophies(ch, components, snapshot, budget);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		snapshot.encoded_size_bound = budget.bytes;
		*snapshot_out = std::move(snapshot);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
	return player_snapshot_capture_result::ok;
}
