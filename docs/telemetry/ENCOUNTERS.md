# Encounter telemetry

Encounter telemetry measures observed run attempts and participant effort. It is
an observation plane only: it does not award rewards, change combat, activate a
balance, or make a production migration safe to skip.

## Identity and storage

Each run has a stable value identity:

`(encounter_boot_id, encounter_process_id, encounter_seq)`

The lifecycle is stored as tagged facts in `telemetry_interval` with
`record_kind=encounter`. The durable event key is the run identity plus
`encounter_event`, `encounter_revision`, and `encounter_participant_pid`. The
normal telemetry replay key `(boot_id, process_id, record_seq)` remains the
transport/retry identity. The second unique key prevents a retry or a late
reward callback from creating a second logical close event under a new record
sequence.

The source snapshot carries environment, season, config, classifier and policy
versions, zone, and bounded group identity. Participant identity is the stable
subject/PID pair captured at the event boundary; no character pointer, name, or
SQL handle enters the queue.

## Events and metrics

The bounded engine emits:

- `start`: one run start, with no outcome.
- `participant_join`: a participant becomes active, including roster changes.
- `participant_leave`: a participant stops contributing, with the observed
  reason when known.
- `close`: one terminal run outcome and run elapsed time.
- `participant_summary`: one terminal participant contribution.

`close.elapsed_usec` is the run elapsed duration from the monotonic start to the
terminal observation. `participant_summary.participant_usec` is the sum of that
participant's active roster segments. These are intentionally different
denominators: a one-hour run with ten players contributes one run-hour and ten
participant-hours. Roster changes split participant segments without changing
the run clock.

`expected_credit_count` is copied from the committed reward/group boundary and
is not used as a participation count. Reports must keep expected reward credit,
distinct participants, and participant effort as separate measures.

Terminal outcomes include `success`, `failure`, `withdrawal`, `death`, `flee`,
`abandonment`, `timeout`, `copyover`, `shutdown`, and `unknown_close`. All
observed terminal outcomes remain in the attempted-run denominator. A start
without a close is an `unclosed_tail`; its end time and duration are unknown,
never inferred from process shutdown or the next run.

## Bounded runtime behavior

The engine retains at most 128 active runs, 64 participants per run, and 256
events per run. A 64-entry terminal identity cache classifies a repeated close
as idempotent or a conflicting close as a duplicate conflict after the active
slot is released. Queue rejection and bounded overflow set
`TELEMETRY_QUALITY_QUEUE_DROP`; gameplay never waits for SQL and never fails
because telemetry cannot be admitted.

Encounter state is reset at runtime initialization. An intentional copyover or
shutdown closes the observed active tail with that explicit outcome. A crash or
hard process loss does not run that path, so the durable start/roster tail is
reported as unclosed rather than receiving synthetic elapsed or participant
time.

## Gameplay observation seams

The first adapter observes combat start and death, group join/leave and roster
synchronization, movement/zone changes, flee and retreat, committed epic zone
success, and lifecycle copyover/shutdown. All adapters are best effort and
value-only. No telemetry call is placed before the gameplay operation it
describes has committed.

The encounter facts are ready for the aggregate/report owners (#268/#269). The
report definition in `scripts/telemetry/encounter_definitions.py` specifies the
denominators and unresolved-tail policy without reading the raw fact stream;
the aggregate implementation remains a separate, bounded work item.
