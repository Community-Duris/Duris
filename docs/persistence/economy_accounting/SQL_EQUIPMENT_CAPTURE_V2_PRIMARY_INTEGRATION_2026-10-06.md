# SQL equipment capture v2 primary integration

Integrated on `6083deabbfe31fcad13699670fd5d5cdf09e245c` after independent
persistence review accepted all seven exact preimages and successors. The raw
item-current-owner projection omitted `equipment_slot`; normalization therefore
left every slot at zero. The omission is repaired without rewriting historical
monetary source evidence. Full R6 opening and release qualification remain open.

## Change and compatibility

Four production files define capture/normalization v2:
`src/persistence/economic_sql_source_snapshot.{h,c}` and
`src/economy/economic_sql_source_normalize.{h,c}`. A separate ordered
`item_current_owner(item_uid,equipment_slot)` projection is read inside the same
existing repeatable-read transaction, session and resource limits. It uses EIE2
framing; ESC2 binds the existing main/item digests and equipment digest. The
physical metadata registry remains23 distinct tables. Original20 table indices,
ten-cell custody rows, ESR1/ESM1/EIM1 bytes, coin-row hashes and lifecycle native
boundary/request hashing remain unchanged.

V2 exposes `observed_equipment_slot` and its separate equipment-row reference,
preserving uint16 values and the existing custody rules. V1 requires absent new
projection/digests and explicitly leaves equipment unobserved. Observed zero is
therefore distinguishable from missing historical evidence. These source hashes
do not grant complete-opening or activation authority; no baseline item insertion,
new source operation, lifecycle selection or production activation is introduced.

The original source test retains its controls and adds equipment framing,
compatibility, limits and contradiction checks. Its actual SQL path now captures
slot7 and restores0, and includes equipment in its original concurrent-update
control. Both affected disposable runners apply existing immutable0038 before
0043. The source runner retains its eleven providers and adds the actual
`shop_trade_recovery_manifest.c` provider required by current item-transfer code.
No migration content, test limits or compiler/sanitizer checks are weakened.

## Verified evidence and limits

Primary verified62 frozen artifacts, all1,305 pinned original source inputs, all
seven preimages/successors and the three unrelated WIP hashes before installation.
Packet: `tmp/sql-equipment-capture-v2-source-20261006`; manifest SHA256
`c9932cdf8c3e78a343df4955e3d54cee5747d3f1acf45d92e7ec6ce9dfd1781d`;
frozen receipt SHA256
`c072f705944e85d766e2a1e6885387b4ce015137b166a73d6d57ca89fd457daa`.

The worker's final unchanged production objects compile in both modes. Final
original SQL harness compilation, actual twelve-provider ASAN/UBSAN manufactured
DTO controls, original client-free refusal execution, clang-format18 and shell
syntax pass. Earlier compile/link failures are retained and corrected; the final
success does not erase them or claim SQL execution.

Primary then ran both original Make production object pairs on the actual
integrated source, plus AST, whitespace, normal accounting contracts and matrix
`--check`. All pass. Receipt/logs:
`bin/tests/sql-equipment-capture-v2-primary-20261006/results.json`.
All920 routes,2,876 occurrences and2,818 unique sites remain unchanged, zero
unmapped; existing source pins bind none of the changed files, so no registry or
matrix rewrite is required. Coverage remains false and release BLOCKED.

Actual MySQL8/MariaDB10.11 capture, repeatable-read/fault/session/allocator controls
and full integrated builds remain in the agreed coherent major batch. Component
DTO checks and object builds are not authentic native opening qualification.
Current inactive behavior, the declined spell-path boundary and unrelated WIP are
preserved. The PA runtime fixture remains unchanged because its empty-lineage
startup exits before source capture.

The [auction/claim implementation handoff](SQL_OPENING_AUCTION_CLAIM_IMPLEMENTATION_HANDOFF_2026-10-06.md)
records the next complete money-opening slice, including mapping recovery and
spendable partial claims. Full physical/item opening, all supported writers and
the authentic independent activation verifier remain original R6 requirements.
