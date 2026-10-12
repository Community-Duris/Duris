# Accounting branch cleanup receipt

The user requested removal of obsolete agent communication commits and a new
completion plan while the implementation agent is stopped. This candidate
consolidates 17 accumulated planning/coordination documents (5,300,345
original Git bytes). It preserves the complete technical requirement contract,
final paused handoff and portable candidates.

## Source and history

- Original paused tip: `11ea970dc616a04069cda703d3afe9cc02b4c7d1`.
- Original history retained in local `codex/accounting-before-cleanup-20261011`
  and the existing canonical branch pending adoption.
- Cleanup branch: `codex/accounting-completion-cleanup`.
- Rewritten implementation tip before final documentation: `3ef06ece496b1fce6da1fa651221c342a139d899`.
- Rewrite boundary: commits reachable from the paused tip and absent from
  master `f45d71accb83e1e864bf75b910693790b765d4df`; master/external parent history is unchanged.
- Examined commits: 1397; redundant commits removed: 169.
- Mechanism: remove only the consolidated paths from candidate historical trees;
  discard a commit only when its resulting tree equals its sole retained parent;
  retain other changes and merge ancestry. Final documentation restores original
  entry-point paths as concise pages and adds this receipt.
- Full original-to-candidate map: `D:/Dev/Temp/accounting-completion-cleanup-20261011/rewrite-map.json`.
- Validation receipt: `D:/Dev/Tests/Duris/accounting-completion-cleanup-20261011/validation.json`.

This is a history rewrite proposal. Original source SHAs in technical evidence
stay original; do not repin them or claim a rerun based on new commit identities.
Every preserved path has the same Git blob and mode as the paused source,
including all runtime/build/tests/migrations, metadata, dated reports and patches.
The 511 source-pin maps and writer policies/census are preserved without edits.
No runtime or acceptance change is introduced.

## Consolidated paths

- `ACCOUNTING_ACCELERATION_CHARTER_2026-10-11.md`
- `ACCOUNTING_WIP_HANDOFF.md`
- `DELIVERY_PLAN.md`
- `EXPERIMENTAL_REVIEW_CHECKPOINT.md`
- `FINISH_ACCOUNTING_IMPLEMENTATION_HISTORY.md`
- `FINISH_ACCOUNTING_PLAN.md`
- `REMAINING_REQUIREMENTS.md`
- `REVIEW_CONTINUATION_2026-09-30.md`
- `REVIEW_CONTINUATION_2026-10-01.md`
- `REVIEW_STATUS_2026-10-01.md`
- `REVIEW_STATUS_2026-10-02.md`
- `REVIEW_STATUS_2026-10-03.md`
- `REVIEW_STATUS_2026-10-04.md`
- `SHARED_STREAM_HANDOFF.md`
- `domain-separation/CONTINUING_PROJECT_COORDINATION.md`
- `domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md`
- `domain-separation/WORKSTREAM_COORDINATION.md`

Old raw status, assignment and polling text is available through each compatibility
page's original-history link. The required architecture instruction, R1–R8,
product decisions, owner boundaries and current dependencies are restated in the
completion plan and requirements. Detailed source contracts and qualification
reports elsewhere are retained, including documentary commits with genuine
technical content. Neither a docs-only label nor an agent author is a deletion rule.

## Adoption procedure

1. Review the tree diff against the original paused tip; it must be confined to
   these documents and this new receipt.
2. Preserve/publish the original backup ref before replacing shared history.
3. Confirm the implementation owner remains stopped and all dirty work/private
   packets are retained. Recheck the remote head is exactly `11ea970dc616a04069cda703d3afe9cc02b4c7d1`.
4. Replace the canonical branch only under explicit user authorization, using
   an exact `--force-with-lease=refs/heads/experimental-accounting:11ea970dc616a04069cda703d3afe9cc02b4c7d1`.
   A changed remote tip requires reconciliation; do not override the lease.
5. Rebase/recreate helper work from retained patches and the commit map on the
   cleaned head. Preserve existing dirty checkouts; do not pull divergent old
   history back into the new branch. Rebind prospective test recipes explicitly.

The cleanup itself requires static/document validation and full blob/mode
equivalence, not new C++ builds or database cohorts. Those are required on actual
implementation resumption and the final combined candidate. No container or
game/database service was started, stopped or removed for this cleanup.

Validation: accounting contracts pass (14 fixtures, 931 writer routes, 2,911
candidate sites, release_ready=False). R1–R8 table rows are exact; all local links
and whitespace checks pass. All 6,941 retained file modes/blobs match the paused
tip. The existing coverage-contract suite has 54 passes, two failures and one
error: stale bandage anchor, ambiguous room-payload selector and stale test-entry
source hash. The same three failures reproduce on a separate untouched checkout
at the original tip; their repairs are recorded in the completion plan. They are
not waived or claimed green, and no runtime release qualification is inferred.

## Examples of removed redundant commits

These commits became empty after consolidation (or their merge parents collapsed
to a single identical candidate parent). Original objects remain recoverable in
preserved history. The full mapping records every removed commit and its exact
retained replacement; the examples below are a small review sample.

| Original commit | Subject |
| --- | --- |
| `eaf8d529fd9d` | docs: plan accounting completion and save stability |
| `0a1d7e1badb7` | Record current dual-engine death and resurrection accounting gap |
| `33d23aeec438` | docs(accounting): consolidate remaining issue tracking |
| `930886421211` | Record parallel ownership and major-plan qualification cadence |
| `e55f1eba63de` | Record completed combined-source retention checks |
| `e453f41e76c2` | Record current source preparation and remaining shared ownership gates |
| `d35dbaba2100` | docs(accounting): reuse PR 678 source research |
| `17c033d69316` | docs(accounting): define parallel quest acceptance prep |
