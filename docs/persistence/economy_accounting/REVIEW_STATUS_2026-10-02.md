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
runtime-state table. That separate fixture repair and the complete dual-engine
save/death/crash/copyover run remain pending. No gameplay completion is claimed.
