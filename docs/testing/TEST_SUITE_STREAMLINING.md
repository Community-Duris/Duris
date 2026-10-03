# Test suite streamlining

This follow-up to [the suite audit](TEST_SUITE_AUDIT.md) implements execution
accounting, explicit profiles, native artifact reuse and resource scheduling.
It also upgrades five behavioral pilots and supplies the disposable database and
recovery matrix. Qualification also exposed and repaired an expired-deadline
shutdown scheduling bug that could lose a live copyover callback, a legacy
migration replay that reapplied old schema definitions after immutable adoption,
food consumption that removed a live item while leaving its SQL custody active,
and permanent account deletion that left retained quest-name aliases behind.
The food defect made the next inventory save fail its ownership guard. Account
deletion now commits quest erasure and character identities together through the
native backend's journal or SQL transaction. A persistence refusal preserves the
irreversible fence and those identities; retry completes after recovery. The
account menu does not rewrite quest state before that atomic boundary.
The operational persistence verifier now recognizes equivalent MySQL/MariaDB
metadata while retaining rejection of signed UIDs and incorrect defaults. Its
disposable tests include corruption, rejection, guarded repair and data preservation.

## Execution and profiles

[The inventory](../../tests/regression_manifest.json) explicitly classifies every
Python entry by purpose, execution mode, minimum unittest case count, profile,
estimated duration, CPU/memory reservations, and manual prerequisite where needed.
A newly discovered or missing file fails inventory validation, including filtered
runs. Metadata changes are intentional review decisions, rather than a naming
heuristic that silently decides whether to run a new test.

The default core profile retains the independent automatic owners and adds the
native-artifact regression. The obsolete S05 source guard that pinned an exact
four-migration list is replaced by the required item-flags SQL journey, using the
maintained migration owner and actual save/reconnect assertions on both engines.
The obsolete mushroom source-order guard required saving before extraction.
Its replacement executes actual food effects and admission/publication functions:
refused admission or commit grants no benefit, successful retirement consumes one
UID, duplicate publication does not repeat the effect, and a level mushroom saves
only after extraction. The SQL player journey additionally requires a destruction
tombstone, one ownership-ledger entry and no reappearance after restart/replay.
Active-accounting food acceptance and interrupted effect publication remain outside
this qualification.

The inventory now contains 891 Python entries: 866 automatic and 25 manual.
The 25 manual entries have explicit providers in the integration matrix. Automatic entries
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

The clock regression executes the production shutdown scheduling branches at eight
expired/immediate deadline and online/offline issuer boundaries. It rejects the
original negative delay that lost copyover callbacks, and preserves restoration
and the positive follow-up after the repair.

The runner launches each entry through a private adapter while preserving its
arguments, working directory and import path. Unittest results include case names,
outcomes, durations and skip reasons. Collected and executed identities must agree;
a successful zero-case suite or a failed suite with a successful process exit is
rejected. Observed skips remain visible when unittest redirects its console
summary, including subtests. Class/module cleanup failures are retained even if
the script ignores the returned unittest result. Successful entries must
acknowledge completed observation; os._exit(0)
cannot bypass final validation, including after successful cases. A recorded
minimum protects against accidental removal from a suite.

Standalone drivers are explicitly one entry-level case. Their native assertions
are not presented as independently observed Python cases. Plain declared function
tests must be invoked, and a declared main function must run. Function invocation
records are evidence of invocation; a failed script leaves their assertion outcomes
unresolved. The adapter does not establish that every C++ assertion is reached.
That evidence requires the behavioral work below.

JSON records are atomically updated after each completed entry, with pending
entries retained. Selected and excluded identities and the inventory checksum
make filtered scope explicit. Reports omit captured output and environment values; skip
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
The inspector takes the authority lock before reading and refuses all pending
recovery journals. It cannot complete recovery while observing a failed journey.
A valid native after-image regression requires byte-preserving refusal; the old
inspector installed that image and returned success. The combat journey relies
on this locked observer instead of racing an unlocked global journal check
against unrelated world/corpse cleanup.

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

The legacy upgrade retains its full first-pass checks. Once a baseline is adopted,
replay uses the validating immutable runner and the current schema contract. It
retains locker row counts, imported extensions and archive values, and compares
the upgraded schema with a fresh bootstrap. A missing equipment slot must be
rejected without legacy repair or migration-history changes. Replay attempts keep
unique, redacted diagnostic logs, including when the driver fails.
Missing adoption rows also remain under the immutable owner if history rows or a
nonzero history-state marker remain; neither corruption can restart legacy DDL.

The current workload has 148 required identities: 66 on each pinned engine and
16 shared rows. It includes the real SQL pool's bank, coin and item coordination,
literal checkpoint recovery and exact room payload recovery. The room row reuses
its qualified seed executable for two complete SQL cold boots and requires the
native publication witness. Literal checkpoint qualification compiles and pins
its executable before exercising the supplied disposable schema.

The currency wrapper no longer rebuilds and reruns the complete item-transfer
executable after mutating its schema with unrelated coin and loader fixtures.
The dedicated item-transfer row remains required on both engines and keeps every
native assertion. The four archive wrappers that used socket readiness now wait
for TCP; the pinned MySQL initializer starts a temporary socket-only server before
its actual service. The existing 90-second readiness limit remains unchanged.

The shared SQL owner creates only new labelled containers with generated
credentials. A container runner shares only its own validated network namespace;
a native Linux runner publishes exclusively on loopback. Legacy wrappers retain
their existing SQL harnesses and now receive the selected engine. Original client
errors, setup/query events, source and synthetic journal digests, and outcomes are
written before cleanup. Credentials are redacted. Every attempt gets a new evidence
directory; a passing rerun cannot replace the original failure.

SQL pool metadata observations respect MySQL 8.0.46's 100 ms transaction-cache
idle interval, documented in [the pinned server implementation](https://github.com/mysql/mysql-server/blob/mysql-8.0.46/storage/innobase/trx/trx0i_s.cc).
The retirement deadline remains five seconds; lease exclusion, rollback and
replacement-connection assertions remain required. Faster metadata polling could
prevent refresh and report a transaction after its session had closed.

The same command runs locally and in the development/master branch
[workflow](../../.github/workflows/integration-matrix.yml):

~~~sh
python3 tests/run_integration_matrix.py --list
make test-integration
make test-integration TEST_ENGINE=mariadb TEST_MATCH=player_death_recovery
python3 tests/qualify_behavioral_faults.py --family offline \
  --evidence-dir bin/behavioral-faults-new-attempt
~~~

The death/resurrection matrix row explicitly selects `--legacy-persistence`
because its fresh schema has accounting inactive. It executes real combat death,
reconnection and resurrection, verifies exact item custody and wallet restoration,
and retires the coin pile. Before and after the journey it requires empty activation,
installation and accounting evidence tables. Every emitted legacy event must remain
uncovered; partial accounting is a failure. The row requires both the persistence
completion witness and `RELEASE ACCOUNTING COVERAGE BLOCKED:` with actual gap counts.

The journey's default mode remains the strict accounting release acceptance. It
fails if an item event lacks an accounting reference or a currency operation lacks
a balanced root and postings. A passing inactive persistence row supplies no
accounting release qualification. The original strict failures remain retained
evidence, and enabling accounting for this journey requires separate implementation
and successful strict acceptance.

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

Recovery explicitly requires namespace/mount capabilities and all ten backup
cases. Both private restore initializers and live daemons disable native InnoDB
AIO, and the MySQL daemon disables its extra X listener. A retained native MySQL
execution failed its second initialization with io_setup EAGAIN and data-dictionary
abort when the shared host kernel exhausted its AIO pool. The new flags avoid that
reservation while preserving the restore owner's schema, value, replay, corruption
and isolated service-boot requirements. The tools image keeps MySQL message files
under its actual basedir lookup and records the resolved restore executable's
version and SHA-256 in each matrix freeze. The copied-journal owner now generates a synthetic capture with real
codec/journal APIs and matching hashed manifests; historical private captures
remain supported only with all four explicit inputs. No configured game, existing
account, shared database or production state is selected. Core retains visible
optional SQL/systemd skips; the complete matrix requires its declared integration
checks to execute. Local executable verification remains the merge evidence;
AGENTS.md does not require waiting for CI.

## Qualification through 22514903d, before final review

Qualification uses private Git exports in owned Linux containers. Each imported
commit and tree is verified before execution; unchanged source mtimes preserve
incremental builds. Maintained world inputs are generated there. No configured
account, checkout .env, shared game or production database is selected.

That inventory declared 885 entries: 861 automatic and 24 manual, with 372 in fast.
That integration workload had 138 required rows: 61 per SQL engine and
16 shared rows. That qualification incorporated a bounded target snapshot through
a209af827, including persistent transport, resumable native compilation,
transactional SQL quest erasure, native account-fence faults, network readiness,
portable copyover, quest allocation faults and Telnet fragmentation checks.
The retired select ceiling guard now protects connection admission before TLS;
the native readiness owner uses a socket above FD_SETSIZE. Boot ordering protects
recovery before the first connection phase, and latency uses the current monotonic
deadline.

Results retain their source boundaries. They are not one complete green core or
integration run at the final review commit.

| Source cohort | Scope | Recorded outcome |
| --- | --- | --- |
| be73f72c69 | Complete core Python stage and maintained builds | All 849 automatic entries attempted in 4,903.46 s: 832 passed, six failed, 11 whole entries skipped. Maintained world, editor, SQL server and native stages passed. |
| 18d69b437 | Explicit compatibility subset and full fast profile | All 32 selected entries passed without skips in 958.89 s; all 370 fast entries passed in 56.52 s, with four explicit partial integration skips. Selection and clean-source records retain the exclusions. |
| d2b747fa6 | Explicit native account/storage/quest subset | All 12 entries passed without skips in 520.74 s; character deletion used ASan and UBSan and passed in 172.25 s. |
| f88aa2a54 | Account cleanup and Telnet compatibility | All 370 fast entries passed in 39.51 s with four partial integration skips; the three focused account/network owners passed in 308.38 s. Both maintained server backends, formatting, and ordinary/durable/uncertain flat-file deletion journeys also passed. |
| 2f4c9e6b6 | Expanded required SQL account journey | All 370 fast entries passed in 45.87 s with four partial integration skips. Both pinned engines passed the strengthened real account/character journey, including quest persistence refusal, restart, permanent erasure and safe name reuse. |
| d9146e0b7 | Native alias journaling and character maintenance target integration | All 370 fast entries passed in 43.76 s with four partial integration skips. Nine affected entries passed without skips in 741.18 s, including deadline maintenance, ASan/UBSan character deletion and all three combat variants. Both maintained server backends, three flat-file deletion journeys and both SQL deletion rows passed. |
| 61ef72381 | Bounded transport integration and native atomic account erasure | Maintained setup, formatting and both server backends passed. All 372 fast entries passed in 54.44 s with four partial integration skips; all 14 affected native/cache owners passed without skips in 166.96 s. All 16 transport witnesses passed in 721.61 s, three combat variants passed in 426.87 s, ordinary flat-file deletion passed and all eight selected SQL rows passed on both pinned engines. Two obsolete recovery fault injections failed and remain retained. |
| ec1aa340c | Corrected native recovery fault injection | Durable and uncertain flat-file recovery passed in 65.95 s and 65.91 s. The only change from 61ef72381 is the deletion test; the production tree and all three frozen executable hashes remain identical. |
| 5f4c2b7c8 | Complete MariaDB engine workload | All 61 required rows passed, with zero skips and no pending workload. |
| cec2329ac | Complete MySQL engine workload | All 61 rows attempted: 60 passed and the earlier staff fixture failed, with zero skips or pending rows. Revised staff recovery passed on both engines in the later cohorts. |
| be73f72c69 | Complete shared workload | 15 of 16 rows passed; the MySQL backup case exposed host AIO exhaustion. |
| 7a532a685 | Revised staff and backup owners | Both engine staff journeys passed; the backup owner passed all ten required cases, including genuine MySQL 8.0.46 restore. |
| 4f2fa5138 | Eight refreshed compatibility journeys | All eight passed without skips: playtime, SQL copyover, Issue331 player and staff recovery on both engines. |

The original core report contains 30 skipped checks: the 11 whole-entry skips and
partial integration skips inside five passing entries. Its six failures were the
copyover watchdog link, writer line anchors, launcher watchdog script, formatting
against an older target tree, retired checkpoint placement and an incomplete
command-queue initializer. Each original failure remains retained. The fast and
explicit 32-entry reruns at 18d69b437 close all six. That subset also passes all
21 real-process runner cases, six real-compiler cache cases, 52 writer cases,
three census cases, production food/deadline fixtures and the new network owners.
Its combat driver passes all three variants in 894.42 s, including a fresh server
build. Opaque native entry counts do not enumerate internal C++ assertions.

The fast reports retain four partial skips inside two otherwise passing entries:
the doctor's real SQL authority check, the restitution backup round trip and two
non-root systemd ownership checks. They have no whole-entry skips or pending
entries. The evidence index matches each skipped case to its passing required
integration provider; it does not turn a partial skip into a fast-profile pass.
Earlier wording that described these fast runs as having no skips was incorrect.

The v30 fast attempt started before source import completed. It is retained as
preliminary and excluded. The subsequent v31 run verifies the same clean commit
and tree before and after execution. JSON/XML, selection records, actual
qualification drivers and per-step provenance keep these distinctions reviewable.

At d2b747fa6, both maintained server backends built and ordinary character
deletion passed. The durable and uncertain account-fence crash/recovery journeys
both failed because permanent account deletion retained the global quest-name
alias after removing the player and account. The prior confirmation fixture
captured an empty identity vector and missed this defect. Its first replacement
at f88aa2a54 supplies two stable PIDs, refuses the second quest erasure, requires no destructive backend
call on refusal, preserves the non-cancellable fence, then verifies idempotent
cleanup and exactly one completion on retry. It fails against the original
production function and passes with the repair under ASan and UBSan.

The target later added alias erasure to native flat-file journals and the SQL
account deletion transaction. Pre-erasing aliases from the account menu would
then bypass validation of persisted state and prevent a late native failure from
rolling aliases back with player data. The final implementation removes that
duplicate cleanup. Its strengthened production-function fixture rejects the
previous 711bde72a owner with "account menu rewrote quest aliases outside the
native erasure transaction". It retains two captured PIDs, irreversible fencing,
refusal and retry assertions while requiring all erasure to stay inside the
backend's atomic boundary. Native repository and real-server journeys qualify
the backend behavior separately.

The repaired flat-file journeys refuse unsafe metadata on the actual native
quest-state file, require unchanged account identities, snapshot and aliases, cold restart and retry,
and verify erasure after a further restart. The SQL owner refuses actual persistence
by withholding only its synthetic schema's quest table. It checks the durable
fence and retained identities, restarts, completes permanent deletion, then safely
reuses the name with a distinct PID before running the original character-deletion
rollback/retry checks. The matrix requires both the account-cleanup witness and
the real native whole-account journey on both engines; the row count stays 138.
The latter refuses corrupt or stale quest state and failed quest writes, injects
a late player-deletion failure and requires byte-identical alias rollback, then
verifies repaired retry and cold restart. Both native cache refresh journeys
require a successful subsequent Observer alias publication, so disabling tracking
cannot satisfy erasure checks.

The v37 durable/uncertain attempts failed because they still changed the runtime
publisher's lock, which native journal preparation does not use. Those failures
remain preserved. The v38 correction changes the quest-state file's metadata,
which the actual native reader must reject, and restores its original mode before
recovery. Both journeys pass their retained-identity, unchanged-byte,
non-cancellable fence, restart/retry, erasure and later-publication assertions.
Git proves that only this test changed; the production subtree and frozen binary
hashes match the completed v37 qualification.

The final Windows observer run retained 52 passing writer cases and three census
cases, but all 12 documentation cases failed while decoding UTF-8 Markdown using
the default Windows code page. The earlier direct `-X utf8` invocation had hidden
that portability defect. The documentation owner now declares UTF-8 on all
repository text reads and passes under the default Windows interpreter. Its
failed v39 report remains retained beside the final observed rerun.

The evidence index accepts historical manifests only after checking that every
requirement is identical except for the two added SQL account-cleanup witnesses.
Earlier passing deletion rows cannot supply it: the strengthened owner must pass
on both engines. Every selected positive row must have zero skips, all named cases,
the required native witnesses, and matching hashed original/result logs. The index
records earlier failures and the source revision of each selected result. Verification
finds passing evidence for all 138 required identities: 61 MySQL, 61 MariaDB and
16 shared rows. Every required case and completion witness is present, with zero
skips or pending coverage in the selected positive records. Each of the 33
behavioral control/fault phases also has its own validated outcome and log hash.
The selected positives span six explicit source cohorts, including the eight
refreshed SQL rows at 61ef72381. This is not a single complete matrix run
at the final review commit.

A separate native probe starts two authenticated MySQL 8.0.46 daemons concurrently
with native AIO and the extra X listener disabled. The original failed backup and
initialization diagnostics remain retained. The QA container keeps its original
image identity; separately installed native tool payloads record their archive,
executable hashes and versions. A transferred tools payload is not reported as
the QA container's image.

The writer classification comparison with the target differs only in this PR's
explicit food-consumption route. After the bounded transport integration through
a209af827, 232 moved observations are remapped with identical source-block or
unique-excerpt proofs. Handler owner checks locate actual functions and operations
instead of fixed source lines. All 52 writer cases and three census cases pass.
The census remains 2,816
occurrences and 2,757 mapped unique sites, with zero unmatched sites. Accounting
release coverage remains blocked.

Both inactive-accounting death/resurrection engine rows pass their gameplay and
persistence assertions and report 27 uncovered item events and two unaccounted
currency operations. The retained strict runs reject that gap. A passing inactive
row does not qualify accounting release. Active-accounting food acceptance and
interrupted effect publication remain unqualified.

The unchanged player-repository driver, including native and quarantine recovery,
was measured at f66e715df225c68110faf8e696dd4886e5f91904 under the same frozen
inputs, compiler/options, environment and container limits with a private cache:

| Build setting | Entire entry | Compilation | Linking | Lookup |
| --- | ---: | ---: | ---: | ---: |
| off | 65.77 s | 52.29 s aggregate build/lookup | included | included |
| cold | 42.77 s | 25.58 s | 0.46 s | 4.01 s |
| warm | 15.14 s | 0 s | 0 s | 1.98 s |

All three attempts pass. The warm run reuses all 55 inspector objects and runs the
existing runtime assertions. Runtime/setup/remainder stays approximately 13 s;
the removed work is compilation. These measurements qualify this fixture
improvement, not a controlled before/after comparison of the complete suite.

The cache's early-publication fault rejects stale value 99 when reverted source
requires 42. Runner regressions reject zero-case collection, disconnected entry
points, ignored unittest failures, hidden skips and premature successful exits.
Real child processes qualify resource overlap, locks, duration priority, timeout
cleanup and cancellation. Eight reviewed fault families supply 33
before/fault/after phases across the offline family and both SQL engines. This
scoped detection is not a suite-wide mutation score.

Core's skips identify prerequisites supplied by the separate required integration
matrix. The 24 manual entries remain excluded from core. A filtered run records
its excluded identities and qualifies only its selected scope. Earlier historical
core and benchmark attempts remain retained without being promoted to current
whole-suite evidence.

Raw JSON/XML/logs, qualification drivers and the source-bound review index remain
ignored artifacts under bin/streamline-review-evidence, with earlier benchmark
material under bin/streamline. All exported bytes are SHA-256 verified before
owned qualification containers are removed. The review branch remains a draft;
no PR merge or auto-merge is performed.

The target continued advancing after the bounded a209af827 integration. Final
review uses the frozen source cohorts above; later SQL pool and commit-reply-loss
changes require branch synchronization and focused qualification before an
eventual merge. The PR remains open for review with that boundary explicit.
