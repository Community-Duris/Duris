# Accounting review status - 2026-10-01

This checkpoint publishes the completed review fixes directly to
`Community-Duris/Duris:experimental-accounting`. It integrates upstream through
`99ae211a9cab4dc842e1fa73e2150b1063d72807`, preserving the canonical, staging
0045, and master 0031 migration histories. The master upgrade now appends the
0052 witness index and 0053 craft receipts without rewriting its existing prefix.

## Completed fixes

- Recipe Craft/Forge now use frozen item retirement, output publication, and
  durable progression receipts across SQL and flatfile recovery. Migration 0053
  supplies the SQL progression authority. See commit `bc4aeab0a` and its
  preceding focused repair, crafting, and pouch commits.
- Save admission integrates the upstream creation and sealed-save fences with
  the broader item-creation publication fence (`eeac01351`).
- The sanitizer prompt fixture now compiles without expensive optimization
  within its existing bounded runtime test (`77f95ef9f`).
- Null recipe output pointers are rejected before reading item identity, with
  actual movement/publication regression coverage (`0464613a1`).
- Recovery evidence updates are integrated, and recipe writer coverage is
  refreshed while retaining upstream creation-backend evidence (`72b3a5ab6`).
- Worker admission now handles an older save ACK that narrows the queued
  component mask while a newer capture waits for journal append. The actual
  worker regression fails before the fix and passes afterward; missing
  components and mismatched revisions remain rejected, and the craft receipt
  reaches its exact successful completion.
- Progression publication now keeps an admitted Craft/Forge save pending until
  its exact completion, instead of issuing a new revision every 500ms. A failed
  completion rearms capture without granting XP or rerolling a skill notch.
  The actual owner regression fails before the fix and passes under ASan/UBSan,
  covering delayed queued and coalesced saves, failure retry, and exact ACK.
- Mini-mode startup now installs progression callbacks before any player
  materialization. Both strict production builds passed, and the actual
  flatfile server completed mortal Craft and Forge with exact materials/tool
  retirement, fresh output UIDs, and durable XP. Retained-pouch and SQL gameplay
  qualification are still running.

## Verification and its limits

| Check | Current evidence |
| --- | --- |
| Strict SQL and flatfile production builds | Passed for the source at `77f95ef9f`; the final merged SQL boot contract still needs a rebuild. |
| Recipe SQL receipts and fault recovery | Passed on disposable MySQL 8.0 and MariaDB 10.11 before the latest upstream integration. |
| Integrated focused runtime checks | All 16 selected checks passed after the first upstream integration, including the ASan/UBSan prompt fixture. |
| Null-output publication regression | Passed using the actual native movement module after `0464613a1`. |
| Combat continuation and quarantine restore | Passed after integrating the upstream recovery evidence. Quarantine archives retain both original component frames and keep recovery fenced on conflicting evidence. |
| Writer census | 2,806 lexical occurrences, 2,748 unique sites, 863 routes, zero unmapped sites; 52 coverage-contract tests passed on the preceding census snapshot. |
| Recipe flatfile restore | Passed through the actual native restore decoder: complete recovery, checksum damage, missing obligation/root, future revision, and invalid receipt filename. |
| Latest migration integration | Runtime manifest validator and all 34 native boot/migration contract tests passed with head 0053 and 225 tables. All three histories converge on the pinned schema on disposable MySQL 8.0.46 and MariaDB 10.11.14, including preserved master receipts/runtime payloads, append/replay, shell/compiled boot, restore selection, and tamper refusal. The fixture uses the current sealed baseline with the master prefix; a captured production clone remains outside this proof. |

Prior build and component passes do not establish an all-green result at this
publication commit. Detailed synthetic fixtures remain in `tests/async/`; local
logs and disposable binaries are not published as production artifacts.

## Remaining work

The accounting release validator remains **blocked**: 750 runtime/projection
writer routes still lack the required qualification. The generated coverage
matrix describes that gap; a complete census alone cannot close it.

Recipe flatfile restore qualification now passes with a complete canonical
inventory save and explicitly requested progression receipts. Mortal Craft/Forge
restart journeys and the proposed mini-mode startup initialization fix remain
in progress and are not yet published.
The latest merged sources also require fresh strict builds and broader regression
coverage. Earlier full-suite failures have not been superseded by a complete
passing run.

Paid same-UID superior enhancement, remaining pouch writers, day-one quest and
loot paths, audit completeness, activation/recovery/backup/retention gates, and
operator policy decisions remain open under [the completion plan](FINISH_ACCOUNTING_PLAN.md).
The branch is **not yet ready to merge or activate**. This push records current
implementation and review status; it performs no deployment or production action.
