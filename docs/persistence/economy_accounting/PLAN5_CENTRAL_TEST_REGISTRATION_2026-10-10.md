# Central accounting test registration - 2026-10-10

The integration manifest now requires the existing native captured source-binding
case by name in the native-origin suite. It also registers dedicated invocations
for all five captured-item binding cases and both Linux audit-budget cases, with
the budget suite's existing opt-in. All original rows, engine pins and execution
controls are preserved. The existing runner already rejects missing, failed or
skipped required cases; the change makes these existing cases mandatory in the
major qualification batch.

Independent finite review authenticated all eight added case names against the
existing source, exact one-case/two-row delta and inverse to the original object.
The maintained runner's --list validation passed and selected three once rows:
native_sql_baseline_origin_audit (3 required), plan5_captured_item_bindings_pure
(5 required) and plan5_audit_budget_linux (2 required). The manifest has114 rows.
No test bodies or Plan5-owned implementation changed; fixtures have not run in
this checkpoint and remain pending for the major-plan batch.

The manifest pin and derived writer matrix are refreshed. Registry478 source
pins,931 unchanged policies,2911 census occurrences,2853 mapped sites and zero
new/unmapped writers are inventory only. All477 other pins and protected WIP
are preserved. Complete host, gameplay, persistence, recovery and release
qualification remain OPEN. Accounting inactive, admission CLOSED, coverage
incomplete, release BLOCKED, goal ACTIVE.
