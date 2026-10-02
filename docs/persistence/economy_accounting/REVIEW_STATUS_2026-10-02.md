# Accounting review status - 2026-10-02

The review checkout integrates `experimental-accounting` through
`2a647b6ec7b21fba79f58b7af8fbc1cf90ec78da`, including retained coin-recovery
receipts and the test-suite audit. Accounting activation and release remain
blocked. The [completion plan](FINISH_ACCOUNTING_PLAN.md) retains the remaining
R1-R8 work; the [October 1 status](REVIEW_STATUS_2026-10-01.md) retains earlier
qualification and its limits.

## Spell component replay repair

Commit `05f3293ae5df0c2b1f4655f7ae55955127db3a77` restores selected component
roots instead of treating every descendant in a retained forest as another
component. Live admission already records root UIDs, while retained item commands
contain the whole forest. The old replay path could reject a valid faerie-sight
forest or retain publication indefinitely because a descendant is not a
player-carried root. The restored callback now receives the original roots;
the movement owner still validates the complete forest before publication.

Vines replay also validates the frozen herb count against the restored roots
and its existing one-to-four limit. The actual effect callback independently
rejects out-of-range counts before multiplying them into shield strength or
requesting a save. Before repair, the executable restore regression accepted
an invalid count. Valid effects, both continuation encodings, and operation
receipt/ACK semantics are retained. Inactive-accounting spell behavior and the
global epoch gate are unchanged.

The focused ASan/UBSan fixture exercises the production restore function and
vines effect callback: zero/negative/excessive counts, mismatched counts,
truncated/trailing contexts, valid one-to-four effects, repeated callbacks,
one root with nine descendants, duplicate/zero UIDs, incorrect root identity,
no roots, and excess roots. Existing forest publication, exact-count admission,
and faerie-sight retirement regressions also pass. These are native component
proofs; integrated active-epoch server restart and save/custody race journeys
remain open.

## Build and integration evidence

Both strict production profiles pass after the repair. All 1,212 native-build
source blobs match the staged source tree committed as `05f3293ae`.

| Profile | Executable SHA-256 |
| --- | --- |
| SQL | `0d5cbec06aae32f4ffa9ec091a90387821c050e69fc8077030bfae2827adb57d` |
| Flatfile | `1c850641b8aef9492955093f160b13f39e5a96abe7aa4385b118da5422676f30` |

Changed C++ files pass clang-format 18.1.3. Incoming currency completion
retention passes 106 sanitizer scenarios; selective pending-input handling
passes on SQL and flatfile. The native flatfile coin fixture passes wallet/pile
operations, retained replay, stale rejection, split-child recovery, and
interrupted commit recovery. Root-runner and source-parser regressions pass.
The first root-runner invocation stopped because the disposable checkout lacked
the area-editor Makefile; it passes after copying the required repository inputs.

The upstream [test audit](../../testing/TEST_SUITE_AUDIT.md) records combined
full-run and focused formatting evidence for source tree
`3de65a8e0664ceb2229808528a319b7e208dd5b3` and test tree
`a82eb958a546a0722fd1d10de558be272e531cfd`. Those match the integrated
`2a647b6ec` base. They do not constitute a full regression run of the new spell
repair. Its focused evidence is recorded above.

The refreshed inventory remains 864 routes, with 2,815 lexical occurrences,
2,756 unique sites, and zero unmapped sites. All 52 writer contracts and 14
accounting fixtures pass. The 751 runtime/projection routes still lack complete
route qualification; `coverage_complete=False` and `release=BLOCKED` remain.
No production repair, migration, activation, or deployment has occurred.

## Fixture auditor denomination repair

The fixture auditor now requires integer denominations and checked signed
64-bit denomination/copper bounds. Its previous `int()` conversion silently
truncated fractional postings and accepted booleans, numeric strings, and
oversized opening balances. The new regression reproduces those failures
before repair and passes afterward, including exact integers above `2**53`,
signed endpoints, and denomination-weighted copper overflow. All 13 auditor
tests and the 14 golden fixtures pass. This strengthens synthetic audit
evidence; it does not qualify native money writers or complete independent
runtime reconciliation.

The fixture auditor also rejects missing, non-string, or zero root/child
operation IDs. Previously a zero root ID and malformed child IDs passed,
while non-string root IDs raised an unhandled parser error. Negative cases
reproduce that gap before repair; the updated 16-test suite passes and CLI
refusals return a bounded audit diagnostic without a traceback. This matches
the native critical-command nonzero-identity rule without claiming complete
child evidence or item-history reconciliation.

## Bandage save/restart qualification

The real mortal bandage journey passes on disposable MySQL 8.0.46 and
MariaDB 10.11.14 using the SQL artifact above. Its previous restart assertion
only compared surviving counts. The fixture now requires the original surviving
bandage UID set in both native custody and the saved player projection, one
original UID tombstone, and exactly one retirement operation/revision unchanged
through two cold restarts. Both engines pass those stronger assertions and the
existing bandage custody contract passes. Accounting remains inactive in this
journey; active-epoch evidence and interruption during consumption/publication
remain open. It does not prove persistence of the NPC healing effect.

## Independent native topology audit repair

The independent reconciler previously skipped its missing-parent check when an
item had lineage UID history or lacked a known opening origin. A current row
and immutable history could agree on a nonexistent parent without reporting
`orphan_item_parent`. The check now runs independently for every native direct
parent edge, including retained tombstones, before ancestor cycle/root checks.
It reports the offending child once even when descendants traverse that edge.

New regressions reproduce five failures before repair and all 41 reconciler
tests pass afterward. They cover history-scoped and ordinary UIDs, missing and
known origins, nested valid custody, missing ancestors, and tombstones. An
actual SQL corruption probe also reproduces the omission on disposable
MariaDB before repair. The complete read-only snapshot probe passes after repair
on MariaDB 10.11.14 and MySQL 8.0.46: consistent current/history rows with an
absent parent report the orphan, and restoring those fixture rows returns the
original exception set. Existing SELECT-only reader, consistent-cut, CLI,
mapping, source, and ledger corruption checks also pass.

This fixes an R7 diagnostic gap. The SQL exporter still marks its output
`complete=false`; source/origin completeness, gameplay writers, integrated
recovery, and release qualification remain open. Native gameplay behavior and
accounting activation are unchanged.

## Independent UID lifetime audit repair

Lineage UID history previously checked revision continuity and final native
state but bypassed the epoch-local creation checks. A second creation of an
already-created UID, a missing first creation, or a malformed creation origin
could escape the lifetime audit. Tombstoned UIDs could also become live again
without a dedicated lifetime exception. A shared read-only lifetime check now
validates creation origins, requires creation evidence for new UIDs, reports
duplicate creation, and reports `resurrected_item_uid` when retained retirement
is followed by a live state. Both lineage and epoch-local histories use it.
Normal creation, movement, retirement, and baseline-live movement remain valid.

Five new negative cases fail before repair; all 47 reconciler tests and six
opening-origin tests pass afterward. The disposable MariaDB SQL probe also
reproduces acceptance of a duplicate creation before repair despite consistent
roots, references, revisions, and native state. The expanded read-only SQL
snapshot probe passes on MariaDB 10.11.14 and MySQL 8.0.46: duplicate creation
reports `duplicate_uid`, valid creation followed by destruction adds no
exception, revival by a later move reports `resurrected_item_uid`, and fixture
restoration returns the original exception set. Earlier snapshot corruption,
consistent-cut, and SELECT-only-reader checks continue to pass.

This strengthens R4/R7 audit diagnostics without changing native writers,
inactive accounting, production data, or activation gates. SQL snapshots remain
explicitly incomplete and do not qualify gameplay or crash recovery.

## Bounded native topology reconciliation

The native topology audit previously walked the full ancestry of every item,
performing quadratic work on deep or corrupt cuts. A 1,200-node regression
requires 721,799 native lookups before repair and fails a 24,000-lookup budget.
The audit now resolves each direct edge, terminal root, missing ancestor and
cycle once using an iterative memoized traversal, with linear graph work and
no recursive stack growth. It retains live edge/root mismatch diagnostics,
tombstone cycle checks, and one orphan diagnostic per missing direct edge.

All 49 reconciler tests pass. The work budget passes for a 1,200-node valid
chain, closed cycle and missing-ancestor chain; explicit small-graph cases
cover self-cycles, shared ancestors, incorrect roots, conflicting edges and
mixed live/tombstone cycles. The expanded SELECT-only SQL probe also passes
on MySQL 8.0.46 and MariaDB 10.11.14 with 1,200 additional native nested rows:
their unknown origins remain reported, and closing the chain into a cycle
adds exactly 1,200 cycle exceptions. Removing these owned fixture rows restores
the original exception set. Earlier lifetime/orphan/source/mapping checks pass.
This bounds graph work in an R7 component; it does not establish full operator
latency, storage budgets, workload qualification or accounting completion.

## Integrated candidate qualification in progress

The complete archived tree of candidate `fbd9f503581cd61369bf7100476ea66e1b3722d4`
matches Git tree `6e0e63d777abc06d2e8350f784ae74247e15594c` in a separate disposable
checkout. Its source tree is `c8804d6bc84b2f779c43030c5986fe92ceb0a245`, test tree
`7fbff73ca6a8ca9edf49b9fe310fad63d8ef4a47`, and script tree
`6904c98e6284fec4946576af11d33a72af6b4359`. The first archive was rejected before
testing because Windows checkout conversion changed its bytes; archiving with
that conversion disabled fixes the provenance check.

The strict production SQL server builds successfully, with executable SHA-256
`34f0302d01deefa1f953c841e15b7250857004988135f556b92a12997f7adf04`.
The area editor and world generators also build. Missing ncurses development
headers initially stopped the editor target; extracting the matching Ubuntu
development package into the disposable qualification directory resolves it
without modifying host packages. `make test-all` has resumed and is running
834 automatic regressions with two workers; 25 manual probes are separate.
No broad passing result is claimed while that run is unfinished. The bounded
topology repair above occurred after this freeze and has separate focused
evidence; this run must not be labeled an uninterrupted full run of later heads.

## Disposable SQL journey port routing

The playtime journey previously ignored `TEST_DB_PORT`, hardcoding the native
server connection to 3306 and omitting a port from its SQL client. It now validates
a bounded ASCII TCP port before the first connection and sends the same explicit
port to both clients. Three connection-free regressions pass, covering custom
and default routing plus invalid-port refusal. They failed before repair without
opening a connection. The actual native repository probe now connects to the
owned MariaDB instance on 34667, but its first item save fails with foreign-key
error 1452: temporary item projections cannot satisfy the migrated permanent
runtime-state table. The repair and bounded qualification below close that
fixture blocker; full accounting qualification remains open.

## Migrated SQL save/recovery journey fixture repair

The playtime repository probe now uses the real migrated tables, with their
runtime-state and custody foreign keys intact. It refuses targets outside the
fresh loopback journey namespace, checks synthetic PID/UID collisions before
insertion, and deletes only its own rows in foreign-key order. Three scope tests
pass, including refusal before compilation or connection. The native SQL probe
passes status saves, duplicate acknowledgements, stale revisions, rollback-safe
missing-custody refusal, inline coin omission, and retained runtime-state rows.

The complete `test_mysql_playtime_journey.py --server <qualified SQL executable>`
run passes on disposable MySQL 8.0.46 and MariaDB 10.11.14. It exercises real
elapsed, quiet and repeated saves, link-loss reconnect, quit/restart without
offline credit, death/reload, process kill and journal recovery, and live
copyover. The following native item-reconciliation and exact spell/quest-XP
receipt probes also pass on both engines. The executable is the frozen candidate
artifact above, SHA-256 `34f0302d01deefa1f953c841e15b7250857004988135f556b92a12997f7adf04`;
the fixture repairs affect tests only and the production source tree remains
`c8804d6bc84b2f779c43030c5986fe92ceb0a245`.

This qualifies these inactive-accounting save/recovery routes with synthetic
characters on an actual native server and database. It does not qualify active
economic roots, all item/currency routes, database interruption, full-world
clone login, flatfile parity, or complete R1-R8 acceptance. The automatic frozen
candidate suite remains in progress and is separate evidence.

## Disposable deletion journey port routing

`run_mysql_deletion_journey.py` also hardcoded port 3306 in the native connection
and omitted the SQL client's port. It now validates `TEST_DB_PORT` before any
connection and routes both clients explicitly. Two connection-free tests pass;
custom/default routing and all seven invalid-port cases fail before repair
without opening a connection. An actual MariaDB instance on the selected owned
port passes the soft-delete and late-cleanup injected failures, retaining the
mapping and inventory through rollback and playable reconnect.

The successful retry remains RED: durable player cleanup completes but the
server reports reconciliation because `zone_story_quest_runtime` is not ready
under the journey's `-s` boot. Its bootstrap currently lives in mobile special-
procedure assignment, which that option skips. This is a separate R8 lifecycle
dependency to repair and requalify; neither successful SQL deletion nor economic
identity/alias erasure is qualified by the port fix. No production rows changed.

## MySQL deletion fixture lock-name repair

The old `deletion_journey_test_<12 hex>` schema produces a 65-character native
exclusion-lock name. MariaDB accepts it, while actual MySQL 8.0.46 refuses it
with error 4163 before boot. A connection-free regression reproduces that bound;
the fixture now uses `deletion_test_<12 hex>`, retaining random isolation while
keeping the unchanged native lock name at 57 characters. All three deletion
port/scope tests pass. The direct disposable MySQL probe reproduces the old
named-lock refusal. With the accompanying boot-owner candidate repair, the
complete refusal/rollback/retry/restart journey passes on both SQL engines.
Production exclusion-lock names, credentials and authority rules are unchanged.

## No-specials quest-state boot repair

The native quest-state bootstrap now runs after world/special initialization,
outside the `no_specials` conditional. Normal boot retains its existing quest
catalog load; `-s` boot loads the catalog before decoding retained quest state.
A failed bootstrap still leaves the quest service disabled and logs its failure.
The ASan/UBSan native helper regression passes normal, no-specials and failed-load
cases. Quest feature, production catalog and account-delete runtime regressions
pass. Incoming help repair PR #679 is preserved; its catalog, cache and nine
audit tests also pass on the combined source.

The combined native source tree is `2fa0d98dac56a1687e4af4314da7ac9b327a670d`.
Both strict production builds pass. SQL executable SHA-256 is
`abfc8124335fc7dbe95540db9582307b587cd7a8087268a7f214a5d441d1cec1`;
flatfile executable SHA-256 is
`de5f8c1089e143dc27aaa8329d78227af1d86986812c677af87cc8fc7eadcc8b`.
The actual deletion journey passes both injected rollback refusals, playable
reconnect, successful retry exactly once and usable account after cold restart
on disposable MySQL 8.0.46 and MariaDB 10.11.14 using that SQL executable.

The first additional flatfile account-menu journey was RED: character authority
deletion refused before quest-state erasure. Native diagnostic instrumentation
localized the refusal to the missing account-reward summon catalog in the new
mini-world fixture. This was a required authority baseline, not evidence that
production deletion should accept missing stores. The repaired fixture below
closes that bounded journey gap. The frozen broader candidate suite remains
separate and in progress. Writer anchors are refreshed without adding writers;
normal validation passes with release still blocked.

## Native flatfile deletion and alias-erasure journey

The deletion inspector now establishes the empty fixture catalogs through real
native repositories, refusing a repeated seed before changing existing bytes.
The real account-menu journey first removes the owned summon catalog and proves
an accurate refusal with byte-identical character snapshot and retained quest
alias. Restoring that catalog permits playable reconnect, save and a successful
deletion exactly once. The original alias is decoded from the checksummed native
quest-state envelope before deletion; it is absent afterward and the erased state
remains byte-identical after cold restart. The account remains usable and its
deleted character cannot be selected. The native character/account deletion
component suite also passes its interruption, recovery and idempotency scenarios.

The full journey passes against the clean strict flatfile executable recorded
above, SHA-256 `de5f8c1089e143dc27aaa8329d78227af1d86986812c677af87cc8fc7eadcc8b`;
production source is unchanged from `b401a8521`. No refusal fence is weakened.
To reproduce on an isolated native build, first run
`python3 tests/async/test_flatfile_character_delete.py`, then
`python3 tests/async/run_flatfile_deletion_journey.py --server <flatfile binary> --inspector bin/tests/flatfile-character-delete-inspector`.
The journey owns all temporary runtime/state files and loopback listeners.
These are inactive-accounting character/quest-alias lifecycle checks; retained
non-personal economic identity, all personal-data domains, active accounting and
the remaining R8 qualification are still open.

## Frozen full-suite outcome and Redis socket fixture repair

The exact frozen `fbd9f5035` run has finished: **822 passed, 11 skipped, one
failed**, in 5,924.75 seconds. It exits nonzero; no full passing result is claimed.
The skipped entries require external SQL/telemetry, backup or other documented
opt-ins. SQL combat/death qualification is now running separately on owned
MySQL/MariaDB instances against the combined current source; it has no result yet.

The lone failure is `test_redis_connection_security_live.py`: its inherited
qualification TMPDIR generates a 109-byte Unix socket pathname. The owned Redis
server refuses that pathname and never opens its TCP listener. A standalone
repeat under the same TMPDIR reproduces the refusal. The test now retains its
other artifacts under TMPDIR and creates its socket in a separate private short
directory. It passes the actual native ASan/UBSan TCP password/ACL/database,
verified TLS, invalid peer-name refusal, Unix-socket authentication/database and
invalid socket/TLS configuration assertions under the previously failing long
TMPDIR. Runtime Redis configuration and authentication rules are unchanged.

This focused repair does not rewrite the frozen suite report into a pass or
qualify the later source/test changes. The full report and console transcript
remain local ignored evidence; current-head and external-service gates remain.

## Disposable help-import qualification connection repair

The skipped atomic help-import fixture hardcoded port 3306, the root user and a
fixed password, so it could not qualify a caller-selected isolated server. It now
uses explicit `TEST_DB_USER`/`TEST_DB_PASSWORD`, validates `TEST_DB_PORT` before
SQL, routes the native importer and SQL observer through the same TCP port, and
requires `TEST_DB_DISPOSABLE=1` on a loopback host. The fresh random fixture schema
and cleanup remain owned by the test. Missing credentials, invalid ports and
unapproved/remote targets refuse before connection or schema creation.

Four connection-free regressions reproduce 13 pre-repair failures and now pass.
The full actual importer test passes on disposable MySQL 8.0.46 and MariaDB
10.11.14 at selected ports: a rejected entry rolls back the existing pages/news,
concurrent reads see only the complete old or complete new set, and a MyISAM
destination refuses before deleting content. The fixture copies the maintained
importer into its own temporary runtime with no checkout `.env`; no production
help or game rows change. This qualifies the bounded tooling fixture, not the
complete skipped SQL/backup matrix or accounting release.

## Real SQL deletion/retention and connection-fault checks

Three checks skipped in the frozen broad run now pass against fresh owned schemas
on actual MySQL 8.0.46 and MariaDB 10.11.14:

- The exact production deletion guard and `sql_delete_player` refuse missing
  authority, unresolved retained evidence and read failure, preserve caller
  transaction rollback, and commit a clean deletion without evicting revisions.
- Two real connections prove retention-first evidence remains visible after the
  observed deletion lock wait, and deletion-first commit prevents a later
  publisher from finding the deleted identity. This exercises the production
  deletion functions and retention's row-lock/evidence boundary, not a full
  concurrent gameplay save.
- The actual SQL pool retires open or killed sessions, rolls back their rows,
  and commits successfully from replacement borrowers.

These checked source files are unchanged in native tree
`2fa0d98dac56a1687e4af4314da7ac9b327a670d`. Each test owns a fresh namespace;
no production tables, credentials or rows are involved. The production-profile
combat fixture reaches healthy death with 12 captured item identities and one
corpse-create operation, but its later dispute remains held: unassisted recovery
uses a `TEST_MUD`-only owner, and the default fixture-side payload removal also
does not release the sealed terminal request. Both production-profile attempts
remain RED at that later acceptance boundary on MariaDB; no complete passing
production conflict journey or dual-engine result is claimed.

The same source builds a separate strict development executable, SHA-256
`8b3556f6e1417c23645883e0e03e19ab9632ca71faf716825753d3373b649b26`.
Its default-coin MariaDB unassisted journey passes durable acknowledgement before
the account menu, original item evidence, self-scoped recovery list/detail,
cold-entry refusal and restart without manual fixture repair. The reset-coin MariaDB variant also passed. The first boon variant failed
its byte-exact unrelated-owner revision check; that RED result is retained.
Later diagnostic qualification passes all three MySQL variants and the MariaDB
boon repeat, as detailed below. The production selector and safety gates
remain unchanged; these development results do not qualify production release.

## Item supply-state audit consistency

The independent reconciler previously accepted a destruction event whose
custody state remained live when its revisions, reference and native row agreed.
It also did not flag a creation event claiming tombstone custody. Five failing
subcases reproduce these omissions across epoch-local and lineage history and
show that a corrupt live destruction could mask later UID reuse.

The shared lifetime audit now reports `invalid_item_supply_state` for those
contradictions and treats an explicit destruction as irreversible retirement
even if its state is corrupt. All 51 reconciler tests pass. The SELECT-only SQL
export corruption/recovery probe passes on disposable MySQL 8.0.46 and MariaDB
10.11.14: changing an existing live event's reason to destruction produces the
new exception, and restoring a valid retirement returns to the exact previous
exception set. No source rows are repaired by the exporter or reconciler.

This is bounded R4/R7 audit evidence. The export remains `sql_partial` with
`complete=false`; native origin/source completeness, runtime writer coverage,
active-accounting journeys and full lifecycle/workload qualification remain open.

## Optional native item-provenance fixture restored

The opt-in item-provenance test failed at link time: it omitted function/data
sections, garbage collection, the maintained fail-closed legacy escape stub, and
the real-query wrapper used by its included native harness. Its compile/run now
matches that maintained SQL driver, including the 64 MiB stack needed for bounded
item payloads. Both engines then exposed an outdated duplicate-source assertion:
a new command reusing a committed logical event is correctly a terminal `EEXIST`
refusal, rather than a retryable raw SQL 1062. Native policy is unchanged.

The corrected test checks the refusal twice and proves there is still exactly
one source claim, no duplicate item/custody event, and no root, item reference,
inbox or outbox for the refused command. The complete probe passes on disposable
MySQL 8.0.46 and MariaDB 10.11.14 with the migrated canonical schema: sourced
creation, nested transfers, pet/locker rows, duplicate quest/world sources, theft,
retirement, exact replay and epoch transition; simultaneous first claimants
retain one owner, native row, event, reference and source; batch retirement
preserves source/event/reference identity and child tombstones. These are native
repository transactions, not full active-accounting player journeys.

Two more frozen-suite opt-ins now pass on both engines against unchanged native
source tree `2fa0d98dac56a1687e4af4314da7ac9b327a670d`:

- The native load/query/archive stack checks 30 ordered retained cases, full and
  metadata-only PID/name refusal, self-scoped detail/list, sanitization, hash and
  schema failures, borrowed transactions, exact source preservation, eight real
  two-connection retention/load orderings, and name-reassignment refusal.
- The four telemetry schema tests check replay/session uniqueness, signed/unsigned
  endpoints, repeated migration verification and deliberate schema-damage
  refusal. The native connection factory checks target/credential selection,
  UTC/strict/charset/deadlines, allocation cleanup, failed-exec continuity, exec
  socket closure and advisory-lock release. Its credential I/O spy forwards to
  the disposable root account; this does not qualify real ingest-role grants.

All targets and damaged/restored rows are owned disposable fixtures. A diagnostic
run also hit an unavailable MariaDB global temporary file during provisioning;
the final native provenance runs use private daemon temporary directories and
complete with zero failed checks on both engines. Docker `make test-db`, full
backup/restore, actual active-accounting gameplay and integrated workload gates
remain open. Focused passes do not rewrite the frozen broad-suite outcome.

## Duplicate UID retirement audit

A second destruction of an already retired UID previously passed the audit when
its event revisions, references, owner and final tombstone all agreed. Two new
regressions reproduce that omission in lineage history and in an epoch opening
that already contains a tombstone. The lifetime audit now reports
`duplicate_item_retirement` for an explicit destruction after retirement.

All 53 reconciler tests pass. On both disposable SQL engines, the SELECT-only
exporter observes a new, otherwise consistent root/event/reference attempting
to destroy the same tombstone again and reports the new exception. Removing
only the fixture corruption and restoring its original native row returns to
the exact baseline exception set. Existing creation, movement, one retirement,
UID-reuse, supply-state and deep topology checks continue to pass. This closes
that bounded R4/R7 audit omission; it does not make the partial exporter or
unqualified routes complete, and activation/release remain blocked.

## Retained-conflict failure readbacks and development variants

The first development MariaDB boon run failed because an unrelated item-owner
revision row changed during the wallet-conversion window. Its assertion ran
before the acknowledged source readback was attached to retained evidence,
leaving no exact after-row set to diagnose. The fixture now stores that complete
readback immediately after observation and reports exact before/after revision
rows on failure. No native selector or assertion is relaxed.

With that diagnostic test, SHA-256
`1e5e87a2b8b204cebeeab3aeada5e4c6d2ab57fe53c047fae0b6b11bda8c4327`,
the complete default/reset-coin/boon MySQL run and a MariaDB boon repeat pass on
the strict development executable recorded above. The initial default/reset-coin
MariaDB passes used the earlier diagnostic-free test. These actual player
journeys prove healthy death with 12 original item identities, one corpse-create
operation, retained-conflict acknowledgement before the account menu, exact
wallet/source evidence, self-scoped recovery list/detail, cold-entry refusal and
restart without manual fixture repair. The fixture checkout is exact
`d3135428207aef41407434849dadf922780cb151` plus only the diagnostic test changes;
native source remains `2fa0d98dac56a1687e4af4314da7ac9b327a670d`.

The later passes do not explain or erase the original owner-revision failure.
Its intermittent cause remains an open qualification finding, now with exact
failure capture for further investigation. These TEST_MUD-only fixture results
do not promote production conflict release or active accounting. Both original
production-profile attempts remain RED, and the broad 839-test frozen run is
still in progress.

## Native backup/restore qualification and socket preflight

The opt-in native recovery suite now runs in the WSL qualification environment:
user/network/mount namespaces and private tmpfs restore mounts are available.
The matching real MariaDB 10.11.14 dump client is extracted under the owned QA
prefix and exposed only inside a private mount namespace; no host package or
production configuration is replaced. The strict SQL/flatfile artifacts retain
the recorded SHA-256 values `abfc8124...` and `de5f8c10...`, with native source
tree `2fa0d98dac56a1687e4af4314da7ac9b327a670d`.

The first full run passed eight cases but failed the SQL restore: its extended
qualification TMPDIR made the candidate socket pathname too long. A separate
120-byte native reproducer captures MariaDB's explicit refusal above 107 bytes.
The restore owner now refuses an overlong encoded pathname before creating or
initializing its datadir or launching a daemon. Three regressions cover 108-byte
refusal without side effects, multibyte names, and admission at 107 bytes. All
39 backup policy tests and the 225-table/50-store lifecycle validator pass.

With a supported temporary path, all nine native integration cases pass in
194.345 seconds. They qualify exact flatfile pending/legacy bank transaction
recovery, first-player WAL recovery without a persisted baseline, corrupted-WAL
refusal and retained quarantine, native locker/spell receipts, lazy-catalog
corruption refusal, foreign-owned checkout staging, and isolated healthy server
boot/shutdown. The MariaDB case captures a real full dump, imports it into a
new private socket-only daemon, verifies canonical schema/history and retained
values, boots the matching SQL server in an isolated namespace, and then proves
identity, balance-baseline and migration-checksum corruption refuse. Source
authority and captured generations remain unchanged.

These are synthetic disposable native recovery drills; they do not qualify a
captured production/staging generation, full-world player login, a MySQL restore
drill, active-accounting evidence in every domain, remote backup custody or
operator erasure propagation. No production data, accounting activation or
deployment changes. Docker integration and the remaining R1-R8 gates stay open.


## Independent UID revisions in the SQL audit

Native `item_transfer_repository.c::insert_ledger` stores the new individual
item revision separately from the new aggregate source-owner revision. The
exporter incorrectly used the latter as the UID's prior revision in lineage
references, attributed history and unattributed history. A disposable MariaDB
probe reproduced refusal of an otherwise valid creation when its aggregate
owner counter was 99. Unit regressions also reproduced incorrect inclusion of
pre-witness history and omission of post-witness history when the counters
crossed the opening item revision.

The SELECT-only exporter now derives prior UID revision as native item revision
minus one, matching the native custody update. Missing ledger joins remain
explicit missing evidence. Impossible revision zero refuses the audit cut
before history filtering. Ten exporter tests and all 53 reconciler tests pass.
On MySQL 8.0.46 and MariaDB 10.11.14 the partial snapshot probe passes with
independent owner counters for current and historical/unattributed UID events,
revision-zero refusal, and exact baseline recovery after removing only fixture
corruption. Contract validation passes 14 fixtures and matrix generation checks.

This is a bounded R4/R7 exporter repair. Native source remains
`2fa0d98dac56a1687e4af4314da7ac9b327a670d`; gameplay behavior, inactive accounting
and safety gates are unchanged. The exporter remains `sql_partial` with
`complete=false`, and activation/release remain blocked. The frozen 79540e65d
839-test run and the exact 8c997b00d 841-test run remain in progress; neither yet
qualifies this later audit repair or supplies the missing external-service gates.
