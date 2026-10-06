# Historical child-cut equipment expectation: primary integration — 2026-10-06

The old SQL child projection intentionally omits child identities, equipment and
original plan evidence. Its native integration assertion expected only child
and plan findings, although the independent auditor correctly also reports
`missing_item_equipment_evidence`. The exact owned successor from
`98abfa4ec6f19faf58a64dff095caec6cf92f78e` retains all original findings:
`missing_child_identity_evidence:2`, `missing_item_equipment_evidence:1`, and
`missing_original_plan:1`. The legacy cut remains incomplete; no reader,
production source, fixture capture or old projection is changed.

Primary verifies the exact peer preimage/successor and Python grammar. All21
original `ChildIdentityTests` run with zero skips on the current independent
reader; command and terminal log hashes are retained in
`bin/tests/plan5-child-equipment-primary-20261006/results.json`. These component
methods do not execute the edited native/private-SQL branch. Plan5's complete
native child method passed on its older pinned source (two configurations and
engines,14 native cases,30 read-only captures,20 historical comparisons), as
reported in [its published qualification](https://github.com/Community-Duris/Duris/blob/4b86e892cae1fc9e0eefc29dc52e2549706e4460/docs/persistence/economy_accounting/PLAN5_NATIVE_BIRTH_AUDIT_RECIPE_QUALIFICATION_2026-10-06.md).
The external protected artifacts have not been locally re-inspected. Current
combined native/SQL qualification remains pending at the major boundary.

This solves the stale expectation without making legacy evidence valid.
Full R6, R7/R8, producer/cold-recovery and release gates remain open. Accounting
stays inactive and unrelated WIP remains untouched.
