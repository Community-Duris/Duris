#!/usr/bin/env python3
"""Runtime framing checks and source contracts for immutable world recovery."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src/world_recovery_pipeline.c").read_text()
HEADER = (ROOT / "src/world_recovery_pipeline.h").read_text()
REDIS = (ROOT / "src/redis.c").read_text()
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
#include <array>
#include <cassert>
#include <cstring>
#include <ctime>
#include <vector>
#include <zlib.h>

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

int main()
{
    world_recovery_header header = {};
    memcpy(header.magic, "WRS9", 4);
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
    blob[4] = 8;
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
    blob[WORLD_RECOVERY_WIRE_HEADER_BYTES + WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES + 4] =
        WORLD_RECOVERY_MAX_ITEM_TREE + 1;
    finish(blob, header);
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 42, nullptr));
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
            "-ffunction-sections", "-fdata-sections", "-Isrc", str(source),
            "src/world_recovery_pipeline.c", "src/world_recovery_codec.c",
            "-Wl,--gc-sections", "-lz", "-pthread",
            "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)
print("[PASS] schema, sequence, completeness, age, length, and checksum framing validates")

for token in (
    "WORLD_RECOVERY_MAX_BYTES = 64 * 1024 * 1024",
    "WORLD_RECOVERY_MAX_RECORD_BYTES = 256 * 1024",
    "WORLD_RECOVERY_MAX_FLOOR_BYTES = 16 * 1024 * 1024",
    "WORLD_RECOVERY_MAX_FLOOR_RECORDS = 32768",
    "WORLD_RECOVERY_CAPTURE_RECORD_BUDGET = 64",
    "WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC = 2000",
    "WORLD_RECOVERY_QUEUE_CAPACITY = 2",
    "WORLD_RECOVERY_MAX_RETRIES = 3",
    "WORLD_RECOVERY_MAX_ITEM_TREE = 12",
):
    assert token in HEADER
capture = section(PIPELINE, "void world_recovery_pipeline_pulse", "bool world_recovery_pipeline_take_completion")
assert "WORLD_RECOVERY_CAPTURE_RECORD_BUDGET" in capture
assert "WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC" in capture
assert "std::chrono::steady_clock::now()" in capture
worker = section(PIPELINE, "void publisher_main()", "bool capture_one_record()")
for forbidden in ("character_list", "object_list", "world[", "zone_table", "P_char", "P_obj", "copyover_write_"):
    assert forbidden not in worker
assert "publish_callback(generation.blob.data()" in worker
assert "crc32(0, generation->blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES" in PIPELINE
assert "std::vector<unsigned char> blob" not in worker
print("[PASS] bounded capture is game-thread owned and publisher traverses no live graph")

save = section(REDIS, "bool redis_save_world_state(void)", "void redis_world_recovery_pulse")
assert "fork()" not in save and "redis_floor_store_request_barrier" in save
generation_read = section(STORE, "bool redis_world_store_read_generation", "bool redis_world_store_publish")
assert "bounded_string" in generation_read and "REDIS_WORLD_GENERATION_CHUNK_BYTES" in generation_read
assert REDIS.count("redis_world_store_read_generation") == 2
initialize = section(REDIS, "bool redis_init(void)", "bool redis_clear_pwipe_state")
ensure = section(REDIS, "static bool redis_world_recovery_ensure_initialized", "static redisReply *redis_command")
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
for token in ("mud:season:%llu:%s", "world_state:writer_fence",
              "world_state:generation:", "world_state:current", "world_state:timestamp",
              "world_state:sequence", "world_state:checksum", "world_state:complete",
              "world_state:clean_shutdown", "floor_drops", "floor_drop_index"):
    assert token in REGISTRY
for token in ("redis_world_store_mark_clean_shutdown",
              "redis_world_store_consume_clean_shutdown"):
    assert token in STORE
assert publisher.index("GET %s") < publisher.index("EVAL %b 9")
assert "header.sequence == sequence" in section(REDIS, "bool redis_has_world_state", "bool redis_clear_world_state")
assert "world_recovery_restore" in section(REDIS, "bool redis_load_world_state", "void event_save_world_state")
consume = section(REDIS, "bool redis_consume_world_state", "bool redis_load_world_state")
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
    "redis_world_recovery_set_materializing(true)",
    "redis_world_recovery_set_materializing(false)",
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
assert "std::fill" in mob_capture and "equipment_vnums" in mob_capture
assert "copyover_write_mob_to_buffer" not in mob_capture
assert "mob->carrying" not in mob_capture
assert "copyover_restore_door_from_buffer" not in restore
assert "copyover_restore_zone_age_from_buffer" not in restore
print("[PASS] recovery publication is atomic and restore accepts only validated framed generations")

flush = section(REDIS, "bool redis_flush_floor_drops", "void redis_remove_floor_drop")
pulse = section(REDIS, "void redis_world_recovery_pulse", "bool redis_world_recovery_drain")
assert "redis_floor_store_submit" in flush
assert "REDIS_FLOOR_DROP_INDEX_SUFFIX" in flush
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
quiesce = section(REDIS, "bool redis_world_recovery_quiesce", "bool redis_has_world_state")
assert "world_recovery_pipeline_cancel()" in quiesce
assert "redis_floor_store_cancel()" in quiesce
assert "redis_world_writer_fence_claim()" in quiesce
assert "redis_world_store_release_fence" not in quiesce
clear = section(REDIS, "bool redis_clear_world_state", "bool redis_load_world_state")
assert clear.index("redis_world_recovery_quiesce()") < clear.index(
    "REDIS_WORLD_GENERATION_PATTERN"
) < clear.index("redis_clear_scan_match(generation_pattern)")
assert clear.index("if (!quiesced)") < clear.index("redis_clear_scan_match(generation_pattern)")
cleanup = section(REDIS, "void redis_cleanup", "void redis_clear_floor_pickups")
assert "redis_world_store_release_fence" in cleanup
assert "world_recovery_capture_forget_character(ch);" in HANDLER
assert "world_recovery_capture_forget_object(obj);" in HANDLER
assert "redis_world_recovery_pulse();" in COMM
assert "redis_world_recovery_drain(3000)" in COMM and "redis_world_recovery_drain(3000)" in COPYOVER
print("[PASS] fenced publisher owns atomic floor handoff and cancel/join lifecycle is fail closed")

print("immutable world recovery contracts passed")
