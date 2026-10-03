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

Current formal groups receive a generation retained on their live head node.
The full group identity is `(encounter_boot_id, encounter_process_id, group_key)`;
the key has bit 63 set and a nonzero 63-bit generation counter. Appointment
retains the head's generation, and leader departure transfers it to the next
head before releasing the old node. Both group allocation sites clear reused
pool metadata. Disbanding and creating another group, even with the same leader,
therefore starts a different group lifetime. Generation exhaustion refuses
capture for a new formal group rather than reusing an identity. Ungrouped PCs
use their positive character PID within the observing producer's scope.

Earlier producers used a leader PID for formal groups. An untagged historical
key cannot establish that the participant was solo. Group identity does not
establish an account, controller, coalition, or a persistent team across
producers. Runtime initialization replaces cached metadata from an earlier
producer; intentional copyover closes the old encounters with its explicit
outcome. The group metadata is not serialized into records or copyover files.

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

The accepted hostile-edge adapter observes actual PCs on both sides after
`set_fighting` assigns its opponent. An NPC attacking a PC can therefore start
the attacked player's observation. Combat between actors with player ownership,
including pets, supplies PvP mode; ordinary NPC combat supplies PvE mode. Every
PC participant adapter checks the NPC flag before reading the PC union member.
A pet remains an NPC actor with an owner; its owner is not given a synthetic PC
participation event. An NPC-only edge starts no player encounter.

These observations still use the existing participant/group encounter IDs.
They do not yet join opposing encounters into one shared battle or establish
coalition sides, changing battle roster segments, decisive winners, or a full
zone attempt. Definition 2 publishes bounded existing encounter and contribution
observations; see [OBSERVATION_PROJECTIONS.md](OBSERVATION_PROJECTIONS.md).
The full remaining balance requirements are recorded in
[IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).
The locally qualified pure shared association and its remaining native/storage
integration gate are described in [BATTLES.md](BATTLES.md).
