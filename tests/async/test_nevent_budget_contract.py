from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / "src" / "new_events.c").read_text()

assert 'DURIS_NEVENT_BUDGET_USEC' in src
assert 'DURIS_NEVENT_MAX_CALLBACKS' in src
assert 'nevent_defer_suffix' in src
assert 'deferred_head->prev_sched = NULL' in src
assert 'event->element = next_pulse' in src
assert 'event->timer > 1' in src
assert 'event->timer = 1' in src
assert 'future_head' in src
assert 'ne_schedule[next_pulse] = deferred_head' in src
assert 'NEVENT BUDGET:' in src
assert 'budget_exhausted' in src
assert 'priority_promotion_used' in src
assert '(max_callbacks <= 0 || executed < max_callbacks)' in src
assert 'priority_promotion_used = TRUE' in src
assert 'CLOCK_MONOTONIC' in src
assert 'gettimeofday(&loop_' not in src
