# Task Checklist

**Session ID**: `phase00-session05-combat-artifact-persistence-correctness`
**Total Tasks**: 12
**Created**: 2026-08-27

---

Legend: `[x]` completed; `[ ]` pending; `[SNNMM]` session reference.

## Setup And Inventory

- [x] T001 [S0005] Confirm Session 05 selection, clean base `20e13a1a`, and local/development safety context.
- [x] T002 [S0005] Trace victim frag publication through `AddFrags()` and `sql_modify_frags()`.
- [x] T003 [S0005] Inventory every bind lookup outcome and all three direct callers.

## Implementation

- [x] T004 [S0005] Apply victim frag loss before the durable update without changing formulas or messages.
- [x] T005 [S0005] Change bind lookup to return status and initialize valid outputs before querying.
- [x] T006 [S0005] Handle query, result-allocation, no-row, fetch, null-column, malformed, and range outcomes deterministically.
- [x] T007 [S0005] Update all direct artifact callers to fail closed on lookup failure.

## Tests And Validation

- [x] T008 [S0005] Add focused victim-order and bind-result source contracts.
- [x] T009 [S0005] Run the focused regression and nearest persistence regressions.
- [x] T010 [S0005] Run changed-line formatting checks and `make -C src`.
- [x] T011 [S0005] Run `make test-all`, whitespace, and file-integrity checks.
- [x] T012 [S0005] Complete review, repair any findings, and validate the session.

## Completion Checklist

- [x] All 12 tasks complete
- [x] No outstanding blocker or unresolved test failure
- [x] Ready for `creview`

## Next Steps

Run the `implement` workflow step.
