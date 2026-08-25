"""Contract for the event-loop budget and deferral path.

Updated when nevent_defer_suffix() was rewritten: the old version only moved the
leading contiguous run of due events to the next pulse, stranding any due event
that sat behind a not-yet-due one for a full ring revolution, and never credited
the revolution to the events the scan did not reach.  The old assertions pinned
those internals (deferred_tail / future_head / the cap-gated promotion), so they
are replaced with the invariants the rewrite has to keep.
"""

from pathlib import Path
from contract_text import contains, count

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / "src" / "new_events.c").read_text()

# Tunables stay environment-overridable.
assert contains(src, 'DURIS_NEVENT_BUDGET_USEC')
assert contains(src, 'DURIS_NEVENT_MAX_CALLBACKS')

# Deferral still exists and still targets the next pulse.
assert contains(src, 'nevent_defer_suffix')
assert contains(src, 'next_pulse = (pulse + 1) % PULSES_IN_TICK;')
assert contains(src, 'event->element    = next_pulse;')
assert contains(src, 'event->timer      = 1;')
assert contains(src, 'event->deferral_count++;')

# The whole suffix is walked: due events move, the rest are credited the
# revolution the scan never gave them.
assert contains(src, 'for (event = deferred_head; event; event = next)')
assert contains(src, 'if (event->timer > 1)')
assert contains(src, 'event->timer--;')
assert not contains(src, 'future_head')

# Moved events are prepended in order, and both bucket ends stay consistent.
assert contains(src, 'ne_schedule[next_pulse] = moved_head;')
assert contains(src, 'ne_schedule_tail[pulse] = event->prev_sched;')
assert contains(src, 'ne_schedule_tail[next_pulse] = moved_tail;')

# Player-event promotion must not be gated on the callback cap: the cap is
# exhausted exactly when player events need the shortcut.
assert contains(src, 'priority_promotion_used')
assert contains(src, 'priority_promotion_used = TRUE')
assert not contains(src, '(max_callbacks <= 0 || executed < max_callbacks) && !priority_promotion_used')
assert count(src, '!priority_promotion_used && nevent_promote_overdue_player') == 2

# Instrumentation and clock source.
assert contains(src, 'NEVENT BUDGET:')
assert contains(src, 'budget_exhausted')
assert contains(src, 'CLOCK_MONOTONIC')
assert not contains(src, 'gettimeofday(&loop_')

print("nevent budget contract OK")
