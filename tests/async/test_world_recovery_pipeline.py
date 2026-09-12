#!/usr/bin/env python3
"""Runtime framing checks and source contracts for immutable world recovery."""

from _paths import SRC, rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (SRC / "world_recovery_pipeline.c").read_text()
HEADER = (SRC / "world_recovery_pipeline.h").read_text()
REDIS = (SRC / "redis.c").read_text()
WORLD_RUNTIME = (SRC / "redis_world_runtime.c").read_text()
STORE = (SRC / "redis_world_store.c").read_text()
REGISTRY = (SRC / "redis_key_registry.def").read_text()
COMM = (SRC / "comm.c").read_text()
COPYOVER = (SRC / "copyover.c").read_text()
HANDLER = (SRC / "handler.c").read_text()
DB = (SRC / "db.c").read_text()
ARTIFACT = (SRC / "artifact.c").read_text()
WORLD_MOB_CAPTURE = PIPELINE[PIPELINE.index("case capture_stage::mobs:"):
                             PIPELINE.index("case capture_stage::objects:")]
assert "GET_MASTER(ch)" in WORLD_MOB_CAPTURE


def section(text: str, start: str, end: str) -> str:
    """Extract a bounded production source section for the compiled fixture."""
    first = text.index(start)
    return text[first:text.index(end, first)]


HARNESS = r'''
#include "world/world_recovery_pipeline.h"
#include "core/utils.h"
#include "core/prototypes.h"
#include <cstdio>
#include "world/world_recovery_codec.h"
#include "persistence/copyover.h"
#include "item/item_ownership_runtime.h"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>
#include <zlib.h>

zone_data zones[1] = {};
zone_data *zone_table = zones;
room_data rooms[2] = {};
room_data *world = rooms;
P_char character_list = nullptr;
P_char linked_pet = nullptr;
P_char linked_master = nullptr;
P_obj object_list = nullptr;
index_data mob_indexes[1] = {};
index_data object_indexes[1] = {};
P_index mob_index = mob_indexes;
P_index obj_index = object_indexes;
int top_of_world = 1;
int top_of_zone_table = -1;
bool reconcile_succeeds = true;
bool hydrate_succeeds = true;
bool materializing = false;
int objects_read = 0;
int objects_extracted = 0;
bool lookup_succeeds = false;
bool fallback_fixture = false;
item_ownership_runtime_entry lookup_entry = {};
std::vector<item_ownership_runtime_entry> hydrated_entries;

void logit(const char *, const char *, ...)
{
}

P_char get_linked_char(P_char ch, ush_int type)
{
    return ch == linked_pet && type == LNK_PET ? linked_master : nullptr;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}

int real_room(int vnum)
{
    return vnum == 100 ? 0 : vnum == 200 ? 1 : -1;
}

int real_mobile(int vnum)
{
    return vnum == 2000 ? 0 : -1;
}

int real_object(int vnum)
{
    return vnum == 1000 ? 0 : -1;
}

P_obj read_object(int vnum, int mode)
{
    if (mode == REAL ? vnum != 0 : real_object(vnum) < 0)
        return nullptr;
    P_obj object = new obj_data{};
    object->R_num = 0;
    if (fallback_fixture)
        object->extra_flags |= ITEM_ARTIFACT;
    object->loc_p = LOC_NOWHERE;
    object->next = object_list;
    object_list = object;
    ++objects_read;
    return object;
}

void obj_to_obj(P_obj object, P_obj parent)
{
    object->loc_p = LOC_INSIDE;
    object->loc.inside = parent;
    object->next_content = parent->contains;
    parent->contains = object;
    for (P_obj ancestor = parent; ancestor; ancestor =
             ancestor->loc_p == LOC_INSIDE ? ancestor->loc.inside : nullptr)
        ancestor->weight += object->weight;
}

void obj_to_room(P_obj object, int room)
{
    object->loc_p = LOC_ROOM;
    object->loc.room = room;
}

void extract_obj(P_obj object, int)
{
    while (object->contains)
    {
        P_obj child = object->contains;
        object->contains = child->next_content;
        extract_obj(child, FALSE);
    }
    if (object_list == object)
        object_list = object->next;
    else
        for (P_obj prior = object_list; prior; prior = prior->next)
            if (prior->next == object)
            {
                prior->next = object->next;
                break;
            }
    std::free(object->name);
    std::free(object->short_description);
    std::free(object->description);
    std::free(object->action_description);
    delete object;
    ++objects_extracted;
}

void extract_char(P_char)
{
}

P_char copyover_restore_mob_from_buffer(const char *, size_t, size_t *)
{
    return nullptr;
}

char *str_dup(const char *value)
{
    return strdup(value);
}

void str_free(const char *value)
{
    std::free(const_cast<char *>(value));
}

bool sql_persistence_reconcile_world_recovery_items(
    const world_recovery_authority_item *items, size_t count,
    item_ownership_runtime_entry *authoritative, size_t authoritative_capacity)
{
    if (!reconcile_succeeds || count != authoritative_capacity)
        return false;
    for (size_t index = 0; index < count; ++index)
    {
        authoritative[index] = {};
        authoritative[index].item_uid = items[index].item_uid;
        authoritative[index].root_item_uid = items[index].root_item_uid;
        authoritative[index].parent_item_uid = items[index].parent_item_uid;
        authoritative[index].vnum = items[index].vnum;
    }
    return true;
}

bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *entries, size_t count)
{
    hydrated_entries.clear();
    if (hydrate_succeeds && count)
        hydrated_entries.assign(entries, entries + count);
    return hydrate_succeeds;
}

bool world_recovery_rehydrate_npc_items(P_char const *, size_t)
{
    return true;
}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
    if (!lookup_succeeds || !entry || item_uid != lookup_entry.item_uid)
        return false;
    *entry = lookup_entry;
    return true;
}

bool item_owner_identity_equal(const item_owner_identity &left,
                               const item_owner_identity &right)
{
    return left.type == right.type && left.id == right.id &&
           left.context_id == right.context_id;
}

bool item_owner_identity_valid(const item_owner_identity &owner)
{
    if (owner.type <= item_owner_type::unknown || owner.type > item_owner_type::shopkeeper)
        return false;
    if (owner.type == item_owner_type::system || owner.type == item_owner_type::destruction)
        return owner.id == 0 && owner.context_id == 0;
    return owner.id != 0;
}

void redis_floor_runtime_set_materializing(bool active)
{
    materializing = active;
}

int copyover_write_door_to_buffer(int, int, char *, size_t)
{
    return 0;
}

int copyover_write_zone_age_to_buffer(int, char *, size_t)
{
    return 0;
}

struct publish_gate
{
    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool release = false;
    world_recovery_header header = {};
};

static bool blocked_publish(const unsigned char *, size_t,
                            const world_recovery_header *header,
                            redis_shared_command_outcome *outcome, void *raw)
{
    auto &gate = *static_cast<publish_gate *>(raw);
    std::unique_lock<std::mutex> lock(gate.mutex);
    gate.header = *header;
    gate.entered = true;
    gate.changed.notify_all();
    gate.changed.wait(lock, [&] { return gate.release; });
    *outcome = REDIS_SHARED_OUTCOME_SUCCESS;
    return true;
}

static void finish(std::vector<unsigned char>& blob, world_recovery_header& header)
{
    header.payload_size = blob.size() - WORLD_RECOVERY_WIRE_HEADER_BYTES;
    header.checksum = crc32(0, blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES,
                            header.payload_size);
    assert(world_recovery_encode_header(&header, blob.data(), blob.size()));
}

static std::vector<unsigned char> frame(world_recovery_record_type type,
                                        const unsigned char *native, size_t native_size)
{
    std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> payload = {};
    size_t payload_size = 0;
    assert(world_recovery_encode_record(type, native, native_size, payload.data(),
                                        payload.size(), &payload_size));
    std::vector<unsigned char> framed(WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES + payload_size);
    assert(world_recovery_encode_record_header(type, payload_size, framed.data(), framed.size()));
    memcpy(framed.data() + WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES, payload.data(), payload_size);
    return framed;
}

static std::vector<unsigned char> object_generation(
    const std::vector<std::vector<world_recovery_item_snapshot>>& trees)
{
    world_recovery_header header = {};
    memcpy(header.magic, "WR12", 4);
    header.schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
    header.header_size = WORLD_RECOVERY_WIRE_HEADER_BYTES;
    header.sequence = 77;
    header.timestamp = time(nullptr);
    header.object_count = trees.size();
    header.complete = 1;
    std::vector<unsigned char> blob(WORLD_RECOVERY_WIRE_HEADER_BYTES);
    for (const auto& tree : trees)
    {
        world_recovery_object_record object = {100, static_cast<uint32_t>(tree.size())};
        std::vector<unsigned char> native(sizeof(object) +
                                          tree.size() * sizeof(tree.front()));
        memcpy(native.data(), &object, sizeof(object));
        memcpy(native.data() + sizeof(object), tree.data(),
               tree.size() * sizeof(tree.front()));
        auto record = frame(world_recovery_record_type::object, native.data(), native.size());
        blob.insert(blob.end(), record.begin(), record.end());
    }
    finish(blob, header);
    return blob;
}

static world_recovery_item_snapshot item(uint64_t uid, uint64_t root, uint64_t parent)
{
    world_recovery_item_snapshot value = {};
    value.item_uid = uid;
    value.root_item_uid = root;
    value.parent_item_uid = parent;
    value.vnum = 1000;
    value.type = ITEM_CONTAINER;
    value.flags = WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED;
    if (fallback_fixture) value.extra_flags = ITEM_ARTIFACT;
    strcpy(value.name, "item");
    strcpy(value.short_description, "an item");
    strcpy(value.description, "An item is here.");
    return value;
}

int main()
{
    rooms[0].number = 100;
    rooms[1].number = 200;
    object_indexes[0].virtual_number = 1000;

    obj_data captured = {};
    captured.obj_uid = 800;
    captured.R_num = 0;
    captured.type = ITEM_CONTAINER;
    std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> capture_buffer = {};
    int captured_size = world_recovery_write_object_to_buffer(
        &captured, 100, reinterpret_cast<char *>(capture_buffer.data()),
        capture_buffer.size());
    assert(captured_size > 0);
    std::vector<unsigned char> captured_native;
    std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> captured_wire = {};
    size_t captured_wire_size = 0;
    assert(world_recovery_encode_record(
        world_recovery_record_type::object, capture_buffer.data(), captured_size,
        captured_wire.data(), captured_wire.size(), &captured_wire_size));
    assert(world_recovery_decode_record(world_recovery_record_type::object,
                                        captured_wire.data(), captured_wire_size,
                                        &captured_native));
    world_recovery_item_snapshot captured_item = {};
    memcpy(&captured_item,
           captured_native.data() + sizeof(world_recovery_object_record),
           sizeof(captured_item));
    assert(captured_item.flags == 0);

    lookup_succeeds = true;
    lookup_entry = {800, 800, 0, {item_owner_type::room, 100, 0}, 1, 1, 1000,
                    item_custody_state::active};
    captured_size = world_recovery_write_object_to_buffer(
        &captured, 100, reinterpret_cast<char *>(capture_buffer.data()),
        capture_buffer.size());
    assert(captured_size > 0);
    assert(world_recovery_encode_record(
        world_recovery_record_type::object, capture_buffer.data(), captured_size,
        captured_wire.data(), captured_wire.size(), &captured_wire_size));
    assert(world_recovery_decode_record(world_recovery_record_type::object,
                                        captured_wire.data(), captured_wire_size,
                                        &captured_native));
    memcpy(&captured_item,
           captured_native.data() + sizeof(world_recovery_object_record),
           sizeof(captured_item));
    assert(captured_item.flags == WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED);
    lookup_entry.owner = {item_owner_type::player, 1, 0};
    assert(world_recovery_write_object_to_buffer(
               &captured, 100, reinterpret_cast<char *>(capture_buffer.data()),
               capture_buffer.size()) == 0);
    lookup_succeeds = false;

    char_data master = {}, pet = {}, ordinary = {}, unlinked_summon = {};
    npc_only_data pet_npc = {}, ordinary_npc = {}, summon_npc = {};
    pet.only.npc = &pet_npc;
    ordinary.only.npc = &ordinary_npc;
    SET_BIT(pet.specials.act, ACT_ISNPC);
    SET_BIT(ordinary.specials.act, ACT_ISNPC);
    pet_npc.R_num = ordinary_npc.R_num = 0;
    pet.in_room = ordinary.in_room = 0;
    pet.next = &ordinary;
    // Provenance survives charm unlinking. Only the ordinary area NPC is saved.
    ordinary.next = &unlinked_summon;
    unlinked_summon.only.npc = &summon_npc;
    SET_BIT(unlinked_summon.specials.act, ACT_ISNPC);
    unlinked_summon.in_room = 0;
    summon_npc.R_num = 0;
    summon_npc.summoned_instance = true;
    linked_pet = &pet;
    linked_master = &master;
    character_list = &pet;

    world_recovery_header header = {};
    memcpy(header.magic, "WR12", 4);
    header.schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
    header.header_size = WORLD_RECOVERY_WIRE_HEADER_BYTES;
    header.sequence = 42;
    header.timestamp = time(nullptr);
    header.payload_size = 0;
    header.checksum = crc32(0, nullptr, 0);
    header.complete = 1;
    std::vector<unsigned char> blob(WORLD_RECOVERY_WIRE_HEADER_BYTES);
    finish(blob, header);
    world_recovery_header decoded = {};
    assert(world_recovery_validate(blob.data(), blob.size(), 300, 42, &decoded));
    assert(!world_recovery_validate(blob.data(), WORLD_RECOVERY_MAX_BYTES + 1, 300, 42, nullptr));
    assert(decoded.sequence == 42);
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 43, nullptr));
    header.complete = 0;
    finish(blob, header);
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 0, nullptr));
    header.complete = 1;
    finish(blob, header);
    blob[4] = 9;
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 0, nullptr));

    copyover_room door = {100, 1, 2};
    auto record = frame(world_recovery_record_type::door,
                        reinterpret_cast<const unsigned char *>(&door), sizeof(door));
    header.door_count = 1;
    blob.assign(WORLD_RECOVERY_WIRE_HEADER_BYTES + record.size(), 0);
    memcpy(blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES, record.data(), record.size());
    finish(blob, header);
    assert(world_recovery_validate(blob.data(), blob.size(), 300, 42, nullptr));
    header.door_count = 2;
    finish(blob, header);
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 42, nullptr));

    world_recovery_object_record object = {100, 2};
    world_recovery_item_snapshot items[2] = {};
    items[0].item_uid = 500;
    items[0].root_item_uid = 500;
    items[0].vnum = 1000;
    items[1].item_uid = 501;
    items[1].root_item_uid = 500;
    items[1].parent_item_uid = 500;
    items[1].vnum = 1001;
    std::vector<unsigned char> native(sizeof(object) + sizeof(items));
    memcpy(native.data(), &object, sizeof(object));
    memcpy(native.data() + sizeof(object), items, sizeof(items));
    record = frame(world_recovery_record_type::object, native.data(), native.size());
    header.door_count = 0;
    header.object_count = 1;
    blob.assign(WORLD_RECOVERY_WIRE_HEADER_BYTES + record.size(), 0);
    memcpy(blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES, record.data(), record.size());
    finish(blob, header);
    assert(world_recovery_validate(blob.data(), blob.size(), 300, 42, nullptr));
    const uint32_t oversized_item_count = WORLD_RECOVERY_MAX_ITEM_TREE + 1;
    const size_t item_count_offset =
        WORLD_RECOVERY_WIRE_HEADER_BYTES + WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES + 4;
    for (size_t index = 0; index < sizeof(oversized_item_count); ++index)
        blob[item_count_offset + index] = oversized_item_count >> (index * 8);
    finish(blob, header);
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 42, nullptr));

    assert(!world_recovery_capture_age_expired(WORLD_RECOVERY_CAPTURE_MAX_AGE_MSEC - 1));
    assert(world_recovery_capture_age_expired(WORLD_RECOVERY_CAPTURE_MAX_AGE_MSEC));
    assert(world_recovery_capture_age_expired(UINT64_MAX));

    publish_gate gate;
    assert(world_recovery_pipeline_init(blocked_publish, &gate));
    assert(world_recovery_pipeline_request());
    for (int pulse = 0; pulse < 8; ++pulse)
        world_recovery_pipeline_pulse();
    {
        std::unique_lock<std::mutex> lock(gate.mutex);
        assert(gate.changed.wait_for(lock, std::chrono::seconds(2),
                                     [&] { return gate.entered; }));
        assert(gate.header.mob_count == 1);
    }
    character_list = nullptr;
    linked_pet = linked_master = nullptr;
    std::atomic<bool> cancel_started = false;
    std::atomic<bool> cancel_returned = false;
    std::thread canceler([&] {
        cancel_started.store(true);
        world_recovery_pipeline_cancel();
        cancel_returned.store(true);
    });
    while (!cancel_started.load())
        std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    assert(!cancel_returned.load());
    {
        std::lock_guard<std::mutex> lock(gate.mutex);
        gate.release = true;
        gate.changed.notify_all();
    }
    canceler.join();
    assert(cancel_returned.load());
    const world_recovery_health canceled = world_recovery_pipeline_health_copy();
    assert(!canceled.initialized && !canceled.worker_running && !canceled.worker_busy);
    world_recovery_pipeline_reset_for_tests();

    const auto valid_objects = object_generation({{item(500, 500, 0)}});
    reconcile_succeeds = false;
    assert(!world_recovery_restore(valid_objects.data(), valid_objects.size(), 300, 77,
                                   nullptr));
    assert(objects_read == 0 && objects_extracted == 0 && !materializing);

    reconcile_succeeds = true;
    hydrate_succeeds = false;
    assert(!world_recovery_restore(valid_objects.data(), valid_objects.size(), 300, 77,
                                   nullptr));
    assert(objects_read == 1 && objects_extracted == 1 && object_list == nullptr &&
           !materializing);

    hydrate_succeeds = true;
    assert(world_recovery_restore(valid_objects.data(), valid_objects.size(), 300, 77,
                                  nullptr));
    assert(object_list && object_list->obj_uid == 500 && object_list->next == nullptr);
    assert(object_list->loc_p == LOC_ROOM && world[object_list->loc.room].number == 100);
    extract_obj(object_list, FALSE);

    const auto duplicates = object_generation(
        {{item(600, 600, 0)}, {item(600, 600, 0)}});
    assert(!world_recovery_restore(duplicates.data(), duplicates.size(), 300, 77, nullptr));

    auto invalid_flags_item = item(650, 650, 0);
    invalid_flags_item.flags = WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED << 1;
    const auto invalid_flags = object_generation({{invalid_flags_item}});
    assert(!world_recovery_restore(invalid_flags.data(), invalid_flags.size(), 300, 77,
                                   nullptr));

    auto type_zero_item = item(675, 675, 0);
    type_zero_item.type = 0;
    type_zero_item.flags = 0;
    const auto type_zero_object = object_generation({{type_zero_item}});
    assert(world_recovery_restore(type_zero_object.data(), type_zero_object.size(), 300, 77,
                                  nullptr));
    assert(object_list && object_list->type == 0);
    extract_obj(object_list, FALSE);

    obj_data live_root = {};
    obj_data moved_child = {};
    live_root.obj_uid = 700;
    live_root.R_num = 0;
    live_root.loc_p = LOC_ROOM;
    live_root.loc.room = 0;
    live_root.next = &moved_child;
    moved_child.obj_uid = 701;
    moved_child.R_num = 0;
    moved_child.loc_p = LOC_ROOM;
    moved_child.loc.room = 0;
    object_list = &live_root;
    const auto moved_descendant = object_generation(
        {{item(700, 700, 0), item(701, 700, 700)}});
    assert(!world_recovery_restore(moved_descendant.data(), moved_descendant.size(), 300, 77,
                                   nullptr));
    object_list = nullptr;

    // Failed Redis materialization rolls back before the forced O reset and
    // SQL fallback. The reset's owned guard must leave only the SQL instance.
    fallback_fixture = true;
    ground_artifact.vnum = 1000;
    ground_artifact.owned = true;
    ground_artifact.locType = ARTIFACT_ONGROUND;
    ground_artifact.location = 100;
    top_of_zone_table = 0;
    hydrate_succeeds = true;
    for (bool clean : {false, true})
    {
        clean_restart = clean;
        recovery_active = true;
        run_artifact_boot();
        assert(!object_list && fallback_loads == 0);
        run_recovery_boot();
        assert(!recovery_active && fallback_resets == 0 && fallback_loads == 0);
        assert(object_list && !object_list->next && object_list->obj_uid == 500);
        assert(IS_ARTIFACT(object_list) && object_list->loc_p == LOC_ROOM);
        assert(world[object_list->loc.room].number == 100);
        extract_obj(object_list, FALSE);
    }
    // Without any recovery generation, legacy boot loads the owned row once.
    run_artifact_boot();
    assert(fallback_loads == 1 && object_list && !object_list->next);
    extract_obj(object_list, FALSE);
    fallback_loads = 0;
    redis_loads = 0;
    recovery_active = true;
    hydrate_succeeds = false;
    run_artifact_boot();
    assert(!object_list && fallback_loads == 0);
    const int reads_before_fallback = objects_read;
    const int extracts_before_fallback = objects_extracted;
    run_recovery_boot();
    assert(redis_loads == 1 && fallback_resets == 1 && fallback_loads == 1);
    assert(!recovery_active && objects_read == reads_before_fallback + 3);
    assert(objects_extracted == extracts_before_fallback + 2);
    assert(object_list && !object_list->next && IS_ARTIFACT(object_list));
    assert(object_list->loc_p == LOC_ROOM && world[object_list->loc.room].number == 100);
    extract_obj(object_list, FALSE);

    // Unowned stock artifacts still load through a forced reset.
    ground_artifact.owned = false;
    reset_zone(0, 2);
    assert(object_list && !object_list->next && IS_ARTIFACT(object_list));
    extract_obj(object_list, FALSE);

    // File copyover rejects a flagged tree with missing custody records.
    {
        auto owned = item(990, 990, 0);
        world_recovery_object_record record = {100, 1};
        std::vector<char> buffer(sizeof(record) + sizeof(owned));
        std::memcpy(buffer.data(), &record, sizeof(record));
        std::memcpy(buffer.data() + sizeof(record), &owned, sizeof(owned));
        size_t consumed = 0;
        reconcile_succeeds = false;
        assert(!copyover_restore_obj_from_buffer(buffer.data(), buffer.size(), &consumed));
        assert(!object_list && consumed == 0);
        reconcile_succeeds = true;
        hydrate_succeeds = false;
        assert(!copyover_restore_obj_from_buffer(buffer.data(), buffer.size(), &consumed));
        assert(!object_list && consumed == 0);
        hydrate_succeeds = true;
    }

    // Empty snapshots retain prototype text and never claim string ownership.
    {
        obj_data plain = {};
        world_recovery_item_snapshot snapshot = {};
        replace_object_text(&plain, snapshot);
        assert(!plain.action_description && !(plain.str_mask & STRUNG_DESC3));
        plain.action_description = const_cast<char *>("prototype text");
        plain.str_mask = STRUNG_KEYS;
        char *borrowed = plain.action_description;
        replace_object_text(&plain, snapshot);
        assert(plain.action_description == borrowed && plain.str_mask == STRUNG_KEYS);
        std::strcpy(snapshot.action_description, "a cavern snake");
        replace_object_text(&plain, snapshot);
        assert(!std::strcmp(plain.action_description, "a cavern snake"));
        assert((plain.str_mask & (STRUNG_KEYS | STRUNG_DESC3)) == (STRUNG_KEYS | STRUNG_DESC3));
        std::strcpy(snapshot.action_description, "another snake");
        replace_object_text(&plain, snapshot);
        assert(!std::strcmp(plain.action_description, "another snake"));
        str_free(plain.action_description);
    }
    for (int invalid : {-1, 128, 255}) {
        auto snapshot = item(991, 991, 0);
        snapshot.material = invalid;
        auto generation = object_generation({{snapshot}});
        assert(!world_recovery_restore(generation.data(), generation.size(), 300, 77, nullptr));
        snapshot.material = 0;
        snapshot.affect_locations[0] = invalid;
        generation = object_generation({{snapshot}});
        assert(!world_recovery_restore(generation.data(), generation.size(), 300, 77, nullptr));
        assert(!object_list);
    }
    // Issue #198: capture actual runtime overrides, then exercise both adapters.
    fallback_fixture = false;
    hydrate_succeeds = true;
    reconcile_succeeds = true;
    obj_data corpse = {}, bag = {}, gloves = {}, fixed = {}, fresh = {};
    corpse.obj_uid = 900; corpse.type = ITEM_CORPSE; corpse.loc_p = LOC_ROOM;
    corpse.name = const_cast<char *>("corpse snake");
    corpse.short_description = const_cast<char *>("the corpse of a cavern snake");
    corpse.description = const_cast<char *>("The corpse of a cavern snake is lying here.");
    corpse.action_description = const_cast<char *>("a cavern snake");
    corpse.weight = 53;
    bag.obj_uid = 901; bag.type = ITEM_CONTAINER; bag.weight = 13;
    gloves.obj_uid = 902; gloves.type = ITEM_ARMOR; gloves.weight = 3;
    gloves.name = const_cast<char *>("boreal hardwood gloves");
    gloves.short_description = const_cast<char *>("some boreal hardwood gloves");
    gloves.description = const_cast<char *>("Some boreal hardwood gloves are here.");
    gloves.wear_flags = ITEM_TAKE | ITEM_WEAR_HANDS;
    gloves.extra_flags = ITEM_GLOW; gloves.anti_flags = 7; gloves.anti2_flags = 9;
    gloves.extra2_flags = 11; gloves.material = 3; gloves.cost = 1234;
    gloves.condition = 42; gloves.craftsmanship = 17; gloves.bitvector = 0x80000000UL;
    gloves.bitvector2 = 13; gloves.bitvector3 = 15; gloves.bitvector4 = 17; gloves.bitvector5 = 19;
    gloves.affected[0].location = APPLY_HIT; gloves.affected[0].modifier = -5;
    gloves.value[0] = 8; gloves.timer[0] = 12345;
    fixed.obj_uid = 903; fixed.type = ITEM_OTHER; fixed.wear_flags = 0;
    corpse.contains = &bag; bag.contains = &gloves; bag.next_content = &fixed;
    char_data mortal = {}; mortal.player.level = 20;
    auto buffer = std::vector<char>(WORLD_RECOVERY_MAX_RECORD_BYTES, static_cast<char>(0xa5));
    const char *scratch_address = buffer.data();
    for (int mode = 0; mode != 6; ++mode) {
        // 0=file copyover; 1=clean restart; 2=crash recovery (same Redis codec).
        assert(buffer.data() == scratch_address);
        const bool file_copyover = mode == 0 || mode >= 3;
        // Exercise both a ground root and nested loot with non-room custody.
        lookup_succeeds = mode >= 3;
        lookup_entry = {};
        lookup_entry.item_uid = mode == 3 ? 900 : 902;
        lookup_entry.root_item_uid = mode == 3 ? 900 : 901;
        lookup_entry.parent_item_uid = mode == 3 ? 0 : 901;
        lookup_entry.vnum = OBJ_VNUM(&corpse);
        lookup_entry.owner = {mode == 5 ? item_owner_type::room : item_owner_type::corpse,
                              mode == 5 ? 100U : 77U, 12};
        lookup_entry.state = item_custody_state::active;
        lookup_entry.item_revision = 29; lookup_entry.owner_revision = 31;
        reconcile_succeeds = !file_copyover; // Models unavailable SQL/flatfile-primary.
        if (lookup_succeeds) {
            assert(world_recovery_write_object_to_buffer(&corpse, 100, buffer.data(), buffer.size()) == 0);
        }
        int size = file_copyover ? copyover_write_obj_to_buffer(&corpse, buffer.data(), buffer.size()) :
            world_recovery_write_object_to_buffer(&corpse, 100, buffer.data(), buffer.size());
        assert(size > 0);
        P_obj restored = nullptr;
        if (file_copyover) {
            FILE *file = std::tmpfile(); assert(file);
            assert(write_obj_entry(file, &corpse, buffer)); std::rewind(file);
            size_t consumed = 99;
            assert(!copyover_restore_obj_from_buffer(buffer.data(), size - 1, &consumed));
            assert(consumed == 0 && object_list == nullptr);
            hydrate_succeeds = false;
            assert(!copyover_restore_obj_from_buffer(buffer.data(), size, &consumed));
            assert(consumed == 0 && object_list == nullptr);
            hydrate_succeeds = true;
            if (lookup_succeeds) {
                auto bad = lookup_entry;
                bad.item_uid++;
                std::memcpy(buffer.data() + size - sizeof(bad), &bad, sizeof(bad));
                assert(!copyover_restore_obj_from_buffer(buffer.data(), size, &consumed));
                assert(consumed == 0 && object_list == nullptr);
            }
            restored = read_obj_entry(file);
            assert(hydrated_entries.size() == (lookup_succeeds ? 1U : 0U));
            if (lookup_succeeds) {
                const auto &entry = hydrated_entries.front();
                assert(entry.item_uid == lookup_entry.item_uid);
                assert(entry.root_item_uid == lookup_entry.root_item_uid);
                assert(entry.parent_item_uid == lookup_entry.parent_item_uid);
                assert(item_owner_identity_equal(entry.owner, lookup_entry.owner));
                assert(entry.item_revision == 29 && entry.owner_revision == 31);
            }
            assert(std::fgetc(file) == EOF); std::fclose(file);
            FILE *truncated = std::tmpfile(); assert(truncated);
            uint32_t length = size;
            assert(std::fwrite(&length, sizeof(length), 1, truncated) == 1);
            assert(std::fwrite(buffer.data(), size - 1, 1, truncated) == 1);
            std::rewind(truncated); assert(!read_obj_entry(truncated)); std::fclose(truncated);
        } else {
            world_recovery_object_record record = {};
            std::memcpy(&record, buffer.data(), sizeof(record));
            std::vector<world_recovery_item_snapshot> tree(record.item_count);
            std::memcpy(tree.data(), buffer.data() + sizeof(record), tree.size() * sizeof(tree.front()));
            auto generation = object_generation({tree});
            clean_restart = mode == 1;
            assert(world_recovery_restore(generation.data(), generation.size(), 300, 77, nullptr));
            for (P_obj obj = object_list; obj; obj = obj->next)
                if (obj->obj_uid == 900) restored = obj;
        }
        assert(restored && restored->obj_uid == 900 && restored->weight == 53);
        assert(!std::strcmp(restored->action_description, "a cavern snake"));
        P_obj restored_bag = nullptr, restored_fixed = nullptr;
        for (P_obj obj = restored->contains; obj; obj = obj->next_content) {
            if (obj->obj_uid == 901) restored_bag = obj;
            if (obj->obj_uid == 903) restored_fixed = obj;
        }
        assert(restored_bag && restored_fixed && restored_bag->weight == 13);
        P_obj gear = restored_bag->contains;
        assert(gear && gear->obj_uid == 902 && gear->loc.inside == restored_bag);
        assert(!std::strcmp(gear->short_description, gloves.short_description));
        assert(!std::strcmp(gear->name, gloves.name));
        assert(!std::strcmp(gear->description, gloves.description));
        assert(gear->wear_flags == gloves.wear_flags && do_get_obj_is_takeable(&mortal, gear));
        assert(!do_get_obj_is_takeable(&mortal, restored_fixed));
        assert(gear->extra_flags == gloves.extra_flags && gear->anti_flags == 7 && gear->anti2_flags == 9);
        assert(gear->extra2_flags == 11 && gear->material == 3 && gear->cost == 1234);
        assert(gear->condition == 42 && gear->craftsmanship == 17 && gear->weight == 3);
        assert(gear->bitvector == gloves.bitvector && gear->bitvector2 == 13 && gear->bitvector5 == 19);
        assert(gear->affected[0].location == APPLY_HIT && gear->affected[0].modifier == -5);
        assert(gear->value[0] == 8 && gear->timer[0] == 12345);
        fresh.name = corpse.name; fresh.description = corpse.description; fresh.type = ITEM_CORPSE;
        fresh.next_content = restored;
        assert(!std::strcmp(fresh.description, restored->description));
        assert(get_obj_in_list_vis(&mortal, "1.corpse", &fresh, false) == &fresh);
        assert(get_obj_in_list_vis(&mortal, "2.corpse", &fresh, false) == restored);
        assert(!get_obj_in_list_vis(&mortal, "3.corpse", &fresh, false));
        extract_obj(restored, FALSE);
        assert(!object_list);
    }
    return 0;
}
'''


FALLBACK_SUPPORT = r'''
// The SQL fixture seeds one owned ground row shared by resets and the loader.
static arti_data ground_artifact = {};
static int fallback_loads = 0;
static int fallback_resets = 0;
static int redis_loads = 0;
static bool recovery_active = true;
static bool clean_restart = false;
static bool mini_mode = false;
static int DB = 0;
static constexpr int ARTIFACT_ONGROUND = 4;
struct MYSQL_RES { bool read = false; };
using MYSQL_ROW = char **;
static MYSQL_RES sql_result;
void qry(const char *, int location_type)
{
    assert(location_type == ARTIFACT_ONGROUND);
    ++fallback_loads;
    sql_result.read = false;
}
MYSQL_RES *mysql_store_result(int) { return &sql_result; }
int mysql_num_rows(MYSQL_RES *) { return ground_artifact.owned ? 1 : 0; }
MYSQL_ROW mysql_fetch_row(MYSQL_RES *result)
{
    static char vnum[] = "1000";
    static char room[] = "100";
    static char *row[] = {vnum, room};
    if (result->read || !ground_artifact.owned)
        return nullptr;
    result->read = true;
    return row;
}
void mysql_free_result(MYSQL_RES *) {}
bool get_artifact_data_sql(int vnum, P_arti out)
{
    assert(vnum == ground_artifact.vnum);
    *out = ground_artifact;
    return true;
}
P_obj get_obj_in_list_num(int rnum, P_obj)
{
    for (P_obj object = object_list; object; object = object->next)
        if (object->R_num == rnum && object->loc_p == LOC_ROOM)
            return object;
    return nullptr;
}
int itemvalue(P_obj) { return 0; }
bool item_load_check(P_obj, int, int) { return true; }
void reset_zone(int zone, int force_item_repop)
{
    assert(zone == 0 && force_item_repop == 2);
    ++fallback_resets;
    struct { char command; int arg1, arg2, arg3, arg4; } command = {'O', 0, 1, 0, 100};
    P_obj obj = nullptr;
    arti_data artidata = {};
    const int respawn = 1;
    int ival = 0;
    int last_cmd = 0;
#define ZCMD command
    switch (command.command)
    {
@RESET_OBJECT@
    }
#undef ZCMD
    (void)last_cmd;
}
@GROUND_LOADER@
int is_copyover_boot() { return 0; }
void addOnMobArtis_sql() {}
void run_artifact_boot();
bool redis_world_recovery_boot_active() { return recovery_active; }
bool redis_world_clean_restart_boot() { return clean_restart; }
bool redis_load_world_state()
{
    ++redis_loads;
    const auto generation = object_generation({{item(500, 500, 0)}});
    return world_recovery_restore(generation.data(), generation.size(), 300, 77, nullptr);
}
void copyover_restore_combat() {}
void calc_zone_mob_level() {}
bool redis_consume_world_state() { return true; }
bool load_moonstone_fragments() { return true; }
void redis_world_recovery_boot_clear() { recovery_active = false; }
nevent_periodic_result nevent_periodic_set_enabled(const char *, bool, int)
{
    return nevent_periodic_result::enabled;
}
void run_artifact_boot()
{
@ARTIFACT_BOOT@
}
void run_recovery_boot()
{
@RECOVERY_BOOT@
}
'''
FALLBACK_SUPPORT = FALLBACK_SUPPORT.replace(
    "@RESET_OBJECT@", section(DB, "\t\t\tcase 'O': /* load an object to room */", "\t\t\tcase 'P':")
).replace(
    "@GROUND_LOADER@", section(ARTIFACT, "void addOnGroundArtis_sql()", "// This function either finds")
).replace(
    "@RECOVERY_BOOT@", section(COMM, "\t// redis crash recovery", "\tPROFILES(RESET);")
).replace(
    "@ARTIFACT_BOOT@", section(DB, "\t\t// skip loading artifacts from db during copyover", "\n\t}\n\telse")
)
HARNESS = HARNESS.replace("int main()", FALLBACK_SUPPORT + "\nint main()", 1)


COPYOVER_HELPERS = section(COPYOVER, "int copyover_write_obj_to_buffer", "int copyover_write_door_to_buffer") + section(COPYOVER, "P_obj copyover_restore_obj_from_buffer", "int copyover_restore_door_from_buffer") + section(COPYOVER, "static int write_obj_entry", "// raw write to socket fd")
TAKEABILITY = section((SRC / "actobj.c").read_text(), "static bool do_get_obj_is_takeable", "static bool do_get_container_item_is_takeable")
SELECTOR = section(HANDLER, "P_obj get_obj_in_list_vis", "/*\n * search the entire world for an object")
SELECTOR_STUBS = r'''
bool ac_can_see_obj(P_char, P_obj, int) { return true; }
bool isname(const char *name, const char *keywords) {
    return keywords && std::strstr(keywords, name);
}
int get_number(char **name) {
    char *dot = std::strchr(*name, '.');
    if (!dot) return 1;
    const int ordinal = std::atoi(*name);
    *name = dot + 1;
    return ordinal;
}
'''
TEXT_RESTORE = section(PIPELINE, "void replace_object_text", "P_obj materialize_object")
HARNESS = HARNESS.replace("int main()", TEXT_RESTORE + COPYOVER_HELPERS + TAKEABILITY + SELECTOR_STUBS + SELECTOR + "\nint main()", 1)

with tempfile.TemporaryDirectory(prefix="duris-world-recovery-") as temp_dir:
    source = Path(temp_dir) / "world_recovery_test.cpp"
    binary = Path(temp_dir) / "world_recovery_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-ffunction-sections", "-fdata-sections", "-Isrc", str(source),
            rel("world_recovery_pipeline.c"), rel("world_recovery_codec.c"),
            rel("redis_command_observability.c"),
            "-Wl,--gc-sections", "-lz", "-pthread",
            "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)
print("[PASS] schema, sequence, completeness, age, length, and checksum framing validates")
print("[PASS] in-flight publication joins before pwipe deletion can continue")
print("[PASS] duplicate/moved items and custody failures fail closed with rollback")
print("[PASS] failed recovery and forced zone reset restore exactly one owned ground artifact")

for token in (
    "WORLD_RECOVERY_MAX_BYTES = 64 * 1024 * 1024",
    "WORLD_RECOVERY_MAX_RECORD_BYTES = 512 * 1024",
    "WORLD_RECOVERY_MAX_FLOOR_BYTES = 16 * 1024 * 1024",
    "WORLD_RECOVERY_MAX_FLOOR_RECORDS = 32768",
    "WORLD_RECOVERY_CAPTURE_RECORD_BUDGET = 1024",
    "WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC = 2000",
    "WORLD_RECOVERY_CAPTURE_MAX_AGE_MSEC = 300000",
    "WORLD_RECOVERY_QUEUE_CAPACITY = 2",
    "WORLD_RECOVERY_MAX_RETRIES = 3",
    "WORLD_RECOVERY_MAX_ITEM_TREE = 512",
    "WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED",
):
    assert token in HEADER
capture = section(PIPELINE, "void world_recovery_pipeline_pulse", "bool world_recovery_pipeline_take_completion")
assert "WORLD_RECOVERY_CAPTURE_RECORD_BUDGET" in capture
assert "WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC" in capture
assert "std::chrono::steady_clock::now()" in capture
assert "world_recovery_capture_age_expired" in capture
assert "fail_capture(true)" in capture
assert "PC_CORPSE" in PIPELINE
assert "item_ownership_runtime_lookup" in PIPELINE
failure = section(PIPELINE, "void fail_capture(bool expired)", "bool submit_capture()")
for token in (
    "capture_failure_completion = { active_capture.generation.sequence, false, 0 }",
    "capture_failure_pending = true",
    "++health.capture_expirations",
    "health.last_capture_duration_msec",
    "active_capture.generation.blob.size()",
):
    assert token in failure
completion = section(
    PIPELINE, "bool world_recovery_pipeline_take_completion", "bool world_recovery_pipeline_drain"
)
assert "capture_failure_pending" in completion
busy = section(PIPELINE, "bool world_recovery_pipeline_busy", "bool world_recovery_capture_age_expired")
assert "capture_failure_pending" in busy
worker = section(PIPELINE, "void publisher_main()", "bool capture_one_record()")
for forbidden in ("character_list", "object_list", "world[", "zone_table", "P_char", "P_obj", "copyover_write_"):
    assert forbidden not in worker
assert "publish_callback(generation.blob.data()" in worker
assert "&outcome, publish_context" in worker
assert "redis_worker_operation_record" in worker
assert "health.publish_operations" in worker
assert "redis_worker_operation_prepare_snapshot(&snapshot.publish_operations)" in PIPELINE
assert "crc32(0, generation->blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES" in PIPELINE
assert "std::vector<unsigned char> blob" not in worker
print("[PASS] bounded capture is game-thread owned and publisher traverses no live graph")

save = section(WORLD_RUNTIME, "bool redis_save_world_state(void)", "void redis_world_recovery_pulse")
assert "fork()" not in save and "redis_floor_store_request_barrier" in save
generation_read = section(STORE, "bool redis_world_store_read_generation", "bool redis_world_store_publish")
assert "bounded_string" in generation_read and "REDIS_WORLD_GENERATION_CHUNK_BYTES" in generation_read
assert WORLD_RUNTIME.count("redis_world_store_read_generation") == 2
initialize = section(WORLD_RUNTIME, "bool redis_world_runtime_start", "void redis_world_runtime_shutdown")
ensure = section(WORLD_RUNTIME, "bool redis_world_recovery_ensure_initialized", "void redis_clear_floor_pickups")
assert "redis_world_writer_fence_claim()" in initialize
assert "redis_world_writer_fence_claim()" not in ensure
assert "world_sequence_floor" in ensure
assert "world_recovery_pipeline_set_sequence_floor(world_sequence_floor)" in ensure
publisher = STORE[STORE.index("bool redis_world_store_publish"):]
for token in (
    "WORLD_PUBLISH_SCRIPT",
    "EVAL %b 9",
    "size > WORLD_RECOVERY_MAX_BYTES",
    "REDIS_WORLD_GENERATION_CHUNK_BYTES",
    "REDIS_WORLD_GENERATION_MANIFEST_BYTES",
    "SET %s %b EX %llu",
    "reply->type == REDIS_REPLY_INTEGER && reply->integer == 1",
):
    assert token in publisher
for token in (
    "redis.call('GET',KEYS[1])~=ARGV[1]",
    "current~=ARGV[2]",
    "redis.call('SET',KEYS[3],ARGV[3])",
    "redis.call('EXPIRE',KEYS[3],ARGV[8])",
    "redis.call('SET',KEYS[2],ARGV[4])",
    "redis.call('DEL',KEYS[8],KEYS[9])",
    "redis.call('PEXPIRE',KEYS[1],ARGV[7])",
):
    assert token in STORE
for token in ("WRG2", "HMAC(EVP_sha256()", "SHA256(", "CRYPTO_memcmp"):
    assert token in STORE
for token in ("SEASON_INFIX", "world_state:writer_fence",
              "world_state:generation:", "world_state:current", "world_state:timestamp",
              "world_state:sequence", "world_state:checksum", "world_state:complete",
              "world_state:clean_shutdown", "floor_drops", "floor_drop_index"):
    assert token in REGISTRY
for token in ("redis_world_store_mark_clean_shutdown",
              "redis_world_store_consume_clean_shutdown"):
    assert token in STORE
assert publisher.index("GET %s") < publisher.index("EVAL %b 9")
assert "header.sequence == sequence" in section(WORLD_RUNTIME, "bool redis_has_world_state", "bool redis_consume_world_state")
assert "world_recovery_restore" in section(WORLD_RUNTIME, "bool redis_load_world_state", "void event_save_world_state")
consume = section(WORLD_RUNTIME, "bool redis_consume_world_state", "bool redis_load_world_state")
assert "redis_world_recovery_quiesce" not in consume
assert "redis_world_store_consume_generation" in consume
assert "redis.call('GET',KEYS[1])~=ARGV[1]" in section(
    STORE, "bool redis_world_store_consume_generation", "bool redis_world_store_publish"
)
assert "redis_consume_world_state()" in COMM
assert "redis_clear_world_state();" not in section(
    COMM, "// redis crash recovery - restore world state from redis snapshot", "PROFILES(RESET)"
)
restore = section(PIPELINE, "bool world_recovery_restore", "void world_recovery_capture_forget_character")
transactional_restore = section(
    PIPELINE, "bool world_recovery_restore_with_floor", "bool world_recovery_restore("
)
for token in (
    "build_recovery_plan",
    "add_object_record",
    "sql_persistence_reconcile_world_recovery_items",
    "materialize_plan",
    "redis_floor_runtime_set_materializing(true)",
    "redis_floor_runtime_set_materializing(false)",
):
    assert token in transactional_restore
assert transactional_restore.index("build_recovery_plan") < transactional_restore.index(
    "sql_persistence_reconcile_world_recovery_items"
) < transactional_restore.index("materialize_plan")
for token in (
    "world_recovery_item_snapshot",
    "root_item_uid",
    "parent_item_uid",
    "existing_tree_matches",
    "rollback_materialized",
    "extract_obj(*item, FALSE)",
    "extract_char(*mob)",
):
    assert token in PIPELINE
mob_capture = section(PIPELINE, "int write_mob_record", "void publisher_main")
assert "entry.num_carrying = 0" in mob_capture
assert "entry.gold = 0" in mob_capture
assert "GET_GOLD(mob)" not in mob_capture
assert "std::fill" in mob_capture and "equipment_vnums" in mob_capture
assert "copyover_write_mob_to_buffer" not in mob_capture
assert "mob->carrying" not in mob_capture
assert "copyover_restore_door_from_buffer" not in restore
assert "copyover_restore_zone_age_from_buffer" not in restore
print("[PASS] recovery publication is atomic and restore accepts only validated framed generations")

FLOOR_RUNTIME = (SRC / "redis_floor_runtime.c").read_text(encoding="ascii")
flush = section(FLOOR_RUNTIME, "bool redis_flush_floor_drops", "void redis_remove_floor_drop")
pulse = section(WORLD_RUNTIME, "void redis_world_recovery_pulse", "bool redis_world_recovery_drain")
assert "redis_floor_store_submit" in flush
assert "floor_key" in flush and "floor_index_key" in flush
assert "REDIS_FLOOR_DROP_INDEX_SUFFIX" in FLOOR_RUNTIME
assert "world_recovery_floor_ack_pending" not in flush
assert "redis_append_command" not in flush and "redis_collect_integer_replies" not in flush
assert flush.count("redis_command") == 0
assert "redis_floor_store_take_barrier" in pulse
assert "world_recovery_pipeline_request" in pulse
assert "redis_floor_store_resume" in pulse
assert "redis_clear_floor_drops_checked()" not in pulse
cancel = section(PIPELINE, "void world_recovery_pipeline_cancel", "bool world_recovery_pipeline_request")
for token in ("stop_requested = true", "queued.clear()", "completions.clear()",
              "active_capture = {}", "publisher_worker.join()"):
    assert token in cancel
quiesce = section(WORLD_RUNTIME, "bool redis_world_recovery_quiesce", "bool redis_has_world_state")
assert "world_recovery_pipeline_cancel()" in quiesce
assert "redis_floor_store_cancel()" in quiesce
assert "redis_world_writer_fence_claim()" in quiesce
assert "redis_world_store_release_fence" not in quiesce
pwipe = section(REDIS, "bool redis_clear_pwipe_state", "bool redis_validate_pwipe_state")
assert pwipe.index("redis_world_recovery_quiesce()") < pwipe.index(
    "redis_maintenance_clear(&config)"
)
assert "redis_clear_world_state" not in WORLD_RUNTIME
cleanup = section(WORLD_RUNTIME, "void redis_world_runtime_shutdown", "bool redis_world_runtime_enabled")
assert "redis_world_store_release_fence" in cleanup
assert "world_recovery_capture_forget_character(ch);" in HANDLER
assert "world_recovery_capture_forget_object(obj);" in HANDLER
assert "redis_world_recovery_pulse();" in COMM
assert "redis_world_recovery_drain(3000)" in COMM and "redis_world_recovery_drain(3000)" in COPYOVER
print("[PASS] fenced publisher owns atomic floor handoff and cancel/join lifecycle is fail closed")

print("[PASS] file/Redis nested corpse metadata, mortal takeability, and ordinal selection")
print("immutable world recovery contracts passed")
