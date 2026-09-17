#!/usr/bin/env python3
"""Offline contract for the durable zone-story quest state schema."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = ROOT / "migrations/immutable/0025_zone_story_quest_state.sql"
VERIFIER = ROOT / "migrations/immutable/0025_zone_story_quest_state.sh"


def main() -> None:
    migration = MIGRATION.read_text()
    verifier = VERIFIER.read_text()
    assert "CREATE TABLE IF NOT EXISTS zone_story_quest_state" in migration
    for token in (
        "state_id TINYINT UNSIGNED NOT NULL",
        "state_version INT UNSIGNED NOT NULL",
        "catalog_revision INT UNSIGNED NOT NULL",
        "state_blob MEDIUMTEXT NOT NULL",
        "updated_at TIMESTAMP(6) NOT NULL",
        "PRIMARY KEY (state_id)",
        "chk_zone_story_quest_state_singleton",
        "chk_zone_story_quest_state_version",
        "chk_zone_story_quest_state_revision",
        "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci",
    ):
        assert token in migration, token
    for token in (
        "zone_story_quest_state",
        "state_blob",
        "state_version",
        "catalog_revision",
        "singleton",
        "checks",
    ):
        assert token in verifier, token

    manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
    step = manifest["migrations"][-1]
    assert step["id"] == "0025_zone_story_quest_state"
    assert step["sequence"] == 25
    assert step["apply_checksum"] == hashlib.sha256(MIGRATION.read_bytes()).hexdigest()
    assert step["verify_checksum"] == hashlib.sha256(VERIFIER.read_bytes()).hexdigest()

    runtime = json.loads(
        (ROOT / "migrations/runtime_compatibility_manifest.json").read_text()
    )
    assert runtime["current_table_count"] == 201
    assert "'zone_story_quest_state'" in runtime["runtime_table_sql_list"]
    assert runtime["migration_head"]["id"] == step["id"]

    lifecycle = json.loads((ROOT / "migrations/data_lifecycle_manifest.json").read_text())
    entry = next(item for item in lifecycle["entries"]
                 if item["id"] == "database:zone_story_quest_state")
    assert entry["locator"] == "zone_story_quest_state"
    assert entry["protected_record"] is True
    assert entry["terminal_action"] == "retain"
    flatfile_entry = next(item for item in lifecycle["entries"]
                          if item["id"] == "file:zone-story-quests")
    assert flatfile_entry["locator"] == "FLATFILE_ROOT/domains/zone-story-quests.state"
    assert flatfile_entry["protected_record"] is True
    assert flatfile_entry["terminal_action"] == "retain"

    repository = (ROOT / "src/sql/zone_story_quest_state_repository.c").read_text()
    assert "SELECT state_version,catalog_revision,state_blob" in repository
    assert "FROM zone_story_quest_state" in repository
    assert "ON DUPLICATE KEY UPDATE" in repository
    print("zone-story quest state schema contract passed")


if __name__ == "__main__":
    main()
