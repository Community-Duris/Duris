# Test suite audit — October 2, 2026

The suite benefits from targeted consolidation and stronger assertions. A wholesale
framework migration or removal of slow persistence tests would lose useful coverage.
This change removes demonstrated duplicate execution, replaces two weak lifecycle
checks with executable assertions, repairs broken fixtures, includes missed tests,
and makes execution failures, skips and costs visible.

## Scope and per-entry decisions

The starting checkout was `5c157f693` on `experimental-accounting`. The final audit
branch incorporates the base through `e831cf7bf`, including its salvage regressions
and currency receipt work. Those production changes belong to the base branch.

The complete uninterrupted gate ran the combined code at `33fa12bbd199efc9f8719eed50feeef9805f5648`.
The exported fixture's tracked file contents were compared with that commit. Its
source stayed fixed throughout execution. Its single failure was upstream coin-
fixture formatting. The formatter then changed only whitespace in three lambda
expressions; replacement XML and a character comparison verified this, and the
affected native regression and complete formatting check passed after repair.

The qualified code is `3beb1d73c96bf201e4e70aadd65643df7bec01d7`. Its `src` tree remains
`3de65a8e0664ceb2229808528a319b7e208dd5b3`; its `tests` tree is `a82eb958a546a0722fd1d10de558be272e531cfd`.
Later audit-evidence edits change only documentation. Final coverage combines
the complete run with that narrowly scoped formatting follow-up; it is not
represented as an uninterrupted zero-failure full run.

The starting inventory had 859 Python entry scripts: 857 `test_*.py` files and two
`*_test.py` files. The old runner selected 832, excluded 25 with explicit manual
prerequisites, and overlooked the two suffix-named telemetry scripts. After three
retirements, two discovery additions and the three upstream additions, the current
inventory again has 859 entries; the normal gate selects 834 and the same 25 remain
explicit manual invocations.

- [TEST_SUITE_AUDIT.csv](TEST_SUITE_AUDIT.csv) gives a decision for every current
  entry and the three retired entries: 862 rows. It records declared intent,
  named cases, target literals, local helpers, Python assertion counts and
  diagnostics, evidence limits, baseline outcome/duration, current core outcome
  with its validation-run provenance, and additional native/SQL verification.
  Every core row cites its supplying full or focused report; formatting and its
  affected native driver cite their post-repair runs.
- [TEST_SUITE_SUPPORT.csv](TEST_SUITE_SUPPORT.csv) records 436 native fixtures, helpers,
  wrappers and root-level verification tools, with their textual owners. Input
  data files are assessed through their owning tests rather than counted as
  independent runnable tests.

The ledger combines static inspection with focused semantic review of discovery,
assertion helpers, duplicate orchestration, lifecycle fixtures, build caching,
broken baseline tests and changed native/SQL paths. Evidence categories are
inventory cues derived from code and local imports; they are not execution traces
or a claim that every assertion in every native fixture was independently mutation
tested. Python assertion counts omit assertions embedded in C++ harness strings
and checks implemented by helpers. A baseline `PASS` is the old runner's script
outcome and may include an optional runtime skip.

The general retain decision preserves distinct named cases and structural guards
where no redundant execution or invalid assertion was established. It does not
endorse a source-only test as proof of gameplay behavior. Shared targets alone do
not make tests redundant: read-only admission, crash replay, retry idempotency,
ownership refusal and post-reconnect behavior protect different failures. An AST
comparison found no identical complete Python entry modules.

Two documented cache SQL harnesses have fixed `cache_test` connection settings.
They remain manual probes requiring an explicitly isolated fixture, rather than
being scheduled without their prerequisites. Other native support files have
textual owners in tests, scripts or documentation; absence of a discovery entry
alone was not grounds for deletion.

## What changed and why

### Consolidate duplicate execution while preserving unique assertions

`test_double_entry_accounting_qualification.py` ran 15 independently discovered
scripts and repeated validator invocations. Its unique five-operation accounting
fixture now lives in `test_audit_accounting_invariants.py`, alongside the auditor's
existing positive and negative fixtures. All 15 standalone regressions remain
discoverable. The fixture proves the auditor accepts the modeled evidence; it does
not prove gameplay produced that evidence.

The golden-fixture test previously accepted a report with zero operations checked.
It now requires the expected distinct operation count and conservation count.
An exact-operation replay case also verifies that replay does not inflate any
reported count. The moved fixture's label identifies an auditor fixture rather
than an end-to-end gameplay journey. This auditor's item checks are limited: the
custody graph is checked separately, while event rows only require a UID. Full item
origin/effect-chain validation remains outside this synthetic test's evidence.

`test_player_load_pets.py` no longer launches the separately discovered item-load
test. The economic lifecycle activation contract no longer relaunches the separate
no-MySQL executable regression. Their pet and activation assertions remain.

The two retired pool lifecycle scripts only checked string presence.
`test_sql_pool_discard_recovery.py` now links the real production pool and asserts:

- exhaustion waits for the acquisition deadline and leaves the outstanding lease
  and capacity counters intact;
- an already-waiting borrower wakes on shutdown rather than waiting for timeout;
- shutdown keeps a borrowed connection open until its owner releases it;
- replacement is refused during closing and all opened connections are eventually
  closed.

The fixture wraps entry to `pthread_cond_timedwait` while calling the real wait.
That observes the waiter reaching synchronization without replacing synchronization
with a fake result. Its connection factory remains a fixture. The separate real
SQL pool test verifies server-session retirement and transaction rollback.

### Enable test cases that previously never ran

The suffix discovery fix includes 21 telemetry unit cases. A separate entry-point
audit found `test_training_dummy_contract.py` defined 15 function tests without
invoking them when launched as a plain Python script. The root gate reported that
empty execution as a pass. A unittest `load_tests` hook now runs all 15 without a
new framework. Enabling them exposed five stale checks against functions that had
moved to split combat and spell files. Those checks now inspect the actual
definitions, preserving refusal, retaliation and explicit-shield requirements.

A root-harness regression executes a private copy with the source provider returning
empty text. It requires an assertion failure and execution of every named case.
The old entry point silently accepted this mutation; the current one rejects it.
The two discovery fixes therefore restore 36 previously omitted cases.

### Make source expectations more honest

The previous shared matcher accepted a required guard when it appeared only in a
comment or quoted string. Code-shaped expectations now preserve lexical state.
Intentional SQL, log and user-visible text uses `literal=True`; plain names and
prose retain their explicit text behavior. Identifier/number boundaries survive
whitespace normalization, and searches retain original offsets and lexical state
when bounded partway through a file.

The scanner handles escaped quotes, raw strings, line-comment continuations and
C++ numeric digit separators. Shared extraction balances braces in code, preserves
quoted messages containing comment markers, and skips prototypes. Adversarial
fixtures cover these cases, including temporary source files used by the real
extraction helper. This remains a lexical helper, not a C++ parser; source presence
cannot establish reachability, runtime ordering or transactional effects. Direct
string checks outside these helpers retain their existing limits.

Canonical text and offset indexes are cached with a bounded cache. The 52 writer
classification cases share one immutable source census instead of rescanning the
same tree 34 times. Every classification assertion still runs.

### Make the runner observable and bounded

The runner discovers both naming conventions and deduplicates paths. The two
telemetry scripts contribute 21 previously omitted unit cases. Manual prerequisites
and serialization of resource-intensive tests remain explicit.

Ordinary scripts have a 900-second deadline; serialized resource scripts have
1,800 seconds. `TEST_TIMEOUT` / `--timeout` overrides these limits. A 30-second
heartbeat names active tests. Failures print immediately with their output.
Timeouts terminate a POSIX process group, then kill resistant descendants, including
ones that have closed stdout; output draining is also bounded. Windows uses
`taskkill /T /F`. Deliberately detached POSIX sessions lie outside the group.
Ctrl+C cancels active and queued work and returns 130.

An atomic JSON report records per-script status, exit code, elapsed seconds and
explicit skipped-check count. It excludes output and environment values. Whole
script skips, partial optional skips, failures, signals and timeouts are distinct.
The footer lists the slowest ten scripts. Seven executable harness tests cover real
child processes, CLI output, interruption, queued work, resistant descendants,
discovery, reporting and function-test execution; existing build contracts remain.

### Retain cache correctness while excluding irrelevant files

Server artifact keys still hash source and header contents, effective build flags,
environment, compiler tools, and directly searchable linker inputs. The linker
does not recursively search Python packages below `/usr/local/lib`, so those
unselected nested libraries no longer invalidate a server artifact. Explicit
nested `-L` and `LIBRARY_PATH` selections still participate by content. Header
search directories remain recursive.

Tests modify real temporary input bytes, including preserved-mtime changes, and
check nested-library exclusions and explicit inclusion. Existing corruption,
concurrent publication, environment and source invalidation checks remain. The
cache continues to reject a build if its inputs change during compilation.

### Repair fixtures without weakening their requirements

The original full gate reported 13 failed scripts. Repairs cover stale loader and
writer anchors, missing native journal/recovery dependencies, guarded accounting
fixture access, hardcoded `g++-14` assumptions, and two non-executable wrapper modes.
Loader contracts follow the actual shared executor and assert public delegation;
they retain their refusal, identity and read-only expectations. Real dependencies
replace missing links; unused sections are discarded where the fixture does not
exercise them.

The optional backup fixture had another hidden link failure because it reused the
qualifier's smaller source list while exercising item mutations. Craft restore and
backup tests now share `_restore_fixture.py`, with the required collector sources,
UID allocator dependency and test-access flags. It replaces the two drifting build
definitions. Privileged execution found two further fixture defects: seeded custody
items lacked a durable UID allocator, so the real server correctly refused boot;
and a migration-runner client invocation duplicated `--no-defaults`, so MariaDB
rejected it. The fixture now seeds complete authority and uses the runner's existing
client option. Both real isolated boot paths pass.

A backup case also expected every unreplayable partial player save to refuse
qualification. The current recovery design instead preserves that save in a durable
quarantine and fences its PID. The earlier boot failure concealed the stale assertion.
The revised case verifies the original WAL bytes survive, the active journal is
drained, missing PID 999 is not materialized and its real load is refused with
`EPERM`, while healthy PID 42 loads account/player/domain state. Corrupt player and
critical journals still refuse qualification. The backup run exercises all nine
cases, rather than treating a fixture compilation as restore evidence.

The combined-base gate found a stale auction inspector-isolation stand-in. The
journey now delegates compilation to `build_inspector`; its isolation test still
patched a removed `subprocess` import and failed before exercising its assertions.
The stand-in now intercepts the actual builder and retains all five cold-build,
supplied-server and failure-cleanup scenarios. Private-copy faults for a lost
inspector binding and shared destination are rejected by those assertions.

The complete gate also detected upstream lambda-layout drift in
`flatfile_accounting_coin_test.cpp`. The repository formatter changes only
whitespace in those three expressions. Its replacement XML and before/after
non-whitespace characters were checked; the complete formatting-tooling test
and the affected native coin regression then passed. No test assertion or server
source changed in that final repair.

Real SQL execution exposed an incorrect fixture expectation that a healthy load
must use exactly the maximum query budget. Optional empty receipt/obligation paths
legitimately use fewer queries. The fixture now enforces the PID/name ceiling and
independently counts actual `mysql_real_query` and prepared-statement executions,
requiring those counts to agree with loader metrics. Existing admission, read-only,
identity turnover and two-connection retention races remain. A SQL pool diagnostic
now reads the client error after the query, avoiding C++ argument-evaluation order
that could print an obsolete error code. Its timeout is unchanged.

## Proof and measured cost

Validation uses exported source snapshots in owned Linux containers with four
CPUs, Python 3.12 and GCC 13. Initial measurements used 4 GiB RAM; final merge
qualification used 5 GiB. Database checks use fresh owned MariaDB 10.11.19 and
MySQL 8.0.46 containers with loopback-only disposable fixtures. The privileged
backup suite additionally starts its own isolated MariaDB 10.11 server and private
service/network namespaces. No repository `.env`, existing game account, running
game, or production database is involved.

| Check | Result |
| --- | --- |
| Original `make test-all -j2 TEST_JOBS=2` | 819 passing scripts, 13 failures; Python phase 5,278.73 seconds |
| Revised full run | 817 passing scripts, 11 explicit skips, three failures; Python phase 5,348.11 seconds |
| Earlier revised run plus follow-ups | Historical composite: 834 core entries accounted for, 823 passed and 11 explicit skips |
| Complete uninterrupted `make test-all -j2 TEST_JOBS=2 TEST_REPORT=bin/test-audit/merge-ready-results.json` | 822 passed, 11 explicit whole-script skips, one upstream fixture-formatting failure; Python phase 4,739.16 seconds; maintained builds, world generation and native targets passed |
| Formatting-only repair | Full formatting-tooling check passed in 29.57 seconds; coin-focused `make test-all` passed its native regression in 139.55 seconds and passed maintained build/world/native targets |
| Qualified current core coverage | All 834 entries accounted for: 823 passed, 11 explicit whole-script skips, zero unresolved failures; complete-run plus explicit formatting follow-up provenance is recorded per row |
| Focused source-helper consumers | 82 scripts passed |
| Current harness behavior | Seven real process/CLI/entry-point cases passed |
| Previously silent training-dummy script | All 15 structural cases executed and passed |
| Accounting auditor | Eleven cases passed, including exact replay and the relocated fixture |
| Integrated base additions | All three salvage regressions passed; 52 writer classification, three census and 29 item-admission cases passed; generated matrix and accounting validator checks passed |
| Final real SQL load and conflict fixtures | Six unittest executions passed across both engines; load coverage includes 30 ordered cases and eight two-connection ordering scenarios per engine |
| Real SQL pool repetitions | 20 complete runs passed, ten fresh empty schemas per engine during concurrent native compilation; original three-second client deadlines retained |
| Auction inspector isolation | Five orchestration scenarios passed; actual private-copy binding/destination faults both rejected |
| Current currency/coin SQL harness | Maintained native harness passed against both engines, including balanced posting/replay/rollback and split/merge; this is the wrapper's native harness, not its entire schema-damage/loader matrix |
| Privileged backup integration | All nine standalone opt-in cases passed in 151.677 seconds, including actual isolated flat-file/MariaDB boot, replay, corruption refusal and quarantine admission checks |
| Restore fixture | Shared backup fixture compiled and executed native craft recovery; craft restore passed valid-state qualification and all six corruption/refusal variants |
| Maintained server build and touched C++ formatting | The final root gate's `make -C src` and maintained tool builds passed; formatting checks passed for `player_death_recovery_mysql_harness.cpp` and `persistence_restore_fixture.cpp` |
| Final documentation | The maintained documentation-contract test passed against this updated audit and the corrected backup guide |
| Harness-focused root gate | `make test-all -j2 TEST_JOBS=2 TEST_MATCH=root_test_harness` passed, including maintained build/world/native targets and the current harness |

The earlier revised full run selected 831 entries before the three upstream salvage additions were
integrated. Its failures were two native-fixture drivers requiring explicit literal
SQL/log matching and an account journey whose cached build was rejected because
the source changed during upstream integration. Both native drivers passed after
repair; the stable account journey passed on retry. The current-base salvage and
changed-entry supplements passed. That earlier composite recorded each script's supplying run.
It is retained above
as historical evidence. The complete gate executes all 834 entries on the
combined base and final behavioral test code. Its single fixture-formatting
failure is repaired and verified separately; the record preserves both outcomes.

Three preliminary merge-qualification gates were deliberately interrupted: two
when privileged backup execution required fixture corrections, and one after the
combined-base gate exposed the stale auction stand-in. Their partial outcomes
are not counted as complete qualification. The completed run contains no cancellations
or source changes. Its JSON still records optional skipped checks within passing
scripts; the eleven whole-script skips are not counted as passes. The final
whitespace repair has no behavioral change, so its native consumer and formatting
tooling were rerun rather than repeating all 834 entries.

Twelve deliberate faults were rejected:

| Injected fault | Observed rejection |
| --- | --- |
| Remove pool closing broadcast | Waiting borrower returned through deadline expiry instead of a wakeup |
| Close an outstanding borrowed handle during shutdown | Lease ownership/shutdown-completion assertion failed |
| Return immediately on pool exhaustion | Minimum wait assertion failed |
| Put required guard only in a comment | Old matcher accepted it; new matcher rejected it |
| Put required guard only in an ordinary string | Old matcher accepted it; new matcher rejected it |
| Put required guard only in a raw string | Old matcher accepted it; new matcher rejected it |
| Restore recursive linker-directory fingerprinting | Actual-byte fixture rejected invalidation by an unsearched nested package |
| Add an unmetered real SQL `SELECT 1` | Actual execution count was 34 while metrics reported 33; observer rejected the mismatch |
| Have the auditor report zero checked operations for every golden fixture | Old positive test accepted it; revised test failed an assertion |
| Give the training-dummy script empty source input | Old entry point exited successfully without running cases; current entry point ran all 15 and failed assertions |
| Lose the auction journey's private inspector binding | Corrected isolation driver rejected the wrong inspector during the mocked journey |
| Compile the auction inspector into a shared destination | Corrected isolation driver rejected the destination before running the journey |

Native mutations compile private copies through the same harness and retain real
synchronization/SQL execution. Source and helper mutations use the actual helper
and existing assertions. Repository production sources are not altered by these
experiments. These are demonstrated regression-detection capabilities, not twelve
new production defects.

| Controlled comparison | Before | After | Scope |
| --- | ---: | ---: | --- |
| 52 writer-coverage cases, same source and registry at measurement | 111.17 s | 3.43 s | Both runs passed; repeated scans reduced from 34 to one |
| 80 matched/indexed lookups in the same `comm.c` | 10.69 s | 0.082 s | About 130 times faster for this repeated-lookup workload |

The retired qualification orchestrator alone took 299.11 seconds in the original
gate; its surviving auditor fixture adds negligible cost to an existing test.
Full gate runs overlapped other validation and select different cases after repairs
and discovery changes. Their wall times are observations, not a controlled overall
speedup percentage. Native compilation, fsync/crash recovery and gameplay journeys
remain substantial costs. Removing those assertions to improve a headline runtime
would weaken coverage.

One additional SQL pool repetition failed at table creation during concurrent
validation; its original diagnostic reported the pre-query error code `0`. Fresh
fixtures then passed on both engines after fixing diagnostic ordering. Final
qualification repeated the complete test twenty times, ten per engine, each in
a fresh empty schema while other tests compiled. All passed with the original
three-second client deadlines. That stress did not reproduce the historical DDL
failure; its original cause remains unknown. A better diagnostic and passing
repetitions do not establish the cause of the earlier failure.

## Reproduction and remaining limits

Use the commands in [TESTING.md](../guides/TESTING.md), with build dependencies
available:

```sh
make test-all -j2 TEST_JOBS=2 TEST_REPORT=bin/test-audit/merge-ready-results.json
python3 tests/async/test_root_test_harness.py
python3 tests/async/test_contract_text.py
python3 tests/async/test_server_build_artifacts.py
python3 tests/async/test_sql_pool_discard_recovery.py
python3 tests/async/test_audit_accounting_invariants.py
python3 tests/async/test_craft_progression_restore.py
```

Real SQL fixtures require their documented empty/migrated disposable schemas and
explicit opt-in environment. Final qualification executed the real pool, load,
conflict and currency/coin harnesses on both engines. The complete `make test-db`
matrix and all 25 manual entries were not run. Those unrelated matrix rows remain
outside this test-suite change's qualification. A source-contract pass is not
gameplay evidence, and a compile followed by a runtime skip is not SQL integration
evidence.

The privileged backup command was executed with real MariaDB/native builds,
`unshare`, private mount/network namespaces and the required capabilities in an
owned disposable container:

```sh
DURIS_RUN_BACKUP_INTEGRATION=1 python3 tests/async/test_persistence_backup_integration.py -v
```

The final JSON, full console log, SQL repetition results and backup transcript
are retained as local ignored artifacts under `bin/test-audit/`. They contain
fixture evidence and are not committed.

The next useful performance work is driven by the JSON timings: identify repeated
native compilation with truly identical flags and dependencies, then share only
verified immutable build inputs. Fixture definitions can be unified when they
already have two callers, as with restore. Promote source-only behavioral promises
to executable cases when their expected refusal, state transition or concurrency
ordering can be exercised. Keep structural tests for registration, forbidden
dependencies and stable architecture constraints. There is no evidence here that
changing framework or deleting tests based on their author would improve correctness.
