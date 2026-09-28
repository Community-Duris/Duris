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
#include "persistence/persistence_checkpoint.h"
#include "player/player_revision_state.h"
#include "core/utility.h"
#include "core/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <glob.h>
#include <stdint.h>
#include "magic/spells.h"
#include "sql/sql.h"
#include "world/zone_story_quest_runtime.h"
#include <time.h>

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

static uint64_t legacy_quest_reward_source_id(uint64_t offering_uid, uint32_t reward_index)
{
	if (!offering_uid || !reward_index)
		return 0;
	// The consumed offering names the completion across callback retries. Mix in
	// the reward ordinal so quests granting several items get separate claims.
	uint64_t value = offering_uid ^
			 (static_cast<uint64_t>(reward_index) * 0x9e3779b97f4a7c15ULL) ^
			 0x6c65676163797175ULL;
	value ^= value >> 30;
	value *= 0xbf58476d1ce4e5b9ULL;
	value ^= value >> 27;
	value *= 0x94d049bb133111ebULL;
	value ^= value >> 31;
	return (value & INT64_MAX) ? value & INT64_MAX : 1;
}

void give_reward(struct quest_complete_data *qcp, P_char mob, P_char pl, uint64_t offering_uid)
{
	struct goal_data *gp;
	P_obj obj;
	int i;
	char Gbuf1[MAX_STRING_LENGTH];
	int temp = 1;
	uint32_t reward_index = 0;

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
			++reward_index;
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
			const uint64_t source_id = legacy_quest_reward_source_id(
				offering_uid ? offering_uid : obj->obj_uid, reward_index);

			const bool to_player = (IS_CARRYING_N(pl) < CAN_CARRY_N(pl)) &&
					       ((total_carried_weight(pl) + GET_OBJ_WEIGHT(obj)) <
						CAN_CARRY_W(pl));
			const bool submitted =
				to_player ?
					item_creation_grant_submit_to_player(
						pl, obj, pl, NULL,
						economic_source_kind::quest_completion, source_id) :
					item_creation_grant_submit_to_room(
						pl, obj, pl->in_room,
						economic_source_kind::quest_completion, NULL, source_id);
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
struct quest_durable_context
{
	int quester_id;
	int completion_index;
	int room;
	uint32_t count;
	uint64_t roots[QUEST_DURABLE_MAX_OFFERINGS];
};
static_assert(sizeof(quest_durable_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

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
				uint64_t offering_uid = 0)
{
	const int room_vnum = (world && pl->in_room >= 0) ? world[pl->in_room].number : 0;
	std::string tracking_error;
	if (!zone_story_quest_runtime::record_legacy_completion(
		    pl, completion, room_vnum, static_cast<int64_t>(time(NULL)), &tracking_error))
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

static bool publish_quest_offering(P_char actor, bool committed, const item_transfer_result &,
				   unsigned int, const uint8_t *encoded, size_t encoded_size)
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

static void complete_quest_offering(P_char actor, bool committed, const item_transfer_result &,
				    unsigned int, const uint8_t *encoded, size_t encoded_size)
{
	quest_durable_context context = {};
	if (!committed || !actor || !encoded || encoded_size != sizeof(context))
		return;
	memcpy(&context, encoded, sizeof(context));
	struct quest_complete_data *completion = quest_completion_by_index(context);
	P_char mob = quest_mobile_for(context);
	if (!completion || !mob)
	{
		logit(LOG_DEBUG, "committed quest offering could not find its quest mobile");
		return;
	}
	act(completion->message, FALSE, mob, 0, actor, completion->echoAll ? TO_ROOM : TO_VICT);
	finish_quest_reward(completion, mob, actor, context.roots[0]);
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
		if (economic_gameplay_authority::active())
			for (struct goal_data *reward = completion->receive; reward;
			     reward = reward->next)
				if (reward->goal_type == QUEST_GOAL_COINS)
					supported = false;
		if (!supported || !count)
		{
			unsupported_goal = true;
			break;
		}
		quest_durable_context context = {
			quester_id, completion_index, mob->in_room, static_cast<uint32_t>(count), {}
		};
		for (size_t index = 0; index < count; ++index)
			context.roots[index] = roots[index]->obj_uid;
		const item_owner_identity owner = { item_owner_type::player,
						    static_cast<uint64_t>(GET_PID(actor)), 0 };
		const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
		item_movement_reject reject = item_movement_reject::none;
		if (!item_movement_transaction_submit_batch(
			    actor, roots, count, NULL, owner, destruction,
			    item_transfer_reason::destruction, GET_VNUM(mob),
			    complete_quest_offering, &context, sizeof(context), NULL, &reject,
			    publish_quest_offering, economic_source_kind::intentional_destruction))
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
				finish_quest_reward(qcp, ch, pl);
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
