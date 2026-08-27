#!/usr/bin/env python3
"""Synthetic runtime and source contracts for linear player-item hydration."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ITEMS = (ROOT / "src/player_load_items.c").read_text()
REPOSITORY = (ROOT / "src/player_load_repository.c").read_text()
MATERIALIZE = (ROOT / "src/player_load_materialize.c").read_text()
NANNY = (ROOT / "src/nanny.c").read_text()
COPYOVER = (ROOT / "src/copyover.c").read_text()

HARNESS = r'''
#include "item_ownership_runtime.h"
#include "player_load_items.h"
#include "player_load_pets.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
size_t allocations = 0;
size_t extracts = 0;
size_t balance_calls = 0;
size_t fail_read_at = static_cast<size_t>(-1);
size_t enchant_activations = 0;
size_t pet_extracts = 0;

void release_tree(P_obj object)
{
    while (object->contains)
    {
        P_obj child = object->contains;
        object->contains = child->next_content;
        release_tree(child);
    }
    if ((object->str_mask & STRUNG_KEYS) && object->name)
        std::free(object->name);
    if ((object->str_mask & STRUNG_DESC1) && object->description)
        std::free(object->description);
    if ((object->str_mask & STRUNG_DESC2) && object->short_description)
        std::free(object->short_description);
    if ((object->str_mask & STRUNG_DESC3) && object->action_description)
        std::free(object->action_description);
    while (object->ex_description)
    {
        extra_descr_data *description = object->ex_description;
        object->ex_description = description->next;
        std::free(description->keyword);
        std::free(description->description);
        std::free(description);
    }
    ++extracts;
    std::free(object);
}

struct test_character
{
    char_data character = {};
    pc_only_data pc = {};

    explicit test_character(int pid)
    {
        character.only.pc = &pc;
        pc.pid = pid;
        character.in_room = NOWHERE;
        character.player.level = 40;
    }
};

void add_pet(player_load_result &result, uint64_t database_id, int vnum, int order = 0)
{
    player_pet_snapshot pet = {};
    pet.mob_vnum = vnum;
    pet.order = order;
    pet.hit = 10;
    pet.max_hit = 20;
    pet.mana = 3;
    pet.max_mana = 5;
    pet.vitality = 4;
    pet.max_vitality = 6;
    pet.charm_duration = 12;
    pet.room_vnum = result.snapshot.room_vnum;
    result.snapshot.pets.push_back(pet);
    result.pet_identities.push_back({ database_id, {} });
}

void add_pet_item(player_load_result &result, size_t pet_index, uint64_t database_id,
                  uint64_t uid, int vnum, int32_t parent_index, int16_t equipment_slot)
{
    player_item_snapshot item = {};
    item.parent_index = parent_index;
    item.equipment_slot = equipment_slot;
    item.object_uid = uid;
    item.vnum = vnum;
    item.weight = vnum == 100 ? 2 : 3;
    item.condition = 100;
    player_load_item_identity identity = {};
    identity.database_id = database_id;
    identity.quantity = 1;
    identity.item_uid = uid;
    identity.owner = { item_owner_type::player, static_cast<uint64_t>(result.pid), 0 };
    identity.item_revision = 1;
    identity.owner_revision = result.item_owner_revision;
    identity.state = item_custody_state::active;
    if (parent_index == PLAYER_SNAPSHOT_NO_PARENT)
        identity.root_item_uid = uid;
    else
    {
        const auto &parent =
            result.pet_identities[pet_index].item_identities[parent_index];
        identity.serialized_parent_id = parent.database_id;
        identity.parent_item_uid = parent.item_uid;
        identity.root_item_uid = parent.root_item_uid;
    }
    result.snapshot.pets[pet_index].items.push_back(item);
    result.pet_identities[pet_index].item_identities.push_back(identity);
}

player_load_result base_result(int pid = 42)
{
    player_load_result result = {};
    result.pid = pid;
    result.item_owner_revision = 7;
    return result;
}

void add_item(player_load_result &result, uint64_t database_id, uint64_t uid, int vnum,
              int32_t parent_index, int16_t equipment_slot)
{
    player_item_snapshot item = {};
    item.parent_index = parent_index;
    item.equipment_slot = equipment_slot;
    item.object_uid = uid;
    item.vnum = vnum;
    item.weight = vnum == 100 ? 2 : 3;
    item.condition = 100;
    player_load_item_identity identity = {};
    identity.database_id = database_id;
    identity.quantity = 1;
    identity.item_uid = uid;
    identity.owner = { item_owner_type::player, static_cast<uint64_t>(result.pid), 0 };
    identity.item_revision = 1;
    identity.owner_revision = result.item_owner_revision;
    identity.state = item_custody_state::active;
    if (parent_index == PLAYER_SNAPSHOT_NO_PARENT)
    {
        identity.root_item_uid = uid;
    }
    else
    {
        const size_t parent = static_cast<size_t>(parent_index);
        identity.serialized_parent_id = result.item_identities[parent].database_id;
        identity.parent_item_uid = result.item_identities[parent].item_uid;
        identity.root_item_uid = result.item_identities[parent].root_item_uid;
    }
    result.snapshot.items.push_back(item);
    result.item_identities.push_back(identity);
}

void reset_test_state()
{
    allocations = 0;
    extracts = 0;
    balance_calls = 0;
    fail_read_at = static_cast<size_t>(-1);
    enchant_activations = 0;
    pet_extracts = 0;
    item_ownership_runtime_reset();
}

void enchant_spell(int, P_char, char *, int, P_char, P_obj)
{
    ++enchant_activations;
}
}

Skill skills[MAX_AFFECT_TYPES + 1] = {};

obj_affect *get_obj_affect(P_obj object, int spell)
{
    for (obj_affect *affect = object->affects; affect; affect = affect->next)
        if (affect->type == spell)
            return affect;
    return nullptr;
}

void *__malloc(size_t size, const char *, const char *, int)
{
    return std::calloc(1, size);
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}

char *str_dup(const char *source)
{
    const size_t size = std::strlen(source) + 1;
    char *copy = static_cast<char *>(std::malloc(size));
    assert(copy);
    std::memcpy(copy, source, size);
    return copy;
}

int real_object(int vnum)
{
    return vnum == 999 ? -1 : vnum;
}

P_obj read_object(int rnum, int)
{
    if (allocations++ == fail_read_at)
        return nullptr;
    P_obj object = static_cast<P_obj>(std::calloc(1, sizeof(obj_data)));
    assert(object);
    object->R_num = rnum;
    object->loc_p = LOC_NOWHERE;
    object->loc.room = NOWHERE;
    object->type = rnum == 100 ? ITEM_CONTAINER : ITEM_OTHER;
    object->weight = rnum == 100 ? 2 : 3;
    return object;
}

int real_mobile(int vnum)
{
    return vnum == 999 ? -1 : vnum;
}

P_char read_mobile(int rnum, int)
{
    P_char pet = static_cast<P_char>(std::calloc(1, sizeof(char_data)));
    assert(pet);
    pet->only.npc = static_cast<npc_only_data *>(std::calloc(1, sizeof(npc_only_data)));
    assert(pet->only.npc);
    pet->only.npc->R_num = rnum;
    SET_BIT(pet->specials.act, ACT_ISNPC);
    pet->in_room = NOWHERE;
    return pet;
}

void extract_char(P_char pet)
{
    ++pet_extracts;
    std::free(pet->only.npc);
    std::free(pet);
}

int setup_pet(P_char, P_char, int duration, int)
{
    return duration;
}

void add_follower(P_char pet, P_char owner)
{
    follow_type *follow = static_cast<follow_type *>(std::calloc(1, sizeof(follow_type)));
    assert(follow);
    follow->follower = pet;
    follow->next = owner->followers;
    owner->followers = follow;
    pet->following = owner;
}

bool char_to_room(P_char pet, int room, int)
{
    pet->in_room = room;
    return true;
}

void extract_obj(P_obj object, int)
{
    release_tree(object);
}

bool obj_can_nest(P_obj object, P_obj parent)
{
    return object && parent && object != parent && OBJ_NOWHERE(object) &&
           (parent->type == ITEM_CONTAINER || parent->type == ITEM_QUIVER ||
            parent->type == ITEM_STORAGE || parent->type == ITEM_CORPSE);
}

void recalc_container_weight(P_obj object)
{
    if (!object || object->type != ITEM_CONTAINER)
        return;
    object->weight = 2;
    for (P_obj child = object->contains; child; child = child->next_content)
        object->weight += child->weight;
}

void balance_affects(P_char)
{
    ++balance_calls;
}

void act(const char *, int, P_char, P_obj, void *, int)
{
}

bool item_owner_identity_valid(const item_owner_identity &owner)
{
    return owner.type != item_owner_type::unknown && owner.id != 0;
}

bool item_owner_identity_equal(const item_owner_identity &left,
                               const item_owner_identity &right)
{
    return left.type == right.type && left.id == right.id &&
           left.context_id == right.context_id;
}

int main()
{
    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, 0, 0);
        add_item(result, 3, 12, 101, PLAYER_SNAPSHOT_NO_PARENT, 1);
        result.snapshot.items[1].string_mask = STRUNG_DESC2;
        result.snapshot.items[1].short_description = "saved item";
        result.snapshot.items[1].extra_descriptions.push_back(
            { "SPELLBOOK", "[1, 7, 31]", true, {} });
        player_load_item_materialize_metrics metrics = {};
        assert(player_load_items_materialize(&owner.character, result, &metrics));
        assert(metrics.outcome == player_load_item_materialize_outcome::applied);
        assert(metrics.item_count == 3 && metrics.maximum_depth == 2);
        assert(metrics.operation_count <= PLAYER_LOAD_ITEM_OPERATIONS_PER_ITEM * 3);
        assert(owner.character.carrying && owner.character.carrying->obj_uid == 10);
        assert(owner.character.carrying->contains &&
               owner.character.carrying->contains->obj_uid == 11);
        assert(owner.character.carrying->weight == 5);
        assert(owner.character.equipment[0] && owner.character.equipment[0]->obj_uid == 12);
        assert(balance_calls == 1 && item_ownership_runtime_size() == 3);
        item_ownership_runtime_entry entry = {};
        assert(item_ownership_runtime_lookup(11, &entry));
        assert(entry.parent_item_uid == 10 && entry.root_item_uid == 10);
        obj_affect enchant = {};
        enchant.type = SKILL_ENCHANT;
        enchant.data = 1;
        owner.character.equipment[0]->affects = &enchant;
        skills[1].spell_pointer = enchant_spell;
        player_load_items_activate_equipment(&owner.character);
        assert(enchant_activations == 1);
        owner.character.equipment[0]->affects = nullptr;
        release_tree(owner.character.carrying);
        release_tree(owner.character.equipment[0]);
    }

    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        player_load_item_materialize_metrics metrics = {};
        assert(player_load_items_materialize(&owner.character, result, &metrics));
        uint64_t revision = 0;
        assert(item_ownership_runtime_owner_revision(
            { item_owner_type::player, 42, 0 }, &revision));
        assert(revision == 7 && metrics.operation_count == 0);
    }

    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, 0, 0);
        fail_read_at = 1;
        player_load_item_materialize_metrics metrics = {};
        assert(!player_load_items_materialize(&owner.character, result, &metrics));
        assert(metrics.outcome == player_load_item_materialize_outcome::allocation_failure);
        assert(extracts == 1 && !owner.character.carrying &&
               item_ownership_runtime_size() == 0);
    }

    {
        reset_test_state();
        test_character owner(42);
        assert(item_ownership_runtime_hydrate_owner(
            { item_owner_type::player, 42, 0 }, 100));
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        player_load_item_materialize_metrics metrics = {};
        assert(!player_load_items_materialize(&owner.character, result, &metrics));
        assert(metrics.outcome == player_load_item_materialize_outcome::ownership_failure);
        assert(extracts == 1 && !owner.character.carrying &&
               item_ownership_runtime_size() == 0);
    }

    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        for (size_t index = 0; index < 200; ++index)
            add_item(result, index + 1, index + 10, 101,
                     PLAYER_SNAPSHOT_NO_PARENT, 0);
        player_load_item_materialize_metrics metrics = {};
        assert(player_load_items_materialize(&owner.character, result, &metrics));
        assert(metrics.item_count == 200);
        assert(metrics.operation_count <= PLAYER_LOAD_ITEM_OPERATIONS_PER_ITEM * 200);
        release_tree(owner.character.carrying);
        owner.character.carrying = nullptr;
    }

    auto invalid = [](player_load_result result) {
        reset_test_state();
        test_character owner(42);
        player_load_item_materialize_metrics metrics = {};
        assert(!player_load_items_materialize(&owner.character, result, &metrics));
        assert(!owner.character.carrying && !owner.character.equipment[0]);
    };

    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        result.item_identities[1].item_uid = 10;
        result.snapshot.items[1].object_uid = 10;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 100, 0, 0);
        result.snapshot.items[0].parent_index = 1;
        result.item_identities[0].serialized_parent_id = 2;
        result.item_identities[0].parent_item_uid = 11;
        result.item_identities[0].root_item_uid = 10;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, MAX_WEAR + 1);
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 999, PLAYER_SNAPSHOT_NO_PARENT, 0);
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        result.item_identities[0].owner.id = 99;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, 0, 0);
        result.item_identities[1].serialized_parent_id = 999;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, 0, 0);
        result.snapshot.items[1].parent_index = 99;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 1);
        add_item(result, 2, 11, 101, PLAYER_SNAPSHOT_NO_PARENT, 1);
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 1, 11, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        result.item_identities[0].quantity = 2;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        result.item_identities[0].root_item_uid = 11;
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        result.snapshot.items[0].extra_descriptions.push_back(
            { "SPELLBOOK", "[1, nope]", true, {} });
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        result.item_identities[0].override_mask = PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS;
        result.snapshot.items[0].affects[0] = { APPLY_LAST + 1, 1 };
        invalid(result);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 101, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, 0, 0);
        invalid(result);
        assert(extracts == 2);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 2, 11, 101, 0, 0);
        add_item(result, 3, 12, 101, 1, 0);
        invalid(result);
        assert(extracts == 3);
    }
    {
        player_load_result result = base_result();
        add_item(result, 1, 10, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        for (size_t index = 1; index <= PLAYER_SNAPSHOT_MAX_DEPTH; ++index)
            add_item(result, index + 1, index + 10, 100,
                     static_cast<int32_t>(index - 1), 0);
        invalid(result);
    }
    {
        player_load_result result = base_result();
        result.snapshot.items.resize(PLAYER_LOAD_ITEM_MAX + 1);
        result.item_identities.resize(PLAYER_LOAD_ITEM_MAX + 1);
        invalid(result);
    }
    {
        reset_test_state();
        test_character owner(42);
        owner.character.in_room = 5;
        player_load_result result = base_result();
        result.snapshot.room_vnum = 123;
        add_pet(result, 3001, 200);
        add_pet_item(result, 0, 3101, 20, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_pet_item(result, 0, 3102, 21, 101, 0, 0);
        std::vector<P_char> pets;
        player_load_pet_materialize_metrics metrics = {};
        assert(player_load_pets_stage(&owner.character, result, &pets, &metrics));
        assert(pets.size() == 1 && pets[0]->carrying &&
               pets[0]->carrying->contains);
        assert(item_ownership_runtime_size() == 0 && owner.character.followers == nullptr);
        player_load_pets_commit(&owner.character, &pets, result);
        assert(pets.empty() && owner.character.followers &&
               owner.character.followers->follower->in_room == NOWHERE);
        player_load_pets_place(&owner.character);
        P_char pet = owner.character.followers->follower;
        assert(pet->in_room == 5 && metrics.pet_count == 1 && metrics.item_count == 2);
        player_load_items_discard(pet);
        std::free(owner.character.followers);
        owner.character.followers = nullptr;
        extract_char(pet);
    }
    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        result.snapshot.room_vnum = 123;
        add_pet(result, 3001, 200, 0);
        add_pet(result, 3002, 999, 1);
        std::vector<P_char> pets;
        player_load_pet_materialize_metrics metrics = {};
        assert(!player_load_pets_stage(&owner.character, result, &pets, &metrics));
        assert(pets.empty() && pet_extracts == 1 && !owner.character.followers);
    }
    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        result.snapshot.room_vnum = 123;
        add_pet(result, 3001, 200);
        for (size_t index = 0; index < 300; ++index)
            add_pet_item(result, 0, 4000 + index, 1000 + index, 101,
                         PLAYER_SNAPSHOT_NO_PARENT, 0);
        std::vector<P_char> pets;
        player_load_pet_materialize_metrics metrics = {};
        assert(player_load_pets_stage(&owner.character, result, &pets, &metrics));
        assert(metrics.item_count == 300 && metrics.operation_count <=
               PLAYER_LOAD_ITEM_OPERATIONS_PER_ITEM * 300 +
                   PLAYER_LOAD_PET_OPERATIONS_PER_PET);
        player_load_pets_discard(&pets);
        assert(pet_extracts == 1 && extracts == 300);
    }
    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        result.snapshot.room_vnum = 123;
        add_pet(result, 3001, 200, 0);
        add_pet(result, 3002, 201, 0);
        std::vector<P_char> pets;
        player_load_pet_materialize_metrics metrics = {};
        assert(!player_load_pets_stage(&owner.character, result, &pets, &metrics));
        assert(pets.empty() && pet_extracts == 1);
    }
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-player-load-items-") as temp_dir:
    source = Path(temp_dir) / "player_load_items_test.cpp"
    binary = Path(temp_dir) / "player_load_items_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "src/player_load_items.c",
            "src/player_load_pets.c",
            "src/item_ownership_runtime.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=20)

for contract in (
    "std::unordered_map<uint64_t, size_t> uid_indices",
    "std::vector<std::vector<size_t>> children",
    "traversal.size() != item_count",
    "PLAYER_SNAPSHOT_MAX_DEPTH",
    "PLAYER_LOAD_ITEM_OPERATIONS_PER_ITEM",
    "item_ownership_runtime_hydrate_batch",
    "staged.published = true",
):
    assert contract in ITEMS
for contract in (
    "item_current_owner",
    "item_owner_revision",
    "ownership_summary_sql",
    "PLAYER_LOAD_SESSION02_COMPONENTS",
):
    assert contract in REPOSITORY
assert REPOSITORY.count("load_items(connection") == 1
assert "player_load_items_materialize" in MATERIALIZE
assert "request.include_items = false" in COPYOVER
rtype_zero = NANNY[NANNY.index("else if (d->rtype == 0)") :]
assert "sql_load_player_items(ch)" not in rtype_zero[:500]

print("linear player-item hydration contracts passed")
