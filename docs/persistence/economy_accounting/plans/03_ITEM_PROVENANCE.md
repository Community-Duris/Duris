# Plan 3: item supply, custody, and provenance

## Detached native mobile stage installed — 2026-10-05

[The reviewed loader integration](../NATIVE_MOBILE_STAGE_INTEGRATION_2026-10-05.md)
prepares an actual NPC outside live list/count/index/event publication and
consumes its private retained stage before room/special callbacks. Both legacy
overloads and probes keep their original path. Source/raw-preimage review only;
no native execution or original birth/UID/source/restore/retirement owner is
claimed. The actual reset/spawn issuer and explicit durable native reference
restore remain unfinished. No full NPC state ledger or new gate is added;
major-plan qualification, R1–R8, release and activation remain open/BLOCKED.

## Coherent SHOP source installed — 2026-10-05

[The composed source milestone](../SHOP_COHERENT_SOURCE_INTEGRATION_2026-10-05.md)
installs 32 reviewed production inputs and 22 current-based recipe/Makefile
inputs, including native checkpoint, literal payload, v8 recovery manifests,
original current SQL images and exact retained receipt verification. Independent
source review and raw-preimage/inverse/AST/include checks passed; no native
execution occurred. Current accounted SHOP admission stays closed and canonical
schema0056 is unchanged. Actual gameplay producer invocation, coherent0057,
cold continuation/publication/ACK, flat parity and original major-plan
qualification remain open. Source census/projection rows are unqualified;
release and activation remain BLOCKED.

## Native reference and original-generation observation — 2026-10-05

[The coherent source integration](../NATIVE_REFERENCE_RUNTIME_INTEGRATION_2026-10-05.md)
extracts the existing native/source-event codecs, updates all 63 focused link
recipes and adds zeroable runtime reference storage. The read-only accessor
checks the originally retained runtime generation before pointer access.
Independent review corrected eleven invalid source-helper calls before import.
Source/format/pins/inventory only; native birth/restore ownership, explicit
durable reference records, major-plan execution and full R1–R8 remain open.
Private cold shop value/receipt proof is reviewed but not installed or qualified.

## Reviewed cold SHOP original SQL images — 2026-10-05

[The frozen four-file source slice](../SHOP_COLD_SQL_SOURCE_CHECKPOINT_2026-10-05.md)
reads actual canonical player values and authenticates complete BEFORE/AFTER
player/keeper images inside the original SQL transaction. Review corrected
keeper foreign-copy and extra-context/reference closure gaps; AFTER readback
uses only original locked identities. Private source/format/pins only, without
actual executable installation or qualification. Coherent dependencies, cold
startup/publication/ACK, both backends and original R1–R8 acceptance stay open.

## Plan 5 evidence and complete central inventory — 2026-10-05

[Exact peer evidence and runner registration](../PLAN5_CENSUS_AND_INVENTORY_INTEGRATION_2026-10-05.md)
imports frozen 71-contract pin-repair and 58-case pure custody evidence without
promoting it to current native qualification. Eight existing test classifications
and two existing SQL matrix omissions are repaired: 914 owners, 92 rows, all
87 original rows unchanged. The [publication recipe path fix](../PUBLICATION_RECIPE_PATH_REPAIR_2026-10-05.md)
is separately published as `7a5f9e97d`. Source/JSON/AST/inventory checks only;
major-plan testing, full writer/player/recovery acceptance and R1–R8 stay open.
The private staged NPC loader is frozen for review; the cold shop player reader
continues in parallel. Existing inactive behavior and safety gates remain.

## Native-mobile owner schema preparation — 2026-10-05

The [current shared handoff](../SHOP_RECOVERY_BINDING_SOURCE_2026-10-05.md)
records private 0060's exact three CHECK-range extensions to owner type 12.
Its unchanged-column/row and partial-retry source contract is reviewed; coherent
0057–0060 integration, measured engine metadata and native birth/custody/quest
consumers remain open. No engine execution or Plan 3 qualification is claimed.

## Integrated NPC flat bundle preparation — 2026-10-05

[Canonical flat image read and exact-before preparation](../QUEST_MOBILE_FLAT_SOURCE_2026-10-05.md)
are source-integrated with no caller or standalone commit. Original admitted
parent, protected namespace/Plan5 evidence, recovery and qualification remain open.
No executed backend parity or native lifecycle completion is claimed.

## Integrated NPC native SQL participant — 2026-10-05

[Borrowed native image/stock SQL participation](../QUEST_MOBILE_SQL_SOURCE_2026-10-05.md)
is source-integrated and registered as a dormant definition. Admitted native
birth/lifecycle/custody, schema0059, flat parity and qualification remain open.
No gameplay route, activation or executed proof is added.

## Integrated NPC values and stock capture component — 2026-10-04

The exact reviewed quest_mobile_native C871b2473/H2a149708 is now source-integrated
and registered in the maintained Makefile. It supplies canonical values and pure
complete ordered NPC EQ/INV capture, including the corrected shared size estimate.
This changes no birth, ID allocator, native custody, quest/lifecycle route, activation
or inactive behavior. Current candidate source pins are refreshed; no coverage
completion is inferred. Builds/tests remain deferred to major-plan readiness.
The previous e018 SQL build/restore evidence remains valid for that historical
tree and does not qualify this newly extended native candidate. Actual native SQL/
flat participant, owner/source authority, rebind, producer and guarded ACK remain
required. See [the detailed source checkpoint](../QUEST_MOBILE_VALUES_SOURCE_2026-10-04.md).

## Quest native values/stock source checkpoint — 2026-10-04

[Private canonical mobile values and full ordered stock capture](../QUEST_MOBILE_VALUES_SOURCE_2026-10-04.md)
are implemented and source reviewed. Shared capture overhead is counted once;
existing bounds and actual equipment/carry order remain. Native birth/custody,
SQL/flat participant, lifecycle/rebind, sequential quest/reward, publication/ACK
and major-plan qualification remain unfinished. No production route or Plan3
completion is claimed.

Start from add-double-entry HEAD 49af585c4. The existing item-transfer
accounting owner and references are the starting point, not a claim of complete
item coverage. Work against isolated UID fixtures while the epoch is inactive.
This plan owns ordinary item and world lifecycle routes; coupled priced
transactions belong to [Plan 4](04_COMPOUND_DOMAINS.md). See
[R4](../REMAINING_REQUIREMENTS.md).

## Result

For every admitted durable item UID, an operator can reconstruct why it
entered the economy, each owner/container/equipment transition, and why it
left. A retry or restoration cannot create a second live copy. Realized trade
prices are linked by Plan 4; no appraisal balance is invented.

## Starting files and first checks

Inspect src/item/item_movement_transaction.c,
src/economy/item_transfer_accounting.c,
src/item/item_transfer_repository.c,
src/persistence/economic_sql_item_transfer_transaction.c, and
src/flatfile/flatfile_item_repository.c. Start with
python3 tests/async/test_item_transfer_accounting.py and
python3 tests/async/test_universal_item_transfer_accounting.py.
Run tests/async/test_economic_accounting_item_reference_mysql.py only
against its disposable SQL fixture.

## Work

1. Audit all item allocation, source admission, movement, saved-item load,
   world reset, pet, spell, consume, extraction, and cleanup call paths.
   Separate real issuance/destruction from temporary objects and projections.
   Classify remaining lexical hits by reachable writer, nonwriter, or
   unsupported route; use the shared inventory format in Plan 5.
2. Extend typed source-event identity for quest/world generation, zone resets,
   mob loot generation, spell creation (e.g. wind blade, flame blade, minor creation),
   and item consumption/destruction (potions, scrolls, bandages, broken keys).
   Dedupe a logical issuance even when retried with another command ID. Keep UID
   lifetime and source lineage through copyover, save, reconnect, and restored snapshots.
3. Complete root/child/event references for all eligible ordinary transfers:
   player, room, container, equipment, locker, pet, corpse handoff, and
   same-owner topology changes. Support PC-to-NPC quest turn-ins without refusing
   durable ownership; consume submitted items under `item_transfer_reason::quest_turnin`.
   Support spell-driven drops (e.g. `remove curse` dropping cursed items to room floor).
   The before-state, event order, exact native ownership ledger row, and final graph
   must agree. Preserve historical root/parent data on destroyed children.
4. Integrate saved-item handoff and retirement with source epoch/root,
   payload digest, receipt, acknowledgement, and safe cleanup ordering.
   Missing payload, conflicting ownership, cyclic topology, or uncertain
   restoration retains evidence and refuses materialization. Do not turn
   extract_obj or template allocation into a universal supply writer.
5. Port each accepted route and refusal to flatfile after SQL qualification.
   Keep the existing item authority/journal; do not add a second mutable
   ownership catalog.

## Independent acceptance

- SQL and flatfile fixtures assert one current owner, one ordered legacy event
  per change, one exact accounting item reference per event, source claim for
  first admission or retirement when policy requires it, and exact-ID replay.
- Player journeys cover get/drop/put/equip, same-owner moves, nested containers,
  pet handoff, sourced world/quest/spell creation, spell component consumption,
  destruction, save/reconnect/restart, and UID-preserving restoration.
- Faults cover two simultaneous claimants, stale revision, missing payload,
  duplicate source with a new command ID, cyclic placement, interrupted commit,
  and unknown origin. Confirm no live publication before committed evidence.
- Focused item regressions and both server builds pass. Item-only operations
  may have no coin legs; money-valued pile create/destruction must use Plan 2's
  balanced money route or refuse in an active epoch.

## Boundary and handoff

Plan 4 consumes the item event/reference adapter for commercial, crafting, and
death composites. Plan 5 independently checks provenance and live UID totals.
This plan is complete when its own item routes and refusals pass on both
backends, without waiting for those composite domains to activate.
