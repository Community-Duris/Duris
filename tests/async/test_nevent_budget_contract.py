"""Contract for the event-loop budget and deferral path.

Updated when nevent_defer_suffix() was rewritten: the old version only moved the
leading contiguous run of due events to the next pulse, stranding any due event
that sat behind a not-yet-due one for a full ring revolution, and never credited
the revolution to the events the scan did not reach.  The old assertions pinned
those internals (deferred_tail / future_head / the cap-gated promotion), so they
are replaced with the invariants the rewrite has to keep.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / "src" / "new_events.c").read_text()

# Tunables stay environment-overridable.
assert 'DURIS_NEVENT_BUDGET_USEC' in src
assert 'DURIS_NEVENT_MAX_CALLBACKS' in src

# Deferral still exists and still targets the next pulse.
assert 'nevent_defer_suffix' in src
assert 'next_pulse = (pulse + 1) % PULSES_IN_TICK;' in src
assert 'event->element    = next_pulse;' in src
assert 'event->timer      = 1;' in src
assert 'event->deferral_count++;' in src

# The whole suffix is walked: due events move, the rest are credited the
# revolution the scan never gave them.
assert 'for (event = deferred_head; event; event = next)' in src
assert 'if (event->timer > 1)' in src
assert 'event->timer--;' in src
assert 'future_head' not in src

# Moved events are prepended in order, and both bucket ends stay consistent.
assert 'ne_schedule[next_pulse] = moved_head;' in src
assert 'ne_schedule_tail[pulse] = event->prev_sched;' in src
assert 'ne_schedule_tail[next_pulse] = moved_tail;' in src

# Player-event promotion must not be gated on the callback cap: the cap is
# exhausted exactly when player events need the shortcut.
assert 'priority_promotion_used' in src
assert 'priority_promotion_used = TRUE' in src
assert '(max_callbacks <= 0 || executed < max_callbacks) && !priority_promotion_used' not in src
assert src.count('!priority_promotion_used && nevent_promote_overdue_player') == 2

# Instrumentation and clock source.
assert 'NEVENT BUDGET:' in src
assert 'budget_exhausted' in src
assert 'CLOCK_MONOTONIC' in src
assert 'gettimeofday(&loop_' not in src

print("nevent budget contract OK")
