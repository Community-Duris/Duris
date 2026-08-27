#!/usr/bin/env python3
"""Source contracts for the current nevent reference and retired legacy API."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8", errors="replace")


events_header = read("src/events.h")
events_callbacks = read("src/events.c")
scheduler = read("src/new_events.c")
prototypes = read("src/prototypes.h")
structs = read("src/structs.h")
reference = read("docs/reference/EVENTS.md")
architecture = read("docs/reference/ARCHITECTURE.md")
configuration = read("docs/operations/CONFIGURATION.md")
all_source = "\n".join(
    path.read_text(encoding="utf-8", errors="replace")
    for path in SRC.rglob("*")
    if path.suffix in {".c", ".h"}
)

# events.h exposes only current callback/traversal helpers, not the removed
# numeric scheduler taxonomy.
assert "enum class regen_resource : uint8_t" in events_header
for member in ("hit", "vitality", "mana", "ward"):
    assert re.search(rf"\b{member}\b", events_header)
assert "LOOP_EVENTS_CH" in events_header
assert "LOOP_EVENTS_OBJ" in events_header
for retired in (
    "#define EVENT_",
    "#define AddEvent",
    "#define FIND_EVENT_TYPE",
    "#define T_PULSES",
    "LAST_EVENT",
):
    assert retired not in events_header

# The old forwarding functions, scheduler tables, and unused factory must not
# leave a second lifecycle API beside nevent_cancel/disarm_*.
for retired in (
    "ClearCharEvents",
    "ClearObjEvents",
    "clear_char_nevents",
    "clear_events_type",
    "EVENT_HIT_REGEN",
    "EVENT_MOVE_REGEN",
    "EVENT_MANA_REGEN",
    "EVENT_WARD_REGEN",
):
    assert re.search(rf"\b{retired}\b", all_source) is None

assert "void StartRegen(P_char, regen_resource);" in prototypes
assert "EventsFactory" not in structs
assert re.search(r"\bevent_loading\b", events_callbacks) is None
assert re.search(r"\bevent_counter\b", events_callbacks) is None
assert re.search(r"\bdead_event_pool\b", events_callbacks) is None
assert "const char *event_names[]" not in events_callbacks
assert "extern const char *event_names[]" not in all_source
assert "File: new_events.c" in scheduler

# The reference describes the scheduler that exists today and rejects the
# obsolete relative-timer/cancel-later model.
for current_contract in (
    "absolute `due_tick`",
    "75 seconds",
    "nevent_schedule_result",
    "nevent_cancel(handle)",
    "nevent_periodic_continue_after()",
    "game thread",
    "dynamic callback map",
    "observation-only",
):
    assert current_contract in reference

for stale_contract in (
    "bucket plus revolution counter",
    "Cancellation: neutering",
    "128-slot analytics",
    "timer value of all events",
):
    assert stale_contract not in reference

assert "timer decremented" not in architecture
assert "Player-event promotion" not in architecture
for setting in (
    "DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC",
    "DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS",
    "DURIS_NEVENT_PLAYER_PRIORITY",
    "DURIS_NEVENT_TRACE_PLAYER",
    "DURIS_NEVENT_ANALYTICS",
):
    assert setting in configuration

print("nevent reference and legacy cleanup contracts passed")
