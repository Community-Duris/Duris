# Required room-floor reset producer: implementation handoff - 2026-10-07

## Reviewed runtime return and retained real-root construction - 2026-10-07

Private `tmp/lifecycle-reset-runtime-return-root-candidate-primary-20261007`
composes99 production files, 23 unchanged original fixtures and five private
schema/manifest files on maintainedaf425e28e. Candidate SHA-256 is
`a91ff6cf48d3920150fcc08893afded3803874d6b3b33f6da9247f661f59ef5a`.
All53 selected production C providers are registered once with existing rules
and flags. Reviewed slices and changed-line formatting/token checks pass;
compiler, native, SQL, migrations, gameplay, fault and recovery remain unrun.
The source implementation remains private, not imported into production Git.

The original adopted COMMIT-attempt restriction and fresh known-abort SQL/full
wallet-bank projection checks remain. Reverse writer ownership now validates the
returned genuine maintenance guard, same idle SQL session/named locks, empty
original writer and strict actual reservation readiness, then nonthrowingly moves
the held local gate/writer metadata back. Boot lock/session/authority are preserved.

Coordinator return atomically restores the original genuine lifecycle reservation
while admission stays closed. Its original accepting policy is retained internally;
generic lease release/finish cannot bypass runtime-origin return. Actual late-cut
acquisition mints only a restrictive initialized-owner marker. Explicit quiesce
suppresses reopening before promotion, during adoption and after reverse transfer;
ordinary and early-recovery reservation behavior is preserved. Exact owner cleanup
checks readiness before releasing the reservation, without an unowned readback gap.

Initialized-world cleanup now retries the real writer-release obligation even
after its confirmation was invalidated by failed SQL cleanup, and skips completed
writer/reservation stages. The capability is revoked exactly once; cleanup_pending
can retry without reissuing or borrowing it, and completed cleanup is idempotent.
Original outbox/save resumption follows completed exclusion cleanup. Actual adopted
activation/abort caller and private pre-promotion revocation are still unwired;
these helpers neither select an epoch nor grant projection authority.

Real O-root preparation captures the authentic invocation/slot/room, actual
allocator UID, detached factory, literal/recipe/binding and original load decision.
Artifacts/corpses stay with their separate owner before the load roll. Review found
that an ignored inner factory cleanup failure could lose its native owner. A narrow
private retaining variant now transfers surviving failed factory state into the
caller-owned handle; held_refusal persists until genuine cleanup. The original
prepare wrapper shares the exact original constructor body with unchanged policy.
No root is sealed/admissible and actual O/P dispatch remains unchanged.

The accepted cold reset/terminal SQL/raw proof, source union, owning pre-listener
census and real pure auction forest provider remain. All original budgets and
32MiB terminal storage/1MiB raw-cell bounds stay. Whole-reset P target chronology,
cross-owner construction order, genuine room nesting/reducing-shell and hot placement,
current season/room CAS, complete terminal/publication, progressed money/artifact
recovery, flat parity, measured schema fingerprints, full normalization/independent
verifier consumer and original major-plan/R1-R8 qualification remain open. A separate
room-nesting successor is under construction. Inactive behavior, active O skip and
all safety gates remain; no release gate or inventory coverage is promoted.

## Reviewed creation census and current coin heads - 2026-10-07

Private `tmp/lifecycle-reset-creation-candidate-primary-20261007` composes
76 production files, 23 unchanged original fixtures and five schema/manifest
files on maintained6d2bd242d. Independent source review and changed-line
formatting passed; no compiler, native, SQL, migration or gameplay runs occurred.

RSC2/version2 retains nine all-row projections: reset origins, full operations,
effects, postings, source claims, children, actual lineage pointers, epoch
existence and restitution UIDs. They share the original RR cut and aggregate
limits with the physical, five-table room and native-mobile captures. A pure
adapter subtracts only added evidence before the native scan and verifies the
combined totals against the original limits afterward.

Historical proof authenticates complete original receipts and globally unique
birth UID/revision and event references. Current creation graphs independently
check the actual book/head, exact literals, all descendants and competing
physical projections; later writer5 revisions remain history under this provider.
The lifecycle owner preserves original findings, accepts only exact creation
matches and rejects uncapped aggregate defects. Review fixed cross-operation
birth ambiguity and the one-column SQL size preflight.

The original recipe successor is separate and unexecuted. Actual O producer,
constructor/source/UID ownership and publication/ACK, whole-forest money/artifact
publication, progressed/opening recovery, runtime0065 registration/fingerprints,
flatfile parity, initialized-world census and full R1-R8 qualification remain
required. Current inactive behavior, public admission and active O skip remain.
Coverage is still incomplete and release is blocked; this closes no release gate.



## Reviewed ordinary reset transaction and original carrier - 2026-10-07

Private `tmp/lifecycle-reset-transaction-candidate-primary-20261007` composes
68 production files, 23 unchanged original fixtures and five schema/manifest
files. Corrected SQL root36a6ae7, literal ee813163, coordinator27784d6 and full
original carrier3a36b4 have independent source acceptance. Formatting passed;
compiler, native, SQL, migration and recovery execution remain unperformed.

Ordinary actorless creation now has same-root custody/literal/source/indexed
evidence and exact typed48. Review corrected inbox revision1 to the actual room
revision plus one, malformed command22 legacy fallback, non-NULL ordinary coin
payload and missing current physical duplicate checks. Both original-ID replay
paths require the matching full original command and immutable origin. ZRO1
retains accepted time, source, season, forest and original recipes; additive0065
and three private manifest successors are required after journal retirement.

[Plan5 reset-origin interface](ZONE_RESET_ORIGIN_PLAN5_INTERFACE_2026-10-07.md)
records storage, audit, restore and qualification ownership. Runtime fingerprints
still require actual MySQL/MariaDB measurements. Complete money/artifact owners,
creation-aware raw census, real producer/publication/ACK, flat parity and original
major-plan qualification remain required. Current public admission and the active
O skip remain unchanged. This source composition closes no full R1-R8 gate.


The original day-one requirement includes zone-reset spawns with zero refusals.
Current active accounting skips the original `O` room-floor object command before
construction (`src/world/db.c:6919-6977`). Its original body at 7316-7385 retains
prototype limits, force-repop, incumbent/non-take checks, artifacts, load checks,
RNG decisions and room publication. Those gameplay decisions must remain.

This is required producer work, independent of the cache correspondence reader.
Root owns its shared admission, supply-policy and durable room contracts. A
bounded producer owner can own `zone_reset_item_owner.{h,c}` and the original
`O` branch once those narrow contracts are ready. Existing private staged object
construction in `world/db.h` can be lent to that owner through a narrow friend;
the original constructor and once-only publication steps must remain intact.

The existing player creation API is insufficient: `queue_creation_grant()` in
`item/item_movement_transaction.c:1582` rejects absent/NPC players, and
`sourced_item_creation()` in `economy/item_transfer_accounting.c:150` requires
a positive player PID and excludes coin piles. A fabricated player cannot supply
reset authority. `item_lifecycle_source()` at 278 binds lineage/UID or a logical
integer; it does not freeze the real reset invocation and command slot.

The existing native-mobile reset path establishes actual invocation, operation,
slot, allocator identity and zone/room VNUMs at
`world/quest_mobile_native_birth.c:664-680`. Reuse that existing reset-generation
boundary for an actorless admitted world-generation operation. Freeze actual
decisions and output UIDs before mutation; commit supply/custody, source claim,
receipt and room literal in the same root, then use retained original publication.

Modern room persistence currently handles admitted `player_drop` at
`item/item_transfer_repository.c:2433,2793,2831`. The new creation requires exact
same-root room literal retention and cold recovery through the existing
`sql_room_item_payload` owner. Prototype coin objects additionally require their
actual denomination issuance and UID-held pile in that root. Artifact and hook
effects require their existing original owners; removing the refusal alone,
changing RNG order or reporting skipped required objects is not completion.

Extend original controls in
`tests/async/native_birth_accounting/alchemist_actual_reset.cpp` and
`run_actual_reset.py`, plus the original room SQL recovery journey. Their current
inactive and modeled/component evidence does not qualify active reset issuance.
Active SQL/flat parity, actual constructor/publication, retries and cold restart
remain original major-plan gates. No producer source or gate is changed by this
handoff; current inactive accounting and the declined spell-path decision remain.

Fresh opening still needs a separate witnessed lifetime for originless NPC cash.
The existing persisted holdings reader and later typed-birth wallet locker do
not adopt those NPCs. Runtime IDs or VNUMs cannot substitute for durable lifetimes;
unknown historical origin must remain explicit in reconciliation.

## Shared reset contracts implemented privately - 2026-10-07

Private `tmp/lifecycle-reset-contract-candidate-primary-20261007` now composes
61 production and23 original fixture files. All earlier 51/23 bodies remain
exact. Six shared contracts, two command-codec files and two accounting-compiler
files have independent source acceptance; no native source is imported or run.

The appended command type22/writer16 freezes the actual invocation/slot,
operation, zone/room, season/revision, complete literal forest, original factory
recipes and exact coin denominations. Private producer-only source capture shares
the existing lazy reset invocation with `M`, including `O` before the first `M`.
Private authority preparation borrows the installed SQL lineage/epoch. Generic
player creation, legacy execution and current admission remain unchanged.

The pure compiler creates room custody at revision1 and actual UID-held pile
issuance with balanced denomination legs. Known zero retains its ordinary pile
effect without an unused virtual account. Source review found and corrected
posting-index gaps when zero piles precede or separate positive piles; the
corrected compiler is `5b397f56`. Canonical command codec is `728a9ce0`.
Formatting passes; original shared-file newline conventions are retained.

The complete producer is still missing: original decisions/construction,
authenticated SQL/flat root and source claim, room/coin/artifact literal retention,
current and historical cold readers, guarded publication/replay/ACK and original
qualification. Room coin cold proof currently authenticates only wallet-transfer
roots; room item cold proof authenticates player drops and excludes artifacts.
Those need explicit creation branches under the new root, preserving old routes.
Post-ACK review now requires the additive0065 original-command carrier; source policies and CLI remain unchanged. Planned
component controls include zero-before-positive, zero-between-positive, all-zero
and positive outputs; they remain unexecuted with the major-plan qualification.
