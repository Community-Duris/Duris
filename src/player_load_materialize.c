#include "player_load_materialize.h"

#include "prototypes.h"
#include "structs.h"
#include "db.h"
#include "utils.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <unordered_set>

#include "assocs.h"
#include "player_revision_state.h"
#include "spells.h"

namespace
{
int64_t value(const player_snapshot_integer &entry)
{
	return entry.is_unsigned ? static_cast<int64_t>(entry.unsigned_value) : entry.signed_value;
}

bool valid_snapshot(const player_load_result &result)
{
	if (result.outcome != player_load_outcome::applied || result.pid <= 0 ||
	    result.snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION ||
	    result.snapshot.pid != result.pid ||
	    result.snapshot.components != PLAYER_LOAD_SESSION01_COMPONENTS ||
	    result.snapshot.status_strings.size() != 7 ||
	    result.snapshot.status_integers.size() != 63 ||
	    result.metrics.query_count > PLAYER_LOAD_QUERY_MAX ||
	    result.metrics.row_count > PLAYER_SNAPSHOT_MAX_ROWS ||
	    result.metrics.byte_count > PLAYER_SNAPSHOT_MAX_BYTES ||
	    result.metrics.transaction_usec > PLAYER_LOAD_TIMEOUT_USEC)
		return false;
	std::unordered_set<unsigned int> integers;
	std::unordered_set<unsigned int> strings;
	for (const player_snapshot_integer &entry : result.snapshot.status_integers)
		if (entry.field < player_status_field::class_primary ||
		    entry.field > player_status_field::last_ip ||
		    !integers.insert(static_cast<unsigned int>(entry.field)).second)
			return false;
	for (const player_snapshot_string &entry : result.snapshot.status_strings)
		if (entry.field < player_status_string_field::name ||
		    entry.field > player_status_string_field::poof_out ||
		    entry.value.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
		    !strings.insert(static_cast<unsigned int>(entry.field)).second)
			return false;
	for (const player_index_value_snapshot &entry : result.snapshot.languages)
		if (entry.index < 0 || entry.index >= MAX_TONGUE)
			return false;
	for (const player_index_value_snapshot &entry : result.snapshot.introductions)
		if (entry.index < 0 || entry.index >= MAX_INTRO)
			return false;
	for (const player_index_value_snapshot &entry : result.snapshot.timers)
		if (entry.index < 0 || entry.index >= NUMB_PC_TIMERS)
			return false;
	for (const player_index_value_snapshot &entry : result.snapshot.undead_slots)
		if (entry.index < 0 || entry.index > MAX_CIRCLE)
			return false;
	for (const player_index_value_snapshot &entry : result.snapshot.forged_items)
		if (entry.index < 0 || entry.index >= MAX_FORGE_ITEMS)
			return false;
	for (const player_skill_snapshot &entry : result.snapshot.skills)
		if (entry.skill_id < 0 || entry.skill_id >= MAX_SKILLS)
			return false;
	for (const player_affect_snapshot &entry : result.snapshot.affects)
		if (entry.wear_off_character.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
		    entry.wear_off_room.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES)
			return false;
	for (uint64_t balance : result.domains.wallet)
		if (balance > INT_MAX)
			return false;
	for (uint64_t balance : result.domains.bank)
		if (balance > INT_MAX)
			return false;
	return true;
}

bool apply_string(P_char ch, const player_snapshot_string &entry)
{
	char *copy = str_dup(entry.value.c_str());
	if (!copy)
		return false;
	switch (entry.field)
	{
	case player_status_string_field::name:
		GET_NAME(ch) = copy;
		break;
	case player_status_string_field::short_description:
		ch->player.short_descr = copy;
		break;
	case player_status_string_field::long_description:
		ch->player.long_descr = copy;
		break;
	case player_status_string_field::description:
		ch->player.description = copy;
		break;
	case player_status_string_field::title:
		GET_TITLE(ch) = copy;
		break;
	case player_status_string_field::poof_in:
		ch->only.pc->poofIn = copy;
		break;
	case player_status_string_field::poof_out:
		ch->only.pc->poofOut = copy;
		break;
	}
	return true;
}

void apply_integer(P_char ch, const player_snapshot_integer &entry, int *hit_difference)
{
	const int64_t loaded = value(entry);
	switch (entry.field)
	{
	case player_status_field::class_primary:
		ch->player.m_class = loaded;
		break;
	case player_status_field::class_secondary:
		ch->player.secondary_class = loaded;
		break;
	case player_status_field::specialization:
		ch->player.spec = loaded;
		break;
	case player_status_field::race:
		GET_RACE(ch) = loaded;
		break;
	case player_status_field::racewar:
		GET_RACEWAR(ch) = loaded;
		break;
	case player_status_field::level:
		ch->player.level = loaded;
		break;
	case player_status_field::sex:
		GET_SEX(ch) = loaded;
		break;
	case player_status_field::weight:
		ch->player.weight = loaded;
		break;
	case player_status_field::height:
		ch->player.height = loaded;
		break;
	case player_status_field::size:
		GET_SIZE(ch) = loaded;
		break;
	case player_status_field::hometown:
		GET_HOME(ch) = loaded;
		break;
	case player_status_field::birthplace:
		GET_BIRTHPLACE(ch) = loaded;
		break;
	case player_status_field::original_birthplace:
		GET_ORIG_BIRTHPLACE(ch) = loaded;
		break;
	case player_status_field::birth_time:
		ch->player.time.birth = loaded;
		break;
	case player_status_field::played_time:
		ch->player.time.played = loaded;
		break;
	case player_status_field::base_strength:
		ch->base_stats.Str = loaded;
		break;
	case player_status_field::base_dexterity:
		ch->base_stats.Dex = loaded;
		break;
	case player_status_field::base_agility:
		ch->base_stats.Agi = loaded;
		break;
	case player_status_field::base_constitution:
		ch->base_stats.Con = loaded;
		break;
	case player_status_field::base_power:
		ch->base_stats.Pow = loaded;
		break;
	case player_status_field::base_intelligence:
		ch->base_stats.Int = loaded;
		break;
	case player_status_field::base_wisdom:
		ch->base_stats.Wis = loaded;
		break;
	case player_status_field::base_charisma:
		ch->base_stats.Cha = loaded;
		break;
	case player_status_field::base_karma:
		ch->base_stats.Kar = loaded;
		break;
	case player_status_field::base_luck:
		ch->base_stats.Luk = loaded;
		break;
	case player_status_field::mana:
		GET_MANA(ch) = loaded;
		break;
	case player_status_field::base_mana:
		ch->points.base_mana = loaded;
		break;
	case player_status_field::hit_difference:
		*hit_difference = loaded;
		break;
	case player_status_field::base_hit:
		ch->points.base_hit = loaded;
		break;
	case player_status_field::vitality:
		GET_VITALITY(ch) = loaded;
		break;
	case player_status_field::base_vitality:
		ch->points.base_vitality = loaded;
		break;
	case player_status_field::extra_memorization:
		ch->only.pc->spells_memmed[MAX_CIRCLE] = loaded;
		break;
	case player_status_field::experience:
		GET_EXP(ch) = loaded;
		break;
	case player_status_field::epic_skill_points:
		ch->only.pc->epic_skill_points = loaded;
		break;
	case player_status_field::skill_points:
		ch->only.pc->skillpoints = loaded;
		break;
	case player_status_field::spell_bind_used:
		ch->only.pc->spell_bind_used = loaded;
		break;
	case player_status_field::action_flags:
		ch->specials.act = loaded;
		break;
	case player_status_field::action_flags_2:
		ch->specials.act2 = loaded;
		break;
	case player_status_field::action_flags_3:
		ch->specials.act3 = loaded;
		break;
	case player_status_field::vote:
		ch->only.pc->vote = loaded;
		break;
	case player_status_field::alignment:
		ch->specials.alignment = loaded;
		break;
	case player_status_field::prestige:
		ch->only.pc->prestige = loaded;
		break;
	case player_status_field::guild_id:
		ch->specials.guild = loaded > 0 ? get_guild_from_id(loaded) : nullptr;
		break;
	case player_status_field::guild_status:
		ch->specials.guild_status = loaded;
		break;
	case player_status_field::time_left_guild:
		ch->only.pc->time_left_guild = loaded;
		break;
	case player_status_field::times_left_guild:
		ch->only.pc->nb_left_guild = loaded;
		break;
	case player_status_field::time_unspecialized:
		ch->only.pc->time_unspecced = loaded;
		break;
	case player_status_field::deaths:
		ch->only.pc->numb_deaths = loaded;
		break;
	case player_status_field::echo:
		ch->only.pc->echo_toggle = loaded;
		break;
	case player_status_field::prompt:
		ch->only.pc->prompt = loaded;
		break;
	case player_status_field::wizard_invisibility:
		ch->only.pc->wiz_invis = loaded;
		break;
	case player_status_field::wimpy:
		ch->only.pc->wimpy = loaded;
		break;
	case player_status_field::aggressive:
		ch->only.pc->aggressive = loaded;
		break;
	case player_status_field::highest_level:
		ch->only.pc->highest_level = loaded;
		break;
	case player_status_field::screen_length:
		ch->only.pc->screen_length = loaded;
		break;
	case player_status_field::last_ip:
		ch->only.pc->last_ip = loaded;
		break;
	case player_status_field::copper:
	case player_status_field::silver:
	case player_status_field::gold:
	case player_status_field::platinum:
	case player_status_field::epics:
	case player_status_field::frags:
	case player_status_field::old_frags:
		break;
	}
}
} // namespace

bool player_load_materialize(P_char ch, const player_load_result &result)
{
	if (!ch || !ch->only.pc || !valid_snapshot(result))
		return false;
	int hit_difference = 0;
	for (const player_snapshot_string &entry : result.snapshot.status_strings)
		if (!apply_string(ch, entry))
			return false;
	for (const player_snapshot_integer &entry : result.snapshot.status_integers)
		apply_integer(ch, entry, &hit_difference);
	ch->only.pc->pid = result.pid;
	ch->player.time.saved = result.saved_at;
	ch->player.time.logon = time(nullptr);
	ch->specials.was_in_room = result.snapshot.room_vnum;
	ch->in_room = real_room(result.snapshot.room_vnum);
	if (ch->in_room != NOWHERE && IS_ROOM(ch->in_room, ROOM_LOCKER))
	{
		const int locker_room = ch->in_room;
		int exit_room = NOWHERE;
		if (world[locker_room].dir_option[0] &&
		    world[locker_room].dir_option[0]->to_room != NOWHERE)
			exit_room = world[locker_room].dir_option[0]->to_room;
		else if (GET_HOME(ch))
			exit_room = real_room(GET_HOME(ch));
		if (exit_room == NOWHERE && GET_BIRTHPLACE(ch))
			exit_room = real_room(GET_BIRTHPLACE(ch));
		if (exit_room != NOWHERE)
		{
			logit(LOG_DEBUG,
			      "player_load_materialize: location=locker outcome=redirected");
			ch->specials.was_in_room = world[exit_room].number;
			ch->in_room = exit_room;
		}
	}
	GET_COPPER(ch) = result.domains.wallet[0];
	GET_SILVER(ch) = result.domains.wallet[1];
	GET_GOLD(ch) = result.domains.wallet[2];
	GET_PLATINUM(ch) = result.domains.wallet[3];
	GET_BALANCE_COPPER(ch) = result.domains.bank[0];
	GET_BALANCE_SILVER(ch) = result.domains.bank[1];
	GET_BALANCE_GOLD(ch) = result.domains.bank[2];
	GET_BALANCE_PLATINUM(ch) = result.domains.bank[3];
	ch->only.pc->wallet_revision = result.domains.wallet_revision;
	ch->only.pc->bank_revision = result.domains.bank_revision;
	ch->only.pc->epics = result.domains.epics;
	ch->only.pc->epic_revision = result.domains.epic_revision;
	ch->only.pc->frags = result.domains.frags;
	ch->only.pc->oldfrags = result.domains.old_frags;
	ch->only.pc->frag_revision = result.domains.frag_revision;
	for (size_t index = 0; index < result.snapshot.conditions.size(); ++index)
		ch->specials.conditions[index] = result.snapshot.conditions[index];
	int32_t *quest[] = {
		&ch->only.pc->quest_active,	   &ch->only.pc->quest_mob_vnum,
		&ch->only.pc->quest_type,	   &ch->only.pc->quest_accomplished,
		&ch->only.pc->quest_started,	   &ch->only.pc->quest_zone_number,
		&ch->only.pc->quest_giver,	   &ch->only.pc->quest_level,
		&ch->only.pc->quest_receiver,	   &ch->only.pc->quest_shares_left,
		&ch->only.pc->quest_kill_how_many, &ch->only.pc->quest_kill_original,
		&ch->only.pc->quest_map_room,	   &ch->only.pc->quest_map_bought,
	};
	for (size_t index = 0; index < result.snapshot.quest_values.size(); ++index)
		*quest[index] = result.snapshot.quest_values[index];
	for (const player_index_value_snapshot &entry : result.snapshot.languages)
		GET_LANGUAGE(ch, entry.index) = entry.value;
	for (const player_index_value_snapshot &entry : result.snapshot.introductions)
	{
		ch->only.pc->introd_list[entry.index] = entry.value;
		ch->only.pc->introd_times[entry.index] = entry.auxiliary;
	}
	for (const player_index_value_snapshot &entry : result.snapshot.timers)
		ch->only.pc->pc_timer[entry.index] = entry.value;
	for (const player_index_value_snapshot &entry : result.snapshot.undead_slots)
		ch->specials.undead_spell_slots[entry.index] = entry.value;
	for (const player_index_value_snapshot &entry : result.snapshot.forged_items)
		ch->only.pc->learned_forged_list[entry.index] = entry.value;
	if (!result.snapshot.granted_commands.empty())
	{
		ch->only.pc->gcmd_arr = static_cast<int *>(
			malloc(result.snapshot.granted_commands.size() * sizeof(int)));
		if (!ch->only.pc->gcmd_arr)
			return false;
		ch->only.pc->numb_gcmd = result.snapshot.granted_commands.size();
		for (size_t index = 0; index < result.snapshot.granted_commands.size(); ++index)
			ch->only.pc->gcmd_arr[index] = result.snapshot.granted_commands[index];
	}
	for (const player_skill_snapshot &entry : result.snapshot.skills)
	{
		ch->only.pc->skills[entry.skill_id].learned = entry.learned;
		ch->only.pc->skills[entry.skill_id].taught = entry.taught;
	}
	for (const player_affect_snapshot &entry : result.snapshot.affects)
	{
		affected_type affect = {};
		affect.type = entry.type;
		affect.duration = entry.duration;
		affect.flags = entry.flags;
		affect.modifier = entry.modifier;
		affect.location = entry.location;
		affect.level = entry.level;
		affect.bitvector = entry.bitvectors[0];
		affect.bitvector2 = entry.bitvectors[1];
		affect.bitvector3 = entry.bitvectors[2];
		affect.bitvector4 = entry.bitvectors[3];
		affect.bitvector5 = entry.bitvectors[4];
		if (!entry.wear_off_character.empty() || !entry.wear_off_room.empty())
			affect_to_char_with_messages(ch, &affect, entry.wear_off_character.c_str(),
						     entry.wear_off_room.c_str());
		else
			affect_to_char(ch, &affect);
	}
	char_shapechange_data **shape = &ch->only.pc->knownShapes;
	for (const player_shape_snapshot &entry : result.snapshot.shapes)
	{
		if (real_mobile(entry.mob_vnum) < 0)
			continue;
		*shape = static_cast<char_shapechange_data *>(
			calloc(1, sizeof(char_shapechange_data)));
		if (!*shape)
			return false;
		(*shape)->mobVnum = entry.mob_vnum;
		(*shape)->timesResearched = entry.times_researched;
		(*shape)->lastResearched = entry.last_researched;
		(*shape)->lastShapechanged = entry.last_shapechanged;
		shape = &(*shape)->next;
	}
	SET_POS(ch, POS_STANDING + STAT_NORMAL);
	GET_HIT(ch) = GET_MAX_HIT(ch) - hit_difference;
	return player_revision_hydrate(result.pid, result.snapshot.revision);
}
