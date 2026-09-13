# Activity and context accounting (#266)

This module is the pure, bounded B-classifier boundary. It accepts typed evidence
and value snapshots; it does not inspect commands, characters, rooms, SQL, files,
queues, or gameplay state. #265 owns the hooks that translate game observations
into these values.

## Ownership and bounds

`telemetry_activity_state` and all service contexts are caller-owned. The state has
fixed slot and configuration arrays (`256` slots and `64` admitted configurations).
Every callback is synchronous and supplied by the caller:

- `telemetry_activity_clock` supplies a monotonic/UTC pair for explicit flush and
  retirement operations.
- `telemetry_activity_sink` receives copied `telemetry_record` values.
- `telemetry_activity_record_key_allocator` is the alias of the #264 allocator;
  activity never keeps a private record sequence.

Capture paths do not allocate. A sealing operation accounts at most
`TELEMETRY_ACTIVITY_MAX_PIECES_PER_OPERATION` pieces: at most seven ordinary
intervals plus one bounded late remainder. The remainder is **not** emitted as
an interval spanning uncrossed boundaries. Its exact active/idle/unknown/linkdead
counters are calculated directly (including an active deadline inside the tail),
and its missing detail is represented by a duration-known, UTC-unknown coverage
gap marked late/incomplete. Gap admissions add bounded control records; they are
not activity intervals. Numeric overflow is checked for the entire cut before
any frontier, counter, or detail admission advances.

## Classifier v1

A newly attached session starts `unknown`. A recognized player-origin observation
starts or extends the configured activity window (300 seconds by default, bounded
to one hour):

| Typed evidence | Activity effect |
| --- | --- |
| `player_action`, movement, interaction, communication, combat participation | active window |
| `reading`, social observation | active window when explicitly observed |
| `automatic_combat`, `following`, `recovery`, keepalive, autonomous event | no automatic active credit |
| `spam` | no new active credit |
| `afk` | force connected idle |
| `linkdead` | resident linkdead only, with zero connection identity |
| unknown/invalid input | rejected or remains unknown |

This is an observation heuristic, not a reward, AFK policy, bot detector, or
proof that a command executed successfully. Activity duration is exclusive:

```
connected = active + idle + unknown
resident  = connected + linkdead
```

The state seals at the activity deadline, every configured interval, and each
meaningful context/dimension boundary. It never extends active time across an
unobserved gap.

## Context and boundaries

Context precedence is `combat`, `travel`, `crafting`, `social`,
`administration`, `other`, `none`, then `unknown`. Context snapshots carry
level band, class, race, faction, zone, group size, config ID, and classifier/
policy versions. Same-zone room movement cannot create a row because room IDs
are not part of this value-only boundary.

Context changes consume a per-player segment budget for the fixed monotonic minute
selected from the process anchor. The default cap is eight including the reserved
overflow segment. The cap is not reset by teleport, reconnect, or configuration
change. The final available interval is reserved for `overflow_unknown` context
and unknown attribution dimensions. Subsequent context-only changes coalesce
without forcing another interval. No interval is admitted beyond the hard budget,
even when category/configuration boundaries or explicit flushes require additional
accounting cuts: those cuts preserve exact counters and accumulate explicit gap
metadata instead. Overflow never relabels known activity as unknown activity.
A mid-minute cap increase takes effect next minute; a decrease may tighten the
remaining budget immediately. The next monotonic minute starts a fresh budget.

UTC is only a label. Monotonic time defines duration. A stable UTC mapping splits
at midnight while ordinary detail remains within the operation budget. A late
remainder that cannot be split within the work bound becomes an explicit gap,
not one cross-day interval. A backward or materially skewed UTC sample marks the affected
interval ambiguous and emits unknown UTC endpoints; it never invents a date.
Unknown UTC samples break the mapping until a fresh known pair is observed.

## Configuration, drops, and recovery

A configuration snapshot must be admitted before detail can reference it. Its
environment, season, classifier version, and policy version must match the session
snapshot; a matching numeric config ID alone is insufficient. Until
then, counters continue as degraded/unknown and detail is suppressed. The pending
`telemetry_disabled` gap is retained until it can be emitted; no old config is
used to relabel the suppressed period. Disabled snapshots may legally omit unused
budgets: zero segment caps retain the module default across minute rollovers.
Suppressed intervals do not consume the detail-row budget, and disabled catch-up
tails retain `telemetry_disabled` rather than being mislabeled as context overflow
or sequence loss. The rollover/churn/re-enable regression checks both retained
metadata and emitted gap records.

A rejected detail/control sink admission or exhausted shared key allocator leaves
cumulative deltas available to the caller and retains bounded gap metadata. The
next successful admission emits a `coverage_gap` with known duration and sequence
bounds when both remain provable. Non-contiguous losses degrade the bounds rather
than claiming a false exact range. No raw command text crosses the API.

## Counter delivery to #264

Every returned `has_delta` result must be applied synchronously to session state,
even when the detail sink rejected the record. Apply activity deltas before the
corresponding detach/attach/exit/checkpoint transition so #264 does not fill that
time as unknown first. Detail admission and exact counter delivery are separate.

Pulse callers must supply a non-null `deltas` array with space for **every resident
slot in the selected pulse cohort**, including slots that may not be due. An
undersized buffer is rejected before any slot consumes elapsed time; retrying
with sufficient capacity produces the original contiguous deltas. Callers own
and consume those returned deltas; the module does not durably enqueue them.
If an individual session cannot represent a cut numerically, pulse reports
`invalid` and leaves that session's frontier unchanged; deltas from other processed
sessions still require consumption.

## Verification

`tests/async/test_telemetry_activity_state.py` compiles and runs the deterministic
state harness in both SQL and `__NO_MYSQL__` modes. It also generates a C++ B
golden harness from the existing `normal_interval.json` and
`detach_reconnect.json` facts; that harness compares actual interval records,
contexts, dimensions, timestamps, and cumulative identities rather than only
checking Python expectations. The same command links the real activity and session
modules into `telemetry_activity_session_integration_harness.cc` in both compile
modes. It verifies exact counter transfer/checkpoints, rejected detail followed by
gap recovery, detach/linkdead/reconnect, unavailable configuration, and two-session
pulse-buffer rejection/retry at the same due timestamp. All checks are required;
there is no pending-fix or skip-success path.

Run the focused test with:

```sh
python3 tests/async/test_telemetry_activity_state.py
```

The repository host's full `make -C src` remains a separate integration check and
requires the project's MySQL build environment; this pure module is intentionally
not registered in the build manifest by #266.
