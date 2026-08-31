#!/usr/bin/env python3
"""Runtime framing checks and source contracts for immutable world recovery."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src/world_recovery_pipeline.c").read_text()
HEADER = (ROOT / "src/world_recovery_pipeline.h").read_text()
REDIS = (ROOT / "src/redis.c").read_text()
WORLD_RUNTIME = (ROOT / "src/redis_world_runtime.c").read_text()
STORE = (ROOT / "src/redis_world_store.c").read_text()
REGISTRY = (ROOT / "src/redis_key_registry.def").read_text()
COMM = (ROOT / "src/comm.c").read_text()
COPYOVER = (ROOT / "src/copyover.c").read_text()
HANDLER = (ROOT / "src/handler.c").read_text()


def section(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first:text.index(end, first)]


HARNESS = r'''
#include "world_recovery_pipeline.h"
#include "world_recovery_codec.h"
#include "copyover.h"
#include "item_ownership_runtime.h"
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
item_ownership_runtime_entry lookup_entry = {};

void logit(const char *, const char *, ...)
{
}

P_char get_linked_char(P_char, ush_int)
{
    return nullptr;
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

P_obj read_object(int vnum, int)
{
    if (real_object(vnum) < 0)
        return nullptr;
    P_obj object = new obj_data{};
    object->R_num = 0;
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
}

void obj_to_room(P_obj object, int room)
{
    object->loc_p = LOC_ROOM;
    object->loc.room = room;
}

void extract_obj(P_obj object, int)
{
    while (object->contains)
        extract_obj(object->contains, FALSE);
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

bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *, size_t)
{
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
};

static bool blocked_publish(const unsigned char *, size_t,
                            const world_recovery_header *,
                            redis_shared_command_outcome *outcome, void *raw)
{
    auto &gate = *static_cast<publish_gate *>(raw);
    std::unique_lock<std::mutex> lock(gate.mutex);
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
    memcpy(header.magic, "WR11", 4);
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

    world_recovery_header header = {};
    memcpy(header.magic, "WR11", 4);
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
    world_recovery_pipeline_pulse();
    {
        std::unique_lock<std::mutex> lock(gate.mutex);
        assert(gate.changed.wait_for(lock, std::chrono::seconds(2),
                                     [&] { return gate.entered; }));
    }
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
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-world-recovery-") as temp_dir:
    source = Path(temp_dir) / "world_recovery_test.cpp"
    binary = Path(temp_dir) / "world_recovery_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-ffunction-sections", "-fdata-sections", "-Isrc", str(source),
            "src/world_recovery_pipeline.c", "src/world_recovery_codec.c",
            "src/redis_command_observability.c",
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

for token in (
    "WORLD_RECOVERY_MAX_BYTES = 64 * 1024 * 1024",
    "WORLD_RECOVERY_MAX_RECORD_BYTES = 256 * 1024",
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

FLOOR_RUNTIME = (ROOT / "src/redis_floor_runtime.c").read_text(encoding="ascii")
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

print("immutable world recovery contracts passed")
