/**************************************************************
 *ship_base.c                                                 *
 *                                                            *
 *New Ship system blah blah blah bug foo about it :P          *
 *Updated with warships. Nov08 -Lucrot                        *
 **************************************************************/

/*
 * OVERVIEW -- where this file sits in the ship system
 * ---------------------------------------------------
 * The spine of the ship subsystem: it owns a ship's whole life, and it owns
 * the clock.  Every other ship_*.c file is called from here, directly or
 * indirectly.  Start here after ships.h and ship_utils.c.
 *
 * A ship's life
 * -------------
 *   new_ship()                allocate, apply class defaults, build the
 *                             ABSTRACT room graph, claim a runtime identity
 *   name_ship()               name it and rebuild every derived string
 *   load_ship()               claim real rooms, place the hull object and the
 *                             control panel, raise LOADED
 *   ... play ...              ship_activity() ticks it; ship_control.c steers
 *                             it; ship_combat.c shoots at it
 *   sink_ship()               (ship_combat.c) starts the sinking timer
 *   finish_sinking()          timer expired: player ships are downgraded to a
 *                             sloop and docked in Davy Jones' locker; NPC
 *                             ships are deleted outright
 *   delete_ship()             release the rooms, the objects and the ship
 *
 * Until LOADED is set a ship exists only as data -- most of the subsystem
 * skips it.  ShipData::runtime_ref is claimed in new_ship() and released in
 * delete_ship(); see ship_identity.c for why anything that must outlive a
 * ship stores that rather than a P_ship.
 *
 * Rooms: three separate ideas, do not confuse them
 * ------------------------------------------------
 *   ship->location  the OCEAN room the hull sits in, as a real room index
 *   ship->room[]    the ship's INTERIOR rooms, as vnums, plus the exits
 *                   between them.  Built in two stages:
 *                     set_ship_layout()          the abstract graph for the
 *                                                class (exits are slot
 *                                                indices, not vnums)
 *                     set_ship_physical_layout() claims real rooms out of the
 *                                                shared pool and wires them up
 *   ship->anchor    the dock the ship returns to
 *
 * Interior rooms come from a FIXED POOL (VROOM_SHIPS_START..VROOM_SHIPS_END).
 * A room is "in use" when its funct is ship_room_proc; find_free_ship_room()
 * scans for one that is not.  clear_ship_layout() is what returns them, so a
 * path that destroys a ship without calling it strands those rooms for the
 * lifetime of the process.
 *
 * The per-tick heartbeat
 * ----------------------
 * ship_activity() runs once per clock pulse over every loaded ship and is
 * where sailing actually happens -- timers, crew stamina, repairs, the
 * undocking sequence, convergence of speed and heading on their ordered
 * values, movement between ocean rooms, ramming, reloads, and finally the
 * autopilot and the NPC AI.  Read its own comment before changing it; the
 * `return` after finish_sinking() is load-bearing.
 *
 * Persistence
 * -----------
 * Do not write ships synchronously.  Call queue_ship_save(ship, reason) after
 * anything worth keeping; flush_pending_ship_saves() writes them at the end
 * of the tick, skipping ships whose ship_save_signature() has not changed and
 * retrying failures later.  write_ship() is the immediate path and is only
 * for callers that must know the write succeeded -- renames, which roll back
 * on failure.  drain_pending_ship_saves() is the copyover path: it ignores
 * the retry gate and reports whether everything is durable, so copyover can
 * be aborted rather than lose ship state.
 *
 * Two backends sit behind all of that, selected by __NO_MYSQL__: SQL tables,
 * or a validated flat-file record built by the static flat_ship_*() helpers
 * near the top of this file.  Everything crossing that boundary is
 * range-checked on the way in -- see flat_ship_slot_is_loadable() for why
 * slot indices in particular matter.
 *
 * A note on string ownership
 * --------------------------
 * ShipData's strings are str_dup()ed and privately owned, but the HULL
 * OBJECT's name and descriptions are shared across every object of that
 * prototype by read_object() (world/db.c).  That is why name_ship() drops
 * those pointers instead of freeing them.  Read the comment there before
 * "fixing" what looks like a leak.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "limits.h"
#include "core/utility.h"
#include "core/utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "combat/ctf.h"
#include "economy/currency_transaction.h"
#include "net/gmcp.h"
#include "world/graph.h"
#include "world/map.h"
#include "item/objmisc.h"
#include "ships/ship_auto.h"
#include "ships/ship_npc.h"
#include "ships/ship_npc_ai.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "sql/sql_player.h"
#include "redis/redis_ship_legacy.h"
#include "redis/redis_world_runtime.h"
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_identity_adapter.h"
#include "flatfile/flatfile_ship_repository.h"
#include "persistence/persistence_mode.h"

#include <cmath>
#include <limits>
#include <new>
#include <utility>
#endif

extern char buf[MAX_STRING_LENGTH];
extern bool insert_money_pickup(int pid, int money);
extern const char *rude_ass[];

struct ship_insurance_context
{
	char owner[MAX_NAME_LENGTH];
	int pid;
	int platinum;
};

/*
 * Completion callback for a ship-insurance bank deposit.
 *
 * `raw_context` carries a ship_insurance_context describing the payout; it is
 * copied out because the caller's buffer does not outlive this call.
 *
 * On a rejected transaction the money is staged at the auction house instead
 * so the player is never simply out of pocket, and if even that fails the
 * failure is escalated to the wizlog.  `owner` may be NULL if the player
 * logged out while the transaction was in flight.
 */
static void ship_insurance_committed(P_char owner, bool committed,
				     const currency_command_result & /*result*/,
				     unsigned int /*error_code*/, const uint8_t *raw_context,
				     size_t context_size)
{
	if (context_size != sizeof(ship_insurance_context))
		return;
	ship_insurance_context context = {};
	memcpy(&context, raw_context, sizeof(context));
	if (!committed)
	{
		logit(LOG_SHIP, "Ship insurance transaction rejected for account of %s: %d",
		      context.owner, context.platinum);
		if (!insert_money_pickup(context.pid, context.platinum * 1000))
		{
			logit(LOG_WIZ, "Failed to stage rejected ship insurance for pid %d",
			      context.pid);
			if (owner)
				send_to_char(
					"Your ship insurance deposit failed. Please contact staff.\r\n",
					owner);
		}
		else if (owner)
			send_to_char(
				"Your ship insurance is waiting at the auction house instead.\r\n",
				owner);
		return;
	}
	wizlog(56, "Ship insurance to account of %s: %d", context.owner, context.platinum);
	logit(LOG_SHIP, "Ship insurance deposit to account of %s: %d", context.owner,
	      context.platinum);
}

/*
 * Fold `len` bytes of `data` into `hash` (FNV-1a).  Building block for
 * ship_save_signature().
 */
static unsigned long long ship_save_signature_mix(unsigned long long hash, const void *data,
						  size_t len)
{
	const unsigned char *ptr = (const unsigned char *)data;
	for (size_t i = 0; i < len; i++)
	{
		hash ^= (unsigned long long)ptr[i];
		hash *= 1099511628211ULL;
	}
	return hash;
}

/*
 * Fingerprint of every persisted field of `ship`.
 *
 * Used to skip pointless writes: queue_ship_save() records the signature of
 * the last state actually written, and the flush compares against it, so a
 * ship that has not really changed does not hit the database.  Two ships that
 * would serialise identically hash identically; anything not persisted is
 * deliberately left out of the mix.
 *
 * Returns 0 for a NULL ship.
 */
unsigned long long ship_save_signature(const P_ship ship)
{
	unsigned long long hash = 1469598103934665603ULL;
	if (!ship)
		return 0;

#define SHIP_SIG_MIX(value) hash = ship_save_signature_mix(hash, &(value), sizeof(value))
#define SHIP_SIG_MIX_CSTR(value)                                                            \
	do                                                                                  \
	{                                                                                   \
		const char *sig_val__ = (value);                                            \
		if (sig_val__)                                                              \
			hash = ship_save_signature_mix(hash, sig_val__, strlen(sig_val__)); \
		else                                                                        \
		{                                                                           \
			const char sig_zero__ = '\0';                                       \
			hash = ship_save_signature_mix(hash, &sig_zero__, 1);               \
		}                                                                           \
	} while (0)

	SHIP_SIG_MIX(ship->m_class);
	SHIP_SIG_MIX_CSTR(ship->name);
	SHIP_SIG_MIX_CSTR(ship->ownername);
	SHIP_SIG_MIX(ship->frags);
	SHIP_SIG_MIX(ship->anchor);
	SHIP_SIG_MIX(ship->time);
	SHIP_SIG_MIX(ship->mainsail);
	SHIP_SIG_MIX(ship->race);
	SHIP_SIG_MIX(ship->money);
	SHIP_SIG_MIX(ship->flags);

	for (int i = 0; i < 4; i++)
	{
		SHIP_SIG_MIX(ship->maxarmor[i]);
		SHIP_SIG_MIX(ship->armor[i]);
		SHIP_SIG_MIX(ship->maxinternal[i]);
		SHIP_SIG_MIX(ship->internal[i]);
	}

	SHIP_SIG_MIX(ship->crew.index);
	SHIP_SIG_MIX(ship->crew.sail_skill);
	SHIP_SIG_MIX(ship->crew.guns_skill);
	SHIP_SIG_MIX(ship->crew.rpar_skill);
	SHIP_SIG_MIX(ship->crew.sail_chief);
	SHIP_SIG_MIX(ship->crew.guns_chief);
	SHIP_SIG_MIX(ship->crew.rpar_chief);

	for (int i = 0; i < MAXSLOTS; i++)
	{
		SHIP_SIG_MIX(ship->slot[i].type);
		SHIP_SIG_MIX(ship->slot[i].index);
		SHIP_SIG_MIX(ship->slot[i].position);
		SHIP_SIG_MIX(ship->slot[i].timer);
		SHIP_SIG_MIX(ship->slot[i].val0);
		SHIP_SIG_MIX(ship->slot[i].val1);
		SHIP_SIG_MIX(ship->slot[i].val2);
		SHIP_SIG_MIX(ship->slot[i].val3);
		SHIP_SIG_MIX(ship->slot[i].val4);
	}

#undef SHIP_SIG_MIX
#undef SHIP_SIG_MIX_CSTR
	return hash;
}

struct ContactData contacts[MAXSHIPS];
struct ShipMap tactical_map[101][101];
// char     status[20];
// char     position[20];
// char     contact[256];

char arg1[MAX_STRING_LENGTH];
char arg2[MAX_STRING_LENGTH];
char arg3[MAX_STRING_LENGTH];
char tmp_str[MAX_STRING_LENGTH];
int shiperror, davy_jones_locker_rnum, ship_transit_rnum;
struct ShipFragData shipfrags[20];

#ifdef __NO_MYSQL__
/*
 * Convert a crew skill to the fixed-point thousandths the flat-file record
 * stores.  Returns false (leaving `*milli` untouched) for a value that is not
 * finite or does not fit an int32, so a corrupt skill cannot be written out.
 */
static bool flat_ship_skill_milli(float value, int32_t *milli)
{
	const double scaled = static_cast<double>(value) * 1000.0;
	if (!milli || !std::isfinite(scaled) || scaled < std::numeric_limits<int32_t>::min() ||
	    scaled > std::numeric_limits<int32_t>::max())
		return false;
	*milli = static_cast<int32_t>(scaled);
	return true;
}

/*
 * Whether a persisted slot record can safely be loaded into a ShipSlot.
 *
 * This is the trust boundary for slot data: it checks the slot index, the
 * mounting position and -- crucially -- that the item index is in range for
 * the slot's OWN type, since ShipSlot::index subscripts a different table
 * (weapon_data, equipment_data, or the commodity names) depending on `type`.
 */
static bool flat_ship_slot_is_loadable(const flatfile_ship_slot_record &slot)
{
	if (slot.slot_index >= MAXSLOTS || slot.position < -1 || slot.position > SLOT_EQUI)
		return false;
	switch (slot.slot_type)
	{
	case SLOT_EMPTY:
		return slot.item_index == -1;
	case SLOT_WEAPON:
	case SLOT_AMMO:
		return slot.item_index >= 0 && slot.item_index < MAXWEAPON;
	case SLOT_CARGO:
	case SLOT_CONTRABAND:
		return slot.item_index >= 0 && slot.item_index < NUM_PORTS;
	case SLOT_EQUIPMENT:
		return slot.item_index >= 0 && slot.item_index < MAXEQUIPMENT;
	default:
		return false;
	}
}

/*
 * Resolve an owner name to a player id for records that predate pid storage.
 * Returns false when the name has no identity, leaving `*pid` untouched.
 */
static bool flat_ship_resolve_legacy_owner(const char *owner_name, uint32_t *pid,
					   std::string *error)
{
	int32_t resolved = 0;
	if (!pid || !flatfile_player_identity_pid(owner_name, &resolved, error) || resolved <= 0)
		return false;
	*pid = static_cast<uint32_t>(resolved);
	return true;
}

/*
 * Serialise a live ship into a flat-file record.
 *
 * Validates the runtime fields it is about to persist -- owner, class, race,
 * db id -- and refuses rather than writing a record that could not be loaded
 * back.  `error` receives a reason on failure.
 */
static bool flat_ship_capture(P_ship ship, flatfile_ship_record *record, std::string *error)
{
	if (!ship || !record || !ship->ownername || ship->db_id < -1 || ship->m_class < 0 ||
	    ship->m_class >= MAXSHIPCLASS || ship->race < std::numeric_limits<int8_t>::min() ||
	    ship->race > std::numeric_limits<int8_t>::max())
	{
		if (error)
			*error = "ship has invalid runtime fields";
		return false;
	}
	int32_t owner_pid = 0;
	if (!flatfile_player_identity_pid(ship->ownername, &owner_pid, error) || owner_pid <= 0)
	{
		if (error && error->empty())
			*error = "ship owner identity is unavailable";
		return false;
	}
	flatfile_ship_record captured = {};
	captured.ship_id = ship->db_id > 0 ? static_cast<uint32_t>(ship->db_id) : 0;
	captured.owner_pid = static_cast<uint32_t>(owner_pid);
	captured.owner_name = ship->ownername;
	captured.ship_name = ship->name ? ship->name : "";
	captured.ship_class = static_cast<uint8_t>(ship->m_class);
	captured.frags = ship->frags;
	captured.anchor_room = ship->anchor;
	captured.time_played = ship->time;
	captured.mainsail = ship->mainsail;
	captured.race = static_cast<int8_t>(ship->race);
	captured.money = ship->money;
	captured.flags = static_cast<uint64_t>(ship->flags);
	for (int index = 0; index < 4; ++index)
	{
		captured.armor[index] = ship->armor[index];
		captured.internal[index] = ship->internal[index];
	}
	captured.crew.crew_index = ship->crew.index;
	if (!flat_ship_skill_milli(ship->crew.sail_skill, &captured.crew.sail_skill_milli) ||
	    !flat_ship_skill_milli(ship->crew.guns_skill, &captured.crew.guns_skill_milli) ||
	    !flat_ship_skill_milli(ship->crew.rpar_skill, &captured.crew.repair_skill_milli))
	{
		if (error)
			*error = "ship has invalid crew skill";
		return false;
	}
	captured.crew.sail_chief = ship->crew.sail_chief;
	captured.crew.guns_chief = ship->crew.guns_chief;
	captured.crew.repair_chief = ship->crew.rpar_chief;
	try
	{
		captured.slots.resize(MAXSLOTS);
	}
	catch (const std::bad_alloc &)
	{
		if (error)
			*error = "could not allocate ship slots";
		return false;
	}
	for (int index = 0; index < MAXSLOTS; ++index)
	{
		auto &slot = captured.slots[index];
		slot.slot_index = static_cast<uint8_t>(index);
		slot.slot_type = ship->slot[index].type;
		slot.item_index = ship->slot[index].index;
		slot.position = ship->slot[index].position;
		slot.timer = ship->slot[index].timer;
		slot.values = { ship->slot[index].val0, ship->slot[index].val1,
				ship->slot[index].val2, ship->slot[index].val3,
				ship->slot[index].val4 };
		if (!flat_ship_slot_is_loadable(slot))
		{
			if (error)
				*error = "ship has invalid slot fields";
			return false;
		}
	}
	*record = std::move(captured);
	return true;
}

/*
 * Whether a flat-file ship record is safe to materialise.
 *
 * The load-side counterpart of flat_ship_capture()'s validation: every field
 * that will be used as an array subscript or a table index is range-checked
 * here, so a damaged file is rejected instead of corrupting the world.
 */
static bool flat_ship_record_is_loadable(const flatfile_ship_record &record, std::string *error)
{
	if (record.ship_id > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
	    record.owner_pid > static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
	    record.ship_class >= MAXSHIPCLASS || record.race == NPCSHIP ||
	    real_room(record.anchor_room) == NOWHERE || record.slots.size() > MAXSLOTS)
	{
		if (error)
			*error = "ship catalog contains fields unsupported by the runtime";
		return false;
	}
	for (const auto &slot : record.slots)
		if (!flat_ship_slot_is_loadable(slot))
		{
			if (error)
				*error = "ship catalog contains invalid slot fields";
			return false;
		}
	int32_t owner_pid = 0;
	if (!flatfile_player_identity_pid(record.owner_name.c_str(), &owner_pid, error) ||
	    owner_pid <= 0 || static_cast<uint32_t>(owner_pid) != record.owner_pid)
	{
		if (error && error->empty())
			*error = "ship owner identity does not match the catalog";
		return false;
	}
	return true;
}

/*
 * Build a live ship from a validated flat-file record and load it into the
 * world.
 *
 * Refuses records that fail flat_ship_record_is_loadable().  `error` receives
 * a reason on failure.  This is the flat-file half of read_ships().
 */
static bool flat_ship_materialize(const flatfile_ship_record &record, std::string *error)
{
	P_ship ship = new_ship(record.ship_class);
	if (!ship)
	{
		if (error)
			*error = "could not allocate ship runtime state";
		return false;
	}
	ship->db_id = static_cast<int>(record.ship_id);
	ship->ownername = str_dup(record.owner_name.c_str());
	CAP(ship->ownername);
	ship->frags = record.frags;
	ship->anchor = record.anchor_room;
	ship->time = record.time_played;
	ship->mainsail = record.mainsail;
	ship->race = record.race;
	ship->money = record.money;
	ship->flags = static_cast<ulong>(record.flags);
	for (int index = 0; index < 4; ++index)
	{
		ship->armor[index] = record.armor[index];
		ship->internal[index] = record.internal[index];
	}
	ship->crew.index = record.crew.crew_index;
	ship->crew.sail_skill = record.crew.sail_skill_milli / 1000.0f;
	ship->crew.guns_skill = record.crew.guns_skill_milli / 1000.0f;
	ship->crew.rpar_skill = record.crew.repair_skill_milli / 1000.0f;
	ship->crew.sail_chief = record.crew.sail_chief;
	ship->crew.guns_chief = record.crew.guns_chief;
	ship->crew.rpar_chief = record.crew.repair_chief;
	for (const auto &stored_slot : record.slots)
	{
		auto &slot = ship->slot[stored_slot.slot_index];
		slot.type = stored_slot.slot_type;
		slot.index = stored_slot.item_index;
		slot.position = stored_slot.position;
		slot.timer = stored_slot.timer;
		slot.val0 = stored_slot.values[0];
		slot.val1 = stored_slot.values[1];
		slot.val2 = stored_slot.values[2];
		slot.val3 = stored_slot.values[3];
		slot.val4 = stored_slot.values[4];
	}
	name_ship(record.ship_name.c_str(), ship);
	if (!load_ship(ship, real_room(record.anchor_room)))
	{
		shipObjHash.erase(ship);
		delete_ship(ship, true);
		if (error)
			*error = "could not place ship in its anchor room";
		return false;
	}
	ship->mainsail = BOUNDED(0, ship->mainsail, SHIP_MAX_SAIL(ship));
	update_crew(ship);
	reset_crew_stamina(ship);
	set_ship_armor(ship, false);
	update_ship_status(ship);
	ship->save_pending = false;
	ship->save_retry_after = 0;
	ship->save_saved_signature = ship_save_signature(ship);
	return true;
}
#endif

/*
 * Boot the whole ship subsystem.  Called once from the world loader.
 *
 * Wires the ship special procedures onto their prototype objects, resolves
 * the two fixed rooms the system needs (Davy Jones' locker for wrecks and
 * the transit room), loads every persisted ship, reaps the ones flagged
 * TO_DELETE, and then brings up the cargo market, Cyric's Revenge and the
 * automatons quest.
 *
 * A failed ship load is fatal under the flat-file backend, where the ship
 * file is the sole authority; under MySQL it is logged and boot continues.
 */
void initialize_ships()
{
	obj_index[real_object0(VOBJ_PANEL)].func.obj = ship_panel_proc;
	obj_index[real_object0(VOBJ_ALL_SHIPS)].func.obj = ship_obj_proc;
	obj_index[real_object0(1223)].func.obj = ship_cargo_info_stick;
	davy_jones_locker_rnum = real_room0(VROOM_DAVY_JONES);
	ship_transit_rnum = real_room0(VROOM_SHIP_TRANSIT);

	if (!read_ships())
	{
#ifdef __NO_MYSQL__
		fatal_boot_error("ship_base", "flat ship authority could not be loaded");
#else
		logit(LOG_FILE, "Error reading ships from file!\r\n");
#endif
	}

	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn;)
	{
		P_ship ship = svs;
		if (IS_SET(ship->flags, TO_DELETE))
		{
			fn = shipObjHash.erase(svs);
			delete_ship(ship);
		}
		else
			fn = shipObjHash.get_next(svs);
	}

	initialize_ship_cargo();
	load_cyrics_revenge();
	// load_zone_ship();
	if (!redis_world_recovery_boot_active() && !load_moonstone_fragments())
	{
		logit(LOG_FILE, "Error initializing automatons quest!\r\n");
	}
}

/*
 * Bring every ship home before the server stops.
 *
 * Puts each ship's passengers ashore and docks the hull, so nobody comes
 * back to find themselves in a ship room that no longer exists, then
 * persists every ship in one batched transaction.  Called during an orderly
 * shutdown; see drain_pending_ship_saves() for the copyover path, which has
 * different durability requirements.
 */
void shutdown_ships()
{
	int i;
	P_char ch, ch_next;
	P_obj obj, obj_next;

#ifndef __NO_MYSQL__
	int batchSize = 1024 * 1024 * 10;
	char *batch = (char *)malloc(batchSize);
	if (!batch)
	{
		fatal_boot_error("ship_base", "shutdown_ships: could not allocate batch buffer");
	}

	// do this update as a transaction
	if (!sql_begin_transaction())
	{
		logit(LOG_DEBUG, "shutdown_ships: start transaction failed");
		free(batch);
		return;
	}
#endif

	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		P_ship ship = svs;
		for (i = 0; i < ship->room_count; i++)
		{
			for (ch = world[real_room(SHIP_ROOM_NUM(ship, i))].people; ch; ch = ch_next)
			{
				if (ch)
				{
					ch_next = ch->next_in_room;
					char_from_room(ch);
					char_to_room(ch, real_room0(ship->anchor), -2);
				}
			}
			for (obj = world[real_room(ship->room[i].roomnum)].contents; obj;
			     obj = obj_next)
			{
				if (obj)
				{
					obj_next = obj->next_content;
					if (OBJ_ROOM(obj))
					{
						obj_from_room(obj);
						obj_to_room(obj, real_room0(ship->anchor));
					}
				}
			}
		}
		if (!write_ship(ship) && !IS_NPC_SHIP(ship) && SHIP_LOADED(ship))
		{
#ifndef __NO_MYSQL__
			if (sql_rollback())
				logit(LOG_DEBUG,
				      "shutdown_ships: rolled back after write_ship failed");
			panic_corruption("shutdown_ships", "write_ship failed after rollback");
#else
			panic_corruption("shutdown_ships", "flat write_ship failed");
#endif
		}
	}

#ifndef __NO_MYSQL__
	if (!sql_commit())
	{
		logit(LOG_DEBUG, "shutdown_ships: commit failed");
		if (sql_rollback())
			logit(LOG_DEBUG, "shutdown_ships: rolled back after commit failure");
		panic_corruption("shutdown_ships", "commit failed after rollback");
	}
	free(batch);
#endif
}

/*
 * Allocate a ship of hull class `m_class` and register it.
 *
 * Sets every field to its class default -- full armour and sail, empty
 * slots, default crew, no chiefs, the docked placeholder designation "**",
 * centred on the tactical map -- builds the abstract room layout, and claims
 * a process-local runtime identity.  The ship is added to shipObjHash but is
 * NOT yet loaded into the world; call load_ship() for that.
 *
 * The caller still has to set ownername and call name_ship().
 *
 * Returns NULL with a code in the global `shiperror` when the game is at
 * MAXSHIPS or a prototype object is missing.
 *
 * KNOWN CAVEAT: the two prototype-object failure paths return without
 * freeing the part-built ship, and the second leaves it in shipObjHash.
 * Both only fire if the ship or panel prototype is missing from the object
 * database, which is a broken-world condition rather than a runtime one.
 */
struct ShipData *new_ship(int m_class, bool /*npc*/)
{
	if (shipObjHash.size() >= MAXSHIPS)
	{
		shiperror = 6;
		return NULL; // AT MAX FOR GAME
	}

	// Make a new ship
	P_ship ship = NULL;
	CREATE(ship, ShipData, 1, MEM_TAG_SHIPDAT);

	ship->db_id = -1;
	ship->shipobj = read_object(VOBJ_ALL_SHIPS, VIRTUAL);
	if (ship->shipobj == NULL)
	{
		shiperror = 16;
		return NULL;
	}
	shipObjHash.add(ship);

	// Set up the new ship
	ship->autopilot = 0;
	ship->npc_ai = 0;
	ship->panel = read_object(VOBJ_PANEL, VIRTUAL);
	if (ship->panel == NULL)
	{
		shiperror = 17;
		return NULL;
	}
	ship->m_class = m_class;
	set_ship_armor(ship, true);
	ship->frags = 0;
	ship->name = NULL;
	ship->x = 50;
	ship->y = 50;
	ship->z = 0;
	ship->flags = 0;
	ship->money = 0;
	ship->target = 0;
	ship->mainsail = SHIPTYPE_MAX_SAIL(m_class);
	ship->repair = SHIP_HULL_WEIGHT(ship);
	ship->setheading = ship->heading = 0;
	ship->setspeed = ship->speed = 0;

	ship->maxspeed_bonus = 0;
	ship->capacity_bonus = 0;

	ship->time = time(NULL);
	ship->contacts_hash = 0;
	ship->last_gmcp_location = -1;
	ship->save_retry_after = 0;
	ship->save_pending = false;
	ship->save_saved_signature = 0;

	for (int j = 0; j < MAXSLOTS; j++)
	{
		ship->slot[j].clear();
	}

	set_crew(ship, DEFAULT_CREW);
	ship->crew.sail_chief = NO_CHIEF;
	ship->crew.guns_chief = NO_CHIEF;
	ship->crew.rpar_chief = NO_CHIEF;

	assignid(ship, "**");
	ship->keywords = str_dup("ship");

	init_ship_layout(ship);
	set_ship_layout(ship, m_class);
	register_ship_runtime(ship);
	return ship;
}

/*
 * Give `ship` a name and rebuild every string derived from it: the hull
 * object's keywords, short description and room description, and the ship's
 * own keyword string.
 *
 * NOTE ON THE NULL-WITHOUT-FREE PATTERN BELOW.  The three shipobj strings are
 * NOT owned by this object -- read_object() (world/db.c) deliberately shares
 * one copy of each string across every object of a prototype, so freeing
 * them here would corrupt every other ship object in the game.  Dropping the
 * pointer is the correct thing to do.  ship->name and ship->keywords are
 * privately str_dup()ed and could in principle be freed; they are not, so a
 * rename leaks those two.  Renames are rare, and the uniform pattern is left
 * alone rather than made subtly inconsistent.
 */
void name_ship(const char *name, P_ship ship)
{
	if (ship->name != NULL)
		ship->name = NULL;

	if (ship->shipobj->description != NULL)
		ship->shipobj->description = NULL;

	if (ship->shipobj->short_description != NULL)
		ship->shipobj->short_description = NULL;

	if (ship->shipobj->name != NULL)
		ship->shipobj->name = NULL;

	if (ship->keywords != NULL)
		ship->keywords = NULL;

	ship->name = str_dup(name);
	sprintf(buf, "&+yThe %s&N %s&n &+yfloats here.", SHIP_CLASS_NAME(ship), name);
	ship->shipobj->description = str_dup(buf);
	sprintf(buf, "&+yThe %s&N %s&n", SHIP_CLASS_NAME(ship), name);
	ship->shipobj->short_description = str_dup(buf);
	sprintf(buf, "ship %s %s", SHIP_CLASS_NAME(ship), ship->ownername);
	ship->shipobj->name = str_dup(buf);
	ship->keywords = str_dup(buf);
}

/*
 * Rewrite the room titles of every room inside `ship` to include its name.
 *
 * Room 0 is always the bridge; the rest are "Aboard the ...", except that
 * warship classes rename specific room indices to launch decks, docking bays
 * and holds.  Those index lists are per class and hard-coded -- they must
 * match the room graph that set_ship_layout() builds for the same class.
 *
 * Sets `shiperror` and returns early if a room vnum does not resolve.
 *
 * NOTE: the old room title is dropped without being freed.  That is
 * deliberate -- world[].name may point at a shared string owned by the world
 * file, and freeing it would corrupt other rooms.
 */
void name_ship_rooms(P_ship ship)
{
	for (int i = 0; i < ship->room_count; i++)
	{
		if (SHIP_ROOM_NUM(ship, i) == -1)
			continue;

		int rroom = real_room0(SHIP_ROOM_NUM(ship, i));
		if (!rroom)
		{
			shiperror = 4;
			return;
		}

		if (i == 0)
		{
			sprintf(buf, "&+ROn the &+WBridge&N&+R of the &+L%s&N %s&N",
				SHIP_CLASS_NAME(ship), ship->name);
		}
		else
		{
			sprintf(buf, "&+yAboard the %s&N %s", SHIP_CLASS_NAME(ship), ship->name);
		}
		if (ship->m_class == SH_CORVETTE)
		{
			if (i == 1 || i == 3 || i == 4)
			{
				sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
		}
		if (ship->m_class == SH_DESTROYER)
		{
			if (i == 1 || i == 2 || i == 3)
			{
				sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
		}
		if (ship->m_class == SH_FRIGATE)
		{
			if (i == 1 || i == 2 || i == 3)
			{
				sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
		}
		if (ship->m_class == SH_CRUISER)
		{
			if (i == 9)
			{
				sprintf(buf, "&+YDocking Bay&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
			if (i == 7 || i == 10 || i == 11)
			{
				sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
		}
		else if (ship->m_class == SH_DREADNOUGHT)
		{
			if (i == 14)
			{
				sprintf(buf, "&+YDocking Bay&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
			if (i == 7)
			{
				sprintf(buf, "&+YSpacious Hold&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
			if (i == 3 || i == 8 || i == 9 || i == 10)
			{
				sprintf(buf, "&+BLaunch Deck&+y of the &+L%s&N %s",
					SHIP_CLASS_NAME(ship), ship->name);
			}
		}

		if (world[rroom].name)
		{
			world[rroom].name = NULL;
		}

		world[rroom].name = str_dup(buf);
	}
}

/*
 * Rename the ship owned by `owner_name` to `new_name`, on `ch`'s authority.
 *
 * Validates the new name through check_ship_name(), updates the ship, its
 * hull object's descriptions and all its room titles, then persists.  Returns
 * FALSE (having explained to `ch`) when there is no such ship, the name is
 * rejected, or the save fails.
 *
 * NOTE: a failed save leaves the ship renamed in memory but not on disk;
 * unlike rename_ship_owner() below, this path has no rollback.
 */
bool rename_ship(P_char ch, char *owner_name, char *new_name)
{
	P_ship temp;

	temp = get_ship_from_owner(owner_name);
	if (!temp)
	{
		if (isname(GET_NAME(ch), owner_name))
		{
			send_to_char("You do not own a ship yet, buy one first!\n", ch);
		}
		else
		{
			send_to_char_f(ch, "%s does not own a ship!\n", owner_name);
		}

		return FALSE;
	}

	if (!check_ship_name(temp, ch, new_name))
		return FALSE;

	name_ship(new_name, temp);
	name_ship_rooms(temp);
	if (!write_ship(temp))
	{
		logit(LOG_DEBUG, "Failed to save renamed ship %s for owner %s.", SHIP_NAME(temp),
		      owner_name);
		return FALSE;
	}

	return TRUE;
}

/*
 * Mark `ship` as needing to be written out, to be flushed later in the tick.
 *
 * Call this after ANY change worth persisting rather than writing
 * immediately: repeated calls in one tick collapse into a single write, and
 * the flush additionally skips ships whose ship_save_signature() has not
 * actually changed.  `reason` is for the debug log and may be NULL.
 *
 * NPC ships and unloaded ships are never persisted and are ignored here.
 */
void queue_ship_save(P_ship ship, const char *reason)
{
	if (!ship || IS_NPC_SHIP(ship) || !SHIP_LOADED(ship))
		return;

	if (!ship->save_pending)
	{
		ship->save_pending = true;
		logit(LOG_DEBUG, "Queued ship save for %s%s%s.", SHIP_NAME(ship),
		      reason ? " after " : "", reason ? reason : "");
	}
	else if (reason)
	{
		logit(LOG_DEBUG, "Refreshed pending ship save for %s after %s.", SHIP_NAME(ship),
		      reason);
	}

	ship->save_retry_after = 0;
}

/*
 * Transfer the ship owned by `old_name` to `new_name` -- used when a player
 * is renamed.
 *
 * Fully transactional in memory: the previous owner and ship name are copied
 * aside first, and if the save fails both are restored before returning
 * FALSE.  On success the Redis snapshot for the old name is invalidated and
 * the legacy per-owner ship file is unlinked.
 *
 * Returns FALSE when there is no such ship, `new_name` is empty, or the save
 * failed (with the ship left exactly as it was).
 */
bool rename_ship_owner(char *old_name, char *new_name)
{
	P_ship ship;
	char *old_ownername;
	char *old_ship_name;

	ship = get_ship_from_owner(old_name);
	if (!ship || !*new_name)
		return FALSE;

	old_ownername = str_dup(ship->ownername ? ship->ownername : "");
	old_ship_name = str_dup(SHIP_NAME(ship) ? SHIP_NAME(ship) : "");
	if (!old_ownername || !old_ship_name)
	{
		if (old_ownername)
			FREE(old_ownername);
		if (old_ship_name)
			FREE(old_ship_name);
		return FALSE;
	}

	CAP(new_name);
	str_free(ship->ownername);
	ship->ownername = str_dup(new_name);
	name_ship(SHIP_NAME(ship), ship);

	if (!write_ship(ship))
	{
		logit(LOG_DEBUG, "Failed to save re-owned ship %s for %s.", SHIP_NAME(ship),
		      old_name);
		str_free(ship->ownername);
		ship->ownername = old_ownername;
		name_ship(old_ship_name, ship);
		FREE(old_ship_name);
		return FALSE;
	}

	FREE(old_ownername);
	FREE(old_ship_name);

	redis_invalidate_ship_snapshot(old_name);

	sprintf(buf, "Ships/%s", old_name);
	unlink(buf);

	return TRUE;
}

/*
 * Bring `ship` into the world at `to_room` (a real room index).
 *
 * Carves out the ship's interior rooms, places the hull object and the
 * control panel, and raises the LOADED flag -- until that flag is set the
 * ship exists only as data and most of the ship system skips it.
 *
 * Returns FALSE, with a code in the global `shiperror`, if the hull object
 * or panel is missing or the room pool could not satisfy the layout.
 */
int load_ship(P_ship ship, int to_room)
{
	if (ship->shipobj == NULL)
	{
		shiperror = 2;
		return FALSE;
	}

	if (ship->panel == NULL)
	{
		shiperror = 3;
		return FALSE;
	}

	if (!set_ship_physical_layout(ship))
	{
		shiperror = 4;
		return FALSE;
	}

	/*for (int i = 0; i < MAX_SHIP_ROOM; i++)
	{
	  if (SHIP_ROOM_NUM(ship, i) != -1)
	  {
	     int rroom = real_room0(SHIP_ROOM_NUM(ship, i));
	     if (!rroom)
	     {
	        shiperror = 4;
	        return FALSE;
	     }

	     world[rroom].funct = ship_room_proc;
	     for (int dir = 0; dir < NUM_EXITS; dir++)
	     {
	        if (SHIP_ROOM_EXIT(ship, i, dir) != -1)
	        {
	           if (!world[rroom].dir_option[dir])
	              CREATE(world[rroom].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

	           world[rroom].dir_option[dir]->to_room = real_room0(SHIP_ROOM_EXIT(ship, i, dir));
	           world[rroom].dir_option[dir]->exit_info = 0;
	        }
	        else
	        {
	           if (world[rroom].dir_option[dir])
	              world[rroom].dir_option[dir]->to_room = -1;
	        }
	     }
	  }
   }*/

	if (IS_SET(ship->flags, SINKING))
		REMOVE_BIT(ship->flags, SINKING);
	if (IS_SET(ship->flags, FLYING))
		REMOVE_BIT(ship->flags, FLYING);
	if (IS_SET(ship->flags, SUNKBYNPC))
		REMOVE_BIT(ship->flags, SUNKBYNPC);
	if (IS_SET(ship->flags, ATTACKBYNPC))
		REMOVE_BIT(ship->flags, ATTACKBYNPC);
	if (IS_SET(ship->flags, RAMMING))
		REMOVE_BIT(ship->flags, RAMMING);
	SET_BIT(ship->flags, DOCKED);
	SET_BIT(ship->flags, LOADED);

	if (ship->panel != NULL)
	{
		obj_to_room(ship->panel, real_room0(ship->bridge));
	}
	else
	{
		shiperror = 23;
		return FALSE;
	}
	if (SHIP_OBJ(ship))
	{
		ship->z = 0;
		SHIP_OBJ(ship)->value[6] = 1;
		obj_to_room(SHIP_OBJ(ship), to_room);
	}
	else
	{
		shiperror = 5;
		return FALSE;
	}
	ship->anchor = world[to_room].number;
	ship->location = to_room;

	update_crew(ship);
	reset_crew_stamina(ship);
	update_ship_status(ship);
	return TRUE;
}

/*
 * Destroy `ship` and free it.
 *
 * CALLER CONTRACT: the ship must already have been removed from
 * shipObjHash -- this routine does not do it, and re-adds the ship on the
 * error paths below on the assumption that it is currently out.  After this
 * returns, every P_ship pointing at it dangles; anything that needed to
 * outlive the ship should have been holding a ShipRuntimeRef instead (see
 * ship_identity.c).
 *
 * `npc` true skips the persistent delete, since NPC ships are never saved.
 * For a player ship, a failed database delete ABORTS the removal -- the ship
 * is put back in the hash and left alive, rather than being destroyed in
 * memory while its row survives on disk.
 *
 * KNOWN LEAK: ship->name, ->ownername, ->id and ->keywords are str_dup()ed
 * and are not released here.  Left alone deliberately; see the note in
 * name_ship() about which ship strings are and are not privately owned.
 */
void delete_ship(P_ship ship, bool npc)
{
	if (!npc)
	{
#ifndef __NO_MYSQL__
		if (!sql_delete_ship(ship->ownername))
		{
			logit(LOG_DEBUG, "Failed to delete ship row for %s; aborting ship removal.",
			      ship->ownername);
			shipObjHash.add(ship);
			return;
		}
#else
		if (ship->db_id > 0)
		{
			const char *root = persistence_mode_flatfile_root();
			std::string error;
			const auto result = root ? flatfile_ship_remove(
							   root, static_cast<uint32_t>(ship->db_id),
							   ship->ownername, &error) :
						   flatfile_ship_result::invalid;
			if (result != flatfile_ship_result::ok &&
			    result != flatfile_ship_result::unchanged)
			{
				logit(LOG_DEBUG,
				      "Failed to delete flat ship for %s; aborting ship removal: %s",
				      ship->ownername,
				      error.empty() ? "authority failure" : error.c_str());
				shipObjHash.add(ship);
				return;
			}
		}
#endif
	}

	unregister_ship_runtime(ship);
	clear_ship_layout(ship);
	clear_references_to_ship(ship);
	/*
	 * The autopilot block is a separate allocation hanging off the ship, so
	 * it has to be released before the ship itself goes; nothing else holds
	 * a pointer to it.
	 */
	clear_autopilot(ship);

	obj_from_room(ship->panel);
	obj_from_room(ship->shipobj);
	extract_obj(ship->panel, TRUE);
	extract_obj(ship->shipobj, TRUE);
	ship->panel = NULL;
	ship->shipobj = NULL;
	if (ship->npc_ai)
		delete ship->npc_ai;
	if (ship == cyrics_revenge)
		cyrics_revenge = 0;

	logit(LOG_STATUS, "Ship \"%s\" (%s) deleted", strip_ansi(ship->name).c_str(),
	      ship->ownername);

	FREE(ship);
}

/*
 * Drop every other ship's target lock on `ship`, telling those crews their
 * target is lost.
 *
 * Must be called before a ship is deleted or docked -- ShipData::target is a
 * raw pointer, and this is what stops it dangling.  See also
 * ship_identity.c, which solves the same problem for state that has to
 * survive a delay.
 */
void clear_references_to_ship(P_ship ship)
{
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		if (svs->target == ship)
		{
			svs->target = NULL;
			act_to_all_in_ship(svs, "&+RTarget lost.\r\n");
		}
	}
}

/*
 * Fill in the ABSTRACT room graph for hull class `m_class`.
 *
 * Every ship class has a fixed interior shape, defined here as a table of
 * exits between room slots 0..room_count-1.  Room 0 is always the bridge;
 * ship->entrance is the slot players board and leave through.  The numbers
 * stored in the exits are slot indices, not room vnums -- real rooms are
 * only assigned later, by set_ship_physical_layout().
 *
 * These per-class shapes must stay in step with the per-class room-title
 * lists in name_ship_rooms(); the two are indexed the same way.
 */
void set_ship_layout(P_ship ship, int m_class)
{
	//    int to_room = 0;

	ship->bridge = 0;
	switch (m_class)
	{
	case SH_SLOOP:
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 1;
		SHIP_ROOM_EXIT(ship, 1, DIR_NORTH) = 0;
		ship->entrance = 1;
		ship->room_count = 2;
		break;

	case SH_YACHT:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_NORTH) = 0;
		ship->entrance = 2;
		ship->room_count = 3;
		break;

	case SH_CLIPPER:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_SOUTH) = 3;
		SHIP_ROOM_EXIT(ship, 3, DIR_NORTH) = 2;
		ship->entrance = 3;
		ship->room_count = 4;
		break;

	case SH_KETCH:
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_EAST) = 2;
		SHIP_ROOM_EXIT(ship, 2, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 2, DIR_WEST) = 1;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 2;
		ship->entrance = 2;
		ship->room_count = 4;
		break;

	case SH_CARAVEL:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 5;
		SHIP_ROOM_EXIT(ship, 0, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 0, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_SOUTH) = 4;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 0;
		SHIP_ROOM_EXIT(ship, 3, DIR_SOUTH) = 6;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 0;
		SHIP_ROOM_EXIT(ship, 4, DIR_NORTH) = 2;
		SHIP_ROOM_EXIT(ship, 4, DIR_EAST) = 5;
		SHIP_ROOM_EXIT(ship, 5, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 5, DIR_EAST) = 6;
		SHIP_ROOM_EXIT(ship, 5, DIR_WEST) = 4;
		SHIP_ROOM_EXIT(ship, 6, DIR_NORTH) = 3;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 5;
		ship->entrance = 5;
		ship->room_count = 7;
		break;

	case SH_CARRACK:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 5;
		SHIP_ROOM_EXIT(ship, 0, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 0, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_SOUTH) = 4;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 0;
		SHIP_ROOM_EXIT(ship, 3, DIR_SOUTH) = 6;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 0;
		SHIP_ROOM_EXIT(ship, 4, DIR_NORTH) = 2;
		SHIP_ROOM_EXIT(ship, 4, DIR_EAST) = 5;
		SHIP_ROOM_EXIT(ship, 5, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 5, DIR_SOUTH) = 7;
		SHIP_ROOM_EXIT(ship, 5, DIR_EAST) = 6;
		SHIP_ROOM_EXIT(ship, 5, DIR_WEST) = 4;
		SHIP_ROOM_EXIT(ship, 6, DIR_NORTH) = 3;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 5;
		SHIP_ROOM_EXIT(ship, 7, DIR_NORTH) = 5;
		ship->entrance = 7;
		ship->room_count = 8;
		break;

	case SH_GALLEON:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 5;
		SHIP_ROOM_EXIT(ship, 0, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 0, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_SOUTH) = 4;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 0;
		SHIP_ROOM_EXIT(ship, 3, DIR_SOUTH) = 6;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 0;
		SHIP_ROOM_EXIT(ship, 4, DIR_NORTH) = 2;
		SHIP_ROOM_EXIT(ship, 4, DIR_SOUTH) = 7;
		SHIP_ROOM_EXIT(ship, 4, DIR_EAST) = 5;
		SHIP_ROOM_EXIT(ship, 5, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 5, DIR_SOUTH) = 8;
		SHIP_ROOM_EXIT(ship, 5, DIR_EAST) = 6;
		SHIP_ROOM_EXIT(ship, 5, DIR_WEST) = 4;
		SHIP_ROOM_EXIT(ship, 6, DIR_NORTH) = 3;
		SHIP_ROOM_EXIT(ship, 6, DIR_SOUTH) = 9;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 5;
		SHIP_ROOM_EXIT(ship, 7, DIR_NORTH) = 4;
		SHIP_ROOM_EXIT(ship, 7, DIR_EAST) = 8;
		SHIP_ROOM_EXIT(ship, 8, DIR_NORTH) = 5;
		SHIP_ROOM_EXIT(ship, 8, DIR_EAST) = 9;
		SHIP_ROOM_EXIT(ship, 8, DIR_WEST) = 7;
		SHIP_ROOM_EXIT(ship, 9, DIR_NORTH) = 6;
		SHIP_ROOM_EXIT(ship, 9, DIR_WEST) = 8;
		ship->entrance = 8;
		ship->room_count = 10;
		break;

	case SH_CORVETTE:
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 2;
		SHIP_ROOM_EXIT(ship, 0, DIR_WEST) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 4;
		SHIP_ROOM_EXIT(ship, 0, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 1, DIR_EAST) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 0;
		SHIP_ROOM_EXIT(ship, 4, DIR_SOUTH) = 0;
		ship->entrance = 2;
		ship->room_count = 5;
		break;

	case SH_DESTROYER:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 7;
		SHIP_ROOM_EXIT(ship, 0, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 0, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 0;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 0;
		SHIP_ROOM_EXIT(ship, 4, DIR_EAST) = 5;
		SHIP_ROOM_EXIT(ship, 5, DIR_NORTH) = 7;
		SHIP_ROOM_EXIT(ship, 5, DIR_EAST) = 6;
		SHIP_ROOM_EXIT(ship, 5, DIR_WEST) = 4;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 5;
		SHIP_ROOM_EXIT(ship, 7, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 7, DIR_SOUTH) = 5;
		ship->entrance = 5;
		ship->room_count = 8;
		break;

	case SH_FRIGATE:
		SHIP_ROOM_EXIT(ship, 0, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 8;
		SHIP_ROOM_EXIT(ship, 0, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 0, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 0;
		SHIP_ROOM_EXIT(ship, 2, DIR_SOUTH) = 4;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 0;
		SHIP_ROOM_EXIT(ship, 3, DIR_SOUTH) = 6;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 0;
		SHIP_ROOM_EXIT(ship, 4, DIR_NORTH) = 2;
		SHIP_ROOM_EXIT(ship, 4, DIR_EAST) = 5;
		SHIP_ROOM_EXIT(ship, 5, DIR_NORTH) = 8;
		SHIP_ROOM_EXIT(ship, 5, DIR_SOUTH) = 7;
		SHIP_ROOM_EXIT(ship, 5, DIR_EAST) = 6;
		SHIP_ROOM_EXIT(ship, 5, DIR_WEST) = 4;
		SHIP_ROOM_EXIT(ship, 6, DIR_NORTH) = 3;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 5;
		SHIP_ROOM_EXIT(ship, 7, DIR_NORTH) = 5;
		SHIP_ROOM_EXIT(ship, 8, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 8, DIR_SOUTH) = 5;
		ship->entrance = 7;
		ship->room_count = 9;
		break;

	case SH_CRUISER:
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 1;
		SHIP_ROOM_EXIT(ship, 1, DIR_NORTH) = 0;
		SHIP_ROOM_EXIT(ship, 1, DIR_DOWN) = 3;
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 3;
		SHIP_ROOM_EXIT(ship, 3, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 3, DIR_EAST) = 4;
		SHIP_ROOM_EXIT(ship, 3, DIR_NORTH) = 5;
		SHIP_ROOM_EXIT(ship, 3, DIR_UP) = 1;
		SHIP_ROOM_EXIT(ship, 4, DIR_WEST) = 3;
		SHIP_ROOM_EXIT(ship, 5, DIR_NORTH) = 6;
		SHIP_ROOM_EXIT(ship, 5, DIR_SOUTH) = 3;
		SHIP_ROOM_EXIT(ship, 5, DIR_DOWN) = 8;
		SHIP_ROOM_EXIT(ship, 6, DIR_NORTH) = 7;
		SHIP_ROOM_EXIT(ship, 6, DIR_SOUTH) = 5;
		SHIP_ROOM_EXIT(ship, 7, DIR_SOUTH) = 6;
		SHIP_ROOM_EXIT(ship, 8, DIR_NORTH) = 9;
		SHIP_ROOM_EXIT(ship, 8, DIR_UP) = 5;
		SHIP_ROOM_EXIT(ship, 9, DIR_SOUTH) = 8;
		SHIP_ROOM_EXIT(ship, 6, DIR_EAST) = 10; // laungh
		SHIP_ROOM_EXIT(ship, 10, DIR_WEST) = 6;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 11; // laungh
		SHIP_ROOM_EXIT(ship, 11, DIR_EAST) = 6;
		ship->entrance = 9;
		ship->room_count = 12;
		break;

	case SH_DREADNOUGHT:
		SHIP_ROOM_EXIT(ship, 0, DIR_DOWN) = 6;
		SHIP_ROOM_EXIT(ship, 1, DIR_SOUTH) = 2;
		SHIP_ROOM_EXIT(ship, 1, DIR_NORTH) = 3; // launch
		SHIP_ROOM_EXIT(ship, 1, DIR_WEST) = 4;
		SHIP_ROOM_EXIT(ship, 1, DIR_EAST) = 5;
		SHIP_ROOM_EXIT(ship, 1, DIR_DOWN) = 11;
		SHIP_ROOM_EXIT(ship, 2, DIR_NORTH) = 1;
		SHIP_ROOM_EXIT(ship, 2, DIR_SOUTH) = 6;
		SHIP_ROOM_EXIT(ship, 2, DIR_DOWN) = 7; // hold
		SHIP_ROOM_EXIT(ship, 2, DIR_WEST) = 8; // launch
		SHIP_ROOM_EXIT(ship, 2, DIR_EAST) = 9; // launch
		SHIP_ROOM_EXIT(ship, 6, DIR_UP) = 0;
		SHIP_ROOM_EXIT(ship, 6, DIR_NORTH) = 2;
		SHIP_ROOM_EXIT(ship, 6, DIR_SOUTH) = 10;
		SHIP_ROOM_EXIT(ship, 6, DIR_WEST) = 12;
		SHIP_ROOM_EXIT(ship, 6, DIR_EAST) = 13;
		SHIP_ROOM_EXIT(ship, 11, DIR_UP) = 1;
		SHIP_ROOM_EXIT(ship, 11, DIR_NORTH) = 14;
		SHIP_ROOM_EXIT(ship, 3, DIR_SOUTH) = 1;
		SHIP_ROOM_EXIT(ship, 4, DIR_EAST) = 1;
		SHIP_ROOM_EXIT(ship, 4, DIR_SOUTH) = 8;
		SHIP_ROOM_EXIT(ship, 5, DIR_WEST) = 1;
		SHIP_ROOM_EXIT(ship, 5, DIR_SOUTH) = 9;
		SHIP_ROOM_EXIT(ship, 7, DIR_UP) = 2;
		SHIP_ROOM_EXIT(ship, 8, DIR_EAST) = 2;
		SHIP_ROOM_EXIT(ship, 8, DIR_SOUTH) = 12;
		SHIP_ROOM_EXIT(ship, 8, DIR_NORTH) = 4;
		SHIP_ROOM_EXIT(ship, 9, DIR_WEST) = 2;
		SHIP_ROOM_EXIT(ship, 9, DIR_SOUTH) = 13;
		SHIP_ROOM_EXIT(ship, 9, DIR_NORTH) = 5;
		SHIP_ROOM_EXIT(ship, 10, DIR_NORTH) = 6;
		SHIP_ROOM_EXIT(ship, 12, DIR_EAST) = 6;
		SHIP_ROOM_EXIT(ship, 12, DIR_NORTH) = 8;
		SHIP_ROOM_EXIT(ship, 13, DIR_WEST) = 6;
		SHIP_ROOM_EXIT(ship, 13, DIR_NORTH) = 9;
		SHIP_ROOM_EXIT(ship, 14, DIR_SOUTH) = 11;

		ship->entrance = 14;
		ship->room_count = 15;
		break;
	case SH_ZONE_SHIP:
		SHIP_ROOM_EXIT(ship, 0, DIR_SOUTH) = 1;
		SHIP_ROOM_EXIT(ship, 1, DIR_NORTH) = 1;

		ship->entrance = 1;
		ship->room_count = 2;
		break;
	default:
		break;
	}
}

/*
 * Blank the ship's room graph: every room vnum and every exit set to -1
 * ("unused").  Called once when a ship is created, before
 * set_ship_layout() fills in the class's shape.
 */
void init_ship_layout(P_ship ship)
{
	for (int j = 0; j < MAX_SHIP_ROOM; j++)
	{
		for (int dir = 0; dir < NUM_EXITS; dir++)
			SHIP_ROOM_EXIT(ship, j, dir) = -1;
		SHIP_ROOM_NUM(ship, j) = -1;
	}
}

/*
 * Return the ship's rooms to the free pool.
 *
 * Frees every exit the ship created, detaches ship_room_proc from each room
 * so find_free_ship_room() will hand it out again, and resets the ship's
 * room bookkeeping.  Called when a ship is unloaded or deleted; failing to
 * call it permanently strands those rooms.
 */
void clear_ship_layout(P_ship ship)
{
	for (int j = 0; j < MAX_SHIP_ROOM; j++)
	{
		if (SHIP_ROOM_NUM(ship, j) != -1)
		{
			int rroom = real_room0(SHIP_ROOM_NUM(ship, j));
			if (!rroom)
				continue;
			for (int dir = 0; dir < NUM_EXITS; dir++)
			{
				if (world[rroom].dir_option[dir])
					FREE(world[rroom].dir_option[dir]);
				world[rroom].dir_option[dir] = 0;
				SHIP_ROOM_EXIT(ship, j, dir) = -1;
			}
			world[rroom].funct = NULL;
		}
		SHIP_ROOM_NUM(ship, j) = -1;
	}
	ship->bridge = -1;
	ship->entrance = -1;
	ship->room_count = 0;
}

/*
 * Find an unused room vnum in the ship zone.
 *
 * Ship interiors are carved out of a fixed pool of rooms
 * (VROOM_SHIPS_START..VROOM_SHIPS_END); a room is "in use" when its funct is
 * ship_room_proc.  The first three are reserved (Davy Jones' locker, the
 * transit room and the undead ferry), so the scan starts past them.
 *
 * Returns -1 when the pool is exhausted OR when it reaches a vnum that does
 * not exist -- the pool has to be contiguous.
 */
int find_free_ship_room()
{
	int rroom, vroom;

	// Three rooms (davy jones for sunken ships, ship transit room, and undead ferry room) are used already.
	vroom = (SHIPZONE * 100) + 3;
	while ((rroom = real_room0(vroom)))
	{
		if (world[rroom].funct != ship_room_proc)
		{
			return vroom;
		}
		if (++vroom > VROOM_SHIPS_END)
		{
			return -1;
		}
	}
	return -1;
}

/*
 * Turn the ship's abstract room graph into real world rooms.
 *
 * Two passes: claim a free room for each of the ship's rooms, then wire up
 * the exits between them (creating direction data where the graph has an
 * exit and freeing it where it does not).  Finally records the bridge and
 * entrance room vnums and titles every room.
 *
 * Returns FALSE if the room pool is exhausted or a vnum fails to resolve.
 *
 * CAVEAT: a mid-way failure leaves the rooms already claimed still marked
 * with ship_room_proc.  The caller is expected to treat this as fatal for the
 * ship (load_ship() does) rather than retrying.
 */
bool set_ship_physical_layout(P_ship ship)
{
	int vroom, rroom, to_room;

	for (int j = 0; j < ship->room_count; j++)
	{
		vroom = find_free_ship_room();
		if ((vroom < VROOM_SHIPS_START) || (vroom > VROOM_SHIPS_END))
			return FALSE;
		if ((rroom = real_room0(vroom)) == 0)
			return FALSE;

		SHIP_ROOM_NUM(ship, j) = vroom;
		world[rroom].funct = ship_room_proc;
	}
	for (int j = 0; j < ship->room_count; j++)
	{
		if ((rroom = real_room0(SHIP_ROOM_NUM(ship, j))) == 0)
		{
			return FALSE;
		}

		for (int dir = 0; dir < NUM_EXITS; dir++)
		{
			if (SHIP_ROOM_EXIT(ship, j, dir) != -1)
			{
				if ((to_room = real_room0(SHIP_ROOM_NUM(
					     ship, SHIP_ROOM_EXIT(ship, j, dir)))) == 0)
					return FALSE;

				if (!world[rroom].dir_option[dir])
					CREATE(world[rroom].dir_option[dir], room_direction_data, 1,
					       MEM_TAG_DIRDATA);

				world[rroom].dir_option[dir]->to_room = to_room;
				world[rroom].dir_option[dir]->exit_info = 0;
			}
			else
			{
				if (world[rroom].dir_option[dir])
				{
					FREE(world[rroom].dir_option[dir]);
					world[rroom].dir_option[dir] = NULL;
				}
			}
		}
	}
	ship->bridge = SHIP_ROOM_NUM(ship, 0);
	ship->entrance = SHIP_ROOM_NUM(ship, ship->entrance);
	name_ship_rooms(ship);

	// Set entrance to/exit from zone to ship to zone here.
	/*
	if( ship == zone_ship )
	{
	  int to_room = real_room( ZONE_SHIP_ZONE_ENTRANCE );
	  if( !to_room )
	    fprintf(stderr, "Failed to link zone ship to zone.\r\n");
	  else
	  {
	    world[real_room(ship->room[1].roomnum)].dir_option[DIR_NORTH]->to_room = to_room;
	    if (!world[to_room].dir_option[DIR_SOUTH])
	      CREATE(world[to_room].dir_option[DIR_SOUTH], room_direction_data, 1, MEM_TAG_DIRDATA);
	    world[to_room].dir_option[DIR_SOUTH]->to_room = real_room(ship->room[1].roomnum);
	    world[to_room].dir_option[DIR_SOUTH]->exit_info = 0;
	  }
	}
	*/
	return TRUE;
}

/*
 * Reload `ship`'s per-arc armour and internal-structure maxima from its
 * class's entry in ship_arc_properties[].
 *
 * `equal` true also fills the current values to full -- a new or repaired
 * ship.  `equal` false keeps the current values but clamps them to the new
 * maxima, which is what a hull downgrade needs so a shrinking ship does not
 * end up with more armour than it can carry.
 */
void set_ship_armor(P_ship ship, bool equal)
{
	ship->maxarmor[SIDE_FORE] = ship_arc_properties[ship->m_class].armor[SIDE_FORE];
	ship->maxarmor[SIDE_PORT] = ship_arc_properties[ship->m_class].armor[SIDE_PORT];
	ship->maxarmor[SIDE_STAR] = ship_arc_properties[ship->m_class].armor[SIDE_STAR];
	ship->maxarmor[SIDE_REAR] = ship_arc_properties[ship->m_class].armor[SIDE_REAR];

	ship->maxinternal[SIDE_FORE] = ship_arc_properties[ship->m_class].internal[SIDE_FORE];
	ship->maxinternal[SIDE_PORT] = ship_arc_properties[ship->m_class].internal[SIDE_PORT];
	ship->maxinternal[SIDE_STAR] = ship_arc_properties[ship->m_class].internal[SIDE_STAR];
	ship->maxinternal[SIDE_REAR] = ship_arc_properties[ship->m_class].internal[SIDE_REAR];

	if (equal)
	{
		ship->armor[SIDE_FORE] = ship->maxarmor[SIDE_FORE];
		ship->armor[SIDE_PORT] = ship->maxarmor[SIDE_PORT];
		ship->armor[SIDE_STAR] = ship->maxarmor[SIDE_STAR];
		ship->armor[SIDE_REAR] = ship->maxarmor[SIDE_REAR];

		ship->internal[SIDE_FORE] = ship->maxinternal[SIDE_FORE];
		ship->internal[SIDE_PORT] = ship->maxinternal[SIDE_PORT];
		ship->internal[SIDE_STAR] = ship->maxinternal[SIDE_STAR];
		ship->internal[SIDE_REAR] = ship->maxinternal[SIDE_REAR];
	}
	else
	{
		ship->armor[SIDE_FORE] = MIN(ship->maxarmor[SIDE_FORE], ship->armor[SIDE_FORE]);
		ship->armor[SIDE_PORT] = MIN(ship->maxarmor[SIDE_PORT], ship->armor[SIDE_PORT]);
		ship->armor[SIDE_STAR] = MIN(ship->maxarmor[SIDE_STAR], ship->armor[SIDE_STAR]);
		ship->armor[SIDE_REAR] = MIN(ship->maxarmor[SIDE_REAR], ship->armor[SIDE_REAR]);

		ship->internal[SIDE_FORE] =
			MIN(ship->maxinternal[SIDE_FORE], ship->internal[SIDE_FORE]);
		ship->internal[SIDE_PORT] =
			MIN(ship->maxinternal[SIDE_PORT], ship->internal[SIDE_PORT]);
		ship->internal[SIDE_STAR] =
			MIN(ship->maxinternal[SIDE_STAR], ship->internal[SIDE_STAR]);
		ship->internal[SIDE_REAR] =
			MIN(ship->maxinternal[SIDE_REAR], ship->internal[SIDE_REAR]);
	}
}

/*
 * Return `ship` to its class's factory condition: full armour and internals,
 * full sail, full repair pool, stopped.
 *
 * `clear_slots` true -- the default -- also empties every slot, discarding
 * weapons, equipment and cargo.  Pass false to keep the fit-out, which is
 * what a repair wants.
 *
 * Used after a hull change and by finish_sinking(), which downgrades a sunk
 * player ship to a sloop and resets it here.
 */
void reset_ship(P_ship ship, bool clear_slots)
{
	set_ship_armor(ship, true);
	ship->mainsail = SHIPTYPE_MAX_SAIL(ship->m_class);
	ship->repair = SHIPTYPE_HULL_WEIGHT(ship->m_class);

	obj_from_room(ship->panel);
	name_ship(ship->name, ship);
	clear_ship_layout(ship);
	set_ship_layout(ship, ship->m_class);
	set_ship_physical_layout(ship);
	obj_to_room(ship->panel, real_room0(ship->bridge));

	ship->timer[T_UNDOCK] = 0;
	ship->timer[T_MANEUVER] = 0;
	ship->timer[T_SINKING] = 0;
	ship->timer[T_BSTATION] = 0;
	ship->timer[T_RAM] = 0;
	ship->timer[T_RAM_WEAPONS] = 0;
	ship->timer[T_MAINTENANCE] = 0;
	ship->timer[T_MINDBLAST] = 0;

	if (IS_SET(ship->flags, SINKING))
		REMOVE_BIT(ship->flags, SINKING);
	if (IS_SET(ship->flags, FLYING))
		REMOVE_BIT(ship->flags, FLYING);
	if (IS_SET(ship->flags, SUNKBYNPC))
		REMOVE_BIT(ship->flags, SUNKBYNPC);
	if (IS_SET(ship->flags, ATTACKBYNPC))
		REMOVE_BIT(ship->flags, ATTACKBYNPC);
	if (IS_SET(ship->flags, RAMMING))
		REMOVE_BIT(ship->flags, RAMMING);
	if (IS_SET(ship->flags, ANCHOR))
		REMOVE_BIT(ship->flags, ANCHOR);
	if (IS_SET(ship->flags, SUMMONED))
		REMOVE_BIT(ship->flags, SUMMONED);

	if (clear_slots)
	{
		for (int j = 0; j < MAXSLOTS; j++)
			ship->slot[j].clear();
	}
}

//--------------------------------------------------------------------
// This proc is added to rooms inside ship
//--------------------------------------------------------------------

/*
 * Special procedure attached to every room INSIDE a ship.
 *
 * Handles what is special about being aboard: leaving the ship through the
 * entrance, boarding another ship alongside, and the movement and look
 * restrictions that apply at sea.  Attached by set_ship_physical_layout(),
 * detached by clear_ship_layout() -- which is also how the room pool tracks
 * which rooms are in use.
 *
 * Returns TRUE when the command was handled, FALSE to fall through to normal
 * processing.
 */
int ship_room_proc([[maybe_unused]] int room, P_char ch, int cmd, char *arg)
{
	int i, j, k;
	P_ship ship;
	int virt;

	if (!ch)
		return false;

	if ((cmd != CMD_LOOK) && (cmd != CMD_DISEMBARK))
		return (FALSE);

	ship = get_ship_from_char(ch);

	if (cmd == CMD_LOOK)
	{
		if (!arg || !*arg || str_cmp(skip_spaces(arg), "out"))
			return (FALSE);
		look_out_ship(ship, ch);
		return (TRUE);
	}
	if (cmd == CMD_DISEMBARK)
	{
		if (IS_IMMOBILE(ch))
		{
			send_to_char("\r\nYou cannot disembark in your present condition.\r\n", ch);
			return false;
		}

		if (SHIP_FLYING(ship) && !IS_TRUSTED(ch))
		{
			send_to_char("\r\nYou cannot disembark in air!\r\n", ch);
			return false;
		}

		if (IS_BLIND(ch) && number(0, 5))
		{
			send_to_char(
				"&+WIt is hard to disembark when you cannot see anything... but you keep trying!\r\n",
				ch);
			return false;
		}

		if (ship->m_class == SH_CRUISER || ship->m_class == SH_DREADNOUGHT)
		{
			//      if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship))
			{
				if (world[ch->in_room].number == ship->entrance || IS_TRUSTED(ch))
				{
					if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) ||
					    IS_FIGHTING(ch))
					{
						send_to_char(
							"You're in no position to disembark!\r\n",
							ch);
						return (TRUE);
					}
					act("You step off the docking bay of this ship.", FALSE, ch,
					    0, 0, TO_CHAR);
					act("$n steps off the ship.", TRUE, ch, 0, 0, TO_ROOM);
					char_from_room(ch);
					char_to_room(ch, ship->shipobj->loc.room, 0);
					act("$n disembarks from the docking bay of $p.", TRUE, ch,
					    ship->shipobj, 0, TO_ROOM);
					return TRUE;
				}
				else
				{
					send_to_char(
						"You must disembark from the lower docking bay!\r\n",
						ch);
					return TRUE;
				}
			}
			//      else
			//      {
			//         send_to_char("All hatches are tightly closed!  You cannot disembark!\r\n", ch);
			//         return TRUE;
			//      }
		}

		i = world[ch->in_room].number;
		j = i - ((int)(i / 10) * 10);
		k = 0;
		if (SHIP_ROOM_EXIT(ship, j, DIR_NORTH) == -1 ||
		    SHIP_ROOM_EXIT(ship, j, DIR_SOUTH) == -1 ||
		    SHIP_ROOM_EXIT(ship, j, DIR_EAST) == -1 ||
		    SHIP_ROOM_EXIT(ship, j, DIR_WEST) == -1)
		{
			k = 1;
		}

		if (world[ch->in_room].number != ship->entrance && !IS_TRUSTED(ch))
		{
			if (!k && !IS_TRUSTED(ch))
			{
				send_to_char(
					"You are not close enough to the edge of the ship to jump out!\r\n",
					ch);
				return TRUE;
			}
		}

		if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) || IS_FIGHTING(ch))
		{
			send_to_char("You're in no position to disembark!\r\n", ch);
			return (TRUE);
		}
		if (IS_NPC(ch))
		{
			virt = mob_index[GET_RNUM(ch)].virtual_number;
			if (virt == EVIL_AVATAR_MOB || virt == GOOD_AVATAR_MOB)
			{
				send_to_char("You can't leave this place!\r\n", ch);
				return (FALSE);
			}
		}

		/* board another ship */
		/*
		  if ((to_ship = ships[t_ship].dock_vehicle) != NONE)
		  {
		   if (!is_valid_ship(to_ship)) {
		     send_to_char("Strange... that is a ghost ship!\r\n", ch);
		   } else {
		     act("You leave this ship and board $p.",
		        FALSE, ch, ships[to_ship].obj, 0, TO_CHAR);
		     act("$n leaves this ship and boards $p.",
		        TRUE, ch, ships[to_ship].obj, 0, TO_ROOM);
		     char_from_room(ch);
		     char_to_room(ch, ships[to_ship].entrance_room, 0);
		     act("$n leaves $p and boards this ship.",
		        TRUE, ch, ships[t_ship].obj, 0, TO_ROOM);
		   }
		  }
		  else
		*/
		{ /* go on land */
			act("You disembark this ship.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n disembarks this ship.", TRUE, ch, 0, 0, TO_ROOM);
			char_from_room(ch);
			char_to_room(ch, ship->shipobj->loc.room, 0);
			act("$n disembarks from $p.", TRUE, ch, ship->shipobj, 0, TO_ROOM);
		}
		return (TRUE);
	}
	return (TRUE);
}

/*
 * Special procedure attached to every ship's HULL object -- the thing
 * floating in the ocean room, as opposed to the control panel inside.
 *
 * Handles what people outside a ship can do with it: looking at it and
 * boarding it.  Finds the owning ship through shipObjHash.
 *
 * Returns TRUE when the command was handled, FALSE to fall through to
 * normal command processing.
 */
int ship_obj_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	char name[MAX_INPUT_LENGTH];
	P_obj obj_entered;
	P_ship ship;

	/* check for periodic event calls */
	if (cmd == -10)
		return FALSE;

	if (cmd != CMD_ENTER)
		return (FALSE);

	one_argument(arg, name);
	obj_entered = get_obj_in_list_vis(ch, name, world[ch->in_room].contents);
	if (obj_entered != obj)
		return (FALSE);

	if (!obj || (obj->type != ITEM_SHIP))
	{
		return (FALSE);
	}
	else
	{
		ship = shipObjHash.find(obj);
		if (ship == NULL)
			return FALSE;

		if (SHIP_FLYING(ship) && !IS_TRUSTED(ch))
		{
			send_to_char("&+RThat ship flies too high to board!&n\r\n", ch);
			return TRUE;
		}
		if (IS_WARSHIP(ship) && ship->speed > 0 && !SHIP_SINKING(ship) && !IS_TRUSTED(ch))
		{
			send_to_char("&+RThat ship is moving too fast to board!&n\r\n", ch);
			return TRUE;
		}
		else if (ship->speed > BOARDING_SPEED && !SHIP_SINKING(ship) && !IS_TRUSTED(ch))
		{
			send_to_char("&+RThat ship is moving too fast to board!&n\r\n", ch);
			return TRUE;
		}
		else if (ship->m_class == SH_CRUISER || ship->m_class == SH_DREADNOUGHT)
		{
			//            if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship))
			{
				act("$n enters through the docking bay of $p.", TRUE, ch,
				    ship->shipobj, 0, TO_ROOM);
				char_from_room(ch);
				act("You step through the docking bay of $p.", FALSE, ch,
				    ship->shipobj, 0, TO_CHAR);
				char_to_room(ch, real_room0(ship->entrance), 0);
				act("$n steps through the docking bay.", TRUE, ch, 0, 0, TO_ROOM);
				return TRUE;
			}
			//            else
			//            {
			//                send_to_char("This ship's hatches are tightly shut and the armor is too tough to break through!\r\n", ch);
			//                return TRUE;
			//            }
		}
		else
		{
			act("$n boards $p.", TRUE, ch, ship->shipobj, 0, TO_ROOM);
			char_from_room(ch);
			act("You board $p.", FALSE, ch, ship->shipobj, 0, TO_CHAR);
			char_to_room(ch, real_room0(ship->entrance), 0);
			act("$n comes aboard.", TRUE, ch, 0, 0, TO_ROOM);
		}
	}
	return (TRUE);
}

bool is_npc_ship_name(const char *);
/*
 * Validate a proposed ship name, explaining any rejection to `ch`.
 *
 * Enforces length, permitted characters and colour-code limits, and screens
 * against the obscenity list.  Returns TRUE when the name may be used.
 */
bool check_ship_name(P_ship ship, P_char ch, char *name)
{
	char plain[MAX_STRING_LENGTH];
	const char *err = 0;

	if (ship && IS_NPC_SHIP(ship))
		return true;

	AnsiString an(name);
	an.plain(plain);
	if (an.empty()) // only ansi
		err = "&+gA ship must bear a name.&n\n";
	else if (an.ch(0) == ' ' || an.ch(an.size() - 1) == ' ')
		err = "&+gColored spaces are still spaces.&n\n";
	else if (an.size() > 20)
		err = "&+gShip names can be at most 20 characters (not including ansi).&n\n";
	else if (sub_string_set(plain, rude_ass))
		err = "&+gName must not contain rude terms.&n\n";
	else if (is_npc_ship_name(plain))
		err = "&+gThis name is reserved, choose another name for your ship!&n\n";
	else if (!strncmp(name, "testcolor", 9))
		err = "&+gWe're no longer testing, give us a name!&n\n";
	else
		for (wchar_t c : an)
		{
			if (GET_BG(c))
				err = "&+gBackground colors are for painters, not sailors!&n\n";
			if (GET_FG(c) == 16)
				err = "&+gI can't see that...&n\n";
		}

	if (err)
	{
		send_to_char(err, ch);
		return false;
	}

	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		if (svs == ship || IS_NPC_SHIP(svs))
			continue;
		if (!strcasecmp(plain, strip_ansi(svs->name).c_str()))
		{
			if (!ship || ship->frags <= svs->frags)
			{
				send_to_char(
					"Another player already has a ship with such name, choose another name for your ship!\r\n",
					ch);
				return false;
			}
		}
	}
	return true;
}

/*
 * Whether `ship` may leave port right now, explaining any refusal to `ch`.
 *
 * Covers the conditions that are not simply damage: the captain's level
 * against the hull class `m_class`, outstanding maintenance, passenger count
 * against capacity, and the state of the crew.  Called from order_undock()
 * (ship_control.c) before the undock timer starts.
 *
 * Returns TRUE when undocking may proceed.
 */
bool check_undocking_conditions(P_ship ship, int m_class, P_char ch)
{
	int arc_weapons[4], arc_weapon_weight[4];

	if (!check_ship_name(ship, ch, ship->name))
		return false;

	for (int a = 0; a < 4; a++)
	{
		arc_weapons[a] = 0;
		arc_weapon_weight[a] = 0;
	}

	for (int sl = 0; sl < MAXSLOTS; sl++)
	{
		if (ship->slot[sl].type == SLOT_WEAPON)
		{
			arc_weapons[ship->slot[sl].position]++;
			arc_weapon_weight[ship->slot[sl].position] +=
				weapon_data[ship->slot[sl].index].weight;
			if (!ship_allowed_weapons[m_class][ship->slot[sl].index])
			{
				send_to_char_f(
					ch,
					"Remove weapon [%d], it is not allowed with this hull!\r\n",
					sl);
				return FALSE;
			}
		}
	}
	for (int a = 0; a < 4; a++)
	{
		if (arc_weapons[a] > ship_arc_properties[m_class].max_weapon_slots[a])
		{
			send_to_char_f(
				ch,
				"Your have too many weapons at one side!\r\nMaximum allowed weapons for this ship is:\r\nFore: %d  Starboard: %d  Port: %d  Rear: %d\r\n",
				ship_arc_properties[m_class].max_weapon_slots[SIDE_FORE],
				ship_arc_properties[m_class].max_weapon_slots[SIDE_STAR],
				ship_arc_properties[m_class].max_weapon_slots[SIDE_PORT],
				ship_arc_properties[m_class].max_weapon_slots[SIDE_REAR]);
			return FALSE;
		}
		if (arc_weapon_weight[a] > ship_arc_properties[m_class].max_weapon_weight[a])
		{
			send_to_char_f(
				ch,
				"Your have overloaded one side with weapons!\r\nMaximum allowed weapon weight for this ship is:\r\nFore: %d  Starboard: %d  Port: %d  Rear: %d\r\n",
				ship_arc_properties[m_class].max_weapon_weight[SIDE_FORE],
				ship_arc_properties[m_class].max_weapon_weight[SIDE_STAR],
				ship_arc_properties[m_class].max_weapon_weight[SIDE_PORT],
				ship_arc_properties[m_class].max_weapon_weight[SIDE_REAR]);
			return FALSE;
		}
	}

	if (SHIPTYPE_MIN_LEVEL(m_class) > GET_LEVEL(ch))
	{
		send_to_char(
			"You are too low for such a big ship! Get more experience or downgrade the hull!'\r\n",
			ch);
		return FALSE;
	}
	return TRUE;
}

/*
 * The ship system's heartbeat: one tick for every loaded ship.  Called from
 * the main game loop.  This is the routine that makes ships actually sail.
 *
 * Per ship, in order:
 *   1. count down all MAXTIMERS timers and announce the ones that expire
 *   2. regenerate (or report exhausted) crew stamina
 *   3. hold or stand down from battlestations
 *   4. run crew repairs on sails, armour and internals
 *   5. finish a sinking whose timer has run out
 *   6. step the undocking sequence, which is a scripted countdown
 *   7. converge speed and heading on their ordered values, charging crew
 *      stamina for the effort
 *   8. move the ship between ocean rooms as its position crosses a boundary,
 *      or try to avoid running aground
 *   9. resolve ramming if the target has come within one room
 *  10. tick weapon reload timers
 *  11. run the autopilot and the NPC AI
 *
 * WARNING -- do not turn the `return` after finish_sinking() into a
 * `continue`.  finish_sinking() FREES an NPC ship and erases it from the
 * hash, which strands the iterator; bailing out of the whole sweep is what
 * keeps that from being a use-after-free.  The cost is that ships later in
 * the hash lose one tick whenever a ship finishes sinking.
 */
void ship_activity()
{
	int j, k, loc;
	float rad;

	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		P_ship ship = svs;

		// Check timers
		if (!SHIP_LOADED(ship))
			continue;

		if (ship->timer[T_RAM_WEAPONS] == 1)
		{
			act_to_all_in_ship(ship, "Your gun crew has recovered from ram impact.&N");
		}
		if (ship->timer[T_RAM] == 1)
		{
			act_to_all_in_ship(ship,
					   "Your crew has returned to their battle stations.");
		}
		if (ship->timer[T_MINDBLAST] == 1)
		{
			act_to_all_in_ship(ship,
					   "Your crew has recovered from mental shock.&N\r\n");
		}

		for (j = 0; j < MAXTIMERS; j++)
		{
			if (ship->timer[j] > 0)
				ship->timer[j] -= 1;
		}

		if (!IS_SET(ship->flags, SINKING))
		{
			// STAMINA REGEN
			if (ship->crew.stamina < ship->crew.max_stamina)
			{
				float stamina_inc = 3;
				if (IS_NPC_SHIP(ship))
					stamina_inc = 4;
				if (ship == cyrics_revenge)
					stamina_inc = 15;
				// if (ship == zone_ship)
				//     stamina_inc = 15;
				if (SHIP_DOCKED(ship) || SHIP_ANCHORED(ship))
					stamina_inc *= 4;

				ship->crew.stamina += stamina_inc;
				if (ship->crew.stamina > ship->crew.max_stamina)
					ship->crew.stamina = ship->crew.max_stamina;
			}
			if (ship->crew.stamina < 0)
			{
				if (number(1, 30) == 1)
				{
					if (ship->crew.stamina > -ship->crew.max_stamina)
						act_to_all_in_ship(
							ship,
							"&+rYour crew looks exhausted.&N\r\n");
					else
						act_to_all_in_ship(
							ship,
							"&+rYour crew looks &+Rcompletely &+rexhausted.&N\r\n");
				}
			}

			// Battle Stations!
			if (ship->target != NULL)
				ship->timer[T_BSTATION] = BSTATION;
			if (ship->timer[T_BSTATION] == 1)
				act_to_all_in_ship(
					ship, "Your crew stands down from battlestations.\r\n");

			// Repairing
			if ((ship->repair > 0) && ship->timer[T_MINDBLAST] == 0)
			{
				float chance = 0.0;
				if (ship->mainsail < (int)((float)SHIP_MAX_SAIL(ship) *
							   (ship->crew.rpar_mod_applied + 0.4)) &&
				    ship->mainsail < SHIP_MAX_SAIL(ship) * 0.9)
				{
					if (SHIP_ANCHORED(ship))
					{
						if (ship->mainsail > 0)
							chance = 500.0;
						else if (ship->timer[T_BSTATION] == 0)
							chance = 100.0;
						else
							chance = 30.0;
					}
					else
					{
						if (ship->mainsail > 0)
							chance = 30.0;
						else
							chance = 0.0;
					}
					chance *= (1.0 + ship->crew.rpar_mod_applied) *
						  ship->crew.get_stamina_mod();
					if (number(0, 1000) < (int)chance)
					{
						ship->mainsail +=
							MIN(ship->crew.get_sail_repair_mod(),
							    (SHIP_MAX_SAIL(ship) - ship->mainsail));
						ship->repair--;
						if (ship->repair == 0)
							act_to_all_in_ship(
								ship,
								"&+RThe ship is out of repair materials!.&N");
						ship->crew.reduce_stamina(3, ship);
						ship->crew.rpar_skill_raise(
							(ship->timer[T_BSTATION] > 0 &&
							 HAS_VALID_TARGET(ship)) ?
								0.1 :
								0.01);
						update_ship_status(ship);
					}
				}
				for (j = 0; j < MAXSLOTS; j++)
				{
					if (ship->repair < 1)
						break;
					if (ship->slot[j].type == SLOT_WEAPON)
					{
						if (SHIP_WEAPON_DAMAGED(ship, j) &&
						    !SHIP_WEAPON_DESTROYED(ship, j))
						{
							chance = 200.0;
							chance *= (1.0 +
								   ship->crew.rpar_mod_applied) *
								  ship->crew.get_stamina_mod();
							if (number(0, 999) < (int)chance)
							{
								ship->slot[j].val2 -= MIN(
									ship->crew
										.get_weapon_repair_mod(),
									ship->slot[j].val2);
								if (ship->slot[j].val2 < 0)
									ship->slot[j].val2 = 0;
								if (!SHIP_WEAPON_DAMAGED(ship, j))
								{
									act_to_all_in_ship_f(
										ship,
										"&+W%s &+Ghas been repaired!&N",
										weapon_data
											[ship->slot[j]
												 .index]
												.name);
									if (ship->slot[j].val1 > 0)
										ship->slot[j].timer =
											(int)((float)weapon_data
												      [j]
													      .reload_time *
											      (1.0 -
											       ship->crew.guns_mod_applied *
												       0.15));
								}
								if (!number(0, 4))
									ship->repair--;
								if (ship->repair == 0)
									act_to_all_in_ship(
										ship,
										"&+RThe ship is out of repair materials!.&N");
								ship->crew.reduce_stamina(1, ship);
								ship->crew.rpar_skill_raise(
									(ship->timer[T_BSTATION] >
										 0 &&
									 HAS_VALID_TARGET(ship)) ?
										0.1 :
										0.01);
							}
						}
					}
				}
				bool can_repair_internal = false;
				for (j = 0; j < 4; j++)
				{
					if (ship->repair < 1)
						break;
					if (ship->internal[j] <
						    (int)((float)ship->maxinternal[j] *
							  (ship->crew.rpar_mod_applied + 0.1)) &&
					    ship->internal[j] < ship->maxinternal[j] * 0.9)
					{
						can_repair_internal = true;
						if (SHIP_ANCHORED(ship))
						{
							if (ship->timer[T_BSTATION] == 0)
							{
								if (ship->internal[j] > 0)
									chance = 250.0;
								else
									chance = 100.0;
							}
							else
							{
								if (ship->internal[j] > 0)
									chance = 100.0;
								else
									chance = 10.0;
							}
						}
						else
						{
							if (ship->internal[j] > 0)
								chance = 30.0;
							else
								chance = 0.0;
						}
						chance *= (1.0 + ship->crew.rpar_mod_applied) *
							  ship->crew.get_stamina_mod();
						if (number(0, 1000) < (int)chance)
						{
							ship->internal[j] += MIN(
								ship->crew.get_hull_repair_mod(),
								(ship->maxinternal[j] -
								 ship->internal[j]));
							ship->repair--;
							if (ship->repair == 0)
								act_to_all_in_ship(
									ship,
									"&+RThe ship is out of repair materials!.&N");
							ship->crew.reduce_stamina(2, ship);
							ship->crew.rpar_skill_raise(
								(ship->timer[T_BSTATION] > 0 &&
								 HAS_VALID_TARGET(ship)) ?
									0.1 :
									0.01);
							update_ship_status(ship);
						}
					}
				}
				if (!can_repair_internal && ship->timer[T_BSTATION] == 0 &&
				    ship->crew.rpar_mod_applied > 0.5)
				{ // highly skilled crew can repair some armor when not in combat
					for (j = 0; j < 4; j++)
					{
						if (ship->repair < 1)
							break;
						if (ship->armor[j] <
							    (int)((float)ship->maxarmor[j] *
								  (ship->crew.rpar_mod_applied -
								   0.5)) &&
						    ship->armor[j] < ship->maxarmor[j] * 0.9)
						{
							if (SHIP_ANCHORED(ship))
							{
								chance = 150.0;
							}
							else
							{
								chance = 20.0;
							}
							chance *= (1.0 +
								   ship->crew.rpar_mod_applied) *
								  ship->crew.get_stamina_mod();
							if (number(0, 1000) < (int)chance)
							{
								ship->armor[j] += MIN(
									ship->crew
										.get_hull_repair_mod(),
									(ship->maxarmor[j] -
									 ship->armor[j]));
								ship->repair--;
								if (ship->repair == 0)
									act_to_all_in_ship(
										ship,
										"&+RThe ship is out of repair materials!.&N");
								ship->crew.reduce_stamina(4, ship);
								ship->crew.rpar_skill_raise(
									(ship->timer[T_BSTATION] >
										 0 &&
									 HAS_VALID_TARGET(ship)) ?
										0.1 :
										0.01);
								update_ship_status(ship);
							}
						}
					}
				}
			}
		}

		// SINKING!!!!
		if (IS_SET(ship->flags, SINKING))
		{
			if (ship->timer[T_SINKING] == 0)
			{
				finish_sinking(ship);
				/*
				 * NOT a `continue`.  For an NPC ship,
				 * finish_sinking() erases it from shipObjHash
				 * and frees it, which leaves `svs` pointing at
				 * freed memory -- get_next() would then read
				 * through it.  Abandoning the whole sweep is
				 * the price of keeping that safe; the ships
				 * after this one simply lose a tick.
				 */
				return;
			}
		}

		// Undocking
		if (ship->timer[T_UNDOCK] > 0)
		{
			if (ship->timer[T_UNDOCK] == 27)
				act_to_all_in_ship(ship,
						   "&+LThe crew begins raising the anchor.&N");
			else if (ship->timer[T_UNDOCK] == 21)
				act_to_all_in_ship(ship,
						   "&+WThe crew finishes raising the anchor.&N");
			else if (ship->timer[T_UNDOCK] == 17)
			{
				if (SHIP_ANCHORED(ship) && !SHIP_DOCKED(ship))
				{
					act_to_all_in_ship(
						ship,
						"&+GThe crew scrambles to their stations, the ship is ready to go.&N");
					REMOVE_BIT(ship->flags, ANCHOR);
					ship->timer[T_UNDOCK] = 0;
					update_ship_status(ship);
				}
				else
					act_to_all_in_ship(
						ship, "&+yThe crew scrambles to their stations.&N");
			}
			else if (ship->timer[T_UNDOCK] == 12)
				act_to_all_in_ship(ship, "&+WThe crew readies the sails.&N");
			else if (ship->timer[T_UNDOCK] == 9)
				act_to_all_in_ship(
					ship,
					"&+yThe first officer begins a checkup of all ship systems.&N");
			else if (ship->timer[T_UNDOCK] == 1)
			{
				assignid(ship, NULL);
				act_to_all_in_ship(
					ship,
					"&+GThe first officer reports everything is in order and the ship is ready to go.&N");
				REMOVE_BIT(ship->flags, DOCKED);
				REMOVE_BIT(ship->flags, ANCHOR);
				update_crew(ship);
				update_ship_status(ship);
			}
		}

		// Maintenance Timer
		if (ship->timer[T_MAINTENANCE] == 1)
		{
		}

		// Undocked and non sinking actions go below here
		if (!SHIP_SINKING(ship) && !SHIP_DOCKED(ship) && !SHIP_ANCHORED(ship))
		{
			if (IS_WATER_ROOM(ship->location) ||
			    IS_ROOM(ship->location, ROOM_DOCKABLE) || SHIP_FLYING(ship))
			{
				// Setspeed to Speed DRANNAK
				P_char ch = captain_is_aboard(ship);
				int realspeed = ship->get_maxspeed(ch);

				if (ship->setspeed > realspeed)
				{
					ship->setspeed = realspeed;
				}
				if (ship->setspeed != ship->speed && ship->timer[T_MINDBLAST] == 0)
				{
					int sp_change = get_next_speed_change(ship);
					ship->speed += sp_change;

					// affect crew stamina
					float sp_rel_change =
						((float)ABS(sp_change) / (float)SHIP_ACCEL(ship)) /
						(1.0 + ship->crew.sail_mod_applied);
					ship->crew.reduce_stamina(
						sp_rel_change * (2.0 + SHIP_HULL_MOD(ship) / 10.0),
						ship);
				}

				// SetHeading to Heading
				if (ship->setheading != ship->heading &&
				    ship->timer[T_MINDBLAST] == 0)
				{
					float hd_change = get_next_heading_change(ship);
					ship->heading += hd_change;
					normalize_direction(ship->heading);

					// affect crew stamina
					float hd_rel_change =
						(ABS(hd_change) / (float)SHIP_HDDC(ship)) /
						(1.0 + ship->crew.sail_mod_applied);
					if (SHIP_IMMOBILE(ship))
						hd_rel_change *= 5;
					ship->crew.reduce_stamina(
						hd_rel_change * (3.0 + SHIP_HULL_MOD(ship) / 10.0),
						ship);
				}
			}

			// Movement Goes here
			if (ship->speed != 0)
			{
				rad = ship->heading * M_PI / 180.000;
				ship->x += (float)((float)ship->speed * sin(rad)) / 150.000;
				ship->y += (float)((float)ship->speed * cos(rad)) / 150.000;

				if ((ship->y >= 51.000) || (ship->x >= 51.000) ||
				    (ship->y < 50.000) || (ship->x < 50.000))
				{
					if (getmap(ship))
					{
						if (SHIP_CLASS(ship) != SH_SLOOP &&
						    SHIP_CLASS(ship) != SH_YACHT)
							ship->crew.sail_skill_raise(0.003);
						loc = tactical_map[(int)ship->x][100 - (int)ship->y]
							      .rroom;
						if (is_valid_sailing_location(ship, loc))
						{
							if (ship->x > 50.999)
							{
								ship->x -= 1.000;
								if (SHIP_FLYING(ship))
								{
									send_to_room_f(
										ship->location,
										"%s&N floats east above you.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N floats in from the west above you.\r\n",
										ship->name);
								}
								else
								{
									send_to_room_f(
										ship->location,
										"%s&N sails east.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N sails in from the west.\r\n",
										ship->name);
								}
							}
							else if (ship->x < 50.000)
							{
								ship->x += 1.000;
								if (SHIP_FLYING(ship))
								{
									send_to_room_f(
										ship->location,
										"%s&N floats west above you.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N floats in from the east above you.\r\n",
										ship->name);
								}
								else
								{
									send_to_room_f(
										ship->location,
										"%s&N sails west.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N sails in from the east.\r\n",
										ship->name);
								}
							}

							if (ship->y > 50.999)
							{
								ship->y -= 1.000;
								if (SHIP_FLYING(ship))
								{
									send_to_room_f(
										ship->location,
										"%s&N floats north above you.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N floats in from the south above you.\r\n",
										ship->name);
								}
								else
								{
									send_to_room_f(
										ship->location,
										"%s&N sails north.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N sails in from the south.\r\n",
										ship->name);
								}
							}
							else if (ship->y < 50.000)
							{
								ship->y += 1.000;
								if (SHIP_FLYING(ship))
								{
									send_to_room_f(
										ship->location,
										"%s&N floats south above you.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N floats in from the north above you.\r\n",
										ship->name);
								}
								else
								{
									send_to_room_f(
										ship->location,
										"%s&N sails south.\r\n",
										ship->name);
									send_to_room_f(
										loc,
										"%s&N sails in from the north.\r\n",
										ship->name);
								}
							}
							if (SHIP_OBJ(ship) &&
							    (loc != ship->location))
							{
								ship->location = loc;
								obj_from_room(SHIP_OBJ(ship));
								obj_to_room(SHIP_OBJ(ship), loc);
								everyone_look_out_ship(ship);
							}
						}
						else
						{
							ship->setspeed = 0;
							ship->speed = 0;
							ship->x = 50.500;
							ship->y = 50.500;

							if (ship->autopilot)
								stop_autopilot(ship);

							int crash_chance =
								(ship->timer[T_BSTATION] == 0) ?
									0 :
									(int)((float)(ship->speed +
										      50) /
									      ((1.0 +
										ship->crew.sail_mod_applied *
											2.0) *
									       ship->crew
										       .get_stamina_mod()));

							if (ship->timer[T_MINDBLAST] == 0)
								act_to_all_in_ship(
									ship,
									"Your crew attempts to stop the ship from crashing into land!");
							else
								crash_chance = 100;

							if (dice(2, 50) <= crash_chance)
								crash_land(ship);
							else
								act_to_all_in_ship(
									ship,
									"Your crew manages to stop the ship from running ashore.");
						}
					}
				}
			}

			// Ramming
			if (IS_SET(ship->flags, RAMMING))
			{
				if (ship->target == NULL)
				{
					act_to_all_in_ship(
						ship,
						"&+WStanding down from ramming mode due to no target.&N");
					REMOVE_BIT(ship->flags, RAMMING);
				}
				else if (ship->speed <= BOARDING_SPEED)
				{
					act_to_all_in_ship(
						ship,
						"&+WStanding down from ramming mode due to low speed.&N");
					REMOVE_BIT(ship->flags, RAMMING);
				}
				else if (ship->timer[T_MINDBLAST] == 0)
				{
					k = getcontacts(ship);
					for (j = 0; j < k; j++)
					{
						if (contacts[j].ship == ship->target)
						{
							if (contacts[j].range < 1.0)
							{
								try_ram_ship(ship, ship->target,
									     contacts[j].bearing);
							}
						}
					}
				}
			}

			// Slot timers
			if (!IS_SET(ship->flags, RAMMING) && ship->timer[T_RAM_WEAPONS] == 0 &&
			    ship->timer[T_MINDBLAST] == 0)
			{
				for (j = 0; j < MAXSLOTS; j++)
				{
					if (ship->slot[j].type == SLOT_WEAPON)
					{
						if (ship->slot[j].timer > 0)
						{
							if (number(0, 99) >=
							    int(ship->crew.get_stamina_mod() * 100))
								continue;
							ship->slot[j].timer--;
							if (ship->slot[j].timer == 0)
							{
								act_to_all_in_ship_f(
									ship,
									"Weapon &+W[%d]&N: [%s] has finished reloading.",
									j,
									ship->slot[j]
										.get_description());
							}
							// affect crew stamina
							ship->crew.reduce_stamina(
								(float)weapon_data[ship->slot[j]
											   .index]
										.weight /
									SHIP_HULL_MOD(ship),
								ship);
							if (HAS_VALID_TARGET(ship))
								ship->crew.guns_skill_raise(0.003);
						}
					}
					if (ship->slot[j].type == SLOT_EQUIPMENT)
					{
						if (ship->slot[j].timer > 0)
						{
							ship->slot[j].timer--;
							if (ship->slot[j].timer == 0)
							{
								if (ship->slot[j].index ==
									    E_LEVISTONE &&
								    !IS_SET(ship->flags, AIR))
								{
									if (SHIP_FLYING(ship))
										land_ship(ship);
									else
										act_to_all_in_ship_f(
											ship,
											"%s is fully recharged.",
											ship->slot[j]
												.get_description());
								}
							}
						}
					}
				}
			}

			if (ship->autopilot)
				autopilot_activity(ship);
			if (ship->npc_ai)
				ship->npc_ai->activity();

			int pirate_chance =
				has_eq_diplomat(ship) ?
					get_property("ships.pirate.diplomat.load.chance", 30000) :
					get_property("ships.pirate.load.chance", 7200);
			if (ship->target == 0 && ship->speed > 0 && number(0, pirate_chance) == 0)
				try_load_pirate_ship(ship);
		}
	}
}

/*
 * Put `ship` alongside in `to_room` and raise DOCKED.
 *
 * Records the room as the ship's anchor point for next time, drops any target
 * lock and any locks other ships had on it, restores the repair pool, resets
 * the contact designation to the docked placeholder "**", and refreshes crew
 * stamina.
 *
 * `to_room` is a REAL room index.  Room 0 is treated as "the room went away"
 * and the ship is moved back to its recorded anchor instead.
 */
void dock_ship(P_ship ship, int to_room)
{
	// Add in Docking event
	act_to_all_in_ship(ship, "Your ship begins docking procedures.");
	if ((real_room0(world[to_room].number) == to_room) && (to_room != 0))
	{
		ship->anchor = world[to_room].number;
		queue_ship_save(ship, "docking anchor update");
	}
	if (to_room == 0)
	{
		act_to_all_in_ship(ship, "ERROR: Room is void, moving back to anchor point.");
		ship->location = real_room0(ship->anchor);
		obj_from_room(ship->shipobj);
		obj_to_room(ship->shipobj, ship->location);
	}
	if (ship->target != NULL)
		ship->target = NULL;

	clear_references_to_ship(ship);

	ship->location = to_room;
	ship->repair = SHIPTYPE_HULL_WEIGHT(ship->m_class);
	assignid(ship, "**");
	act_to_all_in_ship(ship, "Your ship has completed docking procedures.");
	SET_BIT(ship->flags, DOCKED);
	if (IS_SET(ship->flags, ATTACKBYNPC))
		REMOVE_BIT(ship->flags, ATTACKBYNPC);
	reset_crew_stamina(ship);
	update_ship_status(ship);
}

/*
 * Run `ship` aground.
 *
 * Deals a number of hits proportional to hull weight; the first always lands
 * on the forward arc and the rest scatter across arcs and sails.  Then
 * reconciles the damage through update_ship_status(), which may sink the ship
 * outright.
 */
void crash_land(P_ship ship)
{
	act_to_all_in_ship(ship, "&+yCRUNCH!! Your ship crashes into land!&N");
	act_to_outside_ships(ship, NULL, DEFAULT_RANGE, "&+W[%s]&N:%s&N crashes into land!",
			     SHIP_ID(ship), ship->name);
	int hits = (SHIP_HULL_WEIGHT(ship) / 25) + 1;
	for (int k = 0; k < hits; k++)
	{
		if (k == 0 || number(1, 2) == 2)
		{
			int arc = (k == 0) ? SIDE_FORE : number(0, 5);
			int dam = number(1, 9);
			if (arc < 4)
			{
				damage_hull(NULL, ship, dam, arc, 0);
			}
			else // sails
			{
				damage_sail(NULL, ship, dam);
			}
		}
	}
	update_ship_status(ship);
}

/*
 * Complete a sinking whose T_SINKING timer has expired.  Called only from
 * ship_activity().
 *
 * Postponed for another 30 ticks if this is an NPC ship with players still
 * aboard, so nobody is destroyed with the wreck.
 *
 * Otherwise the interior is cleared, the hold is dumped, and then the two
 * kinds of ship part ways:
 *
 *   - A PLAYER ship is NOT destroyed.  Insurance is paid to the owner's bank
 *     (falling back to the auction house, and then to the ship's own coffers,
 *     so the money is never simply lost), the hull is downgraded to a sloop,
 *     and the wreck is docked in Davy Jones' locker for the owner to collect.
 *     Insurance is 90% when sunk by an NPC, 75% for a merchant, 50% for a
 *     warship, and nothing at all for a sloop.
 *   - An NPC ship is erased from the hash and deleted.
 *
 * IMPORTANT FOR CALLERS: in the NPC case this FREES the ship.  ship_activity()
 * returns immediately after calling it for exactly that reason -- see the
 * comment at that call site before changing it.
 */
void finish_sinking(P_ship ship)
{
	// The zone ship does not sink completely.
	// if( ship == zone_ship )
	//  return;

	if (IS_NPC_SHIP(ship) && pc_is_aboard(ship))
	{
		ship->timer[T_SINKING] = 30;
		return;
	}
	if (IS_WATER_ROOM(ship->location))
	{
		act_to_all_in_ship(ship, "&+yYour ship sinks and you swim out in time!\r\n");
		act_to_outside(ship, 10, "%s &+yhas sunk to the depths of the ocean!\r\n",
			       SHIP_NAME(ship));
		act_to_outside_ships(ship, ship, DEFAULT_RANGE,
				     "&+W[%s]:&N %s&N&+y sinks under the ocean.\r\n", SHIP_ID(ship),
				     SHIP_NAME(ship));
	}
	else
	{
		act_to_all_in_ship(ship, "&+yYour ship falls apart and you jump out in time!\r\n");
		act_to_outside(ship, 10, "%s &+yhas fallen to pieces!\r\n", SHIP_NAME(ship));
		act_to_outside_ships(ship, ship, DEFAULT_RANGE,
				     "&+W[%s]:&N %s&N&+y falls to pieces.\r\n", SHIP_ID(ship),
				     SHIP_NAME(ship));
	}
	clear_ship_content(ship);
	jettison_all(ship);

	if (!IS_NPC_SHIP(ship))
	{
		int insurance = 0;
		bool SunkByNPC = SHIP_SUNK_BY_NPC(ship);

		if (ship->m_class != SH_SLOOP) // no insurance for sloops
		{
			if (SunkByNPC)
			{
				// if sunk by NPC, you loose same amount as for switching hulls
				insurance = (int)(SHIPTYPE_COST(ship->m_class) * 0.90);
			}
			else if (IS_MERCHANT(ship))
			{
				insurance = (int)(SHIPTYPE_COST(ship->m_class) * 0.75);
			}
			else if (IS_WARSHIP(ship))
				insurance = (int)(SHIPTYPE_COST(ship->m_class) *
						  0.50); // only partial insurance for warships
		}

		P_char owner = get_char2(SHIP_OWNER(ship));
		int insurance_platinum = insurance / 1000;
		bool insurance_deposited = insurance_platinum == 0;
		if (owner && insurance_platinum > 0)
		{
			ship_insurance_context context = {};
			strncpy(context.owner, ship->ownername, sizeof(context.owner) - 1);
			context.pid = GET_PID(owner);
			context.platinum = insurance_platinum;
			insurance_deposited = currency_transaction_submit_bank_reward(
				owner, (int64_t)insurance_platinum * 1000,
				currency_reason_type::ship_insurance, 0,
				critical_source_site::combat, critical_deadline_class::background,
				ship_insurance_committed, &context, sizeof(context));
		}
		if (!insurance_deposited)
		{
			/* Putting this into auction house instead.
			 ship->money = insurance; // if owner is not online, money go into ships coffer
			 wizlog(56, "Ship insurance to ship's coffer: %d", insurance / 1000);
			 logit(LOG_SHIP, "%s's insurance to ship's coffer: %d", ship->ownername, insurance / 1000);
			 */
			if (!insert_money_pickup(get_player_pid_from_name(SHIP_OWNER(ship)),
						 insurance))
			{
				logit(LOG_SHIP, "%s's insurance refund failed to stage for pid %d",
				      ship->ownername, get_player_pid_from_name(SHIP_OWNER(ship)));
				ship->money += insurance;
				wizlog(56, "Ship insurance staged in ship coffers instead: %s",
				       coin_stringv(insurance));
				logit(LOG_SHIP, "%s's insurance fell back to ship coffers: %s",
				      ship->ownername, coin_stringv(insurance));
			}
			else
			{
				wizlog(56, "Ship insurance to auction house: %s",
				       coin_stringv(insurance));
				logit(LOG_SHIP, "%s's insurance to auction hourse: %s",
				      ship->ownername, coin_stringv(insurance));
			}
		}

		int old_class = ship->m_class;
		ship->m_class = SH_SLOOP; // all ships become sloops after sinking
		reset_ship(ship);

		if (old_class == SH_SLOOP || !SunkByNPC)
			ship->mainsail = 0; // have to pay at least something...

		ship->speed = 0;
		ship->setspeed = 0;

		// Holding room in Old Ship zone.
		obj_from_room(ship->shipobj);
		obj_to_room(ship->shipobj, davy_jones_locker_rnum);
		ship->location = davy_jones_locker_rnum;
		dock_ship(ship, davy_jones_locker_rnum);

		reset_crew_stamina(ship);
		update_ship_status(ship);
		queue_ship_save(ship, "sink cleanup");
	}
	else
	{
		shipObjHash.erase(ship);
		delete_ship(ship);
	}
}

/*
 * Delayed event handler: bring a summoned ship into port.
 *
 * `data` is a string of "<owner name> <real room>".  Finds that owner's ship
 * and, provided it is not at battlestations and not sinking, moves it to the
 * room and docks it there, running a contraband inspection and clearing the
 * hold on arrival.
 *
 * Silently does nothing if the ship cannot be found or is busy fighting --
 * which is the point: you cannot summon your way out of a battle.
 */
void summon_ship_event(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int to_room;
	if (sscanf((const char *)data, "%s %d", buf, &to_room) == 2)
	{
		ShipVisitor svs;
		for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
		{
			P_ship ship = svs;
			if (isname(buf, ship->ownername) && ship->timer[T_BSTATION] == 0 &&
			    !SHIP_SINKING(ship))
			{
				ship->location = to_room;
				obj_from_room(ship->shipobj);
				obj_to_room(ship->shipobj, to_room);
				send_to_room_f(to_room, "&+y%s arrives at port.\r\n&N", ship->name);
				dock_ship(ship, to_room);
				check_contraband(ship, ship->location);
				clear_cargo(ship);
				REMOVE_BIT(ship->flags, SUMMONED);
				if (IS_SET(ship->flags, ATTACKBYNPC))
					REMOVE_BIT(ship->flags, ATTACKBYNPC);
				ship->speed = 0;
				ship->setspeed = 0;
				queue_ship_save(ship, "summon arrival");
				return;
			}
		}
	}
}

/*
 * Lift `ship` off the water.
 *
 * Raises FLYING, puts the ship at altitude, and starts the levistone's
 * duration timer (LEVISTONE_TIME) unless the hull is permanently airborne.
 * The precondition checks live in order_fly() (ship_control.c).
 */
void fly_ship(P_ship ship)
{
	if (!IS_SET(ship->flags, FLYING))
		SET_BIT(ship->flags, FLYING);
	if (!IS_SET(ship->flags, AIR))
	{
		int levi_slot = eq_levistone_slot(ship);
		ship->slot[levi_slot].timer = LEVISTONE_TIME;
		act_to_all_in_ship_f(ship, "&+W%s &+Ghums and glows with soft &+Cblue light.\r\n",
				     ship->slot[levi_slot].get_description());
	}
	act_to_all_in_ship(ship, "&+WYour ship slowly ascends and floats in air!&N\r\n");
	act_to_outside(ship, 10, "%s &+Wslowly ascends and floats in air!&N", SHIP_NAME(ship));
	act_to_outside_ships(ship, ship, DEFAULT_RANGE,
			     "&+W[%s]:&N %s&N &+Wslowly ascends and floats in air!&N\r\n",
			     SHIP_ID(ship), SHIP_NAME(ship));

	ship->shipobj->z_cord = 4;
	update_ship_status(ship);
}

/*
 * Set a flying ship back down.
 *
 * Clears FLYING and returns the ship to the surface, starting the levistone's
 * recharge (LEVISTONE_RECHARGE).  If the ship is over terrain it cannot float
 * on, landing damages it -- a levistone running out over land is expensive.
 */
void land_ship(P_ship ship)
{
	if (IS_SET(ship->flags, FLYING))
		REMOVE_BIT(ship->flags, FLYING);
	if (!IS_SET(ship->flags, AIR))
	{
		int levi_slot = eq_levistone_slot(ship);
		ship->slot[levi_slot].timer = LEVISTONE_RECHARGE;
		act_to_all_in_ship_f(ship, "&+W%s &+Ldims and becomes silent.\r\n",
				     ship->slot[levi_slot].get_description());
	}
	if (IS_WATER_ROOM(ship->location))
	{
		act_to_all_in_ship(
			ship,
			"&+WYour ship slowly descends and lands with a loud &+Bsplash&+W!&N\r\n");
		act_to_outside(ship, 10,
			       "%s &+Wslowly descends and lands with a loud &+Bsplash&+W!&N",
			       SHIP_NAME(ship));
		act_to_outside_ships(
			ship, ship, DEFAULT_RANGE,
			"&+W[%s]:&N %s&N &+Wslowly descends and lands with a loud &+Bsplash&+W!&N\r\n",
			SHIP_ID(ship), SHIP_NAME(ship));
	}
	else
	{
		ship->setspeed = 0;
		ship->speed = 0;

		act_to_all_in_ship(
			ship,
			"&+WYour ship slowly descends and lands with &+Ycreaking &+Wsounds!&N\r\n");
		act_to_outside(ship, 10,
			       "%s &+Wslowly descends and lands with a &+Ycreaking &+Wsounds!&N",
			       SHIP_NAME(ship));
		act_to_outside_ships(
			ship, ship, DEFAULT_RANGE,
			"&+W[%s]:&N %s&N &+Wslowly descends and lands with a &+Ycreaking &+Wsounds!&N\r\n",
			SHIP_ID(ship), SHIP_NAME(ship));
	}

	ship->shipobj->z_cord = 0;
	update_ship_status(ship);
}

/*
 * Persist one ship immediately, through whichever backend is configured.
 *
 * Prefer queue_ship_save() in normal code: it coalesces repeated changes
 * within a tick and skips writes for ships that have not really changed.
 * Call this directly only where the write must be known to have succeeded
 * before continuing -- renames do, because they roll back on failure.
 *
 * Returns FALSE on any backend failure.
 */
int write_ship(P_ship ship)
{
	if (IS_NPC_SHIP(ship))
		return FALSE;
	if (!SHIP_LOADED(ship))
		return FALSE;

#ifndef __NO_MYSQL__
	if (!sql_save_ship(ship))
	{
		ship->save_pending = true;
		ship->save_retry_after = time(NULL) + 1;
		logit(LOG_FILE, "sql_save_ship failed for %s; will retry soon", ship->ownername);
		return FALSE;
	}
	ship->save_pending = false;
	ship->save_retry_after = 0;
	ship->save_saved_signature = ship_save_signature(ship);
	return TRUE;
#else
	const char *root = persistence_mode_flatfile_root();
	std::string error;
	flatfile_ship_record record;
	if (!root || !flat_ship_capture(ship, &record, &error) ||
	    flatfile_ship_upsert(root ? root : "", &record, &error) != flatfile_ship_result::ok)
	{
		ship->save_pending = true;
		ship->save_retry_after = time(NULL) + 1;
		logit(LOG_FILE, "flat ship save failed for %s; will retry soon: %s",
		      ship->ownername, error.empty() ? "authority failure" : error.c_str());
		return FALSE;
	}
	ship->db_id = static_cast<int>(record.ship_id);
	ship->save_pending = false;
	ship->save_retry_after = 0;
	ship->save_saved_signature = ship_save_signature(ship);
	return TRUE;
#endif
}

/*
 * Write out every ship that queue_ship_save() has marked, unless its
 * ship_save_signature() shows nothing actually changed.
 *
 * A failed save is not lost: the ship keeps its pending mark and a retry
 * gate (ShipData::save_retry_after) so it is attempted again later rather
 * than hammered every tick.
 */
void flush_pending_ship_saves(void)
{
	time_t now = time(NULL);
	int flushed = 0;
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		P_ship ship = svs;
		if (!ship || !ship->save_pending)
			continue;
		if (ship->save_retry_after && ship->save_retry_after > now)
			continue;

		unsigned long long current_signature = ship_save_signature(ship);
		if (current_signature == ship->save_saved_signature)
		{
			ship->save_pending = false;
			ship->save_retry_after = 0;
			continue;
		}

		if (write_ship(ship))
		{
			ship->save_pending = false;
			ship->save_retry_after = 0;
			ship->save_saved_signature = current_signature;
			flushed++;
			logit(LOG_DEBUG, "Recovered pending ship save for %s.", SHIP_NAME(ship));
		}
		else
		{
			ship->save_pending = true;
			ship->save_retry_after = now + 1;
		}
	}
	if (flushed > 0)
		logit(LOG_DEBUG, "Flushed %d pending ship save(s).", flushed);
}

/* Copyover cannot leave retry-delayed saves behind: the old process is about
 * to disappear. Ignore retry_after and report whether every pending save is
 * durable so the caller can abort copyover rather than lose ship state. */
bool drain_pending_ship_saves(void)
{
	bool drained = true;
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		P_ship ship = svs;
		if (!ship || !ship->save_pending)
			continue;

		if (!write_ship(ship))
		{
			drained = false;
			logit(LOG_FILE, "Unable to drain pending ship save for %s before copyover.",
			      SHIP_NAME(ship));
		}
	}
	return drained;
}

/*
 * Load every ship from persistent storage at boot.
 *
 * Dispatches to the SQL or flat-file backend.  Ships that fail validation are
 * skipped with a log line rather than aborting the load; ships flagged
 * TO_DELETE are loaded and then removed by initialize_ships().
 *
 * Returns FALSE on a backend-level failure.
 */
int read_ships()
{
#ifndef __NO_MYSQL__
	return sql_load_all_ships() ? TRUE : FALSE;
#else
	const char *root = persistence_mode_flatfile_root();
	if (!root)
	{
		logit(LOG_FILE, "flat ship load failed: state root is unavailable");
		return FALSE;
	}
	std::string error;
	std::vector<flatfile_ship_record> records;
	auto result = flatfile_ship_list(root, &records, &error);
	if (result == flatfile_ship_result::not_found)
	{
		result = flatfile_ship_import_legacy(root, "Ships", flat_ship_resolve_legacy_owner,
						     &error);
		if (result == flatfile_ship_result::not_found)
			result = flatfile_ship_establish(root, {}, &error);
		if (result == flatfile_ship_result::ok ||
		    result == flatfile_ship_result::already_exists)
			result = flatfile_ship_list(root, &records, &error);
	}
	if (result != flatfile_ship_result::ok)
	{
		logit(LOG_FILE, "flat ship load failed: %s",
		      error.empty() ? "authority failure" : error.c_str());
		return FALSE;
	}
	for (const auto &record : records)
		if (!flat_ship_record_is_loadable(record, &error))
		{
			logit(LOG_FILE, "flat ship load failed for ship %u: %s", record.ship_id,
			      error.empty() ? "invalid record" : error.c_str());
			return FALSE;
		}
	for (const auto &record : records)
		if (!flat_ship_materialize(record, &error))
		{
			logit(LOG_FILE, "flat ship load failed for ship %u: %s", record.ship_id,
			      error.empty() ? "materialization failure" : error.c_str());
			return FALSE;
		}
	return TRUE;
#endif
}

/*
 * Rebuild the shipfrags[] leaderboard: the highest-reputation ships in the
 * game, in descending order.  Called periodically; read by
 * display_shipfrags().
 */
//--------------------------------------------------------------------
// Top-frags table update
//--------------------------------------------------------------------
void update_shipfrags()
{
	for (int index = 0; index < 20; ++index)
		shipfrags[index].ship = NULL;

	ShipVisitor visitor;
	for (bool found = shipObjHash.get_first(visitor); found;
	     found = shipObjHash.get_next(visitor))
	{
		if (IS_NPC_SHIP(visitor) || visitor->frags < 0)
			continue;

		int insertion = 0;
		while (insertion < 20 && shipfrags[insertion].ship &&
		       shipfrags[insertion].ship->frags > visitor->frags)
			insertion++;
		if (insertion == 20)
			continue;
		for (int index = 19; index > insertion; --index)
			shipfrags[index] = shipfrags[index - 1];
		shipfrags[insertion].ship = visitor;
	}
}

/*
 * Print the ship frag leaderboard -- the top ships by reputation -- to `ch`.
 * Reads the shipfrags[] table that update_shipfrags() maintains.
 */
void display_shipfrags(P_char ch)
{
	send_to_char("&+L10 most dangerous ships\r\n", ch);
	send_to_char("&+L-======================================================-&N\r\n\r\n", ch);
	for (int i = 0; i < 10; i++)
	{
		if (shipfrags[i].ship == NULL)
		{
			break;
		}
		int found = 0;
		ShipVisitor svs;
		for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
		{
			if (svs == shipfrags[i].ship)
			{
				found = 1;
			}
		}
		if (!found)
		{
			break;
		}
		if (shipfrags[i].ship->frags == 0)
		{
			break;
		}
		if (i != 0)
		{
			if (shipfrags[i].ship == shipfrags[i - 1].ship)
			{
				break;
			}
		}
		send_to_char_f(
			ch,
			"&+W%d:&N %s\r\n&+LCaptain: &+W%-20s &+LClass: &+y%-15s&+R Tonnage Sunk: &+W%d&N\r\n\r\n",
			i + 1, shipfrags[i].ship->name, shipfrags[i].ship->ownername,
			SHIPTYPE_NAME(SHIP_CLASS(shipfrags[i].ship)), shipfrags[i].ship->frags);
	}
}

/*
 * Delete the ship owned by `owner_name`, if there is one.
 *
 * The by-name overload used when a player is deleted.  Removes the ship from
 * the hash before deleting it, which the P_ship overload requires of its
 * caller.  Does nothing when the owner has no ship.
 */
void delete_ship(char *owner_name)
{
	P_ship ship;
	ShipVisitor svs;

	CAP(owner_name);
	// First, we hunt for the ship, and make sure there is one (we can use the same loop as above).
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		// Skip pirate ships.
		if (SHIP_OWNER(svs) == NULL)
		{
			continue;
		}
		// Check if we have the right ship using the same code as to display the owner's name
		if (!strcmp(owner_name, SHIP_OWNER(svs)))
		{
			debug("&+RDeleting ship (%s)...&n", owner_name);
			ship = svs;
			shipObjHash.erase(svs);
			delete_ship(ship);
			return;
		}
	}
	debug("Could not find ship (%s) to delete.", owner_name);
}
