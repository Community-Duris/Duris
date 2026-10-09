# Testing

The project uses focused regression tests in `tests/async/`, plus
schema/migration checks at `tests/` root. The root `Makefile` provides a single
developer and CI gate while retaining fast commands for focused work.

## Layout

```
tests/
â”œâ”€â”€ async/                       # focused regression + source-contract tests
â”‚   â”œâ”€â”€ test_*.py, *_test.py      # standalone scripts and unittest regressions
â”‚   â”œâ”€â”€ run_*.sh                 # special-purpose and legacy thin wrappers
â”‚   â””â”€â”€ run_*_mysql.sh           # MySQL-backed schema-contract tests (need a live DB)
â”œâ”€â”€ run_regression_tests.py      # discovery, deadlines, outcomes and timing reports
â”œâ”€â”€ compare_bootstrap_mud_schema.sh   # diff live schema vs bootstrap baseline
â”œâ”€â”€ test_migration_replay_safety.sh   # migration re-run safety
â””â”€â”€ test_run_migration_persistence_schema.sh
```

## Test styles

**Source-contract tests** â€” read the C sources as text and assert structural
invariants (a registration exists, a call site was not reintroduced, an ordering
holds). These need no database and no build. Their evidence is structural:
text presence cannot prove runtime reachability, a branch's effect, or transaction
atomicity. Prefer an executable fixture for a behavioral requirement.

The shared `contract_text` helpers normalize formatting whitespace while retaining
identifier/number boundaries and lexical states. Code-shaped expectations cannot be satisfied
by a comment or a string containing the same statement. Use `literal=True` when
SQL, log or user-facing text intentionally contains code punctuation or control
keywords. Slice complete lexical regions; starting in the middle of a quoted
message changes how the remainder parses. `_source_contract` and `_paths`
extract definitions using braces in code, skipping prototypes and quoted braces.

**Behavioral tests** â€” exercise linked or extracted production functions, Python
tools, or complete server journeys with private fixtures. For example,
`tests/async/test_sql_pool_discard_recovery.py` exercises the real pool's acquisition
deadline, waiting borrower wake-up, lease ownership during shutdown, replenishment
and exclusion gates. It uses a fake connection factory and real synchronization;
`test_sql_pool_discard_mysql.py` separately requires real disposable SQL sessions.
An extracted-function test has the integration limits of its stubs. A synthetic
accounting fixture validates the auditor, rather than proving gameplay produced
that evidence.

**Schema tests** - verify migrations/persistence contracts against disposable
MySQL or MariaDB instances (development databases only).

## Running

```bash
# Complete safe developer/CI gate: builds all maintained binaries and tools,
# generates world data, then runs Python and native regression tests.
make test-all

# Regression tests without rebuilding the server or area editor:
make test

# Limit concurrency, filter by filename, or inspect discovery:
make test TEST_JOBS=1
make test-python TEST_MATCH=wear
make test-python TEST_TIMEOUT=120 TEST_REPORT=bin/fast-results.json
make test-list

# Single test (preferred while iterating):
python3 tests/async/test_wear_all_regression.py
# or via its wrapper:
tests/async/run_sql_pool_shutdown.sh

# Isolated Docker/MySQL schema suites (Docker is an optional prerequisite):
make test-db

# Required SQL and recovery workload, with owned fixtures and no permitted skips:
make test-integration
python3 tests/run_integration_matrix.py --list
make test-integration TEST_ENGINE=mysql TEST_MATCH=player_death_recovery

# Full historical legacy upgrade, replay, bootstrap equivalence, and compatibility:
tests/async/run_legacy_migration_mysql.sh

# Runtime schema compatibility on both supported variants:
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

The default core profile discovers both test naming conventions and retains every
automatic entry. Every discovered file must have an explicit entry in
`tests/regression_manifest.json`; unclassified additions and missing registered
files fail before execution. Manual prerequisites remain explicit. Use
`make test-fast` for the short offline profile, or `TEST_PROFILE=native`, `journey`,
`database`, or `recovery` with `make test-python` or `make test-list`.
Database/recovery profiles reject required skips. Passing fast entries can still
contain optional integration skips; those cases supply no fast-profile coverage.
The required matrix separately supplies the doctor's real SQL check, native
restitution backup and non-root systemd ownership checks, and rejects their skips.

`make test-integration` provisions
the reviewed SQL and recovery workload in `tests/integration_manifest.json`,
including required cases from mixed suites, manual arguments, privileged recovery,
and non-root recovery. It uses pinned MySQL and MariaDB images, fresh schemas,
frozen builds and private runtime state; missing cases and skipped checks fail.
The SQL deletion owner requires both account fencing/retry and native atomic
erasure: corrupt state and failed writes refuse, late failure rolls aliases back
with player data, and repaired retry permits actual later alias publication.

`TEST_JOBS=0` selects workers from the available CPU budget (cgroup-aware, capped
at eight). The runner starts expensive tests early and schedules overlap using
explicit CPU, memory and exclusive-resource reservations. Set `TEST_CPU_BUDGET`
and `TEST_MEMORY_MB` to match the machine; these reservations are estimates,
not OS resource limits. `TEST_DURATIONS` selects a previous JSON report for
longest-first ordering. Native fixture compilation is capped at two workers.

Each entry runs in a separate interpreter. The adapter observes named unittest
cases, rejects a successful zero-case suite, verifies declared function/main
invocation, and preserves failure/skip outcomes. Native drivers remain explicit
entry-level cases; their internal assertions are not counted as observed Python
cases. Successful entries must acknowledge completed observation, so a premature
zero exit cannot bypass validation. Observed unittest skips do not depend on its
console summary, including skips inside subtests. Ignored class/module cleanup
errors also fail the entry and remain named outcomes. Failure diagnostics print
immediately and a 30-second heartbeat names
active tests. Existing 900/1800-second deadlines remain in metadata; `TEST_TIMEOUT`
overrides them. Timeout/interrupt cleanup terminates process groups on POSIX and
uses `taskkill /T /F` on Windows. Ctrl+C cancels queued work and returns 130.
Deliberately detached POSIX sessions remain outside the group.

`TEST_REPORT` selects the atomic JSON timing/case report, updated after each
completed entry with remaining work listed. `TEST_JUNIT` selects the case-level XML
report (default `bin/test-results.xml`). JSON records statuses, exit codes,
the inventory checksum, selected/excluded identities, skipped-check counts and
build/lookup/remainder phases without captured output or
environment values. Skip reasons are test-supplied. Explicit whole and partial skips
remain visible; successful compilation followed by a SQL skip is not SQL evidence.
Build/world generation precedes the Python timings.

The native accounting and player-inspector fixtures reuse immutable, verified
build artifacts with content/flag/toolchain invalidation. Runtime state remains
private. See [test suite streamlining](../testing/TEST_SUITE_STREAMLINING.md) for
profile boundaries, cache/scheduling proofs, behavioral fault qualifications and
the disposable integration matrix. `.github/workflows/integration-matrix.yml`
runs the same matrix command; local results are the merge evidence.

Do not invoke a discovered sibling test from another test merely to run it again.
Import helpers without executing their test body, and give a unique fixture a
focused owner. See the [suite audit](../testing/TEST_SUITE_AUDIT.md) for per-entry
decisions and the limits of the evidence.

`make test-all` deliberately excludes Docker and externally provisioned
database checks. `make test-db` creates and destroys isolated MySQL containers;
the three scripts at the root of `tests/` require explicitly named disposable
or read-only databases and are manual migration-verification tools. Never point
them at production.

The MariaDB playtime journey also runs explicitly. Supply `TEST_DB_HOST=127.0.0.1`,
`TEST_DB_USER`, and `TEST_DB_PASSWORD` for a disposable local database server,
then run `python3 tests/async/test_mysql_playtime_journey.py --server bin/server/dms_new`
after a MariaDB server build. The journey creates and removes its own schema and
invokes `test_playtime_mysql_repository.py` with that schema's environment. Both
scripts are excluded from argument-free discovery; the database-independent
playtime capture, checkpoint, legacy SQL, and flat-file tests remain in `make test-all`.

MySQL fixtures that reuse a table within a statement must account for MySQL's
`Can't reopen table` restriction on connection-local temporary tables; MariaDB
may accept the same fixture. Use isolated ordinary tables with explicit cleanup
when needed to execute the actual production query shape. Do not weaken queries
or narrow the backend matrix merely to make a temporary-table fixture pass.

### Shared journey server artifacts

The Python runner defaults `DURIS_REGRESSION_BUILD_CACHE` to
`bin/regression-artifacts`. Account recovery, auction/coin-put, flat-file boot
preflight, Chaos kit, combat and full-world boot acquire one compatible flat-file server from
`tests/async/server_build_artifacts.py`. Each journey retains its own temporary
authority, journals, logs, listeners and process cleanup. Executables are shared
read-only; runtime state is never stored in the artifact directory. The
item-prompt ASan/UBSan harness remains a separate build with its existing flags
and timeout. Resource reservations bound overlapping journeys; each server build
retains two compiler jobs and the original 600-second build ceiling. Auction/coin-put
acquires a private copy of the immutable inspector, so independent runners do not
overwrite a shared executable.

Resource scheduling bounds contention within one runner; a filtered run, disabled
cache, or changed build inputs can still require a cold server build. Cache hits
also hash inputs and validate the artifact before reuse. The runner reports build
and validation time separately so this cost remains visible.

The artifact key covers all files under `src/` (including untracked files),
test headers, the helper contract, the flat-file backend, and the inherited build
environment, including profile, feature flags and Make overrides. It also hashes
the effective compiler, Make, assembler, linker, compiler internals, installed
headers and libraries. Standard GNU compiler search directories and explicit
`CPATH`, `CPLUS_INCLUDE_PATH`, `C_INCLUDE_PATH` and `LIBRARY_PATH` directories,
plus explicit include/library paths in Make's effective flags, are fingerprinted
by content. Header search paths are recursive. Library search paths fingerprint
directly searchable libraries, objects and linker scripts; the linker does not
recursively search unrelated Python/data subdirectories. An explicitly selected
nested `-L`/`LIBRARY_PATH` directory is still fingerprinted. Keep external build inputs in these declared paths;
use `DURIS_REGRESSION_BUILD_CACHE=off` for toolchains with hidden inputs or
wrappers that do not support GNU compiler discovery. Runtime-only full-world
diagnostics and shell working-directory variables do not invalidate a build.

On every acquisition, the helper verifies executable and build-log SHA-256
hashes and rechecks the client-free compilation assertions. Missing or malformed
metadata, altered binaries/logs and changed inputs trigger a fresh build. Inputs
are checked again after compilation; a build spanning an input change is rejected.
A per-key lock prevents duplicate concurrent builds, and atomic manifests publish
only successful artifacts. Replacement builds use new directories so an existing
process keeps its executable. Old generations remain until the cache is removed
when no journeys are using it; `make clean-all` also removes `bin/`.

Direct test invocations build privately unless the cache variable is set. To
measure the complete Python gate on the same host and worker count:

```bash
DURIS_REGRESSION_BUILD_CACHE=off python3 tests/run_regression_tests.py --jobs 4
DURIS_REGRESSION_BUILD_CACHE="$PWD/bin/benchmark-fresh" python3 tests/run_regression_tests.py --jobs 4
DURIS_REGRESSION_BUILD_CACHE="$PWD/bin/benchmark-fresh" python3 tests/run_regression_tests.py --jobs 4
```

Choose an unused directory for the clean-cache run. Per-journey output reports
server build time, artifact validation time, and remaining journey/fixture time.
The last category includes inspector compilation and process startup; it is not
solely gameplay time. Cache keys hash environment values without storing their
plaintext in manifests. Never check cache artifacts into Git.

### Build-reuse measurement (2026-09-10)

The complete Python gate was measured sequentially in one Ubuntu 24.04 Linux
container on an Intel Core i7-12700K host, with a four-CPU quota, four regression
workers, two jobs per server build, GCC 13.3.0 and Python 3.12.3. World data was
prepared before each gate. The baseline used commit `206c95de` with timing-only
instrumentation around the original build calls; no assertions or deadlines were
changed. The candidate adds one focused artifact-cache regression.

| Run | Gate result | Gate wall time | Server builds | Server build time | Artifact validation | Journey/fixture time |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Before reuse | 438 passed, 0 failed | 1,314.58 s | 5 | 520.39 s | N/A | 595.74 s |
| Empty artifact cache | 439 passed, 0 failed | 911.18 s | 1 | 113.51 s | 8.91 s | 596.48 s |
| Retained artifact cache | 439 passed, 0 failed | 780.99 s | 0 | 0.00 s | 4.56 s | 580.60 s |

Build, validation and journey/fixture columns sum the five server journeys;
they exclude the parallel tests and the separate sanitizer harness. The gate wall
time includes every discovered test, including that harness. The clean-cache run
was 30.7% faster overall, and the retained-cache run was 40.6% faster. All five
journeys used the same one published artifact across both candidate runs.

These are single-run, host-specific measurements, not an SLA. "Empty cache"
refers to server artifacts; the operating-system page cache was not flushed.
Initial environment-setup attempts were excluded; the reported runs used the same
installed dependencies, Linux line endings, hardware and concurrency.

## Full-world save diagnostics

`test_flatfile_full_world_boot.py` supports opt-in synthetic failure/recovery
experiments without using the configured database:

| Variable | Effect |
| --- | --- |
| `DURIS_FULL_WORLD_ARTIFACT_DIR` | Retain failed synthetic authority, journals, transcripts, and server diagnostics in a private untracked directory; TLS keys are excluded and the fixture password is redacted. |
| `DURIS_FULL_WORLD_REPEATS` | Run 1-100 fresh fixture journeys with one build. |
| `DURIS_FULL_WORLD_BINARY_CACHE` | Legacy opt-in cache prefix below `bin/`; verified generations now live in `<prefix>.artifacts/`. Existing legacy binary/JSON pairs are not reused. `DURIS_REGRESSION_BUILD_CACHE` takes precedence when set. |
| `DURIS_FULL_WORLD_DELAY_CAMP=1` | Hold the synthetic player lock through camp timeout; verify retention, automatic retry, and a later fresh-intent camp. |
| `DURIS_FULL_WORLD_CRASH_PHASE` | `before_ack` or `after_ack` replaces the first clean shutdown with an intentional crash around the save durability boundary. |

Normal runs keep their original deadlines and successful-fixture cleanup.
Classify failures by boot, manual save, camp, recovery, or shutdown; distinguish
an observation timeout from a rejected save. Retain the revision/worker timeline
before diagnosing storage or scheduler latency. A passing rerun cannot establish
the cause of a historical failure whose timeline was discarded.

The separate combat journey reconnects after creation to hydrate account-bank
revision and disables optional boons. It proves that stated fixture scope;
it does not prove first-session currency or boon-enabled death/reward behavior.

## Validation matrix

No single command proves release readiness. Use the narrowest applicable row while
iterating, then run every row required by the session or release gate.

The disposable integration matrix runs death/resurrection with accounting explicitly
inactive (`--legacy-persistence`). It proves durable gameplay and requires empty
accounting authority/evidence before and after execution, then reports the uncovered
legacy events. The journey's default strict mode still rejects those missing roots,
postings and item references. Passing the inactive row does not qualify an accounting
release; retain and review its `RELEASE ACCOUNTING COVERAGE BLOCKED:` evidence.

| Evidence boundary | Command | What it proves | What it does not prove |
|---|---|---|---|
| Documentation | `python3 tests/async/test_documentation_contract.py` | Maintained links, paths, commands, configuration names, safety language, and diagram contracts | Runtime or database behavior |
| Focused source/runtime | `python3 tests/async/test_<feature>.py` | One named invariant or compiled harness | Unrelated domains or integrated load |
| Server build | `make -C src` | C++20 server compiles under the warning profile | Schema compatibility or runtime readiness |
| Repository gate | `make test-all` | Maintained builds, generated world inputs, all discovered Python tests, and native signal tests | Docker database suites, representative data, or a 200-player hold |
| Disposable schema | `make test-db` | Listed schema contracts and legacy-to-current convergence on isolated Docker databases, including the explicitly selected MySQL/MariaDB variants | Complete engine parity across every domain or configured database state |
| Dual-engine boot contract | `tests/async/run_runtime_compatibility_mysql.sh` and `RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh` | Fresh bootstrap, immutable head, drift rejection, and boot compatibility on MySQL 8 and MariaDB 10.11 | A configured or production upgrade |
| Lifecycle/privacy | commands below | Pending-policy fail-closed behavior, synthetic archive/export/erasure contracts, and disposable schemas | Controller approval, legal compliance, or enabled canonical mutation |
| Capacity/fault precursors | commands below | Bounded 25/50/100/200 logical-client codecs and named crash/fault invariants | Representative eight-profile 30-minute 200-player readiness |
| Final Phase 03 gate | Session 14 specification and its future sanitized report | Only the complete qualified workload, fault, reconciliation, privacy, migration, and restore evidence can support readiness | Nothing until every criterion is executed and recorded |

### Load, recovery, and fault contracts

These focused tests use source inspection, compiled local harnesses, or synthetic
state. They do not connect to the configured database:

```bash
python3 tests/async/test_player_load_pipeline.py
python3 tests/async/test_player_load_items.py
python3 tests/async/test_player_load_pets.py
python3 tests/async/test_player_save_pipeline.py
python3 tests/async/test_player_save_journal.py
python3 tests/async/test_critical_command_coordinator.py
python3 tests/async/test_critical_transaction_contract.py
python3 tests/async/test_world_recovery_pipeline.py
python3 tests/async/test_maintenance_scheduler.py
python3 tests/async/test_worker_fault_injection.py
python3 tests/async/test_phase01_recovery_gate.py
python3 tests/async/test_phase02_capacity_and_crash_gate.py
```

The Phase 01 and Phase 02 gates include 25/50/100/200 logical-client waves and bounded
codec/crash coverage. They are precursor evidence only. They do not perform the eight
representative workload profiles, 30-minute 200-player holds, production-clone query
measurements, or complete fault/reconciliation matrix required by Session 14.

### Account recovery contracts

Password reset by email is covered by an executed core harness (injected mail sender and
clock, no network), a wire-level SMTP harness against a fake relay on `127.0.0.1`, a
source-contract pin set, and a telnet journey that boots the client-free server with a
fake relay. None of them needs a real relay or a database; the compiled harnesses need
`g++` plus the libcurl and OpenSSL development headers from the dependency metapackage:

```bash
python3 tests/async/test_account_recovery.py
python3 tests/async/test_account_recovery_smtp_live.py
python3 tests/async/test_account_recovery_contract.py
python3 tests/async/test_account_recovery_journey.py
```

The harnesses never print a reset code or an address; a captured code reaches the driver
only through a temporary file. TLS/STARTTLS and SMTP AUTH are not exercised (the fake relay
is plaintext on loopback, which the configuration rules permit), so the first production
send is the first evidence for a real relay.

### Lifecycle and privacy contracts

Run the source/synthetic controls first:

```bash
python3 scripts/validate_data_lifecycle.py --json
python3 tests/async/test_data_lifecycle_manifest.py
python3 tests/async/test_season_reset_manifest.py
python3 tests/async/test_lifecycle_archive_execution.py
python3 tests/async/test_personal_data_export.py
python3 tests/async/test_account_erasure.py
python3 tests/async/test_immutable_migration_runner.py
python3 tests/async/test_runtime_boot_compatibility.py
```

Then run schema behavior only through the self-contained disposable wrappers:

```bash
tests/async/run_lifecycle_archive_schema_mysql.sh
tests/async/run_personal_data_export_schema_mysql.sh
tests/async/run_account_erasure_schema_mysql.sh
tests/async/run_legacy_migration_mysql.sh
tests/async/run_immutable_migration_ledger_mysql.sh
tests/async/run_runtime_compatibility_mysql.sh
RUNTIME_DB_IMAGE=mariadb:10.11 tests/async/run_runtime_compatibility_mysql.sh
```

Each wrapper must create and destroy its own isolated container. Never replace its
target with `.env` values or use a configured, shared, restored, or production
database. The checked-in lifecycle policy keeps canonical archive, export, and erasure
mutation disabled; passing tests prove the guard and synthetic contract, not policy
approval.

## Phase 03 readiness boundary

Session 14 owns the final readiness claim. Its published outcome and remaining
limitations are recorded in `docs/records/readiness-report.md`.
Until that session qualifies isolated representative data, runs all eight profiles at
25/50/100/200 clients with each 200-client hold lasting at least 30 minutes, injects
the complete fault matrix, reconciles every durable domain after every run, validates
privacy/restore behavior, and publishes sanitized evidence, the repository is not qualified
as 200-player ready. That limitation remains explicit until the complete Session 14 evidence
exists.

Neither `make test-all`, `make test-db`, the Phase 01/02 logical-client gates, nor a
successful server boot may be used as a substitute for that evidence.

The checked-in contract and exact execution procedure are in
[`PHASE03_READINESS.md`](../gates/PHASE03_READINESS.md). Qualification uses a separate ignored
configuration and never reads `.env` implicitly:

```bash
python3 scripts/session14_gate.py \
  --config tmp/session14-gate/config.json \
  --preflight-only
```

`UNQUALIFIED` is a safe refusal, not a failed workload. `QUALIFIED` is also not
readiness evidence; only a complete `PASS` after every minimum-duration case can
support the claim.

## Conventions for new tests

- One concern per file; name it after the feature/regression
  (`test_<feature>.py`). The root runner discovers it automatically. Add a
  `run_<feature>.sh` wrapper only when the test needs special environment setup
  or is useful as a standalone workflow.
- Keep them fast and deterministic; prefer source contracts over full boots
  when the invariant is structural.
- When you change behavior, add or update the focused regression test next to
  it â€” this is a stated repo convention (see `AGENTS.md`).
- Schema-related changes should extend or add a `_schema_mysql` variant so the
  contract is checked against a real database on a clone.

## What exists today (samples)

| Area | Tests |
|------|-------|
| Persistence | `run_persistence_contract_mysql.sh`, dirty-flush retry, SQL pool shutdown, `test_player_corpse_persistence_contract.py`, `run_corpse_persistence_schema_mysql.sh` |
| Crash regressions | wear-all, relic pickup, stuck command gate |
| Saves | copyover save guards, ship save guards/dedup, epic save guards |
| Phase 01 recovery gate | `test_phase01_recovery_gate.py` drives 25/50/100/200 logical-client waves with ambiguous-commit retries and enforces fork/ownership/route contracts |
| Critical commands | `test_critical_command_coordinator.py` exercises identity, codec, journal corruption/replay, multi-key ordering, duplicate attachment, exact completion, retries, fences, bounds, and lifecycle |
| Critical transactions | `test_critical_transaction_contract.py` plus guarded `run_critical_command_schema_mysql.sh` cover schema, duplicate/mismatch, atomic rollback, concurrent locking, ambiguity lookup, outbox retry/dedupe/dead-letter/restart, and reconciliation |
| Help files | class/race helpfile completeness contracts |
| Event loop | hotspot budget regression |
| Build contract | `test_compiler_warning_profile.py`, `test_message_buffer_bounds.py` |
| Untrusted input | `test_unicode_runtime.py`, `test_ansi_runtime.py`, `test_json_utils_runtime.py`, `test_ttype_runtime.py` |

The four untrusted-input suites are behavioral rather than contract-style: they
exercise the live decoders that handle network and player-visible text â€” UTF-8
widths and malformed/overlong/surrogate encodings, ANSI colour parsing and
gradients, JSON and GMCP escaping, and RFC 1091 terminal-type negotiation
including MTTS capability parsing. That is the shape to copy when the code under
test is a parser: every case is a real call, and every defect found while
writing them (unbounded continuation runs, invalid UTF-8 re-encoded as `U+FFFD`
and accepted as a map glyph, `MTTS 4JUNK` silently enabling capabilities,
gradient application dereferencing `end()` on an empty string) stayed as a
regression.

`test_compiler_warning_profile.py` is what keeps the `-Werror` guarantee in
[BUILDING.md](BUILDING.md#warning-profile) real: it fails if any of the six
formerly-excepted categories is suppressed again, by flag, by a reintroduced
exception variable, or by pragma.
