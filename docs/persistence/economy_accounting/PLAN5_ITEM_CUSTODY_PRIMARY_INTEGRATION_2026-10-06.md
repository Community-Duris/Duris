# Item custody-position fix: primary integration — 2026-10-06

Matching opening and current item records could both describe impossible native
custody and report no exception. The completed independent fix validates each
captured position against the original independent custody grammar. Invalid
state/owner/root/parent/revision/equipment combinations now retain UID-only
findings even when both projections agree. Existing topology and equipment
checks, accepted native states, destruction tombstones, creation openings,
unknown historical equipment and original detail/privacy limits remain.

Five owned blobs are imported exactly from peer
`560f91d9d9d0d38748b0cd529b63441b6478a5ad`. All preimages match its parent;
all code/test inputs now match the qualified peer composition. Native tree
`bf7a92a728ad9b5b813626462e56533f8ba39c97`, migrations
`2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d` and the complete central regression
manifest remain exact. Unrelated SHOP harness edits and the untracked restore
report remain untouched. No shared accounting contract or activation changes.

Primary executes the two new custody methods and affected topology method:

```text
python -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests.test_static_custody_positions_cannot_share_an_impossible_opening ReconciliationTests.test_static_custody_preserves_native_states_and_creation_openings ReconciliationTests.test_native_topology_cycle_and_edge_diagnostics -v
```

All3 pass, zero skips, unittest21.622s (complete process22.312s). Actual cases
include18 matching impossible positions,24 accepted live/quarantined owner
controls, tombstones, original creation/historical omission, all54 custody CLI
cuts at detail limits0/1/100, input immutability/privacy and unchanged original
topology findings. All three imported Python files parse. Normal accounting,
generated matrix and current61 runtime metadata checks pass. Coverage remains
false and release BLOCKED; these checks do not qualify native gameplay.

Import receipt SHA256 `1c72db4f5944d6eae5bfadf88e7a3fcb1712ef1dc843e67aec1345f1d51a04fe`;
primary actual-check receipt SHA256 `07c45b2414ef68d001f8cafaa14f2a866bd3375fcec0ebd651734f6eb941be69`. Both receipts
and full logs are retained under `tmp/plan5-item-custody-primary-*20261006*`.

[Peer qualification](PLAN5_ITEM_CUSTODY_POSITION_QUALIFICATION_2026-10-06.md)
reports201 Linux methods,118 Windows reconciliation methods,51 fresh native
grammar cases in each original SQL/client-free closure, and the complete
original baseline recipe on MariaDB10.11.14/MySQL8.0.46, zero selected skips.
It distinguishes authentic native books from modeled placements/current-row
damage and synthetic matching-invalid openings. Its original recipe and source
integrity remain preserved. Primary verifies exact source/report correspondence;
protected peer logs were not independently inspected, and no native recipe was
rerun here. This component does not qualify complete authority capture, full
service restore, private birth/recovery source, any whole Plan or release.
Accounting stays inactive and the declined inactive spell path stays untouched.
