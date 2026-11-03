#!/usr/bin/env python3
"""Deleting a character works when the zone-story catalog is disabled and nothing is stored.

erase_character() failed whenever the catalog had not booted, so every character deletion
in a world without zone-story content (the minimal journey worlds, or a boot whose catalog
failed) ended in "Deletion could not be confirmed". With the catalog disabled nothing is
tracked; the erase now succeeds when no zone-story state has ever been stored, and still
fails closed when an earlier boot stored some, which only the catalog can rewrite.
run_mysql_deletion_journey.py deletes a character this way.
"""
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"
runtime = (SRC / "world/zone_story_quest_runtime.c").read_text()


def body(text, signature):
    start = text.index(signature)
    return text[start:text.index("\n}\n", start) + 3]


erase = body(runtime, "bool erase_character(uint32_t pid, std::string *error)")
assert erase.index("if (!pid)") < erase.index("if (!ready())")
disabled = erase[erase.index("if (!ready())"):erase.index("const std::string before")]
assert "return absent || fail(error," in " ".join(disabled.split())
assert "flatfile_zone_story_quest_result::not_found" in disabled
assert "sql_zone_story_quest_state_result::not_found" in disabled
print("[PASS] disabled journal erasure preserves stored state and accepts absent state")
