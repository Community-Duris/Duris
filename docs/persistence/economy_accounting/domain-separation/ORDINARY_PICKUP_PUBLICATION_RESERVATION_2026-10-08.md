# Ordinary pickup publication investigation and reservation — 2026-10-08

## Disposition

The complete ordinary single floor-item path has a narrower publication gap than
“the callback can fail, therefore the item is lost.” Missing actors and runtime
registry failures retain movement state; readiness can resume it. SQL commits a
player materialization with current custody, and an authoritative load can rebuild
that player inventory. Those protections are real. They do not establish that the
original live pickup callback completed or retain its original native continuation
after the common path erases it.

For a nonretained ordinary pickup, the coordinator checkpoints the critical
journal after a definitive durable result, then retires coordinator fences before
gameplay completion. The movement owner applies its registry and normally erases
pending state before invoking `item_get_completion`. That callback returns void;
stale source placement or failed native placement reports an alert without a
return channel that preserves this movement. Exceptions escaping the callback
also occur after erasure. This is a source-backed ownership finding, not a newly
executed active SQL/native failure or a claim of irrecoverable durable data loss.

The proposed reservation is one **already-owned, eligible, ordinary room-to-player
pickup continuation**, including its save and restart contracts. Setting a
publication flag or replacing a void callback with a caller-set success boolean
is insufficient. Production implementation remains conditional on the owner
inputs listed below and separate review of this exact boundary.

## Frozen scope and inputs

Maintained primary is `4357367798ac045879081a8bdcf861a4699a7ce4` on
`experimental-accounting`; source, tests/async and migrations trees remain
`833d3085815b396861ad18a77635412212381e4b`,
`790f367adf805a69d53aac6460938f5c921f9136` and
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` respectively. Inspected/compiler
source is the read-only export at
`D:\Dev\Temp\currency-authority-20261008\source`. Its cited bodies are checked
against the maintained revision, not the different R0–R14 source in this owned
worktree. Line numbers refer to that exact maintained body.
The preserved source-export archive SHA256 is
`38faf6c8ddf050de9008af2b5315ea040ab5d6fe763d5f63c4066f012c79ef51`;
the appendix independently authenticates the actual cited and compiled bodies.

Owned baseline is `f4d38f4eb047f29eb01187bd84f708f46c3b6928`. Preserve the accepted
[item-custody map](ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md),
[operation inventory](OPERATION_INVENTORY.md) and completed currency/R0–R14 bundles.
The [primary reservation assignment](https://github.com/Community-Duris/Duris/blob/4357367798ac045879081a8bdcf861a4699a7ce4/docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md)
approves investigation and private component feasibility only. No maintained
code, test, driver, authority, schema or observer was changed here.

One single selected ordinary item is the component control. The later native
acceptance unit includes a complete bag subtree; complete owner forests,
compound item/money, locker/corpse, adoption and quest/SHOP remain their existing
owners. No new source-admission authority or format is proposed.

## Complete producer and ownership trace

### Selection and preparation

`src/cmd/actobj.c:2593` parses the native get command. Single floor selection at
`:2782` uses actual visibility, count/weight and takeability checks, then calls
`do_get_finalize_room_item` at `:2856`. That helper and
`do_get_finalize_pickup_core` reach `get_with_phase` with admission phase
(`src/cmd/actobj.c:1425`, `:1892`). This is distinct from `get all`/`all.filter`,
which uses the bulk selection state and batch completion at `:2325`.

`src/item/item_command_policy.c:10` chooses generic durable ownership for eligible
UID-bearing objects; money and player-corpse objects are excluded. Admission at
`src/cmd/actobj.c:1094` resolves source owner through
`src/item/item_get_policy.c:119`. It selects `player_get`, except for a locker
source or player-corpse loot. The ordinary single call at `src/cmd/actobj.c:1114`
passes `item_get_completion`, UID/container/room/show context and **no publication
callback**. The optional argument defaults to null in
`src/item/item_movement_transaction.h:119`.

`src/item/item_movement_transaction.c:2781` forwards that null unchanged to the
common submit owner. Its actual source capture checks retained owner, native
root/parent/VNUM/state, complete recursive descendants, expected item and owner
clocks and literal snapshot, then builds the existing transfer. Active unowned
ordinary movement is refused at `:2537`; inactive absent-item adoption is a
separate creation/continuation. For an already-owned ordinary move, active intent
may be prepared at `:2659` and `src/economy/economic_gameplay_authority.c:919`.
Restricted SQL wallet-root or recovery scope can still refuse it. No authority
installer or active fixture is supplied by the callback.

The builder initializes the legacy scaffold at
`src/item/item_transfer_command.c:2731`, without a publication flag. Gameplay
authority freezes schema-2 intent when genuinely active; it does not silently
make ordinary pickup retained. The common owner chooses submission from the
presence of the publication callback at
`src/item/item_movement_transaction.c:2729`: null chooses ordinary coordinator
submit, nonnull chooses submit-for-publication. The literal-token specialization
and explicitly set original publication bit in that function belong to ordinary
SQL **drop**, not pickup.

`src/persistence/critical_command_coordinator.c:1698` binds the submission policy
before journaling. A schema-2 command cannot downgrade a preexisting publication
hold, but ordinary submit sets its flag false; submit-for-publication sets it
true. The two public entry points at `:3034` differ exactly in that internal
argument. There is no automatic item-type or `player_get` upgrade here.

### Durable execution, checkpoint and fence retirement

The existing SQL root and custody writer remain authoritative. They compare
complete selected descendants/current revisions, mutate custody and ledger,
materialize player items and record accounting/native references, original result,
inbox/outbox in one transaction. The accepted map gives the full root trace;
pickup materialization is specifically
`src/item/item_transfer_repository.c:778`. Its blob carries selected UID topology,
and native rows are inserted/updated under the existing transaction, with checks
against extra descendants at `:883`. It is not an asynchronous ordinary save.

After dispatch, the coordinator worker at
`src/persistence/critical_command_coordinator.c:1427` checkpoints nonretained
applied/already-applied/terminal outcomes **before queuing their completion**.
A failed checkpoint converts the effective outcome to retryable failure. The
game-thread coordinator pulse at `:4320` keeps retained definitive outcomes in
publication-pending state, releasing execution keys but preserving fences.
For nonretained definitive outcomes it releases keys, removes fences, remembers
completion and erases the operation at `:4378`. These are different cuts:
execution-key release permits dispatch ordering; fence retirement and critical
journal checkpoint retire original recovery/publication ownership.

`src/net/comm.c:2389` calls the coordinator pulse and then routes the resulting
batch through the real gameplay completion dispatcher at `:293`. The coordinator
does not call the native pickup callback inside its worker or before retirement.
Ordinary pickup performs no later explicit publication ACK. Its earlier worker
checkpoint must not be described as an ACK after physical pickup.

### Actor resolution, registry and callback

`src/item/item_movement_transaction.c:4239` validates incoming dispositions and
records completion. The ordinary actor is resolved by PID from `character_list`
using `:559`, not by the original runtime ID. A linkdead body still in that list
can be selected; a later body of the same PID can be selected too. The special
destination fallback named `retained_player_transfer_reason` covers only
`soulbind` and `slip` (`:926`), not ordinary get. Craft and typed ordinary drop
have different runtime-identity/publication rules.

If there is no ordinary actor, `:4329` leaves the movement entry pending without
calling `publish`. It retains the original movement ID and completion in RAM;
coordinator fences and the nonretained journal have already retired. Empty-pulse
retry selection at `:4159` requires a retained publication callback or craft, so
it does not automatically retry this ordinary entry when a player merely appears.
`item_movement_transaction_player_ready` at `:4567` can resume it. Actual login
and reconnect call that hook (`src/account/nanny.c:1863`,
`src/account/account.c:314`). It would be incorrect to claim missing-actor
completion always erases pending work or has no readiness path.

When an actor is present, `publish` decodes the result and applies the runtime
registry at `src/item/item_movement_transaction.c:2141`. A registry failure keeps
movement pending; the stale-registry control in
`tests/async/test_item_movement_input_queue.py:940` explicitly checks this busy
state and withheld callback. Those domain/input holds are not the already-retired
coordinator fences. Ordinary success then reaches the nonretained tail at
`src/item/item_movement_transaction.c:2414`, which normally erases its pending
entry before invoking the completion at `:2430`. Retained callbacks, craft,
creation grants and trusted steal have their own branches; they are not ordinary
get protection.

The unchanged `src/cmd/actobj.c:519` callback decodes its context, re-finds UID and
optional container, and checks actual source placement. On missing/stale source
it alerts and returns void. Otherwise it invokes `get_with_phase` with publication
phase, requiring a `placed` outcome; another outcome alerts and returns void.
No return result reaches the movement tail. The callback contains no exception
catch; the common tail has no catch around its invocation. An exception injected
at a dependency can escape this component after pending erasure; that observation
does not assert how a full server would handle every C++ exception.

### Actual native handlers and side effects

Publication phase at `src/cmd/actobj.c:957` bypasses admission by jumping to the
native placement section. An ordinary floor object uses `:1381`: remove the
Redis floor hint, mark player components dirty, `obj_from_room`, user/room text,
then checked `obj_to_char`, followed by lighting updates. It does not redo the
durable transfer or reprice/reselect the item.

`src/world/handler.c:2929` validates actual room membership before unlinking,
updates room/world activity and sets NOWHERE. `:1856` provides checked placement:
it requires a suitable actor/object and NOWHERE, includes training-dummy refusal
and crumble-loot destruction, checks the projected active player ownership at
`:1917`, then links into carrying, updates weight/count/light/activity/generated
key and marks dirty at `:1973`. These branches make “callback was called” weaker
than “the complete native graph was delivered.” The first eligible reservation
must exclude destructive/special item behavior until its original owner supplies
the corresponding after-state contract; it must not suppress those native effects
globally or report destroyed/deferred as successful delivery.

For a complete bag, every child UID/edge/literal and unique native link must be
proved; checking only the bag's `loc` is insufficient. The accepted map already
specifies this full-subtree correspondence and its current-vs-historical proof.

## Other real owners: protection and limits

| Existing path | Actual protection | Why it does not close this reservation |
| --- | --- | --- |
| Missing actor / stale registry | Pending movement and completion remain; readiness retries; movement busy state gates dependent commands. | The nonretained coordinator owner and journal can already be retired. A stale native callback after successful registry apply has a different outcome. |
| Input and prompts | `src/net/comm.c:1520` gates dequeue using movement/bulk busy; `src/cmd/interp.c:1414` includes get/quit/rent/inventory; prompts defer at `src/net/comm.c:4639`. | This is game-thread input policy, not save-worker exclusion or durable original continuation. SAVE is not in the item-dependent command list. |
| Ordinary saves | `src/player/player_snapshot_repository.c:811` locks active custody and reconciles topology; membership/payload conflicts refuse before destructive projection at `:1578`. | A stale snapshot missing a committed pickup cannot rewrite custody. Refusal/recapture does not execute the original native handler or prove no stale queued save was admitted. |
| Save ownership guard | `src/player/player_save_worker.c:326` acquires resident ownership and permit; `src/player/player_save_execution_guard.h:121` honors real holds. Checkpoint admission at `src/player/player_save_pipeline.c:1476` checks creation ownership explicitly and the literal profile at `:1507`. | A hold must actually be installed and bound by its owner. Ordinary get does not call the drop/SHOP checkpoint paths that do so. A mutex/authority lock or busy prompt cannot substitute. |
| Player load | `src/player/player_load_repository.c:1284` reads native payload/custody; topology reconciliation at `:1501` uses durable placement, counts missing payload for explicit repair at `:1529`. `src/player/player_load_items.c:917` materializes the graph and hydrates ownership. | This is a real durable recovery route. It does not prove unique coexistence with an old same-process floor graph, original callback-once effects or replay/ACK ownership. Do not claim permanent item loss from callback failure. |
| Reconnect preflight | `src/account/account.c:270` discards stale bodies for collector/corpse save fences, then calls readiness. | Its `save_fenced` lambda names those two owners; it does not automatically recognize an ordinary get publication hold. PID readiness is not original body authentication. |
| Before-checkpoint restart | Coordinator journal replay at `src/persistence/critical_command_coordinator.c:668` restores the original command/policy; repository exact-ID replay and authoritative load can reconcile a committed result. | `src/item/item_movement_transaction.c:4354` returns immediately for non-publication commands. It does not reconstruct an ordinary native callback/context. |
| Retained item replay | Existing restore at `src/item/item_movement_transaction.c:4499` retains originals for supported player-source publication. | Both restore entry points require `from_owner.type == player`. Simply retaining a room-source pickup would fail that restoration contract. The default unresolved callback also refuses committed physical completion. |
| After-checkpoint cold boot | Current room payload restoration at `src/sql/sql_player.c:11946` and current player load follow durable custody. | Historical drop receipts must not republish a picked-up item; cold boot recovery is distinct from completing the live pickup's messages/native effects. |

Uncertainty is also distinct. The coordinator retries ambiguous/retryable
outcomes with the same original, then retains exhausted work blocked with fences
and journal at `src/persistence/critical_command_coordinator.c:624`. The movement
publisher's early uncertainty retention at
`src/item/item_movement_transaction.c:1983` applies only to its retained callback
or craft predicate. The ordinary nonretained tail can instead notify its callback
as noncommitted and erase movement state, while coordinator uncertainty remains
owned. Do not infer that such notification proves SQL rejection or permits a new
operation ID. This path is source-inspected, not an executed active ambiguity
reproduction in this investigation.

## Distinct cases excluded from the first reservation

- **Locker/corpse:** the single get call chooses `locker_withdraw` or `corpse_loot`
  from the resolved source, with native container/corpse revision and persistence
  effects. Taking a player-corpse object itself is excluded by generic durable
  policy. Neither can be silently folded into a room-source pickup owner.
- **Adoption:** inactive absent capture can first create/adopt and continue a move
  (`src/item/item_movement_transaction.c:2352`); active ordinary missing ownership
  refuses. A world reset/allocated UID is not source authority.
- **Trusted steal:** the common tail explicitly retains its second native boundary
  and checks delivery at `src/item/item_movement_transaction.c:2418`. It is a
  separate reason/context and does not protect ordinary get.
- **Bulk get:** the native batch callback retains additional bulk state and mixed
  synchronous/coin handling. Its real `bulk_get_completion` path and its focused
  test are separate controls; this reservation starts with the single floor call.
- **Quest/SHOP/compound:** their typed command, current participant, hold,
  publication and recovery owners remain separate. A transfer participant must
  stay inside their original atomic root. No quest-preparation path, cost oracle,
  private SHOP participant or generic item/money decomposition is reserved here.

## Private component feasibility and bounded evidence

Private scratch is `D:\Dev\Temp\ordinary-pickup-reservation-20261008` and outputs
are `D:\Dev\Builds\Duris\ordinary-pickup-reservation-20261008\bin`.
`prepare_component.py` extracts the unchanged actual `find_live_item_uid` and
`item_get_completion` bodies with the existing `_paths.py` extractor. It reuses
dependencies from `tests/async/test_publication_retention_runtime.py` and compiles
21 unchanged maintained providers, including the full movement, actual runtime
registry, coordinator, critical journal and character-identity providers. The
existing harness's registry lookup/revision/apply doubles are removed.

This is explicitly schema 1, accounting inactive and `__NO_MYSQL__`. The authority
double returns inactive; it never accepts an active intent. The apply callback,
snapshot capture and native `get_with_phase` are disclosed doubles. Native
placement is a small controlled effect, not production `obj_from_room`/
`obj_to_char_checked`; logging, dirty-state and unrelated craft/collector helpers
retain the existing focused harness doubles. Corpse/NPC-only callback dependencies
abort if reached. Additional link-only drop, restored-save, held-retirement,
lockpick publication and literal-capture dependencies also abort if reached; they
cannot manufacture a successful owner response. Four unchanged codec/validation
providers resolve shared quest-cost, quest-money, SHOP forest and lockpick payload
symbols; those command variants are not exercised. The real CLI/parser, native
visibility/capacity, SQL/root
transaction, player save/load workers and complete world are not executed.
Submission directly supplies the same ordinary reason/context/callback shape;
it is not actual `do_get` execution.

The complete double inventory is retained in the generated private source:

| Bindings | Component behavior and limit |
| --- | --- |
| `economic_gameplay_authority::active`, `prepare_item_transfer` | Inactive boolean; the reused preparation double is craft-only and never reached. No active acceptance. |
| `apply_transfer` | Decode the real command; encode a fixed result with expected owner clocks +1 and item revision 2. Its selected outcome is controlled. No SQL, accounting execution or canonical rejection receipt. |
| `player_item_snapshot_tree_capture` | Capture one synthetic literal entry from UID/VNUM; no native subtree/forest capture. |
| `get_with_phase` | Count calls, optionally return rejected or throw before/after a small carried-link effect. No actual native handlers, text, weight, lights or capacity. |
| `__malloc`, `__free`, `str_dup`, `str_free` | Focused harness heap/string wrappers; no production allocation policy. |
| `obj_to_obj`, `obj_to_char`, `obj_to_room`, `obj_from_char`, `extract_obj` | Reused controlled/no-op native dependencies for other movement branches; not this callback's production native placement. |
| `send_to_char`, `logit`, `statuslog`, `mark_player_dirty_components`, `persistence_alert` | No-op output/dirty bindings; alert increments a counter. No observer or prompt proof. |
| `currency_transaction_coin_item_busy`, `collector_transaction_item_busy`, `spell_component_retirement_waiting_for_effect`, `player_save_pipeline_sealed_save_pending` | False; no real cross-owner contention or save exclusion is established. |
| `collector_catalog_cache_invalidate`, `collector_death_enrollment_note_committed`, `collector_death_enrollment_attach` | No-op/true enrollment bindings; ordinary selected input never enters enrollment. |
| `player_load_item_graph_materialize_creation`, `craft_callback`, `publish_recipe`, `recipe_acknowledged` | Reused creation/craft controls, not exercised by these ordinary cases. No player load or craft qualification. |
| `nevent_is_game_thread`, fixture `nevent_require_game_thread` | Compare current thread with fixture game thread; no actual event loop. |
| `panic_corruption_int`, fixture `panic_corruption`, corpse note, `CheckEqWorthUsing`, link-only drop/save/retirement/literal bindings listed above | Abort if reached. No successful response is supplied for those paths. |
| Global world/object/character lists and `character_identity_test_fixture.h` | Synthetic room indices and linked object/body, with actual identity provider registration and fixture panic behavior. No real world, account login or reconnect. |

The compiler is WSL Ubuntu-22.04 g++ 11.4.0, using C++20, `-Og`, strict warnings
including `-Werror`, and ASan/UBSan with actual providers,
section GC and no MySQL. New compiler temporary files are directed to the task's
D: scratch. Initial scaffold compilation lacked the native corpse constants
header and did not produce a qualified executable; the scaffold was corrected
by including the original `src/classes/necromancy.h`, without editing source bodies.
A subsequent link attempt exposed the shared-variant dependencies above; both
failed attempts are preserved in the private scratch and are not passing evidence.
The final compile passed in 499.763 seconds with no diagnostics. The first process
then refused coordinator initialization: the D: DrvFS directory was `0777`, while
the unchanged journal requires owner-only POSIX directory/file permissions
(`src/persistence/critical_command_journal.c:718`). That failed process reached no
pickup submission and is preserved. No mount settings, source or assertions were
changed. The run-only driver reused the identical qualified binary with eight
fresh owner-only journals under
`/dev/shm/duris-pickup-reservation-20261008-d102clgn`, a RAM-backed POSIX filesystem.
Logs and final journal files were copied to D:. This transient filesystem exception
uses no new C: disk storage; it proves journal/checkpoint mechanics only, not disk
durability, power-loss or restart recovery.

The extracted function SHA256 values are:

| Unchanged function | SHA256 |
| --- | --- |
| `find_live_item_uid` | `214894bd143f18e2d302fa8544d531daab35aeef2807f158482471e89831e027` |
| `item_get_completion` | `dbf71bcb2118320aeee5dc7270cda6466467dc683a34a41e8fc5acdf59baa4ea` |

All eight fresh-process cases exited 0 with ASan leak detection and UBSan halt-on-
error enabled, without sanitizer diagnostics. Each used one actual generated
original ID. Before gameplay dispatch, the real coordinator pulse had retired
both item/player fences, had no publication-pending operation and reported zero
critical journal records. Each case's final journal also contained zero records.

| Case | Actual shared-owner observation | Native dependency and final projection |
| --- | --- | --- |
| Success | Pending erased; registry advances room → player, item revision 1 → 2. | Doubled placement called once; carried by original actor. |
| Terminal-outcome branch (`rejected`) | Pending erased; registry remains room. | Actual completion takes noncommitted branch; placement dependency never called. The apply double uses error 0 and its ordinary incremented result; this is **not** a canonical SQL rejection receipt. |
| Missing actor | Pending remains 1 and registry remains room after completion and an empty pulse. Actual readiness then clears pending and applies registry. | Placement called once after the original actor returns to the list; carried. No automatic empty-pulse retry. |
| Same-PID replacement body | The same missing-actor retention occurs, then readiness selects runtime 7002 for PID 1001. | Placement called once onto the replacement. Runtime 7001 remains registered but is absent from `character_list`; this is a controlled body-selection test, not authentic logout/destruction/reconnect. |
| Stale source topology | Pending erased and registry already player. | Actual callback rejects mismatched room context, alerts once, never calls placement; object remains in room placement. |
| Placement returns rejected | Pending erased and registry already player. | Placement dependency called once; actual callback alerts once; object remains in room placement. |
| Throw before placement effect | Pending erased and registry already player; exception escapes actual callback/movement invocation. | Dependency called once; object remains in room placement. |
| Throw after placement effect | Pending erased and registry already player; exception escapes actual callback/movement invocation. | Dependency called once; object is carried. No retained native phase remains. |

The driver also asserts that subsequent empty pulses/readiness do not call the
placement dependency again. It does not fabricate a completion, manually ACK an
operation, directly erase pending state or alter the actual callback. The initial
directory-setup refusal was corrected outside the binary; no expected observation
was changed to obtain these results.

Private `evidence-index.json` authenticates 34 artifacts, including all attempt
logs, both driver versions, recipes, exact input pins, generated source, binary,
eight result logs and copied final journals. Reproduce with the preserved compile
command and `run_component.py`; use fresh native/evidence directories rather than
overwrite original cases. Exact hashes are:

| Artifact | SHA256 |
| --- | --- |
| Evidence index | `9d5dc0c43cc66f713f5c777f8ec93aa52e6c10570518d39b81cc9d84fc36fe04` |
| Qualified binary | `04f20f0468b0cee7017562fc83a7433456cbebfd4d29ad095a20f2411bb10060` |
| Generated component | `d7cb458b1fa7f9598dcdfea5cd98326fc1186391a9e4e9310530701a12ad623c` |

These controls characterize original ownership mechanics only. An injected throw
before/after a doubled placement is not an unreturned production native handler,
and an inactive component cannot prove active authority/recovery failure. Expected
observations were derived from the frozen control flow; no maintained expected
result or original test control was edited to create a PASS.

## Exact proposed reservation, conditional on owner inputs

**Reserved outcome:** for an already-owned ordinary eligible room root and its
complete subtree, the original successful/rejected/uncertain pickup retains an
authenticated continuation until current durable/native correspondence and clean
observation authorize ACK and save-hold release. Existing ordinary drop behavior,
schemas, command/literal formats and both durable writers remain unchanged.

This is one integration reservation, not permission to implement each row now.
The appendix freezes the exact primary preimage of every proposed path.

| Proposed path | Smallest responsibility/change to review |
| --- | --- |
| `src/cmd/actobj.c` | Select the narrow ordinary eligible single room-source owner after existing policy checks; keep native selection and actual effects. Separate its original physical continuation from post-proof notification. Do not reroute locker/corpse/bulk/adoption/coin/quest paths or already retained drop. |
| `src/item/item_movement_transaction.c` and `src/item/item_movement_transaction.h` | Own the exact immutable pickup command/receipt, original actor participant and bounded native phase state; retain pending on missing actor, registry/current-graph refusal, uncertainty and thrown/unreturned dependency. Submit for publication only when the actual save/current-proof owner is installed. Extend room-source restoration under that owner rather than relaxing all player-source checks. |
| `src/player/player_save_pipeline.c` and `src/player/player_save_pipeline.h` | Supply the actual player BEFORE/AFTER checkpoint/source, drain/permit exclusion, original hold and login/load handoff for pickup. Keep same-ID ownership through publication, ACK and release; no caller-set ready flag or public ACK-by-ID. Existing drop/SHOP/quest profiles cannot simply be borrowed. |
| `src/persistence/economic_sql_item_transfer_transaction.c` and `src/persistence/economic_sql_item_transfer_transaction.h` | Within the existing typed item owner, supply the original receipt plus fresh locked current player/room custody and complete materialization proof to the native continuation. Preserve lock/session/cleanup order and existing root transaction; do not add a writer or use historical receipt as current graph authority. Exact implementation is blocked on the original source/save owner contract below. |
| `tests/async/test_publication_retention_runtime.py` | Add a separate ordinary-pickup component section using unchanged actual movement/coordinator/registry and callback, preserving its original retained-give/craft/uncertainty controls byte-for-byte. Keep doubles labeled; component assertions protect state ownership, not active SQL acceptance. |
| `tests/async/run_sql_room_item_payload_recovery_journey.py` | After the owner contract exists, extend its actual active drop producer with one reciprocal active pickup and complete bag/sibling subtree, save/relogin/cold proof and authenticated phase faults. Keep seeded/inactive preparation explicitly distinguished. |

`critical_command_repository.c`, the item writer, player load/materialization and
world handlers are inspected dependencies, **not reserved edits**. If the original
receipt reader or player source cannot be provided without changing another
owner, return that exact dependency for review; do not expand this reservation.
No flat-file/native parity change is reserved by this SQL-first slice.

Required transition invariants are:

1. **Success:** exact current owner/topology/literal correspondence for every
   selected UID, unchanged unrelated roots and original operation binding precede
   physical effects. Native departure/placement each have started/returned state;
   completed effects are verified rather than repeated. Messages/dirty/light/
   capacity behavior retain their native ordering and supported outcomes.
2. **Rejection:** distinguish never-admitted refusal from authenticated terminal
   execution rejection. Prove the original source remains and compare the actual
   canonical rejection receipt; no invented destination, successful physical
   callback or replacement ID. Do not erase another owner's unresolved hold.
3. **Uncertainty:** journal-uncertain admission, ambiguous COMMIT, unavailable
   current proof, malformed/contradictory receipt and exhausted retries preserve
   the original ID/bytes/continuation and coordinator/save fences. No false
   noncommit notification or compensation that assumes rollback.
4. **Publication:** a live actor lookup by PID is not sufficient body/graph proof.
   A vanished/replaced body must reach the original owner's permitted load/adoption
   cut or remain waiting. Whole-subtree census must reject duplicates, foreign
   links, partial presence and stale revisions. A handler started without returning
   retains ownership; a returned placement is never rerun solely because hydration
   or ACK failed.
5. **ACK/release:** bind exact original receipt and prove current physical graph
   plus transaction cleanup first. Checkpoint the original once, then release its
   save obligation; an ACK failure retries ACK, not native handlers. An independent
   hold release failure remains owned. No caller supplies proof or authority by ID.
6. **Save/load:** drain actual queued/inflight/retained frames before admission;
   exclude new ordinary saves through the real permit/hold owner until final proof.
   Preserve custody membership/topology guards and unrelated components. Restart
   restores the original room-source pickup and its player/save participant before
   login/readiness; authoritative loads must not duplicate a live floor UID.

## Inputs that prevent production implementation now

The missing interfaces are concrete owner evidence, not a need for a new facade:

- The player save owner must provide the genuine pre-pickup complete player source,
  acknowledged save revision/body binding, selected incoming literal subtree and
  resulting complete player forest, plus the permitted resident/hold/load transition.
  An ordinary drop token requires a currently carried root and cannot be reused
  for an incoming room root. A SHOP borrowed lock or copied BEFORE/AFTER type alone
  cannot prove absence of stale queued saves.
- The existing typed item/SQL owner must provide fresh current selected custody,
  current player materialization/room absence and exact original receipt under its
  real lock/session/cleanup cut. The ordinary-drop verifier at
  `src/persistence/critical_command_repository.c:4355` captures a drop literal
  sidecar contract; it is not a generic pickup verifier. The normal load repository
  and history verifier are inputs, not a ready current native-publication capability.
- The original native/player owner must define replacement-body and same-process
  reload uniqueness, eligible object behavior and handler-started/unreturned recovery.
  The void callback and root `loc` checks do not provide this cut.
- Native qualification requires authenticated active regular SQL fixture/lineage,
  original already-owned bag/player/room/world, actual producer and observer binaries,
  real save/log-in paths and owned phase fault boundaries. No accepting authority
  stub, seeded expected graph or inactive prompt result supplies those prerequisites.

**First useful testable slice:** the private inactive component above already
characterizes success, missing actor/readiness and erased stale/exception callback
continuations with the actual shared owners. After review, the smallest maintained
test reservation is the separate section in the existing retention test, preserving
its original controls. Production retention wiring must wait for the save/current
proof/restart owners above; adding only a publication callback now would create a
room-source journal that existing restoration cannot own.

Required native qualification then starts with real active ordinary drop → pickup
of the same complete bag subtree, including a sibling branch, exact UID/edge/
literal and accounting/native-reference conservation, original operation per move,
save/logout/relogin and two cold boots without Redis. Fault cuts cover durable
commit before callback, departure before return, returned placement before
hydration, physical proof before ACK, and ACK before hold release. Missing/replaced
actor, stale/foreign/duplicate graph, stale save, rejection, lost commit reply and
conflicting receipts must retain the correct original owner without repeating
effects. Verify native graph and SQL independently; do not inject expected fields
into an observer. Seeded after-ACK recovery and inactive acquisition retain their
limited labels.
The current optional producer at
`tests/async/run_sql_room_item_payload_recovery_journey.py:325` performs inactive
preparation, guarded active drop and cold boots in a disposable synthetic route;
its qualification explicitly records `global_route_qualification=False` at
`:546`. It does not already execute the reciprocal active pickup or the future
bag/sibling acceptance unit. This investigation did not run that journey.

## Original controls, collision and import constraints

Preserve the unchanged controls in `tests/async/test_publication_retention_runtime.py`
(retained give/retry, original runtime identity, craft/progression and exhausted
ambiguity), `tests/async/test_item_movement_input_queue.py` (actual registry failure
and dependent queue hold), `tests/async/test_item_movement_prompt_runtime.py`
(nonretained deferred output), `tests/async/test_bulk_get_publication.py`
(bulk selection/reporting), `tests/async/test_publication_ack_checkpoint.py`
(extracted ACK checkpoint concurrency/retry with doubled journal and fence bindings),
`tests/async/test_player_item_custody_write_guard.py`
(projection guard), and the native room-item journey. No control was changed or
claimed newly passing here. A later patch needs the relevant component checks,
required maintained build after C++ edits, and the native qualification above;
no broad build or DB action was used to force this investigation forward.

The [primary checkpoint](https://github.com/Community-Duris/Duris/blob/4357367798ac045879081a8bdcf861a4699a7ce4/docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md)
reports private typed flat SHOP storage/proof at `e8b8d842f`, with 119 production
files and reported candidate SHA256
`24e0d474f8f4a110892629be08d2cfcad1dbfb22cca7347a099b7b6043b86264`.
Its atomic storage/proof progress remains private and source-reviewed;
real producer hold/save-drain, flat dispatch/native publication/ACK/recovery and
shared keeper classification remain dependencies. Public source trees are unchanged.
No private implementation was inspected or imported. The common movement/save/
typed SQL owners above overlap primary work; arrange the exact primary candidate
base and owner disposition before reserving edits. Quest retains its separate
temporal QP03 oracle and all quest-preparation paths; Plan 5 is untouched.

Import dependencies are exact preimages in the appendix, the accepted map and
completed owned preparations. Compare the final reviewed patch against both the
actual primary candidate and the R0–R14 owned callers before import; these two
source bases differ. A future rollback must remove only the reviewed owned patch
and preserve unrelated work, original journals/receipts and admitted holds. Never
turn off retention or discard a room-source original that the old restoration
cannot read; drain/qualify any live obligations under their owner before a release
change. This is a release dependency, not authorization for operational action.

This finite investigation does not complete or resume the blocked Plans 1–5/
applicable original R1–R8 native Goal. No server, DB, migration, activation,
deployment, Docker repair or full native build was performed.

## Exact inspected preimages

`maintained` pins the primary revision above; `owned` pins the owned baseline.
Proposed paths are conditional review reservations only. Private component
sources/results remain on D: and are not repository changes.

<!-- BLOB_PINS -->

| Revision | Inspected file | Exact Git blob |
| --- | --- | --- |
| maintained | `docs/persistence/economy_accounting/EXPERIMENTAL_REVIEW_CHECKPOINT.md` | `e302bf632d4cb9b8fd8c63f04361c69ea37c98fe` |
| owned | `docs/persistence/economy_accounting/domain-separation/ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `70e8cc751e01bb283629582b7572365059725ccc` |
| owned | `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| maintained | `docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md` | `8ca2d3e2d3af0f4d62318a1f8dd2a8837a6cfde0` |
| maintained | `src/account/account.c` | `a65c6b8c3c915a8515c413903c6f25f5cf56ae25` |
| maintained | `src/account/character_identity.c` | `f7c46058bc7881d68404648dfbdca09d1f25edaa` |
| maintained | `src/account/nanny.c` | `05ff831e396bc5027228db0a086a4ef58320775f` |
| maintained | `src/classes/necromancy.h` | `24a022cf2e586a8ca320c2deea5d5fd9ec6a2b8e` |
| maintained | `src/cmd/actobj.c` | `a2114fddb6816f1534488ff11457a1001d47a14c` |
| maintained | `src/cmd/interp.c` | `74281b6f20ccd88a588d63cdda143da275258e85` |
| maintained | `src/combat/chaos_pouch_ledger.c` | `a0c8363bc13ba6926ef320ded9dda2bba67a4984` |
| maintained | `src/combat/chaos_pouch_publication.c` | `bd2f920acc95209c04cfa7e5cf15e6932b69853e` |
| maintained | `src/economy/economic_accounting_intent.c` | `00db5654239f221392bc79b5bdc5ee6046d55a43` |
| maintained | `src/economy/economic_accounting_plan.c` | `8204617fcf1ea5f322f06897f9c5c8848f9aa38b` |
| maintained | `src/economy/economic_accounting_types.c` | `f1ada31fc487e5649bda982d30482ace1e6ce009` |
| maintained | `src/economy/economic_gameplay_authority.c` | `4afa41bdb718068e4862f422b701fa5ad6953d56` |
| maintained | `src/economy/economic_source_event.c` | `f91b9c3eb94dfd2900dfa39e97391cad7966f301` |
| maintained | `src/economy/item_transfer_accounting.c` | `b7012f63c3ce96573eaf23c0424d6447f71e74ac` |
| maintained | `src/economy/native_quest_coin_give.c` | `eaf9fc466bfda32e6acd33b88bb6bed9fa81f0a2` |
| maintained | `src/economy/native_quest_cost.c` | `439ceade2196d33ac60e8428a12fda2f19564837` |
| maintained | `src/economy/shop_trade_recovery_manifest.c` | `1dc5d70e0c8dd13ade685a80a183a7783e827abf` |
| maintained | `src/item/craft_pouch_mutation.c` | `0de5f65698c85a056303ab35b1156a4feeb0b650` |
| maintained | `src/item/item_command_policy.c` | `4b7b98ffcb697d10752cbc051342cf5432d0510c` |
| maintained | `src/item/item_get_policy.c` | `159d27f43ff731c92becfd2c69b0dccfe35cbdb1` |
| maintained | `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` |
| maintained | `src/item/item_movement_transaction.h` | `6b4c9d4e52af84687cff6921450a0782ef42f489` |
| maintained | `src/item/item_ownership_runtime.c` | `cf0aae0dfa3ce2d7397d357127ec1b6e3c8074ba` |
| maintained | `src/item/item_transfer_command.c` | `637051605ad1b1b27b9254eb510acb347852f5e9` |
| maintained | `src/item/item_transfer_repository.c` | `3476ec8518eaeb743ec90ecff93c10de7fe18923` |
| maintained | `src/item/lockpick_retirement_continuation.c` | `c95ca2ccf4af9d30265036ecebc709d9be81865f` |
| maintained | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| maintained | `src/persistence/critical_command.c` | `f7bc9b86f0fbf3b75fe6ae2b47ed62442f1d4763` |
| maintained | `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| maintained | `src/persistence/critical_command_journal.c` | `a64124327c421ff928597619600597b534fbc600` |
| maintained | `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| maintained | `src/persistence/economic_sql_item_transfer_transaction.c` | `2da2b2b11979424e339389db89e9b7b6a981056c` |
| maintained | `src/persistence/economic_sql_item_transfer_transaction.h` | `331cc5d4c311f824e8b80ab842f03f3b2677911c` |
| maintained | `src/player/player_load_items.c` | `44d5c223ed3623807f59b320f91a439ad3d6e7c1` |
| maintained | `src/player/player_load_repository.c` | `9763bfabcd8dc92af814ed894d3e1d63429e3e5b` |
| maintained | `src/player/player_save_execution_guard.h` | `1dfdb7767c13f31d701007b9445158de1562a136` |
| maintained | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| maintained | `src/player/player_save_pipeline.h` | `e1c2d87094c2eeaa120ceff65406ac7f7b7d5baa` |
| maintained | `src/player/player_save_worker.c` | `014974c40a0ebf11373c74ecb20bf76a23ea0ef0` |
| maintained | `src/player/player_snapshot_codec.c` | `27d440a90d714fd1cba95720edd9e59eec976844` |
| maintained | `src/player/player_snapshot_repository.c` | `21c7108c041613b2b76d1b305b1fe5d842bb360d` |
| maintained | `src/sql/sql_player.c` | `57a536a3a192b5432b17fd7ac088b76f7559b41e` |
| maintained | `src/world/handler.c` | `0054c7db2f7cb990dfde047e6de09250d54670e3` |
| maintained | `src/world/quest_mobile_native_reference.c` | `f0308d6b79912af6e1b9233307b66ee66cc76497` |
| maintained | `tests/async/_paths.py` | `7675d09df7aaeb1db606b598b821c3d4c4f4c8f2` |
| maintained | `tests/async/character_identity_test_fixture.h` | `b72342cb7b917778d6d69f91162ca63c1f6854ad` |
| maintained | `tests/async/contract_text.py` | `a8b7a1279f112971423b04c8861b462e79605dff` |
| maintained | `tests/async/run_sql_room_item_payload_recovery_journey.py` | `2327a7ba985e5d71d4abc804597b2e39434a2764` |
| maintained | `tests/async/test_bulk_get_publication.py` | `c5f205cec45b5cc9411e80711d999541fe21ae2b` |
| maintained | `tests/async/test_item_movement_input_queue.py` | `c72e2261b2cb2a122d1b7b93b370f338cdd6f481` |
| maintained | `tests/async/test_item_movement_prompt_runtime.py` | `dc801e619534c6dabe745fb3455d5d986ac6eb0e` |
| maintained | `tests/async/test_player_item_custody_write_guard.py` | `25d9abeb304194498ddefd453f592026bbcea6e4` |
| maintained | `tests/async/test_publication_ack_checkpoint.py` | `e55aea09b78ac185eed79e96913ec79784e5703a` |
| maintained | `tests/async/test_publication_retention_runtime.py` | `6af4bb93605fa07c31b4ea42026100d91cbe3843` |
