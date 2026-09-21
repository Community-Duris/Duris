#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "classes/rogues.h"
#include "combat/damage.h"
#include "combat/guard.h"
#include "combat/justice.h"
#include "item/objmisc.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/item_command_policy.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "world/weather.h"

extern struct str_app_type str_app[];
extern P_char character_list;
extern P_obj object_list;
extern P_room world;

struct slip_movement_context
{
	uint64_t item_uid;
	uint32_t source_pid;
	uint32_t victim_pid;
	int32_t source_room;
	int32_t victim_room;
};

static_assert(sizeof(slip_movement_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

static P_char find_slip_player(uint32_t pid)
{
	for (P_char character = character_list; character; character = character->next)
		if (IS_PC(character) && GET_PID(character) == static_cast<int>(pid))
			return character;
	return NULL;
}

static P_obj find_slip_item(uint64_t item_uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

static bool slip_transfer_publication(P_char /*callback_actor*/, bool committed,
				      const item_transfer_result &result, unsigned int error_code,
				      const uint8_t *encoded, size_t encoded_size)
{
	slip_movement_context context = {};
	if (!encoded || encoded_size != sizeof(context))
	{
		persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
				  "invalid_context", "uid=unknown");
		return false;
	}
	memcpy(&context, encoded, sizeof(context));
	P_char source = find_slip_player(context.source_pid);
	if (!committed)
	{
		if (source)
			send_to_char(
				"The Slip transfer did not commit; the item remains with you.\r\n",
				source);
		logit(LOG_FILE,
		      "item_movement: command=slip outcome=not_committed source_pid=%u "
		      "victim_pid=%u uid=%llu error=%u",
		      context.source_pid, context.victim_pid, (unsigned long long)context.item_uid,
		      error_code);
		return true;
	}
	if (result.root_item_uid != context.item_uid)
	{
		persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
				  "result_identity_mismatch",
				  "expected_uid=%llu result_uid=%llu source_pid=%u",
				  (unsigned long long)context.item_uid,
				  (unsigned long long)result.root_item_uid, context.source_pid);
		return false;
	}

	P_char victim = find_slip_player(context.victim_pid);
	P_obj object = find_slip_item(context.item_uid);
	if (!source || !victim || !object)
	{
		persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
				  "stale_live_topology",
				  "item_uid=%llu source_pid=%u victim_pid=%u",
				  (unsigned long long)context.item_uid, context.source_pid,
				  context.victim_pid);
		return false;
	}
	if (source->in_room != context.source_room || victim->in_room != context.victim_room)
		logit(LOG_FILE,
		      "item_movement: command=slip recovering committed publication after room "
		      "change source_pid=%u victim_pid=%u source_room=%d->%d victim_room=%d->%d",
		      context.source_pid, context.victim_pid, context.source_room, source->in_room,
		      context.victim_room, victim->in_room);
	const bool already_published = OBJ_CARRIED_BY(object, victim);
	if (already_published)
		return true;
	if (IS_OBJ_STAT2(object, ITEM2_CRUMBLELOOT) && !IS_TRUSTED(victim))
	{
		persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
				  "recipient_policy_refused", "item_uid=%llu victim_pid=%u",
				  (unsigned long long)context.item_uid, context.victim_pid);
		return false;
	}
	if (!OBJ_CARRIED_BY(object, victim))
	{
		if (!OBJ_NOWHERE(object) && (!source || !OBJ_CARRIED_BY(object, source)))
		{
			persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
					  "source_not_carrying", "item_uid=%llu source_pid=%u",
					  (unsigned long long)context.item_uid, context.source_pid);
			return false;
		}
		if (total_carried_weight(victim) + GET_OBJ_WEIGHT(object) > CAN_CARRY_W(victim))
		{
			persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
					  "destination_at_capacity", "item_uid=%llu victim_pid=%u",
					  (unsigned long long)context.item_uid, context.victim_pid);
			return false;
		}
		if (source && OBJ_CARRIED_BY(object, source))
			obj_from_char(object);
		obj_to_char(object, victim);
		object = find_slip_item(context.item_uid);
	}
	if (!object || !OBJ_CARRIED_BY(object, victim))
	{
		persistence_alert(AVATAR, "item_movement", "slip_publish", "none", "none",
				  "publication_refused", "item_uid=%llu victim_pid=%u",
				  (unsigned long long)context.item_uid, context.victim_pid);
		return false;
	}

	act("You successfuly slip $p into $N's pockets!", 0, source, object, victim, TO_CHAR);
	if (IS_TRUSTED(source) && GET_LEVEL(source) < OVERLORD)
	{
		statuslog(GET_LEVEL(source), "%s slips %s to %s.", J_NAME(source),
			  object->short_description, J_NAME(victim));
		logit(LOG_WIZ, "%s slips %s to %s.", J_NAME(source), object->short_description,
		      J_NAME(victim));
		sql_log(source, WIZLOG, "Slips %s to %s", object->short_description,
			J_NAME(victim));
	}
	notch_skill(source, SKILL_SLIP, (int)get_property("skill.notch.slip", 5));
	writeCharacter(source, 1, source->in_room);
	writeCharacter(victim, 1, victim->in_room);
	char_light(source);
	room_light(source->in_room, REAL);
	if (source->in_room == victim->in_room)
		nq_action_check(source, victim, NULL);
	return true;
}

void do_slip(P_char ch, char *argument, int /*cmd*/)
{
	char obj_name[MAX_INPUT_LENGTH], vict_name[MAX_INPUT_LENGTH];
	P_char vict;
	P_obj obj, container;
	P_char t_ch = NULL;
	int success, percent, check, bits, bits2;

	argument = one_argument(argument, obj_name);

	argument = one_argument(argument, vict_name);

	/* do they have the skill? */
	if (!(GET_CHAR_SKILL(ch, SKILL_SLIP)))
	{
		send_to_char("You don't know how.\r\n", ch);
		return;
	}

	// Are we targeting a person or object?
	bits = generic_find(vict_name, FIND_OBJ_INV, ch, &t_ch, &container);
	bits2 = generic_find(obj_name, FIND_OBJ_INV, ch, &t_ch, &obj);
	if ((container = get_obj_in_list_vis(ch, vict_name, ch->carrying)))
	{
		if (bits && bits2 && (container->type == ITEM_CONTAINER))
		{
			// Will it fit?
			if (((GET_OBJ_WEIGHT(obj) + container_total_weight(container)) <=
			     container->value[0]) ||
			    ((container->value[0] == -1)))
			{
				put(ch, obj, container, FALSE);
				act("You slip $p into $P...", TRUE, ch, obj, container, TO_CHAR);
				if ((GET_CHAR_SKILL(ch, SKILL_SLIP) - GET_OBJ_WEIGHT(obj)) <
				    number(1, 100))
				{
					send_to_char("But others notice.\r\n", ch);
					act("You notice $n trying to sneak $p into $s $P.", TRUE,
					    ch, obj, container, TO_NOTVICT);
				}
				else
				{
					send_to_char("Successfully, too.\r\n", ch);
				}
			}
			else
			{
				send_to_char("It wont fit.\r\n", ch);
			}
		}
		// Most likely wont see this message.
		else if (!bits && bits2)
		{
			send_to_char("Slip it into what?.\r\n", ch);
		}
		else if (bits && !bits2)
		{
			send_to_char("You don't seem to have anything like that.", ch);
		}
	}
	else
	{
		/* Check if character can see obj/victim, or if obj is cursed, etc... */
		if (!*obj_name || !*vict_name)
		{
			send_to_char("Slip what to who?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
		{
			send_to_char("You do not seem to have anything like that.\r\n", ch);
			return;
		}
		if (IS_SET(obj->extra_flags, ITEM_NODROP) && !IS_TRUSTED(ch))
		{
			send_to_char("You can't let go of it! Yeech!!\r\n", ch);
			return;
		}
		if (!(vict = get_char_room_vis(ch, vict_name)))
		{
			send_to_char("No one by that name around here.\r\n", ch);
			return;
		}
		if ((IS_NPC(vict) && (GET_RNUM(vict) == real_mobile(20))) ||
		    IS_AFFECTED(vict, AFF_WRAITHFORM))
		{
			send_to_char("They couldn't carry that if they tried.\r\n", ch);
			return;
		}

		/* will it immobilize the victim? */
		if ((total_carried_weight(vict) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(vict))
		{
			send_to_char("They cannot possibly carry anymore!\r\n", ch);
			return;
		}
		else
		/* skill check & success check */
		{
			percent = (GET_CHAR_SKILL(ch, SKILL_SLIP) - GET_OBJ_WEIGHT(obj));
			check = number(1, 100);

			if (IS_FIGHTING(ch))
			{
				percent -= (int)get_property("skill.slip.fightingPenalty", 20);
			}

			if (check <= percent)
			{
				success = 1;
			}
			else
			{
				success = 0;
			}
		}

		if (success)
		{
			if (IS_PC(ch) && IS_PC(vict) && ch != vict &&
			    item_command_uses_durable_ownership(obj))
			{
				const item_owner_identity source = {
					item_owner_type::player, static_cast<uint64_t>(GET_PID(ch)),
					0
				};
				const item_owner_identity destination = {
					item_owner_type::player,
					static_cast<uint64_t>(GET_PID(vict)), 0
				};
				item_ownership_runtime_entry ownership = {};
				if (!obj->obj_uid ||
				    !item_ownership_runtime_lookup(obj->obj_uid, &ownership) ||
				    ownership.state != item_custody_state::active ||
				    !item_owner_identity_equal(ownership.owner, source))
				{
					send_to_char(
						"The item's ownership records refused the Slip; nothing was moved.\r\n",
						ch);
					logit(LOG_FILE,
					      "item_movement: command=slip outcome=owner_mismatch actor=%s uid=%llu",
					      J_NAME(ch), (unsigned long long)obj->obj_uid);
					return;
				}
				if (IS_OBJ_STAT2(obj, ITEM2_CRUMBLELOOT) && !IS_TRUSTED(vict))
				{
					send_to_char(
						"That item cannot be slipped to a player without crumbling; nothing was moved.\r\n",
						ch);
					return;
				}
				const slip_movement_context context = {
					obj->obj_uid, static_cast<uint32_t>(GET_PID(ch)),
					static_cast<uint32_t>(GET_PID(vict)), ch->in_room,
					vict->in_room
				};
				item_movement_reject reject = item_movement_reject::none;
				if (!item_movement_transaction_submit(
					    ch, obj, NULL, source, destination,
					    item_transfer_reason::slip, GET_PID(ch), NULL, &context,
					    sizeof(context), NULL, &reject,
					    slip_transfer_publication))
				{
					send_to_char(
						"The Slip transfer could not start; the item remains with you.\r\n",
						ch);
					logit(LOG_FILE,
					      "item_movement: command=slip outcome=%s actor=%s uid=%llu",
					      item_movement_reject_name(reject), J_NAME(ch),
					      (unsigned long long)obj->obj_uid);
					return;
				}
				send_to_char(
					"The Slip transfer is pending; the item remains with you until it commits.\r\n",
					ch);
				return;
			}

			/* notch skill */
			notch_skill(ch, SKILL_SLIP, (int)get_property("skill.notch.slip", 5));

			/* transfer the obj */
			obj_from_char(obj);
			obj_to_char(obj, vict);
			// If this is not suppost to be a secretive hand off, uncomment below.
			// act("$n slips $p to $N.", 1, ch, obj, vict, TO_NOTVICT);
			act("You successfuly slip $p into $N's pockets!", 0, ch, obj, vict,
			    TO_CHAR);

			/* for now repor it anytime */
			if (IS_TRUSTED(ch) && GET_LEVEL(ch) < OVERLORD)
			{
				statuslog(GET_LEVEL(ch), "%s slips %s to %s.", J_NAME(ch),
					  obj->short_description, J_NAME(vict));
				logit(LOG_WIZ, "%s slips %s to %s.", J_NAME(ch),
				      obj->short_description, J_NAME(vict));
				sql_log(ch, WIZLOG, "Slips %s to %s", obj->short_description,
					J_NAME(vict));
			}
			if (ch != vict)
			{
				writeCharacter(ch, 1, ch->in_room);
				writeCharacter(vict, 1, vict->in_room);
			}

			/* something about a light bug in do_give */
			char_light(ch);
			room_light(ch->in_room, REAL);
			nq_action_check(ch, vict, NULL);
		}
		else
		{
			// act("The weight of %p proves too much for your slyness.", 0, ch, obj, 0, TO_CHAR);
			/* an idea: if you want a fail to place the object in the room (a penalty for poor skill) uncomment below */
			act("You fumble $p as its weight proves too much for your slyness.", 0, ch,
			    obj, 0, TO_CHAR);
			act("$p falls to the ground as $n slyly looks the other way.", 0, ch, obj,
			    0, TO_NOTVICT);
			obj_from_char(obj);
			obj_to_room(obj, ch->in_room);

			writeCharacter(ch, 1, ch->in_room);
		}
	}
}
