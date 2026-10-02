# Test suite streamlining

This follow-up to [the suite audit](TEST_SUITE_AUDIT.md) implements execution
accounting, explicit profiles, native artifact reuse and resource scheduling.
Behavioral upgrades and the remaining database/recovery orchestration are prepared
below as separate follow-ups. Production code, schemas and runtime protocols are
outside this change.

## Execution and profiles

[The inventory](../../tests/regression_manifest.json) explicitly classifies every
Python entry by purpose, execution mode, minimum unittest case count, profile,
estimated duration, CPU/memory reservations, and manual prerequisite where needed.
A newly discovered or missing file fails inventory validation, including filtered
runs. Metadata changes are intentional review decisions, rather than a naming
heuristic that silently decides whether to run a new test.

The default core profile retains every previously automatic entry and adds the
native-artifact regression. The 25 manual entries remain explicit. Automatic entries
with optional SQL checks remain in core, and their skips remain visible.

| Profile | Scope | Evidence boundary |
| --- | --- | --- |
| core | Every automatic entry, including optional integration checks | All selected scripts attempted; explicit skips still limit coverage |
| fast | Explicitly selected short offline/source/tool checks | Requires maintained tools and generated world inputs; no complete server build |
| native | Focused compiled fixtures and other offline checks | Linked/extracted production behavior within each fixture's stated limits |
| journey | Automatic process and server journeys | Private fixtures, real commands and lifecycle paths; manual journeys remain excluded |
| database | Database entries plus mixed suites with optional SQL cases | Required skips fail; manual prerequisites currently refuse automatic execution |
| recovery | Restore/backup/quarantine entries | Required skips fail; the copied-journal manual fixture is not provisioned automatically |

Use the existing full gate, or choose a profile:

~~~sh
make test-all -j2 TEST_JOBS=2
make test-fast TEST_JOBS=2
make test-python TEST_PROFILE=journey TEST_JOBS=2
make test-list TEST_PROFILE=database
python3 tests/run_regression_tests.py --profile fast --match training
~~~

The runner launches each entry through a private adapter while preserving its
arguments, working directory and import path. Unittest results include case names,
outcomes, durations and skip reasons. Collected and executed identities must agree;
a successful zero-case suite or a failed suite with a successful process exit is
rejected. A recorded minimum protects against accidental removal from a suite.

Standalone drivers are explicitly one entry-level case. Their native assertions
are not presented as independently observed Python cases. Plain declared function
tests must be invoked, and a declared main function must run. Function invocation
records are evidence of invocation; a failed script leaves their assertion outcomes
unresolved. The adapter does not establish that every C++ assertion is reached.
That evidence requires the behavioral work below.

JSON records are atomically updated after each completed entry, with pending
entries retained. Reports omit captured output and environment values; skip
reasons are supplied by the tests. JUnit XML exposes the same observed outcomes.
A module failure after successful cases remains a separate entry failure in XML.
The JSON case list mixes named unittest cases, function invocations and opaque
entry-level cases; its size is not a count of independent native assertions.

~~~sh
make test-python TEST_PROFILE=fast \
  TEST_REPORT=bin/fast-results.json TEST_JUNIT=bin/fast-results.xml
~~~

## Build reuse and scheduling

The accounting store, authority and bank fixtures share content-verified native
objects where compiler/options match. Their sanitizer flags, macros, linker
wrappers and private native-filesystem state remain intact. The player inspector's
actual build recipe has a shared owner used by its native regression and journeys.
Automatic inspector users bind to immutable cached binaries or private copies.

Cache keys conservatively include native source/header/fixture contents, compiler
and installed-toolchain bytes, flags, environment and selected link inputs.
Preserved-mtime edits invalidate reuse. Corrupt binaries/objects are rejected.
Publication is process-locked, and existing executable inodes are not overwritten.
New objects are staged without trusted manifests until all inputs are verified
after linking. Changed inputs reject publication; an interrupted builder cannot
poison the old key. Runtime player data, writable databases and
fault state are never cached. DURIS_REGRESSION_BUILD_CACHE=off retains uncached
standalone execution; compiled outputs still reside below bin.

Native build records distinguish object compilation, linking and lookup.
Server records distinguish build and lookup. JSON entry phases include the
remainder as entry_other: runtime, fixture setup, other subprocesses and assertions
are not separately measured inside unchanged drivers.

The scheduler orders entries by the last successful duration when provided,
otherwise by reviewed estimates. CPU, memory and named exclusive resource
reservations bound overlap. Heavy entries no longer wait for every lightweight
test to finish. Memory/CPU reservations are estimates, not OS-enforced limits;
use a container/cgroup for a hard machine limit. Unknown contention should be
resolved with a real resource declaration, not an unbounded worker increase.

~~~sh
make test-all -j2 TEST_JOBS=2 TEST_CPU_BUDGET=4 TEST_MEMORY_MB=4096 \
  TEST_DURATIONS=bin/previous-results.json
~~~

Native fixture compiler concurrency respects its CPU reservation, capped at two.
The existing 900/1800-second entry deadlines are preserved in metadata, with
TEST_TIMEOUT available as an explicit override. Process-group timeout cleanup,
immediate failure diagnostics, heartbeat and interrupt exit 130 remain.
Cancellation accounts for queued entries without starting them.

## Prepared behavioral follow-up

Begin with these concrete pilots. Each new assertion must identify its production
requirement and reject a deliberately incorrect implementation before a weaker
overlapping check is retired.

| Pilot | Existing owner | Executable upgrade | Deliberate fault / acceptance |
| --- | --- | --- | --- |
| Save refusal | test_epic_save_guards.py and the actual epic refund callers | Inject failed persistence at the production save boundary; inspect the required state and user-visible result | Remove/refactor the refusal branch; distinguish attempted save from acknowledged durable state |
| Currency replay | flatfile accounting store/bank/coin fixtures; currency_transaction_mysql_harness.cpp | Generate bounded sequences of transfer, duplicate replay, restart and rollback against the real implementation and a small independent model | Duplicate an effect, drop rollback or change a denomination; balances conserve and replay leaves state/counts unchanged |
| Item custody | player/item transfer and accounting-reference fixtures | Exercise split/merge, handoff, failed publication, restart and quarantine through production APIs | Allow two owners or admit a fenced PID; enforce one owner per live UID and durable admission policy |
| Load/query budget | player_death_recovery_mysql_harness.cpp | Extend existing actual-query/metric agreement across generated empty/nonempty retained-state combinations | Add an unmetered query or hide a query from metrics; actual count agrees and stays within the declared ceiling |
| Concurrency ordering | pool-discard and deletion/retention fixtures | Add bounded deterministic synchronization at the actual ownership/wait/commit boundaries | Premature lease close, missed wakeup or wrong lock ordering must fail without lengthening client deadlines |

Keep registration and forbidden-dependency checks as structural evidence.
Use generated sequences against the actual implementation; testing only a
reimplementation of the model would not qualify these behaviors. Start with
bounded deterministic generation using existing tools. Add a property-testing
dependency only if that pilot demonstrates a useful capability beyond the existing
harness. Preserve a minimal failing sequence as a normal regression.

The existing twelve audit faults and the new entry/cache/scheduler faults provide
a starting catalog, not a suite-wide mutation score. Establish a requirement-to-
case map for these pilots and review surviving faults. Code/branch coverage can
identify unexercised paths, but cannot establish assertion correctness on its own.

## Prepared database and recovery matrix

The database and recovery profiles expose the outstanding prerequisite boundary
now; they do not provision all manual fixtures. The next implementation should
provide the missing owners, not source the checkout's .env or point tests at a
configured game/database.

| Fixture family | Existing entry points / provider | Missing integration work |
| --- | --- | --- |
| Owned schema and migration wrappers | make test-db; run_runtime_compatibility_mysql.sh; legacy wrappers | Record the exact supported image digest and required cases; run parity rows for both supported engines where applicable |
| Empty pool/deletion/load fixtures | test_sql_pool_discard_mysql.py; player death conflict/recovery/deletion drivers | Allocate fresh engine-specific schemas with the existing name guards and TEST_DB_DISPOSABLE opt-in; retain real errors and query timelines |
| Accounting, item reconcile and operation receipts | Manual economic schema/reference, player-save reconcile and spell-receipt drivers | Supply exact migrations, fixture identities and cleanup; validate actual native/runtime cases beyond their source contracts |
| Mixed flat-file/SQL drivers | epic stone, locker receipt recovery, restitution target | Split prerequisite-specific cases into declared matrix rows; include a non-root row for user-systemd checks rather than accepting its skip |
| Frozen artifact SQL journeys | pa_accounting_batch_artifact.py and pa_copyover/web helpers | Freeze a single verified source/binary/descriptor, pass DURIS_ACCOUNTING_BASE_BUILD, lease private fixture schemas and runtime roots |
| Other real-server journeys | issue331, playtime, pet restart, mob gold and death/resurrection drivers | Supply private server/promotion helpers and explicit arguments; do not assume every manual entry needs SQL |
| Privileged backup/restore | test_persistence_backup_integration.py | Provide Linux namespaces/capabilities and private MariaDB; require all nine cases to run |
| Copied-journal recovery | test_player_save_journal_quarantine.py | Produce a synthetic captured journal plus matching custody manifests under a private fixture; never capture an existing user's journal |

Review manual classification separately: test_pa_runtime_sql.py itself protects a
wrapper's source contract, while its run_pa_runtime_sql.sh wrapper provides real
SQL evidence. A SQL-shaped filename or successful compilation does not establish
a database execution.

For each matrix row, record the exact commit/source manifest, compiler/options,
engine image/digest, fixture provider, required case IDs and capability boundary.
Missing required prerequisites or cases must produce an incomplete/failed matrix,
even when all executed entries pass. Keep excluded manual entries explicit in the
selected workload manifest.

Preserve sanitized failure artifacts before cleanup: original client error,
process/case timeline, seed, query/commit observations and synthetic journal/schema
digests where relevant. Flush outcomes while the run progresses. A passing rerun
must remain a separate attempt; it must not replace the first failure.
The historical pool DDL failure remains unexplained until original-quality evidence
or a reproducible fault establishes a cause.

Expose the same matrix through local commands and workflows for the maintained
development branch. Existing master backup and dual-engine telemetry workflows
remain useful owners; their branch scope does not cover every development branch.
Local executable verification remains the merge evidence required by AGENTS.md.

## Validation record

The implementation is qualified with private exported sources in an owned Linux
container. Maintained world inputs are generated there; no configured account,
repository .env, shared game or production database is used.

The native-artifact suite executes the real compiler and binaries for object reuse,
content/flag/toolchain invalidation, corruption, concurrent publication, cache-off
execution and failed input stability. Installed-header/library hashing is also
protected by the existing server-artifact suite. The outer runner exercises real
child processes for discovery, missing entry points, named results, strict skips,
resource/lock overlap, duration priority, timeout and cancellation.

Final command outcomes and performance measurements are recorded after execution.
Raw JSON/XML/logs remain ignored artifacts under bin/streamline.
