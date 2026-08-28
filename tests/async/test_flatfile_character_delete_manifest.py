#!/usr/bin/env python3
"""Validate the character-delete disposition inventory and runtime fence."""

import json
import pathlib


ROOT = pathlib.Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/ongoing-projects/flatfile-character-delete-manifest.json"

required_ids = {
    "identity",
    "account_membership",
    "player_snapshot",
    "player_domain",
    "item_custody",
    "boons",
    "recipes",
    "spellbook",
    "offline_messages",
    "auctions",
    "artifacts",
    "locker_access",
    "association_membership",
    "frag_leaderboard",
    "locker_contents",
    "ships_and_cargo",
    "corpses_and_saved_items",
    "account_bound_summons",
    "historical_operation_ledgers",
}
allowed_dispositions = {
    "prepared_tombstone",
    "prepared_remove",
    "prepared_rewrite",
    "checked_empty",
    "covered_by_identity",
    "retain_history",
    "unimplemented",
}
manifest = json.loads(MANIFEST.read_text())
if manifest.get("schema_version") != 1:
    raise SystemExit("unsupported character-delete manifest version")
entries = manifest.get("entries")
if not isinstance(entries, list):
    raise SystemExit("character-delete manifest entries are missing")
entry_ids = [entry.get("id") for entry in entries]
if len(entry_ids) != len(set(entry_ids)):
    raise SystemExit("character-delete manifest contains duplicate IDs")
if set(entry_ids) != required_ids:
    missing = sorted(required_ids - set(entry_ids))
    extra = sorted(set(entry_ids) - required_ids)
    raise SystemExit(f"character-delete manifest coverage drift: missing={missing} extra={extra}")

for entry in entries:
    if entry.get("disposition") not in allowed_dispositions:
        raise SystemExit(f"invalid disposition for {entry['id']}")
    evidence = entry.get("evidence", {})
    relative = pathlib.PurePosixPath(evidence.get("path", ""))
    if relative.is_absolute() or ".." in relative.parts:
        raise SystemExit(f"unsafe evidence path for {entry['id']}")
    path = ROOT / relative
    token = evidence.get("token")
    if not path.is_file() or not isinstance(token, str) or not token or token not in path.read_text():
        raise SystemExit(f"stale evidence for {entry['id']}")

blockers = {entry["id"] for entry in entries if entry["disposition"] == "unimplemented"}
if not blockers or manifest.get("runtime_exposure") != "fenced":
    raise SystemExit("incomplete character deletion is not fenced")

files_source = (ROOT / "src/files.c").read_text()
start = files_source.index("int deleteCharacter(P_char ch, bool bDeleteLocker)")
end = files_source.index("void PurgeCorpseFile", start)
delete_body = files_source[start:end]
runtime_calls = [
    "remove_all_artifacts_sql(ch)",
    "remove_all_locker_access(ch)",
    "GET_ASSOC(ch)->kick(ch)",
    "sql_soft_delete_character(GET_PID(ch))",
    "remove_char_from_list(ch->desc->account",
    "sql_delete_locker(GET_PID(ch), 0)",
    "sql_delete_player(GET_PID(ch))",
    "delete_ship(GET_NAME(ch))",
]
positions = [delete_body.find(call) for call in runtime_calls]
if any(position < 0 for position in positions) or positions != sorted(positions):
    raise SystemExit("live character-delete call graph drifted from the manifest")
if "flatfile_character_delete" in delete_body:
    raise SystemExit("incomplete flat character deletion was exposed through the live route")

coordinator = (ROOT / "src/flatfile_character_delete.c").read_text()
prepared_order = [
    "flatfile_artifact_prepare_player_release",
    "flatfile_frag_leaderboard_prepare_tombstone",
    "flatfile_player_snapshot_prepare_remove",
    "flatfile_player_domain_prepare_remove",
    "flatfile_item_repository_prepare_player_remove",
    "flatfile_boon_prepare_player_remove",
    "flatfile_recipe_prepare_clear",
    "flatfile_spellbook_prepare_clear",
    "flatfile_offline_message_prepare_remove",
    "append_operation(&operations, &identity_operation)",
]
positions = [coordinator.find(token) for token in prepared_order]
if any(position < 0 for position in positions) or positions != sorted(positions):
    raise SystemExit("core delete operation order drifted or identity is not success-last")

print(f"flat-file character-delete manifest passed with {len(blockers)} runtime blockers")
