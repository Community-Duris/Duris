/* ***************************************************************************
 *  File: quest.c                                            Part of Duris *
 *  Usage: Quest procs for mobs... -DCL                                      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 *************************************************************************** */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/native_quest_consumption_capture.h"
#include "world/quest_mobile_native.h"
#include "world/quest_mobile_native_binding.h"
#include "item/item_ownership_runtime.h"
#include "item/item_movement_transaction.h"
#include "item/item_command_policy.h"
#include "economy/collector_presence.h"
#include "combat/training_dummy.h"
#include "mob/studioproc.h"
#include "economy/currency_transaction.h"
#include "persistence/persistence_checkpoint.h"
#include "persistence/persistence_mode.h"
#include "persistence/quest_reward_obligation_pipeline.h"
#include "player/player_save_pipeline.h"
#include "player/player_revision_state.h"
#include "player/player_snapshot.h"
#include "world/quest_reward_recovery.h"
#include "world/native_quest_frozen_continuation.h"
#include "world/native_quest_recovery_context.h"
#include "persistence/critical_command_coordinator.h"
#ifndef __NO_MYSQL__
#include "persistence/quest_reward_obligation_repository.h"
#include "persistence/critical_command_repository.h"
#include "persistence/economic_sql_item_transfer_transaction.h"
#include "player/player_sql_transaction_cleanup.h"
#endif
#include "world/zone_story_quest_production.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/files.h"
#include <ctype.h>
#include <climits>
#include <cerrno>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <glob.h>
#include <new>
#include "magic/spells.h"
#include "sql/sql.h"
#include "world/zone_story_quest_runtime.h"
#include <time.h>
#include <openssl/sha.h>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <array>
#include <vector>

/* external variables */

extern Skill skills[];
extern P_char character_list;
extern P_obj object_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern const char *item_types[];
extern const struct stat_data stat_factor[];
extern struct str_app_type str_app[];
extern struct zone_data *zone_table;
extern int mini_mode;
extern long new_exp_table[]; // Arih: Fixed type mismatch bug - was int, should be long

#define QUEST_FILE "areas/world.qst"
#define MINI_QUEST_FILE "areas_mini/mini.qst"

struct quest_data quest_index[MAX_QUESTS];
int number_of_quests = 0;

bool quest_completion(struct quest_complete_data *, P_char, P_char);
void give_reward(struct quest_complete_data *, P_char, P_char, uint64_t);

bool execute_quest_routine(P_char ch, int cmd)
{
	int arg;
	int qi = find_quester_id(GET_RNUM(ch));
	struct quest_msg_data *qdata = quest_index[qi].quest_message;

	if (!IS_ALIVE(ch) || IS_IMMOBILE(ch))
		return false;

	for (qdata = quest_index[qi].quest_message; qdata; qdata = qdata->next)
	{
		if (cmd == CMD_NONE)
		{
			if (sscanf(qdata->key_words, QC_ACTION " %d", &arg) == 1)
			{
				if (!number(0, (arg * WAIT_SEC) / PULSE_MOBILE))
				{
					act(qdata->message, FALSE, ch, 0, 0, TO_ROOM);
					return true;
				}
			}
			//    } else if (cmd == CMD_INCOMING) {
		}
	}

	return false;
}

int binary_search(int num, int min, int max)
{
	int mid, midn;

	if (min > max)
		return (-1);

	midn = (min + max) >> 1;
	mid = quest_index[midn].quester;

	if (num == mid)
		return (midn);
	else if (num < mid)
		return (binary_search(num, min, midn - 1));
	else
		return (binary_search(num, midn + 1, max));
}

int find_quester_id(int num)
{
	return (binary_search(num, 0, number_of_quests - 1));
}

void questcheck(P_char ch)
{
	int i;
	char tmp_buf[MAX_STRING_LENGTH];

	for (i = 1; i < number_of_quests; i++)
	{
		if (quest_index[i].quester <= quest_index[i - 1].quester)
		{
			snprintf(tmp_buf, MAX_STRING_LENGTH,
				 "Real: %d Virtual: %d is out of order with Real: %d Virtual: %d\n",
				 i, quest_index[i].quester, i - 1, quest_index[i - 1].quester);
			send_to_char(tmp_buf, ch);
		}
	}
}

bool quest_completion(struct quest_complete_data *qcp, P_char mob, P_char pl)
{
	struct goal_data *gp, *gp2;
	P_obj obj;
	int num_needed, count;

	for (gp = qcp->give; gp; gp = gp->next)
	{
		for (gp2 = gp->next, num_needed = 1; gp2; gp2 = gp2->next)
		{
			if ((gp2->goal_type == gp->goal_type) && (gp2->number == gp->number))
				num_needed++;
		}
		switch (gp->goal_type)
		{
		case QUEST_GOAL_ITEM:
			for (obj = mob->carrying, count = 0; obj; obj = obj->next_content)
				if (obj_index[obj->R_num].virtual_number == gp->number)
					count++;
			if (count < num_needed)
				return (FALSE);
			break;
		case QUEST_GOAL_ITEM_TYPE:
			for (obj = mob->carrying, count = 0; obj; obj = obj->next_content)
				if (obj->type == gp->number)
					count++;
			if (count < num_needed)
				return (FALSE);
			break;
		case QUEST_GOAL_COINS:
			if (GET_MONEY(mob) < gp->number)
				return (FALSE);
			break;
		}
	}
	//  send_to_char(qcp->message, pl);
	act(qcp->message, FALSE, mob, 0, pl, qcp->echoAll ? TO_ROOM : TO_VICT);
	for (gp = qcp->give; gp; gp = gp->next)
	{
		if (gp->goal_type == QUEST_GOAL_ITEM)
		{
			for (obj = mob->carrying; obj; obj = obj->next_content)
				if (obj_index[obj->R_num].virtual_number == gp->number)
					break;
			if (!obj)
				return (FALSE);
			obj_from_char(obj);
			extract_obj(obj, TRUE); // Arti quest item?
			obj = NULL;
		}
		if (gp->goal_type == QUEST_GOAL_COINS)
			SUB_MONEY(mob, gp->number, 0);
	}
	for (gp = qcp->give; gp; gp = gp->next)
	{
		if (gp->goal_type == QUEST_GOAL_ITEM_TYPE)
		{
			for (obj = mob->carrying; obj; obj = obj->next_content)
				if (obj->type == gp->number)
					break;
			if (!obj)
				return (FALSE);
			obj_from_char(obj);
			extract_obj(obj, TRUE); // Arti quest item?
			obj = NULL;
		}
	}
	return (TRUE);
}

struct quest_reward_recovery_attempt
{
	uint32_t player_pid = 0;
	critical_operation_id offering_operation = {};
	size_t pending_items = 0;
	player_revision_t required_save_revision = 0;
	std::vector<player_quest_xp_receipt_snapshot> expected_receipts;
	player_component_mask_t pending_save_components = 0;
	bool acknowledge = true;
	bool dispatch_complete = false;
	bool wait_for_player_save = false;
	bool failed = false;
};

std::unordered_map<std::string, quest_reward_recovery_attempt> quest_reward_recoveries;
std::unordered_map<uint64_t, std::string> quest_reward_recovery_items;

std::string quest_reward_operation_key(const critical_operation_id &operation)
{
	return std::string(reinterpret_cast<const char *>(operation.bytes.data()),
			   operation.bytes.size());
}

std::string quest_reward_entitlement_key(const critical_operation_id &operation, uint32_t pid,
					 uint32_t reward_index)
{
	std::string key = quest_reward_operation_key(operation);
	key.push_back('\0');
	for (uint32_t value : { pid, reward_index })
		for (size_t index = 0; index < sizeof(value); ++index)
			key.push_back(static_cast<char>((value >> (index * 8)) & 0xff));
	return key;
}

static P_char quest_reward_character_present(uint32_t pid)
{
	for (P_char player = character_list; player; player = player->next)
		if (IS_PC(player) && GET_PID(player) == static_cast<int>(pid))
			return player;
	return NULL;
}

static void request_quest_reward_recovery_save(const std::string &key, P_char player = NULL)
{
	const auto found = quest_reward_recoveries.find(key);
	if (found == quest_reward_recoveries.end())
		return;
	auto &attempt = found->second;
	if (!attempt.wait_for_player_save || attempt.required_save_revision ||
	    attempt.pending_items || !attempt.dispatch_complete)
		return;
	if (!player)
		player = quest_reward_character_present(attempt.player_pid);
	if (!player)
		return;
	const int room_vnum = player->in_room >= 0 && world ? world[player->in_room].number :
							      NOWHERE;
	const auto saved = attempt.expected_receipts.empty() ?
				   player_save_pipeline_request(player,
								attempt.pending_save_components,
								RENT_CRASH, room_vnum) :
				   player_save_pipeline_request_quest_xp(
					   player, attempt.pending_save_components,
					   attempt.expected_receipts.data(),
					   attempt.expected_receipts.size(), room_vnum);
	player_revision_snapshot revision = {};
	if ((saved == player_save_pipeline_result::queued ||
	     saved == player_save_pipeline_result::coalesced) &&
	    player_revision_snapshot_copy(attempt.player_pid, &revision) &&
	    revision.current_revision)
		attempt.required_save_revision = revision.current_revision;
}

void finish_quest_reward_recovery(const std::string &key)
{
	const auto found = quest_reward_recoveries.find(key);
	if (found == quest_reward_recoveries.end() || found->second.pending_items ||
	    found->second.wait_for_player_save || !found->second.dispatch_complete)
		return;
	if (!found->second.failed && found->second.acknowledge)
		quest_reward_obligation_pipeline_submit(found->second.player_pid,
							found->second.offering_operation);
	quest_reward_recoveries.erase(found);
}

void quest_reward_recovery_save_acknowledged(int pid, uint64_t revision,
					     const player_quest_xp_receipt_snapshot *receipts,
					     size_t receipt_count)
{
	if (pid <= 0 || !revision || !receipts || !receipt_count)
		return;
	for (auto current = quest_reward_recoveries.begin();
	     current != quest_reward_recoveries.end();)
	{
		auto entry = current++;
		auto &attempt = entry->second;
		if (!attempt.wait_for_player_save ||
		    attempt.player_pid != static_cast<uint32_t>(pid) ||
		    attempt.expected_receipts.empty() || revision < attempt.required_save_revision)
			continue;
		bool all_matched = true;
		for (const auto &expected : attempt.expected_receipts)
		{
			bool matched = false;
			for (size_t index = 0; index < receipt_count; ++index)
				matched = matched ||
					  (receipts[index].offering_operation.bytes ==
						   expected.offering_operation.bytes &&
					   receipts[index].reward_index == expected.reward_index &&
					   receipts[index].amount == expected.amount);
			all_matched = all_matched && matched;
		}
		if (!all_matched)
			continue;
		attempt.wait_for_player_save = false;
		finish_quest_reward_recovery(entry->first);
	}
}

bool quest_reward_recovery_pending_save_receipts(
	int pid, std::vector<player_quest_xp_receipt_snapshot> *receipts,
	player_component_mask_t *components)
{
	if (pid <= 0 || !receipts || !components)
		return false;
	std::vector<player_quest_xp_receipt_snapshot> pending;
	player_component_mask_t required = 0;
	try
	{
		for (const auto &[key, attempt] : quest_reward_recoveries)
		{
			if (attempt.player_pid != static_cast<uint32_t>(pid) ||
			    !attempt.wait_for_player_save || attempt.expected_receipts.empty())
				continue;
			if (pending.size() + attempt.expected_receipts.size() > 64)
				return false;
			pending.insert(pending.end(), attempt.expected_receipts.begin(),
				       attempt.expected_receipts.end());
			required |= attempt.pending_save_components;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*receipts = std::move(pending);
	*components = required;
	return true;
}

static void native_quest_gameplay_pulse() noexcept;

void quest_reward_recovery_pulse(void)
{
	native_quest_gameplay_pulse();
	for (auto current = quest_reward_recoveries.begin();
	     current != quest_reward_recoveries.end();)
	{
		auto entry = current++;
		auto &key = entry->first;
		auto &attempt = entry->second;
		request_quest_reward_recovery_save(key);
		if (!attempt.wait_for_player_save)
			continue;
		if (!attempt.required_save_revision)
			continue;
		// Applied XP survives admission failure and player revision release. Only
		// an exact receipt completion may remove its live replay fence.
		if (!attempt.expected_receipts.empty())
			continue;
		player_revision_snapshot revision = {};
		if (!player_revision_snapshot_copy(attempt.player_pid, &revision))
		{
			attempt.wait_for_player_save = false;
			attempt.failed = true;
			finish_quest_reward_recovery(key);
			continue;
		}
		if (revision.acknowledged_revision < attempt.required_save_revision)
			continue;
		attempt.wait_for_player_save = false;
		finish_quest_reward_recovery(key);
	}
}

void quest_reward_recovery_item_complete(P_char player, uint64_t item_uid, bool committed,
					 unsigned int)
{
	const auto item = quest_reward_recovery_items.find(item_uid);
	if (item == quest_reward_recovery_items.end())
		return;
	const std::string key = item->second;
	quest_reward_recovery_items.erase(item);
	const auto attempt = quest_reward_recoveries.find(key);
	if (attempt == quest_reward_recoveries.end())
		return;
	if (!committed)
		attempt->second.failed = true;
	if (attempt->second.pending_items)
		--attempt->second.pending_items;
	request_quest_reward_recovery_save(key, player);
	finish_quest_reward_recovery(key);
}

void quest_reward_recovery_currency_complete(P_char player, bool committed,
					     const currency_command_result &, unsigned int,
					     const uint8_t *context, size_t context_size)
{
	if (!context || context_size != sizeof(critical_operation_id))
		return;
	critical_operation_id offering_operation = {};
	memcpy(offering_operation.bytes.data(), context, offering_operation.bytes.size());
	const std::string key = quest_reward_operation_key(offering_operation);
	const auto attempt = quest_reward_recoveries.find(key);
	if (attempt == quest_reward_recoveries.end())
		return;
	if (!committed)
		attempt->second.failed = true;
	if (attempt->second.pending_items)
		--attempt->second.pending_items;
	request_quest_reward_recovery_save(key, player);
	finish_quest_reward_recovery(key);
}

void give_reward(struct quest_complete_data *qcp, P_char mob, P_char pl, uint64_t offering_uid)
{
	struct goal_data *gp;
	P_obj obj;
	int i;
	char Gbuf1[MAX_STRING_LENGTH];
	int temp = 1;

	wizlog(58, "%s has completed quest from %s [%d].", GET_NAME(pl), mob->player.short_descr,
	       GET_VNUM(mob));

	sql_log(pl, QUESTLOG, "Completed quest from %s &n[%d].", mob->player.short_descr,
		GET_VNUM(mob));

	for (gp = qcp->receive; gp; gp = gp->next)
	{
		switch (gp->goal_type)
		{
		case QUEST_GOAL_ITEM:
		{
			obj = read_object(gp->number, VIRTUAL);

			if (!obj)
			{
				logit(LOG_DEBUG, "give_reward(): obj %d not loadable", gp->number);
				break;
			}

			// No duplicating artifacts.
			if (get_artifact_data_sql(OBJ_VNUM(obj), NULL))
			{
				// It's imperative that we extract FALSE (no poofing arti data)..
				extract_obj(obj);
				break;
			}

			const bool to_player = (IS_CARRYING_N(pl) < CAN_CARRY_N(pl)) &&
					       ((total_carried_weight(pl) + GET_OBJ_WEIGHT(obj)) <
						CAN_CARRY_W(pl));
			uint32_t duplicate_ordinal = 0;
			for (struct goal_data *prior = qcp->receive; prior != gp;
			     prior = prior->next)
				if (prior->goal_type == QUEST_GOAL_ITEM &&
				    prior->number == gp->number)
					++duplicate_ordinal;
			const uint64_t source_id = quest_item_reward_source_id(
				offering_uid, gp->number, duplicate_ordinal);
			const economic_source_kind source =
				offering_uid ? economic_source_kind::quest_completion :
					       economic_source_kind{};
			const bool submitted =
				to_player ?
					item_creation_grant_submit_to_player(pl, obj, pl, NULL,
									     source, source_id) :
					item_creation_grant_submit_to_room(
						pl, obj, pl->in_room, source, nullptr, source_id);
			if (!submitted)
			{
				extract_obj(obj, FALSE);
				send_to_char(
					"The quest reward service is busy; no item was granted.\r\n",
					pl);
				break;
			}
			if (to_player)
			{
				act("$n gives you $p.", FALSE, mob, obj, pl, TO_VICT);
				act("$n gives $p to $N.", FALSE, mob, obj, pl, TO_NOTVICT);
			}
			else
			{
				act("$n places $p on the ground in front of you.", FALSE, mob, obj,
				    pl, TO_VICT);
				act("$n places $p on the ground in front of $N.", FALSE, mob, obj,
				    pl, TO_NOTVICT);
			}
			statuslog(58, "%s was rewarded %s [%d] by %s", GET_NAME(pl),
				  obj->short_description, obj_index[obj->R_num].virtual_number,
				  mob->player.short_descr);
			sql_log(pl, QUESTLOG, "Rewarded %s [%d] by %s", obj->short_description,
				obj_index[obj->R_num].virtual_number, mob->player.short_descr);
			// sql_quest_finish(pl, mob, 1, obj_index[obj->R_num].virtual_number);
			break;
		}
		case QUEST_GOAL_COINS:

			/* if( (temp = sql_quest_trophy(mob)) > 1){
				  snprintf(Gbuf1, MAX_STRING_LENGTH, "$n says 'This quest is very commonly done, reward is currently very low.'\r\n");
				  act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
			   }  else
				       */
			temp = 1;

			snprintf(Gbuf1, MAX_STRING_LENGTH, "$n gives %s to you.",
				 coin_stringv(gp->number / temp));
			act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
			act("$n gives some coins to $N.", FALSE, mob, 0, pl, TO_NOTVICT);
			statuslog(58, "%s was rewarded %d coins by %s", GET_NAME(pl),
				  gp->number / temp, mob->player.short_descr);
			sql_log(pl, QUESTLOG, "Rewarded %s &ncoins by %s",
				coin_stringv(gp->number / temp), mob->player.short_descr);
			ADD_MONEY(pl, gp->number / temp);

			// sql_quest_finish(pl, mob, 2, gp->number / temp);
			break;
		case QUEST_GOAL_SKILL:
			if (IS_NPC(pl))
				break;

			for (i = 0; (i < MAX_SKILLS) && (i != gp->number); i++)
				;

			if (i >= MAX_SKILLS)
				break;

			if (SKILL_DATA_ALL(pl, i).rlevel[pl->player.spec] > (int)GET_LEVEL(pl))
			{
				act("$n says to you, 'Come back when you are more powerful.'",
				    FALSE, mob, 0, pl, TO_VICT);
				break;
			}
			if (pl->only.pc->skills[gp->number].learned < 1)
			{
				snprintf(Gbuf1, MAX_STRING_LENGTH, "$n teaches you '%s'.",
					 skills[gp->number].name);
				act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
				act("$n teaches $N a new skill.", FALSE, mob, 0, pl, TO_NOTVICT);
				pl->only.pc->skills[gp->number].learned = 1;
			}
			break;
		case QUEST_GOAL_EXP:
			/*
		  if( (temp = sql_quest_trophy(mob)) > 1){
		   snprintf(Gbuf1, MAX_STRING_LENGTH, "$n says 'This quest is very commonly done, reward is currently very low.'\r\n");
			   act(Gbuf1, FALSE, mob, 0, pl, TO_VICT);
			}  else temp = 1;
				*/

			// Capping exp at 1 notch per quest.
			temp = gp->number;
			if (temp > new_exp_table[GET_LEVEL(pl) + 1] / 10)
				temp = new_exp_table[GET_LEVEL(pl) + 1] / 10;
			gain_exp(pl, NULL, temp, EXP_QUEST);
			send_to_char("You gain some experience.\n", pl);
			statuslog(58, "%s was rewarded %d experience by %s", GET_NAME(pl), temp,
				  mob->player.short_descr);
			sql_log(pl, QUESTLOG, "Rewarded %d experience by %s", temp,
				mob->player.short_descr);

			if (pl->group)
			{
				for (struct group_list *gl = pl->group; gl; gl = gl->next)
				{
					if (gl->ch == pl)
						continue;
					if (!IS_PC(gl->ch))
						continue;
					if (gl->ch->in_room == pl->in_room)
					{
						temp = gp->number;
						if (temp >
						    new_exp_table[GET_LEVEL(gl->ch) + 1] / 10)
							temp = new_exp_table[GET_LEVEL(gl->ch) + 1] /
							       10;
						temp = gp->number;
						if (temp > new_exp_table[GET_LEVEL(gl->ch) + 1])
							temp = new_exp_table[GET_LEVEL(gl->ch) + 1];
						gain_exp(gl->ch, NULL, temp, EXP_QUEST);
						send_to_char("You gain some experience.\n", gl->ch);
						statuslog(58, "%s was rewarded %d experience by %s",
							  GET_NAME(gl->ch), temp,
							  mob->player.short_descr);
						sql_log(gl->ch, QUESTLOG,
							"Rewarded %d experience by %s",
							gp->number / temp, mob->player.short_descr);
					}
				}
			}

			// sql_quest_finish(pl, mob, 3, gp->number / temp);
			break;
		}
	}
}

// A private quest handoff consumes every required item in one durable commit.
// The player keeps incomplete sets, so an NPC never holds an unsaved offering.
constexpr size_t QUEST_DURABLE_MAX_OFFERINGS = 14;
constexpr size_t QUEST_DURABLE_MAX_CREDITED_PLAYERS = QUEST_REWARD_MAX_CREDITED_PIDS;
constexpr int QUEST_EXP_TABLE_ENTRIES = 63;
static P_char quest_reward_character_present(uint32_t pid);
struct quest_durable_context
{
	int quester_id;
	int completion_index;
	int room;
	uint32_t count;
	uint64_t roots[QUEST_DURABLE_MAX_OFFERINGS];
	uint64_t completed_at;
	uint32_t credited_count;
	uint32_t credited_pids[QUEST_DURABLE_MAX_CREDITED_PLAYERS];
	int32_t credited_levels[QUEST_DURABLE_MAX_CREDITED_PLAYERS];
	uint32_t party_size;
	int32_t player_level;
	int32_t player_racewar;
	int32_t strongest_party_level;
	uint64_t skill_eligibility_mask;
	char character_name[MAX_NAME_LENGTH + 1];
};
static_assert(sizeof(quest_durable_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

static bool capture_quest_credit_context(P_char actor, quest_durable_context *context)
{
	if (!actor || !context || IS_NPC(actor) || GET_PID(actor) <= 0 || IS_TRUSTED(actor) ||
	    GET_LEVEL(actor) < 0 || !GET_NAME(actor) || !*GET_NAME(actor))
		return false;
	const size_t name_length = strlen(GET_NAME(actor));
	if (name_length > MAX_NAME_LENGTH)
		return false;
	memcpy(context->character_name, GET_NAME(actor), name_length + 1);
	context->player_level = GET_LEVEL(actor);
	context->player_racewar = GET_RACEWAR(actor);
	context->strongest_party_level = context->player_level;
	context->credited_count = 1;
	context->credited_pids[0] = static_cast<uint32_t>(GET_PID(actor));
	context->credited_levels[0] = GET_LEVEL(actor);
	if (actor->group)
	{
		size_t visited = 0;
		for (group_list *member = actor->group; member; member = member->next)
		{
			if (++visited > QUEST_DURABLE_MAX_CREDITED_PLAYERS)
				return false;
			if (!member->ch || !IS_PC(member->ch) || IS_TRUSTED(member->ch) ||
			    member->ch->in_room != actor->in_room || GET_PID(member->ch) <= 0 ||
			    quest_reward_character_present(
				    static_cast<uint32_t>(GET_PID(member->ch))) != member->ch)
				continue;
			const uint32_t pid = static_cast<uint32_t>(GET_PID(member->ch));
			bool duplicate = false;
			for (size_t index = 0; index < context->credited_count; ++index)
				duplicate = duplicate || context->credited_pids[index] == pid;
			if (duplicate)
				continue;
			if (context->credited_count == QUEST_DURABLE_MAX_CREDITED_PLAYERS)
				return false;
			context->credited_pids[context->credited_count++] = pid;
			context->credited_levels[context->credited_count - 1] =
				GET_LEVEL(member->ch);
			context->strongest_party_level =
				std::max(context->strongest_party_level, GET_LEVEL(member->ch));
		}
	}
	context->party_size = context->credited_count;
	return true;
}

// This snapshot travels with the consumed-item command. Keep the numeric reward
// terms independent of the mutable quest catalog for later recovery.
static bool capture_quest_offering_continuation(
	P_char mob, P_char actor, int quester_id, int completion_index,
	const quest_complete_data *completion, quest_durable_context &context,
	item_transfer_continuation *continuation, const std::string *original_definition = nullptr,
	const quest_reward_continuation *fee_terms = nullptr)
{
	if (!mob || !actor || !completion || !continuation || !world || mob->in_room < 0 ||
	    GET_PID(actor) <= 0 || (context.count == 0 && !fee_terms) ||
	    context.count > QUEST_DURABLE_MAX_OFFERINGS)
		return false;
	const std::string *definition_id =
		original_definition ? original_definition :
				      zone_story_quest_production::definition_id_for(completion);
	const int zone_number = zone_story_quest_production::zone_for_giver_vnum(GET_VNUM(mob));
	if (!definition_id || definition_id->empty() ||
	    definition_id->size() > QUEST_REWARD_MAX_DEFINITION_ID_BYTES || zone_number <= 0 ||
	    !context.credited_count ||
	    context.credited_count > QUEST_DURABLE_MAX_CREDITED_PLAYERS ||
	    context.party_size != context.credited_count || !context.character_name[0])
		return false;
	size_t reward_count = 0;
	for (const goal_data *reward = completion->receive; reward; reward = reward->next)
	{
		if (++reward_count > 64 ||
		    (reward->goal_type != QUEST_GOAL_ITEM &&
		     reward->goal_type != QUEST_GOAL_COINS &&
		     reward->goal_type != QUEST_GOAL_SKILL && reward->goal_type != QUEST_GOAL_EXP))
			return false;
	}
	uint32_t xp_award_count = 0;
	for (const goal_data *reward = completion->receive; reward; reward = reward->next)
		if (reward->goal_type == QUEST_GOAL_EXP)
		{
			if (context.credited_count >
			    QUEST_REWARD_MAX_CREDITED_PIDS - xp_award_count)
				return false;
			xp_award_count += context.credited_count;
		}
	const size_t name_length = strlen(context.character_name);
	const size_t bytes = 36 + context.count * sizeof(uint64_t) + sizeof(uint32_t) +
			     reward_count * sizeof(uint32_t) * 4 + 6 * sizeof(uint32_t) +
			     context.credited_count * sizeof(uint32_t) + sizeof(uint32_t) +
			     name_length + sizeof(uint32_t) + definition_id->size() +
			     sizeof(uint32_t) +
			     static_cast<size_t>(xp_award_count) * 3 * sizeof(uint32_t);
	if (bytes > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	continuation->kind = item_transfer_continuation_kind::quest_offering;
	try
	{
		continuation->data.assign(bytes, 0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	auto put32 = [&](size_t offset, uint32_t value)
	{
		for (size_t byte = 0; byte < sizeof(value); ++byte)
			continuation->data[offset + byte] =
				static_cast<uint8_t>(value >> (byte * 8));
	};
	auto put64 = [&](size_t offset, uint64_t value)
	{
		for (size_t byte = 0; byte < sizeof(value); ++byte)
			continuation->data[offset + byte] =
				static_cast<uint8_t>(value >> (byte * 8));
	};
	put32(0, 5);
	put32(4, static_cast<uint32_t>(GET_PID(actor)));
	put32(8, static_cast<uint32_t>(quester_id));
	put32(12, static_cast<uint32_t>(completion_index));
	put32(16, static_cast<uint32_t>(GET_VNUM(mob)));
	put32(20, static_cast<uint32_t>(world[mob->in_room].number));
	if (!context.completed_at)
		return false;
	put64(24, context.completed_at);
	put32(32, context.count);
	size_t offset = 36;
	for (size_t index = 0; index < context.count; ++index, offset += sizeof(uint64_t))
		put64(offset, context.roots[index]);
	put32(offset, static_cast<uint32_t>(reward_count));
	offset += sizeof(uint32_t);
	context.skill_eligibility_mask = 0;
	size_t reward_index = 0;
	for (const goal_data *reward = completion->receive; reward; reward = reward->next)
	{
		put32(offset, static_cast<uint32_t>(reward->goal_type));
		put32(offset + sizeof(uint32_t), static_cast<uint32_t>(reward->number));
		uint32_t flags = 0;
		if (reward->goal_type == QUEST_GOAL_SKILL && reward->number >= 0 &&
		    reward->number < MAX_SKILLS && actor->player.spec <= MAX_SPEC &&
		    SKILL_DATA_ALL(actor, reward->number).rlevel[actor->player.spec] <=
			    context.player_level)
		{
			flags |= QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION;
			context.skill_eligibility_mask |= UINT64_C(1) << reward_index;
		}
		uint32_t frozen_xp = 0;
		if (reward->goal_type == QUEST_GOAL_EXP)
		{
			for (size_t recipient = 0; recipient < context.credited_count; ++recipient)
			{
				const int32_t level = context.credited_levels[recipient];
				if (level < 0 || level + 1 >= QUEST_EXP_TABLE_ENTRIES)
					return false;
				const int64_t next_level_xp = new_exp_table[level + 1];
				const int64_t level_cap = recipient == 0 ? next_level_xp / 10 :
									   next_level_xp;
				if (level_cap <= 0)
					return false;
				const uint32_t amount = static_cast<uint32_t>(
					std::min<int64_t>(reward->number, level_cap));
				if (!amount)
					return false;
				if (recipient == 0)
					frozen_xp = amount;
			}
		}
		put32(offset + 2 * sizeof(uint32_t), flags);
		put32(offset + 3 * sizeof(uint32_t), frozen_xp);
		offset += sizeof(uint32_t) * 4;
		++reward_index;
	}
	put32(offset, static_cast<uint32_t>(zone_number));
	put32(offset + 4, static_cast<uint32_t>(context.player_level));
	put32(offset + 8, static_cast<uint32_t>(context.player_racewar));
	put32(offset + 12, context.party_size);
	put32(offset + 16, static_cast<uint32_t>(context.strongest_party_level));
	put32(offset + 20, context.credited_count);
	offset += 6 * sizeof(uint32_t);
	for (size_t index = 0; index < context.credited_count; ++index, offset += sizeof(uint32_t))
		put32(offset, context.credited_pids[index]);
	put32(offset, static_cast<uint32_t>(name_length));
	offset += sizeof(uint32_t);
	memcpy(continuation->data.data() + offset, context.character_name, name_length);
	offset += name_length;
	put32(offset, static_cast<uint32_t>(definition_id->size()));
	offset += sizeof(uint32_t);
	memcpy(continuation->data.data() + offset, definition_id->data(), definition_id->size());
	offset += definition_id->size();
	put32(offset, xp_award_count);
	// Store exact awards in reward-major order so future party changes cannot
	// alter the recipients or their progression cap.
	size_t awards_written = 0;
	const size_t awards_offset = offset + sizeof(uint32_t);
	for (size_t reward = 0; reward < reward_count; ++reward)
	{
		const goal_data *goal = completion->receive;
		for (size_t prior = 0; prior < reward; ++prior)
			goal = goal->next;
		if (goal->goal_type != QUEST_GOAL_EXP)
			continue;
		for (size_t recipient = 0; recipient < context.credited_count; ++recipient)
		{
			const int32_t level = context.credited_levels[recipient];
			const int64_t next_level_xp = new_exp_table[level + 1];
			const int64_t cap = recipient == 0 ? next_level_xp / 10 : next_level_xp;
			const uint32_t amount =
				static_cast<uint32_t>(std::min<int64_t>(goal->number, cap));
			const size_t record =
				awards_offset + awards_written++ * 3 * sizeof(uint32_t);
			put32(record, context.credited_pids[recipient]);
			put32(record + sizeof(uint32_t), static_cast<uint32_t>(reward));
			put32(record + 2 * sizeof(uint32_t), amount);
		}
	}
	if (awards_written != xp_award_count)
		return false;
	if (fee_terms)
	{
		if (context.count || fee_terms->version != 6 || fee_terms->root_count)
			return false;
		constexpr size_t tail_bytes =
			4 + 16 + ECONOMIC_SOURCE_EVENT_BYTES + 8 + 16 + ITEM_TRANSFER_RESULT_BYTES;
		if (continuation->data.size() > ITEM_TRANSFER_CONTINUATION_MAX_BYTES - tail_bytes)
			return false;
		put32(0, 6);
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
		if (economic_source_event_encode(fee_terms->action_source, &source) !=
		    economic_accounting_error::ok)
			return false;
		auto &out = continuation->data;
		out.insert(out.end(), { 'Q', 'R', 'F', '6' });
		out.insert(out.end(), fee_terms->action_operation.bytes.begin(),
			   fee_terms->action_operation.bytes.end());
		out.insert(out.end(), source.begin(), source.end());
		const size_t native_offset = out.size();
		out.resize(native_offset + 8);
		put64(native_offset, fee_terms->action_mobile_instance_id);
		out.insert(out.end(), fee_terms->triggering_acceptance.bytes.begin(),
			   fee_terms->triggering_acceptance.bytes.end());
		out.insert(out.end(), fee_terms->original_acceptance_result.begin(),
			   fee_terms->original_acceptance_result.end());
		quest_reward_continuation decoded;
		if (!quest_fee_reward_continuation_decode(out.data(), out.size(), &decoded))
			return false;
	}
	return true;
}

static P_obj quest_object_by_uid(uint64_t uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == uid)
			return object;
	return NULL;
}

static struct quest_complete_data *quest_completion_by_index(const quest_durable_context &context)
{
	if (context.quester_id < 0 || context.quester_id >= number_of_quests ||
	    context.completion_index < 0)
		return NULL;
	struct quest_complete_data *completion = quest_index[context.quester_id].quest_complete;
	for (int index = 0; completion && index < context.completion_index; ++index)
		completion = completion->next;
	return completion;
}

static P_char quest_mobile_for(const quest_durable_context &context)
{
	if (context.quester_id < 0 || context.quester_id >= number_of_quests)
		return NULL;
	for (P_char mobile = character_list; mobile; mobile = mobile->next)
		if (IS_NPC(mobile) && GET_RNUM(mobile) == quest_index[context.quester_id].quester &&
		    mobile->in_room == context.room)
			return mobile;
	return NULL;
}

static void finish_quest_reward(struct quest_complete_data *completion, P_char mob, P_char pl,
				uint64_t offering_uid, uint64_t completed_at = 0,
				int32_t completion_room_vnum = 0)
{
	const int room_vnum = completion_room_vnum ?
				      completion_room_vnum :
				      (world && pl->in_room >= 0 ? world[pl->in_room].number : 0);
	const int64_t completion_time = completed_at ? static_cast<int64_t>(completed_at) :
						       static_cast<int64_t>(time(NULL));
	const std::string tracking_id = "legacy-offering-v1-" + std::to_string(offering_uid);
	std::string tracking_error;
	if (!zone_story_quest_runtime::record_legacy_completion(
		    pl, completion, room_vnum, completion_time, &tracking_error,
		    offering_uid ? std::string_view(tracking_id) : std::string_view{}))
		logit(LOG_DEBUG, "zone-story quest completion was not recorded: %s",
		      tracking_error.c_str());
	give_reward(completion, mob, pl, offering_uid);
	if (!completion->disappear)
		return;
	act(completion->disappear_message, FALSE, mob, 0, pl,
	    completion->echoAll ? TO_ROOM : TO_VICT);
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (mob->equipment[slot])
			extract_obj(unequip_char(mob, slot), TRUE);
	while (mob->carrying)
		extract_obj(mob->carrying, TRUE);
	extract_char(mob);
}

static bool publish_quest_offering(const critical_operation_id & /*operation_id*/, P_char actor,
				   bool committed, const item_transfer_result &, unsigned int,
				   const uint8_t *encoded, size_t encoded_size)
{
	quest_durable_context context = {};
	if (!actor || !encoded || encoded_size != sizeof(context))
		return false;
	memcpy(&context, encoded, sizeof(context));
	if (!context.count || context.count > QUEST_DURABLE_MAX_OFFERINGS ||
	    !quest_completion_by_index(context))
		return false;
	if (!committed)
	{
		send_to_char("Your quest offering could not be accepted. Please try again.\r\n",
			     actor);
		return true;
	}
	if (!quest_mobile_for(context))
		return false;
	P_obj roots[QUEST_DURABLE_MAX_OFFERINGS] = {};
	size_t present = 0;
	for (size_t index = 0; index < context.count; ++index)
	{
		roots[index] = quest_object_by_uid(context.roots[index]);
		if (roots[index])
		{
			if (!OBJ_CARRIED_BY(roots[index], actor))
				return false;
			++present;
		}
	}
	if (present && present != context.count)
		return false;
	for (size_t index = 0; index < context.count; ++index)
		if (roots[index])
			extract_obj(roots[index], TRUE);
	mark_player_dirty_components(GET_PID(actor), PLAYER_COMPONENT_STATUS |
							     PLAYER_COMPONENT_EQUIPMENT |
							     PLAYER_COMPONENT_INVENTORY);
	return true;
}

static void complete_quest_offering(P_char actor, bool committed,
				    const item_transfer_result &transfer_result, unsigned int,
				    const uint8_t *encoded, size_t encoded_size)
{
	quest_durable_context context = {};
	if (!committed || !actor || !encoded || encoded_size != sizeof(context))
		return;
	memcpy(&context, encoded, sizeof(context));
	struct quest_complete_data *completion = quest_completion_by_index(context);
	P_char mob = quest_mobile_for(context);
	if (!completion)
	{
		logit(LOG_DEBUG, "committed quest offering could not find its quest definition");
		send_to_char("Your committed quest reward is still pending recovery.\r\n", actor);
		return;
	}
	if (mob)
		act(completion->message, FALSE, mob, 0, actor,
		    completion->echoAll ? TO_ROOM : TO_VICT);
	bool recoverable_rewards = !critical_operation_id_is_zero(transfer_result.operation_id);
	for (const goal_data *reward = completion->receive; reward; reward = reward->next)
		recoverable_rewards = recoverable_rewards &&
				      (reward->goal_type == QUEST_GOAL_ITEM ||
				       reward->goal_type == QUEST_GOAL_COINS ||
				       reward->goal_type == QUEST_GOAL_SKILL ||
				       reward->goal_type == QUEST_GOAL_EXP);
	if (recoverable_rewards)
	{
		quest_reward_continuation continuation = {};
		const std::string *definition_id =
			zone_story_quest_production::definition_id_for(completion);
		if (!definition_id)
		{
			send_to_char("Your committed quest reward is still pending recovery.\r\n",
				     actor);
			return;
		}
		continuation.version = 5;
		continuation.player_pid = static_cast<uint32_t>(GET_PID(actor));
		continuation.quester_id = static_cast<uint32_t>(context.quester_id);
		continuation.completion_index = static_cast<uint32_t>(context.completion_index);
		const int giver_vnum =
			mob ? GET_VNUM(mob) :
			      (context.quester_id >= 0 && context.quester_id < number_of_quests &&
					       quest_index[context.quester_id].quester >= 0 ?
				       mob_index[quest_index[context.quester_id].quester]
					       .virtual_number :
				       0);
		if (giver_vnum <= 0)
		{
			send_to_char("Your committed quest reward is still pending recovery.\r\n",
				     actor);
			return;
		}
		continuation.mobile_vnum = static_cast<uint32_t>(giver_vnum);
		continuation.room_vnum = static_cast<uint32_t>(world[context.room].number);
		continuation.completed_at = context.completed_at;
		continuation.zone_number =
			static_cast<uint32_t>(zone_story_quest_production::zone_for_giver_vnum(
				static_cast<int>(continuation.mobile_vnum)));
		continuation.player_level = context.player_level;
		continuation.player_racewar = context.player_racewar;
		continuation.party_size = context.party_size;
		continuation.strongest_party_level = context.strongest_party_level;
		continuation.credited_count = context.credited_count;
		continuation.character_name = context.character_name;
		continuation.definition_id = *definition_id;
		continuation.root_count = context.count;
		for (size_t index = 0; index < context.count; ++index)
			continuation.roots[index] = context.roots[index];
		for (size_t index = 0; index < context.credited_count; ++index)
			continuation.credited_pids[index] = context.credited_pids[index];
		size_t reward_index = 0;
		for (const goal_data *reward = completion->receive; reward;
		     reward = reward->next, ++reward_index)
		{
			if (continuation.reward_count == continuation.rewards.size())
				return;
			const uint32_t flags =
				context.skill_eligibility_mask & (UINT64_C(1) << reward_index) ?
					QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION :
					0;
			uint32_t frozen_xp = 0;
			if (reward->goal_type == QUEST_GOAL_EXP)
				for (size_t recipient = 0; recipient < context.credited_count;
				     ++recipient)
				{
					if (continuation.xp_award_count ==
					    continuation.xp_awards.size())
						return;
					const int32_t level = context.credited_levels[recipient];
					if (level < 0 || level + 1 >= QUEST_EXP_TABLE_ENTRIES)
						return;
					const int64_t cap = recipient == 0 ?
								    new_exp_table[level + 1] / 10 :
								    new_exp_table[level + 1];
					const uint32_t amount = static_cast<uint32_t>(
						std::min<int64_t>(reward->number, cap));
					if (recipient == 0)
						frozen_xp = amount;
					continuation.xp_awards[continuation.xp_award_count++] = {
						context.credited_pids[recipient],
						static_cast<uint32_t>(reward_index), amount
					};
				}
			continuation.rewards[continuation.reward_count++] = {
				static_cast<uint32_t>(reward->goal_type),
				static_cast<uint32_t>(reward->number), flags, frozen_xp
			};
		}
		quest_reward_recover_pending(actor, transfer_result.operation_id, continuation);
		for (size_t index = 0; index < continuation.xp_award_count; ++index)
		{
			const auto &award = continuation.xp_awards[index];
			if (award.recipient_pid == continuation.player_pid)
				continue;
			if (P_char recipient = quest_reward_character_present(award.recipient_pid))
				quest_reward_recover_xp_entitlement(
					recipient, transfer_result.operation_id, continuation,
					award.reward_index, award.amount);
		}
		if (!mob || !completion->disappear)
			return;
		act(completion->disappear_message, FALSE, mob, 0, actor,
		    completion->echoAll ? TO_ROOM : TO_VICT);
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (mob->equipment[slot])
				extract_obj(unequip_char(mob, slot), TRUE);
		while (mob->carrying)
			extract_obj(mob->carrying, TRUE);
		extract_char(mob);
		return;
	}
	if (!mob)
	{
		send_to_char("Your committed quest reward is still pending recovery.\r\n", actor);
		return;
	}
	finish_quest_reward(completion, mob, actor, context.roots[0], context.completed_at,
			    world[context.room].number);
}

// The existing journal owns both original full commands. Literal obligation rows
// never authorize fee reward dispatch; this friend borrows their real carrier.
bool quest_reward_obligation_native_fee_owner::verify_in_transaction(
	MYSQL *connection, const critical_operation_id &action,
	std::span<const uint8_t> literal) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)action;
	(void)literal;
	return false;
#else
	try
	{
		critical_native_recovery_envelope actual, parent;
		quest_reward_continuation terms;
		item_transfer_payload payload;
		native_quest_recovery_context accepted, released;
		if (!connection ||
		    !quest_reward_continuation_decode(literal.data(), literal.size(), &terms) ||
		    terms.version != 6 || terms.action_operation.bytes != action.bytes ||
		    !critical_native_quest_continuation_owner::copy_fee_obligation_context(
			    action, literal, &actual, &parent) ||
		    actual.command.operation_id.bytes != action.bytes ||
		    !item_transfer_command_decode_payload(actual.command, &payload) ||
		    !payload.native_cost.fee_only ||
		    actual.phase != critical_native_recovery_phase::continuation_pending ||
		    native_quest_recovery_context_decode(actual.command, actual.attachment,
							 &released) !=
			    player_snapshot_codec_result::ok ||
		    released.publication_stage !=
			    native_quest_recovery_publication_stage::physically_proven ||
		    !released.receipt.present || released.receipt.error_code ||
		    (released.receipt.outcome != critical_apply_outcome::applied &&
		     released.receipt.outcome != critical_apply_outcome::already_applied) ||
		    payload.continuation.data.size() != literal.size() ||
		    !std::equal(literal.begin(), literal.end(),
				payload.continuation.data.begin()) ||
		    !quest_fee_reward_trigger_binding_valid(terms, parent.command) ||
		    native_quest_recovery_context_decode(parent.command, parent.attachment,
							 &accepted) !=
			    player_snapshot_codec_result::ok ||
		    !accepted.receipt.present || accepted.receipt.error_code ||
		    (accepted.receipt.outcome != critical_apply_outcome::applied &&
		     accepted.receipt.outcome != critical_apply_outcome::already_applied) ||
		    accepted.receipt.result_size != ITEM_TRANSFER_RESULT_BYTES ||
		    !std::equal(terms.original_acceptance_result.begin(),
				terms.original_acceptance_result.end(),
				accepted.receipt.result_payload.begin()))
			return false;
		critical_completion completion{};
		completion.operation_id = actual.command.operation_id;
		completion.outcome = released.receipt.outcome;
		completion.durable_revision = released.receipt.durable_revision;
		completion.error_code = released.receipt.error_code;
		completion.failure_stage = released.receipt.failure_stage;
		completion.result_size = released.receipt.result_size;
		completion.result_payload = released.receipt.result_payload;
		return economic_sql_native_fee_verify_receipt_in_transaction(
			       connection, actual.command, completion) == 0 &&
		       economic_sql_native_fee_verify_acceptance_in_transaction(
			       connection, actual.command, parent.command,
			       terms.original_acceptance_result) == 0 &&
		       economic_sql_native_fee_verify_obligation_in_transaction(
			       connection, actual.command, literal) == 0;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_reward_obligation_native_fee_owner::ready(
	const critical_operation_id &action, const quest_reward_continuation &terms) noexcept
{
#ifdef __NO_MYSQL__
	(void)action;
	(void)terms;
	return false;
#else
	if (!nevent_is_game_thread() || terms.version != 6)
		return false;
	try
	{
		std::vector<uint8_t> literal;
		if (!quest_fee_reward_continuation_encode(terms, &literal))
			return false;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			proven = !mysql_real_query(connection, "START TRANSACTION", 17) &&
				 verify_in_transaction(connection, action, literal) &&
				 transaction.same_session();
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		return proven && cleanup.rollback_confirmed && !cleanup.cleanup_error &&
		       cleanup.disposition == player_sql_cleanup_disposition::idle_verified &&
		       transaction.same_session();
	}
	catch (...)
	{
		return false;
	}
#endif
}

void quest_reward_recover_pending(P_char player, const critical_operation_id &offering_operation,
				  const quest_reward_continuation &continuation,
				  uint64_t xp_applied_mask, uint64_t economic_applied_mask,
				  bool economic_history_verified)
{
	if (!player || IS_NPC(player) || GET_PID(player) <= 0 ||
	    static_cast<uint32_t>(GET_PID(player)) != continuation.player_pid ||
	    critical_operation_id_is_zero(offering_operation) ||
	    (!continuation.root_count && continuation.version != 6) ||
	    continuation.root_count > continuation.roots.size())
		return;
	if (continuation.version == 6 &&
	    !quest_reward_obligation_native_fee_owner::ready(offering_operation, continuation))
		return;
	if (!economic_history_verified)
	{
		send_to_char(
			"Your pending quest reward is held for ownership and payment review.\r\n",
			player);
		return;
	}
	uint64_t economic_slots = 0;
	for (size_t index = 0; index < continuation.reward_count; ++index)
		if (continuation.rewards[index].type == QUEST_GOAL_ITEM ||
		    continuation.rewards[index].type == QUEST_GOAL_COINS)
			economic_slots |= UINT64_C(1) << index;
	if (economic_applied_mask & ~economic_slots)
		return;
	const std::string key = quest_reward_operation_key(offering_operation);
	if (quest_reward_recoveries.find(key) != quest_reward_recoveries.end())
		return;
	bool all_effects_supported = true;
	for (size_t index = 0; index < continuation.reward_count; ++index)
		all_effects_supported =
			all_effects_supported &&
			(continuation.rewards[index].type == QUEST_GOAL_ITEM ||
			 continuation.rewards[index].type == QUEST_GOAL_COINS ||
			 (continuation.rewards[index].type == QUEST_GOAL_SKILL &&
			  continuation.version >= 3) ||
			 (continuation.rewards[index].type == QUEST_GOAL_EXP &&
			  ((continuation.version >= 4 && continuation.credited_count == 1 &&
			    continuation.credited_pids[0] == continuation.player_pid) ||
			   (continuation.version >= 5 && continuation.credited_count > 1))));
	quest_durable_context context = {};
	context.quester_id = static_cast<int>(continuation.quester_id);
	context.completion_index = static_cast<int>(continuation.completion_index);
	context.count = continuation.root_count;
	context.completed_at = continuation.completed_at;
	for (size_t index = 0; index < continuation.root_count; ++index)
		context.roots[index] = continuation.roots[index];
	struct quest_complete_data *completion = quest_completion_by_index(context);
	bool tracking_complete = false;
	const std::string tracking_id =
		continuation.version == 6 ?
			"native-fee-action-v6-" + quest_reward_operation_key(offering_operation) :
			"legacy-offering-v1-" + std::to_string(continuation.roots[0]);
	std::string tracking_error;
	if (continuation.version >= 2 && !continuation.definition_id.empty())
	{
		std::vector<uint32_t> credited_pids;
		try
		{
			credited_pids.assign(continuation.credited_pids.begin(),
					     continuation.credited_pids.begin() +
						     continuation.credited_count);
		}
		catch (const std::bad_alloc &)
		{
			tracking_error = "unable to allocate frozen quest-credit recipients";
		}
		if (!credited_pids.empty())
			tracking_complete =
				zone_story_quest_runtime::record_authoritative_completion(
					continuation.definition_id,
					static_cast<int32_t>(continuation.zone_number),
					continuation.player_pid, credited_pids,
					static_cast<int32_t>(continuation.room_vnum),
					static_cast<int64_t>(continuation.completed_at),
					continuation.character_name, continuation.player_level,
					continuation.player_racewar, true, continuation.party_size,
					continuation.strongest_party_level, &tracking_error,
					tracking_id);
		if (!tracking_complete)
			logit(LOG_DEBUG,
			      "pending zone-story quest completion could not be recovered: %s",
			      tracking_error.c_str());
	}
	else if (continuation.version == 1 && completion)
	{
		bool exact_terms = true;
		size_t reward_index = 0;
		for (const goal_data *goal = completion->receive; goal; goal = goal->next)
		{
			if (reward_index >= continuation.reward_count ||
			    continuation.rewards[reward_index].type !=
				    static_cast<uint32_t>(goal->goal_type) ||
			    continuation.rewards[reward_index].number !=
				    static_cast<uint32_t>(goal->number))
				exact_terms = false;
			++reward_index;
		}
		exact_terms = exact_terms && reward_index == continuation.reward_count;
		if (exact_terms)
		{
			tracking_complete = zone_story_quest_runtime::record_legacy_completion(
				player, completion, static_cast<int32_t>(continuation.room_vnum),
				static_cast<int64_t>(continuation.completed_at), &tracking_error,
				tracking_id);
			if (!tracking_complete)
				logit(LOG_DEBUG,
				      "pending zone-story quest completion could not be recovered: %s",
				      tracking_error.c_str());
		}
	}
	if (quest_reward_recoveries.size() >= 1024)
		return;
	quest_reward_recovery_attempt attempt;
	attempt.player_pid = continuation.player_pid;
	attempt.offering_operation = offering_operation;
	attempt.acknowledge = all_effects_supported && tracking_complete;
	try
	{
		quest_reward_recoveries.emplace(key, attempt);
	}
	catch (const std::bad_alloc &)
	{
		return;
	}
	bool skill_changed = false;
	std::vector<player_quest_xp_receipt_snapshot> xp_receipts;
	bool xp_receipts_available = true;
	try
	{
		xp_receipts.reserve(continuation.reward_count);
		quest_reward_recoveries[key].expected_receipts.reserve(continuation.reward_count);
		std::vector<player_quest_xp_receipt_snapshot> already_pending;
		player_component_mask_t pending_components = 0;
		size_t added = 0;
		for (size_t index = 0; index < continuation.reward_count; ++index)
			if (continuation.rewards[index].type == QUEST_GOAL_EXP &&
			    !(xp_applied_mask & (UINT64_C(1) << index)))
				++added;
		if (!quest_reward_recovery_pending_save_receipts(GET_PID(player), &already_pending,
								 &pending_components) ||
		    already_pending.size() + added > 64)
		{
			xp_receipts_available = false;
			quest_reward_recoveries[key].failed = true;
		}
	}
	catch (const std::bad_alloc &)
	{
		xp_receipts_available = false;
		quest_reward_recoveries[key].failed = true;
	}
	for (size_t index = 0; index < continuation.reward_count; ++index)
	{
		const auto &reward = continuation.rewards[index];
		if (reward.type == QUEST_GOAL_EXP)
		{
			uint32_t amount = reward.frozen_amount;
			if (continuation.version >= 5 && continuation.credited_count > 1)
			{
				amount = 0;
				for (size_t award_index = 0;
				     award_index < continuation.xp_award_count; ++award_index)
					if (continuation.xp_awards[award_index].recipient_pid ==
						    continuation.player_pid &&
					    continuation.xp_awards[award_index].reward_index ==
						    index)
						amount = continuation.xp_awards[award_index].amount;
			}
			if (!xp_receipts_available || continuation.version < 4 || !amount ||
			    amount > INT_MAX ||
			    (continuation.credited_count == 1 &&
			     continuation.credited_pids[0] != continuation.player_pid) ||
			    (continuation.credited_count > 1 && continuation.version < 5))
			{
				quest_reward_recoveries[key].failed = true;
				continue;
			}
			if (xp_applied_mask & (UINT64_C(1) << index))
				continue;
			gain_exp(player, nullptr, static_cast<int>(amount), EXP_QUEST);
			xp_receipts.push_back(
				{ offering_operation, static_cast<uint32_t>(index), amount });
			// Capacity was reserved before gain_exp. Retain application identity
			// before capture/admission can fail; later saves collect this receipt.
			quest_reward_recoveries[key].expected_receipts.push_back(
				xp_receipts.back());
			quest_reward_recoveries[key].wait_for_player_save = true;
			quest_reward_recoveries[key].pending_save_components =
				PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
			continue;
		}
		if (reward.type != QUEST_GOAL_SKILL ||
		    !(reward.flags & QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION))
			continue;
		if (reward.number >= MAX_SKILLS)
		{
			quest_reward_recoveries[key].failed = true;
			continue;
		}
		if (player->only.pc->skills[reward.number].learned < 1)
		{
			player->only.pc->skills[reward.number].learned = 1;
			skill_changed = true;
		}
	}
	if (skill_changed || !xp_receipts.empty())
	{
		player_component_mask_t components = 0;
		if (skill_changed)
			components |= PLAYER_COMPONENT_SKILLS;
		if (!xp_receipts.empty())
			components |= PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
		quest_reward_recoveries[key].pending_save_components = components;
		quest_reward_recoveries[key].wait_for_player_save = true;
	}
	for (size_t index = 0; index < continuation.reward_count; ++index)
	{
		const auto &reward = continuation.rewards[index];
		if (reward.type == QUEST_GOAL_COINS)
		{
			if (economic_applied_mask & (UINT64_C(1) << index))
				continue;
			critical_operation_id child_operation = {};
			if (!critical_operation_id_derive(
				    offering_operation, QUEST_REWARD_CURRENCY_OPERATION_DOMAIN,
				    static_cast<uint32_t>(index + 1), &child_operation))
			{
				quest_reward_recoveries[key].failed = true;
				continue;
			}
			++quest_reward_recoveries[key].pending_items;
			if (!currency_transaction_submit_wallet_value_identified(
				    player, child_operation, static_cast<int64_t>(reward.number),
				    currency_reason_type::wallet_reward,
				    static_cast<int64_t>(index + 1), critical_source_site::recovery,
				    critical_deadline_class::recovery,
				    quest_reward_recovery_currency_complete,
				    offering_operation.bytes.data(),
				    offering_operation.bytes.size()))
			{
				--quest_reward_recoveries[key].pending_items;
				quest_reward_recoveries[key].failed = true;
			}
			continue;
		}
		if (reward.type != QUEST_GOAL_ITEM)
			continue;
		if (economic_applied_mask & (UINT64_C(1) << index))
			continue;
		P_obj object = read_object(static_cast<int>(reward.number), VIRTUAL);
		if (!object)
		{
			quest_reward_recoveries[key].failed = true;
			continue;
		}
		if (get_artifact_data_sql(OBJ_VNUM(object), NULL))
		{
			extract_obj(object);
			quest_reward_recoveries[key].failed = true;
			continue;
		}
		uint32_t duplicate_ordinal = 0;
		for (size_t prior = 0; prior < index; ++prior)
			if (continuation.rewards[prior].type == QUEST_GOAL_ITEM &&
			    continuation.rewards[prior].number == reward.number)
				++duplicate_ordinal;
		const uint64_t source_id =
			continuation.version == 6 ?
				quest_item_reward_source_id(continuation, index) :
				quest_item_reward_source_id(continuation.roots[0],
							    static_cast<int>(reward.number),
							    duplicate_ordinal);
		const uint64_t item_uid = object->obj_uid;
		try
		{
			quest_reward_recovery_items.emplace(item_uid, key);
			++quest_reward_recoveries[key].pending_items;
		}
		catch (const std::bad_alloc &)
		{
			quest_reward_recovery_items.erase(item_uid);
			extract_obj(object, FALSE);
			quest_reward_recoveries[key].failed = true;
			continue;
		}
		if (!item_creation_grant_submit_to_player_before_entry_with_completion(
			    player, object, player, quest_reward_recovery_item_complete,
			    economic_source_kind::quest_completion, source_id))
		{
			quest_reward_recovery_items.erase(item_uid);
			--quest_reward_recoveries[key].pending_items;
			extract_obj(object, FALSE);
			quest_reward_recoveries[key].failed = true;
		}
	}
	quest_reward_recoveries[key].dispatch_complete = true;
	request_quest_reward_recovery_save(key, player);
	if (continuation.reward_count)
		send_to_char(
			all_effects_supported && tracking_complete ?
				"Your committed quest reward is being recovered.\r\n" :
				"Part of your committed quest reward is still pending recovery.\r\n",
			player);
	else if (!all_effects_supported || !tracking_complete)
		send_to_char("Your committed quest reward is still pending recovery.\r\n", player);
	finish_quest_reward_recovery(key);
}

void quest_reward_recover_xp_entitlement(P_char player,
					 const critical_operation_id &offering_operation,
					 const quest_reward_continuation &continuation,
					 uint32_t reward_index, uint32_t amount)
{
	if (!player || IS_NPC(player) || GET_PID(player) <= 0 || !amount || amount > INT_MAX ||
	    static_cast<uint32_t>(GET_PID(player)) == continuation.player_pid ||
	    critical_operation_id_is_zero(offering_operation) || continuation.version < 5 ||
	    reward_index >= continuation.reward_count ||
	    continuation.rewards[reward_index].type != QUEST_GOAL_EXP ||
	    continuation.credited_count < 2)
		return;
	if (continuation.version == 6 &&
	    !quest_reward_obligation_native_fee_owner::ready(offering_operation, continuation))
		return;
	const uint32_t recipient_pid = static_cast<uint32_t>(GET_PID(player));
	bool frozen_award = false;
	for (size_t index = 0; index < continuation.xp_award_count; ++index)
		frozen_award = frozen_award ||
			       (continuation.xp_awards[index].recipient_pid == recipient_pid &&
				continuation.xp_awards[index].reward_index == reward_index &&
				continuation.xp_awards[index].amount == amount);
	if (!frozen_award)
		return;
	const std::string key =
		quest_reward_entitlement_key(offering_operation, recipient_pid, reward_index);
	if (quest_reward_recoveries.find(key) != quest_reward_recoveries.end() ||
	    quest_reward_recoveries.size() >= 1024)
		return;
	std::vector<player_quest_xp_receipt_snapshot> already_pending;
	player_component_mask_t pending_components = 0;
	if (!quest_reward_recovery_pending_save_receipts(GET_PID(player), &already_pending,
							 &pending_components) ||
	    already_pending.size() >= 64)
		return;
	quest_reward_recovery_attempt attempt;
	attempt.player_pid = recipient_pid;
	attempt.offering_operation = offering_operation;
	attempt.acknowledge = false;
	try
	{
		attempt.expected_receipts.push_back({ offering_operation, reward_index, amount });
		quest_reward_recoveries.emplace(key, std::move(attempt));
	}
	catch (const std::bad_alloc &)
	{
		return;
	}
	gain_exp(player, nullptr, static_cast<int>(amount), EXP_QUEST);
	quest_reward_recoveries[key].wait_for_player_save = true;
	quest_reward_recoveries[key].pending_save_components = PLAYER_COMPONENT_STATUS |
							       PLAYER_COMPONENT_TROPHIES;
	const player_quest_xp_receipt_snapshot receipt = { offering_operation, reward_index,
							   amount };
	const int room_vnum = player->in_room >= 0 && world ? world[player->in_room].number :
							      NOWHERE;
	const auto saved = player_save_pipeline_request_quest_xp(
		player, PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES, &receipt, 1,
		room_vnum);
	player_revision_snapshot revision = {};
	if ((saved == player_save_pipeline_result::queued ||
	     saved == player_save_pipeline_result::coalesced) &&
	    player_revision_snapshot_copy(recipient_pid, &revision) && revision.current_revision)
	{
		quest_reward_recoveries[key].required_save_revision = revision.current_revision;
	}
	quest_reward_recoveries[key].dispatch_complete = true;
	finish_quest_reward_recovery(key);
}

static bool submit_durable_quest_offering(P_char mob, P_char actor, int quester_id, P_obj offering)
{
	if (quester_id < 0 || !offering)
		return false;
	int completion_index = 0;
	bool matched_goal = false;
	bool unsupported_goal = false;
	for (struct quest_complete_data *completion = quest_index[quester_id].quest_complete;
	     completion; completion = completion->next, ++completion_index)
	{
		P_obj roots[QUEST_DURABLE_MAX_OFFERINGS] = {};
		size_t count = 0;
		bool matching = false;
		bool supported = true;
		bool complete = true;
		for (struct goal_data *goal = completion->give; goal; goal = goal->next)
			matching = matching || (goal->goal_type == QUEST_GOAL_ITEM &&
						OBJ_VNUM(offering) == goal->number);
		if (!matching)
			continue;
		for (struct goal_data *goal = completion->give; goal; goal = goal->next)
		{
			if (goal->goal_type != QUEST_GOAL_ITEM ||
			    count == QUEST_DURABLE_MAX_OFFERINGS)
			{
				supported = false;
				break;
			}
			P_obj selected = NULL;
			for (P_obj item = actor->carrying; item; item = item->next_content)
			{
				if (OBJ_VNUM(item) != goal->number)
					continue;
				bool used = false;
				for (size_t index = 0; index < count; ++index)
					used = used || roots[index] == item;
				if (!used)
				{
					selected = item;
					break;
				}
			}
			if (!selected)
			{
				complete = false;
				break;
			}
			if (!item_command_uses_durable_ownership(selected))
			{
				supported = false;
				break;
			}
			roots[count++] = selected;
		}
		matched_goal = true;
		if (!complete)
			continue;
		quest_durable_context context = {};
		context.quester_id = quester_id;
		context.completion_index = completion_index;
		context.room = mob->in_room;
		context.count = static_cast<uint32_t>(count);
		if (!capture_quest_credit_context(actor, &context))
		{
			send_to_char(
				"Quest completion credit cannot be captured safely. Please try again.\r\n",
				actor);
			return true;
		}
		if (economic_gameplay_authority::active())
		{
			bool recoverable_rewards = true;
			for (struct goal_data *reward = completion->receive; reward;
			     reward = reward->next)
			{
				recoverable_rewards = recoverable_rewards &&
						      (reward->goal_type == QUEST_GOAL_ITEM ||
						       reward->goal_type == QUEST_GOAL_COINS ||
						       reward->goal_type == QUEST_GOAL_SKILL ||
						       reward->goal_type == QUEST_GOAL_EXP);
			}
			if (!recoverable_rewards)
				supported = false;
		}
		if (!supported || !count)
		{
			unsupported_goal = true;
			break;
		}
		context.completed_at = static_cast<uint64_t>(time(NULL));
		for (size_t index = 0; index < count; ++index)
			context.roots[index] = roots[index]->obj_uid;
		item_transfer_continuation continuation;
		if (!capture_quest_offering_continuation(mob, actor, quester_id, completion_index,
							 completion, context, &continuation))
		{
			send_to_char("The quest offering service is busy. Please try again.\r\n",
				     actor);
			return true;
		}
		const item_owner_identity owner = { item_owner_type::player,
						    static_cast<uint64_t>(GET_PID(actor)), 0 };
		const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
		item_movement_reject reject = item_movement_reject::none;
		if (!item_movement_transaction_submit_batch(
			    actor, roots, count, NULL, owner, destruction,
			    item_transfer_reason::quest_turnin, GET_VNUM(mob),
			    complete_quest_offering, &context, sizeof(context), NULL, &reject,
			    publish_quest_offering, economic_source_kind::intentional_destruction,
			    0, continuation))
		{
			logit(LOG_DEBUG, "durable quest offering refused: %s",
			      item_movement_reject_name(reject));
			send_to_char("The quest offering service is busy. Please try again.\r\n",
				     actor);
		}
		else
			send_to_char("Your quest offering is being accepted.\r\n", actor);
		return true;
	}
	if (matched_goal)
		send_to_char(
			unsupported_goal ?
				"This quest cannot accept that durable item safely.\r\n" :
				"Bring all the requested items together before offering them.\r\n",
			actor);
	return matched_goal;
}

void tell_quest(int id, P_char pl)
{
	struct quest_msg_data *qmp;
	struct quest_complete_data *qcp;
	struct goal_data *gp;
	char Gbuf2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	P_obj obj;

	send_to_char("My key words are:\n", pl);
	for (qmp = quest_index[id].quest_message; qmp; qmp = qmp->next)
	{
		snprintf(Gbuf2, MAX_STRING_LENGTH, "+ Key: %s\n", qmp->key_words);
		send_to_char(Gbuf2, pl);
	}
	for (qcp = quest_index[id].quest_complete; qcp; qcp = qcp->next)
	{
		send_to_char("I'm looking for:\n", pl);
		for (gp = qcp->give; gp; gp = gp->next)
		{
			switch (gp->goal_type)
			{
			case QUEST_GOAL_ITEM:
				obj = read_object(gp->number, VIRTUAL);
				if (obj && real_object(gp->number) > 0)
				{
					snprintf(Gbuf2, MAX_STRING_LENGTH, "  - item #%d %s\n",
						 gp->number, obj->short_description);
					extract_obj(obj);
				}
				else
				{
					logit(LOG_DEBUG, "tell_quest(): obj %d not loadable",
					      gp->number);
					snprintf(Gbuf2, MAX_STRING_LENGTH, "  - ?????\n");
				}
				send_to_char(Gbuf2, pl);
				break;
			case QUEST_GOAL_ITEM_TYPE:
				sprinttype(gp->number, item_types, buf);
				checked_snprintf(Gbuf2, MAX_STRING_LENGTH,
						 "  - item type %d - %s\n", gp->number, buf);
				send_to_char(Gbuf2, pl);
				break;
			case QUEST_GOAL_COINS:
				snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %s\n",
					 coin_stringv(gp->number));
				send_to_char(Gbuf2, pl);
				break;
			default:
				send_to_char("  - Don't know\n", pl);
			}
		}
		send_to_char("And I will reward:\n", pl);
		for (gp = qcp->receive; gp; gp = gp->next)
		{
			switch (gp->goal_type)
			{
			case QUEST_GOAL_ITEM:
				obj = read_object(gp->number, VIRTUAL);
				if (obj && real_object(gp->number) > 0)
				{
					snprintf(Gbuf2, MAX_STRING_LENGTH, "  - item #%d %s\n",
						 gp->number, obj->short_description);
					extract_obj(obj);
				}
				else
				{
					logit(LOG_DEBUG, "tell_quest(): obj %d not loadable",
					      gp->number);
					snprintf(Gbuf2, MAX_STRING_LENGTH, "  - ?????\n");
				}
				send_to_char(Gbuf2, pl);
				break;
			case QUEST_GOAL_COINS:
				snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %s\n",
					 coin_stringv(gp->number));
				send_to_char(Gbuf2, pl);
				break;
			case QUEST_GOAL_SKILL:
				snprintf(Gbuf2, MAX_STRING_LENGTH, "  - %s\n",
					 skills[gp->number].name);
				send_to_char(Gbuf2, pl);
				break;
			case QUEST_GOAL_EXP:
				if (gp->number > 0)
					snprintf(Gbuf2, MAX_STRING_LENGTH,
						 "  - %d Experience points\n", gp->number);
				else
					snprintf(Gbuf2, MAX_STRING_LENGTH, "  - ?????\n");
				send_to_char(Gbuf2, pl);
				break;
			default:
				send_to_char("  - Don't know\n", pl);
			}
		}
		if (qcp->disappear)
			send_to_char("  I will disappear after this quest is finished.\n", pl);
	}
}

static bool submit_native_quest_give(P_char, P_char, int, P_obj) noexcept;

int quester(P_char ch, P_char pl, int cmd, char *arg)
{
	P_char vict;
	char name[MAX_INPUT_LENGTH];
	int quester_id;
	struct quest_msg_data *qmp;
	struct quest_complete_data *qcp;
	char Gbuf1[MAX_STRING_LENGTH], *temparg;

	/* ask/tell "key word" or give "item" */
	if ((cmd != CMD_ASK) && (cmd != CMD_TELL) && (cmd != CMD_GIVE))
		return (FALSE);

	/* if quester can't see player */
	if (((!CAN_SEE(ch, pl) || !CAN_SEE(pl, ch)) && (GET_LEVEL(pl) < MINLVLIMMORTAL)) ||
	    IS_FIGHTING(ch) || (GET_STAT(ch) <= STAT_SLEEPING))
		return (FALSE);

	if (affected_by_spell(ch, TAG_CONJURED_PET) || affected_by_spell(pl, TAG_CONJURED_PET))
		return (FALSE);

	if (cmd == CMD_ASK || cmd == CMD_TELL)
	{ /* player asked about quest */
		half_chop(arg, name, Gbuf1);
		if (!*name || !*Gbuf1 ||
		    (!(vict = get_char_room_vis(pl, name)) && (GET_LEVEL(pl) < MINLVLIMMORTAL)) ||
		    (vict != ch))
			return (FALSE);
		if ((quester_id = find_quester_id(GET_RNUM(ch))) < 0)
			return (FALSE);

		if ((cmd == CMD_TELL) && IS_TRUSTED(pl) && !strn_cmp(Gbuf1, "quest", 5))
		{
			tell_quest(quester_id, pl);
			return (TRUE);
		}
		for (qmp = quest_index[quester_id].quest_message; qmp; qmp = qmp->next)
		{
			if (qmp->key_words && isname(Gbuf1, qmp->key_words))
			{
				// no dollars/ansi here
				checked_snprintf(name, sizeof name, "$n asks $N about %s.", Gbuf1);
				act(name, FALSE, pl, 0, ch, TO_NOTVICT);
				act(qmp->message, FALSE, ch, 0, pl,
				    qmp->echoAll ? TO_ROOM : TO_VICT);
				if (qmp->echoAll)
					send_to_room("\n", ch->in_room);
				else
					send_to_char("\n", pl);

				return (TRUE);
			}
		}
		return (FALSE);
	}
	if (cmd == CMD_GIVE)
	{
		/* This next chunk of code is to deal with the case where someone's
		 * giving the quest mob money.  Need to check a different argument to
		 * get the name.
		 * -- DTS 2/28/95
		 */
		for (temparg = arg; isspace(*temparg); temparg++)
			; /* skip whitespaces */
		const bool giving_coins = isdigit(*temparg);
		if (giving_coins)
			temparg = one_argument(arg, name);

		temparg = one_argument(temparg, name);
		one_argument(temparg, name);
		if (!(vict = get_char_room_vis(pl, name)) || vict != ch)
			return (FALSE);
		quester_id = find_quester_id(GET_RNUM(ch));
		if (!giving_coins)
		{
			char offering_name[MAX_INPUT_LENGTH];
			one_argument(arg, offering_name);
			P_obj offering = get_obj_in_list_vis(pl, offering_name, pl->carrying);
			if (economic_gameplay_authority::active_regular_sql())
			{
				if (!submit_native_quest_give(ch, pl, quester_id, offering))
					send_to_char(
						"This quest cannot accept that item safely right now.\r\n",
						pl);
				return TRUE;
			}
			if (item_command_uses_durable_ownership(offering))
			{
				if (submit_durable_quest_offering(ch, pl, quester_id, offering))
					return (TRUE);
				send_to_char(
					"This quest cannot accept that durable item safely.\r\n",
					pl);
				return (TRUE);
			}
		}
		if (economic_gameplay_authority::active())
		{
			send_to_char("This quest cannot accept offerings right now.\r\n", pl);
			return (TRUE);
		}
		do_give(pl, arg, -4); /* give item to mob */
		if (quester_id < 0)
			return (TRUE);

		for (qcp = quest_index[quester_id].quest_complete; qcp; qcp = qcp->next)
		{
			if (quest_completion(qcp, ch, pl))
			{
				finish_quest_reward(qcp, ch, pl, 0);
				return (TRUE);
			}
		}
		return (TRUE);
	}
	return (TRUE);
}

int quest_sort_comp(const void *va, const void *vb)
{
	struct quest_data *a, *b;

	a = (struct quest_data *)va;
	b = (struct quest_data *)vb;

	if (a->quester > b->quester)
		return 1;
	else if (a->quester == b->quester)
		return 0;
	else
		return -1;
}

void quick_sort_quest_index(int /*min*/, int /*max*/)
{
	qsort(quest_index, number_of_quests, sizeof(struct quest_data), quest_sort_comp);
}

static FILE *open_quest_stream(const char *filename)
{
	FILE *quest_f = fopen(filename, "r");
	if (quest_f)
		return quest_f;

	// Do not fall back to globbing areas/qst/*.qst here.
	// Mini mode is effectively unused, and that fallback silently mixed the
	// mini mob table with the full quest set, producing a flood of real_mobile()
	// warnings for questers that do not exist in areas/mini.mob.
	return NULL;
}

void boot_the_quests(void)
{
	int temp;
	char tbuf, letterStrn[256], letter;
	FILE *quest_f;
	struct quest_msg_data *qmp;
	struct quest_complete_data *qcp;
	struct goal_data *gp;
	char filename[MAX_STRING_LENGTH];

	if (mini_mode == 1)
	{
		strcpy(filename, MINI_QUEST_FILE);
	}
	else
	{
		strcpy(filename, QUEST_FILE);
	}

	if (!(quest_f = open_quest_stream(filename)))
	{
		if (mini_mode == 1)
			return; /* mini mode is unused; skip quests cleanly if the file is absent */
		perror("Error in boot quest\n");
		return;
	}
	quest_index[number_of_quests].quest_message = 0;
	quest_index[number_of_quests].quest_complete = 0;

	for (number_of_quests = 0; number_of_quests < MAX_QUESTS; number_of_quests++)
	{
		tbuf = 0;
		temp = 0;
		if (fscanf(quest_f, "%c%d \n", &tbuf, &temp) != 2)
			break;
		if (tbuf == '#')
		{ /* a new quest */

			quest_index[number_of_quests].quester = real_mobile(temp);
			if (temp <= 0)
			{
				perror("Error in boot quest: mob id must be greater than 0\n");
				exit(1);
			}
			if (quest_index[number_of_quests].quester < 0)
			{
				fprintf(stderr, "Error in boot quest:  real_mobile(%d) = %d\n",
					temp, quest_index[number_of_quests].quester);
				fprintf(stderr,
					"Continuing anyway, against my better judgement.\n");
				/*
				   perror("Error in boot quest: mob does not exist.\n");
				   raise(SIGSEGV);
				 */
			}
			//      letter = 0;
			REQUIRED_FSCANF(quest_f, " %255s \n", letterStrn);
			while (letterStrn[0] != 'S')
			{
				switch (letterStrn[0])
				{
				case 'M':
					CREATE(qmp, quest_msg_data, 1, MEM_TAG_QSTMSG);

					qmp->key_words = fread_string(quest_f);
					qmp->message = fread_string(quest_f);
					if (qmp->message &&
					    qmp->message[strlen(qmp->message) - 1] == '\n')
						qmp->message[strlen(qmp->message) - 1] = '\0';
					qmp->echoAll = (letterStrn[1] == 'A');
					qmp->next = quest_index[number_of_quests].quest_message;
					quest_index[number_of_quests].quest_message = qmp;
					break;
				case 'Q':
					CREATE(qcp, quest_complete_data, 1, MEM_TAG_QSTCOMP);

					qcp->message = fread_string(quest_f);
					qcp->receive = 0;
					qcp->give = 0;
					qcp->echoAll = (letterStrn[1] == 'A');
					qcp->disappear = 0;
					qcp->disappear_message = 0;
					qcp->next = quest_index[number_of_quests].quest_complete;
					quest_index[number_of_quests].quest_complete = qcp;
					break;
				case 'D':
					if (quest_index[number_of_quests].quest_complete)
					{
						quest_index[number_of_quests]
							.quest_complete->disappear = TRUE;
						quest_index[number_of_quests]
							.quest_complete->disappear_message =
							fread_string(quest_f);
					}
					else
					{
						logit(LOG_EXIT,
						      "Error in boot quest: quester %d.\n",
						      mob_index[quest_index[number_of_quests].quester]
							      .virtual_number);
						exit(1);
					}
					break;
				case 'G':
					if (quest_index[number_of_quests].quest_complete)
					{
						CREATE(gp, goal_data, 1, MEM_TAG_QSTGOAL);

						REQUIRED_FSCANF(quest_f, " %c \n", &letter);
						switch (letter)
						{
						case 'I':
							gp->goal_type = QUEST_GOAL_ITEM;
							break;
						case 'T':
							gp->goal_type = QUEST_GOAL_ITEM_TYPE;
							break;
						case 'C':
							gp->goal_type = QUEST_GOAL_COINS;
							break;
						default:
							logit(LOG_EXIT,
							      "Error in boot quest: quester %d.\n",
							      mob_index[quest_index[number_of_quests]
										.quester]
								      .virtual_number);
							exit(1);
						}
						gp->number = 0;
						REQUIRED_FSCANF(quest_f, " %d \n", &(gp->number));
						gp->next = quest_index[number_of_quests]
								   .quest_complete->give;
						quest_index[number_of_quests].quest_complete->give =
							gp;
					}
					else
					{
						logit(LOG_EXIT,
						      "Error in boot quest: quester %d.\n",
						      mob_index[quest_index[number_of_quests].quester]
							      .virtual_number);
						exit(1);
					}
					break;
				case 'R':
					if (quest_index[number_of_quests].quest_complete)
					{
						CREATE(gp, goal_data, 1, MEM_TAG_QSTGOAL);

						REQUIRED_FSCANF(quest_f, " %c \n", &letter);
						switch (letter)
						{
						case 'I':
							gp->goal_type = QUEST_GOAL_ITEM;
							break;
						case 'C':
							gp->goal_type = QUEST_GOAL_COINS;
							break;
						case 'S':
							gp->goal_type = QUEST_GOAL_SKILL;
							break;
						case 'E':
							gp->goal_type = QUEST_GOAL_EXP;
							break;
						default:
							logit(LOG_EXIT,
							      "Error in boot quest: quester %d.\n",
							      mob_index[quest_index[number_of_quests]
										.quester]
								      .virtual_number);
							exit(1);
						}
						gp->number = 0;
						REQUIRED_FSCANF(quest_f, " %d \n", &(gp->number));
						if (gp->goal_type == QUEST_GOAL_SKILL &&
						    gp->number >= MAX_SKILLS)
						{
							gp->number = 0;
							gp->goal_type = QUEST_GOAL_UNKNOWN;
						}
						gp->next = quest_index[number_of_quests]
								   .quest_complete->receive;
						quest_index[number_of_quests]
							.quest_complete->receive = gp;
					}
					else
					{
						logit(LOG_EXIT,
						      "Error in boot quest: quester %d.\n",
						      mob_index[quest_index[number_of_quests].quester]
							      .virtual_number);
						exit(1);
					}
					break;
				default:
					logit(LOG_EXIT,
					      "Error in boot quest: quester %d, letterStrn=%s.\n",
					      mob_index[quest_index[number_of_quests].quester]
						      .virtual_number,
					      letterStrn);
					exit(1);
				}
				if (fscanf(quest_f, " %255s \n", letterStrn) != 1)
				{
					logit(LOG_EXIT, "Error in boot quest: quester %d.\n",
					      mob_index[quest_index[number_of_quests].quester]
						      .virtual_number);
					exit(1);
				}
			}
		}
		else if (tbuf == '$')
		{
			break;
		}
		else
		{
			logit(LOG_EXIT, "Error in boot quest: quester #%d.\n", number_of_quests);
			exit(1);
		}
	}
	fclose(quest_f);

	if (number_of_quests == MAX_QUESTS)
	{
		logit(LOG_EXIT, "\t\tMaximum of %d quests exceeded\n", MAX_QUESTS);
		exit(1);
	}
	fprintf(stderr, "\t\t%d quests allocated\n", number_of_quests);

	if (number_of_quests > 0)
		quick_sort_quest_index(0, number_of_quests - 1);
}

void assign_the_questers(void)
{
	int count;

	for (count = 0; count < number_of_quests; count++)
	{
		mob_index[quest_index[count].quester].qst_func = quester;
		/*
		   fprintf(stderr, "Assigning: mob# %d\n", quest_index[count].quester);
		 */
	}
}

#define QUEST_FILE_TROPHY "areas/quest.trophy"
#define TEMP_QUEST_FILE_TROPHY "areas/temp_quest.trophy"

int addQuestTropy(int questID)
{
	int found = 0;
	int t_id = 0;
	int t_trophy = 0;
	FILE *f;
	FILE *temp_f;

	f = fopen(QUEST_FILE_TROPHY, "r");
	temp_f = fopen(TEMP_QUEST_FILE_TROPHY, "w+");

	if (!f || !temp_f)
	{
		wizlog(56, "Cant open file %s", QUEST_FILE_TROPHY);
		return 0;
	}

	while (!(feof(f)))
	{
		REQUIRED_FSCANF(f, "%d %d\n", &t_id, &t_trophy);

		if (t_id == questID)
		{
			found = 1;
			t_trophy++;
		}
		fprintf(temp_f, "%d %d\n", t_id, t_trophy);
	}

	if (!found)
		fprintf(temp_f, "%d %d\n", questID, 0);

	fclose(f);
	fclose(temp_f);
	rename(TEMP_QUEST_FILE_TROPHY, QUEST_FILE_TROPHY);

	return 0;
}

float getQuestTropy(int questID)
{
	int t_id = 0;
	int t_trophy = 0;
	int total_trophy = 0;
	int trophy = 0;
	FILE *f;

	f = fopen(QUEST_FILE_TROPHY, "r");

	if (!f)
	{
		wizlog(56, "Cant open file %s", QUEST_FILE_TROPHY);
		return 0;
	}

	while (!(feof(f)))
	{
		REQUIRED_FSCANF(f, "%d %d\n", &t_id, &t_trophy);

		if (t_id == questID)
			trophy = t_trophy;

		total_trophy = total_trophy + t_trophy; // total trophy
	}
	fclose(f);

	return (float)((float)(trophy * 1.00) / (float)(total_trophy * 1.00));
}

bool has_quest(P_char ch)
{
	int qi;

	if (!IS_NPC(ch))
		return FALSE;

	qi = find_quester_id(GET_RNUM(ch));

	if (qi < 0)
	{
		return FALSE;
	}

	if (quest_index[qi].quest_complete || quest_index[qi].quest_message)
		return TRUE;

	return FALSE;
}

bool has_quest_ask(int qi)
{
	int arg;
	struct quest_msg_data *qdata;

	for (qdata = quest_index[qi].quest_message; qdata; qdata = qdata->next)
	{
		// Skip the room messages on timers and the unblock messages.
		if ((sscanf(qdata->key_words, QC_ACTION " %d", &arg) == 1) ||
		    is_abbrev(QC_UNBLOCK " ", qdata->key_words))
		{
			continue;
		}
		else
			return TRUE;
	}
	return FALSE;
}

bool has_quest_complete(int qi)
{
	if (quest_index[qi].quest_complete)
		return TRUE;

	return FALSE;
}

// Sole issuer of the original sequential NPC-stock consumption decision. The
// caller must be the original native quest continuation owner; these values
// neither publish effects nor acknowledge the retained operation.
namespace
{
struct native_quest_original_branch
{
	quest_complete_data view{};
	std::vector<goal_data> give, receive;
	std::string message, disappear_message, definition_id;
};
}
class quest_native_gameplay_owner;
class quest_native_completion_owner final
{
    public:
	static item_native_quest_preparation_state
	prepare(P_char mobile, P_char final_giver, int quester_id, int completion_index,
		item_native_quest_preparation_token *output) noexcept;

    private:
	friend class quest_native_gameplay_owner;
	static item_native_quest_preparation_state
	prepare_original(P_char, P_char, int, int, const native_quest_original_branch *,
			 const critical_operation_id *, item_native_quest_preparation_token *,
			 const critical_command * = nullptr,
			 const native_quest_recovery_receipt * = nullptr) noexcept;
};

item_native_quest_preparation_state
quest_native_completion_owner::prepare(P_char mobile, P_char player, int quester_id,
				       int completion_index,
				       item_native_quest_preparation_token *output) noexcept
{
	return prepare_original(mobile, player, quester_id, completion_index, nullptr, nullptr,
				output);
}

item_native_quest_preparation_state quest_native_completion_owner::prepare_original(
	P_char mob, P_char player, int quester_id, int completion_index,
	const native_quest_original_branch *original_branch,
	const critical_operation_id *original_child, item_native_quest_preparation_token *output,
	const critical_command *triggering_acceptance,
	const native_quest_recovery_receipt *triggering_receipt) noexcept
{
	using state = item_native_quest_preparation_state;
#ifdef __NO_MYSQL__
	(void)mob;
	(void)player;
	(void)quester_id;
	(void)completion_index;
	(void)original_branch;
	(void)original_child;
	(void)triggering_acceptance;
	(void)triggering_receipt;
	(void)output;
	return state::refused;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() || !mob || !player || !IS_NPC(mob) ||
	    !IS_PC(player) || !player->only.pc || GET_PID(player) <= 0 ||
	    player->in_room == NOWHERE || player->in_room != mob->in_room || quester_id < 0 ||
	    completion_index < 0 ||
	    (!original_branch &&
	     (quester_id >= number_of_quests || quest_index[quester_id].quester != GET_RNUM(mob))))
		return state::refused;
	try
	{
		const quest_complete_data *completion =
			original_branch ? &original_branch->view :
					  quest_index[quester_id].quest_complete;
		if (!original_branch)
			for (int i = 0; completion && i < completion_index; ++i)
				completion = completion->next;
		if (!completion)
			return state::refused;
		quest_mobile_native_reference reference;
		std::vector<player_item_snapshot> native_items;
		if (!quest_mobile_native_reference_copy(mob, mob->runtime_id, &reference) ||
		    quest_mobile_native_items_observe(mob, reference, &native_items) !=
			    player_snapshot_capture_result::ok)
			return state::refused;
		// Observe genuine original runtime cash metadata only when the ordered
		// original preliminary pass first reaches a COINS requirement.
		quest_mobile_native_cash_reference cash;
		bool cash_observed = false;
		std::vector<native_quest_cost_requirement> attempted_costs;
		// Preserve original duplicate/availability checks, including each fee
		// independently against the same original NPC cash (not their sum).
		for (auto *goal = completion->give; goal; goal = goal->next)
		{
			if (goal->goal_type == QUEST_GOAL_COINS)
			{
				if (!cash_observed)
				{
					native_quest_cost_projection unchanged;
					if (!quest_mobile_native_cash_reference_copy(
						    mob, mob->runtime_id, &cash) ||
					    native_quest_cost_project(cash.denominations,
								      cash.cash_revision, {},
								      &unchanged) !=
						    native_quest_cost_projection_result::ok)
						return state::refused;
					cash_observed = true;
				}
				const int64_t available =
					cash.denominations[0] + 10 * cash.denominations[1] +
					100 * cash.denominations[2] + 1000 * cash.denominations[3];
				if (available < goal->number)
					return state::not_matched;
				continue;
			}
			int needed = 1;
			for (auto *later = goal->next; later; later = later->next)
				if (later->goal_type == goal->goal_type &&
				    later->number == goal->number)
					++needed;
			int count = 0;
			for (P_obj object = mob->carrying; object; object = object->next_content)
				if ((goal->goal_type == QUEST_GOAL_ITEM &&
				     OBJ_VNUM(object) == goal->number) ||
				    (goal->goal_type == QUEST_GOAL_ITEM_TYPE &&
				     object->type == goal->number))
					++count;
			if ((goal->goal_type == QUEST_GOAL_ITEM ||
			     goal->goal_type == QUEST_GOAL_ITEM_TYPE) &&
			    count < needed)
				return state::not_matched;
		}
		std::vector<uint64_t> ordered_roots;
		std::unordered_set<uint64_t> consumed;
		bool success = true;
		bool invalid_cost_slot = false;
		const auto select_pass = [&](int kind)
		{
			uint64_t slot = 0;
			for (auto *goal = completion->give; goal; goal = goal->next, ++slot)
			{
				if (kind == QUEST_GOAL_ITEM && goal->goal_type == QUEST_GOAL_COINS)
				{
					if (!cash_observed || slot > UINT32_MAX)
					{
						invalid_cost_slot = true;
						return false;
					}
					attempted_costs.push_back(
						{ static_cast<uint32_t>(slot), goal->number });
					continue;
				}
				if (goal->goal_type != kind)
					continue;
				P_obj selected = nullptr;
				for (P_obj object = mob->carrying; object;
				     object = object->next_content)
					if (!consumed.count(object->obj_uid) &&
					    (kind == QUEST_GOAL_ITEM ?
						     OBJ_VNUM(object) == goal->number :
						     object->type == goal->number))
					{
						selected = object;
						break;
					}
				if (!selected)
					return false;
				if (!selected->obj_uid ||
				    !consumed.insert(selected->obj_uid).second)
					return false;
				ordered_roots.push_back(selected->obj_uid);
			}
			return true;
		};
		// The first destructive pass handles ITEM; the later TYPE pass may
		// fail after an overlapping prefix was already selected. That prefix
		// is an original quest_action with no reward, not an all-or-nothing set.
		if (!select_pass(QUEST_GOAL_ITEM))
			success = false;
		else if (!select_pass(QUEST_GOAL_ITEM_TYPE))
			success = false;
		if (invalid_cost_slot)
			return state::refused;
		const bool fee_only = ordered_roots.empty() && !attempted_costs.empty();
		if (ordered_roots.empty() && !fee_only)
			return state::refused;
		critical_operation_id fee_operation{};
		if (fee_only)
		{
			if (!original_child || critical_operation_id_is_zero(*original_child) ||
			    !triggering_acceptance || !triggering_receipt ||
			    !triggering_receipt->present || triggering_receipt->error_code ||
			    triggering_receipt->result_size != ITEM_TRANSFER_RESULT_BYTES ||
			    (triggering_receipt->outcome != critical_apply_outcome::applied &&
			     triggering_receipt->outcome !=
				     critical_apply_outcome::already_applied))
				return state::refused;
			fee_operation = *original_child;
		}
		std::vector<player_item_snapshot> selected;
		std::vector<int32_t> indexes(native_items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		std::vector<uint64_t> roots(native_items.size());
		size_t selected_roots = 0;
		for (size_t i = 0; i < native_items.size(); ++i)
		{
			const auto &item = native_items[i];
			const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
			if (!root &&
			    (item.parent_index < 0 || static_cast<size_t>(item.parent_index) >= i))
				return state::refused;
			roots[i] = root ? item.object_uid : roots[item.parent_index];
			if (!consumed.count(roots[i]))
				continue;
			if (root && item.equipment_slot)
				return state::refused;
			if (root)
				++selected_roots;
			auto literal = item;
			if (!root)
				literal.parent_index = indexes[item.parent_index];
			indexes[i] = static_cast<int32_t>(selected.size());
			selected.push_back(std::move(literal));
		}
		std::vector<uint8_t> selected_bytes;
		if (selected_roots != consumed.size() || (!fee_only && selected.empty()) ||
		    selected.size() > ITEM_TRANSFER_MAX_ITEMS ||
		    player_item_snapshot_list_encode(selected, &selected_bytes) !=
			    player_snapshot_codec_result::ok ||
		    selected_bytes.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
			return state::refused;
		item_transfer_payload payload{};
		payload.from_owner = { item_owner_type::native_mobile, reference.mobile_instance_id,
				       0 };
		payload.to_owner =
			fee_only ?
				item_owner_identity{ item_owner_type::player,
						     static_cast<uint32_t>(GET_PID(player)), 0 } :
				item_owner_identity{ item_owner_type::destruction, 0, 0 };
		payload.reason = item_transfer_reason::quest_turnin;
		payload.reason_id = reference.mobile_vnum;
		payload.expected_from_revision = reference.stock_revision;
		uint64_t native_owner_revision = 0;
		if (!item_ownership_runtime_peek_owner_revision(payload.from_owner,
								&native_owner_revision) ||
		    native_owner_revision != reference.stock_revision)
			return state::refused;
		if (!item_ownership_runtime_peek_owner_revision(payload.to_owner,
								&payload.expected_to_revision))
			return state::refused;
		payload.multi_root = !fee_only;
		payload.item_count = static_cast<uint16_t>(selected.size());
		for (size_t i = 0; i < selected.size(); ++i)
		{
			const auto &item = selected[i];
			item_ownership_runtime_entry current{};
			const uint64_t parent = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
							0 :
							selected[item.parent_index].object_uid;
			uint64_t root = item.object_uid;
			for (int32_t j = item.parent_index; j != PLAYER_SNAPSHOT_NO_PARENT;
			     j = selected[j].parent_index)
				root = selected[j].object_uid;
			if (!item_ownership_runtime_lookup(item.object_uid, &current) ||
			    !item_owner_identity_equal(current.owner, payload.from_owner) ||
			    current.state != item_custody_state::active ||
			    current.vnum != item.vnum || current.root_item_uid != root ||
			    current.parent_item_uid != parent || !current.item_revision ||
			    current.item_revision == UINT64_MAX)
				return state::refused;
			payload.items[i] = { item.object_uid, root,
					     parent,	      current.item_revision,
					     item.vnum,	      item_custody_state::active };
		}
		std::sort(payload.items.begin(), payload.items.begin() + payload.item_count,
			  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
		payload.item_blob_size = fee_only ? 0 :
						    static_cast<uint32_t>(selected_bytes.size());
		if (!fee_only)
			std::copy(selected_bytes.begin(), selected_bytes.end(),
				  payload.item_blob.begin());
		payload.native_mobile.present = true;
		payload.native_mobile.reference = reference;
		payload.native_mobile.action = item_native_mobile_action::consumption;
		payload.native_mobile.final_giver_pid = static_cast<uint32_t>(GET_PID(player));
		if (!attempted_costs.empty())
		{
			payload.native_cost.present = true;
			payload.native_cost.fee_only = fee_only;
			payload.native_cost.completion_slot =
				fee_only ? static_cast<uint32_t>(completion_index) : 0;
			payload.native_cost.wallet_mapping_id = cash.wallet_mapping_id;
			if (native_quest_cost_project(cash.denominations, cash.cash_revision,
						      attempted_costs,
						      &payload.native_cost.projection) !=
			    native_quest_cost_projection_result::ok)
				return state::refused;
		}
		quest_reward_continuation fee_terms;
		if (fee_only)
		{
			fee_terms.version = 6;
			fee_terms.player_pid = static_cast<uint32_t>(GET_PID(player));
			fee_terms.mobile_vnum = static_cast<uint32_t>(reference.mobile_vnum);
			fee_terms.completion_index = static_cast<uint32_t>(completion_index);
			fee_terms.action_operation = fee_operation;
			fee_terms.action_mobile_instance_id = reference.mobile_instance_id;
			fee_terms.action_source = { economic_source_kind::quest_action,
						    fee_operation,
						    reference.birth_source.generation,
						    reference.mobile_revision,
						    static_cast<uint32_t>(completion_index) };
			fee_terms.triggering_acceptance = triggering_acceptance->operation_id;
			std::copy_n(triggering_receipt->result_payload.begin(),
				    ITEM_TRANSFER_RESULT_BYTES,
				    fee_terms.original_acceptance_result.begin());
			if (!quest_fee_reward_trigger_binding_valid(fee_terms,
								    *triggering_acceptance))
				return state::refused;
		}
		if (success)
		{
			if (ordered_roots.size() > QUEST_DURABLE_MAX_OFFERINGS)
				return state::refused;
			quest_durable_context context{};
			context.quester_id = quester_id;
			context.completion_index = completion_index;
			context.room = mob->in_room;
			context.count = static_cast<uint32_t>(ordered_roots.size());
			std::copy(ordered_roots.begin(), ordered_roots.end(), context.roots);
			const auto completed_at = time(nullptr);
			if (completed_at <= 0)
				return state::refused;
			context.completed_at = static_cast<uint64_t>(completed_at);
			if (!capture_quest_credit_context(player, &context) ||
			    !capture_quest_offering_continuation(
				    mob, player, quester_id, completion_index, completion, context,
				    &payload.continuation,
				    original_branch ? &original_branch->definition_id : nullptr,
				    fee_only ? &fee_terms : nullptr))
				return state::refused;
		}
		// A failed consumed prefix leaves continuation exactly empty. Successful
		// completion preserves the existing version-5 full frozen reward terms.
		critical_operation_id operation{};
		critical_command original;
		if (original_child)
			operation = *original_child;
		if ((original_child ? critical_operation_id_is_zero(operation) :
				      !critical_operation_id_generate(&operation)) ||
		    !item_transfer_command_build_native_mobile(
			    &original, operation, payload, critical_source_site::command,
			    critical_deadline_class::interactive))
			return state::refused;
		static_assert(ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES == MAX_STRING_LENGTH);
		item_native_quest_publication_terms publication_terms;
		const auto freeze_text = [](const char *text, std::string &out)
		{
			if (!text)
				return true;
			const size_t size = strnlen(text, MAX_STRING_LENGTH);
			if (size == MAX_STRING_LENGTH)
				return false;
			out.assign(text, size);
			return true;
		};
		// The original preliminary message follows successful availability
		// checks even if an overlapping ITEM/TYPE pass later consumes a prefix.
		// This original quest_action has no reward or disappearance terms.
		if (!freeze_text(completion->message, publication_terms.message))
			return state::refused;
		publication_terms.echo_all = completion->echoAll != 0;
		if (success)
		{
			if (!freeze_text(completion->disappear_message,
					 publication_terms.disappear_message))
				return state::refused;
			publication_terms.disappear = completion->disappear != 0;
		}
		std::shared_ptr<const quest_native_consumption_capture> captured(
			new quest_native_consumption_capture(
				std::move(original), reference, mob->runtime_id,
				static_cast<uint32_t>(GET_PID(player)),
				static_cast<uint32_t>(completion_index), std::move(ordered_roots),
				std::move(publication_terms),
				success && !payload.native_cost.present ?
					economic_source_kind::quest_completion :
					economic_source_kind::quest_action,
				fee_only));
		return item_native_quest_preparation_owner::begin_consumption(
			player, mob, std::move(captured), output);
	}
	catch (...)
	{
		return state::refused;
	}
#endif
}

item_native_quest_preparation_state
quest_native_completion_prepare(P_char native_mobile, P_char final_giver, int quester_id,
				int completion_index,
				item_native_quest_preparation_token *output) noexcept
{
	return quest_native_completion_owner::prepare(native_mobile, final_giver, quester_id,
						      completion_index, output);
}

#ifndef __NO_MYSQL__
struct native_quest_frozen_recovery_state
{
	std::string key;
	critical_command command;
	std::vector<uint8_t> continuation;
	quest_reward_continuation terms;
	uint64_t player_runtime_id = 0, xp_mask = 0, economic_mask = 0;
	std::unique_ptr<critical_native_recovery_envelope> current, pending, returned;
	size_t retained_bytes = 0;
	bool started = false, effect_returned = false, known_not_attempted = false;
	bool pending_returned = false, durable_returned = false;
	bool acknowledged = false, retired = false, poisoned = false;
	bool restored = false;
	bool terminal_pair_attempted = false;
	std::unique_ptr<critical_native_recovery_envelope> restored_original, terminal_parent;
};
namespace
{
bool native_quest_terminal_parent_live(const critical_native_recovery_envelope &) noexcept;
void native_quest_cleanup_retired_pair(const critical_native_recovery_envelope &,
				       const critical_native_recovery_envelope &) noexcept;
std::unordered_map<std::string, std::shared_ptr<native_quest_frozen_recovery_state>>
	native_quest_frozen_recoveries;
size_t
native_quest_frozen_bytes(const native_quest_frozen_recovery_state &state,
			  const critical_native_recovery_envelope *pending = nullptr,
			  const critical_native_recovery_envelope *returned = nullptr) noexcept
{
	size_t bytes = sizeof(state) +
		       sizeof(std::pair<const std::string,
					std::shared_ptr<native_quest_frozen_recovery_state>>) +
		       CRITICAL_COMMAND_MAX_ENCODED_BYTES;
	const auto add = [&](size_t value)
	{
		if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    value > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += value;
		return true;
	};
	if (!add(state.key.capacity()) || !add(state.key.capacity()) ||
	    !add(state.continuation.capacity()) || !add(state.terms.character_name.capacity()) ||
	    !add(state.terms.definition_id.capacity()))
		return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
	for (const auto *record : std::array<const critical_native_recovery_envelope *, 5>{
		     state.current.get(), pending ? pending : state.pending.get(),
		     returned ? returned : state.returned.get(), state.restored_original.get(),
		     state.terminal_parent.get() })
		if (record && (!add(sizeof(*record) + CRITICAL_COMMAND_MAX_ENCODED_BYTES) ||
			       !add(record->attachment.capacity())))
			return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
	return bytes;
}
bool native_quest_frozen_current(const native_quest_frozen_recovery_state &state) noexcept
{
	const auto found = native_quest_frozen_recoveries.find(state.key);
	return found != native_quest_frozen_recoveries.end() && found->second.get() == &state;
}
}
#endif

bool quest_native_frozen_continuation_owner::checkpoint(
	native_quest_frozen_recovery_state &state) noexcept
{
#ifdef __NO_MYSQL__
	(void)state;
	return false;
#else
	if (!state.pending)
		return true;
	if (state.poisoned || !native_quest_frozen_current(state) || !state.current ||
	    !critical_native_quest_continuation_owner::checkpoint_context(*state.current,
									  *state.pending))
		return false; // Retain exact expected/successor, including uncertain returned bytes.
	if (!native_quest_frozen_current(state))
	{
		state.poisoned = true;
		return false;
	}
	state.current = std::move(state.pending);
	if (state.pending_returned)
		state.durable_returned = true;
	state.pending_returned = false;
	// Pinned map identity, moves, scalar charging only after journal I/O.
	const size_t bytes = native_quest_frozen_bytes(state);
	if (!budget(&state, bytes))
	{
		state.poisoned = true;
		return false;
	}
	state.retained_bytes = bytes;
	return true;
#endif
}

bool quest_native_frozen_continuation_owner::retire_completed(
	native_quest_frozen_recovery_state &state) noexcept
{
#ifdef __NO_MYSQL__
	(void)state;
	return false;
#else
	if (!nevent_is_game_thread() || state.poisoned || !native_quest_frozen_current(state) ||
	    !state.current || state.pending)
		return false;
	if (state.retired)
		return true; // Only the actual paired journal success sets this latch.
	try
	{
		// A read-only ACK probe may precede the live parent's handoff successor.
		// Refresh unattempted lookup values; an actual pair attempt pins both preimages.
		if (state.terminal_parent && !state.terminal_pair_attempted)
			state.terminal_parent.reset();
		if (!state.terminal_parent)
		{
			auto parent = std::make_unique<critical_native_recovery_envelope>();
			if (!critical_native_quest_continuation_owner::copy_parent_context(
				    *state.current, parent.get()))
				return false; // No matching parent is never evidence of retirement.
			state.terminal_parent = std::move(parent);
			const size_t bytes = native_quest_frozen_bytes(state);
			if (!budget(&state, bytes))
			{
				state.terminal_parent.reset();
				return false;
			}
			state.retained_bytes = bytes;
		}
		native_quest_recovery_context parent_context, child_context;
		if (!native_quest_recovery_pair_context_valid(*state.terminal_parent,
							      *state.current, nullptr) ||
		    native_quest_recovery_context_decode(
			    state.terminal_parent->command, state.terminal_parent->attachment,
			    &parent_context) != player_snapshot_codec_result::ok ||
		    native_quest_recovery_context_decode(state.command, state.current->attachment,
							 &child_context) !=
			    player_snapshot_codec_result::ok)
			return false;
		if (parent_context.child_handoff_stage == 1 &&
		    native_quest_terminal_parent_live(*state.terminal_parent))
		{
			// A live token must first persist its original handoff2 and reach take.
			// The next pulse copies that actual successor, never this stale parent.
			if (!state.terminal_pair_attempted)
				state.terminal_parent.reset();
			return false;
		}
		const auto matches = [](const native_quest_recovery_receipt &expected,
					const critical_apply_result &actual)
		{
			return expected.present && !expected.error_code && !actual.error_code &&
			       (expected.outcome == critical_apply_outcome::applied ||
				expected.outcome == critical_apply_outcome::already_applied) &&
			       actual.outcome == critical_apply_outcome::already_applied &&
			       expected.durable_revision == actual.durable_revision &&
			       expected.failure_stage == actual.failure_stage &&
			       expected.result_size == actual.result_size &&
			       expected.result_payload == actual.result_payload;
		};
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17))
			{
				const auto parent_receipt =
					critical_command_repository_verify_native_quest_in_transaction(
						connection, state.terminal_parent->command);
				const auto child_receipt =
					state.terms.version == 6 ?
						critical_apply_result{} :
						critical_command_repository_verify_native_quest_in_transaction(
							connection, state.command);
				quest_reward_obligation_readback obligation;
				unsigned int error = 0;
				proven =
					matches(parent_context.receipt, parent_receipt) &&
					(state.terms.version == 6 ?
						 quest_reward_obligation_native_fee_owner::
							 verify_in_transaction(
								 connection,
								 state.command.operation_id,
								 state.continuation) :
						 matches(child_context.receipt, child_receipt)) &&
					quest_reward_obligation_repository_read_exact_in_transaction(
						connection, state.terms.player_pid,
						state.command.operation_id, state.continuation,
						&obligation,
						&error) == quest_reward_obligation_result::ok &&
					!error && obligation.acknowledged &&
					transaction.same_session();
			}
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		lease.reuse(cleanup);
		if (!proven || !transaction.same_session() || !cleanup.rollback_confirmed ||
		    cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		// Historical native roots plus the exact actual ACK authorize cleanup only.
		// Current player/world projections may legitimately have advanced with rewards.
		// Never rebind actors, publish stock or repeat reward/skill effects here.
		state.acknowledged = true;
		state.terminal_pair_attempted = true;
		if (!critical_native_quest_continuation_owner::transition_pair(
			    *state.terminal_parent, *state.current, nullptr))
			return false; // Retain both full preimages through uncertain journal I/O.
		state.retired = true;
		native_quest_cleanup_retired_pair(*state.terminal_parent, *state.current);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_native_frozen_continuation_owner::drive(native_quest_frozen_recovery_state &state,
						   bool allow_effects) noexcept
{
#ifdef __NO_MYSQL__
	(void)state;
	(void)allow_effects;
	return false;
#else
	if (!nevent_is_game_thread() || state.poisoned || !native_quest_frozen_current(state))
		return false;
	try
	{
		if (state.terminal_pair_attempted)
			return retire_completed(
				state); // Neither retained preimage may advance now.
		if (!checkpoint(state))
			return state.effect_returned && !state.poisoned &&
			       native_quest_frozen_current(state);
		if (state.retired ||
		    ((state.restored || state.acknowledged || state.durable_returned) &&
		     retire_completed(state)))
			return true;
		if (state.terminal_pair_attempted)
			return false; // Genuine terminal proof owns cleanup, never reward dispatch.
		// Stable retained writer owns the actual returned fact.
		if (state.started && !state.effect_returned && !state.known_not_attempted)
			return false; // Cold or actual started/unreturned never authorizes reward repetition.
		if (!state.started)
		{
			if (state.terms.version == 6 &&
			    !quest_reward_obligation_native_fee_owner::ready(
				    state.command.operation_id, state.terms))
				return false;
			if (!allow_effects || state.current->revision >= UINT64_MAX - 1)
				return false;
			native_quest_recovery_context context;
			if (native_quest_recovery_context_decode(
				    state.command, state.current->attachment, &context) !=
				    player_snapshot_codec_result::ok ||
			    context.publication_steps[5] != 0)
				return false;
			auto began =
				std::make_unique<critical_native_recovery_envelope>(*state.current);
			++began->revision;
			context.publication_steps[5] = 1;
			if (native_quest_recovery_context_encode(state.command, context,
								 &began->attachment) !=
			    player_snapshot_codec_result::ok)
				return false;
			auto ended = std::make_unique<critical_native_recovery_envelope>(*began);
			++ended->revision;
			context.publication_steps[5] = 2;
			if (native_quest_recovery_context_encode(state.command, context,
								 &ended->attachment) !=
			    player_snapshot_codec_result::ok)
				return false;
			const size_t bytes =
				native_quest_frozen_bytes(state, began.get(), ended.get());
			if (!budget(&state, bytes))
				return false;
			state.retained_bytes = bytes;
			state.pending = std::move(began);
			state.returned = std::move(ended);
			state.started = true;
			state.known_not_attempted = true;
			if (!checkpoint(state))
				return false;
		}
		if (!state.effect_returned)
		{
			if (!allow_effects || !state.known_not_attempted || !state.returned)
				return false;
			P_char actor = find_character_by_runtime_id(state.player_runtime_id);
			if (!actor || !IS_PC(actor) || !actor->only.pc ||
			    GET_PID(actor) != static_cast<int>(state.terms.player_pid))
				return false;
			state.known_not_attempted = false;
			quest_reward_recover_pending(actor, state.command.operation_id, state.terms,
						     state.xp_mask, state.economic_mask, true);
			for (size_t i = 0; i < state.terms.xp_award_count; ++i)
			{
				const auto &award = state.terms.xp_awards[i];
				if (award.recipient_pid == state.terms.player_pid)
					continue;
				if (P_char recipient =
					    quest_reward_character_present(award.recipient_pid))
					quest_reward_recover_xp_entitlement(
						recipient, state.command.operation_id, state.terms,
						award.reward_index, award.amount);
			}
			// Both successors and their owner storage existed before the first call.
			// Returning is handoff, not proof that any reward succeeded or was saved.
			state.effect_returned = true;
			state.pending = std::move(state.returned);
			state.pending_returned = true;
			if (!checkpoint(state))
				return !state.poisoned && native_quest_frozen_current(state);
		}
		if (state.durable_returned && !state.retired)
			(void)retire_completed(state);
		return state.effect_returned && !state.poisoned;
	}
	catch (...)
	{
		// Actual interrupted calls never repeat. Existing reward receipts retain
		// their independent login/entitlement recovery, not inferred completion.
		return false;
	}
#endif
}

void quest_native_frozen_continuation_owner::acknowledged(
	const quest_reward_ack_completion &completion) noexcept
{
#ifndef __NO_MYSQL__
	if (!nevent_is_game_thread() || completion.error_code ||
	    (completion.result != quest_reward_obligation_result::ok &&
	     completion.result != quest_reward_obligation_result::already_acknowledged))
		return;
	for (const auto &[key, state] : native_quest_frozen_recoveries)
		if (state && !state->poisoned && state->effect_returned &&
		    state->terms.player_pid == completion.player_pid &&
		    state->command.operation_id.bytes == completion.offering_operation.bytes)
			state->acknowledged =
				true; // Actual original repository completion, never caller bool.
#else
	(void)completion;
#endif
}

void quest_native_frozen_continuation_owner::pulse() noexcept
{
#ifndef __NO_MYSQL__
	if (!nevent_is_game_thread())
		return;
	size_t remaining = QUEST_REWARD_ACK_PIPELINE_PULSE_MAX;
	for (auto it = native_quest_frozen_recoveries.begin();
	     it != native_quest_frozen_recoveries.end() && remaining--;)
	{
		auto held = it->second;
		(void)drive(
			*held,
			false); // Settle actual writers/ACK only; never dispatch reward effects.
		if (held->retired && !held->poisoned)
			it = native_quest_frozen_recoveries.erase(it);
		else
			++it;
	}
	(void)budget(nullptr, 0);
#endif
}

bool quest_native_frozen_continuation_owner::publish(const critical_command &command,
						     const quest_reward_obligation_record &record,
						     uint64_t original_player_runtime_id,
						     bool *started) noexcept
{
	if (!started)
		return false;
	*started = false;
#ifdef __NO_MYSQL__
	(void)command;
	(void)record;
	(void)original_player_runtime_id;
	return false;
#else
	if (!nevent_is_game_thread() || !original_player_runtime_id ||
	    (command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	     command.payload_version !=
		     ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION) ||
	    command.operation_id.bytes != record.offering_operation.bytes)
		return false;
	try
	{
		item_transfer_payload payload{};
		quest_reward_continuation terms;
		if (!item_transfer_command_decode_payload(command, &payload) ||
		    !item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    payload.native_mobile.action != item_native_mobile_action::consumption ||
		    payload.continuation.kind != item_transfer_continuation_kind::quest_offering ||
		    payload.continuation.data != record.continuation ||
		    payload.native_recovery.publication_terms.disappear ||
		    !quest_reward_continuation_decode(record.continuation.data(),
						      record.continuation.size(), &terms) ||
		    (terms.version != 5 && !(terms.version == 6 && payload.native_cost.fee_only)) ||
		    terms.player_pid != payload.native_recovery.player_pid)
			return false;
		const std::string key = quest_reward_operation_key(command.operation_id);
		auto found = native_quest_frozen_recoveries.find(key);
		if (found == native_quest_frozen_recoveries.end())
		{
			P_char actor = find_character_by_runtime_id(original_player_runtime_id);
			if (!actor || !IS_PC(actor) || !actor->only.pc ||
			    GET_PID(actor) != static_cast<int>(terms.player_pid))
				return false;
			auto state = std::make_shared<native_quest_frozen_recovery_state>();
			state->key = key;
			state->command = command;
			state->continuation = record.continuation;
			state->terms = std::move(terms);
			state->player_runtime_id = original_player_runtime_id;
			state->xp_mask = record.xp_applied_mask;
			state->economic_mask = record.economic_applied_mask;
			state->current = std::make_unique<critical_native_recovery_envelope>();
			native_quest_recovery_context context;
			if (!critical_native_quest_continuation_owner::copy_context(
				    command, state->current.get()) ||
			    state->current->phase !=
				    critical_native_recovery_phase::continuation_pending ||
			    native_quest_recovery_context_decode(
				    command, state->current->attachment, &context) !=
				    player_snapshot_codec_result::ok ||
			    context.publication_stage !=
				    native_quest_recovery_publication_stage::physically_proven ||
			    !context.receipt.present ||
			    (context.receipt.outcome != critical_apply_outcome::applied &&
			     context.receipt.outcome != critical_apply_outcome::already_applied))
				return false;
			state->started = context.publication_steps[5] != 0;
			state->effect_returned = state->durable_returned =
				context.publication_steps[5] == 2;
			state->retained_bytes = native_quest_frozen_bytes(*state);
			found = native_quest_frozen_recoveries.emplace(key, std::move(state)).first;
			if (!budget(nullptr, 0))
			{
				native_quest_frozen_recoveries.erase(found);
				return false;
			}
		}
		auto state = found->second;
		if (!critical_command_equal(command, state->command) ||
		    state->player_runtime_id != original_player_runtime_id ||
		    state->continuation != record.continuation ||
		    state->xp_mask != record.xp_applied_mask ||
		    state->economic_mask != record.economic_applied_mask)
			return false;
		const bool handed = drive(*state, true);
		*started = state->started && !state->effect_returned && !state->known_not_attempted;
		return handed;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
enum class native_quest_gameplay_phase : uint8_t
{
	preparing_acceptance,
	submitted_acceptance,
	original_give_hooks,
	choose_branch,
	preparing_consumption,
	submitted_consumption,
	blocked
};
struct native_quest_gameplay_flow
{
	uint64_t player_runtime_id = 0, native_runtime_id = 0, original_root_uid = 0;
	uint32_t player_pid = 0;
	int quester_id = -1, completion_index = 0;
	std::array<uint8_t, 9> give_hooks{};
	std::vector<std::unique_ptr<const native_quest_original_branch>> branches;
	size_t retained_bytes = sizeof(native_quest_gameplay_flow);
	bool branch_program_frozen = false, program_durable = false,
	     acceptance_handoff_durable = false;
	bool program_capture_failed = false;
	std::shared_ptr<const critical_command> acceptance_command, prepared_child, completed_child;
	std::unique_ptr<critical_native_recovery_envelope> recovery, pending, hook_return,
		child_context;
	size_t recovery_bytes = 0;
	uint8_t pending_kind = 0, pending_hook = 0;
	int pending_branch = 0;
	bool hook_not_attempted = false, child_allocated = false, child_observed = false;
	bool child_handoff_durable = false, child_retired = false, child_taken = false;
	bool child_frame_durable = false;
	bool acceptance_rejected = false, acceptance_retired = false;
	bool restored = false, cold_child_complete = false, original_program_lost = false;
	std::unique_ptr<critical_native_recovery_envelope> restored_original;
	critical_operation_id child_operation{};
	item_native_quest_gameplay_result child_result = item_native_quest_gameplay_result::pending;
	item_native_quest_preparation_token token;
	native_quest_gameplay_phase phase = native_quest_gameplay_phase::preparing_acceptance;
};
std::unordered_map<uint64_t, std::shared_ptr<native_quest_gameplay_flow>> native_quest_gameplay;
#ifndef __NO_MYSQL__
// Startup identities are original operation IDs, never the shared runtime zero.
std::unordered_map<std::string, std::shared_ptr<native_quest_gameplay_flow>>
	native_quest_passive_gameplay;
std::unordered_map<std::string, std::shared_ptr<const critical_native_recovery_envelope>>
	native_quest_passive_children;
bool native_quest_terminal_parent_live(const critical_native_recovery_envelope &parent) noexcept
{
	for (const auto &[runtime, flow] : native_quest_gameplay)
		if (flow->acceptance_command && flow->acceptance_command->operation_id.bytes ==
							parent.command.operation_id.bytes)
			return true;
	return false;
}
void native_quest_cleanup_retired_pair(const critical_native_recovery_envelope &parent,
				       const critical_native_recovery_envelope &child) noexcept
{
	const auto exact = [](const critical_native_recovery_envelope &a,
			      const critical_native_recovery_envelope &b)
	{
		return a.command.operation_id.bytes == b.command.operation_id.bytes &&
		       a.revision == b.revision && a.phase == b.phase &&
		       a.attachment == b.attachment;
	};
	// These maps contain passive mirrors of the exact coordinator records.
	// Successful retirement is already latched; cleanup performs no encoding/allocation.
	for (auto it = native_quest_passive_gameplay.begin();
	     it != native_quest_passive_gameplay.end(); ++it)
		if (it->second->recovery && exact(*it->second->recovery, parent))
		{
			native_quest_passive_gameplay.erase(it);
			break;
		}
	for (const auto &[runtime, flow] : native_quest_gameplay)
		if (flow->recovery && exact(*flow->recovery, parent))
			flow->acceptance_retired =
				true; // Keep a live token until its original take.
	for (auto it = native_quest_passive_children.begin();
	     it != native_quest_passive_children.end(); ++it)
		if (exact(*it->second, child))
		{
			native_quest_passive_children.erase(it);
			break;
		}
}
#endif
}

class quest_native_gameplay_owner final
{
    public:
	static bool begin(P_char mobile, P_char player, int quester_id, P_obj offering) noexcept;
	static void pulse() noexcept;

    private:
	friend class quest_native_frozen_continuation_owner;
	static bool restore_phase2(const critical_native_recovery_envelope &) noexcept;
	static bool restore_phase2_bounded(const critical_native_recovery_envelope &,
					   player_save_coin_replay_budget_scope_owner &,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t) noexcept;
	static void passive_pulse() noexcept;
#ifndef __NO_MYSQL__
	static bool charge_passive_child(native_quest_gameplay_flow &) noexcept;
#endif
	friend bool
	quest_native_frozen_continuation_owner::budget(const native_quest_frozen_recovery_state *,
						       size_t) noexcept;
	[[maybe_unused]] static bool charge_budget(size_t bytes) noexcept
	{
		return item_native_quest_gameplay_publication_owner::retained_budget(bytes);
	}
	[[maybe_unused]] static bool freeze_program(native_quest_gameplay_flow &) noexcept;
	static bool budget(const native_quest_gameplay_flow *replace = nullptr,
			   size_t replacement = 0) noexcept;
	static void erase(uint64_t) noexcept;
	static bool enroll(native_quest_gameplay_flow &, std::shared_ptr<const critical_command>,
			   bool rejected) noexcept;
	static bool checkpoint(native_quest_gameplay_flow &) noexcept;
	static bool prepare_context(native_quest_gameplay_flow &, native_quest_recovery_context &,
				    uint8_t kind, int branch = 0) noexcept;
	static bool hook_begin(native_quest_gameplay_flow &, uint8_t) noexcept;
	static bool hook_returned(native_quest_gameplay_flow &) noexcept;
	static bool persist_program(native_quest_gameplay_flow &) noexcept;
	static bool persist_branch(native_quest_gameplay_flow &, int, bool clear_child) noexcept;
	static bool allocate_child(native_quest_gameplay_flow &) noexcept;
	static bool persist_child_command(native_quest_gameplay_flow &,
					  std::shared_ptr<const critical_command>) noexcept;
	static bool finish_child(native_quest_gameplay_flow &, item_native_quest_gameplay_result,
				 std::shared_ptr<const critical_command>) noexcept;
	static bool charge_recovery(native_quest_gameplay_flow &, size_t) noexcept;
};

bool quest_native_gameplay_owner::budget(const native_quest_gameplay_flow *replace,
					 size_t replacement) noexcept
{
	size_t bytes = 0;
	for (const auto &[runtime, flow] : native_quest_gameplay)
	{
		const size_t retained = flow.get() == replace ? replacement : flow->retained_bytes;
		if (retained > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += retained;
	}
#ifndef __NO_MYSQL__
	for (const auto &[key, flow] : native_quest_passive_gameplay)
	{
		const size_t retained = flow.get() == replace ? replacement : flow->retained_bytes;
		if (retained > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += retained;
	}
	for (const auto &[key, record] : native_quest_passive_children)
	{
		const size_t retained = sizeof(*record) + 2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES +
					record->attachment.capacity();
		if (retained > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += retained;
	}
	for (const auto &[key, state] : native_quest_frozen_recoveries)
	{
		if (state->retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += state->retained_bytes;
	}
#endif
	return item_native_quest_gameplay_publication_owner::retained_budget(bytes);
}

bool quest_native_frozen_continuation_owner::budget(
	const native_quest_frozen_recovery_state *replace, size_t replacement) noexcept
{
#ifdef __NO_MYSQL__
	(void)replace;
	(void)replacement;
	return false;
#else
	if (!nevent_is_game_thread())
		return false;
	size_t bytes = 0;
	for (const auto &[runtime, flow] : native_quest_gameplay)
	{
		if (flow->retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += flow->retained_bytes;
	}
	for (const auto &[key, flow] : native_quest_passive_gameplay)
	{
		if (flow->retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += flow->retained_bytes;
	}
	for (const auto &[key, record] : native_quest_passive_children)
	{
		const size_t retained = sizeof(*record) + 2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES +
					record->attachment.capacity();
		if (retained > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += retained;
	}
	for (const auto &[key, state] : native_quest_frozen_recoveries)
	{
		const size_t charge = state.get() == replace ? replacement : state->retained_bytes;
		if (charge > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += charge;
	}
	return quest_native_gameplay_owner::charge_budget(bytes);
#endif
}
void quest_native_gameplay_owner::erase(uint64_t runtime) noexcept
{
	native_quest_gameplay.erase(runtime);
	(void)budget();
}
#ifndef __NO_MYSQL__
namespace
{
bool native_quest_gameplay_current(const native_quest_gameplay_flow &flow) noexcept
{
	const auto found = native_quest_gameplay.find(flow.native_runtime_id);
	return found != native_quest_gameplay.end() && found->second.get() == &flow;
}
size_t native_quest_gameplay_recovery_bytes(
	const native_quest_gameplay_flow &flow,
	const critical_native_recovery_envelope *pending = nullptr,
	const critical_native_recovery_envelope *returned = nullptr) noexcept
{
	size_t bytes = flow.acceptance_command ? CRITICAL_COMMAND_MAX_ENCODED_BYTES : 0;
	const auto add = [&](size_t value)
	{
		if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    value > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += value;
		return true;
	};
	if (flow.prepared_child && !add(CRITICAL_COMMAND_MAX_ENCODED_BYTES))
		return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
	if (flow.completed_child && !add(CRITICAL_COMMAND_MAX_ENCODED_BYTES))
		return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
	for (const auto *record : std::array<const critical_native_recovery_envelope *, 5>{
		     flow.recovery.get(), pending ? pending : flow.pending.get(),
		     returned ? returned : flow.hook_return.get(), flow.child_context.get(),
		     flow.restored_original.get() })
		if (record && (!add(sizeof(*record) + CRITICAL_COMMAND_MAX_ENCODED_BYTES) ||
			       !add(record->attachment.capacity())))
			return PLAYER_SAVE_PIPELINE_MAX_BYTES + 1;
	return bytes;
}
}

bool quest_native_gameplay_owner::charge_recovery(native_quest_gameplay_flow &flow,
						  size_t bytes) noexcept
{
	if (flow.recovery_bytes > flow.retained_bytes)
		return false;
	const size_t original = flow.retained_bytes - flow.recovery_bytes;
	if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - original || !budget(&flow, original + bytes))
		return false;
	flow.recovery_bytes = bytes;
	flow.retained_bytes = original + bytes;
	return true;
}

bool quest_native_gameplay_owner::checkpoint(native_quest_gameplay_flow &flow) noexcept
{
	if (!flow.pending)
		return true;
	if (!native_quest_gameplay_current(flow) || !flow.recovery ||
	    flow.phase == native_quest_gameplay_phase::blocked ||
	    (flow.pending_kind == 11 ?
		     (!flow.child_context ||
		      !critical_native_quest_continuation_owner::transition_pair(
			      *flow.recovery, *flow.child_context, flow.pending.get())) :
		     !critical_native_quest_continuation_owner::checkpoint_context(*flow.recovery,
										   *flow.pending)))
		return false; // Own the exact expected/successor until CAS durability is confirmed.
	if (flow.pending_kind == 11)
		flow.child_retired = true; // Latch the actual paired journal return before cleanup.
	if (!native_quest_gameplay_current(flow))
	{
		flow.phase = native_quest_gameplay_phase::blocked;
		return false;
	}
	flow.recovery = std::move(flow.pending);
	switch (flow.pending_kind)
	{
	case 1:
		flow.acceptance_handoff_durable = true;
		break;
	case 2:
		break; // The live not-attempted marker existed before the start write.
	case 3:
		flow.give_hooks[flow.pending_hook] = 2;
		break;
	case 4:
		flow.program_durable = true;
		break;
	case 11:
		// The exact latest child is now durable in the advanced parent. Remove
		// only the passive copy; no allocation or absence proof after paired I/O.
		for (auto child = native_quest_passive_children.begin();
		     child != native_quest_passive_children.end(); ++child)
			if (child->second->command.operation_id.bytes == flow.child_operation.bytes)
			{
				native_quest_passive_children.erase(child);
				break;
			}
		[[fallthrough]];
	case 5:
	case 8:
		flow.completion_index = flow.pending_branch;
		flow.child_allocated = flow.child_observed = flow.child_handoff_durable = false;
		flow.child_retired = flow.child_taken = false;
		flow.phase = native_quest_gameplay_phase::choose_branch;
		flow.child_operation = {};
		flow.prepared_child.reset();
		flow.child_frame_durable = false;
		flow.cold_child_complete = false;
		flow.completed_child.reset();
		flow.child_context.reset();
		break;
	case 6:
		flow.child_allocated = true;
		break;
	case 7:
		flow.child_handoff_durable = true;
		break;
	case 9:
		flow.child_frame_durable = true;
		break;
	case 10:
		flow.give_hooks.back() = 2;
		flow.program_durable = true;
		flow.hook_return.reset();
		break;
	default:
		flow.phase = native_quest_gameplay_phase::blocked;
		return false;
	}
	flow.pending_kind = 0;
	// Map pin, moves and checked scalar charging only after journal I/O.
	if (!charge_recovery(flow, native_quest_gameplay_recovery_bytes(flow)))
	{
		flow.phase = native_quest_gameplay_phase::blocked;
		return false;
	}
	return true;
}

bool quest_native_gameplay_owner::prepare_context(native_quest_gameplay_flow &flow,
						  native_quest_recovery_context &context,
						  uint8_t kind, int branch) noexcept
{
	try
	{
		if (!flow.acceptance_command || !flow.recovery || flow.pending ||
		    flow.recovery->revision == UINT64_MAX || !native_quest_gameplay_current(flow))
			return false;
		auto successor =
			std::make_unique<critical_native_recovery_envelope>(*flow.recovery);
		++successor->revision;
		if (native_quest_recovery_context_encode(*flow.acceptance_command, context,
							 &successor->attachment) !=
			    player_snapshot_codec_result::ok ||
		    !charge_recovery(flow,
				     native_quest_gameplay_recovery_bytes(flow, successor.get())))
			return false;
		flow.pending = std::move(successor);
		flow.pending_kind = kind;
		flow.pending_branch = branch;
		return checkpoint(flow);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::enroll(native_quest_gameplay_flow &flow,
					 std::shared_ptr<const critical_command> command,
					 bool rejected) noexcept
{
	try
	{
		if (!command || !native_quest_gameplay_current(flow))
			return false;
		if (flow.acceptance_command)
		{
			if (flow.acceptance_rejected != rejected ||
			    !critical_command_equal(*command, *flow.acceptance_command) ||
			    !checkpoint(flow))
				return false;
			if (rejected || flow.acceptance_handoff_durable)
				return true;
			native_quest_recovery_context retained;
			if (native_quest_recovery_context_decode(
				    *flow.acceptance_command, flow.recovery->attachment,
				    &retained) != player_snapshot_codec_result::ok)
				return false;
			retained.parent_acceptance = flow.acceptance_command->operation_id;
			return prepare_context(flow, retained, 1);
		}
		item_transfer_payload payload{};
		auto current = std::make_unique<critical_native_recovery_envelope>();
		native_quest_recovery_context context;
		if (!item_transfer_command_decode_payload(*command, &payload) ||
		    !item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    payload.native_mobile.action != item_native_mobile_action::acceptance ||
		    payload.native_recovery.player_pid != flow.player_pid ||
		    item_transfer_result_root(payload) != flow.original_root_uid ||
		    !critical_native_quest_continuation_owner::copy_context(*command,
									    current.get()) ||
		    current->phase != critical_native_recovery_phase::continuation_pending ||
		    native_quest_recovery_context_decode(*command, current->attachment, &context) !=
			    player_snapshot_codec_result::ok ||
		    context.publication_stage !=
			    native_quest_recovery_publication_stage::physically_proven ||
		    !context.receipt.present ||
		    (rejected ?
			     context.receipt.outcome != critical_apply_outcome::terminal_failure :
			     (context.receipt.error_code ||
			      (context.receipt.outcome != critical_apply_outcome::applied &&
			       context.receipt.outcome !=
				       critical_apply_outcome::already_applied))) ||
		    context.branch_program_frozen || context.child_handoff_stage ||
		    (!critical_operation_id_is_zero(context.parent_acceptance) &&
		     context.parent_acceptance.bytes != command->operation_id.bytes))
			return false;
		const size_t bytes = CRITICAL_COMMAND_MAX_ENCODED_BYTES + sizeof(*current) +
				     CRITICAL_COMMAND_MAX_ENCODED_BYTES +
				     current->attachment.capacity();
		if (!charge_recovery(flow, bytes))
			return false;
		flow.acceptance_command = std::move(command);
		flow.recovery = std::move(current);
		flow.give_hooks = context.give_hooks;
		flow.acceptance_rejected = rejected;
		if (rejected)
			return true; // Exact original rejection is observed through the private item owner.
		context.parent_acceptance = flow.acceptance_command->operation_id;
		return prepare_context(flow, context, 1);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::hook_begin(native_quest_gameplay_flow &flow,
					     uint8_t hook) noexcept
{
	try
	{
		if (!checkpoint(flow) || !flow.recovery || hook >= flow.give_hooks.size())
			return false;
		if (flow.hook_return)
			return flow.pending_hook == hook && flow.hook_not_attempted;
		if (flow.give_hooks[hook] != 0 || flow.recovery->revision >= UINT64_MAX - 1)
			return false; // No cold started/unreturned hook can acquire a live retry marker.
		native_quest_recovery_context context;
		if (native_quest_recovery_context_decode(*flow.acceptance_command,
							 flow.recovery->attachment, &context) !=
			    player_snapshot_codec_result::ok ||
		    context.give_hooks[hook] != 0)
			return false;
		auto began = std::make_unique<critical_native_recovery_envelope>(*flow.recovery);
		++began->revision;
		context.give_hooks[hook] = 1;
		if (native_quest_recovery_context_encode(*flow.acceptance_command, context,
							 &began->attachment) !=
		    player_snapshot_codec_result::ok)
			return false;
		auto returned = std::make_unique<critical_native_recovery_envelope>(*began);
		++returned->revision;
		context.give_hooks[hook] = 2;
		if (native_quest_recovery_context_encode(*flow.acceptance_command, context,
							 &returned->attachment) !=
			    player_snapshot_codec_result::ok ||
		    !charge_recovery(flow, native_quest_gameplay_recovery_bytes(flow, began.get(),
										returned.get())))
			return false;
		flow.pending = std::move(began);
		flow.hook_return = std::move(returned);
		flow.pending_kind = 2;
		flow.pending_hook = hook;
		flow.hook_not_attempted = true;
		return checkpoint(flow);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::hook_returned(native_quest_gameplay_flow &flow) noexcept
{
	if (!flow.hook_return || flow.hook_not_attempted ||
	    flow.give_hooks[flow.pending_hook] != 2 || flow.pending)
		return false;
	// The last actual return and its original post-GIVE program are one writer.
	// Never publish the pre-call hook-only returned carrier for this hook.
	if (static_cast<size_t>(flow.pending_hook) + 1 == flow.give_hooks.size())
		return persist_program(flow);
	flow.pending = std::move(flow.hook_return);
	flow.pending_kind = 3;
	return checkpoint(flow); // Entire returned writer existed before the original hook call.
}

bool quest_native_gameplay_owner::persist_program(native_quest_gameplay_flow &flow) noexcept
{
	try
	{
		if (!checkpoint(flow) || !flow.branch_program_frozen ||
		    flow.program_capture_failed || flow.original_program_lost ||
		    !std::all_of(flow.give_hooks.begin(), flow.give_hooks.end(),
				 [](uint8_t step) { return step == 2; }))
			return false;
		if (flow.program_durable)
			return true;
		native_quest_recovery_context context;
		if (!flow.recovery || native_quest_recovery_context_decode(
					      *flow.acceptance_command, flow.recovery->attachment,
					      &context) != player_snapshot_codec_result::ok)
			return false;
		const bool last_returned = context.give_hooks.back() == 1;
		if (last_returned)
		{
			if (!flow.hook_return || flow.hook_not_attempted ||
			    static_cast<size_t>(flow.pending_hook) + 1 != flow.give_hooks.size() ||
			    !std::all_of(context.give_hooks.begin(), context.give_hooks.end() - 1,
					 [](uint8_t step) { return step == 2; }))
				return false;
			context.give_hooks.back() = 2;
		}
		context.branches.reserve(flow.branches.size());
		for (const auto &branch : flow.branches)
		{
			native_quest_recovery_branch out;
			for (const auto &goal : branch->give)
				out.give.push_back(
					{ static_cast<uint8_t>(goal.goal_type), goal.number });
			for (const auto &goal : branch->receive)
				out.receive.push_back(
					{ static_cast<uint8_t>(goal.goal_type), goal.number });
			out.message = branch->message;
			out.disappear_message = branch->disappear_message;
			out.definition_id = branch->definition_id;
			out.message_present = branch->view.message != nullptr;
			out.disappear_message_present = branch->view.disappear_message != nullptr;
			out.echo_all = branch->view.echoAll != 0;
			out.disappear = branch->view.disappear != 0;
			context.branches.push_back(std::move(out));
		}
		context.branch_program_frozen = true;
		context.next_branch = static_cast<uint32_t>(flow.completion_index);
		// Captured immutable branches and the actual returned fact remain owned
		// on allocation, budget or journal refusal. Retry this same combined CAS.
		return prepare_context(flow, context, last_returned ? 10 : 4);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::persist_branch(native_quest_gameplay_flow &flow, int branch,
						 bool clear_child) noexcept
{
	try
	{
		if (!checkpoint(flow) || branch < 0 ||
		    static_cast<size_t>(branch) > flow.branches.size())
			return false;
		native_quest_recovery_context context;
		if (!flow.recovery ||
		    native_quest_recovery_context_decode(*flow.acceptance_command,
							 flow.recovery->attachment, &context) !=
			    player_snapshot_codec_result::ok ||
		    !context.branch_program_frozen ||
		    context.next_branch != static_cast<uint32_t>(flow.completion_index))
			return false;
		context.next_branch = static_cast<uint32_t>(branch);
		if (clear_child)
		{
			if (context.next_child_command.empty() || !flow.child_context ||
			    flow.child_retired || !flow.child_handoff_durable ||
			    !flow.child_taken ||
			    flow.child_result !=
				    item_native_quest_gameplay_result::prefix_applied ||
			    context.child_handoff_stage != 2 ||
			    context.next_child_operation.bytes != flow.child_operation.bytes ||
			    branch != flow.completion_index + 1 || !flow.prepared_child ||
			    !critical_command_equal(flow.child_context->command,
						    *flow.prepared_child))
				return false;
			context.latest_child_branch = static_cast<uint32_t>(flow.completion_index);
			context.latest_child_revision = flow.child_context->revision;
			context.latest_child_command = context.next_child_command;
			context.latest_child_attachment = flow.child_context->attachment;
			context.next_child_command.clear();
			context.next_child_operation = {};
			context.child_handoff_stage = 0;
		}
		return prepare_context(flow, context, clear_child ? 11 : 5, branch);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::allocate_child(native_quest_gameplay_flow &flow) noexcept
{
	try
	{
		if (!checkpoint(flow))
			return false;
		if (flow.child_allocated)
			return true;
		if (critical_operation_id_is_zero(flow.child_operation) &&
		    !critical_operation_id_generate(&flow.child_operation))
			return false;
		native_quest_recovery_context context;
		if (!flow.recovery ||
		    native_quest_recovery_context_decode(*flow.acceptance_command,
							 flow.recovery->attachment, &context) !=
			    player_snapshot_codec_result::ok ||
		    context.child_handoff_stage || !context.branch_program_frozen ||
		    context.next_branch != static_cast<uint32_t>(flow.completion_index))
			return false;
		context.next_child_operation = flow.child_operation;
		context.child_handoff_stage = 1;
		return prepare_context(flow, context, 6);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::persist_child_command(
	native_quest_gameplay_flow &flow, std::shared_ptr<const critical_command> command) noexcept
{
	try
	{
		if (!checkpoint(flow) || !flow.child_allocated || !command ||
		    command->operation_id.bytes != flow.child_operation.bytes)
			return false;
		if (flow.prepared_child && !critical_command_equal(*flow.prepared_child, *command))
			return false;
		if (flow.child_frame_durable)
			return flow.prepared_child !=
			       nullptr; // Confirmed CAS still retains the original command.
		native_quest_recovery_context context;
		std::vector<uint8_t> original_frame;
		if (!flow.recovery ||
		    native_quest_recovery_context_decode(*flow.acceptance_command,
							 flow.recovery->attachment, &context) !=
			    player_snapshot_codec_result::ok ||
		    context.child_handoff_stage != 1 ||
		    context.next_child_operation.bytes != command->operation_id.bytes ||
		    context.next_branch != static_cast<uint32_t>(flow.completion_index) ||
		    critical_command_encode(*command, &original_frame) !=
			    critical_command_codec_result::ok ||
		    original_frame.empty() ||
		    original_frame.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
		    (!context.next_child_command.empty() &&
		     context.next_child_command != original_frame))
			return false;
		if (!flow.prepared_child)
		{
			const size_t retained = native_quest_gameplay_recovery_bytes(flow);
			if (retained > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
			    CRITICAL_COMMAND_MAX_ENCODED_BYTES >
				    PLAYER_SAVE_PIPELINE_MAX_BYTES - retained ||
			    !charge_recovery(flow, retained + CRITICAL_COMMAND_MAX_ENCODED_BYTES))
				return false;
			flow.prepared_child = std::move(command);
		}
		context.next_child_command = std::move(original_frame);
		return prepare_context(flow, context, 9);
	}
	catch (...)
	{
		return false;
	}
}

bool quest_native_gameplay_owner::finish_child(
	native_quest_gameplay_flow &flow, item_native_quest_gameplay_result result,
	std::shared_ptr<const critical_command> command) noexcept
{
	try
	{
		if (!checkpoint(flow) || !flow.child_allocated || !flow.child_frame_durable ||
		    !flow.prepared_child || !command ||
		    !critical_command_equal(*command, *flow.prepared_child) ||
		    command->operation_id.bytes != flow.child_operation.bytes ||
		    (result != item_native_quest_gameplay_result::prefix_applied &&
		     result != item_native_quest_gameplay_result::completion_applied &&
		     result != item_native_quest_gameplay_result::rejected))
			return false;
		if (!flow.child_observed)
		{
			item_transfer_payload payload{};
			if (!item_transfer_command_decode_payload(*command, &payload) ||
			    !item_transfer_native_mobile_recovery_shape_valid(payload) ||
			    payload.native_mobile.action !=
				    item_native_mobile_action::consumption ||
			    payload.native_recovery.player_pid != flow.player_pid ||
			    payload.native_mobile.final_giver_pid != flow.player_pid ||
			    (result == item_native_quest_gameplay_result::prefix_applied &&
			     payload.continuation.kind != item_transfer_continuation_kind::none) ||
			    (result == item_native_quest_gameplay_result::completion_applied &&
			     payload.continuation.kind !=
				     item_transfer_continuation_kind::quest_offering))
				return false;
			auto child = std::make_unique<critical_native_recovery_envelope>();
			if (result != item_native_quest_gameplay_result::completion_applied)
			{
				native_quest_recovery_context context;
				if (!critical_native_quest_continuation_owner::copy_context(
					    *command, child.get()) ||
				    child->phase !=
					    critical_native_recovery_phase::continuation_pending ||
				    native_quest_recovery_context_decode(
					    *command, child->attachment, &context) !=
					    player_snapshot_codec_result::ok ||
				    context.publication_stage !=
					    native_quest_recovery_publication_stage::
						    physically_proven ||
				    !context.receipt.present ||
				    ((result == item_native_quest_gameplay_result::rejected) !=
				     (context.receipt.outcome ==
				      critical_apply_outcome::terminal_failure)))
					return false;
			}
			else
				child.reset(); // Reward owner retains the original child until genuine reward ACK.
			size_t bytes = native_quest_gameplay_recovery_bytes(flow);
			const size_t extra =
				CRITICAL_COMMAND_MAX_ENCODED_BYTES +
				(child ? sizeof(*child) + CRITICAL_COMMAND_MAX_ENCODED_BYTES +
						 child->attachment.capacity() :
					 0);
			if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
			    extra > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes ||
			    !charge_recovery(flow, bytes + extra))
				return false;
			flow.completed_child = std::move(command);
			flow.child_context = std::move(child);
			flow.child_result = result;
			flow.child_observed = true;
		}
		else if (flow.child_result != result ||
			 !critical_command_equal(*command, *flow.completed_child))
			return false;
		native_quest_recovery_context context;
		if (native_quest_recovery_context_decode(*flow.acceptance_command,
							 flow.recovery->attachment, &context) !=
			    player_snapshot_codec_result::ok ||
		    context.next_child_operation.bytes != flow.child_operation.bytes ||
		    context.next_branch != static_cast<uint32_t>(flow.completion_index) ||
		    context.child_handoff_stage != (flow.child_handoff_durable ? 2 : 1))
			return false;
		if (!flow.child_handoff_durable)
		{
			context.child_handoff_stage = 2;
			if (result == item_native_quest_gameplay_result::prefix_applied &&
			    flow.completion_index == INT_MAX)
				return false;
			// Keep the producing branch while its exact child frame remains.
			// Only original retirement/take followed by clear+advance moves it.
			if (!prepare_context(flow, context, 7))
				return false;
		}
		// Prefix waits for paired advance after take; rewards wait for exact genuine ACK.
		if (flow.child_context && !flow.child_retired &&
		    result == item_native_quest_gameplay_result::rejected)
		{
			if (!critical_native_quest_continuation_owner::transition_pair(
				    *flow.recovery, *flow.child_context, nullptr))
				return false;
			flow.child_retired = flow.acceptance_retired =
				true; // Only this actual paired retirement return supplies both facts.
			if (!native_quest_gameplay_current(flow))
				return false;
		}
		if (flow.child_retired)
		{
			// No allocation after the irreversible retirement. Retrying this
			// exact cleanup is safe even when the returned latch was already set.
			for (auto child = native_quest_passive_children.begin();
			     child != native_quest_passive_children.end(); ++child)
				if (child->second->command.operation_id.bytes ==
				    flow.child_operation.bytes)
				{
					native_quest_passive_children.erase(child);
					break;
				}
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#endif

#ifndef __NO_MYSQL__
namespace
{
bool native_quest_passive_equal(const critical_native_recovery_envelope &a,
				const critical_native_recovery_envelope &b)
{
	return a.phase == b.phase && a.revision == b.revision && a.attachment == b.attachment &&
	       critical_command_equal(a.command, b.command);
}
size_t native_quest_passive_bytes(const critical_native_recovery_envelope &record)
{
	const size_t fixed = sizeof(record) + 2 * CRITICAL_COMMAND_MAX_ENCODED_BYTES;
	return record.attachment.capacity() > PLAYER_SAVE_PIPELINE_MAX_BYTES - fixed ?
		       PLAYER_SAVE_PIPELINE_MAX_BYTES + 1 :
		       fixed + record.attachment.capacity();
}
bool native_quest_passive_total(size_t *output)
{
	size_t bytes = 0;
	const auto add = [&](size_t value)
	{
		if (value > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += value;
		return true;
	};
	for (const auto &[runtime, flow] : native_quest_gameplay)
		if (!add(flow->retained_bytes))
			return false;
	for (const auto &[key, flow] : native_quest_passive_gameplay)
		if (!add(flow->retained_bytes))
			return false;
	for (const auto &[key, record] : native_quest_passive_children)
		if (!add(native_quest_passive_bytes(*record)))
			return false;
	for (const auto &[key, state] : native_quest_frozen_recoveries)
		if (!add(state->retained_bytes))
			return false;
	*output = bytes;
	return true;
}
}
#endif

bool quest_native_gameplay_owner::restore_phase2(
	const critical_native_recovery_envelope &envelope) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	return false;
#else
	try
	{
		item_transfer_payload payload{};
		native_quest_recovery_context context;
		if (envelope.phase != critical_native_recovery_phase::continuation_pending ||
		    !envelope.revision ||
		    !item_transfer_command_decode_payload(envelope.command, &payload) ||
		    !item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    native_quest_recovery_context_decode(envelope.command, envelope.attachment,
							 &context) !=
			    player_snapshot_codec_result::ok ||
		    !context.receipt.present ||
		    context.publication_stage !=
			    native_quest_recovery_publication_stage::physically_proven)
			return false;
		const std::string key = quest_reward_operation_key(envelope.command.operation_id);
		for (const auto &[runtime, flow] : native_quest_gameplay)
			if (flow->acceptance_command &&
			    flow->acceptance_command->operation_id.bytes ==
				    envelope.command.operation_id.bytes)
				return flow->restored_original &&
				       native_quest_passive_equal(*flow->restored_original,
								  envelope);
		auto existing = native_quest_passive_gameplay.find(key);
		if (existing != native_quest_passive_gameplay.end())
			return existing->second->restored_original &&
			       native_quest_passive_equal(*existing->second->restored_original,
							  envelope);
		auto child_existing = native_quest_passive_children.find(key);
		if (child_existing != native_quest_passive_children.end())
			return native_quest_passive_equal(*child_existing->second, envelope);
		auto reward_existing = native_quest_frozen_recoveries.find(key);
		if (reward_existing != native_quest_frozen_recoveries.end())
			return reward_existing->second->restored_original &&
			       native_quest_passive_equal(
				       *reward_existing->second->restored_original, envelope);
		if (native_quest_passive_gameplay.size() + native_quest_passive_children.size() +
			    native_quest_frozen_recoveries.size() + native_quest_gameplay.size() >=
		    ITEM_MOVEMENT_PENDING_MAX)
			return false;
		if (payload.native_mobile.action == item_native_mobile_action::consumption)
		{
			if (payload.continuation.kind ==
				    item_transfer_continuation_kind::quest_offering &&
			    (context.receipt.outcome == critical_apply_outcome::applied ||
			     context.receipt.outcome == critical_apply_outcome::already_applied))
			{
				auto state = std::make_shared<native_quest_frozen_recovery_state>();
				state->key = key;
				state->command = envelope.command;
				state->continuation = payload.continuation.data;
				if (!quest_reward_continuation_decode(state->continuation.data(),
								      state->continuation.size(),
								      &state->terms) ||
				    (state->terms.version != 5 &&
				     !(state->terms.version == 6 && payload.native_cost.fee_only)) ||
				    state->terms.player_pid != payload.native_recovery.player_pid ||
				    payload.native_recovery.publication_terms.disappear)
					return false;
				state->current =
					std::make_unique<critical_native_recovery_envelope>(
						envelope);
				state->restored_original =
					std::make_unique<critical_native_recovery_envelope>(
						envelope);
				state->started = context.publication_steps[5] != 0;
				state->effect_returned = state->durable_returned =
					context.publication_steps[5] == 2;
				state->restored = true;
				state->retained_bytes = native_quest_frozen_bytes(*state);
				native_quest_frozen_recoveries.reserve(
					native_quest_frozen_recoveries.size() + 1);
				auto installed = native_quest_frozen_recoveries.emplace(key, state);
				size_t bytes = 0;
				if (!installed.second || !native_quest_passive_total(&bytes) ||
				    !item_native_quest_gameplay_publication_owner::restore_budget(
					    bytes))
				{
					if (installed.second)
						native_quest_frozen_recoveries.erase(
							installed.first);
					return false;
				}
				return true;
			}
			auto record =
				std::make_shared<const critical_native_recovery_envelope>(envelope);
			native_quest_passive_children.reserve(native_quest_passive_children.size() +
							      1);
			auto installed =
				native_quest_passive_children.emplace(key, std::move(record));
			size_t bytes = 0;
			if (!installed.second || !native_quest_passive_total(&bytes) ||
			    !item_native_quest_gameplay_publication_owner::restore_budget(bytes))
			{
				if (installed.second)
					native_quest_passive_children.erase(installed.first);
				return false;
			}
			return true;
		}
		auto flow = std::make_shared<native_quest_gameplay_flow>();
		flow->acceptance_command =
			std::make_shared<const critical_command>(envelope.command);
		flow->recovery = std::make_unique<critical_native_recovery_envelope>(envelope);
		flow->restored_original =
			std::make_unique<critical_native_recovery_envelope>(envelope);
		flow->player_pid = payload.native_recovery.player_pid;
		flow->original_root_uid = item_transfer_result_root(payload);
		flow->give_hooks = context.give_hooks;
		flow->branch_program_frozen = flow->program_durable = context.branch_program_frozen;
		flow->original_program_lost = !context.branch_program_frozen &&
					      std::all_of(context.give_hooks.begin(),
							  context.give_hooks.end(),
							  [](uint8_t step) { return step == 2; });
		flow->completion_index = static_cast<int>(context.next_branch);
		flow->acceptance_handoff_durable = context.parent_acceptance.bytes ==
						   envelope.command.operation_id.bytes;
		flow->acceptance_rejected = context.receipt.outcome ==
					    critical_apply_outcome::terminal_failure;
		flow->child_operation = context.next_child_operation;
		flow->child_allocated = context.child_handoff_stage != 0;
		flow->child_handoff_durable = context.child_handoff_stage == 2;
		flow->restored = true;
		if (!context.next_child_command.empty())
		{
			auto child = std::make_shared<critical_command>();
			if (critical_command_decode(context.next_child_command.data(),
						    context.next_child_command.size(),
						    child.get()) !=
			    critical_command_codec_result::ok)
				return false;
			flow->prepared_child = std::move(child);
			flow->child_frame_durable = true;
		}
		flow->branches.reserve(context.branches.size());
		size_t retained = sizeof(*flow) + key.capacity() + 1;
		const auto charge = [&](size_t value)
		{
			if (value > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained)
				return false;
			retained += value;
			return true;
		};
		if (!charge(flow->branches.capacity() *
			    sizeof(std::unique_ptr<const native_quest_original_branch>)))
			return false;
		for (const auto &original : context.branches)
		{
			auto branch = std::make_unique<native_quest_original_branch>();
			branch->message = original.message;
			branch->disappear_message = original.disappear_message;
			branch->definition_id = original.definition_id;
			const auto goals = [](const auto &from, std::vector<goal_data> &to)
			{
				to.reserve(from.size());
				for (const auto &goal : from)
					to.push_back({ static_cast<char>(goal.type), goal.number,
						       nullptr });
				for (size_t i = 1; i < to.size(); ++i)
					to[i - 1].next = &to[i];
			};
			goals(original.give, branch->give);
			goals(original.receive, branch->receive);
			branch->view.message = original.message_present ? branch->message.data() :
									  nullptr;
			branch->view.disappear_message = original.disappear_message_present ?
								 branch->disappear_message.data() :
								 nullptr;
			branch->view.echoAll = original.echo_all;
			branch->view.disappear = original.disappear;
			branch->view.give = branch->give.empty() ? nullptr : branch->give.data();
			branch->view.receive = branch->receive.empty() ? nullptr :
									 branch->receive.data();
			if (!charge(sizeof(*branch) + branch->give.capacity() * sizeof(goal_data) +
				    branch->receive.capacity() * sizeof(goal_data) +
				    branch->message.capacity() + 1 +
				    branch->disappear_message.capacity() + 1 +
				    branch->definition_id.capacity() + 1))
				return false;
			flow->branches.push_back(std::move(branch));
		}
		flow->recovery_bytes = native_quest_gameplay_recovery_bytes(*flow);
		if (!charge(flow->recovery_bytes))
			return false;
		flow->retained_bytes = retained;
		flow->phase = flow->branch_program_frozen ?
				      native_quest_gameplay_phase::choose_branch :
				      native_quest_gameplay_phase::original_give_hooks;
		native_quest_passive_gameplay.reserve(native_quest_passive_gameplay.size() + 1);
		auto installed = native_quest_passive_gameplay.emplace(key, flow);
		size_t bytes = 0;
		if (!installed.second || !native_quest_passive_total(&bytes) ||
		    !item_native_quest_gameplay_publication_owner::restore_budget(bytes))
		{
			if (installed.second)
				native_quest_passive_gameplay.erase(installed.first);
			return false;
		}
		return true; // Exact original branch/hook/child progress, no effect or coordinator call.
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_native_frozen_continuation_owner::restore(
	const critical_native_recovery_envelope &envelope) noexcept
{
	return quest_native_gameplay_owner::restore_phase2(envelope);
}

bool quest_native_frozen_continuation_owner::published(const critical_command &command) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	return false;
#else
	if (!nevent_is_game_thread())
		return false;
	try
	{
		critical_native_recovery_envelope current;
		return critical_native_quest_continuation_owner::copy_context(command, &current) &&
		       restore(current);
	}
	catch (...)
	{
		return false;
	}
#endif
}

void quest_native_gameplay_owner::passive_pulse() noexcept
{
#ifndef __NO_MYSQL__
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_sql())
		return;
	try
	{
		std::vector<std::string> keys;
		keys.reserve(native_quest_passive_gameplay.size());
		for (const auto &[key, flow] : native_quest_passive_gameplay)
			keys.push_back(key);
		for (const auto &key : keys)
		{
			auto found = native_quest_passive_gameplay.find(key);
			if (found == native_quest_passive_gameplay.end())
				continue;
			auto flow = found->second;
			bool terminal_held = false;
			for (const auto &[reward_key, state] : native_quest_frozen_recoveries)
				if (state->terminal_pair_attempted && state->terminal_parent &&
				    flow->acceptance_command &&
				    state->terminal_parent->command.operation_id.bytes ==
					    flow->acceptance_command->operation_id.bytes)
				{
					terminal_held = true;
					break;
				}
			if (terminal_held)
				continue; // The original terminal pair owns this exact passive parent.
			P_char player = nullptr, mobile = nullptr;
			bool bound = false;
			if (flow->prepared_child)
			{
				item_native_quest_preparation_token token;
				const bool original_child_retained =
					item_native_quest_gameplay_publication_owner::restored_token(
						*flow->prepared_child, &token);
				if (original_child_retained &&
				    item_native_quest_gameplay_publication_owner::restored_participant(
					    *flow->prepared_child, &token, &player, &mobile))
				{
					flow->token = token;
					flow->phase =
						native_quest_gameplay_phase::submitted_consumption;
					bound = true;
				}
				else
				{
					// An admitted original child keeps its exact token while its
					// genuine execution/readback is pending; never reprepare it.
					if (original_child_retained)
						continue;
					const std::string child_key =
						quest_reward_operation_key(flow->child_operation);
					std::shared_ptr<const critical_native_recovery_envelope>
						child;
					auto retained =
						native_quest_passive_children.find(child_key);
					if (retained != native_quest_passive_children.end())
						child = retained->second;
					auto reward =
						native_quest_frozen_recoveries.find(child_key);
					if (!child &&
					    reward != native_quest_frozen_recoveries.end() &&
					    reward->second->current)
						child = std::make_shared<
							const critical_native_recovery_envelope>(
							*reward->second->current);
					if (child &&
					    critical_command_equal(child->command,
								   *flow->prepared_child) &&
					    item_native_quest_gameplay_publication_owner::
						    restored_readback(*child, &player, &mobile))
					{
						native_quest_recovery_context context;
						item_transfer_payload payload{};
						if (native_quest_recovery_context_decode(
							    child->command, child->attachment,
							    &context) !=
							    player_snapshot_codec_result::ok ||
						    !item_transfer_command_decode_payload(
							    child->command, &payload))
							continue;
						auto next_context =
							payload.continuation.kind ==
										item_transfer_continuation_kind::
											none ||
									context.receipt.outcome ==
										critical_apply_outcome::
											terminal_failure ?
								std::make_unique<
									critical_native_recovery_envelope>(
									*child) :
								nullptr;
						auto old_completed = flow->completed_child;
						auto old_context = std::move(flow->child_context);
						const auto old_result = flow->child_result;
						const auto old_phase = flow->phase;
						const bool old_observed = flow->child_observed,
							   old_cold = flow->cold_child_complete;
						flow->completed_child = flow->prepared_child;
						flow->child_context = std::move(next_context);
						flow->child_result =
							context.receipt.outcome ==
									critical_apply_outcome::
										terminal_failure ?
								item_native_quest_gameplay_result::
									rejected :
							payload.continuation.kind ==
									item_transfer_continuation_kind::
										none ?
								item_native_quest_gameplay_result::
									prefix_applied :
								item_native_quest_gameplay_result::
									completion_applied;
						flow->child_observed = true;
						flow->cold_child_complete = true;
						flow->phase = native_quest_gameplay_phase::
							submitted_consumption;
						if (!charge_passive_child(*flow))
						{
							flow->completed_child =
								std::move(old_completed);
							flow->child_context =
								std::move(old_context);
							flow->child_result = old_result;
							flow->phase = old_phase;
							flow->child_observed = old_observed;
							flow->cold_child_complete = old_cold;
							continue;
						}
						bound = true;
					}
				}
				if (!bound && flow->child_handoff_durable)
					continue;
			}
			if (!bound)
			{
				native_quest_recovery_context original;
				if (native_quest_recovery_context_decode(
					    flow->recovery->command, flow->recovery->attachment,
					    &original) != player_snapshot_codec_result::ok)
					continue;
				critical_native_recovery_envelope latest;
				const auto *cut = flow->recovery.get();
				if (!original.latest_child_command.empty())
				{
					if (critical_command_decode(
						    original.latest_child_command.data(),
						    original.latest_child_command.size(),
						    &latest.command) !=
					    critical_command_codec_result::ok)
						continue;
					latest.revision = original.latest_child_revision;
					latest.phase =
						critical_native_recovery_phase::continuation_pending;
					latest.attachment =
						std::move(original.latest_child_attachment);
					cut = &latest;
				}
				// Genuine fresh SQL/native/world proof and confirmed rollback inside
				// this owner authenticate the latest actual cut before actor rebinding.
				if (!item_native_quest_gameplay_publication_owner::restored_readback(
					    *cut, &player, &mobile))
					continue;
			}
			if (!player || !mobile || !mobile->runtime_id || !player->runtime_id ||
			    native_quest_gameplay.count(mobile->runtime_id))
				continue;
			const int quester = find_quester_id(GET_RNUM(mobile));
			if (quester < 0)
				continue;
			native_quest_gameplay.reserve(native_quest_gameplay.size() + 1);
			auto installed = native_quest_gameplay.emplace(mobile->runtime_id, flow);
			if (!installed.second)
				continue;
			flow->player_runtime_id = player->runtime_id;
			flow->native_runtime_id = mobile->runtime_id;
			flow->quester_id = quester;
			native_quest_passive_gameplay.erase(found);
			// Same charged owner moved between maps; no aggregate increase.
		}
		std::vector<std::string> rewards;
		rewards.reserve(native_quest_frozen_recoveries.size());
		for (const auto &[key, state] : native_quest_frozen_recoveries)
			if (state->restored && !state->retired)
				rewards.push_back(key);
		for (const auto &key : rewards)
		{
			auto found = native_quest_frozen_recoveries.find(key);
			if (found == native_quest_frozen_recoveries.end())
				continue;
			auto state = found->second;
			if (quest_native_frozen_continuation_owner::retire_completed(*state))
				continue; // Actual ACK cleanup does not require the obsolete pre-reward cut.
			if (state->terminal_pair_attempted)
				continue; // No actor rebind or effect context can replace a pair retry.
			if (state->player_runtime_id)
			{
				(void)quest_native_frozen_continuation_owner::drive(*state, true);
				continue;
			}
			P_char player = nullptr, mobile = nullptr;
			if (!item_native_quest_gameplay_publication_owner::restored_readback(
				    *state->current, &player, &mobile))
				continue;
			MYSQL *connection = sql_pool_acquire();
			player_sql_pool_lease lease(connection);
			if (!connection || player_sql_idle_error(connection))
				continue;
			player_sql_cleanup cleanup;
			player_sql_transaction_cleanup transaction(connection, cleanup);
			transaction.starting();
			std::vector<quest_reward_obligation_record> records;
			unsigned int error = 0;
			bool read = false;
			try
			{
				read = !mysql_real_query(connection, "START TRANSACTION", 17) &&
				       quest_reward_obligation_repository_pending(
					       connection, state->terms.player_pid, &records,
					       &error) == quest_reward_obligation_result::ok &&
				       transaction.same_session();
			}
			catch (...)
			{
				read = false;
			}
			transaction.finish();
			lease.reuse(cleanup);
			if (!read || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
			    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
				continue;
			const auto record =
				std::find_if(records.begin(), records.end(),
					     [&](const auto &value) {
						     return value.offering_operation.bytes ==
							    state->command.operation_id.bytes;
					     });
			if (record == records.end() ||
			    record->continuation != state->continuation ||
			    (record->terms.version != 5 && record->terms.version != 6) ||
			    record->terms.player_pid != state->terms.player_pid)
				continue;
			state->player_runtime_id = player->runtime_id;
			state->xp_mask = record->xp_applied_mask;
			state->economic_mask = record->economic_applied_mask;
			(void)quest_native_frozen_continuation_owner::drive(*state, true);
		}
	}
	catch (...)
	{ /* Original passive records remain owned on every refusal. */
	}
#endif
}

#ifndef __NO_MYSQL__
bool quest_native_gameplay_owner::charge_passive_child(native_quest_gameplay_flow &flow) noexcept
{
	if (flow.recovery_bytes > flow.retained_bytes)
		return false;
	const size_t original = flow.retained_bytes - flow.recovery_bytes;
	const size_t replacement = native_quest_gameplay_recovery_bytes(flow);
	if (replacement > PLAYER_SAVE_PIPELINE_MAX_BYTES - original)
		return false;
	const size_t old_retained = flow.retained_bytes;
	flow.recovery_bytes = replacement;
	flow.retained_bytes = original + replacement;
	size_t bytes = 0;
	if (native_quest_passive_total(&bytes) &&
	    item_native_quest_gameplay_publication_owner::restore_budget(bytes))
		return true;
	flow.retained_bytes = old_retained;
	flow.recovery_bytes = old_retained - original;
	return false;
}
#endif

bool quest_native_gameplay_owner::freeze_program(native_quest_gameplay_flow &flow) noexcept
{
	if (flow.branch_program_frozen)
		return true;
	// A later catalog cannot replace the lost original last-hook cut.
	if (flow.original_program_lost)
		return false;
	const size_t original_budget = flow.retained_bytes;
	size_t retained = original_budget;
	try
	{
		std::vector<std::unique_ptr<const native_quest_original_branch>> branches;
		std::unordered_set<const quest_complete_data *> seen_branches;
		const auto charge = [&](size_t bytes)
		{
			if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained ||
			    !budget(&flow, retained + bytes))
				return false;
			retained += bytes;
			return true;
		};
		for (const auto *source = quest_index[flow.quester_id].quest_complete; source;
		     source = source->next)
		{
			if (!seen_branches.insert(source).second ||
			    branches.size() >= static_cast<size_t>(INT_MAX) ||
			    !charge(sizeof(native_quest_original_branch) +
				    2 * sizeof(std::unique_ptr<const native_quest_original_branch>)))
				throw std::bad_alloc();
			auto branch = std::make_unique<native_quest_original_branch>();
			const auto copy_text = [&](const char *text, std::string &out)
			{
				if (!text)
					return true;
				const size_t size = strnlen(text, MAX_STRING_LENGTH);
				if (size == MAX_STRING_LENGTH || !charge(size + 1))
					return false;
				out.assign(text, size);
				return true;
			};
			const auto copy_goals =
				[&](const goal_data *head, std::vector<goal_data> &out)
			{
				std::unordered_set<const goal_data *> seen;
				for (auto *goal = head; goal; goal = goal->next)
				{
					if (!seen.insert(goal).second ||
					    !charge(2 * sizeof(goal_data)))
						return false;
					out.push_back({ goal->goal_type, goal->number, nullptr });
				}
				for (size_t i = 1; i < out.size(); ++i)
					out[i - 1].next = &out[i];
				return true;
			};
			if (!copy_text(source->message, branch->message) ||
			    !copy_text(source->disappear_message, branch->disappear_message) ||
			    !copy_goals(source->give, branch->give) ||
			    !copy_goals(source->receive, branch->receive))
				throw std::bad_alloc();
			if (const auto *definition =
				    zone_story_quest_production::definition_id_for(source))
			{
				if (definition->size() > QUEST_REWARD_MAX_DEFINITION_ID_BYTES ||
				    !charge(definition->size() + 1))
					throw std::bad_alloc();
				branch->definition_id = *definition;
			}
			branch->view.message = source->message ? branch->message.data() : nullptr;
			branch->view.disappear_message = source->disappear_message ?
								 branch->disappear_message.data() :
								 nullptr;
			branch->view.echoAll = source->echoAll;
			branch->view.disappear = source->disappear;
			branch->view.give = branch->give.empty() ? nullptr : branch->give.data();
			branch->view.receive = branch->receive.empty() ? nullptr :
									 branch->receive.data();
			branches.push_back(std::move(branch));
		}
		flow.branches = std::move(branches);
		flow.retained_bytes = retained;
		flow.branch_program_frozen = true;
		return true;
	}
	catch (...)
	{
		(void)budget(&flow, original_budget);
		return false;
	}
}

bool quest_native_gameplay_owner::begin(P_char mobile, P_char player, int quester_id,
					P_obj offering) noexcept
{
#ifdef __NO_MYSQL__
	(void)mobile;
	(void)player;
	(void)quester_id;
	(void)offering;
	return false;
#else
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_sql() ||
	    !mobile || !player || !offering || !IS_NPC(mobile) || !IS_PC(player) ||
	    !player->only.pc || GET_PID(player) <= 0 || !player->runtime_id ||
	    !mobile->runtime_id || player->in_room == NOWHERE ||
	    player->in_room != mobile->in_room || quester_id < 0 ||
	    quester_id >= number_of_quests || quest_index[quester_id].quester != GET_RNUM(mobile) ||
	    !OBJ_CARRIED_BY(offering, player) ||
	    (IS_SET(offering->extra_flags, ITEM_NODROP) && !IS_TRUSTED(player)) ||
	    IS_OBJ_STAT2(offering, ITEM2_SOULBIND) ||
	    IS_SET(offering->extra2_flags, ITEM2_CRAFTED) || training_dummy_is(mobile) ||
	    collector_presence_is_npc(mobile) || GET_RNUM(mobile) == real_mobile(250) ||
	    IS_AFFECTED(mobile, AFF_WRAITHFORM) ||
	    (!IS_TRUSTED(player) && IS_CARRYING_N(mobile) >= CAN_CARRY_N(mobile) &&
	     !mob_index[GET_RNUM(mobile)].qst_func) ||
	    (IS_ARTIFACT(offering) && racewar(player, mobile)))
		return false;
	try
	{
		quest_mobile_native_reference actual_reference;
		if (!quest_mobile_native_reference_copy(mobile, mobile->runtime_id,
							&actual_reference))
			return false;
		const auto conflicts = [&](const critical_command &original)
		{
			item_transfer_payload retained{};
			return !item_transfer_command_decode_payload(original, &retained) ||
			       retained.native_recovery.player_pid ==
				       static_cast<uint32_t>(GET_PID(player)) ||
			       retained.native_mobile.reference.mobile_instance_id ==
				       actual_reference.mobile_instance_id;
		};
		for (const auto &[key, flow] : native_quest_passive_gameplay)
			if (!flow->acceptance_command || conflicts(*flow->acceptance_command))
				return false;
		for (const auto &[key, record] : native_quest_passive_children)
			if (conflicts(record->command))
				return false;
		for (const auto &[key, state] : native_quest_frozen_recoveries)
			if (state->restored && !state->retired && conflicts(state->command))
				return false;
		if (native_quest_gameplay.size() + native_quest_passive_gameplay.size() +
				    native_quest_passive_children.size() +
				    native_quest_frozen_recoveries.size() >=
			    ITEM_MOVEMENT_PENDING_MAX ||
		    native_quest_gameplay.count(mobile->runtime_id))
			return false;
		for (const auto &[runtime, flow] : native_quest_gameplay)
			if (flow->player_pid == static_cast<uint32_t>(GET_PID(player)))
				return false;
		auto flow = std::make_shared<native_quest_gameplay_flow>();
		flow->player_runtime_id = player->runtime_id;
		flow->native_runtime_id = mobile->runtime_id;
		flow->player_pid = static_cast<uint32_t>(GET_PID(player));
		flow->quester_id = quester_id;
		flow->original_root_uid = offering->obj_uid;
		auto inserted = native_quest_gameplay.emplace(flow->native_runtime_id, flow);
		if (!inserted.second)
			return false;
		if (!budget())
		{
			erase(flow->native_runtime_id);
			return false;
		}
		const auto prepared = item_native_quest_preparation_owner::begin_acceptance(
			player, mobile, offering, &flow->token);
		if (prepared == item_native_quest_preparation_state::refused)
		{
			erase(flow->native_runtime_id);
			return false;
		}
		if (!item_native_quest_gameplay_publication_owner::retain(flow->token))
		{
			if (item_native_quest_preparation_owner::cancel(flow->token))
				erase(flow->native_runtime_id);
			else
				flow->phase = native_quest_gameplay_phase::blocked;
			return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

void quest_native_gameplay_owner::pulse() noexcept
{
	quest_native_frozen_continuation_owner::pulse();
#ifndef __NO_MYSQL__
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_sql())
		return;
	try
	{
		passive_pulse();
		std::vector<uint64_t> keys;
		keys.reserve(native_quest_gameplay.size());
		for (const auto &[runtime, flow] : native_quest_gameplay)
			keys.push_back(runtime);
		for (uint64_t runtime : keys)
		{
			auto found = native_quest_gameplay.find(runtime);
			if (found == native_quest_gameplay.end())
				continue;
			auto flow = found->second;
			if (flow->phase == native_quest_gameplay_phase::blocked ||
			    !checkpoint(*flow))
				continue;
			P_char player = find_character_by_runtime_id(flow->player_runtime_id);
			P_char mobile = find_character_by_runtime_id(flow->native_runtime_id);
			if (!player || !mobile || !IS_PC(player) || !player->only.pc ||
			    !IS_NPC(mobile) ||
			    GET_PID(player) != static_cast<int>(flow->player_pid))
				continue; // Real body absence retains original operation and driver.
			if (flow->restored && flow->acceptance_rejected)
			{
				if (!flow->acceptance_retired &&
				    !critical_native_quest_continuation_owner::retire_continuation(
					    *flow->recovery))
					continue;
				flow->acceptance_retired = true;
				erase(runtime);
				continue;
			}
			if (flow->restored && !flow->acceptance_handoff_durable &&
			    !enroll(*flow, flow->acceptance_command, false))
				continue;
			const auto phase = flow->phase;
			if (phase == native_quest_gameplay_phase::submitted_acceptance ||
			    phase == native_quest_gameplay_phase::submitted_consumption)
			{
				if (flow->child_taken)
				{
					(void)persist_branch(*flow, flow->completion_index + 1,
							     true);
					continue; // Retry the exact clear writer; the original child was already taken.
				}
				std::shared_ptr<const critical_command> original;
				const auto publication =
					flow->cold_child_complete ?
						flow->child_result :
						item_native_quest_gameplay_publication_owner::
							observe_completed(flow->token, &original);
				if (flow->cold_child_complete)
					original = flow->completed_child;
				if (publication == item_native_quest_gameplay_result::pending)
					continue;
				if (publication == item_native_quest_gameplay_result::unavailable)
				{
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				if (phase == native_quest_gameplay_phase::submitted_acceptance)
				{
					if ((publication != item_native_quest_gameplay_result::
								    acceptance_applied &&
					     publication !=
						     item_native_quest_gameplay_result::rejected) ||
					    !enroll(*flow, original,
						    publication ==
							    item_native_quest_gameplay_result::
								    rejected))
						continue;
					if (flow->acceptance_rejected && !flow->acceptance_retired)
					{
						if (!critical_native_quest_continuation_owner::
							    retire_continuation(*flow->recovery))
							continue;
						if (!native_quest_gameplay_current(*flow))
							continue;
						flow->acceptance_retired = true;
					}
					const auto taken =
						item_native_quest_gameplay_publication_owner::take(
							flow->token);
					if (taken == item_native_quest_gameplay_result::pending)
						continue;
					if (taken != publication)
					{
						flow->phase = native_quest_gameplay_phase::blocked;
						continue;
					}
					if (flow->acceptance_rejected)
						erase(runtime);
					else
						flow->phase = native_quest_gameplay_phase::
							original_give_hooks;
					continue; // Complete original command/body and phase2 owner survived take.
				}
				if (!finish_child(*flow, publication, original))
					continue;
				// Rejection already retired the exact pair. A successful reward keeps
				// both journal records until its original frozen ACK owner proves completion.
				const auto taken =
					flow->cold_child_complete ?
						flow->child_result :
						item_native_quest_gameplay_publication_owner::take(
							flow->token);
				if (flow->cold_child_complete && flow->child_handoff_durable &&
				    (flow->child_retired ||
				     publication ==
					     item_native_quest_gameplay_result::completion_applied))
				{
					// finish_child already removed any actually retired passive
					// child without allocation; the reward owner remains separate.
					flow->cold_child_complete = false;
				}
				if (taken == item_native_quest_gameplay_result::pending)
					continue;
				if (taken != publication)
				{
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				if (publication !=
				    item_native_quest_gameplay_result::prefix_applied)
				{
					erase(runtime); // Successful reward child retains its separate genuine-ACK owner.
					continue;
				}
				flow->child_taken = true;
				(void)persist_branch(*flow, flow->completion_index + 1, true);
				continue; // Failed consumed prefix resumes the durably retained NEXT branch.
			}
			if (phase == native_quest_gameplay_phase::original_give_hooks)
			{
				if (flow->program_capture_failed)
					continue; // Exact returned writer may settle; no recapture.
				bool held = false;
				for (size_t i = 0; i < flow->give_hooks.size(); ++i)
				{
					if (flow->give_hooks[i] == 1)
					{
						held = true;
						break;
					}
					if (flow->give_hooks[i] == 2)
						continue;
					P_obj root = nullptr;
					if (!item_native_quest_gameplay_publication_owner::
						    observe_give(flow->player_runtime_id,
								 flow->player_pid,
								 flow->native_runtime_id,
								 flow->original_root_uid, &player,
								 &mobile, &root))
					{
						held = true;
						break;
					}
					const auto still_owned =
						native_quest_gameplay.find(runtime);
					if (still_owned == native_quest_gameplay.end() ||
					    still_owned->second != flow)
					{
						held = true;
						break;
					}
					if (!hook_begin(*flow, static_cast<uint8_t>(i)) ||
					    !item_native_quest_gameplay_publication_owner::
						    observe_give(flow->player_runtime_id,
								 flow->player_pid,
								 flow->native_runtime_id,
								 flow->original_root_uid, &player,
								 &mobile, &root) ||
					    !native_quest_gameplay_current(*flow))
					{
						held = true;
						break;
					}
					flow->give_hooks[i] = 1;
					flow->hook_not_attempted = false;
					switch (i)
					{
					case 0:
						if (IS_TRUSTED(player) && IS_ARTIFACT(root))
							logit(LOG_OBJ,
							      "%s gives artifact %s (%d) to %s.",
							      J_NAME(player),
							      root->short_description,
							      obj_index[root->R_num].virtual_number,
							      J_NAME(mobile));
						break;
					case 1:
						if (IS_TRUSTED(player))
							wizlog(GET_LEVEL(player),
							       "%s gives %s to %s.", J_NAME(player),
							       root->short_description,
							       J_NAME(mobile));
						break;
					case 2:
						if (IS_TRUSTED(player))
							logit(LOG_WIZ, "%s gives %s to %s.",
							      J_NAME(player),
							      root->short_description,
							      J_NAME(mobile));
						break;
					case 3:
						if (IS_TRUSTED(player))
							sql_log(player, WIZLOG, "Gave %s to %s.",
								root->short_description,
								J_NAME(mobile));
						break;
					case 4:
						artifact_switch_check(player, root);
						break;
					case 5:
						char_light(player);
						break;
					case 6:
						room_light(player->in_room, REAL);
						break;
					case 7:
						nq_action_check(player, mobile, nullptr);
						break;
					case 8:
						studioproc_give(mobile, root, player);
						break;
					}
					flow->give_hooks[i] = 2;
					if (i + 1 == flow->give_hooks.size())
					{
						P_obj ignored = nullptr;
						if (!item_native_quest_gameplay_publication_owner::
							    observe_give(flow->player_runtime_id,
									 flow->player_pid,
									 flow->native_runtime_id, 0,
									 &player, &mobile,
									 &ignored) ||
						    !native_quest_gameplay_current(*flow) ||
						    !freeze_program(*flow))
							flow->program_capture_failed = true;
						// Capture once before any last-return writer can become durable.
						// A failed capture retains actual return and durable start,
						// never permission to call again or adopt a later catalog.
					}
					const bool returned_durable =
						!flow->program_capture_failed &&
						hook_returned(*flow);
					if (!returned_durable || flow->program_capture_failed)
					{
						held = true;
						break;
					}
					// Root removal is legal only when its original callback's
					// existing mutation owner supplies subsequent durable proof.
					// No raw pointer survives the call, including purge/reuse.
					P_obj ignored = nullptr;
					if (!item_native_quest_gameplay_publication_owner::
						    observe_give(flow->player_runtime_id,
								 flow->player_pid,
								 flow->native_runtime_id, 0,
								 &player, &mobile, &ignored))
					{
						held = true;
						break;
					}
				}
				if (!held && freeze_program(*flow) && persist_program(*flow))
					flow->phase = native_quest_gameplay_phase::choose_branch;
				continue; // Freeze each addressed decision only AFTER real GIVE callbacks.
			}
			if (phase == native_quest_gameplay_phase::choose_branch)
			{
				if (flow->quester_id < 0 || flow->completion_index < 0)
				{
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				if (!flow->branch_program_frozen || !flow->program_durable)
				{
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				if (static_cast<size_t>(flow->completion_index) >=
				    flow->branches.size())
				{
					P_obj ignored = nullptr;
					if (!flow->child_allocated &&
					    item_native_quest_gameplay_publication_owner::
						    observe_give(flow->player_runtime_id,
								 flow->player_pid,
								 flow->native_runtime_id, 0,
								 &player, &mobile, &ignored) &&
					    critical_native_quest_continuation_owner::
						    retire_continuation(*flow->recovery))
						erase(runtime);
					continue; // Only original observed no-match completion retires this phase2 carrier.
				}
				if (!allocate_child(*flow))
					continue;
				P_obj ignored = nullptr;
				if (!item_native_quest_gameplay_publication_owner::observe_give(
					    flow->player_runtime_id, flow->player_pid,
					    flow->native_runtime_id, 0, &player, &mobile, &ignored))
					continue;
				native_quest_recovery_context actual_trigger_context;
				if (!flow->acceptance_command || !flow->recovery ||
				    native_quest_recovery_context_decode(*flow->acceptance_command,
									 flow->recovery->attachment,
									 &actual_trigger_context) !=
					    player_snapshot_codec_result::ok)
				{
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				const auto prepared =
					quest_native_completion_owner::prepare_original(
						mobile, player, flow->quester_id,
						flow->completion_index,
						flow->branches[flow->completion_index].get(),
						&flow->child_operation, &flow->token,
						flow->acceptance_command.get(),
						&actual_trigger_context.receipt);
				if (prepared == item_native_quest_preparation_state::not_matched)
				{
					if (flow->completion_index == INT_MAX)
					{
						flow->phase = native_quest_gameplay_phase::blocked;
						continue;
					}
					(void)persist_branch(*flow, flow->completion_index + 1,
							     true);
					continue;
				}
				if (prepared == item_native_quest_preparation_state::refused)
				{
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				if (!item_native_quest_gameplay_publication_owner::retain(
					    flow->token))
				{
					item_native_quest_preparation_owner::cancel(flow->token);
					flow->phase = native_quest_gameplay_phase::blocked;
					continue;
				}
				flow->phase = native_quest_gameplay_phase::preparing_consumption;
				continue;
			}
			const bool acceptance = phase ==
						native_quest_gameplay_phase::preparing_acceptance;
			const auto prepared =
				acceptance ? item_native_quest_preparation_owner::poll_acceptance(
						     flow->token, player, mobile) :
					     item_native_quest_preparation_owner::poll_consumption(
						     flow->token, player, mobile);
			if (prepared == item_native_quest_preparation_state::pending)
				continue;
			if (prepared != item_native_quest_preparation_state::ready)
			{
				if (item_native_quest_preparation_owner::cancel(flow->token) &&
				    !flow->recovery)
					erase(runtime);
				else
					flow->phase = native_quest_gameplay_phase::blocked;
				// Existing definite cancellation is preserved. An admitted parent phase2
				// remains owned until the missing authenticated child-cancellation seam.
				continue;
			}
			if (!acceptance)
			{
				std::shared_ptr<const critical_command> original_child;
				critical_submit_result failure =
					critical_submit_result::unavailable;
				if (!item_native_quest_preparation_owner::prepare_consumption_command(
					    flow->token, player, mobile, &original_child, &failure))
				{
					if (failure == critical_submit_result::identity_conflict ||
					    failure == critical_submit_result::invalid)
						flow->phase = native_quest_gameplay_phase::blocked;
					continue; // Prepare-only failure attempted no child journal admission.
				}
				if (!persist_child_command(*flow, original_child))
					continue;
				P_obj ignored = nullptr;
				if (!item_native_quest_gameplay_publication_owner::observe_give(
					    flow->player_runtime_id, flow->player_pid,
					    flow->native_runtime_id, 0, &player, &mobile,
					    &ignored) ||
				    !native_quest_gameplay_current(*flow))
					continue;
			}
			const auto submitted =
				acceptance ?
					item_native_quest_preparation_owner::submit_acceptance(
						flow->token, player, mobile) :
					item_native_quest_preparation_owner::submit_consumption(
						flow->token, player, mobile);
			if (critical_submit_result_keeps_operation(submitted))
				flow->phase =
					acceptance ?
						native_quest_gameplay_phase::submitted_acceptance :
						native_quest_gameplay_phase::submitted_consumption;
			else if (submitted == critical_submit_result::identity_conflict ||
				 submitted == critical_submit_result::invalid)
				flow->phase = native_quest_gameplay_phase::blocked;
			// Unavailable/overload/known journal refusal may retry the SAME
			// retained immutable command/token; uncertain admission never retries.
		}
	}
	catch (...)
	{ /* Original tokens and flows remain owned. */
	}
#endif
}

static bool submit_native_quest_give(P_char mobile, P_char player, int quester_id,
				     P_obj offering) noexcept
{
	return quest_native_gameplay_owner::begin(mobile, player, quester_id, offering);
}
static void native_quest_gameplay_pulse() noexcept
{
	quest_native_gameplay_owner::pulse();
}

// Pure CURRENT for the original native quest driver/reward owners. Runtime and
// passive registries share flow pointers during genuine promotion; commands
// also cross the item owner. Allocation-free identity scans preserve that cut.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)

bool nq_world_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
	{
		errno = EOVERFLOW;
		return false;
	}
	bytes += extra;
	return true;
}
bool nq_world_rows(size_t &bytes, size_t count, size_t width) noexcept
{
	if (width && count > SIZE_MAX / width)
	{
		errno = EOVERFLOW;
		return false;
	}
	return nq_world_add(bytes, count * width);
}
bool nq_world_text(size_t &bytes, const std::string &value) noexcept
{
	// GNU13 C++11 ABI basic_string keeps capacities <=15 in its inline buffer.
	if (value.capacity() == SIZE_MAX)
	{
		errno = EOVERFLOW;
		return false;
	}
	return value.capacity() <= 15 || nq_world_add(bytes, value.capacity() + 1);
}
bool nq_world_command(size_t &bytes, const critical_command &value) noexcept
{
	size_t heap = 0;
	return critical_command_current_heap_bytes(value, &heap) && nq_world_add(bytes, heap);
}
bool nq_world_envelope(size_t &bytes, const critical_native_recovery_envelope *value) noexcept
{
	return !value ||
	       (nq_world_add(bytes, sizeof(*value)) && nq_world_command(bytes, value->command) &&
		nq_world_rows(bytes, value->attachment.capacity(), sizeof(uint8_t)));
}
template <class Table> bool nq_world_table(size_t &bytes, const Table &table) noexcept
{
	// Actual default GNU13 unordered_map node type: its hash cache policy is
	// selected by the real key/hash instantiation, not a guessed node overhead.
	using node = std::__detail::_Hash_node<
		typename Table::value_type,
		std::__cache_default<typename Table::key_type, typename Table::hasher>::value>;
	// These original tables use default allocators. GNU13's sole bucket is
	// inline; a real allocated bucket array has at least two prime-policy buckets.
	if (!table.bucket_count())
	{
		errno = EIO;
		return false;
	}
	return nq_world_add(bytes, sizeof(table)) &&
	       (table.bucket_count() <= 1 ||
		nq_world_rows(bytes, table.bucket_count(),
			      sizeof(std::__detail::_Hash_node_base *))) &&
	       nq_world_rows(bytes, table.size(), sizeof(node));
}
#ifndef __NO_MYSQL__
bool nq_world_terms(size_t &bytes, const quest_reward_continuation &terms) noexcept
{
	// All other continuation fields are fixed members of the owning object.
	return nq_world_text(bytes, terms.character_name) &&
	       nq_world_text(bytes, terms.definition_id);
}
#endif

bool nq_world_flow_seen(const native_quest_gameplay_flow *flow,
			const std::shared_ptr<native_quest_gameplay_flow> *current) noexcept
{
	for (const auto &[runtime, pointer] : native_quest_gameplay)
	{
		if (&pointer == current)
			return false;
		if (pointer.get() == flow)
			return true;
	}
#ifndef __NO_MYSQL__
	for (const auto &[key, pointer] : native_quest_passive_gameplay)
	{
		if (&pointer == current)
			return false;
		if (pointer.get() == flow)
			return true;
	}
#endif
	return false;
}
bool nq_world_flow_command_seen(const critical_command *command,
				const std::shared_ptr<const critical_command> *current) noexcept
{
	for (const auto &[runtime, flow] : native_quest_gameplay)
		if (flow)
			for (const auto *pointer :
			     { &flow->acceptance_command, &flow->prepared_child,
			       &flow->completed_child })
			{
				if (pointer == current)
					return false;
				if (pointer->get() == command)
					return true;
			}
#ifndef __NO_MYSQL__
	for (const auto &[key, flow] : native_quest_passive_gameplay)
		if (flow)
			for (const auto *pointer :
			     { &flow->acceptance_command, &flow->prepared_child,
			       &flow->completed_child })
			{
				if (pointer == current)
					return false;
				if (pointer->get() == command)
					return true;
			}
#endif
	return false;
}
bool nq_world_shared_command(size_t &bytes,
			     const std::shared_ptr<const critical_command> &command) noexcept
{
	if (!command)
		return true;
	bool item_owned = false;
	if (!item_native_quest_retained_command_allocation_owned(command.get(), &item_owned))
		return false;
	// Live commands use make_shared<const command>; the original cold decoder
	// also uses make_shared<command>, then converts its shared pointer to const.
	static_assert(
		sizeof(std::_Sp_counted_ptr_inplace<critical_command, std::allocator<void>,
						    __gnu_cxx::_S_atomic>) ==
		sizeof(std::_Sp_counted_ptr_inplace<const critical_command, std::allocator<void>,
						    __gnu_cxx::_S_atomic>));
	return item_owned || nq_world_flow_command_seen(command.get(), &command) ||
	       (nq_world_add(bytes, sizeof(std::_Sp_counted_ptr_inplace<const critical_command,
									std::allocator<void>,
									__gnu_cxx::_S_atomic>)) &&
		nq_world_command(bytes, *command));
}
bool nq_world_flow(size_t &bytes, const native_quest_gameplay_flow &flow) noexcept
{
	if (!nq_world_add(bytes, sizeof(std::_Sp_counted_ptr_inplace<native_quest_gameplay_flow,
								     std::allocator<void>,
								     __gnu_cxx::_S_atomic>)) ||
	    !nq_world_rows(bytes, flow.branches.capacity(), sizeof(flow.branches[0])) ||
	    !nq_world_shared_command(bytes, flow.acceptance_command) ||
	    !nq_world_shared_command(bytes, flow.prepared_child) ||
	    !nq_world_shared_command(bytes, flow.completed_child) ||
	    !nq_world_envelope(bytes, flow.recovery.get()) ||
	    !nq_world_envelope(bytes, flow.pending.get()) ||
	    !nq_world_envelope(bytes, flow.hook_return.get()) ||
	    !nq_world_envelope(bytes, flow.child_context.get()) ||
	    !nq_world_envelope(bytes, flow.restored_original.get()))
		return false;
	for (const auto &branch : flow.branches)
		if (branch &&
		    (!nq_world_add(bytes, sizeof(*branch)) ||
		     !nq_world_rows(bytes, branch->give.capacity(), sizeof(goal_data)) ||
		     !nq_world_rows(bytes, branch->receive.capacity(), sizeof(goal_data)) ||
		     !nq_world_text(bytes, branch->message) ||
		     !nq_world_text(bytes, branch->disappear_message) ||
		     !nq_world_text(bytes, branch->definition_id)))
			return false;
	// branch.view pointers alias its owned strings/goal vectors; P_char/P_obj
	// identities and actual gameplay bodies belong to the genuine whole-native G.
	return true;
}
#ifndef __NO_MYSQL__
bool nq_world_frozen(size_t &bytes, const native_quest_frozen_recovery_state &state) noexcept
{
	return nq_world_add(bytes,
			    sizeof(std::_Sp_counted_ptr_inplace<native_quest_frozen_recovery_state,
								std::allocator<void>,
								__gnu_cxx::_S_atomic>)) &&
	       nq_world_text(bytes, state.key) && nq_world_command(bytes, state.command) &&
	       nq_world_rows(bytes, state.continuation.capacity(), sizeof(uint8_t)) &&
	       nq_world_terms(bytes, state.terms) &&
	       nq_world_envelope(bytes, state.current.get()) &&
	       nq_world_envelope(bytes, state.pending.get()) &&
	       nq_world_envelope(bytes, state.returned.get()) &&
	       nq_world_envelope(bytes, state.restored_original.get()) &&
	       nq_world_envelope(bytes, state.terminal_parent.get());
}
#endif
#endif
}

bool quest_native_retained_storage_bytes(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)output;
	errno = ENOTSUP;
	return false;
#else
	if (!output || !nevent_is_game_thread())
	{
		errno = EINVAL;
		return false;
	}
	if (sizeof(void *) != 8 || sizeof(size_t) != 8)
	{
		errno = ENOTSUP;
		return false;
	}
	if (__gnu_cxx::__default_lock_policy != __gnu_cxx::_S_atomic)
	{
		errno = ENOTSUP;
		return false;
	}
	const int saved_errno = errno;
	errno = 0;
	size_t bytes = 0;
	if (!nq_world_table(bytes, native_quest_gameplay) ||
	    !nq_world_table(bytes, quest_reward_recoveries) ||
	    !nq_world_table(bytes, quest_reward_recovery_items))
		return false;
	for (const auto &[key, attempt] : quest_reward_recoveries)
		if (!nq_world_text(bytes, key) ||
		    !nq_world_rows(bytes, attempt.expected_receipts.capacity(),
				   sizeof(player_quest_xp_receipt_snapshot)))
			return false;
	for (const auto &[uid, key] : quest_reward_recovery_items)
		if (!nq_world_text(bytes, key))
			return false;
	for (const auto &[runtime, flow] : native_quest_gameplay)
	{
		if (!flow)
		{
			errno = EIO;
			return false;
		}
		if (!nq_world_flow_seen(flow.get(), &flow) && !nq_world_flow(bytes, *flow))
		{
			if (!errno)
				errno = EIO;
			return false;
		}
	}
#ifndef __NO_MYSQL__
	if (!nq_world_table(bytes, native_quest_passive_gameplay) ||
	    !nq_world_table(bytes, native_quest_passive_children) ||
	    !nq_world_table(bytes, native_quest_frozen_recoveries))
		return false;
	for (const auto &[key, flow] : native_quest_passive_gameplay)
	{
		if (!flow)
		{
			errno = EIO;
			return false;
		}
		if (!nq_world_text(bytes, key) ||
		    (!nq_world_flow_seen(flow.get(), &flow) && !nq_world_flow(bytes, *flow)))
		{
			if (!errno)
				errno = EIO;
			return false;
		}
	}
	for (const auto &[key, record] : native_quest_passive_children)
	{
		if (!record)
		{
			errno = EIO;
			return false;
		}
		if (!nq_world_text(bytes, key))
			return false;
		bool seen = false;
		for (const auto &[prior_key, prior] : native_quest_passive_children)
		{
			if (&prior == &record)
				break;
			seen = seen || prior.get() == record.get();
		}
		if (!seen &&
		    (!nq_world_add(bytes, sizeof(std::_Sp_counted_ptr_inplace<
						  const critical_native_recovery_envelope,
						  std::allocator<void>, __gnu_cxx::_S_atomic>)) ||
		     !nq_world_command(bytes, record->command) ||
		     !nq_world_rows(bytes, record->attachment.capacity(), sizeof(uint8_t))))
		{
			if (!errno)
				errno = EIO;
			return false;
		}
	}
	for (const auto &[key, state] : native_quest_frozen_recoveries)
	{
		if (!state)
		{
			errno = EIO;
			return false;
		}
		if (!nq_world_text(bytes, key))
			return false;
		bool seen = false;
		for (const auto &[prior_key, prior] : native_quest_frozen_recoveries)
		{
			if (&prior == &state)
				break;
			seen = seen || prior.get() == state.get();
		}
		if (!seen && !nq_world_frozen(bytes, *state))
		{
			if (!errno)
				errno = EIO;
			return false;
		}
	}
#endif
	*output = bytes;
	errno = saved_errno;
	return true;
#endif
}

#include <thread>
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13
#include <bits/allocated_ptr.h>
#endif
namespace
{
constexpr size_t recovery_library_allocator_frames =
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
constexpr size_t recovery_library_copy_frames =
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
constexpr size_t recovery_library_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t recovery_library_default_frames =
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
	2 * (3 * sizeof(void *)) + sizeof(uint8_t);
constexpr size_t recovery_library_vector_frames =
	recovery_library_allocator_frames + recovery_library_copy_frames +
	recovery_library_relocate_frames + recovery_library_default_frames +
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
constexpr size_t recovery_library_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + recovery_library_allocator_frames;

// Nontrivial row/description construction and destruction are source scopes,
// not heap metadata. The nested member objects already live in sizeof(row).
constexpr size_t recovery_library_nontrivial_frames =
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
	3 * recovery_library_allocator_frames;
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t recovery_library_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t recovery_library_string_frames =
	recovery_library_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	recovery_library_allocator_frames +
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
constexpr size_t recovery_library_size_constructor_frames =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<size_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<size_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

// Actual codec linked owner/row-vector observation and relay source scopes.
// These declared carriers complement the real T-specific CURRENT profiles.
constexpr size_t recovery_owner_source_frames =
	// add/rows/capacity + byte/row/payload/intent/context overload arguments,
	// genuine const row range iterators, row reference and current-heap scalar.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 4 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 10 * sizeof(void *) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + 2 * sizeof(size_t) +
	4 * sizeof(bool) +
	// current(this,out), bytes and actual linked cursor; prefix/peak this,
	// outputs, requests/frames/bytes plus callback arguments/results.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 5 * sizeof(void *) +
	7 * sizeof(size_t) + 4 * sizeof(bool) +
	// invoke this/F-reference/failure/frames/local value, actual lambda prefix;
	// watcher ctor this/owner/value and destructor this; observe formal pair.
	2 * sizeof(void *) + sizeof(bool) + 3 * sizeof(size_t) + 6 * sizeof(void *) + sizeof(bool);
}

namespace
{
// Actual installed span/array/initializer_list source constructor/accessors.
// The view object itself is a real caller local,priced separately.
constexpr size_t recovery_library_view_frames =
	// span(pointer,count) this/first/count; to_address/__to_address params+results;
	// dynamic extent constructor and accessor each actual receiver/size value.
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	2 * (sizeof(void *) + sizeof(size_t)) +
	// span default/array/converting constructors,actual reference and extent;
	// begin/end actual normal_iterator return and ctor source references.
	4 * sizeof(void *) + sizeof(size_t) + 2 * (4 * sizeof(void *)) +
	// subspan this/offset/count/returned span; size/empty/data/index receiver/
	// result,where size reaches the real extent accessor.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::span<const uint8_t>) +
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	// array begin/end/data/_S_ptr and actual operator[]/_S_ref argument/results.
	4 * (4 * sizeof(void *)) + 3 * sizeof(void *) + sizeof(size_t) +
	// initializer_list private compiler ctor and member begin/end/size.
	2 * sizeof(void *) + sizeof(size_t) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
}
// Genuine declared-source observer inventory. This is source storage, never
// physical G. Independent review/native correspondence are separate evidence.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && __cplusplus == 202002L &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL)
template <class Table> constexpr size_t quest_current_table_frames() noexcept
{
	using iterator = typename Table::const_iterator;
	using value = typename Table::value_type;
	using node = std::__detail::_Hash_node<
		value,
		std::__cache_default<typename Table::key_type, typename Table::hasher>::value>;
	using base = std::__detail::_Node_iterator_base<
		value,
		std::__cache_default<typename Table::key_type, typename Table::hasher>::value>;
	// nq_world_table(bytes&,table&) + unordered_map bucket_count/size receiver,
	// return and hashtable receiver/return. Real range-for begin/end saved values,
	// begin/end receiver and constructor argument/receiver/base carriers, _M_begin
	// node return/receiver, iterator dereference/_M_v/_M_valptr/aligned-buffer
	// pointers, prefix increment/_M_incr/_M_next, equality operands/bool result,
	// and the actual structured-binding pair reference. All map node/layout types
	// are selected from this actual Table, rather than a bucket/node estimate.
	return 2 * sizeof(void *) + 2 * (2 * sizeof(void *) + 2 * sizeof(size_t)) +
	       2 * sizeof(iterator) +
	       2 * (sizeof(const Table *) + sizeof(iterator) + sizeof(node *) + sizeof(iterator *) +
		    sizeof(base *) + sizeof(node *) + sizeof(const void *)) +
	       sizeof(iterator *) + sizeof(node *) + sizeof(value *) + 2 * sizeof(void *) +
	       sizeof(value *) + sizeof(iterator *) + sizeof(base *) + sizeof(node *) +
	       sizeof(node *) + 2 * sizeof(const base *) + sizeof(bool) + sizeof(const value *);
}
template <class Vector> constexpr size_t quest_current_vector_frames() noexcept
{
	// Actual saved range iterators and element reference; begin/end const
	// receiver/result/normal-iterator constructor, deref/increment/equality,
	// both base() receiver/reference results, capacity receiver/result.
	return 2 * sizeof(typename Vector::const_iterator) + sizeof(typename Vector::value_type *) +
	       2 * (sizeof(const Vector *) + sizeof(typename Vector::const_iterator) +
		    2 * sizeof(void *)) +
	       2 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) +
	       sizeof(const Vector *) + sizeof(size_t);
}
template <class T> constexpr size_t quest_current_shared_pointer_frames() noexcept
{
	// Actual __shared_ptr get/boolean/deref/arrow receivers and their returned
	// pointers/references/boolean. Member object remains physical-owner storage.
	return 5 * sizeof(void *) + sizeof(bool) + 2 * sizeof(T *);
}
template <class T> constexpr size_t quest_current_unique_pointer_frames() noexcept
{
	// unique_ptr::get -> __uniq_ptr_impl::_M_ptr -> tuple get -> __get_helper ->
	// Tuple_impl::_M_head -> Head_base::_M_head: six receiver/ref/result pairs.
	// bool/deref/arrow add three receivers, pointer/ref returns and bool result.
	return 12 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(T *) + sizeof(bool);
}
}
#else
}
#endif
bool quest_native_current_source_frames(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	(void)output;
	errno = ENOTSUP;
	return false;
#else
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8)
	{
		errno = EINVAL;
		return false;
	}
	size_t frames = 0, item_ownership = 0, command_frames = 0;
	if (!item_native_quest_retained_command_allocation_owned_source_frames(&item_ownership) ||
	    !critical_command_current_heap_observer_frame_bytes(&command_frames))
		return false;
	// Real public getter: output/saved errno/bytes, thread get_id/ctor/equality
	// (five id values plus ctor receiver), errno pointer and scalar returns.
	if (!nq_world_add(frames, sizeof(output) + sizeof(int) + sizeof(size_t) +
					  5 * sizeof(std::thread::id) + sizeof(void *) +
					  sizeof(int *) + 3 * sizeof(bool)) ||
	    !nq_world_add(frames, quest_current_table_frames<decltype(native_quest_gameplay)>()) ||
	    !nq_world_add(frames,
			  quest_current_table_frames<decltype(quest_reward_recoveries)>()) ||
	    !nq_world_add(frames,
			  quest_current_table_frames<decltype(quest_reward_recovery_items)>()))
		return false;
#ifndef __NO_MYSQL__
	if (!nq_world_add(frames,
			  quest_current_table_frames<decltype(native_quest_passive_gameplay)>()) ||
	    !nq_world_add(frames,
			  quest_current_table_frames<decltype(native_quest_passive_children)>()) ||
	    !nq_world_add(frames,
			  quest_current_table_frames<decltype(native_quest_frozen_recoveries)>()))
		return false;
#endif
	// flow_seen and flow_command_seen each own their nested runtime/passive
	// table walks while the original outer walk is still live. This is in
	// addition to the six genuine top-level registry scans, not a substitute.
	if (!nq_world_add(frames,
			  2 * quest_current_table_frames<decltype(native_quest_gameplay)>()))
		return false;
#ifndef __NO_MYSQL__
	if (!nq_world_add(
		    frames,
		    2 * quest_current_table_frames<decltype(native_quest_passive_gameplay)>()) ||
	    // Children and frozen states have real nested prior-pointer dedup loops.
	    !nq_world_add(frames,
			  quest_current_table_frames<decltype(native_quest_passive_children)>()) ||
	    !nq_world_add(frames,
			  quest_current_table_frames<decltype(native_quest_frozen_recoveries)>()))
		return false;
#endif
	// The two original command-scan loops each own an actual three-pointer
	// initializer backing array and view, begin/end/element pointers and member
	// begin/end receiver/results. No restored envelope or ownership flag.
	if (!nq_world_add(frames,
			  4 * sizeof(void *) + 2 * sizeof(bool) +
				  2 * (3 * sizeof(const std::shared_ptr<const critical_command> *) +
				       sizeof(std::initializer_list<
					       const std::shared_ptr<const critical_command> *>) +
				       3 * sizeof(void *) + 4 * sizeof(void *))) ||
	    !nq_world_add(frames, item_ownership))
		return false;
	// Genuine declared command CURRENT profile, not its unrelated copy/encode
	// request allowance. The query's P+B source formals are separately owned.
	if (!nq_world_add(frames, command_frames) ||
	    !nq_world_add(frames, sizeof(size_t *) + sizeof(bool)))
		return false;
	// Real nq_world_shared_command's byte/command refs,item-owned and bool;
	// command's byte/value refs,heap scalar and boolean; envelope's two formals;
	// flow/frozen/terms two formals each and boolean. Add/rows have their genuine
	// reference/value/width parameters and return. Text has three actual capacity
	// call sites (SIZE_MAX,SSO,allocated request); GNU13 const capacity reaches
	// data/local-data/pointer_to/addressof with its eleven pointer carriers.
	using branches = decltype(std::declval<native_quest_gameplay_flow>().branches);
	if (!nq_world_add(
		    frames,
		    2 * sizeof(void *) + 2 * sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) +
			    sizeof(bool) + 4 * (2 * sizeof(void *) + sizeof(bool)) +
			    sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(void *) +
			    2 * sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) +
			    3 * (11 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
			    sizeof(int *)) ||
	    !nq_world_add(frames, quest_current_vector_frames<branches>()) ||
	    !nq_world_add(frames,
			  quest_current_shared_pointer_frames<native_quest_gameplay_flow>()) ||
	    !nq_world_add(frames, quest_current_shared_pointer_frames<const critical_command>()) ||
	    !nq_world_add(
		    frames,
		    quest_current_unique_pointer_frames<const native_quest_original_branch>()) ||
	    !nq_world_add(frames,
			  quest_current_unique_pointer_frames<critical_native_recovery_envelope>()))
		return false;
#ifndef __NO_MYSQL__
	// Top-level full child/frozen shared pointers are different genuine template
	// instantiations. The original dedup seen locals are two actual booleans.
	if (!nq_world_add(
		    frames,
		    quest_current_shared_pointer_frames<const critical_native_recovery_envelope>()) ||
	    !nq_world_add(
		    frames,
		    quest_current_shared_pointer_frames<native_quest_frozen_recovery_state>()) ||
	    !nq_world_add(frames, 2 * sizeof(bool)))
		return false;
#endif
	*output = frames;
	return true;
#endif
}
size_t quest_native_current_source_profile_query_frames() noexcept
{
	// Actual profile output/return, three scalars; dependent item query formals
	// and critical pure query P+B; twelve typed table calls (six outer, four flow
	// alias and two dedup), four shared,two unique,one vector profile returns;
	// checked add reference/value/result. Unsupported build branches use less.
	return sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	       item_native_quest_retained_command_allocation_owned_source_profile_query_frames() +
	       sizeof(size_t *) + sizeof(bool) + 19 * sizeof(size_t) + sizeof(void *) +
	       sizeof(size_t) + sizeof(bool);
}

namespace
{
using quest_replay_reserve = bool (*)(size_t, void *) noexcept;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && __cplusplus == 202002L &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && !defined(__NO_MYSQL__)
struct quest_phase2_workspace
{
	player_save_coin_replay_budget_scope_owner &pipeline;
	quest_replay_reserve reserve;
	void *callback_context;
	size_t outer;
	const item_transfer_payload *payload = nullptr;
	const native_quest_recovery_context *context = nullptr;
	const std::string *key = nullptr;
	const native_quest_gameplay_flow *flow = nullptr;
	const native_quest_frozen_recovery_state *frozen = nullptr;
	const critical_native_recovery_envelope *record = nullptr;
	const critical_command *child = nullptr;
	const native_quest_original_branch *branch = nullptr;
	const std::vector<uint8_t> *left = nullptr, *right = nullptr;
	bool current(size_t *output) const noexcept
	{
		size_t bytes = outer, world = 0, pipe = 0, heap = 0;
		if (!output || !quest_native_retained_storage_bytes(&world) ||
		    !player_save_native_recovery_replay_owner::current_storage_bytes(pipeline,
										     &pipe) ||
		    !nq_world_add(bytes, world) || !nq_world_add(bytes, pipe) ||
		    !nq_world_add(bytes, sizeof(*this)))
			return false;
		if (payload &&
		    (!item_transfer_payload_current_heap_bytes(*payload, &heap) ||
		     !nq_world_add(bytes, sizeof(*payload)) || !nq_world_add(bytes, heap)))
			return false;
		if (context &&
		    (!native_quest_recovery_context_current_heap_bytes(*context, &heap) ||
		     !nq_world_add(bytes, sizeof(*context)) || !nq_world_add(bytes, heap)))
			return false;
		if (key && (!nq_world_add(bytes, sizeof(*key)) || !nq_world_text(bytes, *key)))
			return false;
		if (flow && !nq_world_flow_seen(flow, nullptr) && !nq_world_flow(bytes, *flow))
			return false;
		if (frozen)
		{
			bool owned = false;
			for (const auto &[k, p] : native_quest_frozen_recoveries)
				owned = owned || p.get() == frozen;
			if (!owned && !nq_world_frozen(bytes, *frozen))
				return false;
		}
		if (record)
		{
			bool owned = false;
			for (const auto &[k, p] : native_quest_passive_children)
				owned = owned || p.get() == record;
			if (!owned &&
			    (!nq_world_add(bytes,
					   sizeof(std::_Sp_counted_ptr_inplace<
						   const critical_native_recovery_envelope,
						   std::allocator<void>, __gnu_cxx::_S_atomic>)) ||
			     !nq_world_command(bytes, record->command) ||
			     !nq_world_rows(bytes, record->attachment.capacity(), sizeof(uint8_t))))
				return false;
		}
		if (child && (!nq_world_add(bytes, sizeof(std::_Sp_counted_ptr_inplace<
							   critical_command, std::allocator<void>,
							   __gnu_cxx::_S_atomic>)) ||
			      !nq_world_command(bytes, *child)))
			return false;
		if (branch &&
		    (!nq_world_add(bytes, sizeof(*branch)) ||
		     !nq_world_text(bytes, branch->message) ||
		     !nq_world_text(bytes, branch->disappear_message) ||
		     !nq_world_text(bytes, branch->definition_id) ||
		     !nq_world_rows(bytes, branch->give.capacity(), sizeof(goal_data)) ||
		     !nq_world_rows(bytes, branch->receive.capacity(), sizeof(goal_data))))
			return false;
		for (const auto *buffer : { left, right })
			if (buffer && (!nq_world_add(bytes, sizeof(*buffer)) ||
				       !nq_world_rows(bytes, buffer->capacity(), sizeof(uint8_t))))
				return false;
		*output = bytes;
		return true;
	}
	bool peak(size_t request, size_t *prefix = nullptr) noexcept
	{
		size_t bytes = 0, frames = 0;
		if (!quest_native_current_source_frames(&frames) || !current(&bytes) ||
		    !nq_world_add(bytes, frames) ||
		    !nq_world_add(bytes, player_save_native_recovery_replay_owner::
						 current_observer_frame_bytes()) ||
		    !nq_world_add(bytes, quest_native_restore_phase2_source_frame_bytes()) ||
		    !nq_world_add(bytes, request) || !reserve || !reserve(bytes, callback_context))
		{
			errno = ENOBUFS;
			return false;
		}
		if (prefix)
			*prefix = bytes;
		return true;
	}
	template <class F, class R> R invoke_native_codec(F &&body, R failure) noexcept
	{
		if (!peak(native_quest_recovery_codec_source_profile_query_frames()))
			return failure;
		const size_t source = native_quest_recovery_codec_source_frame_bytes();
		size_t request = native_quest_recovery_codec_entry_inline_bytes();
		if (!nq_world_add(request, source) || !peak(request))
			return failure;
		size_t bytes = 0;
		return peak(0, &bytes) ? body(bytes) : failure;
	}
	template <class F, class R> R invoke(F &&body, R failure) noexcept
	{
		size_t bytes = 0;
		return peak(0, &bytes) ? body(bytes) : failure;
	}
	bool command_copy(const critical_command &source, size_t fixed = 0) noexcept
	{
		size_t request = 0;
		return critical_command_fresh_copy_request_bytes(source, &request) &&
		       nq_world_add(request, fixed) &&
		       nq_world_add(request, critical_command_copy_frame_bytes()) && peak(request);
	}
	bool envelope_copy(const critical_native_recovery_envelope &source,
			   size_t fixed = sizeof(critical_native_recovery_envelope)) noexcept
	{
		size_t request = 0;
		return critical_command_fresh_copy_request_bytes(source.command, &request) &&
		       nq_world_add(request, fixed) &&
		       nq_world_add(request, source.attachment.size()) &&
		       nq_world_add(request, critical_command_copy_frame_bytes()) && peak(request);
	}
	bool string_assign(const std::string &target, const std::string &source) noexcept
	{
		size_t request = 0;
		if (source.size() > target.capacity())
		{
			request = source.size();
			if (request < 2 * target.capacity())
				request = 2 * target.capacity();
			if (request == SIZE_MAX)
				return false;
			++request;
		}
		return peak(request);
	}
	template <class Table> bool map_reserve(Table &table, size_t count)
	{
		// Admit the actual owning source before the policy's out-of-line query.
		// Its authentic installed-body qualification remains an explicit HOLD.
		if (!peak(0))
			return false;
		std::__detail::_Prime_rehash_policy policy(table.max_load_factor());
		const size_t buckets = policy._M_next_bkt(policy._M_bkt_for_elements(count));
		if (buckets > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		    !peak(buckets * sizeof(std::__detail::_Hash_node_base *)))
			return false;
		table.reserve(count);
		return true;
	}
	template <class Table> bool map_node(const Table &, const std::string &name) noexcept
	{
		using node = std::__detail::_Hash_node<
			typename Table::value_type,
			std::__cache_default<typename Table::key_type,
					     typename Table::hasher>::value>;
		size_t request = sizeof(node);
		return (name.size() <= 15 || nq_world_add(request, name.size() + 1)) &&
		       peak(request);
	}
};
bool quest_phase2_equal_bounded(const critical_native_recovery_envelope &a,
				const critical_native_recovery_envelope &b,
				quest_phase2_workspace &budget) noexcept
{
	if (a.phase != b.phase || a.revision != b.revision || a.attachment != b.attachment)
		return false;
	std::vector<uint8_t> left, right;
	budget.left = &left;
	budget.right = &right;
	const bool equal = budget.invoke(
				   [&](size_t prefix) noexcept
				   {
					   return critical_command_encode_bounded(
						   a.command, &left, budget.reserve,
						   budget.callback_context, prefix);
				   },
				   critical_command_codec_result::overflow) ==
				   critical_command_codec_result::ok &&
			   budget.invoke(
				   [&](size_t prefix) noexcept
				   {
					   return critical_command_encode_bounded(
						   b.command, &right, budget.reserve,
						   budget.callback_context, prefix);
				   },
				   critical_command_codec_result::overflow) ==
				   critical_command_codec_result::ok &&
			   left == right;
	budget.left = budget.right = nullptr;
	return equal;
}
#endif
}

// Complete declared-source inventory for the original phase2 algorithm.
// Sum the real supported instantiations, not physical G, heap requests or a
// reserve margin. Native/emitted correspondence is qualified separately.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && __cplusplus == 202002L &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && !defined(__NO_MYSQL__)
constexpr size_t quest_restore_hash_frames =
	// hash<string>::operator()(this,string&), _Hash_impl::hash(ptr,len,seed),
	// actual _Hash_bytes(ptr,len,seed): buf/end and loop p; len_aligned/hash/
	// loop data and tail data. Count both lexical data declarations. Static
	// mul is not a per-call object. GCC13.3 hash_bytes.cc128ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã…â€œ153,64-bit branch.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + 2 * sizeof(size_t) + sizeof(size_t) +
	sizeof(void *) + 2 * sizeof(size_t) + 4 * sizeof(const char *) + 4 * sizeof(size_t) +
	sizeof(size_t) +
	// unaligned_load pointer+result; actual memcpy dst/src/length/return;
	// load_bytes pointer/int/result and its n/ptr loop; shift_mix value/result.
	sizeof(void *) + sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
	sizeof(int) + sizeof(size_t) + sizeof(int) + sizeof(void *) + 2 * sizeof(size_t) +
	// basic_string data()/length()/size() receivers, pointers and results.
	3 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *);
constexpr size_t quest_restore_prime_frames =
	// _M_next_bkt this/n/result; n_primes,last_prime,next_bkt lexical locals.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(size_t) + 2 * sizeof(const unsigned long *) +
	// lower_bound first/last/value reference/result, __iter_less_val adapter,
	// __lower_bound first/last/value/comp/len/half/middle/result. Actual unsigned
	// long pointer iterators: no recursion, allocation or iterator proxy state.
	4 * sizeof(void *) + sizeof(__gnu_cxx::__ops::_Iter_less_val) + 5 * sizeof(void *) +
	2 * sizeof(std::ptrdiff_t) + sizeof(__gnu_cxx::__ops::_Iter_less_val) +
	// distance/__distance and advance/__advance/category constructors.
	4 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + 4 * sizeof(void *) +
	3 * sizeof(std::ptrdiff_t) + 3 * sizeof(std::random_access_iterator_tag) +
	// _Iter_less_val::operator() this/iterator/value-reference/result;
	// both max<size_t> operand refs/result, each two argument temporaries.
	3 * sizeof(void *) + sizeof(bool) + 2 * (3 * sizeof(void *) + 2 * sizeof(size_t)) +
	// _M_need_rehash this/three n parameters/min_bkts/returned pair; both pair
	// brace construction instantiations: this/two forwarded scalar refs and
	// forward receivers/results. floor conversions input/output double.
	sizeof(void *) + 3 * sizeof(size_t) + sizeof(double) + sizeof(std::pair<bool, size_t>) +
	2 * (3 * sizeof(void *) + sizeof(bool) + sizeof(size_t) + 4 * sizeof(void *)) +
	4 * sizeof(double) +
	// _M_bkt_for_elements this/n/result, max_load_factor this/float result,
	// _M_state/_M_reset receiver/state-ref and returned state reference.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(void *) + sizeof(float) + 5 * sizeof(void *);
constexpr size_t quest_restore_equal_frames =
	// Genuine vector<byte> ==/!= formals, size queries, begin/end normal iterator
	// return/constructor scopes; equal/__equal_aux/__equal_aux1/__equal<true>
	// first/last/second/returned bool and its two real compile-time bool locals.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	3 * (4 * sizeof(void *)) + 4 * (3 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(bool) +
	sizeof(std::ptrdiff_t) +
	// __memcmp and actual builtin memcmp pointer/length/int results; runtime
	// is_constant_evaluated result. Array<16> == uses the same byte comparison,
	// plus two real array references and data/_S_ptr source receiver/returns.
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int)) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + 8 * sizeof(void *) +
	// equal_to<string>, operator==(string,string), size queries and traits
	// compare/_S_compare: arguments/results/counts and real int comparison.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	2 * sizeof(size_t) + sizeof(int) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
constexpr size_t quest_restore_all_of_frames =
	// Original array<9> begin/end/data/_S_ptr calls, all_of -> find_if_not ->
	// __find_if(random_access), Iter_negate/Iter_pred constructors/operators.
	// Original predicate is a GNU13 empty noncapturing closure (one byte);
	// its actual receiver,uint8_t step and bool return are explicitly priced.
	8 * sizeof(void *) + 3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	2 * sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(std::random_access_iterator_tag) +
	3 * sizeof(char) + 8 * sizeof(void *) + 4 * sizeof(bool) + sizeof(uint8_t);

template <class Table> constexpr size_t quest_restore_map_frames() noexcept
{
	using value = typename Table::value_type;
	using iterator = typename Table::iterator;
	using const_iterator = typename Table::const_iterator;
	using node = std::__detail::_Hash_node<
		value,
		std::__cache_default<typename Table::key_type, typename Table::hasher>::value>;
	using base = std::__detail::_Hash_node_base;
	using node_allocator = std::allocator<node>;
	// Genuine unordered_map/Hashtable find receiver/key/result, small-table
	// range iterator, hash code/bucket/node, find-before-node predecessor/p/node
	// and next-bucket temporaries. find_node adds its predecessor and return.
	const size_t find = 4 * sizeof(void *) + 2 * sizeof(iterator) + sizeof(iterator) +
			    2 * sizeof(size_t) + 2 * sizeof(node *) + 4 * sizeof(void *) +
			    sizeof(size_t) + 3 * sizeof(base *) + 2 * sizeof(node *) +
			    sizeof(bool) + 4 * sizeof(void *) + sizeof(size_t) + sizeof(base *) +
			    sizeof(node *);
	// Actual _M_emplace(true): this/key-ref/node guard (two real fields, h/node),
	// iterator loop/code/bkt/pos/returned pair. The private _Scoped_node fields
	// are a hashtable-allocation receiver pointer and node*, both pointer-aligned
	// in this authenticated64-bit layout. No invented public guard substitute.
	const size_t emplace =
		3 * sizeof(void *) + sizeof(void *) + sizeof(node *) + 2 * sizeof(iterator) +
		2 * sizeof(size_t) + sizeof(iterator) + sizeof(std::pair<iterator, bool>) +
		// guard constructor/destructor: this/h/two forwarded argument references;
		// node allocator receiver+returned node/value address and live allocator.
		6 * sizeof(void *) + sizeof(node_allocator) + sizeof(node *) + sizeof(value *) +
		// allocation/placement/value pair ctor(key-ref,mapped-ref), forward source
		// scopes and pair result. Actual key string ctor source is added below.
		recovery_library_allocator_frames + 6 * sizeof(void *) + 4 * sizeof(void *) +
		2 * sizeof(value *) + 4 * sizeof(void *);
	// _M_insert_unique_node receiver/bkt/code/node/n_elt/state-ref/rehash-pair/
	// returned iterator; _M_insert_bucket_begin receiver/bkt/node/before pointer;
	// stored-code/hash/bucket/index/equality/extract-key/node value descendants.
	const size_t insert =
		3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::pair<bool, size_t>) +
		sizeof(iterator) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(base *) +
		10 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(bool) +
		sizeof(typename Table::key_equal) + sizeof(std::__detail::_Select1st) +
		sizeof(std::__detail::_Mod_range_hashing);
	// reserve wrappers this/n; Hashtable rehash saved-state ref,bkt; _M_rehash
	// this/bkt/state-ref; unique _M_rehash_aux this/bkt/true-tag/new buckets/p/
	// bbegin/next/bkt, allocate/deallocate bucket allocator and pointer scopes.
	const size_t rehash = 3 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
			      2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
			      3 * sizeof(size_t) + sizeof(std::true_type) +
			      2 * sizeof(std::allocator<base *>) + 8 * sizeof(void *) +
			      3 * sizeof(size_t) + recovery_library_allocator_frames;
	// erase(iterator) wrappers and _M_erase: const iterator/node/bkt/prev/next,
	// remove bucket begin receiver/bkt/next/next_bkt; returned iterator, actual
	// _M_deallocate_node and _M_deallocate_node_ptr allocators/p/value pointers.
	const size_t erase =
		4 * sizeof(void *) + 2 * sizeof(const_iterator) + 2 * sizeof(iterator) +
		3 * sizeof(node *) + 2 * sizeof(base *) + 3 * sizeof(size_t) + 3 * sizeof(void *) +
		sizeof(size_t) + sizeof(node *) + sizeof(size_t) + 6 * sizeof(void *) +
		sizeof(node_allocator) + sizeof(value *) + recovery_library_allocator_frames;
	// Actual node value/iterator begin/end/deref/increment/equality closure is
	// the paired typed CURRENT inventory. Reserve helper's real policy object
	// and count/buckets/request/this/table source locals are added separately.
	return find + emplace + insert + rehash + erase + quest_current_table_frames<Table>() +
	       quest_restore_prime_frames + quest_restore_hash_frames + quest_restore_equal_frames +
	       recovery_library_string_frames;
}

template <class T> constexpr size_t quest_restore_shared_frames() noexcept
{
	using control = std::_Sp_counted_ptr_inplace<T, std::allocator<void>, __gnu_cxx::_S_atomic>;
	using allocator = typename control::__allocator_type;
	using guard = std::__allocated_ptr<allocator>;
	// make_shared actual default allocator + _Sp_alloc_shared_tag and returned
	// shared_ptr; shared_ptr/base constructors own this/tag; __shared_count owns
	// this/p-ref/tag, rebind allocator, actual allocated guard, mem and pi.
	// Element object and control live in the separately admitted heap request.
	return sizeof(std::allocator<void>) +
	       sizeof(std::_Sp_alloc_shared_tag<std::allocator<void>>) +
	       sizeof(std::shared_ptr<T>) + 4 * sizeof(void *) +
	       sizeof(std::_Sp_alloc_shared_tag<std::allocator<void>>) + 2 * sizeof(void *) +
	       sizeof(std::_Sp_alloc_shared_tag<std::allocator<void>>) + sizeof(allocator) +
	       sizeof(guard) + 2 * sizeof(control *) +
	       // __allocate_guarded/guard constructor/get/null assignment/dtor plus
	       // addressof/to_address/allocator allocate+deallocate source closures.
	       10 * sizeof(void *) + sizeof(std::nullptr_t) + recovery_library_allocator_frames +
	       // _Sp_counted_ptr_inplace constructor/default Impl/_Sp_ebo_helper/_S_get/
	       // _M_ptr/aligned-buffer, actual allocator passed by value at each boundary.
	       9 * sizeof(void *) + 3 * sizeof(std::allocator<void>) +
	       // shared copy/move/conversion/get/bool/operator->, count copy/add-ref and
	       // destruction/release/dispose/destroy. Control _M_destroy owns actual rebind
	       // allocator+guard. _M_release owns this/both_counts/unique_ref; _M_release_last_use
	       // owns this and count arithmetic. Atomic dispatch uses actual pointer,int,
	       // single-thread result, builtin result or single-path local result. No heap
	       // of a new mutex, no invocation of game callbacks is implied by the profile.
	       16 * sizeof(void *) + 5 * sizeof(bool) + 5 * sizeof(void *) + 2 * sizeof(long long) +
	       3 * sizeof(bool) + 2 * sizeof(int) + sizeof(allocator) + sizeof(guard) +
	       5 * sizeof(void *) + 4 * (sizeof(void *) + sizeof(int)) + 2 * sizeof(int) +
	       2 * sizeof(bool) +
	       // alloc_traits destroy -> destroy_at -> actual T destructor receiver and
	       // forward/placement construct_at source scopes, not T's owned inline data.
	       8 * sizeof(void *) + sizeof(size_t);
}

template <class T> constexpr size_t quest_restore_unique_frames() noexcept
{
	// Actual unique_ptr<T/default_delete<T>>, its __uniq_ptr_data/impl and tuple
	// pointer+deleter constructor/get/release/reset/move/convert/destructor
	// scopes. Each receiver, forwarded pointer/reference, returned pointer,
	// tuple get/head, default-delete and sized delete carrier is source storage.
	return sizeof(std::unique_ptr<T>) + sizeof(std::default_delete<T>) + 4 * sizeof(void *) +
	       sizeof(T *) + 4 * sizeof(void *) + 4 * (3 * sizeof(void *) + sizeof(T *)) +
	       2 * sizeof(bool) + 4 * (2 * sizeof(void *) + sizeof(T *)) + 4 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::nullptr_t);
}
// Actual default objects: frozen state one string/one continuation byte vector,
// terms two strings; flow one branch vector; original branch three strings and
// two goal vectors. Critical commands/envelopes are covered by their genuine
// copy profile and byte-vector inventory below. Their owned inline members
// already belong to the actual heap/object request, not this source profile.
constexpr size_t quest_restore_default_frames =
	3 * sizeof(void *) +
	6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	8 * (5 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	// Matching real string/byte/goal/unique-element-vector member destructors.
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	8 * recovery_library_allocator_frames +
	// Branch goal fill: generic lambda captures budget, from/to refs, source
	// iterators/goal ref, actual goal_data aggregate temporary and i; reserve
	// before the complete fill makes every push_back nonallocating. Its actual
	// trivial construct/forward/construct_at source scopes still occur.
	3 * sizeof(void *) + 2 * sizeof(std::vector<native_quest_recovery_goal>::const_iterator) +
	sizeof(const native_quest_recovery_goal *) + sizeof(goal_data) + sizeof(size_t) +
	8 * sizeof(void *) + sizeof(goal_data *) + sizeof(size_t) +
	// Branch vector has already reserved the complete count. Each move-push
	// owns the genuine converted const unique pointer and relocation/construct
	// references; no prospective reallocation is assumed from this cut.
	sizeof(std::unique_ptr<const native_quest_original_branch>) + 7 * sizeof(void *) +
	2 * sizeof(std::unique_ptr<const native_quest_original_branch>);

size_t quest_restore_complete_frames() noexcept
{
	using runtime = decltype(native_quest_gameplay);
	using children = decltype(native_quest_passive_children);
	using flows = decltype(native_quest_passive_gameplay);
	using frozen = decltype(native_quest_frozen_recoveries);
	// restore_phase2_bounded real formals; three duplicate iterators, runtime
	// loop begin/end/pair-ref; key/shared state/record/flow/child/branch locals;
	// installed result pairs, bytes/retained and charge lambda source captures.
	const size_t method =
		4 * sizeof(void *) + sizeof(size_t) + sizeof(typename flows::iterator) +
		sizeof(typename children::iterator) + sizeof(typename frozen::iterator) +
		2 * sizeof(typename runtime::iterator) + sizeof(typename runtime::value_type *) +
		sizeof(std::string) + sizeof(std::shared_ptr<native_quest_frozen_recovery_state>) +
		sizeof(std::shared_ptr<const critical_native_recovery_envelope>) +
		sizeof(std::shared_ptr<native_quest_gameplay_flow>) +
		sizeof(std::shared_ptr<critical_command>) +
		sizeof(std::unique_ptr<native_quest_original_branch>) +
		sizeof(std::pair<typename flows::iterator, bool>) +
		sizeof(std::pair<typename children::iterator, bool>) +
		sizeof(std::pair<typename frozen::iterator, bool>) + 5 * sizeof(size_t) +
		sizeof(void *) + sizeof(size_t) + sizeof(bool);
	// Genuine phase2 workspace current/peak/invoke (bool and two codec result
	// instantiations)/command-copy/envelope-copy/string-assign/map-reserve/node
	// source scopes. Actual workspace/payload/context private objects are counted
	// by physical current, so do not charge their complete inline size here.
	const size_t owner =
		// Native-codec preflight F/receiver/reference and failure, source/request/bytes,
		// two pure getter return carriers and the checked-add/admission result.
		4 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool) +
		sizeof(player_snapshot_codec_result) + 6 * sizeof(void *) + 6 * sizeof(size_t) +
		3 * sizeof(bool) + 6 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
		3 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
		sizeof(critical_command_codec_result) + sizeof(player_snapshot_codec_result) +
		8 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(bool) +
		sizeof(std::__detail::_Prime_rehash_policy) + 4 * sizeof(void *) +
		3 * sizeof(size_t) + sizeof(float) +
		// Actual fresh-copy request helper: command/output,total, four vector-size
		// calls and checked-add descendant; copy-frame getter result carrier.
		7 * sizeof(void *) + 7 * sizeof(size_t) + 2 * sizeof(bool) +
		// Private unregistered frozen/record scans include their genuine own map
		// iterators/structured bindings. Flow seen and world-owned helper scopes are
		// also traversed by the paired CURRENT profile, admitted separately by peak.
		2 * sizeof(typename frozen::const_iterator) +
		sizeof(typename frozen::value_type *) +
		2 * sizeof(typename children::const_iterator) +
		sizeof(typename children::value_type *) +
		// equal helper a/b/budget refs, actual two vector locals and bool result;
		// its already admitted command codec descendants own their separate source.
		3 * sizeof(void *) + 2 * sizeof(std::vector<uint8_t>) + sizeof(bool) +
		// key helper operation_id receiver and string(begin,end) constructor source.
		sizeof(void *) + 4 * sizeof(void *) + sizeof(std::string) + sizeof(size_t);
	// Original retained scalar helpers frozen_bytes/gameplay_recovery_bytes have
	// real five-pointer arrays, array range iterators and captured add lambdas;
	// passive_total owns all four table walks and passive_bytes's fixed scalar.
	const size_t retention =
		2 * (3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
		     sizeof(std::array<const critical_native_recovery_envelope *, 5>) +
		     3 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
		2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(void *) + sizeof(bool) +
		quest_current_table_frames<runtime>() + quest_current_table_frames<children>() +
		quest_current_table_frames<flows>() + quest_current_table_frames<frozen>();
	// Paired bridge formals/return; own profile query occurs again in peak,
	// while the complete source allowance also covers the real CURRENT-profile
	// query before its physical census. No physical G value is inferred here.
	size_t capacity = 0;
	if (!item_native_quest_restore_budget_source_frames(&capacity))
		return SIZE_MAX;
	const size_t pipeline = player_save_coin_replay_budget_scope_owner::observer_frame_bytes();
	if (!pipeline)
	{
		errno = ENOTSUP;
		return SIZE_MAX;
	}
	size_t total = method + owner + retention + 4 * sizeof(void *) + sizeof(size_t) +
		       sizeof(bool) + quest_native_restore_phase2_source_profile_query_frames() +
		       quest_native_current_source_profile_query_frames() +
		       quest_restore_map_frames<children>() + quest_restore_map_frames<flows>() +
		       quest_restore_map_frames<frozen>() +
		       quest_restore_shared_frames<native_quest_frozen_recovery_state>() +
		       quest_restore_shared_frames<native_quest_gameplay_flow>() +
		       quest_restore_shared_frames<const critical_native_recovery_envelope>() +
		       quest_restore_shared_frames<const critical_command>() +
		       quest_restore_shared_frames<critical_command>() +
		       quest_restore_unique_frames<native_quest_original_branch>() +
		       quest_restore_unique_frames<const native_quest_original_branch>() +
		       quest_restore_unique_frames<critical_native_recovery_envelope>() +
		       quest_restore_default_frames + recovery_library_vector_frames +
		       recovery_library_copy_frames + recovery_library_size_constructor_frames +
		       recovery_library_move_frames + recovery_library_string_frames +
		       quest_restore_equal_frames + quest_restore_all_of_frames;
	// Every original restore_budget branch enters the exact original capacity
	// graph. Own its genuine query and returned source before selecting it.
	// SAME pipeline census reaches bootstrap plus the complete held observer;
	// wrapper-only allowance is insufficient. These are source, never heap G.
	if (!nq_world_add(total, capacity) ||
	    !nq_world_add(total, item_native_quest_restore_budget_source_profile_query_frames()) ||
	    !nq_world_add(total, pipeline) ||
	    !nq_world_add(total, player_save_coin_replay_budget_scope_owner::
					 observer_source_profile_query_frames()) ||
	    !nq_world_add(total, 2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)))
		return SIZE_MAX;
	return total;
}
#endif
}
size_t quest_native_restore_phase2_source_frame_bytes() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && __cplusplus == 202002L &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && !defined(__NO_MYSQL__)
	return quest_restore_complete_frames();
#else
	return sizeof(void *) + sizeof(size_t);
#endif
}
size_t quest_native_restore_phase2_source_profile_query_frames() noexcept
{
	// Getter+complete helper returns and three const subtotals; three map calls
	// each five const subtotals+return; five shared, three unique and seven table; CURRENT-query and its alias-query
	// return carriers. This prices only querying the source profile.
	return 2 * sizeof(size_t) + 3 * sizeof(size_t) + 3 * 6 * sizeof(size_t) +
	       5 * sizeof(size_t) + 3 * sizeof(size_t) + 7 * sizeof(size_t) + 2 * sizeof(size_t) +
	       // New actual capacity/pipeline/total locals; checked-add reference/value,
	       // result and overflow errno pointer; own fixed-query return. The two named
	       // lower query laws own their accessors and genuine nested pure getters.
	       3 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	       2 * sizeof(size_t) + item_native_quest_restore_budget_source_profile_query_frames() +
	       player_save_coin_replay_budget_scope_owner::observer_source_profile_query_frames();
}

size_t quest_native_restore_phase2_entry_inline_bytes() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && __cplusplus == 202002L &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && !defined(__NO_MYSQL__)
	// Real empty workspace, payload and context before the first original decode.
	// Only transient prospective preflight; the actual workspace CURRENT owns
	// these three real objects after entry, with no inflated outgoing prefix.
	return sizeof(quest_phase2_workspace) + sizeof(item_transfer_payload) +
	       sizeof(native_quest_recovery_context);
#else
	return 0;
#endif
}

bool quest_native_gameplay_owner::restore_phase2_bounded(
	const critical_native_recovery_envelope &envelope,
	player_save_coin_replay_budget_scope_owner &actual_scope, quest_replay_reserve reserve,
	void *callback_context, size_t outer) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	(void)actual_scope;
	(void)reserve;
	(void)callback_context;
	(void)outer;
	return false;
#elif !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||           \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	(void)envelope;
	(void)actual_scope;
	(void)reserve;
	(void)callback_context;
	(void)outer;
	errno = ENOTSUP;
	return false;
#else
	try
	{
		quest_phase2_workspace budget{ actual_scope, reserve, callback_context, outer };
		item_transfer_payload payload{};
		budget.payload = &payload;
		native_quest_recovery_context context;
		budget.context = &context;
		if (envelope.phase != critical_native_recovery_phase::continuation_pending ||
		    !envelope.revision ||
		    !budget.invoke(
			    [&](size_t prefix) noexcept
			    {
				    return item_transfer_command_decode_payload_bounded(
					    envelope.command, &payload, reserve, callback_context,
					    prefix);
			    },
			    false) ||
		    !budget.invoke(
			    [&](size_t prefix) noexcept
			    {
				    return item_transfer_native_mobile_recovery_shape_valid_bounded(
					    payload, reserve, callback_context, prefix);
			    },
			    false) ||
		    budget.invoke_native_codec(
			    [&](size_t prefix) noexcept
			    {
				    return native_quest_recovery_context_decode_bounded(
					    envelope.command, envelope.attachment, &context,
					    reserve, callback_context, prefix);
			    },
			    player_snapshot_codec_result::allocation_failure) !=
			    player_snapshot_codec_result::ok ||
		    !context.receipt.present ||
		    context.publication_stage !=
			    native_quest_recovery_publication_stage::physically_proven)
			return false;
		if (!budget.peak(envelope.command.operation_id.bytes.size() + 1))
			return false;
		const std::string key = quest_reward_operation_key(envelope.command.operation_id);
		budget.key = &key;
		for (const auto &[runtime, flow] : native_quest_gameplay)
			if (flow->acceptance_command &&
			    flow->acceptance_command->operation_id.bytes ==
				    envelope.command.operation_id.bytes)
				return flow->restored_original &&
				       quest_phase2_equal_bounded(*flow->restored_original,
								  envelope, budget);
		auto existing = native_quest_passive_gameplay.find(key);
		if (existing != native_quest_passive_gameplay.end())
			return existing->second->restored_original &&
			       quest_phase2_equal_bounded(*existing->second->restored_original,
							  envelope, budget);
		auto child_existing = native_quest_passive_children.find(key);
		if (child_existing != native_quest_passive_children.end())
			return quest_phase2_equal_bounded(*child_existing->second, envelope,
							  budget);
		auto reward_existing = native_quest_frozen_recoveries.find(key);
		if (reward_existing != native_quest_frozen_recoveries.end())
			return reward_existing->second->restored_original &&
			       quest_phase2_equal_bounded(
				       *reward_existing->second->restored_original, envelope,
				       budget);
		if (native_quest_passive_gameplay.size() + native_quest_passive_children.size() +
			    native_quest_frozen_recoveries.size() + native_quest_gameplay.size() >=
		    ITEM_MOVEMENT_PENDING_MAX)
			return false;
		if (payload.native_mobile.action == item_native_mobile_action::consumption)
		{
			if (payload.continuation.kind ==
				    item_transfer_continuation_kind::quest_offering &&
			    (context.receipt.outcome == critical_apply_outcome::applied ||
			     context.receipt.outcome == critical_apply_outcome::already_applied))
			{
				if (!budget.peak(
					    sizeof(std::_Sp_counted_ptr_inplace<
						    native_quest_frozen_recovery_state,
						    std::allocator<void>, __gnu_cxx::_S_atomic>)))
					return false;
				auto state = std::make_shared<native_quest_frozen_recovery_state>();
				budget.frozen = state.get();
				if (!budget.string_assign(state->key, key))
					return false;
				state->key = key;
				if (!budget.command_copy(envelope.command))
					return false;
				state->command = envelope.command;
				if (!budget.peak(payload.continuation.data.size()))
					return false;
				state->continuation = payload.continuation.data;
				if (!budget.invoke(
					    [&](size_t prefix) noexcept
					    {
						    return quest_reward_continuation_decode_bounded(
							    state->continuation.data(),
							    state->continuation.size(),
							    &state->terms, reserve,
							    callback_context, prefix);
					    },
					    false) ||
				    (state->terms.version != 5 &&
				     !(state->terms.version == 6 && payload.native_cost.fee_only)) ||
				    state->terms.player_pid != payload.native_recovery.player_pid ||
				    payload.native_recovery.publication_terms.disappear)
					return false;
				if (!budget.envelope_copy(envelope))
					return false;
				state->current =
					std::make_unique<critical_native_recovery_envelope>(
						envelope);
				if (!budget.envelope_copy(envelope))
					return false;
				state->restored_original =
					std::make_unique<critical_native_recovery_envelope>(
						envelope);
				state->started = context.publication_steps[5] != 0;
				state->effect_returned = state->durable_returned =
					context.publication_steps[5] == 2;
				state->restored = true;
				state->retained_bytes = native_quest_frozen_bytes(*state);
				if (!budget.map_reserve(native_quest_frozen_recoveries,
							native_quest_frozen_recoveries.size() + 1))
					return false;
				if (!budget.map_node(native_quest_frozen_recoveries, key))
					return false;
				auto installed = native_quest_frozen_recoveries.emplace(key, state);
				size_t bytes = 0;
				if (!installed.second || !native_quest_passive_total(&bytes) ||
				    !budget.invoke(
					    [&](size_t prefix) noexcept
					    {
						    return item_native_quest_gameplay_publication_owner::
							    restore_budget_bounded(bytes, reserve,
										   callback_context,
										   prefix);
					    },
					    false))
				{
					if (installed.second)
						native_quest_frozen_recoveries.erase(
							installed.first);
					return false;
				}
				return true;
			}
			if (!budget.envelope_copy(
				    envelope, sizeof(std::_Sp_counted_ptr_inplace<
						      const critical_native_recovery_envelope,
						      std::allocator<void>, __gnu_cxx::_S_atomic>)))
				return false;
			auto record =
				std::make_shared<const critical_native_recovery_envelope>(envelope);
			budget.record = record.get();
			if (!budget.map_reserve(native_quest_passive_children,
						native_quest_passive_children.size() + 1))
				return false;
			if (!budget.map_node(native_quest_passive_children, key))
				return false;
			auto installed =
				native_quest_passive_children.emplace(key, std::move(record));
			size_t bytes = 0;
			if (!installed.second || !native_quest_passive_total(&bytes) ||
			    !budget.invoke(
				    [&](size_t prefix) noexcept
				    {
					    return item_native_quest_gameplay_publication_owner::
						    restore_budget_bounded(bytes, reserve,
									   callback_context,
									   prefix);
				    },
				    false))
			{
				if (installed.second)
					native_quest_passive_children.erase(installed.first);
				return false;
			}
			return true;
		}
		if (!budget.peak(sizeof(std::_Sp_counted_ptr_inplace<native_quest_gameplay_flow,
								     std::allocator<void>,
								     __gnu_cxx::_S_atomic>)))
			return false;
		auto flow = std::make_shared<native_quest_gameplay_flow>();
		budget.flow = flow.get();
		if (!budget.command_copy(envelope.command,
					 sizeof(std::_Sp_counted_ptr_inplace<const critical_command,
									     std::allocator<void>,
									     __gnu_cxx::_S_atomic>)))
			return false;
		flow->acceptance_command =
			std::make_shared<const critical_command>(envelope.command);
		if (!budget.envelope_copy(envelope))
			return false;
		flow->recovery = std::make_unique<critical_native_recovery_envelope>(envelope);
		if (!budget.envelope_copy(envelope))
			return false;
		flow->restored_original =
			std::make_unique<critical_native_recovery_envelope>(envelope);
		flow->player_pid = payload.native_recovery.player_pid;
		flow->original_root_uid = item_transfer_result_root(payload);
		flow->give_hooks = context.give_hooks;
		flow->branch_program_frozen = flow->program_durable = context.branch_program_frozen;
		flow->original_program_lost = !context.branch_program_frozen &&
					      std::all_of(context.give_hooks.begin(),
							  context.give_hooks.end(),
							  [](uint8_t step) { return step == 2; });
		flow->completion_index = static_cast<int>(context.next_branch);
		flow->acceptance_handoff_durable = context.parent_acceptance.bytes ==
						   envelope.command.operation_id.bytes;
		flow->acceptance_rejected = context.receipt.outcome ==
					    critical_apply_outcome::terminal_failure;
		flow->child_operation = context.next_child_operation;
		flow->child_allocated = context.child_handoff_stage != 0;
		flow->child_handoff_durable = context.child_handoff_stage == 2;
		flow->restored = true;
		if (!context.next_child_command.empty())
		{
			if (!budget.peak(
				    sizeof(std::_Sp_counted_ptr_inplace<critical_command,
									std::allocator<void>,
									__gnu_cxx::_S_atomic>)))
				return false;
			auto child = std::make_shared<critical_command>();
			budget.child = child.get();
			if (budget.invoke(
				    [&](size_t prefix) noexcept
				    {
					    return critical_command_decode_bounded(
						    context.next_child_command.data(),
						    context.next_child_command.size(), child.get(),
						    reserve, callback_context, prefix);
				    },
				    critical_command_codec_result::overflow) !=
			    critical_command_codec_result::ok)
				return false;
			flow->prepared_child = std::move(child);
			budget.child = nullptr;
			flow->child_frame_durable = true;
		}
		if (!budget.peak(context.branches.size() * sizeof(flow->branches[0])))
			return false;
		flow->branches.reserve(context.branches.size());
		size_t retained = sizeof(*flow) + key.capacity() + 1;
		const auto charge = [&](size_t value)
		{
			if (value > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained)
				return false;
			retained += value;
			return true;
		};
		if (!charge(flow->branches.capacity() *
			    sizeof(std::unique_ptr<const native_quest_original_branch>)))
			return false;
		for (const auto &original : context.branches)
		{
			if (!budget.peak(sizeof(native_quest_original_branch)))
				return false;
			auto branch = std::make_unique<native_quest_original_branch>();
			budget.branch = branch.get();
			if (!budget.string_assign(branch->message, original.message))
				return false;
			branch->message = original.message;
			if (!budget.string_assign(branch->disappear_message,
						  original.disappear_message))
				return false;
			branch->disappear_message = original.disappear_message;
			if (!budget.string_assign(branch->definition_id, original.definition_id))
				return false;
			branch->definition_id = original.definition_id;
			const auto goals = [&](const auto &from, std::vector<goal_data> &to)
			{
				if (!budget.peak(from.size() * sizeof(goal_data)))
					return false;
				to.reserve(from.size());
				for (const auto &goal : from)
					to.push_back({ static_cast<char>(goal.type), goal.number,
						       nullptr });
				for (size_t i = 1; i < to.size(); ++i)
					to[i - 1].next = &to[i];
				return true;
			};
			if (!goals(original.give, branch->give) ||
			    !goals(original.receive, branch->receive))
				return false;
			branch->view.message = original.message_present ? branch->message.data() :
									  nullptr;
			branch->view.disappear_message = original.disappear_message_present ?
								 branch->disappear_message.data() :
								 nullptr;
			branch->view.echoAll = original.echo_all;
			branch->view.disappear = original.disappear;
			branch->view.give = branch->give.empty() ? nullptr : branch->give.data();
			branch->view.receive = branch->receive.empty() ? nullptr :
									 branch->receive.data();
			if (!charge(sizeof(*branch) + branch->give.capacity() * sizeof(goal_data) +
				    branch->receive.capacity() * sizeof(goal_data) +
				    branch->message.capacity() + 1 +
				    branch->disappear_message.capacity() + 1 +
				    branch->definition_id.capacity() + 1))
				return false;
			flow->branches.push_back(std::move(branch));
			budget.branch = nullptr;
		}
		flow->recovery_bytes = native_quest_gameplay_recovery_bytes(*flow);
		if (!charge(flow->recovery_bytes))
			return false;
		flow->retained_bytes = retained;
		flow->phase = flow->branch_program_frozen ?
				      native_quest_gameplay_phase::choose_branch :
				      native_quest_gameplay_phase::original_give_hooks;
		if (!budget.map_reserve(native_quest_passive_gameplay,
					native_quest_passive_gameplay.size() + 1))
			return false;
		if (!budget.map_node(native_quest_passive_gameplay, key))
			return false;
		auto installed = native_quest_passive_gameplay.emplace(key, flow);
		size_t bytes = 0;
		if (!installed.second || !native_quest_passive_total(&bytes) ||
		    !budget.invoke(
			    [&](size_t prefix) noexcept
			    {
				    return item_native_quest_gameplay_publication_owner::
					    restore_budget_bounded(bytes, reserve, callback_context,
								   prefix);
			    },
			    false))
		{
			if (installed.second)
				native_quest_passive_gameplay.erase(installed.first);
			return false;
		}
		return true; // Exact original branch/hook/child progress, no effect or coordinator call.
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool quest_native_frozen_continuation_owner::restore_bounded(
	const critical_native_recovery_envelope &envelope,
	player_save_coin_replay_budget_scope_owner &scope, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer) noexcept
{
	return quest_native_gameplay_owner::restore_phase2_bounded(envelope, scope, reserve,
								   context, outer);
}
