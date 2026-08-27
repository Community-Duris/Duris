#!/usr/bin/env python3
"""Runtime and source contracts for player revision/component state."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/player_revision_state.h").read_text()
SOURCE = (ROOT / "src/player_revision_state.c").read_text()
SQL_PLAYER = (ROOT / "src/sql_player.c").read_text()


HARNESS = r'''
#include "player_revision_state.h"

#include <cassert>
#include <cstdint>
#include <limits>

int main()
{
    player_revision_reset_for_tests();
    assert(!player_revision_hydrate(0, 0));
    assert(player_revision_hydrate(7, 5));
    assert(player_revision_state_count() == 1);

    player_revision_snapshot snapshot = {};
    assert(player_revision_snapshot_copy(7, &snapshot));
    assert(snapshot.current_revision == 5);
    assert(snapshot.acknowledged_revision == 5);

    player_revision_t revision = 0;
    assert(player_revision_mark(7, PLAYER_COMPONENT_STATUS, &revision));
    assert(revision == 6);
    player_component_mask_t components = 0;
    assert(player_revision_queue(7, &revision, &components));
    assert(revision == 6);
    assert(components == PLAYER_COMPONENT_STATUS);
    assert(player_revision_begin_inflight(7, revision, components));

    assert(player_revision_mark(7, PLAYER_COMPONENT_INVENTORY, &revision));
    assert(revision == 7);
    assert(player_revision_queue(7, &revision, &components));
    assert(revision == 7);
    assert(components == (PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_INVENTORY));

    assert(!player_revision_acknowledge(7, 5, PLAYER_COMPONENT_STATUS));
    assert(player_revision_acknowledge(7, 6, PLAYER_COMPONENT_STATUS));
    assert(player_revision_snapshot_copy(7, &snapshot));
    assert(snapshot.acknowledged_revision == 6);
    assert(snapshot.unacknowledged_components == PLAYER_COMPONENT_INVENTORY);
    assert(snapshot.queued_components == PLAYER_COMPONENT_INVENTORY);

    assert(player_revision_begin_inflight(7, 7, PLAYER_COMPONENT_INVENTORY));
    assert(player_revision_mark(7, PLAYER_COMPONENT_INVENTORY, &revision));
    assert(revision == 8);
    assert(player_revision_acknowledge(7, 7, PLAYER_COMPONENT_INVENTORY));
    assert(player_revision_snapshot_copy(7, &snapshot));
    assert(snapshot.unacknowledged_components == PLAYER_COMPONENT_INVENTORY);
    assert(snapshot.dirty_components == PLAYER_COMPONENT_INVENTORY);

    assert(player_revision_queue(7, &revision, &components));
    assert(player_revision_begin_inflight(7, revision, components));
    assert(player_revision_fail_inflight(7, revision, components));
    assert(player_revision_snapshot_copy(7, &snapshot));
    assert(snapshot.inflight_components == 0);
    assert(snapshot.queued_components == PLAYER_COMPONENT_INVENTORY);

    assert(player_revision_hydrate(7, 7));
    assert(!player_revision_hydrate(7, 6));
    assert(!player_revision_hydrate(7, 9));
    assert(!player_revision_mark(7, PLAYER_PHASE2_ECONOMY_BOUNDARY, nullptr));

    assert(player_revision_hydrate(8, std::numeric_limits<player_revision_t>::max()));
    assert(!player_revision_mark(8, PLAYER_COMPONENT_STATUS, nullptr));
    assert(player_revision_snapshot_copy(8, &snapshot));
    assert(snapshot.current_revision == std::numeric_limits<player_revision_t>::max());
    assert(snapshot.overflowed);

    player_revision_forget(7);
    assert(!player_revision_snapshot_copy(7, &snapshot));
    assert(player_revision_state_count() == 1);
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-player-revision-") as temp_dir:
    source = Path(temp_dir) / "revision_test.cpp"
    binary = Path(temp_dir) / "revision_test"
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
            "src/player_revision_state.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

assert "std::unordered_map<int, player_revision_entry>" in SOURCE
assert "MAX_PLAYER_REVISION_STATES" in SOURCE
assert "component_revisions" in SOURCE
assert "numeric_limits<player_revision_t>::max()" in SOURCE
assert "PLAYER_PHASE2_ECONOMY_BOUNDARY" in HEADER
assert "PLAYER_PHASE2_OWNERSHIP_BOUNDARY" in HEADER
print("[PASS] PID-keyed runtime state is monotonic, cumulative, exact, and overflow-safe")

schemas = (
    ROOT / "migrations/run_migration.sh",
    ROOT / "migrations/pfile_to_db_combined_migration.sql",
    ROOT / "migrations/bootstrap_multithread_safe.sql",
)
for schema in schemas:
    text = schema.read_text()
    if schema.name == "run_migration.sh":
        assert "player_save_revision.sql" in text
    else:
        assert "save_revision" in text
        assert "BIGINT UNSIGNED NOT NULL DEFAULT" in text.upper()

migration = (ROOT / "migrations/player_save_revision.sql").read_text()
assert "information_schema.columns" in migration
assert "column_name = 'save_revision'" in migration
assert "BIGINT UNSIGNED NOT NULL DEFAULT 0" in migration
assert "ALTER TABLE player_data ADD COLUMN" in migration
print("[PASS] additive guarded schema initializes legacy and new rows at revision zero")

load_start = SQL_PLAYER.rindex("bool sql_load_player_status(P_char ch, int pid)")
load_end = SQL_PLAYER.index("bool sql_load_player_skills", load_start)
load_body = SQL_PLAYER[load_start:load_end]
assert 'last_ip, save_revision "' in load_body
assert "sql_row_revision" in load_body
assert "!revision_valid || !player_revision_hydrate" in load_body
assert "outcome=hydrate_failure" in load_body

save_start = SQL_PLAYER.rindex("bool sql_save_player_status(P_char ch, int type, int room)")
save_end = SQL_PLAYER.index("bool sql_save_player_skills", save_start)
save_body = SQL_PLAYER[save_start:save_end]
pid_assignment = save_body.index("mysql_insert_id")
initialization = save_body.index("player_revision_hydrate(pid, 0)")
assert pid_assignment < initialization

delete_start = SQL_PLAYER.rindex("bool sql_delete_player(int pid)")
delete_end = SQL_PLAYER.index("bool sql_delete_player_by_name", delete_start)
delete_body = SQL_PLAYER[delete_start:delete_end]
assert delete_body.index("sql_run_query") < delete_body.index("player_revision_forget")
assert SQL_PLAYER.count("player_revision_forget(pid);") == 1

rename_start = SQL_PLAYER.rindex("bool sql_player_rename(P_char ch, const char *new_name)")
rename_end = SQL_PLAYER.index("int sql_get_player_pid", rename_start)
rename_body = SQL_PLAYER[rename_start:rename_end]
assert "player_revision_forget" not in rename_body
assert "player_revision_hydrate" not in rename_body
print("[PASS] required load/new/delete lifecycle is PID-stable and fail-closed")

production_sources = [
    path
    for path in (ROOT / "src").glob("*.c")
    if path.name != "player_revision_state.c"
]
mark_callers = [path.name for path in production_sources if "player_revision_mark(" in path.read_text()]
assert sorted(mark_callers) == ["player_save_pipeline.c", "sql_player.c"], (
    f"uncontrolled production marks: {mark_callers}"
)
assert "writeCharacter" not in SOURCE
assert "sql_save_player" not in SOURCE
assert "player_save_pipeline_mark" in (ROOT / "src/redis.c").read_text()
print("[PASS] production marks are limited to the pipeline and fenced legacy compatibility")

print("player revision and component state contracts passed")
