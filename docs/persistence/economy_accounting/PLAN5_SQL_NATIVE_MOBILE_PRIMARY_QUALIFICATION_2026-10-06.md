# Retained SQL NPC value audit: primary integration — 2026-10-06

The SQL restore/canonical audit previously checked retained NPC metadata without
independently decoding the saved native value. Primary integrates Plan5's solved
51cfdc7b14fc45649e74b5e67e042c86df80f27f reader slice: actual QMNIMG1/2/QMNREF
grammar, checksums, complete stock/DFS/equipment/string/value bounds and literal
cash. SQL lifetime ID, stock/mobile revisions and live/retired state must match
the decoded image. Historical v1 cash remains unobserved. No NPC is created,
no source/birth authority is supplied and audit consumers remain SELECT-only.

Both consumers use bounded256-row unsigned lifetime pagination,64KiB capsule
chunks,4MiB images and100k-row/32MiB audit limits. The retained_native_mobiles
count is reported. Five Python inputs match the peer successors except the
primary runner's exact migration-head assertion now follows canonical0062.
Only the incoming audit paragraph is added; existing operations prose remains.

All26 original RestoreProjectionTests/CanonicalAuditTests pass with zero skips.
The original full test_restore_economic_coin_effects.py passes in
549.370s with fresh native SQL/flatfile compilation,32 native
cases and3026 matching independent decoder decisions. Both fresh private
MariaDB10.11.14 and MySQL8.0.46 engines explicitly query/assert canonical0062.
Each passes45 NPC cuts within84 canonical cuts,72 refusals and51 full-entry cuts,
including259-root pagination and damaged second-page evidence. Retained rows
and authority remain unchanged. No flags/providers/deadlines/assertions were
lowered. The initial real0062 executable-mode failure was separately repaired
and committed as 4e006c3a738d00c19ab3d09f573b57c5115e2033; both failure and successful retry are retained.

Exact archive/per-file pins and original full logs/results are under
bin/tests/plan5-sql-native-mobile-primary-retry-20261006; component/installation
and first failure evidence are under bin/tests/plan5-sql-native-mobile-primary-20261006.
Retry archive SHA2560bac35b5818bdf5414216c32477244bb69f0e67961a65669046ab1c836d03fc1; log SHA256a12e0a61cd5f0ce1f758f9f1faafda37c80247d75f9888f0a6bcd7f645961e67.
Original summary JSON's fixed61 label is stale; actual executed setup/head
assertion proves62. Temporary native binaries are observed hashes, not retained
files. Earlier peer full flatfile native-image controls are separately attributed
in PLAN5_NATIVE_MOBILE_RESTORE_PRIMARY_QUALIFICATION_2026-10-06.md.

This closes the independent SQL saved-image omission. Synthetic/native audit
evidence does not qualify real producer journeys, birth/source integration,
paid quest, auction opening/spending, held retirement, cold ACK or full Plan/R1–R8.
Those implementation/qualification gates remain open. Accounting stays inactive,
coverage incomplete and release BLOCKED; unrelated WIP and declined spell change remain.
