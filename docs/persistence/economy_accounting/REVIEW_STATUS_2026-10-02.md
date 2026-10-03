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


## Restore qualification retains complete epic revision history

A native MariaDB restore regression reproduced a false qualification: changing
only a saved player's epic revision from zero to nine with no ledger events
still passed every restore check. Aggregate value and the last-event balance
comparison alone cannot establish recoverable revision authority.

The read-only restore qualifier now requires the saved revision to equal the
latest post-opening ledger revision (or its opening revision when no event
exists), and the event count to cover that entire revision interval. Checked
DECIMAL subtraction covers the full unsigned range without underflow. The
native regression also removes two cancelling events from a four-event history:
its balance and latest event stay correct, but qualification now refuses. Exact
fixture repair restores qualification and the original baseline values.

The production SELECT is independently exercised through a separate read-only
connection on disposable MySQL 8.0.46 and MariaDB 10.11.14: opening-only and
contiguous histories, future/stale revisions, missing interior events, a later
opening cut, repair/re-read and UINT64_MAX boundaries pass. The focused runner
is `tests/async/run_restore_epic_history_mysql.py`. All nine native recovery
integration cases pass in 229.673 seconds with the repaired qualifier, including
the full MariaDB dump/import, retained values and isolated SQL server boot.
All 24 migration-runner tests and 39 backup policy tests pass on Linux. The
initial Windows migration-runner attempt emitted errors without a retained
final diagnostic; qualification uses the completed native Linux run. Contract and generated
matrix checks pass. Native source remains `2fa0d98dac56a1687e4af4314da7ac9b327a670d`.

This closes the bounded R8 epic revision qualification omission. These synthetic
fixtures are separate from complete accounting, captured-clone/player workload
qualification, MySQL candidate restore, erasure propagation and remote backup
custody. Both frozen broad suites remain unfinished. R1-R8, activation and
release remain open; production data and runtime safety gates are unchanged.


## Frozen 79540e65d broad result and NPC cash contract repair

The exact published `79540e65d03b4735bdf790b057e59e1492a38ebb` candidate completed
strict production SQL, area-editor and world-generator builds, then all 839
automatic scripts: 826 passed, 11 skipped and two failed in 6,127.39 seconds.
The 25 manual checks remain separate. Input root tree is
`9950b102324a023c806c81e1249e979970011e92`, native source
`2fa0d98dac56a1687e4af4314da7ac9b327a670d`, tests
`73f8f000b74be1e14f792ecee5c26bd1815f9a52` and scripts
`81acd1715fc13ec1fc96017650b1710e3ae61755`. The result remains failed; later
focused repairs do not rewrite that report or qualify an untested exact head.

`test_economy_writer_coverage_contract.py` retained eight old NPC cash-assignment
line numbers after native boot integration shifted their locations. Inspection
confirms the same four-denomination parsing in each of the two mobile formats,
matching the already-updated registry and current census. Refreshing only the
test's expected locations makes all 52 writer contracts pass. Route counts and
coverage/activation policy remain unchanged. The second failure is the stale
help-document assertion in `test_supported_server_build_contract.py`; its
repair is tracked separately. The frozen 8c997b00d run remains in progress.


## Help-build documentation contract repair

The other frozen broad-run failure asserted the old phrase "Without MySQL
(`-D__NO_MYSQL__` builds)" after the help integration documented the maintained
`PERSISTENCE_BACKEND=flatfile` target and its `__NO_MYSQL__` define instead. The
contract now verifies those explicit build facts across normalized prose, while
retaining all dependency, authority and client-free content-path checks. The
focused build contract passes. Native code, help behavior and documentation
remain unchanged; the strict builds and native help proofs recorded above are
separate evidence. The frozen 79540e65d report remains 826/11/2, and 8c997b00d
already contains the same stale writer assertion while its run continues.


## Native MySQL restore candidates with explicit policy

The restore owner previously created only MariaDB datadirs, leaving MySQL
candidate recovery unimplemented. A policy regression reproduced rejection of
an explicit MySQL choice. Version-1 policies now accept the optional
`restore_database_engine` value `mariadb` or `mysql`; omission preserves the
existing MariaDB default. Unknown/non-string values refuse. MySQL requires a
resolved MySQL 8.0 executable from the clean tool PATH, rejecting missing tools,
MariaDB compatibility symlinks and unsupported families before initialization.
The resolved installation supplies its basedir. Both engines retain private
new datadirs, TCP disabled, schema-only import accounts, bounded socket paths,
complete schema/history checks and isolated native server boot. Qualification
receipts record the selected engine.

All 40 backup policy tests and seven Linux provisioning/socket tests pass.
The ten-case native recovery suite passes in 361.778 seconds with both SQL
engines enabled: original flatfile/WAL/receipt/corruption cases plus full MySQL
8.0.46 and MariaDB 10.11.14 dump/import, retained values, migration history,
SQL server boot and identity/value/epic-revision/checksum corruption refusal.
A follow-up two-case SQL run passes in 605.993 seconds with direct VERSION()
readbacks proving the source and restored daemon match the selected engine.
The actual dump client is MariaDB 10.11.14 on both sources. It is exposed with
the separate MySQL executable only in the disposable mount namespace; host
packages and production configuration are unchanged. The native SQL/flatfile
artifacts remain `abfc8124...` / `de5f8c10...`, source tree `2fa0d98...`.

This closes the bounded MySQL isolated-candidate restore implementation gap.
These synthetic source/candidate drills do not certify captured staging or
production generations, full-world player recovery, complete accounting,
erasure propagation, remote backup custody or workload budgets. Inactive
accounting, activation/refusal gates and the declined spell path are preserved.
R1-R8 and release remain incomplete. The completed 79540e65d broad run stays
826 passed / 11 skipped / 2 failed; the frozen 8c997b00d run remains underway.

## Nested locker cold-reload and exact transfer receipts

The original manual SQL locker journey passed on both engines but stopped after
saving the withdrawal, before its cold reload. It also did not check exact item
revisions or immutable transfer receipt stability. The maintained journey now
checks original root UID 880000000101 and child UID 880000000102, revisions
5 -> 6 -> 7, exactly two UID events per deposit/withdrawal operation, distinct
operation IDs, exact reverse owner/context identities, and successful durable
critical inbox receipts. Saves and cold reloads preserve those events verbatim.
A fourth boot loads the withdrawn backpack and its original child through the
real game client; native nesting, extra descriptions and affects still match.
Accounting is explicitly inactive before and after the journey.

The strengthened run passes on MariaDB 10.11.14 and MySQL 8.0.46 using the
unchanged strict SQL artifact `abfc8124335fc7dbe95540db9582307b587cd7a8087268a7f214a5d441d1cec1`
and native source tree `2fa0d98...`. The daemon logs verify both versions.
QA source: `/opt/duris-accounting-locker-recovery-review/source`; maintained
journey SHA-256 `526e521da14991f4739ed7ce1a81a977e02ea3acc5efe96f45b0eb3a965282f1`.
The frozen generated full-world inputs come from the 79540e65d QA checkout;
`world.mob` SHA-256 is `34853f86b0e6b7c33503ab6874df1689ecd7b6fe8f0a86bdb91427f9dd1c6558`.
Local successful log: `tmp/locker-recovery-dual-sql.local.log`, SHA-256
`ad0c0c9259b21f2ca9e1a60cd30e4234787f397afccc09f32df7d4d44556e936`.
The initial environment attempt lacked generated world.mob and refused boot;
no native fix was needed. Both original and strengthened journeys pass after
supplying their documented generated-area prerequisite.

This closes a bounded R4/R8 locker qualification omission. The synthetic
character/items in a disposable full world do not certify captured-clone
recovery, process-crash publication, active epochs or flatfile parity. Active
locker-fee refusal remains intact. No production mutation or activation occurs.

## Bounded combat repeats retain the unexplained failure

Five further MariaDB development-profile boon journeys pass: healthy death
returns 12 original items through one corpse-create command, with attack-to-menu
6.465, 6.074, 6.101, 6.099 and 6.133 seconds. Retained conflict recovery passes
durable ACK before the account menu, self-scoped list/detail, cold entry refusal
and restart stability, with `manual_fixture_repair=False`. Evidence remains in
`/opt/duris-accounting-combat-b401-review/evidence/boon-repeat-{3,4}/mariadb/`
under five distinct disposable schema IDs; the directory number reflects the
readiness-loop counter, not chronological repetition order. The full diagnostic
fixture SHA remains `1e5e87a2b8b204cebeeab3aeada5e4c6d2ab57fe53c047fae0b6b11bda8c4327`.
These repeats do not explain the first MariaDB owner-revision RED. Keep that
finding and the production-profile TEST_MUD-only release-selector RED open;
no assertion, selector or inactive behavior is weakened.

## Independent audit item-revision type and range repair

A clean disposable snapshot previously reported no exceptions with boolean item
revisions or a prior revision of 2^64 followed by 2^64 + 1. Those values cannot
represent the native unsigned 64-bit item lifetime. Separately, lineage
references and unreferenced history rejected otherwise valid transitions in the
upper half of that range. Native item/ledger and accounting-reference schemas
all use BIGINT UNSIGNED; denomination signed ranges are a different contract.

The independent reconciler now validates exact integer item revisions in
0..UINT64_MAX for native custody, baseline/creation origins and current-epoch
reference/event evidence before history-scope shortcuts. Lineage references,
lineage events, unreferenced events and unattributed events validate their
one-step transition with the same range. Boolean, negative and overflowing
values refuse; consistent evidence can reach UINT64_MAX. Existing orphan
reference and broken-history diagnostics remain intact for in-range evidence.
No native transaction, price, ownership or inactive gameplay path changes.

All 57 reconciliation regressions and ten exporter/origin tests pass. New
corruptions cover both signed-boundary sides, UINT64_MAX, overflow and booleans
across every history scope. The actual SELECT-only snapshot probe passes on
MySQL 8.0.46 and MariaDB 10.11.14: a changed witnessed baseline, exact native
move to UINT64_MAX, independent aggregate owner counter 99, missing high-revision
reference detection and exact original snapshot restoration. Probe tables now
use the native unsigned item-revision column types. The original partial-cut,
corruption, consistent-read and CLI refusal cases also pass on both engines.

QA source: `/opt/duris-accounting-audit-uint64-review/source`; reconciler
SHA-256 `9fc39b8c0591d756849e0dd9911f74f253bbeff98d31031cbaad4de09fc4440a`,
SQL probe SHA-256 `5b925b4507b7eb1a6577329fe9b0dadf4ed3c511d33a351f0c0eabf7d9b892d6`.
Logs: `tmp/audit-revision-bounds-red.local.log`,
`tmp/audit-revision-bounds-green.local.log`, and
`tmp/audit-uint64-dual-sql.local.log` (SHA-256
`a873be145052f50793b596c55521e43d4be9dd1fe932081fee7363e6a7ed28bc`).
The 14 accounting fixtures and generated matrix --check also pass. Native
source remains `2fa0d98...`; coverage remains incomplete and release BLOCKED.
This closes a bounded R4/R7 input-validation and native-range mismatch, without
claiming complete sources/origins, active gameplay or release qualification.

## Independent money revision evidence repair

A separate clean snapshot probe reported no exceptions when current native
wallet and bank revisions were JSON booleans equal to revision one. Ordinary
opening/effect counters could also exceed the native uint64 range. Conversely,
pile mapping and prior-epoch retirement-root checks imposed a signed revision
limit despite the unsigned native schema. These are evidence-validation gaps;
coin denominations and copper totals retain their signed checked arithmetic.

The independent audit now uses exact integer 0..UINT64_MAX checks for native
money holdings, ordinary account origins/effects, retained creation/retirement
effect counters and pile mappings. The bounded holdings view enforces the same
range. Invalid types/overflow refuse; valid high retirement/pile revisions
remain admissible. Existing account history, posting and balance mismatch
checks remain intact. The item revision helper shares the range check and its
one-step item lifetime invariant is unchanged.

All 59 reconciliation tests and ten exporter/origin tests pass. The SQL cut
fixture uses native unsigned money revision types and passes on both engines:
a witnessed wallet effect to UINT64_MAX, witnessed pile/item authority at that
revision, stale wallet-revision detection, and exact original snapshot recovery.
The previous UID high-revision/missing-reference and full partial-cut corruption
suite also pass. The 14 accounting fixtures and generated matrix --check pass.
QA source: `/opt/duris-accounting-money-revision-review/source`; reconciler
SHA-256 `38865bccd189de7577502575348589dc4bd88c927a45369016e1677646027f7a`,
SQL probe SHA-256 `40771ee67a2dba6f9fdaad8eb0c3b69706cafdfbe2d8d45e7b9fa863dcd7428e`.
Logs: `tmp/audit-money-revision-red.local.log`,
`tmp/audit-money-revision-green.local.log`, and
`tmp/money-revision-dual-sql.local.log` (SHA-256
`1317197130e94591beab1297019b13eef376eb0b4a0fe1f8c15b67c2e3b90898`).
This closes a bounded R2/R7 audit mismatch. Native source remains `2fa0d98...`;
no transaction, inactive gameplay or activation gate changes. SQL export
remains partial, full native-source qualification incomplete and release BLOCKED.

## Native item-source baseline qualification refresh

The existing durable item-source snapshot and baseline fixture also passes on
MySQL 8.0.46 and MariaDB 10.11.14 at native source `2fa0d98...`: pet/shop/siege
ancestry includes nine cycle/depth-exhausted refusals, 99 valid chain items,
rerun/reopened repaired quarantine, duplicate/cross-owner source handling and
missing-source table refusal without durable ownership state changes. Native
capture is compiled against the actual repository implementation. QA source:
`/opt/duris-accounting-item-source-acb1-review/source`; local log:
`tmp/item-source-acb1-dual-sql.local.log`. These bounded native/component probes
do not attest complete native classes or source identities for all gameplay
writers, captured-clone authority or active-epoch release.

## SQL restore money revision-history repair

The actual native MariaDB dump/restore test reproduced another false admission:
player 42 retained the exact original denomination totals but wallet revision 9
with no immutable event still qualified. Its expected refusal failed in the
155.811-second RED run. Unwitnessed or lost revision authority can fence later
legitimate retries even when aggregate value remains conserved.

Restore now checks complete post-opening wallet and bank revision histories.
Successful native currency-ledger/inbox receipts and committed nonbaseline
economic account effects are both eligible witnesses. Economic effects resolve
their canonical account keys through SQL native mappings, preserving lineage,
kind, lifetime and context. This accommodates owners such as accounted shops
that advance authority without a legacy currency row. UNION counts a revision
once when both native and economic evidence witness it. Saved counters must
match the latest witnessed revision and the interval must contain every revision;
DECIMAL arithmetic preserves the full uint64 range without unsigned underflow.
These checks do not assert uniqueness/authenticity of every economic root or
replace independent complete source/value/custody reconciliation.

Both SELECT-only SQL component probes pass exact/empty cuts, future/stale
counters, missing history, repair, pre-opening history, UINT64_MAX and the
native/economic bridge. A committed economic-only final step passes; rejected
roots, unacknowledged receipts and wrong mapping context refuse. The full native
recovery suite passes all ten cases in 282.579 seconds with both SQL engines
enabled. Its SQL cases refuse future wallet/bank counters, remove a cancelling
native money pair while retaining every aggregate and the latest event, refuse,
restore the exact missing rows and qualify again. Existing epic gaps, schema
history, account identity, value, checksum and isolated native boot checks pass.
The other WAL/flatfile/receipt/private-checkout cases also pass unchanged.

The 40 backup policy, seven provisioning/socket, 24 migration-history and
17 backup-remediation tests pass. The first remediation-contract attempt lacked
a deployment fixture in the QA copy; it passes after copying the repository's
deploy inputs. All 14 accounting fixtures and generated matrix --check pass.
Native SQL/flatfile source and artifacts remain `2fa0d98...`, `abfc8124...` and
`de5f8c10...`. Inactive gameplay, activation gates and the declined spell path
remain unchanged; no production data or operational state is changed.

QA native suite: `/opt/duris-accounting-backup-qualification-e7d7/source`;
component source: `/opt/duris-accounting-restore-currency-history-review/source`.
Qualifier SHA-256 `ccbfc8f76a10d393e71d2458ebd2281f4e7ca279845a5bfe79826c309d672de5`;
SQL component SHA-256 `6879e74622f8d9fe31aee3d2dbe8832bced0161cc6ef8b36df4bab3684e1a12d`;
native integration SHA-256 `7aa9e4f8a156e14bb1302d69aaf84f3508a7537049bf7e6588d155580459d509`.
Logs: `tmp/backup-money-revision-red.local.log`,
`tmp/restore-currency-history-dual-sql.local.log`, and
`tmp/backup-currency-revision-green.local.log` (SHA-256
`6031b5be89a5e4be4d6eb5f6afe93d791c2059a5b5ad17289e83e571f6b42c2f`).
This closes a bounded native money-history restore omission. Captured-clone,
full-world player recovery, full accounting, erasure propagation, remote custody
and measured workload remain separate open gates; release remains BLOCKED.

## Diagnostics integration and writer-location continuity

Canonical experimental-accounting advanced from `8140dbd70` to `4fffb0748`
while the native money-history restore fix was being published. The local fix
rebased cleanly and was pushed as `820607ce5`, with canonical GitHub SHA
readback. The upstream persistence diagnostics changes are preserved; native
source is now `5d6cf93e958abab2f9c32d4c9c86159a6808dc7f`. Previous `2fa0d98...`
native build/journey evidence remains attached to its original source.

The matrix --check reproduced stale generation and three unmapped sites.
Direct source review confirms they are the existing staff world-info temporary
object preview and death snapshot quarantine SQL update, shifted by diagnostics
code. Their classifications and refusal boundaries are unchanged. The registry
census and site locations now name the integrated upstream source; regenerated
coverage remains 864 routes, 2,815 occurrences, 2,756 unique sites, zero unmapped,
coverage_complete=False and release BLOCKED. All 52 writer contracts pass.
This repairs inventory drift without qualifying additional runtime writers.

The exact frozen `8c997b00d` broad run finished: 828 passed, 11 skipped and two
failed among 841 automatic tests in 7,160.80 seconds. The failed writer-location
and supported-help-build contracts are the same stale assertions subsequently
repaired with focused passes. Strict SQL/editor/world builds and the admitted
real journeys passed. Its final report is
`tmp/integrated-8c997b00d-results.local.json`; do not relabel this failed frozen
run as current-head qualification. External service and manual gates remain.

At publication of this inventory repair, integrated diagnostics/save/journal
focused checks and backup policy/socket tests pass. The strict production SQL
build passes (SHA-256 `3ed76722665a7025d07d5ea4353fb1a57dfc31d6daef1d5d1830014c41468601`);
flatfile compilation is still running and fresh native recovery remains pending.
The standalone doctor suite's SQL case was explicitly skipped in the focused
run. A separate invocation passes all ten cases, including native read-only
custody capture without authority changes, on both engines after canonical
bootstrap and all 53 migrations. Its first attempt omitted migration 0038's
item equipment-slot prerequisite; it passes with the fully migrated schema.
Log: `tmp/diagnostics-doctor-dual-sql-green.local.log`. Earlier native proofs and these
component results do not close R1-R8 or the full qualification gate.

## SQL restore revision-root identity repair and integrated native refresh

The frozen `820607ce5` qualifier admits two unrelated committed economic roots
claiming the same wallet/bank revision. Its UNION projected only native identity
and revision, so distinct operations collapsed into one apparent witness. A
changed economic before-revision also admitted alongside the native final
revision. The RED SELECT probe retains native currency-ledger revision uniqueness
constraints; the collision is across economic/native evidence, not a native
ledger row that the real schema would already refuse.

The qualifier now preserves the original root and before/after revision in each
witness. A durably receipted native child resolves through its declared economic
root. UNION deduplicates only matching root/transition witnesses; per-revision
grouping rejects distinct roots or conflicting transitions before counting the
complete interval. Current native wallet/bank effects must be one-step
transitions. Several distinct native child revisions within one root remain
admissible. Full uint64 comparison, empty/opening cuts, and legacy native-only
histories remain supported. No native operation or inactive gameplay changes.

The actual SELECT-only probes pass on both engines: same-root bridge, receipted
parent/child bridge, multiple native child revisions, distinct-root collision,
missing revision plus conflicting witness, backward/skipping transition refusal,
exact repaired history, stale/future counters and UINT64_MAX. These are bounded
SQL component proofs, not a complete audit of root payload authenticity,
sources, values or active-accounting player journeys. Qualifier SHA-256:
`1e5f61eb140b43cca2a92d545af01f8fc8d12a816ab311aee9cdf4586fbba2c4`;
component SHA-256:
`32311549032f75c977e3bd09870286f9e0840c4684f0d3207a1c043f899c45f0`.
Logs: `tmp/restore-currency-revision-identity-red-real-constraints.local.log`
and `tmp/restore-currency-revision-identity-green.local.log` (SHA-256
`d9a119c178ed84c915f45dc1b16c578b397d6f25bfae9fb22ed5df44f3407912`).

Native source `5d6cf93e958abab2f9c32d4c9c86159a6808dc7f`, including upstream
persistence diagnostics, passes both strict production builds. SQL executable
SHA-256 is `3ed76722665a7025d07d5ea4353fb1a57dfc31d6daef1d5d1830014c41468601`;
flatfile is `4d913107685f82231888536bf9e2b05be2c2e36ce6add1282613a7e23aad9dc5`.
The ten-case native backup/recovery suite passes in 281.916 seconds with both SQL
engines enabled: full dump/import, exact completed migration history, source and
candidate version readbacks, wallet/bank/epic history corruption refusal, retained
receipts, WAL/flatfile recovery, private-checkout isolation and native service
boot. Its first integrated attempt added an impossible native duplicate-revision
fixture, which the real ledger uniqueness indexes refused; that probe is removed
from the native suite and constrained correctly in the independent SQL component.
The 301.181-second failed fixture run remains retained separately, not relabeled.

QA source: `/opt/duris-accounting-diagnostics-integration-820607ce/source`.
Logs: `tmp/diagnostics-integration-build.local.log`,
`tmp/diagnostics-current-native-recovery-fixture-red.local.log`, and
`tmp/diagnostics-current-native-recovery.local.log`. The same SQL executable also
passes the complete nested locker deposit/reload/withdrawal/reload journey on
both engines, preserving original UIDs, exact 5->6->7 revisions, original
metadata, exactly two immutable item events per transfer and durable receipts.
Accounting remains inactive. QA: `/opt/duris-accounting-diagnostics-locker-review/source`;
log `tmp/diagnostics-locker-dual-sql.local.log`, SHA-256
`4e930cb526450ebdea953e4333d62e363d442baf4c86d75bb71fb75f13b24d95`.

The earlier 40 policy/seven socket tests and integrated diagnostics/save/journal
checks remain passing; all 14 accounting fixtures, matrix --check and 52 writer
contracts pass after the source-location refresh. This closes a bounded R8
revision-root ambiguity and refreshes named native checks after integration.
Full money value/source reconstruction, active journeys, flatfile parity,
captured-clone/full-world, erasure, remote custody and measured workload remain
open. The original combat RED and declined inactive spell boundary are retained;
coverage_complete=False and release BLOCKED.


## SQL restore denomination history after native commerce

A genuine committed native shop purchase reproduced a restore false refusal.
Its wallet changed from (0,0,0,1), revision 4, to (0,0,8,0), revision 5;
the bank stayed (2,0,0,0) while revision 7 advanced to 8. The shop transaction
retains its economic root, receipt, exact account effects and original item UID,
but emits no legacy currency-ledger row. Revision qualification passed while
the previous legacy-only SUM value query raised restore_currency_value_mismatch.
The native RED is retained in `tmp/native-shop-money-restore-red.local.log`.

The restore qualifier now walks the same committed native/economic witnesses
through their denomination before/after vectors. Each before-image must match
the previous after-image, and the final vector must match native authority at
its witnessed current revision. Native ledger before-images derive from the
exact after-image minus its delta using DECIMAL(65,0). Economic effects retain
their actual vectors. A bridge deduplicates only identical root, transition and
vectors; disagreement refuses. NULL/negative vectors and checked copper totals
beyond signed 64-bit refuse without SQL arithmetic wrap. Events at/before the
opening cut are excluded. Full uint64 native revision comparison is unchanged.
No native gameplay, activation, production state or declined spell path changes.

Both engines pass the final native-purchase probe: economic-only commerce,
current-value corruption, NULL bank value, changed economic before-image,
weighted copper overflow, exact repair, a matching synthetic legacy bridge,
conflicting bridge refusal, later opening cut and unchanged final authority.
The bridge row is an explicitly synthetic fault probe; the actual shop emitted
none. The test-only purchase-cut selector stops the existing native SQL harness
after a committed buy without altering its default full buy/sell/replay/rollback
fixture. The full buy/sell probe also passes on both engines, including late
replay, insufficient funds, native rollback and paused-epoch refusal. Two
successful roots and the original UID return to keeper custody; exact values
and receipt history survive the independent read-only checks.
Native component fixtures select a synthetic accounting lineage internally;
these are not active-epoch server/player qualification or an activation baseline.

The complete ten-case native recovery suite passes in 321.980 seconds with both
SQL engines enabled, including full dump/import, schema/history/value checks,
exact interrupted legacy-money replay, WAL/flatfile recovery, receipt corruption
refusals, private-checkout isolation and isolated service boot. Native source and
SQL/flatfile artifacts remain `5d6cf93...`, `3ed76722...` and `4d913107...`.
The separate revision-history SELECT probe passes on both engines after sharing
the witness selector. Native Linux migration, backup-policy and remediation
checks pass 24/40/17 cases. The first migration-test invocation under Windows
failed POSIX ownership, symlink and shell prerequisites; the native Linux run
passes all 24, without changing their checks. All 14 accounting fixtures and
matrix --check pass; changed C++ lines match clang-format 18.

QA roots: `/opt/duris-accounting-native-shop-restore-money-review/source`,
`/opt/duris-accounting-native-shop-restore-money-full-trade-review/source`, and
`/opt/duris-accounting-diagnostics-integration-820607ce/source`.
Logs: `tmp/native-shop-money-restore-latest.local.log`,
`tmp/native-shop-money-full-trade.local.log`,
`tmp/restore-native-money-values-recovery.local.log`,
`tmp/restore-money-values-revision-regression.local.log`, and
`tmp/restore-money-focused-native.local.log`.
Qualifier SHA-256 `981084fda98177a8ec5a92446834190e92adde8033e2bc10ea0ef33c467efddf`;
native money probe SHA-256
`6eb6054a65c3cd9d615420781230d12970a7bf1609f7c6450ba80ddac153f528`;
recovery log SHA-256
`91a19264311348368a676b6065c798bdea8d76f96914b018be2d46ed403e1a6f`.

The full default native shop transaction probe also passed separately on both
engines after all 53 migrations: exact custody/payload, keeper cash exceptions,
realized price, buy/sell atomicity, outbox, exact/late replay, insufficient funds,
rollback faults and paused-epoch refusal. Log:
`tmp/diagnostics-shop-dual-sql.local.log`, SHA-256
`738dac01a6fc05f959526dbb0a4abfa3c30587dd6ef7bff7ea9ba2fa40215990`.
These close a bounded restored wallet/bank value omission and refresh named
native components. Full holdings/source reconstruction, active player journeys,
flatfile parity, captured-clone/full-world, erasure, remote custody and measured
workload gates remain open. Frozen `88d3b364c` broad regression is in progress;
it does not contain this later qualifier/test change. Coverage remains
coverage_complete=False and release BLOCKED.

Final `native-shop-money-restore-latest.local.log` SHA-256 `55a304880c6b71091d7cd0a55d2abf5ac168a30cac9464207407cb7edd00c98d`.

Final `native-shop-money-full-trade.local.log` SHA-256 `d694e8c77a9bc5248167c5063b36067e0876fc517fefc5580ef7d277a2d631fd`.


## Independent audit checked copper totals

The read-only reconciler accepted a consistent wallet, opening and before/after
history whose platinum field was INT64_MAX. Each individual denomination fit
its field and the postings still summed to zero, yet the actual weighted copper
holding could not exist in the native money range. The RED snapshot reported
zero exceptions. The new regression fails before the repair; the published
`29d922ac6` reconciler also fails the new SQL effect-vector overflow probe.

Parsed denomination vectors now check their weighted copper total in addition
to every signed 64-bit field. Native holdings, openings and account-effect
before/after images refuse positive and negative overflow; representable upper
boundary values remain admitted. The CLI refuses malformed vectors with status
2 even at zero output-detail limit, leaves the input bytes unchanged, and passes
again only after exact external fixture repair. No mutable authority, native
operation, gameplay decision, price, activation or declined spell path changes.

All 61 reconciler tests pass in 0.786 seconds; ten exporter/origin tests, 16 audit
invariant tests and 14 accounting fixtures also pass. Both engines pass the
complete SELECT-only partial SQL snapshot probe, including independent native
and effect corruption, exact restored snapshot equality, full uint64 wallet/pile
and UID boundaries, history/reference refusal and the existing source/custody
checks. The component uses synthetic rows and widened native coin columns to
exercise the audit vector range; it does not establish that a real player wallet
can hold those injected values. The canonical evidence columns remain BIGINT.
The exporter still reports sql_partial/complete=false. These are bounded R2/R7
malformed-evidence refusals, not full independent reconciliation or gameplay,
flatfile parity, captured-clone, lifecycle or measured workload qualification.
Native source stays `5d6cf93...`; coverage_complete=False and release BLOCKED.

QA: `/opt/duris-accounting-audit-checked-copper-review/source`.
RED logs: `tmp/audit-checked-copper-red.local.log` and
`tmp/audit-checked-copper-sql-red.local.log`. GREEN logs:
`tmp/audit-checked-copper-final.local.log` and
`tmp/audit-checked-copper-sql-green.local.log`.

`scripts/reconcile_economy_accounting.py` SHA-256 `78a097fd2ebf0fd8e8dddb000cfff8d47d3892a204fa772705573e3a6ce99a66`.

`tests/async/run_economic_sql_audit_snapshot_mysql.py` SHA-256 `3245046af108dcbddce05fde82829c461f012edf274e5b3430ffd1efe4bb3e9c`.

`tmp/audit-checked-copper-sql-green.local.log` SHA-256 `b0301057fb9ee46bf95c9c8e266478e845d0d47ed50f8f872fb294ec3c0e51db`.


## Flatfile legacy deletion accounting admission

The actual native player-domain removal helper prepared a wallet-removal operation
under an active accounting control record. The new native fixture reproduces
that RED before the fix; no complete typed erasure root exists at this boundary.
Direct player-wallet/shared-bank removal now checks legacy admission under the
borrowed authority lock before clearing outputs or staging operations. Whole
character/account deletion checks the same gate before new compound removals,
including accounts with no items or characters. Published authority journals
remain recoverable; active control returns conflict, corrupt control refuses,
and the existing inactive/paused legacy behavior remains available.

The full native deletion harness passes with ASan/UBSan, warnings as errors,
active/corrupt/empty-account refusal, unchanged native file bytes, unchanged
preparation outputs on admission refusal, reopened-lock retry, exact external
control repair, paused retry, the original 17-operation budget and interrupted
character/account journal recovery. The metadata selector exists only in the
native test executable through DURIS_FLATFILE_ACCOUNTING_TEST. It does not create
a source-complete activation baseline or active-epoch player/server proof.
The borrowed-read ASan/UBSan harness also passes, including allocation failures,
exact retained receipts and recovery. Narrow component executables discard
unreferenced removal APIs with linker sections instead of mocking accounting
admission; domain, initial shared-bank hydration, locker payment/replay/recovery
and playtime checks pass. The locker fixture's SQL half was explicitly skipped
without its dedicated disposable SQL target and is not claimed here.

Both strict production builds pass. The real mortal account-menu journey passes
missing-authority refusal, unchanged snapshot/quest alias, playable retry,
exactly-once deletion, alias erasure and a usable account after cold restart,
with accounting inactive. Its first dispatch preceded completion of the new
inspector build and stopped at a missing executable; the subsequent dispatch
used the completed sanitizer inspector and passes without changing the journey.
All ten native recovery/restore cases pass in 427.430 seconds at this source,
including both SQL engine full dump/import/schema/history/value checks, exact
legacy transaction replay, private foreign-owned checkout isolation and service
boot. This refresh remains an isolated fixture/recovery result, not a captured
full-world generation or complete accounting restore certification.

Native source tree: 4968da540845b5679c72974a6a43b784259a4e3a.
SQL binary SHA-256: 15b335def2225622fc164f7ad7a9c06afca29c164a6a1d3224551a2875f9daa2.
Flatfile binary SHA-256: fdb67946a5ffb7aba346710399ae070c7c86af26ec6253c41a9f09db11b32a59.
QA: /opt/duris-accounting-flat-delete-review/source and
/opt/duris-accounting-flat-delete-build-review/source.
RED log: tmp/flat-delete-admission-red.local.log, SHA-256
03e32033e976050355a7e8f0c61a3cde64e63aa51f9dec3169dc4683a45809a1.
Sanitizer GREEN log: tmp/flat-delete-admission-green.local.log, SHA-256
a810781f0c9c5ec04456c50d5eb7d341a9c7fa5c69922d66df8183866cc75575.
Borrowed-read log: tmp/flat-delete-borrowed-reads.local.log, SHA-256
7dc3d65ae45ab29637fe5d5b2e8bbaf47244b2710655d515277452201b35d43f.
Native build/component log: tmp/flat-delete-build-checks.local.log;
strict build logs: tmp/flat-delete-strict-sql.local.log and
tmp/flat-delete-strict-flatfile.local.log.
Gameplay log: tmp/flat-delete-current-journey.local.log, SHA-256
d5ee5d4e36237eb343692dc44306b34f850548d92f25e7215af7fc761d41f127.

Only three source anchors shift in the generated matrix; counts and backend
qualification statuses are unchanged. Refusal evidence is recorded for the two
flatfile lifecycle routes without promoting them to complete accounting.
SQL physical/account deletion admission and account-menu admission before
publishing a durable deletion fence are separate unfinished boundaries. Typed
erasure, non-personal identity retention, complete native/economic audit,
source-complete active journeys, captured-clone/full-world and workload gates
remain open. The earlier frozen 88d3b364c broad run is still progressing and
cannot certify these changes. coverage_complete=False; release BLOCKED.

Recovery log: tmp/flat-delete-native-recovery.local.log, SHA-256
9f801895144b1bc5692422401de74255f15b09b0faf9799a649112480f1e3f92.

## SQL legacy deletion accounting admission

The exact production sql_player_deletion_guard/sql_delete_player definitions
reproduced physical player deletion under a synthetic active-epoch pointer on
both native SQL engines. The previous PID/death-conflict guard did not establish
accounting admission. The new implementation acquires the existing native writer
lease before an owned transaction begins. Outer character-deletion owners acquire
it before BEGIN and retain it through confirmed commit/rollback. Both the guard
(before any cached PID approval or consistent read) and the physical-delete
boundary validate the exact lease, session, named-lock ownership, reconnect
setting and local authority lock. No caller boolean can assert admission.

Whole-account cleanup also acquires the existing native gate before BEGIN and
retains it through commit/rollback. That route currently has strict-build and
source-order evidence only; these character tests do not qualify whole-account
runtime erasure or admission before the account menu writes its durable fence.
The offline pfile build initially failed to link the new guard dependency. Its
existing no-SQL stubs now expose explicit unavailable admission and a null DB;
the strict pfile rebuild passes without linking native SQL authority into the
offline tool. The first retry also exposed shell environment expansion in the
local command; the saved LF driver passes with the intended compiler wrapper.

Native MariaDB/MySQL component fixtures pass active/staged/missing-schema
refusal before BEGIN with unchanged identity, cleanup and revision state;
empty/absent/null/lost lease refusal; reconnect-enabled invalidation and
exact repair; retained death-conflict/read-error refusal; rollback; and ordinary
inactive deletion. Two real-session REPEATABLE_READ races still pass retention-
first and deletion-first ordering. An observer cannot acquire the native writer
lock during deletion or after SQL commit while the owner retains its lease.
These exact-definition fixtures use minimal temporary tables and synthetic
metadata, not a source-complete baseline or active player qualification.
ASan/UBSan execution of production account-menu/character-delete bodies also
passes admission failure before BEGIN, rollback/ambiguous commit, publication,
cancel and retry. Client-free and PID revision checks pass.

Both strict production server profiles pass. The actual inactive mortal
character-deletion journey passes on fresh canonical 53-migration MySQL and
MariaDB schemas: unavailable lifecycle metadata refuses accurately without
losing the player, mapping or original item rows; exact repair permits playable
login/save/retry; existing soft-delete and late-cleanup SQL faults roll back;
retry deletes once and cold restart leaves the account usable. The actual
flatfile inactive refusal/retry/alias-erasure/cold-restart journey also passes.
All ten native recovery/restore cases pass in 316.257 seconds, including both
SQL engines' dump/import/schema/history/value checks, exact legacy replay,
flatfile WAL/catalog/receipts, private foreign-owned checkout and service boot.
These are owned disposable fixtures and isolated recovery checks, not captured
full-world restore, active gameplay or complete accounting certification.

Native source tree: 971da56436ece326fa248f6b175b2e49784cb7eb.
The complete tracked native source was hash-compared to the strict-build/recovery
checkout after the offline stub repair; no mismatches remain.
SQL binary SHA-256: 35d277a3911c7be14a80f138941e22cc607d4c54b5f604c75750427beb1218c2.
Flatfile binary SHA-256: f06a3863e91fc6dd13cfed64721809be223ed87d4006402e7b6e3ddd00f6bb3f.
QA: /opt/duris-accounting-delete-admission-sql-green-review/source and
/opt/duris-accounting-sql-delete-build-review/source.

Local ignored evidence SHA-256:

- tmp/deletion-accounting-admission-sql-red.local.log: 5c5f926c38122e1ab5dbe175906b1eb2ea99a991bd0287588a530706aaae22f8
- tmp/deletion-accounting-admission-sql-green-latest.local.log: 2ce66bff2478360cf42be7483c0403ff7aca493d4cef3318c8f6709e88ff4772
- tmp/deletion-accounting-admission-native-unit-latest.local.log: 1652b3f25ed5b8658e2175735a85ac1f59ad9d8ba78b78393fab2597b0328f29
- tmp/sql-delete-current-journeys.local.log: e5135d40bcf0290280c92a55874c6c8d39ecb5bdc7578b0e55872faa76963ad5
- tmp/sql-delete-current-flatfile-journey.local.log: 8c854b3ad1417d59b836a42fd81eb1ae4637112ca12792be7df502e591f325f4
- tmp/sql-delete-native-recovery.local.log: 36840d470b094df09cd417041a01dfb3ceb4231d78b77e66da1eaac47154c909
- tmp/sql-delete-strict-pfile.local.log: a4db67065f714c162ceef658dfce93c63850ab81eb019b39e3c8706a6a28d03f
- tmp/sql-delete-strict-pfile-retry.local.log: 96e6658d2911c3b7953307767f87b71ded51956fcc6e17b82e64d3877f8c8264
- tmp/sql-delete-source-contracts.local.log: 649de28fc9aaa64fbc7eea93b7831b90ae6487e142bd986216a5496a63855a66

The census retains 864 routes, 2,815 occurrences, 2,756 unique sites and zero
unmapped sites after source-anchor refresh. Backend completion statuses remain
unverified; refusal evidence is recorded separately. The frozen 88d3b364c broad
run does not contain this or the preceding three milestones and cannot certify
them. coverage_complete=False; release BLOCKED. All R1-R8 full-feature gates,
account-menu pre-fence admission, typed erasure, complete audit/retention,
captured full-world rollback generation and measured workload remain open.
The pending captured-generation path question remains unanswered. Production
activation/data and the declined inactive spell-path changes were not touched.

The full writer-contract refresh initially exposed four stale test anchors
(two assertion failures and two missing-key errors), including core/files.c
locations affected by the new include and transaction-owner changes. Literal
expectations now follow identical source lines from frozen 88d3b364c to this
candidate; route classifications, ownership counts and activation assertions
are retained. The original failure log remains available rather than being
relabelled as a passing run.

After refresh, all 54 writer-contract tests pass (52 matrix tests plus two route-evidence tests); the standalone writer-site check also passes. Normal 14-fixture validation and matrix --check pass with release still BLOCKED. The final dual-engine native deletion run additionally refuses a held lease on each of two other live SQL sessions.

- tmp/sql-delete-writer-checks.local.log: a7800d0f6d6e0c690809912bbbd35c859fb654363d11e32f2b8c74753a0a238e
- tmp/sql-delete-writer-checks-reanchored.local.log: 9e34262bda18c10c5f86fdb122abeb14ca43ac444acd32de1d55250ea58d6dce
- tmp/deletion-accounting-admission-sql-green-final.local.log: 0f05d6deb9a0683e308785d97ebce59a5efb55808e85583e337478dfab0dcb10

## SQL account confirmation before the durable deletion fence

On both actual inactive SQL server profiles, confirming whole-account deletion
while lifecycle metadata was unavailable wrote blocked=2 before backend refusal.
The character/item rows survived, but the account was permanently fenced and
normal login could not remain usable. The production confirmation body also
reproduced fence writing without held native admission in a sanitizer fixture.
The first local runtime dispatch lacked the configured linker wrapper; its
subsequent native RED is retained separately. The first real RED driver could
not find rg in the controlled WSL PATH; the available fallback confirmed the
expected defect on both fresh owned engines.

New SQL confirmation acquires the existing native writer lease before changing
the in-memory account flag or invoking write_account. Outer transactions and
native admission failure return an accurate no-fence refusal and usable account
menu. The lease spans the durable fence write and ends before worker-save drains;
physical account cleanup establishes its own previously repaired admission.
Already-fenced requests retain their existing irreversible retry/cancel policy.
This does not remove or roll back a previously accepted deletion fence.

ASan/UBSan execution of the actual SQL confirmation owner passes unavailable
admission, outer transaction refusal, failed fence-write restoration, no session
closing/runtime cleanup on early refusal, lease release before worker draining,
already-fenced retry/cancel and exactly-once successful publication. The real
canonical 53-migration journey passes on MySQL and MariaDB: unavailable native
lifecycle metadata leaves the account flag, character, mapping and original
item rows unchanged; metadata repair permits normal login/inventory/save;
existing character admission/soft-delete/late-cleanup faults and deletion once/
usable-account cold restart still pass. This is native read-error refusal on
inactive synthetic accounts, not active-epoch or whole-account erasure proof.
The flatfile inactive deletion journey also passes, without certifying its
still-open pre-fence admission.

The first strict SQL build caught missing explicit SQL session/transaction
headers in account.c; they are now included. Both strict production server
profiles and the offline pfile build pass. All ten native recovery/restore cases
pass in 275.327 seconds with both SQL engines, exact legacy replay and isolated
service boot. Normal 14-fixture validation, matrix --check, all 54 writer-contract
tests, the standalone writer-site check and deletion source contracts pass.

Native source tree: 30d8b4b453e956455e77773c58201d98fce93954.
SQL binary SHA-256: 9eca90409074ace4ade5387941e628c4968b65809f67644d772e539b534f7214.
Flatfile binary SHA-256: 5f7356d2d6c1ad9378e3a3ddef42db8180b3a24c9a4ff73bd71616da0d8057d2.
QA: /opt/duris-accounting-account-fence-build-review/source.

Local ignored evidence SHA-256:

- tmp/account-fence-admission-runtime-red.local.log: 121b70ad53634568d31591ce5ff29f75132c5cac36ba2a1b63f5ce8eaf0f93ef
- tmp/account-fence-admission-runtime-red-native.local.log: 42be6ca13cd3cbcf62862d84f800f2258f2fc74bb405a17797fb41b2fb6a9a3e
- tmp/account-fence-admission-runtime-green.local.log: aa9a3a0c0153ca71e0df4c66f946ae2ac8c53a8e867b85a5a55f077de248fbb9
- tmp/account-fence-admission-red.local.log: 29b7cf2a146b5fe04010837d382ce9b7b6ad99c37eb334210a9d007b44489082
- tmp/account-fence-admission-red-native.local.log: 097b2f8712b10304bb6049f78fb1399edd51acbd04603ab9b82664305411e1ef
- tmp/account-fence-build-checks.local.log: d27a0cb12485a08e4b26c8ea1d566416f9836c9a31e630be7f5288185749def0
- tmp/account-fence-build-retry.local.log: 2d41835b1252e9b3dbdc2b7f12164a7e960d41d4b9e59c318418599c5ac751f2
- tmp/account-fence-current-journeys.local.log: 6c03c25129a06215380a803154f18af5cc07f88bf5923cbc2c3685dfe80bea9a
- tmp/account-fence-current-flatfile-journey.local.log: 8c854b3ad1417d59b836a42fd81eb1ae4637112ca12792be7df502e591f325f4
- tmp/account-fence-native-recovery.local.log: 89dfb8890fa0d3ddd682b862518b8b4ef9ab9928a109315f4a4ae60c99522c98
- tmp/account-fence-source-contracts.local.log: 649de28fc9aaa64fbc7eea93b7831b90ae6487e142bd986216a5496a63855a66
- tmp/account-fence-writer-checks.local.log: d72889b2fd73969d39278c5de6de9908940427b5a9c2a2ec2ee321653f1e08a0

The frozen 88d3b364c broad run hit a 180-second native inspector compilation
timeout before reaching its full-world journey. A separate unchanged-source
native inspector build passes in 98.804 seconds; this is consistent with
resource contention but does not establish the cause or replace the failed
full test. No timeout was increased or failure waived. That broad run remains
in progress, is frozen at its original source, and contains none of the later
repair milestones. Current-head broad, captured full-world/rollback generation,
complete native source and measured workload gates remain open.
coverage_complete=False; release BLOCKED; all full R1-R8 gates remain open.
Flatfile pre-fence admission and whole-account typed erasure/cleanup are next
lifecycle boundaries. The captured-generation path remains pending. No
production activation/data or declined inactive spell-path changes occurred.

## Frozen 88d3b364c result and publication-ACK harness repair

The exact frozen broad run completed with 829 passes, 11 skips and three failures
in 8,161.730 seconds, across 843 automatic cases; its manual cases remain outside
that count. test_formatting_tooling reported player_save_journal.c and
player_snapshot_repository.c formatting drift. test_flatfile_full_world_boot
stopped at the 180-second inspector compilation deadline. The isolated
publication-ACK test failed to compile after native diagnostics added observation
calls; its fixture lacked the trace-stage/type/function bindings. The results
belong only to original 88d3b364c/native tree 5d6cf93..., not later repairs.

The ACK fixture now binds that observation boundary using its existing isolated
command/journal types. It still executes the exact production ACK function,
leaves coordinator reads and duplicate ACKs responsive while disk checkpointing
blocks, retains the operation on checkpoint failure, and removes it only after
successful retry. It now additionally checks actual traced command/outcome
values for the successful checkpoint, failure and successful retry; a duplicate
in-flight ACK emits no checkpoint result. The focused checkpoint test passes.
No production source, checkpoint ordering or journal semantics changed.
Native source remains 30d8b4b453e956455e77773c58201d98fce93954.

Local ignored evidence SHA-256:

- tmp/integrated-88d3b364c.local.log: 074c6cf341d83d26ce100c43831cff37cfe4a6da833d1a5eac01d267edc6753d
- tmp/integrated-88d3b364c-results.local.json: 13e17fc7d47648e787deb8f6a08f7cbe5bad0ce30396f7cd64668e8de30bb03d
- tmp/frozen-88-inspector-build-measurement.local.log: 42feea9f3e133fffba1f94fb9c5adae5e6eeaad6e83554ff15183a0a35d8bcfc
- tmp/publication-ack-checkpoint-fixed.local.log: 30eebc056adbece1c4337566deb9139f43b45e0f4bfe39476d78f542062e0ce2

The original broad run remains failed. Formatter repair, unchanged-source
full-world retry and a fresh current-head broad result are separate outstanding
checks. No compilation deadline has been raised. The flatfile account-fence
journey independently reproduced corrupt accounting metadata being refused only
after a permanent account fence; that implementation is still unfinished.
All R1-R8 full-feature requirements, coverage_complete=False and release BLOCKED
remain unchanged. The captured-generation path is still pending.

## Native formatter contract repair

The frozen broad run's two formatter violations are repaired in
player_save_journal.c and player_snapshot_repository.c. The diff changes only
spacing and line breaks in the archive-frame predicate, timer-row builder and
retained-death failure helper. No persistence, accounting or inactive gameplay
behavior changes. The full formatting-tooling contract passes in the Linux QA
copy; strict production SQL, flatfile and offline pfile builds pass.
Normal 14-fixture validation, matrix --check, all 54 writer-contract tests and
the standalone writer-site contract pass. Source anchors were refreshed without
changing route disposition, excerpts or qualification; 864 routes/2,815
occurrences/2,756 unique sites/zero unmapped remain inventory counts only.

Native source tree: a132651b48e9449f5d091741250b2a3f5ab600cb.
A comparison of all 1,214 tracked native files, normalized only for CRLF/LF,
finds zero differences between the staged source and the strict-build QA.
SQL binary SHA-256: 12e8d895cf7642b43acca42555273d14363645e327061aa8ba6f05ae7f581eb5.
Flatfile binary SHA-256: cedf591aecb47a0931f2b928f71f881612b84e6d44c9bf2cb165f0702c7dd65d.
QA: /opt/duris-accounting-formatter-c12-build-review/source.

The first local formatting dispatch hit Windows CRLF shell-hook parsing. The
archived QA also needed its shell hooks normalized and Git's CRLF cleaning
configured before its changed-lines check. The obsolete formatter subprocess
started under the earlier line-ending configuration was stopped; the complete
correctly configured retry passes. These setup failures remain in ignored logs.
No test, formatter policy, timeout or acceptance requirement was weakened.

Local ignored evidence SHA-256:

- tmp/formatter-c12-build-checks.local.log: b16851bdb4b62eb3998d586ac1c84871f2b763671b54c3b70f5c918c2a593e6e
- tmp/formatter-c12-native-retry.local.log: eb051de526640654124dc8f00beb1d4b95a706fad612bca0ab5d393bf0c2dcca
- tmp/formatter-c12-writer-checks.local.log: ecc2f2c56ff82a920d2f8a65b297df6fa0428b9d467d7a3da713a36d9d313dce
- tmp/formatter-c12-source-pin-native.local.log: 7d7ff7be4ac8dd41f93be18851487c406f2a241b05fdb3b273a31ba34041a269

The original frozen broad run remains failed. Unchanged-source full-world retry,
a fresh integrated broad run and all full R1-R8 gates remain open. The flatfile
pre-fence fix is undergoing separate qualification and is not included in this
formatter-only source pin. coverage_complete=False; release BLOCKED. No declined
inactive spell-path or production activation/data changes occurred.

## Remote cd4cc413f integration and formatter publication qualification

The normal formatter milestone push was rejected because experimental-accounting
advanced to cd4cc413fdaf62bf93bf0eee7908883f1254f08d with PR #684 account loading
and PR #685 world-loop supervision. The unpublished formatter commit was rebased
onto that exact remote head, with the separate flatfile WIP preserved and
restored without conflict. No published history was rewritten or force-pushed.
The formatter evidence above remains pinned to its original earlier source.

The combined formatter/upstream native tree is
44528a2ff5d25f63df2869e3ae8ac1a9ed325855. All 1,220 native files match the strict
QA, with only CRLF/LF normalized. Strict production SQL, flatfile and pfile builds
pass. The complete formatting contract passes. Account-loader ASan/UBSan worker,
stale-session, admission saturation, cancellation/shutdown and repository
transaction regressions pass; the 24 native watchdog regressions, account
projection source contracts and seven production-service contracts also pass.
Normal 14-fixture accounting validation, matrix --check, all 54 writer-contract
tests and the standalone writer-site contract pass. The first writer-contract
run exposed stale SQL/copyover line references after the remote integration;
four literal tuple anchors and two copyover assignment-line anchors now refer
to the identical original source excerpts. No disposition or proof gate changed.
The archived test setup initially requested the unpublished rebased SHA from
GitHub; the exact published cd4cc413f base is now used for its QA Git index.

SQL binary SHA-256: 9f260bae9e175230aafb8e8c4bd58c544a172045a89aaaf816b6f21ad9b69896.
Flatfile binary SHA-256: 3c856e88cf2b941a2c458a4911ef113b73c482bfca45e94c6e45244de6b39b2d.
QA: /opt/duris-accounting-remote-cd4-formatter-review/source.

Local ignored evidence SHA-256:

- tmp/remote-cd4-formatter-build-checks.local.log: 1a3304a96b9481e3b8e232ac84fde9b1d75832a93b6ecda63c23a4fcc4e434dd
- tmp/remote-cd4-formatter-source-pin.local.log: 657627881729b5d0a6805186d0f3f4776e6295314ce6b9e5c9fe05d9bf6ec534
- tmp/remote-cd4-formatter-native-retry.local.log: 02ddf786341209c9f1583faa02268ffa0a8bdc1e965b9d364d4cc168f870e93d
- tmp/remote-cd4-contracts.local.log: d71406bfa7b7d6c00ce54b4ea3b5d8d6587914f48b3ff3fd56c7b505393dc262
- tmp/remote-cd4-formatter-writer-checks-final.local.log: 18b64619cd99e3aac5dffdce4bab7de401007a277d695e4108863e6648c64f1b

Current integrated gameplay, recovery and broad qualification remain separate
open checks. The flatfile pre-fence repair is not included in this source pin.
Full R1-R8 requirements remain open; coverage_complete=False; release BLOCKED.
No production activation/data or declined inactive spell-path changes occurred.

## Remote PR #686 and fresh native qualification requirement

The second normal milestone push was rejected because experimental-accounting
advanced to 47085b61acf1e688e85363547a8c58c4efdcf743 (PR #686 session queues).
The unpublished formatter milestone was rebased again, preserving the separate
flatfile WIP. The current formatter-only native tree is
ceb8a4a8473d9928cfd373e968ccb3072e18a97a. Its full formatter contract, bounded
session-queue native harness and all 54 writer contracts pass. The semantic
writer excerpts/counts and release blockers are unchanged.

Native build inspection found that copied .d files retain absolute object
**targets** in earlier QA directories. In the cached integrated flatfile-fence
SQL profile, 685 of 708 dependency targets still name the diagnostics QA, and
only four name the current QA; the flatfile profile has the corresponding
685 of 704 old targets. Therefore those dependency rules cannot establish that
unchanged .c objects were rebuilt for new shared descriptor headers. Source-byte
matching and successful cached linker exits do not resolve this. Cached
integrated binaries above, including the cd4cc413f integration, must not be used
as current native qualification after PRs #684-#686. Their logs/hashes remain
historical results. This finding does not retag the pre-integration source-pinned
component and real-journey evidence.

Fresh strict production SQL, flatfile and pfile builds are now running under
new qualified-clean object directories in the 47085b61a formatter QA; no cached
object is copied into those directories. The separate flatfile fence candidate
will be rebuilt from a verified current source with dependency targets rebased
to its own QA. Current integrated gameplay, recovery and broad results remain
open until those actual binaries are tested. No production or native game rule
was changed to work around the build-verification issue.

The unchanged-source frozen 88d3b364c full-world retry passed its inspector
stage, but then hit the server artifact builder's existing 600-second deadline.
It did not reach the full-world player journey. The original three-failure broad
result and this separate retry remain failures; neither deadline was increased.

Local ignored evidence SHA-256:

- tmp/remote-470-cached-dependency-inspection.local.log: 941d795cbe613fab1ab7dfc89479d842746ae295800d745efbb1e14d96dadea8
- tmp/frozen-88-full-world-retry.local.log: 7cdf487b586a4266311d16eb53cdd9ab56bd92136df6349e379295a693f424d5
- tmp/remote-470-formatter-writer-checks.local.log: a985fdaf011dabfe20c397f1ee4779d749faf12dfc36a0a0aabf0c4c6a0c85ef

The native flatfile pre-fence active/corrupt refusal, ordinary metadata save,
paused retry and fresh-load sanitizer component passes under current headers.
Its integrated fresh-binary qualification is still pending. A separate native
probe of committed-fence acknowledgement is in progress and is not a solved
issue. Full R1-R8 gates remain open; coverage_complete=False; release BLOCKED.
The captured backup-generation path remains pending; production activation/data
and the declined inactive spell-path change remain untouched.

## Fresh qualification on remote 47085b61a

The formatter-only native tree ceb8a4a8473d9928cfd373e968ccb3072e18a97a
now passes strict production SQL, flatfile and pfile builds. All server objects
were rebuilt in empty qualified-clean directories, avoiding the copied .d
target defect. All 1,220 tracked native files match the QA after CRLF/LF
normalization. SQL binary SHA-256:
43f350055f084ccd0f562160fcbf873b4cd3d2aba12f591b143744f7c11aa182.
Flatfile binary SHA-256:
c79fea2a073db884216bdabbef257204ad95f5b24a567872617d90b8e0e4dead.
QA: /opt/duris-accounting-remote-470-formatter-review/source.

The fresh SQL binary passes the real inactive deletion journey on both MySQL
and MariaDB, including pre-fence metadata/admission refusal, playable retry,
soft-delete/late-cleanup rollback, deletion once and cold restart. The fresh
flatfile binary passes the existing missing-authority character deletion
refusal, preserved snapshot/quest alias, playable retry, deletion once, alias
erasure and usable account after cold restart. The full formatter contract,
bounded native session queues, currency/item input/output queue and auxiliary
prompt contracts pass. Normal 14-fixture validation, generated matrix --check,
54 writer contracts and standalone site checks pass. These are bounded
qualification results, not a full R1-R8 acceptance or current broad-suite pass.

The separate flatfile pre-fence admission repair remains outside this source
pin and is undergoing fresh-binary recovery qualification. An independent
native fault probe now establishes another open R1/R8 defect: interrupting
after authority-journal commit makes account-save report refusal, while native
journal recovery installs the permanent deletion fence. The corrected probe
explicitly recovers the journal before fresh load; the earlier non-recovering
probe did not establish this failure. This acknowledgement defect needs its own
complete fix and qualification; the admission repair does not claim to solve it.

Local ignored evidence SHA-256:

- tmp/remote-470-formatter-fresh-build.local.log: aa3210f9bd182b17b197b8b46e811ac8460f949e9365d903a8252752b714b3ab
- tmp/remote-470-formatter-source-pin.local.log: 657627881729b5d0a6805186d0f3f4776e6295314ce6b9e5c9fe05d9bf6ec534
- tmp/remote-470-formatter-checks.local.log: 7f9d280c2767c622740b61bf24e851cd70fb34d122c9e943ff8284e0c6a69e7d
- tmp/remote-470-formatter-writer-checks.local.log: a985fdaf011dabfe20c397f1ee4779d749faf12dfc36a0a0aabf0c4c6a0c85ef
- tmp/remote-470-fresh-sql-journeys.local.log: 6c03c25129a06215380a803154f18af5cc07f88bf5923cbc2c3685dfe80bea9a
- tmp/remote-470-fresh-flatfile-baseline-journey.local.log: 8c854b3ad1417d59b836a42fd81eb1ae4637112ca12792be7df502e591f325f4
- tmp/flat-account-fence-ack-red-recovery.local.log: c67788068436b96f7e8860d64bc2569f1f13c4e8f8de0722b10fa62bb3cb52e5

Full R1-R8 requirements and current integrated broad, full-world and captured
generation qualification remain open. coverage_complete=False; release BLOCKED.
No timeout, safety gate, inactive game rule or declined spell-path was changed.
