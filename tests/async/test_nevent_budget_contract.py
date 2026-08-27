"""Contract for the event-loop budget and deferral path.

The absolute due tick is authoritative. Budget deferral moves only already-due
work to the next physical bucket while preserving its original deadline.
"""

from pathlib import Path
from contract_text import contains, count

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / "src" / "new_events.c").read_text()

# Tunables stay environment-overridable.
assert contains(src, 'DURIS_NEVENT_BUDGET_USEC')
assert contains(src, 'DURIS_NEVENT_MAX_CALLBACKS')

# Deferral still exists and targets the next physical bucket.
assert contains(src, 'nevent_defer_suffix')
assert contains(src, 'next_bucket = nevent_bucket_for_tick(nevent_add_ticks(ne_event_tick, 1));')
assert contains(src, 'event->element = next_bucket;')
assert contains(src, 'event->deferral_count++;')

# The whole suffix is walked: due events move and later revolutions stay put.
assert contains(src, 'for (event = deferred_head; event; event = next)')
assert contains(src, 'if (event->due_tick > ne_event_tick)')
assert not contains(src, 'event->timer')
assert not contains(src, 'future_head')

# Moved events are unlinked and reinserted through the authoritative ordering path.
assert contains(src, 'nevent_unlink_schedule(event);')
assert contains(src, 'nevent_link_schedule(event, static_cast<int>(next_bucket));')

# Due tick, effective priority/aging, and sequence are the single insertion order.
assert contains(src, 'nevent_sorts_before')
assert contains(src, 'left->due_tick < right->due_tick')
assert contains(src, 'left_priority > right_priority')
assert contains(src, 'left->sequence < right->sequence')
assert not contains(src, 'priority_promotion_used')
assert count(src, 'nevent_defer_suffix(next_event, &new_debt)') == 2

# Instrumentation and clock source.
assert contains(src, 'NEVENT BUDGET:')
assert contains(src, 'budget_exhausted')
assert contains(src, 'CLOCK_MONOTONIC')
assert not contains(src, 'gettimeofday(&loop_')

print("nevent budget contract OK")
