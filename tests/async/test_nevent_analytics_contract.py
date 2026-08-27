from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")

required = [
    "DURIS_NEVENT_ANALYTICS",
    "NEVENT ANALYTICS PULSE:",
    "NEVENT ANALYTICS WINDOW:",
    "nevent_analytics",
    "total_executed",
    "total_deferred",
    "peak_executed",
    "peak_total_us",
    "budget_exhausted_pulses",
    "PULSES_IN_TICK",
    "NEVENT_ANALYTICS_CALLBACK_SLOTS",
    "nevent_analytics_record_callback",
    "nevent_analytics_record_deferred",
    "NEVENT ANALYTICS CALLBACK:",
    "func=%p",
    "avg_us=%.2f",
    "callback_overflow",
]

missing = [snippet for snippet in required if snippet not in source]
assert not missing, f"missing scheduler analytics contract snippets: {missing}"

assert contains(source, "avg_executed")
assert contains(source, "avg_total_us")
assert contains(source, "nevent_analytics_reset")
assert contains(source, "executed")
assert contains(source, "deferred")
assert contains(source, "ne_event_counter")
assert contains(source, "IS_NPC(ch) && GET_MASTER(ch) && IS_AFFECTED5(GET_MASTER(ch), AFF5_ORDERING)")

print("scheduler analytics contract passed")
