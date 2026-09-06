#!/usr/bin/env python3
"""Runtime crash/corruption contracts for the typed player snapshot journal."""

from _paths import SRC, rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
JOURNAL = (SRC / "player_save_journal.c").read_text()
CODEC = (SRC / "player_snapshot_codec.c").read_text()
WORKER = (SRC / "player_save_worker.c").read_text()
DIAGNOSTICS = (SRC / "actinf.c").read_text()


HARNESS = r'''
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"

#include <cassert>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

struct replay_state
{
    std::vector<std::pair<int, player_revision_t>> applied;
    bool blocked = false;
    bool stale_death = false;
    std::vector<uint8_t> expected_death;
};

player_save_apply_result replay_apply(const player_snapshot &snapshot, void *raw)
{
    auto &state = *static_cast<replay_state *>(raw);
    if (snapshot.death) {
        std::vector<uint8_t> bytes;
        assert(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok);
        assert(bytes == state.expected_death);
        if (state.stale_death)
            return {player_save_apply_outcome::stale_revision, snapshot.revision + 1, 0};
    }
    if (state.blocked)
        return {player_save_apply_outcome::retryable_failure, snapshot.revision - 1, 1205};
    state.applied.push_back({snapshot.pid, snapshot.revision});
    return {player_save_apply_outcome::applied, snapshot.revision, 0};
}

player_snapshot make_snapshot(int pid, player_revision_t revision)
{
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    snapshot.save_intent = 4;
    snapshot.room_vnum = 1201;
    snapshot.encoded_size_bound = 8192;
    snapshot.status_integers.push_back(
        {player_status_field::level, 50, 0, false});
    snapshot.status_strings.push_back(
        {player_status_string_field::name, "journal-player"});
    snapshot.conditions = {1, 2, 3, 4, 5};
    snapshot.quest_values[3] = 77;
    snapshot.languages.push_back({1, 90, 0});
    snapshot.introductions.push_back({2, 44, 12345});
    snapshot.timers.push_back({3, 67890, 0});
    snapshot.undead_slots.push_back({4, 2, 0});
    snapshot.forged_items.push_back({5, 6001, 0});
    snapshot.granted_commands.push_back(42);
    snapshot.skills.push_back({9, 80, 1});
    player_affect_snapshot affect = {};
    affect.type = 11;
    affect.duration = 12;
    affect.bitvectors[2] = 99;
    affect.wear_off_character = "gone";
    snapshot.affects.push_back(affect);
    player_item_snapshot parent = {};
    parent.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    parent.vnum = 500;
    parent.string_mask = 1;
    parent.name = "container";
    parent.values[0] = 8;
    parent.dynamic_affects.push_back({1, 2, 3});
    player_item_extra_description_snapshot description = {};
    description.keyword = "SPELLBOOK";
    description.spellbook = true;
    description.spell_ids = {7, 12};
    parent.extra_descriptions.push_back(description);
    snapshot.items.push_back(parent);
    player_item_snapshot child = {};
    child.parent_index = 0;
    child.vnum = 501;
    snapshot.items.push_back(child);
    player_pet_snapshot pet = {};
    pet.mob_vnum = 700;
    pet.room_vnum = 1201;
    pet.items.push_back(child);
    pet.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    snapshot.pets.push_back(pet);
    snapshot.shapes.push_back({800, 2, 100, 200});
    snapshot.trophies.push_back({12, 300});
    snapshot.recipes_are_external = true;
    return snapshot;
}

uint64_t read_u64(const unsigned char *bytes)
{
    uint64_t value = 0;
    for (unsigned int index = 0; index < 8; ++index)
        value |= static_cast<uint64_t>(bytes[index]) << (index * 8);
    return value;
}

void corrupt_first_payload(const std::string &path)
{
    const int fd = open(path.c_str(), O_RDWR);
    assert(fd >= 0);
    unsigned char value = 0;
    assert(pread(fd, &value, 1, 72 + 10) == 1);
    value ^= 0x5a;
    assert(pwrite(fd, &value, 1, 72 + 10) == 1);
    assert(fsync(fd) == 0);
    close(fd);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string directory = argv[1];
    const std::string journal = directory + "/player-save.journal";
    const std::string quarantine = directory + "/player-save.journal.quarantine";

    player_snapshot original = make_snapshot(10, 1);
    std::vector<uint8_t> encoded;
    assert(player_snapshot_encode(original, &encoded) == player_snapshot_codec_result::ok);
    player_snapshot decoded = {};
    assert(player_snapshot_decode(encoded.data(), encoded.size(), &decoded) ==
           player_snapshot_codec_result::ok);
    assert(decoded.pid == original.pid && decoded.revision == original.revision);
    assert(decoded.status_strings[0].value == "journal-player");
    assert(decoded.items[1].parent_index == 0);
    assert(decoded.items[0].extra_descriptions[0].spell_ids[1] == 12);
    assert(decoded.pets[0].items[0].vnum == 501);
    assert(encoded[0] == 1 && !decoded.death); // Existing records keep their format.
    auto truncated = encoded;
    truncated.pop_back();
    assert(player_snapshot_decode(truncated.data(), truncated.size(), &decoded) ==
           player_snapshot_codec_result::truncated);

    player_snapshot death = make_snapshot(80, 4);
    death.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
    death.items.clear();
    death.pets.clear();
    death.status_integers.push_back({player_status_field::deaths, 7, 0, false});
    death.death.emplace();
    auto &recovery = *death.death;
    recovery.operation_id.bytes[0] = 1;
    recovery.corpse_room_vnum = 500;
    recovery.wallet_revision = 12;
    recovery.wallet_before = {INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX};
    recovery.wallet_pile_uid = 1003;
    player_item_snapshot corpse = {};
    corpse.object_uid = 1000;
    corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    corpse.vnum = VOBJ_CORPSE;
    corpse.type = ITEM_CORPSE;
    corpse.name = "corpse journal-player";
    corpse.description = "The corpse of journal-player is lying here.";
    corpse.values[0] = 50;
    corpse.values[CORPSE_PID] = death.pid;
    corpse.values[CORPSE_SAVEID] = 100;
    corpse.values[CORPSE_FLAGS] = PC_CORPSE;
    recovery.corpse.push_back(corpse);
    player_item_snapshot bag = original.items[0];
    bag.object_uid = 1001;
    bag.parent_index = 0;
    recovery.corpse.push_back(bag);
    player_item_snapshot coins = {};
    coins.object_uid = 1002;
    coins.parent_index = 1;
    coins.vnum = 3;
    coins.type = ITEM_MONEY;
    coins.values[0] = 123;
    recovery.corpse.push_back(coins);
    coins.object_uid = 1003;
    coins.parent_index = 0;
    for (int i = 0; i < 4; ++i) coins.values[i] = INT32_MAX;
    recovery.corpse.push_back(coins);
    recovery.custody = {
        {{1001, 1001, 0, 8, 500, item_custody_state::active}, {item_owner_type::player, 80, 0}, 10},
        {{1002, 1001, 1001, ITEM_TRANSFER_ABSENT_REVISION, 3, item_custody_state::absent}, {}, 0},
        {{1003, 1003, 0, ITEM_TRANSFER_ABSENT_REVISION, 3, item_custody_state::absent}, {}, 0},
        // An unexplained descendant remains evidence, never guessed to be consumed.
        {{1004, 1001, 1001, 9, 3, item_custody_state::active}, {item_owner_type::player, 80, 0}, 10},
    };
    critical_operation_id unsettled = {};
    unsettled.bytes[0] = 2;
    recovery.unresolved_operations.push_back(unsettled);
    std::vector<uint8_t> death_bytes;
    assert(player_snapshot_encode(death, &death_bytes) == player_snapshot_codec_result::ok);
    assert(player_snapshot_decode(death_bytes.data(), death_bytes.size(), &decoded) == player_snapshot_codec_result::ok);
    assert(decoded.items.empty() && decoded.pets.empty());
    assert(decoded.death->corpse[2].parent_index == 1 && decoded.death->corpse[2].values[0] == 123);
    assert(decoded.death->custody[3].item.item_uid == 1004);
    assert(decoded.death->wallet_before[3] == INT32_MAX && decoded.death->wallet_revision == 12);
    assert(decoded.death->unresolved_operations[0].bytes == unsettled.bytes);
    auto bad_death = death;
    bad_death.items.push_back(bag); // Cannot also restore these assets to active inventory.
    assert(player_snapshot_encode(bad_death, &encoded) == player_snapshot_codec_result::invalid_value);
    bad_death = death;
    bad_death.death->custody.erase(bad_death.death->custody.begin());
    assert(player_snapshot_encode(bad_death, &encoded) == player_snapshot_codec_result::invalid_value);
    bad_death = death;
    bad_death.death->corpse[2].object_uid = bag.object_uid;
    assert(player_snapshot_encode(bad_death, &encoded) == player_snapshot_codec_result::invalid_value);
    bad_death = death;
    bad_death.death->corpse[3].values[0] -= 1;
    assert(player_snapshot_encode(bad_death, &encoded) == player_snapshot_codec_result::invalid_value);
    bad_death = death;
    bad_death.death->corpse[0].values[CORPSE_PID] += 1;
    assert(player_snapshot_encode(bad_death, &encoded) == player_snapshot_codec_result::invalid_value);
    auto short_death = death_bytes;
    short_death.pop_back();
    assert(player_snapshot_decode(short_death.data(), short_death.size(), &decoded) == player_snapshot_codec_result::truncated);

    assert(player_save_journal_init(directory.c_str()));
    struct stat status = {};
    assert(stat(directory.c_str(), &status) == 0 && (status.st_mode & 0777) == 0700);
    assert(stat(journal.c_str(), &status) == 0 && (status.st_mode & 0777) == 0600);
    assert(player_save_journal_append(make_snapshot(10, 1)) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(10, 2)) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(20, 1)) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(20, 1)) == player_save_journal_result::ok);
    assert(player_save_journal_checkpoint(10, 1) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 3);

    replay_state replay;
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::ok);
    assert((replay.applied == std::vector<std::pair<int, player_revision_t>>{{10, 2}, {20, 1}}));
    assert(player_save_journal_health_copy().duplicates == 1);
    assert(player_save_journal_health_copy().records == 0);

    // A later player checkpoint cannot discard an unresolved death. Restart and
    // replay retain its complete payload until its own disposition is acknowledged.
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(80, 5)) == player_save_journal_result::ok);
    assert(player_save_journal_checkpoint(80, 5) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 1);
    player_save_journal_shutdown();
    assert(player_save_journal_init(directory.c_str()));
    replay.expected_death = death_bytes;
    replay.stale_death = true;
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == 1);
    replay.stale_death = false;
    replay.blocked = true;
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == 1);
    replay.blocked = false;
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 0);

    assert(player_save_journal_append(make_snapshot(30, 3)) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(31, 4)) == player_save_journal_result::ok);
    player_save_journal_shutdown();
    corrupt_first_payload(journal);
    assert(player_save_journal_init(directory.c_str()));
    auto health = player_save_journal_health_copy();
    assert(health.corrupt_records == 1 && health.records == 1);
    assert(stat(quarantine.c_str(), &status) == 0 && (status.st_mode & 0777) == 0600);
    replay.applied.clear();
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::ok);
    assert((replay.applied == std::vector<std::pair<int, player_revision_t>>{{31, 4}}));

    assert(player_save_journal_append(make_snapshot(40, 5)) == player_save_journal_result::ok);
    replay.blocked = true;
    assert(player_save_journal_replay(replay_apply, &replay) ==
           player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == 1);
    replay.blocked = false;
    player_save_journal_shutdown();

    unsigned char header[24] = {};
    int fd = open(journal.c_str(), O_RDONLY);
    assert(fd >= 0 && read(fd, header, sizeof(header)) == static_cast<ssize_t>(sizeof(header)));
    close(fd);
    const uint64_t record_size = read_u64(header + 16);
    assert(record_size > 10);
    assert(truncate(journal.c_str(), record_size - 10) == 0);
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_health_copy().corrupt_records >= 1);
    assert(player_save_journal_health_copy().records == 0);
    player_save_journal_shutdown();

    assert(truncate(journal.c_str(), 0) == 0);
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_append(make_snapshot(50, 6)) == player_save_journal_result::ok);
    player_save_journal_shutdown();
    fd = open(journal.c_str(), O_RDWR);
    assert(fd >= 0);
    const unsigned char unsupported_version = 2;
    assert(pwrite(fd, &unsupported_version, 1, 8) == 1);
    assert(fsync(fd) == 0);
    close(fd);
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_health_copy().unsupported_records == 1);
    assert(player_save_journal_health_copy().records == 0);
    player_save_journal_shutdown();

    const std::string unsafe_directory = directory + "-unsafe";
    assert(mkdir(unsafe_directory.c_str(), 0755) == 0);
    // A restrictive caller umask must not turn this rejection fixture safe.
    assert(chmod(unsafe_directory.c_str(), 0755) == 0);
    assert(!player_save_journal_init(unsafe_directory.c_str()));
    const std::string quota_directory = directory + "-quota";
    assert(mkdir(quota_directory.c_str(), 0700) == 0);
    assert(player_save_journal_init(quota_directory.c_str(), 256));
    assert(player_save_journal_append(make_snapshot(60, 1)) ==
           player_save_journal_result::quota_exceeded);
    assert(player_save_journal_health_copy().quota_exceeded);
    player_save_journal_shutdown();
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-player-journal-") as temp_dir:
    journal_dir = Path(temp_dir) / "journal"
    source = Path(temp_dir) / "journal_test.cpp"
    binary = Path(temp_dir) / "journal_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-pthread",
            "-Isrc",
            str(source),
            rel("player_snapshot_codec.c"),
            rel("player_save_journal.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary), str(journal_dir)], check=True, timeout=15)

for contract in (
    "JOURNAL_MAGIC",
    "JOURNAL_FORMAT_VERSION",
    "JOURNAL_HEADER_SIZE",
    "frame_checksum",
    "PLAYER_SNAPSHOT_MAX_STRING_BYTES",
    "PLAYER_SNAPSHOT_MAX_ROWS",
    "PLAYER_SNAPSHOT_MAX_OBJECTS",
):
    assert contract in JOURNAL + CODEC
assert "sizeof(player_snapshot)" not in CODEC
assert "sql" not in CODEC.lower()
print("[PASS] snapshot codec is typed, endian-stable, bounded, and host-layout independent")

for contract in (
    "O_NOFOLLOW",
    "O_CLOEXEC",
    "fchmod(fd, 0600)",
    "mkdir(directory, 0700)",
    "fdatasync(fd)",
    "sync_directory()",
    "PLAYER_SAVE_JOURNAL_MAX_BYTES",
    "PLAYER_SAVE_JOURNAL_MAX_AGE_MSEC",
):
    assert contract in JOURNAL
append_body = JOURNAL.split("player_save_journal_result player_save_journal_append", 1)[1].split(
    "player_save_journal_result player_save_journal_checkpoint", 1
)[0]
assert append_body.index("write_all(fd, frame.bytes.data()") < append_body.index("fdatasync(fd)")
print("[PASS] append, permissions, quota, and sync boundaries fail closed")

for contract in (
    "find_next_magic",
    "append_quarantine",
    "JOURNAL_TEMP_NAME",
    "O_EXCL",
    "rename(temporary.c_str(), journal_path.c_str())",
    "frame.snapshot.revision > durable_revision",
    "std::sort(frames.begin()",
    "health.duplicates",
    "replay_blocked",
):
    assert contract in JOURNAL
assert JOURNAL.index("fdatasync(fd) == 0") < JOURNAL.index(
    "rename(temporary.c_str(), journal_path.c_str())"
)
print("[PASS] corruption, atomic compaction, duplicate suppression, and ordered replay are bounded")

for contract in (
    "player_save_worker_set_journal_hooks",
    "journal_append_callback",
    "durably_spilled",
    "journal_ack_callback",
):
    assert contract in WORKER
for metric in (
    "player_journal state=",
    "quarantined_bytes",
    "checkpoint_failures",
    "quota_exceeded",
    "age_limit_exceeded",
):
    assert metric in DIAGNOSTICS
print("[PASS] worker durable-handoff hooks and redacted journal health are integrated")

print("typed player persistence journal contracts passed")
