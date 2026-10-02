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
