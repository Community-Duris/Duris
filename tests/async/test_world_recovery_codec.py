#!/usr/bin/env python3
"""Golden vectors and round trips for the schema-10 recovery wire format."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src" / "world_recovery_pipeline.c").read_text(encoding="ascii")
FLOOR = (ROOT / "src" / "redis_floor_store.c").read_text(encoding="ascii")

HARNESS = r'''
#include "copyover.h"
#include "world_recovery_codec.h"

#include <array>
#include <cassert>
#include <cstring>
#include <vector>

int main()
{
    world_recovery_header header = {};
    header.sequence = 0x0102030405060708ULL;
    header.timestamp = -2;
    header.payload_size = 0x1112131415161718ULL;
    header.checksum = 0xa1b2c3d4U;
    header.mob_count = 1;
    header.object_count = 2;
    header.door_count = 3;
    header.zone_count = 4;
    header.complete = 1;
    const std::array<unsigned char, WORLD_RECOVERY_WIRE_HEADER_BYTES> expected_header = {
        0x57,0x52,0x31,0x30, 0x0a,0x00,0x00,0x00, 0x40,0x00,0x00,0x00,
        0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,
        0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,
        0xd4,0xc3,0xb2,0xa1,
        0x01,0x00,0x00,0x00, 0x02,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00, 0x04,0x00,0x00,0x00,
        0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };
    std::array<unsigned char, WORLD_RECOVERY_WIRE_HEADER_BYTES> encoded_header = {};
    assert(world_recovery_encode_header(&header, encoded_header.data(), encoded_header.size()));
    assert(encoded_header == expected_header);
    world_recovery_header decoded_header = {};
    assert(world_recovery_decode_header(encoded_header.data(), encoded_header.size(),
                                        &decoded_header));
    assert(decoded_header.sequence == header.sequence && decoded_header.timestamp == -2);
    encoded_header[63] = 1;
    assert(!world_recovery_decode_header(encoded_header.data(), encoded_header.size(),
                                         &decoded_header));

    copyover_room door = {0x01020304, -2, 0x11223344};
    std::array<unsigned char, 12> encoded_door = {};
    size_t encoded_size = 0;
    assert(world_recovery_encode_record(world_recovery_record_type::door,
                                        reinterpret_cast<const unsigned char *>(&door),
                                        sizeof(door), encoded_door.data(), encoded_door.size(),
                                        &encoded_size));
    const std::array<unsigned char, 12> expected_door = {
        0x04,0x03,0x02,0x01, 0xfe,0xff,0xff,0xff, 0x44,0x33,0x22,0x11
    };
    assert(encoded_size == expected_door.size() && encoded_door == expected_door);
    std::array<unsigned char, WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES> record_header = {};
    assert(world_recovery_encode_record_header(world_recovery_record_type::door, encoded_size,
                                               record_header.data(), record_header.size()));
    assert(!world_recovery_encode_record_header(
        static_cast<world_recovery_record_type>(99), encoded_size,
        record_header.data(), record_header.size()));
    const std::array<unsigned char, 8> expected_record_header = {
        0x0c,0x00,0x00,0x00, 0x03,0x01,0x00,0x00
    };
    assert(record_header == expected_record_header);
    world_recovery_record_type decoded_type = {};
    uint32_t decoded_size = 0;
    assert(world_recovery_decode_record_header(record_header.data(), record_header.size(),
                                               &decoded_type, &decoded_size));
    assert(decoded_type == world_recovery_record_type::door && decoded_size == 12);
    record_header[5] = 2;
    assert(!world_recovery_decode_record_header(record_header.data(), record_header.size(),
                                                &decoded_type, &decoded_size));
    std::vector<unsigned char> native;
    assert(world_recovery_decode_record(world_recovery_record_type::door, encoded_door.data(),
                                        encoded_door.size(), &native));
    copyover_room decoded_door = {};
    memcpy(&decoded_door, native.data(), sizeof(decoded_door));
    assert(decoded_door.vnum == door.vnum && decoded_door.dir == door.dir &&
           decoded_door.state == door.state);

    world_recovery_object_record object = {321, 1};
    world_recovery_item_snapshot item = {};
    item.item_uid = 0x0102030405060708ULL;
    item.root_item_uid = item.item_uid;
    item.vnum = 456;
    item.type = 7;
    item.flags = WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED;
    item.values[0] = -99;
    item.timers[0] = 0x0102030405060708LL;
    memcpy(item.name, "golden", 7);
    std::vector<unsigned char> native_object(sizeof(object) + sizeof(item));
    memcpy(native_object.data(), &object, sizeof(object));
    memcpy(native_object.data() + sizeof(object), &item, sizeof(item));
    std::array<unsigned char, WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES +
                                  WORLD_RECOVERY_WIRE_ITEM_BYTES> encoded_object = {};
    assert(world_recovery_encode_record(world_recovery_record_type::object,
                                        native_object.data(), native_object.size(),
                                        encoded_object.data(), encoded_object.size(),
                                        &encoded_size));
    assert(encoded_size == encoded_object.size());
    assert(encoded_object[0] == 0x41 && encoded_object[1] == 0x01);
    assert(encoded_object[4] == 1 && encoded_object[8] == 0x08 &&
           encoded_object[15] == 0x01);
    assert(encoded_object[40] == WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED);
    assert(encoded_object[44] == 0x9d && encoded_object[45] == 0xff);
    assert(encoded_object[76] == 0x08 && encoded_object[83] == 0x01);
    assert(!memcmp(encoded_object.data() + 124, "golden", 7));
    assert(world_recovery_decode_record(world_recovery_record_type::object,
                                        encoded_object.data(), encoded_object.size(), &native));
    world_recovery_item_snapshot decoded_item = {};
    memcpy(&decoded_item, native.data() + sizeof(object), sizeof(decoded_item));
    assert(decoded_item.item_uid == item.item_uid && decoded_item.values[0] == -99 &&
           decoded_item.timers[0] == item.timers[0] && !strcmp(decoded_item.name, "golden"));

    copyover_mob mob = {};
    mob.vnum = 10;
    mob.idnum = 20;
    mob.room = 30;
    mob.max_hit = 40;
    mob.max_mana = 50;
    mob.max_vitality = 60;
    mob.gold = 70;
    mob.num_affects = 1;
    for (int &equipment : mob.equipment_vnums)
        equipment = -1;
    copyover_affect affect = {};
    affect.type = -3;
    affect.duration = 99;
    affect.flags = 0xaabbccddU;
    affect.bitvector5 = static_cast<unsigned long>(0x01020304UL);
    std::vector<unsigned char> native_mob(sizeof(mob) + sizeof(affect));
    memcpy(native_mob.data(), &mob, sizeof(mob));
    memcpy(native_mob.data() + sizeof(mob), &affect, sizeof(affect));
    std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> encoded_mob = {};
    assert(world_recovery_encode_record(world_recovery_record_type::mob, native_mob.data(),
                                        native_mob.size(), encoded_mob.data(),
                                        encoded_mob.size(), &encoded_size));
    assert(encoded_size == 342);
    assert(world_recovery_decode_record(world_recovery_record_type::mob, encoded_mob.data(),
                                        encoded_size, &native));
    copyover_mob decoded_mob = {};
    copyover_affect decoded_affect = {};
    memcpy(&decoded_mob, native.data(), sizeof(decoded_mob));
    memcpy(&decoded_affect, native.data() + sizeof(decoded_mob), sizeof(decoded_affect));
    assert(decoded_mob.vnum == mob.vnum && decoded_mob.gold == mob.gold &&
           decoded_mob.num_affects == 1);
    assert(decoded_affect.type == affect.type && decoded_affect.flags == affect.flags &&
           decoded_affect.bitvector5 == affect.bitvector5);

    std::vector<unsigned char> floor;
    assert(world_recovery_encode_floor_object(native_object.data(), native_object.size(), &floor));
    assert(floor.size() == 5 + encoded_object.size() && !memcmp(floor.data(), "WRF4:", 5));
    uint64_t root_uid = 0;
    assert(world_recovery_floor_object_root_uid(floor.data(), floor.size(), &root_uid));
    assert(root_uid == item.item_uid);
    floor[0] = 'X';
    assert(!world_recovery_floor_object_root_uid(floor.data(), floor.size(), &root_uid));
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-world-codec-") as temp_dir:
    source = Path(temp_dir) / "codec_test.cpp"
    binary = Path(temp_dir) / "codec_test"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-Isrc",
            str(source),
            "src/world_recovery_codec.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

assert "world_recovery_encode_header" in PIPELINE
assert "world_recovery_encode_record" in PIPELINE
assert "world_recovery_decode_record" in PIPELINE
assert 'memcpy(generation.blob.data(), &header' not in PIPELINE
assert "const bool prepared = prepare_batch(job)" in FLOOR
assert "const bool succeeded = prepared && execute_batch(context, job)" in FLOOR
assert "prepared ? redis_observability_now_usec() : 0" in FLOOR
assert FLOOR.index("prepare_batch(job)") < FLOOR.index("execute_batch(context, job)")

print("schema-10 little-endian recovery codec golden vectors passed")
