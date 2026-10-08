# Accounting test inventory and lifecycle component registration - 2026-10-07

Central inventory previously raised `regression inventory mismatch` for three
tracked accounting tests: `test_auction_retained_seller_fee.py`,
`test_inert_money_literal.py` and the newly imported
`test_flatfile_lifecycle_v2.py`. The failure was reproduced before any manifest
change. The first registration helper stopped before edits because it expected
only the newly imported omission; the full observed three-file failure guided
this complete inventory fix. No failed native build was retried.

The first two are explicit automatic native script entries. They keep original
no-argument drivers, real codec providers, sanitizer flags and internal budgets.
Their purposes preserve extracted-fee/registry-seam component limits; registration
is not runtime, SQL, boot, gameplay or release qualification.

The lifecycle V2 component is a recovery/manual script with its required source
checkout and fresh artifacts supplied by one existing offline matrix row:
`python tests/async/test_flatfile_lifecycle_v2.py --native-source .
--artifacts {evidence}/native`. It runs once, with the existing900-second matrix
budget; original600-second build and45-second case budgets remain. Every original
row, top-level field, test body and native compiler/provider list is preserved.

Actual `inventory()` and matrix `workload()` validate:926 tests and107 rows.
Central integration/recovery `--list` selects the new row/script correctly.
The existing result contract accepts the complete summary and refuses controlled
link-failure, partial-case, missing-envelope, skipped and unobserved-script results.
These are result-enforcement checks, not executed native cases. The row requires
65 coverage cases,12 accepted witnesses, five envelopes and zero skips; native
V2 installer qualification remains separate. No suite or evidence gate is removed.

Manifest pins and actual source/enforcement receipts:
`bin/tests/plan5-lifecycle-v2-registration-primary-20261007/RESULT.json`.
All previous JSON rows remain semantically exact; no runtime/services/build occurs
in this registration milestone. [The implementation evidence](PLAN5_LIFECYCLE_V2_PRIMARY_INTEGRATION_2026-10-07.md)
retains the original component link failure on missing WSL math libraries and its
unexecuted runtime scope. Major-candidate native qualification, census/activation,
Plan5 full operator integration and R1-R8 release remain open. Accounting stays
inactive, writer coverage/gates and declined spell behavior remain unchanged.
