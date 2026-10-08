# Craft/Forge recipe completion authority boundary - 2026-10-08

Disposition: source-grounded integration map of the existing recipe owner. No
new extraction or acceptance runner is reserved. The closed R1 planner and its
accepted SQL/flatfile recipe journeys already cover the distinct preparation and
ordinary persistence work. The maintained progression and craft ACK fixtures
specify the remaining component controls. Actual active-world phase qualification
depends on its original participants, backend, publisher/save/ACK and cold driver;
this document supplies neither those inputs nor execution evidence.

## Frozen scope and existing proof

All current source anchors below refer to public export
`0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a`, authenticated from Git bytes in a
fresh private export. Its `src` tree is
`833d3085815b396861ad18a77635412212381e4b`; its `tests/async` tree is
`790f367adf805a69d53aac6460938f5c921f9136`. These match the preceding public
source/test trees. The new public commit changes documentation only. The
[latest public preparation review](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md)
reports private `d9eaa45b` NMB4/MBR4 staged helpers and expressly unexecuted native
qualification. It exposes no new recipe completion interface.

Reuse [R1's exact accepted review](https://github.com/Community-Duris/Duris/blob/7a3ee6fb71157f355d63c85de34da748fef60fee/docs/persistence/economy_accounting/domain-separation/R1_CRAFTING_REVIEW_2026-10-07.md),
implementation `48cdf9cb0893873651216f7940aae2691d060e58`, and
[terminal handoff](https://github.com/Community-Duris/Duris/blob/4f7fa384b09914094a56cd8040e6f9c1588f92a4/docs/persistence/economy_accounting/domain-separation/HANDOFF.md).
R1 extracted the material planner while retaining live fact capture, selection,
submission, output UID admission, pouch conservation, progression, receipts and
recovery. Its focused recipe-transaction/progression checks passed. Both
maintained development builds passed with dependency recompilation and complete
740-object links; these were incremental builds using qualified R0 objects.

Accepted recipe-only journeys used real mortal Craft/Forge, exact material/tool
retirement, fresh output UIDs, saved XP and retained pouch counters, then copyover
and two cold restarts, on each backend. SQL used disposable loopback MariaDB and
the canonical migration runner. The flat journey used a private launcher adding
the existing baseline codec and adapter; both preceding link/setup failures remain
retained. The original journey copied its executable before boot. These historical
results are not current active-accounting or combined-candidate qualification.

| Historical artifact | Exact SHA256 retained in the R1 review |
| --- | --- |
| SQL server | `b85d8bbbd7970cf7fc831959531841658319a826c10df194bcec5fd80f0b72ab` |
| Flatfile server | `79e385143b87bf6d013f2e9c4a9d8393ee8359bbf71894076ed4370083872843` |
| `recipe-sql.log` | `a472e1302f5ee8e77cd852f9b60f46e87cda117194a490e497d667b1569f9d58` |
| `recipe-flat-with-adapter.log` | `a21bef07e9b30c7df4a786164d19a918d5771e98eb46edff200cc8c7ff9afa87` |

This map covers ordinary recipe Craft and Forge. The continuation also serves
Alchemy, but its other disciplines are relevant only to schema and hook limits.
Legacy smith uses a different debit/create/grant/ore path; reuse the separate
[paid repair/smith boundary](PAID_REPAIR_SMITH_AUTHORITY_BOUNDARY_2026-10-08.md).
No planner, material inventory, schema, registry, shared owner, Plan5 or FINISH
change follows from this delivery. The continuing native Goal remains BLOCKED
and unfinished; all closed bundles and their failures remain preserved.

## Existing operation and authority owners

| Phase | Existing owner and frozen facts | Meaning of success |
| --- | --- | --- |
| Recipe selection | `src/economy/crafting.c:74` selects actual carried input pointers, captures output UID, recipe VNUM, discipline and XP, and delegates once. | A candidate and requirements exist; no input retirement or XP award yet. |
| Native admission | `src/item/item_movement_transaction.c:3525` captures complete input trees and output snapshots, owner/item revisions and retained pouch mutation, builds one original operation and submits it for publication. | Accepted, awaiting durability, attached, or uncertain ownership; the returned Boolean is not business completion. |
| Durable item root | SQL `src/item/item_transfer_repository.c:1842` and flat `src/flatfile/flatfile_item_repository.c:3911` retain item effects and progression obligation with the item operation. | Backend result for that original root; player progression is a subsequent checkpoint. |
| Registry and physical publication | `src/item/item_movement_transaction.c:2141`, `src/item/item_movement_transaction.c:860` apply the result to the registry, pouch and native graphs. | Original inputs retired and outputs placed; recipe progression may still be waiting. |
| Gameplay progression | `src/economy/craft_progression.c:161` owns one live application per operation and requests the canonical player save. | `waiting` until a matching saved receipt is acknowledged; `ready` is a progression checkpoint, not the item journal ACK. |
| Player checkpoint | `src/player/player_save_pipeline.c:1548`, `src/player/player_save_worker.c:989` and backend repositories persist required progression components with matching receipts and deliver exact completion. | The receipt-bearing save is acknowledged; the craft item fence can still be retained. |
| Finalization | `src/item/item_movement_transaction.c:1787` ACKs the original operation, extracts its pending owner, cleans progression, resolves the registered actor again, then calls business hooks. | Post-ACK completion/notification for a surviving actor. |
| Recovery | `src/item/item_movement_transaction.c:4354`, `src/player/player_load_pipeline.c:409`, `src/economy/craft_progression.c:99` restore pending operations and only their saved receipt identities. | Reuse original command and durable state; no fresh recipe quote or new command ID. |

The command payload owns original UID/topology/revision and frozen continuation
facts. Backend custody and operation history own the durable item result. The
runtime registry, carried objects and pouch extra descriptions are projections.
The progression attempt map owns an in-process award/save retry, while SQL
`applied_revision` or a verified flat `.craft` file proves its saved checkpoint.
None of these clocks can substitute for another.

## Capture, reservation and refusal

`src/economy/crafting.c:82` computes XP once using the plan item value and current
configured rate, bounds it to `INT_MAX`, then selects carried requirements without
mutation. Pouch mode replaces physical low/high material selection with generated
usage; essence and tool still come from actual inventory. Missing requirements,
oversized selections or allocation failures refuse before shared submission.
`src/economy/crafting.c:141` freezes PID, Craft/Forge discipline, XP, recipe VNUM
and output UID. The small callback context stores only the output UID.

The shared owner validates those terms against the submitting actor, one output
and recipe ID (`src/item/item_movement_transaction.c:3545`). It captures current
owner revision and complete input forests, refusing foreign custody, duplicate
roots, fences, competing movements or coin work
(`src/item/item_movement_transaction.c:3553`,
`src/item/item_movement_transaction.c:3577`). The retained pouch is captured
separately, must be active and unconsumed, and has normalized snapshot topology;
its actual custody topology/revision stays in its enclosing item entry
(`src/item/item_movement_transaction.c:3595`, `src/item/craft_pouch_mutation.h:17`).

Pouch usage is sorted and duplicate VNUM counts are combined with overflow
refusal, then the ledger prepares explicit before/after snapshots
(`src/item/item_movement_transaction.c:3615`). The pouch codec checks canonical
usage and verifies the ledger relationship; payload decoding binds exactly one
retained pouch and keeps collection-specific conservation separate
(`src/item/craft_pouch_mutation.c:43`, `src/item/craft_pouch_mutation.c:142`).
This is a frozen ledger mutation, not permission to recalculate counters later.

Outputs must be nowhere, unfenced, have unique UIDs distinct from all input UIDs,
and carry complete captured snapshots (`src/item/item_movement_transaction.c:3658`).
Canonical command validation checks overlap against every captured input entry;
ordinary Craft/Forge requires exactly one output snapshot with matching recipe
VNUM/UID (`src/item/item_transfer_command.c:1149`). The generic craft owner can
serve other output shapes, but this recipe route does not admit nested outputs.
Command construction also keys the output as absent
(`src/item/item_transfer_command.c:1511`).
The continuation wraps the pouch bytes, and one multi-root craft payload binds
the original player owner on both sides, recipe ID, selected output UID, item
entries and output blob (`src/item/item_movement_transaction.c:3693`).
`src/item/craft_recipe_continuation.h:153` matches those identities on decode.
The continuation carries recipe terms, not live pointers or a fresh lookup.

One generated operation ID is used for command construction and publication
submission (`src/item/item_movement_transaction.c:3736`). When accounting is
active, the existing authority prepares the typed item transfer with crafting
source; refusal stays a refusal. Its intent uses consumed input UID lifetime as
issuance source, excluding the retained pouch; a rebuilt output or command ID
does not confer new issuance authority (`src/economy/item_transfer_accounting.c:336`).

The pending owner is installed before coordinator submission. `journal_uncertain`
retains it and returns true; definite nonacceptance erases it and returns false
(`src/item/item_movement_transaction.c:3773`). The callers dispose their staged
output on false (`src/economy/crafting.c:1037`, `src/economy/crafting.c:1557`).
The reserved message means requirements are held for resolution. Retrying the
business command to mint another ID is not recovery of that reservation.

## Durable root, physical publication and business completion

SQL's craft writer checks the stored selected graph and frozen output/pouch
facts, retires consumed custody, retains the pouch, inserts fresh output custody
and payload, advances owner revision and inserts `player_craft_progression`
terms (`src/item/item_transfer_repository.c:1865`,
`src/item/item_transfer_repository.c:2012`,
`src/item/item_transfer_repository.c:2076`). The critical repository owns the
transaction, result/outbox, accounting record and final COMMIT
(`src/persistence/critical_command_repository.c:2033`,
`src/persistence/critical_command_repository.c:3045`,
`src/persistence/critical_command_repository.c:3116`,
`src/persistence/critical_command_repository.c:3154`). The progression row starts
with `applied_revision=0`; it is an obligation, not saved XP.

Flatfile stages `.craft-obligation` with PID, original operation, discipline and
XP alongside item catalog images (`src/flatfile/flatfile_item_repository.c:3911`).
The accounted route stages that obligation with accounting operations and commits
them together; the ordinary route uses the authority transaction's operation
commit when an obligation exists (`src/flatfile/flatfile_item_repository.c:3966`,
`src/flatfile/flatfile_item_repository.c:4044`). Original-ID lookup verifies command
digest and matching obligation (`src/flatfile/flatfile_item_repository.c:3510`).
These existing roots must remain atomic; a later facade must not split pouch,
input retirement, output issuance or obligation storage across new operations.

Completion handling validates the incoming batch before retries or effects.
After craft publication starts, a changed semantic receipt blocks that owner;
a later duplicate in the same batch cannot clear the conflict
(`src/item/item_movement_transaction.c:4250`). `publish` retains ambiguous/retryable
outcomes and successful results that cannot decode, preserving original recovery
ownership (`src/item/item_movement_transaction.c:1981`). A registered actor with
the original PID is required; this may be its replacement registered runtime
lifetime after reconnect (`src/item/item_movement_transaction.c:568`).

For committed work, registry application precedes physical craft publication.
`src/item/item_ownership_runtime.c:625` accepts original or already projected
custody revisions, destroys consumed entries, retains pouch custody and admits
the captured output graph. `src/item/item_movement_transaction.c:787` reconstructs
missing outputs from the frozen blob and committed result, checking existing
UID/VNUM/parent matches. `craft_live_ready` checks actor ownership of the pouch,
remaining inputs and output roots (`src/item/item_movement_transaction.c:741`).
Missing-output materialization still needs a usable current object prototype and
allocation (`src/player/player_load_items.c:596`); the creation materializer binds
captured identities and full snapshot overrides (`src/player/player_load_items.c:786`).
Frozen payload bytes do not remove that native-world dependency.

`publish_craft` applies the frozen pouch, extracts remaining input roots, then
places nowhere outputs (`src/item/item_movement_transaction.c:885`). The pouch
publisher allocates/validates replacement ledger nodes before splicing them;
an already-after counter image is accepted without incrementing twice
(`src/combat/chaos_pouch_publication.c:39`). Partial physical progress can be
retried under the same retained owner. Placement is followed by progression
publication, which can still return waiting
(`src/item/item_movement_transaction.c:2173`). A definitive unsuccessful craft
discards only staged nowhere outputs and proceeds to finalization without XP.

When progression is ready, `craft_publication_ready` preserves that phase for
ACK retry. `finalize_craft` resolves the actor and decodes notification terms
before attempting the original coordinator ACK. On failure or exception it
retains the same owner and effects. The coordinator checkpoints the journal
before removing fences and operation state
(`src/persistence/critical_command_coordinator.c:3126`). After successful ACK,
the item owner extracts its pending node before any external hook, erases the
progression attempt, invokes notification, re-resolves the captured runtime ID,
then invokes completion (`src/item/item_movement_transaction.c:1827`). Independent
exception containment prevents a failed post-ACK hook from restarting publication.

`complete_recipe_craft` only finds the frozen output UID in current carrying and
emits finish messages (`src/economy/crafting.c:40`). It does not grant XP, save,
retire materials or reconstruct recipe state. A notification may retire the
actor, suppressing the later callback. Replayed work has no original callback
context; persisted continuation and owners resolve it without reproducing the
old cosmetic callback. Definite never-admission is the explicit no-journal-ACK
case; it does not apply progression or clean an admitted progression attempt.

## Exact progression, save and recovery ordering

The two acknowledgments must retain their separate meanings:

1. `publish` validates PID/terms and creates an attempt keyed by original operation
   with a receipt containing operation, discipline and XP
   (`src/economy/craft_progression.c:161`). It compares PID and those receipt
   fields on repeats. An acknowledged attempt returns ready immediately.
2. Otherwise it bounds pending awards, sets `applied=true` **before** gameplay
   effects, notches Craft/Forge at 50 and calls `gain_exp` with frozen XP
   (`src/economy/craft_progression.c:194`). This is live progression. Craft/Forge
   skill outcome is determined here; it is not a pre-admission frozen notch.
3. It requests STATUS, SKILLS, AFFECTS and TROPHIES. Queued/coalesced saves set
   `save_pending`; repeats do not supersede the admitted revision. Capture failure
   can retry after 500 ms without repeating the award
   (`src/economy/craft_progression.c:221`, `src/player/craft_progression_hooks.h:32`).
4. The pipeline includes applied, unacknowledged receipts and required components
   in the canonical envelope (`src/player/player_save_pipeline.c:1548`). Receipt
   merge refuses conflicting discipline/XP for one operation and preserves schema
   and receipt capacity (`src/player/player_save_pipeline.c:1106`). The codec also
   requires the four components and distinct valid receipt identities
   (`src/player/player_snapshot_codec.c:455`).
5. The worker accepts matching active PID/revision completion; on applied or
   already-applied it requires `player_revision_acknowledge` for claimed components.
   Only then does it move the active snapshot's receipts to successful completion;
   finished failures carry `failed_craft_receipts`
   (`src/player/player_save_worker.c:971`, `src/player/player_save_worker.c:989`,
   `src/player/player_save_worker.c:1050`). The pipeline dispatches these to
   `saved(pid,true/false,receipts)` (`src/player/player_save_pipeline.c:3933`).
6. `save_completed` does **not** apply XP, create missing attempts, inspect current
   recipe/config or itself ACK the item journal. For each existing matching PID
   and receipt it sets `acknowledged=true` on success. Failure clears `save_pending`
   and retry time, leaving `applied` intact (`src/economy/craft_progression.c:80`).
   This hook has no revision argument; exact revision authority comes from its
   worker/backend caller. A subsequent publish returns ready, enabling item ACK.
7. `recover_receipts` first validates count, identities, duplicates and conflicts
   with existing PID/receipt entries. **Before** marking any supplied receipt
   acknowledged, it checks every live applied/unacknowledged attempt for that PID
   is present in the incoming saved set (`src/economy/craft_progression.c:99`).
   Thus an older reconnect image cannot erase the live award/checkpoint fence.
   Its subsequent loop marks matching attempts acknowledged or inserts new
   `applied=true, acknowledged=true` attempts, suppressing reapplication. Allocation
   or capacity failure can return false during this loop; it is not a general
   rollback transaction. No gameplay effects run in recovery.
8. Only `acknowledged(operation)` erases the attempt, after the item journal ACK
   (`src/economy/craft_progression.c:241`,
   `src/item/item_movement_transaction.c:1842`). Save acknowledgment alone does
   not erase the operation's retained item owner or authorize business callbacks.

The saved receipt is deliberately smaller than the command: it contains operation,
discipline and XP, with PID supplied by its owner/envelope. `receipt_equal` does
not compare recipe VNUM, output UID or pouch bytes
(`src/economy/craft_progression.c:39`). Those remain bound by the original item
payload/continuation and backend root. Calling a progression hook with arbitrary
new terms is not a replacement for authenticating that root. Current object lookup
resolves a projection for the original UID; current recipe/config lookup cannot
replace the admitted terms or prove their prior result.

SQL saves validate receipt terms against a successful original inbox operation
and set `applied_revision` in the same transaction as player components and
`save_revision` (`src/player/player_snapshot_repository.c:2176`,
`src/player/player_snapshot_repository.c:3071`). Flat saves verify the root and
obligation, write operation-specific `.craft` receipts with the save revision,
clear transient receipt vectors from the materialized main player image, then
commit those files with that image (`src/flatfile/flatfile_player_repository.c:1556`,
`src/flatfile/flatfile_player_repository.c:1680`). Filename/schema/operation checks
come from `src/flatfile/flatfile_craft_progression.h:7`. A filename alone is no proof.

On cold replay, the item owner restores the original command/PID/payload with
`completion=nullptr`, `registry_applied=true` and `recovered_publication=true`
(`src/item/item_movement_transaction.c:4499`). Load requests capture only pending
craft operation IDs; SQL selects their successful applied receipts bounded by the
loaded save revision (`src/player/player_load_repository.c:2418`). Flat load reads
those requested `.craft` files, checks root, obligation and revision under the same
authority cut (`src/flatfile/flatfile_player_repository.c:636`). Its root helper
checks a retained successful non-coin result; it is not a new current-world census
or complete command-authentication API (`src/flatfile/flatfile_item_repository.c:3394`).

Materialization loads progression state first, then invokes receipt recovery when
items/pets/recovery are not degraded (`src/player/player_load_materialize.c:895`).
Boot installs hooks before the coordinator's replay setup
(`src/net/comm.c:957`, `src/net/comm.c:1143`). The craft-specific publication path
takes precedence over a generic restored publication callback. Missing actor or
unproven native graph retains the original work; a saved receipt does not invent
a missing participant or authorize new output creation from current config.
Login later invokes the existing player-ready publication edge
(`src/account/nanny.c:1863`, `src/item/item_movement_transaction.c:4567`).

## Coverage and implementation dependency disposition

| Existing input | Covered contract and limit |
| --- | --- |
| Closed R1 review/terminal handoff | Accepted planner/components, maintained builds and genuine recipe-only journeys on SQL/flat, copyover and two cold boots. Reuse exactly; no current active-accounting inference. |
| `tests/async/test_recipe_craft_transaction.py:77` | Extracts actual submission/completion functions with controlled game/submission leaves; exercises Craft/Forge and pouch/physical requirements, refusal, selection and absence of direct XP/save effects. Historical R1 passed; no real backend/publication proof from this component. |
| `tests/async/test_craft_progression.py:54` | Actual progression owner and snapshot codec, controlled skill/XP/save leaves: retries, wrong saved receipt, stale reconnect refusal, cold saved receipt suppression, slow queued/coalesced saves, duplicate/schema rejection. Historical R1 passed; simulated initialize/recover is not a server cold boot. |
| `tests/async/test_craft_publication_ack_retention.py:35` | Maintained 30-scenario fixture uses real coordinator/journal/codecs/item owner/progression with controlled world/skill/save and execution leaves. Specifies ACK retry/exception, actor absence/retirement, post-ACK exceptions/reentrancy, never-admission and receipt conflicts during progression/ACK waiting. Native fixture availability does not establish current execution or actual SQL/flat gameplay. |
| `tests/async/test_craft_progression_restore.py:25` | Maintained disposable flat restore test specifies corrupted applied/obligation files, missing obligation/root, future revision and renamed receipt refusals. Restore decoding is separate from original participant publication. Not rerun or newly qualified here. |
| `tests/async/run_alchemist_crafting_journey.py:641` | Current source retains real recipe setup and ordinary/pouch Craft/Forge observations, saved XP, copyover/two cold boots. Historical accepted R1 results remain pinned to their original binaries/launchers, not this current script by filename. |

No distinct runnable acceptance gap is established by this source review.
Duplicating the planner, progression retry controls or craft ACK fixture would
add no new authority proof. No follow-up implementation or runner is reserved.
When an actual domain facade is assigned, its smallest integration is to delegate
to this existing recipe submission and retained publication/save pipeline, preserve
the full frozen command and expose admission, durable root, progression waiting,
ACK pending and completion as distinct observations. These are design obligations,
not new callable APIs or permission to replace the existing owners.

| Dependent step | Exact required input / owner constraint |
| --- | --- |
| Genuine active recipe admission | Initialized original actor, supported acquisition of exact material/tool/pouch lifetimes, configured recipe and active backend admission. Existing candidate construction alone does not supply these. |
| Commit-to-publication/save cuts | Original command/result and current graph/registry, real publisher, receipt-bearing player checkpoint and bounded native phase witness. Fixture wrappers are not server fault hooks. |
| Original-ID uncertainty/ACK replay | Real retained operation and journal, backend reconciliation, matching saved receipt and original fence owner. Never rebuild terms from a new rate, recipe lookup, output UID or replacement command. |
| Copyover/cold interruption proof | Original backend state, executable, replay/load chronology and real driver controlling the intended cut. Historical ordinary R1 restarts remain accepted but do not prove every active interruption cut. |
| Facade/primary import | Primary's current preimages, private owner overlap and its adoption/release decision. Private NMB reports expose no replacement recipe owner; optional documentation creates no adoption wait. |

Unavailable inputs block only these dependent executions. They do not reopen
closed evidence or justify a broad run, seeded authority, schema/registry change,
shared observer, new journal or fabricated native interface. This delivery ran
source/blob/anchor/link/design and diff checks only. Native, build, database,
server, migration, journal and gameplay execution are UNEXECUTED for this map.

## Exact body pins and private review evidence

The appendix pins whole inspected bodies rather than extracted replacements.
Current bodies use the public revision above; historical reference bodies retain
their own revisions. Private export, manifests, anchor text, link resolution,
ordering checks and publication proof live under
`D:\Dev\Temp\craft-recipe-completion-boundary-20261008`. No private evidence,
logs, player data, credentials or generated build output is committed.

<!-- BODY_PINS -->

| Revision | Whole body | Git blob |
| --- | --- | --- |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `docs/persistence/economy_accounting/domain-separation/PREPARATION_BUNDLES_REVIEW_2026-10-08.md` | `b09d878d29d674147ab0baab1b51e9e11c38294a` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `migrations/immutable/0053_craft_progression.sql` | `0eb2280b098b1c4f9871561efb3f1638bef49e33` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `migrations/immutable/0054_alchemy_publication.sql` | `896052817ca767575c8a19fa708be2e59c5b2998` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/account/nanny.c` | `05ff831e396bc5027228db0a086a4ef58320775f` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/combat/chaos_pouch_publication.c` | `bd2f920acc95209c04cfa7e5cf15e6932b69853e` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/economy/craft_progression.c` | `58d85913ae67dd4d1053c79c60caadc841b04f9e` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/economy/crafting.c` | `14b5db3633c939434fbaa323ead1b8bac6e94d64` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/economy/item_transfer_accounting.c` | `b7012f63c3ce96573eaf23c0424d6447f71e74ac` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/flatfile/flatfile_craft_progression.h` | `77b26f0314bec65ec1df47e622f20990f8ccafeb` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/flatfile/flatfile_item_repository.c` | `14f21c674e6226bd7e66bce57461f29d06367583` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/flatfile/flatfile_player_repository.c` | `83c3690d486493b98b0e0da2e39f514fdff8422a` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/craft_pouch_mutation.c` | `0de5f65698c85a056303ab35b1156a4feeb0b650` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/craft_pouch_mutation.h` | `96f14b5e1dfe14d294c0219531084589a931d6bf` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/craft_recipe_continuation.h` | `22c2f24e0c10fee4d47276d5df5bc43f63ff6baf` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/item_ownership_runtime.c` | `cf0aae0dfa3ce2d7397d357127ec1b6e3c8074ba` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/item_transfer_command.c` | `637051605ad1b1b27b9254eb510acb347852f5e9` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/item/item_transfer_repository.c` | `3476ec8518eaeb743ec90ecff93c10de7fe18923` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/craft_progression_hooks.h` | `2acb84bb87af4de213630ec1d7d56088cce10942` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_load_items.c` | `44d5c223ed3623807f59b320f91a439ad3d6e7c1` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_load_materialize.c` | `2ec4341c3b6ab4e768f7435958d2884f57306ffc` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_load_pipeline.c` | `6877c02960d94f6e17ee09e7ef0b84e2673e9960` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_load_repository.c` | `9763bfabcd8dc92af814ed894d3e1d63429e3e5b` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_save_worker.c` | `014974c40a0ebf11373c74ecb20bf76a23ea0ef0` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_snapshot_codec.c` | `27d440a90d714fd1cba95720edd9e59eec976844` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `src/player/player_snapshot_repository.c` | `21c7108c041613b2b76d1b305b1fe5d842bb360d` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `tests/async/run_alchemist_crafting_journey.py` | `91eb84d7fac5b6b72a5c75ffda64af87d0c66999` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `tests/async/test_craft_progression.py` | `2ca6808483267239c43b428a824f97df22c97212` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `tests/async/test_craft_progression_restore.py` | `e8228f5afc3d1503a27b0dc6dc8904be7a18889c` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `tests/async/test_craft_publication_ack_retention.py` | `85862ea50f201c3ad61a3203a4c436fe4895aa7b` |
| `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` | `tests/async/test_recipe_craft_transaction.py` | `12ed4f36c7bc24dbb8e9e3907f783baf98f02441` |
| `7a3ee6fb71157f355d63c85de34da748fef60fee` | `docs/persistence/economy_accounting/domain-separation/R1_CRAFTING_REVIEW_2026-10-07.md` | `4c6f5c33183269e14389830da683a8d63601d2ff` |
| `4f7fa384b09914094a56cd8040e6f9c1588f92a4` | `docs/persistence/economy_accounting/domain-separation/HANDOFF.md` | `0ac0571f68606c62d1079a0d0c58bb46c5f52919` |
