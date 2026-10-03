# Accounting WIP checkpoint — successor handoff

The user requested a fast checkpoint commit and transfer to another agent. This is **unfinished work, not release/activation qualification**. No production mutation, migration execution, or accounting activation was performed in this wrap-up.

## Included implementation

The source packet has 38 implementation/test/migration paths, assembled in `/opt/data/workspaces/duris-accounting-batch-223300-integration-07` on `63239882e7eb57be707a958d7caf9b97a9d51e99`:

- Schema-36 activation-receipt DDL/bootstrap/registration and its qualification fixtures.
- C13/C18 exact-session, data-only receipt reader and focused regression coverage.
- C22 private SQL wallet-root qualification projection, including refusal of fresh and already-frozen standalone item roots. This is not production activation.
- C23 opt-in retained-publication transaction protocol and lifetime-fence handoff. Parent corrected a session-loss fixture that incorrectly required zero COMMIT calls after its successful COMMIT; it now compares against the pre-fault counter baselines. Ordinary terminal behavior is intended to remain unchanged but the new SQL protocol is not parent-qualified.
- C24 stale source-contract updates, intended for the still-pending measured schema-36 runtime pins.
- Prior item-state/Wind Blade fixture corrections, not a completed player journey.

The checkpoint is carried forward onto the then-current remote branch; concurrent upstream commits must be preserved. Upstream through `654bcf474f25220fc59994caaaeb217fa5daddcc` includes a new staged-epoch activation routine (`ac0fac4e0`) and corpse/item and writer-evidence changes. These newer changes were **not qualified by this session**. Reconcile that activation routine with the receipt/retained-publication contract before use.

## Exact verification boundary

Evidence root:
`/opt/data/workspaces/duris-persistence-tools/batched-accounting-20260926-223300`

At the pinned integration-07 source, direct source checks passed: retained-publication contracts **7/7**, wallet-root contracts **5/5**; the new counter contract first failed against the uncorrected fixture as intended. Formatting and diff checks passed.

`integration-07/parent-components-01/result.json` contains recorded zero exits for:

1. Diff check.
2. `make test TEST_MATCH=economic_sql_retained_publication TEST_JOBS=1 CXX=g++-14` (source-contract target plus its project prerequisites).
3. `make test TEST_MATCH=economic_gameplay_authority TEST_JOBS=1 CXX=g++-14` (native admission fixtures).
4. Wallet-root source contracts.
5. Formatting of the selected C/C++ files.

**That batch was interrupted. Its last saved status is RUNNING, no task process remained on inspection, and it has no successful final source-hash/lease-release/negative-control completion record. Do not promote it to an overall PASS.** Preserve its original logs. Earlier integration-06 component/negative-control evidence applies only to its recorded older source.

Full current-server build/link, C23 real SQL protocol, current combined boot, genuine positive SQL receipt readback, dual-engine ordered schema-36 migration/replay, and the full Wind Blade journey are **not complete**. No claim here supersedes these gaps.

## Important remaining work

- Runtime manifest/header pins are still the old 217-table values. Schema-36 source adds the receipt table. Measure the real 218-table metadata on both supported engines; do not invent fingerprints or treat the new source-contract expectations as already passing. Do not apply migration 36 to a live database from this checkpoint.
- C24's parent driver is outside Git at `integration-07/verify-schema36-parent.py`, with its exact input manifest at `integration-07/prepared.json`. It is pinned to the preserved integration-07 worktree, not automatically to this new commit. Its original Git-porcelain parser defect was reproduced and corrected; strict **read-only** preflight passed (`integration-07/schema36-readonly-preflight.json`). No SQL matrix was executed. Re-pin deliberately for any changed source.
- Untouched schema-35 comparison checkout: `/opt/data/workspaces/duris-accounting-batch-223300-schema35-reference`, at `c79725aa4dc7e0f1d0dd65d41c08bf3523cd6e0c`.
- C28 report: `reviews/c28-upstream-coin-baseline/review.md`. It reports that the receipt reader rejects pile-bearing baseline witnesses, that the schema-36 fixture seeds no coin piles, and conditional tombstone/custody validation risks. **Parent reconciliation of that report was interrupted**; verify these against the latest source. It is not runtime evidence.
- C26/C27/C29/C30 read-only review deliveries still require intake. No live subagents remained at wrap-up; absence from the live list is not qualification.
- Preserve C21/C25 corrections: do not resurrect the disproven active-transit reachability claim, and do not wire the unsafe flatfile repair helper merely because it exists.
- Wind Blade still needs post-reconnect SQL UID/timer/flags/custody checks, expiry/save/reconnect, and verified cleanup. The display predicate is `a slender sword of vapor`, not keyword `blade`.

## Source/evidence provenance and operating boundary

- Complete frozen original packets and the 38-path merge manifest: `integration-07/source-intake-01/intake.json` and its `inputs/` directory.
- Original worker handoffs: `tasks/c23-retained-publication/` and `tasks/c24-schema36-runner/`. C23 exceeded the patch-only rule by running expensive checks; its build/test claims were not adopted as parent qualification.
- Parent-only native/Docker/SQL work must use both existing resource leases from `verify-c07-windblade-parent.py`; preserve the exact lock inodes. Workers remain patch-only or read-only.
- Keep all work local/disposable unless the user separately authorizes operational changes. This checkpoint is not permission to activate accounting or deploy.
- Original worktrees, immutable packets, logs, and external drivers were retained for the successor rather than cleaned up.
