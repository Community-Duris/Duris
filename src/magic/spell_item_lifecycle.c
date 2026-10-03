#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "core/utils.h"
#include "core/defines.h"
#include "core/safe_format.h"
#include "combat/damage.h"
#include "world/achievements.h"
#include "item/objmisc.h"
#include "item/item_command_policy.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/critical_command.h"
#include "persistence/persistence_checkpoint.h"
#include "kingdom/kingdom_store_piece.h"
#include "magic/spells.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern P_char character_list;
extern P_obj object_list;
extern struct str_app_type str_app[];

enum class conjured_weapon_kind : uint8_t
{
	ensis_unguis,
	lancea_cineralae,
	simulacrum_anguis,
};

struct conjured_weapon_grant_context
{
	uint64_t item_uid;
	uint32_t actor_pid;
	float self_damage;
	uint8_t kind;
};

static_assert(sizeof(conjured_weapon_grant_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

static int conjured_weapon_vnum(uint8_t kind)
{
	switch (static_cast<conjured_weapon_kind>(kind))
	{
	case conjured_weapon_kind::ensis_unguis:
		return 56;
	case conjured_weapon_kind::lancea_cineralae:
		return 93;
	case conjured_weapon_kind::simulacrum_anguis:
		return 171;
	}
	return -1;
}

static P_obj magic_find_object_by_uid(uint64_t item_uid)
{
	if (!item_uid)
		return NULL;
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

static void conjured_weapon_publish_effect(P_char actor, P_obj blade, conjured_weapon_kind kind,
					   float self_damage)
{
	if (!actor || !blade || !IS_ALIVE(actor))
		return;

	switch (kind)
	{
	case conjured_weapon_kind::ensis_unguis:
		act("$n forearm snaps as $e tears free $p from $s &+rflesh&n.", TRUE, actor, blade,
		    0, TO_ROOM);
		act("Your forearm snaps as you tear free $p from your &+rflesh&n.", TRUE, actor,
		    blade, 0, TO_CHAR);
		break;
	case conjured_weapon_kind::lancea_cineralae:
		act("As $n writhes in agony a disgusting growth erupts from $s shoulder $e rips away $p.",
		    TRUE, actor, blade, 0, TO_ROOM);
		act("You writhe in agony as a disgusting growth erupts from your shoulder, you rip away $p.",
		    TRUE, actor, blade, 0, TO_CHAR);
		break;
	case conjured_weapon_kind::simulacrum_anguis:
		act("$n clutches $s abdomen as $s entrails burst out in a writhing mass, forming $p.",
		    TRUE, actor, blade, 0, TO_ROOM);
		act("You clutch your abdomen as your entrails burst out in a writhing mass, forming $p.",
		    TRUE, actor, blade, 0, TO_CHAR);
		break;
	}

	spell_damage(actor, actor, self_damage, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

static void conjured_weapon_grant_completed(P_char actor, bool committed,
					    const item_transfer_result &result,
					    unsigned int /*error_code*/, const uint8_t *encoded,
					    size_t encoded_size)
{
	if (!actor || !encoded || encoded_size != sizeof(conjured_weapon_grant_context) ||
	    GET_PID(actor) <= 0)
		return;

	conjured_weapon_grant_context context = {};
	memcpy(&context, encoded, sizeof(context));
	if (context.actor_pid != static_cast<uint32_t>(GET_PID(actor)) ||
	    conjured_weapon_vnum(context.kind) < 0 || !isfinite(context.self_damage) ||
	    context.self_damage < 0.0f)
	{
		logit(LOG_FILE,
		      "conjured weapon completion had invalid context (uid=%llu pid=%d kind=%u damage=%f)",
		      (unsigned long long)context.item_uid, GET_PID(actor),
		      static_cast<unsigned int>(context.kind), context.self_damage);
		return;
	}
	if (!committed)
	{
		send_to_char(
			"The conjured weapon could not be delivered; no health was spent. Please try again later.\r\n",
			actor);
		return;
	}
	if (result.root_item_uid != context.item_uid || !result.item_count)
	{
		logit(LOG_FILE,
		      "conjured weapon completion identity mismatch (expected=%llu actual=%llu pid=%d)",
		      (unsigned long long)context.item_uid,
		      (unsigned long long)result.root_item_uid, GET_PID(actor));
		return;
	}

	P_obj blade = magic_find_object_by_uid(context.item_uid);
	if (!blade || !OBJ_CARRIED_BY(blade, actor) ||
	    OBJ_VNUM(blade) != conjured_weapon_vnum(context.kind))
	{
		logit(LOG_FILE,
		      "conjured weapon grant committed without live publication (uid=%llu pid=%d)",
		      (unsigned long long)context.item_uid, GET_PID(actor));
		return;
	}
	conjured_weapon_publish_effect(
		actor, blade, static_cast<conjured_weapon_kind>(context.kind), context.self_damage);
}

static uint64_t conjured_weapon_source_id()
{
	critical_operation_id occurrence = {};
	if (!critical_operation_id_generate(&occurrence))
		return 0;
	uint64_t source_id = 0;
	for (size_t index = 0; index < sizeof(source_id); ++index)
		source_id |= static_cast<uint64_t>(occurrence.bytes[index]) << (index * 8);
	return source_id;
}

static bool submit_conjured_weapon(P_char actor, P_obj blade, conjured_weapon_kind kind)
{
	if (!actor || !blade)
		return false;
	const bool active = economic_gameplay_authority::active();
	if (!IS_PC(actor) && !active)
	{
		obj_to_char(blade, actor);
		conjured_weapon_publish_effect(actor, blade, kind, GET_HIT(actor) * 0.10f);
		return true;
	}

	const conjured_weapon_grant_context context = {
		blade->obj_uid,
		static_cast<uint32_t>(GET_PID(actor)),
		GET_HIT(actor) * 0.10f,
		static_cast<uint8_t>(kind),
	};
	const uint64_t source_id = IS_PC(actor) ? conjured_weapon_source_id() : 0;
	if (IS_PC(actor) && source_id &&
	    item_creation_grant_submit_to_player_with_completion(
		    actor, blade, actor, conjured_weapon_grant_completed, &context, sizeof(context),
		    NULL, economic_source_kind::spell_creation, source_id))
		return true;

	extract_obj(blade, FALSE);
	if (IS_PC(actor))
		send_to_char(
			"The conjured weapon could not be created right now; no health was spent. Please try again later.\r\n",
			actor);
	return false;
}

int has_soulbind(P_char ch)
{
	int result = 0;
	struct affected_type *findaf, *next_af;

	for (findaf = ch->affected; findaf; findaf = next_af)
	{
		next_af = findaf->next;
		if (findaf->type == TAG_SOULBIND)
		{
			result = findaf->modifier;
			break;
		}
	}
	return result;
}

static void remove_soulbind_except(P_char ch, uint64_t keep_uid)
{
	P_obj obj, next_obj;

	// find any instance of their soulbound item and remove it
	for (obj = object_list; obj; obj = next_obj)
	{
		next_obj = obj->next;
		/* Store gear is not soulbound (ruled 2026-09-16), so this spell
		 * does not meet it in practice. The guard stays anyway: a store
		 * piece's keywords are ordinary words, including its buyer's name,
		 * so if one ever were soulbound, matching names here would destroy
		 * every such piece whose keywords hold this character's name.
		 * kingdom_store_bound() knows one by its maker's mark as well as
		 * by its vnum. */
		if (obj->obj_uid != keep_uid && obj->name &&
		    IS_SET(obj->extra2_flags, ITEM2_SOULBIND) && !kingdom_store_bound(obj) &&
		    isname(GET_NAME(ch), obj->name))
		{
			if (economic_gameplay_authority::active() &&
			    item_command_uses_durable_ownership(obj))
			{
				logit(LOG_FILE,
				      "soulbind retirement withheld without item custody (uid=%llu)",
				      (unsigned long long)obj->obj_uid);
				continue;
			}
			extract_obj(obj);
		}
	}
}

void remove_soulbind(P_char ch)
{
	remove_soulbind_except(ch, 0);
}

void load_soulbind(P_char ch);

struct soulbind_movement_context
{
	uint64_t item_uid;
	uint32_t source_pid;
	uint32_t victim_pid;
	int32_t source_room;
	int32_t victim_room;
	uint8_t replace_existing;
};

static_assert(sizeof(soulbind_movement_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

static P_char find_soulbind_player(uint32_t pid)
{
	for (P_char character = character_list; character; character = character->next)
		if (IS_PC(character) && GET_PID(character) == static_cast<int>(pid))
			return character;
	return NULL;
}

static P_obj find_soulbind_item(uint64_t item_uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

static bool soulbind_metadata_applied(P_char victim, P_obj obj)
{
	if (!victim || !obj)
		return false;
	const int vnum = OBJ_VNUM(obj);
	return vnum >= 0 && has_soulbind(victim) == vnum && IS_OBJ_STAT2(obj, ITEM2_SOULBIND);
}

static bool apply_soulbind_metadata(P_char victim, P_obj obj, bool require_durable_save = false)
{
	if (!victim || !obj || OBJ_VNUM(obj) < 0 || !obj->name || !obj->short_description)
		return false;
	if (!soulbind_metadata_applied(victim, obj))
	{
		struct affected_type af;
		char gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
		memset(&af, 0, sizeof(struct affected_type));
		af.type = TAG_SOULBIND;
		af.modifier = OBJ_VNUM(obj);
		af.duration = -1;
		af.location = 0;
		af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM | AFFTYPE_NODISPEL;
		affect_to_char(victim, &af);

		snprintf(gbuf2, MAX_STRING_LENGTH, "%s %s", GET_NAME(victim), obj->name);
		if ((obj->str_mask & STRUNG_KEYS) && obj->name)
			str_free(obj->name);
		obj->str_mask |= STRUNG_KEYS;
		obj->name = str_dup(gbuf2);

		snprintf(buffer, MAX_STRING_LENGTH, "%s &+Lbearing the &+Wsoul&+L of &+r%s&n",
			 obj->short_description, GET_NAME(victim));
		set_short_description(obj, buffer);

		SET_BIT(obj->extra_flags, ITEM_NOSELL);
		SET_BIT(obj->extra_flags, ITEM_NORENT);
		SET_BIT(obj->extra2_flags, ITEM2_CRUMBLELOOT);
		SET_BIT(obj->extra2_flags, ITEM2_SOULBIND);
		REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
		REMOVE_BIT(obj->extra_flags, ITEM_INVISIBLE);
		SET_BIT(obj->extra_flags, ITEM_NOREPAIR);
		REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
		act("&+W$n &+rbegins to chant loudly, calling forth the &+Bblood &+rof their enemies. &+W$n's &+rhands begin to &+Rg&+rl&+Ro&+rw &+Rbrightly &+ras drops of blood begin to form.\r\n"
		    "&+W$n &+rgently takes the &+Rblood&+r and begins to spread it about their $p&+r, which starts to glow with an &+Lun&+rho&+Lly &+Rlight.&N",
		    TRUE, victim, obj, 0, TO_ROOM);
		act("&+rYou begin to chant loudly, calling forth the &+Bblood &+rof your enemies. Your &+rhands begin to &+Rg&+rl&+Ro&+rw &+Rbrightly &+ras drops of blood begin to form.\r\n"
		    "&+rYou &+rgently take the &+Rblood&+r and begin to spread it about your $p&+r, which starts to glow with an &+Lun&+rho&+Lly &+Rlight.&N",
		    FALSE, victim, obj, 0, TO_CHAR);
	}
	if (IS_PC(victim) && GET_PID(victim) > 0)
		mark_player_dirty_components(GET_PID(victim), PLAYER_COMPONENT_STATUS |
								      PLAYER_COMPONENT_EQUIPMENT |
								      PLAYER_COMPONENT_INVENTORY);
	if (!do_save_silent(victim, 1))
	{
		logit(LOG_WIZ, "Failed to save %s after soulbind.", GET_NAME(victim));
		return !require_durable_save;
	}
	return true;
}

static bool publish_nonplayer_soulbind(P_char source, P_char victim, P_obj object)
{
	if (!source || !victim || !object || !OBJ_CARRIED_BY(object, source))
		return false;
	obj_from_char(object);
	obj_to_char(object, victim);
	return OBJ_CARRIED_BY(object, victim);
}

static bool soulbind_transfer_publication(const critical_operation_id & /*operation_id*/,
					  P_char /*callback_actor*/, bool committed,
					  const item_transfer_result &result,
					  unsigned int error_code, const uint8_t *encoded,
					  size_t encoded_size)
{
	soulbind_movement_context context = {};
	if (!encoded || encoded_size != sizeof(context))
	{
		persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none", "none",
				  "invalid_context", "uid=unknown");
		return false;
	}
	memcpy(&context, encoded, sizeof(context));
	P_char owner = find_soulbind_player(context.source_pid);
	if (!committed)
	{
		if (owner)
			send_to_char(
				"The soulbind transfer did not commit; the item remains with you.\r\n",
				owner);
		logit(LOG_FILE,
		      "item_movement: command=soulbind outcome=not_committed source_pid=%u "
		      "victim_pid=%u uid=%llu error=%u",
		      context.source_pid, context.victim_pid, (unsigned long long)context.item_uid,
		      error_code);
		return true;
	}
	if (result.root_item_uid != context.item_uid)
	{
		persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none", "none",
				  "result_identity_mismatch",
				  "expected_uid=%llu result_uid=%llu source_pid=%u",
				  (unsigned long long)context.item_uid,
				  (unsigned long long)result.root_item_uid, context.source_pid);
		return false;
	}

	P_char victim = find_soulbind_player(context.victim_pid);
	P_obj object = find_soulbind_item(context.item_uid);
	if (!owner || !victim || !object)
	{
		persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none", "none",
				  "stale_live_topology",
				  "item_uid=%llu source_pid=%u victim_pid=%u",
				  (unsigned long long)context.item_uid, context.source_pid,
				  context.victim_pid);
		return false;
	}
	if ((context.source_room >= 0 && owner->in_room != context.source_room) ||
	    (context.victim_room >= 0 && victim->in_room != context.victim_room))
		logit(LOG_FILE,
		      "item_movement: command=soulbind recovering committed publication after room "
		      "change source_pid=%u victim_pid=%u source_room=%d->%d victim_room=%d->%d",
		      context.source_pid, context.victim_pid, context.source_room, owner->in_room,
		      context.victim_room, victim->in_room);
	if (OBJ_CARRIED_BY(object, victim) && soulbind_metadata_applied(victim, object))
		return apply_soulbind_metadata(victim, object, true);
	if (has_soulbind(victim) != 0 && !context.replace_existing)
	{
		persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none", "none",
				  "victim_already_soulbound", "item_uid=%llu victim_pid=%u",
				  (unsigned long long)context.item_uid, context.victim_pid);
		return false;
	}
	if (!OBJ_CARRIED_BY(object, victim))
	{
		if (!OBJ_NOWHERE(object) && (!owner || !OBJ_CARRIED_BY(object, owner)))
		{
			persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none",
					  "none", "source_not_carrying",
					  "item_uid=%llu source_pid=%u",
					  (unsigned long long)context.item_uid, context.source_pid);
			return false;
		}
		if (total_carried_weight(victim) + GET_OBJ_WEIGHT(object) > CAN_CARRY_W(victim))
		{
			persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none",
					  "none", "destination_at_capacity",
					  "item_uid=%llu victim_pid=%u",
					  (unsigned long long)context.item_uid, context.victim_pid);
			return false;
		}
		if (owner && OBJ_CARRIED_BY(object, owner))
			obj_from_char(object);
		obj_to_char(object, victim);
		object = find_soulbind_item(context.item_uid);
	}
	if (!object || !OBJ_CARRIED_BY(object, victim))
	{
		persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none", "none",
				  "publication_refused", "item_uid=%llu victim_pid=%u",
				  (unsigned long long)context.item_uid, context.victim_pid);
		return false;
	}

	if (has_soulbind(victim) != 0)
	{
		if (!context.replace_existing)
		{
			persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none",
					  "none", "victim_already_soulbound",
					  "item_uid=%llu victim_pid=%u",
					  (unsigned long long)context.item_uid, context.victim_pid);
			return false;
		}
		remove_soulbind(victim);
		for (struct affected_type *findaf = victim->affected; findaf; findaf = findaf->next)
			if (findaf->type == TAG_SOULBIND)
			{
				affect_remove(victim, findaf);
				break;
			}
		if (owner)
			send_to_char(
				"Cleared the recipient's previous soulbind after the transfer committed.\r\n",
				owner);
	}
	if (!apply_soulbind_metadata(victim, object, true))
	{
		persistence_alert(AVATAR, "item_movement", "soulbind_publish", "none", "none",
				  "metadata_apply_failed", "item_uid=%llu victim_pid=%u",
				  (unsigned long long)context.item_uid, context.victim_pid);
		return false;
	}
	if (owner)
		mark_player_dirty_components(GET_PID(owner), PLAYER_COMPONENT_STATUS |
								     PLAYER_COMPONENT_EQUIPMENT |
								     PLAYER_COMPONENT_INVENTORY);
	mark_player_dirty_components(GET_PID(victim), PLAYER_COMPONENT_STATUS |
							      PLAYER_COMPONENT_EQUIPMENT |
							      PLAYER_COMPONENT_INVENTORY);
	return true;
}

bool spell_item_lifecycle_restore_replayed_command(const critical_command &command)
{
	if (command.type != critical_command_type::item_transfer || !command.publication_required)
		return true;
	item_transfer_payload payload = {};
	if (!item_transfer_command_decode_payload(command, &payload))
		return false;
	if (payload.reason != item_transfer_reason::soulbind ||
	    payload.continuation.kind != item_transfer_continuation_kind::soulbind_transfer)
		return true;
	if (payload.continuation.data.size() != 1 || payload.continuation.data[0] > 1 ||
	    !payload.selected_item_uid || payload.from_owner.id > UINT32_MAX ||
	    payload.to_owner.id > UINT32_MAX)
		return false;
	const soulbind_movement_context context = { payload.selected_item_uid,
						    static_cast<uint32_t>(payload.from_owner.id),
						    static_cast<uint32_t>(payload.to_owner.id),
						    -1,
						    -1,
						    payload.continuation.data[0] };
	return item_movement_transaction_restore_replayed_publication(
		command, soulbind_transfer_publication, &context, sizeof(context));
}

struct soulbind_reload_context
{
	uint64_t item_uid;
	uint32_t recipient_pid;
	int32_t item_vnum;
};

static_assert(sizeof(soulbind_reload_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

static void soulbind_reload_completed(P_char actor, bool committed,
				      const item_transfer_result &result,
				      unsigned int /*error_code*/, const uint8_t *encoded,
				      size_t encoded_size)
{
	if (!actor || !encoded || encoded_size != sizeof(soulbind_reload_context) ||
	    GET_PID(actor) <= 0)
		return;

	soulbind_reload_context context = {};
	memcpy(&context, encoded, sizeof(context));
	if (context.recipient_pid != static_cast<uint32_t>(GET_PID(actor)) ||
	    context.item_vnum <= 0)
	{
		logit(LOG_FILE,
		      "soulbind reload completion had invalid context (uid=%llu pid=%d vnum=%d)",
		      (unsigned long long)context.item_uid, GET_PID(actor), context.item_vnum);
		return;
	}
	if (!committed)
	{
		send_to_char(
			"The soulbound item could not be restored; your existing soulbound item was kept.\r\n",
			actor);
		return;
	}
	if (result.root_item_uid != context.item_uid || !result.item_count)
	{
		logit(LOG_FILE,
		      "soulbind reload completion identity mismatch (expected=%llu actual=%llu pid=%d)",
		      (unsigned long long)context.item_uid,
		      (unsigned long long)result.root_item_uid, GET_PID(actor));
		send_to_char(
			"The ownership authority returned an unexpected soulbound item; your existing item was kept.\r\n",
			actor);
		return;
	}

	P_obj replacement = magic_find_object_by_uid(context.item_uid);
	if (!replacement ||
	    (!OBJ_CARRIED_BY(replacement, actor) && !OBJ_WORN_BY(replacement, actor)) ||
	    OBJ_VNUM(replacement) != context.item_vnum ||
	    !IS_OBJ_STAT2(replacement, ITEM2_SOULBIND))
	{
		logit(LOG_FILE,
		      "soulbind reload committed without live publication (uid=%llu pid=%d)",
		      (unsigned long long)context.item_uid, GET_PID(actor));
		send_to_char(
			"The ownership authority restored the soulbound item, but it is not available yet.\r\n",
			actor);
		return;
	}
	if (has_soulbind(actor) != context.item_vnum)
	{
		logit(LOG_FILE,
		      "soulbind reload found changed binding state (uid=%llu pid=%d expected=%d actual=%d)",
		      (unsigned long long)context.item_uid, GET_PID(actor), context.item_vnum,
		      has_soulbind(actor));
		send_to_char(
			"Your soulbound state changed while the replacement was arriving; the existing state was kept.\r\n",
			actor);
		return;
	}

	remove_soulbind_except(actor, context.item_uid);
	send_to_char(
		"&+yAfter a brief moment, you feel &+Wwhole&+y once again, ready to &+rconquer &+ythe world.\r\n",
		actor);
}

void do_soulbind(P_char ch, char *argument, int /*cmd*/)
{
	P_obj obj;
	P_char victim;
	char gbuf1[MAX_STRING_LENGTH], gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH],
		gbuf3[MAX_STRING_LENGTH];
	struct affected_type *findaf;
	bool replace_existing = false;

	argument_interpreter(argument, gbuf1, gbuf2);

	if (IS_TRUSTED(ch))
	{
		if (!*gbuf1 || !(victim = ParseTarget(ch, gbuf1)))
		{
			send_to_char("To Remove Character's Soulbound Status: soulbind <char>\r\n",
				     ch);
			send_to_char(
				"To Set Character's Soulbound Status: soulbind <char> <item>\r\n",
				ch);
			return;
		}

		for (findaf = victim->affected; findaf; findaf = findaf->next)
		{
			if (findaf->type == TAG_SOULBIND)
			{
				if (*gbuf2)
				{
					if (economic_gameplay_authority::active())
					{
						send_to_char(
							"Soulbound items cannot be replaced right now.\r\n",
							ch);
						return;
					}
					replace_existing = true;
					break;
				}
				if (economic_gameplay_authority::active())
				{
					send_to_char(
						"Soulbound items cannot be cleared right now.\r\n",
						ch);
					return;
				}
				remove_soulbind(victim);
				affect_remove(victim, findaf);
				snprintf(buffer, MAX_STRING_LENGTH, "%s", GET_NAME(victim));
				checked_snprintf(gbuf3, MAX_STRING_LENGTH,
						 "Cleared soulbind status on %s.\r\n", buffer);
				send_to_char(gbuf3, ch);
				logit(LOG_WIZ, "%s cleared soulbind on %s", J_NAME(ch),
				      J_NAME(victim));
				return;
			}
		}
	}
	else
	{
		victim = ch;
	}

	if (has_soulbind(victim) != 0 && !replace_existing)
	{
		send_to_char(
			"&+yYour &+Ysoul &+ycalls out to the world to bring forth your &+ritem&+y...\r\n",
			victim);
		load_soulbind(victim);
		return;
	}

	// If victim doesn't have soulbind, and God isn't setting it for them.
	if (get_frags(victim) < 2000 && !IS_TRUSTED(ch))
	{
		send_to_char("&+LYou have not yet earned the right to use that ability.\r\n",
			     victim);
		return;
	}

	if (IS_TRUSTED(ch) ? *gbuf2 : *gbuf1)
	{
		// We look for the item in the God/high fragger's inventory (not necessarily the victim).
		if (!(obj = get_obj_in_list(IS_TRUSTED(ch) ? gbuf2 : gbuf1, ch->carrying)))
		{
			send_to_char(
				"&+rYou must be carrying the item you wish to &+Wsoulbind&+r in your inventory.&n\r\n",
				ch);
			return;
		}

		// If the victim has a soulbound item already (note: for God setting for someone, it was cleared above).
		if (affected_by_spell(victim, TAG_SOULBIND) && !replace_existing)
		{
			send_to_char("&+rA character may only &+Wsoulbind&+r once.&n\r\n", victim);
			return;
		}

		// Make sure object is valid type, not arti, etc.
		if ((obj->type == ITEM_CONTAINER || obj->type == ITEM_STORAGE ||
		     obj->type == ITEM_TREASURE || obj->type == ITEM_POTION ||
		     obj->type == ITEM_TELEPORT || obj->type == ITEM_WAND ||
		     obj->type == ITEM_KEY || obj->contains || obj->type == ITEM_FOOD ||
		     IS_OBJ_STAT2(obj, ITEM2_STOREITEM) || IS_OBJ_STAT2(obj, ITEM2_SOULBIND) ||
		     IS_OBJ_STAT(obj, ITEM_NOSELL) || IS_SET(obj->extra_flags, ITEM_ARTIFACT)))
		{
			// This message goes to the function caller, not necessarily the victim.
			send_to_char("That item is not a valid type to &+Wsoulbind&n.\r\n", ch);
			return;
		}

		if (ch != victim && (!IS_PC(ch) || !IS_PC(victim)))
		{
			if (!publish_nonplayer_soulbind(ch, victim, obj) ||
			    !apply_soulbind_metadata(victim, obj))
			{
				send_to_char("The soulbind could not be applied.\r\n", ch);
			}
			return;
		}

		if (ch != victim)
		{
			const item_owner_identity source = { item_owner_type::player,
							     static_cast<uint64_t>(GET_PID(ch)),
							     0 };
			const item_owner_identity destination = {
				item_owner_type::player, static_cast<uint64_t>(GET_PID(victim)), 0
			};
			if (total_carried_weight(victim) + GET_OBJ_WEIGHT(obj) >
			    CAN_CARRY_W(victim))
			{
				send_to_char("The recipient cannot carry that item right now.\r\n",
					     ch);
				return;
			}
			item_ownership_runtime_entry ownership = {};
			if (!obj->obj_uid ||
			    !item_ownership_runtime_lookup(obj->obj_uid, &ownership) ||
			    ownership.state != item_custody_state::active ||
			    !item_owner_identity_equal(ownership.owner, source))
			{
				send_to_char(
					"The item's ownership records do not permit this soulbind transfer.\r\n",
					ch);
				logit(LOG_FILE,
				      "item_movement: command=soulbind outcome=owner_mismatch actor=%s uid=%llu",
				      J_NAME(ch), (unsigned long long)obj->obj_uid);
				return;
			}
			const soulbind_movement_context context = {
				obj->obj_uid,
				static_cast<uint32_t>(GET_PID(ch)),
				static_cast<uint32_t>(GET_PID(victim)),
				ch->in_room,
				victim->in_room,
				static_cast<uint8_t>(replace_existing)
			};
			const item_transfer_continuation continuation = {
				item_transfer_continuation_kind::soulbind_transfer,
				{ static_cast<uint8_t>(replace_existing) }
			};
			item_movement_reject reject = item_movement_reject::none;
			if (!item_movement_transaction_submit(
				    ch, obj, NULL, source, destination,
				    item_transfer_reason::soulbind, GET_PID(ch), NULL, &context,
				    sizeof(context), NULL, &reject, soulbind_transfer_publication,
				    {}, 0, continuation))
			{
				send_to_char(
					"The soulbind transfer could not start; the item and recipient were unchanged.\r\n",
					ch);
				logit(LOG_FILE,
				      "item_movement: command=soulbind outcome=%s actor=%s uid=%llu",
				      item_movement_reject_name(reject), J_NAME(ch),
				      (unsigned long long)obj->obj_uid);
				return;
			}
			send_to_char(
				"The soulbind transfer is pending; no item or soulbound metadata changes until it commits.\r\n",
				ch);
			return;
		}

		if (!apply_soulbind_metadata(victim, obj))
		{
			send_to_char("The soulbind could not be applied.\r\n", ch);
			return;
		}
	}
	else
	{
		send_to_char("What item would you like to &+Wsoulbind&n?\r\n", ch);
	}
}

void load_soulbind(P_char ch)
{
	int item;
	P_obj obj;
	char gbuf2[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];

	if (ch && economic_gameplay_authority::active())
	{
		send_to_char("Soulbound items cannot be restored right now.\r\n", ch);
		return;
	}

	item = has_soulbind(ch);
	if (item == 0)
	{
		send_to_char("&+rYour &+Wsoul &+rhas not bound with anything yet.&n\r\n", ch);
		return;
	}
	/* snprintf(gbuf2, MAX_STRING_LENGTH, "%d", item);
	 send_to_char(gbuf2, ch);*/
	obj = read_object(item, VIRTUAL);
	if (!obj)
	{
		send_to_char(
			"The soulbound item could not be recreated; your existing soulbound item was kept.\r\n",
			ch);
		return;
	}
	snprintf(gbuf2, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), obj->name);
	obj->name = str_dup(gbuf2);
	snprintf(buffer, MAX_STRING_LENGTH, "%s &+Lbearing the &+Wsoul&+L of &+r%s&n",
		 obj->short_description, GET_NAME(ch));
	set_short_description(obj, buffer);
	SET_BIT(obj->extra_flags, ITEM_NOSELL);
	SET_BIT(obj->extra_flags, ITEM_NORENT);
	SET_BIT(obj->extra2_flags, ITEM2_CRUMBLELOOT);
	SET_BIT(obj->extra2_flags, ITEM2_SOULBIND);
	REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
	REMOVE_BIT(obj->extra_flags, ITEM_INVISIBLE);
	SET_BIT(obj->extra_flags, ITEM_NOREPAIR);
	REMOVE_BIT(obj->extra_flags, ITEM_NODROP);

	if (!IS_PC(ch))
	{
		obj_to_char(obj, ch);
		remove_soulbind_except(ch, obj->obj_uid);
		send_to_char(
			"&+yAfter a brief moment, you feel &+Wwhole&+y once again, ready to &+rconquer &+ythe world.\r\n",
			ch);
		return;
	}

	const soulbind_reload_context context = {
		obj->obj_uid,
		static_cast<uint32_t>(GET_PID(ch)),
		item,
	};
	if (item_creation_grant_submit_to_player_with_completion(
		    ch, obj, ch, soulbind_reload_completed, &context, sizeof(context), NULL,
		    economic_source_kind::lifecycle))
		return;

	extract_obj(obj, FALSE);
	send_to_char(
		"The soulbound item could not be restored; your existing soulbound item was kept.\r\n",
		ch);
}

/* ---- DRAGOON SPELLS ----*/
void spell_ensis_unguis(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			P_obj /*obj*/)
{
	P_obj blade;

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	blade = read_object(real_object(56), REAL);
	if (!blade)
	{
		logit(LOG_DEBUG, "spell_ensis_unguis(): obj 56 not loadable");
		return;
	}

	blade->extra_flags |= ITEM_NORENT;
	blade->bitvector = 0;
	blade->value[6] = GET_LEVEL(ch);

	/* how about some gay de procs for flame blade? Yeah baby! */
	if (GET_LEVEL(ch) >= 51)
	{
		blade->value[5] = 124;
		blade->value[7] = 30; // procs sunray
	}
	else if (GET_LEVEL(ch) >= 41)
	{
		blade->value[5] = 26;
		blade->value[7] = 25; // procs fireball, better chance.
	}
	else if (GET_LEVEL(ch) >= 36)
	{
		blade->value[5] = 26;
		blade->value[7] = 40; // procs fireball
	}
	else if (GET_LEVEL(ch) >= 21)
	{
		blade->value[5] = 195;
		blade->value[7] = 40; // procs flameburst
	}

	blade->timer[0] = 180;
	if (IS_PC(ch))
		blade->timer[1] = GET_PID(ch);
	else
		blade->timer[1] = -1;

	submit_conjured_weapon(ch, blade, conjured_weapon_kind::ensis_unguis);
}

void spell_lancea_cineralae(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
			    P_char /*victim*/, P_obj /*obj*/)
{
	P_obj blade;

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	blade = read_object(real_object(93), REAL);
	if (!blade)
	{
		logit(LOG_DEBUG, "spell_lancea_cineralae(): obj 93 not loadable");
		return;
	}

	blade->extra_flags |= ITEM_NORENT;
	blade->bitvector = 0;
	blade->value[6] = GET_LEVEL(ch);

	/* how about some gay de procs for flame blade? Yeah baby! */
	if (GET_LEVEL(ch) >= 51)
	{
		blade->value[5] = 124;
		blade->value[7] = 30; // procs sunray
	}
	else if (GET_LEVEL(ch) >= 41)
	{
		blade->value[5] = 26;
		blade->value[7] = 25; // procs fireball, better chance.
	}
	else if (GET_LEVEL(ch) >= 36)
	{
		blade->value[5] = 26;
		blade->value[7] = 40; // procs fireball
	}
	else if (GET_LEVEL(ch) >= 21)
	{
		blade->value[5] = 195;
		blade->value[7] = 40; // procs flameburst
	}

	blade->timer[0] = 180;
	if (IS_PC(ch))
		blade->timer[1] = GET_PID(ch);
	else
		blade->timer[1] = -1;

	submit_conjured_weapon(ch, blade, conjured_weapon_kind::lancea_cineralae);
}

void spell_simulacrum_anguis(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
			     P_char /*victim*/, P_obj /*obj*/)
{
	P_obj blade;

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	blade = read_object(real_object(171), REAL);
	if (!blade)
	{
		logit(LOG_DEBUG, "spell_simulacrum_anguis(): obj 171 not loadable");
		return;
	}

	blade->extra_flags |= ITEM_NORENT;
	blade->bitvector = 0;
	blade->value[6] = GET_LEVEL(ch);

	/* how about some gay de procs for flame blade? Yeah baby! */
	if (GET_LEVEL(ch) >= 51)
	{
		blade->value[5] = 124;
		blade->value[7] = 30; // procs sunray
	}
	else if (GET_LEVEL(ch) >= 41)
	{
		blade->value[5] = 26;
		blade->value[7] = 25; // procs fireball, better chance.
	}
	else if (GET_LEVEL(ch) >= 36)
	{
		blade->value[5] = 26;
		blade->value[7] = 40; // procs fireball
	}
	else if (GET_LEVEL(ch) >= 21)
	{
		blade->value[5] = 195;
		blade->value[7] = 40; // procs flameburst
	}

	blade->timer[0] = 180;
	if (IS_PC(ch))
		blade->timer[1] = GET_PID(ch);
	else
		blade->timer[1] = -1;

	submit_conjured_weapon(ch, blade, conjured_weapon_kind::simulacrum_anguis);
}
