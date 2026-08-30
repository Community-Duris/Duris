#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "actoth.c").read_text()

manual_save = source[source.index("void do_save(") : source.index("void do_not_here(")]
status_event = source[
    source.index("static void event_manual_character_save_status(", source.index("static void event_manual_character_save_status(") + 1) :
    source.index("static struct deferred_save_slot *find_deferred_save_slot")
]

assert '"Save already queued for %s.\\r\\n"' in manual_save
assert '"Save queued for %s.\\r\\n"' in manual_save
assert manual_save.index("find_manual_save_status") < manual_save.index("persistence_schedule_character_save")
assert "revision.acknowledged_revision >= status->revision" in status_event
assert '"Save complete for %s.\\r\\n"' in status_event
assert '"Save failed for %s; please try again.\\r\\n"' in status_event

print("[PASS] manual saves report queued, duplicate, completion, and failure states")
