#!/usr/bin/env python3
"""Regression coverage for self-healing saved item placement."""

from _paths import SRC, rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "player/player_load_repository.h"

#include <cassert>
#include <vector>

namespace
{
player_item_snapshot item(uint64_t uid, int32_t parent = PLAYER_SNAPSHOT_NO_PARENT)
{
    player_item_snapshot value = {};
    value.object_uid = uid;
    value.vnum = static_cast<int32_t>(uid);
    value.parent_index = parent;
    return value;
}

player_load_item_identity identity(uint64_t database_id, uint64_t uid, uint64_t root,
                                   uint64_t parent, uint64_t serialized_parent)
{
    player_load_item_identity value = {};
    value.database_id = database_id;
    value.serialized_parent_id = serialized_parent;
    value.item_uid = uid;
    value.root_item_uid = root;
    value.parent_item_uid = parent;
    value.owner = { item_owner_type::player, 42, 0 };
    value.state = item_custody_state::active;
    return value;
}

void reconcile(std::vector<player_item_snapshot> *items,
               std::vector<player_load_item_identity> *identities,
               size_t *promoted, size_t *repaired)
{
    assert(player_load_reconcile_item_topology(items, identities, promoted, repaired));
}
}

int main()
{
    // Exact historical failure: the projection says the consumable is in a bag,
    // while authoritative custody says it is top-level. Loading must un-nest it.
    {
        std::vector<player_item_snapshot> items = { item(100), item(101, 0) };
        std::vector<player_load_item_identity> identities = {
            identity(10, 100, 100, 0, 0), identity(11, 101, 101, 0, 10)
        };
        size_t promoted = 0, repaired = 0;
        reconcile(&items, &identities, &promoted, &repaired);
        assert(promoted == 0 && repaired == 1);
        assert(items[1].parent_index == PLAYER_SNAPSHOT_NO_PARENT);
        assert(identities[1].serialized_parent_id == 0);
    }

    // The opposite lag is also recoverable: custody moved the item into the bag
    // before the replacement projection landed.
    {
        std::vector<player_item_snapshot> items = { item(100), item(101) };
        std::vector<player_load_item_identity> identities = {
            identity(10, 100, 100, 0, 0), identity(11, 101, 100, 100, 0)
        };
        size_t promoted = 0, repaired = 0;
        reconcile(&items, &identities, &promoted, &repaired);
        assert(promoted == 0 && repaired == 1);
        assert(items[1].parent_index == 0);
        assert(identities[1].serialized_parent_id == 10);
    }

    // If the two sources name different existing bags, custody wins.
    {
        std::vector<player_item_snapshot> items = { item(100), item(200), item(101, 0) };
        std::vector<player_load_item_identity> identities = {
            identity(10, 100, 100, 0, 0), identity(20, 200, 200, 0, 0),
            identity(11, 101, 200, 200, 10)
        };
        size_t promoted = 0, repaired = 0;
        reconcile(&items, &identities, &promoted, &repaired);
        assert(promoted == 0 && repaired == 1);
        assert(items[2].parent_index == 1);
        assert(identities[2].serialized_parent_id == 20);
    }

    // Contents survive a missing authoritative parent by moving to top level.
    {
        std::vector<player_item_snapshot> items = { item(101) };
        std::vector<player_load_item_identity> identities = {
            identity(11, 101, 999, 999, 0)
        };
        size_t promoted = 0, repaired = 0;
        reconcile(&items, &identities, &promoted, &repaired);
        assert(promoted == 1 && repaired == 0);
        assert(items[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT);
        assert(identities[0].root_item_uid == 101 && identities[0].parent_item_uid == 0);
    }

    // A custody cycle is real corruption and must still be refused.
    {
        std::vector<player_item_snapshot> items = { item(100), item(101) };
        std::vector<player_load_item_identity> identities = {
            identity(10, 100, 100, 101, 11), identity(11, 101, 100, 100, 10)
        };
        size_t promoted = 0, repaired = 0;
        assert(!player_load_reconcile_item_topology(
            &items, &identities, &promoted, &repaired));
    }
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-player-load-topology-") as temp_dir:
    source = Path(temp_dir) / "player_load_topology_test.cpp"
    binary = Path(temp_dir) / "player_load_topology_test"
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
            rel("player_load_topology.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=20)

repository = (SRC / "player_load_repository.c").read_text()
flatfile = (SRC / "flatfile_player_repository.c").read_text()
materialize = (SRC / "player_load_materialize.c").read_text()
for source in (repository, flatfile):
    assert "player_load_reconcile_item_topology" in source
assert "outcome=topology_repaired" in materialize
assert "recovery=next_full_save" in materialize

print("player-load item topology self-healing passed")
