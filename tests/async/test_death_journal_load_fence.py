#!/usr/bin/env python3
"""Real journal replay and materializer regressions for startup load admission."""

from _paths import SRC, rel
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (SRC / "player_save_pipeline.c").read_text()
PIPELINE_H = (SRC / "player_save_pipeline.h").read_text()
MATERIALIZE = (SRC / "player_load_materialize.c").read_text()
ACCOUNT = (SRC / "account.c").read_text()
NANNY = (SRC / "nanny.c").read_text()
COPYOVER = (SRC / "copyover.c").read_text()


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for end in range(opening, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
            if depth == 0:
                return text[start : end + 1]
    raise AssertionError(signature)


HARNESS = r'''
#include "classes/necromancy.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "guild/assocs.h"
#include "item/item_ownership_runtime.h"
#include "player/player_load_items.h"
#include "player/player_load_materialize.h"
#include "player/player_load_pets.h"
#include "player/player_save_journal.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot_codec.h"
#include "world/db.h"
#include "world/vnum.obj.h"
#include "world/quest_reward_recovery.h"

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

player_save_pipeline_replay_gate replay_gate;
int reset_count = 0;
int item_materialize_count = 0;
int pet_stage_count = 0;
P_room world = nullptr;
bool item_movement_transaction_pending_spell_effects(uint32_t, std::vector<critical_operation_id> *) { return true; }
bool item_movement_transaction_pending_craft_progression(uint32_t, std::vector<critical_operation_id> *) { return true; }
void spell_component_retirement_recover_receipts(uint32_t, const player_load_spell_effect_receipt *, size_t) {}
void quest_reward_recover_pending(P_char, const critical_operation_id &, const quest_reward_continuation &, uint64_t, uint64_t, bool) {}
void quest_reward_recover_xp_entitlement(P_char, const critical_operation_id &, const quest_reward_continuation &, uint32_t, uint32_t) {}

bool player_save_pipeline_loads_allowed(void)
{
    return replay_gate.loads_allowed();
}

void reset_char(P_char) { ++reset_count; }
int real_mobile(int) { return -1; }
int real_room(int) { return NOWHERE; }
char affect_total(P_char, int) { return 0; }
affected_type *affect_to_char(P_char, affected_type *) { return nullptr; }
void affect_to_char_with_messages(P_char, affected_type *, const char *, const char *) {}
int BOUNDED(int low, int value, int high) { return value < low ? low : value > high ? high : value; }
char *str_dup(const char *value)
{
    const size_t size = std::strlen(value) + 1;
    auto *copy = static_cast<char *>(std::malloc(size));
    if (copy) std::memcpy(copy, value, size);
    return copy;
}
void logit(const char *, const char *, ...) {}
void wizlog(int, const char *, ...) {}
P_Guild get_guild_from_id(int) { return nullptr; }
bool player_revision_hydrate(int, player_revision_t) { return true; }
void gameplay_read_state_reset(gameplay_read_state *) {}
bool gameplay_read_state_publish(gameplay_read_state *, const int64_t *, size_t,
                                 const int32_t *, size_t) { return true; }
void player_load_items_discard(P_char) {}
bool player_load_items_materialize(P_char, const player_load_result &,
                                   player_load_item_materialize_metrics *)
{
    ++item_materialize_count;
    return true;
}
bool player_load_item_graph_materialize(P_char, const std::vector<player_item_snapshot> &,
                                        const std::vector<player_load_item_identity> &, int32_t,
                                        uint64_t, bool,
                                        player_load_item_materialize_metrics *)
{
    ++item_materialize_count;
    return true;
}
bool player_load_pets_stage(P_char, const player_load_result &, std::vector<P_char> *,
                            player_load_pet_materialize_metrics *)
{
    ++pet_stage_count;
    return true;
}
void player_load_pets_commit(P_char, std::vector<P_char> *, const player_load_result &) {}
void player_load_pets_discard(std::vector<P_char> *) {}
bool item_ownership_runtime_hydrate_owner(const item_owner_identity &, uint64_t) { return true; }
bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *, size_t)
{
    return true;
}
void item_ownership_runtime_forget_player_domain(uint32_t) {}

player_snapshot ordinary_snapshot(int pid, player_revision_t revision)
{
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    snapshot.save_intent = 4;
    snapshot.room_vnum = 1201;
    snapshot.encoded_size_bound = 8192;
    snapshot.status_integers.push_back({player_status_field::level, 50, 0, false});
    snapshot.status_strings.push_back({player_status_string_field::name, "journal-player"});
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

player_snapshot terminal_death_snapshot()
{
    player_snapshot death = ordinary_snapshot(80, 4);
    player_item_snapshot bag = death.items[0];
    death.schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
    player_quest_xp_receipt_snapshot xp = {};
    xp.offering_operation.bytes[0] = 88;
    xp.amount = 75;
    death.quest_xp_receipts.push_back(xp);
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
        {{1001, 1001, 0, 8, 500, item_custody_state::active},
         {item_owner_type::player, 80, 0}, 10},
        {{1002, 1001, 1001, ITEM_TRANSFER_ABSENT_REVISION, 3, item_custody_state::absent}, {}, 0},
        {{1003, 1003, 0, ITEM_TRANSFER_ABSENT_REVISION, 3, item_custody_state::absent}, {}, 0},
        {{1004, 1001, 1001, 9, 3, item_custody_state::active},
         {item_owner_type::player, 80, 0}, 10},
    };
    return death;
}

struct apply_state
{
    bool reject = false;
    int calls = 0;
    int conflict_rows = 0;
};

player_save_apply_result apply_snapshot(const player_snapshot &snapshot, void *raw)
{
    auto &state = *static_cast<apply_state *>(raw);
    ++state.calls;
    assert(snapshot.pid == 80 || snapshot.pid == 81);
    if (snapshot.death) {
        assert(snapshot.schema_version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION);
        assert(snapshot.quest_xp_receipts.size() == 1 && snapshot.quest_xp_receipts[0].amount == 75);
    }
    if (state.reject)
        return {player_save_apply_outcome::retryable_failure, snapshot.revision - 1, 1205};
    return {player_save_apply_outcome::applied, snapshot.revision, 0};
}

player_load_result stale_cold_load()
{
    player_load_result result = {};
    result.pid = 80;
    result.outcome = player_load_outcome::applied;
    result.read_components = PLAYER_LOAD_SESSION04_READS;
    result.authoritative_item_count = 1;
    result.snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    result.snapshot.pid = 80;
    result.snapshot.revision = 3;
    result.snapshot.save_intent = 4;
    result.snapshot.encoded_size_bound = 8192;
    result.snapshot.components = PLAYER_LOAD_SESSION03_COMPONENTS;
    for (unsigned int field = 0; field < 63; ++field)
    {
        const auto kind = static_cast<player_status_field>(field);
        const int64_t value = kind == player_status_field::level ? 61 : 0;
        result.snapshot.status_integers.push_back({kind, value, 0, false});
    }
    const char *strings[] = {"stale-sql-character", "", "", "", "", "", ""};
    for (unsigned int field = 0; field < 7; ++field)
        result.snapshot.status_strings.push_back(
            {static_cast<player_status_string_field>(field), strings[field]});
    player_item_snapshot item = {};
    item.object_uid = 501;
    item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    item.vnum = 500;
    item.name = "stale sql item";
    result.snapshot.items.push_back(item);
    player_load_item_identity item_identity = {};
    item_identity.item_uid = 501;
    item_identity.root_item_uid = 501;
    item_identity.owner = {item_owner_type::player, 80, 0};
    item_identity.item_revision = 1;
    item_identity.owner_revision = 1;
    item_identity.state = item_custody_state::active;
    result.item_identities.push_back(item_identity);
    player_pet_snapshot pet = {};
    pet.mob_vnum = 700;
    pet.room_vnum = 1201;
    result.snapshot.pets.push_back(pet);
    player_load_pet_identity pet_identity = {};
    pet_identity.database_id = 1;
    pet_identity.pet_uid = 9001;
    pet_identity.owner_revision = 1;
    result.pet_identities.push_back(pet_identity);
    return result;
}

player_load_result healthy_load()
{
    player_load_result result = stale_cold_load();
    result.pid = 81;
    result.snapshot.pid = 81;
    result.snapshot.revision = 1;
    result.snapshot.status_strings[0].value = "healthy-player";
    result.item_identities[0].owner = {item_owner_type::player, 81, 0};
    return result;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string root = argv[1];
    const std::string blocked_dir = root + "/blocked";
    const std::string healthy_dir = root + "/healthy";
    assert(mkdir(root.c_str(), 0700) == 0);

    // Startup and replay-in-progress both deny materialization before any SQL conflict row.
    assert(!replay_gate.loads_allowed());
    assert(player_save_journal_init(blocked_dir.c_str()));
    const player_snapshot death = terminal_death_snapshot();
    std::vector<uint8_t> death_bytes;
    const auto death_encoded = player_snapshot_encode(death, &death_bytes);
    assert(death_encoded == player_snapshot_codec_result::ok);
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    player_save_journal_shutdown();
    assert(player_save_journal_init(blocked_dir.c_str()));
    replay_gate.begin_replay();
    assert(!replay_gate.loads_allowed());
    char_data legacy_character = {};
    pc_only_data legacy_pc = {};
    legacy_character.only.pc = &legacy_pc;
    legacy_character.player.level = 9;
    legacy_pc.pid = 900;
    player_load_result stale = stale_cold_load();
    assert(!player_load_materialize(&legacy_character, stale));
    assert(legacy_character.player.level == 9 && legacy_pc.pid == 900);
    assert(reset_count == 0 && item_materialize_count == 0 && pet_stage_count == 0);

    apply_state blocked{true, 0, 0};
    const auto blocked_result = player_save_journal_replay(apply_snapshot, &blocked);
    replay_gate.finish_replay(blocked_result == player_save_journal_result::ok);
    assert(blocked_result == player_save_journal_result::replay_blocked);
    assert(blocked.calls == 1 && blocked.conflict_rows == 0);
    assert(player_save_journal_health_copy().records == 1);
    assert(!replay_gate.loads_allowed());
    assert(!player_load_materialize(&legacy_character, stale));
    assert(legacy_character.player.level == 9 && legacy_pc.pid == 900);
    assert(reset_count == 0 && item_materialize_count == 0 && pet_stage_count == 0);
    player_save_journal_shutdown();

    // A successful real replay publishes readiness, after which a valid load materializes.
    assert(player_save_journal_init(healthy_dir.c_str()));
    assert(player_save_journal_append(ordinary_snapshot(81, 1)) == player_save_journal_result::ok);
    player_save_journal_shutdown();
    assert(player_save_journal_init(healthy_dir.c_str()));
    replay_gate.begin_replay();
    apply_state accepted{false, 0, 0};
    const auto healthy_result = player_save_journal_replay(apply_snapshot, &accepted);
    assert(healthy_result == player_save_journal_result::ok);
    replay_gate.finish_replay(healthy_result == player_save_journal_result::ok);
    assert(replay_gate.loads_allowed() && player_save_journal_health_copy().records == 0);

    char_data healthy_character = {};
    pc_only_data healthy_pc = {};
    healthy_character.only.pc = &healthy_pc;
    const player_load_result loaded = healthy_load();
    assert(player_load_materialize(&healthy_character, loaded));
    assert(healthy_character.player.level == 61 && healthy_pc.pid == 81);
    assert(reset_count == 1 && item_materialize_count > 0 && pet_stage_count > 0);
    player_save_journal_shutdown();
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="death-journal-load-fence-") as temp_dir:
    temp = Path(temp_dir)
    source = temp / "replay_load_fence.cpp"
    binary = temp / "replay_load_fence"
    source.write_text(HARNESS)
    compiled = subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-pthread", "-Isrc", str(source),
            rel("player_snapshot_codec.c"), rel("player_save_journal.c"),
            rel("player_load_materialize.c"), "-o", str(binary),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if compiled.returncode:
        raise RuntimeError(compiled.stderr)
    journal_root = temp / "runtime-journals"
    subprocess.run([str(binary), str(journal_root)], check=True, timeout=20)

assert "player_save_pipeline_replay_gate" in PIPELINE_H
assert "player_save_pipeline_loads_allowed" in PIPELINE_H
assert "replay_gate.finish_replay(replay == player_save_journal_result::ok &&" in PIPELINE
assert "health.initialized && !stop_requested" in PIPELINE
assert "replay_gate.begin_replay()" in PIPELINE
shutdown = function_body(PIPELINE, "void player_save_pipeline_shutdown(void)")
assert shutdown.index("replay_gate.begin_replay()") < shutdown.index("stop_requested = true")
assert "player_save_pipeline_loads_allowed()" in MATERIALIZE
assert MATERIALIZE.index("player_save_pipeline_loads_allowed()") < MATERIALIZE.index("reset_char(ch)")
for path, source_text in (("account", ACCOUNT), ("legacy", NANNY), ("copyover", COPYOVER)):
    assert "player_load_materialize(" in source_text, path
assert "account_death_recovery_query_complete" in ACCOUNT
recovery_handler = function_body(ACCOUNT, "void account_death_recovery_query_complete")
assert "player_load_materialize(" not in recovery_handler
print("PASS: durable terminal journal replay refusal blocks real cold-load materialization; healthy replay permits it")
