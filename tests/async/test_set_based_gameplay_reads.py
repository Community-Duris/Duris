#!/usr/bin/env python3
"""Runtime and source contracts for set-based PvP and epic-task reads."""

from _paths import SRC, rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPOSITORY = (SRC / "player_load_repository.c").read_text()
MATERIALIZE = (SRC / "player_load_materialize.c").read_text()
FIGHT = (SRC / "fight.c").read_text()
EPIC = (SRC / "epic.c").read_text()
COMM = (SRC / "comm.c").read_text()
COPYOVER = (SRC / "copyover.c").read_text()

HARNESS = r'''
#include "epic_task_catalog.h"
#include "gameplay_read_state.h"

#include <cassert>
#include <cstdint>

int real_zone0(const int zone) { return zone == 999 ? -1 : zone; }

int number(int from, int to)
{
    static uint32_t state = 0x8a5cd789U;
    if (from == to)
        return from;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return from + static_cast<int>(state % static_cast<uint32_t>(to - from + 1));
}

int main()
{
    gameplay_read_state state = {};
    int seconds = 0;
    assert(!gameplay_read_state_heaven_seconds(&state, 5000, 100, &seconds));

    const int64_t deaths[] = {4999, 4900, 1401, 1400};
    const int32_t completed[] = {20};
    assert(gameplay_read_state_publish(&state, deaths, 4, completed, 1));
    assert(gameplay_read_state_recent_count(&state, 5000) == 3);
    assert(gameplay_read_state_heaven_seconds(&state, 5000, 100, &seconds));
    assert(seconds == 400);
    assert(gameplay_read_state_heaven_seconds(&state, 5000, 500, &seconds));
    assert(seconds == 48000);

    const int64_t ascending[] = {10, 11};
    assert(!gameplay_read_state_publish(&state, ascending, 2, completed, 1));
    assert(gameplay_read_state_recent_count(&state, 5000) == 3);
    assert(gameplay_read_state_zone_completed(&state, 20));
    assert(!gameplay_read_state_zone_completed(&state, 30));
    assert(gameplay_read_state_add_completed_zone(&state, 30));
    assert(gameplay_read_state_add_completed_zone(&state, 30));
    assert(state.completed_zone_count == 2);

    const uint64_t rejected = gameplay_read_state_add_provisional(&state, 5001);
    assert(rejected && gameplay_read_state_recent_count(&state, 5001) == 3);
    assert(gameplay_read_state_finish_provisional(&state, rejected, false));
    assert(gameplay_read_state_recent_count(&state, 5001) == 2);
    const uint64_t committed = gameplay_read_state_add_provisional(&state, 5002);
    assert(committed && gameplay_read_state_finish_provisional(&state, committed, true));
    assert(gameplay_read_state_recent_count(&state, 5002) == 3);
    assert(!gameplay_read_state_finish_provisional(&state, committed, true));

    epic_task_catalog_reset_for_tests();
    assert(epic_task_catalog_select(&state) == -1);
    const int32_t zones[] = {10, 20, 30, 40};
    assert(epic_task_catalog_publish(zones, 4));
    assert(epic_task_catalog_ready() && epic_task_catalog_size() == 4);
    const int32_t invalid[] = {10, 999};
    assert(!epic_task_catalog_publish(invalid, 2));
    assert(epic_task_catalog_size() == 4);

    int selected_10 = 0;
    int selected_other = 0;
    for (int index = 0; index < 12000; ++index)
    {
        const int selected = epic_task_catalog_select(&state);
        assert(selected == 10 || selected == 40);
        if (selected == 10)
            ++selected_10;
        else
            ++selected_other;
    }
    assert(selected_10 > 4800 && selected_10 < 7200);
    assert(selected_other > 4800 && selected_other < 7200);
    assert(gameplay_read_state_add_completed_zone(&state, 10));
    assert(gameplay_read_state_add_completed_zone(&state, 40));
    assert(epic_task_catalog_select(&state) == -1);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-gameplay-reads-") as temp_dir:
    source = Path(temp_dir) / "gameplay_reads_test.cpp"
    binary = Path(temp_dir) / "gameplay_reads_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
            "-D__NO_MYSQL__", "-Isrc", str(source),
            rel("gameplay_read_state.c"), rel("epic_task_catalog.c"),
            "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=10)

assert "PLAYER_LOAD_QUERY_MAX = 22" in (SRC / "player_load_repository.h").read_text()
assert "FROM pkill_info pi JOIN pkill_event pe" in REPOSITORY
assert "ORDER BY pe.stamp DESC, pi.id DESC LIMIT 20" in REPOSITORY
assert "FROM epic_gain" in REPOSITORY and "UNION" in REPOSITORY
assert "load_gameplay_reads(connection, &result)" in REPOSITORY
assert "gameplay_read_state_publish" in MATERIALIZE
assert "PLAYER_LOAD_SESSION04_READS" in MATERIALIZE

heaven = FIGHT[FIGHT.index("void setHeavenTime") : FIGHT.index("static combat_outcome_participant")]
assert "gameplay_read_state_heaven_seconds" in heaven
assert "qry(" not in heaven and "mysql_" not in heaven and "redis_" not in heaven
assert "gameplay_read_state_add_provisional" in FIGHT
assert "gameplay_read_state_finish_provisional" in FIGHT

task = EPIC[
    EPIC.index("int epic_random_task_zone") : EPIC.index("void epic_choose_new_epic_task")
]
assert "epic_task_catalog_select" in task
assert "ORDER BY RAND" not in task and "qry(" not in task and "mysql_" not in task
assert "gameplay_read_state_add_completed_zone" in EPIC
assert "epic_task_catalog_refresh" in COMM
assert "include_gameplay_reads" not in COPYOVER

print("set-based PvP and epic-task read contracts passed")
