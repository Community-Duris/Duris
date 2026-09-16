/*
 *  kingdom_store_piece.h
 *  Duris
 *
 *  What the rest of the engine asks about guild-store gear: is this object a
 *  store piece, and which purchase did it come from.
 *
 *  Store gear is ordinary property (ruled 2026-09-16) -- given, looted and
 *  sold like anything else -- so none of this decides who may wear a piece.
 *  Knowing a piece is store gear decides one thing: that apply_ac() must not
 *  put a material armour-class floor under armour whose class is scaled to the
 *  level it was made at.
 *
 *  Both are INLINE on purpose. The files that ask -- cmd/actobj.c (wearing),
 *  magic/magic.c (remove_soulbind) and magic/affects.c (armour class) -- are
 *  each compiled into small test harnesses on their own, and a call into
 *  kingdom_craft.c would make every one of those need it linked or stubbed.
 *  Nothing here reaches beyond what those files already use.
 *
 *  kingdom.h stays the cheap, forward-declarations-only seam; this header is
 *  for engine files that already include the object and character structs.
 */

#ifndef _KINGDOM_STORE_PIECE_H_
#define _KINGDOM_STORE_PIECE_H_

#include "core/structs.h"
#include "core/utils.h"
#include "kingdom/kingdom_craft_bind.h"
#include "world/db.h"
#include "world/vnum.obj.h"

/* world/db.c. Like every engine file that reads the object index, this
 * declares it itself: no header exports it. */
extern P_index obj_index;

/* True for a piece the guild store made. Every one is read from the dedicated
 * blank VOBJ_KINGDOM_CRAFT_BLANK (#48018) and nothing else is, so the vnum
 * marks store gear exactly -- unlike ITEM2_STOREITEM or ITEM2_CRAFTED, which
 * other items carry too. */
inline bool kingdom_store_piece(const struct obj_data *obj)
{
	return obj && obj->R_num >= 0 &&
	       obj_index[obj->R_num].virtual_number == VOBJ_KINGDOM_CRAFT_BLANK;
}

/* True for gear the store's binding governs: a store piece by vnum, OR any
 * object carrying a binding token in its action description, where the store
 * writes it (kingdom_craft_bind.h). The second arm fails closed: a piece whose
 * object index were unresolved (R_num < 0) would fail the vnum test, and would
 * otherwise fall through to the legacy soulbind name test that a character
 * named "Kingdom" or "Steel" passes -- and would get the material armour-class
 * floor (magic/affects.c) its own scaled armour class is meant to replace. */
inline bool kingdom_store_bound(const struct obj_data *obj)
{
	return kingdom_store_piece(obj) ||
	       (obj && kingdom_craft_binding_present(obj->action_description));
}

/* True when `ch` is the character who bought store piece `obj`: a player
 * whose PLAYER ID is the one in the piece's binding token
 * (kingdom_craft_bind.h). Never a name test -- a store piece's keywords are
 * ordinary words a character could be named after -- and never true for a
 * mob, an unsaved character or anything the binding does not govern. */
inline bool kingdom_store_piece_owner(struct char_data *ch, const struct obj_data *obj)
{
	return ch && IS_PC(ch) && GET_PID(ch) > 0 && kingdom_store_bound(obj) &&
	       kingdom_craft_binding_is(obj->action_description, GET_PID(ch));
}

#endif /* _KINGDOM_STORE_PIECE_H_ */
