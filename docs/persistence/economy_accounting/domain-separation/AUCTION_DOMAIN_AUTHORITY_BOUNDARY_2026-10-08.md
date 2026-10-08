# Auction domain authority boundary — 2026-10-08

## Disposition and frozen inputs

Auction already has useful domain separation: pure listing, bid, settlement,
money-claim and item-claim accounting adapters; borrowed-transaction SQL
participants; a separate native preparation/publication owner; and retained
original-command recovery context. Do not extract these plans again or add a
facade that turns their value structs into apparent authority. The next useful
owned slice is focused executable acceptance of the existing native world
observer. Complete SQL/native player publication, flat execution and cold
recovery qualification remain with their original owners.

This document is the only repository change. It implements no auction behavior,
changes no public API/schema/wire format, and enables no route. The continuing
native Goal remains **BLOCKED and unfinished**. The separate quest temporal-pair
reader/assertion work, shared owners, registry, drivers and Plan5 are outside this
reservation.

Source statements below use maintained primary
`52b3dd103c383424bebd9092c431241323c9b439`, with exact tree identities:

| Input | Git tree |
| --- | --- |
| src | `833d3085815b396861ad18a77635412212381e4b` |
| tests/async | `790f367adf805a69d53aac6460938f5c921f9136` |
| migrations | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |

Completed owned context is frozen at
`f70d1147d9d23e5325399c1508ab29a904f3cb06`. Reuse the
[operation inventory](OPERATION_INVENTORY.md),
[post-R14 dependencies](POST_R14_OWNER_DEPENDENCIES_2026-10-07.md),
[currency boundary](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md),
[currency acceptance blueprint](CURRENCY_NATIVE_ACCEPTANCE_BLUEPRINT_2026-10-08.md),
[item custody boundary](ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md), and
[ordinary pickup component handoff](ORDINARY_PICKUP_COMPONENT_HANDOFF_2026-10-08.md).
R0–R14 stay closed at their declared component scopes; auction does not reopen
them. Historical ACTIVE wording in older handoffs is not this Goal's current
status. Inventory auction rows identify dependencies; the ownership and effect
cuts below supply the additional auction design.

The maintained [experimental checkpoint](https://github.com/Community-Duris/Duris/blob/52b3dd103c383424bebd9092c431241323c9b439/docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md)
reports private flat SHOP source integration with candidate SHA256
`4f73b841dfdf8c149ca9afff403c30c20c15a728accf8d54a891e0f366473c9e`:
126 production files, 23 original fixtures, five schema/manifest files and 68 C
providers. Its source review/format/preservation checks pass, while compiler,
native, gameplay, SQL, persistence and recovery are explicitly UNEXECUTED.
Its genuine flat holder and current readers are private SHOP work; this is no
published auction capability or auction qualification receipt. No private
candidate was imported, inspected or executed for this document.

## Commands, identities and original owners

The command vocabulary and bounded payload/result live in
`src/economy/auction_command.h`. Actor PID, account name, racewar, expected wallet
and bank revisions, requested value, prices, listing fee, closing fee basis
points, extension, end time and selected root UID/revision/VNUM values travel as
command facts. The result preserves action/event, auction/status, seller/winner/
previous bidder, final price, wallet delta/claim credit, balances, revisions and
selected root results. Payload V1 and native payload V2 are distinct; schema 2
is the accounting envelope version, not a synonym for either payload version.
Root count is bounded by nine, object blob by 32768 bytes and result by 320.

| Cut | Published implementation and ownership |
| --- | --- |
| Player request | `src/economy/auction_houses.c:1517` copies the actual actor account/name/PID/racewar and runtime wallet/bank revisions, and freezes configured fee/extension values. SQL bid entry at `src/economy/auction_houses.c:2491` parses the positive bid and submits; it does not debit money. Staff remove at `src/economy/auction_houses.c:2396` checks trust and submits one auction at a time. Those gameplay access checks remain in the command owner. |
| Listing request | `src/economy/auction_houses.c:1795` fills the actor, computes the listing fee and end time, and submits the selected roots. An item description/blob or selected root list is not custody proof. Native preparation must capture the real selected literals and complete acknowledged player body. |
| Claim selection | `src/economy/auction_houses.c:2800` selects pending money first; otherwise it selects the first unclaimed item auction/roots. These query results are selection hints. The repository later locks entitlement and source rows again. Money and item pickups are separate original commands; buy-now does not merge them into physical delivery. |
| Route selection | `src/economy/auction_transaction.c:168` sends an active regular SQL auction to `auction_native_publication_submit`; otherwise it uses the older pending-command path. Actorless active submissions use the background owner and only finalize/remove are accepted by the native submitter. Legacy callbacks and legacy root-only hydration are not the active native publication path. |
| Real preparation | `src/economy/auction_native_publication.c:2320` polls and holds the actual acknowledged player checkpoint, then calls the private `auction_preparation_owner::freeze`. Native V2 carries original level/save revision, ordered before UIDs, before/after/selected digests and full selected counts through `src/economy/auction_native_command_context.h`. It preserves the original command identity/fences/site/deadline. |
| Fresh accounting capture | `src/economy/economic_gameplay_authority.c:432` requires the selected regular SQL projection for fresh native preparation. It borrows a real pool session, reads under a transaction, verifies the same session and confirmed rollback/idle cleanup, and only then publishes the prepared command. A schema-2 retry only validates its existing frozen binding; it cannot recapture fresh authority. No backend substitution or synthetic accepted timestamp is an admission grant. |
| Transaction ownership | `src/persistence/critical_command_repository.c:3173` dispatches the action to the corresponding SQL lock/execute-and-record participant. The original inbox transaction owns native rows, accounting/source allocations, receipt, outbox and COMMIT. Participants borrow that transaction and cannot independently admit, commit, ACK or publish. |
| Native publication/recovery | `src/economy/auction_native_publication.h`, `src/economy/auction_recovery_context.h`, and the private auction interfaces in `src/player/player_save_pipeline.h:222` retain preparation, immutable original bodies, receipt, physical progress and guarded ACK. Value decoding and pure expected-forest calculation cannot mint those capabilities. |

## State and effect map

For bid, let `P` be locked current price, `W` the locked previous winner, `B` the
requested value clamped to a positive buy price, `C` genuine bidder pending
credit and `F = floor(B * closing_fee_basis_points / 10000)`. First bid permits
`B >= P`; an existing winner requires `B > P`. Payment is `B - P` for the same
winner, otherwise `B`; credit pays `min(payment, C)` first and the wallet pays
the remainder. Bank amount remains unchanged even though its lifetime/revision
is captured and fenced; the existing successful bid result advances both wallet
and bank revisions by one, including a bid fully paid by claim credit. The actual
branch is
`src/economy/auction_repository.c:1034`; the pure comparison/calculation is
`src/economy/auction_accounting.c:318`.

| Operation | Required original state | Atomic domain effects | Later physical effects |
| --- | --- | --- | --- |
| List | Real seller/account/racewar and money fences; legal fee/prices/blob; selected player roots and complete source forest/custody. | Insert auction with a real new ID/listing operation, open status/revision 1; transfer selected custody player→auction; debit listing fee; create/verify the new escrow lifetime only after the native insert in the same transaction. Native branch starts at `src/economy/auction_repository.c:976`; lifetime contract is `src/economy/auction_listing_accounting.h:36`. | Detach/destroy original live selected trees under retained progress, publish all-node custody/current balances, prove whole after player forest, then guarded ACK. Listing fee accounting is not proof of physical removal. |
| First bid | Open status, authoritative custody state, seller PID and seller account different from bidder; exact listing revision and original listing operation; bidder claim presence/absence and wallet/bank lifetimes. | Spend credit/wallet; install winner/value; increment auction revision; append original bid ledger. Escrow holds the resulting winning price. Different-winner extension adds configured seconds unless buy-now closes. | Auction custody remains with auction; refresh current auction runtime custody and actor balances under original native owner. No item placement. |
| Same-winner raise | Same state plus genuine original winning bid operation at current revision. | Pay only the increase, credit first. No previous-winner refund and no different-winner extension. Compare exact result fields and revisions. | Same custody/publication obligations as bid. |
| Outbid | Genuine previous winner/price/previous bid operation, previous claim endpoint or authenticated absence, plus bidder funds/credit. | New bidder pays full bid; stage the old full price to the previous winner's pending claim; advance auction/escrow and record source allocations. This refund is a claim, not immediate wallet payment. | Original bidder's native balances publish; the offline previous winner's money awaits its own claim command. No borrowing another actor's player hold. |
| Buy-now through bid | Positive buy price reached; seller claim endpoint or authenticated absence; all ordinary bid facts. | Clamp to buy price; close listing; stage seller `B-F` and closing fee sink; stage winner item entitlement; retire empty escrow mapping after the plan proves zero. Outbid refund still occurs if applicable. | No delivery in this transaction. A later item-claim command transfers custody and publishes the tree. |
| Timed finalize | Open/authoritative listing, end time reached, original winning source if any, exact escrow lifetime and staged root set. | Increment listing revision/close. Winner: seller net proceeds/fee, winner item entitlement. No winner: seller item entitlement. Custody stays auction. `src/economy/auction_repository.c:1137`; `src/economy/auction_settlement_accounting.h:74`. | Actorless background publication checks current auction custody; no invented player checkpoint or wallet for an actorless command. |
| Staff remove | Original command trust check; locked open/authoritative listing and exact staged items/source. | Increment revision/set removed; stage item entitlement to seller. Existing settlement semantics advance the escrow witness without reimbursing the previous winner. Preserve that behavior; this document proposes no refund policy change. | Current auction custody publication; item return is a later seller claim. |
| Money claim | Positive actual pickup row within native bounds, exact claim revision, original beneficiary/mapping and ordered remaining source allocation set. | Credit wallet; zero pickup and increment claim revision; consume each original source allocation and bind the claim root in the same transaction. `src/economy/auction_repository.c:1194`; `src/economy/auction_money_claim_accounting.h:37`. | Publish actual current wallet/bank through the native owner, then guarded ACK/notice. No item move. |
| Item claim | Original immutable listing/selected literal source plus current unclaimed `claim_pid`, selected root revisions, exact complete forest and current player/auction owner revisions. | Transfer every selected native node auction→player, mark original custody roots claimed with claim operation/time, advance auction and owner/item revisions, link native item events/accounting references. `src/economy/auction_repository.c:1231`; `src/economy/auction_item_claim_accounting.h:39`. | Allocate inert trees through existing original allocator, enroll/reload/place under retained stages, hydrate all nodes/owner clocks and prove full player after image before ACK. No root-only reconstruction or legacy callback delivery. |

Time stays with its owner. The native bid branch does **not** compare end time
before accepting a bid; finalize compares `end_time > time(nullptr)` and refuses
early closure. Different-winner extension stays in the native repository/backend.
A future rules interface must not silently invent a bid
deadline, use a display countdown as authority, or make pure plan replay depend
on today's clock. Preserve each backend's existing range checks. Pure settlement
freezes end time; the transaction decides whether the actual time permits
finalization.

## Existing pure decisions and facts that may cross the boundary

| Existing value interface | What it already decides/compares | What it does not supply |
| --- | --- | --- |
| `src/economy/auction_accounting.h` / `src/economy/auction_accounting.c` | Bid eligibility from supplied listing values, clamping, same-winner difference, credit/wallet split, refund/seller/fee/escrow postings, exact result and source/frozen-intent binding. Writer 9, listing-fee sink 26 and closing-fee sink 25 are existing identities. | Seller account authentication, SQL row presence, real time, live player identity, endpoint creation, admission or physical custody. Seller-account refusal remains in native repository, beyond the PID-only pure listing check. |
| `src/economy/auction_listing_accounting.h` | Existing wallet/bank lifetime intent and post-insert listing/escrow/item event comparison, including supplied genuine source literals/native facts. | Reserving an auction ID or an escrow mapping during a read-only producer capture; proving native selected trees from a caller-created blob. |
| `src/economy/auction_settlement_accounting.h` | Exact original listing/winning source, end time, root/claim rows, escrow/seller/actor account witnesses; sale/remove plan and native after-entitlement comparison. | Clock permission, staff trust, actual current all-node custody, money-source allocation persistence or physical item return. |
| `src/economy/auction_money_claim_accounting.h` | Exact ordered source-set digest and claim→wallet plan against supplied before/result. Source limit 4096 is retained. | Existence/remaining amount/beneficiary ownership of source rows, actual consumption or receipt durability. |
| `src/economy/auction_item_claim_accounting.h` | Exact listing/claim source, staged claimant rows, money witnesses, all-node source custody and matching native item events/revisions. | Original immutable source authentication, current entitlement locks, loader allocation/reload or placement authority. |
| `auction_native_selected_forest_valid` / `auction_native_expected_player_forest` in `src/economy/auction_native_publication.c:127` | Bounded literal topology and root identity; whole before→after forest projection with native insertion order. Rejection/non-moving actions retain before; listing removes selected exact trees; claim inserts complete selected trees and applies original level behavior. | Fresh SQL rows, an admitted command, snapshot capture, a real physical effect or ACK. A valid projected forest is an expected value. |

The pure implementations and their existing SQL bindings are concrete reuse
points. These are already action-specific interfaces; no shared generic auction
executor is proposed:

| Action | Pure implementation | Borrowed SQL participant |
| --- | --- | --- |
| Listing | `src/economy/auction_listing_accounting.c:373` | `src/persistence/economic_sql_auction_listing_transaction.c:600` locks; execute-and-record captures original before, executes native list, obtains actual escrow identity and records the matching plan/source/item references. |
| Bid | `src/economy/auction_accounting.c:318` | `src/persistence/economic_sql_auction_bid_transaction.c:412` locks; execute-and-record compares locked preimage, handles genuinely absent endpoints and records allocations in the original root. |
| Finalize/remove | `src/economy/auction_settlement_accounting.c:316` | `src/persistence/economic_sql_auction_settlement_transaction.c:533` locks; execute-and-record compares exact original settlement, creates a genuinely absent seller endpoint when needed, records claim source and retires escrow only under the existing action semantics. |
| Money claim | `src/economy/auction_money_claim_accounting.c:237` | `src/persistence/economic_sql_auction_money_claim_transaction.c:415` locks; execute-and-record recomputes the original source-set intent, records exact native result and consumes remaining original source allocations. |
| Item claim | `src/economy/auction_item_claim_accounting.c:462` | `src/persistence/economic_sql_auction_item_claim_transaction.c:632` locks; execute-and-record obtains authentic original selected literals for native V2, compares the native custody result and records exact item references/source claim. |

Cross-boundary inputs may be immutable command/result bytes, exact decoded
listing/claim/account values, original operation/source IDs, revisions, ordered
UID/topology/literal vectors, digests and allocation rows. Their provenance must
remain explicit: producer-captured, locked before mutation, retained historical,
or freshly observed current. A field called `authority`, a digest match or a
balanced accounting plan does not authenticate where those values came from.
Only the original native/player/backend owners supply the actual holds, sessions,
mapping lifetimes, world observations, outcome receipts and publication permits.

### Missing endpoint semantics

`src/economy/auction_repository.c:1677` distinguishes native claim-row presence
from amount. An existing zero row is present and must have a valid mapping;
it cannot be encoded as absence. True absence requires no native pickup row,
no active key, no historical mapping for that lineage/kind/native beneficiary,
and no pending source row. The original AEC1 intent freezes absent **beneficiary
PIDs**, with zero mapping IDs, rather than guessing an endpoint ID.

`src/persistence/economic_sql_auction_bid_transaction.c:412` locks the selected
authority lifetimes and absent endpoints under the borrowed original transaction.
Its execute step at `src/persistence/economic_sql_auction_bid_transaction.c:501`
rechecks session/lineage/epoch/mappings/clocks and regenerates the exact original
intent from locked before-state before native mutation. Only after a successful
native operation may it create the actual previous-winner/seller endpoint in the
same rollback-owned transaction, then plan/record against those resolved keys.
The frozen original absent tags/zero IDs remain unchanged. An unused absent
bidder endpoint is not created merely to complete a plan.

Retained endpoint readback in `src/economy/auction_repository.h` and
`src/persistence/economic_sql_auction_claim_endpoint.c` authenticates the original
creator/root/effect/source rows. It returns observed resolved keys without
rewriting the original request. An originally unused absent bidder can still
return ENODATA even if a later command created a beneficiary endpoint. A missing
or retired mapping is not permission to invent replacement identity. Source
allocation consumption and zero balances do not erase original creator proof.

## Historical, current and physical cuts

The original listing operation and the winning bid operation are separate
identities. `src/economy/auction_repository.c:1735` captures the listing and, when
there is a winner, authenticates the same auction revision's bid ledger with
matching winner/price. Listing/claim source authentication through
`src/persistence/economic_sql_auction_retained.h` preserves the original listing
receipt and native literal source. An accepted native V2 request must retain its
exact entitlement/digest/fences; a fresh V1 request may obtain the authentic
source before binding. Replay must not reconstruct an unseen original header
from current auction or player state.

| Proof cut | Exact responsibility and source |
| --- | --- |
| Original locked capture | `src/economy/auction_repository.c:1949` locks owner clocks before globally UID-ordered custody rows, including player/auction membership and root/parent closure. At `src/economy/auction_repository.c:2110` it compares real player level/save revision, complete ordered saved player image and before digest. Selected-node/root counts, full literals, expected after digest and every custody fence are checked before binding. Bid/finalize/remove freeze the current auction nodes even though they move no item. |
| Supported listing literal limit | The listing capture near `src/economy/auction_repository.c:2155` extracts the selected subtree and enforces the historical first-root single-node constraint. General forest-capable helpers do not establish arbitrary-container listing support. Preserve actual supported behavior and test refusals rather than claiming a new capacity. |
| Compound SQL mutation | `src/persistence/economic_sql_auction_bid_transaction.c:590` records the original operation, accounting effects/postings, source claim, credit consumption, previous-winner refund source slot 1 and seller proceeds source slot 2. On buy-now it proves empty escrow and retires that mapping. Other action participants use the same original root. A failure must roll back the whole compound operation. |
| Durable receipt | `src/persistence/critical_command_repository.c:3276` encodes the canonical result, inserts outbox only for mutation, finishes inbox, verifies retained accounting and exact root outbox/session, then COMMITs. Connection failure during COMMIT can be ambiguous; a local failure return is not proof of no native mutation. Existing-ID and receipt paths use retained verification, including `src/persistence/critical_command_repository.c:2084` and `src/persistence/critical_command_repository.c:4199`. |
| Historical retained verifier | `economic_sql_auction_verify_retained` in `src/persistence/economic_sql_auction_retained.c:2623` authenticates original receipt/accounting/source/item evidence under a borrowed transaction. Later source consumption, mapping retirement and current balances cannot recreate or invalidate history merely by differing. This verifier is not a universal current-custody/publication witness. |
| Fresh publication SQL cut | `src/economy/auction_native_publication.c:1304` acquires a fresh actual SQL read transaction and verifies the original retained receipt, real current player account/race/level/save revision and money revisions, auction seller/winner/status/revision, owner clocks, complete player image and all selected/current auction custody rows. It requires confirmed rollback/idle cleanup before exposing observations. Current money/owner revisions may have legitimately advanced where checks allow it; selected moves still compare exact per-node before/after revision and topology. |
| Real refusal | The same SQL-cut function requires genuine delivered `never_admitted` plus an initial envelope, independently excludes original inbox/outbox/accounting/native/source rows, and checks original current fences/world state before cancellation. A manufactured terminal failure or a stub returning no receipt cannot release the original hold. |
| Physical world cut | `src/economy/auction_native_publication.c:704` observes actual game-thread object list, rooms, bodies, descriptors and reciprocal links via its bounded census. It rejects duplicate relevant selected/player-tree UIDs, duplicate matching actor bodies, mismatched runtime and malformed graph/link situations, compares captured complete player image, and observes selected carried/detached/absent trees. The same body discovered through several lists is deduplicated. Runtime zero requires actual PID body absence. This observation returns values/pointers; it grants no SQL, coordinator or ACK authority. |

The world census includes equipment, containment, descriptor original bodies and
detached objects. `capture_body` at `src/economy/auction_native_publication.c:588`
combines ordinary saved-item capture (including its NORENT policy) with selected
full literal subtrees. Selected literal strings/metadata/topology cannot be
replaced by only ordinary saved roots. Actual capture providers are
`src/player/player_snapshot_capture.c:707` and
`src/player/player_snapshot_capture.c:773`; codecs and canonical bytes are in
`src/player/player_snapshot_codec.c`. This is auction's published player/body
observer, not proof that a private flat SHOP PC/legacy-pet source cut has been
adopted for auction or that every native pet journey has run.

## Publication, ACK, save and recovery ownership

1. The actual native submitter at `src/economy/auction_native_publication.c:2641`
   checks game thread, original PC/PID/runtime, bounded pending state and busy
   state; it obtains the real player checkpoint token for actor commands. After
   acknowledgement/hold, preparation retains complete original bodies and binds
   native/accounting facts. `src/economy/auction_native_publication.c:2366`
   strongly retains the original envelope before submission. Ambiguous append,
   overload or delivered refusal cannot replace that attempt with a fresh ID.
2. `src/player/player_save_pipeline.c:6227` submits through the private auction
   checkpoint owner, retaining the original held before/after bodies and encoded
   NAR attachment. Mutable context copies are not release authority. Background
   finalize/remove use their distinct coordinator owner rather than a fake PID.
3. Completion handling at `src/economy/auction_native_publication.c:2725` retains
   retryable/ambiguous cases and compares delivered original operation/outcome.
   The native owner at `src/economy/auction_native_publication.c:1939` verifies
   retained receipt plus fresh SQL/world witness, records receipt/expected after
   in original context, and publishes only through guarded physical stages.
4. `src/economy/auction_native_publication.c:1254` checkpoints progress before
   each effect. Progress 1 means started without a proven return; only the local
   once permit can enter that callback, and it is consumed before invocation.
   Throw/false/unreturned effects cannot be blindly reinvoked. Progress 2 records
   a completed return. Existing inert allocator/reload/enrollment/placement and
   complete runtime custody/balance publication stay with the native owner.
   `src/economy/auction_native_publication.c:2240` hydrates current all-node and
   owner revisions, proves runtime/whole after image, then checkpoints
   `physically_proven`. A callback counter alone is insufficient proof.
5. `src/player/player_save_pipeline.c:6459` proves exact held command, save
   revision/body pair, generation/reservation, no conflicting pending saves and
   a clean ordinary publication census; it calls the actual native participant,
   checks the census again, authenticates terminal context, and invokes guarded
   ACK. `src/persistence/critical_command_coordinator.c:3677` compares full
   command/receipt/outcome and validates the native context. At
   `src/persistence/critical_command_coordinator.c:3799` it durably replaces the
   original recovery envelope with continuation-pending; uncertain ACK remains
   held. Only then are execution fences removed and the acknowledged hold
   consumed. SQL COMMIT and physical publication are distinct from this ACK.
6. `src/economy/auction_native_publication.c:2397` notices/dirty marking run only
   after continuation release, with original progress and final continuation
   retirement. List/item-claim mark STATUS/EQ/INV for subsequent ordinary save.
   Notification does not prove that a later ordinary save has completed. The
   destination-5 event outbox in `src/economy/auction_transaction.c:237` is a
   separate committed-event publication/acknowledgement path.
7. `src/economy/auction_native_publication.c:2854` restores the exact passive NAR
   envelope/context; `src/economy/auction_native_publication.c:2941` handles an
   original replayed-command shell without recapturing/admitting/releasing.
   `src/economy/auction_native_publication.c:2783` permits restored runtime
   rebinding only after actual checkpoint, receipt, fresh SQL and whole-world
   proof. Live attempts require their original runtime; PID equality cannot
   replace a body. Cold after-state proof uses the distinct
   `restored_after_proven` stage and preserves callback progress. Failed proof
   retains the original attempt rather than replaying physical effects.

The game loop wires native auction completion/pulse/publication separately in
`src/net/comm.c`. Keep the original order and private capabilities. Neither a
new domain test nor a plan result may call ACK, consume a player hold or retire
a recovery context directly.

## Flat backend boundary

`src/flatfile/flatfile_auction_repository.h:93` explicitly labels the typed
schema-2 listing/bid/closure/money/item owners **inactive**. Published source
contains their definitions but the active regular native route remains SQL.
`src/economy/economic_gameplay_authority.c:472` refuses fresh selected non-SQL
auction preparation; native submit under `__NO_MYSQL__` also refuses. Existence
of a callable typed flat backend test entry is not active native flat support.

The flat backend at `src/flatfile/flatfile_auction_repository.c:1276` takes its
real root authority lock, recovers the root transaction, loads the catalog and
authenticates original operation digest/result on replay. Its bid branch at
`src/flatfile/flatfile_auction_repository.c:1902` owns native bidder/price/refund/
claim/end-time effects; finalize/remove and claim branches follow there. Under
the same lock it checks authority/source allocations and stages wallet/item
images, catalog/history, pending sources, accounting evidence and mapping
operations. At `src/flatfile/flatfile_auction_repository.c:2428` it builds original
receipt and after-images; at `src/flatfile/flatfile_auction_repository.c:2570` all
evidence/mapping operations commit through one authority journal. Do not split
those into independent domain writes or let a helper reacquire/recover a root
while another owner is using its original cut.

`src/flatfile/flatfile_item_repository.c:4628` routes legacy auction apply to the
legacy entry. Current typed inactive APIs and flat harnesses can establish
backend component facts, but cannot prove SQL-enabled flat worker selection,
native player hold, actual physical publication, once-only ACK or disk/cold
recovery. Those need a published auction-specific retained producer/save-holder/
worker/readback interface and authentic original fixtures. No SQL ACK is flat
durability proof; no shared private SHOP implementation is imported by analogy.

## Narrow next acceptance reservation

**First useful implementable slice:** reserve only a new
`tests/async/test_auction_native_world_observation.py` for the existing
`auction_native_world_observe` boundary, with explicit hand-checked expected
forests also checked against its pure expected-forest/selected validators.
Do not use the function under test as its own sole expected-value oracle.
This is component acceptance,
not new auction policy or native execution qualification. There is no production
change reserved. The existing ownership test extracts the older pending
`publish` body and doubles balance publication; extending its assertions would
still miss this distinct actual observer. Keep it unchanged as a historical
control. A dedicated test is justified by the uncovered published observer.

| Reserved interface/input | Implementation constraint |
| --- | --- |
| `src/economy/auction_native_publication.h` and `src/economy/auction_native_publication.c` | Compile the actual maintained translation unit under `__NO_MYSQL__` with section GC for the selected public observation/forest functions. Keep the actual observer/helper code; do not textually extract or rewrite it. SQL-dependent submission/publication bodies are outside this component's execution. If actual provider closure cannot link, report the concrete dependency before expanding this reservation. Do not invoke private submit/physical/ACK methods or restore active routes. |
| Capture and value providers | Link actual `src/player/player_snapshot_capture.c`, `src/player/player_snapshot_codec.c` and actual dependencies discovered by the compiler. No stub for capture, snapshot/forest serialization, UID census, selected-tree comparison or ownership checks. Record the full direct/transitive provider closure and exact source blobs; unresolved authority providers cannot be replaced to make a link pass. |
| World fixture | Explicit synthetic `char_data`/PC, `obj_data`, room and descriptor graphs provide component input only. Use real character identity definitions, object templates and initialized globals required by actual capture. A game-thread identity fixture may model the test thread; it must not certify native scheduler ownership. Name each remaining inert support double in evidence. No database, journal, player-save or submission authority fixture is supplied by this reservation. |
| Build/storage | C++20, existing strict warnings, ASan/UBSan and baseline/Og comparison for this focused component. Keep writable outputs at `D:\Dev\Builds\Duris\auction-world-observation-20261008\bin`, scratch/evidence under a distinct `D:\Dev\Temp\auction-world-observation-20261008` directory, using `/mnt/d` in WSL and direct D: Docker mounts. No installed package/framework/dependency or broad build is needed. Inspect provider closure first; do not claim an unexecuted recipe already compiles. |
| Delivery/import | Pin the source primary and original controls; authenticate generated harness, exact commands/flags, compiler inputs, ELF and per-case results. Make the single test portable through repository paths, never local private export paths. Qualify on a bare export of the same maintained revision as well as the owned branch plus explicit required source import; the owned older source tree alone is not that candidate. Document missing/different providers rather than silently using another revision. |

Required component cases, each with a positive baseline and an isolated damaged
input, are: exact carried/detached/absent selected tree observations; complete
unrelated player inventory/equipment preservation; native claim insertion order
and listing removal projection; original runtime/PID matching and a replacement
body with the same PID; duplicate selected UID and duplicate world UID; selected
root linked simultaneously to room/player or inconsistent reciprocal location;
extra/missing/reparented child, sibling/containment cycle and malformed global
prev/next chain; descriptor-only/original body discovery; runtime-zero with a
remaining matching PID body; literal selected NORENT subtree versus ordinary
capture policy; invalid selected root order/VNUM/string mask; supported bounds
and refusal without changing a sentinel output. These assert the observer and
value helpers' actual contracts. Do not infer item custody SQL or a successful live auction from
synthetic world graph acceptance. If capture requires an unavailable genuine
provider, report the missing provider and shrink no assertion silently.

### Original controls and subsequent owner acceptance

The following tests were inspected as source, **not run for this document**.
Preserve their original fixtures, compile flags and declared scopes. Resolve
actual current codec/link dependencies before execution; older recipe lists may
predate native V2 providers. This document supplies no fresh passing receipt.

| Existing control | Scope retained / next obligation |
| --- | --- |
| `tests/async/test_auction_bid_accounting.py` and `tests/async/auction_bid_accounting_test.cpp` | Pure bid/outbid/buy-now, credit and damaged source/result/revision controls. Reuse unchanged; no second plan implementation. |
| `tests/async/test_auction_accounting_context.py`, `tests/async/test_auction_listing_accounting.py`, `tests/async/test_auction_settlement_accounting.py`, `tests/async/test_auction_money_claim_accounting.py`, `tests/async/test_auction_item_claim_accounting.py` | Preserve each original context/action component. Their values-only success is not authority or native publication proof. |
| `tests/async/test_auction_retained_seller_fee.py` | Runs original bid component and literal retained fee predicate with real plan providers, positive/zero fee and damaged payout controls. It explicitly reports native SQL qualification 0. Its extracted expected-forest provider is not the native world observer or full owner. |
| `tests/async/test_auction_ownership_publication.py` | Historical extracted pending-publish/root hydration test, `__NO_MYSQL__`, balance double. Do not label it complete current native publication. |
| `tests/async/test_auction_transactional_cutover.py` | Codec and source-contract/SQL-versus-client-free route controls. Source assertions do not execute actual native player effects. |
| `tests/async/auction_bid_sql_accounting_mysql_harness.cpp`, `tests/async/run_auction_bid_sql_accounting_mysql.py`, `tests/async/run_auction_bid_sql_accounting_schema_mysql.sh` | Original disposable SQL bid/refund/sale/claim/endpoint/source and rollback/reconnect component controls. Inspect the harness's genuine borrowed transaction and retained creator checks; caller-supplied SQL rows are not a lifecycle-origin native player. Runner requires explicit loopback disposable schema and currently fixes output under bin/tests; prepare an isolated D: source/build layout preserving those runner requirements. Do not run against .env production by default. |
| `tests/async/auction_transaction_mysql_harness.cpp`, `tests/async/run_auction_transaction_disposable_mysql.py`, `tests/async/run_auction_transaction_disposable_schema_mysql.sh` | Original transaction/idempotence/receipt/outbox controls remain. Native acceptance must exercise the actual command→coordinator→repository→native publisher and save/ACK/replay sequence on the identified candidate, not substitute this backend component. |
| `tests/async/run_auction_listing_sql_accounting_mysql.py`, `tests/async/run_auction_settlement_sql_accounting_mysql.py`, `tests/async/run_auction_money_claim_sql_accounting_mysql.py` | Retain original action participants and original schema wrappers as dependencies when qualifying a compound scenario. Do not run broad suites merely for a documentation change. |
| `tests/async/test_flatfile_auction_repository.py`, `tests/async/flatfile_auction_repository_harness.cpp`, `tests/async/flatfile_auction_source_cases.py`, `tests/async/flatfile_auction_consumption_cases.py`, `tests/async/flatfile_auction_attribution_cases.py`, `tests/async/flatfile_auction_money_cases.py` | Original catalog/source/credit/claim/transaction fault backend controls. Typed inactive schema-2 tests remain inactive component proof; they do not certify the active native flat worker, real save holder or cold crash durability. |

After the observer component, reserve native acceptance only against a named
published owner candidate with original driver/fixture identities. Required
owner inputs are actual lifecycle-origin item/coin receipts and complete opening
correspondence; selected regular lineage/epoch/mappings; authenticated listing,
winning-source and entitlement history; actual player account/PID/racewar/runtime,
whole acknowledged STATUS/EQ/INV before/after bodies, original level/save revision,
all-node custody/owner clocks and full literal metadata; real SQL pooled session
cleanup/rollback/COMMIT uncertainty; actual native allocator/reload/proclib/placement
and runtime counters; original coordinator/player holds, NAR journal and generation
CAS; and a disk-backed stop/restart/load driver with genuine pet/NORENT/runtime
correspondence. For flat acceptance, also require original retained producer
attempt/save-holder, selected worker and borrowed-root current native/money readers.
Private SHOP progress or a synthetic auction row cannot supply these.

The native acceptance chain must pair first bid, self raise, outbid and buy-now
with original money/item claims; timed sale/no-bid expiry and removal; absent
versus existing-zero endpoints; exact original retries after later source
consumption/mapping retirement; stale auction/owner/item/player/runtime/topology
cuts; never-admitted versus canonical durable rejection; ambiguous COMMIT/append;
failures before/during/after physical callback return, context checkpoint and ACK;
warm retry and cold before/after replay. Assert the full original receipt/command,
all money allocations, all item nodes/literals/owner clocks, physical graph/counters,
save body/revision, notice progress and original hold/fence disposition at each
cut. Partial financial balancing cannot stand in for those assertions. Do not
wait for all native journeys to finish before implementing a newly published
usable case, and do not promote a component pass to full qualification.

## Verification of this document

The appendix authenticates every directly named existing source/test/context
file against its frozen Git blob. All maintained src/tests bytes used for this
trace were compared with Git; maintained documentation was read from its pinned
revision. Relative context links were checked against the owned revision, and
the maintained checkpoint link against the complete pinned primary. Line anchors
were resolved and reviewed against those exact bodies. Proposed test paths are
reservations only and do not claim an existing blob.

Executed proof for this delivery is limited to document/source/link/blob and
sole-file diff checks. No C++ compilation, existing test execution, database,
server, migration, native journey, ACK/save/recovery or broad qualification was
run. The prior pickup/R0–R14 handoffs and private primary checkpoint remain
separately scoped evidence. Adoption/review of this design does not complete or
resume the continuing native Goal.

<!-- BLOB_PINS -->

| Revision | Inspected file | Exact Git blob |
| --- | --- | --- |
| maintained | `docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md` | `5328ca2201bd92c517ebf549aca932b684d8cc33` |
| owned | `docs/persistence/economy_accounting/domain-separation/CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e` |
| owned | `docs/persistence/economy_accounting/domain-separation/CURRENCY_NATIVE_ACCEPTANCE_BLUEPRINT_2026-10-08.md` | `15bcec8b8631c46b89f1cc07790f71a2857041bb` |
| owned | `docs/persistence/economy_accounting/domain-separation/ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `70e8cc751e01bb283629582b7572365059725ccc` |
| owned | `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| owned | `docs/persistence/economy_accounting/domain-separation/ORDINARY_PICKUP_COMPONENT_HANDOFF_2026-10-08.md` | `e43b18ef4c5956246200b5ddcc89c655aab8c7e5` |
| owned | `docs/persistence/economy_accounting/domain-separation/POST_R14_OWNER_DEPENDENCIES_2026-10-07.md` | `c32aad66519191ab2530b0f2b1c7e14968822753` |
| maintained | `src/economy/auction_accounting.c` | `eb13c45a40cca2932404735c9a00b5c96c548970` |
| maintained | `src/economy/auction_accounting.h` | `61f71f469d0fc50859aa80ab7c744474893241f1` |
| maintained | `src/economy/auction_command.h` | `a253b28a0e1a7b671cbeef83fe4793c06eb8e459` |
| maintained | `src/economy/auction_houses.c` | `f032b614e58e3e43caeac255be3c6d3f8c6d35f7` |
| maintained | `src/economy/auction_item_claim_accounting.c` | `1e0e96c96fa12ffa58b9865a675299451f2337b4` |
| maintained | `src/economy/auction_item_claim_accounting.h` | `1e79bf5c06eb9c39bd7567c6a5e25e4e84a0aaa8` |
| maintained | `src/economy/auction_listing_accounting.c` | `c92685a87bf98f703824445b9984ba38dd6d9bb5` |
| maintained | `src/economy/auction_listing_accounting.h` | `af353a7fa833eb6d29b9286ba37fee897b1dc018` |
| maintained | `src/economy/auction_money_claim_accounting.c` | `7357540c17535e46ee9a383e6aa6693b478ea437` |
| maintained | `src/economy/auction_money_claim_accounting.h` | `651abd5fb558e76590e216bbb6b19a9216db397e` |
| maintained | `src/economy/auction_native_command_context.h` | `5df71287c8c83414bafb3acd7ce98d53258f2135` |
| maintained | `src/economy/auction_native_publication.c` | `4c982a8d5347fd90c9a111f0eb1b25cf30dd751d` |
| maintained | `src/economy/auction_native_publication.h` | `1913d66408925ee2427acc5b3eef2a16fa725239` |
| maintained | `src/economy/auction_recovery_context.h` | `a92a704ea329cf9daa2eddbd35338f1a87381437` |
| maintained | `src/economy/auction_repository.c` | `b24c8f0b93fc0e3fb781b37153b4b1a86c1ffcc4` |
| maintained | `src/economy/auction_repository.h` | `a0a82ad5d7a34dbd984ad9b4775504705a623212` |
| maintained | `src/economy/auction_settlement_accounting.c` | `b7ed3a61886a6991a5ebdc52deddc2e91fd4d231` |
| maintained | `src/economy/auction_settlement_accounting.h` | `18be6cf97aa9dca831609c6fc34764982cbfc9b2` |
| maintained | `src/economy/auction_transaction.c` | `56a5d3652b5d7efe0921c761e66b56fb49c1334b` |
| maintained | `src/economy/economic_gameplay_authority.c` | `4afa41bdb718068e4862f422b701fa5ad6953d56` |
| maintained | `src/flatfile/flatfile_auction_repository.c` | `b9a94200855dbb1541d965b5f9a868367568e8b7` |
| maintained | `src/flatfile/flatfile_auction_repository.h` | `4437175875890ceaee33f67c54e405bef8f20f42` |
| maintained | `src/flatfile/flatfile_item_repository.c` | `14f21c674e6226bd7e66bce57461f29d06367583` |
| maintained | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| maintained | `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| maintained | `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| maintained | `src/persistence/economic_sql_auction_bid_transaction.c` | `ed60930e2acccf761ddfc7165ed30bb1a1d0edb7` |
| maintained | `src/persistence/economic_sql_auction_claim_endpoint.c` | `30c823ce4d51fdad03a833299a579850b1dee930` |
| maintained | `src/persistence/economic_sql_auction_item_claim_transaction.c` | `2b615828c5cf34ec42fde6560f7d9c2b3949d635` |
| maintained | `src/persistence/economic_sql_auction_listing_transaction.c` | `bc1aaa4581bed9c54ec32727217d00ce6be32eea` |
| maintained | `src/persistence/economic_sql_auction_money_claim_transaction.c` | `45a17721a92cf8e76da470c929570fdb8786ed7e` |
| maintained | `src/persistence/economic_sql_auction_retained.c` | `9db610e513f76f170ba0f9faa0c762adfc2d02a0` |
| maintained | `src/persistence/economic_sql_auction_retained.h` | `ba7075d66ce1ed3ca74159b4895e1e58de09c949` |
| maintained | `src/persistence/economic_sql_auction_settlement_transaction.c` | `4d0abff3633b2b2c9c2924a0a8554265bd68f86b` |
| maintained | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| maintained | `src/player/player_save_pipeline.h` | `e1c2d87094c2eeaa120ceff65406ac7f7b7d5baa` |
| maintained | `src/player/player_snapshot_capture.c` | `9aacf75b74b45f1d2f0911e7325b9d7a3125921b` |
| maintained | `src/player/player_snapshot_codec.c` | `27d440a90d714fd1cba95720edd9e59eec976844` |
| maintained | `tests/async/auction_bid_accounting_test.cpp` | `8a323e0ee11c1f3d2bbd01a5815cbc5657104ddf` |
| maintained | `tests/async/auction_bid_sql_accounting_mysql_harness.cpp` | `891b82e6fc33429d6cb613162b0ac6bbe8f68273` |
| maintained | `tests/async/auction_transaction_mysql_harness.cpp` | `fd1cfbde193f847125f333cdfdf876abfea391fb` |
| maintained | `tests/async/flatfile_auction_attribution_cases.py` | `5e42882630ee781c39de691219c3fe48fe5c0119` |
| maintained | `tests/async/flatfile_auction_consumption_cases.py` | `0f859dacef07196233f089aadf760a12f0e674e6` |
| maintained | `tests/async/flatfile_auction_money_cases.py` | `6ca9cdeeefa968c990d187526dcbaab7c72f2543` |
| maintained | `tests/async/flatfile_auction_repository_harness.cpp` | `d628da33feb2126c16314b6311952c053c2df937` |
| maintained | `tests/async/flatfile_auction_source_cases.py` | `12e059edc663c0810987702761d4aba57dc1fa7f` |
| maintained | `tests/async/run_auction_bid_sql_accounting_mysql.py` | `fc6675115df1a1868a1e159f309615b88a109f13` |
| maintained | `tests/async/run_auction_bid_sql_accounting_schema_mysql.sh` | `8096bf2a8a9755363458498a740de668225fafb3` |
| maintained | `tests/async/run_auction_listing_sql_accounting_mysql.py` | `cd4d9a2baed7e1c9ee4783d7aae4efe22c176364` |
| maintained | `tests/async/run_auction_money_claim_sql_accounting_mysql.py` | `b8c060d3e3577325d58d4a08379f6f485d8c5da4` |
| maintained | `tests/async/run_auction_settlement_sql_accounting_mysql.py` | `c439f9142564e353fb6d23ad4129aff1e58f86a9` |
| maintained | `tests/async/run_auction_transaction_disposable_mysql.py` | `283fd86beaa89280240aa93b306c297fba875f33` |
| maintained | `tests/async/run_auction_transaction_disposable_schema_mysql.sh` | `199c1865d65b4672535e6fe6b3d639f32c0e8522` |
| maintained | `tests/async/test_auction_accounting_context.py` | `7aed89c6588747c353de82c7dc7f9fbb5398c37e` |
| maintained | `tests/async/test_auction_bid_accounting.py` | `b0d579d48d70831366280227375461592f92b5c7` |
| maintained | `tests/async/test_auction_item_claim_accounting.py` | `77f39711c90953d1e2f5ce30c9a37c380a6300f3` |
| maintained | `tests/async/test_auction_listing_accounting.py` | `c2c3545fdc113af117955b94719ba6c244341ded` |
| maintained | `tests/async/test_auction_money_claim_accounting.py` | `81648959bf2c1123050ddef137b41b43c2fc3fc7` |
| maintained | `tests/async/test_auction_ownership_publication.py` | `22e4f597390d5161f3ae7a4aacedf5dd46d41779` |
| maintained | `tests/async/test_auction_retained_seller_fee.py` | `f8abea5af4d46557410f67d668eb42d4179e385f` |
| maintained | `tests/async/test_auction_settlement_accounting.py` | `a8d550082983f551f8b58fd815e190717847190b` |
| maintained | `tests/async/test_auction_transactional_cutover.py` | `9a66858c73b9deff3a1af91dc0847cf052575eb5` |
| maintained | `tests/async/test_flatfile_auction_repository.py` | `832646792147737a81c52f3f847d2f659981c4e4` |
