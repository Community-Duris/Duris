from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")

required = [
    "DURIS_NEVENT_ANALYTICS",
    "NEVENT ANALYTICS PULSE:",
    "NEVENT ANALYTICS MINUTE:",
    "nevent_analytics",
    "total_executed",
    "total_deferred",
    "peak_executed",
    "peak_total_us",
    "budget_exhausted_pulses",
    "PULSES_IN_TICK",
]

missing = [snippet for snippet in required if snippet not in source]
assert not missing, f"missing scheduler analytics contract snippets: {missing}"

assert "avg_executed" in source
assert "avg_total_us" in source
assert "nevent_analytics_reset" in source
assert "executed" in source
assert "deferred" in source
assert "ne_event_counter" in source
assert "IS_NPC(ch) && GET_MASTER(ch) && IS_AFFECTED5(GET_MASTER(ch), AFF5_ORDERING)" in source

print("scheduler analytics contract passed")
