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
#include <array>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

bool fail_one_directory_sync = false;
extern "C" int __real_fsync(int fd);
extern "C" int __wrap_fsync(int fd)
{
    struct stat status{};
    if (fail_one_directory_sync && fstat(fd, &status) == 0 && S_ISDIR(status.st_mode)) {
        fail_one_directory_sync = false;
        errno = EIO;
        return -1;
    }
    return __real_fsync(fd);
}

struct replay_state
{
    std::vector<std::pair<int, player_revision_t>> applied;
    bool blocked = false;
    bool ambiguous = false;
    bool block_death_only = false;
    bool stale_death = false;
    bool newer_death = false;
    bool stale_receipt = false;
    bool newer_receipt = false;
    bool verified_receipt = false;
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
        if (state.newer_death)
            return {player_save_apply_outcome::already_applied, snapshot.revision + 1, 0};
    }
    if (!snapshot.quest_xp_receipts.empty() || !snapshot.spell_effect_receipts.empty() || !snapshot.craft_receipts.empty()) {
        if (state.verified_receipt)
            return {player_save_apply_outcome::stale_revision, snapshot.revision + 1, 0,
                    player_save_custody_diagnosis::none, true};
        if (state.stale_receipt)
            return {player_save_apply_outcome::stale_revision, snapshot.revision + 1, 0};
        if (state.newer_receipt)
            return {player_save_apply_outcome::already_applied, snapshot.revision + 1, 0};
    }
    if (state.ambiguous)
        return {player_save_apply_outcome::ambiguous_commit, snapshot.revision - 1, 2013};
    if (state.blocked && (!state.block_death_only || snapshot.death))
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
    snapshot.output_preferences = "v1;m=1;12=27";
    return snapshot;
}

uint64_t read_u64(const unsigned char *bytes)
{
    uint64_t value = 0;
    for (unsigned int index = 0; index < 8; ++index)
        value |= static_cast<uint64_t>(bytes[index]) << (index * 8);
    return value;
}

uint32_t crc_payload(const std::vector<uint8_t> &bytes)
{
    uint32_t crc = UINT32_MAX;
    for (uint8_t byte : bytes) {
        crc ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0U - (crc & 1U)));
    }
    return ~crc;
}

// CRC32 is a corruption check, not a proof that two operation bodies match.
// Solve its 32-bit linear correction using a second quest value, producing two
// codec-valid snapshots with different bytes at the exact same save identity.
player_snapshot checksum_collision(const player_snapshot &original)
{
    auto changed = original;
    changed.quest_values[0] ^= 1;
    changed.quest_values[1] = 0;
    std::vector<uint8_t> original_bytes, baseline_bytes, probe_bytes;
    assert(player_snapshot_encode(original, &original_bytes) == player_snapshot_codec_result::ok);
    assert(player_snapshot_encode(changed, &baseline_bytes) == player_snapshot_codec_result::ok);
    const uint32_t baseline = crc_payload(baseline_bytes);
    std::array<uint32_t, 32> basis{}, coefficients{};
    for (unsigned bit = 0; bit < 32; ++bit) {
        changed.quest_values[1] = std::bit_cast<int32_t>(UINT32_C(1) << bit);
        assert(player_snapshot_encode(changed, &probe_bytes) == player_snapshot_codec_result::ok);
        uint32_t value = crc_payload(probe_bytes) ^ baseline;
        uint32_t coefficient = UINT32_C(1) << bit;
        for (int pivot = 31; pivot >= 0; --pivot) {
            if (!(value & (UINT32_C(1) << pivot))) continue;
            if (basis[pivot]) {
                value ^= basis[pivot];
                coefficient ^= coefficients[pivot];
            } else {
                basis[pivot] = value;
                coefficients[pivot] = coefficient;
                break;
            }
        }
    }
    uint32_t remaining = crc_payload(original_bytes) ^ baseline, correction = 0;
    for (int pivot = 31; pivot >= 0; --pivot) {
        if (!(remaining & (UINT32_C(1) << pivot))) continue;
        assert(basis[pivot]);
        remaining ^= basis[pivot];
        correction ^= coefficients[pivot];
    }
    assert(remaining == 0);
    changed.quest_values[1] = std::bit_cast<int32_t>(correction);
    assert(player_snapshot_encode(changed, &probe_bytes) == player_snapshot_codec_result::ok);
    assert(probe_bytes != original_bytes && crc_payload(probe_bytes) == crc_payload(original_bytes));
    return changed;
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

size_t append_frame_copies(const std::string &path, size_t count)
{
    int fd = open(path.c_str(), O_RDONLY);
    struct stat status{};
    assert(fd >= 0 && fstat(fd, &status) == 0 && status.st_size > 72);
    std::vector<unsigned char> frame(status.st_size);
    assert(read(fd, frame.data(), frame.size()) == static_cast<ssize_t>(frame.size()));
    close(fd);
    fd = open(path.c_str(), O_WRONLY | O_APPEND);
    assert(fd >= 0);
    for (size_t index = 0; index < count; ++index)
        assert(write(fd, frame.data(), frame.size()) == static_cast<ssize_t>(frame.size()));
    assert(fdatasync(fd) == 0);
    close(fd);
    return frame.size();
}

std::vector<unsigned char> read_file_bytes(const std::string &path)
{
    const int fd = open(path.c_str(), O_RDONLY);
    struct stat status{};
    assert(fd >= 0 && fstat(fd, &status) == 0 && status.st_size >= 0);
    std::vector<unsigned char> bytes(status.st_size);
    size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = read(fd, bytes.data() + offset, bytes.size() - offset);
        assert(count > 0);
        offset += count;
    }
    close(fd);
    return bytes;
}

void corrupt_last_payload(const std::string &path)
{
    const int fd = open(path.c_str(), O_RDWR);
    assert(fd >= 0);
    const off_t last = lseek(fd, -1, SEEK_END);
    assert(last > 72);
    unsigned char value = 0;
    assert(pread(fd, &value, 1, last) == 1);
    value ^= 0x5a;
    assert(pwrite(fd, &value, 1, last) == 1 && fsync(fd) == 0);
    close(fd);
}

void downgrade_frame(const std::string &path, uint8_t version, size_t preference_offset, size_t preference_size)
{
    const int fd = open(path.c_str(), O_RDWR);
    struct stat status{};
    assert(fd >= 0 && fstat(fd, &status) == 0);
    std::vector<uint8_t> bytes(status.st_size);
    assert(read(fd, bytes.data(), bytes.size()) == static_cast<ssize_t>(bytes.size()));
    bytes.erase(bytes.begin() + 72 + preference_offset,
                bytes.begin() + 72 + preference_offset + preference_size + 4);
    auto put = [&](size_t offset, uint64_t value, size_t width) {
        for (size_t i = 0; i < width; ++i) bytes[offset+i] = (value >> (i*8)) & 255;
    };
    put(16, bytes.size(), 8);
    put(44, version, 4);
    put(64, bytes.size()-72, 4);
    put(72, version, 4);
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i >= 68 && i < 72) continue;
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0U-(crc&1U)));
    }
    put(68, ~crc, 4);
    assert(pwrite(fd, bytes.data(), bytes.size(), 0) == static_cast<ssize_t>(bytes.size()));
    assert(ftruncate(fd, bytes.size()) == 0 && fsync(fd) == 0);
    close(fd);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string directory = argv[1];
    const std::string journal = directory + "/player-save.journal";
    const std::string quarantine = directory + "/player-save.journal.quarantine.archive";

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
    assert(decoded.output_preferences == original.output_preferences);
    assert(encoded[0] == PLAYER_SNAPSHOT_SCHEMA_VERSION && !decoded.death); // Base format, not death.
    auto truncated = encoded;
    truncated.pop_back();
    assert(player_snapshot_decode(truncated.data(), truncated.size(), &decoded) ==
           player_snapshot_codec_result::truncated);

    player_snapshot quest_xp = make_snapshot(11, 2);
    quest_xp.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
    quest_xp.encoded_size_bound += 4 + 24;
    player_quest_xp_receipt_snapshot xp_receipt = {};
    xp_receipt.offering_operation.bytes[0] = 0x42;
    xp_receipt.reward_index = 63;
    xp_receipt.amount = 1234;
    quest_xp.quest_xp_receipts.push_back(xp_receipt);
    std::vector<uint8_t> quest_xp_bytes;
    assert(player_snapshot_encode(quest_xp, &quest_xp_bytes) ==
           player_snapshot_codec_result::ok);
    assert(player_snapshot_decode(quest_xp_bytes.data(), quest_xp_bytes.size(), &decoded) ==
           player_snapshot_codec_result::ok);
    assert(decoded.schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
           decoded.quest_xp_receipts.size() == 1 &&
           decoded.quest_xp_receipts[0].offering_operation.bytes ==
               xp_receipt.offering_operation.bytes &&
           decoded.quest_xp_receipts[0].reward_index == 63 &&
           decoded.quest_xp_receipts[0].amount == 1234);
    quest_xp.quest_xp_receipts.push_back(xp_receipt);
    assert(player_snapshot_encode(quest_xp, &quest_xp_bytes) ==
           player_snapshot_codec_result::invalid_value);
    for (size_t size = 0; size < quest_xp_bytes.size(); ++size) {
        decoded.pid = 999;
        assert(player_snapshot_decode(quest_xp_bytes.data(), size, &decoded) != player_snapshot_codec_result::ok);
        assert(decoded.pid == 999);
    }

    player_snapshot spell = make_snapshot(12, 3);
    spell.schema_version = PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
    player_spell_effect_receipt_snapshot spell_receipt = {};
    spell_receipt.operation_id.bytes[0] = 0xa5;
    spell_receipt.effect_id = 6;
    spell.spell_effect_receipts.push_back(spell_receipt);
    std::vector<uint8_t> spell_bytes;
    assert(player_snapshot_encode(spell, &spell_bytes) ==
           player_snapshot_codec_result::ok);
    assert(player_snapshot_decode(spell_bytes.data(), spell_bytes.size(), &decoded) ==
           player_snapshot_codec_result::ok);
    assert(decoded.schema_version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
           decoded.spell_effect_receipts.size() == 1 &&
           decoded.spell_effect_receipts[0].operation_id.bytes ==
               spell_receipt.operation_id.bytes &&
           decoded.spell_effect_receipts[0].effect_id == spell_receipt.effect_id);
    spell.spell_effect_receipts.push_back(spell_receipt);
    assert(player_snapshot_encode(spell, &spell_bytes) ==
           player_snapshot_codec_result::invalid_value);
    for (size_t size = 0; size < spell_bytes.size(); ++size) {
        decoded.pid = 999;
        assert(player_snapshot_decode(spell_bytes.data(), size, &decoded) != player_snapshot_codec_result::ok);
        assert(decoded.pid == 999);
    }

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
    assert(decoded.output_preferences == death.output_preferences);
    auto legacy_base = death;
    legacy_base.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    legacy_base.death.reset();
    std::vector<uint8_t> base_bytes;
    assert(player_snapshot_encode(legacy_base, &base_bytes) == player_snapshot_codec_result::ok);
    auto legacy_death = death_bytes;
    legacy_death.erase(legacy_death.begin() + base_bytes.size() - 4 - death.output_preferences.size(),
                       legacy_death.begin() + base_bytes.size());
    for (uint8_t version : {2, 4}) {
        legacy_death[0] = version;
        assert(player_snapshot_decode(legacy_death.data(), legacy_death.size(), &decoded) == player_snapshot_codec_result::ok);
        assert(decoded.schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION && decoded.output_preferences.empty());
        assert(decoded.death->corpse[2].values[0] == 123);
    }
    auto wire6_death = death_bytes;
    wire6_death[0] = 6;
    assert(player_snapshot_decode(wire6_death.data(), wire6_death.size(), &decoded) == player_snapshot_codec_result::ok);
    assert(decoded.schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION &&
           decoded.death->corpse[2].values[0] == 123);
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

    spell.spell_effect_receipts.pop_back();
    quest_xp.quest_xp_receipts.pop_back();
    for (const auto &original : {make_snapshot(91, 1), spell, quest_xp}) {
        const auto colliding = checksum_collision(original);
        const auto duplicates_before = player_save_journal_health_copy().duplicates;
        assert(player_save_journal_append(original) == player_save_journal_result::ok);
        assert(player_save_journal_append(colliding) == player_save_journal_result::ok);
        replay_state collision_replay;
        assert(player_save_journal_replay(replay_apply, &collision_replay) == player_save_journal_result::ok);
        assert(collision_replay.applied.size() == 2);
        assert(player_save_journal_health_copy().duplicates == duplicates_before);
        assert(player_save_journal_health_copy().records == 0);
    }
    // The same CRC collision bucket still suppresses a genuine duplicate only.
    const auto duplicate_source = make_snapshot(92, 1);
    const auto duplicate_collision = checksum_collision(duplicate_source);
    const auto duplicates_before = player_save_journal_health_copy().duplicates;
    for (const auto &snapshot : {duplicate_source, duplicate_collision, duplicate_source})
        assert(player_save_journal_append(snapshot) == player_save_journal_result::ok);
    replay_state collision_with_duplicate;
    assert(player_save_journal_replay(replay_apply, &collision_with_duplicate) == player_save_journal_result::ok);
    assert(collision_with_duplicate.applied.size() == 2);
    assert(player_save_journal_health_copy().duplicates == duplicates_before + 1);
    assert(player_save_journal_health_copy().records == 0);

    // Existing journal envelopes retain their original schema number while
    // decoding normalizes their snapshots. Exercise all four previous versions.
    for (uint8_t version : {1, 2, 3, 4}) {
        auto legacy = version % 2 ? make_snapshot(90, 1) : death;
        legacy.pets.clear();
        auto prefix = legacy;
        prefix.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
        prefix.death.reset();
        std::vector<uint8_t> prefix_bytes;
        assert(player_snapshot_encode(prefix, &prefix_bytes) == player_snapshot_codec_result::ok);
        assert(player_save_journal_append(legacy) == player_save_journal_result::ok);
        player_save_journal_shutdown();
        downgrade_frame(journal, version, prefix_bytes.size()-4-legacy.output_preferences.size(), legacy.output_preferences.size());
        assert(player_save_journal_init(directory.c_str()));
        assert(player_save_journal_health_copy().records == 1);
        replay_state old_replay;
        if (legacy.death) {
            legacy.output_preferences.clear();
            assert(player_snapshot_encode(legacy, &old_replay.expected_death) == player_snapshot_codec_result::ok);
        }
        assert(player_save_journal_replay(replay_apply, &old_replay) == player_save_journal_result::ok);
        assert(old_replay.applied.size() == 1 && old_replay.applied[0].first == legacy.pid);
        assert(player_save_journal_health_copy().records == 0);
    }

    // A later player checkpoint cannot discard an unresolved death. If replay
    // cannot prove that death's own disposition, quarantine the complete PID
    // group durably rather than acknowledging or retrying it forever.
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(80, 5)) == player_save_journal_result::ok);
    assert(player_save_journal_checkpoint(80, 5) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 1);
    player_save_journal_shutdown();
    assert(player_save_journal_init(directory.c_str()));
    replay.expected_death = death_bytes;
    replay.stale_death = true;
    replay.applied.clear();
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 0);
    assert(player_save_journal_pid_quarantined(death.pid));
    assert(replay.applied.empty());
    player_save_journal_shutdown();
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_health_copy().records == 0);
    assert(player_save_journal_pid_quarantined(death.pid));
    player_save_journal_shutdown();

    // A newer already-applied revision also cannot prove death custody.
    const std::string newer_death_directory = directory + "-newer-death-unproven";
    assert(player_save_journal_init(newer_death_directory.c_str()));
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    replay.newer_death = true;
    replay.stale_death = false;
    replay.applied.clear();
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 0);
    assert(player_save_journal_pid_quarantined(death.pid));
    assert(replay.applied.empty());
    player_save_journal_shutdown();

    // Retryable errors remain retryable: they do not ACK or quarantine the PID.
    const std::string retry_death_directory = directory + "-retry-death";
    assert(player_save_journal_init(retry_death_directory.c_str()));
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    replay.newer_death = false;
    replay.blocked = true;
    replay.block_death_only = true;
    replay.applied.clear();
    assert(player_save_journal_replay(replay_apply, &replay) ==
           player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == 1);
    assert(!player_save_journal_pid_quarantined(death.pid));
    replay.blocked = false;
    replay.block_death_only = false;
    assert(player_save_journal_replay(replay_apply, &replay) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 0);
    assert(!player_save_journal_pid_quarantined(death.pid));
    player_save_journal_shutdown();

    // Protect modern death frames with spell and XP receipts through checkpoints.
    death.schema_version = PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION;
    death.spell_effect_receipts.push_back(spell_receipt);
    death.encoded_size_bound += 24;
    unsigned int case_number = 0;
    for (bool with_xp : {false, true}) {
        if (with_xp) {
            death.schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
            death.quest_xp_receipts.push_back(xp_receipt);
            death.encoded_size_bound += 28;
        }
        for (int outcome : {0, 1, 2, 3}) {
            const std::string isolated = directory + "-receipt-death-" + std::to_string(++case_number);
            assert(player_save_journal_init(isolated.c_str()));
            assert(player_save_journal_append(death) == player_save_journal_result::ok);
            assert(player_save_journal_checkpoint(death.pid, death.revision) == player_save_journal_result::ok);
            assert(player_save_journal_health_copy().records == 1);
            replay_state proof;
            assert(player_snapshot_encode(death, &proof.expected_death) == player_snapshot_codec_result::ok);
            proof.stale_death = outcome == 0;
            proof.newer_death = outcome == 1;
            proof.blocked = outcome == 2;
            proof.ambiguous = outcome == 3;
            const bool retry = outcome >= 2;
            assert(player_save_journal_replay(replay_apply, &proof) ==
                   (retry ? player_save_journal_result::replay_blocked : player_save_journal_result::ok));
            assert(player_save_journal_health_copy().records == (retry ? 1 : 0));
            assert(player_save_journal_pid_quarantined(death.pid) == !retry);
            player_save_journal_shutdown();
            assert(player_save_journal_init(isolated.c_str()));
            assert(player_save_journal_pid_quarantined(death.pid) == !retry);
            if (retry) {
                proof.blocked = proof.ambiguous = false;
                assert(player_save_journal_replay(replay_apply, &proof) == player_save_journal_result::ok);
                assert(player_save_journal_health_copy().records == 0);
            }
            player_save_journal_shutdown();
        }
    }
    auto craft = spell;
    craft.schema_version = PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
    craft.components = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_SKILLS | PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_TROPHIES;
    craft.spell_effect_receipts.clear();
    player_craft_receipt_snapshot craft_receipt = {};
    craft_receipt.operation_id.bytes[0] = 99;
    craft_receipt.discipline = 2;
    craft_receipt.experience = 7000;
    craft.craft_receipts.push_back(craft_receipt);
    craft.encoded_size_bound += 1024;
    for (const player_snapshot &receipt_save : {spell, quest_xp, craft}) {
        for (int outcome : {0, 1, 2, 3, 4}) {
            const std::string isolated = directory + "-receipt-operation-" + std::to_string(++case_number);
            assert(player_save_journal_init(isolated.c_str()));
            assert(player_save_journal_append(receipt_save) == player_save_journal_result::ok);
            assert(player_save_journal_checkpoint(receipt_save.pid, receipt_save.revision + 1) == player_save_journal_result::ok);
            assert(player_save_journal_health_copy().records == 1);
            replay_state proof;
            proof.stale_receipt = outcome == 0;
            proof.newer_receipt = outcome == 1;
            proof.verified_receipt = outcome == 2;
            proof.blocked = outcome == 3;
            proof.ambiguous = outcome == 4;
            if (proof.verified_receipt)
                assert(!player_save_result_matches_exact_request(receipt_save, replay_apply(receipt_save, &proof)));
            const bool retry = outcome >= 3;
            assert(player_save_journal_replay(replay_apply, &proof) ==
                   (retry ? player_save_journal_result::replay_blocked : player_save_journal_result::ok));
            assert(player_save_journal_health_copy().records == (retry ? 1 : 0));
            assert(player_save_journal_pid_quarantined(receipt_save.pid) == (outcome < 2));
            player_save_journal_shutdown();
            assert(player_save_journal_init(isolated.c_str()));
            assert(player_save_journal_pid_quarantined(receipt_save.pid) == (outcome < 2));
            if (retry) {
                proof.blocked = proof.ambiguous = false;
                assert(player_save_journal_replay(replay_apply, &proof) == player_save_journal_result::ok);
                assert(player_save_journal_health_copy().records == 0);
            }
            player_save_journal_shutdown();
        }
        const std::string isolated = directory + "-receipt-exact-ack-" + std::to_string(++case_number);
        assert(player_save_journal_init(isolated.c_str()));
        assert(player_save_journal_append(receipt_save) == player_save_journal_result::ok);
        assert(player_save_journal_checkpoint(receipt_save.pid, receipt_save.revision) == player_save_journal_result::ok);
        assert(player_save_journal_health_copy().records == 1);
        auto mismatch = receipt_save;
        mismatch.room_vnum += 1;
        assert(!player_save_journal_worker_ack(mismatch, mismatch.revision, nullptr));
        assert(player_save_journal_health_copy().records == 1);
        assert(player_save_journal_worker_ack(receipt_save, receipt_save.revision, nullptr));
        assert(player_save_journal_health_copy().records == 0);
        player_save_journal_shutdown();
    }

    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_append(make_snapshot(30, 3)) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(31, 4)) == player_save_journal_result::ok);
    player_save_journal_shutdown();
    corrupt_first_payload(journal);
    const auto corrupt_original = read_file_bytes(journal);
    assert(!player_save_journal_init(directory.c_str()));
    auto health = player_save_journal_health_copy();
    assert(health.corrupt_records == 1);
    assert(stat(quarantine.c_str(), &status) == 0 && (status.st_mode & 0777) == 0600);
    assert(status.st_size > 0 && player_save_journal_health_copy().quarantined_bytes > 0);
    assert(read_file_bytes(journal) == corrupt_original);
    assert(player_save_journal_pid_quarantined(30));
    assert(player_save_journal_pid_quarantined(31));
    player_save_journal_shutdown();
    // Disposable fixture-only reset after proving the raw evidence is preserved.
    assert(unlink(quarantine.c_str()) == 0);
    assert(truncate(journal.c_str(), 0) == 0);
    assert(player_save_journal_init(directory.c_str()));

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
    const auto truncated_original = read_file_bytes(journal);
    assert(!player_save_journal_init(directory.c_str()));
    assert(player_save_journal_health_copy().corrupt_records >= 1);
    assert(read_file_bytes(journal) == truncated_original);
    assert(player_save_journal_pid_quarantined(40));
    player_save_journal_shutdown();

    assert(unlink(quarantine.c_str()) == 0);
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
    const auto unsupported_original = read_file_bytes(journal);
    assert(!player_save_journal_init(directory.c_str()));
    assert(player_save_journal_health_copy().unsupported_records == 1);
    assert(read_file_bytes(journal) == unsupported_original);
    assert(player_save_journal_pid_quarantined(50));
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

    const std::string death_directory = directory + "-death-identity";
    auto other_death = death;
    other_death.death->operation_id.bytes[0] = 3;
    assert(player_save_journal_init(death_directory.c_str()));
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    assert(player_save_journal_append(other_death) == player_save_journal_result::ok);
    assert(player_save_journal_checkpoint(death.pid, death.revision) ==
           player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 2);
    assert(!player_save_journal_worker_ack(death, death.revision + 1, nullptr));
    assert(player_save_journal_worker_ack(death, death.revision, nullptr));
    assert(player_save_journal_health_copy().records == 1);
    player_save_journal_shutdown();
    assert(player_save_journal_init(death_directory.c_str()));
    replay_state remaining_death;
    assert(player_snapshot_encode(other_death, &remaining_death.expected_death) ==
           player_snapshot_codec_result::ok);
    remaining_death.blocked = true;
    assert(player_save_journal_replay(replay_apply, &remaining_death) ==
           player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == 1);
    remaining_death.blocked = false;
    assert(player_save_journal_replay(replay_apply, &remaining_death) ==
           player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 0);
    player_save_journal_shutdown();

    assert(player_snapshot_encode(death, &death_bytes) == player_snapshot_codec_result::ok);
    const std::string synced_directory = directory + "-renamed-before-sync";
    const std::string synced_file = synced_directory + "/player-save.journal";
    assert(player_save_journal_init(synced_directory.c_str()));
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(81, 1)) == player_save_journal_result::ok);
    const auto original_deaths = read_file_bytes(synced_file);
    fail_one_directory_sync = true;
    assert(!player_save_journal_worker_ack(death, death.revision, nullptr));
    assert(!fail_one_directory_sync);
    const auto after_rename = read_file_bytes(synced_file);
    assert(after_rename.size() < original_deaths.size() && !after_rename.empty());
    assert(player_save_journal_worker_ack(death, death.revision, nullptr));
    assert(player_save_journal_health_copy().records == 1);
    player_save_journal_shutdown();
    assert(player_save_journal_init(synced_directory.c_str()));
    replay_state synced_remaining;
    synced_remaining.blocked = true;
    assert(player_save_journal_replay(replay_apply, &synced_remaining) ==
           player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == 1);
    player_save_journal_shutdown();

    const std::string partial_directory = directory + "-partial-proof";
    const std::string partial_file = partial_directory + "/player-save.journal";
    assert(player_save_journal_init(partial_directory.c_str()));
    assert(player_save_journal_append(make_snapshot(70, 1)) == player_save_journal_result::ok);
    assert(player_save_journal_append(make_snapshot(71, 1)) == player_save_journal_result::ok);
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    const auto partial_original = read_file_bytes(partial_file);
    replay_state partial;
    partial.expected_death = death_bytes;
    partial.blocked = true;
    partial.block_death_only = true;
    const std::string partial_temp = partial_file + ".tmp";
    assert(mkdir(partial_temp.c_str(), 0700) == 0);
    assert(player_save_journal_replay(replay_apply, &partial) ==
           player_save_journal_result::io_failure);
    assert(partial.applied.size() == 2);
    assert(read_file_bytes(partial_file) == partial_original);
    assert(rmdir(partial_temp.c_str()) == 0);
    partial.applied.clear();
    assert(player_save_journal_replay(replay_apply, &partial) ==
           player_save_journal_result::replay_blocked);
    assert((partial.applied == std::vector<std::pair<int, player_revision_t>>{{70, 1}, {71, 1}}));
    assert(player_save_journal_health_copy().records == 1);
    player_save_journal_shutdown();
    assert(player_save_journal_init(partial_directory.c_str()));
    assert(player_save_journal_health_copy().records == 1);
    partial.blocked = false;
    partial.applied.clear();
    assert(player_save_journal_replay(replay_apply, &partial) == player_save_journal_result::ok);
    assert((partial.applied == std::vector<std::pair<int, player_revision_t>>{{80, 4}}));
    assert(player_save_journal_health_copy().records == 0);
    player_save_journal_shutdown();

    // The writer must not create a valid journal which checkpoint/startup cannot
    // read. Use distinct PIDs to exercise the many-player replay cost.
    const std::string capacity_directory = directory + "-many-pids";
    assert(player_save_journal_init(capacity_directory.c_str()));
    for (int index = 0; index < 4841; ++index)
        assert(player_save_journal_append(make_snapshot(10000 + index, 1)) ==
               player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 4841);
    assert(player_save_journal_checkpoint(10000, 1) == player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 4840);
    player_save_journal_shutdown();
    assert(player_save_journal_init(capacity_directory.c_str()));
    assert(player_save_journal_health_copy().records == 4840);
    replay_state many;
    assert(player_save_journal_replay(replay_apply, &many) == player_save_journal_result::ok);
    assert(many.applied.size() == 4840);
    assert(many.applied.front().first == 10001 && many.applied.back().first == 14840);
    assert(player_save_journal_health_copy().records == 0);
    player_save_journal_shutdown();

    // The legacy writer may have exceeded the new admission bound already.
    const std::string legacy_directory = directory + "-legacy-over-limit";
    const std::string legacy_file = legacy_directory + "/player-save.journal";
    assert(player_save_journal_init(legacy_directory.c_str()));
    assert(player_save_journal_append(make_snapshot(6000, 1)) ==
           player_save_journal_result::ok);
    player_save_journal_shutdown();
    const size_t frame_size = append_frame_copies(legacy_file, PLAYER_SAVE_JOURNAL_MAX_RECORDS);
    assert(player_save_journal_init(legacy_directory.c_str()));
    assert(player_save_journal_health_copy().records == PLAYER_SAVE_JOURNAL_MAX_RECORDS + 1);
    assert(player_save_journal_health_copy().record_limit_exceeded);
    assert(player_save_journal_append(make_snapshot(6001, 1)) ==
           player_save_journal_result::quota_exceeded);
    assert(player_save_journal_health_copy().backpressure == 1);
    assert(stat(legacy_file.c_str(), &status) == 0 &&
           status.st_size == static_cast<off_t>(frame_size *
                                                (PLAYER_SAVE_JOURNAL_MAX_RECORDS + 1)));
    const auto protected_legacy_bytes = read_file_bytes(legacy_file);
    const std::string temporary_path = legacy_directory + "/player-save.journal.tmp";
    assert(mkdir(temporary_path.c_str(), 0700) == 0);
    assert(player_save_journal_checkpoint(6000, 1) == player_save_journal_result::io_failure);
    assert(read_file_bytes(legacy_file) == protected_legacy_bytes);
    assert(rmdir(temporary_path.c_str()) == 0);
    replay_state legacy_replay;
    legacy_replay.blocked = true;
    assert(player_save_journal_replay(replay_apply, &legacy_replay) ==
           player_save_journal_result::replay_blocked);
    assert(player_save_journal_health_copy().records == PLAYER_SAVE_JOURNAL_MAX_RECORDS + 1);
    assert(read_file_bytes(legacy_file) == protected_legacy_bytes);
    legacy_replay.blocked = false;
    assert(player_save_journal_replay(replay_apply, &legacy_replay) ==
           player_save_journal_result::ok);
    assert(player_save_journal_health_copy().records == 0);
    player_save_journal_shutdown();

    // Verify a corrupt frame beyond the old boundary is examined/quarantined.
    assert(player_save_journal_init(legacy_directory.c_str()));
    assert(player_save_journal_append(make_snapshot(6002, 1)) ==
           player_save_journal_result::ok);
    player_save_journal_shutdown();
    append_frame_copies(legacy_file, 4096);
    corrupt_last_payload(legacy_file);
    const auto late_corrupt_original = read_file_bytes(legacy_file);
    assert(!player_save_journal_init(legacy_directory.c_str()));
    assert(player_save_journal_health_copy().corrupt_records == 1);
    assert(player_save_journal_health_copy().quarantined_bytes == frame_size);
    assert(read_file_bytes(legacy_file) == late_corrupt_original);
    assert(player_save_journal_pid_quarantined(6002));
    assert(player_save_journal_pid_quarantined(6003));
    player_save_journal_shutdown();

    const std::string unsupported_directory = legacy_directory + "-unsupported";
    const std::string unsupported_file = unsupported_directory + "/player-save.journal";
    assert(player_save_journal_init(unsupported_directory.c_str()));
    assert(player_save_journal_append(make_snapshot(6003, 1)) ==
           player_save_journal_result::ok);
    player_save_journal_shutdown();
    append_frame_copies(unsupported_file, 4096);
    int last_fd = open(unsupported_file.c_str(), O_RDWR);
    assert(last_fd >= 0);
    const unsigned char late_unsupported_version = 2;
    assert(pwrite(last_fd, &late_unsupported_version, 1, frame_size * 4096 + 8) == 1);
    assert(fsync(last_fd) == 0);
    close(last_fd);
    const auto late_unsupported_original = read_file_bytes(unsupported_file);
    assert(!player_save_journal_init(unsupported_directory.c_str()));
    assert(player_save_journal_health_copy().unsupported_records == 1);
    assert(read_file_bytes(unsupported_file) == late_unsupported_original);
    assert(player_save_journal_pid_quarantined(6003));
    const std::string legacy_archive = unsupported_directory + "/player-save.journal.quarantine.archive";
    assert(stat(legacy_archive.c_str(), &status) == 0 &&
           status.st_size > static_cast<off_t>(frame_size));
    assert(player_save_journal_health_copy().quarantined_bytes == frame_size);
    player_save_journal_shutdown();
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-player-journal-") as temp_dir:
    journal_dir = Path(temp_dir) / "journal"
    source = Path(temp_dir) / "journal_test.cpp"
    binary = Path(temp_dir) / "journal_test"
    source.write_text(HARNESS)
    compiled = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-pthread",
            "-Wl,--wrap=fsync",
            "-Isrc",
            str(source),
            rel("player_snapshot_codec.c"),
            rel("player_save_journal.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if compiled.returncode:
        print(compiled.stderr)
        compiled.check_returncode()
    subprocess.run([str(binary), str(journal_dir)], check=True, timeout=180)

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
assert "MYSQL *" not in CODEC
assert "mysql_query" not in CODEC
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
    "commit_quarantine_archive",
    "sha256",
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
