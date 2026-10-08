# Ordinary item custody authority boundary — 2026-10-08

This document maps ordinary carried → room → carried movement and one complete
container subtree. It proposes a small future DB-authoritative facade around the
existing preparation, durable transaction and native publication owners. It adds
no callable API, format, storage authority, migration, RAM-authority switch or
implementation. Source inspection is the only new evidence produced here.

## Frozen inputs and limits

Maintained source is pinned to `21d6ec7c94f2f4eb97c59ce3bae383b2d9b653d7`
on `experimental-accounting`. The inspected read-only source export is
`D:\Dev\Temp\currency-authority-20261008\source`, originally exported at
`a6aec4c4a058a5174e5b679708103762fa05e05a`; each cited source/test body is
authenticated against the maintained pin in the appendix. Production, migration
and async-test trees at the maintained pin are respectively
`833d3085815b396861ad18a77635412212381e4b`,
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` and
`790f367adf805a69d53aac6460938f5c921f9136`.

Owned preparation is pinned to `a24454539b3c8dc223f06241efc90104c9573ee0`:
[operation inventory](OPERATION_INVENTORY.md),
[currency authority map](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md) and
[currency acceptance blueprint](CURRENCY_NATIVE_ACCEPTANCE_BLUEPRINT_2026-10-08.md).
I1 already identifies owned transfer payload/snapshot/mutation/result preparation
and remaining live placement/current authority coupling. I2 and R0–R14 remain
their existing owners' work. This document does not repeat those extractions or
the quest/shop quote inventories. Currency and compound coin acceptance remain
in those currency documents.

The [maintained preparation review](https://github.com/Community-Duris/Duris/blob/21d6ec7c94f2f4eb97c59ce3bae383b2d9b653d7/docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md)
records the currency blueprint PASS and this finite documentation assignment.
The [primary checkpoint](https://github.com/Community-Duris/Duris/blob/21d6ec7c94f2f4eb97c59ce3bae383b2d9b653d7/docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md)
reports private warm-forest and SHOP materialization work at
`84a435c3deca43258c0c804ec16cc964197c2aea`. That report is a constraint, not
published implementation or qualification. No private candidate was inspected.
The continuing Plans 1–5/applicable original R1–R8 Goal remains blocked and
unfinished. This preparation does not authorize activation or deployment.

## What owns the facts

| Fact | Existing owner and evidence | Boundary to preserve |
| --- | --- | --- |
| User intent, visibility, take/drop permission, actual location and native effects | `src/cmd/actobj.c:920`, `src/item/item_command_policy.c:10`, `src/item/item_get_policy.c:119` | Native policy chooses a real object; a numeric UID alone does not establish access. Money and player corpses use distinct paths. |
| Prepared immutable transfer | `src/item/item_transfer_command.h:297`, `src/item/item_movement_transaction.c:2599` | Owner identity, selected UID, old root/parent, expected item/owner clocks, target topology, complete literal blob and reason remain bound together. |
| Durable current custody/topology | `src/item/item_transfer_repository.c:2612`, `src/persistence/sql_room_item_payload.c:899` | SQL current rows and owner revisions are authoritative; current proof is taken under the transaction's locks/session. |
| Historical operation and correspondence | `src/persistence/economic_sql_item_transfer_transaction.c:727`, `src/persistence/critical_command_repository.c:3084` | Accounting operation, native ledger/reference, inbox/result and immutable payload authenticate what committed. They do not prove where an item is now. |
| RAM custody projection and actual native object graph | `src/item/item_ownership_runtime.c:295`, `src/item/ordinary_drop_recovery.c:139` | Both must agree with durable current authority. Hydration changes a projection; it cannot authorize a missing item or synthesize a source. |
| Publication obligation, save exclusion and ACK | `src/item/item_movement_transaction.c:1885`, `src/player/player_save_pipeline.c:1835`, `src/persistence/critical_command_coordinator.c:3126` | Original identity and fences survive uncertainty. Durable success, physical publication, journal checkpoint and hold release are separate cuts. |

An owner is the typed `(type,id,context_id)` identity, not a native pointer or an
object VNUM. A room owner uses the virtual room number; a player owner uses PID
and zero context. The selected item may be inside another root: its **old** root
and parent identify the source topology, while target root/parent identify the
new topology. `src/item/item_transfer_command.c:446` and
`src/item/item_transfer_command.c:507` calculate selected-root and target topology;
ordinary drop here is specifically a carried top-level root with no parent.
Selecting a descendant from a container is a different shape, even when all
members retain the same player owner.

The transfer's item clock is not the player's save revision, and neither is an
owner aggregate clock or the accounting epoch/season. The result carries root,
count and resulting revision evidence; taking its maximum revision does not
replace per-item or per-owner comparison. All overflow, binding and retained
result checks remain in their existing owners.

## Complete subtree, not a roots-only inventory

Use a genuine carried bag `R`, its child `C`, nested bag `N` and leaf `L` as a
design example. These are symbolic names for actual captured UIDs, not fixture
IDs or authority to allocate/adopt objects. Before and after a top-level drop:

| Member | Root UID | Parent UID | Before owner | After owner |
| --- | --- | --- | --- | --- |
| R | R | 0 | player PID | room VNUM |
| C | R | R | player PID | room VNUM |
| N | R | R | player PID | room VNUM |
| L | R | N | player PID | room VNUM |

The reciprocal top-level pickup keeps these UIDs and edges and transfers every
member back to the player. It is a new operation with fresh expected clocks;
replaying the drop is not a pickup. No descendant is recreated from a prototype.
Literal state includes the codec's real text, values, flags, condition, weight,
affects and other supported fields, with parent indices bound to the same UID
graph. The four-member example is not a claim that the three-member existing
native journey covers sibling branches.

`src/item/item_movement_transaction.c:315` recursively captures every live child
and checks cached root, parent, VNUM and active state against native nesting.
The resulting entries are sorted by UID; the literal snapshot has its own parent
indices. Equal counts alone are insufficient: UID sets and each root/parent edge
must correspond. SQL selects all current descendants of the selected roots and
requires exact cardinality and member comparisons before mutation at
`src/item/item_transfer_repository.c:2593`. A roots-only payload, missing leaf,
extra child, foreign parent, duplicate UID or cycle must never become a partial
success. The target parent, when present in another movement shape, also has its
own current owner/root/state/revision proof at
`src/item/item_transfer_repository.c:2755`.

Room drop additionally validates the complete existing player projection and
literal rows before retiring that projection. Its guarded retirement proves the
native row/parent set and descendants, not merely deletion of a root and reliance
on SQL cascade: `src/item/item_transfer_repository.c:237`.
`src/persistence/sql_room_item_payload.c:372` compares the captured literal graph
with locked current custody and native player rows; immutable room payload is
recorded in the same transaction at `src/item/item_transfer_repository.c:2831`.
`migrations/immutable/0055_sql_room_item_payload.sql` explicitly leaves custody in
`item_current_owner`; the sidecar is not another current-owner table.

One subtree is the unit selected here. A complete player/keeper/world **forest**
also includes every unrelated root and descendant relevant to its owner cut.
This document grants no shortcut from this selected-subtree proof to a complete
owner forest, and no authority to omit legacy pet or foreign-container facts
when a compound participant's existing contract requires them.

## Current command-to-durable-to-native path

### Ordinary active SQL drop

1. `do_drop` reaches `submit_player_drop` after native command checks; the ordinary
   carried branch uses it at `src/cmd/actobj.c:5281`. Destination resolution
   distinguishes a room drop from locker deposit at
   `src/item/item_command_policy.c:81`. With accounting active, ordinary room
   drop uses `item_movement_transaction_prepare_sql_drop` at
   `src/cmd/actobj.c:937`; it does not use the legacy physical-drop callback.
2. `src/item/item_movement_transaction.c:3068` checks the actual carried shape,
   player/coin/save/coordinator conflicts, owned top-level UID and complete
   literal subtree eligibility. It begins the existing literal-inventory save
   checkpoint. Preparation retains identities/context, not live object pointers
   across pulses. The pulse re-resolves actor/root and room and recaptures before
   admission at `src/item/item_movement_transaction.c:3159`.
3. `src/player/player_save_pipeline.c:1779` requires the same literal bytes,
   captured/acknowledged/current save revision agreement and no dirty, queued,
   inflight, retained or worker snapshot. `:1835` binds the hold to the original
   generated operation ID only after that acknowledged cut. This is the concrete
   save-drain proof; an authority lock alone cannot stand in for it.
4. `src/item/item_movement_transaction.c:2500` reads runtime expectations,
   captures complete owned entries and literal bytes, builds the existing command
   and asks the gameplay authority for the schema-2 intent at `:2659`. Missing
   retained ownership on active ordinary movement is refused; the inactive
   absent-item adoption path is not active source admission. A missing cached
   room clock keeps an optimistic zero expectation at `:2559`; SQL must verify it,
   and this read installs no new cache authority.
5. The immutable original, publication bit and hold are retained before submitting
   for publication at `src/item/item_movement_transaction.c:2666` and `:2723`.
   Journal uncertainty or a thrown admission with uncertain disposition keeps
   the same operation and hold at `:2736`. Cancellation only releases unheld,
   unadmitted preparation; it cannot cancel an admitted uncertain original.
6. The root SQL owner locks accounting authority and disables reconnect semantics
   through `src/persistence/economic_sql_item_transfer_transaction.c:560`.
   `src/persistence/critical_command_repository.c:3009` invokes the existing
   custody writer inside its transaction, then records accounting and references,
   canonical result, outbox and inbox. The writer checks owner clocks, complete
   descendants and per-item expectations, updates current custody and native
   ledger, records room literals, retires the player projection and advances
   owner clocks. The root verifies retained proof/outbox and commits at
   `src/persistence/critical_command_repository.c:3132`. A connection failure
   around COMMIT remains ambiguous; it cannot be converted into a fresh drop.
7. Game-thread completions are routed through `src/net/comm.c:293`.
   `src/item/item_movement_transaction.c:1885` invokes the ordinary live publisher
   with the original command/receipt. The publisher proves current SQL and the
   complete actual graph, executes `obj_from_char` then `obj_to_room` once, tracks
   started/returned boundaries and atomically hydrates the projected custody.
   See `src/item/ordinary_drop_recovery.c:624`. A handler that started without
   returning cannot be called again as though no effect occurred. A completed
   placement followed by hydration failure retries projection proof, not placement.
8. After physical proof, the critical journal ACK must succeed, then the original
   literal hold must release. Only then is `sql_drop_notification` called; it
   reports the drop and never places/hydrates again:
   `src/cmd/actobj.c:646`. A failed ACK or release keeps the owned obligation.

### Room pickup and the ordinary compatibility path

`src/cmd/actobj.c:1094` resolves the real source owner and submits a durable
`player_get` with original UID/container/room context and `item_get_completion`.
`src/item/item_get_policy.c:119` checks retained ownership against live placement;
its live fallback is not permission to adopt an active unowned item.
`src/economy/item_transfer_accounting.c:17` accepts defined already-owned ordinary
get/drop shapes and excludes coin members. Therefore active owned pickup is not
categorically unsupported. The general builder still must obtain valid active
intent and backend admission; a numeric UID or a room reset alone cannot supply it.

On a successful SQL get, the same custody writer advances the whole subtree and
`materialize_direct_player_items` writes the player projection from the existing
blob (`src/item/item_transfer_repository.c:778`, `:2839`). Old room payload history
is not a second live copy: its revision no longer matches current room custody.
The reciprocal operation must retain the original drop receipt and its history.

The current ordinary get call supplies a completion callback, **no retained
publication callback**. The common completion path applies the runtime registry
at `src/item/item_movement_transaction.c:2141` and normally erases the pending
movement before invoking its callback at `:2414`. The callback re-resolves the
item and checks its source, then calls the native publication phase at
`src/cmd/actobj.c:519`. Room placement uses the actual `obj_from_room` and checked
`obj_to_char` path at `src/cmd/actobj.c:1390`, with native messages/light/dirty state.
Its alert on stale live topology is not equivalent to the ordinary SQL drop's
typed retained current-graph/ACK owner. Do not describe this callback as providing
that proof, or silently change it in this documentation task.

Inactive ordinary drop also uses the common callback and
`publish_player_drop` (`src/cmd/actobj.c:581`). Inactive unowned capture may build
creation and then continue movement; active unowned requests are refused at
`src/item/item_movement_transaction.c:2537`. These compatibility behaviors must
not be relabeled qualified active round-trip acceptance.

### Flat-file backend

`src/economy/economic_command_admission.c:163` permits supported item-transfer
accounting commands while explicitly refusing native quest transport. This is
different from central coin admission; use the currency map for that boundary.
The existing `src/flatfile/flatfile_item_repository.c:3471` acquires the authority
lock, recovers its transaction, reads the catalog and verifies operation identity
and digest on replay. It prepares complete ownership, room/artifact and player
materialization after-images (`:3845`, `:3863`), then stages accounting, source
claim and references into the same authority transaction (`:3946`).

Preserve that existing root; do not introduce a second item writer or commit
accounting separately from physical materialization. Backend transaction support
does not qualify native publication/restart parity. The SQL ordinary-drop
checkpoint and recovery entry points explicitly refuse `__NO_MYSQL__`; the
restored save owner admits only the distinct typed coin scope on that build at
`src/player/player_save_pipeline.c:3216`. An item facade cannot claim flat parity
by exposing the SQL drop path or passing repository-only tests.

## Recovery and save nonoverwrite are separate obligations

Historical proof answers “did this exact operation commit with these bytes and
effects?” `src/persistence/economic_sql_item_transfer_transaction.c:874` requires
the original immutable ordinary-drop payload and ledger proof, without
manufacturing it from current rows. A valid receipt can survive subsequent
pickup, owner-clock advancement, epoch changes or other legitimate moves.
It does not authorize republishing the old drop into the room.

Current physical proof answers “does the entire currently authoritative graph
exist exactly once, at these roots/parents/owners and revisions?”
`src/persistence/sql_room_item_payload.c:899` reads current season/room/custody and
joins payload at the **current item revision**. Its LEFT JOIN preserves a missing
descendant as a refusal; the provenance query binds the original operation,
accounting/native references and ledger and rejects competing legacy saved rows.
`src/item/ordinary_drop_recovery.c:521` then compares fresh retained receipt,
current SQL graph and complete native graph. The native census at `:139` includes
global objects, all room/container links and foreign parents; partial presence,
duplicates, extra children, wrong placement or an exhausted census cannot prove
absence. Room owner revision may legitimately exceed the original result; the
publisher hydrates its current value while preserving immutable command/result.

Before-ACK restart is owned by
`src/item/item_movement_transaction.c:4354`: it restores the original SQL drop
obligation and save hold. `src/player/player_save_pipeline.c:3183` reserves the
original execution/save ownership, drains covered ordinary frames, performs
fresh graph proof and calls the typed coordinator ACK. ID-only ACK explicitly
refuses guarded publication holds at
`src/persistence/critical_command_coordinator.c:3126`. Rejected retained drop
uses source proof and exact rejection receipt; it manufactures no destination
payload (`src/item/ordinary_drop_recovery.c:466`). Unavailable reads/allocation,
session changes or unconfirmed transaction cleanup leave the obligation owned
(`src/item/ordinary_drop_recovery.c:959`).

After-ACK full cold SQL boot is a different path:
`src/sql/sql_player.c:11946` enumerates current exact room roots, reads the complete
current graph and publishes supported staged objects. The critical journal can
already be empty. Keep this path separate from replaying an outstanding command;
do not require the historical epoch to be currently active just to authenticate
retained evidence, and do not use old history when current custody has moved on.

Ordinary saves are projections, not item custody mutations.
`src/player/player_snapshot_repository.c:811` locks active player custody,
requires UID/VNUM membership correspondence and validates/reconciles authoritative
root/parent/slot topology. `:1578` runs that guard before destructive projection;
valid topology reconciliation uses durable custody, while missing members or
payload conflicts fail closed. Partial legacy component frames have their own
protected plan; they must not delete unrelated equipment or foreign custody.
These guards do not eliminate the need for admission save-drain/holds: queued
stale payload, publication and relogin must be qualified together. Preserve the
existing inline coin exception and separate pet/death contracts.

## Smallest proposed future facade

The facade is an ownership boundary inside the current movement pipeline, not a
new service, generic repository framework, command type or selectable authority.
Existing command policy and owned preparation remain its input; the existing
SQL and flat-file root transactions remain its only durable mutation paths.
Implementation requires a separately authorized reservation.

| Boundary | Required future responsibility | Reuse; do not replace |
| --- | --- | --- |
| Prepare | Capture the complete selected subtree/literal state and actual native participant identities; reject unsupported shapes before admission. | Existing command policies, snapshot codec, transfer builder and ordinary-drop eligibility/checkpoint. |
| Admit | Validate DB-authoritative current owner/topology/clocks and existing active intent; bind original ID and save/publication fences. Cached expectations may cause refusal/retry, never supply missing authority. | Existing gameplay authority, coordinator and original save owner; no UID-based implicit creation. |
| Commit | Execute one current-custody transition with complete materialization, accounting/native correspondence, immutable result and replay checks. | Existing SQL root and flat authority transaction, original writers and codecs. |
| Publish | Use the original owner's retained proof to reconcile current durable graph, native graph and runtime projection; own native handlers once through their returned boundaries. | Ordinary SQL drop's typed publisher; general pickup needs an explicitly owned equivalent contract before claiming symmetry. |
| Complete/recover | ACK only after current physical proof and clean transaction observation, then release the original hold; recover the same original across crashes. | Coordinator journal checkpoint, restored save owner and current room/player boot/load paths. |

Keep the facade narrow enough that swapping a storage adapter cannot change
native gameplay selection, messages, capacity/weight/lighting, root/parent
semantics, dirty flags or recovery obligations. Return existing owned outcomes
and receipts; a successful durable result is not “the item was delivered.”
No wrapper should expose a caller-set “published” flag, ACK-by-ID capability,
unchecked cached-owner authorization or save-guard bypass.

A single move consumes one complete selected subtree. A purchase, reward,
craft, quest exchange or item-plus-money action belongs to its existing compound
owner and transaction. The facade may contribute its prepared custody participant
under that owner; it must not submit independent money/item commands or ACK one
participant early. Do not copy quest preparation or SHOP materialization into
this boundary. A container containing money is also outside the narrow ordinary
SQL drop literal contract (`src/persistence/sql_room_item_payload.c:36`);
rejecting it is not permission to move the bag and leave its coins behind.

## Existing tests: meaningful evidence and exact limits

No test, build, server, migration or database operation was run for this document.
The following are inspected assertions and runner contracts, not new PASS claims.

| Existing evidence | What its actual checks protect | What remains unproven here |
| --- | --- | --- |
| `tests/async/item_transfer_accounting_test.cpp:111` and `tests/async/test_item_transfer_accounting.py` | Typed ordinary movement intent, supported owner/reason binding, existing plan construction under compiler/sanitizer harness. | Native source capture, active admission, graph placement and save/restart behavior. |
| `tests/async/test_item_ownership_runtime.py:51` | Runtime apply refuses conflicting multi-item changes atomically; reload/owner-clock hydration and later multi-owner checks preserve projection invariants. | Durable DB authority or global native graph proof; its embedded harness is not a server. |
| `tests/async/item_transfer_mysql_harness.cpp:919` | Real SQL nested get/put, exact old/new parent/root/revision and native accounting references, replay; broader harness checks root rollback and writer fence at `:698`. | The nested get example selects one child. SQL dispatch does not exercise the native pickup callback or complete active player round trip. |
| `tests/async/sql_room_item_payload_test.cpp:90` | Complete three-member literal capture; wrong parent, duplicate UID, malformed blob, unsupported money/corpse/artifact/equipment and multi-root shapes refuse without changing output at `:106`. | Locked current SQL, native command and global object-list enrollment. |
| `tests/async/sql_room_item_payload_mysql_harness.cpp:1204` | Original native player rows/UIDs, exact source, rollback faults, schema-2 sidecar/reference/receipt, retained verification, current graph provenance/season refusals; final declared scope at `:1423` explicitly excludes gameplay publication. | Two cold SQL connections are not two server boots. Retained restart/ACK checks do not establish full native source-to-publication parity. |
| `tests/async/test_sql_room_item_publication_runtime.py:49` | Real room placement helper preserves literals and sibling links, rejects unsupported placement, and checks UID clearing. | A small injected graph/room harness does not prove complete production world census or active authority. |
| `tests/async/test_player_item_custody_write_guard.py:65` | Source contracts put custody reconciliation/orphan protection before destructive saves and preserve native recapture diagnostics. | Text/order checks are not an executable stale queued-save journey. |
| `tests/async/test_item_movement_prompt_runtime.py:383` | Embedded movement/output harness compares synchronous/deferred native prompt/message behavior, including conflicts and WebSocket/Telnet forms. | Accounting is explicitly inactive in the fixture; it stubs coordinator/publication/authority boundaries. |
| `tests/async/test_live_item_movement_contract.py:21` | Source contracts inspect pointer-free pending state, builder ordering, fences and native callback routes. | Presence/order assertions cannot authenticate physical publication or close the pickup retention gap. |
| `tests/async/run_sql_room_item_payload_recovery_journey.py:325` | Optional active-drop producer uses real created character, complete-world private O/P reset, inactive nested get/save, guarded stopped-fixture setup, literal checkpoint, real active drop, original journal ACK and two full SQL cold boots with exact three UIDs/edges/literals. | It qualifies a disposable synthetic route, explicitly `global_route_qualification=False` at `:546`; initial get is inactive. It is not active reciprocal pickup, sibling forest, private warm producer, global activation or all R1–R8. |

The same journey's default mode starts from supplied hash-pinned ACK/export seed
and a complete native observer. Preserve the distinction between seeded
after-ACK cold recovery and its optional actual drop producer. Neither mode
permits replacing native observed fields with expected JSON. The existing
universal-reference test is narrower still: it indexes an accounting reference;
its title alone must not be cited as complete item-domain acceptance.

## Owner inputs, collisions and prioritized reservation

Available maintained inputs are the actual ordinary get/drop producers,
complete-subtree transfer/literal preparation, guarded ordinary SQL drop owner,
both durable backend roots, current player/room codecs and the focused runner
above. Reuse them. Existing native drop evidence can establish a baseline when
the owner supplies its authenticated binaries/world/fixture; it does not supply
the missing current pickup publication owner.

The primary checkpoint reports warm capture of genuine O roots/P children and
complete selected ownership wrappers, but actual cursor/S boundary, P-before-O,
aggregate coordinator reservation, current season/room SQL cut, admission/adoption
and real comm/publication/recovery remain missing. Active O refusal stays. A
successful manually prepared room fixture cannot substitute for those producer
inputs or authorize adoption. SHOP's complete player/keeper forest participant,
canonical typed root/receipt, hold/save-drain, native publication and ACK remain
owner work. Keeper wallet/treasury classification and authenticated current cash
image/history transition remain blocked; do not infer classification from VNUM
or shop tag or rewrite postings. These are reported constraints only.

Likely collisions are the shared `item_movement_transaction.c`,
`player_save_pipeline.c`, `critical_command_repository.c`,
`economic_sql_item_transfer_transaction.c` and the player/room codecs and flat
materialization/root owners. Source line numbers here are frozen, not an edit
reservation. Quest's separate cost oracle/module/tests and all quest-preparation
paths are excluded. Agree actual ownership and candidate base before any source
reservation; do not cherry-pick private excerpts into this owned branch.

1. **First useful reservation, conditional on owner inputs:** one already-owned,
   eligible ordinary SQL bag subtree, active carried → room drop → carried pickup.
   Reserve the smallest integration at the existing pickup publication/completion
   boundary, with the original save owner and transaction owner, to give pickup
   an authenticated current graph and retained publication/ACK contract. Preserve
   the already-qualified drop path. Identify the exact original player load/save
   source and current room graph owner before coding; no general facade extraction
   or fresh runner is useful while these ownership inputs are missing.
2. Extend the existing native room-item journey when that contract exists. Keep
   real inactive acquisition/guarded disposable setup explicitly labeled, then
   witness both active operations with fresh expected clocks. Add a real sibling
   branch alongside the nested bag/leaf. Check the complete UID set/edges/literals,
   one owner/location per UID, unchanged unrelated player roots, exact references,
   one original operation per move, final player projection, save/logout/relogin
   and two cold boots with Redis disabled. Observe native graph and durable state
   independently. The first acceptance claim remains this selected route.
3. At the owned boundaries inject loss after durable commit/before publication,
   after native departure, after placement/before hydration, and after physical
   proof/before ACK. Include a definitive rejection, uncertain COMMIT and a
   contradictory receipt. Restart/retry must keep the same original ID and bytes,
   avoid duplicate handler calls and retain fences on unavailable/conflicting
   proof. Queue a stale save before admission and another during held publication;
   verify no resurrected room copy or overwritten player custody after relogin.
4. Negative current-graph controls must independently alter/remove one descendant,
   add a foreign child/duplicate native UID, change a parent or expected revision,
   move current custody after a historically valid receipt, remove provenance,
   advance season, and force read/cleanup/allocation unavailability. Refuse the
   complete move/publication; preserve history and original recovery ownership.
   Do not demand that an old drop republish after a legitimate later pickup.
5. Reserve flat native publication/restart parity separately only after its real
   owner and save participant are supplied. Reuse the flat atomic root and run
   the equivalent native journeys/fault cuts. Compound item/money or quest/shop
   integration follows under its existing atomic owner after their inputs close;
   it is not part of this first ordinary-move reservation.

Future scratch/evidence belongs in a distinct task directory on D:, build outputs
under the project's task-specific D: BIN_ROOT, with direct D: Docker binds when
needed. This document created no runner, runtime fixture, candidate binary or
qualification packet. The appendix authenticates preparation inputs only.

## Exact inspected file pins

`maintained` means the frozen primary revision above; `owned` means the frozen
preparation baseline above. Every explicit source line and document link was
checked against its exact body. Test assertions were read, not executed.

<!-- BLOB_PINS -->

| Revision | Inspected file | Exact Git blob |
| --- | --- | --- |
| maintained | `docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md` | `cd159b9ed6741b2c06cc135bdc33abd25e128866` |
| owned | `docs/persistence/economy_accounting/domain-separation/CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e` |
| owned | `docs/persistence/economy_accounting/domain-separation/CURRENCY_NATIVE_ACCEPTANCE_BLUEPRINT_2026-10-08.md` | `15bcec8b8631c46b89f1cc07790f71a2857041bb` |
| owned | `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| maintained | `docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md` | `8ea3791950678202948ce0a4d744ac10fd90d24d` |
| maintained | `migrations/immutable/0055_sql_room_item_payload.sql` | `3019661c83fff580ed5eb341d29a2488ae826f13` |
| maintained | `src/cmd/actobj.c` | `a2114fddb6816f1534488ff11457a1001d47a14c` |
| maintained | `src/economy/economic_command_admission.c` | `a3e5244e30144a8dd0dff8485abe130e64045ffb` |
| maintained | `src/economy/item_transfer_accounting.c` | `b7012f63c3ce96573eaf23c0424d6447f71e74ac` |
| maintained | `src/flatfile/flatfile_item_repository.c` | `14f21c674e6226bd7e66bce57461f29d06367583` |
| maintained | `src/item/item_command_policy.c` | `4b7b98ffcb697d10752cbc051342cf5432d0510c` |
| maintained | `src/item/item_get_policy.c` | `159d27f43ff731c92becfd2c69b0dccfe35cbdb1` |
| maintained | `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` |
| maintained | `src/item/item_ownership_runtime.c` | `cf0aae0dfa3ce2d7397d357127ec1b6e3c8074ba` |
| maintained | `src/item/item_transfer_command.c` | `637051605ad1b1b27b9254eb510acb347852f5e9` |
| maintained | `src/item/item_transfer_command.h` | `f1d8a67d565133eeb45b50135dd82af82f50c439` |
| maintained | `src/item/item_transfer_repository.c` | `3476ec8518eaeb743ec90ecff93c10de7fe18923` |
| maintained | `src/item/ordinary_drop_recovery.c` | `44c5a772e5d89702e071b5425400b66d9cce8785` |
| maintained | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| maintained | `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| maintained | `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| maintained | `src/persistence/economic_sql_item_transfer_transaction.c` | `2da2b2b11979424e339389db89e9b7b6a981056c` |
| maintained | `src/persistence/sql_room_item_payload.c` | `9de8ded54b76440d38ac08c5d933a0fb62911fa7` |
| maintained | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| maintained | `src/player/player_snapshot_repository.c` | `21c7108c041613b2b76d1b305b1fe5d842bb360d` |
| maintained | `src/sql/sql_player.c` | `57a536a3a192b5432b17fd7ac088b76f7559b41e` |
| maintained | `tests/async/item_transfer_accounting_test.cpp` | `0757fc00cb7f8c41454b7f182bcbebf15f610c19` |
| maintained | `tests/async/item_transfer_mysql_harness.cpp` | `a82364b596e562e7ccc36fff43097fe20aad33b6` |
| maintained | `tests/async/run_sql_room_item_payload_recovery_journey.py` | `2327a7ba985e5d71d4abc804597b2e39434a2764` |
| maintained | `tests/async/sql_room_item_payload_mysql_harness.cpp` | `6846713c0d977dc79356db8c0e33ef98a6fb4afa` |
| maintained | `tests/async/sql_room_item_payload_test.cpp` | `ba7f072bfb4dc4627d12668c38677daa3a9e309c` |
| maintained | `tests/async/test_item_movement_prompt_runtime.py` | `dc801e619534c6dabe745fb3455d5d986ac6eb0e` |
| maintained | `tests/async/test_item_ownership_runtime.py` | `d3da99ccce6449fc315ee5c77cfded818e90f345` |
| maintained | `tests/async/test_item_transfer_accounting.py` | `1915d06d49dbf81f0cdf087e9c2d7c17ac9e9dc7` |
| maintained | `tests/async/test_live_item_movement_contract.py` | `772b3a2dd80d4b4e8ad95f42410cd016ac41c79b` |
| maintained | `tests/async/test_player_item_custody_write_guard.py` | `25d9abeb304194498ddefd453f592026bbcea6e4` |
| maintained | `tests/async/test_sql_room_item_publication_runtime.py` | `fa4ed5f626311946bf91c9cea8c0cd4991fb5c66` |
