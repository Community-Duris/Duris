# Collector authority, custody, catalog and publication boundary — 2026-10-08

Collector already has reusable policy and collection/purchase/expiry preparations.
The remaining architecture boundary is ownership of native observation, durable
compound mutation, current proof and publication. Reuse those preparations and
the existing transaction owners. A new Collector facade or another singleton
image extraction would add no authority capability. The one proposed follow-up
here is a bounded acceptance reservation for collection preparation using its
genuine native capture, codec and custody providers.

## Frozen evidence and disposition

All source, migration and async-test references use published primary commit
`31ad6ae72914669dd384eaf29a4c3259badd2dae`. Its trees are:

| Tree | Git object |
| --- | --- |
| `src` | `833d3085815b396861ad18a77635412212381e4b` |
| `migrations` | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| `tests/async` | `790f367adf805a69d53aac6460938f5c921f9136` |

The read-only archive of those three trees has SHA256
`9e03fed11567967b2af93c21239c7b1293e5d0452c785c11116fc2a05878e503`
(45,762,560 bytes; 3,069 files), privately extracted at
`D:\Dev\Temp\collector-authority-20261008\source`. Line anchors below refer to
that export. The appendix authenticates cited bodies with Git blobs; no moving
worktree line number is the source of an implementation claim.

Preserved preparation documents are read at owned baseline
`d8d0b5194967445eca02a40b736869b05abf6c34`:
[inventory](OPERATION_INVENTORY.md),
[post-R14 dependencies](POST_R14_OWNER_DEPENDENCIES_2026-10-07.md),
[R0 handoff](HANDOFF.md),
[item custody map](ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md) and
[currency map](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md).
K1/K3 already identify policy and purchase/expiry preparation; K2/R0 is closed.
R0 implementation `3d2b85b0688684721f8db559cb3ea35b1830a1cc` supplies
`src/economy/collector_collection_image.h`: owned snapshot plus explicit child
weights, checked own-weight subtraction, singleton normalization and bounded
production-codec encoding. Its original component/codec and maintained-build
qualification remains scoped as recorded in the handoff. Native observation,
topology, revisions and custody were deliberately left with their owners.
The published tree mapped here has no such header and still contains the inline
calculation at `src/economy/collector_collection_preparation.c:136`. This is an
adoption distinction, not a reason to repeat R0 or infer primary adoption.

The coordinator separately reports private primary `be8942cd`, 134 production
files and 72 C providers, including retained flat submission and prospective
NBC4/NMB4 cash-role improvements. That candidate is **reported, source-review
only; compiler/native execution UNEXECUTED**. Its bodies were not available to
this inspection, read, imported or adopted here. Neither public flat storage
readers nor that report establishes a completed flat gameplay publication owner.
The post-R14 exact439 shutdown/SHOP/Collector candidate and original native
player/coordinator/publisher driver remain external dependencies. Prior SQL754
receipt/cache reports remain private owner evidence.

This delivery changes this document only. Source, anchors, blobs, links and the
sole-file diff were reviewed; no C++ compilation, regression execution, database,
server, migration, journal, native batch or gameplay was run. The continuing
Plans 1–5/applicable original R1–R8 Goal remains **BLOCKED and unfinished**.
Existing primary work, Plan 5, coverage registries, lifecycle and the independent
quest-fee stream retain their owners and dispositions.

## Values and owning capabilities

| Existing surface | Reusable value work | Capability retained by its owner |
| --- | --- | --- |
| `src/economy/collector_policy.h`, `src/economy/collector_policy.c:116` | Rules, valid records, checked price and lifecycle transitions; indexed bounded due queue. | No clock read, native object, current row lock, eligibility acquisition or durable write. |
| `src/economy/collector_purchase_preparation.h:13`, `src/economy/collector_purchase_preparation.c:42` | Owned actor/detail/custody values produce the existing fenced purchase payload and decoded singleton with unchanged outputs on refusal. | Copying PID/account/racewar, revisions or capacity does not retain the body, bank mapping, inventory or authority. |
| `src/economy/collector_expiry_preparation.c:10` | Available, due, unpaused held singleton plus destruction revision produces the existing expiry payload. | No destruction effect or permission to destroy another root. |
| `src/economy/collector_collection_preparation.c:237`, closed R0 | Native adapter captures source-root fences and singleton bytes; R0 separates the numeric/image part. | Actual graph census, eligibility, registry agreement and detach remain native-owner work. |
| `src/economy/collector_command.h:43`, `src/economy/collector_command.c:156` | Bounded typed action, original listing fence/time, owner identities/clocks, item rows and literal blob; validation of complete unique sorted source topology. | A serialized fence is an expectation, not a held lock or proof of current native placement. |
| `src/economy/collector_accounting.h:12`, `src/economy/collector_accounting.c:207` | Frozen intent and pure comparison of locked before-facts to the exact proposed result/accounting plan. | Authority structs contain evidence values; the SQL session or flat authority lock must authenticate and retain them. |
| `src/economy/collector_catalog_source.c:17`, `src/economy/collector_catalog_cache.c:31` | Durable bootstrap/detail reads and generation-aware runtime projection. | Cache readiness/revision is not admission, current locked proof or publication permission. |
| `src/economy/collector_transaction.c:546`, `src/economy/collector_purchase_publication.c:456`, `src/player/player_save_pipeline.c:2996` | Original command retention and typed sealed completion support retries. | These owners retain continuation, save exclusion, native proof, ACK and callback lifetime; a Boolean or result record cannot replace them. |

Keep identities distinct: listing ID, death operation ID, beneficiary PID, item
UID, VNUM, native pointer, per-attempt runtime ID, owner `(type,id,context_id)`,
wallet/bank lifetime mapping, lineage/epoch and native SQL bank/materialized row
ID. A corpse owner combines beneficiary PID and corpse SAVEID; room ownership
uses virtual room number, not array index. Collector owner ID is the listing ID
with zero context. A held item is a singleton with root UID equal to selected UID
and parent zero. Collection source rows instead retain their complete **old**
root and parent identities. `src/economy/collector_collection_preparation.c:85`,
`src/economy/collector_purchase_preparation.c:42` and
`src/item/item_transfer_command.c:1416` are the concrete identity boundaries.

## Actual lifecycle and acquisition boundary

| Operation | Native capture / preparation | Durable or publication boundary |
| --- | --- | --- |
| Death enrollment | `src/economy/collector_death_enrollment.c:59` freezes enabled policy for a real PC corpse; `:96` resumes the original committed death identity/policy; `:133` attaches eligible individual snapshot UIDs to the corpse-create transfer. | `src/economy/collector_repository.c:1615` recomputes eligibility from the payload's real decoded snapshots, authenticates existing death facts and selects only missing UIDs. `:1777` enrolls against post-transfer item revisions. The original item transaction composes this with custody and publishes invalidation/committed enrollment (`src/item/item_movement_transaction.c:2104`, `:2286`). |
| Cancel on acquisition/destruction | `src/economy/collector_custody_boundary.h:17` classifies the actual transfer reason and endpoints; candidates are matched against every affected item UID, including descendants. | `src/economy/collector_repository.c:1432` prepares cancellation under catalog/listing and item-history locks; `:1559` persists the plan only with the successful item mutation. `src/persistence/critical_command_repository.c:3055` composes boundary and enrollment before inbox/outbox/commit. |
| Collect | `src/economy/collector_maintenance.c:268` invokes the existing collection preparation once due. Capture proves native location and full original ownership root, while the image describes only the selected singleton. | `src/economy/collector_repository.c:2190` locks and validates current source authority and stored physical facts before policy/price/detach. `:951` repairs remaining topology, moves the selected item and records custody. Native completion calls `src/economy/collector_collection_preparation.c:310` before runtime publication and `:325` detach. |
| Activate / pause / resume | `src/economy/collector_maintenance.c:226`, `:65`, `:369` prepare typed transitions from due entries and feature-enable boundaries. | The repository checks the listing revision and pure transition under the same catalog/listing transaction. These are metadata transitions; they do not transfer an item or spend money. |
| List / inspect | `src/economy/collector_service.c:160` gates real PC access, live service room and cache readiness; `:230` lists beneficiary entries. `:260` requests detail; `:877` checks request/listing/consumer and re-resolves the current player. | `src/economy/collector_listing_pipeline.c:40` reads actual selected backend detail; `:164` rejects mismatched response identity. `src/economy/collector_purchase_preparation.c:121` requires canonical equality of runtime/detail records and genuine singleton decode. A read is not a reservation. |
| Purchase | `src/economy/collector_service.c:349` captures real PID/account/racewar, wallet/bank and player-owner clocks, capacity and observed time. `:808` prepares from fresh returned detail and current held registry. | Active purchase uses `src/economy/collector_transaction.c:546` with original listing/operation and accounting intent. Repository commits custody, native item, wallet/bank clocks, ledger and catalog atomically. Retained publication subsequently obtains current proof, materializes/verifies and ACKs through the save owner. |
| Expire | `src/economy/collector_maintenance.c:131` consumes its own detail response, captures current held/destruction clocks and wall time, then uses `src/economy/collector_expiry_preparation.c:10`. | Due, available and unpaused policy plus exact held singleton fences drive Collector → destruction, destroyed state, item/owner/listing/catalog clocks and ledger. The held accounting capability covers this topology; collection of a whole source tree is outside that capability. |

Eligibility at death is per snapshot: positive UID/VNUM, TAKE, excluding money,
corpses, artifact, transient, NORENT, NOSELL, ACCOUNT_BOUND and keyword `unique`
unless `powerunique` (`src/economy/collector_policy.c:39`). A container's
eligibility neither enrolls nor excludes all children by inference. Attachment
requires snapshots and authoritative transfer rows to correspond; eligible UIDs
are sorted and unique. Existing death identity/policy wins on resume, rather than
current defaults. Enrollment records a candidate; it does not put the object
into Collector custody or grant beneficiary access to its current environment.

Actual acquisition closes candidate lifetime. Mobile claim is classified even
when the aggregate custody endpoint remains a room or corpse; raised-pet claim
has its own reason. Any player/shopkeeper endpoint, including a later drop/put,
closes stale candidacy. Destruction closes it as destroyed. Ordinary room/corpse
environmental movement does not close candidacy by itself. The corpse lifecycle
classifier (`src/economy/collector_custody_boundary.h:62`) distinguishes
destruction, resurrection/raise/follower claim, nested release to a player, and
ordinary release/remove/upsert. Do not replace those cases with “corpse gone” or
“container touched” rules. Flat corpse composition consumes sorted actual
affected UIDs and post-item revisions (`src/flatfile/flatfile_collector_repository.c:1019`,
`src/flatfile/flatfile_corpse_repository.c:459`). This source shows that flat
composition; it does not establish an equivalent SQL corpse-lifecycle hook.

The SQL item boundary uses an initial observation followed by a current locking
item-history read. An empty range protects against concurrent enrollment; a
candidate appearing between reads causes retry to preserve catalog → listing
lock order. Cancellation follows all matching affected UIDs and records actual
post-transfer item revision, with a same-original-death exclusion. It is not an
arbitrary historical ledger scan or a selected-container-only invalidation.
No candidate cancellation becomes authoritative from a rejected item transfer.
The existing legacy-execution guards in apply/enrollment remain real limits;
this map grants no new schema-2 enrollment capability.

Maintenance translates preparation outcomes into the existing terminal policy:
missing/destroyed item → destroyed cancellation, claimed custody → claimed,
excluded item → excluded, stale custody/invalid topology → quarantined; limit or
allocation failures retry rather than manufacture a terminal reason
(`src/economy/collector_maintenance.c:268`). Candidate cancellation and held
cancellation differ: held cancellation must also transfer its exact singleton
to destruction/quarantine with the appropriate custody state. Terminal records
cannot be reopened by a later drop, cache reload or replay.

## Root completeness, image fidelity and native lifetime

Collection searches the full `object_list` for the selected UID and rejects a
repeated pointer chain or duplicate selected UID (`src/economy/collector_collection_preparation.c:25`).
It walks reciprocal parent membership to the registry's old root (`:70`), proves
that root's actual room or PC corpse identity (`:85`), then traverses every item
in that source root (`:107`). Each row must agree on UID, VNUM, old root/parent,
owner identity/revision, active state and nonzero/nonmaximum item revision.
The resulting rows are sorted by UID. Native pointer uniqueness in that walk
and selected-UID uniqueness in the global scan are distinct checks; neither is
a general global census of every unrelated UID. Command validation separately
requires strict UID ordering, one root and connected acyclic parent chains
(`src/economy/collector_command.c:156`). Preserve both stages.

The source root may include an ancestor, selected container, descendants and
siblings. All are revision/topology fences even though only the selected item
changes owner. The selected current item revision may exceed the original
enrolled revision after environmental changes; it must not be older, zero or
maximum. The native preparation must not shrink this fence to one item or use
the minimum sorted UID as the selected/root identity.

The image uses the real ordinary `player_item_snapshot_tree_capture`, retaining
its normal property mask/prototype-reference behavior, then keeps the first
snapshot, sets parent to `PLAYER_SNAPSHOT_NO_PARENT`, equipment to zero and
weight to selected total minus direct child totals. Direct child weights include
their own contained weight; subtracting every descendant again would be wrong.
Negative weights, checked accumulation/subtraction, int32 range, codec failure,
empty bytes and the Collector blob cap are refusal boundaries. The public body
is `src/economy/collector_collection_preparation.c:136`; the real ordinary versus
full-literal entry points are `src/player/player_snapshot_capture.c:766` and
`:773`. A proposed test must not silently switch to forced full-literal capture.
Closed R0 already owns these image calculations on its compatible branch.

Before live collection publication, `collector_collection_live_matches` recaptures
the full root fence and exact singleton bytes, assigns the selected pointer only
on success and rejects changed item rows/count/bytes (`:175`, `:310`). It does
not acquire durable authority. SQL additionally validates its stored physical
tree and selected cost/own weight (`src/economy/collector_repository.c:752`),
reparents direct children to the selected item's former parent, subtracts only
selected own weight from ancestors and deletes only the selected physical row
(`:829`). Live detach releases direct children to that former container or room,
then extracts the selected object (`src/economy/collector_collection_preparation.c:325`).
Collection is therefore a singleton acquisition with complete source-root
effects/fences, not wholesale purchase or destruction of a container tree.
Every fenced source-root row, including remaining siblings, receives the
repository's next item revision and custody ledger entry; both owner clocks
advance. A surviving child becomes its own root when the selected source root
is removed, or retains the enclosing old root when collection was nested.

The native adapter is not entirely pure even before detach:
`src/item/item_ownership_runtime.c:608` creates a missing valid owner-revision
projection at zero when queried, whereas `:596` only peeks. Collection asks for
the destination Collector owner revision through the former API. Preserve and
disclose that existing projection initialization; it is not durable key creation,
custody transfer or a reason to invent a new writer.

Raw `P_obj` and `P_char` pointers remain local observation/effect inputs. The
async listing request retains request/listing/consumer/PID facts, not a borrowed
body across the wait. Service re-resolves the player and repeats accessibility,
busy, listing and actor capture after detail returns. The retained purchase
resolves PID and obtains its current runtime ID on each attempt
(`src/economy/collector_transaction.c:156`); the native owner compares PID,
runtime, canonical account and racewar before/after effects. Runtime identity is
an attempt lifetime check, not a new identity added to the serialized command.

## Time, price, capacity and financial ownership

| Fact | Existing rule | Authority / reuse limit |
| --- | --- | --- |
| Death and due times | Enrollment freezes death time, collection/sale delays, holding duration and price rules; defaults in `src/economy/collector_policy.h` are disabled, 12h collection, 24h sale, 7d holding, 200%, minimum 100 copper. | `collector::enroll` checks additions and valid frozen rules (`src/economy/collector_policy.c:196`). Configuration reload affects capture and pause/resume scheduling; it does not rewrite an existing frozen death policy. |
| Collection price | `src/economy/collector_policy.c:175` computes checked ceil of positive current base value × percent / 100, floors at minimum; nonpositive base contributes zero. Collection freezes that price from current captured/stored selected cost. | Do not reprice on list, purchase, replay or publication. A future image provider cannot substitute prototype cost or quote from a different item. |
| Sale and holding window | Activation requires `now >= sale_at`, then sets actual `available_at=now`, expiry `now+holding_duration` (`:269`). Purchase requires `available_at <= now < expires_at`; expiry requires `now >= expires_at`, both unpaused (`:286`, `:308`). | Wall time is captured as payload `observed_at`; the repository applies that original time. Replay must not refresh it. Steady clock throttles background audits and leases, not business deadlines. |
| Feature pause | `:324`, `:337` pause an available unexpired listing and shift expiry by exact elapsed pause time on resume, with overflow guards. Maintenance bounds transition time by availability/pause and the configured enable-change boundary. | Cache unready alone is not a policy pause. `src/economy/collector_presence.c:42` preserves service NPC presence during normal cache refresh while command accessibility remains gated. |
| Capacity | `src/economy/collector_purchase_preparation.c:31` uses captured carried count/limit and overflow-safe carried weight plus nonnegative singleton weight. | `capacity_admitted` is a native admission fact frozen into the command, not a current SQL inventory recomputation or a general future capacity promise. Publication proves exact physical state under the shared save hold. |
| Wallet / bank | `src/economy/collector_service.c:327` values denominations 1/10/100/1000 with negative/overflow rejection. Purchase spends carried wallet only. SQL locks PID/account/racewar wallet and the corresponding shared account-bank row (`src/economy/collector_repository.c:1029`). | Account normalization, racewar and mapping lifetimes remain distinct from PID and bank row ID. A large bank balance cannot satisfy a short wallet. No new bank-funding or coin-pile policy is introduced. |
| Purchase result | SQL rewrites the remaining wallet into canonical denominations, advances wallet **and bank** revision, leaves bank amounts unchanged and writes native currency effects (`:1106`). Accounting plan checks those exact facts (`src/economy/collector_accounting.c:207`). | Writer 7 debits wallet value and credits sink 24, with the bank as a zero-value-change account and singleton custody event. This is a compound purchase, not independently committable money and item operations. |

Listing revision, catalog revision, item revision, both owner revisions,
wallet/bank revisions, mapping revisions and player-save revision are different
clocks. Successful collect/purchase/expire advances the relevant item/listing and
owner/catalog clocks; activate/pause/resume change metadata clocks without item
movement. Candidate cancellation records the enclosing transfer's post-item
clock; held cancellation advances the held item clock. Hint/hint_ack are durable
first-availability notification metadata, not eligibility, price or custody
changes (`src/economy/collector_command.h:33`). Never treat catalog revision as a
substitute for every domain clock or a durable result's maximum revision as fresh
current-state evidence.

## Durable source, atomicity and retained proof

Bootstrap and detail reads route through the real SQL pool or configured flat
root (`src/economy/collector_catalog_source.c:17`, `:60`). Detail is bounded and
nonlocking (`src/economy/collector_repository.c:2161`); bootstrap validates
catalog, held rows and death projection (`:1872`). Cache installs only completed
generations, refuses a catalog older than runtime, rebuilds custody/death/due
projection before ready and preserves later leases for identical entries
(`src/economy/collector_catalog_cache.c:31`, `src/economy/collector_runtime.c:212`).
Invalidation clears readiness and latches another read if one is inflight;
periodic refresh may keep a last good projection on failure. Runtime publication
ignores older listing revisions, rejects different bytes at the same revision
and takes the catalog maximum (`src/economy/collector_runtime.c:228`). These are
projection rules, not permissions to purchase from stale cache or replace a
sealed original command with a newer listing.
`src/economy/collector_runtime.c:395` selects available, unpaused beneficiary
records without taking wall time; list output is a discovery hint. Purchase
preparation and locked policy enforce the actual time window again. Due-queue
lease state is scheduling state and cannot extend a player's sale permission.

SQL mutation composes catalog/listing locks, wallet/bank and ordered owner locks,
full root/held authority and physical payload checks in the already open native
transaction (`src/economy/collector_repository.c:2190`, `:2450`). Accounted
execution accepts writer 7 purchase or writer 8 held expiry/cancel with exact
frozen intent; source-tree collection is explicitly outside held accounting
(`src/economy/collector_accounting.h:62`).
`src/persistence/economic_sql_collector_transaction.c:345` locks lineage/epoch
and mappings on the actual session; `:408` renews/compares that context, obtains
locked before-facts, executes the native repository and records the matching
accounting operation/effects/postings/item reference. It borrows the outer
transaction and does not COMMIT independently.

The outer critical repository encodes the full result, writes native outbox and
inbox, verifies retained accounting/outbox/session, and commits once
(`src/persistence/critical_command_repository.c:3401`). A connection failure at
COMMIT remains ambiguous and reconciles the original ID. No facade may commit
wallet first and custody later, fabricate receipt success from preparation, or
discard the original operation after uncertain submission.

Flat repository source has the corresponding borrowed authority-lock discipline:
it recovers/authenticates original operation and intent, prepares domain images,
custody/world changes, wallet, singleton materialization and item/accounting
references; then stages them in one authority operations commit
(`src/flatfile/flatfile_collector_repository.c:1242`, `:1912`). Its retained
reader (`:2011`) authenticates original completion/native evidence under a
caller-owned lock; its current projection (`:2146`) checks active mappings,
current catalog/domain/owner state, selected singleton uniqueness, exact original
success bytes/clocks, saved snapshot and materialization reconciliation. That
reader's existence does not wire the public active flat gameplay submission or
publication path: `src/economy/collector_purchase_publication.c:530` returns
unavailable for no-MySQL/flat primary, and `:456` refuses flat publication.
Do not substitute private reported wiring for these inspectable public limits.

Historical receipt and current proof have separate jobs. The SQL retained verifier
(`src/persistence/critical_command_repository.c:4631`) checks the original inbox,
canonical result and corresponding native/accounting facts. Publication also
calls `src/economy/collector_repository.c:2625` in a fresh locked transaction:
catalog/listing, wallet/bank, owner rows and exact singleton physical rows and
extensions must agree. For committed success, wallet vector/revision stays exact
to the original; shared bank revision may be newer, with exact vector required
when its revision is unchanged. Catalog/owner revisions may advance legitimately,
but the selected purchased record/item and original literal payload may not be
replaced. Rejection proves current held state and absence of selected player item
without materialization. Player-save revision is read as part of this proof;
whole-player save exclusion remains with the shared publication owner.

The native publisher holds the actual SQL transaction through observation/effect
and checks retained receipt against the seal on that same session
(`src/economy/collector_purchase_publication.c:305`). It rechecks transaction and
session identity, then requires confirmed rollback and verified idle cleanup
before reusing the pool lease. A cached projection, historical ledger, caller
assertion of a lock or successful arithmetic cannot supply that capability.

## Publication, save, replay and cold ownership

Active admission preserves original listing, operation, command bytes, frozen
intent, accepted timestamp and publication-required schema 2
(`src/economy/collector_transaction.c:546`).
`src/economy/economic_gameplay_authority.c:746` requires existing wallet/bank
coverage and preserves admitted intent on historical replay; wallet-root/recovery
qualification scopes retain their specific refusals. Already admitted legacy
commands cannot simply be retagged with today's epoch or mutable listing.
The transaction retains its owner before admission and keeps uncertainty.

Completion validates original operation/disposition/failure stage, canonical
272-byte result and zero tail, and exact transition/clocks/vectors against the
original listing (`src/economy/collector_transaction.c:91`). A whole completion
batch is sealed before effects; contradictory receipts retain the first seal and
block progress (`:657`). Exact canonical retries and reentrant callbacks must
retain this owner, not create a fresh purchase or refund.

The actual publication effect (`src/economy/collector_service.c:474`) uses a
bounded global selected-UID census and genuine graph materializer. It records
materializer-started/returned state, so a partial/throwing materializer is not
blindly repeated. It requires a single carried root with no children, SQL's real
positive materialized row ID, ordinary snapshot/codec byte agreement and correct
registry/balances/catalog; it re-resolves the body around effects. Flat fixture
row-ID conventions in legacy service code are not SQL proof. Existing partial
state remains unresolved until the same owner can prove it.

The native SQL owner strengthens this with actual descriptor/body/object/room/
inventory/equipment/morph census, current registry ceilings, identity and literal
checks (`src/economy/collector_purchase_publication.c:58`, `:77`, `:179`, `:276`).
An online rejected purchase invokes no materializer and publishes only verified
current balances. An offline committed purchase proves absence of a live body
and selected native object, publishes nonphysical current projection, and leaves
the real durable singleton for future load (`:226`, `:305`). It must not summon a
replacement body/item or ACK while partial materialization is unresolved.

The shared save owner freezes canonical original command bytes, installs a
publication execution hold before admission and releases a new hold only for
exact synchronous pre-admission refusal
(`src/player/player_save_pipeline.c:2896`). Restoration registers the original
schema-2 publication obligation (`:2682`, `src/economy/collector_transaction.c:469`)
without materializing or ACKing at boot. Before publish it checks the exact held
command/generation, pending saves, dirty/unacknowledged revisions, retained
journal frames, covered ordinary saves and publication census; it calls native
proof, checks census again, then alone supplies guarded ACK (`:2996`). A verified
never-admitted completion has a distinct cancellation path. These are shared
player/coordinator capabilities, not Collector policy return values.

After an effect succeeds but ACK fails, the owner retries native proof rather
than repeating the grant. Notification starts only after ACK and is at most
once even if it throws (`src/economy/collector_transaction.c:156`);
`src/economy/collector_service.c:602` does not materialize a second time.
Result-only outbox cannot replace a retained purchase's sealed completion or
native proof (`src/economy/collector_transaction.c:790`). Legacy completion,
outbox convergence and bounded service recovery are different paths (`:320`,
`src/economy/collector_service.c:674`, `:722`, `:1027`): a reconnect alone does not
prove a fresh inventory reload, and legacy failure behavior is not the active
purchase owner's publication contract.

## One future reservation: actual-provider collection capture acceptance

**Proposed only; not implemented or executed here.** Reserve one offline component
acceptance delivery for `collector_collection_prepare` plus
`collector_collection_live_matches` and existing command validation. The exact
proposed new paths are `tests/async/test_collector_collection_native_capture.py`
and `tests/async/collector_collection_native_capture_harness.cpp`; neither exists
in this delivery. Keep the old focused harness because its native detach seams
serve another scope. This reservation creates no production API/facade and does
not reopen R0. On a compatible candidate already containing R0, call its image
preparer through the real adapter; if qualifying published primary instead, pin
its original inline body explicitly and make no R0 adoption claim.

Existing `tests/async/test_collector_collection_preparation.py:23` links only
policy, collection adapter and its harness. That harness replaces identity,
corpse/Collector owner IDs, registry lookup/revision, `isname`, snapshot capture,
codec and native detach helpers
(`tests/async/collector_collection_preparation_harness.cpp:115`, `:141`, `:158`,
`:178`, `:195`). It checks useful adapter decisions, but its 32-byte fake codec
cannot establish actual snapshot properties/prototype masks/canonical bytes or
real registry interaction. R0 improved real-codec/image coverage on its owned
branch; native capture/custody seams remained intentional. The new acceptance
benefit is those actual providers, not another test of extracted subtraction.

| Required provider in the reserved component | Available public path / qualification |
| --- | --- |
| Entire collection adapter and policy | `src/economy/collector_collection_preparation.c`, `src/economy/collector_policy.c`; call public functions, no copied/extracted function bodies. |
| Genuine ordinary capture and canonical item codec | `src/player/player_snapshot_capture.c`, `src/player/player_snapshot_codec.c`; keep ordinary masks, real properties/strings and parent rules. |
| Genuine custody projection and identity/topology validation | `src/item/item_ownership_runtime.c`, `src/item/item_transfer_command.c`, `src/economy/collector_command.c`, `src/economy/collector_codec.c`; seed real runtime API, never define replacement lookup/revision/identity functions in the harness. |
| Native eligibility helper and transitive link closure | `src/world/handler.c:967` supplies actual `isname`; dependent provider closure must be probed against the exact candidate. No successful link, exact minimal link set, compiler or execution is claimed here. |
| Fixture-only seams | Explicitly disclose synthetic `world`, object/prototype/global lists and fatal/log/game-thread seams if needed. No DB/coordinator/save/publication/materializer double may be called authority evidence. Unused detach dependencies may be section-excluded only if recorded; never call or qualify detach that way. |

Use manual fixture facts for expected UID-sorted source rows and a manually
authored expected singleton snapshot encoded by the real codec. Do not compute
the oracle by calling the production preparation twice. Require production
command encode/decode validation too, because preparation traversal and command
UID/topology validation are separate. Positive controls should include room and
PC-corpse roots, selected root and nested selected item, descendants/siblings,
nonmonotonic UID placement, canonical property/affect/string data and prototype
mask behavior. Every preparation/refusal leaves native graph and genuine custody
item rows and preseeded owner revisions unchanged; refused outputs retain
sentinels. Seed the destination zero-revision owner explicitly before measuring
those controls. A separate missing-destination-key control must characterize
the existing zero-revision projection initialization described above rather
than incorrectly require a mutation-free owner lookup.

| Meaningful control | Required observation / remaining limit |
| --- | --- |
| Change nonselected sibling/ancestor/descendant revision, owner revision, old root/parent, VNUM, state, count or reciprocal membership after prepare. | `live_matches` refuses the old full-root fence; successful selected-only byte equality is insufficient. No claim that unrelated roots are globally fenced. |
| Duplicate selected UID, pointer cycle, bad corpse PID/SAVEID or wrong room VNUM; duplicate nonselected UID in otherwise similar sibling rows. | Selected global census/location checks and command strict UID uniqueness are exercised separately. Record which layer refuses. Do not invent a guarantee that native traversal alone checks every UID globally. |
| Alter selected persisted literal property/cost/condition/affect/extension under unchanged custody fences. | Exact ordinary recapture/codec bytes refuse publication. A field omitted by the real ordinary mask must be characterized as such, not forced into a full-literal oracle. |
| Flip eligibility exclusion/TAKE or `unique`/`powerunique`, or use player/shop/mobile-claimed custody before prepare. | Assert actual existing eligibility/custody outcome; do not introduce a blanket parent/child policy. Mobile aggregate-room claim history requires durable boundary acceptance outside this component. |
| Child aggregate weights exceed selected total, negative weight, canonical blob limit, root count limit and revision maximum/staleness. | Existing refusal/strong-output behavior is observed through actual adapter/providers. R0's arithmetic edge matrix remains reused evidence; impossible native field values are not fabricated as valid native objects. |
| Repeat unchanged prepare/compare with manual expected rows/bytes and preseeded owner keys. | Deterministic payload, no native/item-row/owner-clock mutation, exact selected pointer returned only on successful comparison. Missing valid owner-key initialization has its separately characterized behavior. This proves capture/preparation, not detach, durable custody or ACK. |

A future runner must pin source trees/blobs, compiler/profile/dependency inputs,
exact provider list and all seams; retain complete commands, stdout/stderr,
bounded compile/link/runtime deadlines and nonzero/timeout artifacts privately
on task-specific D: storage. O1 and Og sanitizer profiles may be used if the
candidate/toolchain supports them, with independent binary and output artifacts.
Qualification would be limited to the actual linked capture/codec/custody/command
component and executed controls. A missing genuine provider, new linker defect
or failed control is a finding, not permission to add copied fallback providers,
rewrite production policy or repair a shared runner in this reservation.

This component needs the exact public source (available) and a future verified
provider/compiler closure (unexecuted). It does not need private native journey
exports. The separate original Collector admission → compound commit → retained
normal-stop → replay → cold-load/publication/ACK acceptance still needs the exact
compatible native/shutdown candidate, original driver and player/coordinator/save
owners, source/export authentication, native materialization/current-proof
dependencies and executed meaningful rejection/retry/damage controls identified
by the post-R14 owner. Public SQL/accounting runners are available starting
points, not substitutes for that missing driver. Nothing here forces that journey
against an unavailable candidate or promotes prior reported SQL754 evidence.

Collision limits: future capture work touches only those two proposed test paths
unless a separately reviewed finding changes scope. Preserve Collector source,
R0, existing runners, custody/transaction/accounting repositories, shared save/
coordinator/publication owners, source/export drivers, coverage/registry/schema,
Plan 5, operational scripts, quest-fee work and private evidence. No new authority
or lifecycle mode is required. Separation already suffices for reuse; execution
of this reservation would supply the missing actual-provider component evidence.

## Existing tests and limits of this inspection

The focused purchase and expiry preparation runners use the production codec
and owned value fixtures (`tests/async/test_collector_purchase_preparation.py:28`,
`tests/async/test_collector_expiry_preparation.py`). Policy/death enrollment,
runtime/cache, listing pipeline, maintenance, service and transaction tests are
existing narrower controls, with provider seams that must be named for any new
claim. For example, cache replaces source/runtime and service replaces registry,
detail pipeline, transaction and materializer
(`tests/async/collector_catalog_cache_harness.cpp`,
`tests/async/collector_service_harness.cpp`). The transaction runner's explicit
link list (`tests/async/test_collector_transaction.py:33`) is not the full native
purchase publication/save closure. Preserved R0 handoff records adjacent shared
link failures and scoped diagnostics; this inspection neither reruns nor repairs
them and does not convert them into passes.

`tests/async/test_collector_repository.py:19` is source-contract inspection,
while `tests/async/collector_repository_mysql_harness.cpp` and
`tests/async/run_collector_sql_accounting_mysql.py` describe disposable SQL/native
repository/accounting scopes. Flat repository harness is another storage scope.
None was executed here. Schema ownership remains the existing catalog,
notification identity and item-owner migrations
(`migrations/immutable/0018_collector_catalog.sql`,
`migrations/immutable/0021_collector_notification_identity.sql`,
`migrations/collector_item_owner.sql`); no migration is proposed.

## File authentication appendix

Source/test/migration blobs below are resolved at
`31ad6ae72914669dd384eaf29a4c3259badd2dae`. Preserved documentation is resolved
at `d8d0b5194967445eca02a40b736869b05abf6c34`; R0's image header is resolved at
`3d2b85b0688684721f8db559cb3ea35b1830a1cc`. These are body identities, not
qualification results. Future proposed paths have no blob or execution result.

<!-- AUTHENTICATED_BLOBS -->

### Published primary

| Repository path | Git blob |
| --- | --- |
| `migrations/collector_item_owner.sql` | `98508c2ec4d87f5ddf80ae0e3b4ee086edcc3ecc` |
| `migrations/immutable/0018_collector_catalog.sql` | `c29540285e883773b0aa9114ab9038e4ffd41931` |
| `migrations/immutable/0021_collector_notification_identity.sql` | `501a7eaf9418552482969c32c3ce2ee37cafa56b` |
| `src/economy/collector_accounting.c` | `ea03a9cd7a5b0960faa9b305cfb7491e6ab42c99` |
| `src/economy/collector_accounting.h` | `ab8cc062cf54da40e8ceef496a92384eb4048bf3` |
| `src/economy/collector_catalog_cache.c` | `ef31af72fd9ad31502f414479811c12e21453ae0` |
| `src/economy/collector_catalog_source.c` | `6f6af8cd685ef7a773842bac74420eaaad5de5fc` |
| `src/economy/collector_codec.c` | `b129bad185d1a6aefb9f1bde93ebeb6974f79cc5` |
| `src/economy/collector_collection_preparation.c` | `90bca1b00e8991650a7cc3958866fcefcf732515` |
| `src/economy/collector_command.c` | `ac2f432e46e29335a4b1e0fdc5340fa831105973` |
| `src/economy/collector_command.h` | `a1f5329bc19bce2662016867733f3a2cbbe37ff6` |
| `src/economy/collector_custody_boundary.h` | `7bcf4d1bdcf59fee81fad32cbd3b74383d8c3286` |
| `src/economy/collector_death_enrollment.c` | `2deaa35fbc6059c6dd53974da109a84ff6562d38` |
| `src/economy/collector_expiry_preparation.c` | `43961d2bbfd3b9e4b6b49bbbb347c39cecc3869e` |
| `src/economy/collector_listing_pipeline.c` | `3b770d0ed18e4f0ef6fccb8a819fafcf1d9328b5` |
| `src/economy/collector_maintenance.c` | `3e9c2c7c9e60d23648a2bebc22bfc69f76268b17` |
| `src/economy/collector_policy.c` | `931ed49946224e9eefed11071930383049d46dcf` |
| `src/economy/collector_policy.h` | `05231b0be7b76eb2442f9c7f58e0c5191d22235e` |
| `src/economy/collector_presence.c` | `ff687bc86227345c3f9302a30f18efac264f9447` |
| `src/economy/collector_purchase_preparation.c` | `4e2fb11ef18b3dcf10abb9c3efedb90e59ca7464` |
| `src/economy/collector_purchase_preparation.h` | `7557081d2345f0b5ad2c40bedafb6346caeba563` |
| `src/economy/collector_purchase_publication.c` | `cdd47d59b2631519bdfc08c2d2c506ab571c09eb` |
| `src/economy/collector_repository.c` | `95398c4b0201e1ed1ab853ce9ede120273f8403a` |
| `src/economy/collector_runtime.c` | `99a0b2264d5f33d231e05530124ccd00310bdf54` |
| `src/economy/collector_service.c` | `d00dec184310c6d2c1c477f8803cf2895298e0d3` |
| `src/economy/collector_transaction.c` | `9c586a1bdfdaff3272b7f8cb66942b2336068247` |
| `src/economy/economic_gameplay_authority.c` | `4afa41bdb718068e4862f422b701fa5ad6953d56` |
| `src/flatfile/flatfile_collector_repository.c` | `05624ed816c315983b7a9f3183526e7b8be18a1c` |
| `src/flatfile/flatfile_corpse_repository.c` | `f6200b77cb6d75b5722dc044a9e0223cd2e357ad` |
| `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` |
| `src/item/item_ownership_runtime.c` | `cf0aae0dfa3ce2d7397d357127ec1b6e3c8074ba` |
| `src/item/item_transfer_command.c` | `637051605ad1b1b27b9254eb510acb347852f5e9` |
| `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| `src/persistence/economic_sql_collector_transaction.c` | `473714372c2f71cf7bb1728441946e0cee44913e` |
| `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| `src/player/player_snapshot_capture.c` | `9aacf75b74b45f1d2f0911e7325b9d7a3125921b` |
| `src/player/player_snapshot_codec.c` | `27d440a90d714fd1cba95720edd9e59eec976844` |
| `src/world/handler.c` | `0054c7db2f7cb990dfde047e6de09250d54670e3` |
| `tests/async/collector_catalog_cache_harness.cpp` | `c0787f2867abc67cf12f7552b9dbd623adda230b` |
| `tests/async/collector_collection_preparation_harness.cpp` | `32902630fca84b9ed580a60fed7a5aa83e9e9926` |
| `tests/async/collector_repository_mysql_harness.cpp` | `9d72600774c8eaac458ec4f35b1c8344b2db34c8` |
| `tests/async/collector_service_harness.cpp` | `b1b3eaa04eb40e4207352b7a82d53999262ab841` |
| `tests/async/run_collector_sql_accounting_mysql.py` | `479f5400acca963a8c7bff9a2901bf49fb4511e3` |
| `tests/async/test_collector_collection_preparation.py` | `775c75d7ba911e26c5a1cf1bcf6bc3e85e10b7f3` |
| `tests/async/test_collector_expiry_preparation.py` | `4dcdba88391aa9cad1ce11bc31ad8bc26bda4f02` |
| `tests/async/test_collector_purchase_preparation.py` | `596a7a6374e8263584d501c50cc55a9ccf2dc72e` |
| `tests/async/test_collector_repository.py` | `68f7d10e566f0c2536d158dd0b6e6e696148424b` |
| `tests/async/test_collector_transaction.py` | `4aa84e0bc7f5130db13737d2426dbd4479f22c17` |

### Owned preparation baseline

| Repository path | Git blob |
| --- | --- |
| `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| `docs/persistence/economy_accounting/domain-separation/POST_R14_OWNER_DEPENDENCIES_2026-10-07.md` | `c32aad66519191ab2530b0f2b1c7e14968822753` |
| `docs/persistence/economy_accounting/domain-separation/HANDOFF.md` | `493abc0e785f339acf687b0bb347f79454bac373` |
| `docs/persistence/economy_accounting/domain-separation/ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `70e8cc751e01bb283629582b7572365059725ccc` |
| `docs/persistence/economy_accounting/domain-separation/CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e` |

### Closed R0

| Repository path | Git blob |
| --- | --- |
| `src/economy/collector_collection_image.h` | `cb3de74a025126f93c9a039b9d6c3facd5774821` |
