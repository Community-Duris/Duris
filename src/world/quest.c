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
#include "item/item_movement_transaction.h"
#include "item/item_command_policy.h"
#include "economy/currency_transaction.h"
#include "persistence/persistence_checkpoint.h"
#include "persistence/persistence_mode.h"
#include "persistence/quest_reward_obligation_pipeline.h"
#include "player/player_save_pipeline.h"
#include "player/player_revision_state.h"
#include "player/player_snapshot.h"
#include "world/quest_reward_recovery.h"
#include "world/zone_story_quest_production.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/files.h"
#include <ctype.h>
#include <climits>
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

void quest_reward_recovery_pulse(void)
{
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
constexpr size_t QUEST_DURABLE_MAX_OFFERINGS =
	zone_story_quest_production::ZONE_STORY_QUEST_MAX_DURABLE_OFFERINGS;
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
	uint64_t daily_recipient_mask;
	uint32_t season_id;
	uint32_t catalog_revision;
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
static bool capture_quest_offering_continuation(P_char mob, P_char actor, int quester_id,
						int completion_index,
						const quest_complete_data *completion,
						quest_durable_context &context,
						item_transfer_continuation *continuation)
{
	if (!mob || !actor || !completion || !continuation || !world || mob->in_room < 0 ||
	    GET_PID(actor) <= 0 || context.count == 0 ||
	    context.count > QUEST_DURABLE_MAX_OFFERINGS)
		return false;
	const std::string *definition_id =
		zone_story_quest_production::definition_id_for(completion);
	const int zone_number = zone_story_quest_production::zone_for_giver_vnum(GET_VNUM(mob));
	if (!definition_id || definition_id->empty() ||
	    definition_id->size() > QUEST_REWARD_MAX_DEFINITION_ID_BYTES || zone_number < 0 ||
	    !context.credited_count ||
	    context.credited_count > QUEST_DURABLE_MAX_CREDITED_PLAYERS ||
	    context.party_size != context.credited_count || !context.character_name[0])
		return false;
	context.season_id = zone_story_quest_runtime::current_season_id();
	context.catalog_revision = zone_story_quest_runtime::content_revision();
	context.daily_recipient_mask = 0;
	uint32_t daily_count = 0;
	for (size_t i = 0; i < context.credited_count; ++i)
	{
		P_char recipient = quest_reward_character_present(context.credited_pids[i]);
		if (zone_story_quest_runtime::daily_eligible(recipient, *definition_id,
							     context.strongest_party_level,
							     context.completed_at))
		{
			context.daily_recipient_mask |= UINT64_C(1) << i;
			++daily_count;
		}
	}
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
	const size_t bytes =
		36 + context.count * sizeof(uint64_t) + sizeof(uint32_t) +
		reward_count * sizeof(uint32_t) * 4 + 6 * sizeof(uint32_t) +
		context.credited_count * sizeof(uint32_t) + sizeof(uint32_t) + name_length +
		sizeof(uint32_t) + definition_id->size() + sizeof(uint32_t) +
		static_cast<size_t>(xp_award_count) * 3 * sizeof(uint32_t) + 16 + daily_count * 4;
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
	put32(0, 6);
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
	offset = awards_offset + awards_written * 12;
	put32(offset, context.season_id);
	put32(offset + 4, context.catalog_revision);
	put32(offset + 8, 1);
	put32(offset + 12, daily_count);
	offset += 16;
	for (size_t i = 0; i < context.credited_count; ++i)
		if (context.daily_recipient_mask & (UINT64_C(1) << i))
		{
			put32(offset, context.credited_pids[i]);
			offset += 4;
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
		continuation.version = 6;
		continuation.season_id = context.season_id;
		continuation.catalog_revision = context.catalog_revision;
		continuation.daily_policy_revision = 1;
		for (size_t i = 0; i < context.credited_count; ++i)
			if (context.daily_recipient_mask & (UINT64_C(1) << i))
				continuation.daily_pids[continuation.daily_count++] =
					context.credited_pids[i];
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

void quest_reward_recover_pending(P_char player, const critical_operation_id &offering_operation,
				  const quest_reward_continuation &continuation,
				  uint64_t xp_applied_mask, uint64_t economic_applied_mask,
				  bool economic_history_verified)
{
	if (!player || IS_NPC(player) || GET_PID(player) <= 0 ||
	    static_cast<uint32_t>(GET_PID(player)) != continuation.player_pid ||
	    critical_operation_id_is_zero(offering_operation) || !continuation.root_count ||
	    continuation.root_count > continuation.roots.size())
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
		zone_story_quest_runtime::frozen_daily_context daily;
		if (continuation.version >= 6)
		{
			daily.season_id = continuation.season_id;
			daily.catalog_revision = continuation.catalog_revision;
			daily.policy_revision = continuation.daily_policy_revision;
			daily.eligible_pids.assign(continuation.daily_pids.begin(),
						   continuation.daily_pids.begin() +
							   continuation.daily_count);
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
					tracking_id, continuation.version >= 6 ? &daily : nullptr);
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
		const uint64_t source_id = quest_item_reward_source_id(
			continuation.roots[0], static_cast<int>(reward.number), duplicate_ordinal);
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

				zone_story_quest_runtime::encountered(pl, ch);
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
