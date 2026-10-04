#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_movement_transaction.h"
#include "magic/spells.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

room_data rooms[2] = {};
P_room world = rooms;
extern const int top_of_world = 1;
index_data templates[2] = {};
P_index obj_index = templates;
P_index mob_index = templates;
P_obj object_list = nullptr;
P_char character_list = nullptr;

static bool accounting_active = false;
static bool consent = true;
static int submitted = 0;
static int unequipped = 0;
static int carried_removed = 0;
static int room_placed = 0;
static int messages = 0;
static pc_only_data caster_pc = {}, victim_pc = {};
static char_data caster = {}, victim = {};
static obj_data worn = {}, held = {};

bool economic_gameplay_authority::active() { return accounting_active; }
bool is_linked_to(P_char, P_char, ush_int) { return consent; }
bool affected_by_spell(P_char, int) { return false; }
void affect_from_char(P_char, int) {}
void spell_dispel_magic(int, P_char, char *, int, P_char, P_obj) {}
void send_to_char(const char *, P_char) { ++messages; }
void act(const char *, int, P_char, P_obj, void *, int) { ++messages; }
void wizlog(int, const char *, ...) {}
void logit(const char *, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }

P_obj unequip_char(P_char player, int slot, bool)
{
    ++unequipped;
    P_obj object = player->equipment[slot];
    assert(object && OBJ_WORN_BY(object, player));
    player->equipment[slot] = nullptr;
    object->loc_p = LOC_NOWHERE;
    object->loc.wearing = nullptr;
    return object;
}
void obj_from_char(P_obj object)
{
    ++carried_removed;
    assert(OBJ_CARRIED_BY(object, &victim));
    assert(victim.carrying == object);
    victim.carrying = object->next_content;
    object->next_content = nullptr;
    object->loc_p = LOC_NOWHERE;
    object->loc.carrying = nullptr;
}
void obj_to_room(P_obj object, int room)
{
    ++room_placed;
    assert(OBJ_NOWHERE(object));
    object->loc_p = LOC_ROOM;
    object->loc.room = room;
}

bool item_movement_transaction_submit(
    P_char actor, P_obj object, P_obj, const item_owner_identity &source,
    const item_owner_identity &destination, item_transfer_reason reason, int64_t reason_id,
    item_movement_completion_fn, const void *, size_t, P_obj,
    item_movement_reject *, item_movement_publication_fn,
    economic_source_kind, uint64_t, const item_transfer_continuation &)
{
    assert(actor == &victim && object == &worn);
    assert(source.type == item_owner_type::player && source.id == 42);
    assert(destination.type == item_owner_type::room && destination.id == 100);
    assert(reason == item_transfer_reason::player_drop && reason_id == 100);
    assert(victim.equipment[WIELD] == &worn && OBJ_WORN_BY(&worn, &victim));
    ++submitted;
    return true;
}

static void reset()
{
    caster_pc = {};
    victim_pc = {};
    caster_pc.pid = 43;
    victim_pc.pid = 42;
    caster = {};
    victim = {};
    caster.only.pc = &caster_pc;
    victim.only.pc = &victim_pc;
    caster.player.level = victim.player.level = 20;
    caster.in_room = victim.in_room = 0;
    rooms[0].number = 100;
    rooms[1].number = 200;
    worn = {};
    held = {};
    worn.obj_uid = 9001;
    held.obj_uid = 9002;
    worn.loc_p = LOC_WORN;
    worn.loc.wearing = &victim;
    held.loc_p = LOC_CARRIED;
    held.loc.carrying = &victim;
    SET_BIT(worn.extra_flags, ITEM_NODROP);
    SET_BIT(held.extra_flags, ITEM_NODROP);
    victim.equipment[WIELD] = &worn;
    victim.carrying = &held;
    object_list = &worn;
    worn.next = &held;
    character_list = &caster;
    caster.next = &victim;
    accounting_active = false;
    consent = true;
    submitted = unequipped = carried_removed = room_placed = messages = 0;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    reset();
    const std::string selected = argv[1];
    if (selected == "inactive_selection")
    {
        spell_remove_curse(30, &caster, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
        assert(unequipped == 1 && carried_removed == 1 && room_placed == 2);
        assert(submitted == 0 && OBJ_ROOM(&worn) && OBJ_ROOM(&held));
        assert(IS_SET(worn.extra_flags, ITEM_NODROP) && IS_SET(held.extra_flags, ITEM_NODROP));
    }
    else if (selected == "active_cursed_selection")
    {
        accounting_active = true;
        spell_remove_curse(30, &caster, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
        assert(submitted == 1 && "consenting cursed target must enter the typed drop owner");
        assert(unequipped == 0 && carried_removed == 0 && room_placed == 0);
        assert(OBJ_WORN_BY(&worn, &victim) && OBJ_CARRIED_BY(&held, &victim));
        assert(IS_SET(worn.extra_flags, ITEM_NODROP) && IS_SET(held.extra_flags, ITEM_NODROP));
    }
    else if (selected == "no_consent")
    {
        consent = false;
        spell_remove_curse(30, &caster, nullptr, SPELL_TYPE_SPELL, &victim, nullptr);
        assert(unequipped == 0 && carried_removed == 0 && room_placed == 0 && submitted == 0);
    }
    else if (selected == "object_uncurse")
    {
        spell_remove_curse(30, &caster, nullptr, SPELL_TYPE_SPELL, nullptr, &held);
        assert(!IS_SET(held.extra_flags, ITEM_NODROP));
        assert(OBJ_CARRIED_BY(&held, &victim) && room_placed == 0 && submitted == 0);
    }
    else
        assert(false && "unknown native case");
}
