/****************************************************************************
 *
 *  File: fight.c                                            Part of Duris
 *  Usage: Combat system and messages.
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#define TROPHY

#include "core/prototypes.h"
#include "cmd/track.h"
#include "combat/death_messages.h"
#include "telemetry/telemetry_runtime.h"
#include "net/output_style.h"
#include "item/item_actions.h"
#include "item/weapon_actions.h"
#include "item/native_artifact_actions.h"
#include "world/difficulty.h"
#include "world/bloodstains.h"
#include "core/structs.h"
#include "core/files.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "item/enhance.h"
#include "core/utility.h"
#include "core/utils.h"
#include <algorithm>
#include <math.h>
#include <set>
#include <vector>
#include <stdio.h>
#include <string.h>
#include "account/account_reward.h"
#include "world/achievements.h"
#include "combat/arena.h"
#include "combat/arenadef.h"
#include "combat/attack_continuation.h"
#include "combat/attack_effects.h"
#include "combat/attack_resolution.h"
#include "combat/death_messages.h"
#include "economy/boon.h"
#include "combat/ctf.h"
#include "combat/damage.h"
#include "combat/dam_mods.h"
#include "combat/training_dummy.h"
#include "classes/disguise.h"
#include "classes/dreadlord.h"
#include "world/epic.h"
#include "world/events.h"
#include "net/gmcp.h"
#include "world/hardcore_config.h"
#include "combat/grapple.h"
#include "world/map.h"
#include "core/mm.h"
#include "classes/necromancy.h"
#include "item/objmisc.h"
#include "world/outposts.h"
#include "persistence/persistence_checkpoint.h"
#include "classes/paladins.h"
#include "item/plushit.h"
#include "classes/reavers.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "mob/studioproc.h"
#include "world/vnum.obj.h"
#include "world/weather.h"
#include "world/world_quest.h"
#include "net/ws_handlers.h"
#include "guild/artifact_guild_transaction.h"
#include "persistence/corpse_lifecycle_transaction.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_death_enrollment.h"
#include "economy/collector_presence.h"
#include "player/player_save_pipeline.h"
#include "persistence/persistence_observability.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/forced_weapon_drop.h"
#include "combat/combat_outcome_transaction.h"
#include "persistence/gameplay_read_state.h"
/*
 * external variables //
 */
extern P_char combat_list;
extern P_char combat_next_ch;

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern char debug_mode;
extern const struct race_names race_names_table[];
extern void event_wait(P_char ch, P_char victim, P_obj obj, void *data);
void release_mob_mem(P_char ch, P_char victim, P_obj obj, void *data);

// extern const int material_absorbtion[][];
extern const struct stat_data stat_factor[];
extern float fake_sqrt_table[];
extern int pulse;
extern int arena_hometown_location[];
extern struct arena_data arena;
extern struct agi_app_type agi_app[];
extern struct dex_app_type dex_app[];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern const int class_tohit_mod[];
extern Skill skills[];
extern long new_exp_table[]; // Arih: Fixed type mismatch bug - was int, should be long
extern struct wis_app_type wis_app[];

extern const char *arena_death_msg(P_obj p_weapon);
extern void send_to_arena(const char *msg, int race);
extern int get_numb_chars_in_group(struct group_list *group);
extern int get_number_allies_in_room(P_char ch, int room_index);
extern P_char misfire_check(P_char ch, P_char spell_target, int flag);
extern bool check_reincarnate(P_char ch);
bool handle_imprison_damage(P_char, P_char, int);
extern bool innate_two_daggers(P_char);
extern bool divine_blessing_parry(P_char, P_char);
extern int get_honing(P_obj);
extern void apply_honing(P_obj, int);
extern void holy_crusade_check(P_char, P_char);
extern int is_wearing_necroplasm(P_char);

extern bool has_dragoon_mount(P_char ch);
extern bool is_dragoon_mounted(P_char ch);
extern bool is_dragoon_mount(P_char mount);
extern bool is_in_dragoon_group(P_char ch, P_char vict);
extern int get_next_dragoon_circle(P_char ch);
extern P_char get_dragoon_mount(P_char ch);

/* Structures */

// extern struct mm_ds *wdead_trophy_pool;

extern dam_mod_predicate spell_damage_modifiers[NUM_SPELL_PREDICATES];
extern dam_mod_predicate raw_damage_modifiers[NUM_RAW_PREDICATES];

#define SOUL_TRAP_SINGLE 0
#define SOUL_TRAP_GROUP 1

/* Weapon attack texts */

struct melee_death_messages
{
	const char *attacker;
	const char *victim;
	const char *room;
} melee_death_messages_table[] = {
	// HIT
	{ "Your fearsome punch hits $N, smashing $M to death.",
	  "$n's final punch sends you to meet your maker.", "$n punches $N to death." },
	{ "You hit $N in the throat, $E chokes, gasps and dies.",
	  "$n smashes your throat.  Gasping and choking, you descend into darkness.",
	  "$n punches $N in the throat, $E chokes, gasps and dies." },
	// BLUDGEON
	{ "You bludgeon $N, killing $M instantly.", "You are bludgeoned to death by $n.",
	  "$n ruthlessly bludgeons $N to death." },
	{ "Your wicked bludgeon crushed $N to death.",
	  "A wicked bludgeon from $n sends you spiraling into darkness.",
	  "A blood spray erupts from $N as $E is bludgeoned to death by $n." },
	// PIERCE
	{ "You successfully stab $N in the heart, killing $M instantly.",
	  "$n stabs you directly in the heart, killing you instantly.",
	  "$n stabs the heart of $N who falls to the ground clutching at the wound." },
	{ "Your stab cuts a vital artery and causes $N to fall before your feet.",
	  "$n's stab causes massive blood-loss and you fall to the ground, lifeless.",
	  "$n stabs viciously at $N, whose lifeless body falls to the ground." },
	// SLASH
	{ "You beautifully slash $N into two parts - both dead.",
	  "Your upper body is disconnected from your legs as $n slashes you.",
	  "$N is slashed into two by a master stroke performed by $n." },
	{ "Your final slash sends $N's head bouncing along the ground.",
	  "Your vision spins into darkness as your head falls towards the ground.",
	  "$n neatly performs a coup de grace and beheads $N." },
	// WHIP
	{ "Your whip opens a fatal wound and $N falls to the ground dead.",
	  "$n's lashing whip causes immense blood-loss, effectively killing you.",
	  "With a final stunning whip, $n ends $N's life." },
	{ "The crack of your whip is the last thing $N hears before death.",
	  "A stinging whip from $n spills your entrails onto the ground.",
	  "The gash opened up by $n's whip fatally wounds $N." },
	// CLAW
	{ "Your claw opens a fatal wound, and $N gives up the fight for good.",
	  "$n's claw finds your vital organs and rips them out, killing you instantly.",
	  "With a final swipe of a claw, $n ends $N's life for good." },
	{ "You swipe a vicious claw at $N, causing a fatal gash.",
	  "A fearsome swipe of a claw from $n ends your existence.",
	  "A final fearsome swipe of a claw from $n kills $N quickly." },
	// BITE
	{ "Your bite opens a fatal wound, and $N gives up the fight for good.",
	  "$n's final bite crushes your throat.", "With a final chomp, $n ends $N's life." },
	{ "You bite down hard upon $N and have the satisfaction of killing $M!",
	  "A final bite from $n chews through you and your life slips away.",
	  "$n nearly chomps $N in half with a wicked bite, gruesomely killing $M." },
	// STING
	{ "Your sting finally reached a vital organ killing $N instantly.",
	  "You begin to blackout as overwhelming pain expands outward from $n's sting.",
	  "$n's sting reached $N's heart killing $M instantly." },
	{ 0, 0, 0 },
	// CRUSH
	{ "Your final crushing blow caves in $N's chest, and $E dies rather quickly.",
	  "$n crushes your chest, and all your hopes of escaping this battle alive.",
	  "Snapping ribs can be heard as $n caves in $N's chest with a fearsome blow." },
	{ "You crushed $N's skull, I'm afraid $E's dead.",
	  "$n's crushing blow to your head makes you see stars, then nothing.",
	  "$n crushes $N's skull.  $N instantly collapses, lifeless." },
	// MAUL
	{ "Your maul hits $N squarely, smashing the life from $M.",
	  "$n's final devastating maul sends you to meet your maker.",
	  "$n smashes $N to death with a vicious maul." },
	{ 0, 0, 0 },
	// THRASH
	{ "$N's chest falls into itself as your hooves land on it!",
	  "$n's hooves crush your chest completely.",
	  "$N's chest falls into itself as $n's hooves land on it!" },
	{ 0, 0, 0 },
	// TOUCH
	{ "$N's life is drained away by your supernatural touch!",
	  "$n's touch drains the last life force from your body!",
	  "$N's touch leaves behind a lifeless husk of what once was $n!" },
};

/* Location of attack texts */
struct attack_hit_type location_hit_text[] = { { "", " body",
						 "" }, /* extras represent percent chance */
					       { "", " body", "" },
					       { "", " body", "" },
					       { "", " body", "" },
					       { " on $S left leg", " left leg", "" },
					       { " on $S right leg", " right leg", "" },
					       { " on $S left arm", " left arm", "" },
					       { " on $S right arm", " right arm", "" },
					       { " on $S shoulder", " shoulder", "" },
					       { " on $S chest", " chest", "" },
					       { " on $S chest", " chest", "" },
					       { " on $S stomach", " stomach", "" },
					       { " on $S head", " head", "" } };

int on_front_line(P_char);

// This function figures a time for victim to sit around in heaven (and sets it).
// Right now it has a base of level+20 sec + 3 minutes for every PvP death within the hour.
void setHeavenTime(P_char victim)
{
	// Victim must be a real PC, but doesn't have to be alive.
	if (!victim || IS_NPC(victim))
		return;
	const int64_t now = time(nullptr);
	int time_in_heaven = 0;
	if (!gameplay_read_state_heaven_seconds(&victim->only.pc->gameplay_reads, now,
						GET_LEVEL(victim) + 20, &time_in_heaven))
	{
		logit(LOG_DEBUG,
		      "setHeavenTime: component=recent_pvp outcome=unavailable actor=redacted");
		return;
	}
	const size_t recent_deaths =
		gameplay_read_state_recent_count(&victim->only.pc->gameplay_reads, now);

	victim->only.pc->pc_timer[PC_TIMER_HEAVEN] = now + time_in_heaven;
	debug("actor=redacted time in heaven: %d seconds, recent deaths = %zu.", time_in_heaven,
	      recent_deaths);
}

static combat_outcome_participant *combat_outcome_add_participant(combat_outcome_payload *payload,
								  P_char ch,
								  combat_participant_role role,
								  bool in_room, bool leader)
{
	if (!payload || !ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return nullptr;
	for (size_t index = 0; index < payload->participant_count; ++index)
	{
		auto &entry = payload->participants[index];
		if (entry.pid != static_cast<uint32_t>(GET_PID(ch)))
			continue;
		if (role == combat_participant_role::killer ||
		    role == combat_participant_role::victim)
			entry.role = role;
		if (in_room)
			entry.flags |= COMBAT_PARTICIPANT_IN_ROOM;
		if (leader)
			entry.flags |= COMBAT_PARTICIPANT_LEADER;
		return &entry;
	}
	if (payload->participant_count >= payload->participants.size())
		return nullptr;
	auto &entry = payload->participants[payload->participant_count++];
	entry.pid = static_cast<uint32_t>(GET_PID(ch));
	entry.role = role;
	entry.flags = (in_room ? COMBAT_PARTICIPANT_IN_ROOM : 0) |
		      (leader ? COMBAT_PARTICIPANT_LEADER : 0);
	entry.level = static_cast<uint16_t>(std::max(0, GET_LEVEL(ch)));
	entry.racewar = static_cast<uint8_t>(GET_RACEWAR(ch));
	entry.expected_frag_revision = ch->only.pc->frag_revision;
	entry.expected_epic_revision = ch->only.pc->epic_revision;
	entry.expected_wallet_revision = ch->only.pc->wallet_revision;
	entry.expected_bank_revision = ch->only.pc->bank_revision;
	strlcpy(entry.account_name.data(), get_account_name_safe(ch), entry.account_name.size());
	strlcpy(entry.description.data(), GET_NAME(ch), entry.description.size());
	return &entry;
}

static void combat_outcome_committed(bool committed, const combat_outcome_result &, unsigned int,
				     const combat_outcome_payload &payload)
{
	P_char victim = find_player_by_pid(payload.victim_pid);
	if (victim && payload.gameplay_read_token)
		gameplay_read_state_finish_provisional(&victim->only.pc->gameplay_reads,
						       payload.gameplay_read_token, committed);
	if (!committed)
	{
		if (victim)
			send_to_char(
				"The PvP outcome could not be recorded; no rewards were applied.\r\n",
				victim);
		return;
	}
	for (size_t index = 0; index < payload.participant_count; ++index)
	{
		const auto &entry = payload.participants[index];
		P_char ch = find_player_by_pid(entry.pid);
		if (!ch || IS_NPC(ch))
			continue;
		if (entry.frag_delta > 0)
		{
			char buffer[512];
			snprintf(buffer, sizeof(buffer), "You just gained %.02f frags!\r\n",
				 static_cast<double>(entry.frag_delta) / 100.0);
			send_to_char(buffer, ch, LOG_PUBLIC);
			int recent = 0;
			for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
			{
				if (afp->type == TAG_PLR_RECENT_FRAG)
				{
					recent = afp->modifier;
					break;
				}
			}
			struct affected_type af = {};
			af.type = TAG_PLR_RECENT_FRAG;
			af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
			af.modifier = recent + static_cast<int>(entry.frag_delta);
			af.duration = get_property("epic.frag.thrill.duration", 45) * WAIT_SEC;
			affect_from_char(ch, TAG_PLR_RECENT_FRAG);
			affect_to_char(ch, &af);
			if (entry.epic_delta)
				epic_publish_pvp_award(ch, static_cast<int>(entry.epic_delta));
			if (entry.wallet_delta_copper)
			{
				snprintf(buffer, sizeof(buffer), "You get %s in blood money.\r\n",
					 coin_stringv(entry.wallet_delta_copper));
				send_to_char(buffer, ch);
			}
			if (entry.flags & COMBAT_PARTICIPANT_SPILL_BLOOD)
			{
				struct affected_type *task = get_epic_task(ch);
				if (task && abs(task->modifier) == SPILL_BLOOD)
					affect_remove(ch, task);
			}
			if (IS_ILLITHID(ch))
				illithid_advance_level(ch);
			if (victim)
			{
				check_boon_completion(ch, victim, entry.frag_delta / 100.0,
						      BOPT_FRAG);
				check_boon_completion(ch, victim, entry.frag_delta / 100.0,
						      BOPT_FRAGS);
			}
		}
		else if (entry.frag_delta < 0)
		{
			char buffer[512];
			snprintf(buffer, sizeof(buffer), "You just lost %.02f frags!\r\n",
				 static_cast<double>(-entry.frag_delta) / 100.0);
			send_to_char(buffer, ch);
			struct affected_type af = {};
			af.type = TAG_RECENTLY_FRAGGED;
			af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
			af.duration = 60 * WAIT_SEC;
			affect_to_char(ch, &af);
			if (entry.flags & COMBAT_PARTICIPANT_SPILL_BLOOD)
			{
				send_to_char("The &+yGods of Duris&n are very pleased with YOUR "
					     "&+Rblood&n, too!!!\r\n",
					     ch);
				send_to_char(
					"You can now progress further in your quest for epic power!\r\n",
					ch);
				struct affected_type *task = get_epic_task(ch);
				if (task && abs(task->modifier) == SPILL_BLOOD)
					affect_remove(ch, task);
			}
		}
	}
}

static bool submit_pvp_outcome(P_char ch, P_char victim, bool award_frags)
{
	if (!ch || !victim || IS_NPC(victim))
		return false;
	if (IS_NPC(ch))
	{
		if (!ch->following || IS_NPC(ch->following) ||
		    ch->in_room != ch->following->in_room || !grouped(ch, ch->following))
			return false;
		ch = ch->following;
	}
	combat_outcome_payload payload = {};
	payload.victim_pid = static_cast<uint32_t>(GET_PID(victim));
	payload.room_vnum = world[ch->in_room].number;
	strlcpy(payload.room_name.data(), world[ch->in_room].name, payload.room_name.size());
	if (!combat_outcome_add_participant(&payload, ch, combat_participant_role::killer, true,
					    ch->group && ch->group->ch == ch))
		return false;
	if (ch->group)
		for (struct group_list *gl = ch->group; gl; gl = gl->next)
			if (gl->ch != ch &&
			    !combat_outcome_add_participant(&payload, gl->ch,
							    combat_participant_role::killer_group,
							    gl->ch->in_room == ch->in_room, false))
				return false;
	if (!combat_outcome_add_participant(&payload, victim, combat_participant_role::victim, true,
					    victim->group && victim->group->ch == victim))
		return false;
	if (victim->group)
		for (struct group_list *gl = victim->group; gl; gl = gl->next)
			if (gl->ch != victim &&
			    !combat_outcome_add_participant(
				    &payload, gl->ch, combat_participant_role::victim_group,
				    gl->ch->in_room == victim->in_room, false))
				return false;
	if (award_frags)
	{
		int allies = 0;
		for (P_char current = world[ch->in_room].people; current;
		     current = current->next_in_room)
			if (IS_PC(current) && !opposite_racewar(ch, current) &&
			    !IS_TRUSTED(current))
				++allies;
		if (allies < 1)
			return false;
		float gain = 100.0F / allies;
		if (EVIL_RACE(ch) && GOOD_RACE(victim))
			gain *= get_property("frag.evil.penalty", 0.666);
		for (P_char current = world[ch->in_room].people; current;
		     current = current->next_in_room)
		{
			if (!IS_PC(current) || (current != ch && !grouped(ch, current)) ||
			    !fragWorthy(current, victim))
				continue;
			auto *entry = combat_outcome_add_participant(
				&payload, current,
				current == ch ? combat_participant_role::killer :
						combat_participant_role::killer_group,
				true, current->group && current->group->ch == current);
			if (!entry)
				return false;
			float real_gain = gain;
			if (GET_LEVEL(current) > GET_LEVEL(victim) + 5)
				real_gain *= get_property("frag.leveldiff.modifier.low", 0.500);
			else if (GET_LEVEL(current) + 5 < GET_LEVEL(victim))
				real_gain *= get_property("frag.leveldiff.modifier.high", 1.200);
			entry->frag_delta = static_cast<int64_t>(real_gain);
			int recent = 0;
			for (struct affected_type *afp = current->affected; afp; afp = afp->next)
				if (afp->type == TAG_PLR_RECENT_FRAG)
				{
					recent = afp->modifier;
					break;
				}
			if (real_gain + recent >= get_property("epic.frag.threshold", 0.10) * 100)
			{
				int epic_gain =
					static_cast<int>((real_gain / 100.0F) *
							 get_property("epic.frag.amount", 20.000));
				epic_gain += GET_FRAGS(victim);
				epic_gain = std::clamp(
					epic_gain, get_property("epic.frag.minimum.epics", 100),
					get_property("epic.frag.maximum.epics", 1500));
				struct affected_type *task = get_epic_task(current);
				if (task && abs(task->modifier) == SPILL_BLOOD)
				{
					epic_gain += 500;
					entry->flags |= COMBAT_PARTICIPANT_SPILL_BLOOD;
				}
				entry->epic_delta = epic_calculate_pvp_award(current, epic_gain);
			}
			if (GET_RACE(current) == RACE_HALFLING ||
			    GET_CLASS(current, CLASS_MERCENARY))
				entry->wallet_delta_copper =
					static_cast<int64_t>(10000 * real_gain);
		}
		float loss = gain;
		if (GET_LEVEL(ch) > GET_LEVEL(victim) + 5)
			loss *= get_property("frag.leveldiff.modifier.low", 0.500);
		else if (GET_LEVEL(ch) + 5 < GET_LEVEL(victim))
			loss *= get_property("frag.leveldiff.modifier.high", 1.200);
		auto *victim_entry = combat_outcome_add_participant(
			&payload, victim, combat_participant_role::victim, true,
			victim->group && victim->group->ch == victim);
		if (!victim_entry)
			return false;
		victim_entry->frag_delta = -static_cast<int64_t>(loss);
		struct affected_type *task = get_epic_task(victim);
		if (task && abs(task->modifier) == SPILL_BLOOD && loss > 0)
			victim_entry->flags |= COMBAT_PARTICIPANT_SPILL_BLOOD;
	}
	critical_operation_id operation_id = {};
	payload.gameplay_read_occurred_at = time(nullptr);
	payload.gameplay_read_token = gameplay_read_state_add_provisional(
		&victim->only.pc->gameplay_reads, payload.gameplay_read_occurred_at);
	if (!combat_outcome_transaction_submit(payload, combat_outcome_committed, &operation_id))
	{
		if (payload.gameplay_read_token)
			gameplay_read_state_finish_provisional(&victim->only.pc->gameplay_reads,
							       payload.gameplay_read_token, false);
		return false;
	}
	for (size_t index = 0; index < payload.participant_count; ++index)
	{
		const auto &entry = payload.participants[index];
		if (entry.epic_delta <= 0)
			continue;
		P_char participant = find_player_by_pid(entry.pid);
		if (!participant || IS_NPC(participant) ||
		    !artifact_guild_transaction_submit(participant, operation_id,
						       static_cast<int>(entry.epic_delta),
						       EPIC_PVP))
			logit(LOG_FILE,
			      "artifact_guild: component=combat_capture outcome=deferred_effect_unavailable actor=redacted");
	}
	return true;
}

void AddFrags(P_char ch, P_char victim)
{
	if (!submit_pvp_outcome(ch, victim, true))
		send_to_char("The PvP outcome service is busy; no rewards were applied.\r\n",
			     victim);
}

static void wake_death_extract_retry(P_char ch);

namespace
{
struct corpse_transfer_context
{
	uint64_t corpse_uid;
	uint64_t item_uid;
	uint64_t corpse_save_id;
};

P_obj corpse_live_item(uint64_t uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == uid)
			return object;
	return NULL;
}

static bool death_wallet_pending(P_char ch)
{
	return ch && IS_PC(ch) &&
	       (GET_COPPER(ch) || GET_SILVER(ch) || GET_GOLD(ch) || GET_PLATINUM(ch));
}

// A refused corpse handoff resubmits into the same refusal forever. The owner is
// recorded here so the death finalizes through the durable disposition instead.
bool corpse_transfer_disputed(P_char character)
{
	return character && IS_PC(character) && character->only.pc->death_custody_disputed;
}

void note_corpse_transfer_dispute(P_char character)
{
	if (character && IS_PC(character))
		character->only.pc->death_custody_disputed = true;
}

void clear_corpse_transfer_dispute(P_char character)
{
	if (character && IS_PC(character))
		character->only.pc->death_custody_disputed = false;
}

bool submit_next_corpse_item(P_char character, P_obj corpse);

void corpse_item_completion(P_char character, bool committed, const item_transfer_result &result,
			    unsigned int error_code, const uint8_t *encoded, size_t encoded_size)
{
	if (!character || !encoded || encoded_size != sizeof(corpse_transfer_context))
		return;
	corpse_transfer_context context = {};
	memcpy(&context, encoded, sizeof(context));
	P_obj corpse = corpse_live_item(context.corpse_uid);
	P_obj item = context.item_uid ? corpse_live_item(context.item_uid) : NULL;
	if (!committed)
	{
		// Resubmitting reproduces the refusal. The death is finalized through the
		// durable disposition, which preserves the refused payload and custody.
		note_corpse_transfer_dispute(character);
		persistence_alert(AVATAR, "corpse", "ownership_transfer", "none", "none",
				  "rejected_preserved", "item_uid=%llu error=%u disputed=1",
				  context.item_uid, error_code);
		return;
	}
	if (!corpse || (context.item_uid && (!item || !OBJ_CARRIED_BY(item, character))) ||
	    corpse->value[CORPSE_SAVEID] != static_cast<int>(context.corpse_save_id))
	{
		persistence_alert(AVATAR, "corpse", "ownership_publish", "none", "none",
				  "stale_live_topology", "item_uid=%llu", context.item_uid);
		return;
	}
	if (result.corpse_revision &&
	    !corpse_lifecycle_transaction_note_item_transfer(
		    static_cast<uint32_t>(corpse->value[CORPSE_PID]),
		    static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]), result.corpse_revision))
		persistence_alert(AVATAR, "corpse", "revision_publish", "none", "none",
				  "runtime_rejected", "save_id=%d", corpse->value[CORPSE_SAVEID]);
	if (item)
	{
		obj_from_char(item);
		obj_to_obj(item, corpse);
	}
	mark_player_dirty_components(GET_PID(character), PLAYER_COMPONENT_STATUS |
								 PLAYER_COMPONENT_EQUIPMENT |
								 PLAYER_COMPONENT_INVENTORY);
	// Batch publication has already installed every captured root. Legacy roots
	// without registry entries are adopted through the single-root path first.
	if (character->carrying)
		(void)submit_next_corpse_item(character, corpse);
	else
	{
		writeCorpse(corpse);
		(void)collector_catalog_cache_refresh();
		collector_death_enrollment_end(corpse);
		wake_death_extract_retry(character);
	}
}

bool submit_next_corpse_item(P_char character, P_obj corpse)
{
	if (!character || !corpse || !IS_PC(character) || !corpse->value[CORPSE_SAVEID])
		return false;
	const item_owner_identity source = { item_owner_type::player,
					     static_cast<uint64_t>(GET_PID(character)), 0 };
	std::vector<P_obj> roots;
	P_obj unregistered = NULL;
	item_ownership_runtime_entry runtime = {};
	for (P_obj candidate = character->carrying; candidate; candidate = candidate->next_content)
	{
		if (!candidate->obj_uid ||
		    (item_ownership_runtime_lookup(candidate->obj_uid, &runtime) &&
		     !item_owner_identity_equal(runtime.owner, source)))
		{
			note_corpse_transfer_dispute(character);
			return false;
		}
		if (!item_ownership_runtime_lookup(candidate->obj_uid, &runtime))
			unregistered = candidate;
		roots.push_back(candidate);
	}
	if (roots.empty())
	{
		writeCorpse(corpse);
		collector_death_enrollment_end(corpse);
		return true;
	}
	const collector_death_enrollment_resume_result collector_resume =
		collector_death_enrollment_resume(character, corpse);
	if (collector_resume == collector_death_enrollment_resume_result::invalid)
	{
		note_corpse_transfer_dispute(character);
		return false;
	}
	if (collector_resume == collector_death_enrollment_resume_result::unavailable)
	{
		persistence_report(persistence_severity::info, AVATAR, "collector", "death", "none",
				   "none", "death_enrollment_waiting_for_catalog", "save_id=%d",
				   corpse->value[CORPSE_SAVEID]);
		return false;
	}
	const item_owner_identity destination = {
		item_owner_type::corpse,
		item_corpse_owner_id(static_cast<uint32_t>(GET_PID(character)),
				     static_cast<uint32_t>(corpse->value[CORPSE_SAVEID])),
		0
	};
	const corpse_transfer_context context = {
		corpse->obj_uid, unregistered ? unregistered->obj_uid : 0,
		static_cast<uint64_t>(corpse->value[CORPSE_SAVEID])
	};
	item_movement_reject reject = item_movement_reject::none;
	const bool submitted = unregistered ?
				       item_movement_transaction_submit(
					       character, unregistered, NULL, source, destination,
					       item_transfer_reason::corpse_create,
					       corpse->value[CORPSE_SAVEID], corpse_item_completion,
					       &context, sizeof(context), corpse, &reject) :
				       item_movement_transaction_submit_batch(
					       character, roots.data(), roots.size(), NULL, source,
					       destination, item_transfer_reason::corpse_create,
					       corpse->value[CORPSE_SAVEID], corpse_item_completion,
					       &context, sizeof(context), corpse, &reject);
	if (!submitted)
	{
		if (!item_movement_reject_is_transient(reject))
			note_corpse_transfer_dispute(character);
		persistence_alert(AVATAR, "corpse", "ownership_submit", "none", "none",
				  "failed_preserved", "roots=%zu reason=%d", roots.size(),
				  static_cast<int>(reject));
		return false;
	}
	return true;
}
}

P_obj make_corpse(P_char ch, int loss)
{
	P_obj corpse, o;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int e_time;

	corpse = read_object(2, VIRTUAL);
	if (!corpse)
	{
		logit(LOG_EXIT, "make_corpse: no valid corpse object found");
		return NULL;
	}

	corpse->str_mask = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);

	if (IS_PC(ch))
		snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), "corpse _pcorpse_");
	else
		snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), "corpse _npcorpse_");

	corpse->name = str_dup(buf);

	if (IS_PC(ch))
	{
		snprintf(buf2, MAX_STRING_LENGTH, "%s %s",
			 index("AEIOU", race_names_table[ch->player.race].normal[0]) == NULL ? "a" :
											       "an",
			 race_names_table[ch->player.race].normal);
	}
	checked_snprintf(buf, MAX_STRING_LENGTH, "The corpse of %s is lying here.",
			 IS_PC(ch) ? buf2 : ch->player.short_descr);
	DECAP(buf + 14);

	corpse->description = str_dup(buf);

	snprintf(buf, MAX_STRING_LENGTH, "the corpse of %s",
		 IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
	corpse->short_description = str_dup(buf);

	/*
	 * for animate dead and resurrect
	 */
	snprintf(buf, MAX_STRING_LENGTH, "%s", IS_NPC(ch) ? ch->player.short_descr : GET_NAME(ch));
	corpse->action_description = str_dup(buf);

	/*
	 * changed this, add everything to ch's inven before xferring to
	 * corpse, this makes the corpse weight correct.  (also simplifies
	 * things.)
	 */

	// An admission fence leaves the authoritative wallet intact. The death retry
	// converts it after that fence drains, then resumes normal corpse custody.
	if (!IS_TRUSTED(ch))
		(void)money_to_inventory(ch);

	corpse->value[CORPSE_LEVEL] = GET_LEVEL(ch); /* for animate dead */

	if (GET_LEVEL(ch) > 1)
		corpse->value[CORPSE_EXP_LOSS] = -loss;

	/*
	 * have to change the 'loc.carrying' pointers to 'loc.inside' pointers
	 * for the whole object list, else ugly problems occur later.
	 */
	hold_durable_pet_items(ch);
	unequip_all(ch);
	if (IS_NPC(ch))
	{
		corpse->contains = ch->carrying;
		ch->carrying = NULL;
	}

	for (o = corpse->contains; o; o = o->next_content)
	{
		o->loc_p = LOC_INSIDE;
		o->loc.inside = corpse;
		if (IS_ARTIFACT(o) && IS_PC(ch))
			if (!remove_owned_artifact_sql(o, GET_PID(ch)))
				wizlog(56, "couldn't unflag arti after %s died", GET_NAME(ch));
	}

	if (IS_NPC(ch))
	{
		e_time = get_property("timer.decay.corpse.npc", 120) * WAIT_MIN;
		corpse->weight = total_carried_weight(ch) * 2;
		corpse->value[CORPSE_WEIGHT] = total_carried_weight(ch);
		corpse->value[CORPSE_FLAGS] = NPC_CORPSE;
		if (ch->only.npc)
			corpse->value[CORPSE_VNUM] = mob_index[GET_RNUM(ch)].virtual_number;
		else
			corpse->value[CORPSE_VNUM] = 0;
		corpse->value[CORPSE_RACEWAR] = 0;
	}
	else
	{
		e_time = difficulty_pc_corpse_decay_minutes() * WAIT_MIN;
		corpse->weight = GET_WEIGHT(ch);
		corpse->value[CORPSE_WEIGHT] = 0;
		corpse->value[CORPSE_FLAGS] = PC_CORPSE;
		corpse->value[CORPSE_PID] = GET_PID(ch);

		if (IS_RACEWAR_UNDEAD(ch))
			corpse->value[CORPSE_RACEWAR] = 3;
		else if (EVIL_RACE(ch))
			corpse->value[CORPSE_RACEWAR] = 2;
		else
			corpse->value[CORPSE_RACEWAR] = 1;

		/* value[6] is reserved for saved file id - Tharkun */
		corpse->value[CORPSE_SAVEID] = static_cast<int>(time(NULL));

		if (IS_HUMANOID(ch))
			corpse->value[CORPSE_FLAGS] |= HUMANOID_CORPSE; /* for carving */
	}

	account_bound_reward_prepare_player_corpse(ch, corpse);
	if (IS_PC(ch))
	{
		int contents_weight = total_carried_weight(ch);
		corpse->value[CORPSE_WEIGHT] = contents_weight > 0 ? contents_weight : 0;
	}

	corpse->value[CORPSE_RACE] = GET_RACE(ch);

	if (IS_NPC(ch))
	{
		IS_CARRYING_N(ch) = 0;
		GET_CARRYING_W(ch) = 0;
	}

	set_obj_affected(corpse, e_time, TAG_OBJ_DECAY, 0);

	if (ch->in_room == NOWHERE)
	{
		if (real_room(ch->specials.was_in_room) != NOWHERE)
			obj_to_room(corpse, real_room(ch->specials.was_in_room));
		else
		{
			// No place sane to put it
			obj_to_room(corpse, real_room(CORPSE_STORAGE_II));
		}
	}
	else
	{
		obj_to_room(corpse, ch->in_room);
	}
	/*
	 * added by DTS 8/1/95 - ghosts and wraiths shouldn't leave corpses...
	 * just dump contents of corpse to room and extract corpse
	 */
	if (IS_NPC(ch) && IS_NOCORPSE(ch))
	{
		if (corpse->contains)
		{
			while (corpse->contains)
			{
				o = corpse->contains;
				obj_from_obj(o);
				// Should always be true, but just in case.
				if (OBJ_ROOM(corpse))
				{
					obj_to_room(o, corpse->loc.room);
				}
				else
				{
					extract_obj(
						o,
						TRUE); // Yep, if the arti doesn't have a place to go..
				}
			}
		}

		switch (GET_RACE(ch))
		{
		case RACE_GHOST:
			act("$n dissolves into thin air.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_F_ELEMENTAL:
		case RACE_EFREET:
			act("$n disappears in a burst of fire.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_W_ELEMENTAL:
			act("$n sinks into the ground.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_A_ELEMENTAL:
			act("$n vanishes into thin air.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_E_ELEMENTAL:
			act("$n crumbles to dust.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_UNDEAD:
		case RACE_LICH:
		case RACE_VAMPIRE:
			act("$n crumbles to dust.", TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_SKELETON:
			act("$n falls to the ground, leaving a pile of inanimate bones.", TRUE, ch,
			    0, 0, TO_ROOM);
			break;
		case RACE_ZOMBIE:
			act("$n shambles for a moment, and falls to pieces.", TRUE, ch, 0, 0,
			    TO_ROOM);
			break;
		case RACE_WRAITH:
		case RACE_SPECTRE:
			act("$n howls loudly as $e dissipates into the wind.", TRUE, ch, 0, 0,
			    TO_ROOM);
			break;
		case RACE_SHADOW:
			act("$n explodes into all corners of the room, covering it in darkness!",
			    TRUE, ch, 0, 0, TO_ROOM);
			spell_darkness(GET_LEVEL(ch), ch, 0, 0, 0, 0);
			break;
		case RACE_DEVA:
			act("$n explodes into all corners of the room, covering it in pure light!",
			    TRUE, ch, 0, 0, TO_ROOM);
			spell_continual_light(GET_LEVEL(ch), ch, 0, 0, 0, 0);
			break;
		case RACE_V_ELEMENTAL:
			act("$n howls, and returns to the void which spawned it.", TRUE, ch, 0, 0,
			    TO_ROOM);
			break;
		case RACE_I_ELEMENTAL:
			act("A blank look overcomes $n's face for a moment, before $e shatters into tiny fragments!",
			    TRUE, ch, 0, 0, TO_ROOM);
			break;
		case RACE_ARCHON:
		case RACE_ASURA:
		case RACE_TITAN:
		case RACE_AVATAR:
		case RACE_GHAELE:
		case RACE_BRALANI:
		case RACE_ELADRIN:
			act("$n &+Wglows white&n &+wbefore &+Ldisappearing...&n", TRUE, ch, 0, 0,
			    TO_ROOM);
			break;
		default:
			act("$n &+gquickly returns to the plane from whence they were summoned...&n",
			    TRUE, ch, 0, 0, TO_ROOM);
			break;
		}

		obj_from_room(corpse);
		extract_obj(corpse);
		corpse = NULL;
	}
	if (corpse && IS_PC(ch))
	{
		collector_death_enrollment_begin(ch, corpse);
		mark_player_dirty_components(GET_PID(ch), PLAYER_COMPONENT_STATUS |
								  PLAYER_COMPONENT_EQUIPMENT |
								  PLAYER_COMPONENT_INVENTORY);
		writeCorpse(corpse);
		// A movement already in flight for this player makes the first corpse
		// transfer conflict, and a refusal here reads as failed_preserved even
		// though nothing was lost. die() defers the death while the pipeline is
		// busy and the recovery event restarts the chain once it drains.
		if (!item_movement_transaction_player_busy(ch) &&
		    !currency_transaction_player_busy(ch))
			(void)submit_next_corpse_item(ch, corpse);
	}

	return corpse;
}

/*
 * When ch kills victim
 */
void change_alignment(P_char ch, P_char victim)
{
	int a_al, v_al, change = 0;

	if (CHAR_IN_ARENA(ch) || CHAR_IN_ARENA(victim) || IS_NPC(ch))
		return;

	a_al = GET_ALIGNMENT(ch);
	v_al = GET_ALIGNMENT(victim);

	/*
	 * global modifiers
	 */

	if (IS_NPC(victim))
	{
		if (IS_SET(victim->only.npc->aggro_flags, AGGR_ALL))
			v_al -= 100;

		if (IS_SET(victim->only.npc->aggro_flags, AGGR_GOOD_ALIGN))
		{
			if (IS_GOOD(ch))
				v_al -= 125;
			else
				v_al -= 50;
		}

		if (IS_SET(victim->only.npc->aggro_flags, AGGR_NEUTRAL_ALIGN))
		{
			if (IS_NEUTRAL(ch))
				v_al -= 25;
			else
				v_al -= 50;
		}

		if (IS_SET(victim->only.npc->aggro_flags, AGGR_EVIL_ALIGN))
		{
			if (IS_EVIL(ch))
				v_al += 125;
			else
				v_al += 75;
		}

		if (IS_DEMON(victim))
			change += 2;

		if (IS_SET(victim->specials.act, ACT_NICE_THIEF))
			v_al += 25;

		if (IS_SET(victim->specials.act, ACT_STAY_ZONE))
			v_al += 25;

		if (IS_SET(victim->specials.act, ACT_WIMPY))
			v_al += 10;
	}
	else
	{
		if (victim->only.pc->aggressive > -1)
			v_al -= 100;

		if (GET_SPEC(victim, CLASS_ROGUE, SPEC_ASSASSIN) || GET_CLASS(ch, CLASS_ASSASSIN))
			change += 2;

		if (GET_CLASS(victim, CLASS_ANTIPALADIN))
			change += 5;

		if (GET_CLASS(victim, CLASS_PALADIN))
			change -= 5;

		if (is_linked_to(ch, victim, LNK_CONSENT))
			change -= 10;
	}

	/*
	 * at this point v_al will range from -1275 to 1135 depending on
	 * various flags and such, this will allow for the possibility of
	 * alignment shifts even when ch is -1000 or 1000, without having to
	 * pick nits.
	 */

	/*
	 * ok, 'good' alignment is the most vulnerable to change, neutral not
	 * nearly so, and evil is very tough to change (except to become more
	 * evil)
	 */

	if (a_al > 350)
	{
		/*
		 * ch is 'good'
		 */

		if (v_al > 350)
		{
			/*
			 * victim is also 'good', this is by far the most drastic
			 * shift
			 */

			if (v_al > a_al)
			{
				/*
				 * victim is 'more' good
				 */
				change += ((a_al - v_al) / 40 - 9); /*
				                                     * -28 to -9
				                                     */
			}
			else
			{
				/*
				 * victim is 'less' good, but still good
				 */
				change += ((v_al - a_al) / 50 - 6); /*
				                                     * -18 to -6
				                                     */
			}
		}
		else if (v_al > -351)
		{
			/*
			 * victim is neutral, not near as bad
			 */

			change += (v_al / -80); /*
			                         * -4 to 4
			                         */
		}
		else
		{
			/*
			 * victim is evil, adds to align, but not alot
			 */

			change += (v_al / -150); /*
			                          * 8 to 2
			                          */
		}
	}
	else if (a_al > -351)
	{
		/*
		 * ch is 'neutral'
		 */

		/*
		 * neutral chars don't change much for killing either extreme,
		 * instead, killing more 'moderate' alignments has the largest
		 * effect
		 */

		if (v_al > 0)
			change -= ((1135 - v_al) / 100); /*
			                                  * 0 to -11
			                                  */
		else
			change += ((1275 + v_al) / 100); /*
			                                  * 12 to 0
			                                  */
	}
	else
	{
		/*
		 * ch is 'evil'
		 */

		/*
		 * almost ANYTHING an evil kills can be regarded as an evil act.
		 * The only exception is killing demons, and killing things MORE
		 * evil than they are currently, but not a lot in any case.
		 */

		if (v_al < a_al)
		{
			/*
			 * victim is more evil, very small shift towards neutral
			 */

			change += ((v_al - a_al) / -200); /*
			                                   * 4 to 0
			                                   */
		}
		else
		{
			/*
			 * victim is 'less' evil
			 */

			change += ((v_al - a_al) / -100); /*
			                                   * -21 to 0
			                                   */
		}
	}

	/*
	 * unless I really screwed something up, change now ranges for -36 to
	 * 20, since we want to minimize alignment shifts, we are going to use
	 * those as a basis for 'chance of shift'.  Max actual change will be
	 * -4 to 2, and it's going to take concerted effort over a LONG time
	 * to change it much at all.
	 */

	/*
	 * final mod, nothing tougher to change than 'old' evil and nothing
	 * faster than a fall from grace
	 */

	if (IS_EVIL(ch) && (change > 1))
		change = BOUNDED(1, change - (GET_LEVEL(ch) / 15), change);
	if (IS_GOOD(ch) && (change < 0))
		change -= (GET_LEVEL(ch) / 10);

	if (EVIL_RACE(ch) && GOOD_RACE(victim))
	{ // Lets try to keep evil race folks
		change += (-number(50, 500)); // from having a free ride when they
	} // obtain good alignment

	/*
	 * The Druid balance will flux a lot.  However, it will balance itself
	 * as druids commune in forests.
	 */

	if (GET_CLASS(ch, CLASS_DRUID) ||
	    (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
	{
		if (world[ch->in_room].sector_type != SECT_FOREST)
			change *= 2;
	}

	if (change < 0)
	{
		while (change < 0)
		{
			a_al -= (number(0, 9) < MIN(10, -(change)));
			change += 7;
		}
	}
	else
	{
		while (change > 0)
		{
			a_al += (number(0, 9) < MIN(10, change));
			change -= 10;
		}
	}

	GET_ALIGNMENT(ch) = BOUNDED(-1000, a_al, 1000);
}

/*
 * this routine was repeated all over, made it a function, basically
 * handles switched gods and shapechangers, so that the right body dies.
 * Returns the right char, or NULL, if there is a problem.  JAB
 */
P_char ForceReturn(P_char ch)
{
	P_char true_id, t_ch = ch;
	int is_avatar = FALSE, virt;

	if (!t_ch)
		return NULL;

	if (IS_AFFECTED(ch, AFF_WRAITHFORM))
	{
		BackToUsualForm(ch);
		return ch;
	}
	/*
	 * morph'ed players go back...
	 */
	if (IS_MORPH(t_ch))
	{
		virt = mob_index[GET_RNUM(t_ch)].virtual_number;
		if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
			is_avatar = TRUE;
		true_id = MORPH_ORIG(t_ch);
		if (!is_avatar)
		{
			act("$n's dying body slowly changes back into $N.", FALSE, t_ch, 0, true_id,
			    TO_ROOM);
			send_to_char(
				"As the last of life leaves you, your body resumes its natural form.\r\n",
				t_ch);
		}
		else
		{
			act("$n has been vanquished!", FALSE, t_ch, 0, 0, TO_ROOM);
			send_to_char(
				"As the last of life leaves your form, you return to your normal body.\r\n",
				t_ch);
		}
		if (GET_OPPONENT(t_ch))
			stop_fighting(t_ch);
		un_morph(t_ch);
		return true_id;
	}

	if (ch && IS_PC(ch) && ch->only.pc->switched && IS_MORPH(ch->only.pc->switched))
	{
		t_ch = ch->only.pc->switched;
		send_to_char(
			"&+RYour mind can no longer hold this form as your true body has been destroyed!!&n\r\n",
			t_ch);
		act("&+BThe soul holding $n &+Bto this realm has perished!\r\n$n&+B vanishes.",
		    FALSE, t_ch, 0, 0, TO_ROOM);
		if (GET_OPPONENT(t_ch))
			stop_fighting(t_ch);
		un_morph(t_ch);
		return ch;
	}

	/*
	 * switched god
	 */

	if (t_ch->desc && t_ch->desc->original && IS_PC(t_ch->desc->original) &&
	    t_ch->desc->original->only.pc->switched)
	{
		do_return(t_ch, 0, CMD_DEATH);
	}
	return (t_ch);
}

/* These two functions are no longer used.  No idea when they went out of style.
 *   Discovered them on 6/29/2015 while moving arti code from file-based to DB-based.
void update_all_arti_blood(P_char ch, int mod)
{
  int      i, count;
  char     tmp_buf[MAX_STRING_LENGTH];
  P_obj    obj;
  P_char tch;

  //dont feed if misfire check active.

  for( tch = world[ch->in_room].people; tch; tch = tch->next_in_room )
  {
    if( (ch == tch || grouped(tch, ch)) && affected_by_spell(tch, TAG_NOMISFIRE) )
    {
      break;
    }
  }

  if( !tch )
  {
    send_to_char("&+RYour artifact(s) do not seem happy with this kind of blood.&n\r\n", ch);
    return;
  }

  count = 0;
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
    {
      if( IS_ARTIFACT(ch->equipment[i]) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
      {
        count++;
      }
    }
  }
  P_obj tobj = ch->carrying;
  while (tobj)
  {
    if( IS_ARTIFACT(ch->equipment[i]) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
      count++;
    tobj = tobj->next_content;
  }

  for (i = 0; i < MAX_WEAR; i++)
  {
    obj = ch->equipment[i];
    if( obj )
    {
      if( (IS_ARTIFACT(ch->equipment[i]) && isname("unique", obj->name)) || CAN_WEAR(obj, ITEM_WEAR_IOUN) )
      {
        UpdateArtiBlood(ch, ch->equipment[i], mod);
      }
      else if( IS_ARTIFACT(ch->equipment[i]) )
      {
        UpdateArtiBlood(ch, ch->equipment[i], (int)(mod/count));
      }
    }
  }
}

void perform_arti_update(P_char ch, P_char victim)
{
  float    frags;

  // Check ch's group, see if any members' artis feed

  if (ch->group)
  {
    struct group_list *gl;

    gl = ch->group;

    frags = gl->ch->only.pc->frags - gl->ch->only.pc->oldfrags;

    while (gl)
    {
      if (IS_PC(gl->ch) && (gl->ch->in_room == ch->in_room) && fragWorthy(gl->ch, victim))
      {
        update_all_arti_blood(gl->ch, gl->ch->only.pc->frags - gl->ch->only.pc->oldfrags);
      }

      gl = gl->next;
    }
  }
  else
  {
    if (IS_PC(ch) && fragWorthy(ch, victim) && ((ch->only.pc->frags) > (ch->only.pc->oldfrags)))
      update_all_arti_blood(ch, ch->only.pc->frags - ch->only.pc->oldfrags);
  }
}
*/

void kill_gain(P_char ch, P_char victim);
/*
 * Death recovery: die() refuses to extract a character whose terminal save failed,
 * to protect the live state that has not reached the database. Without a retry the
 * player is stranded in STAT_DEAD - every command blocked by "Lie still; you are
 * DEAD!!!" - while still being a live target in the room. These two functions
 * re-attempt the save and finish the death the moment it lands.
 *
 * The same deferral covers make_corpse()'s asynchronous ownership handoff.
 * submit_next_corpse_item() moves registered corpse roots in one transaction
 * and each completion is only published while the owner is still live, so
 * extracting the character mid-chain stranded the remaining items as active
 * rows in item_current_owner while the terminal save wrote an empty
 * player_items.  The next login then failed the item custody invariant with
 * "Sorry, I couldn't load that character!".  Waiting for the chain to drain
 * keeps the database row set and the saved payload in agreement.
 */
#define DEATH_EXTRACT_RETRY_INITIAL 4
#define DEATH_EXTRACT_RETRY_MAX 60
// The death recovery budget: a refused handoff must reach durable storage and
// release the character to the account menu inside this window.
#define DEATH_DISPOSITION_TIMEOUT_MSEC 2000
// How long a custody handoff may drain before the wait stops being routine.
// DEATH_EXTRACT_RETRY_INITIAL is a delay in PULSES and WAIT_SEC of them make a
// second, so the poll runs about once a second: this is a wait of half a
// minute, an order of magnitude past any healthy drain, and the point at which
// an immortal wants to hear about it.
#define DEATH_RECOVERY_STALL_SECONDS 30

/*
 * Should this poll tell the immortals about the wait?
 *
 * ELAPSED TIME, NOT POLLS. The retry is scheduled a second at a time, but
 * ne_events() runs an entry whose tick is due OR LATE, and a main loop held up
 * by blocking work is exactly the condition this alert exists to expose:
 * counting callbacks would report one second of waiting after a thirty-second
 * stall and say nothing at all. So the wait is measured against a monotonic
 * clock taken when it started, and the poll count is diagnostic only.
 *
 * `alerts` is how many have already gone out for THIS wait, which is what
 * turns the answer into one line per window instead of one per poll: the next
 * is due once the wait reaches the window after the last.
 *
 * Its own function rather than an expression inline so the contract test can
 * compile THIS arithmetic instead of a copy of it -- a lifted copy cannot
 * notice the original drifting away from it, and neither can a test that only
 * reads the comparison.
 */
static bool death_custody_wait_should_alert(uint64_t waited_usec, int alerts)
{
	if (alerts < 0)
		return false;

	const uint64_t window_usec = (uint64_t)DEATH_RECOVERY_STALL_SECONDS * 1000000;

	// A wait long enough to overflow this is a wait no clock will see.
	if ((uint64_t)alerts + 1 > UINT64_MAX / window_usec)
		return false;

	return waited_usec >= window_usec * ((uint64_t)alerts + 1);
}

/** Forget the wait: whatever happens next is not the handoff that started it. */
static void death_custody_wait_reset(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;

	ch->only.pc->death_custody_wait_since_usec = 0;
	ch->only.pc->death_custody_wait_alerts = 0;
	ch->only.pc->death_custody_wait_polls = 0;
}

/** Finish a death whose record is durable: report it, then release to the account menu. */
static void release_after_terminal_death(P_char ch, const char *outcome)
{
	persistence_report(persistence_severity::ok, AVATAR, "player_save", "death", "none", "none",
			   outcome, "extract_refused=0");
	send_to_char("Your death has been recorded; the world lets go of you.\r\n", ch);
	ch->only.pc->pc_timer[1] = 0; // reset flee timer
	add_track(ch, NUM_EXITS);
	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
		update_ingame_racewar(-GET_RACEWAR(ch));
	extract_char_after_terminal_save(ch);
}

/** Record the refused death disposition durably; false keeps live state for a retry. */
static bool save_disputed_death_disposition(P_char ch, uint64_t corpse_uid)
{
	if (!ch)
		return false;
	// Resume the sealed request before allocating a new operation ID or wallet
	// object; retries must not change the revision or item identities.
	const player_save_terminal_result resumed = player_save_pipeline_terminal_death_resume(
		ch, corpse_uid, DEATH_DISPOSITION_TIMEOUT_MSEC);
	if (resumed != player_save_terminal_result::not_pending)
		return resumed == player_save_terminal_result::database_acknowledged;
	P_obj corpse = corpse_uid ? corpse_live_item(corpse_uid) : NULL;
	critical_operation_id operation = {};
	if (!corpse || !critical_operation_id_generate(&operation))
		return false;
	P_obj wallet_pile = NULL;
	if (GET_COPPER(ch) || GET_SILVER(ch) || GET_GOLD(ch) || GET_PLATINUM(ch))
	{
		// A wallet still holding coins means its conversion never committed. The
		// disposition owes the player that wallet instead of dropping it.
		wallet_pile = create_money(GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch),
					   GET_PLATINUM(ch));
		if (!wallet_pile)
			return false;
	}
	// A journaled death can still be rejected by the database's custody checks.
	// Keep the character in its private recovery hold until MariaDB acknowledges
	// the disposition; otherwise reconnect could race an unapplied death.
	const player_save_terminal_result saved = player_save_pipeline_terminal_death(
		ch, corpse, wallet_pile, operation,
		calculate_save_room(ch, RENT_DEATH, ch->in_room), DEATH_DISPOSITION_TIMEOUT_MSEC,
		false);
	if (wallet_pile)
		extract_obj(wallet_pile, FALSE);
	const bool durable = saved == player_save_terminal_result::database_acknowledged;
	persistence_report(durable ? persistence_severity::ok : persistence_severity::alert, AVATAR,
			   "player_save", "death", "none", "none",
			   durable ? "death_disposition_recorded" : "death_disposition_failed",
			   "outcome=%u wallet=%d", (unsigned)saved, wallet_pile ? 1 : 0);
	return durable;
}

struct death_extract_retry_context
{
	int delay;
	uint64_t corpse_uid;
};

static void event_death_extract_retry(P_char ch, P_char victim, P_obj obj, void *data);
static void hold_for_death_extract_retry(P_char ch);
static bool death_retry_fallback_pending = false;

static void schedule_death_extract_retry(P_char ch, uint64_t corpse_uid, int delay)
{
	if (!ch || IS_NPC(ch) || !GET_NAME(ch))
		return;

	if (delay < DEATH_EXTRACT_RETRY_INITIAL)
		delay = DEATH_EXTRACT_RETRY_INITIAL;
	if (delay > DEATH_EXTRACT_RETRY_MAX)
		delay = DEATH_EXTRACT_RETRY_MAX;
	ch->only.pc->death_retry_corpse_uid = corpse_uid;
	ch->only.pc->death_retry_delay = delay;

	// add_event() rejects dead character owners. Briefly expose a live state while
	// linking the private recovery event, then restore the pending death before
	// returning to the game loop.
	const death_extract_retry_context context = { delay, corpse_uid };
	GET_HIT(ch) = 1;
	SET_POS(ch, GET_POS(ch) + STAT_NORMAL);
	const nevent_schedule_result scheduled = add_event(
		event_death_extract_retry, delay, ch, NULL, NULL, 0, &context, sizeof(context));
	hold_for_death_extract_retry(ch);
	ch->only.pc->death_retry_due_usec = 0;
	if (!scheduled)
	{
		// Retain the retry on the character without another allocation. The game
		// pulse can retry a refused event without releasing unsaved live assets.
		ch->only.pc->death_retry_due_usec =
			persistence_observability_now_usec() +
			static_cast<uint64_t>(delay) * 1000000 / WAIT_SEC;
		death_retry_fallback_pending = true;
		persistence_alert(AVATAR, "player_save", "death", "none", "none",
				  "death_recovery_schedule_failed", "delay=%d", delay);
	}
}

/** Run the existing guarded finalizer on the next pulse after publication. */
static void wake_death_extract_retry(P_char ch)
{
	if (!ch || !IS_PC(ch) || GET_STAT(ch) != STAT_DEAD)
		return;
	// Reschedule the existing event, preserving its corpse identity and avoiding
	// extraction inside the coordinator's completion dispatch.
	if (P_nevent event = get_scheduled(ch, event_death_extract_retry))
		(void)nevent_reschedule_after(nevent_handle_from_event(event), 0);
	if (ch->only.pc->death_retry_due_usec)
		ch->only.pc->death_retry_due_usec = persistence_observability_now_usec();
}

/** Retry failed event admission from the game thread without extracting unsaved state. */
void death_extract_retry_pulse(void)
{
	if (!death_retry_fallback_pending)
		return;
	death_retry_fallback_pending = false;
	const uint64_t now = persistence_observability_now_usec();
	for (P_char ch = character_list, next; ch; ch = next)
	{
		next = ch->next;
		if (!IS_PC(ch) || !ch->only.pc->death_retry_due_usec)
			continue;
		if (ch->only.pc->death_retry_due_usec > now)
		{
			death_retry_fallback_pending = true;
			continue;
		}
		death_extract_retry_context context = { ch->only.pc->death_retry_delay,
							ch->only.pc->death_retry_corpse_uid };
		ch->only.pc->death_retry_due_usec = 0;
		event_death_extract_retry(ch, NULL, NULL, &context);
	}
}

static void hold_for_death_extract_retry(P_char ch)
{
	GET_HIT(ch) = 1;
	// The real corpse already represents this death in the room. Keep the
	// fail-closed player state alive for persistence recovery, but do not leave a
	// second, lootable-looking body in the world while the terminal save retries.
	if (ch->in_room != NOWHERE)
		char_from_room(ch);
	SET_POS(ch, GET_POS(ch) + STAT_DEAD);
}

static void event_death_extract_retry(P_char ch, P_char victim, P_obj obj, void *data)
{
	const death_extract_retry_context context =
		data ? *((death_extract_retry_context *)data) :
		       death_extract_retry_context{ DEATH_EXTRACT_RETRY_INITIAL, 0 };
	const int previous_delay = context.delay;

	(void)victim;
	(void)obj;

	if (!ch || IS_NPC(ch) || !GET_NAME(ch) || !ch->only.pc)
		return;
	ch->only.pc->death_retry_corpse_uid = 0;
	ch->only.pc->death_retry_delay = 0;
	ch->only.pc->death_retry_due_usec = 0;

	if (ch->in_room != NOWHERE && (CHAR_IN_ARENA(ch) || GET_STAT(ch) != STAT_DEAD))
	{
		clear_corpse_transfer_dispute(ch);
		persistence_alert(AVATAR, "player_save", "death", "none", "none",
				  "death_recovery_abandoned", "stat=%d room=%d", GET_STAT(ch),
				  ch->in_room);
		return;
	}
	// update_pos() derives a sleeping state from the retained 1 HP between
	// pulses. NOWHERE is the private recovery hold, so restore its dead marker;
	// a genuinely resumed character has been placed back in a room and took the
	// abandonment branch above.
	if (GET_STAT(ch) != STAT_DEAD)
		hold_for_death_extract_retry(ch);

	const bool items_busy = item_movement_transaction_player_busy(ch);
	const bool currency_busy = currency_transaction_player_busy(ch);

	if (items_busy || currency_busy)
	{
		// Corpse ownership handoffs are expected bounded work, not a failure.
		// Poll them steadily so the account menu follows the final handoff
		// promptly; reserve exponential backoff for an actual save failure.
		//
		// THE POLL ITSELF IS NOT NEWS (issue #174). The old single-item
		// transfer chain spent multiple polls draining, each broadcasting
		// at AVATAR to every immortal
		// online -- a death that was draining correctly read as a database
		// stall. The routine wait belongs in the log. The channel hears about
		// it only once the wait is long enough that someone should look, and
		// then once per window rather than once per poll.
		//
		// It also said "corpse_items" for a condition that is equally true of
		// a wallet conversion still in flight, which sent readers looking in
		// the wrong subsystem. Both are named now, and the wait is reported in
		// seconds: the delay is in PULSES, and reading it as seconds is what
		// turned a 13-poll drain into a "52 second" report.
		const uint64_t now = persistence_observability_now_usec();

		if (!ch->only.pc->death_custody_wait_since_usec)
			ch->only.pc->death_custody_wait_since_usec = now;

		const uint64_t since = ch->only.pc->death_custody_wait_since_usec;
		const uint64_t waited_usec = now > since ? now - since : 0;
		const uint64_t waited_whole = waited_usec / 1000000;
		const int waited_sec = waited_whole > INT_MAX ? INT_MAX : (int)waited_whole;
		const int polls = ++ch->only.pc->death_custody_wait_polls;

		if (death_custody_wait_should_alert(waited_usec,
						    ch->only.pc->death_custody_wait_alerts))
		{
			ch->only.pc->death_custody_wait_alerts++;
			persistence_alert(AVATAR, "player_save", "death", "none", "none",
					  "death_recovery_awaiting_custody",
					  "items=%d currency=%d polls=%d waited_sec=%d delay=%d",
					  items_busy ? 1 : 0, currency_busy ? 1 : 0, polls,
					  waited_sec, DEATH_EXTRACT_RETRY_INITIAL);
		}
		else
			persistence_report(persistence_severity::info, AVATAR, "player_save",
					   "death", "none", "none",
					   "death_recovery_awaiting_custody",
					   "items=%d currency=%d polls=%d "
					   "waited_sec=%d delay=%d",
					   items_busy ? 1 : 0, currency_busy ? 1 : 0, polls,
					   waited_sec, DEATH_EXTRACT_RETRY_INITIAL);

		schedule_death_extract_retry(ch, context.corpse_uid, DEATH_EXTRACT_RETRY_INITIAL);
		return;
	}

	// Past the wait: a dispute, a restart, a save or a release. None of them is
	// the handoff whose clock is running, so the next one starts its own.
	death_custody_wait_reset(ch);

	P_obj corpse = context.corpse_uid ? corpse_live_item(context.corpse_uid) : NULL;
	if ((!IS_TRUSTED(ch) || corpse_transfer_disputed(ch)) && death_wallet_pending(ch))
	{
		// A deferred starter-kit admission can transiently fence the player before
		// money_to_inventory() gets a chance to submit. Give the normal currency
		// transaction another admission attempt after the fence drains. It owns the
		// wallet revision/ledger and publishes the zero wallet before the death
		// snapshot captures any money object, so fallback evidence cannot duplicate
		// an uncleared authoritative wallet.
		const bool submitted = money_to_inventory(ch);
		const persistence_severity wallet_severity =
			submitted ? persistence_severity::info : persistence_severity::alert;
		persistence_report(wallet_severity, AVATAR, "player_save", "death", "none", "none",
				   "death_recovery_restarting_wallet", "submitted=%d delay=%d",
				   submitted ? 1 : 0,
				   submitted ? DEATH_EXTRACT_RETRY_INITIAL : previous_delay * 2);
		schedule_death_extract_retry(ch, context.corpse_uid,
					     submitted ? DEATH_EXTRACT_RETRY_INITIAL :
							 previous_delay * 2);
		return;
	}
	if (corpse_transfer_disputed(ch))
	{
		// The refused assets only exist on the live character and in the ledger.
		// Never fall through to the ordinary save, which would record an empty
		// character while a missing corpse still owed them their payload.
		if (!corpse || !save_disputed_death_disposition(ch, context.corpse_uid))
		{
			persistence_alert(AVATAR, "player_save", "death", "none", "none",
					  corpse ? "death_disposition_retry" :
						   "death_recovery_corpse_missing",
					  "delay=%d", previous_delay * 2);
			schedule_death_extract_retry(ch, context.corpse_uid, previous_delay * 2);
			return;
		}
		clear_corpse_transfer_dispute(ch);
		collector_death_enrollment_end(corpse);
		release_after_terminal_death(ch, "death_disposition_completed");
		return;
	}
	if (corpse && ch->carrying)
	{
		const bool submitted = submit_next_corpse_item(ch, corpse);
		persistence_report(
			submitted ? persistence_severity::info : persistence_severity::alert,
			AVATAR, "player_save", "death", "none", "none",
			"death_recovery_restarting_corpse_items", "submitted=%d delay=%d",
			submitted ? 1 : 0, DEATH_EXTRACT_RETRY_INITIAL);
		// Publication removes the item from the character. Until that happens the
		// terminal snapshot must not capture it and extraction must not drop it.
		schedule_death_extract_retry(ch, context.corpse_uid, DEATH_EXTRACT_RETRY_INITIAL);
		return;
	}

	if (!persistence_save_character_terminal(ch, RENT_DEATH))
	{
		persistence_alert(AVATAR, "player_save", "death", "none", "none",
				  "death_recovery_retry", "delay=%d", previous_delay * 2);
		schedule_death_extract_retry(ch, context.corpse_uid, previous_delay * 2);
		return;
	}

	// Terminal publication must release intake even when no corpse-item handoff ever ran.
	collector_death_enrollment_end(corpse);
	release_after_terminal_death(ch, "death_recovery_completed");
}

bool death_extract_retry_pending(P_char ch)
{
	return ch && IS_PC(ch) && ch->only.pc &&
	       (ch->only.pc->death_retry_delay > 0 || ch->only.pc->death_retry_due_usec ||
		get_scheduled(ch, event_death_extract_retry) != nullptr);
}

bool death_extract_retry_copy_state(P_char ch, uint64_t *corpse_uid, int *delay)
{
	if (!corpse_uid || !delay)
		return false;
	*corpse_uid = 0;
	*delay = 0;
	if (!death_extract_retry_pending(ch))
		return true;
	*corpse_uid = ch->only.pc->death_retry_corpse_uid;
	*delay = ch->only.pc->death_retry_delay;
	return *delay >= DEATH_EXTRACT_RETRY_INITIAL && *delay <= DEATH_EXTRACT_RETRY_MAX;
}

bool death_extract_retry_restore(P_char ch, uint64_t corpse_uid, int delay)
{
	if (!ch || IS_NPC(ch) || !ch->only.pc || delay < DEATH_EXTRACT_RETRY_INITIAL ||
	    delay > DEATH_EXTRACT_RETRY_MAX)
		return false;
	hold_for_death_extract_retry(ch);
	schedule_death_extract_retry(ch, corpse_uid, delay);
	return death_extract_retry_pending(ch);
}

static long lich_death_residual_experience(long experience, int level)
{
	const double percentage = static_cast<double>(new_exp_table[level]) /
				  static_cast<double>(new_exp_table[level + 1]);
	return MAX(1L, static_cast<long>(experience * percentage));
}

static P_char credited_player_killer(P_char victim, P_char killer)
{
	if (!killer || killer == victim)
		return NULL;
	if (IS_PC(killer))
		return killer;
	if (!IS_PC_PET(killer))
		return NULL;
	P_char master = GET_MASTER(killer);
	if (!master || master->in_room != killer->in_room || !killer->group ||
	    killer->group != master->group)
		return NULL;
	return master;
}

void die(P_char ch, P_char killer)
{
	char buf[MAX_STRING_LENGTH];
	P_char tmp_ch;
	P_obj tempobj;
	struct affected_type *af, *next_af;
	P_obj corpse = NULL;
	uint64_t death_corpse_uid = 0;
	int loss = 0, i;

	if (!ch)
	{
		logit(LOG_EXIT, "die called in fight.c with no ch");
		return;
	}
	if (training_dummy_is(ch))
	{
		GET_HIT(ch) = GET_MAX_HIT(ch);
		SET_POS(ch, POS_STANDING + STAT_NORMAL);
		return;
	}

	if (!killer)
		return;
	if (IS_PC(ch))
		(void)telemetry_runtime_game_encounter_leave(ch,
							     telemetry_encounter_outcome::death);

	// Upon death, we want to kill followers.
	if (IS_PC(ch) && ch->followers)
		do_dismiss(ch, NULL, CMD_DEATH);

#if defined(CTF_MUD) && (CTF_MUD == 1)
	if (affected_by_spell(ch, TAG_CTF))
	{
		int stat = GET_STAT(ch);
		SET_POS(ch, GET_POS(ch) + STAT_NORMAL);
		drop_ctf_flag(ch);
		SET_POS(ch, GET_POS(ch) + stat);
	}
#endif

	REMOVE_BIT(ch->specials.affected_by3, AFF3_PALADIN_AURA);
	clear_links(ch, LNK_PALADIN_AURA);

	/* switched god */
	if (ch->desc && ch->desc->original && ch->desc->original->only.pc->switched)
		do_return(ch, 0, -4);

	ch = ForceReturn(ch);
	death_custody_wait_reset(ch);
	// A new death starts undisputed. Nothing else retires the entry when a
	// recovery is abandoned, and a stale one would skip the corpse handoff.
	clear_corpse_transfer_dispute(ch);
	/* count xp gained by killer */

	/* make mirror images disappear */
	if (IS_ALIVE(ch) && IS_NPC(ch) && GET_VNUM(ch) == 250)
	{
		if (ch == killer)
		{
			act("&+LYou disappear into thin air.&n", TRUE, ch, 0, 0, TO_CHAR);
			act("&+L$n&+L disappears into thin air.&n", TRUE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			act("Upon being struck, you disappear into thin air.", TRUE, ch, 0, 0,
			    TO_CHAR);
			act("Upon being struck, $n disappears into thin air.", TRUE, ch, 0, 0,
			    TO_ROOM);
		}
		extract_char(ch);
		return;
	}

	/* drop any disguise */
	if (IS_DISGUISE(ch))
		remove_disguise(ch, TRUE);

	if (affected_by_spell(ch, SPELL_DRACONIC_APOTHEOSIS))
	{
		for (struct affected_type *race_affect = ch->affected; race_affect;
		     race_affect = race_affect->next)
			if (race_affect->type == SPELL_DRACONIC_APOTHEOSIS)
				GET_RACE(ch) = race_affect->modifier;
	}

	if (check_outpost_death(ch, killer))
		return;

	// PCs and NPCs without a die proc.  Note: Uses lazy eval since PC->specials.act ACT_SPEC_DIE
	//   is actually PLR_SMARTPROMPT (which isn't implemented as of 5/16/2015).
	if (IS_PC(ch) || !IS_SET(ch->specials.act, ACT_SPEC_DIE))
	{
		act("$n is dead! &+RR.I.P.&n", TRUE, ch, 0, 0, TO_ROOM);
		// Only show configured death messages for hardcore characters - Arih
		if (IS_PC(ch) && IS_HARDCORE(ch) && hardcore_config_get()->death_messages_enabled)
		{
			act("&-L&+rYou feel yourself falling to the ground.&n", FALSE, ch, 0, 0,
			    TO_CHAR);
			act("&-L&+rYour soul leaves your body in the cold sleep of death...&n",
			    FALSE, ch, 0, 0, TO_CHAR);
		}
		// Do nothing for PCs and !exp mobs.
		if (IS_PC(ch) || GET_EXP(ch) <= 0 || economic_gameplay_authority::active())
		{
		}
		// Check Thanksgiving first.
		else if (GET_LEVEL(ch) > 44 && get_property("thanksgiving", 0.000) &&
			 (number(0, 100) < get_property("thanksgiving.turkey.chance", 5.000)))
		{
			thanksgiving_proc(ch);
		}
		// Then Christmas.
		else if (GET_LEVEL(ch) > 35 && get_property("christmas", 0.000) &&
			 (number(0, 100) < get_property("christmas.elf.chance", 5.000)))
		{
			christmas_proc(ch);
		}
		// NPCs that are worth exp and not PC pets may load a random item.
		enhance_on_eligible_npc_death(ch, killer);
	}

	/* This is where we were saving the newbies from being killed by high lvls.
	 if( (IS_PC(ch)) && !IS_HARDCORE(ch) && ((GET_LEVEL(killer) - GET_LEVEL(ch)) > 15)
	   && (IS_PC(killer) || (IS_NPC(killer) && IS_PC_PET(killer))) && (equipped_value(ch) < 250) )
	  {
	    newbie_reincarnate(ch);
	    return;
	  }
	 */

	// For innate resurrection which I've never heard of.
	if (check_reincarnate(ch))
		return;

	P_char eth_ch = get_linked_char(ch, LNK_ETHEREAL);
	if (!eth_ch)
		eth_ch = get_linking_char(ch, LNK_ETHEREAL);
	if (eth_ch)
	{
		clear_links(eth_ch, LNK_ETHEREAL);
		clear_links(ch, LNK_ETHEREAL);
	}

	if (!killer)
		return;

	holy_crusade_check(killer, ch);
	soul_taking_check(killer, ch);

	// Changed the order on this to take advantage of C's lazy evaluation.
	// You don't get exp for killing someone who's LD?  Odd..
	if (ch && killer && (IS_NPC(ch) || ch->desc) && (killer != ch) && !IS_TRUSTED(killer))
	{
		kill_gain(killer, ch);

		if (IS_NPC(ch) && IS_SET(ch->specials.act, ACT_ELITE) && GET_LEVEL(ch) > 49)
			group_gain_epic(killer, EPIC_ELITE_MOB, GET_VNUM(ch), (GET_LEVEL(ch) - 49));
	}

	/* victim is pc */
	if (killer && IS_PC(ch) && !IS_TRUSTED(ch) && !IS_TRUSTED(killer))
	{
		logit(LOG_DEATH, "%s killed by %s at %s", GET_NAME(ch),
		      (IS_NPC(killer) ? killer->player.short_descr : GET_NAME(killer)),
		      world[ch->in_room].name);
		statuslog(ch->player.level, "%s killed by %s at [%d] %s", GET_NAME(ch),
			  (IS_NPC(killer) ? killer->player.short_descr : GET_NAME(killer)),
			  world[ch->in_room].number, world[ch->in_room].name);

		// If killer is a PC, or a pet with master group and in room.. then we have PvP
		P_char credited_killer = credited_player_killer(ch, killer);
		if (credited_killer)
		{
			// It's important that this is before sql_save_pkill, 'cause we don't want to count this death as recent.
			setHeavenTime(ch);
			if (opposite_racewar(ch, credited_killer))
			{
				const bool award = !CHAR_IN_ARENA(ch) &&
						   fragWorthy(credited_killer, ch) &&
						   !affected_by_spell(ch, TAG_RECENTLY_FRAGGED);
				if (!submit_pvp_outcome(credited_killer, ch, award))
					send_to_char(
						"The PvP outcome service is busy; no rewards were "
						"applied.\r\n",
						ch);
			}
		}
	}

	if (IS_NPC(killer) && CAN_ACT(killer) && killer != ch &&
	    MIN_POS(killer, POS_STANDING + STAT_RESTING))
	{
		add_event(retarget_event, PULSE_VIOLENCE - 1, killer, NULL, NULL, 0, NULL, 0);
	}
	/* count xp loss for victim and apply */
	if (IS_PC(ch) && !CHAR_IN_ARENA(ch) && (GET_LEVEL(ch) > 1) && !IS_TRUSTED(ch))
	// && (GET_RACEWAR(ch) != 1)) Goods lose exp again.
	{
		if (IS_PC(ch) && (GET_RACE(ch) == RACE_LICH))
		{
			long tmp = loss = GET_EXP(ch);
			lose_level(ch);
			// This is complicated because 10M exp at 51 is not the same as 10M exp at 50/52/etc.
			GET_EXP(ch) = lich_death_residual_experience(tmp, GET_LEVEL(ch));
			// Amount of exp lost is all exp to lose level + the portion lost into the level below.
			loss += new_exp_table[GET_LEVEL(ch)] - GET_EXP(ch);
			loss *= -1;
		}
		else
		{
			loss = gain_exp(ch, NULL, 0, EXP_DEATH);
		}
		debug("&+RDeath&n: %s lost %d experience from death.", J_NAME(ch), loss);
	}

	/*
	  for (i = GET_LEVEL(ch) + 1; i > minlvl && (new_exp_table[i] <= GET_EXP(ch)); i++)
	  {
	    GET_EXP(ch) -= new_exp_table[i];
	    advance_level(ch);
	  }
	*/

	if (IS_PC(killer))
		nq_char_death(killer, ch);
	if (GET_OPPONENT(ch))
		stop_fighting(ch);
	StopAllAttackers(ch);

	REMOVE_BIT(ch->specials.act2, PLR2_WAIT);

	if (!ch || !killer)
		return;

	if (IS_PC(ch) || !IS_SET(ch->specials.act, ACT_SPEC_DIE))
	{
		if (!CAN_SPEAK(ch))
			death_rattle(ch);
		else
			death_cry(ch);
	}
	if (IS_PC(ch) && !CHAR_IN_ARENA(ch))
	{
		send_to_char(
			"\r\n&+RYour wounds claim you at last. Your spirit slips free of your body...&n\r\n",
			ch);
	}

	// Dragon mobs now will drop a dragon scale
	// No longer includes !exp mobs like dragon illusions.
	if (!economic_gameplay_authority::active() && GET_RACE(ch) == RACE_DRAGON &&
	    GET_EXP(ch) > 0)
	{
		P_obj dragon_scale = read_object(VOBJ_DRAGON_SCALE, VIRTUAL);
		obj_to_char(dragon_scale, ch);
	}

	if (!economic_gameplay_authority::active() && IS_NPC(ch) && (GET_LEVEL(ch) > 51) &&
	    !IS_PC_PET(ch) && !affected_by_spell(ch, TAG_CONJURED_PET)) // soul shard - Drannak
	{
		int dchance = 5;

		if (IS_ELITE(ch))
			dchance += 5;

		if (number(1, 250) < dchance)
		{
			P_obj teobj = read_object(400230, VIRTUAL);
			obj_to_char(teobj, ch);
			act("&+LAs &+R$n &+Lfalls to the ground, a small shard of their &+rlifeforce&n manifests&+L.&N",
			    FALSE, ch, 0, 0, TO_ROOM);
		}
	}

	// possibility to find a recipe for the items in the zone.
	//  Only find recipes from mobs inside their own zone.
	if (!economic_gameplay_authority::active() && IS_PC(killer) && !IS_PC_PET(ch) &&
	    in_their_zone(ch) && !affected_by_spell(ch, TAG_CONJURED_PET))
		random_recipe(killer, ch);

	// object code - Normal kills.  Kvark
	if (!economic_gameplay_authority::active() && (IS_PC(killer) || IS_PC_PET(killer)) &&
	    IS_NPC(ch) && IS_ALIVE(killer))
	{
		// if(GET_LEVEL(ch) < 30 || GET_LEVEL(killer) < 20)
		//   {
		if (check_random_drop(killer, ch, TRUE))
		{
			if (!number(0, 25)) // &&
			// (GET_LEVEL(ch) > 51))
			{
				tempobj = create_stones();
			}
			else
			{
				// if(GET_LEVEL(ch) < 30 || GET_LEVEL(killer) < 20) //removing level restriction, adding racewar check goods only - drannak
				// if(GET_RACEWAR(killer) == RACEWAR_GOOD || GET_RACEWAR(killer) == RACEWAR_EVIL) //allowing evils to get randoms 1/26/13 drannak
				if (GET_LEVEL(killer) > 0)
				{
					tempobj = create_random_eq_new(killer, ch, -1, -1);
					send_to_char(
						"It appears you were able to salvage a piece of equipment from your enemy.\r\n",
						killer);
				}
				else
				{
					tempobj = create_material(killer, ch);
					send_to_char(
						"It appears you were able to salvage a piece of material from your enemy.\r\n",
						killer);
				}
			}
			if (tempobj && ch)
				obj_to_char(tempobj, ch);
		}
		if (check_random_drop(killer, ch, FALSE))
		{
			if (!number(0, 25)) // &&
				// (GET_LEVEL(ch) > 51))
				tempobj = create_stones();
			else
				tempobj = create_material(killer, ch);

			if (tempobj)
				obj_to_char(tempobj, ch);
		}
		// }
	}

	update_pos(ch);
	SET_POS(ch, GET_POS(ch) + STAT_DEAD);
	update_pos(ch);

	/* Mark room as dirty for GMCP updates (character died) */
	if (ch->in_room >= 0)
		gmcp_mark_room_dirty(ch->in_room);

	if (!CHAR_IN_ARENA(ch) || IS_NPC(ch))
	{
		// world quest hook
		if ((IS_PC(killer) || IS_PC_PET(killer)) && killer->in_room >= 0 &&
		    !affected_by_spell(ch, TAG_CONJURED_PET))
		{
			quest_kill(killer, ch);
			if (killer->group)
			{
				for (tmp_ch = world[killer->in_room].people; tmp_ch;
				     tmp_ch = tmp_ch->next_in_room)
				{
					if (killer->group == tmp_ch->group && tmp_ch != killer &&
					    tmp_ch != ch)
					{
						quest_kill(tmp_ch, ch);
					}
				}
			}
		}

		studioproc_kill(killer,
				ch); /* before the ACT_SPEC_DIE block, which can return early */
		// Special death handlers can drop equipment before make_corpse runs.
		// Retain committed raised-pet gear under its durable owner first.
		hold_durable_pet_items(ch);

		if (IS_NPC(ch) && (ch->specials.act & ACT_SPEC_DIE) &&
		    (ch->specials.act & ACT_SPEC))
		{
			if (!mob_index[GET_RNUM(ch)].func.mob)
			{
				snprintf(buf, MAX_STRING_LENGTH, "%s %d", ch->player.name,
					 GET_RNUM(ch));
				logit(LOG_STATUS, "%s", buf);
				logit(LOG_STATUS, "No special function for ACT_SPEC_DIE");
				REMOVE_BIT(ch->specials.act, ACT_SPEC_DIE);
				corpse = make_corpse(ch, loss);
			}
			else
			{
				(*mob_index[GET_RNUM(ch)].func.mob)(ch, killer, CMD_DEATH, 0);
				// If this mob has been extracted and been given a release mem event.
				if (ch->nevents && ch->nevents->func == release_mob_mem)
					return;
			}
		}
		else
		{
			corpse = make_corpse(ch, loss);
		}
		if (corpse)
			death_corpse_uid = corpse->obj_uid;

		if (corpse && killer != ch &&
		    (has_innate(killer, INNATE_MUMMIFY) || has_innate(killer, INNATE_REQUIEM)))
		{
			mummify(killer, ch, corpse);
		}

		if (corpse && killer != ch &&
		    (affected_by_spell(killer, SPELL_SPAWN) || has_innate(killer, INNATE_SPAWN) ||
		     has_innate(killer, INNATE_ALLY)))
		{
			// Original: if((IS_NPC(ch) && !number(0, 2)) || !number(0, 3))
			//   !0,2 -> 1/3 || !0,3 -> 1/4 --> 1/3 || 1/4 == 1/3 + 2/3 * 1/4 == 1/3 + 1/6 == 3 / 6 == 1/2
			//   So, changing "!number(0, 2)) || !number(0, 3)" to "!number(0, 1)"
			//   Below makes more sense 'cause 1/2 NPC and 1/4 PC.
			//   But IS_PC check on NPCs 1/2 the time, and no longer 2 calls to number() for NPCs 1/2 the time.
			//   Conclusion: IS_PC == 2 dereferences (->specials.act) + bit check VS number() == 3 nested function calls+
			//     so should be same on NPC 1/2 the time and faster the other 1/2 and slower on a PC all the time.
			//     A lot more NPCs die than PCs, and the slowdown is just the 2 dereferences on a PC 100% of the time,
			//     so should be faster overall.
			if ((IS_NPC(ch) && !number(0, 1)) || (IS_PC(ch) && !number(0, 3)))
			{
				if (IS_PC(killer) && !affected_by_spell(killer, SPELL_SPAWN) &&
				    !affected_by_spell(killer, TAG_SPAWN))
				{
					send_to_char(
						"You are not willing to summon pets from death blows.\r\n",
						killer);
				}
				else if (IS_PC(ch) && (item_movement_transaction_player_busy(ch) ||
						       ch->carrying))
					persistence_report(persistence_severity::info, AVATAR,
							   "player_save", "death", "none", "none",
							   "spawn_raise_skipped_ownership_pending",
							   "corpse_uid=%llu",
							   (unsigned long long)corpse->obj_uid);
				else
					spawn_raise_undead(killer, ch, corpse);
			}
		}

		if (IS_PC(ch))
		{
			if (!CHAR_IN_ARENA(ch) || hardcore_config_get()->death_count_arena_deaths)
			{
				ch->only.pc->numb_deaths++;
			}
			// Hardcore chars are permanently removed at the configured death count.
			if (IS_HARDCORE(ch) &&
			    hardcore_config_death_is_final(ch->only.pc->numb_deaths))
			{
				update_pos(ch);
				if (hardcore_config_get()->death_hall_of_fame)
					checkHallOfFame(ch, GET_NAME(killer));
				// save killer to database for hall of fame
				if (hardcore_config_get()->death_record_killer)
					db_query(
						"UPDATE player_data SET killed_by = '%s' WHERE pid = %d",
						GET_NAME(killer), GET_PID(ch));
				if (hardcore_config_get()->death_messages_enabled)
				{
					act("&+LThe &+rhand &+Lof &+WGod &+Lgrabs &+R$n &+Lby the &+cthroat&+L.&N",
					    FALSE, ch, 0, 0, TO_ROOM);
					act("&+LThe &+rhand &+Lof &+WGod &+Ltears &+R$n&+L's &+wsoul &+Lfrom this &+cplane &+Lof existence.&N",
					    FALSE, ch, 0, 0, TO_ROOM);
					act("&+L$n's &+cbody &+Llands on the ground in a crumpled heap, &+wsoul &+Lgone forever.&N",
					    FALSE, ch, 0, 0, TO_ROOM);

					send_to_char(
						"&+LThe &+rhand &+Lof &+WGod &+Lgrabs &+RYou &+Lby the &+cthroat&+L.&N\r\n",
						ch);
					send_to_char(
						"&+LThe &+rhand &+Lof &+WGod &+Ltears &+RYour&+L &+wsoul &+Lfrom this &+cplane &+Lof existence.&N\r\n",
						ch);
					send_to_char(
						"&+WGod&+L stands before you and says '&+wYou have died your last death, Your existence shall not continue.&+L'\r\n",
						ch);
					send_to_char(
						"&+WGod&+L says '&+wYou have died a miserable soul with a value of &+R&+w.&+L'&N\r\n",
						ch);
				}

				statuslog(ch->player.level,
					  "%s's existence on Duris was just ended...by %s!",
					  GET_NAME(ch), GET_NAME(killer));

				// If it's not an immortal.
				if (GET_LEVEL(ch) < MINLVLIMMORTAL)
					update_ingame_racewar(-GET_RACEWAR(ch));

#ifdef USE_ACCOUNT
				// With account system, return to account menu instead of disconnecting
				if (ch->desc && ch->desc->account)
				{
					P_desc d = ch->desc;

					if (hardcore_config_get()->death_messages_enabled)
					{
						// Send death messages BEFORE showing account menu - Arih
						send_to_char(
							"&-L&+rYou feel yourself falling to the ground.&n\r\n",
							ch);
						send_to_char(
							"&-L&+rYour soul leaves your body in the cold sleep of death...&n\r\n\r\n",
							ch);
					}

					// Delete character file
					deleteCharacter(ch);

					// Disconnect descriptor before extract_char so it won't show menu
					ch->desc = NULL;

					// extract_char cleans up followers, equipment, etc. and frees the char
					extract_char(ch);

					// Return to account menu
					d->character = NULL;
					d->term_type = TERM_ANSI; // Preserve ANSI mode
					SEND_TO_Q(
						"&+RYour character has been permanently deleted.&n\r\n\r\n",
						d);
					STATE(d) = CON_DISPLAY_ACCT_MENU;
					display_account_menu(d, NULL);
					return;
				}
#endif

				// Without account system, or no descriptor, just disconnect
				if (ch->desc)
					close_socket(ch->desc);
				deleteCharacter(ch);
				extract_char(ch); // extract_char also calls free_char
				ch = NULL;

				return;
			}

			// This code has never been tested, so commenting out. Torgal 11/21/12
			// if (!IS_PC_PET(ch))
			//{
			//	check_boon_completion(killer, ch, 0, BOPT_MOB);
			//	check_boon_completion(killer, ch, 0, BOPT_RACE);
			//}

			GET_MANA(ch) = 0;

			for (af = ch->affected; af; af = next_af)
			{
				next_af = af->next;
				if (!(af->flags & AFFTYPE_PERM))
					affect_remove(ch, af);
				else if (af->type == TAG_MEMORIZE)
					af->flags &= ~MEMTYPE_FULL;
			}

			check_saved_corpse(ch);

			disarm_char_nevents(ch, NULL);
			ch->specials.conditions[DISEASE_TYPE] = 0;
			ch->specials.conditions[POISON_TYPE] = 0;

			/* remove all undead/druid/harpy spells */
			for (i = 1; i <= MAX_CIRCLE; i++)
				ch->specials.undead_spell_slots[i] = 0;

			update_pos(ch);
			SET_POS(ch, STAT_DEAD + GET_POS(ch));
			update_pos(ch);

			/* even things out so it doesn't barf when removing perm affect items */
			affect_total(ch, FALSE);
		}
	}
	/*
	 * this WAS in extract_char, but only death removes you from mobs mem
	 * now. renting won't do it, as long as the game is up and neither
	 * char nor mob dies, they WILL remember!
	 */

	if (IS_PC(ch) && !CHAR_IN_ARENA(ch))
	{
		P_char mob, mob_next;

		for (mob = character_list; mob; mob = mob_next)
		{
			mob_next = mob->next;
			forget(mob, ch);
		}
	}
	if (IS_PC(ch))
	{
		REMOVE_BIT(ch->specials.act2, PLR2_SPEC_TIMER);
		if (!CHAR_IN_ARENA(ch) &&
		    IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP))
		{
			// No live transfer is guaranteed to touch a payload-less custody row
			// (it may be a separate root, or the only owned item). Route the death
			// directly through the durable disposition instead of allowing an
			// ordinary empty snapshot or waiting forever for a mismatch callback.
			note_corpse_transfer_dispute(ch);
			persistence_alert(AVATAR, "player_save", "death", "none", "none",
					  "load_item_payload_gap_disposition",
					  "extract_refused=1 recovery_scheduled=1");
		}
		if (!CHAR_IN_ARENA(ch) &&
		    (item_movement_transaction_player_busy(ch) ||
		     currency_transaction_player_busy(ch) || corpse_transfer_disputed(ch) ||
		     (!IS_TRUSTED(ch) && death_wallet_pending(ch))))
		{
			persistence_report(corpse_transfer_disputed(ch) ?
						   persistence_severity::alert :
						   persistence_severity::info,
					   AVATAR, "player_save", "death", "none", "none",
					   "corpse_items_in_flight",
					   "extract_refused=1 recovery_scheduled=1 disputed=%d",
					   corpse_transfer_disputed(ch) ? 1 : 0);
			schedule_death_extract_retry(ch, death_corpse_uid,
						     DEATH_EXTRACT_RETRY_INITIAL);
			return;
		}
		P_obj death_corpse = death_corpse_uid ? corpse_live_item(death_corpse_uid) : NULL;
		if (!CHAR_IN_ARENA(ch) && death_corpse && ch->carrying)
		{
			const bool submitted = submit_next_corpse_item(ch, death_corpse);
			persistence_report(submitted ? persistence_severity::info :
						       persistence_severity::alert,
					   AVATAR, "player_save", "death", "none", "none",
					   "corpse_items_restart",
					   "submitted=%d extract_refused=1 recovery_scheduled=1",
					   submitted ? 1 : 0);
			schedule_death_extract_retry(ch, death_corpse_uid,
						     DEATH_EXTRACT_RETRY_INITIAL);
			return;
		}
		if (!CHAR_IN_ARENA(ch) && !persistence_save_character_terminal(ch, RENT_DEATH))
		{
			persistence_alert(AVATAR, "player_save", "death", "none", "none",
					  "terminal_save_failed",
					  "extract_refused=1 recovery_scheduled=1");
			send_to_char(
				"Your death could not be saved. You remain in the world for recovery.\r\n",
				ch);
			// the save pipeline retries on its own, but nothing else ever retries
			// the death itself; this completes the extraction once it succeeds
			schedule_death_extract_retry(ch, death_corpse_uid,
						     DEATH_EXTRACT_RETRY_INITIAL);
			return;
		}
		if (!CHAR_IN_ARENA(ch))
			collector_death_enrollment_end(death_corpse);
		GET_HIT(ch) = 1;
		ch->only.pc->pc_timer[1] = 0; // reset flee timer
	}

	add_track(ch, NUM_EXITS);

	if (!CHAR_IN_ARENA(ch) || IS_NPC(ch))
	{
		if (IS_NPC(ch))
		{
			extract_char(ch);
			ch = NULL;
		}
		else
		{
			// If it's not an immortal.
			if (GET_LEVEL(ch) < MINLVLIMMORTAL)
				update_ingame_racewar(-GET_RACEWAR(ch));
			extract_char_after_terminal_save(ch);
			ch = NULL;
		}
	}
	else
	{
		char strn[MAX_STRING_LENGTH];

		if (ch == killer)
		{
			snprintf(strn, MAX_STRING_LENGTH, "&+W%s killed %s own dumb self!&N\r\n",
				 GET_NAME(ch), HSHR(ch));
			send_to_arena(strn, -1);
			ARENA_PLAYER(ch).frags -= 1;
		}
		else
		{
			snprintf(strn, MAX_STRING_LENGTH, "&+R%s was %s by %s!&N\r\n", GET_NAME(ch),
				 arena_death_msg(killer->equipment[WIELD]), GET_NAME(killer));
			send_to_arena(strn, -1);
			ARENA_PLAYER(killer).frags += 2;
			arena.team[arena_team(killer)].score += 1;
			send_to_char("&+GYou gain 2 frags for scoring a primary kill!&N\r\n",
				     killer);
			if (arena.type != TYPE_DEATHMATCH && arena.type != TYPE_KING_OF_THE_HILL)
			{
				for (tmp_ch = world[killer->in_room].people; tmp_ch;
				     tmp_ch = tmp_ch->next_in_room)
				{
					if (killer->group)
					{
						if (killer->group == tmp_ch->group &&
						    tmp_ch != ch && killer != tmp_ch)
						{
							if (arena_id(tmp_ch) != -1)
							{
								ARENA_PLAYER(tmp_ch).frags += 1;
								send_to_char(
									"&+GYou gain a frag from an assist kill!&N\r\n",
									ch);
							}
						}
					}
				}
			}
		}

		/* figure out which arena ch is in */
		if (arena_id(ch) == -1)
		{
			GET_HIT(ch) = GET_MAX_HIT(ch);
			update_pos(ch);
			send_to_char("&+WYou do not belong in the arena!&N\r\n", ch);
			char_from_room(ch);
			char_to_room(ch, real_room(ch->player.birthplace), -1);
			return;
		}

		switch (arena.deathmode)
		{
		case DEATH_TOLL:
		case DEATH_EVEN_TRADE:
		case DEATH_WINNER_TAKES_ALL:
		case DEATH_FREE:
			char_from_room(ch);
			if (ARENA_PLAYER(ch).lives > 0)
			{
				ARENA_PLAYER(ch).lives--;
				arena_char_spawn(ch);
				snprintf(
					strn, MAX_STRING_LENGTH,
					"&+WYou breathe new air as you respawn.\r\nLives remaining: %d\r\n",
					ARENA_PLAYER(ch).lives);
				send_to_char(strn, ch);
			}
			else
			{
				char_to_room(ch, real_room(arena_hometown_location[arena_team(ch)]),
					     -1);
				snprintf(strn, MAX_STRING_LENGTH,
					 "&+C%s has been vanquished!\r\n&N", GET_NAME(ch));
				send_to_arena(strn, -1);
				SET_BIT(ARENA_PLAYER(ch).flags, PLAYER_DEAD);
			}
			GET_HIT(ch) = GET_MAX_HIT(ch);
			update_pos(ch);
			break;
		default:
			break;
		}
		if (arena_team_count(arena_team(ch)) < 1)
		{
			if (arena.type != TYPE_DEATHMATCH && arena.type != TYPE_KING_OF_THE_HILL)
			{
				send_to_arena("&+LOne side has been completely vanquished!&N\r\n",
					      -1);
				arena.stage++;
			}
		}

		return;
		/*
		   for (i = 0; i <= LAST_HOME; i++)
		   {
		   nr = world[ch->in_room].number;

		   if ((nr >= hometown_arena[i][1]) &&
		   (nr <= hometown_arena[i][2]))
		   {
		   char_from_room(ch);
		   char_to_room(ch, real_room(hometown_arena[i][0]), -1);

		   GET_HIT(ch) = GET_MAX_HIT(ch) / 4;
		   update_pos(ch);

		   act("&+W$n suddenly appears with a flash.", TRUE, ch, 0, 0, TO_ROOM);
		   send_to_char("&+WYou are restored to health, albeit a bit disoriented.\r\n", ch);

		   CharWait(ch, 30);

		   return;
		   }
		   }
		   */
		send_to_char("couldn't find yer arena, this is bad.\r\n", ch);
	}
}
// end of die

void kill_gain(P_char ch, P_char victim)
{
	int gain, XP;
	struct group_list *gl;
	int group_size = 0;
	int highest_level = 0;

	if (IS_PC(victim))
		if ((GOOD_RACE(ch) && GOOD_RACE(victim)) || (EVIL_RACE(ch) && EVIL_RACE(victim)))
			gain = 1;
		else
			gain = (new_exp_table[GET_LEVEL(victim)] / 2);
	else
		gain = GET_EXP(victim);

	if (!ch->group)
	{
		send_to_char("You receive your share of experience.\r\n", ch);
		gain_exp(ch, victim, gain, EXP_KILL);
		if (IS_PC(ch))
			add_bloodlust(ch, victim);

		// This is for all kinds of kill-type achievements
		update_achievements(ch, victim, 0, 2);

		// Addicted to blood stuff:
		update_addicted_to_blood(ch, victim);

		if ((GET_LEVEL(victim) > 30) && !IS_PC(victim) &&
		    !affected_by_spell(victim, TAG_CONJURED_PET))
		{
			if ((number(1, 5000) < GET_C_LUK(ch)) ||
			    (GET_RACE(victim) == RACE_DRAGON && (GET_VNUM(victim) > 10) &&
			     (GET_VNUM(victim) != 1108) && (GET_LEVEL(victim) > 49)))
			{
				send_to_char(
					"&+cAs your body absorbs the &+Cexperience&+c, you seem to feel a bit more epic!\r\n",
					ch);
				gain_epic(ch, EPIC_RANDOMMOB, 0, 1);
			}
		}
		change_alignment(ch, victim);

		if (!IS_PC(victim) && affected_by_spell(victim, SPELL_CONTAIN_BEING) &&
		    GET_CLASS(ch, CLASS_SUMMONER) && IS_SPECIALIZED(ch) && IS_PC(ch))
		{
			if (!valid_conjure(ch, victim))
			{
				send_to_char(
					"You cannot learn to summon a being outside of your area of expertise.\r\n",
					ch);
				return;
			}
			else if (ch && victim)
			{
				int chance = BOUNDED(1, GET_C_CHA(ch), 230);
				chance -= GET_LEVEL(victim);
				if ((number(1, GET_C_INT(victim)) < chance) &&
				    (GET_VNUM(victim) != 1255))
					learn_conjure_recipe(ch, victim);
			}
		}
		return;
	}

	for (gl = ch->group; gl; gl = gl->next)
	{
		// Ppl out of room still count against exp gain? Erm... no
		if (IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (ch->in_room == gl->ch->in_room))
			group_size++;

		if (IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (ch->in_room == gl->ch->in_room) &&
		    GET_LEVEL(gl->ch) > highest_level)
		{
			highest_level = GET_LEVEL(gl->ch);
		}
	}

	/* This prevents group from ganking solo racewar victims and gaining
	// tremendous exps. For PVP, the exps are divided amongst the group.
	if((IS_PC(ch) ||
	IS_PC_PET(ch)) &&
	IS_PC(victim))
	exp_divider = MAX(group_size, 1);
	else
	exp_divider = 1; // enabled and tweaked exp divider  -Odorf */

	/*if( ( RACE_GOOD(ch) && get_property("exp.groupLimit.good", 10) &&
	  group_size > get_property("exp.groupLimit.good", 10) ) ||
	  ( RACE_EVIL(ch) && get_property("exp.groupLimit.evil", 8) &&
	  group_size > get_property("exp.groupLimit.evil", 8) ) )
	  {
	  exp_divider *= 10;
	  }  //removed group cap for exp  -Odorf*/

	// Group exp groupexp function - For searching.
	// exp gain drops slower than group size increases to avoid people being unable to get in groups  -Odorf
	// +2/3 - Groupsize:exp ratio - 1: 1, 2: 3/4, 3: 3/5, 4: 1/2, 5: 3/7, .. , 10:1/4, .. , 16:1/6.. and so on.
	// +3/4 - Groupsize:exp ratio - 1: 1, 2: 4/5, 3: 2/3, 4: 4/7, 5: 1/2, .. ,  9:1/3, .. , 17:1/5.. and so on.
	float exp_divider = ((float)group_size + 3.0) / 4.0;

	for (gl = ch->group; gl; gl = gl->next)
	{
		if (IS_PC(gl->ch) && !IS_TRUSTED(gl->ch) && (gl->ch->in_room == ch->in_room))
		{
			XP = (int)(((float)GET_LEVEL(gl->ch) / (float)highest_level) *
				   ((float)gain / exp_divider));

			/* power leveler stopgap measure */
			if ((GET_LEVEL(gl->ch) + 40) < highest_level)
				XP /= 5000;
			else if ((GET_LEVEL(gl->ch) + 30) < highest_level)
				XP /= 1000;
			else if ((GET_LEVEL(gl->ch) + 20) < highest_level)
				XP /= 150;
			else if ((GET_LEVEL(gl->ch) + 15) < highest_level)
				XP /= 40;

			if (XP && IS_PC(gl->ch))
			{
				logit(LOG_EXP, "%s: %d, group kill of: %s [%d]", GET_NAME(gl->ch),
				      XP, GET_NAME(victim),
				      (IS_NPC(victim) ? GET_VNUM(victim) : 0));
			}

			send_to_char("You receive your share of experience.\r\n", gl->ch);
			gain_exp(gl->ch, victim, XP, EXP_KILL);
			if (IS_PC(gl->ch))
				add_bloodlust(gl->ch, victim);
			// this is for all kinds of kill-type quests
			update_achievements(gl->ch, victim, 0, 2);

			// Addicted to blood stuff:
			update_addicted_to_blood(gl->ch, victim);

			if ((GET_LEVEL(victim) > 30) && !IS_PC(victim) &&
			    !affected_by_spell(victim, TAG_CONJURED_PET))
			{
				if ((number(1, 5000) < GET_C_LUK(gl->ch)) ||
				    (GET_RACE(victim) == RACE_DRAGON &&
				     (GET_VNUM(victim) != 1108) && (GET_VNUM(victim) > 10) &&
				     (GET_LEVEL(victim) > 49)))
				{
					send_to_char(
						"&+cAs your body absorbs the &+Cexperience&+c, you seem to feel a bit more epic!\r\n",
						gl->ch);
					gain_epic(gl->ch, EPIC_RANDOMMOB, 0, 1);
				}
			}

			change_alignment(gl->ch, victim);

			if (!IS_PC(victim) && affected_by_spell(victim, SPELL_CONTAIN_BEING) &&
			    GET_CLASS(gl->ch, CLASS_SUMMONER) && IS_SPECIALIZED(gl->ch) &&
			    IS_PC(gl->ch))
			{
				if (!valid_conjure(gl->ch, victim))
				{
					send_to_char(
						"You cannot learn to summon a being outside of your area of expertise.\r\n",
						gl->ch);
				}
				else if (gl->ch && victim)
				{
					int chance = BOUNDED(1, GET_C_CHA(gl->ch), 230);
					chance -= GET_LEVEL(victim);
					if ((number(1, GET_C_INT(victim)) < chance) &&
					    (GET_VNUM(victim) != 1255))
						learn_conjure_recipe(gl->ch, victim);
				}
			}
		}
	}
}

/*
 * This function is now merely a wrapper translating old-style arguments to the new
 * damage API. It is provided for backward compatibility and should not be
 * used in the new code, call directly spell_damage and melee_damage instead.
 */
bool damage(P_char ch, P_char victim, double dam, int attacktype)
{
	struct damage_messages tmsg;
	int type, flags, circle;

	tmsg = {};

	if (IS_SPELL_S(attacktype))
	{
		if (attacktype == SPELL_HOLY_WORD || attacktype == SPELL_UNHOLY_WORD ||
		    attacktype == SPELL_VOICE_OF_CREATION)
			type = SPLDAM_HOLY;
		else
			type = SPLDAM_GENERIC;

		if (IS_AGG_SPELL(attacktype))
			flags = 0;
		else
			flags = SPLDAM_NODEFLECT;

		circle = GetLowestSpellCircle_p(attacktype);
		if (circle < 4)
			flags |= SPLDAM_MINORGLOBE;
		if (circle < 5)
			flags |= SPLDAM_SPIRITWARD;
		if (circle < 6)
			flags |= SPLDAM_GRSPIRIT;
		if ((circle < 7) && (attacktype != SPELL_DETONATE))
			flags |= SPLDAM_GLOBE;
		if (attacktype == SPELL_NEG_ENERGY_BARRIER)
			flags |= SPLDAM_GRSPIRIT;

		if ((attacktype >= SPELL_FIRE_BREATH && attacktype <= SPELL_LIGHTNING_BREATH) ||
		    attacktype == SPELL_SHADOW_BREATH_1 || attacktype == SPELL_SHADOW_BREATH_2)
			flags |= SPLDAM_BREATH;

		return spell_damage(ch, victim, dam, type, flags, &tmsg);
	}
	else
	{
		const bool actor_listed = ch && char_in_list(ch);
		const bool victim_listed = victim && char_in_list(victim);
		if (!actor_listed || !victim_listed)
			return TRUE;

		const uint64_t actor_runtime_id = ch->runtime_id;
		const uint64_t victim_runtime_id = victim->runtime_id;
		const int raw_result = raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &tmsg);
		if (raw_result != DAM_NONEDEAD)
			return TRUE;

		ch = find_character_by_runtime_id(actor_runtime_id);
		victim = find_character_by_runtime_id(victim_runtime_id);
		if (!ch || !victim || !IS_ALIVE(ch) || !IS_ALIVE(victim))
			return TRUE;

		if (!IS_FIGHTING(ch) && (ch->in_room == victim->in_room))
		{
			set_fighting(ch, victim);
			const int retaliation_result =
				attack_back(ch, victim, attacktype > FIRST_SKILL);
			return retaliation_result != DAM_NONEDEAD;
		}
		return FALSE;
	}
}

/**
 * this function performs checks for globe blocking, shrug, deflects, defensive nuke procs etc.
 * also recalculates the damage depending on type and some protecting spells like prot-cold, soulshield
 * explanation for some arguments:
 *
 * flags - a value describing damage type, used to check protections, vulnerabilities
 *        one of SPLDAM_FIRE, SPLDAM_COLD, SPLDAM_GAS, SPLDAM_ACID, SPLDAM_GENERIC,
 *        SPLDAM_LIGHTING, SPLDAM_NEGATIVE, SPLDAM_HOLY
 *        maybe be combined with one of the flags: SPLDAM_ISBREATH, SPLDAM_NOSHRUG
 *        for example SPLDAM_GAS | SPLDAM_ISBREATH for gaseous breath
 *        SPLDAM_NOSHRUG decides whether damage can be deflected, shrugged, absorbed with proc
 *        in most cases not set, exceptions are damage from soulhield, fireshield, throw lightning
 *        some item procs
 * magic_circle - spell's circle, used in globe checks
 *
 * TO DO:
 *   consider implementing globe checks as extra flags like SPLDAM_MINORGLOBEBLOCKS,
 *   SPLDAM_SPIRITWARDBLOCKS etc
 *   replace SPLDAM_ prefix with something else, it may collide with constants
 *   for spells. SPLDAM, PHSDAM
 */
int spell_damage(P_char ch, P_char victim, double dam, int type, uint flags,
		 struct damage_messages *messages, int *damAccumulator)
{
	struct damage_messages dummy_messages;
	struct affected_type *af;
	struct proc_data data;
	P_obj item;
	int result, circle;

	// Just making sure.
	if (!ch || !victim)
		return DAM_NONEDEAD;
	const bool ch_listed = char_in_list(ch);
	const bool victim_listed = char_in_list(victim);
	if (!ch_listed && !victim_listed)
		return DAM_BOTHDEAD;
	if (!ch_listed)
		return DAM_CHARDEAD;
	if (!victim_listed)
		return DAM_VICTDEAD;

	if (training_dummy_is(ch))
		return DAM_NONEDEAD;
	if (collector_presence_is_npc(ch) || collector_presence_is_npc(victim))
		return DAM_NONEDEAD;
	if (training_dummy_is(victim) && !training_dummy_target_allowed(ch, victim))
	{
		training_dummy_retarget_nonpet(ch, victim);
		return DAM_NONEDEAD;
	}

	if (messages == NULL)
	{
		dummy_messages = {};
		messages = &dummy_messages;
	}

	dam = (double)((int)dam - check_damage_ward(ch, victim, (int)dam));
	if (dam <= 0)
		return DAM_NONEDEAD;

	// This is for Lich spell damage vamping.
	flags |= SPLDAM_SPELL;

	///////
	// Special Conditions
	///////

	// vamping from spells might happen
	if (type == SPLDAM_FIRE && ch != victim &&
	    ((IS_AFFECTED2(victim, AFF2_FIRE_AURA) && IS_AFFECTED2(victim, AFF2_FIRESHIELD)) ||
	     GET_RACE(victim) == RACE_F_ELEMENTAL))
	{
		act("$N&+R absorbs your spell!", FALSE, ch, 0, victim, TO_CHAR);
		act("&+RYou absorb&n $n's&+R spell!", FALSE, ch, 0, victim, TO_VICT);
		act("$N&+R absorbs&n $n's&+R spell!", FALSE, ch, 0, victim, TO_NOTVICT);
		vamp(victim, dam / 4, GET_MAX_HIT(victim) * 1.1);

		// Solving issue of fire elementals not unmorting after vamping from fire spell
		update_pos(victim);
		if (IS_NPC(victim))
			do_alert(victim, NULL, 0);

		return DAM_NONEDEAD;
	}
	if (type == SPLDAM_COLD && ch != victim &&
	    ((IS_AFFECTED4(victim, AFF4_ICE_AURA)) && IS_AFFECTED3(victim, AFF3_COLDSHIELD)))
	{
		act("$N&+C absorbs your spell!", FALSE, ch, 0, victim, TO_CHAR);
		act("&+CYou absorb&n $n's&+C spell!", FALSE, ch, 0, victim, TO_VICT);
		act("$N&+C absorbs&n $n's &+Cspell!", FALSE, ch, 0, victim, TO_NOTVICT);
		vamp(victim, dam / 4, GET_MAX_HIT(victim) * 1.1);

		update_pos(victim);
		if (IS_NPC(victim))
			do_alert(victim, NULL, 0);

		return DAM_NONEDEAD;
	}

	///////
	// Aggro Handling (these should come after the above special conditions)
	///////

	// Training dummies are deliberately non-hostile and never remember attackers.
	if (!training_dummy_is(victim))
		remember(victim, ch);

	if (!training_dummy_is(victim) && IS_PC_PET(ch) && GET_MASTER(ch)->in_room == ch->in_room &&
	    CAN_SEE(victim, GET_MASTER(ch)))
	{
		remember(victim, GET_MASTER(ch));
	}

	///////
	// Special Conditions
	///////

	if (has_innate(victim, INNATE_EVASION) && GET_SPEC(victim, CLASS_MONK, SPEC_WAYOFDRAGON))
	{
		if ((((int)get_property("innate.evasion.removechance", 15.000))) > number(1, 100))
		{
			send_to_char("You twist out of the way avoiding the harmful magic!\r\n",
				     victim);
			act("$n twists out of the way avoiding the harmful magic!", FALSE, victim,
			    0, ch, TO_ROOM);
			return DAM_NONEDEAD;
		}
	}
	// globes check
	if (ch != victim)
	{
		if ((IS_AFFECTED3(victim, AFF3_SPIRIT_WARD) && (flags & SPLDAM_SPIRITWARD)) ||
		    (IS_AFFECTED3(victim, AFF3_GR_SPIRIT_WARD) && (flags & SPLDAM_GRSPIRIT)))
		{
			act("&+WThe globe around your body flares as it bears the brunt of&n $n&+W's assault!",
			    FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
			act("&+WThe globe around&n $N&+W's body flares as it bears the brunt of your assault!",
			    FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
			attack_back(ch, victim, FALSE);
			return DAM_NONEDEAD;
		}
		if (((IS_AFFECTED(victim, AFF_MINOR_GLOBE)) && (flags & SPLDAM_MINORGLOBE)) ||
		    (IS_AFFECTED2(victim, AFF2_GLOBE) && (flags & SPLDAM_GLOBE)))
		{
			act("&+RThe globe around your body flares as it bears the brunt of&n $n&+R's assault!",
			    FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
			act("&+RThe globe around&n $N&+R's body flares as it bears the brunt of your assault!",
			    FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
			attack_back(ch, victim, FALSE);
			return DAM_NONEDEAD;
		}
	}
	// end of globes check

	/* check for deflectable spells - basically all but shields damage and already deflected spells */
	if ((ch != victim) && !training_dummy_is(victim) && !IS_SET(flags, SPLDAM_NODEFLECT))
	{
		/* deflection */
		if (IS_AFFECTED4(victim, AFF4_DEFLECT) && IS_ALIVE(ch))
		{
			act("&+cA &+Ctranslucent&n&+c field &+Wflashes&n&+c around your body upon contact with&n $n&n&+c's assault, deflecting it back at $m!",
			    FALSE, ch, 0, victim, TO_VICT);
			act("&+cA &+Ctranslucent&n&+c field &+Wflashes&n&+c around&n $N&n&+c's body upon contact with&n $n&n&+c's assault, deflecting it back at $m!",
			    FALSE, ch, 0, victim, TO_NOTVICT);
			act("&+cA &+Ctranslucent&n&+c field &+Wflashes&n&+c around&n $N&n&+c's body upon contact with your assault, deflecting it back at YOU!",
			    FALSE, ch, 0, victim, TO_CHAR);

			affect_from_char(victim, SPELL_DEFLECT);
			result = spell_damage(victim, ch, dam * 0.7, type, flags | SPLDAM_NODEFLECT,
					      messages);

			if (result == DAM_NONEDEAD)
			{
				attack_back(ch, victim, FALSE);
				return DAM_NONEDEAD;
			}
			else
			{
				return DAM_CHARDEAD;
			}
		}

		if (!IS_ALIVE(victim))
			if (IS_ALIVE(ch))
				return DAM_VICTDEAD;
			else
				return DAM_BOTHDEAD;
		else if (!IS_ALIVE(ch))
			return DAM_CHARDEAD;

		/* defensive spell hook for equipped items - Tharkun */
		for (size_t proc_index = 0; proc_index < ARRAY_SIZE(proccing_slots); proc_index++)
		{
			if ((item = victim->equipment[proccing_slots[proc_index]]) == NULL)
				continue;

			if (IS_PC_PET(victim) && OBJ_VNUM(item) == 1251)
				continue;

			if (obj_index[item->R_num].func.obj != NULL)
			{
				data.victim = ch;
				data.dam = (int)dam;
				data.attacktype = type;
				data.flags = flags;
				data.messages = messages;

				if (invoke_object_special(item, victim, CMD_GOTNUKED,
							  (char *)&data))
				{
					if (GET_STAT(victim) == STAT_DEAD)
						return DAM_VICTDEAD;
					else if (GET_STAT(ch) == STAT_DEAD)
						return DAM_CHARDEAD;
					else
						return DAM_NONEDEAD;
				}
			}
		}

		/* defensive spell hook for mob proc - Torgal */
		if (victim && IS_NPC(victim) && mob_index[GET_RNUM(victim)].func.mob &&
		    !affected_by_spell(victim, TAG_CONJURED_PET))
		{
			data.victim = ch;
			data.dam = (int)dam;
			data.attacktype = type;
			data.flags = flags;
			data.messages = messages;

			if ((*mob_index[GET_RNUM(victim)].func.mob)(victim, ch, CMD_GOTNUKED,
								    (char *)&data))
			{
				if (GET_STAT(victim) == STAT_DEAD)
					return DAM_VICTDEAD;
				else if (GET_STAT(ch) == STAT_DEAD)
					return DAM_CHARDEAD;
				else
					return DAM_NONEDEAD;
			}
		}

		if (IS_AFFECTED4(victim, AFF4_STORNOGS_SPHERES))
		{
			act("&+RThe sphere circling you darts in front of&n $n&+R's assault, absorbing its magic and leaving you unharmed!",
			    FALSE, ch, 0, victim, TO_VICT);
			act("&+RThe sphere circling&n $N&+R's body darts in front of&n $n&+R's assault!",
			    FALSE, ch, 0, victim, TO_NOTVICT);
			act("&+RThe sphere circling&n $N&+R's body darts in front of your assault!",
			    FALSE, ch, 0, victim, TO_CHAR);
			af = get_spell_from_char(victim, SPELL_STORNOGS_SPHERES);

			if (af && --af->modifier == 0)
			{
				affect_from_char(victim, SPELL_STORNOGS_SPHERES);
				REMOVE_BIT(victim->specials.affected_by4, AFF4_STORNOGS_SPHERES);
				send_to_char("The last sphere protecting you disappears.\r\n",
					     victim);
			}
			attack_back(ch, victim, FALSE);
			return DAM_NONEDEAD;
		}

		if (GET_CHAR_SKILL(victim, SKILL_ARCANE_RIPOSTE) > 0 && !IS_TRUSTED(victim))
		{
			int skill_lvl = GET_CHAR_SKILL(victim, SKILL_ARCANE_RIPOSTE);

			if (!IS_STUNNED(victim) &&
			    ((dam > 10 && notch_skill(victim, SKILL_ARCANE_RIPOSTE,
						      get_property("skill.notch.arcane", 10))) ||
			     (number(1, 100) < skill_lvl / 4)))
			{
				act("$N frowns in &+cconcentration&n as $E intercepts $n's spell and &+Churls it back at $m!&n",
				    TRUE, ch, 0, victim, TO_NOTVICT);
				act("$N frowns in &+cconcentration&n as $E intercepts your spell and &+Churls it back at you!&n",
				    TRUE, ch, 0, victim, TO_CHAR);
				act("With &+cgreat mastery&n you intercept $n's spell and &+Churl it back at $m!&n",
				    TRUE, ch, 0, victim, TO_VICT);

				result = spell_damage(victim, ch, (skill_lvl * dam) / 100, type,
						      flags, messages);

				if (result == DAM_VICTDEAD)
					return DAM_CHARDEAD;
				else if (result == DAM_CHARDEAD)
					return DAM_VICTDEAD;
				else
					return result;
			}
		}

		if (GET_CHAR_SKILL(victim, SKILL_DISPERSE_FLAMES) && type == SPLDAM_FIRE &&
		    GET_CHAR_SKILL(victim, SKILL_DISPERSE_FLAMES) > number(0, 100) &&
		    !IS_TRUSTED(victim))
		{
			if ((dam > 10 &&
			     notch_skill(victim, SKILL_DISPERSE_FLAMES,
					 get_property("skill.notch.pyrokinetics", 2))) ||
			    (!number(0, 2) && number(1, 56) <= GET_LEVEL(victim)))
			{
				act("$N &+rsmiles slightly as the &+Yflames &+rdwindle and die before reaching $M.&n",
				    TRUE, ch, 0, victim, TO_NOTVICT);
				act("$N &+rsmiles slightly as the &+Yflames &+rdwindle and die before reaching $M.&n",
				    TRUE, ch, 0, victim, TO_CHAR);
				act("&+rYou take control over the &+Yflames &+rand will them to dwindle and die.&n",
				    TRUE, ch, 0, victim, TO_VICT);
				return DAM_NONEDEAD;
			}
		}

		if (GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) && type == SPLDAM_FIRE &&
		    GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) > number(0, 100) &&
		    !IS_TRUSTED(victim))
		{
			if ((dam > 10 &&
			     notch_skill(victim, SKILL_FLAME_MASTERY,
					 get_property("skill.notch.pyrokinetics", 2))) ||
			    (!number(0, 10) && number(1, 56) <= GET_LEVEL(ch)))
			{
				act("$N &+rsmiles slightly as $E stops the &+Yflames &+rsummoned by&n $n &+rand hurls them back at $m!&n",
				    TRUE, ch, 0, victim, TO_NOTVICT);
				act("&+rTo your horror you see&n $N &+rsmile slightly as $E halts the &+Yflames &+rand hurls them back at you!&n",
				    TRUE, ch, 0, victim, TO_CHAR);
				act("&+rYou laugh as you send &+Yflames &+rand burning &+Wectoplasm &+rback towards&n $n.&n",
				    TRUE, ch, 0, victim, TO_VICT);
				result = spell_damage(victim, ch,
						      GET_CHAR_SKILL(victim, SKILL_FLAME_MASTERY) *
							      dam / 80,
						      type, flags, messages);
				if (result == DAM_VICTDEAD)
					return DAM_CHARDEAD;
				else if (result == DAM_CHARDEAD)
					return DAM_VICTDEAD;
				else
					return result;
			}
		}
	}

	if (type == SPLDAM_FIRE && affected_by_spell(victim, SPELL_ICE_ARMOR))
	{
		act("&+C$N&+C's &+WI&+Cc&+wy &+BShield &+Rmelts &+Caround $M from $n's &+Rf&+ri&+Yer&+Ry &+rassault, &+Cbut leaves $M unharmed!",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("&+C$N&+C's &+WI&+Cc&+wy &+BShield &+Rmelts &+Caround $M from your &+Rf&+ri&+Yer&+Ry &+rassault, &+Cbut leaves $M unharmed!",
		    TRUE, ch, 0, victim, TO_CHAR);
		act("&+CYour &+Wi&+Cc&+wy &+Bshield &+Rmelts &+Caround you from the &+Rf&+ri&+Yer&+Ry &+rassault, &+Cbut leaves you unharmed!&n",
		    TRUE, ch, 0, victim, TO_VICT);
		affect_from_char(victim, SPELL_ICE_ARMOR);
		return DAM_NONEDEAD;
	}

	if (type == SPLDAM_HOLY && affected_by_spell(victim, SPELL_NEG_ARMOR))
	{
		act("$n's &+WH&+wo&+Ll&+Wy &+wspell &+Ldisperses the negative shield surrounding $N&+L, but leaves $M unharmed!&n",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("&+LThe &+WH&+wo&+Ll&+Wy &+wspell &+Ldisperses the negative shield surrounding $N&+L, but leaves $M unharmed!&n",
		    TRUE, ch, 0, victim, TO_CHAR);
		act("&+LThe barrier of negative energy &=LWFLASHES&n &+Las the &+WH&+wo&+Ll&+Wy &+wspell &+Ldisperses your shield but leaves you unharmed!&n",
		    TRUE, ch, 0, victim, TO_VICT);
		affect_from_char(victim, SPELL_NEG_ARMOR);
		return DAM_NONEDEAD;
	}

	if (!IS_SET(flags, SPLDAM_NOSHRUG))
	{
		if (resists_spell(ch, victim))
			return attack_back(ch, victim, FALSE);

		// @50dam: 1/10 chance, @100dam: 1/5chance, @499dam: 5/6 chance, @500dam: 100% chance to pass 1st check.
		// Also, 1% chance per 2 levels: 28% at lvl 56 with 500+dam.
		// So, 2.8% chance @lvl 56 with 50-99 dam, 5.6% chance @lvl 56 with 100-149 dam (most common).
		if ((ch != victim) && (dam / 50) > number(0, 9) &&
		    has_innate(victim, INNATE_SPELL_ABSORB) &&
		    number(1, 100) <= (GET_LEVEL(victim) / 2) && USES_SPELL_SLOTS(victim))
		{
			for (circle = get_max_circle(victim); circle >= 1; circle--)
			{
				if (victim->specials.undead_spell_slots[circle] <
				    max_spells_in_circle(victim, circle))
				{
					act("&+L$N&+L absorbs the power of your spell!&n", FALSE,
					    ch, 0, victim, TO_CHAR);
					act("&+LYou absorb the power of $n&+L's spell!&n", FALSE,
					    ch, 0, victim, TO_VICT);
					act("&+L$N&+L absorbs the power of $n&+L's spells!&n",
					    FALSE, ch, 0, victim, TO_NOTVICT);
					send_to_char_f(
						victim, "&+LYou regain a %d%s circle slot.&n\r\n",
						circle,
						((circle == 3) ?
							 "rd" :
							 ((circle == 2) ?
								  "nd" :
								  ((circle == 1) ? "st" : "th"))));
					victim->specials.undead_spell_slots[circle]++;
					return attack_back(ch, victim, FALSE);
				}
			}
		}
	}

	if (!(flags & SPLDAM_NODEFLECT) && IS_AFFECTED4(victim, AFF4_HELLFIRE) && !number(0, 5))
	{
		act("&+LYour spell is absorbed by&n $N's &+Rhellfire!", FALSE, ch, 0, victim,
		    TO_CHAR);
		act("$n's&+L spell is absorbed by your &+Rhellfire!", FALSE, ch, 0, victim,
		    TO_VICT);
		act("$n's&+L spell is absorbed by&n $N's &+Rhellfire!", FALSE, ch, 0, victim,
		    TO_NOTVICT);
		vamp(victim,
		     static_cast<double>(dam) * get_property("vamping.hellfire.absorb", 0.14),
		     static_cast<double>(GET_MAX_HIT(victim)) * VAMPPERCENT(victim));
		return DAM_NONEDEAD;
	}

	///////
	// Damage Modifiers
	///////

	damage_profile damProf;
	damProf.baseDamage = dam;
	damProf.addedMod = 0.0;
	damProf.increasedMod = 1.0;
	damProf.moreMod = 1.0;
	const uint64_t ch_runtime_id = ch->runtime_id;
	const uint64_t victim_runtime_id = victim->runtime_id;
	auto refresh_spell_damage_participants = [&]()
	{
		ch = find_character_by_runtime_id(ch_runtime_id);
		victim = find_character_by_runtime_id(victim_runtime_id);
		const bool attacker_alive = ch && IS_ALIVE(ch);
		const bool victim_alive = victim && IS_ALIVE(victim);
		if (!attacker_alive && !victim_alive)
			return DAM_BOTHDEAD;
		if (!victim_alive)
			return DAM_VICTDEAD;
		if (!attacker_alive)
			return DAM_CHARDEAD;
		return DAM_NONEDEAD;
	};

	// accumulate modifiers into damProf
	for (size_t modifier_index = 0; modifier_index < ARRAY_SIZE(spell_damage_modifiers);
	     modifier_index++)
	{
		damage_mod dam_mod = { dam_mod_type::None, 0.0 };
		spell_damage_modifiers[modifier_index](ch, victim, dam, type, flags, &dam_mod,
						       messages);
		if (dam_mod.requires_participant_revalidation)
		{
			result = refresh_spell_damage_participants();
			if (result != DAM_NONEDEAD)
				return result;
		}

		// if (dam_mod.type != dam_mod_type::None && (dam_mod.mod < 0 || dam_mod.mod > 0))
		// 	debug("spell_damage: spell_damage_modifiers[%d] - mod: %f, type: %d", i, dam_mod.mod, dam_mod.type);

		switch (dam_mod.type)
		{
		case dam_mod_type::None:
			break;
		case dam_mod_type::Added:
			damProf.addedMod += dam_mod.mod;
			break;
		case dam_mod_type::Increased:
			damProf.increasedMod += dam_mod.mod;
			break;
		case dam_mod_type::More:
			damProf.moreMod *= (1 + dam_mod.mod);
			break;
		}
	}

	dam = (damProf.baseDamage + BOUNDEDF(-100.0, damProf.addedMod, 100.0)) *
	      BOUNDEDF(0.05, damProf.increasedMod, 4.0) * BOUNDEDF(0.1, damProf.moreMod, 2.0);
	dam = MAX(1, dam);

	// Server-wide mob spell dial. It sits outside the modifier profile so the profile's
	// 2.0 cap on "more" multipliers cannot absorb it.
	if (difficulty_world_npc(ch))
	{
		const double spell_dial = difficulty_multiplier(DIFFICULTY_MOB_SPELL);
		if (spell_dial != 1.0)
			dam = MAX(1, dam * spell_dial);
	}

	// debug("spell_damage: %s doing %f damage to %s (base=%f, added=%f, increased=%f, more=%f, type=%d)!",
	//       GET_NAME(ch),
	//       dam,
	//       GET_NAME(victim),
	//       damProf.baseDamage,
	//       damProf.addedMod,
	//       damProf.increasedMod,
	//       damProf.moreMod,
	//       type);

	// ugly hack - we smuggle damage_type for eq poofing messages on 8 highest bits
	messages->type |= type << 24;
	result = raw_damage(ch, victim, dam, (RAWDAM_DEFAULT ^ flags) | RAWDAM_NOWARD, messages,
			    damAccumulator);

	const bool acid_item_damage = type == SPLDAM_ACID && !number(0, 3);
	if (acid_item_damage)
	{
		P_char acid_victim = find_character_by_runtime_id(victim_runtime_id);
		if (acid_victim)
			DamageStuff(acid_victim, SPLDAM_ACID);
	}

	if (result == DAM_NONEDEAD)
	{
		result = refresh_spell_damage_participants();
		if (result != DAM_NONEDEAD)
			return result;

		if (ilogb(dam) > number(3, 60))
		{
			DamageStuff(victim, type);
			result = refresh_spell_damage_participants();
			if (result != DAM_NONEDEAD)
				return result;
		}

		attack_back(ch, victim, FALSE);
		result = refresh_spell_damage_participants();
		if (result != DAM_NONEDEAD)
			return result;

		if (IS_AFFECTED5(victim, AFF5_WET) && type == SPLDAM_LIGHTNING &&
		    ilogb(dam) > number(0, 10))
		{
			struct affected_type shock_af;

			act("The &+Celectricity&n surges through $n's wet clothes immobilizing $m momentarily!",
			    FALSE, victim, 0, 0, TO_ROOM);
			act("The &+Celectricity&n surges through your wet clothes paralyzing you in a shock!",
			    FALSE, victim, 0, 0, TO_CHAR);
			memset(&shock_af, 0, sizeof(shock_af));
			shock_af.type = SPELL_LIGHTNING_BOLT;
			shock_af.flags = AFFTYPE_SHORT;
			shock_af.duration = (int)(get_property("shock.duration", 2.000 * WAIT_SEC));
			affect_to_char(victim, &shock_af);
		}
	}
	return result;
}

// End spell_damage()

int check_shields(P_char ch, P_char victim, int dam, int flags)
{
	// Shield damage reverses the spell_damage roles; keep its result orientation.
	const bool ch_listed = ch && char_in_list(ch);
	const bool victim_listed = victim && char_in_list(victim);
	const bool ch_initially_alive = ch_listed && IS_ALIVE(ch);
	const bool victim_initially_alive = victim_listed && IS_ALIVE(victim);
	if (!ch_initially_alive && !victim_initially_alive)
		return DAM_BOTHDEAD;
	if (!ch_initially_alive)
		return DAM_VICTDEAD;
	if (!victim_initially_alive)
		return DAM_CHARDEAD;
	if (training_dummy_is(victim))
		return DAM_NONEDEAD;

	const uint64_t ch_runtime_id = ch->runtime_id;
	const uint64_t victim_runtime_id = victim->runtime_id;
	auto refresh_check_shields_participants = [&]()
	{
		ch = find_character_by_runtime_id(ch_runtime_id);
		victim = find_character_by_runtime_id(victim_runtime_id);
		const bool ch_alive = ch && IS_ALIVE(ch);
		const bool victim_alive = victim && IS_ALIVE(victim);
		if (!ch_alive && !victim_alive)
			return DAM_BOTHDEAD;
		if (!ch_alive)
			return DAM_VICTDEAD;
		if (!victim_alive)
			return DAM_CHARDEAD;
		return DAM_NONEDEAD;
	};

	int result = DAM_NONEDEAD;
	double soulshielddam = get_property("damage.shield.soulshield", 0.400);
	double negshielddam = get_property("damage.shield.negativeshield", 0.450);
	double fshield = get_property("damage.shield.fireshield", 0.750);
	double cshield = get_property("damage.shield.coldshield", 0.750);
	double lshield = get_property("damage.shield.lightningshield", 0.750);
	double ifshield = get_property("damage.shield.infernalfuryshield", 0.90);

	uint sflags = SPLDAM_GLOBE | SPLDAM_NODEFLECT | RAWDAM_TRANCEVAMP;

	// if the incoming damage is not reduced, don't reduce the shield damage
	if (flags & PHSDAM_NOREDUCE)
		sflags |= PHSDAM_NOREDUCE;

	char buf[256];

	struct damage_messages lightningshield = {
		"$N &+Ygets zapped as $E hits you!&n",
		"&+YYou get a huge shock as you hit&n $n.",
		"$N &+Yis shocked as $E hits&n $n.",
		"$N &+Yis shocked to death as $E hits you!&n",
		"&+YAs you hit&n $n, &+Ya gigantic shock kills you!&n",
		"$N &+Yis shocked to death after hitting&n $n."
	};
	struct damage_messages coldshield = {
		"$N &+Bshivers from the unnatural cold as $E hits you!&n",
		"&+BYou shiver from the unnatural cold as you hit&n $n.",
		"$N &+Bshivers as $E hits&n $n.",
		"$N &+Bis frozen to death as $E hits you!&n",
		"&+BBRR! As you hit&n $n&+B, deadly cold fills your bones..&n",
		"$N &+Bis frozen to the bones after hitting&n $n."
	};
	struct damage_messages fireshield = {
		"$N &+ris burned severely as $E hits you!",
		"&+rYou are burned as you hit&n $n!",
		"$N &+ris burned severely as $E hits $n!",
		"&+rOnly a charred corpse remains after&n $N &+rtouches the &+Rflaming aura&N around you.",
		"&+rPHEW! Wasn't that a hot experience - the &+Raura&+r around&n $n &+r_fries_ you!",
		"&+rAs &n$N &+rtouches the &+Rfiery aura&N around $n&+r's body, $E is burned fatally."
	};
	struct damage_messages negshield = {
		"$N&+r ignites into &+Lblack flames&n&+r as $E hits you!",
		"&+rYou ignite into &+Lblack flames&n&+r as you hit&n $n!",
		"$N&+r ignites into &+Lblack flames&n&+r as $E hits&n $n!",
		"$N&+L is dissolved completely upon contact with your barrier!",
		"&+LYou are dissolved completely upon contact with&n $n's&+L barrier!",
		"$N&+L is dissolved completely upon contact with&n $n's&+L barrier!"
	};
	struct damage_messages soulshield = {
		"$N &+wsuffers from contact with the aura about you.",
		"&+wYour spirit suffers from contact with&n $n's &+waura!",
		"$N &+wsuffers from contact with the aura about&n $n.",
		"&+wThe aura of power around your body dissolves&n $N &+wcompletely!",
		"&+wYour body is dissolved by the aura around&n $n!",
		"$N &+wis dissolved completely by the aura around&n $n!"
	};
	struct damage_messages soulshield_spec = {
		"$N suffers from contact with the &+Wholy&n aura about you.",
		"Your spirit suffers from contact with $n's &+Wholy&n aura!",
		"$N suffers from contact with the &+Wholy&n aura about $n.",
		"The &+WHOLY&n aura of power around your body dissolves $N completely!",
		"Your body is dissolved by the &+WHOLY&n aura around $n!",
		"$N is dissolved completely by the &+WHOLY&n aura around $n!"
	};
	struct damage_messages acid_blood = {
		"$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
		"&+LThe world &+rexp&+Rlo&+rdes in p&+Ra&+rin &+Las the black blood greedily eats into your &+rflesh.",
		"$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
		"&+LWhat was once&n $N, &+Lbut now only a mass of burnt flesh, crumbles in a heap on the ground.",
		"&+LA pain beyond imagination overwhelms you as the black blood eats its way into your heart.",
		"&+LWhat was once&n $N, &+Lbut now only a mass of burnt flesh, crumbles in a heap on the ground."
	};
	struct damage_messages thornskin = {
		"As $N strikes you, you smile as $E tears $S &+Rfl&+res&+Rh&n!",
		"As you strike $n, you are &+rscratched&n by $s &+ythorny skin&n!",
		"As $N strikes $n, $e smiles as $N tears $S &+Rfl&+res&+Rh&n!",
		"$N is &+rscratched&n to death by your hide.",
		"You are &+rscratched&n to death by $n's hide.",
		"$N is &+rscratched&n to death by $n's hide.",
	};
	struct damage_messages infernalfury = {
		"&+yThe &+Rinf&+rer&+Lnal e&+rner&+Rgies &+ysurrounding you &+Wsear &+yinto&n $N &+yas $E hits you!&n",
		"&+yYou are engulfed by &+Rinf&+rer&+Lnal e&+rner&+Rgies &+yas you hit&n $n&+y!&n",
		"$N &+yis is engulfed by &+Rinf&+rer&+Lnal e&+rner&+Rgies &+yas $E hits&n $n&+y!&n",
		"&+yThe &+Rinf&+rer&+Lnal e&+rner&+Rgies &+Wsear &+ythe life out of&n $N &+yas $E touches you for the last time.&n",
		"&+yThe &+Wpower &+yof the &+RNin&+Le He&+Rlls &+yconsumes you!&n",
		"&+yAs &n$N &+Wtouches &+ythe &+Rinf&+rer&+Lnal e&+rner&+Rgies &+yaround&n $n, &+y$E falls lifeless to the ground."
	};

	if (training_dummy_is(ch))
		return DAM_NONEDEAD;
	if (collector_presence_is_npc(ch) || collector_presence_is_npc(victim))
		return DAM_NONEDEAD;
	if (training_dummy_is(victim) && !training_dummy_target_allowed(ch, victim))
	{
		training_dummy_retarget_nonpet(ch, victim);
		return DAM_NONEDEAD;
	}

	// Reject all other faiths MWD25
	if (IS_AFFECTED5(ch, AFF5_JUDICIUM_FIDEI))
	{
		// Reduce shield damages by a quarter
		dam = dam * 0.25f;
	}

	// if you ever change soulshield or neg shield damage make sure to check the
	// better one first here
	if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD) && !IS_UNDEADRACE(ch))
	{
		result = spell_damage(victim, ch, (int)(dam * negshielddam), SPLDAM_NEGATIVE,
				      sflags | SPLDAM_GRSPIRIT, &negshield);
		if (result == DAM_NONEDEAD)
			result = refresh_check_shields_participants();
	}
	else if (IS_AFFECTED2(victim, AFF2_SOULSHIELD) &&
		 ((IS_EVIL(ch) && IS_GOOD(victim)) || (IS_GOOD(ch) && IS_EVIL(victim)) ||
		  opposite_racewar(ch, victim)))
	{
		if (GET_SPEC(victim, CLASS_CLERIC, SPEC_HOLYMAN))
		{
			result =
				spell_damage(victim, ch, (int)(dam * soulshielddam), SPLDAM_HOLY,
					     SPLDAM_NODEFLECT | RAWDAM_TRANCEVAMP | SPLDAM_GRSPIRIT,
					     &soulshield_spec);
			if (result != DAM_NONEDEAD)
				return result;
			result = refresh_check_shields_participants();
			if (result != DAM_NONEDEAD)
				return result;

			// Little bonus for devotion. Jan08 -Lucrot
			int rnumber = 9;
			rnumber -= GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 50;

			// Elite and greater races get a similar bonus also. Jan08 -Lucrot
			if (IS_NPC(ch) && !affected_by_spell(ch, TAG_CONJURED_PET) &&
			    GET_LEVEL(ch) >= 51 && (IS_ELITE(ch) || IS_GREATER_RACE(ch)))
			{
				rnumber = 6;
			}

			if (result == DAM_NONEDEAD && !number(0, rnumber)) /*&&
			                                                     !IS_AFFECTED3(ch, AFF3_GR_SPIRIT_WARD))*/
			{
				struct affected_type af;

				memset(&af, 0, sizeof(af));
				af.flags = AFFTYPE_SHORT;
				switch (number(1, 9))
				{
				case 1:
					if (!IS_ELITE(ch) && !IS_GREATER_RACE(ch) &&
					    (GET_LEVEL(ch) + 5) > GET_LEVEL(victim))
					{
						if (!check_freedom_of_movement(victim, false))
						{
							af.type = SPELL_MINOR_PARALYSIS;
							af.bitvector2 = AFF2_MINOR_PARALYSIS;
							af.duration = PULSE_VIOLENCE;
							affect_to_char(ch, &af);
							act("&+wYour entire body freezes upon contact with the &+Wholy aura&n &+wsurrounding&n $N!",
							    FALSE, ch, 0, victim, TO_CHAR);
							act("$n's &+wentire body freezes upon contact with the &+Wholy aura&n &+wsurrounding&n $N!",
							    FALSE, ch, 0, victim, TO_NOTVICT);
							act("$n's &+wentire body freezes upon contact with the &+Wholy aura&n &+wsurrounding you!&n",
							    FALSE, ch, 0, victim, TO_VICT);
							update_pos(ch);
							break;
						}
					}
					[[fallthrough]];
				case 2:
					if (!EYELESS(ch) && !affected_by_spell(ch, SPELL_BLINDNESS))
					{
						snprintf(
							buf, sizeof buf,
							"$N &+wmutters a prayer to %s, and then heavy &+Ldarkness &+wshrouds your vision.&n",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						snprintf(
							buf, sizeof buf,
							"$n &+wgropes around as if blind as $N mutters a prayer to %s.",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
						snprintf(
							buf, sizeof buf,
							"&+wYou send a prayer to %s, to shroud your foes in &+Ldarkness.&n",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_VICT);
						blind(victim, ch, number(4, 8) * WAIT_SEC);
						result = refresh_check_shields_participants();
						if (result != DAM_NONEDEAD)
							return result;
						break;
					}
					[[fallthrough]];
				case 3:
					if (!affected_by_spell(ch, SPELL_CURSE))
					{
						af.type = SPELL_CURSE;
						af.location = APPLY_SAVING_SPELL;
						af.modifier = 10;
						af.duration = 7 * PULSE_VIOLENCE;
						affect_to_char(ch, &af);
						act("&+wYour strength dwindles as&n $N &+wgrabs you by your forehead and calls down the wraith of $S deity.&n",
						    FALSE, ch, 0, victim, TO_CHAR);
						act("$N &+wgrabs&n $n &+wby his forehead and calls down the might of $S deity.&n",
						    FALSE, ch, 0, victim, TO_NOTVICT);
						act("&+wYou touch&n $n's &+wforehead and call down the might of your deity.&n",
						    FALSE, ch, 0, victim, TO_VICT);
						break;
					}
					[[fallthrough]];
				case 4:
					if (IS_CLERIC(ch) && !IS_ELITE(ch) && !IS_GREATER_RACE(ch))
					{
						snprintf(
							buf, sizeof buf,
							"&+YBright light falls from above and&n $N &+Ysends forth divine power!&n");
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						snprintf(
							buf, sizeof buf,
							"&+w%s&+w sends a ray of light down upon&n $N.",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
						snprintf(
							buf, sizeof buf,
							"&+wYou send a prayer to %s&+w who instills you with &+Rwrath!&n",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_VICT);
						spell_silence(GET_LEVEL(victim), victim, 0, 0, ch,
							      0);
						result = refresh_check_shields_participants();
						if (result != DAM_NONEDEAD)
							return result;
						break;
					}
					[[fallthrough]];
				case 5:
					act("$N &+wmutters a silent prayer to $S gods to aid $M in battle.&n",
					    FALSE, ch, 0, victim, TO_CHAR);
					act("&+wBright light encases&n $N &+was $e mutters a prayer to $S deity.&n",
					    FALSE, ch, 0, victim, TO_NOTVICT);
					act("&+wYour god rewards your faithfulness by sending aid from the Heavens!&n",
					    FALSE, ch, 0, victim, TO_VICT);
					spell_flamestrike(GET_LEVEL(victim), victim, 0, 0, ch, 0);
					result = refresh_check_shields_participants();
					if (result != DAM_NONEDEAD)
						return result;
					break;
				case 6:
					if ((GET_VITALITY(victim) + 30) < GET_MAX_VITALITY(victim))
					{
						act("&+wBright light encases&n $N &+was $E mutters a prayer to $S god.&n",
						    FALSE, ch, 0, victim, TO_CHAR);
						act("&+wBright light encases&n $N &+was $E mutters a prayer to $S god.&n",
						    FALSE, ch, 0, victim, TO_NOTVICT);
						act("&+wYou send a prayer to the Heavens to invigorate your spirit.&n",
						    FALSE, ch, 0, victim, TO_VICT);
						spell_vigorize_critic(GET_LEVEL(victim), victim, 0,
								      0, victim, 0);
						result = refresh_check_shields_participants();
						if (result != DAM_NONEDEAD)
							return result;
						break;
					}
					[[fallthrough]];
				case 7:

					if (!IS_AFFECTED4(ch, AFF4_NOFEAR) && !IS_ELITE(ch) &&
					    !IS_GREATER_RACE(ch))
					{
						act("$N &+wresponds to your attack with a vicious look of pure hatred!&n",
						    FALSE, ch, 0, victim, TO_CHAR);
						act("$N &+wassumes a nasty look and tries to instill fear in $n!",
						    FALSE, ch, 0, victim, TO_NOTVICT);
						act("&+wYou muster up the nastiest look you can to scare off your foe.&n",
						    FALSE, ch, 0, victim, TO_VICT);
						spell_fear(GET_LEVEL(victim), victim, 0, 0, ch, 0);
						result = refresh_check_shields_participants();
						if (result != DAM_NONEDEAD)
							return result;
						break;
					}
					[[fallthrough]];
				case 8:
					if (GET_HIT(victim) + 50 < GET_MAX_HIT(victim))
					{
						snprintf(
							buf, sizeof buf,
							"&+wBright light falls from above and&n $N&+w's wounds begin to heal!&n");
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						snprintf(
							buf, sizeof buf,
							"&+w%s&+w sends a ray of light down upon&n $N&+w, healing some of $S wounds.",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
						snprintf(
							buf, sizeof buf,
							"&+wYou send a prayer to %s&+w, who smiles upon you and mends some of your wounds.",
							get_god_name(victim));
						act(buf, FALSE, ch, 0, victim, TO_VICT);
						spell_heal(GET_LEVEL(victim), victim, 0, 0, victim,
							   0);
						result = refresh_check_shields_participants();
						if (result != DAM_NONEDEAD)
							return result;
						break;
					}
					[[fallthrough]];
				case 9:
					snprintf(
						buf, sizeof buf,
						"$N &+wmutters a prayer to %s &+was $e stares coldly at you.&n",
						get_god_name(victim));
					act(buf, FALSE, ch, 0, victim, TO_CHAR);
					snprintf(
						buf, sizeof buf,
						"&+w%s&+w sends a mighty wrath upon&n $n &+was &n$N&+w's prayers are heard.",
						get_god_name(victim));
					act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
					snprintf(
						buf, sizeof buf,
						"&+wYou send a prayer to %s&+w, who responds with a bolt from the Heavens!",
						get_god_name(victim));
					act(buf, FALSE, ch, 0, victim, TO_VICT);
					spell_full_harm(GET_LEVEL(victim), victim, 0, 0, ch, 0);
					result = refresh_check_shields_participants();
					if (result != DAM_NONEDEAD)
						return result;
					break;

				default:
					break;
				}
			}
		}
		else
		{
			dam = MAX(1, (int)(dam * soulshielddam));
			result = spell_damage(victim, ch, dam, SPLDAM_HOLY,
					      sflags | SPLDAM_GRSPIRIT, &soulshield);
			if (result != DAM_NONEDEAD)
				return result;
			result = refresh_check_shields_participants();
			if (result != DAM_NONEDEAD)
				return result;
		}
	}

	// infernal fury / fireshield / coldshield / lightning shield are mutually exlusive
	if (result == DAM_NONEDEAD && IS_AFFECTED(victim, AFF_INFERNAL_FURY))
	{
		result = spell_damage(victim, ch, (int)(dam * ifshield), SPLDAM_NEGATIVE,
				      SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
				      &infernalfury);
		if (result == DAM_NONEDEAD)
			result = refresh_check_shields_participants();
	}
	else if (result == DAM_NONEDEAD && IS_AFFECTED2(victim, AFF2_FIRESHIELD))
	{
		result = spell_damage(victim, ch, (int)(dam * fshield), SPLDAM_FIRE, sflags,
				      &fireshield);
		if (result == DAM_NONEDEAD)
			result = refresh_check_shields_participants();
	}
	else if (result == DAM_NONEDEAD && IS_AFFECTED3(victim, AFF3_COLDSHIELD))
	{
		result = spell_damage(victim, ch, (int)(dam * cshield), SPLDAM_COLD, sflags,
				      &coldshield);
		if (result == DAM_NONEDEAD)
			result = refresh_check_shields_participants();
	}
	else if (result == DAM_NONEDEAD && IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
	{
		result = spell_damage(victim, ch, (int)(dam * lshield), SPLDAM_LIGHTNING, sflags,
				      &lightningshield);
		if (result == DAM_NONEDEAD)
			result = refresh_check_shields_participants();
	}

	// thornskin can apply on top of other damage shields
	if (result == DAM_NONEDEAD && IS_AFFECTED5(victim, AFF5_THORNSKIN))
	{
		struct affected_type *afp = get_spell_from_char(victim, SPELL_THORNSKIN);
		double thornDamage =
			afp == NULL ?
				2 :
				dice(MAX(2, afp->level / 10),
				     2); // 2 dam for bit, (level/10, min 2)d2 - 2d2 at 1, 5d2 at 50
		if (affected_by_spell(victim, SPELL_IRONWOOD))
		{
			// double damage if ironwood is up as well
			thornDamage *= 2.0;
		}
		result = raw_damage(victim, ch, thornDamage,
				    RAWDAM_DEFAULT | PHSDAM_NOREDUCE | flags, &thornskin);
		if (result != DAM_NONEDEAD)
			return result;
		result = refresh_check_shields_participants();
		if (result != DAM_NONEDEAD)
			return result;
	}

	if ((result == DAM_NONEDEAD) && has_innate(victim, INNATE_ACID_BLOOD) &&
	    (dam > ((flags & PHSDAM_TOUCH) ? 9 : 5)) && !number(0, 9))
	{
		act("&+LBlack blood &+wspurts from your wound as&n $N's &+wweapon &+wrips your &+rflesh.&n",
		    FALSE, victim, 0, ch, TO_CHAR);
		act("&+LBlack blood &+wspurts from&n $n &+was your weapon &+wrips $s &+rflesh.&n",
		    FALSE, victim, 0, ch, TO_VICT);
		act("&+LBlack blood &+wspurts from $n &+was&n $N's &+Lweapon &+wrips $s &+rflesh.&n",
		    FALSE, victim, 0, ch, TO_NOTVICT);
		if ((15 + GET_C_AGI(ch) / 6) > number(0, 100))
		{
			act("$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
			    FALSE, victim, 0, ch, TO_CHAR);
			act("&+wYou jump out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
			    FALSE, victim, 0, ch, TO_VICT);
			act("$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
			    FALSE, victim, 0, ch, TO_NOTVICT);
		}
		else
		{
			result = spell_damage(victim, ch, 40 + number(1, 40), SPLDAM_ACID,
					      SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &acid_blood);
		}
	}
	return result;
}

/* melee_damage
 * This functions handles physical damage and modifies it against stoneskin type spells,
 * calculates vamping from melee damage and also resolves damage from fireshield type
 * spells to the attacker.
 */
int melee_damage(P_char ch, P_char victim, double dam, int flags, struct damage_messages *messages,
		 int *damAccumulator)
{
	struct damage_messages dummy_messages;
	unsigned int skin;
	int result, shld_result, ac;
	float reduction = 0.0f;
	bool dragonfist;

	// float    f_cur_hit, f_max_hit, f_skill = 0;  <-- ill use those for max_str later

	const bool ch_listed = ch && char_in_list(ch);
	const bool victim_listed = victim && char_in_list(victim);
	const bool ch_initially_alive = ch_listed && IS_ALIVE(ch);
	const bool victim_initially_alive = victim_listed && IS_ALIVE(victim);
	if (!ch_initially_alive && !victim_initially_alive)
		return DAM_BOTHDEAD;
	if (!ch_initially_alive)
		return DAM_CHARDEAD;
	if (!victim_initially_alive)
		return DAM_VICTDEAD;

	const uint64_t ch_runtime_id = ch->runtime_id;
	const uint64_t victim_runtime_id = victim->runtime_id;
	auto refresh_melee_damage_participants = [&]()
	{
		ch = find_character_by_runtime_id(ch_runtime_id);
		victim = find_character_by_runtime_id(victim_runtime_id);
		const bool live_ch = ch && IS_ALIVE(ch);
		const bool live_victim = victim && IS_ALIVE(victim);
		if (!live_ch && !live_victim)
			return DAM_BOTHDEAD;
		if (!live_ch)
			return DAM_CHARDEAD;
		if (!live_victim)
			return DAM_VICTDEAD;
		return DAM_NONEDEAD;
	};
	if (collector_presence_is_npc(ch) || collector_presence_is_npc(victim))
		return DAM_NONEDEAD;

	if (messages == NULL)
	{
		dummy_messages = {};
		messages = &dummy_messages;
	}

	if (!IS_FIGHTING(ch) && !(flags & PHSDAM_NOENGAGE) && ch->in_room == victim->in_room)
	{
		set_fighting(ch, victim);
	}

	dragonfist = FALSE;
	if (!(flags & PHSDAM_NOREDUCE))
	{
		/*dam -= BOUNDED(0,
		  (int) ((dam *
		  (100 -
		  BOUNDED(-100, BOUNDED(-100, GET_AC(victim), 100),
		  100))) / 800), (int) ((dam - 1)));
		  */
		// If they have fist up, the have a 33% chance of hitting through ac.
		if (get_spell_from_char(ch, SKILL_FIST_OF_DRAGON) && !number(0, 2))
		{
			dragonfist = TRUE;
			// If the ac is > 0, apply it (increases damage), otherwise skip.
			if ((ac = calculate_ac(victim)) > 0)
				dam += (dam * (ac / 1000.00));
		}
		else
		{
			dam += (dam * calculate_ac(victim) / 1000.00);
		}

		if (has_innate(victim, INNATE_TROLL_SKIN))
			dam *= dam_factor[DF_TROLLSKIN];
		/*
		    if(affected_by_spell(ch, TAG_NOMISFIRE)) //new misfire code - drannak 8-12-2012
		    {
		    dam = dam;
		//send_to_char("damage output normal\r\n", ch);
		}
		else
		{
		dam = (dam * .5);
		//send_to_char("&+Rdamage output halved\r\n", ch);
		}
		*/

		if (has_innate(victim, INNATE_GUARDIANS_BULWARK) && victim->equipment[WEAR_SHIELD])
			dam *= dam_factor[DF_GUARDIANS_BULWARK];

		if (affected_by_spell(victim, SKILL_BERSERK))
			dam *= dam_factor[DF_BERSERKMELEE];

		if (rapier_dirk_check(victim))
			dam *= dam_factor[DF_SWASHBUCKLER_DEFENSE];

		if (rapier_dirk_check(ch))
			dam *= dam_factor[DF_SWASHBUCKLER_OFFENSE];

		if (IS_RIDING(ch) && (GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_DEMONIC) ||
				      GET_SPEC(ch, CLASS_PALADIN, SPEC_CAVALIER)))
		{
			dam *= 1.20;
		}

		if (IS_AFFECTED2(victim, AFF2_SOULSHIELD) &&
		    (((IS_EVIL(ch) && IS_GOOD(victim)) || (IS_GOOD(ch) && IS_EVIL(victim)) ||
		      opposite_racewar(ch, victim))))
		{
			dam *= dam_factor[DF_SOULMELEE];
		}

		if (IS_AFFECTED4(victim, AFF4_PROT_LIVING) && !IS_UNDEADRACE(ch))
			dam *= dam_factor[DF_PROTLIVING];
	}

	if (!(flags & PHSDAM_NOPOSITION))
	{
		if (MIN_POS(victim, POS_STANDING + STAT_NORMAL))
		{
			if (get_linked_char(ch, LNK_FLANKING) == victim)
				dam = (dam * get_property("damage.modifier.flank", 1.5));
			if (get_linked_char(ch, LNK_CIRCLING) == victim)
				dam = (dam * get_property("damage.modifier.circle", 2.0));
		}
		// They get 1/2 the bonus damage if target not standing.
		else
		{
			if (get_linked_char(ch, LNK_FLANKING) == victim)
				dam = dam * (1 + get_property("damage.modifier.flank", 1.5)) / 2;
			if (get_linked_char(ch, LNK_CIRCLING) == victim)
				dam = dam * (1 + get_property("damage.modifier.circle", 2.0)) / 2;
		}

		if (MIN_POS(victim, POS_STANDING + STAT_NORMAL))
		{
			// No bonus.
		}
		else if (MIN_POS(victim, POS_KNEELING + STAT_DEAD) && !GROUNDFIGHTING_CHECK(victim))
		{
			dam *= dam_factor[DF_KNEELING];
		}
		else if (MIN_POS(victim, POS_SITTING + STAT_DEAD) && !GROUNDFIGHTING_CHECK(victim))
		{
			dam *= dam_factor[DF_SITTING];
		}
		else if (MIN_POS(victim, POS_PRONE + STAT_DEAD) && !GROUNDFIGHTING_CHECK(victim))
		{
			dam *= dam_factor[DF_PRONE];
		}
	}

	if (GET_CLASS(ch, CLASS_BERSERKER) && (GET_HIT(ch) > 0) &&
	    affected_by_spell(ch, SKILL_BERSERK))
	{
		if (IS_NPC(ch))
			dam = (dam * BOUNDED(1, ((GET_MAX_HIT(ch) / GET_HIT(ch)) / 4), 2));
		else
			dam = (dam * BOUNDED(1, ((GET_MAX_HIT(ch) / GET_HIT(ch)) / 2), 3));

		// Since rage no longer flurries, this is going to be a damage increase
		// while the berserker is raged.
		if (affected_by_spell(ch, SKILL_RAGE))
			dam *= dam_factor[DF_RAGED];
	}

	dam = (double)((int)dam - check_damage_ward(ch, victim, (int)dam));
	if (dam <= 0)
		return 0;

	// Earth elementals ignore earth aura.
	if (IS_AFFECTED2(victim, AFF2_EARTH_AURA) && !(flags & PHSDAM_NOREDUCE) && !number(0, 5) &&
	    GET_RACE(ch) != RACE_E_ELEMENTAL)
	{
		dam = 0;
		act("$n's &+yattack glances off of your stone hide!", TRUE, ch, NULL, victim,
		    TO_VICT);
		act("$n's &+yattack glances off of&n $N's &+ystone hide!", TRUE, ch, NULL, victim,
		    TO_NOTVICT);
		act("&+yYour attack glances off of&n $N's &+ystone hide!", TRUE, ch, NULL, victim,
		    TO_CHAR);
	}

	// Combat mind bypasses displacement.
	if (affected_by_spell(victim, SPELL_DISPLACEMENT) && (!number(0, 5)) &&
	    !(flags & PHSDAM_NOREDUCE) && !affected_by_spell(ch, SPELL_COMBAT_MIND))
	{
		dam = 0;
		act("&+WThe aura of displacement around&n $n&+W absorbs most of the assault!&n",
		    FALSE, victim, 0, 0, TO_ROOM);
		act("$e THOUGHT $s was going to hit you...&n", FALSE, victim, 0, 0, TO_CHAR);
	}

	if (affected_by_spell(victim, SPELL_STONE_SKIN) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.stoneSkin", 0.75);
		skin = SPELL_STONE_SKIN;
	}
	else if (affected_by_spell(victim, SPELL_BIOFEEDBACK) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.biofeedback", 0.65);
		skin = SPELL_BIOFEEDBACK;
	}
	else if (affected_by_spell(victim, SPELL_SHADOW_SHIELD) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.stoneSkin", 0.75);
		skin = SPELL_SHADOW_SHIELD;
	}
	else if (affected_by_spell(victim, SPELL_IRONWOOD) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.ironwood", 0.80);
		skin = SPELL_IRONWOOD;
	}
	else if (affected_by_spell(victim, SPELL_ICE_ARMOR) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.icearmor", .75);
		skin = SPELL_ICE_ARMOR;
	}
	else if (affected_by_spell(victim, SPELL_NEG_ARMOR) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.negarmor", .75);
		skin = SPELL_NEG_ARMOR;
	}
	else if (affected_by_spell(victim, SPELL_DRAKESCALE_AEGIS) && !(flags & PHSDAM_NOREDUCE))
	{
		reduction = 1. - get_property("damage.reduction.drakescaleAegis", 0.75);
		skin = SPELL_DRAKESCALE_AEGIS;
	}
	else
	{
		skin = 0;
	}

	//-------------------------------
	// ranged stuff, moved from range.c
	// check for flags & PHSDAM_ARROW
	/*
	   if((IS_PC(victim) && dam < 120) || (IS_NPC(victim) && dam < 75))
	   {
	   act("The missile is slowed by your defensive magic.", FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
	   act("Your missile deflects harmlessly off of $N!", FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
	   }
	   else
	   {
	   act("The arrow finds a weak point in your defensive magic and strikes true.", FALSE, ch, 0, victim, TO_VICT | ACT_NOTTERSE);
	   act("Your missile finds a weak point in&n $N's &ndefensive magic!", FALSE, ch, 0, victim, TO_CHAR | ACT_NOTTERSE);
	   }
	   */
	//-------------------------------

	if (skin && !dragonfist)
	{
		dam -= get_property("damage.reduction.skinSpell.deduct", 46);

		if (dam > 10 && !(flags & PHSDAM_NOREDUCE))
			dam -= dam * reduction;

		// for mobs flagged with a skin spell that aren't pets, 1% chance to break it.  pets are more likely to have it broken (5%)
		decrease_skin_counter(victim, skin);
	}

	if (affected_by_spell(ch, ACH_DRAGONSLAYER) && (GET_RACE(victim) == RACE_DRAGON))
		dam *= 1.10;

	if (affected_by_spell(ch, SKILL_DREADNAUGHT) && !(flags & PHSDAM_NOREDUCE))
	{
		struct affected_type *paf = get_spell_from_char(ch, SKILL_DREADNAUGHT);
		// 80% to 40% reduction based on skill level
		float attacker_reduction = BOUNDED(20, paf->level, 60) / 100.0;
		dam *= attacker_reduction;
	}

	if (affected_by_spell(victim, SKILL_DREADNAUGHT) && !(flags & PHSDAM_NOREDUCE))
	{
		struct affected_type *paf = get_spell_from_char(victim, SKILL_DREADNAUGHT);
		// 20% to 60% reduction based on skill level (with a chance for a few extra percent when skill is 91+)
		float skill =
			BOUNDED(20, paf->level, 60) +
			(paf->level > 90 ? number((paf->level - 90) / 2, paf->level - 90) : 0);
		float victim_reduction = (100.0 - skill) / 100.0;
		dam *= victim_reduction;
	}

	dam = MAX(1, dam);

	messages->type |= 1 << 24;

	result = raw_damage(ch, victim, dam, RAWDAM_DEFAULT | flags | RAWDAM_NOWARD, messages,
			    damAccumulator);

	if (result != DAM_NONEDEAD)
		return result;
	result = refresh_melee_damage_participants();
	if (result != DAM_NONEDEAD)
		return result;

	if (affected_by_spell(ch, SPELL_WARRING_ZEAL))
	{
		struct damage_messages wz_messages = {
			"Your strike $N with &+Wholy &+rfe&+Rrv&+ror&n!",
			"You feel the &+Wholy &+Ypower&n of $n's strike!",
			"$n's strike bathes $N in &+Wholy &+rfl&+Ram&+res&n!",
			"$N is consumed by your &+Wholy &+rfl&+Ram&+res&n!",
			"You are consumed by $n's &+Wholy &+rfl&+Ram&+res&n!",
			"$N is consumed by $n's &+Wholy &+rfl&+Ram&+res&n!",
			DAMMSG_TERSE
		};
		int local_dam =
			dice((int)get_property("spell.warringzeal.dize.num", 8),
			     (int)get_property("spell.warringzeal.dize.num",
					       5)); // default is 2-10 damage after reductions
		result = spell_damage(ch, victim, local_dam, SPLDAM_HOLY, SPLDAM_NODEFLECT,
				      &wz_messages, damAccumulator);
		if (result != DAM_NONEDEAD)
			return result;
		result = refresh_melee_damage_participants();
		if (result != DAM_NONEDEAD)
			return result;
	}

	if (ilogb(dam) / 2 > number(1, 100))
	{
		DamageStuff(victim, SPLDAM_GENERIC);
		result = refresh_melee_damage_participants();
		if (result != DAM_NONEDEAD)
			return result;
	}

	if (dam <= 5 || (flags & PHSDAM_NOSHIELDS))
	{
		if (!(flags & PHSDAM_NOENGAGE))
			attack_back(ch, victim, TRUE);
		return result;
	}

	// If they actually took damage, check for coldshield/fireshield/etc.
	if (dam > 0)
		shld_result = check_shields(ch, victim, dam, flags);
	else
		shld_result = DAM_NONEDEAD;

	if (shld_result == DAM_CHARDEAD)
		return DAM_VICTDEAD;
	else if (shld_result == DAM_VICTDEAD)
		return DAM_CHARDEAD;
	else if (shld_result == DAM_NONEDEAD)
	{
		result = refresh_melee_damage_participants();
		if (result != DAM_NONEDEAD)
			return result;
		if (!(flags & PHSDAM_NOENGAGE))
			return attack_back(ch, victim, TRUE);
	}

	return shld_result;
}

/*
	 * returns TRUE if victim is no longer attackable (dead/fled/linkdead rescue/etc),
	 * else FALSE.  With proper
	 * checks in calling routines, this should make us much more
	 * stable. messages should provide set of messages for attacker, victim
	 * and room in non-killing and killing version. messages->type is a flag
	 * which can be 0, DAMMSG_TERSE, DAMMSG_MELEE or a combination of those.
	 * DAMMSG_TERSE means message will not be shown for people with terse on,
	 * DAMMSG_MELEE is reserved for hit() messages which contain information
	 * about victim's condition after damage is dealt.
	 *
	 * do not call any version of damage with dam == 0, if you want to display
	 * a miss message, do it yourself, if you want to engage, use set_fighting()
	 *
	 * TO DO: change return type to:
	 *   VICT_DEAD == 1
	 *   CHAR_DEAD == -1
	 *   BOTH_DEAD == -2
	 *   NONE_DEAD == 0
	 * adapt code using damage(). it will allow us to avoid a lot
	 * of expensive char_in_list() calls
	 * probably the only case where damage can return -2 is when a dying
	 * mob procs on death, spell_damage and melee_damage can quite
	 * often return -1, on deflects and shields
	 */
int raw_damage(P_char ch, P_char victim, double dam, uint flags, struct damage_messages *messages,
	       int *damAccumulator)
{
	struct affected_type *af, *next_af;
	char buffer[MAX_STRING_LENGTH];
	P_char tch, orig;
	int max_hit, room, act_flag;
	unsigned int new_stat;

	if (!ch)
	{
		logit(LOG_EXIT, "raw_damage in fight.c called without ch");
		return DAM_NONEDEAD;
	}
	if (!char_in_list(ch))
	{
		if (victim && !char_in_list(victim))
			return DAM_BOTHDEAD;
		return DAM_CHARDEAD;
	}
	if (training_dummy_is(ch))
		return DAM_NONEDEAD;

	if (!victim)
		return DAM_NONEDEAD;
	if (!char_in_list(victim))
		return DAM_VICTDEAD;
	const uint64_t ch_runtime_id = ch->runtime_id;
	if (collector_presence_is_npc(ch) || collector_presence_is_npc(victim))
		return DAM_NONEDEAD;
	if (training_dummy_is(victim) && !training_dummy_target_allowed(ch, victim))
	{
		training_dummy_retarget_nonpet(ch, victim);
		return DAM_NONEDEAD;
	}

	if (ch && victim) // Just making sure.
	{
		if (!IS_SET(flags, RAWDAM_NOWARD))
		{
			dam = (double)((int)dam - check_damage_ward(ch, victim, (int)dam));
			if (dam <= 0)
				return DAM_NONEDEAD;
		}

		update_groupies(ch, true);

		/* we crash, because victim has most likely been extracted and its memory may now belong to something else */
		if (GET_STAT(victim) == STAT_DEAD)
		{
			logit(LOG_DEBUG, "damage called on dead character (%s)!", GET_NAME(victim));
			return DAM_VICTDEAD;
		}

		// CMD_FIRE (do_fire) handles its own hide removal.  This is important for shadow archery.
		appear(ch, (flags & !PHSDAM_ARROW));
		appear(victim);

		if (victim != ch)
		{
			if (CHAR_IN_SAFE_ROOM(ch) && !training_dummy_is(victim))
				return DAM_NONEDEAD;

			if (should_not_kill(ch, victim))
				return DAM_NONEDEAD;

			if (IS_TRUSTED(victim) && IS_SET(victim->specials.act, PLR_AGGIMMUNE))
				return DAM_NONEDEAD;

			if (victim->following == ch)
				stop_follower(victim);
		}

		///////
		// Damage Modifiers
		///////

		damage_profile damProf;
		damProf.baseDamage = dam;
		damProf.addedMod = 0.0;
		damProf.increasedMod = 1.0;
		damProf.moreMod = 1.0;

		// accumulate modifiers into damProf
		for (size_t modifier_index = 0; modifier_index < ARRAY_SIZE(raw_damage_modifiers);
		     modifier_index++)
		{
			damage_mod dam_mod = { dam_mod_type::None, 0.0 };
			raw_damage_modifiers[modifier_index](ch, victim, dam, 0, flags, &dam_mod,
							     messages);

			// if (dam_mod.type != dam_mod_type::None && (dam_mod.mod < 0 || dam_mod.mod > 0))
			// 	debug("raw_damage: raw_damage_modifiers[%d] - mod: %f, type: %d", i, dam_mod.mod, dam_mod.type);

			switch (dam_mod.type)
			{
			case dam_mod_type::None:
				break;
			case dam_mod_type::Added:
				damProf.addedMod += dam_mod.mod;
				break;
			case dam_mod_type::Increased:
				damProf.increasedMod += dam_mod.mod;
				break;
			case dam_mod_type::More:
				damProf.moreMod *= (1 + dam_mod.mod);
				break;
			}
		}

		dam = (damProf.baseDamage + BOUNDEDF(-100.0, damProf.addedMod, 100.0)) *
		      BOUNDEDF(0.10, damProf.increasedMod, 4.0) *
		      BOUNDEDF(0.1, damProf.moreMod, 2.0);
		dam = MAX(1, dam);

		// debug("raw_damage: %s doing %f damage to %s (base=%f, added=%f, increased=%f, more=%f)!",
		//       GET_NAME(ch),
		//       dam,
		//       GET_NAME(victim),
		//       damProf.baseDamage,
		//       damProf.addedMod,
		//       damProf.increasedMod,
		//       damProf.moreMod);

		dam = BOUNDED(1, (int)dam, 32766);

		if (training_dummy_is(victim))
		{
			const int recorded_damage = static_cast<int>(dam);
			training_dummy_note_attacker(victim, ch);
			training_dummy_record_damage(victim, recorded_damage);
			if (damAccumulator)
				*damAccumulator += recorded_damage;
			if (IS_PC(ch) && ch->desc)
				send_to_char_f(ch, "The training dummy absorbs %d damage.\r\n",
					       recorded_damage);
			return DAM_NONEDEAD;
		}

		check_blood_alliance(victim, (int)dam);

		if (IS_AFFECTED5(victim, AFF5_IMPRISON) && (flags & RAWDAM_IMPRISON) &&
		    handle_imprison_damage(ch, victim, (int)dam))
		{
			return DAM_NONEDEAD;
		}

		if (IS_AFFECTED5(victim, AFF5_VINES) && (flags & RAWDAM_VINES) &&
		    (af = get_spell_from_char(victim, SPELL_VINES)))
		{
			bool vine_success = FALSE;
			if (number((20 + af->modifier / 4), 100))
			{
				act("The &+Gvines&n protecting you bear the brunt of the assault.",
				    FALSE, ch, 0, victim, TO_VICT);
				act("The &+Gvines&n protecting $N bear the brunt of the assault.",
				    FALSE, ch, 0, victim, TO_NOTVICT);
				act("The &+Gvines&n surrounding $N bear the brunt of your attack.",
				    FALSE, ch, 0, victim, TO_CHAR);
				vine_success = TRUE;
			}

			af->modifier -= (int)dam;

			if (af->modifier <= 0)
			{
				act("The &+Gvines&n surrounding you wither and die.", FALSE, ch, 0,
				    victim, TO_VICT);
				act("The &+Gvines&n surrounding $N wither and die.", FALSE, ch, 0,
				    victim, TO_NOTVICT);
				act("The &+Gvines&n surrounding $N wither and die.", FALSE, ch, 0,
				    victim, TO_CHAR);
				REMOVE_BIT(victim->specials.affected_by5, AFF5_VINES);
				affect_from_char(victim, SPELL_VINES);
			}

			if (vine_success)
				return (DAM_NONEDEAD);
		}

		if (!IS_TRUSTED(victim))
		{
			if (flags & RAWDAM_NOKILL)
			{
				if (GET_HIT(victim) > 1)
				{
					if (GET_HIT(victim) - dam < 1)
						dam = GET_HIT(victim) - 1;
				}
				else
				{
					dam = 0;
				}
			}
			else if (GET_HIT(victim) - dam < -11)
			{
				dam = GET_HIT(victim) + 11;
			}
			GET_HIT(victim) -= dam;
			telemetry_runtime_game_combat_damage(
				ch, victim, dam > 0.0 ? static_cast<std::uint64_t>(dam) : 0U, 0U);

			/* Send GMCP updates for combat */
			gmcp_char_vitals(victim); /* Update victim's vitals */
			gmcp_combat_target(ch, GET_OPPONENT(ch)); /* Update with actual opponent */
		}

		if (IS_PC(victim) && victim->desc &&
		    (IS_SET(victim->specials.act, PLR_SMARTPROMPT) ||
		     IS_SET(victim->specials.act, PLR_OLDSMARTP)))
		{
			victim->desc->prompt_mode = TRUE;
		}
		// Switched monster for example
		else if (victim->desc && (orig = victim->desc->original) != NULL)
		{
			if (IS_SET(orig->specials.act, PLR_SMARTPROMPT) ||
			    IS_SET(orig->specials.act, PLR_OLDSMARTP))
			{
				orig->desc->prompt_mode = TRUE;
			}
		}

		// Exps for damage
		// only getting damage exp once from the same mob, to prevent cheese
		if (IS_NPC(victim) && GET_HIT(victim) < GET_LOWEST_HIT(victim))
		{
			if (!(flags & RAWDAM_NOEXP))
			{
				gain_exp(ch, victim,
					 MIN(dam, GET_LOWEST_HIT(victim) - GET_HIT(victim)),
					 EXP_DAMAGE);
			}
			GET_LOWEST_HIT(victim) = GET_HIT(victim);
		}

		if (damAccumulator)
			*damAccumulator = *damAccumulator + (int)dam;

		snprintf(buffer, MAX_STRING_LENGTH, "&+w[Damage: %2d ] &n", (int)dam);

		if (IS_PC(ch) && !IS_TRUSTED(ch) && IS_SET(ch->specials.act2, PLR2_DAMAGE) &&
		    (!messages || !IS_SET(messages->type, DAMMSG_TERSE) ||
		     !IS_SET(ch->specials.act2, PLR2_TERSE)))
		{
			send_to_char(buffer, ch);
		}

		for (tch = world[victim->in_room].people; tch; tch = tch->next_in_room)
		{
			if (IS_TRUSTED(tch) && IS_SET(tch->specials.act2, PLR2_DAMAGE) &&
			    (!messages || !IS_SET(messages->type, DAMMSG_TERSE) ||
			     !IS_SET(tch->specials.act2, PLR2_TERSE)))
			{
				send_to_char(buffer, tch);
			}
			// If it's a charmie, charmed by a PC with PET_DAMAGE toggled on.
			// Note: We want pet damage to show for illusionist / no-order pets which are
			//   affected by SPELL_CHARM_PERSON, but not AFF_CHARM.
			else if ((IS_AFFECTED(ch, AFF_CHARM) ||
				  affected_by_spell(ch, SPELL_CHARM_PERSON)) &&
				 IS_PC(tch) && IS_SET(tch->specials.act3, PLR3_PET_DAMAGE) &&
				 tch == GET_MASTER(ch))
			{
				send_to_char(buffer, tch);
			}
		}

		if (messages && messages->type & DAMMSG_TERSE)
			act_flag = ACT_NOTTERSE;
		else
			act_flag = 0;
		/*
			    char showdam[MAX_STRING_LENGTH];
			    snprintf(showdam, MAX_STRING_LENGTH, " [&+wDamage: %d&n] ", (int) dam);
			*/
		new_stat = calculate_ch_state(victim);

		if (new_stat == STAT_DEAD)
		{
			for (af = victim->affected; af; af = af->next)
			{
				if (af->type == TAG_RACE_CHANGE ||
				    af->type == SPELL_DRACONIC_APOTHEOSIS)
				{
					GET_RACE(victim) = af->modifier;
				}
			}
		}

		if ((ch != victim))
			check_vamp(ch, victim, dam, flags);

		P_char rider = GET_RIDER(victim);

		if (rider != NULL)
		{
			if (IS_DRAGOON(rider) && affected_by_spell(rider, SPELL_STIGMATA_DRACONICA))
			{
				if (is_dragoon_mounted(rider))
				{
					P_char mount = get_dragoon_mount(rider);

					if (mount == victim)
						check_vamp(rider, victim, dam, flags);
				}
			}
		}

		if (!messages)
		{
			if (new_stat == STAT_DEAD)
				soul_trap(ch, victim);
		}
		else if (new_stat == STAT_DEAD)
		{
			if (!soul_trap(ch, victim))
			{
				act(messages->death_attacker, FALSE, ch, messages->obj, victim,
				    TO_CHAR);
				act(messages->death_victim, FALSE, ch, messages->obj, victim,
				    TO_VICT);
				act(messages->death_room, FALSE, ch, messages->obj, victim,
				    TO_NOTVICTROOM);
			}
		}
		else if (messages->type & (DAMMSG_EFFECT_HIT | DAMMSG_EFFECT | DAMMSG_HIT_EFFECT))
		{
			dam_message(dam, ch, victim, messages);
		}
		else
		{
			act(messages->attacker, FALSE, ch, messages->obj, victim,
			    TO_CHAR | act_flag);
			act(messages->victim, FALSE, ch, messages->obj, victim, TO_VICT | act_flag);
			act(messages->room, FALSE, ch, messages->obj, victim,
			    TO_NOTVICTROOM | act_flag);
		}

		if (GET_STAT(victim) == STAT_SLEEPING && new_stat != STAT_DEAD)
		{
			act("$n has a RUDE awakening!", TRUE, victim, 0, 0, TO_ROOM);
			affect_from_char(victim, SPELL_SLEEP);
			if (IS_AFFECTED(victim, AFF_SLEEP))
				REMOVE_BIT(victim->specials.affected_by, AFF_SLEEP);
			SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
		}

		/* instant nuke of minor para if hit */
		if (IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS) && ch != victim)
		{
			act("$n's crushing blow frees $N from a magic which held $M motionless.",
			    FALSE, ch, 0, victim, TO_ROOM);
			act("$n's blow shatters the magic paralyzing you!", FALSE, ch, 0, victim,
			    TO_VICT);
			act("Your blow disrupts the magic keeping $N frozen.", FALSE, ch, 0, victim,
			    TO_CHAR);
			for (af = victim->affected; af; af = next_af)
			{
				next_af = af->next;
				if (af->bitvector2 & AFF2_MINOR_PARALYSIS)
					affect_remove(victim, af);
			}
			REMOVE_BIT(victim->specials.affected_by2, AFF2_MINOR_PARALYSIS);
		}

		/* if char is bound there is a chance the dam cut the binding */
		if (IS_AFFECTED(victim, AFF_BOUND))
		{
			if (number(0, 100) <= dam)
			{
				/* ok char if free */
				REMOVE_BIT(victim->specials.affected_by, AFF_BOUND);
				act("$N's bindings are cut free!", FALSE, ch, 0, victim,
				    TO_NOTVICT);
				act("Your blow cuts through $N's bindings.", FALSE, ch, 0, victim,
				    TO_CHAR);
				send_to_char(
					"Your bindings are cut from the damage, you're free!\r\n",
					victim);
			}
		}

		/* make mirror images disappear */
		if (victim && IS_NPC(victim) && !(flags & RAWDAM_NOKILL) &&
		    GET_VNUM(victim) == 250 && (GET_HIT(victim) < 15))
		{
			act("Upon being struck, $n disappears into thin air.", TRUE, victim, 0, 0,
			    TO_ROOM);
			extract_char(victim);
			return DAM_VICTDEAD;
		}

		if (victim && IS_DISGUISE(victim))
		{
			if ((victim->disguise.hit < 0) || affected_by_spell(victim, SPELL_MIRAGE))
				remove_disguise(victim, TRUE);
			else if (victim)
				victim->disguise.hit -= (int)dam;
		}

		if (victim && affected_by_spell(victim, SPELL_VITAL_INTERCESSION))
			vital_intercession_heal(victim, dam, SPELL_VITAL_INTERCESSION);
		if (victim && affected_by_spell(victim, SPELL_HOLY_INTERCESSION))
			vital_intercession_heal(victim, dam, SPELL_HOLY_INTERCESSION);

		if (new_stat == STAT_DEAD)
		{
			P_char killer = NULL;

			if (ch && IS_NPC(ch))
			{
				if (GET_MASTER(ch) && IS_PC(GET_MASTER(ch)))
					killer = GET_MASTER(ch);
			}
			else
			{
				killer = ch;
			}

			if (!affected_by_spell(victim, TAG_PVPDELAY) && IS_PC(victim))
			{
				char bufpc[MAX_STRING_LENGTH],
					portal_description[MAX_STRING_LENGTH];

				// send_to_char("no pvp here! die and make portal\r\n", victim);
				P_obj portal = read_object(400220, VIRTUAL);
				if (portal)
				{
					portal->value[0] = world[victim->in_room].number;
					snprintf(bufpc, MAX_STRING_LENGTH, "%s %s",
						 GET_NAME(victim), "corpseportal portal");
					portal->name = str_dup(bufpc);
					snprintf(portal_description, MAX_STRING_LENGTH, "%s %s&n",
						 portal->description, GET_NAME(victim));
					set_long_description(portal, portal_description);
					set_short_description(portal, portal_description);
					obj_to_room(portal, real_room(400000));
				}
			}
			if (messages && victim && killer && IS_PC(victim) &&
			    opposite_racewar(killer, victim) && !IS_TRUSTED(killer) &&
			    !IS_TRUSTED(victim) && (messages->type & 0xff000000))
			{
				DestroyStuff(victim,
					     static_cast<int>((messages->type & 0xff000000) >> 24));
				remove_soulbind(victim);
			}
		}

		if (new_stat < STAT_SLEEPING)
		{
			switch (new_stat)
			{
			case STAT_DEAD:
				break;
			case STAT_DYING:
				act("$n is mortally wounded, and will die soon, if not aided.",
				    TRUE, victim, 0, 0, TO_ROOM);
				if (!number(0, 1))
				{
					act("&+rYou feel your pulse begin to slow and you realize you are mortally wounded...&n",
					    FALSE, victim, 0, 0, TO_CHAR);
				}
				else
				{
					act("&+rYour consciousness begins to fade in and out as your mortality slips away.....&n",
					    FALSE, victim, 0, 0, TO_CHAR);
				}
				break;
			case STAT_INCAP:
				act("$n is incapacitated and will slowly die, if not aided.", TRUE,
				    victim, 0, 0, TO_ROOM);
				if (!number(0, 1))
				{
					act("&+RYou watch as the world spins around your gruesomely cut body.\r\n"
					    "&+RYou feel your strength wane, leaving you for the carrion crawlers to\r\n"
					    "&+Rdevour.&n",
					    FALSE, victim, 0, 0, TO_CHAR);
				}
				else
				{
					act("&+RYour blood rushes out of your veins, as you slowly run out of life.\r\n"
					    "&+RBlood pours from your many serious wounds and your strength fails.\r\n"
					    "&+RYou realize this may have been your last battle, and prepare for oblivion.&n",
					    FALSE, victim, 0, 0, TO_CHAR);
				}
				break;
			}
			if (new_stat != STAT_DEAD)
			{
				StartRegen(victim, regen_resource::hit);
				StartRegen(victim, regen_resource::ward);
			}
		}
		else
		{
			StartRegen(victim, regen_resource::hit);
			StartRegen(victim, regen_resource::ward);
			max_hit = GET_MAX_HIT(victim);

			if (dam > GET_HIT(victim) && ch != victim)
			{
				act("&=LCYIKES!&n&-L  Another hit like that, and you've had it!!",
				    FALSE, victim, 0, 0, TO_CHAR);
			}
			else if (dam > (max_hit / 10) && ch != victim)
			{
				act("&+MOUCH!&n  That really did &+MHURT!&n", FALSE, victim, 0, 0,
				    TO_CHAR);
			}

			if (GET_HIT(victim) < (max_hit / 8) && ch != victim)
			{
				send_to_char(
					"You wish that your wounds would stop &+RBLEEDING&n so much!\r\n",
					victim);
			}

			if (GET_HIT(victim) < 2 && new_stat > STAT_INCAP && number(0, 1))
				Stun(victim, ch, PULSE_VIOLENCE, FALSE);
		}

		/* new, unless vicious, no auto attacks on helpless targets */

		if (!IS_AWAKE(victim) || (GET_HIT(victim) < -2) || IS_IMMOBILE(victim))
		{
			if (IS_FIGHTING(victim))
				stop_fighting(victim);
			StopMercifulAttackers(victim);
		}

		update_pos(victim);

		if (GET_HIT(victim) < 0 && affected_by_spell(victim, TAG_BUILDING))
		{
			debug("%s killed building %s.", J_NAME(ch), J_NAME(victim));
			new_stat = STAT_DEAD;
		}

		if (new_stat == STAT_DEAD)
		{
			room = ch->in_room;

			die(victim, ch);
			ch = find_character_by_runtime_id(ch_runtime_id);
			if (!is_char_in_room(ch, room))
				return DAM_BOTHDEAD;
			else
				return DAM_VICTDEAD;
		}

		if (IS_MINOTAUR(victim) && !number(0, 5) &&
		    (GET_HIT(victim) < GET_MAX_HIT(victim) / 6) &&
		    !affected_by_spell(victim, SKILL_BERSERK))
		{
			berserk(victim, 6 * PULSE_VIOLENCE);
		}

		if (GET_CLASS(victim, CLASS_BERSERKER) &&
		    (GET_HIT(victim) < GET_MAX_HIT(victim) / 4) &&
		    !affected_by_spell(victim, SKILL_BERSERK))
		{
			berserk(victim, 10 * PULSE_VIOLENCE);
		}

		/*
			   if (GET_SPEC(victim, CLASS_BERSERKER, SPEC_RAGELORD)
			   && (GET_HIT(victim) < GET_MAX_HIT(victim) / 2))
			   if (!affected_by_spell(victim, SKILL_BERSERK))
			   {
			   int duration = 3 * (MAX(24, (GET_CHAR_SKILL(victim, SKILL_BERSERK) + GET_LEVEL(victim))));
			   berserk(victim, MAX(duration, 8 * PULSE_VIOLENCE));
			   }
			   */

		if (has_innate(victim, INNATE_EMBRACE_DEATH))
			do_innate_embrace_death(victim);

		return DAM_NONEDEAD;
	}
	return 0;
}

/*
	 * this performs an attack of ch on victim using weapon currently
	 * wielded as primary.
	 * the function performs check if attacker missed,
	 * the hit cannot be dodged/parried/blocked since those are tested in
	 * pv_common.
	 *
	 * TO DO: (These might be done already)
	 *   1) Add return value similar to proposed for damage
	 *   2) Make hit take slot with weapon as an argument or weapon itself
	 *        to avoid silly swapping.
	 */
bool hit(P_char ch, P_char victim, P_obj weapon, int *damAccumulator)
{
	// Death teardown can clear player storage. Prove membership before liveness or skill reads.
	if (!ch || !victim || !char_in_list(ch) || !char_in_list(victim) || !IS_ALIVE(ch) ||
	    !IS_ALIVE(victim))
		return FALSE;

	P_char tch, mount, gvict;
	int msg, to_hit, diceroll, wpn_skill, sic, tmp, wpn_skill_num;
	double dam;
	int room, pos;
	int vs_skill = GET_CHAR_SKILL(ch, SKILL_VICIOUS_STRIKE);
	struct affected_type ir;
	char attacker_msg[512];
	char victim_msg[512];
	char room_msg[512];
	struct damage_messages messages;
	int blade_skill, chance;
	static bool vicious_hit = false;
	int devcrit = number(1, 100);

#ifdef FIGHT_DEBUG
	char buf[512];
#endif

	if (!victim || !IS_ALIVE(ch))
		return FALSE;

	if (IS_AFFECTED(ch, AFF_BOUND))
	{
		send_to_char("Your binds are too tight for that!\r\n", ch);
		return FALSE;
	}

	if (IS_IMMOBILE(ch))
	{
		send_to_char("Ugh, you are unable to move!\r\n", ch);
		return FALSE;
	}

	if (GET_STAT(victim) == STAT_DEAD)
	{
		send_to_char("aww, leave them alone, they are dead already.\r\n", ch);
		statuslog(AVATAR, "%s hitting dead %s", GET_NAME(ch), GET_NAME(victim));
		return FALSE;
	}

	if (ch->in_room != victim->in_room || ch->specials.z_cord != victim->specials.z_cord)
	{
		send_to_char("Who?\r\n", ch);
		return FALSE;
	}

	room = ch->in_room;
	mount = get_linked_char(victim, LNK_RIDING);

	if (mount)
	{
		if (GET_CHAR_SKILL(victim, SKILL_MOUNTED_COMBAT) &&
		    (notch_skill(victim, SKILL_MOUNTED_COMBAT,
				 get_property("skill.notch.defensive", 17)) ||
		     GET_CHAR_SKILL_P(victim, SKILL_MOUNTED_COMBAT) * 0.3 > number(0, 100)))
		{
			return hit(ch, mount, weapon);
		}
		/*else if (is_natural_mount(victim, mount) && (GET_LEVEL(victim) * 0.3 > number(0, 100))) // for natural mounts skill is equal to level
			  {
			  return hit(ch, mount, weapon);
			  }*/
	}

	if (!can_hit_target(ch, victim))
	{
		send_to_char("Seems that it's too crowded!\r\n", ch);
		return FALSE;
	}

	if (weapon && weapon->type != ITEM_WEAPON)
		weapon = NULL;

	if (plushit_blocks(
		    ch, victim,
		    weapon)) /* AFF3_SILVER / AFF3_PLUS* enforcement; no-op unless enabled in lib/duris.properties */
		return FALSE;

	if ((IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim) && !IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE))
	{
		if (on_front_line(ch))
		{
			if (!on_front_line(victim) && !(weapon && IS_REACH_WEAPON(weapon)))
			{
				act("$N tries to attack $n but can't quite reach!", TRUE, victim, 0,
				    ch, TO_NOTVICT);
				act("You try to attack $n but can't quite reach!", TRUE, victim, 0,
				    ch, TO_VICT);
				act("$N tries to attack you but can't quite reach!", TRUE, victim,
				    0, ch, TO_CHAR);
				return FALSE;
			}
		}
		else
		{
			send_to_char("Sorry, you can't quite reach them!\r\n", ch);
			return FALSE;
		}
	}

	if (weapon)
		msg = get_weapon_msg(weapon);
	else if (IS_NPC(ch) && ch->only.npc->attack_type)
		msg = ch->only.npc->attack_type;
	else
		msg = MSG_HIT;

	wpn_skill_num = required_weapon_skill(weapon);
	wpn_skill = (IS_PC(ch) || IS_AFFECTED(ch, AFF_CHARM)) ? GET_CHAR_SKILL(ch, wpn_skill_num) :
								MIN(100, GET_LEVEL(ch) * 2);
	// Thieves only get 1h slash for shortswords.
	if (wpn_skill_num == SKILL_1H_SLASHING && GET_SPEC(ch, CLASS_ROGUE, SPEC_THIEF) &&
	    weapon->value[0] != WEAPON_SHORTSWORD)
	{
		wpn_skill = 0;
	}
	to_hit = chance_to_hit(ch, victim, wpn_skill, weapon);

	diceroll = number(1, 100);

	/* Making this linear.  might should be less than linear. - Lohrr
		  int rollmod = 6; //statupdate2013 - drannak
		  if (GET_C_INT(ch) < 90)
		    rollmod = 7;
		  else if (GET_C_INT(ch) > 140)
		    rollmod = 4;
		  //an increased change to critical hit if affected by rage
		  // if (affected_by_spell(ch, SKILL_RAGE))
		  //   diceroll -= (GET_CHAR_SKILL(ch, SKILL_RAGE) / 10);

		  if (affected_by_spell(ch, SKILL_RAGE))
		    rollmod -= 1;

		  int critroll = (int) (GET_C_INT(ch) / rollmod);
		*/
	// At 100 int : 5% crit, at 200 int : 25% crit
	int critroll = CRITRATE(ch);
	if (critroll >= number(1, 100))
		sic = -1;
	else if (diceroll < 96)
		sic = 0;
	// Fumble
	else
		sic = 1;

	/* if (diceroll < 5) //crit
		   sic = -1;
		   else if (diceroll < 96) //fumble
		   sic = 0;
		   else
		   sic = 1;*/

	if ((sic == -1) && has_innate(victim, INNATE_AMORPHOUS_BODY))
	{
		act("You try to find a vital spot on your enemy, but $S body is too amorphous!",
		    FALSE, ch, NULL, victim, TO_CHAR);
		act("$e tried to find a vital spot on you, but your body is too amorphous for that!",
		    FALSE, ch, NULL, victim, TO_VICT);
		sic = 0;
	}

	wpn_skill = BOUNDED(GET_LEVEL(ch) / 2, wpn_skill, 95);

	// Crit to miss if they fail a weaponskill + luck check
	if (sic == -1 && number(30, 101) > wpn_skill + (GET_C_LUK(ch) / 4))
		sic = 0;
	// Fumble to miss if they make a weaponskill + agi check.
	if (sic == 1 && number(1, 101) <= wpn_skill + (GET_C_AGI(ch) / 2))
		sic = 0;

	// Quickstep stops crits if victim can respond
	bool bIsQuickStepMiss = FALSE;
	if (sic == -1 && !IS_AFFECTED2(victim, AFF2_STUNNED) && !IS_IMMOBILE(victim) &&
	    (notch_skill(victim, SKILL_QUICK_STEP,
			 get_property("skill.notch.criticalAttack", 10)) ||
	     GET_CHAR_SKILL(victim, SKILL_QUICK_STEP) > number(1, 100)))
	{
		bool qs = FALSE;
		if (GET_POS(victim) == POS_STANDING)
		{
			act("$n attempts to powerfully strike you down, but you cunningly side step and escape the attack.",
			    FALSE, ch, NULL, victim, TO_VICT);
			act("You lash out with a powerful strike, but $N cunningly sidesteps and escapes the attack.",
			    FALSE, ch, NULL, victim, TO_CHAR);
			act("$N cunningly sidesteps &n's attack.", FALSE, ch, NULL, victim,
			    TO_NOTVICT);

			qs = TRUE;
		}
		else if (GET_POS(victim) >= POS_KNEELING)
		{
			act("You tuck and roll away from the attack.", FALSE, ch, NULL, victim,
			    TO_VICT);
			act("$N tucks and rolls away from your attack.", FALSE, ch, NULL, victim,
			    TO_CHAR);
			act("$N tucks and rolls away from &n's attack.", FALSE, ch, NULL, victim,
			    TO_NOTVICT);

			qs = TRUE;
		}

		if (qs)
		{
			if (((GET_LEVEL(victim) / 5) + STAT_INDEX(GET_C_AGI(victim))) >
			    number(0, 40))
			{
				// need to know this, as quick-step crit misses won't fumble to ground
				bIsQuickStepMiss = TRUE;
				sic = 1;
			}
			else
				sic = 0;
		}
	}

	/* if (sic == -1 &&
		   GET_CHAR_SKILL(ch, SKILL_SNEAKY_STRIKE) / 10 > number(0, 49) &&
		   !is_preparing_for_sneaky_strike(ch))
		   {
		   sneaky_strike(ch, victim);
		   sic = 0;
		   }*/

	if (sic == 1 && !affected_by_spell(ch, SPELL_COMBAT_MIND))
	{
		switch (number(1, 5))
		{
		case 1:
			if (weapon && GET_LEVEL(ch) > 1 &&
			    !IS_SET(weapon->extra_flags, ITEM_NODROP))
			{
				if (bIsQuickStepMiss && economic_gameplay_authority::active() &&
				    IS_PC(ch))
					send_to_char(
						"You stumble, but keep hold of your weapon.\r\n",
						ch);
				else if (bIsQuickStepMiss)
				{
					for (pos = 0; pos < MAX_WEAR; pos++)
						if (ch->equipment[pos] == weapon)
							break;
					P_obj weap = pos < MAX_WEAR ? unequip_char(ch, pos) : NULL;
					if (weap)
					{
						act("&-L&+YYou swing at your foe _really_ badly, losing control of your&n $q&-L&+Y!\r\n",
						    FALSE, ch, weap, victim, TO_CHAR);
						act("$n stumbles with $s attack, losing control of $s weapon!",
						    TRUE, ch, 0, 0, TO_ROOM);
						obj_to_char(weap, ch);
					}
					char_light(ch);
					room_light(ch->in_room, REAL);
				}
				else
					forced_weapon_drop(ch, weapon,
							   forced_weapon_drop_cause::combat_fumble);
			}
			else
			{
				send_to_char("You stumble, but recover in time!\r\n", ch);
			}
			return FALSE;
		case 2:
			stop_fighting(ch);
			tch = get_random_char_in_room(ch->in_room, NULL, 0);
			if (tch != ch)
			{
				act("$n stumbles, and jabs at $N!", TRUE, ch, 0, tch, TO_NOTVICT);
				act("You stumble in your attack, and jab at $N!", TRUE, ch, 0, tch,
				    TO_CHAR);
				act("$n stumbles, and jabs at YOU!!", TRUE, ch, 0, tch, TO_VICT);
				return FALSE;
			}
			else
			{
				act("$n stumbles, hitting $mself!", TRUE, ch, 0, tch, TO_NOTVICT);
				act("You stumble in your attack, and hit yourself!", TRUE, ch, 0,
				    tch, TO_CHAR);
				return FALSE;
			}
			break;
		default:
			break;
		}
	}

	if (IS_GRAPPLED(victim))
	{
		gvict = grapple_attack_check(victim);
		if (gvict && (gvict != ch))
		{
			chance = grapple_misfire_chance(ch, gvict, 0);
			if (number(1, 100) <= chance)
			{
				victim = gvict;
				act("$n's attack misses $s target and hits $N instead!", TRUE, ch,
				    0, victim, TO_NOTVICT);
				act("Your hold causes you to get in the way of $n's attack!", TRUE,
				    ch, 0, victim, TO_VICT);
				act("Your attack misses your target and hits $N instead!", TRUE, ch,
				    0, victim, TO_CHAR);
			}
		}
	}

	if (!IS_FIGHTING(ch))
		set_fighting(ch, victim);

	/* The blind and eyeless cannot see "flashing lights." - Lucrot */
	if (IS_AFFECTED4(victim, AFF4_DAZZLER) && !IS_AFFECTED5(ch, AFF5_DAZZLEE) &&
	    !NewSaves(ch, SAVING_SPELL, 5) && !has_innate(ch, INNATE_EYELESS) &&
	    !IS_AFFECTED(ch, AFF_BLIND) && !IS_GREATER_RACE(victim))
	{
		struct affected_type af;

		send_to_char(
			"&+YSpa&+Wrk&+Ys&+L...  &=LWflashing lights&n&+L...  &nyou cannot concentrate!\r\n",
			ch);
		act("$n suddenly appears a bit confused!", TRUE, ch, 0, 0, TO_ROOM);
		act("You watch in glee as $N looks dazzled.", FALSE, victim, 0, ch, TO_CHAR);

		memset(&af, 0, sizeof(af));
		af.type = SPELL_DAZZLE;
		af.bitvector5 = AFF5_DAZZLEE;
		af.duration = PULSE_VIOLENCE * 3;
		af.flags = AFFTYPE_SHORT;
		affect_to_char_with_messages(ch, &af, "You no longer see spots.", NULL);
	}

	// Reflections/images never actually hit (even on a crit):
	if (IS_NPC(ch) && GET_VNUM(ch) == 250)
	{
		to_hit = 0;
		sic = 0;
	}

	if (diceroll >= to_hit && sic != -1)
	{
		act("$n misses $N.", FALSE, ch, NULL, victim, TO_NOTVICT | ACT_NOTTERSE);
		act("You miss $N.", FALSE, ch, NULL, victim, TO_CHAR | ACT_NOTTERSE);
		act("$n misses you.", FALSE, ch, NULL, victim, TO_VICT | ACT_NOTTERSE);
		// damage tier going to 0 due to miss.
		if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE))
			get_spell_from_char(ch, SPELL_CEGILUNE_BLADE)->modifier = 0;
		remember(victim, ch);
		attack_back(ch, victim, TRUE);
		return FALSE;
	}

	/***** we managed to hit the opponent *****/
	if (weapon && (IS_SET(weapon->extra2_flags, ITEM2_BLESS)) && !number(0, 99))
	{
		act("A &+Cblessed glow&n around your $q fades.", FALSE, ch, weapon, 0, TO_CHAR);
		affect_from_obj(weapon, SPELL_BLESS);
		REMOVE_BIT(weapon->extra2_flags, ITEM2_BLESS);
	}

	if (!weapon && affected_by_spell(ch, SPELL_VAMPIRIC_TOUCH) && !IS_UNDEADRACE(victim) &&
	    !IS_CONSTRUCT(victim) && !NewSaves(victim, SAVING_PARA, 0))
	{
		const uint64_t actor_runtime_id = ch->runtime_id;
		act("You touch $N with your bare hands, draining $S life force.", FALSE, ch, 0,
		    victim, TO_CHAR);
		act("$n touches you with $s bare hands, draining your life force.", FALSE, ch, 0,
		    victim, TO_VICT);
		damage(ch, victim, to_hit, SPELL_VAMPIRIC_TOUCH);
		ch = find_character_by_runtime_id(actor_runtime_id);
		if (!ch || !IS_ALIVE(ch))
			return FALSE;
		affect_from_char(ch, SPELL_VAMPIRIC_TOUCH);
		vamp(ch, to_hit, static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		return FALSE;
	}

	if (sic == -1)
	{
		if (GET_SPEC(victim, CLASS_BERSERKER, SPEC_RAGELORD) &&
		    IS_AFFECTED2(victim, AFF2_FLURRY))
		{
			send_to_char(
				"&+rYour RaGe overwhelms you, making you blind to the enemies battering assaults.\r\n",
				victim);
			sic = 0;
		}
	}

	if (sic == -1)
	{
		if (GET_CHAR_SKILL(victim, SKILL_INDOMITABLE_RAGE) > number(50, 160) &&
		    affected_by_spell(victim, SKILL_BERSERK))
		{
			act("$n&+W's critical hit sends you into a &+rpure &+Rberserk &+rtrance! &+RROARRRRRRR!&n\r\n",
			    TRUE, ch, 0, victim, TO_VICT);
			act("$N lets out a fearsome &+RROAR&n!\r\n", TRUE, ch, 0, victim, TO_CHAR);
			sic = 0;

			bzero(&ir, sizeof(ir));
			ir.type = SKILL_INDOMITABLE_RAGE;
			ir.duration = 5;
			ir.flags = AFFTYPE_SHORT;
			// hitroll
			ir.modifier = 5;
			ir.location = APPLY_HITROLL;
			affect_to_char(ch, &ir);
			// damroll
			ir.modifier = 5;
			ir.location = APPLY_DAMROLL;
			affect_to_char(ch, &ir);
			// save fear
			ir.modifier = -5;
			ir.location = APPLY_SAVING_FEAR;
			affect_to_char(ch, &ir);
		}
	}

	if ((sic == -1) && (GET_CHAR_SKILL(ch, SKILL_DEVASTATING_CRITICAL) > devcrit))
	{
		send_to_char("&=LWYou score a DEVASTATING HIT!!!!!&N\r\n", ch);
		make_bloodstain(ch);
	}
	else if (sic == -1 && (!GET_CLASS(ch, CLASS_MONK) || GET_LEVEL(ch) <= 50))
	{
		send_to_char("&=LWYou score a CRITICAL HIT!!!!!&N\r\n", ch);

		if (!number(0, 9))
			make_bloodstain(victim);
	}
	// Monks > 50 get crit message when target is invalid for pressure points
	else if (sic == -1 && GET_CLASS(ch, CLASS_MONK) && GET_LEVEL(ch) > 50)
	{
		if (!IS_HUMANOID(victim) || IS_UNDEADRACE(victim) || IS_ANGEL(victim) ||
		    IS_GREATER_RACE(victim) || IS_ELITE(victim))
		{
			send_to_char("&=LWYou score a CRITICAL HIT!!!!!&N\r\n", ch);
			if (!number(0, 9))
				make_bloodstain(victim);
		}
	}

	if (sic == -1 && (notch_skill(ch, SKILL_CRITICAL_ATTACK,
				      get_property("skill.notch.criticalAttack", 10)) ||
			  (1 * GET_CHAR_SKILL(ch, SKILL_CRITICAL_ATTACK)) > number(1, 100)))
	{
		const attack_continuation critical_continuation =
			begin_attack_continuation(ch, victim, weapon);
		critical_attack(ch, victim, msg);
		const attack_continuation_result after_critical =
			check_attack_continuation(critical_continuation);
		if (!after_critical.can_continue())
			return TRUE;
		ch = after_critical.actor;
		victim = after_critical.target;
		weapon = after_critical.weapon;
	}

	if (has_innate(ch, INNATE_BATTLE_FRENZY) && !number(0, 20) && IS_HUMANOID(victim))
	{
		const attack_continuation frenzy_continuation =
			begin_attack_continuation(ch, victim, weapon);
		const int frenzy_result = battle_frenzy(ch, victim);
		if (frenzy_result != DAM_NONEDEAD)
			return FALSE;
		const attack_continuation_result after_frenzy =
			check_attack_continuation(frenzy_continuation);
		if (!after_frenzy.can_continue())
			return FALSE;
		ch = after_frenzy.actor;
		victim = after_frenzy.target;
		weapon = after_frenzy.weapon;
	}

	if (GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK) > 0 &&
	    !affected_by_spell(ch, SKILL_WHIRLWIND) && !vicious_hit &&
	    GET_POS(ch) == POS_STANDING && GET_RACE(victim) != RACE_CONSTRUCT)
	{
		if (notch_skill(ch, SKILL_VICIOUS_ATTACK,
				get_property("skill.notch.offensive.auto", 4)) ||
		    0.1 * GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK) > number(0, 100))
		{
			act("$n slips beneath $N's guard dealing a vicious attack!!", TRUE, ch, 0,
			    victim, TO_NOTVICT);
			act("You slip beneath $N's guard dealing a vicious attack!", TRUE, ch, 0,
			    victim, TO_CHAR);
			act("$n slips beneath your guard, dealing you a vicious attack!", TRUE, ch,
			    0, victim, TO_VICT);

			const attack_continuation continuation =
				begin_attack_continuation(ch, victim, weapon);
			vicious_hit = TRUE;
			hit(ch, victim, weapon);
			vicious_hit = FALSE;
			const attack_continuation_result checked =
				check_attack_continuation(continuation);
			if (!checked.can_continue())
				return FALSE;
			ch = checked.actor;
			victim = checked.target;
			weapon = checked.weapon;
		}
	}

	/* calculate the damage */

	if (IS_NPC(ch))
	{
		dam = dice(ch->points.damnodice, ch->points.damsizedice);
	}
	else if (GET_CLASS(ch, CLASS_MONK))
	{
		dam = MonkDamage(ch);
	}
	else if (GET_CLASS(ch, CLASS_PSIONICIST) && affected_by_spell(ch, SPELL_COMBAT_MIND))
	{
		dam = dice(ch->points.damnodice, ch->points.damsizedice);
	}
	else
	{
		dam = number(1, 4); /* 1d4 dam with bare hands */
		// Those with unarmed damage get an additional 4 damage with skill at 100.
		if (GET_CHAR_SKILL(ch, SKILL_UNARMED_DAMAGE))
			dam += GET_CHAR_SKILL(ch, SKILL_UNARMED_DAMAGE) / 25;
	}

	if (weapon)
	{
		if (IS_SET(weapon->extra_flags, ITEM_TWOHANDS))
		{
			if (IS_PC(ch) || IS_PC_PET(ch))
			{
				dam = static_cast<double>(dam_factor[DF_TWOHANDED_MODIFIER]) *
				      dice(weapon->value[1], weapon->value[2]);
			}
			else
			{
				dam += static_cast<double>(dam_factor[DF_TWOHANDED_MODIFIER]) *
				       dice(weapon->value[1], weapon->value[2]);
			}
		}
		else
		{
			if (IS_PC(ch) || IS_PC_PET(ch))
				dam = dice(weapon->value[1], weapon->value[2]);
			else
				dam += dice(weapon->value[1], weapon->value[2]);
		}

		dam *= dam_factor[DF_WEAPON_DICE];
		/* Guess healing blade is out.
			if( IS_SWORD(weapon) && affected_by_spell(ch, SPELL_HEALING_BLADE) )
			{
			  vamp(ch, number(1, 4), GET_MAX_HIT(ch));
			}
			*/
	}

	tmp = TRUE_DAMROLL(ch) * dam_factor[DF_DAMROLL_MOD];
	// Randomize a bit by dropping between 0 and 10% of the damage incurred via damroll mods.
	tmp = (tmp * number(90, 100)) / 100;
	dam += tmp;

	if (sic == -1 && GET_CHAR_SKILL(ch, SKILL_DEVASTATING_CRITICAL) > devcrit)
	{
		// Caps at 200% - 267% more damage
		dam = (int)(dam * (300 + GET_CHAR_SKILL(ch, SKILL_DEVASTATING_CRITICAL)) / 150);
	}
	else if (sic == -1)
	{
		dam = (int)(dam * 2.0);
	}

	// What's this diceroll < level-40 ?  Monks get additional crits?
	if (GET_CLASS(ch, CLASS_MONK) && GET_LEVEL(ch) > 30 &&
	    (sic == -1 || diceroll < GET_LEVEL(ch) - 40))
	{
		if (sic != -1)
		{
			dam *= 1.5;
		}
		// Monk special critical hits don't work against a variety of victims.
		else if (IS_HUMANOID(victim) && !IS_UNDEADRACE(victim) && !IS_ANGEL(victim) &&
			 !IS_GREATER_RACE(victim) && !IS_ELITE(victim) &&
			 monk_critic(ch, victim, damAccumulator))
		{
			return FALSE;
		}
	}

	/* WTF is this BS?  Randomly drop damage by up to 70%?!?
		 *   Commenting this out atm, although it does randomize damage a good bit.
		 * It would be of better use in randomizing the damroll damage instead of both dice and damroll.
		 *   And so, I'm utilizing a similar approach above in the damroll damage.
		// Weapon skill check, used to be offense.
		if( sic != -1 )
		{
		  dam = (int) (dam * number(3, 10) / 10);
		}
		*/

	if (has_divine_force(ch))
		dam *= get_property("damage.modifier.divineforce", 1.250);

	/* Just commenting this out.. it's handled down below in minotaur_race_proc().
		// Dropped this to 30sec and made it not stack.
		if( (GET_RACE(victim) == RACE_MINOTAUR) && !number(0, 25) && !affected_by_spell(ch, TAG_MINOTAUR_RAGE) )
		{
		  struct affected_type af;
		  act("&+LAs $n&+L strikes you, the power of your &+rance&+Lstor&+rs&+L fill you with &+rR&+RAG&+RE&+L!&n", TRUE, ch, 0, victim, TO_VICT);
		  act("&+LAs &n$n&+L strikes $N&+L, the power of &n$N's&+L &+rance&+Lstor&+rs&+L fill them with &+rR&+RAG&+RE&+L!&n", TRUE, ch, 0, victim, TO_NOTVICT);

		  memset(&af, 0, sizeof(af));
		  af.type = TAG_MINOTAUR_RAGE;
		  af.location = APPLY_COMBAT_PULSE;
		  af.modifier = -1;
		  af.duration = 30;
		  af.flags = AFFTYPE_SHORT;
		  affect_to_char(victim, &af);

		  memset(&af, 0, sizeof(af));
		  af.type = TAG_MINOTAUR_RAGE;
		  af.location = APPLY_SPELL_PULSE;
		  af.modifier = -1;
		  af.duration = 30;
		  af.flags = AFFTYPE_SHORT;
		  affect_to_char(victim, &af);
		}
		*/

	// Replaced with innate intercept: intercept_defensiveproc below.
	if (has_innate(victim, INNATE_INTERCEPT) && intercept_defensiveproc(victim, ch))
	{
		// Not sure this is necesary, but if the attack is intercepted.. it shoiuld not complete, right?
		return FALSE;
	}

	if (has_innate(ch, INNATE_MELEE_MASTER))
		dam *= dam_factor[DF_MELEEMASTERY];

	messages = {};
	messages.attacker = attacker_msg;
	messages.victim = victim_msg;
	messages.room = room_msg;
	messages.obj = weapon;

	if (vs_skill > 0)
	{
		if (GET_CHAR_SKILL(ch, SKILL_ANATOMY) > 0)
			vs_skill += (int)(GET_CHAR_SKILL(ch, SKILL_ANATOMY) / 2);

		if (IS_NPC(ch) && IS_ELITE(ch))
			vs_skill += number(25, 100);
	}

	// PC 15% max per attack with 100 anatomy.
	if (vs_skill > 0 &&
	    (notch_skill(ch, SKILL_VICIOUS_STRIKE,
			 get_property("skill.notch.offensive.vicious.strike", 17)) ||
	     vs_skill > number(1, 1000)))
	{
		if (IS_PC(ch))
		{
			dam += static_cast<double>(
				       get_property("damage.modifier.vicious.strike", 1.050)) *
			       GET_CHAR_SKILL(ch, SKILL_VICIOUS_STRIKE);
		}
		else if (IS_ELITE(ch))
		{
			dam += (int)GET_LEVEL(ch) * 1.5;
		}
		else if (IS_GREATER_RACE(ch))
		{
			dam += GET_LEVEL(ch) * 1.25;
		}
		else if (IS_NPC(ch))
		{
			dam += GET_LEVEL(ch);
		}

		snprintf(attacker_msg, sizeof attacker_msg,
			 "You feel a powerful rush of &+rAnG&+RE&+rr&n as your%%s %s %%s.",
			 attack_hit_text[msg].singular);
		snprintf(victim_msg, sizeof victim_msg,
			 "A sense of &+RWi&+rLD H&+RAt&+rE &nsurrounds $n as $s%%s %s %%s.",
			 attack_hit_text[msg].singular);
		snprintf(room_msg, sizeof room_msg,
			 "A sense of &+RWi&+rLD H&+RAt&+rE &nsurrounds $n as $s%%s %s %%s.",
			 attack_hit_text[msg].singular);
		messages.type = DAMMSG_HIT_EFFECT;
	}
	else if (notch_skill(victim, SKILL_BOILING_BLOOD,
			     get_property("skill.notch.defensive", 17)) ||
		 GET_CHAR_SKILL(victim, SKILL_BOILING_BLOOD) / 10 > number(1, 100))
	{
		snprintf(attacker_msg, sizeof attacker_msg,
			 "$N is so overcome with bloodlust, your %s barely grazes $M!",
			 attack_hit_text[msg].singular);
		snprintf(victim_msg, sizeof victim_msg,
			 "You are so overcome with bloodlust, $n's %s barely grazes you!",
			 attack_hit_text[msg].singular);
		snprintf(room_msg, sizeof room_msg,
			 "$N is so overcome with bloodlust, $n's %s barely grazes $M!",
			 attack_hit_text[msg].singular);
		dam = 1;
	}
	else if (get_linked_char(ch, LNK_FLANKING) == victim)
	{
		snprintf(attacker_msg, sizeof attacker_msg,
			 "You %%s as your%%s %s reaches $S unprotected flank.",
			 attack_hit_text[msg].singular);
		snprintf(victim_msg, sizeof victim_msg,
			 "$n %%s as $s%%s %s reaches your unprotected flank.",
			 attack_hit_text[msg].singular);
		snprintf(room_msg, sizeof room_msg,
			 "$n %%s as $s%%s %s reaches $S unprotected flank.",
			 attack_hit_text[msg].singular);
		messages.type = DAMMSG_EFFECT_HIT;
	}
	else if (get_linked_char(ch, LNK_CIRCLING) == victim)
	{
		snprintf(attacker_msg, sizeof attacker_msg,
			 "$N screams in pain as your %s tears into $S flesh",
			 attack_hit_text[msg].singular);
		snprintf(victim_msg, sizeof victim_msg,
			 "You scream in pain as $n's %s tears into your flesh.",
			 attack_hit_text[msg].singular);
		snprintf(room_msg, sizeof room_msg,
			 "$N screams in pain as $n's %s tears into $S flesh.",
			 attack_hit_text[msg].singular);
		messages.type = DAMMSG_EFFECT_HIT;
	}
	/* Set property skill.anatomy.ratio to determine how often to check anatomy_strike().
		 *  100 skill is approximately 4 percent for players and 5% for
		 *  elite mobs. Apr09 -Lucrot
		 */
	// 5% for elite mobs.
	else if (IS_NPC(ch) && IS_ELITE(ch) &&
		 get_property("skill.anatomy.NPC", 5.000) <= number(1, 100) &&
		 IS_HUMANOID(victim) && IS_HUMANOID(ch))
	{
		dam = anatomy_strike(ch, victim, msg, &messages, attacker_msg, victim_msg, room_msg,
				     sizeof attacker_msg, (int)dam);
	}
	else if (GET_CHAR_SKILL(ch, SKILL_ANATOMY) && IS_HUMANOID(victim) && IS_HUMANOID(ch) &&
		 GET_CHAR_SKILL(ch, SKILL_ANATOMY) / 25 >= (number(1, 100)))
	{
		dam = anatomy_strike(ch, victim, msg, &messages, attacker_msg, victim_msg, room_msg,
				     sizeof attacker_msg, (int)dam);
	}
	else
	{
		snprintf(attacker_msg, sizeof attacker_msg, "Your%%s %s %%s.",
			 attack_hit_text[msg].singular);
		snprintf(victim_msg, sizeof victim_msg, "$n's%%s %s %%s.",
			 attack_hit_text[msg].singular);
		snprintf(room_msg, sizeof room_msg, "$n's%%s %s %%s.",
			 attack_hit_text[msg].singular);
		messages.type = DAMMSG_HIT_EFFECT | DAMMSG_TERSE;
	}

	dam *= ch->specials.damage_mod;
	if (difficulty_world_npc(ch))
		dam *= difficulty_multiplier(DIFFICULTY_MOB_MELEE);

	if (GET_RACE(ch) == RACE_ORC)
		dam = orc_horde_dam_modifier(ch, dam, TRUE);
	else if (GET_RACE(victim) == RACE_ORC)
		dam = orc_horde_dam_modifier(victim, dam, FALSE);

	if (weapon && IS_SLAYING(weapon, victim))
		dam *= get_property("damage.modifier.slaying", 1.050);

	dam = BOUNDED(1, (int)dam, 32766);

	if (has_innate(victim, INNATE_WEAPON_IMMUNITY))
	{
		if (weapon)
		{
			if (!IS_SET(weapon->extra2_flags, ITEM2_MAGIC))
				dam = 1;
		}
		else if (GET_LEVEL(ch) < 51 &&
			 (!ch->equipment[WEAR_HANDS] ||
			  !IS_SET(ch->equipment[WEAR_HANDS]->extra2_flags, ITEM2_MAGIC)))
		{
			dam = 1;
		}
	}

	if (weapon && IS_PC(ch) && ilogb(dam) > number(5, 400))
	{
		const attack_continuation damage_continuation =
			begin_attack_continuation(ch, victim, weapon);
		DamageOneItem(ch, 1, weapon, FALSE);
		const attack_continuation_result after_item_damage =
			check_attack_continuation(damage_continuation);
		if (after_item_damage.can_continue())
		{
			ch = after_item_damage.actor;
			victim = after_item_damage.target;
			weapon = after_item_damage.weapon;
		}
		else if (after_item_damage.outcome == attack_continuation_outcome::weapon_changed)
		{
			weapon = nullptr;
		}
		else
		{
			return TRUE;
		}
		messages.obj = weapon;
	}

	tmp = melee_death_messages_table[2 * msg + 1].attacker ? number(0, 1) : 0;
	messages.death_attacker = melee_death_messages_table[2 * msg + tmp].attacker;
	messages.death_victim = melee_death_messages_table[2 * msg + tmp].victim;
	messages.death_room = melee_death_messages_table[2 * msg + tmp].room;

	//!!!
	// | RAWDAM_NOEXP,   // hitting yields normal exp -Odorf &messages)
	const attack_continuation melee_continuation =
		begin_attack_continuation(ch, victim, weapon);
	if (melee_damage(ch, victim, dam,
			 (msg == MSG_HIT ? PHSDAM_TOUCH : PHSDAM_HELLFIRE | PHSDAM_BATTLETIDE),
			 &messages, damAccumulator) != DAM_NONEDEAD)
	{
		return TRUE;
	}
	const attack_continuation_result after_melee =
		check_attack_continuation(melee_continuation);
	if (after_melee.can_continue())
	{
		ch = after_melee.actor;
		victim = after_melee.target;
		weapon = after_melee.weapon;
	}
	else if (after_melee.outcome == attack_continuation_outcome::weapon_changed)
	{
		weapon = nullptr;
	}
	else
	{
		return TRUE;
	}

	if (IS_NPC(victim) && (GET_POS(victim) < POS_STANDING))
	{
		do_alert(victim, 0, 0);
		do_stand(victim, 0, 0);
	}

	const attack_continuation reaver_continuation =
		begin_attack_continuation(ch, victim, weapon);
	const bool reaver_hit_handled = reaver_hit_proc(ch, victim, weapon);
	if (reaver_hit_handled)
		return TRUE;
	const attack_continuation_result after_reaver =
		check_attack_continuation(reaver_continuation);
	if (after_reaver.can_continue())
	{
		ch = after_reaver.actor;
		victim = after_reaver.target;
		weapon = after_reaver.weapon;
	}
	else if (after_reaver.outcome == attack_continuation_outcome::weapon_changed)
	{
		weapon = nullptr;
	}
	else
	{
		return TRUE;
	}

	if (affected_by_spell(ch, SPELL_DREAD_BLADE))
	{
		const attack_continuation dread_continuation =
			begin_attack_continuation(ch, victim, weapon);
		const bool dread_handled = dread_blade_proc(ch, victim);
		if (dread_handled)
			return TRUE;
		const attack_continuation_result after_dread =
			check_attack_continuation(dread_continuation);
		if (after_dread.can_continue())
		{
			ch = after_dread.actor;
			victim = after_dread.target;
			weapon = after_dread.weapon;
		}
		else if (after_dread.outcome == attack_continuation_outcome::weapon_changed)
		{
			weapon = nullptr;
		}
		else
		{
			return TRUE;
		}
	}

	if (GET_CLASS(ch, CLASS_PALADIN) && holy_weapon_proc(ch, victim))
		return TRUE;

	if (GET_RACE(ch) == RACE_MINOTAUR)
		minotaur_race_proc(ch, victim);

	if (affected_by_spell(ch, ACH_YOUSTRAHDME) && IS_UNDEADRACE(victim))
	{
		const attack_continuation lightbringer_continuation =
			begin_attack_continuation(ch, victim, weapon);
		const bool lightbringer_handled = lightbringer_proc(ch, victim, TRUE);
		if (lightbringer_handled)
			return TRUE;
		const attack_continuation_result after_lightbringer =
			check_attack_continuation(lightbringer_continuation);
		if (after_lightbringer.can_continue())
		{
			ch = after_lightbringer.actor;
			victim = after_lightbringer.target;
			weapon = after_lightbringer.weapon;
		}
		else if (after_lightbringer.outcome == attack_continuation_outcome::weapon_changed)
		{
			weapon = nullptr;
		}
		else
		{
			return TRUE;
		}
	}

	blade_skill = GET_CLASS(ch, CLASS_AVENGER) ? SKILL_HOLY_BLADE : SKILL_TAINTED_BLADE;
	if (notch_skill(ch, blade_skill, get_property("skill.notch.offensive.auto", 4)) ||
	    (GET_CHAR_SKILL(ch, blade_skill) >= number(1, 100) && !number(0, 19)))
	{
		if (tainted_blade(ch, victim))
			return TRUE;
	}

	if (weapon && weapon->value[4] != 0)
	{
		const int poison_spell = weapon->value[4];
		const attack_continuation poison_continuation =
			begin_attack_continuation(ch, victim, weapon);
		if (IS_POISON(poison_spell))
			(skills[poison_spell].spell_pointer)(10, ch, 0, 0, victim, 0);
		else
			poison_lifeleak(10, ch, 0, 0, victim, 0);
		const attack_continuation_result after_poison =
			check_attack_continuation(poison_continuation);
		if (after_poison.can_continue())
		{
			ch = after_poison.actor;
			victim = after_poison.target;
			weapon = after_poison.weapon;
			weapon->value[4] = 0; /* remove on success */
		}
		else
		{
			P_obj live_poison_weapon = nullptr;
			for (P_obj object = object_list; object; object = object->next)
				if (object == poison_continuation.weapon &&
				    object->obj_uid == poison_continuation.weapon_uid)
				{
					live_poison_weapon = object;
					break;
				}
			if (live_poison_weapon)
				live_poison_weapon->value[4] = 0;
			return TRUE;
		}
	}

	if (weapon && is_char_in_room(ch, room) && is_char_in_room(victim, room) &&
	    IS_ALIVE(victim) && (!IS_PC_PET(ch) || OBJ_VNUM(weapon) != 1251))
	{
		weapon_proc(weapon, ch, victim);
	}

	return TRUE;
}

/* old mob-based trophy; deprecated in favor of zone trophy */
/*
	   void do_trophy_mob(P_char ch, char *arg, int cmd)
	   {
	   int      real, clear_it = FALSE;
	   char     Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
	   char     Gbuf3[MAX_STRING_LENGTH];
	   P_char   who, rch;
	   struct trophy_data *tr;

	#ifndef TROPHY
	send_to_char("temp. disabled...\r\n", ch);
	return;
	#endif
	rch = GET_PLYR(ch);

	if (IS_NPC(rch))
	{
	send_to_char("DUH.\r\n", ch);
	return;
	}

	argument_interpreter(arg, Gbuf2, Gbuf3);

	if (IS_TRUSTED(rch))
	{
	if (!(who = get_char_vis(rch, Gbuf2)))
	{
	send_to_char("They don't appear to be in the game.\r\n", ch);
	return;
	}

	if (*Gbuf3 && isname("clear", Gbuf3))
	{
	struct trophy_data *temp;

	//      tr = who->only.pc->trophy;
	//      who->only.pc->trophy = NULL;
	for (; tr; tr = temp)
	{
	temp = tr->next;
	mm_release(dead_trophy_pool, tr);
	}
	send_to_char("Ok, trophy cleared.\r\n", ch);
	return;
	}
	else if( *Gbuf3 && isname("frags", Gbuf3))
	{
	show_frag_trophy(ch, who);
	return;
	}
	}
	else
	{
	who = rch;
	}

	if( isname("frags", Gbuf2) )
	{
	show_frag_trophy(ch, ch);
	}
	else
	{
	snprintf(Gbuf1, MAX_STRING_LENGTH, "&+WTrophy data:&n\r\n");
	for (tr = who->only.pc->trophy; tr; tr = tr->next)
	{
	char     let;

	if (tr->kills < 1000)
	let = 'G';
	else if (tr->kills < 2500)
	let = 'Y';
	else
	let = 'R';
	real = real_mobile0(tr->vnum);
	snprintf(Gbuf1 + strlen(Gbuf1), MAX_STRING_LENGTH - strlen(Gbuf1),
	    "&+W(&n&+%c%2d.%02d&+W)&n %s\r\n", let,
	    tr->kills / 100, tr->kills % 100,
	    real ? mob_index[real].desc2 : "Unknown");

	}
	strcat(Gbuf1, "\r\n");
	page_string(ch->desc, Gbuf1, 1);
	return;
	}

	}
	*/

bool is_nopoof(P_obj obj)
{
	return IS_ARTIFACT(obj) || isname("quest", obj->name) || isname("fun", obj->name);
}

void DestroyStuff(P_char victim, int type)
{
	P_obj item;
	int poof_chance = get_property("pvp.eq.poof.chance", 10);
	//  int poof_chance_niceq_multiplier = (int)(get_property("pvp.eq.poof.niceeq.chance.multiplier", 2));

	if (!(victim))
	{
		logit(LOG_EXIT, "destroystuff called in fight.c with no victim");
		return;
	}

	for (size_t proc_index = 0; proc_index < ARRAY_SIZE(proccing_slots); proc_index++)
	{
		item = victim->equipment[proccing_slots[proc_index]];

		if (!item || is_nopoof(item))
			continue;

		//    worn++;

		// poof_chance =
		//  MIN((int)
		//  (get_property("damage.minPoofChance", 5.) +
		//  obj_index[item->R_num].number / 5),
		//  (int) get_property("damage.maxPoofChance", 20.));

		// if(IS_SET(item->bitvector, AFF_STONE_SKIN) ||
		// IS_SET(item->bitvector, AFF_HASTE) ||
		// IS_SET(item->bitvector, AFF_AWARE) ||
		// IS_SET(item->bitvector, AFF_HIDE) ||
		// IS_SET(item->bitvector2, AFF2_FIRE_AURA) ||
		// IS_SET(item->bitvector2, AFF2_GLOBE) ||
		// IS_SET(item->bitvector3, AFF3_GR_SPIRIT_WARD) ||
		// IS_SET(item->bitvector4, AFF4_DAZZLER) ||
		// IS_SET(item->bitvector4, AFF4_HELLFIRE) ||
		// IS_SET(item->bitvector4, AFF4_REGENERATION) ||
		// obj_index[item->R_num].func.obj != NULL)
		// {
		// poof_chance = (int)(poof_chance_niceq_multiplier * poof_chance);
		// }

		if (poof_chance > number(1, 100))
		{
			//      poofed++;
			statuslog(AVATAR, "%s [%d] was destroyed.", item->short_description,
				  (item->R_num >= 0) ? obj_index[item->R_num].virtual_number : 0);
			DamageOneItem(victim, type, item, TRUE);
		}
	}

	// if(worn)
	// {
	// if(!poofed)
	// {
	// do
	// {
	// slot = number(0, sizeof(proccing_slots) / sizeof(int) - 1);
	// }
	// while (!(item = victim->equipment[proccing_slots[slot]]) ||
	// is_nopoof(item));
	// statuslog(AVATAR, "%s [%d] was destroyed.", item->short_description,
	// (item->R_num >=
	// 0) ? obj_index[item->R_num].virtual_number : 0);
	// DamageOneItem(victim, type, item, TRUE);
	// }
	// }
	// else
	// {
	// statuslog(AVATAR, "%s died with no eq.", victim->player.name);
	// }
}
