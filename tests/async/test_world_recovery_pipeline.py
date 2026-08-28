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
COMM = (ROOT / "src/comm.c").read_text()
COPYOVER = (ROOT / "src/copyover.c").read_text()
HANDLER = (ROOT / "src/handler.c").read_text()


def section(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first:text.index(end, first)]


HARNESS = r'''
#include "world_recovery_pipeline.h"
#include "copyover.h"
#include <cassert>
#include <cstring>
#include <ctime>
#include <vector>
#include <zlib.h>

int main()
{
    world_recovery_header header = {};
    memcpy(header.magic, "WRS7", 4);
    header.schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
    header.header_size = sizeof(header);
    header.sequence = 42;
    header.timestamp = time(nullptr);
    header.payload_size = 0;
    header.checksum = crc32(0, nullptr, 0);
    header.complete = 1;
    std::vector<unsigned char> blob(sizeof(header));
    memcpy(blob.data(), &header, sizeof(header));
    world_recovery_header decoded = {};
    assert(world_recovery_validate(blob.data(), blob.size(), 300, 42, &decoded));
    assert(!world_recovery_validate(blob.data(), WORLD_RECOVERY_MAX_BYTES + 1, 300, 42, nullptr));
    assert(decoded.sequence == 42);
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 43, nullptr));
    header.complete = 0;
    memcpy(blob.data(), &header, sizeof(header));
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 0, nullptr));
    header.complete = 1;
    header.schema_version++;
    memcpy(blob.data(), &header, sizeof(header));
    assert(!world_recovery_validate(blob.data(), blob.size(), 300, 0, nullptr));

    struct framed_record { uint32_t size; uint8_t type; uint8_t reserved[3]; } record = {};
    copyover_room door = {100, 1, 2};
    record.size = sizeof(door);
    record.type = 3;
    header.schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
    header.payload_size = sizeof(record) + sizeof(door);
    header.door_count = 1;
    blob.resize(sizeof(header) + header.payload_size);
    memcpy(blob.data() + sizeof(header), &record, sizeof(record));
    memcpy(blob.data() + sizeof(header) + sizeof(record), &door, sizeof(door));
    header.checksum = crc32(0, blob.data() + sizeof(header), header.payload_size);
    memcpy(blob.data(), &header, sizeof(header));
    assert(world_recovery_validate(blob.data(), blob.size(), 300, 42, nullptr));
    header.door_count = 2;
    memcpy(blob.data(), &header, sizeof(header));
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
            "src/world_recovery_pipeline.c", "-Wl,--gc-sections", "-lz", "-pthread",
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
    "WORLD_RECOVERY_CAPTURE_RECORD_BUDGET = 64",
    "WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC = 2000",
    "WORLD_RECOVERY_QUEUE_CAPACITY = 2",
    "WORLD_RECOVERY_MAX_RETRIES = 3",
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
assert "crc32(0, generation.blob.data() + sizeof(header)" in worker
assert "std::vector<unsigned char> blob" not in worker
print("[PASS] bounded capture is game-thread owned and publisher traverses no live graph")

save = section(REDIS, "bool redis_save_world_state(void)", "void redis_world_recovery_pulse")
assert "fork()" not in save and "redis_floor_store_request_barrier" in save
bounded_get = section(REDIS, "static redisReply *redis_get_bounded_string", "static void redis_prime_artifact_caches")
assert "STRLEN" in bounded_get and "WORLD_RECOVERY_MAX_BYTES" in REDIS
assert REDIS.count("redis_get_bounded_string(redis_ctx, generation_key") == 2
initialize = section(REDIS, "bool redis_init(void)", "bool redis_clear_pwipe_state")
ensure = section(REDIS, "static bool redis_world_recovery_ensure_initialized", "static redisReply *redis_command")
assert "redis_world_writer_fence_claim()" in initialize
assert "redis_world_writer_fence_claim()" not in ensure
publisher = STORE[STORE.index("bool redis_world_store_publish"):]
for token in (
    "WORLD_PUBLISH_SCRIPT",
    "EVAL %b 8",
    "size > WORLD_RECOVERY_MAX_BYTES",
    "assumed_bytes_per_second",
    "maximum_publish_timeout_msec",
    "reply->type == REDIS_REPLY_INTEGER && reply->integer == 1",
):
    assert token in publisher
for token in (
    "redis.call('GET',KEYS[1])~=ARGV[1]",
    "current~=ARGV[2]",
    "redis.call('SET',KEYS[3],ARGV[3])",
    "redis.call('EXPIRE',KEYS[3],ARGV[8])",
    "redis.call('SET',KEYS[2],ARGV[4])",
    "redis.call('DEL',KEYS[8])",
    "redis.call('PEXPIRE',KEYS[1],ARGV[7])",
):
    assert token in STORE
for token in ("mud:season:%llu:%s", "world_state:writer_fence",
              "world_state:generation:", "world_state:current", "world_state:timestamp",
              "world_state:sequence", "world_state:checksum", "world_state:complete",
              "floor_drops"):
    assert token in STORE
assert publisher.index("GET %s") < publisher.index("EVAL %b 8")
assert "header.sequence == sequence" in section(REDIS, "bool redis_has_world_state", "time_t redis_world_state_timestamp")
assert "world_recovery_restore" in section(REDIS, "bool redis_load_world_state", "void event_save_world_state")
restore = section(PIPELINE, "bool world_recovery_restore", "void world_recovery_capture_forget_character")
assert restore.count("copyover_restore_door_from_buffer") == 1 and ") < 0" in restore
assert restore.count("copyover_restore_zone_age_from_buffer") == 1
print("[PASS] recovery publication is atomic and restore accepts only validated framed generations")

flush = section(REDIS, "bool redis_flush_floor_drops", "void redis_remove_floor_drop")
pulse = section(REDIS, "void redis_world_recovery_pulse", "bool redis_world_recovery_drain")
assert "redis_floor_store_submit" in flush
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
    '"world_state:generation:*"'
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
