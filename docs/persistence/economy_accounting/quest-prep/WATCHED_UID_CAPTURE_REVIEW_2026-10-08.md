# Watched-UID capture independent review - 2026-10-08

Disposition: PASS for the incremental owned reader and focused unit scope.
Code `cf0bb0a977e6995946c235b70feaaa604dc3f689` and
[handoff `807f5b2323c1b7da12c17b9e07821847807993f6`](https://github.com/Community-Duris/Duris/blob/807f5b2323c1b7da12c17b9e07821847807993f6/docs/persistence/economy_accounting/quest-prep/WATCHED_UID_CAPTURE_HANDOFF_2026-10-08.md)
are remotely available on the existing quest-prep branch. No primary adoption,
genuine SQL, gameplay, fault/recovery or native authority qualification is claimed.

## Exact scope and independent checks

The code commit changes only the owned capture reader and new focused reader test;
the following handoff commit adds only its assigned document. The coordinator
authenticated the six exact preimage/result/dependency Git blobs in the handoff:
reader preimage `ecf52e1ec9dbdd0a72e0eb01890f4e099aa5f757`, result
`8a079422a452028e4ff8c5e94f8031fa9b350358`, test
`5d4aa65a80cb2d779a9b45ab0daeb3e39b47e462`, unchanged case data
`44c08a70031bde27ebb9a0996c2706e93a5cf8d0`, unchanged adjacent assertions
`d6c149c4632ecc939fd388c403883149ac5ce2f2`, and maintained mobile decoder
`06a2ebeaddd4bf604990b0f6cacf0c842555ccf3`. Both commits pass whitespace checks.

The coordinator read the complete code diff and new tests, then independently ran
these exact maintained entry points from the clean published quest worktree under
Ubuntu-22.04 WSL, with TMPDIR/TMP/TEMP all `/mnt/d/Dev/Temp`:

```text
python3 -B tests/async/quest_accounting_prep/test_capture_quest_cut.py -v
python3 -B tests/async/quest_accounting_prep/test_quest_cut_checks.py -v
```

PASS: all 19 reader tests and all 17 existing assertion tests. The coordinator
also inspected and executed the worker's temporary preimage-comparison script:
all 14 cases (QP01-QP07, native-shaped and legacy fake cuts) retain byte-identical
serialized no-option results, identical SQL/bindings and cleanup when comparing
the actual parent Git reader with the result. This script remains private scratch
on D:, outside Git. These are fake-cursor reader/oracle results, not SQL execution.
Worker-reported Windows results and its existing Linux-only `fcntl` limitation
remain separately recorded in the handoff; no portability repair was imported.

## Behavior and limits

The optional repeatable CLI `--item-uid` / keyword-only `item_uids=()` validates
strict integer native UID range, duplicates and at most 2,048 selections before
cursor construction (before connecting in the CLI). Existing allocator ranges
exclude UINT64_MAX. A parameterized OR predicate keeps watched current rows when
they leave ordinary owner/VNUM filters. History uses observed current UIDs plus
all watched UIDs, including those without current rows. Nonempty selections
record exact sorted watched and missing UID lists. Missing rows stay missing.
A supplied root does not implicitly include descendants or establish completeness.

Original defaults, filters, epoch/engine/legacy gates, disposable target and binary
safeguards, read-only repeatable-read transaction, row/byte/BLOB limits, rollback,
transaction-close verification and cursor/connection cleanup remain. Focused
controls exercise moved/destroyed foreign-VNUM descendants, missing-row history,
nonrecursive parameterized selection, malformed/excess selections and original
refusal/cleanup behavior. No owner codec, migration, source policy or authority
was changed; history can still refuse at the original row/byte budget.

The owned reader is absent from observed primary. Select its previously reviewed
pack dependencies before importing this incremental code; do not merge the prep
branch's older production tree. Exact preimages and limits are in the handoff.
Availability and this review add no primary adoption wait.

## Current primary observation and next bounded work

Observed primary `84a435c3deca43258c0c804ec16cc964197c2aea` adds only six
published documents. Source tree `833d3085815b396861ad18a77635412212381e4b`
and migrations tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` are unchanged.
It reports a private 115-production-file candidate and unexecuted major-plan
qualification. Warm forest/admission/current cut/comm/recovery wiring, typed SHOP
root/receipt/player participant and keeper-wallet/treasury classification remain
owner dependencies. In particular the reported keeper cash discrepancy must not
be hidden by omitting native mismatches or inventing classification. None of that
private implementation is available to this review or creates a native quest hook.

Next quest delivery is a separate native NPC cost cut assertion and focused tests,
using actual maintained mobile/account decoders and existing bind/book patterns.
Owned new paths only: `tests/async/quest_accounting_prep/native_quest_cost_checks.py`,
`tests/async/quest_accounting_prep/test_native_quest_cost_checks.py`, and
`quest-prep/NATIVE_NPC_COST_ASSERTION_HANDOFF_2026-10-08.md`. Check QP02 paid
terms against explicit original instance, observed wallet mapping account and
original cost operation: exact cash vectors/revisions, transition, linked effect,
denominated debit/opposite sink postings, and no second player debit. Isolate
prior funding in its own interval. Do not change historical `static_complete`,
retirement, capture or other existing controls, copy wire codecs, implement a
native owner, or treat supplied mapping IDs as authentication. Missing same-cut
mapping/physical publication and real paid-branch reachability stay explicit.
Pure cut assertions and rejection sensitivity are executable preparation; genuine
SQL/native qualification still depends on authentic original owner inputs.
Architecture's already active currency blueprint continues without interruption.
The broad native Goal remains BLOCKED and heartbeat ACTIVE; neither completed
reader work nor the next finite assignment completes the accounting project.
