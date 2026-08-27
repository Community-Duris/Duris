from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")
structs = (ROOT / "src" / "structs.h").read_text(encoding="utf-8", errors="replace")

assert contains(source, "nevent_is_player_timed")
assert contains(source, "NEVENT_PRIORITY_PLAYER")
assert contains(structs, "due_tick")
assert contains(source, "event->priority")
assert contains(source, "PLAYER EVENT TIMING:")
assert contains(source, "nevent_link_schedule")
assert contains(source, "nevent_sorts_before")
assert contains(source, "left_priority > right_priority")
assert contains(structs, "deferral_count")
assert contains(source, "NEVENT_NORMAL_AGING_DEFERRALS")
assert contains(source, "NEVENT_PRIORITY_AGED_NORMAL")
assert contains(source, "event_wait")
assert contains(source, "func == event_wait")
assert contains(source, "func == event_ward_regen")
print("player event timing priority contract passed")
