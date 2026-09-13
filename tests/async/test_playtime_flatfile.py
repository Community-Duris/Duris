#!/usr/bin/env python3
"""Issue #259: playtime survives real flat-file status merges and stale retries."""
from pathlib import Path
import subprocess
import tempfile
from _paths import rel

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "flatfile/flatfile_player_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "player/player_playtime.h"
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 2);
    std::string root = argv[1], error;
    int32_t pid = 0;
    assert(flatfile_identity_allocate_pid(root, &pid, &error) == flatfile_identity_result::ok);
    assert(pid == 1);
    assert(flatfile_identity_claim(root, pid, "Player", "PlaytimeAccount", &error) == flatfile_identity_result::ok);
    player_snapshot baseline;
    baseline.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    baseline.pid = pid;
    baseline.revision = 1;
    baseline.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    baseline.save_intent = 1;
    baseline.room_vnum = 1201;
    baseline.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
    for (unsigned field = 0; field <= static_cast<unsigned>(player_status_field::last_ip); ++field) {
        const auto tag = static_cast<player_status_field>(field);
        int value = tag >= player_status_field::base_strength && tag <= player_status_field::base_luck ? 50 : 0;
        if (tag == player_status_field::level) value = 1;
        if (tag == player_status_field::played_time) value = 3600;
        baseline.status_integers.push_back({tag, value, 0, false});
    }
    baseline.status_strings.push_back({player_status_string_field::name, "Player"});
    auto apply = [&](const player_snapshot &s) {
        auto result = flatfile_player_snapshot_apply(root, s, &error);
        if (result.outcome == player_save_apply_outcome::terminal_failure)
            std::cerr << error << '\n';
        return result.outcome;
    };
    auto read_total = [&]() {
        player_snapshot loaded;
        assert(flatfile_player_snapshot_load(root, pid, &loaded, &error) == flatfile_player_load_result::ok);
        for (const auto &row : loaded.status_integers)
            if (row.field == player_status_field::played_time)
                return row.is_unsigned ? row.unsigned_value : static_cast<uint64_t>(row.signed_value);
        std::abort();
    };
    assert(apply(baseline) == player_save_apply_outcome::applied);
    assert(read_total() == 3600);
    auto elapsed = baseline;
    elapsed.revision = 2;
    elapsed.components = PLAYER_COMPONENT_STATUS;
    for (auto &row : elapsed.status_integers)
        if (row.field == player_status_field::played_time)
            row.signed_value = player_playtime_total(3600, 10000, 10600);
    assert(apply(elapsed) == player_save_apply_outcome::applied);
    assert(read_total() == 4200);
    assert(apply(elapsed) == player_save_apply_outcome::already_applied);
    assert(read_total() == 4200);
    assert(apply(baseline) == player_save_apply_outcome::stale_revision);
    assert(read_total() == 4200);
    const auto loaded = static_cast<unsigned int>(read_total());
    assert(player_playtime_total(loaded, 100000, 100010) == 4210);
    std::cout << "[PASS] flat-file baseline/status merge, repeated and stale revisions, reload with new session clock\n";
}
'''

SOURCES = [
    "flatfile_player_repository.c", "player_load_topology.c", "flatfile_identity_repository.c",
    "flatfile_item_repository.c", "coin_transfer_command.c", "flatfile_player_snapshot_file.c",
    "flatfile_corpse_repository.c", "flatfile_locker_repository.c", "flatfile_world_item_repository.c",
    "flatfile_artifact_repository.c", "flatfile_shop_trade_repository.c",
    "flatfile_shop_trade_materialization.c", "flatfile_shopkeeper_repository.c",
    "flatfile_auction_repository.c", "flatfile_boon_repository.c",
    "flatfile_player_domain_repository.c", "flatfile_authority_transaction.c",
    "player_snapshot_codec.c", "flatfile_store.c", "item_transfer_command.c",
    "corpse_lifecycle_command.c", "shop_trade_command.c", "critical_command.c", "epic_command.c",
    "currency_command.c", "auction_command.c", "combat_outcome_command.c", "boon_reward_command.c",
    "boon_shop_command.c", "persistence_observability.c", "persistence_mode.c",
    "flatfile_ip_activity_repository.c",
]
with tempfile.TemporaryDirectory(prefix="duris-playtime-flatfile-") as temporary:
    root = Path(temporary)
    source, binary = root / "playtime.cpp", root / "playtime"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-D__NO_MYSQL__", "-ffunction-sections", "-fdata-sections",
                    "-Isrc", "-Isrc/no_mysql", str(source), *[rel(name) for name in SOURCES],
                    "-Wl,--gc-sections", "-lcrypto", "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    for name in ("state", "state/players", "state/identities", "state/identities/names", "state/domains"):
        (root / name).mkdir(mode=0o700)
    subprocess.run([str(binary), str(root / "state")], check=True)
