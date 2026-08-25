"""Contract for incapacitated regen and bounded event deferral."""

from pathlib import Path

from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
limits = (ROOT / "src" / "limits.c").read_text()
new_events = (ROOT / "src" / "new_events.c").read_text()

assert contains(limits, "else if (GET_STAT(ch) > STAT_INCAP)")
assert contains(limits, "if (IS_FIGHTING(ch) || IS_DESTROYING(ch))")
assert contains(new_events, "event->deferral_count >= NEVENT_MAX_DEFERRALS;")
assert contains(new_events, "ordinary events")

print("nevent regen/death and bounded-deferral contracts passed")
