#!/usr/bin/env python3
"""Source contracts for bounded periodic maintenance callbacks."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text()


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


artifact = source("src/artifact.c")
drannak = source("src/drannak.c")
hardcore = source("src/hardcore.c")
periodic = source("src/nevent_periodic.c")
redis = source("src/redis.c")
checkpoint = source("src/persistence_checkpoint.c")
ship_base = source("src/ships/ship_base.c")
events = source("src/new_events.c")

poof = function_body(artifact, "void event_artifact_check_poof_sql(")
assert "ARTIFACT_EXPIRY_BATCH_SIZE = 1" in artifact
assert "ORDER BY vnum LIMIT %zu" in poof
assert "AND vnum = %d" in poof
assert "nevent_periodic_continue_after(1);" in poof

wars = function_body(artifact, "void event_artifact_wars_sql(")
assert "ARTIFACT_WARS_OWNER_BATCH_SIZE = 4" in artifact
assert "GROUP BY location HAVING" in wars
assert "ORDER BY location LIMIT %zu" in wars
assert "UPDATE artifacts SET timer = FROM_UNIXTIME" in wars
assert "nevent_periodic_continue_after(1);" in wars
assert "arti_list" not in artifact and "add_artidata_to_list" not in artifact

binding = function_body(artifact, "void event_artifact_check_bind_sql(")
assert "ARTIFACT_BIND_BATCH_SIZE = 8" in artifact
assert "WHERE vnum > %d ORDER BY vnum LIMIT %zu" in binding
assert "nevent_periodic_continue_after(1);" in binding

dirty = function_body(checkpoint, "void event_flush_dirty_players(P_char /*ch*/")
assert "DIRTY_PLAYER_BATCH_SIZE = 8" in dirty
assert "character->runtime_id" in dirty
assert "find_character_by_runtime_id" in dirty
assert "nevent_periodic_continue_after(1);" in dirty
assert "flush_dirty_players();" not in dirty

surnames = function_body(drannak, "void event_update_surnames(")
assert "SURNAME_UPDATE_BATCH_SIZE = 4" in surnames
assert surnames.count("update_shipfrags();") == 1
assert "character->runtime_id" in surnames
assert "find_character_by_runtime_id" in surnames
assert "getLeaderBoardPtsWithShipFrags" in surnames
assert "nevent_periodic_continue_after(1);" in surnames
assert "long getLeaderBoardPtsWithShipFrags(" in hardcore

ship_refresh = function_body(ship_base, "void update_shipfrags()")
ship_score = function_body(drannak, "int calculate_shipfrags(")
assert ship_refresh.count("shipObjHash.get_first") == 1
assert "for (int i = 0; i < 20; i++)" in ship_score

complete = function_body(periodic, "void nevent_periodic_complete(")
continuation = function_body(periodic, "void nevent_periodic_continue_after(")
assert "continuation_slices++" in complete
assert "completed_runs++" in complete
assert "run_continuation = true" in continuation
assert "job.completed_runs" in events and "job.continuation_slices" in events

print("bounded nevent maintenance slicing contracts passed")
