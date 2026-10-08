# Room-reset retained origin: Plan5 interface - 2026-10-07

This interface is source-reviewed and private. The combined candidate is
`tmp/lifecycle-reset-transaction-candidate-primary-20261007/candidate` with
68 production files,23 original fixtures and five schema/manifest files.
Nothing in this report qualifies actual resets, activates accounting or grants
publication/ACK. No compiler, native, SQL, migration or gameplay run was performed.

## Ownership and immutable storage

Primary owns the command22/writer16 codec/compiler, root coordinator, original
producer, literal/coin/artifact participants and activation/census integration.
Plan5 owns independent reconciliation, audit/export, backup/restore and release
qualification. Consume the narrow contracts below; keep producer and authority
changes with the primary. No policy decisions or runtime fingerprints are inferred.

`zone_reset_item_birth_origin` is an additive0065 table with four NOT NULL columns:

| Column | Binding |
| --- | --- |
| root_item_uid BIGINT UNSIGNED | Nonzero primary key; exact typed48 root UID |
| birth_operation BINARY(16) | Nonzero unique; inbox FK, update/delete RESTRICT |
| room_revision BIGINT UNSIGNED | Exact typed48 destination revision |
| canonical_origin LONGBLOB | Canonical ZRO1, maximum524352 bytes |

There is no mutable-current-custody FK. Insert byte-identical immutable origin
in the original successful SQL transaction after exact inbox/outbox/indexed
evidence and before COMMIT. Existing unequal origin refuses; replay never
backfills a missing origin from present holdings. This records committed issuance,
not completion of physical publication. Controller/export decisions stay pending.

ZRO1 layout is little-endian: magic4 `ZRO1`, version u16=1, reserved u16=0,
command length u32, result length u32=48, full canonical command bytes and exact
TIR48 bytes. Maximum is16+524288+48=524352. The full command preserves original
accepted time, keys/revisions, intent, invocation/slot, season, literal forest,
recipes and explicit coins. Digests alone cannot reconstruct those inputs.

## Exact historical proof

Root exports `economic_sql_zone_reset_item_verify_retained(MYSQL*, const
critical_command&, unsigned int result_code, std::span<const uint8_t>) noexcept`.
Origin reader `zone_reset_item_origin_sql_lock(MYSQL*, const critical_operation_id&,
zone_reset_item_retained_origin*) noexcept` returns authenticated original command
and result, with absent origin remaining unknown. It authenticates historical
root evidence before locking/rereading the immutable carrier. Both borrow the
existing reconnect-disabled IN_TRANS session and grant no transaction or ACK.

Recompile the exact command; compare complete canonical intent/plan and every
indexed effect/posting/item/reference/source row with total-count refusal for
extras. Require committed success, original command/key hashes, type22/schema2/
payload1 and exact result. Root result: first UID/count, from_revision0,
to_revision=expected_room_revision+1, max_item_revision1, corpse0, collectorfalse.
Inbox durable_revision equals that same actual room revision. Root outbox retains
existing item destination4/event1/version1/TIR48, index0; there are no fake pile
children or currency rows. Review corrected a copied native revision1 constant.

Creation ledger uses from-owner `(0,0,0)` and revision0, room destination `(3,
room_vnum,0)`, room revision above, item revision1, equipment0, reason_type2,
reason_id0 and zone_event source. Preserve original event order/topology and refs
before0/after1, legacy root/event correlation. Historical proof never requires
original room placement, current quantities, current season or accounting epoch.

Plan5's existing `verify_canonical_root` projects absent before-owner `(7,0,0)`.
Add an explicitly authenticated writer16/type22 branch for this `(0,0,0)`
contract; do not globally reinterpret other retained histories. Money evidence
uses actual pile UID accounts, including zero effects/revision1 and nested coins.
Current writer5 transfer proof and complete reset forest recovery remain distinct.

## Registration and qualification handoff

The private0065 SQL and read-only metadata script have all three private migration
manifest successors with exact checksums. Existing immutable migrations/baseline
are preserved. Primary runtime registration remains pending: actual table/check
enumerations, migration history heads/digests and both engine-measured fingerprints.

Coordinate protected lifecycle registration and excluded `canonical_origin`
disclosure; add inbox dependency and recovery/audit purpose while preserving
pending controller decisions. Extend original canonical audit and restore tools
with bounded reset-origin enumeration/decoding, exact original command proof and
missing/orphan/conflicting-root refusal. Whole-database transactional hex-blob
backup already captures this table; runtime table-list validation must require it.
No new backup mechanism is needed.

Original major-plan qualification must include fresh/populated MySQL/MariaDB
upgrade, room revisions0/7/MAX-1 and MAX refusal, zero/nested coins, malformed22
without literal, changed/truncated/missing origin, duplicate UID, wrong hash/source/
result, rollback before commit, commit interruption, journal ACK retirement and
historical proof after room/custody/coin progression. Source review is not a pass
for these cases. Full producer, money/artifact physical execution, raw census,
flat parity, genuine initialized-world and full R1-R8 release remain required.

## Reviewed source pins

- SQL root `36a6ae7e88997735aeeedfbefed1a571192d9aece19dd8333a6fac2f375781c8`.
- Room literal `ee8131630de32aa16a9858803463101954a7259e3f4278c0eb89c1539310b1ac`.
- Coordinator `27784d6b64d1dbada84ec164f99f4fefbd71c52a416b6ba2e8448cbbe970476c`.
- Origin SQL `3a36b4d281cccaa9ff598d94585f75cfd6b19e0251a249078baa08420fe2d1fe`.
- Additive0065 SQL `a84c7adce5f08701216db341b0154ab40ed4eb28df2b8ed35890d0d24751fa4a`.
