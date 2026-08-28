# Redis System Audit

Date: 2026-08-28
Branch: `redis-refactor`
Audit baseline commit: `68a916ec`
Status: Implementation in progress; RDS-001, RDS-002, RDS-003, RDS-004, RDS-005, RDS-006, RDS-007,
RDS-009, RDS-010, RDS-011, RDS-012, RDS-013, RDS-014, RDS-019, RDS-020, RDS-022,
RDS-023, RDS-024, RDS-027, RDS-028, RDS-016, RDS-017, RDS-008, and RDS-021 are
remediated, as are RDS-018, RDS-025, and RDS-015; RDS-026 remains open.

## Implementation progress

### 2026-08-28 - RDS-026 floor-delta runtime boundary

Completed in this interval:

- Moved fixed-capacity floor-drop staging, object encoding, replacement/removal
  coalescing, materialization suppression, and bounded worker submission from `redis.c`
  into `redis_floor_runtime.c/.h`.
- The typed runtime now owns its enabled, quiesced, and materializing state. Boot,
  recovery quiescence, pwipe, and shutdown update that state explicitly before worker
  cancellation or draining.
- Precomputed the season-scoped floor hash and index keys during boot configuration. The
  periodic flush no longer formats or resolves either key on the game thread.
- Routed object movement, extraction, checkpoint flushing, and recovery materialization
  through the typed header. Four more translation units no longer include `redis.h`,
  reducing direct umbrella consumers from 13 to 9.
- Reduced `redis.c` from 1,802 to 1,685 lines and `redis.h` from 50 to 46 lines. Extended
  the module-boundary gate to keep floor staging, encoding, lifecycle state, and callers
  out of the composition root.

Performance effect:

- Drop/remove capture retains the same constant-time state checks and fixed 1,024-entry
  in-memory arrays. It performs no Redis, SQL, filesystem, process, sleep, or wait work.
- Flush retains the same bounded local vector construction and one background-worker
  enqueue. Reusing boot-computed keys removes two namespace/key formatting operations
  from each non-empty periodic flush.

Validation:

- `./scripts/format.sh --check` and `make -C src -j2`: passed under the warning-as-error
  profile.
- Floor/world gating, failure containment, pwipe invalidation, module ownership, and the
  compiled recovery planner/materializer checks passed.
- All 24 `tests/async/test_redis*.py` regressions passed, including the isolated live
  floor-worker barrier, fault recovery, ACL, maintenance, and world-store suites.

RDS-026 remains partially remediated. Gameplay floor-delta staging is now separate from
the root; world snapshot orchestration, direct administrative cleanup, legacy ship
cleanup, and broad lifecycle/configuration policy remain.

### 2026-08-28 - RDS-026 report-cache facade boundary

Completed in this interval:

- Moved epoch-scoped content-cache keys, key resolution, report generation, local reads,
  asynchronous writes and invalidations, artifact warm-up, and cache-worker lifecycle from
  `redis.c` into `redis_report_cache.c/.h`.
- Replaced the public generic cache set/get/delete surface with report-specific named,
  fraglist, epic-zone, and artifact operations. Administrative named-cache invalidation now
  uses an explicit operation instead of accepting an arbitrary cache key.
- Routed seven report-only translation units through the typed header and removed their
  `redis.h` dependency. Artifact rendering now reads the report-cache facade state rather
  than the broad mutable Redis global.
- Reduced `redis.c` from 2,281 to 1,802 lines and `redis.h` from 76 to 50 lines. Direct
  `redis.h` consumers fell from 20 to 13, and the module-boundary gate now rejects report
  generation, report-cache storage, or report-only callers returning to the composition
  root.

Performance effect:

- Gameplay-facing cache reads remain an enabled check plus the existing bounded local
  cache lookup or transform. Cache publication and invalidation still perform one bounded
  enqueue to the existing background worker; no synchronous Redis, SQL, filesystem,
  process, sleep, or wait work was added to those paths.
- The only direct Redis read in the facade is the existing six-key artifact warm-up. It
  remains bounded by the configured connection and command deadlines and runs only during
  boot after the cache worker starts.

Validation:

- `./scripts/format.sh --check` and `make -C src -j2`: passed under the warning-as-error
  profile.
- Report/artifact codecs, season scoping, subsystem identity, pwipe invalidation,
  administrator nonblocking, failure containment, command observability, transactional
  cutover, and module-boundary regressions passed.
- All 24 `tests/async/test_redis*.py` regressions passed, including every isolated live
  Redis transport, ACL, cache, floor, presence, donation, maintenance, and world-store
  suite.

RDS-026 remains partially remediated. Player checkpoints, donation delivery, presence
delivery, and report caches are separate typed modules; world/floor orchestration and
administrative lifecycle policy remain in the Redis composition root.

### 2026-08-28 - RDS-026 presence runtime boundary

Completed in this interval:

- Moved DurisWeb visibility policy, public/private field selection, payload encoding,
  online/offline/clear submission, and presence enabled state from `redis.c` into
  `redis_presence_runtime.c/.h`, beside the existing payload and healing-worker modules.
- Boot now enables the facade only after presence-worker initialization succeeds. Pwipe
  and shutdown disable submissions before canceling or draining the worker, so callers no
  longer infer availability from the broad Redis global.
- Routed login/logout and connection cleanup through the typed presence header. `nanny.c`
  and `actoth.c` no longer include `redis.h`; direct umbrella consumers fell from 22 to 20.
- Reduced `redis.c` from 2,337 to 2,281 lines and `redis.h` from 81 to 76 lines. Extended
  the module-boundary gate to keep payload/policy/worker calls and presence declarations in
  their owning module.

Performance effect:

- Online/offline behavior is unchanged: one constant-time enabled check, local payload
  encoding, and one bounded worker enqueue. There is no synchronous Redis, SQL,
  filesystem, logging, sleep, or wait work on the gameplay path.
- The new enable/disable calls occur only at boot, pwipe, or shutdown. Failed worker
  initialization now avoids even the previous rejected submit attempts.

Validation:

- `./scripts/format.sh --check` and `make -C src -j2`: passed under the warning-as-error
  profile.
- Presence privacy/escaping, live outage healing and queue bounds, DurisWeb integration
  security, Redis failure containment, pwipe invalidation, and module-boundary regressions
  passed.
- All 24 `tests/async/test_redis*.py` regressions passed, including every isolated live
  Redis suite.

RDS-026 remains partially remediated. Player checkpoints, donation delivery, and presence
delivery are separate typed modules; world/floor orchestration, report-cache facades, and
administrative lifecycle policy remain in the Redis composition root.

### 2026-08-28 - RDS-026 donation runtime boundary

Completed in this interval:

- Moved donation enabled state, bounded queue polling, recipient filtering, game-message
  formatting, audit logging, and the periodic callback from `redis.c` into
  `redis_donation_runtime.c/.h`, beside the existing authenticated donation worker.
- Replaced the exported mutable `redis_donation_enabled` global with typed enabled/set
  accessors. Boot remains the only configuration owner, while the event registry reads the
  module state directly and no longer depends on the Redis umbrella header for donation
  declarations.
- Kept the exact eight-message per-pulse budget and local `redis_donation_worker_take()`
  loop. Subscription, socket polling, validation, replay protection, and retries remain on
  the worker thread.
- Reduced `redis.c` from 2,400 to 2,337 lines and `redis.h` from 84 to 81 lines. Extended
  the module-boundary gate to prevent donation delivery or mutable enabled state from
  returning to the composition root.

Performance effect:

- The event callback performs the same bounded local dequeue and player iteration as
  before. No Redis command, connection, allocation policy, retry, wait, or new gameplay
  work was introduced.
- Boot-time accessor calls replace direct boolean reads only during configuration and
  lifecycle transitions; the one periodic enabled check remains constant-time.

Validation:

- `./scripts/format.sh --check` and `make -C src -j2`: passed under the warning-as-error
  profile.
- Donation security/codec, live worker outage healing, failure containment, boot-log
  hygiene, administrator nonblocking, periodic-registry ASan/UBSan, and module-boundary
  regressions passed.
- All 24 `tests/async/test_redis*.py` regressions passed, including every isolated live
  Redis transport, worker, ACL, maintenance, and world-store case.

RDS-026 remains partially remediated. Player checkpoints and donation delivery are now
separate typed modules; world/floor orchestration, report-cache facades, presence delivery,
and administrative lifecycle policy remain in the Redis composition root.

### 2026-08-28 - RDS-026 player-checkpoint module boundary

Completed in this interval:

- Moved dirty-player marking, batched checkpoint capture, local dirty counts, worker-health
  snapshots, and the periodic checkpoint callback from `redis.c` into the typed
  `persistence_checkpoint.c/.h` module. The Redis public header no longer exports
  revisioned player-save APIs or includes player revision/observability types.
- Routed every checkpoint caller through the new header and removed the Redis umbrella
  include from eight translation units that use no Redis behavior. The baseline set of 29
  direct `redis.h` consumers is now 22, including the one implementation bridge that
  invokes the existing floor-flush callback after a checkpoint cycle.
- Reduced `redis.c` from the audit baseline of 2,525 lines to 2,400 and `redis.h` to 84
  lines. Added a boundary regression that rejects reintroduction of checkpoint APIs,
  player-save dependencies, or checkpoint-only consumers into the Redis module.
- Updated existing player-save, periodic-maintenance, pwipe, and Redis failure contracts
  to inspect the owning module rather than assuming monolithic source layout.

Performance effect:

- Runtime logic and call order are unchanged: dirty marking remains one bounded in-memory
  pipeline operation, periodic capture remains sliced at eight character IDs, and the
  existing asynchronous floor submission still runs only after the checkpoint cycle.
- The change reduces compile dependencies and review surface; it adds no allocation,
  Redis, SQL, filesystem, logging, sleep, or wait work to gameplay.

Validation:

- `./scripts/format.sh --check` and `make -C src -j2`: passed under the warning-as-error
  profile.
- All 24 `tests/async/test_redis*.py` regressions passed, including the new module-boundary
  gate and all isolated live Redis suites.
- Dirty-flush, player-save pipeline/revision, bounded periodic-maintenance, failure
  containment, and pwipe invalidation focused regressions passed.

RDS-026 remains partially remediated. The unrelated player-checkpoint surface is now
separate; remaining work is to move the world/floor, content-cache, presence/donation, and
administrative lifecycle facades out of the Redis composition root without changing their
already bounded runtime behavior.

### 2026-08-28 - RDS-015 pwipe and materialization runtime closure

Completed in this interval:

- Moved world-recovery quiescence ahead of the pwipe maintenance connection attempt. An
  active publisher and the floor pipeline are now canceled and joined even when the fresh
  administrative connection or fence renewal fails; the destructive reset remains failed
  closed and the already-quiesced process cannot publish old-season state.
- Extended the compiled world-recovery harness with a deliberately blocked publisher. A
  concurrent cancel is proven not to return until the publisher is released and joined,
  and the health snapshot is stopped before deletion is allowed to continue.
- Exercised the real recovery planner and materializer under ASan/UBSan with fake world,
  SQL-authority, and custody adapters. Duplicate item identities and a descendant moved
  out of its declared parent are rejected before mutation. SQL reconciliation failure
  creates nothing, while atomic custody-hydration failure removes the object root created
  earlier in the same attempt and clears the materialization guard.
- Refreshed the season-boundary source gate for the categorized Redis command wrapper so
  it continues to verify the maintenance `PING` without depending on a retired call shape.

Performance effect:

- The only production ordering change runs during explicit pwipe after gameplay has
  already entered shutdown. It adds no work to a game pulse, normal capture, publication,
  Redis command, or successful gameplay path.
- The blocking publisher, fake adapters, fault cases, and sanitizers exist only in the
  regression process.

Validation:

- `./scripts/format.sh --check` and `make -C src -j2`: passed.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed framing plus live-thread
  cancel/join ordering, duplicate/moved graph rejection, SQL custody refusal, atomic
  hydration failure, rollback, and materialization-guard checks under ASan/UBSan.
- Pwipe invalidation, quiescence, durable season-boundary, live Redis fault-recovery, and
  isolated scoped-deletion regressions passed.
- All 23 `tests/async/test_redis*.py` regressions passed. The complete recovery-codec,
  recovery-pipeline, recovery-transaction, pwipe fencing/quiescence/completion, and
  season-reset manifest/fence groups also passed.

RDS-015 is remediated. The layered suite now uses isolated Redis for transport, ACL,
timeout, atomic publication, ambiguous-reply, worker-healing, and destructive deletion
behavior; compiled runtime harnesses for publisher lifecycle and recovery
planning/materialization; executable backup fault stubs; and source contracts only for
static ownership and forbidden-call invariants.

### 2026-08-28 - RDS-015 live timeout and uncertain-reply recovery

Completed in this interval:

- Added an isolated TCP fault proxy that suppresses the reply only after Redis executes a
  selected command. A world-generation `EVAL` can therefore commit its pointer/floor
  handoff while the publisher observes transport failure; a direct retry of the same
  fenced sequence is proven successful with one current generation and no resurrected
  floor delta.
- Exercised the floor worker with the same fault at `EXEC`. The first batch attempt is
  deliberately ambiguous after Redis commits it, then the bounded worker reconnects,
  retries the idempotent mutations, drains successfully, resets its failure streak, and
  leaves one consistent hash/index result.
- Added a stalled-server case that reaches the real hiredis command deadline. This exposed
  that hiredis reports socket deadlines as `REDIS_ERR_IO` plus `EAGAIN` on this platform,
  which the health layer had counted as transport rather than timeout.
- Centralized Redis outcome classification for shared commands and all workers. Socket
  deadline errno values now map to timeout consistently, while EOF/reset failures remain
  transport failures. The connection adapter preserves an errored context after an
  authentication or database-selection transport failure so callers can classify and
  free it instead of losing the cause as a null context.

Performance effect:

- The classifier runs only after an existing Redis operation fails or completes on a
  background, boot, recovery, or stopped-server path. Successful connection and gameplay
  submission behavior is unchanged.
- No Redis, SQL, filesystem, process, network, logging, allocation, sleep, or wait work was
  added to gameplay. The fault proxy and stalled endpoint exist only inside the regression
  process.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_fault_recovery_live.py`: passed real socket timeout,
  committed-but-lost world `EVAL` reply, idempotent retry, lost floor `EXEC` reply,
  reconnect, retry, final-state, and worker-health checks against isolated Redis.
- `python3 tests/async/test_redis_command_observability.py` and
  `python3 tests/async/test_redis_connection_security_live.py`: passed centralized outcome
  and bounded connection/security contracts.
- All 23 `tests/async/test_redis*.py` regressions passed, as did the backup atomicity,
  authority-first recovery, recovery codec/pipeline, and persistence-status gates.

At this checkpoint RDS-015 remained partially remediated. Live coverage included worker outage healing,
timeouts, uncertain publication/transaction replies, fencing, atomic floor handoff, ACLs,
TLS/authentication, scoped maintenance deletion, presence, donation, caches, and bounded
queues. Completion still requires a live pwipe orchestration harness and a materialization
harness with injected semantic/SQL-custody failure and rollback checks.

### 2026-08-28 - RDS-025 background-worker observability

Completed in this interval:

- Added one reusable fixed-memory operation-health record to the presence, report-cache,
  floor, donation, and world-publication worker snapshots. Each reports calls, successes,
  failures, unavailable/timeout/transport/response categories, consecutive failures,
  total/last/maximum latency, and last-success age.
- Recorded Redis work at the existing worker execution boundaries, including presence
  lease refreshes, cache jobs, prepared floor batches, donation subscription/socket reads,
  and world-generation publish attempts. The world-store callback now returns a typed
  timeout/transport/response outcome instead of collapsing publication failure to a
  Boolean. Connection-open failures remain separately visible for the reconnecting
  presence, cache, floor, and donation workers.
- Exposed the same typed snapshots through both `redis detailed` and trusted `world
  persistence`. Both surfaces remain metadata-only and distinguish a worker with no
  observed operation from a healthy or consecutively failing worker.
- Added runtime counter-contract coverage plus live isolated-Redis assertions for outage
  healing, successful-operation counts, failure streak reset, latency, and last-success
  age across the reconnecting workers. World publication retains focused source contracts
  around its already tested bounded publisher lifecycle.

Performance effect:

- No gameplay submission, local cache lookup, donation dequeue, presence event capture,
  floor serialization, or incremental world-capture behavior changed. Timing and counter
  updates execute only on background worker threads around Redis work that already
  existed.
- Each record update uses fixed fields and a monotonic-clock sample under the worker's
  existing health mutex. Health rendering copies fixed snapshots and performs no Redis,
  SQL, filesystem, process, network, logging, allocation, sleep, or wait operation.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_command_observability.py`: passed shared and worker
  runtime counter, latency, outcome, failure-streak, and last-success-age contracts.
- `python3 tests/async/test_redis_cache_store_live.py`,
  `python3 tests/async/test_redis_floor_store_live.py`,
  `python3 tests/async/test_redis_presence_worker_live.py`, and
  `python3 tests/async/test_redis_donation_worker_live.py`: passed against isolated Redis.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed publisher instrumentation,
  bounded capture, framing, atomic publication, and restore contracts.
- All 22 `tests/async/test_redis*.py` regressions passed, as did the persistence-status,
  phase-01 recovery-load, and world-recovery codec/pipeline gates.

RDS-025 is complete. Shared commands and every active background Redis worker now expose
redacted calls, latency, categorized failures, failure streaks, last-success age, and
connection/reconnect state through the same local typed health snapshots used by both
operator surfaces.

### 2026-08-28 - RDS-025 shared-command observability

Completed in this interval:

- Added one fixed-memory typed health snapshot for the remaining shared Redis command
  layer used by boot cache priming, world/floor recovery reads, and stopped-server
  maintenance. Commands are classified by redacted subsystem/key class (`world`, `floor`,
  `cache_boot`, or `maintenance`) and operation kind (`read`, `write`, `scan`, or `script`).
- Recorded calls, successes, failure breakdown (unavailable, timeout, transport, Redis
  error reply, or missing reply), consecutive failures, total/last/maximum latency, and
  last-success age. Primary connection attempts, reconnect attempts/outcomes, and recovery
  transitions are tracked separately.
- Routed both `redis detailed` and trusted `world persistence` diagnostics through the same
  snapshot. The simple status now reports shared-layer health without treating one
  successful maintenance connection as proof that the primary world connection is healthy.
- Wrapped the previously direct paged floor `HMGET` call so every shared-context Redis
  command is classified and measured. Connection-open failures for cache priming and
  destructive maintenance are also represented instead of disappearing before command
  telemetry.

Performance effect:

- No gameplay queue submission, local cache read, floor mutation capture, presence update,
  donation dequeue, or world-capture path was changed. Measurement surrounds only Redis
  commands that already block during boot, recovery, or stopped-server maintenance.
- Counters are fixed-size relaxed atomics. Status copies a fixed four-scope snapshot and
  performs no Redis, SQL, filesystem, process, logging, allocation, sleep, or wait work.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_command_observability.py`: passed runtime classification,
  latency, failure-breakdown, consecutive-failure reset, last-success age, primary
  connection isolation, reconnect, and recovery-transition checks.
- `python3 tests/async/test_redis_admin_nonblocking.py` and
  `python3 tests/async/test_persistence_status_contract.py`: passed local-only operator and
  trusted persistence-diagnostic contracts.

RDS-025 remains partially remediated. This interval closes the shared boot/recovery/
maintenance command blind spot; a later interval must add comparable latency,
last-success-age, timeout, and consecutive-failure fields to each background worker before
the finding is complete.

### 2026-08-28 - RDS-018 bounded fuzzy recovery contract

Completed in this interval:

- Defined world recovery as a bounded fuzzy snapshot instead of claiming point-in-time
  semantics. The durable timestamp remains capture start, conservatively measuring age
  from the oldest possible record.
- Added a hard five-minute capture deadline. A capture that reaches it is discarded before
  publication, emits a fixed non-allocating failure completion, resumes the paused floor
  worker, and can be retried by a later periodic request from a new sequence.
- Restricted acceptable cross-time state to reconstructible NPC, door, and zone data.
  NPC equipment and inventory remain omitted, and NPC-carried gold is now forced to zero
  so recovery cannot replay transient currency into the persisted player economy. Floor
  item restore retains stable hierarchy/UID validation and complete SQL custody before any
  materialization; player and ship state remain SQL-authoritative.
- Made the door stage scan all fixed directions for one room per capture step. Empty and
  sparse rooms therefore consume up to six times fewer pulse-budget steps without changing
  the record or time ceilings.
- Added local-only capture expiry, active/last duration, capture-failure, and distinct
  publication-failure telemetry to `world persistence`.

Performance effect:

- Capture remains bounded to 64 steps or 2 ms per pulse. The only added active-capture work
  is one monotonic-clock age comparison per pulse; no Redis, SQL, filesystem, process,
  network, logging, sleep, or wait operation was added to gameplay.
- Failure handoff uses fixed pipeline state rather than allocating a completion record.
  Door traversal performs less work; the publisher and boot-only validation paths retain
  all external I/O and cryptography.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed capture expiry boundaries,
  failure handoff/source contracts, bounded pulse ownership, NPC gold exclusion, framing,
  atomic publication, and transactional restore contracts.
- `python3 tests/async/test_redis_failure_containment.py`,
  `python3 tests/async/test_redis_floor_world_gate.py`,
  `python3 tests/async/test_redis_world_store_live.py`,
  `python3 tests/async/test_world_recovery_codec.py`, and
  `python3 tests/async/test_redis_admin_nonblocking.py`: passed.

RDS-018 is complete under the audit's recommended explicit fuzzy-snapshot option. Exact
economic and persisted-item authority is excluded from fuzzy capture; reconstructible
world state has a documented maximum cross-time window and expired captures fail closed.

### 2026-08-28 - RDS-021 coherent report-cache freshness

Completed in this interval:

- Added explicit asynchronous Redis/local TTLs to the remaining persistent report caches:
  24 hours for boot-derived named-set output and 15 minutes for fraglist output. Epic-zone
  and artifact output retain their existing 15-minute bounds.
- Replaced the frozen rendered fraglist timer with a bounded `FRC1` frame containing stable
  display segments, generation time, absolute cap deadline, component lengths, and a
  content-derived revision. Cache hits validate the frame and render the countdown from
  the current time, so a displayed timer cannot freeze or count down stale text.
- Added a shared immutable-value transform to the local cache. Countdown rendering holds a
  reference only long enough to render outside the mutex and allocates only the returned
  display buffer; it does not create an intermediate cache-value copy.
- Invalidated fraglist state after successful level-cap maintenance commits as well as
  combat-outcome publication. The TTL remains a backstop if either asynchronous delete is
  dropped during an outage.
- Removed the redundant second `level_cap` SQL query during fraglist regeneration by
  deriving the other-side display from the already-read singleton row with the same
  configured clamp.

Performance effect:

- A cache hit remains one short local mutex acquisition and one result allocation. It adds
  bounded header parsing/countdown formatting but removes the intermediate allocation from
  the initial implementation; it performs no Redis, SQL, filesystem, process, logging, or
  wait operation.
- A fraglist miss now issues one fewer SQL query. Redis TTL publication and commit-driven
  invalidation remain coalesced worker jobs, so no Redis command is added to gameplay or
  maintenance completion paths.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_report_cache_codec.py`: passed under ASan/UBSan with dynamic
  countdown, expiry, future-clock, schema, revision, length, and corruption cases.
- `python3 tests/async/test_redis_cache_store_live.py`: passed against isolated Redis,
  including immutable local transform, TTL, outage healing, coalescing, and queue bounds.
- All 21 `tests/async/test_redis*.py` tests passed, including live worker, ACL, transport,
  recovery, namespace, failure-containment, and destructive-maintenance coverage.
- `python3 tests/async/test_frag_cap_config_contract.py`,
  `python3 tests/async/test_maintenance_scheduler.py`,
  `python3 tests/async/test_redis_failure_containment.py`, and
  `python3 tests/async/test_combat_outcome_transactional_cutover.py`: passed.

RDS-021 is complete: all active content caches have explicit bounded freshness or
revision/schema validation, authoritative mutation invalidation, and stable-data rendering.

### 2026-08-28 - RDS-008 least-privilege subsystem identities

Completed in this interval:

- Added immutable world, presence, cache, donation, and maintenance connection settings.
  Production requires complete scoped credential pairs, rejects empty or reused usernames,
  and requires the donation pair only when that subscriber is enabled. Local development
  retains the shared credential pair as an explicit fallback.
- Routed world/floor recovery, presence publication, reconstructible cache publication and
  boot priming, donation subscription, and pwipe/stopped-server deletion through their
  matching identities. The broad maintenance identity is opened only for pwipe preflight,
  pwipe deletion, or the stopped-server helper.
- Documented per-subsystem key/channel ACL boundaries and rotation behavior. The
  stopped-server helper prefers the maintenance pair, rejects a partial pair, and keeps its
  password out of command arguments.
- Added live Redis ACL isolation coverage proving allowed world, presence, cache, donation,
  and maintenance operations and `NOPERM` failures for cross-subsystem keys, channels, and
  unrelated application prefixes.

Performance effect:

- Credential resolution and immutable settings construction happen once at boot.
  Authentication occurs only when existing worker, boot/recovery, or maintenance
  connections open or reconnect; no per-command settings construction was added.
- Gameplay paths retain bounded serialization, local cache reads, and queue submission.
  They gain no Redis, SQL, filesystem, process, allocation, logging, wait, connection, or
  authentication work. Cache priming uses its own identity in one boot-only connection.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `./scripts/format.sh --check` and `git diff --check`: passed.
- All 21 `tests/async/test_redis*.py` tests passed, including isolated live Redis ACL,
  connection/TLS/socket, worker outage-healing, queue-bound, fencing, and publication tests.
- `python3 tests/async/test_season_reset_fence.py`,
  `python3 tests/async/test_data_lifecycle_manifest.py`, and
  `python3 tests/async/test_world_recovery_pipeline.py`: passed.

RDS-008 is complete: transport, authentication, database selection, deployment/season
isolation, authenticated sensitive payloads, and least-privilege subsystem identity
separation now fail closed in production.

### 2026-08-28 - RDS-008 authenticated recovery generations

Completed in this interval:

- Replaced the structural 56-byte `WRG1` manifest with a 120-byte `WRG2` authenticated
  manifest. It carries the bounded layout fields and writer-qualified upload token, a
  SHA-256 digest of the complete generation, and an HMAC-SHA256 tag.
- Bound the tag to the manifest, application/environment/deployment namespace, SQL season
  epoch, and generation sequence. Copying a valid manifest across deployments, seasons,
  or sequence keys therefore fails constant-time authentication.
- Required an independent 32-256 byte `REDIS_WORLD_STATE_SECRET` whenever world recovery
  is enabled. An optional bounded `REDIS_WORLD_STATE_SECRET_PREVIOUS` is read-only and
  supports a controlled rotation window; publication always uses the current key. Secret
  storage is scrubbed during reinitialization and shutdown.
- Verify the manifest HMAC before allocating the declared generation buffer. After bounded
  chunk reads, verify the complete payload digest before returning bytes to schema,
  semantic, SQL-custody, or materialization logic. Unsigned `WRG1`, wrong-key, forged tag,
  same-size chunk mutation, cross-context replay, malformed, missing, and oversized data
  all fail closed.

Performance effect:

- Generation SHA-256 and HMAC computation run only in the existing publisher worker.
  Gameplay capture retains the same bounded native copies and queue operation with no
  cryptography, Redis, SQL, filesystem, allocation, lock, logging, or wait added.
- Verification is boot/recovery-only. HMAC rejection occurs before generation allocation;
  the single payload digest pass occurs after the already-required bounded chunk read and
  before materialization.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_world_store_live.py`: passed live Redis publication and
  read with valid authentication, wrong-key rejection, previous-key rotation acceptance,
  same-size chunk forgery rejection, forged-manifest rejection, and existing fencing,
  chunking, TTL, cleanup, and cross-season cases.
- `python3 tests/async/test_redis_recovery_auth.py`: passed secret bounds, WRG2 layout,
  context binding, HMAC-before-allocation, digest-before-return, and worker-owned signing
  contracts.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed immutable publication,
  schema/sequence/framing validation, atomic floor handoff, and authenticated store
  integration contracts.

Redis write access alone can no longer forge an accepted world generation or donation
event.

### 2026-08-28 - RDS-008 Unix-socket transport

Completed in this interval:

- Extended the shared Redis connection adapter with an optional absolute Unix-socket path.
  TCP and socket endpoints are mutually exclusive, socket paths are bounded to the platform
  address limit, relative paths fail closed, and TLS is rejected for local socket transport.
  ACL username/password authentication, explicit database selection, connect timeouts, and
  command timeouts apply identically after either connection type is established.
- Routed the primary context plus presence, donation, cache, floor, and world workers through
  the same immutable socket-aware settings object. `REDIS_SOCKET` is mutually exclusive with
  `REDIS_HOST` and `REDIS_PORT`; ambiguous configuration disables Redis before connecting.
- Extended stopped-server destructive maintenance to accept exact
  `unix:/absolute/socket/database` allow-list and confirmation targets. Socket maintenance
  retains local-environment, namespace, authentication, database, bounded deletion, and
  empty-postcondition gates while rejecting TCP fields or TLS in socket mode.
- Documented both transports in `.env.example`, the configuration reference, the migration
  runbook, and operator help without weakening the existing production TCP/TLS policy.

Performance effect:

- Endpoint selection occurs only while constructing or reconnecting a Redis context. Unix
  sockets can reduce local Redis transport overhead by avoiding the TCP/IP loopback stack.
- Gameplay remains unchanged: it performs no connection construction, Redis command, wait,
  allocation, filesystem access, or endpoint branching. All Redis I/O stays in existing
  workers, boot/recovery, shutdown, or explicit maintenance paths.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_connection_security_live.py`: passed under ASan/UBSan
  against authenticated TCP, verified TLS, and authenticated Unix-socket endpoints,
  including explicit database selection and invalid mixed/relative/TLS socket rejection.
- `python3 tests/async/test_redis_clear_scoped_live.py`: passed both TCP and Unix-socket
  destructive maintenance against isolated Redis while preserving unrelated keys.
- `python3 tests/async/test_migration_runner_cli_safety.py`: passed socket/TCP target syntax,
  mutual-exclusion, allow-list, confirmation, and no-`FLUSHDB` contracts.

RDS-008 transport and namespace work is complete. Recovery-payload authentication was
completed by the later WRG2 interval; least-privilege subsystem identity separation remains
open within the finding.

### 2026-08-28 - RDS-008 deployment namespace isolation

Completed in this interval:

- Made `REDIS_NAMESPACE` mandatory whenever Redis is enabled. Boot accepts only
  `duris:<ENVIRONMENT>:<deployment>`, requires the environment component to exactly match
  `ENVIRONMENT`, and restricts deployment identifiers to a bounded lowercase safe alphabet.
  Missing, malformed, oversized, or cross-environment values disable Redis before a
  connection or worker can start.
- Replaced the fixed `mud:season:<epoch>:` active prefix with
  `<REDIS_NAMESPACE>:season:<epoch>:` for world recovery, floor deltas, presence, retry
  tokens, report caches, player transition events, and donation delivery. The namespace
  and SQL epoch are captured once at boot and passed immutably to the world store and
  background workers.
- Extended stopped-server destructive maintenance to delete only the configured namespace
  plus the explicitly retained legacy `mud:*` and retired `ship:snapshot:*` patterns.
  Namespace syntax, local-environment enforcement, target allow-listing, exact confirmation,
  bounded deletion, and empty postconditions all fail closed.
- Updated the DurisWeb and donation producer contracts to require the same configured
  namespace as the server. Separate local, production, blue/green, test, or developer
  deployments can safely share a Redis database without addressing each other's active
  keys or channels.

Performance effect:

- Namespace validation and key construction occur only during boot. The key builder uses
  bounded stack buffers and direct copies with no heap allocation, Redis command, SQL
  query, lock, wait, filesystem access, or process work.
- Gameplay cache reads retain the preformatted keys introduced by RDS-001. Presence,
  donation, floor, and world operations continue to hand immutable resolved names to their
  existing bounded workers, so no per-event namespace formatting was added.
- The additional namespace scan is confined to explicitly invoked stopped-server
  destructive maintenance.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_namespace.py`: passed under ASan/UBSan with valid local
  and production namespaces plus missing, cross-environment, unsafe-character, boundary,
  oversized, and undersized-output rejection.
- `python3 tests/async/test_redis_season_scope.py`: passed namespace-plus-epoch inventory,
  boot capture, worker configuration, and cache resolution contracts.
- `python3 tests/async/test_redis_clear_scoped_live.py`: passed against isolated Redis,
  deleting current-namespace and legacy keys while preserving an unrelated application.
- `python3 tests/async/test_redis_world_store_live.py`: passed namespace-aware fencing,
  publication, read, consume, cleanup, and cross-season isolation against isolated Redis.
- `python3 tests/async/test_redis_key_registry.py` and
  `python3 tests/async/test_migration_runner_cli_safety.py`: passed registry ownership and
  destructive-maintenance safety contracts.

Remaining RDS-008 work is Unix-socket transport. ACL/password authentication, verified
TLS, explicit database selection, production endpoint policy, and complete active-surface
namespace isolation are implemented.

### 2026-08-28 - RDS-001 complete season-scoped Redis isolation

Completed in this interval:

- Bound every active Redis key, key pattern, prefix, and pub/sub channel to
  `mud:season:<epoch>:`. Presence, retry tokens, report caches, player transition events,
  and donation notices now use the same durable SQL season identity as world recovery and
  floor deltas.
- Captured the active SQL epoch once during Redis initialization. All workers receive
  immutable resolved keys or channels, cache keys are preformatted at boot, and the old
  process continues targeting only its captured old epoch after pwipe advances SQL to the
  next epoch.
- Kept the previous unscoped presence, cache, donation, world, floor, online, and ship
  surfaces as explicit cleanup-only registry entries. They can be removed during a
  connected pwipe but can never become active again.
- Strengthened destructive invalidation postconditions. Pattern deletion performs a
  second complete `SCAN` proving no match remains, direct deletion verifies `EXISTS=0`,
  world metadata and floor hash/index deletion verify absence, and ship cleanup verifies
  its retired pattern is empty.
- Updated DurisWeb and donation producer contracts to read `season_reset_state` and use
  only the active epoch. Cross-season live tests prove that a presence worker does not
  touch old keys and a donation worker has no subscription to an old channel.

Performance effect:

- Epoch resolution and all fixed key formatting occur once during boot. Cache reads use
  preformatted keys, so gameplay report lookups add no per-call formatting, SQL query,
  Redis command, allocation, lock, or wait.
- Presence and donation behavior remains worker-owned and bounded. Their gameplay paths
  still perform only the existing payload validation/copy and queue operation.
- The additional `SCAN` and `EXISTS` postconditions run only during pwipe, administrator
  cleanup, or stopped-server maintenance after writers are quiesced.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_season_scope.py`: passed all-active-surface inventory,
  boot-captured epoch, immutable worker configuration, cache resolution, and old-surface
  cleanup contracts.
- `python3 tests/async/test_redis_presence_worker_live.py`: passed under ASan/UBSan,
  including old-season key isolation.
- `python3 tests/async/test_redis_donation_worker_live.py`: passed under ASan/UBSan,
  including zero old-season subscribers and bounded current-season delivery.
- `python3 tests/async/test_redis_pwipe_invalidation.py` and
  `python3 tests/async/test_season_reset_fence.py`: passed checked deletion,
  postcondition, boot fence, and irreversible-boundary contracts.
- `python3 tests/async/test_redis_key_registry.py` and
  `python3 tests/async/test_data_lifecycle_manifest.py`: passed with 42 declared Redis
  surfaces, including the retained cleanup-only legacy inventory.

### 2026-08-28 - RDS-017 bounded recovery commands and aggregate memory

Completed in this interval:

- Replaced each monolithic generation value with a 56-byte `WRG1` manifest and at most
  64 writer-qualified chunks of at most 1 MiB each. Publication writes expiring chunks on
  the existing publisher thread, then atomically advances the fenced generation pointer
  to the manifest. Writer-qualified upload keys prevent a stale publisher from deleting a
  current publisher's chunks.
- Reads validate the exact manifest first, check each chunk with `STRLEN`, require the
  exact expected chunk size, and assemble no more than the 64 MiB generation ceiling.
  Missing, malformed, incorrectly sized, or oversized chunks fail recovery closed.
- Added a season-scoped sorted-set index for floor-record UIDs. The floor worker updates
  the hash and index together in transactions bounded to 64 mutations and 1 MiB of value
  data. Snapshot barriers expose only completed groups and retries are idempotent.
- Replaced boot-time `HGETALL` with count-checked, 64-record `ZRANGE`/`HMGET` pages. Boot
  enforces 32,768 floor records, 16 MiB of floor payload, and a 64 MiB combined
  generation-plus-floor payload ceiling before recovery planning.
- Extended the Redis registry from 35 to 37 surfaces for the generation-chunk key format
  and floor index. Generation publication and administrator cleanup delete the floor hash
  and index together, and generation manifests/chunks retain bounded TTL cleanup.

Performance effect:

- Gameplay capture is unchanged: it performs only the existing bounded native snapshot
  copies and queue operations. It performs no Redis, SQL, filesystem, process, logging, or
  wait operation.
- The largest generation Redis command or reply is reduced from roughly 64 MiB to 1 MiB,
  eliminating the generation-sized Hiredis command-formatting buffer. Chunk work remains
  publisher-thread or boot-only.
- Floor publication remains worker-only. Transaction groups cap queued Redis output at
  64 mutations and 1 MiB of values; boot pages cap each floor fetch to 64 validated
  records instead of allocating the complete hash reply.
- Application-owned recovery bytes are explicitly capped at 64 MiB combined, including
  at most 16 MiB of floor payload. Planning/materialization allocations remain boot-only
  and are bounded by the same validated record counts and payload ceilings.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed bounded chunk, manifest,
  index, and no-game-thread-I/O source contracts.
- `python3 tests/async/test_redis_failure_containment.py`: passed bounded worker,
  transactional index, and fail-closed publication contracts.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed paged boot-read and feature
  gate contracts, including the absence of `HGETALL`.
- `python3 tests/async/test_redis_floor_store_live.py`: passed under ASan/UBSan against
  isolated Redis with hash/index consistency before and after an ordered barrier.
- `python3 tests/async/test_redis_world_store_live.py`: passed against isolated Redis with
  multi-chunk round trips, stale-writer isolation, TTLs, previous-generation cleanup, and
  missing, oversized, and malformed chunk/manifest rejection.
- `python3 tests/async/test_redis_key_registry.py` and
  `python3 tests/async/test_data_lifecycle_manifest.py`: passed with 37 Redis surfaces.

### 2026-08-28 - RDS-016 portable world-recovery codec

Completed in this interval:

- Replaced the ABI-dependent schema-8 generation with schema 9: an exact 64-byte header,
  fixed-width little-endian integers, explicit record types, per-record version bytes, and
  required zeroed reserved fields. Mobile, affect, object-tree, door, and zone records no
  longer persist compiler padding, host-endian values, `time_t`, or `unsigned long` layout.
- Added field-by-field decoding with exact length and count checks, fixed string bounds,
  native-width overflow checks, schema/type/version rejection, and existing semantic and
  SQL-authority validation before materialization.
- Moved generation encoding into the existing publisher thread. It compacts the native
  capture buffer in place using one fixed worker-local scratch record, so publication does
  not allocate a second generation-sized buffer.
- Changed floor deltas to the same encoded object-tree layout under the `WRF3:` prefix.
  Conversion occurs in the existing floor worker after submission, and the hash field UID
  must equal the decoded root UID before boot planning.
- Documented the wire format and deliberate fail-closed schema-8 policy. Because recovery
  data is expiring and reconstructible, an old generation takes the normal-boot path; the
  first schema-9 publication replaces it and atomically clears old floor deltas.

Performance effect:

- Gameplay capture still performs the same bounded native field capture and memory copies.
  It gains no durable encoding loop, allocation, lock, Redis call, logging, or wait.
- Generation encoding and checksumming remain publisher-thread work. In-place compaction
  avoids a second generation-sized allocation and reduces the durable header from the
  native 72-byte layout to 64 bytes on the supported build.
- Floor encoding is worker-owned. Submission retains the existing bounded copy and queue;
  the game thread does not parse or encode the durable representation.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_world_recovery_codec.py`: passed under ASan/UBSan with exact
  header, record-header, door, object, mobile/affect, and floor golden vectors plus
  field-by-field round trips and malformed-version checks.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed schema, checksum, framing,
  bounds, background publication, fencing, and transactional recovery contracts.
- `python3 tests/async/test_redis_floor_store_live.py`: passed against isolated Redis,
  including background conversion and exact `WRF3:` root-UID verification.

### 2026-08-28 - RDS-022 authoritative Redis key registry

Completed in this interval:

- Added one declarative registry, initially covering 35 active and cleanup-only Redis keys, key
  formats, prefixes, patterns, and channels across world recovery, floor deltas,
  presence, content caches, donation delivery, and retired ship snapshots.
- Replaced every `mud:` and `ship:snapshot:` literal in runtime C/C++ sources with
  compile-time constants generated from the registry. World, presence, donation, cache,
  pwipe, administrator, and ship-cleanup paths now consume the same declarations.
- Made the lifecycle validator derive the Redis store IDs, kinds, and exact locators from
  the registry instead of a second handwritten Python list. The manifest now explicitly
  inventories content caches, donation pub/sub, retired ship cache cleanup, floor deltas,
  world recovery, and presence.
- Added fail-closed tests for unknown, missing, duplicate, and unused registry stores;
  manifest/registry drift; runtime Redis literals outside the registry; and destructive
  maintenance patterns that differ from the registry-owned patterns.

Performance effect:

- Runtime key lookup remains compile-time constant access. No registry parsing, scanning,
  allocation, lock, network operation, or additional branch runs in gameplay paths.
- Artifact boot priming now formats its six declared keys into fixed stack buffers instead
  of duplicating those keys in a command literal. All validation and inventory comparison
  runs offline in Python.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_key_registry.py`: passed with 37 declared surfaces and
  exact destructive-maintenance pattern matching.
- `python3 tests/async/test_data_lifecycle_manifest.py`: passed, including registry drift
  and fail-closed coverage.
- `python3 scripts/validate_data_lifecycle.py --json`: passed with 172 database tables,
  21 non-database stores, and 37 Redis surfaces.

### 2026-08-28 - RDS-027 supported server build contract

Completed in this interval:

- Removed the commented `-D__NO_MYSQL__` switch from the production Makefile. The partial
  preprocessor stubs remain available to focused unit harnesses but are no longer presented
  as a whole-server build configuration.
- Corrected the build and help-system guides to state that MySQL/MariaDB client support is
  mandatory for the server. Redis remains optional at runtime while Hiredis and OpenSSL are
  intentionally mandatory build dependencies of the single supported binary.
- Added a focused source/documentation contract that fails if the unsupported toggle is
  advertised again or the mandatory dependency statement disappears.

Performance effect:

- No runtime source, flags, linked-library set, or executable behavior changed. This
  interval only makes the supported build matrix truthful.

Validation:

- `make -C src -j2`: passed with the maintained warning-as-error profile.
- `python3 tests/async/test_supported_server_build_contract.py`: passed.
- `python3 tests/async/test_documentation_contract.py`: passed.

### 2026-08-28 - RDS-008 runtime connection security

Completed in this interval:

- Replaced six independent host/port-only connection constructors with one immutable,
  shared settings object and one bounded connection adapter used by the primary context,
  presence, caches, floor publication, world publication, and donation subscription.
- Every connection now authenticates with either password-only `AUTH` or ACL
  username/password authentication when configured, then explicitly selects `REDIS_DB`.
  Authentication or selection failure closes the connection and cannot degrade into an
  unauthenticated database-0 connection.
- Added verified TLS with a reusable Hiredis/OpenSSL context, CA validation, peer
  verification, SNI/certificate-name support, and a fail-closed rule that refuses a
  non-loopback production endpoint without TLS.
- Centralized strict validation for port, database, TLS boolean, username/password
  pairing, CA presence, and production transport policy. Logs never include credentials.
- Retained the larger bounded command timeout used only for background world-generation
  payload publication without weakening the normal 100 ms command bound.

Performance effect:

- Authentication, database selection, and the optional TLS handshake execute only while
  creating a connection at boot or on an existing background worker reconnect. No new
  Redis command, socket operation, allocation, logging, or wait was added to a game pulse,
  player command, cache read, login/logout submission, or floor capture path.
- All workers retain their existing fixed queue, byte, batch, retry, and backoff limits.
  The TLS context is created once and reused across connections rather than rebuilt for
  every worker reconnect.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_connection_security_live.py`: passed under ASan/UBSan
  against isolated password-protected plaintext and TLS Redis servers, proving password
  authentication, ACL `default` username authentication, wrong-password rejection,
  database-2 isolation, verified TLS/SNI, certificate-name mismatch rejection,
  invalid-database rejection, and the required-TLS fail-closed gate.
- All five existing live cache, floor, presence, donation, and world-store tests passed,
  preserving outage healing, queue saturation, fencing, and publication behavior.

### 2026-08-28 - RDS-023 scoped destructive Redis maintenance

Completed in this interval:

- Removed every `FLUSHDB`/`FLUSHALL` operation from the standalone clear script and the
  legacy migration runner. Both now use one shared cursor scanner that deletes only
  `mud:*` and retired `ship:snapshot:*` keys, with at most 128 keys per `DEL` call.
- Added fail-closed destructive gates: exact `ENVIRONMENT=local`, numeric explicit Redis
  database, exact `host:port/database` allow-list membership, and exact target
  confirmation. Non-loopback targets require verified TLS with a readable CA file.
- Added maintenance support for Redis ACL username/password, explicit database selection,
  TLS, and CA verification. Passwords reach `redis-cli` through `REDISCLI_AUTH` rather than
  a process-list command argument.
- Added a full postflight scan for both owned patterns. Missing `redis-cli`, failed PING,
  command failure, malformed scan reply, remaining key, unsafe environment, wrong target,
  or wrong confirmation returns nonzero.
- Made the legacy migration skip Redis connection requirements only when `REDIS` is not
  enabled. When enabled, a missing client or any invalidation/postflight failure increments
  the migration failure count and produces a nonzero exit.
- Corrected the runbook and database/configuration references to describe the actual
  owner-only `.env`, allow-list, confirmation, scoped deletion, ACL/TLS/database settings,
  stopped-writer requirement, and failure semantics.

Performance effect:

- No runtime or gameplay code changed in this interval. Destructive maintenance is
  offline-only and uses cursor scans plus `DEL` chunks of at most 128 keys instead of
  blocking an entire shared database with `FLUSHDB`.
- Unrelated Redis application keys remain untouched, so Duris maintenance no longer
  invalidates another service's cache or working set.

Validation:

- `python3 tests/async/test_redis_clear_scoped_live.py`: passed against an isolated Redis,
  proving local/allow-list/confirmation gates, exact deletion of three Duris keys,
  preservation of an unrelated key, and a clean postflight.
- `python3 tests/async/test_migration_runner_cli_safety.py`: passed.
- `bash -n scripts/clear-redis.sh scripts/clear-duris-redis-keys.sh
  migrations/run_migration.sh`: passed.
- `shellcheck scripts/clear-redis.sh scripts/clear-duris-redis-keys.sh`: passed.
- `python3 scripts/security_source_check.py`: passed.
- `python3 tests/async/test_documentation_contract.py`: passed.

### 2026-08-28 - RDS-011/RDS-020 nonblocking runtime administration and donation delivery

Completed in this interval:

- Replaced all remote key existence, count, and TTL queries in `redis` and `redis detailed`
  with bounded local snapshots from the world, floor, cache, presence, donation, and
  player-save pipelines. Both commands explicitly identify their output as local telemetry.
- Retired the ambiguous administrator helpers that mapped Redis failures to valid empty
  values: `redis_key_exists`, `redis_get_ttl`, `redis_hlen`, `redis_scard`, and the now-unused
  remote world timestamp accessor.
- Changed report-cache invalidation APIs to return queue acceptance. Online administrator
  cache clears now say `Queued`, `Rejected`, or `Partial` instead of claiming a remote key
  was already cleared.
- Refused world, floor, and all-state clears while the server is live. Those operations
  require stopped-writer proof, scans, fencing, and postflight checks and no longer run on
  the simulation thread under an administrator command.
- Moved donation connect, subscribe, socket reads, signature validation, replay filtering,
  and exponential reconnect backoff into a dedicated worker. Its fixed queue holds at most
  64 validated events and retains at most 256 replay IDs; the game pulse only dequeues and
  broadcasts at most eight fixed-size events.
- Added donation worker health for connection failures, reconnects, received, validated,
  rejected, replayed, dropped, queue depth, and high water to local administrator status.

Performance effect:

- Online administrator status and cache-clear commands issue no Redis commands, scans,
  reconnects, or waits. Cache invalidation is the existing bounded background queue.
- Donation outages and reconnects can no longer consume the 250 ms connect or 100 ms
  command timeout on a game pulse. The pulse does only a bounded mutex-protected dequeue
  and at most eight existing broadcasts.
- Broad destructive work is rejected online instead of risking a multi-page `SCAN` stall.
  Boot, recovery, shutdown, pwipe, and stopped-server maintenance retain checked Redis I/O.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_donation_worker_live.py`: passed under ASan/UBSan,
  covering startup outage healing, authenticated delivery, replay rejection, and bounded
  flood behavior at the 64-event queue limit.
- `python3 tests/async/test_redis_donation_security.py`: passed.
- `python3 tests/async/test_redis_admin_nonblocking.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_cache_store_live.py`: passed.

### 2026-08-28 - RDS-009 expiring presence leases

Completed in this interval:

- Replaced the persistent `mud:online` hash with an expiring generation pointer and
  per-session keys: `mud:presence:current` selects one worker instance and
  `mud:presence:session:<instance>:<pid>` holds the privacy-filtered JSON.
- Session and generation keys expire after 180 seconds. The background presence worker
  retains at most 1,024 active payloads and refreshes them every 60 seconds in Lua batches
  of at most 64, so a crash or missed logout ages all presence out without another process.
- Boot clear atomically claims a new presence generation and deletes the legacy hash.
  Old-generation session keys become invisible immediately and expire naturally.
- Heartbeats require the exact current worker instance. If a newer process claims the
  pointer, the older worker drops its local active set and cannot overwrite or republish
  the new generation.
- Updated pwipe deletion to remove the generation pointer, all session keys, retry tokens,
  and the legacy hash. Updated lifecycle inventory and the DurisWeb read contract for the
  new key model.
- Added active-session, lease-refresh, and lease-failure counters to local administrator
  health without adding status-time Redis queries.

Performance effect:

- Login, logout, invisibility, and disconnect paths still perform only bounded JSON
  encoding and queue submission; they gain no Redis command, reconnect, or wait.
- Lease refresh is entirely worker-owned. One background Lua call refreshes up to 64
  sessions every 60 seconds, with the active set capped at the existing 1,024 queue bound.
- Consumers avoid a persistent full hash and read only the current generation's expiring
  keys. Missing or expired keys are ordinary offline results.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_presence_worker_live.py`: passed under ASan/UBSan,
  covering startup outage healing, exact payload/TTL publication, background refresh,
  logout deletion, wrong-type retry bounds, newer-instance fencing, post-worker expiry,
  cancellation, and queue/payload limits.
- `python3 tests/async/test_redis_presence_privacy.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_data_lifecycle_manifest.py`: passed.

### 2026-08-28 - RDS-004/RDS-005 authority-first transactional recovery

Completed in this interval:

- Replaced root-plus-immediate-VNUM recovery with schema 9 item records containing a
  complete, ordered tree of at most 12 items. Every node carries its fixed-width UID,
  root UID, parent UID, VNUM, type, values, timers, and bounded display strings. The same
  binary record is used by world generations and `WRF3` floor deltas, so descendants are
  never regenerated by VNUM with a new identity.
- Added a no-mutation semantic planning phase. It resolves every room, mobile, object,
  door, and zone; rejects invalid ranges and strings; verifies parent-before-child tree
  topology; and rejects duplicate UIDs across both the generation and floor records.
- Batch-reconciles every planned item against `item_current_owner` and
  `item_owner_revision` in one boot-only SQL transaction. Recovery requires exact UID,
  root, parent, room owner/context, VNUM, active state, and row count before creating an
  entity.
- Added atomic multi-owner runtime-custody hydration. SQL reads now return owned typed
  rows without mutating runtime custody; live entities are created first, then all
  custody rows are installed as one rollback-capable operation, and doors/zones are
  applied only after that succeeds.
- Tracks every newly created mob and object root and removes them on any creation or
  hydration failure. Existing SQL-loaded room trees may be reused only when their exact
  UID/VNUM/topology and complete node count match the plan.
- Combined floor and generation records into the same plan and authority transaction.
  Redis floor records are decoded but never materialized independently, and recovery
  data is cleared only after the complete restore succeeds.
- Defined NPC-held items as non-authoritative recovery data: world recovery preserves the
  mobile state but deliberately serializes no equipment or inventory. This prevents the
  old VNUM-only recreation path from duplicating valuable items.
- On any recovery failure, boot now performs a full normal reset of every zone. It no
  longer reports that normal state already exists after the earlier reset was skipped.

Performance effect:

- Drop capture remains fixed-memory and bounded to 12 nodes. It performs no Redis, SQL,
  disk, process, or logging work; socket publication remains on the floor worker.
- Floor values are submitted as their binary records, removing JSON object construction,
  heap-owned string duplication, and text encoding. Pending-drop removal now swaps with
  the final bounded entry instead of shifting the rest of the batch.
- SQL graph reconciliation and atomic runtime hydration run only during boot recovery.
  Queries are batched at 256 UIDs, while ordinary gameplay and periodic capture retain
  their existing time/record budgets.

Failure policy and bounds:

- A tree larger than 12 nodes is not recoverable and fails capture closed rather than
  truncating descendants. Twelve matches the live item-movement transaction bound.
- Schema 7 generations and legacy JSON floor values are rejected; they cannot be mixed
  with the identity-complete schema.
- This closes custody and transactional materialization, but does not close RDS-016's
  native-layout wire-format finding or RDS-017's aggregate `HGETALL` memory concern.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_world_recovery_transaction.py`: passed.
- `python3 tests/async/test_item_ownership_runtime.py`: passed, including rejected and
  successful multi-owner atomic hydration.
- `python3 tests/async/test_redis_floor_store_live.py`: passed with binary values that
  include embedded NUL bytes.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.

### 2026-08-28 - RDS-024 explicit clean-restart recovery

Completed in this interval:

- Defined the existing graceful-restart behavior as intentional restart recovery rather
  than treating every fresh generation as evidence of a crash.
- Added a season-scoped, fenced clean-shutdown marker containing the exact current
  generation sequence. It is written only after world and floor workers drain, expires
  with the generation, and cannot be written by a stale writer token.
- Consume the marker atomically at the next Redis initialization. A marker classifies the
  boot as clean only when its sequence exactly matches the validated current generation;
  absent, stale, failed, or already-consumed markers classify recovery as crash recovery.
- Added a non-quiescing, compare-and-delete operation for a successfully restored
  generation. Boot no longer calls the administrator clear path that cancels the writer
  and disables recovery publication until another restart.
- Updated boot logs and operator documentation to distinguish clean restart, crash, and
  copyover recovery.

Performance effect:

- Marker work is limited to graceful shutdown and boot. It adds no gameplay-loop work.
- Successful restore consumes one exact generation in a single Lua operation while
  keeping the already-claimed publisher fence and background pipeline available.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_world_store_live.py`: passed against isolated Redis,
  covering stale-writer rejection, marker TTL, exact sequence, and one-use consumption.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.

### 2026-08-28 - RDS-011 asynchronous floor deltas and snapshot barrier

Completed in this interval:

- Moved floor `HSET`/`HDEL`, connection attempts, retries, and reply collection to a
  dedicated worker. Gameplay now serializes an immutable bounded batch and enqueues it.
- Bounded the worker to eight batches, 16 MiB of values, 2,048 mutations per batch,
  256 KiB per value, and 128 bytes per key. Connection outages use exponential backoff;
  permanent command errors stop retrying after three attempts.
- Added an ordered barrier before each world capture. Pre-capture deltas must be
  acknowledged before capture begins; the floor worker then pauses so post-barrier
  mutations cannot be deleted by the generation's atomic floor handoff.
- Resume occurs after either publication completion or capture failure. Destructive
  quiesce cancels and joins the floor worker before deleting state.
- Added queue, byte, barrier, completion, failure, reconnect, and drop health without a
  Redis query.

Performance effect:

- Object drops, pickups, periodic floor flushes, and world-save preflight no longer wait
  for Redis. A full gameplay batch performs bounded binary serialization and memory
  copies, then returns after a queue lock; all socket I/O is background work.
- Floor mutations remain pipelined per immutable batch, preserving the prior reduction in
  Redis round trips.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_floor_store_live.py`: passed under ASan/UBSan against
  isolated Redis, proving initial-outage healing, pre-barrier visibility, paused
  post-barrier work, ordered resume, shutdown drain, and delete/set behavior.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.

### 2026-08-28 - RDS-017 bounded world-recovery memory and retention

Completed in this interval:

- Rejected recovery values above the 64 MiB wire-format ceiling before validation and
  before publication.
- Added a server-side `STRLEN` guard to recovery reads, so Redis does not transfer an
  oversized generation to Hiredis before the process rejects it.
- Reserved the durable header at capture start and published that same owned blob. The
  publisher no longer allocates and copies a second header-plus-payload vector.
- Added an atomic generation TTL of at least one hour (or four times the configured
  accepted recovery age). Successful, uncertain, and orphaned generation writes now age
  out even if a later cleanup never observes them.
- Scaled the worker-only publication timeout from the payload size at a conservative
  16 MiB/s assumption, capped at five seconds, instead of applying the 100 ms control
  command timeout to a possible 64 MiB write.

Performance effect:

- No new work runs during ordinary gameplay. The size guard is a boot/admin recovery
  operation, while framing and publication remain on the existing recovery worker.
- Publishing removes one full-generation allocation and memory copy. Oversized restore
  candidates are rejected inside Redis without transferring the value to the game process.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed, including the 64 MiB
  validation ceiling and single-owned-blob publication contract.
- `python3 tests/async/test_redis_world_store_live.py`: passed against isolated Redis,
  including the generation TTL and publication size ceiling.
- `python3 tests/async/test_redis_failure_containment.py`: passed.

Remaining related work was completed by the later chunked-generation and indexed-floor
interval above.

### 2026-08-28 - RDS-011/RDS-021 local report cache and asynchronous publication

Completed in this interval:

- Replaced synchronous Redis reads for named reports, fraglists, epic-zone reports, and
  artifact lists with a 32-entry in-process cache. Existing TTLs use a monotonic local
  deadline, and callers retain their SQL/generation fallback on a local miss.
- Moved cache `SET`, `SETEX`, and `DEL` operations to a dedicated worker. The queue is
  bounded to 64 jobs, 4 MiB total payload, 1 MiB per value, and 128 bytes per key.
- Coalesced queued mutations by key without replacing the in-flight front job, so an
  outage retains the newest cache state without accumulating redundant report payloads.
- Added bounded worker-only timeouts, exponential reconnect backoff, three-attempt
  command-error handling, one-second shutdown drain, and pwipe cancel/join before checked
  deletion.
- Preserved restart-time artifact cache hits with one boot-only Lua read of all six
  artifact variants and their remaining TTLs. Persistent or expired legacy values are not
  seeded, and local TTL is capped at the existing 900-second contract.
- Added local queue, byte, coalescing, drop, failure, reconnect, and entry-count health to
  administrator status without a Redis query.

Performance effect:

- Player-facing named-report, fraglist, epic-zone, and artifact cache hits perform no
  socket I/O. They take a short mutex and one bounded memory copy.
- Report publication and transaction-triggered cache invalidation no longer wait on Redis.
  Repeated mutations for one key collapse to the newest queued value.
- The only warm-cache read is one batched boot operation before the game loop. Named,
  fraglist, and epic-zone caches are generated during the existing boot sequence.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_cache_store_live.py`: passed under ASan/UBSan, covering
  local reads before Redis startup, outage healing, coalescing, TTL behavior, immediate
  local invalidation, job/byte/value bounds, and cancellation.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_artifact_cache_codec.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.

Remaining related work was completed by the later simulation-thread isolation and coherent
report-cache freshness intervals above.

### 2026-08-28 - RDS-009/RDS-011 asynchronous presence publication

Completed in this interval:

- Moved Redis presence `HSET`, `HDEL`, and login/logout publication off login, quit, and
  disconnect paths onto a dedicated worker with an owned Redis connection.
- Added a fixed 1,024-job queue. Submission performs only a bounded payload copy under a
  short mutex; a full queue rejects new noncritical work without waiting for Redis.
- Added worker-only bounded connection and command timeouts plus exponential reconnect
  backoff from 100 ms to 60 seconds. Jobs queued during an outage remain ordered and are
  delivered after Redis heals.
- Combined each state mutation and optional event into one Lua operation. A one-hour
  operation token makes a timeout retry idempotent, avoiding duplicate pub/sub events when
  the first result was ambiguous.
- Retry markers are written only after the state mutation succeeds. Three consecutive
  command errors drop the noncritical job so a permanent wrong-type or ACL failure cannot
  block every later presence update.
- Pwipe cancels and joins the worker before checked presence deletion. Normal shutdown
  drains it for at most one second, then discards remaining noncritical work.
- Added local worker state, queue depth/high-water, completion, drop, failure, and
  reconnect counters to administrator status without adding a Redis query.
- Added the presence state, channel, and one-hour retry-token keyspace to the lifecycle
  manifest and its fail-closed required-store validation.

Performance effect:

- Login, logout, link-loss, and invisibility handling perform no Redis connection or
  command wait. They now do bounded JSON encoding and queue submission only.
- A visible session transition previously made two sequential synchronous Redis calls;
  it now makes one Lua call on the background worker. Redis outage reconnects never run on
  the simulation thread.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_presence_worker_live.py`: passed under ASan/UBSan,
  including submission before Redis startup, ordered outage healing, final presence state,
  permanent Redis command errors, cancellation races, shutdown drain, and queue
  saturation.
- `python3 tests/async/test_redis_presence_privacy.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_data_lifecycle_manifest.py`: passed.
- `python3 tests/async/test_boot_log_hygiene.py`: passed.

Remaining related work:

- RDS-009 expiry was subsequently completed in the interval above.
- RDS-011 simulation-thread isolation was subsequently completed in the interval above;
  world/floor preflight remains boot-only.

### 2026-08-28 - RDS-011 pipelined floor-delta flush

Completed in this interval:

- Changed floor-delta flushes from one blocking request/reply cycle per `HDEL` or `HSET`
  to hiredis command buffering followed by one ordered reply collection.
- Preserved the existing fail-closed contract: queued deltas are released only after
  every reply is received and has the expected integer type. Partial append, timeout,
  Redis error, allocation failure, or unexpected reply retains the full retry buffer.
- Kept the existing 1,024-add and 1,024-remove memory bounds and the world-capture gate.

Performance effect:

- A full flush now uses one pipelined network exchange instead of as many as 2,048
  sequential round trips. It sends the same Redis mutations and does not add any command.
- No Redis work was added to object movement. This interval reduces the worst existing
  flush latency. Later intervals fully isolated noncritical gameplay writes, donation
  reconnects, and online administrator commands from the simulation thread.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-001/RDS-007 epoch-scoped world recovery

Completed in this interval:

- Changed world generations, pointer metadata, writer leases, and the floor-delta hash to
  the `mud:season:<epoch>:` namespace. Readers use the active SQL epoch; the background
  publisher captures its epoch once when it acquires the boot lease.
- Preserved `(epoch, sequence)` identity across generation publication, validation,
  restore, previous-generation cleanup, floor handoff, status, and administrator clear
  paths.
- Added cross-epoch live Redis coverage. The old and new writers can overlap, but an old
  writer advances only its old pointer while the new epoch remains unchanged.
- Replaced multi-round-trip watched publication with one Lua compare-and-set that checks
  the exact writer token and expected prior pointer before atomically writing the
  generation, pointer metadata, floor handoff, and lease renewal.
- Made pwipe invalidation check the current captured-epoch floor delete, legacy floor and
  recovery keys, presence, all cache scans, and ship snapshot scans. Removed redundant
  unchecked cache deletion calls from that path.

Performance effect:

- Added no SQL query or network round trip to snapshot capture, publication, restore, or
  gameplay. Epoch reads are process-local and key formatting occurs only where the same
  Redis operation already existed.
- Publication remains on the background worker. The atomic script reduces its Redis
  round trips; floor batching retains the same bounds and command count.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_world_store_live.py`: passed, including overlapping
  old/new epoch writers.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_season_reset_fence.py`: passed.

Remaining related work:

- RDS-001's remaining presence, content-cache, and channel isolation was completed by the
  later all-active-surface epoch interval above.
- RDS-007 is complete: publication is scoped by durable season epoch and requires both
  the exclusive writer token and expected prior pointer in one atomic operation.

### 2026-08-28 - RDS-001 durable season reset boundary

Completed in this interval:

- Added immutable migration `0003_season_reset_state` with a singleton monotonic season
  epoch and explicit `active`/`resetting` status. The migration is additive,
  re-runnable, verified, lifecycle-inventoried, and sealed into the MySQL 8 and MariaDB
  10.11 runtime compatibility fingerprints.
- Boot now reads the singleton exactly once and refuses to start unless it contains one
  valid positive epoch in the `active` state. An interrupted destructive reset therefore
  cannot silently reopen gameplay.
- Pwipe now verifies a fresh bounded Redis administrative connection before its SQL
  boundary, locks the singleton, advances the epoch, and commits `resetting` before the
  first destructive season mutation.
- The reset records its irreversible boundary conservatively before attempting the first
  state mutation. Any failed or ambiguous result after that point keeps `_pwipe` set and
  forces process shutdown instead of clearing the flags and telling players the old
  season resumed.
- A successful reset changes the exact new epoch back to `active` only after Redis
  invalidation, SQL postconditions, and account reward policy processing have finished.

Performance effect:

- Added one indexed singleton read during database boot and no recurring query, pulse,
  player-save, or gameplay work.
- The transaction, Redis preflight connection, and completion update run only during an
  explicitly requested pwipe while persistence workers are already quiesced.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_season_reset_fence.py`: passed.
- `python3 tests/async/test_pwipe_quiescence.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_data_lifecycle_manifest.py`: passed.
- `python3 tests/async/test_immutable_migration_runner.py`: passed.
- `python3 tests/async/test_runtime_boot_compatibility.py`: passed.
- Runtime full-schema, migration-history, engine, collation, index, and column drift tests
  passed against both `mysql:8.0` and `mariadb:10.11`.

Remaining related work:

- RDS-001's remaining all-namespace and deletion-postcondition work was completed by the
  later all-active-surface epoch interval above.
- RDS-007 now has durable monotonic season identity, but world generation keys and the
  pointer transaction are not yet scoped to that identity.

### 2026-08-28 - RDS-002/RDS-003 fenced atomic world publication

Completed:

- Added a random single-writer token with a renewable 10-minute Redis lease. Lease
  acquisition occurs at boot, and game-loop snapshot requests fail fast if it was not
  acquired.
- Moved publication into a focused store adapter. Its atomic compare-and-set verifies the
  fence token and expected prior pointer, writes the generation, advances all pointer
  metadata, consumes the stable pre-capture floor hash, and renews the lease.
- Removed the later game-thread floor-hash delete and its acknowledgment state. Floor
  changes made during capture remain in bounded memory and are flushed only after the
  completed transaction, so they are not consumed with the captured generation.
- Added explicit pipeline cancellation that discards active/queued captures and
  completions, joins an in-flight publisher, and prevents new capture requests.
- Administrator and pwipe world clears now acquire/retain the writer fence, cancel and
  join before deletion, remove all generation keys, check the clear result, and keep
  publication quiesced until process shutdown.
- Added a real isolated Redis test covering lease exclusion, stale-writer rejection,
  atomic pointer/floor behavior on both sides of publication, prior-generation cleanup,
  and compare-and-delete fence release.

Performance effect:

- Removed the synchronous post-publication floor `DEL` from the simulation thread and
  removed a redundant floor flush attempt from the periodic event wrapper.
- Fence acquisition is boot-only. Publication validation and lease renewal execute as
  one Redis script on the existing background publisher, not on a game pulse.
- Normal capture remains bounded to 64 records or 2 ms per pulse. Failed boot fencing
  disables publication for that process instead of retrying connections from gameplay.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_world_store_live.py`: passed against an isolated local
  Redis server.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_boot_log_hygiene.py`: passed.
- `./scripts/format.sh --check`: passed.

Remaining related work:

- RDS-001 still needs the SQL season epoch and irreversible reset state machine. Redis
  disabled-at-reset behavior and postconditions remain unsafe until that lands.
- RDS-007 is partially remediated by the exclusive renewable lease and watched pointer
  transaction, but sequence identity is not yet scoped to a durable SQL season epoch.
- Staged UID reconciliation and duplicate rejection were subsequently completed in the
  RDS-004/RDS-005 interval above.

### 2026-08-28 - RDS-028 legacy Redis API retirement

Completed:

- Removed the obsolete `mud:next_obj_uid` read and write from server boot and shutdown. The
  SQL range allocator remains the sole UID authority.
- Removed the retired key from administrator status output so stale values cannot be
  mistaken for authoritative allocator state.
- Removed the unused generic ping, publish, and string-read exports and their
  implementations. The already-retired floor lookup exports remain absent.
- Added a source contract that prevents the legacy key and APIs from returning.

Performance effect:

- Redis-enabled boot and shutdown each perform one fewer synchronous network command.
- Administrator status performs one fewer sequential Redis read.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_legacy_api_retirement.py`: passed.
- `python3 tests/async/test_item_ownership_contract.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-010 donation subscriber hardening

Completed:

- Disabled the donation subscriber and its periodic polling job by default. Enabling it
  requires exact `REDIS_DONATION_SUBSCRIBER=TRUE` plus an independent secret of at least 32
  bytes.
- Added a versioned HMAC-SHA256 event envelope with constant-time signature comparison,
  stable event IDs, a five-minute timestamp window, and a bounded replay-ID window.
- Required integer cents, positive bounded amounts, uppercase currency, bounded text, and
  public-donor names. Rejects control bytes and the in-game color prefix before display or
  logging.
- Limited processing to eight messages per game pulse and added exponential reconnect
  backoff capped at 60 seconds.
- Documented the exact producer signature contract and the remaining at-most-once pub/sub
  delivery semantics.

Performance effect:

- The default server no longer opens a donation subscriber connection or runs its
  once-per-second polling event.
- This interval initially retained a zero-timeout game-thread socket poll with a hard
  per-pulse budget and exponential reconnect backoff. The later RDS-011/RDS-020 interval
  moved the socket and reconnect lifecycle entirely to a bounded worker.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_donation_security.py`: passed.
- `python3 tests/async/test_boot_log_hygiene.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_nevent_periodic_rearm_runtime.py`: passed under ASan/UBSan.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-013 unsafe corpse cleanup retirement

Completed:

- Replaced the obsolete cleanup implementation with a fail-closed retirement stub.
- The stub exits nonzero without loading credentials, connecting to MySQL or Redis, or
  changing either persistent store.
- Updated the operations runbook so the command is no longer presented as a usable cleanup
  procedure.
- Added behavioral coverage with probe database and cache clients to prove invocation has
  no storage side effects.

Performance effect:

- None in the running server. This change affects only an explicitly invoked offline
  operations script.

Validation:

- `shellcheck scripts/delete_corpses.sh`: passed.
- `bash -n scripts/delete_corpses.sh`: passed.
- `python3 tests/async/test_delete_corpses_retired.py`: passed.

### 2026-08-28 - RDS-009 presence privacy and encoding

Completed:

- Centralized exact `DURISWEB_PRIVATE_PRESENCE=TRUE` policy and invisible-staff filtering
  for both WebSocket and Redis presence transports.
- Default Redis presence payloads now omit account name, IP address, client name, and client
  version. Private fields and invisible staff require the explicit opt-in.
- Replaced `snprintf` JSON assembly with cJSON encoding so quotes, backslashes, and control
  characters in client-provided metadata cannot corrupt the stored payload.
- Submits an offline removal when a now-invisible character logs in and suppresses its
  login/logout publication.
- Added a compiled payload harness for public/private field selection, exact opt-in parsing,
  and JSON escaping, plus updated the DurisWeb security contract and documentation.

Performance effect:

- Normal visible login/logout keeps the existing bounded Redis command count. Invisible
  staff skip publication, and JSON encoding performs only small in-memory work at session
  boundaries rather than on the game pulse hot path.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_presence_privacy.py`: passed.
- `python3 tests/async/test_durisweb_integration_security.py`: passed.
- `python3 tests/async/test_durisweb_auth_runtime.py`: passed.
- `python3 tests/async/test_durisweb_secret_config.py`: passed.
- `python3 tests/async/test_websocket_protocol_contract.py`: passed.
- `python3 tests/async/test_runtime_connection_trust.py`: passed.
- `./scripts/format.sh --check`: passed.

The expiring per-session lease work was subsequently completed in the RDS-009 interval
above.

### 2026-08-28 - RDS-014 artifact cache safety

Completed:

- Added a versioned artifact-cache schema and a typed codec that validates view identity,
  item count, every required field, integer ranges, booleans, and bounded strings before
  rendering.
- Invalidates one malformed cache variant and falls back to a freshly generated MySQL
  result instead of reporting false emptiness or dereferencing malformed JSON.
- Renders the owned MySQL result directly on a cache miss and makes cache publication best
  effort, removing the prior second Redis read.
- Distinguishes SQL/generation failure from a valid empty artifact list.
- Added allocation checks to artifact JSON generation and a 900-second cache TTL.
- Added a compiled cJSON harness covering valid mortal/staff payloads, mismatched schema
  identity, missing fields, invalid types, invalid ranges, and malformed JSON.

Performance effect:

- Cache hits still require one bounded Redis read. Cache misses no longer re-read Redis after
  publication; the SQL result already in memory is rendered directly.
- The 900-second TTL creates an occasional SQL refresh in exchange for bounded stale-data
  lifetime.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_artifact_cache_codec.py`: passed.
- `python3 tests/async/test_artifact_guild_transactional_cutover.py`: passed.
- `python3 tests/async/test_combat_artifact_persistence.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-019 floor-delta world-recovery gate

Completed:

- Gated floor-drop capture, removal, flush, and restore on `REDIS_WORLD_STATE` instead of
  general Redis configuration.
- Removed the unused floor-pickup writer and lookup APIs, eliminating a synchronous Redis
  `SADD` from every tracked floor pickup.
- Removed the unused floor-drop lookup API and stopped periodic player checkpoint work from
  attempting floor flushes when world recovery is disabled.
- Retained explicit cleanup of the legacy pickup set and floor hash for pwipe and operator
  cleanup.
- Added a focused contract that rejects floor-delta Redis work outside world recovery.

Performance effect:

- With the default `REDIS_WORLD_STATE=FALSE`, floor gameplay no longer performs floor-delta
  Redis commands, snapshot-string allocation/copying, or retry-buffer maintenance.
- World recovery enabled behavior retains its existing bounded batching and ACK rules.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_floor_world_gate.py`: passed.
- `python3 tests/async/test_redis_failure_containment.py`: passed.
- `python3 tests/async/test_world_recovery_pipeline.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `./scripts/format.sh --check`: passed.

### 2026-08-28 - RDS-012 database backup integrity

Completed:

- Removed the Redis-dependent and legacy-flat-file mode selection. The backup script now
  always dumps the authoritative MySQL database.
- Required an explicit allow-listed database identity, safe local socket or TLS transport,
  owner-only environment file, and owner-only backup directory.
- Added `set -euo pipefail`, `MYSQL_PWD` credential handling, owner-only temporary output,
  gzip validation, required Duris schema markers, file and directory sync, and atomic rename.
- Made dump, compression, content validation, and publication failures exit nonzero without
  publishing a backup or retaining a temporary file.
- Made `cycle_mud.sh` stop before restart when its required database backup fails.
- Updated the runbook and added stubbed behavioral coverage for success, dump failure,
  malformed dump, compression failure, cleanup, and cycle failure propagation.

Validation:

- `shellcheck scripts/backup_pfiles.sh scripts/cycle_mud.sh`: passed.
- `bash -n scripts/backup_pfiles.sh scripts/cycle_mud.sh`: passed.
- `python3 tests/async/test_backup_pfiles.py`: passed.
- `python3 tests/async/test_cycle_pwipe_status.py`: passed.
- `python3 tests/async/test_start_mud_worktree_safety.py`: passed.

### 2026-08-28 - RDS-006 MySQL ship authority

Completed:

- Removed the Redis ship snapshot loader and publisher APIs and their JSON codec.
- Made `sql_load_ship()` always query MySQL and its dependent ship tables.
- Stopped publishing ship state from `sql_save_ship()`, including saves that participate in
  an outer transaction.
- Retained deletion of legacy `ship:snapshot:*` keys during ship deletion, owner rename,
  pwipe, and explicit namespace cleanup so stale snapshots cannot become active again.
- Updated focused source-contract tests to reject any Redis access in ship load/save paths.

Validation:

- `make -C src -j2`: passed with the warning-as-error profile.
- `python3 tests/async/test_redis_ship_snapshot_invalidation.py`: passed.
- `python3 tests/async/test_ship_save_guards.py`: passed.
- `python3 tests/async/test_redis_pwipe_invalidation.py`: passed.
- `python3 tests/async/test_nevent_ship_volley_runtime.py`: passed.
- `python3 tests/async/test_ship_nested_transaction.py`: passed.
- `python3 tests/async/test_ship_shutdown_txn.py`: passed.

Remaining work:

- All findings other than RDS-002, RDS-003, RDS-004, RDS-005, RDS-006, RDS-007, RDS-009,
  RDS-010, RDS-011, RDS-012, RDS-013, RDS-014, RDS-019, RDS-020, RDS-023, RDS-024, and
  RDS-028 remain open. The acceptance criteria are not yet met.

## Executive summary

The Redis implementation has several good containment mechanisms: player saves do not
depend on Redis, network calls have bounded timeouts, world capture is sliced on the game
thread, the publisher receives owned bytes instead of live pointers, and recovery blobs
have framing, age, sequence, count, and CRC checks. Those controls are valuable and should
be preserved.

The system is not yet safe to treat as a reliable recovery layer or authoritative
read-through cache. This audit found 4 critical, 10 high, 12 medium, and 2 low severity
issues. The most important risks are:

1. A season reset can report Redis invalidation success without contacting Redis, or fail
   after SQL has already been destructively changed and then resume the old process.
2. An in-flight world publisher can republish pre-wipe state after the wipe or an operator
   clear.
3. A crash between world-generation publication and floor-delta deletion can restore the
   same floor object twice.
4. World recovery's native-layout schema and aggregate floor-hash read remain portability
   and peak-memory risks even though item custody and materialization are now validated
   transactionally.
5. Ship snapshots are trusted before SQL and can be stale, uncommitted, malformed, or from
   another environment.

The safest short-term posture is to keep `REDIS_WORLD_STATE=FALSE`, avoid relying on the
current Redis-assisted pwipe path, disable the ship snapshot read path, and fix the backup
script before considering Redis recovery production-ready. The long-term fix is a small,
versioned Redis adapter with a season/environment namespace, authenticated connections,
writer fencing, typed codecs, and an explicit distinction between configured and healthy.

## Scope and method

The review traced runtime and shutdown call paths through:

- [`src/redis.c`](../../src/redis.c), [`src/redis.h`](../../src/redis.h), and
  [`src/wizredis.c`](../../src/wizredis.c)
- [`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c),
  [`src/copyover.c`](../../src/copyover.c), boot/event/shutdown code, and SQL item custody
- ship, artifact, fraglist, presence, and donation consumers
- `.env.example`, operational documentation, lifecycle policy, migrations, and scripts
- focused Redis, recovery, pwipe, ship, lifecycle, and documentation tests

Validation performed:

- `make -C src -j2`: passed with the repository warning-as-error profile.
- 11 focused Python test programs: all passed.
- `world_recovery_validate` runtime harness: passed its structural cases.
- `__NO_MYSQL__` syntax probe: failed, including unguarded MySQL and hiredis types.
- ShellCheck on Redis-facing operational scripts: failed and exposed the backup pipeline
  issue described below.

No game server was started. No configured MySQL or Redis service was contacted. No
migration, wipe, backup, clear, or corpse-cleanup script was executed.

## Severity rubric

| Severity | Meaning in this report |
| --- | --- |
| Critical | A reachable path can cause cross-season resurrection, durable duplication, destructive partial reset, or equivalent integrity failure. |
| High | A credible path can corrupt authoritative behavior, expose private data, crash the server, or cause sustained availability failure. |
| Medium | Material correctness, performance, operability, lifecycle, or maintainability weakness without the same immediate blast radius. |
| Low | Confirmed cleanup, build-option, documentation, or dead-code issue with limited direct runtime impact. |

## System map

| Area | Redis keys/channels | Intended role | Current authority behavior |
| --- | --- | --- | --- |
| World recovery | `<namespace>:season:<epoch>:world_state:generation:<seq>`, current pointer, diagnostic metadata | Optional recent world reconstruction | An independent HMAC authenticates the namespace/season/sequence-bound manifest and complete payload before schema, semantic graph, complete item SQL custody, and rollback-capable materialization checks; NPC-held items are intentionally omitted. |
| Floor deltas | `<namespace>:season:<epoch>:floor_drops`; retired `mud:floor_pickups` cleanup | Bridge changes around a world snapshot | Versioned bounded binary item trees join the generation plan and complete SQL authority check before any materialization. |
| Ship cache | `ship:snapshot:<owner>` | Reconstructible SQL cache | Cache is returned before SQL without TTL, row revision, or schema/environment identity. |
| Content caches | `<namespace>:season:<epoch>:cache:named`, fraglist, epic-zone, and artifact variants | Reconstructible command output | Player reads use bounded local memory, Redis publication is asynchronous, every entry has an explicit TTL, artifact and fraglist payloads are versioned, and countdown rendering uses a stored absolute deadline. |
| Presence | `<namespace>:season:<epoch>:presence:current`, expiring session keys, player channel, and retry tokens | Web presence and login/logout events | Privacy-safe payloads use a fenced 180-second per-session lease refreshed only by the bounded worker. |
| Donation integration | `<namespace>:season:<epoch>:nchat` pub/sub | Broadcast external donation notices | A bounded worker accepts only authenticated, fresh, replay-protected envelopes and delivers at most eight events per game pulse. |
| Legacy UID | `mud:next_obj_uid` | Retired counter | No runtime read, write, or administrator display remains. |

All runtime connections share bounded ACL/password, explicit database, verified TCP TLS,
and optional local Unix-socket settings. Every active key and channel uses a required, boot-validated
`duris:<environment>:<deployment>` namespace followed by the captured SQL season epoch.

## Finding index

| ID | Severity | Finding |
| --- | --- | --- |
| RDS-001 | Critical | Season reset invalidation is non-atomic and has unsafe success and failure semantics. |
| RDS-002 | Critical | In-flight writers can republish state after a wipe or operator clear. |
| RDS-003 | Critical | World publication and floor-delta cleanup have a deterministic duplication window. |
| RDS-004 | Critical | Descendant item restoration violates complete SQL custody and identity rules. |
| RDS-005 | High | Recovery mutates the live world without staging, rollback, or exact success accounting. |
| RDS-006 | High | Ship snapshots can override MySQL with stale, uncommitted, or unsafe data. |
| RDS-007 | High | World sequence publication has no compare-and-swap, lease, or instance fencing. |
| RDS-008 | High | Redis lacks authentication, TLS, database selection, and namespace isolation. |
| RDS-009 | High | Redis presence bypasses the documented privacy policy and emits unsafe JSON. |
| RDS-010 | High | Donation pub/sub lacks authenticity, input limits, work budgets, and reconnect backoff. |
| RDS-011 | High | Synchronous calls run on the simulation thread and generic Redis outages are sticky. |
| RDS-012 | High | Automatic database backup selection and success detection are unreliable. |
| RDS-013 | High | The corpse cleanup script targets a retired world format and can over-delete floor data. |
| RDS-014 | High | Artifact cache failure can report false emptiness, while malformed cache data can crash. |
| RDS-015 | Medium | Existing tests are dominated by source-contract assertions rather than live Redis behavior. |
| RDS-016 | Medium | World recovery uses native C/C++ struct layout as its durable wire format. |
| RDS-017 | Medium | Read and publication memory bounds do not match the documented 64 MiB generation bound. |
| RDS-018 | Medium | Incremental world capture is a fuzzy, cross-time snapshot. |
| RDS-019 | Medium | Floor tracking remains active when world recovery is disabled and includes unused, unbounded data. |
| RDS-020 | Medium | Administrator status and clear commands can misreport success, omit stores, and stall the game. |
| RDS-021 | Medium | Content caches lack a coherent version, freshness, and invalidation contract. |
| RDS-022 | Medium | The lifecycle inventory omits most Redis stores and cannot discover that omission. |
| RDS-023 | Medium | Migration and clear scripts use broad `FLUSHDB` semantics without full connection safety. |
| RDS-024 | Medium | Fresh snapshots survive graceful shutdown and trigger a path documented as crash-only. |
| RDS-025 | Medium | Redis health and failure observability are too coarse for incident diagnosis. |
| RDS-026 | Medium | Redis responsibilities are concentrated in a monolith with unrelated persistence coupling. |
| RDS-027 | Low | The advertised no-MySQL build toggle is broken and hiredis remains a mandatory build dependency. |
| RDS-028 | Low | Retired keys and unused public APIs remain in the operational surface. |

## Detailed findings

### RDS-001 - Season reset invalidation is non-atomic and unsafe

Severity: Critical
Confidence: Confirmed from reachable code paths
Remediation status: Completed on branch. Pwipe uses a durable irreversible SQL reset
fence, all active Redis surfaces use the boot-captured monotonic season epoch, writers are
quiesced before invalidation, and every enabled-Redis deletion has an explicit absence
postcondition. Disabled Redis is safe because a later season cannot address old keys.

Evidence:

The audit baseline allowed null-context deletion to report success, ran Redis invalidation
after destructive SQL without an irreversible process fence, and used unscoped presence,
cache, and pub/sub names that a later season could address.

The current reset preflights a fresh bounded Redis connection, commits an incremented
`resetting` SQL epoch before the first destructive mutation, and forces shutdown on every
later ambiguous result. Redis workers are stopped first. The old process retains its
captured old epoch while deleting and verifying that exact namespace; a new process can
start only after SQL marks the incremented epoch `active`. Active registry surfaces all
contain `<epoch>`, while former unscoped names are cleanup-only.

Impact:

Old Redis values may remain only when Redis was explicitly disabled or unreachable, but
they are unreachable from the next epoch and cannot reappear. A failure after the SQL
boundary cannot resume gameplay: the process stops and boot remains fenced in
`resetting` for operator recovery.

Recommendation implemented: preserve the irreversible state machine, boot-captured epoch,
writer quiescence, exact postconditions, and all-active-surface registry test as mandatory
reset invariants.

### RDS-002 - In-flight writers can republish cleared state

Severity: Critical
Confidence: Confirmed
Remediation status: Completed on branch; destructive world clears cancel and join the
publisher before deletion and retain an exclusive fence until shutdown, while every
publication transaction rejects a stale writer token.

Evidence:

- Pwipe deliberately skips the normal 3-second world-recovery drain
  ([`src/comm.c`](../../src/comm.c#L1841)).
- After `sql_pwipe()` clears Redis, normal process cleanup still calls
  [`redis_cleanup()`](../../src/redis.c#L464), which drains an initialized world pipeline
  for up to 30 seconds and lets the publisher finish.
- The publisher opens its own connection and does not check `_pwipe`
  ([`src/redis.c`](../../src/redis.c#L108)). It can therefore publish a capture created
  before the wipe after Redis was cleared.
- `redis clear world` and `redis clear all` also delete keys without quiescing or fencing
  the active publisher ([`src/wizredis.c`](../../src/wizredis.c#L269)).

Impact:

Pre-wipe world state can be recreated after a successful clear. Operator clears are also
not durable: a queued generation can reappear seconds later.

Recommendation:

Add explicit `quiesce`, `cancel`, and `join` lifecycle operations. A destructive clear
must prevent new captures, discard queued pre-boundary generations, join the publisher,
and then clear. The Redis publication transaction must also compare a captured season
epoch and writer-fence token so that a stale worker cannot publish even if process-side
coordination fails.

### RDS-003 - Snapshot and floor-delta handoff can duplicate objects

Severity: Critical
Confidence: Confirmed failure window
Remediation status: Completed on branch for the deterministic handoff window; generation
publication and consumption of the stable pre-capture floor hash are now one watched
transaction, verified on both success and rejected-publication paths against live Redis.

Evidence:

- Floor deltas are flushed before capture. After the generation is published and its
  completion is consumed, the main thread separately deletes `mud:floor_drops`
  ([`src/redis.c`](../../src/redis.c#L1134)). The delete is not part of the pointer
  `MULTI`/`EXEC` transaction.
- A crash after pointer publication but before that delete leaves the same pre-boundary
  floor objects in both the current generation and the floor hash.
- Boot restores the floor hash before it fetches or validates the current generation
  ([`src/redis.c`](../../src/redis.c#L1284)), then restores the world generation.
- Both paths accept the same root UID when SQL still says it belongs to that room. There
  is no in-memory UID deduplication between the two restores.
- The comment at [`src/redis.c`](../../src/redis.c#L1012) says the world restore needs the
  floor hash, but the world restore no longer consults it. The current regression test
  explicitly asserts that the old Redis floor checks are absent.

Impact:

One narrow but ordinary crash timing can instantiate two roots with the same UID, plus
duplicated contents. Duplicate live identities undermine later ownership, movement, save,
and extraction logic.

Recommendation:

Use generation-scoped delta keys and atomically consume the pre-capture delta generation
in the same transaction that advances `world_state:current`. Independently, build an
in-memory set of planned UIDs during staged restore and reject duplicate roots or
descendants before materialization. Add kill-point integration tests immediately before
and after pointer `EXEC` and delta deletion.

### RDS-004 - Descendant restoration violates custody and identity

Severity: Critical
Confidence: Confirmed
Remediation status: Completed on branch; schema 9 and `WRF3` carry complete bounded item
trees, every node is reconciled against SQL before creation, runtime custody is hydrated
atomically, and NPC-held items are explicitly omitted instead of recreated by VNUM.

Evidence:

- Floor-delta JSON stores at most 16 immediate child VNUMs, without child UIDs, state,
  affects, ownership, or nesting ([`src/redis.c`](../../src/redis.c#L612)). Restore calls
  `read_object()` for each VNUM, assigning a new SQL-reserved UID
  ([`src/redis.c`](../../src/redis.c#L983)).
- World records preserve immediate child UIDs, but
  [`world_recovery_restore()`](../../src/world_recovery_pipeline.c#L588) validates only
  the top-level root against SQL. `copyover_restore_obj_from_buffer()` then creates every
  child and applies its saved UID without checking owner, parent, state, or VNUM against
  SQL ([`src/copyover.c`](../../src/copyover.c#L1784)).
- The root SQL check hydrates only that one item. Normal movement requires every descendant
  to have matching runtime custody, root, parent, VNUM, and active state
  ([`src/item_movement_transaction.c`](../../src/item_movement_transaction.c#L62)).
- NPC equipment is serialized by VNUM only. Carried items include a UID, but restore
  ignores it and creates a fresh item ([`src/copyover.c`](../../src/copyover.c#L1735)).

Impact:

- A child moved away after capture can be recreated inside its old container.
- A floor-delta child receives a new UID and can be adopted as a new authoritative item,
  duplicating the value represented by the stale snapshot.
- A world-snapshot child with an old UID may collide with SQL or remain unusable because
  its runtime custody was never hydrated.
- NPC inventory and equipment can be duplicated or lose identity entirely.

Recommendation:

Serialize a complete, bounded item tree with fixed-width UID, VNUM, parent UID, root UID,
state, and required mutable fields. Batch-read every referenced UID from SQL and validate
the complete graph before creating any object. Hydrate the full verified graph atomically.
Define an explicit policy for non-authoritative NPC equipment; do not recreate valuable
items by VNUM alone.

### RDS-005 - Restore is not transactional or exactly accounted

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; restore now performs structural decode, semantic
planning, exact SQL authority reconciliation, rollback-tracked entity creation, atomic
runtime hydration, and only then door/zone application. Failure preserves Redis evidence
and performs a full normal zone boot.

Evidence:

- Structural validation runs first, but restore then mutates the live world record by
  record. A later invalid door or zone returns false without removing mobs or objects
  already created ([`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c#L588)).
- The return values from mob and object restore helpers are ignored. Missing VNUMs or
  rooms can consume the full record, count as restored, and still produce overall success.
- Floor deltas are materialized before the generation pointer or blob is even fetched
  ([`src/redis.c`](../../src/redis.c#L1298)).
- Boot skips initial zone resets after the earlier `redis_has_world_state()` check. If
  restore later fails, it logs that normal boot state will be used, but the initial reset
  already did not run and partial recovery objects are not rolled back
  ([`src/comm.c`](../../src/comm.c#L951),
  [`src/new_events.c`](../../src/new_events.c#L1917)).

Impact:

A structurally valid but semantically unusable generation can yield a partial or mixed
world. The snapshot is then cleared, reducing the evidence available for diagnosis or a
controlled retry.

Recommendation:

Split recovery into `decode -> semantic plan -> authority reconciliation -> materialize`.
The planning phases must not mutate the world. Require exact planned/applied/skipped
counts, and treat an unexpected skip as failure. Materialize through a rollback-capable
arena or explicitly track and remove every created entity before falling back to a full
normal zone boot.

### RDS-006 - Ship snapshots override SQL with unsafe state

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; MySQL is now the only ship read authority.

Evidence:

- [`sql_load_ship()`](../../src/sql_player.c#L10429) returns a Redis snapshot before
  querying the SQL row. The key has no TTL, schema version, SQL revision, database
  identity, or season identity.
- Cache write and invalidation outcomes are ignored. A Redis outage during save leaves an
  older key that can become readable again later. SQL delete similarly returns success
  even if its cache invalidation did nothing ([`src/sql_player.c`](../../src/sql_player.c#L10551)).
- `sql_save_ship()` joins an outer transaction but always writes the cache before that
  outer owner commits ([`src/sql_player.c`](../../src/sql_player.c#L10196)).
  `shutdown_ships()` wraps all ships in such a transaction
  ([`src/ships/ship_base.c`](../../src/ships/ship_base.c#L210)). A later rollback leaves
  uncommitted Redis snapshots.
- Deserialization checks only that `m_class >= 0`. `new_ship(m_class)` indexes
  `ship_type_data[m_class]`, whose length is `MAXSHIPCLASS`, without an upper-bound check
  ([`src/redis.c`](../../src/redis.c#L1694),
  [`src/ships/ship_base.c`](../../src/ships/ship_base.c#L281)). Other numeric fields are
  also accepted without domain bounds.

Impact:

Redis can resurrect stale ship state, expose state that rolled back in SQL, or cause an
out-of-bounds read and server crash from malformed cache data.

Recommendation:

Disable the Redis ship read path until the cache is revisioned. At minimum, query SQL for
existence and row revision, require an exact revision match, validate every domain field,
and use a versioned codec. Publish only after the owning transaction commits, preferably
through an outbox. Add a short TTL and treat every cache error as a miss, never as
authority.

### RDS-007 - Publication has no fencing or compare-and-swap

Severity: High
Confidence: Confirmed; multi-instance impact is conditional on concurrent writers
Remediation status: Completed; a renewable exclusive writer lease and one atomic Lua
compare-and-set require the exact writer token and expected prior pointer. All world
recovery identities and keys use the durable SQL season epoch, so a stale process cannot
publish into the active season.

Evidence:

- Sequences are process-local. The pipeline reads `world_state:current` only when it is
  initialized and the main context already exists
  ([`src/redis.c`](../../src/redis.c#L202)).
- `redis_save_world_state()` initializes the pipeline before it reconnects the main
  context ([`src/redis.c`](../../src/redis.c#L1108)). If Redis was down at initialization
  and returns afterward, sequence numbering can restart at 1 despite durable generations.
- The publisher reads the prior pointer, writes its generation, and unconditionally sets
  the new pointer. It does not use `WATCH`, Lua compare-and-set, a lease, or a writer token
  ([`src/redis.c`](../../src/redis.c#L139)).

Impact:

A late recovery from startup outage can overwrite or rewind sequence state. Two server
instances, a stale process, or overlapping deployment can race, overwrite the same
generation key, move the pointer backward, and delete a generation another writer needs.

Recommendation:

Allocate sequences in Redis or SQL under the current season epoch. Advance the pointer
with one Lua script that requires the expected epoch, prior sequence, and writer-fence
token. Give each generation a globally unique writer/sequence identity. Explicitly
support one writer or enforce that invariant with a renewable lease.

### RDS-008 - Redis has no connection or namespace security

Severity: High
Confidence: Confirmed configuration gap; exploitability depends on deployment reachability
Remediation status: Completed on branch. Runtime and destructive-maintenance connections
support ACL/password authentication, explicit database selection, verified TCP TLS, and
bounded Unix-socket transport. Non-loopback production runtime endpoints fail closed without
TLS, while destructive maintenance requires an exact local target allow-list and
confirmation. Every active key and channel uses a required
application/environment/deployment namespace plus the SQL season epoch. World generation
manifests and payloads use an independent rotating HMAC secret. Production requires
distinct world, presence, cache, maintenance, and enabled-donation ACL identities.

Evidence:

- Every runtime owner connects through the shared adapter
  ([`src/redis_connection.c`](../../src/redis_connection.c)), which performs bounded TCP,
  optional verified TLS, ACL/password authentication, and explicit database selection.
- Runtime and maintenance configuration share host, port, database, TLS, CA, and an exact
  `duris:<environment>:<deployment>` namespace while using distinct subsystem credentials.
  Runtime validation rejects a namespace whose environment differs from `ENVIRONMENT`
  before connecting.
- TCP and absolute Unix-socket endpoints are mutually exclusive and use the same bounded
  authenticated, database-selected connection adapter. TLS is required for non-loopback
  production TCP and rejected for local socket transport.
- Recovery generations use a `WRG2` manifest whose constant-time HMAC verification binds
  the full payload digest to namespace, season, and sequence before materialization.
- Live ACL coverage proves that subsystem identities cannot cross key or channel
  boundaries and that maintenance cannot address an unrelated application prefix.

Impact:

A compromised subsystem identity is contained to that subsystem's scoped key or channel
surface. Donation envelopes and world generations additionally require independent HMAC
keys, while namespace and season binding prevent cross-deployment and cross-season
acceptance. The maintenance identity remains intentionally privileged and must be reserved
for stopped-server or pwipe workflows.

Recommendation:

Support ACL username/password, verified TLS, explicit database, Unix sockets, and an
application/environment/season prefix. Refuse a non-loopback production endpoint without
TLS. Apply least-privilege ACLs per subsystem and channel. Sign recovery generations and
external donation envelopes with independent keys so Redis access alone is insufficient.

### RDS-009 - Presence bypasses the documented privacy policy

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; privacy/visibility policy and JSON encoding are
centralized, while each presence entry now has a fenced 180-second lease refreshed by the
background worker and expires after process failure or missed logout.

Evidence:

- [`redis_player_online()`](../../src/redis.c#L2327) always writes character name,
  account, IP, client name, client version, and login time. It does not inspect
  `DURISWEB_PRIVATE_PRESENCE` and does not exclude invisible staff.
- The WebSocket implementation does enforce both controls
  ([`src/ws_handlers.c`](../../src/ws_handlers.c#L389)), and the configuration reference
  promises that the default feed omits these values
  ([`docs/operations/CONFIGURATION.md`](../operations/CONFIGURATION.md#L125)).
- Redis JSON is assembled with `snprintf` rather than cJSON. Client name and version come
  from client-supplied GMCP strings and may contain quotes, backslashes, or control
  characters ([`src/gmcp.c`](../../src/gmcp.c#L87)).
- `mud:online` has no per-entry TTL. Failed logout or boot cleanup leaves stale private
  data.

Impact:

The Redis feed discloses data the documented opt-in says is private and can reveal staff
who intentionally used invisibility. Client-controlled text can also make the stored JSON
invalid.

Recommendation:

Use the same central presence-policy helper for WebSocket and Redis. Build JSON with
cJSON, omit private fields unless exact opt-in is active, and skip invisible staff by
default. Use expiring per-session keys or a heartbeat/lease rather than a persistent hash.

### RDS-010 - Donation pub/sub is unauthenticated and unbounded

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; the subscriber is explicitly gated and requires
authenticated, bounded, fresh, replay-protected events. A dedicated bounded worker owns
connect, subscribe, validation, replay filtering, and reconnect backoff; the game pulse
only dequeues at most eight fixed-size validated events.

Evidence:

- Donation polling is enabled whenever general Redis is enabled and runs every second
  ([`src/new_events.c`](../../src/new_events.c#L1994)). There is no separate feature flag.
- On disconnection, every poll immediately retries a bounded 250 ms connection with no
  exponential backoff or jitter ([`src/redis.c`](../../src/redis.c#L1959)).
- The payload needs only `type=donation`. Amount, currency, character, public flag, and
  message have no required-field, finite-number, sign, size, character-set, or replay
  validation ([`src/redis.c`](../../src/redis.c#L1890)).
- Text is placed directly into colorized game output and logs. Newlines and in-game color
  controls are not escaped. All replies already buffered by hiredis are drained in an
  unbounded loop.

Impact:

A Redis publisher can impersonate donors, inject misleading in-game/log text, and send a
burst that monopolizes a game pulse. An outage can impose a 250 ms hitch every second.

Recommendation:

Add an explicit donation feature flag, signed and replay-protected message envelope,
bounded strings and amount/currency validation, display escaping, per-pulse message/time
budgets, and exponential reconnect backoff. If these messages represent payment events,
use a durable stream with stable event IDs rather than at-most-once pub/sub.

### RDS-011 - Simulation-thread I/O and sticky failures

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch. Presence, report cache, floor delta, world
publication, and donation network work use bounded workers. Donation reconnect/subscribe
is worker-owned, online administrator status is local-only, cache clears enqueue bounded
invalidation, and broad recovery clears are refused online. Shared synchronous commands
remain only in boot, recovery materialization, shutdown, pwipe, and stopped maintenance.

Evidence:

- The main hiredis context is synchronous with a 100 ms command timeout. Floor pickups,
  logins, cache reads/writes/invalidations, presence, and administrator commands call it
  from game-thread paths.
- A player login performs a hash write and publish. Each floor pickup performs an
  immediate `SADD` even when world recovery is disabled
  ([`src/redis.c`](../../src/redis.c#L547)). Detailed status performs many sequential
  calls.
- Generic cache, presence, and helper methods return immediately when the context is null
  or errored and never reconnect. Only world-recovery paths invoke `redis_reconnect()`.
  With the default `REDIS_WORLD_STATE=FALSE`, an initial outage or first I/O error can
  disable the main integration until process restart.
- `redis_enabled` means configured, not available. Callers use it as both concepts.

Impact:

Redis latency can become game latency. Failure handling is inconsistent: donation retries
too aggressively, world recovery can heal, and ordinary caches/presence remain dead.

Recommendation:

Introduce one adapter state machine with `disabled`, `connecting`, `healthy`, `backoff`,
and `degraded` states. Queue noncritical writes to a bounded worker, pipeline batches, and
make reads fail fast through a circuit breaker. Expose configuration and live health as
separate APIs. Set and test a total per-pulse Redis budget.

### RDS-012 - Automatic backups are not reliably database backups

Severity: High
Confidence: Confirmed shell behavior
Remediation status: Completed on branch; backups are MySQL-only, validated, atomic, and
fail closed before restart.

Evidence:

- [`scripts/backup_pfiles.sh`](../../scripts/backup_pfiles.sh#L20) selects `mysqldump`
  only when `REDIS` is exactly lowercase `true` or `1`. The server accepts case-insensitive
  `TRUE`, and `.env.example` uses uppercase `TRUE`. The default documented configuration
  therefore selects the legacy flat-file branch even though MySQL is authoritative.
- Database backup mode is incorrectly coupled to Redis enablement.
- `mysqldump | gzip` runs without `set -o pipefail`, and `$?` checks `gzip`, not
  `mysqldump`. A failed dump can produce a valid empty gzip and print `Backup complete`.
- The script returns success after either the success or failure message. The cycle script
  invokes it automatically and does not check its result
  ([`scripts/cycle_mud.sh`](../../scripts/cycle_mud.sh#L182)).

Impact:

Operators can believe they have current database backups when only legacy files, or an
empty compressed dump, exist.

Recommendation:

Select backup mode from the actual database architecture, not Redis. Use `set -euo
pipefail`, write to an owner-only temporary file, verify `mysqldump` and gzip status,
validate the dump contains expected schema/data, fsync and atomically rename it, and exit
nonzero on any failure. Make the cycle stop or enter a clearly configured degraded mode
when its required backup fails.

### RDS-013 - Corpse cleanup operates on a retired Redis representation

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; the obsolete cleanup implementation is retired
and now exits nonzero without connecting to or changing either persistent store.

Evidence:

- [`scripts/delete_corpses.sh`](../../scripts/delete_corpses.sh#L67) reads and rewrites a
  legacy JSON key named `mud:world_state`. Current recovery uses binary
  `mud:world_state:generation:<sequence>` plus `mud:world_state:current`.
- Its `redis-cli` calls use the default endpoint and database, ignoring `REDIS_HOST` and
  `REDIS_PORT` loaded from `.env`.
- If a cleanup proceeds, it deletes the entire `mud:floor_drops` hash because it "may
  contain corpses" ([`scripts/delete_corpses.sh`](../../scripts/delete_corpses.sh#L168)).
  That hash can contain every kind of floor object.

Impact:

The script does not remove corpses from the active generation format and can remove
unrelated recovery data from the wrong Redis instance. The runbook nevertheless describes
it as purging Redis corpse state.

Recommendation:

Retire the script until it can decode the current versioned format offline. A replacement
should stop and fence the server, connect with the full safe Redis configuration, identify
exact authoritative corpse UIDs, preserve non-corpse deltas, create a verified backup, and
postflight both SQL and Redis.

### RDS-014 - Artifact cache errors are unsafe

Severity: High
Confidence: Confirmed
Remediation status: Completed on branch; cache payloads are versioned and validated, and
every miss/error/malformed payload falls back to the owned MySQL result.

Evidence:

- When Redis is configured but unavailable, `list_artifacts_sql()` generates fresh JSON
  from SQL, tries to cache it, then tries Redis again. If the cache write failed, it tells
  the player `No artifacts found` instead of rendering the generated result
  ([`src/artifact.c`](../../src/artifact.c#L365)).
- A parseable but malformed cached array is dereferenced without checking required field
  existence or type. Missing `vnum`, `locType`, `shortDesc`, or `racewar` can dereference
  null or a null `valuestring` ([`src/artifact.c`](../../src/artifact.c#L435)).
- Artifact keys have no TTL or schema version.

Impact:

A Redis outage produces false gameplay information. Corrupt, stale, cross-environment,
or malicious cache data can crash the game process.

Recommendation:

Generate once and return the owned result directly while asynchronously attempting the
cache write. Validate a versioned typed schema field by field; delete and ignore malformed
entries. Treat all cache failures as misses and preserve the SQL result.

### RDS-015 - Redis tests are mostly source contracts

Severity: Medium
Confidence: Confirmed by test inspection and execution
Remediation status: Completed on branch. Ephemeral Redis tests cover TCP, TLS, Unix
socket, authentication, selected database, subsystem ACL isolation, scoped destructive
maintenance, cache/presence/donation outage healing, floor barriers, world fencing,
authenticated chunked generations, atomic floor handoff, command timeout, and
committed-but-lost `EVAL`/`EXEC` replies. Runtime codec tests cover privacy escaping,
donation authentication/replay/bounds, and recovery framing. The backup script is exercised
with successful and failing dump/compression stubs. A compiled ASan/UBSan harness now
blocks and joins an in-flight publisher, runs real recovery planning/materialization
against fake world and SQL custody adapters, rejects duplicate/moved item graphs, and
proves rollback after atomic custody hydration failure. Pwipe quiescence precedes the
maintenance connection attempt, while isolated Redis tests verify connection failures and
scoped deletion/postflight behavior.

At the audit baseline, most Redis assertions searched source text for helper names,
ordering, and forbidden tokens. The world harness exercised framing but did not
materialize a world or use a Redis server. The original gaps were:

- initial outage and healing, command timeout, or uncertain `EXEC` outcomes
- pwipe with a null context, a real invalidation failure, or an in-flight publisher
- crash points between generation write, pointer `EXEC`, completion, and floor delete
- duplicate or moved descendants, NPC inventory, semantic restore failure, or rollback
- ship outer-transaction rollback, missed invalidation, malformed JSON, or stale revision
- presence privacy and escaping, donation authenticity/bursts/backoff, or oversized values
- script behavior against stubbed failing `mysqldump` and isolated Redis endpoints

The original transport, worker, atomic-publication, pwipe, recovery-materialization,
privacy/authenticity, and backup gaps are now covered or obsolete because Redis ship read
authority and process-child persistence were retired. Source inspection remains only for
static invariants such as the absence of gameplay-thread Redis calls and deliberate
omission of NPC-carried inventory from recovery records.

Recommendation: add ephemeral Redis integration tests on a random Unix socket or isolated
port, plus small materialization harnesses with fake world/SQL custody adapters. Keep
source-contract tests only for invariants they can actually prove.

### RDS-016 - The recovery format is process-native

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. Schema 9 defines a 64-byte header and versioned
record frames with fixed-width little-endian fields. Native structs remain in-process
capture objects only; background publication encodes generations and floor records, and
boot decodes field by field before semantic and SQL-authority validation.

The baseline copied headers and records directly from native structs containing `time_t`,
`unsigned long`, compiler padding, and host-endian integers. The current codec assigns an
exact width and byte order to every durable field, requires record version 1 and zeroed
reserved bytes, bounds every record before decode, and rejects schema 8 rather than
attempting an unsafe ABI-dependent compatibility read.

Golden vectors cover exact encoded bytes independently from native layout. Round-trip
tests cover signed values, 64-bit identities and timers, mobile affects, complete object
trees, doors, and floor records. The format and compatibility policy are documented in
[`WORLD_RECOVERY_FORMAT.md`](../persistence/WORLD_RECOVERY_FORMAT.md).

Recommendation: use an explicit little-endian codec with fixed-width integers, bounded
lengths, and per-record versions. Add golden vectors and previous-version compatibility
tests. Keep the in-process structs separate from the durable format.

### RDS-017 - Memory and size bounds are incomplete

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. Generation storage is chunked into a small
manifest plus at most 64 one-MiB values, every read is length-checked before transfer,
floor recovery uses a transactionally maintained UID index and 64-record pages, and boot
enforces explicit generation, floor, record-count, and combined payload ceilings.

The audit baseline performed a complete `GET` and a monolithic publication command for a
possible 64 MiB generation, so Hiredis could allocate generation-sized reply and command
buffers. It also restored the entire floor hash with `HGETALL`, had no floor aggregate or
record-count ceiling, and could leave orphan generation keys indefinitely.

The current publisher writes at most 1 MiB per Redis command on its worker thread and
atomically publishes only the fixed-size manifest. Boot accepts only exact manifest and
chunk lengths. Floor writes update the hash and sorted-set index in bounded worker
transactions; boot verifies index/hash counts and reads 64 fields per page. The accepted
application payload is at most 64 MiB combined, with at most 16 MiB and 32,768 records
from the floor bridge. All generation artifacts expire, and successful replacement or
consumption removes the known prior/current artifacts in bounded commands.

Recommendation: check `STRLEN` before `GET`, reject over-limit data, use a chunked or
streamed format, and document a true peak-memory budget. Use a publication-specific
deadline based on measured local throughput. Attach a conservative TTL and perform
bounded epoch-aware orphan collection.

### RDS-018 - Capture is not point-in-time

Severity: Medium
Confidence: Confirmed design property
Remediation status: Completed on branch by adopting the recommended explicit bounded
fuzzy-snapshot contract. Capture start remains the conservative timestamp, captures expire
after five minutes and never publish when expired, economic/persisted-item authority is
excluded from fuzzy state, and failure safely resumes floor publication for later retry.

Capture walks NPCs, room objects, doors, and zone ages over many pulses with a 64-record
and 2 ms per-pulse budget ([`src/world_recovery_pipeline.c`](../../src/world_recovery_pipeline.c#L395)).
Entities and doors can move or change after their record is captured but before the
generation is sealed. The timestamp records capture start, not a consistent boundary.

Impact: one generation can combine states that never coexisted. SQL root reconciliation
limits some stale item restores, but it does not make mob, door, zone, container, or floor
state point-in-time.

Recommendation: explicitly call this a fuzzy recovery snapshot and define acceptable
invariants. For invariants that must be exact, capture stable IDs and mutation revisions,
then retry or exclude a record when its revision changes before sealing. Consider a short
game-thread finalization pass that verifies captured revisions.

### RDS-019 - Floor tracking runs when recovery is off

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch; disabled world recovery now performs no floor-delta
capture, write, removal, lookup, restore, or periodic flush work.

`.env.example` enables general Redis but disables world recovery. Floor drop buffering and
the immediate floor-pickup `SADD` check only general Redis, while floor restore occurs only
inside world recovery. `redis_check_floor_pickup()` and `redis_check_floor_drop()` have no
runtime callers. `mud:floor_pickups` has no TTL and is cleared only by successful recovery,
pwipe, or an administrator.

Impact: the default copied configuration performs synchronous writes and accumulates a
set that provides no recovery benefit. Floor hashes can also persist indefinitely when no
generation ACK clears them.

Recommendation: gate the entire floor subsystem on `redis_world_state_enabled`, or remove
it when recovery is off. Delete the unused pickup set or make deltas generation-scoped and
expiring. Add explicit caps and overflow counters to both addition and removal buffers.

### RDS-020 - Administrator Redis commands are unreliable

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. Status reports bounded local typed worker/pipeline
state without Redis queries. Cache clear responses distinguish accepted, rejected, and
partial background submission, while world/floor/all clears are refused online rather
than running scans or reporting unchecked success.

- Status helpers map unavailable/error to `0`, `false`, or `-1`, which is also a valid
  empty, missing, or persistent-key result ([`src/redis.c`](../../src/redis.c#L2423)).
- `redis clear world`, `floor`, and individual caches always print `Cleared` even when no
  context exists or a command failed ([`src/wizredis.c`](../../src/wizredis.c#L209)).
- `clear all` omits ship snapshots and online presence, despite its name. It also does not
  clear in-memory floor batches.
- Detailed status performs many sequential synchronous commands on the game thread.
- As described in RDS-002, clear does not fence active writers.

Recommendation: return typed results (`ok`, `unavailable`, `partial`, `failed`), show live
connection health, and report exact postconditions. Drive status and clear from a complete
key registry. Run broad scans/clears outside the simulation thread, with confirmation,
writer fencing, and a bounded deadline.

### RDS-021 - Cache freshness is inconsistent

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. Runtime report reads are bounded local-memory
lookups, Redis writes and invalidations are asynchronous and coalesced, and every active
content cache has an explicit TTL. Artifact JSON and fraglist frames are versioned and
validated; fraglist countdown presentation is rendered from an absolute deadline on each
local hit.

Named, fraglist, artifact, and ship caches are persistent and unversioned; only epic zones
has a 900-second TTL. Invalidation is distributed across unrelated gameplay files.
Fraglist caches an already-rendered countdown derived from `next_update`, so the displayed
timer freezes until another invalidation. The maintenance level-cap completion path does
not invalidate that cache ([`src/comm.c`](../../src/comm.c#L137),
[`src/redis.c`](../../src/redis.c#L2137)).

The current named cache has a 24-hour backstop; fraglist, epic-zone, and artifact caches
have 15-minute backstops. Combat-outcome and level-cap commits invalidate fraglist state.
The `FRC1` payload stores stable content, generation time, source-content revision, and the
absolute cap deadline; malformed, stale, future, or revision-mismatched frames are misses.
Local rendering advances the timer without external I/O, and regeneration no longer makes
the redundant second level-cap query.

Recommendation: cache stable data, not time-relative presentation. Put schema version,
source revision, generated time, and bounded TTL in every cache contract. Centralize
invalidation after authoritative commit, or prefer revision-keyed reads so missed deletes
cannot make old data current.

### RDS-022 - Lifecycle inventory misses Redis stores

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. One declarative runtime registry owns all active
and cleanup-only Redis surfaces, runtime code consumes its constants, destructive
maintenance patterns are checked against it, and lifecycle validation derives Redis store
coverage and exact locators from it.

The audit baseline lifecycle manifest had only `redis:world_recovery`, while runtime keys
were duplicated as string literals. The registry now declares 42 surfaces mapped to five
lifecycle stores: world/floor recovery, presence, content caches, donation pub/sub, and
retired ship-cache cleanup. Runtime source is prohibited from introducing another
`mud:` or `ship:snapshot:` literal, and the validator fails when a registry store is
missing from the manifest or a manifest Redis store is absent from the registry.

The lifecycle report exposes both non-database store count and Redis surface count, so
inventory growth is visible in validation output. Destructive maintenance retains only
the two registry-owned patterns and is checked for exact equality by the focused test.

Recommendation: create one declarative Redis key/channel registry used by runtime,
pwipe/admin tools, documentation, and lifecycle validation. The validator should compare
the manifest against keys/channels generated from that registry rather than a duplicate
handwritten list.

### RDS-023 - Broad FLUSHDB scripts lack full safety

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. Broad database flushes are removed. A shared
scanner deletes only the two declared Duris key patterns with bounded `DEL` calls after local
environment, exact target allow-list, explicit database, ACL/TLS, CA, and confirmation
checks, then verifies an empty postcondition. Missing tooling or any failure is nonzero.

- [`scripts/clear-redis.sh`](../../scripts/clear-redis.sh#L31) performs `FLUSHDB` using
  host and port only. It has a good owner/mode/symlink check for `.env`, but no explicit
  Redis database, ACL, TLS, environment allow-list, or destructive confirmation.
- The legacy migration runner requires Redis host/port even if Redis is disabled and
  flushes the entire database. If `redis-cli` is absent it prints `skipped` without
  incrementing its failure count, then exits successfully
  ([`migrations/run_migration.sh`](../../migrations/run_migration.sh#L3033)).
- The runbook contradicts the implementation by saying `clear-redis.sh` does not read
  `.env` ([`docs/operations/RUNBOOK.md`](../operations/RUNBOOK.md#L238)).

Recommendation: replace `FLUSHDB` with deletion of an exact versioned application prefix.
Require explicit environment/instance identity, stopped-writer proof, full authenticated
connection settings, confirmation, and postflight verification. A missing client or
failed invalidation must fail a migration that depends on cache clearing.

### RDS-024 - Graceful shutdown is treated as crash recovery

Severity: Medium
Confidence: Confirmed behavior; product intent needs confirmation
Remediation status: Completed on branch by preserving world state across a graceful
restart as an explicit policy. A fenced, expiring, one-use marker identifies the exact
clean generation; boot logs clean restart separately from crash recovery, and successful
restore consumes the generation without quiescing the next publisher.

Normal shutdown drains the recovery pipeline but does not clear the current generation
([`src/redis.c`](../../src/redis.c#L464)). A restart within the configured maximum age sees
that generation before `boot_db()` and takes the crash-recovery path. Documentation says
this path runs after an unclean exit
([`docs/operations/RUNBOOK.md`](../operations/RUNBOOK.md#L204)).

Recommendation: decide explicitly whether world preservation across graceful restart is
desired. If yes, rename and document it as restart recovery and record a clean-shutdown
marker. If no, clear the generation only after every required graceful shutdown save has
completed. Test manual shutdown, reboot, autoreboot, copyover, crash, and pwipe separately.

### RDS-025 - Observability is too coarse

Severity: Medium
Confidence: Confirmed
Remediation status: Completed on branch. Presence, report-cache, floor, donation, and
world-publication workers expose calls, categorized failures, consecutive failures,
latency, last-success age, queue/high-water state where applicable, and connection or
reconnect state without network queries. Boot/recovery/maintenance shared-context commands
expose the same redacted diagnostic dimensions plus subsystem and operation classification.
Both typed snapshot families feed `redis detailed` and trusted `world persistence`.

The world pipeline exposes useful counters and timing, but the shared command layer emits
only one global rate-limited line per second with a broad outcome. It omits command class,
subsystem, key class, retry state, consecutive failures, and recovery transition
([`src/redis.c`](../../src/redis.c#L81)). Status conflates empty data with unavailable
Redis, and there are no comparable health snapshots for donation, ship cache, presence,
or generic caches.

Recommendation: add redacted per-subsystem counters for calls, latency, timeout, error,
circuit state, reconnects, dropped work, last success age, and queue high water. Expose a
single typed health snapshot to both operator commands and health diagnostics.

### RDS-026 - Responsibilities are overly coupled

Severity: Medium
Confidence: Confirmed structural issue
Remediation status: Partially remediated. Connection/security, key registry, world store,
floor, presence, donation, and cache workers already have typed modules. Revisioned player
checkpoint ownership is in `persistence_checkpoint.c/.h`, and donation state plus bounded
game delivery is in `redis_donation_runtime.c/.h`. Presence policy, payload submission, and
enabled state are in `redis_presence_runtime.c/.h`. Epoch-scoped report keys, report
generation, local reads, asynchronous publication/invalidation, boot warm-up, and worker
lifecycle are in `redis_report_cache.c/.h`; the generic public cache API has been replaced
by report-specific operations. Fixed-capacity floor-delta capture, coalescing, lifecycle
state, and background submission are in `redis_floor_runtime.c/.h`, with season keys
precomputed at boot. Twenty former checkpoint, presence, report, or floor-only consumers
no longer include `redis.h`, which exports none of those subsystem APIs or mutable states.
The remaining composition root still owns world snapshot orchestration, direct
administrative cleanup, legacy ship cleanup, and broad configuration/lifecycle policy.

At the audit baseline, `src/redis.c` was 2,525 lines and combined
connection/configuration, world publication, floor recovery, ship serialization, report
caches, presence, pub/sub, administrator helpers, and legacy UID handling. `redis.h` also
owned revisioned player-save wrapper APIs that did not use Redis, and 29 C/C++ translation
units included the header. The composition root is now 1,685 lines, the header is 46
lines, and 9 translation units include it; the remaining world and administrative policy
still has a broad review and lifecycle surface.

Impact: changes have a broad compile/review surface, subsystem policy is inconsistent,
and tests tend to assert monolithic source layout rather than typed interfaces.

Recommendation: finish moving world snapshot orchestration, direct administrative
cleanup, legacy ship cleanup, and broad lifecycle policy behind typed modules. Keep each
subsystem's availability, codec, TTL, authority, and lifecycle policy beside its
implementation, leaving `redis.c` as the small boot/configuration composition root.

### RDS-027 - The no-MySQL build option is broken

Severity: Low
Confidence: Confirmed by syntax probe
Remediation status: Completed on branch by removing the unsupported whole-server toggle
and documenting MySQL/MariaDB client support as mandatory. Narrow `__NO_MYSQL__` unit
harness stubs remain explicitly non-production.

The Makefile advertises a commented `-D__NO_MYSQL__` toggle, but `redis.c` declares
`redisContext` pointers and a `redisReply` helper signature outside the hiredis guard.
Included SQL headers also expose unguarded `MYSQL` types. The syntax probe failed before
linking. Hiredis is linked unconditionally even though runtime Redis is optional
([`src/Makefile`](../../src/Makefile#L31)).

Recommendation: either remove the unsupported build promise or make the feature guards
complete and conditionalize libraries and objects. Add the supported variant to CI.

### RDS-028 - Legacy keys and unused APIs remain

Severity: Low
Confidence: Confirmed by call-site search
Remediation status: Completed on branch; the retired UID key is no longer read, written, or
displayed, and the unused generic Redis exports have been removed.

`mud:next_obj_uid` is read only to log that SQL remains authoritative, yet it is still
written at cleanup and displayed by the administrator command
([`src/redis.c`](../../src/redis.c#L512)). `redis_ping`, `redis_check_floor_pickup`,
`redis_check_floor_drop`, `redis_publish`, and `redis_get_string` have no production
callers.

Recommendation: remove retired key writes and dead exports after one explicit cleanup
release, or mark a bounded compatibility window. This reduces operator ambiguity and
keeps the key registry and lifecycle inventory accurate.

## Prioritized remediation plan

### P0 - Integrity blockers

1. Replace pwipe's delete-list protocol with a fenced season epoch and irreversible reset
   state machine. Stop old writers before any SQL mutation.
2. Make world publication and floor-delta consumption one epoch-aware atomic operation.
3. Stage recovery and validate every item UID, VNUM, parent, root, state, and owner before
   materialization. Add exact rollback or full normal-boot fallback. Completed on branch
   by schema 9 transactional recovery.
4. Remove Redis as a ship authority. Re-enable only after SQL revision validation and
   post-commit publication exist.
5. Fix `backup_pfiles.sh` mode selection, `pipefail`, validation, atomic publication, and
   nonzero failure propagation.

### P1 - Trust and availability

1. Introduce the central Redis adapter, namespace/key registry, ACL/TLS/database settings,
   health state machine, circuit breaker, and reconnect backoff.
2. Replace native recovery structs with a fixed-width versioned codec and add a true
   inbound/peak-memory bound. Completed by the schema-9 codec and RDS-017 chunked,
   indexed recovery storage.
3. Enforce presence privacy centrally and harden donation envelopes, budgets, escaping,
   and feature gating.
4. Move noncritical Redis writes off the simulation thread and pipeline bounded batches.

### P2 - Operability and maintainability

1. Version and expire all reconstructible caches; centralize commit-aware invalidation.
2. Rewrite or retire destructive Redis scripts and make admin clear/status checked,
   complete, fenced, and asynchronous.
3. Expand lifecycle inventory to every key and channel from the same runtime registry.
4. Split the Redis monolith and remove legacy UID/dead APIs.
5. Reconcile clean-shutdown recovery semantics and operational documentation.

## Required regression matrix

The following tests should gate production recovery enablement:

| Test | Required assertion |
| --- | --- |
| Redis starts down, later heals | No game-thread stall loop; all intended subsystems recover through bounded backoff; sequences do not rewind. |
| Pwipe with null/error context | Reset cannot claim success; no destructive SQL begins without the Redis/epoch precondition. |
| Pwipe with capture queued/publishing | Old generation cannot publish after the reset fence. |
| Kill after generation `SET` | Prior pointer remains valid; orphan is later collected. |
| Kill after pointer `EXEC`, before floor cleanup | Each UID is materialized at most once. |
| Root child moved after capture | Stale child is rejected; no new UID is created for it. |
| Missing VNUM/room or invalid door/zone | Restore applies nothing and normal boot produces a complete world. |
| Ship save inside outer rollback | Redis never exposes the uncommitted ship; SQL remains authoritative. |
| Cache update/invalidation outage | A recovered Redis cannot make an older ship or deleted row current. |
| Malformed ship/artifact JSON | Typed validation rejects the key without a crash and falls back to SQL. |
| Presence with privacy off/invisible staff | No account, IP, client metadata, or invisible identity reaches Redis. |
| Donation flood/spoof/replay | Signature, limits, replay protection, work budget, and backoff hold. |
| Oversized generation | Rejected before a large allocation or materialization. |
| Concurrent/stale publishers | Epoch and fence CAS permit only the current writer to advance the pointer. |
| Backup dump failure | No backup is published, exit is nonzero, and cycle handling is explicit. |
| Clean shutdown vs crash vs copyover | Each transition follows its documented recovery policy exactly. |

## Acceptance criteria for closing this audit

- No Redis-derived entity is materialized before complete structural, semantic, and SQL
  authority validation.
- A stale process cannot write into the current season namespace.
- Every destructive reset or clear has a preflight, writer fence, exact checked mutation,
  and postflight; it never resumes a partially reset old process.
- MySQL remains authoritative for ships and all durable item custody under cache failure.
- Redis configuration supports least privilege and environment isolation.
- Private presence is omitted by default on every transport.
- No synchronous Redis failure can consume an unbounded or repeated game-thread budget.
- Every declared key/channel appears in one runtime key registry, lifecycle inventory,
  pwipe policy, admin tooling policy, and behavioral test matrix.
- Ephemeral live-Redis tests cover outage, retry, transaction ambiguity, crash boundaries,
  malformed data, and multi-writer fencing.
