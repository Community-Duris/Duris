# Test suite streamlining

This follow-up to [the suite audit](TEST_SUITE_AUDIT.md) implements execution
accounting, explicit profiles, native artifact reuse and resource scheduling.
It also upgrades five behavioral pilots and supplies the disposable database and
recovery matrix. Production code, schemas and runtime protocols are unchanged.

## Execution and profiles

[The inventory](../../tests/regression_manifest.json) explicitly classifies every
Python entry by purpose, execution mode, minimum unittest case count, profile,
estimated duration, CPU/memory reservations, and manual prerequisite where needed.
A newly discovered or missing file fails inventory validation, including filtered
runs. Metadata changes are intentional review decisions, rather than a naming
heuristic that silently decides whether to run a new test.

The default core profile retains every previously automatic entry and adds the
native-artifact regression. The remaining 24 manual entries have explicit providers in the integration matrix. Automatic entries
with optional SQL checks remain in core, and their skips remain visible.

| Profile | Scope | Evidence boundary |
| --- | --- | --- |
| core | Every automatic entry, including optional integration checks | All selected scripts attempted; explicit skips still limit coverage |
| fast | Explicitly selected short offline/source/tool checks | Requires maintained tools and generated world inputs; no complete server build |
| native | Focused compiled fixtures and other offline checks | Linked/extracted production behavior within each fixture's stated limits |
| journey | Automatic process and server journeys | Private fixtures, real commands and lifecycle paths; manual journeys remain excluded |
| database | Database entries plus mixed suites with optional SQL cases | Required skips fail; the matrix supplies each reviewed prerequisite |
| recovery | Restore/backup/quarantine entries | Required skips fail; quarantine uses a generated private journal |

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
rejected. Observed skips remain visible when unittest redirects its console
summary. Successful entries must acknowledge completed observation; os._exit(0)
cannot bypass final validation, including after successful cases. A recorded
minimum protects against accidental removal from a suite.

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
Cache-off builds aggregate fingerprinting, compilation and linking in the compile
field rather than separating those phases.
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

## Behavioral qualification

The behavioral owners execute production code and compare its results with
independent requirements. The reviewed fault catalog is
[behavioral_faults.json](../../tests/behavioral_faults.json). Qualification copies
only source, test and migration inputs below `bin/`, mutates that private copy,
and runs positive controls before and after each fault. A missing mutation site,
compile failure, surviving fault or unsuccessful control fails qualification.
The original checkout is checked for unchanged production source bytes.

| Requirement | Executable owner | Independent assertion | Reviewed fault |
| --- | --- | --- | --- |
| SAVE-REFUSAL | test_epic_save_guards.py | The actual epic callback preserves learned/taught skills on refusal; reset callers attempt a checkpoint and diagnose its failed acknowledgement | Ignore the refused refund |
| CURRENCY-REPLAY | test_flatfile_accounting_bank.py | Three fixed seeds generate transfers, refusals, old replay and process restart; modeled denomination balances/revisions conserve value and replay preserves all durable bytes | Rotate denominations; misreport an old replay as a new commit |
| CUSTODY-HISTORY | test_flatfile_item_repository.py | Twelve split/merge/handoff steps retain exactly two live UIDs with one modeled owner and topology; failed publication, interrupted commit and old replay preserve the required state | Retain the source owner after a handoff |
| CUSTODY-ADMISSION | test_player_save_journal_quarantine.py | The real journal/codec archives fenced records byte for byte, refuses failed sync and protected append, and replays unaffected records; terminal refusal fences the remaining PID | Admit a quarantined PID |
| LOAD-QUERY | player_death_recovery_mysql_harness.cpp | All 32 item/pet/retained combinations, full/metadata requests and PID/name lookup agree with independently wrapped actual SQL executions and the query ceilings; refused loads expose no materializable payload | Hide an executed query from metrics |
| POOL-LEASE | test_sql_pool_discard_mysql.py | A borrower waits at the actual condition-variable boundary; marked leases remain owned until release; a clean replacement wakes and commits while abandoned transactions roll back | Expose a marked lease for reuse; remove release wake-up |

These are scoped fault detections, not a suite-wide mutation score. Native
assertions inside a standalone driver remain one opaque Python entry; their
seed/step and completion witnesses are required separately by the matrix.
Existing deletion/retention tests continue to synchronize at observed server
row-lock requests, and the generated load owner also checks those orderings.

The epic fixture compiles the whole production translation unit but supplies
external persistence and output boundaries. It distinguishes a save attempt from
an acknowledged checkpoint; it does not assert that a stub wrote durable data.
The journal fixture uses a mock receipt repository behind the real codec/journal;
full native custody and recovery remain separately exercised by the existing
player repository and integration journeys. The non-root systemd cases execute
ownership, manager, unit and cgroup validation with a fixture manager; they do not
prove communication with a live user's systemd service.

## Disposable database and recovery matrix

[The workload](../../tests/integration_manifest.json) declares engine/provider,
arguments, opt-ins, case identities and native witnesses for every SQL, recovery
and manual owner. Coverage is validated before filtering. Missing required cases,
skips, absent witnesses or nonzero exits fail a row. Setup/build/capability errors
leave required coverage incomplete, including in JSON and JUnit. Filters record
all excluded identities and cannot support a complete-matrix claim.

The matrix freezes both backend binaries from a clean Git commit, verifies source
hashes after building, and records the full source manifest, compiler/options,
image IDs/digests, required cases, individual outcomes and pending workload.
MySQL 8.0.46 and MariaDB 10.11.19 are pinned to image digests. Nested compiler
containers bind to the recorded tools image ID. Fresh schemas apply the maintained
bootstrap and migration owner twice and check runtime compatibility; empty-schema
fixtures receive their explicit guarded prefixes instead.

The shared SQL owner creates only new labelled containers with generated
credentials. A container runner shares only its own validated network namespace;
a native Linux runner publishes exclusively on loopback. Legacy wrappers retain
their existing SQL harnesses and now receive the selected engine. Original client
errors, setup/query events, source and synthetic journal digests, and outcomes are
written before cleanup. Credentials are redacted. Every attempt gets a new evidence
directory; a passing rerun cannot replace the original failure.

The same command runs locally and in the development/master branch
[workflow](../../.github/workflows/integration-matrix.yml):

~~~sh
python3 tests/run_integration_matrix.py --list
make test-integration
make test-integration TEST_ENGINE=mariadb TEST_MATCH=player_death_recovery
python3 tests/qualify_behavioral_faults.py --family offline \
  --evidence-dir bin/behavioral-faults-new-attempt
~~~

The matrix requires Linux, Docker, maintained build dependencies and native
MariaDB recovery tools. The checked-in tools image provides them without reading
the repository's `.env`. Windows users can run the following owned container
procedure from Bash/WSL. Use a fresh name for each attempt:

~~~sh
docker build -f tests/integration/Dockerfile -t duris-regression-tools:local .
# Pull both exact images listed in tests/integration_manifest.json first.
fixture=duris-matrix-private-attempt
docker create --name "$fixture" --restart=no --cpus=4 --memory=6g \
  --cap-add=SYS_ADMIN --cap-add=NET_ADMIN \
  --security-opt=apparmor=unconfined --security-opt=seccomp=unconfined \
  -v /var/run/docker.sock:/var/run/docker.sock duris-regression-tools:local
docker start "$fixture"
git archive HEAD > bin/matrix-source.tar
docker cp bin/matrix-source.tar "$fixture:/tmp/source.tar"
docker exec "$fixture" tar -xf /tmp/source.tar -C /suite
# A normal checkout uses .git here. For a worktree copy its resolved Git
# metadata/common object store, or use a fresh clean checkout instead.
docker cp .git "$fixture:/suite/.git"
docker exec -e "DURIS_TEST_CONTAINER=$fixture" -w /suite "$fixture" \
  python3 tests/run_integration_matrix.py
docker cp "$fixture:/suite/bin/integration-results" bin/
# Remove only this newly created fixture after retaining its evidence.
docker rm --force "$fixture"
~~~

Recovery explicitly requires namespace/mount capabilities and all nine backup
cases. The copied-journal owner now generates a synthetic capture with real
codec/journal APIs and matching hashed manifests; historical private captures
remain supported only with all four explicit inputs. No configured game, existing
account, shared database or production state is selected. Core retains visible
optional SQL/systemd skips; the complete matrix requires its declared integration
checks to execute. Local executable verification remains the merge evidence;
AGENTS.md does not require waiting for CI.

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

The complete safe gate ran against 886bd827 after merging development base
4d027735. make test-all -j2 TEST_JOBS=2 TEST_CPU_BUDGET=4 TEST_MEMORY_MB=4096
passed all maintained builds/native checks and completed 835 Python entries:
824 passed, 11 explicitly skipped, no failures or pending work. The Python phase
took 3402.24 seconds (56 minutes 42 seconds); preceding build/world setup is
outside that timing. It recorded 1846 observations, including function invocations
and opaque entries, rather than 1846 independent native assertions.

Review during qualification reproduced two reporting defects: hidden unittest
summaries lost skip accounting, and premature zero exits bypassed observation.
Both were fixed in subsequent focused commits. The final executable test code is
5360389f; its src tree remains c8804d6b and its tests tree is 0762828c.
Every automatic unittest entry was rerun after the completion protocol change:
129 passed, 9 explicitly skipped, no failures across 138 entries in 187.68 seconds.
The run includes all 14 runner cases and all 6 real native-artifact cases. The
complete gate's 697 script entries each had their final entry record; this is
compatibility evidence for the added acknowledgement, not a fresh body execution.
The final fast profile was also rerun separately: 366 passed, 1 whole-entry skip,
no failures across 367 entries in 32.08 seconds after world/tool setup. It recorded
986 observations, with the additional partial SQL/systemd skips visible. Raw reports identify the
separate attempts; no passing rerun replaces the initial failure evidence.

The unchanged player-repository driver, including native and quarantine recovery
assertions, was measured under the same frozen sources, compiler/options,
environment and container limits with a fresh private cache:

| Build setting | Entire entry | Compilation | Linking | Lookup |
| --- | ---: | ---: | ---: | ---: |
| off | 65.77 s | 52.29 s aggregate build/lookup | included | included |
| cold | 42.77 s | 25.58 s | 0.46 s | 4.01 s |
| warm | 15.14 s | 0 s | 0 s | 1.98 s |

All three attempts passed. The warm run reused all 55 inspector objects and ran
the existing runtime assertions. Runtime/setup/remainder stayed approximately
13 seconds; the removed work was compilation. In the complete gate the accounting
store reused 13 authority objects and compiled its own driver in 4.42 seconds.
These measurements qualify this fixture improvement; they are not a controlled
before/after comparison of the entire historical suite.

The interrupted-builder regression was also run with a deliberately injected
early-publication fault. It rejected stale value 99 where reverted source required
42. Runner regressions reject zero-case collection, disconnected entry points,
ignored unittest failures, hidden skips, and premature exits before/during/after
cases. Real child-process tests qualify resource overlap, locks, timing priority,
timeout cleanup and cancellation.

The 11 whole-entry skips and additional partial skips are integration boundaries:
disposable SQL, privileged restore, and non-root user-systemd capabilities were
not provisioned. The 25 manual entries remain excluded from core. Their complete
execution belongs to the prepared matrix above.

Raw JSON/XML/logs remain ignored artifacts under bin/streamline and are retained
outside the owned qualification container before cleanup.
