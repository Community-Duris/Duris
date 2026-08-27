# Event system (`nevent`)

Reference for the deferred-work scheduler in `src/new_events.c`. This document
covers the mechanism: how events are stored, scheduled, fired, cancelled, and
bounded. For the incident history behind the wheel's load-bearing invariants,
see [ARCHITECTURE.md](ARCHITECTURE.md#event-wheel).

Two files divide the work:

- **`src/new_events.c`** — the scheduler: the wheel, insertion, the per-pulse
  execution loop, budgeting, deferral, cancellation.
- **`src/events.c`** — the callbacks themselves: `event_hit_regen`,
  `event_mana_regen`, `event_move_regen`, `event_wait`, `event_reset_zone`.

## The wheel

`src/new_events.c:116`

```c
P_nevent ne_schedule[PULSES_IN_TICK];       /* 300 buckets */
P_nevent ne_schedule_tail[PULSES_IN_TICK];  /* O(1) append */
```

A hashed timer wheel with one bucket per pulse in a tick. Each bucket holds a
doubly-linked list of events; the tail array makes appending O(1) and
`prev_sched` makes removal O(1).

| Constant | Value | Source | Meaning |
|----------|-------|--------|---------|
| `OPT_USEC` | `250000` | `config.h:82` | 250 ms per pulse, 4 pulses/second |
| `WAIT_SEC` | `4` | `config.h:105` | pulses per second |
| `PULSES_IN_TICK` | `300` | `config.h:107` | buckets; one revolution = 75 s |

Each pulse, `ne_events()` visits bucket `pulse` and **only** that bucket.

## Scheduling: bucket plus revolution counter

`add_event()` (`src/new_events.c:610`) reduces to three lines:

```c
loc            = (delay + pulse) % PULSES_IN_TICK;
event->timer   = (delay / PULSES_IN_TICK) + 1;
event->element = loc;
```

`element` is the position on the ring; `timer` is how many full revolutions to
skip first. The execution loop decrements on every visit and fires at zero:

```c
if (--(current_nevent->timer) > 0)
    continue;    /* not this revolution */
```

A 1000-pulse delay lands in bucket `(1000 + pulse) % 300` with `timer = 4`: it is
passed over three times and fires on the fourth visit. This is how a fixed
300-bucket array represents an unbounded delay range. Insertion is O(1)
regardless of delay — there is no priority queue and no comparison sort.

`add_event()` rejects a negative delay, rejects a dead `ch` (except
`release_mob_mem`), rejects a `victim` with no `ch`, and promotes a zero delay to
one pulse when called after this pulse's `Events()` call (`after_events_call`).

## The event record

`struct nevent_data`, `src/structs.h:2018`. Each event is on **three intrusive
lists simultaneously**:

| Field(s) | List | Answers |
|----------|------|---------|
| `prev_sched`, `next_sched` | wheel bucket | "what fires this pulse?" |
| `next_char_nev` | owning character | "what does this character have pending?" |
| `next_obj_nev` | owning object | "what does this object have pending?" |

The character and object chains are what make targeted cancellation cheap. The
overload set at `src/new_events.c:1366-1400` shows the cost difference:

| Call | Cost |
|------|------|
| `get_scheduled(func)` | O(300 × N) — scans every bucket |
| `get_scheduled(ch, func)` | O(k) — walks `ch->nevents` |
| `get_scheduled(obj, func)` | O(k) — walks `obj->nevents` |

Prefer the two-argument forms. The global scan exists for diagnostics and
should not appear in a per-player or per-pulse path.

Two other fields matter:

- **`data`** is a heap copy. `add_event()` allocates `data_size` bytes and
  `memcpy`s the caller's payload, so passing a stack local is safe.
  `clear_nevent()` frees it.
- **`cld`** is a `char_link_data` created when `ch != victim`, so a destroyed
  victim cannot leave the event holding a dangling pointer.

`scheduled_tick`, `sequence`, `priority`, and `deferral_count` support the
budget, ordering, and telemetry described below.

## Cancellation: neutering, not unlinking

`disarm_char_nevents()` (`src/new_events.c:524`) does **not** remove the event
from its bucket:

```c
e1->func  = NULL;
e1->timer = 1;
```

The event is still visited; `ne_events()` sees a null `func`, skips the call, and
frees the record. The header comment states the contract: *"They fire but do
nothing."*

This is a re-entrancy requirement, not an optimisation. Callbacks routinely
cancel events, including their own and including the one currently being
iterated — `current_nevent` is a global. Unlinking mid-iteration would invalidate
the loop's `next_event` pointer. Neutering avoids the entire class of bug.

The cost is that a cancelled event holds its pool slot until its bucket comes
around again: up to 300 pulses (75 s). "Cancelled" and "freed" are not the same
instant.

Passing `func == NULL` neuters every event on the character; passing a specific
function neuters only events of that type and leaves `e->ch` intact, because the
event is not being pulled from the `next_char_nev` list.

## Memory

Events are pool-allocated: `mm_get(ne_dead_event_pool)` on creation,
`mm_release()` after firing. There is no per-event `malloc`. At Duris's event
rates — regeneration, wait gates, casting and affect timers for every character —
heap churn would fragment badly.

`clear_nevent()` (`src/new_events.c:309`) is the teardown: remove the character
link, free `data`, and unlink from all three lists.

## Per-pulse budget

`src/new_events.c:41-42`

```c
#define NEVENT_BUDGET_USEC_DEFAULT   25000L   /* 25 ms */
#define NEVENT_MAX_CALLBACKS_DEFAULT 4000L
```

25 ms of a 250 ms pulse — the wheel gets 10% of the frame; commands, combat and
sockets get the rest. The wall-clock budget is intended to be the binding limit;
a callback cap low enough to end pulses well inside the time budget starves the
wheel and builds a backlog.

The budget is sampled asymmetrically, because `clock_gettime()` is not free:

- **skip path** (`--timer`, not yet due): checked every 64 scanned events
- **execute path**: checked after every callback

## Deferral

When the budget is exhausted, `nevent_defer_suffix()` (`src/new_events.c:1101`)
moves the entire remaining suffix of the bucket to `pulse + 1` with `timer = 1`,
preserving order and incrementing `deferral_count`.

One detail in that function is load-bearing:

```c
/* Scheduled for a later ring traversal.  The scan never reached it, so
 * decrement here; otherwise it silently loses a whole revolution. */
if (event->timer > 1)
{
    event->timer--;
    continue;
}
```

Events in the deferred suffix with `timer > 1` were never visited, so they never
received their decrement. Without this, every saturated pulse would push them a
full revolution (75 s) into the future — silently, with no error.

## Catch-up debt

Deferring once is harmless. Deferring the same suffix every pulse starves the
tail of a busy bucket permanently. `src/new_events.c:869-921` repays the backlog
on a bounded schedule:

```c
nevent_catchup_quota = (debt + remaining - 1) / remaining;          /* ceil */
extra_callbacks      = MIN(quota, max_extra_callbacks);
extension_us         = MIN(max_extension_usec,
                           extra_callbacks * MAX(1L, nevent_avg_callback_us));
```

- Deferring N events adds N to `nevent_catchup_debt` and opens a repayment window
  of `NEVENT_CATCHUP_WINDOW_PULSES` (4).
- Each pulse in the window repays `ceil(debt / remaining)`, front-loaded so the
  debt converges.
- The budget extension is sized from `nevent_avg_callback_us`, an EWMA of
  measured callback cost (`NEVENT_CALLBACK_EWMA_SHIFT 4`, 1/16 weighting) — not a
  fixed guess.
- Both extensions are capped: +5 ms and +4000 callbacks.
- `nevent_complete_deferred()` decrements the debt only when a previously
  deferred event actually runs.

A load spike therefore produces a brief, bounded, self-correcting delay rather
than a permanent stall.

## Priority and promotion

`nevent_is_player_timed()` (`src/new_events.c:182`) classifies PC regeneration
(`event_hit_regen`, `event_mana_regen`, `event_move_regen`), `event_spellcast`,
`event_memorize` and `event_wait` as player-timed.

`nevent_link_schedule()` (`src/new_events.c:206`) inserts player-timed events at
the **front** of the bucket, after any existing player prefix, and everything
else at the tail. Players perceive regeneration and wait-gate lag; mob
housekeeping is not perceived the same way.

That ordering creates the inverse risk: a permanently busy player prefix would
starve ordinary events. `nevent_promote_overdue_event()`
(`src/new_events.c:258`) is the release valve — on budget exhaustion, one
non-player event is hoisted to run before the suffix is deferred. It is limited
to one promotion per pulse (`priority_promotion_used`), and it is deliberately
**not** gated on the callback cap, since it exists precisely for saturated
pulses. The contract is asserted in the source:

```c
static_assert(NEVENT_MAX_DEFERRALS == 0U,
    "Immediate overdue-event promotion is part of the scheduler contract");
```

## Telemetry

Per-callback cost is tracked in a 128-slot analytics ring keyed by function name
(`nevent_callback_label()` maps hot function pointers to names directly and falls
back to `get_function_name()`, which reads `lib/misc/event_names`).

| Log line | Emitted when |
|----------|--------------|
| `NEVENT BUDGET:` | any event was deferred this pulse |
| `NEVENT CATCHUP:` | a repayment window is open or new debt was added |
| `NEVENT SLOW:` | the loop exceeded 50 ms |

`NEVENT BUDGET` carries `scanned`, `executed`, `deferred`, `catchup_debt`,
`max_late_ticks`, the name of the latest event, and the slowest callback with its
cost — enough to identify a saturating callback without a profiler.

## Configuration

Read by `nevent_config_limit()`. The first two are also listed in
[CONFIGURATION.md](CONFIGURATION.md#diagnostics).

| Variable | Default | Effect |
|----------|---------|--------|
| `DURIS_NEVENT_BUDGET_USEC` | `25000` | Wall-clock budget per pulse. |
| `DURIS_NEVENT_MAX_CALLBACKS` | `4000` | Callback count cap per pulse. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC` | `5000` | Cap on the catch-up budget extension. |
| `DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS` | `4000` | Cap on catch-up extra callbacks. |
| `DURIS_NEVENT_PLAYER_PRIORITY` | `1` | Non-zero enables player-timed front insertion. |
| `DURIS_NEVENT_TRACE_PLAYER` | `0` | Non-zero logs `PLAYER EVENT TIMING` per player event. |
| `DURIS_NEVENT_ANALYTICS` | `0` | Non-zero enables the per-callback analytics window. |

## Invariants

- `ne_events()` is **not re-entrant**. `current_nevent` is a global; a callback
  must never invoke the event loop. Nothing enforces this.
- A callback may cancel any event, including its own, because cancellation
  neuters rather than unlinks.
- `add_event()` may refuse an event. Callers that set state cleared only by the
  scheduled event must handle refusal — see `CharWait()` in `src/events.c` and
  [ARCHITECTURE.md](ARCHITECTURE.md#command-gate).
- Deferral covers the whole unscanned suffix, and every event left behind still
  has its timer accounted for.
- A long-running callback cannot be preempted. The budget bounds how many
  callbacks run, never how long one runs; slicing a sweep across invocations is
  the caller's responsibility (`generic_char_event` in `src/handler.c` is the
  worked example).
