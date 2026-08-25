from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")
structs = (ROOT / "src" / "structs.h").read_text(encoding="utf-8", errors="replace")

assert contains(source, "nevent_is_player_timed")
assert contains(source, "NEVENT_PRIORITY_PLAYER")
assert contains(structs, "scheduled_tick")
assert contains(source, "event->priority")
assert contains(source, "PLAYER EVENT TIMING:")
assert contains(source, "nevent_link_schedule")
assert contains(source, "last_player")
assert contains(structs, "deferral_count")
assert contains(source, "NEVENT_MAX_DEFERRALS      0U")
assert contains(source, "nevent_promote_overdue_player")
assert contains(source, "event_wait")
assert contains(source, "func == event_wait")
print("player event timing priority contract passed")
