#!/usr/bin/env python3
"""Contracts for truthful deferred flush and save helper results."""

from _paths import SRC
from pathlib import Path

text = (SRC / "actoth.c").read_text()

save = text[text.index("bool do_save_silent"):text.index("void do_save(", text.index("bool do_save_silent"))]
flush = text[text.index("bool persistence_flush_character_saves"):text.index(
    "bool persistence_flush_all_character_saves", text.index("bool persistence_flush_character_saves")
)]
flush_all = text[text.index("bool persistence_flush_all_character_saves"):text.index(
    "struct persistence_deferred_save_snapshot", text.index("bool persistence_flush_all_character_saves")
)]

checks = {
    "save helper reports character failure": "return false;" in save,
    "save helper reports ship failure": "if (!write_ship(ship))" in save,
    "direct flush captures result": "bool saved = do_save_silent" in flush,
    "direct flush clears only success": flush.index("if (saved)") < flush.index("memset(slot, 0"),
    "direct flush retains and rearms failure": "schedule_deferred_save" in flush,
    "direct flush reports outcome": "deferred_save_flush_failed" in flush,
    "global flush aggregates failure": "bool all_saved = true;" in flush_all and
                                       "all_saved = false;" in flush_all,
    "global flush clears only success": "if (do_save_silent" in flush_all and
                                        "memset(slot, 0" in flush_all,
    "missing global character is categorical": "deferred_save_discard_global" in flush_all,
    "capacity fallback checks result": "Deferred player save synchronous fallback failed" in text,
}

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())
print("deferred save flush checks passed")
