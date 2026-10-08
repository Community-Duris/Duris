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

## Creation census successor and current observation boundary

The private76-file candidate preserves ZRO1/0065 and adds RSC2/version2 raw
evidence; the original six-table predecessor remains private reference only.
Its nine tables append `economic_lineage_state(lineage,active_epoch,revision)`,
`economic_epoch(lineage,epoch)` and `player_death_restitution_runtime(item_uid)`
after the origin/full-operation/effect/posting/source-claim/child projections.
All rows and exact NULL/binary values survive. RSC2 binds version2, physical and
legacy-room digests, nine content digests and additional row/cell/byte counters.
Original aggregate and single-cell limits remain; there is no new source budget.

Historical root proof requires globally unique birth UID/revision-one ledger
and literal identities and legacy-operation/event references, across all rows.
Later revisions are retained history. Current money requires the actual lineage
pointer/book and unique original revision-one effect; valid other-book histories
are distinct, while malformed/orphan/wrong-book/current-progressed evidence
refuses. Restitution UIDs participate in physical duplicate detection.
Independent audit/restore ownership remains Plan5. Hashes and synthetic packets
do not prove the original RR session, physical publication or release readiness.

Reviewed pins: raw capture `eb49b8ac`, historical proof `11fc50e3`, creation
correspondence `ce9c5067`, lifecycle owner `c468b7e7`. The size preflight now uses
`GREATEST(0,length...)` so the single-column projection is a valid SQL expression.
Qualification still needs real current/historical book transitions, duplicate
descendants, foreign event references, missing/malformed origins, restitution
conflicts and later pickup/drop coexistence in the original both-engine batch.
No compiler, native, SQL or recovery execution is claimed for this successor.

## Original native terminal-service handoff remains required

The reviewed private84 source candidate adds canonical ZRR1 recovery observations
and a separately typed coordinator owner. It does not change the five schema
files or add terminal-service retention to ZRO1/0065. The original command and
economic TIR48 alone do not establish returned constructor callbacks after native
journal retirement. The actual original terminal service body must transfer
durably before journal retirement and room/item advancement fences are released.

The ongoing private retention slice uses a BODY-only observation contract:
present/original/context/canonical bytes, with missing terminal evidence unknown.
It must never invent envelope phase, revision, process generation or delivery
authority on read. Only the actual private world owner and pinned terminal
transfer may write it after authentic original root/command/receipt proof.
The proposed nullable terminal column and guarded private schema65 successor
are still implementation work, not an installed or qualified storage contract.
Any eventual raw census must retain that complete NULL/binary column and charge
it against the original cumulative and single-cell limits. Plan5 independent
audit, backup/restore and release qualification ownership remain unchanged.

Accepted source pins: factory d3d2f271/5abff8c2/5eb52ca5, ZRR1 c35c145f8/hc9d42736,
coordinator c2766334/h6b74ca59/journal0e4aac52 and room owner a46a5e58/h0957c6e.
The public creation adapter remains gated. This source checkpoint does not prove
physical publication, service success, native qualification or release readiness.

## Reviewed original terminal and full-world interfaces - 2026-10-07

The private92 candidate includes source-reviewed nullable terminal context in
0065, same-session original-body retention, fifth raw origin column and pure
full-command/TIR correlation. NULL remains unknown; byte-identical replay cannot
rewrite original delivery metadata. Storage cap32MiB and unchanged raw cell cap
1MiB are separate enforced boundaries. Matching Plan5 engine/restore readers and
real migration/backup/recovery qualification remain the independent owner's work.

The cold owner consumes only an authenticated original locked root plus actual
terminal service body, preserving removed descendants as history. A complete
owning world census includes rooms/bodies/descriptors/items/global order and signed
cash, with banks retained separately as projections. The actual late boot owner
provides only a synchronous guarded borrow before listeners/callbacks/input and
revokes it before release. The source SQL normalization/independent verifier and
activation consumer are still required; no raw census or terminal DTO is authority.
Production accounting stays inactive; all cross-stream release gates remain open.

## Reviewed same-cut NPC lifetime interface - 2026-10-08

Shared private111/23/5 retains the actual synchronous world/full raw SQL borrow,
original session/slot/save epoch/closed outbox/request/evidence authentication,
world-first residual limits and verifier rollback/release/failure latch. The
borrowed activation view now adds `native_wallets` (const lifetime span) and
`npc_money` (const primary report), beside existing `pc_money` and raw providers.

The actual original mapping verifier authenticates historical native birth inboxes
before current locks, once inside this borrow after raw capture. It moves current
lifetime metadata only after complete validation. Root retains those exact values;
they expire with the raw borrow and grant no authority. Historical birth epoch and
current cash/revision are distinct. Primary NPC comparison reparses raw6 images and
joins complete references/current denominations/UID/mapping/lineage/origin metadata.
Missing/duplicate/mixed/unknown findings and originless NPCs gate acceptance using
complete counts; unloaded lifetimes and retired history remain explicit.

Independent Plan5 must inspect original raw sources and perform its own full
money/item/forest reconciliation; primary reports never replace independent proof.
Native `quest_mobile_native` is one all-row table with five physical columns.
Its reader selects six fields, including canonical-image length; this is not
six providers. Its original
scalar bounds and 4MiB canonical-image allowance remain; cumulative raw totals are
not added twice. Originless opening/full item join, initial inactive installation,
actual independent callback/activation and committed publication remain missing.

The actual mobile P caller now distinguishes authentic absence from unavailable/
unsupported chronology and retains lookup/nest refusal before command success.
Cross-owner custody/full room producer remains open. Six native prepend hooks and
flat collector admission/save-hold/publication/ACK source remain composed. SQL
collector bodies are preserved; dispatch equality is line-ending-normalized only.
All source is unrun. No Plan5 audit/backup/release ownership or coverage gate changes.

## Reviewed warm forest and SHOP staging prerequisites - 2026-10-08

Private115 source candidate c833d941d8f57c580b3d2fc37c1015bef97a08d114cc418a3984e7bc5333aeff
retains the existing full initialized activation view/raw6/lifetime/PC/NPC contracts.
Warm room forests and canonical keeper/custody/proposed player proof helpers are
source-reviewed prerequisites, not source admission or independent audit results.
Typed SHOP backend/atomic original player journal staging is still under review,
not composed or executed. Real producer hold/save-drain and all major-plan/native/
recovery qualification stay open. The root owns positive shared-keeper current
projection and historical wallet/treasury namespace transition; Plan5 must retain
full raw/current cash disagreement findings until that owner is complete. No
prototype/tag inference, missing-row filtering or historical rewrite is allowed.
Plan5 independence, audit/backup/release ownership and original gates remain.

## Reviewed typed flat SHOP proof integration - 2026-10-08

Private119 candidate24e0d474f8f4a110892629be08d2cfcad1dbfb22cca7347a099b7b6043b86264
now composes complete v6/v7/v8 native/accounting one-journal execution plus separate
immutable/current borrowed proofs and the original players-store target participant.
Canonical denial and valid final-revision predicates match original contracts.
Source-only acceptance; original initialized raw/lifetime/primary views unchanged.
Real hold/save-drain/producer/dispatch/publication/ACK/recovery and authenticated
keeper namespace transition remain open. Plan5 independent audit/backup/release
ownership and original major-plan/R1-R8 qualification remain unchanged.

## Reviewed actual reset dispatcher and flat SHOP projection - 2026-10-08

Private `tmp/lifecycle-native-dispatch-shop-projection-candidate-primary-20261008`
integrates seven independently reviewed successor files into the full existing
119-production/23-original-fixture/five-schema candidate. Candidate SHA256:
`55f9bf80794eb274d69d49e7a265627c39f2643abe5e9d557741d0c74c44521b`.
Real reset entry/continue/tail/S/abort observations now prove command order and
last_cmd without moving original decisions; current O/P execution and earlier
held-stage identity are separate. P-before-O uses the real invocation and no
invented O receipt. UUID refusal stays once-only; aborted factories remain held.
The explicit selected-flat SHOP observer validates installed regular PID/bank
mappings and the original racewar context, rechecks the same atomic projection,
and preserves output on refusal. Original SQL observers/installers stay.

Changed-line formatting, whole predecessor preservation and independent source
review pass; compiler/native/gameplay/SQL/persistence/recovery remain UNEXECUTED.
The actual O/P factory/admission/source-CAS/budget/adoption/publication/recovery
route stays incomplete. Genuine flat player hold/drain and keeper checkpoint
stage are now owned independently; actual commit/uncertainty/producer freeze/
publication/ACK remain root-owned and open. Shared keeper cash classification,
historical transition, complete opening/item correspondence, activation, Plan5
and full R1-R8 qualification remain open. Gates/inactive behavior stay unchanged.

## Reviewed complete flat source checkpoint retention - 2026-10-08

Private `tmp/lifecycle-flat-shop-retained-source-candidate-primary-20261008`
integrates the accepted keeper stage, real save-holder, recovery-free current
authority/money and full native player-cut providers, then their independently
reviewed whole-catalog/custody/budget successors. Full candidate SHA256:
`4f73b841dfdf8c149ca9afff403c30c20c15a728accf8d54a891e0f366473c9e`.
It retains 126 production files, 23 unchanged original fixtures, five unchanged
schema/manifest files and 68 selected C providers registered once. Only the new
native checkpoint object was added; removing that line reconstructs the former
Makefile byte-for-byte, with original rules/flags unchanged. Authenticated
documentation-only remote updates are preserved on a new linear review branch
in the same worktree; all three unrelated changes remain unchanged.

The flat holder owns the actual queued and successfully acknowledged STATUS/EQ/
INV body, real revision, pinned selected root/mapping and early original save
exclusion. SQL tokens do not select it. Current authority/money/native readers
borrow the same genuine root lock without recovering or reacquiring during a
source cut. Full current-file PC plus original UID-zero legacy-pet forests match
every active player-owner custody row exactly; modern pets retain their separate
namespace. Real item revisions and owner clock, including zero, stay. Retained
coin payload compares the complete canonical body after topology/position proof.
Fresh full runtime/pet/NORENT correspondence remains the native caller's duty.

The private keeper stage indexes original items once and keeps original custody/
coin rules. Exact complete CURRENT catalog bytes now retain the real header clock,
historical version and every unrelated keeper for same-attempt BEFORE/AFTER proof.
The original shared 32 MiB literal budget charges full retained native preparation
once; no extra budget or larger cap exists. The actual owner must calculate all
retained structures/dynamic bodies before strong stage transfer. Native handoff
requires this real reservation and consumes the original hold marker once.

Independent source review, changed-line formatting and stated full predecessor
preservation checks pass. Compiler/native/gameplay/SQL/persistence/recovery remain
UNEXECUTED under major-plan deferral; this publication records private source
integration, not published code or qualification. Supported client-free flat worker
and replay already use the genuine flat owner; SQL-client flat mode is unsupported.
Original flat producer preparation/retained-attempt ownership is implementing
independently. Native source/runtime/keeper identity, once-only commit/uncertainty/
readback, complete command freeze/submission/publication/ACK and cold recovery
remain open. Plan3 full factory/admission/source-CAS/budget/adoption/pulse/recovery,
shared keeper classification/current-image/historical transition, complete opening/
item correspondence, activation, Plan5 and all R1-R8 qualification remain open.
Original gates, inactive behavior and declined spell path remain unchanged.

## Reviewed original flat attempt and equipped-pet compatibility - 2026-10-08

Private `tmp/lifecycle-flat-shop-original-attempt-candidate-primary-20261008`
integrates independently accepted original preparation/retention, full modern-pet
source cuts and matching complete flat SHOP backend position correction.
Candidate SHA256 `bd4d4a56c71cbabe2ecc8b56880e2046a856905f458cfdfb164ea4409a255ffe`:
128 production files, 23 unchanged original fixtures, five unchanged schema/manifest
inputs and 68 selected C providers registered once; Makefile is unchanged.
Incoming coordinator documentation and all three unrelated local changes remain.

The failure was concrete: native pet equipped roots use slot+1, while original
flat pet custody uses slot zero. The earlier source cut and SHOP backend incorrectly
equated them, refusing genuine equipped legacy and modern pets. Authentic native
forest roles now keep PC/keeper equality and independently validate each pet's
original native positions and zero custody slot. Full bodies, owner namespaces,
UID/tree consumption, revisions/clocks, coin proof and original aggregate PC/legacy
codec byte/object/depth limits stay. Earlier acceptance of that predicate is
superseded by this corrected source; no schema, policy or stored data changes.

The distinct flat preparation owns the actual original queued/acknowledged player
snapshot, early held generation, selection and full source stage. Complete nested
retained allocations, including modern-pet custody/coin buffers and whole catalogs,
are charged to the existing shared 32 MiB limit before stage transfer. Its exact
irreversible marker precedes the native attempt; retries retain the same stage and
first outcome. Attempted cancellation/reissue refuses. Only a private same-stage
proof from the genuine native owner can resolve AFTER or terminal unpublished BEFORE.
Original SQL owner/driver bodies and production/inactive safety gates stay.

Source review also corrects an unnecessary work item: original persistence mode
explicitly rejects flatfile-primary in SQL-client builds. The supported client-free
build already selects the genuine flat owner for both worker and journal replay.
No new SQL-client flat route is required or enabled; SQL ACK cannot prove flat data.

Independent source review, changed-line formatting and full predecessor preservation
pass. Compiler/native/SQL/gameplay/persistence/recovery remain UNEXECUTED under the
user's major-plan cadence; this is private source integration, not published code
or runtime qualification. The genuine once-only native source provider and complete
runtime/keeper identity, uncertainty/readback, command freezing/submission/publication/
ACK and cold recovery remain open. Plan3 factory/admission/source-CAS/budget/adoption/
pulse/recovery, shared keeper classification/current-image/historical transition,
full opening/item correspondence, activation, Plan5 and R1-R8 qualification stay open.
Original Plan1 independent acceptance retains its recorded scope; no full implementation
or release gate is promoted. The declined inactive spell path remains unchanged.

## Reviewed native flat source, frozen submission and prospective cash role - 2026-10-08

Private `tmp/lifecycle-flat-shop-retained-submission-candidate-primary-20261008`
integrates the genuine once-only native provider, full command freezer/submission
and explicit prospective NBC4/NMB4 cash-role capture/codec. Candidate SHA256
`be8942cdd7bebaa240f11ddcbee9317416107595c536866f5125a5008e79992c`: 134 production files, 23 unchanged original fixtures,
five unchanged schema/manifest inputs and 72 selected C providers registered once.
Four new object lines preserve all original Makefile rules and flags. Actual counts
are derived from selected files; the parent's inherited126 metadata was stale
(its actual source count128 was already recorded correctly). Frozen parent unchanged.

Independent source review found and corrected four concrete defects: older player
domain journals must drain before each CURRENT cut; active foreign UID/root/parent
cache links must be censused; genuine keeper/destination transforms must exist in
the supported client-free profile; newly retained decoded manifest capacities must
be charged before submission and preserved across exact retries. The provider uses original root locks, complete
player/pet/keeper literals, mapping/money/custody and whole-catalog BEFORE/AFTER;
one sealed attempt and immutable outcome/readback preserve retries. Pure helpers'
exact original bodies now serve both builds without exposing SQL observers.

The genuine READY flat source freezes one full v8 command. Native-stage, command
and decoded manifest/encoded journal retention charge the unchanged shared32MiB budget. Exact
submission retries retain command bytes, callbacks, original ACK/body and early
execution hold; synchronous refusal cannot release an already attempted source.
No coordinator call occurs under pipeline/leaf locks. Original production/admission,
publication/ACK, inactive and declined spell gates remain unchanged and closed.

The actual configured keeper selector conflates absence and ambiguity. NBC4
distinguishes exact zero/one matches and refuses ambiguity; NMB4 binds that role to
the complete unchanged NBC3/image/stock evidence and exact full intent/key envelope.
Shared keeper uses its actual shop+1 key, including shop0, without an invented CAS
revision or wallet. Historical NBC1/2/3, commands1-3 and nonzero-wallet MBR1 stay.
Values are not factory, source admission, mapping or publication authority.

Changed-line formatting, predecessor/dependency preservation and independent source
review pass. Compiler/native/gameplay/SQL/persistence/recovery remain UNEXECUTED
under user major-plan deferral. This checkpoint publishes documentation only;
private source is unqualified. Full flat driver/publication/ACK/terminal disposition/
cold recovery, role-aware accounting/results/atomic native-image+SHOP participants,
birth factory/admission and room O/P integration, complete opening correspondence,
shared keeper historical transition, activation, Plan5 and R1-R8 qualification
remain. Original Plan1 independent acceptance retains its scope; no full gate closes.

## Reviewed flat CURRENT readers, cache publication and live guarded ACK - 2026-10-08

Private `tmp/lifecycle-flat-shop-publication-candidate-primary-20261008` integrates
the accepted full CURRENT/never-admitted BEFORE readers, flat runtime-cache
publisher and corrected original live held-body/guarded publication-ACK owner.
Candidate SHA256 `06f8c714cad027bd34aae137411e14b9dcf15f1a2460b8030504dc2994e75b3e`: 136 production files,
23 unchanged original fixtures, five unchanged schema/manifest inputs and 73
selected C providers registered once. The previous candidate's Makefile is exact;
its four added object lines and all original flags/rules remain. Incoming remote
coordinator documentation and all three unrelated local changes are preserved.

The borrowed-root BEFORE reader requires full original player-domain recovery,
authentic accounting/native receipt absence, exact identity/mapping/money/save
ACK/level/racewar/native-custody and complete v8 BEFORE bindings. Missing required
accounting indexes refuse; the original optional native trade catalog can be
absent and yield ENOENT. Absence grants no cancellation or attempted-hold release.
Modern pets retain their own namespace and actual zero-valid clock, including
empty forests. Full durable catalog and cache UID/root/parent/owner-wide census
precede hydration. Only exact original selected BEFORE witnesses authorize cache
transitions; target/stock/other item fields must match CURRENT. Existing 8192-UID
observer bounds are retained via sorted chunks. A false publication result can
follow successful item-batch hydration; hold and retry obligations must survive.

The private flat held-body accessor derives the actual original command and
queued/ACKed full snapshot from the genuine publication owner. Independent review
found and corrected nested-ticket acquisition: the callback now borrows that
actual owner/reservation, without recapturing AFTER or reacquiring its root lock.
The live wrapper uses the genuine flat covered-snapshot observer, ordinary journal
census and existing coordinator's exact command/receipt guarded ACK, or complete
retained refusal before synchronous native cleanup. Coordinator entry occurs after
pipeline/leaf locks. Another correction preserves the original exact local cleanup
after durable ACK even if a different PID later poisons global integrity. Only
that private acknowledged owner can consume the exact flat slot/command/epoch/
generation/hold and issue the original deferred-save/replay wakeup.

Final corrected slices pass independent source review, changed-line formatting,
complete predecessor and dependency checks. Compiler/native/SQL/gameplay/
persistence/recovery remain UNEXECUTED under user major-plan deferral. This is a
documentation-only milestone; private implementation is unqualified. The genuine
physical callback/retained effects/terminal native disposition/cold registration
and recovery/producer driver/admission remain unfinished. Role-aware NBC4
accounting/MBR4 result source is separately pending independent review; genuine
birth-to-SHOP CAS/atomic native-image+SHOP ownership, source factory/room O/P,
opening correspondence, shared keeper historical transition, activation, Plan5
and full R1-R8 acceptance remain. Original inactive/declined spell/production
safety gates stay; original Plan1 independent acceptance retains its scope.

## Reviewed prospective birth accounting/result and live publication budget - 2026-10-08

Private `tmp/lifecycle-flat-shop-birth-accounting-candidate-primary-20261008`
integrates the accepted NBC4/NMB4 role-aware accounting/MBR4 result and exact
live publication-retention budget with the previously reviewed CURRENT/cache/
held-body/guarded-ACK source. Candidate SHA256 `a41d13a7a78b0eb7e4ff8e34281d8050fbad2c112433ddaa24a7ac404d0527b4`: 140 production files,
23 unchanged original fixtures, five unchanged schema/manifest inputs and 75
selected C providers registered once. Two new object lines retain every previous
Makefile byte; six additions total preserve original flags/rules and no original
codec, policy, schema, helper, stored history or safety gate changes.

Ordinary prospective births preserve original NMB3 wallet effects, including known
zero revision1, then rebind full verified NMB4 metadata. Shared keeper stock uses
actual SHOP shop+1 custody (slot0 included) and original equipment-zero convention;
the complete original native image keeps genuine equipment. Shared births create
no wallet, treasury or cash postings. Distinct supplied participant values bind
actual shop ID, explicit shop/owner presence and BEFORE/AFTER clocks plus all four
born cash denominations. Pure structural correlation does not invent owner1 for
empty stock, a CAS increment, mapping creation or current storage authority.
Distinct canonical264-byte MBR4 binds whole image/NMB4/NBC4/plan and supplied shared
values. Old MBR1 and its nonzero-wallet requirement remain exact and reject MBR4.
Genuine birth-to-SHOP CAS policy, born-UID absence, existing custody preservation,
ordinary mapping creation and single atomic storage participant remain unfinished.

The original live flat publisher can separately reserve its actual retained
forest/vector/item/string/property/nested capacities once under unchanged32MiB,
before first nonthrowing moves. It borrows the genuine outer publication owner;
exact token/op/root/epoch/generation/submitted bytes and original native/command/
manifest charges must match. All future shared capacity censuses include this
charge; exact retries cannot resize/reset/reclaim it. No nested ticket/root lookup
or native/publication/cancellation/ACK authority follows from a byte count. The
physical owner still must supply the complete real allocation census and preserve
those exact allocations and once-only handler stages.

Both slices pass independent source review, changed-line formatting, full
predecessor/new-path and dependency authentication. Compiler/native/SQL/gameplay/
persistence/recovery remain UNEXECUTED under user major-plan deferral. This is a
documentation-only milestone; source is private and unqualified. Live physical
publication/terminal native disposition, cold registration/recovery and full
driver/admission remain; full birth/source factory/room O/P, opening correspondence,
shared keeper historical transition, activation, Plan5 and original R1-R8 gates
remain. Original inactive-accounting and declined spell behavior is preserved;
no full qualification gate is closed.

## Immutable cold movement and Plan5 source joined - 2026-10-08

[Source integration handoff](COLD_WORKING_PLAN5_SOURCE_INTEGRATION_2026-10-08.md)
records private candidate `7108537ee60f94cfefdfef5dca8a627c0ef3a73844ac3c228301c0cf952389d0`:150 production files,23 unchanged original
fixtures,five unchanged schema inputs and11 additional audit/restore paths.
All81 selected C providers remain registered once;12 additional native source
slices authenticate1643 dependency records. The Plan5 source closure has44
Python dependencies;108 existing central rows are preserved,with three new
offline rows and29 added required methods. Composition2d9642 changes no
maintained implementation; compiler/native/SQL/gameplay/recovery remain unrun.

Cold movement now retains genuine reachable immutable PC/keeper/target forests
before its full charge. Actual native pointers/R_num/NORENT ordering and full
BEFORE/AFTER bindings are preserved. Scalar phase advances immediately after a
normally returned successful original effect,BEFORE post-effect recensus; a
refusing recensus retains that next expectation. Original full caller,registration,
reload/effects/money/custody/final ACK and shared keeper atomic writes remain open.
Independent Plan5 previous-owner/position/provenance/zero-net/action fixes and
exact backup/tombstone object/version guards are source-joined. Peer27-overlay
tests do not qualify this smaller candidate. Major-plan execution remains deferred;
inactive accounting,declined spell path,32MiB budget and activation gates stay.
Original Plan1 acceptance keeps its recorded scope;fullPlans2-4,combinedPlan5,
R1-R8 and release completion are unproven. The goal remains active.

## Flat cold enrollment and exact retained capacity - 2026-10-08

Private `tmp/lifecycle-shared-shop-reader-cold-holder-candidate-primary-20261008`
SHA256 `a226828cfc3773cc4e72ad2900dc0a33801778a0d1ee1acf7be47a3129ee3cc0` now composes11 additional source-accepted slices on the
previous147 candidate:150 production files,23 unchanged fixtures,five unchanged
schema/manifest inputs,81 selected C providers registered once and1480 new
authenticated dependency records. Source composition adca26 preserves the
three unrelated local changes. No compiler/native/SQL/gameplay/persistence/
recovery tests ran under the user's major-plan deferral.

New source slices seal the real flat catalog after optional boot assignments,
observe authentic SHOP BEFORE state in the borrowed original SQL session,
prepare the complete unpublished original selected literal graph, measure its
actual pool/text/description/affect allocations including binary spellbooks,
authenticate the genuine restored outer/current/world/cash cut before once-only
enrollment, and count all current retained holder capacities without duplicating
embedded structures. Original32MiB charge, SQL behavior and safety gates remain.

The boot seal precedes recovery/gameplay workers; the earlier logging worker
already exists. The shared SHOP reader supplies BEFORE evidence only: native-ID
locks, whole-budget reservation, atomic SHOP/native writes, full264-byte MBR4,
result storage and recovery remain root-owned and unfinished. No SHOP wallet or
native-mobile custody owner is fabricated. Cold native enrollment remains
unwired until the full caller allocates and charges every future transition
variant before effects. Current-holder census success is not that full charge.

Root next joins preallocated immutable working forests with original finite
effect/reload/money/custody publication and guarded ACK, then genuine registration
and replay. UID-zero legacy pet absence remains unknown for an absent player.
Independent Plan5 audit/backup fixes are being reviewed for narrow integration;
their peer qualification does not cover this private native candidate. Original
Plan1 acceptance stays in its recorded scope; Plans2-4,combinedPlan5,R1-R8 and
release remain incomplete. This is a documentation-only source checkpoint;
private implementation is unexecuted and unqualified. Inactive accounting,
declined spell-path decision and activation gates stay unchanged.

## Reviewed ordinary world and flat cold census - 2026-10-08

Private `tmp/lifecycle-ordinary-world-cold-census-candidate-primary-20261008`
SHA256 `4c9182212e2aaf750afa070fb843d415a06c77e6773aae89a7882de9ce04d000` integrates five additional independently source-accepted
slices on the previous147 candidate: ordinary published-world owner7bfa,
actual SQL reader406, corrected cash observer985, corrected full original
forest reader0cfe and cold world wrapper690e. There are148 production files,
23 unchanged original fixtures, five unchanged schema/manifest inputs and80
selected C providers registered once. Makefile stays byte-exact with its six
original additions. All592 new actual dependency records authenticate.
Composition b37964/066185 preserves both new docs-only remote advances and
the three unrelated local changes. No merge, rebase or force push is used.

Ordinary published-world selection uses the full genuine original NMB4/NBC4/
264-byte MBR4 terminal attachment and immutable origin. Its actual SQL reader
selects matching recovery, retained result and origin locks before/after the
existing mapping/native/custody cuts. Historical policy stays explicit; shared
and corrupt/ambiguous evidence do not downgrade to a historical constructor.
The native world owner stays SQL-free under its original idle-session token.

The cold physical helper now verifies authentic BEFORE or CURRENT keeper cash
from the caller's real retained cash stage, rather than requiring CURRENT before
effects. Original configured-body, bounded catalog, PC/pet and NORENT predicates
remain. The flat forest counterpart reconstructs complete original ordered
BEFORE bytes from actual CURRENT parent UIDs and original selected literals.
Independent review caught a type mismatch and an inherited target-UID0 lookup;
corrected0cfe uses the validated account string and reverses target weight only
for an actual destination, in BOTH new flat and original SQL cold readers.
Rejected7e5e/a46 and superseded5d remain preserved as rejected ancestors.

The new SQL-free world wrapper authenticates canonical retained payload/PID and
the complete manifest/selected UID union, then invokes the original bounded
world/descriptor/UID/selected/target observer. Strict absent-body requests empty
only their request spans; full authoritative forests remain retained. Strict
proof requires actual presence/absence, all locations and present-owner forest
matches. Relaxed observation retains genuine BEFORE/detached mismatch flags.
Flat item IDs remain -1; no SQL row, runtime identity or returned handler is
fabricated. Root must still join full command/receipt/source/pet/money/target-
phase proof in the genuine native callback; helper success cannot publish/ACK.

The existing approved reset/restore behavior resolves shared keeper gameplay:
cold restoration replaces the reset keeper with saved cash/stock; warm reset
does not overlay the catalog or union live inventories. No new historical keeper
row-ID lifetime requirement is introduced. A parallel owner implements a two-
phase borrowed-session SHOP reader: keeper/SHOP owner before native-ID locks,
then full existing custody/physical/sidecars after the root's original ascending
native locks. No native-mobile custody owner, wallet, AFTER clock or CAS is
invented. Atomic birth storage/result/recovery and publication remain unfinished.

Complete cold effects/enrollment/budget/guarded ACK, boot wiring, admission,
opening correspondence, shared keeper atomic storage, activation and combined
Plan5 qualification remain open. UID-zero legacy pet absence remains unresolved
when the genuine player is absent; VNUM/order cannot supply an identity proof.
Compiler/native/SQL/gameplay/persistence/recovery tests remain UNEXECUTED under
the user's major-plan deferral. This milestone publishes documentation only;
implementation stays private/unqualified. No Plan2-5/R1-R8/release completion is
claimed, and inactive-accounting/declined-spell/activation safety gates stay intact.

## Integrated ordinary birth owners and restored flat outer - 2026-10-08

Private `tmp/lifecycle-ordinary-origin-restored-outer-candidate-primary-20261008`
SHA256 `48570aecc6c006fe4bc030930e90b01e6a1c38a9fc358bd7f6fc7e0f5078727e` now integrates nine independently source-accepted slices
and 13 unique source overrides. There are 147 production files, 23 unchanged
original fixtures, five unchanged schema/manifest inputs and 79 selected C
providers registered once. Makefile remains unchanged with its six original
additions. All 809 dependency records and 22 authenticated predecessor-to-successor
bindings passed the actual composition d77e0c. Three unrelated local changes and
both documentation-only remote advances are preserved.

Ordinary birth now retains full original NMB4/NBC4/264-byte MBR4 across recovery,
current wallet lifetime reads, immutable origin storage and the genuine producer.
Fresh non-alchemists record the actual original decision return before capturing
the not-attempted capsule; historical constructors are not converted. Genuine
configured-role recensus precedes fresh submit, cold reconstruction and all three
SQL publication cuts. Both original metadata installers validate the full typed
receipt against the real wallet mapping. Historical policies remain explicit;
shared/ambiguous keeper roles still refuse. No UID is substituted for a wallet.

The private authority method preserves its existing friend set and installed
metadata. Ordinary coordinator dispatch selects the real reviewed recovery family
while preserving original callback readiness and execution/extension prerequisites.
Generic command-only native submission refuses. Physical ACK authenticates the
exact current receipt; continuation replay retains original fences until confirmed
origin transfer AND journal retirement. No simple retirement bypass exists.

The restored flat outer authenticates its original passive slot, command, root,
PID, epoch and reservation. The actual coordinator delivery function is
`critical_command_coordinator_pulse`; its live publication state, exact receipt,
frozen command and retained fence heads are checked before effects and before ACK.
Original covered-save observation precedes the native root lock. One immutable
complete cold-stage allocation charge joins the original 32 MiB census; exact
local hold consumption follows durable ACK even under unrelated-PID poison.
Never-admitted cleanup remains closed until its genuine native owner is complete.

Independent reviews accepted recovery704, lifetimes7d4/origin ef9, authority510,
passive2ec, current receipt3174, outer8b116, producerde976 and coordinatorb0b.
The next published-world successor is frozen separately for source review; its
real SQL participant still needs ordinary original-command/recovery/retained-proof
and origin-lock selection. Full cold PC/keeper/pet presence-or-absence source proof,
native effects, original boot wiring, admission/driver, shared keeper atomic CAS,
opening correspondence, activation and combined Plan5 qualification remain open.
Unidentified legacy-pet absence is not fabricated from VNUM or a null player.

No compiler/native/SQL/gameplay/persistence/recovery execution was performed for
this milestone under the user's major-plan deferral. It publishes documentation
only; implementation remains private and unqualified. Plan1's original independent
acceptance is not expanded into Plans2-5, R1-R8 or release completion. Accounting,
inactive spell behavior and all activation/safety gates remain unchanged.

## Reviewed ordinary birth storage and flat cold helpers - 2026-10-08

Private `tmp/lifecycle-ordinary-birth-cold-helpers-candidate-primary-20261008`
SHA256 `d9eaa45b9511152cae7cda2a91a003fc2ecd045be52c703b2901300749d69332` integrates three independently
source-accepted slices: ordinary NMB4/MBR4 SQL storage, its original repository
dispatch, and genuine flat boot-template/procedure-binding/reload helpers.
There are 143 production files, 23 unchanged original fixtures, five unchanged
schema/manifest inputs and 77 selected C providers registered once. Makefile
remains byte-exact to the previous candidate with its six original additions.
Three unrelated worktree changes and the docs-only remote advance are preserved.

Ordinary storage reuses the actual inbox/session, inserted wallet mapping,
absence/source/custody/accounting evidence and one original root transaction.
Known-zero wallets remain revision1. Both original-ID receipt paths authenticate
the full original NMB4 and actual264-byte MBR4; inbox/outbox/completion use exact
typed bytes and outbox version4. Historical NMB1-3/MBR1 paths remain preserved.
Historical receipt proof is separate from current native publication proof.
Shared keeper roles still refuse; no wallet or owner clock is fabricated.
Independent backend f17755/e7563b and repository e74aa9/826bab reviews pass.

The four-file flat helper slice authenticates the original sealed boot catalog,
retains a backend-bound procedure stage and preserves original reload/proclib
effect/uncertainty states. SQL stages refuse foreign prepared-flat holders.
Independent f7ae9d/a1fe02 review passes. Actual boot wiring, restored-flat slot,
allocation budget, complete present-or-absent PC/keeper/pet census, cold effects,
terminal cleanup and guarded ACK remain root-owned and incomplete.

Two independent owners continue prospective ordinary recovery/terminal codecs
and historical origin/current lifetime readers. They must read the genuine full
original published attachment; plan/current reconstruction cannot replace it.
Source factory/admission, ordinary physical publication, shared atomic keeper
CAS/storage, opening correspondence, activation, Plan5 and all original R1-R8
qualification remain open. No inactive-accounting/declined-spell/safety gate is
changed. Compiler/native/SQL/gameplay/persistence/recovery are UNEXECUTED under
the user's major-plan deferral. This checkpoint publishes documentation only;
the integrated implementation is private and not qualified for merge.

## Reviewed live flat shop physical publication - 2026-10-08

Private `tmp/lifecycle-flat-shop-live-reviewed-candidate-primary-20261008`
SHA256 `87c253f48e89a1b3656c787c7a630331da35a3c43fdd549bc099b6e1a0d7265b` integrates the corrected live
native publisher and original borrowed-lock PC/keeper/pet/NORENT observer.
The complete candidate retains 140 production files, 23 unchanged original
fixtures, five unchanged schema/manifest inputs and 75 selected C providers.
Makefile is unchanged from the previous candidate; its original six additions,
flags and rules remain. All three unrelated local changes are preserved.

The actual outer held-save owner and READY native source bind the original v8
command. Genuine receipt/CURRENT, full physical literal/UID/target/keeper/pet
census and current custody precede placement. Actual retained vector/string/
property capacities are charged once under the original shared32MiB before moves;
exact retries retain the same allocations and original returned-handler stages.
Fresh CURRENT/money/census checks precede the existing guarded ACK. Original
account-bank publication fences prevent supported later operations advancing
that bank before ACK; no historical money or SQL item IDs are fabricated.

Review found that changing target fields in a copied v8 payload contradicted its
manifest and refused a returned carrying-to-nesting stage. Corrective8de9aeab
derives that temporary carried forest from authenticated original AFTER literals
and retained BEFORE PC values. The frozen command/manifest stays unchanged.
Independent73c06f/cd19e7/e91b13 source reviews, formatting, full predecessor
reconstruction and 50 live dependencies pass. Compiler/native/gameplay/SQL/
persistence/recovery remain UNEXECUTED under user major-plan deferral. This is
a documentation-only checkpoint; the reviewed implementation remains private.

Next cold work requires a distinct passive restored-flat slot, authentic outer
publication owner/capacity/consume path, full absent-owner/pet source census and
genuine flat boot-template/procedure-binding/reload counterparts. Attempted
native holds remain on never-admitted outcomes until actual terminal cleanup is
proved. Complete cold/driver/admission, birth CAS/atomic storage/source factory,
opening correspondence, keeper historical transition, activation, Plan5 and
original R1-R8 qualification remain open. No inactive/declined-spell/safety gate
changes or full release acceptance follow from this source review.

The Plan3 source trace distinguishes absent, present-zero and advanced owner
clocks; empty stock supplies no fabricated clock. Cold boot resets before SHOP
restoration replaces the keeper with saved cash/stock, while warm reset does not
overlay the durable catalog. No generic live inventory union is introduced.
Ordinary NMB4 backend work proceeds separately; shared atomic participation is
unfinished. Immutable0059 retains five physical native-table columns and six
selected reader fields, as corrected in published78d71393e; no schema changes.
