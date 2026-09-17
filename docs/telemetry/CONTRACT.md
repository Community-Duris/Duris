# Telemetry contract — version 1 (#260)

Status: implementation candidate; **not frozen or integration-ready until #260
merges**. This document and the public headers must be reviewed together with the
golden fixtures. Declarations and specification tests do not implement a writer,
classifier, SQL store or gameplay instrumentation.

`STORAGE_DESIGN.md` preserves the complete published design. Its workload numbers,
SQL examples and table shapes are proposals, not measurements. The precise v1
rules below resolve its open representation choices. Budget proposals require the
separate #272 load/teardown gate before production approval. No production
telemetry, retention, account linkage or automatic balance changes are authorized.

## Ownership and build boundaries

- #260 owns the shared headers and fixture format. Consumers request amendments
  here rather than fork record types or change enum meanings.
- #261 owns SQL schema, private connection, idempotent apply and immutable migration
  registration. Allocate migration IDs from current master, not this document.
- #262 owns the preallocated queue, sequential writer, retry and stop protocol.
- #263 owns effective config identity and typed classifier policy snapshots.
- #264 owns logical session/connection state and cumulative counters.
- #266 owns pure activity evidence/classification and contextual interval splitting.
- #265 alone owns gameplay/lifecycle hooks and shared build registration after its
  predecessors merge. #267 owns the additive progression fact extension and the
  telemetry-only XP/level hooks in `world/limits.c`; see `PROGRESSION.md`. #268
  owns external rollups; #269 owns report presentation.
- No new header includes game character, SQL, socket, mutex or filesystem types.
  Standalone compilation must work both with and without `__NO_MYSQL__`.
- Flat-file authority explicitly reports disabled/unsupported telemetry. There is
  no file sink, SQL fallback, dependency on telemetry for login/save, or fabricated
  healthy zero. An independent SQL endpoint would require a separate reviewed
  connection-role/configuration change.

## Units, identity and time

All v1 durations and monotonic endpoints are unsigned **microseconds**; UTC
occurrence labels are signed Unix-epoch microseconds. This deliberately chooses
microseconds rather than the storage design's illustrative millisecond columns.
SQL uses corresponding unsigned/signed BIGINT fields. Never infer units from a
column name that omits the suffix. `TELEMETRY_UTC_UNKNOWN` is the sole unknown UTC
sentinel; zero UTC is a real epoch value. Ingestion time is assigned at commit,
never substituted for occurrence time.

A producer is `(boot_id, process_id)`, both nonzero incarnation tokens. `process_id`
is not an OS PID. Provision boot identity once at startup with collision checking;
assign process incarnation independently. Restored backups/test worlds require a
new environment identity. No per-event randomness/hashing is needed.

- Replay key: `(producer.boot_id, producer.process_id, record_seq)`; sequence starts
  at one, increases for each sealed attempted record, and is never reused after
  admission rejection. A retried immutable record retains its key and fields.
- Session key: original producer identity plus `session_seq`. It denotes one
  resident character session, not one accepted socket.
- Connection key: current producer identity plus `connection_seq`. Reconnect makes
  a new connection key, not a new session. All-zero means detached only where the
  payload permits it; partially zero identities are invalid.
- Subject, PID, environment and season are scoped at entry. PID must be positive;
  subject/season/environment identifiers must be nonzero. Subject is a restricted
  opaque character key, not a name, IP or raw account identifier. Environment and
  season distinguish reused player IDs and restored worlds. This release does
  not claim historical account linkage or distinct-account reports.
- Sequence/revision exhaustion disables further observation for that incarnation;
  never wrap or silently reuse identities. Saturating diagnostic counters expose
  overflow; durations that cannot be represented are invalid, not wrapped facts.

Intervals are half-open `[start,end)` in one producer's monotonic clock. Positive
interval duration equals `end-start`. UTC clock changes never create negative or
extra elapsed time. Seal the current interval at an observed clock jump, mark
clock-discontinuity quality, and start a fresh reporting anchor. UTC endpoints may
move backwards; duration remains monotonic. Clock-ambiguous intervals are excluded
from exact UTC-day attribution, not forced into an invented day.

For a normal UTC midnight, split using the current stable monotonic-to-UTC anchor
and conserve duration exactly. This is deterministic mapping, not proportional
splitting across an unobserved clock jump. Unknown UTC keeps session duration but
cannot contribute to a known daily reporting bucket.

## Resident sessions, connections and classification

Enter occurs on actual gameplay entry. Link loss detaches the connection while
the live character remains resident. A reconnect attaches a new connection to
the same session. Actual quit/rent/camp/idle extraction ends the logical session;
a mere socket loss does not. End reason `disconnect` means eventual resident
unload after connection loss, not the detach event itself.

Successful copyover hands over the original session key, last checkpoint revision,
absolute counters and quality. The new process has a new producer and connection
key, and establishes a new monotonic anchor. No interval subtracts timestamps
from different process incarnations. Do not invent time spent outside observed
gameplay during handoff. Account through the old process cut, then use
`telemetry_runtime_session_handoff_copy` to export a bounded handoff. Its previous
producer is the current exporting process, not necessarily the original session
producer after multiple copyovers. The last allocated checkpoint revision includes
rejected admissions; zero means none has been sealed. Import with
`telemetry_runtime_session_resume`, preserving scope/counters/quality and seeding
the next revision after that value. It emits one `connection_attached`, not a new
`session_entered`; do not also invoke a separate attach. A bare connection transition
cannot restore absent runtime state. Failed/absent handoff leaves an explicitly unclosed old
session; the replacement creates a new one. Shutdown and crash behavior must not
require telemetry durability for gameplay teardown.

The exclusive cumulative accounting identities are:

```
connected = active + idle + unknown
resident = connected + linkdead
```

`unknown` here is connected time with unavailable activity evidence. It is not
unloaded time, missing context, queue backlog or an extra bucket added on top of
active time. Resident-linkdead intervals have no live connection key and contribute
only to linkdead/resident. Connected intervals belong to exactly one active, idle
or unknown category. Checkpoints carry absolute cumulative counters and strictly
increasing per-session revisions. They are never additive on replay.

Classifier v1 proposal: a connected player is active for the configured
`active_window_usec` after a recognized player-origin action or observed combat
participation. Default proposal is 300 seconds, bounded by one hour. Count accepted
movement, meaningful interaction/cast/use, communication and combat participation;
do not count empty/unknown input, protocol keepalives, queued-but-unexecuted input,
autonomous pet/room events or staff force as player evidence. Recognized actions
that fail downstream can still demonstrate input, but are not proof of productive
play. This is an observational heuristic, not a bot detector or reward rule.
The #266 module receives typed evidence from #265 and cannot reparse raw commands.
Evidence unavailable on attach begins unknown; actual observed evidence establishes
the activity window. After its expiry while connected, classify idle. Split exactly
at the activity deadline rather than marking the entire next pulse active.

Context uses a single descriptive category with documented precedence: combat,
travel, crafting, social, administration, other, none; unavailable is unknown.
Capture dimensions at observation time: level band, primary class identifier,
race, faction/racewar, zone, group size, config and classifier/policy versions.
Level bands are fixed five-level bands starting at level one; class/race/faction
IDs use explicit versioned mappings, not enum ordinals inferred at query time.
Secondary class and specialization analyses require a versioned extension rather
than silently overloading `class_id`. Group size zero means unknown, one solo.
Unknown level/class/race/faction is zero; unknown zone is -1, not a guessed room.

Meaningful level-band/class/race/faction/zone/group/config changes seal the current
interval. Same-zone room movement does not force a record. Per-player context
segmentation is capped per fixed monotonic minute from the process anchor. The
candidate default is eight segments, **including one reserved overflow segment**.
When another split would exceed the cap, aggregate the remainder as
`overflow_unknown` context with unknown attribution dimensions. Preserve known
activity/resident counters. Context overflow is not evidence that the player's
activity itself was unknown. Actual accounting can continue in bounded counters
without emitting every transition. Reset the cap on the next monotonic window,
not on each teleport, reconnect or property change.

## Record grains, validation and serialization

Public `telemetry_record` is a tagged trivially-copyable value bounded by 512 bytes;
that is an in-process queue limit, **not a native-layout wire format**. Zero-init
values, set the active union member and reserved fields explicitly. SQL binding
and any canonical fixture encoding serialize named fields, never struct padding,
uninitialized union storage or host endianness. Compare canonical typed content
for replay identity; bytewise `memcmp` of the native record is not the contract.
Unknown enum values/schema versions or nonzero reserved bits are rejected.
`schema_version` is the sole record compatibility version; there is no separate
unserialized record-version constant.

- Interval: immutable one-category elapsed segment, dimensions and quality.
- Lifecycle: logical enter/exit or explicit connection attach/detach, no duration.
- Checkpoint: absolute resident/connection counters plus revision and observation.
- Coverage gap: missing inclusive record-sequence bounds, known duration if any,
  dropped-record count and reason. Bounds zero/zero mean unknown, not empty proof.
- Configuration: immutable bounded effective snapshot admitted before any new
  record references its identity; uses the same finite control reserve.
- Progression: one observed XP storage fact or one level transition with bounded
  source/reason/modifier fields, mutable before/after snapshots, and explicit
  observation status. Level-threshold values are not reward/loss totals.

An all-zero session reference is allowed only for process-wide gap/config records.
Every other payload must identify its subject and scope. Partial identities are
invalid. Generic constexpr validation covers representation invariants; semantic
repository checks also validate scoped session immutability, cumulative monotonicity
and config content. Header validation alone is not full-record validation.
`telemetry_record_is_valid` and `telemetry_config_validate` share the same complete
configuration representation checks, including disabled bounds. The nonzero digest
predicate checks presence/shape only: #263 computes SHA-256 once on publication;
#261 recomputes and compares it on worker-side materialization before durable
acceptance. Queue admission never claims cryptographic verification and must not
hash each gameplay observation. Numeric dimensions are bounded by their field
widths with zone >= -1; mapping existence is a versioned semantic check.

## Configuration identity and publication

Config ID is immutable within the environment namespace. Revision orders local
publication; it is not a replacement for content identity. Store the fixed 32-byte
fingerprint of the versioned canonical typed effective config and build/content/
property/classifier/policy identity. Hash once on effective change, not per event.
Canonical encoding concatenates the following fields in exactly this order, all
unsigned big-endian integers of the specified bit width, with no delimiters:

1. `schema_version` (16).
2. `build_version`, `content_version`, `property_version`, `classifier_version`,
   `policy_version` (32 each, in that order).
3. `season_id`, `environment_id` (64 each).
4. `interval_usec`, `checkpoint_interval_usec`, `active_window_usec` (64 each).
5. `context_segments_per_minute` (32), `pulse_slot_count` (16), `backend` (8),
   `enabled` (8).

Exclude config ID, revision, `effective_utc_usec` (publication metadata), the
fingerprint itself and reserved/native padding. The digest is SHA-256 over these
bytes, stored as 32 raw bytes (fixtures render lowercase hexadecimal). Identical
effective values keep the same digest across publication times. No credentials,
hostnames, raw properties, paths or arbitrary JSON cross the capture boundary.
The property-version registry owned by #263 maps to a reviewed, allowlisted effective
property snapshot; never claim a process-local counter reconstructs historical config.

The game-side config owner copies/publishes state without SQL. It must admit a
configuration control record before attributing new intervals to that config.
Failure to admit degrades coverage; retry the same config identity. Retain bounded
cumulative counters but suppress dependent interval/checkpoint emissions until it
is admitted; retain gap diagnostics for later publication. Do not invent config ID
zero or attribute the missing segment to a stale config. Its historical context
remains unknown even when later checkpoints recover duration. The worker
materializes the config row before dependent facts. A missing config is explicitly
incomplete, never resolved by joining current live properties.

## Transport, thread and failure contract

There is one game-thread producer, one preallocated bounded RAM queue and one
sequential writer. Runtime capture/enqueue/pulse does bounded in-memory work only:
no SQL, file writes, fsync, pool acquisition, blocking locks or unbounded allocation.
The transport's writer pulse, repository apply/config and drain operations are
**worker-only**. A similarly named runtime pulse does not license calling writer
work from the game loop. Health is a synchronized cached value copy; readers never
walk worker-owned connection state.

Candidate limits are 8,192 records total, 128 control-reserved slots, 128 records /
64 KiB per batch and a two-second oldest-record trigger. The reservation is within
total capacity, not extra capacity. Detail stops before consuming it; lifecycle,
checkpoint, configuration and gap records may use it. Control can still be dropped.
Preserve producer FIFO order (and therefore within-class order) and never overwrite
worker-owned records. A first-seen replay key advances that producer's sequence;
retries may revisit the same key unchanged. Repository tests may deliberately
submit a conflicting payload to verify rejection. These are versioned
upper-bound proposals, not outage-duration guarantees.

Admission success means retained in RAM, not durable. Drop new detail/control on
saturation; retain bounded gap counters/ranges for subsequent reporting. Where
noncontiguous losses cannot fit bounded metadata, aggregate a conservative gap
with incomplete coverage rather than claim every sequence in a broad span was lost.
Process death before gap publication leaves an unknown tail. No durable spool.

The writer owns one verified private connection, never the game handle or gameplay
pool. One unresolved transaction at a time. Roll back or reconcile an ambiguous
commit before issuing a later batch. Retrying keeps keys and payloads unchanged.
Configuration materialization is internal to `telemetry_repository_apply`: the
config row and keyed configuration fact commit atomically in that batch. There is
no public keyless `apply_config` entry point. Apply result semantics:

- identical duplicate: no-op; differing content at the same key: conflict.
- older checkpoint: retain the immutable observation but do not regress projection.
- newer checkpoint: require nondecreasing counters and immutable session scope.
- same session revision with different totals: conflict even under another record key.
- retryable/ambiguous batch: retain unchanged, with bounded backoff/catch-up rate.
- invalid/conflicting rows: bounded isolation and diagnostics; do not blanket-ignore
  constraints or permanently block every valid row behind one malformed record.

`request_stop` is nonblocking. #265 owns the thread handle in a process-lifetime
coordinator: initialize the queue, start one worker (repository init and all SQL
run there), request stop, let that worker drain within the approved connector
policy and publish stopped, then join OFF the game thread. Transport init does not
spawn a hidden thread. The coordinator retains state through join and then calls
repository/transport/runtime shutdown. Cached stopped health is not a substitute
for join. No additional join API is needed because the coordinator owns the handle.
Shutdown/destruction is legal only after the worker
has exited and ownership has been joined safely. A SQL timeout does not prove
bounded connector cancellation. #262/#272 must demonstrate teardown or report a
blocker; never detach a thread touching freed state or run an unbounded game-thread
join. Telemetry must not veto valid game shutdown because observations were lost.

## Six-table logical schema and first reports

#261 finalizes physical SQL and indexes within these frozen logical grains:

1. `telemetry_session`: projection keyed by environment/season plus stable session
   key; immutable subject scope, latest revision/counters, lifecycle/completeness.
2. `telemetry_interval`: append-only **tagged fact stream**, not only duration rows.
   Store all admitted record kinds with an increasing ingest ID and unique scoped
   replay key. This resolves dedupe for lifecycle/gap/config/checkpoint records
   without a seventh dedupe table. Non-interval records contribute no interval
   duration. Use typed nullable columns by kind, not arbitrary payload JSON.
3. `telemetry_config`: immutable scoped config identity/fingerprint and typed fields.
4. `telemetry_player_day`: rollup-owned generation/definition/environment/season/
   UTC-day/subject/session contribution, duration buckets and attributable coverage.
5. `telemetry_cohort_day`: rollup-owned generation/definition/environment/season/
   day/level-band/primary-class/race/faction/zone/config/category sums and counts.
6. `telemetry_rollup_state`: definition/generation committed input watermark,
   publication status and coverage/rebuild metadata.

This explicitly refines the storage design's conceptual interval-only table: the
fact stream's record kind is mandatory. No gameplay foreign keys, new gameplay-row
counters, triggers, DB scheduler, partitioning or uncontrolled retention jobs.
The logical version and typed field mapping are shared fixtures, not SQL migrations.

First reports: (a) per-character resident versus connected active/idle/unknown and
linkdead totals, marked incomplete when necessary; (b) observed connected activity
and attribution coverage by the bounded cohort dimensions above. Report session
projection totals separately from sealed-interval coverage. A later checkpoint can
recover total duration after a drop but cannot reconstruct its lost zone history.
Do not sum every checkpoint or combine checkpoint totals with interval totals.

External rollup consumes keyset pages from a stable committed prefix produced by
the sole sequential writer; cursor and aggregate changes commit atomically. After
ambiguous rollup commit, reread the cursor before applying sums again. Versions and
rebuild generations are explicit; publish a new generation atomically, never
truncate tables currently served. No OFFSET scan or raw history query in gameplay.

Reports include input watermark, occurrence coverage, quality flags and provisional
status. No median from sums, average-of-ratios XP/hour, additive multi-day distinct
accounts, or claimed complete crash tail. XP/reward metrics are later extensions
with authoritative ledger reconciliation, not inferred from observational time.
Retention, deletion, archive, backup/restore and subject processing must be listed
by #261's lifecycle owner before operational activation; no purge is authorized.

## Versioning, verification and remaining work

Enums are append-only within a compatible version; changing meaning, units,
identity, grain or fingerprint encoding requires a new schema version and
new golden fixtures. H/K/P extensions get explicit bounded record kinds, not a
raw callback, string map or opaque payload escape hatch. Unknown versions fail
closed for telemetry only and never block ordinary gameplay persistence.

`test_telemetry_contract_headers.py` checks independent headers/layout, validators
and signatures without SQL/game runtime. Golden fixtures and their validator must
cover normal intervals, replay conflict, ambiguous commit, stale/new checkpoints,
dropped detail versus recovered totals, level/zone/config changes, midnight and
clock jumps, link loss/reconnect, copyover and unclosed crash tail. These are a
specification oracle, **not evidence that downstream implementations already pass**.

Run the focused checks from the repository root:

```sh
python3 tests/async/test_telemetry_contract_headers.py
python3 tests/async/test_telemetry_contract_fixtures.py
```

Shared fixtures live in `tests/async/fixtures/telemetry/contract/`. Downstream
implementations must consume the same cases through their actual boundaries; the
reference oracle is not a substitute for queue, repository or classifier tests.
The public declarations are intentionally not added to `src/Makefile` here, and
no gameplay journey is applicable until #265 installs actual runtime hooks.

#260 is not complete until header/document/fixture agreement is checked. The
standalone schema declaration does not justify enabling SQL telemetry or changing
ready/blocked labels before the contract actually merges.
