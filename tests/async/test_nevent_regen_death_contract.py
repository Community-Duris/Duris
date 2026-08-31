"""Contract for incapacitated regen and bounded event deferral."""

from _paths import SRC
from pathlib import Path

from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
limits = (SRC / "limits.c").read_text()
new_events = (SRC / "new_events.c").read_text()

assert contains(limits, "else if (GET_STAT(ch) > STAT_INCAP)")
assert contains(limits, "if (IS_FIGHTING(ch))")
assert contains(new_events, "NEVENT_NORMAL_AGING_DEFERRALS")
assert contains(new_events, "NEVENT_NORMAL_AGING_TICKS")
assert contains(new_events, "NEVENT_PRIORITY_AGED_NORMAL")
assert contains(new_events, "func == event_ward_regen")

print("nevent regen/death and bounded-deferral contracts passed")
