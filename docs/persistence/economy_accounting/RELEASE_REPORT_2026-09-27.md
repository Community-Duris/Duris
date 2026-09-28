# Economy accounting release qualification — 2026-09-27

**Decision: BLOCKED.** This is an audit progress record, not an activation or
deployment authorization. The release validator refuses the current writer
inventory, and neither backend has a certified native-authority audit export or
the complete player and fault journey evidence required by Plan 5.

## Source and test identity

The initial Git base was `49af585c4b9c8cfa5ead0ac07025f39d4720a659`
on `add-double-entry`; the shared branch advanced during qualification and
was at `95ae59ccc3d98901b5ce39604109601c879311c3` at the final contract
check. Focused Python checks and both server builds ran against a **moving,
uncommitted working tree** with concurrent changes from other persistence
tasks. There is no single integrated commit to certify from those runs. The
disposable SQL runs used a clean `git archive` of the initial `49af585c4`
commit, not the later uncommitted tree. The release gate must be rerun on one
final integrated commit after Plans 1–4 land.

## Executed evidence

| Backend / scope | Command or method | Result and limits |
| --- | --- | --- |
| Contract and census, current checkout | `python scripts/validate_economy_accounting.py` | Passed contract validation: 13 fixtures, 492 writer rows, 2,759 lexical candidate occurrences. This does not qualify runtime coverage. |
| Release gate, current checkout | `python scripts/validate_economy_accounting.py --release` | **Refused**, `writer has no executable evidence`. This is the expected blocked result. |
| Coverage matrix, current checkout | `python scripts/generate_economy_writer_coverage.py --check` and `python tests/async/test_economy_writer_coverage_contract.py` | Matrix checked with 492 registry rows and no supplemental candidate. All 42 source-contract tests passed. The suite covers pre-mutation refusal ordering for coin theft, numbered quests, smelting and blackjack, complete mapping of the current direct-SQL, money-helper and typed-submit lexical families, and focused item files through conjuration review. Coverage remains incomplete; these are not executable gameplay proofs. |
| Audit fixture, current checkout | `python tests/async/test_reconcile_economy_accounting.py`; `python tests/async/test_audit_accounting_invariants.py` | 16 and 9 tests passed. Creation origins, exact before/after item revisions, and retired account terminal state are included. These are synthetic snapshots and operation fixtures, not live native reconciliation. |
| SQL coin component, current checkout | `python3 tests/async/test_coin_transfer_accounting.py` under WSL | One component harness passed. This demonstrates a balanced SQL accounting component, not a qualified gameplay route or flatfile equivalent. |
| Flatfile evidence and dispatcher, current checkout | `python3 tests/async/test_flatfile_accounting_store.py`; `python3 tests/async/test_economic_flatfile_dispatch.py` under WSL | Storage harness passed 85 injected commit/recovery write, sync, rename, remove and process-exit cases; dispatcher passed in SQL and client-free modes after its harness was updated for the current item route. These are isolated component tests. |
| Flatfile accounting gate, current checkout | `python3 tests/async/test_economic_accounting_flatfile_gate.py` under WSL | Passed schema-2 item movement, sourced room creation/retirement, exact item references, replay and journal recovery; unsupported coin variants were refused. This is an isolated harness, not a real player journey. |
| Lifecycle, current checkout | `python3 tests/async/test_persistence_backup.py`, `test_flatfile_backup_manifest.py`, and `test_account_erasure.py` under WSL | 29, 4, and 7 tests passed using disposable fixtures. WSL Python 3.10 required a temporary `hashlib.file_digest` compatibility shim outside the repository; the Windows Python 3.12 run cannot execute the Unix `fcntl` / `os.getuid` paths. |
| Lifecycle manifest, current checkout | `python3 scripts/validate_data_lifecycle.py` | Passed: 217 database tables and 34 non-database stores. Existing accounting SQL tables and flatfile evidence are registered in the manifest/backup path. |
| MariaDB 10.11, clean base archive | `ECONOMIC_ACCOUNTING_DB_IMAGE=mariadb:10.11 bash tests/async/run_economic_accounting_schema_mysql.sh` with a WSL Docker CLI shim | Fresh bootstrap, migration run/replay, runtime compatibility, 10 accounting schema tests, baseline verification and 10 baseline schema tests passed. The later SQL bank harness failed to link (`sql_pool_discard_connection` and `player_snapshot_repository_*` undefined), so the full wrapper did **not** pass. Container was disposed. |
| MariaDB 10.11, clean `95ae59ccc` archive | Same full disposable wrapper | **Failed before accounting tests:** migration `0036_economic_sql_activation_receipt` references a verifier committed as mode `100644`, so the migration runner receives `PermissionError`. After making only the temporary archive copy executable, migration processing advanced but runtime compatibility refused a normalized metadata fingerprint mismatch and stale immutable migration state. Neither run qualifies the clean head. Containers were disposed. |
| MySQL 8.0, clean base archive | Disposable schema-only slice of `run_economic_accounting_schema_mysql.sh`, ending after baseline schema tests | Fresh bootstrap, migration run/replay, runtime compatibility, 10 accounting schema tests, baseline verification and 10 baseline schema tests passed. This did not execute the authority, bank, baseline transaction or source-snapshot portions. Container was disposed. |
| MariaDB server build, current checkout | `make -s -C src CC=g++-12 -j2 BIN_ROOT=.../bin/plan5` under WSL | Passed with the maintained warning profile, including incremental rebuild after the coin-steal, numbered-quest, smelter and blackjack refusal guards. The default `g++` is 11.4 and rejects `-Wuse-after-free=3`; GCC 12.3 was selected explicitly. An initial build into the existing root-owned object tree also failed on permissions, so output was isolated under `bin/plan5`. Four small uninitialized-value fixes were needed in unrelated gameplay/UI files before the warning-clean build passed. |
| Flatfile server build, current checkout | `make -s -C src CC=g++-12 PERSISTENCE_BACKEND=flatfile -j2 BIN_ROOT=.../bin/plan5-flatfile` under WSL | Passed with the maintained warning profile, including incremental rebuild after the refusal guards. Generated artifacts stayed under `bin/`. |
| Synthetic audit size sample, current checkout | `/usr/bin/time -v python3 scripts/reconcile_economy_accounting.py /tmp/duris-plan5-audit-bench.json --limit 0` under WSL Python 3.10 | A 33,155,369 byte JSON snapshot (95,000 rejected roots and 95,000 receipts, no native holdings/items) returned zero exceptions in 0.82 s wall time with 193,220 KiB maximum resident memory. This is a development-host parser/reconciler sample, not the release-host 32 MiB mixed-authority budget. |

The MariaDB and MySQL tests used disposable Docker databases only. No
production database, `.env` credential, player data or operational migration
was used. A passed schema slice is narrower than an end-to-end backend pass.

## Route and workload coverage

The current matrix has 492 registry rows, including the formerly supplemental
legacy auction settlement definition. Its lexical scan has 2,701 unique
path/line/family sites, 1,420 mapped to current registry evidence and **1,281
unmapped**. Lexical sites are
candidates, not a count of real writers; each needs semantic classification or
reachability proof before `census_complete` can be true. The scan now includes
ship coffer mutations and case-insensitive direct SQL writes to ship/bank
tables. Ship hydration is recorded as a projection, while combat rewards,
coffer claims, insurance fallback, SQL saves and deletes need explicit
authority or refusal decisions. The newest source review found reachable direct
numbered-quest rewards, quest requirement consumption, reward-item allocation
and coin theft; each is now an explicit unqualified route. The coin-steal path
debits victim cash before a separate `ADD_MONEY` credit, so it now refuses an
active epoch before that debit. Numbered-quest economic actions now refuse
before requirement consumption or reward publication; tag/skill-only actions
can continue without rewriting NPC cash. The smelter now refuses recognized
coin and ore handoffs before its direct cash or item mutation. Blackjack now
refuses new wagers, game actions and periodic payouts while accounting is
active. These are source-order guards, not qualified SQL/flatfile gameplay
evidence. A prior gift to the quest NPC has already passed through its separate
give route before the quest callback. An unresolved blackjack wager from before
activation needs a quiescence and disposition policy; its periodic payout is
refused after activation, leaving the table state pending.

All direct indexed `cash[]`/`bank[]` assignments found by the current scanner
are now classified. One local `bank[6]` declaration was removed as a lexical
false positive; the SQL `load_bank` assignment fills only a temporary load
result. The shared `ADD_MONEY` and `SUB_MONEY` helpers are linked to their
direct NPC/live mutations, while shared-bank display publication is a distinct
projection and its unused single-denomination variant is a dormant candidate.
Committed wallet publication and SQL player-load materialization are now
separate projection routes with revision boundaries. The new-player flat-file
baseline read-back is separated from legacy character-file fallback loading.
NPC template parsing, conversion/scaling, PET_NOCASH clearing, copyover wallet
decode and the later legacy gold override have distinct source routes.

Two recovery findings need an authority decision. The legacy pet file writes
four denominations, but restorePetStatus reads and immediately zeroes them;
restorePet then calls convertMob, which recalculates template cash. The
current code does not establish whether those saved coins were an admitted
holding or how their loss/reissue is accounted for. Copyover captures gold in
both its legacy mob entry and generated-NPC state, then overwrites decoded gold
from the legacy entry in both recovery paths without checking agreement. An
empty generated state also leaves the other denominations from the template.
The matrix requires refusal of active-epoch publication until source identity,
complete wallet and gold consistency are executable checks. A second projection
finding is SQL login: sql_load_account_bank zeros the PC bank before its query,
and the nanny login caller ignores a failed load after placing the PC in a room.
The matrix marks that route blocked until a successful, revision-matched bank
read is proven before publication. These are audit findings; no domain mutation
was changed in this pass.

Every direct GET_COPPER/SILVER/GOLD/PLATINUM assignment, indexed cash/bank
assignment, ship-coffer assignment, and bulk coin mutation currently found by
the scanner is linked to a reviewed route. This includes provisional new-character
and new-NPC initialization, explicit service NPC sinks, SQL wallet/bank loads,
and the three CLEAR_MONEY call sites; the header macro itself is classified as
a definition only. The bulk scanner now focuses on coin arguments and pile helpers
instead of treating unrelated object memset/memcpy calls as money writers. Pile
appearance-only calls, live NPC wallet-to-pile clearing, room pile merging and
transient container put are distinct in the matrix. All 166 current shared
money-helper sites are now linked to routes. The 157 newly linked sites include
guild deposits and withdrawals, service fees, travel refunds, ship sales and
purchases, corpse wallet recovery, NPC theft, and special-procedure rewards.
Legacy auction offer, bid, pickup and settlement definitions, its rejected-credit
callback, and the old boon cash completion are marked dormant after an in-tree
caller search; helper prototypes
are nonwriters. These are source classifications, not executable refusal proofs.
All 160 current typed submission/builder sites are now linked. The 151 newly
linked sites include starter bank and item grants, spell/item admission,
crafting, auction/shop/collector dispatch, payments and lifecycle handoffs.
Header declarations and in-memory command builders are classified as
nonwriters; submission wrappers and gameplay callers retain separate routes.
The remaining unmapped lexical sites are in item lifecycle and publication
families. Their semantic review and release evidence remain incomplete.

The first item pass linked all 28 current `create_money`, `MakeScrap` and
`instantiate_object_template` lexical sites. Template construction and header
declarations are provisional/nonwriter; flatfile corpse coin materialization is
a recovery projection that needs source proof. Ship treasure-chest coin loot is
a potential issuance source. Transfer-wellness death and two special-procedure
shatter paths convert a live NPC/player wallet to a pile, so the old wallet and
new pile must reconcile as a transfer. Existing coin put/drop, corpse recovery
and scrap helpers carry the remaining linked sites. `read_object`, `extract_obj`
and item owner/publication calls still require a route-by-route review.

All 82 previously unmapped item calls in `src/cmd/actobj.c` are now linked.
Committed get/drop/give and pet handoffs are marked as live projections that
must verify the committed UID result before publication. Separate direct
legacy get/drop/put/give branches remain blocked until their UID movement is
typed. Eating, drinking, poisoning and junking have explicit consumption or
destruction routes; junk's fixed coin reward must be linked to the retired item
UID. Staff recovery of an item from an invalid location is a distinct operator
custody route. Weight and equipment relinks retain the same owner and require
slot/topology proof. The definition-only food-template scanner stays dormant.

All 84 previously unmapped item calls in `src/world/handler.c` are now linked.
Fresh prototype weight probes, refused creation candidates, rejected wallet-pile
staging and failed corpse-compaction staging are separated from admitted UID
retirement. Shared room, character, container and equipment link helpers are
unfenced custody mutation points until their caller proves an exact committed
owner transition. `extract_obj` explicitly does **not** retire durable custody
rows; its call sites include legitimate unload and rejected staging as well as
direct decay/destruction. Generic decay and character teardown remain blocked
until each admitted UID is distinguished from an unload. The committed corpse
callbacks are separately marked as projections requiring receipt and topology
proof. Two visible post-commit creation candidates remain unqualified: bone-pile
publication after corpse compaction and room-coin-pile publication during
resurrection. Each needs its source value, new UID and native result linked to
the same root before activation.

All 119 previously unmapped item calls in `src/cmd/actoth.c` are now linked.
The 90 forage branch/publication sites are one world-source food grant route;
the giant's tree publication is a separate room creation path. The doodle-only
forage probe never admits its object. Player steal has a direct fallback after
`submit_trusted_steal` returns false, so a rejected typed submission can still
move an item without its UID root. Potion and legacy scroll consumption,
donation transfer versus duplicate destruction, and room blood consumption are
separate sink/ownership routes. `do_quit` has no in-tree caller because
`CMD_QUIT` dispatches `do_camp`; its retained pre-save drop/extract branch is
classified as dormant, along with `do_old_descend`. Their lack of current
reachability is not a license to revive them under an active epoch.

All 47 previously unmapped item calls in `src/sql/sql_player.c` are now
classified. Player, locker, private-chest, corpse and saved-room-item loaders
are recovery projections whose selected UID and complete source graph still
need active-epoch proof; failed staged materializations are distinct from
destruction. The recursive locker loader checks its durable owner only when
the saved `obj_uid` column is nonempty, leaving a missing-UID row able to keep
the newly instantiated prototype UID. The shopkeeper catalog restore query
does not select saved UIDs at all and equips stock rebuilt from templates,
including derived stock; it is an unqualified identity-creation route, not a
retained-UID projection. These paths must refuse active-epoch publication until
source identity and selected custody can be proven.

All 40 previously unmapped item calls in `src/world/db.c` are now linked.
`read_object` and rejected reset candidates remain unpublished; room, NPC,
equipment and container placement are four separate creation destinations.
The B/P reset commands can select an existing container by object number,
including one already held by a player, so the destination owner and parent UID
must be established before a generated item is inserted. Replacing an occupied
NPC equipment slot is a same-owner live relink and has its own projection gate.

All 60 previously unmapped item calls in `src/classes/salchemist.c` are now
linked. Ingredient consumption, potion and poison output, furnace smelting,
encrustment and delayed enchantment have distinct source and sink routes;
temporary recipe and material probes remain unpublished. A successful
encrustment consumes the base item and jewel before publishing a new item. The
chaos-material pouch helper increments a generated-use counter without debiting
the pouch balance; if its subsequent object creation fails, that counter can
advance without a published item. That failure path needs its own operation
identity and reconciliation rule before the route can be enforced.

All 85 previously unmapped item calls in `src/guild/artifact.c` are now
linked. Boot restoration on either backend creates owned ground and NPC
artifacts from saved vnum/location data without selecting retained item UIDs;
NPC vnum alone does not distinguish individual NPC instances. Staff file import,
duplicate cleanup, timer poof and swap can remove or replace admitted copies
through direct object operations and later separate character/corpse or artifact
tracking writes. The periodic artifact-wars penalty drops player artifacts
directly after its timer update. These routes need exact source, UID and owner
proof before active-epoch publication. Display-only prototype reads and dummy
character unload are classified separately. Staff swap can leave a provisional
replacement in the global object list on failed preflight; the SQL binding
repair branch also uses a prototype after extraction and can extract it twice.
Those two defects are recorded as audit findings, not qualified writer paths.

All 30 previously unmapped unique item sites in `src/mob/mobact.c` are now
linked. NPC mage behavior can generate a corpse in a room without a death
source or prior UID. NPC thief, item-ranking and hunt behavior only move existing
weapons between carrying and equipment slots under the same NPC owner, but
their active-epoch projection still requires exact UID and slot-state proof.

All 46 previously unmapped item calls in `src/world/random.zone.c` are now
linked. Random-zone setup creates room fixtures, keys, chests, sigils, epic
stones and NPC stock; the chest can later move from a room to a spawned NPC.
Its chest filler loads `VOBJ_COINS` (vnum 3), assigns a generated amount to
`value[3]` and inserts that coin pile into the chest. This is world coin
issuance requiring a balanced source posting, even though the current lexical
scanner reaches it through the object constructor and publication calls rather
than the direct payload assignment. A level-potion chest branch is unreachable
under a literal `false`; the relic proc's player potion grant is reachable.
The random quest consumes a player's sigil and grants epic points and generated
items through separate direct actions. Labyrinth reset destroys some room items
and relocates corpses/artifacts to its entrance, while lab creation can grant a
fresh relic to an NPC. None has active-epoch root or replay proof.

All 37 previously unmapped item sites across `world_recovery_pipeline.c`,
`world_recovery_npc_items.c`, `world_singletons.c` and
`flatfile_corpse_restore.c` are now linked. A prior inventory row called NPC
gear reconstruction a projection, but its G/E reset path reads new template
objects by vnum/count and equips recovered NPCs with fresh UIDs; it is now an
unqualified creation route. Copyover room objects do restore saved item UIDs and
parent links, so they remain projections gated on selected snapshot and
authoritative graph proof. Singleton shopkeeper reconciliation can transfer
recovered stock between NPC instances, destroy produced duplicate stock, or
create missing produced stock; these effects have separate routes. Flat-file
corpse restore publishes retained room-item UIDs, but it constructs the corpse
shell from a fresh prototype without assigning a retained corpse `obj_uid`.
Staged cleanup and partial-publication rollback are classified separately from
durable destruction; the saved coin-pile materialization route now includes its
room publication call.

All 37 previously unmapped item sites in `src/magic/spell_corpse_lifecycle.c`
are now linked. The committed player-resurrection callback's room drop,
transient-item cleanup, corpse contents return and corpse removal are live
projections requiring exact result/UID checks. The older full and lesser
resurrection bodies still contain direct inventory movement, corpse extraction
and wallet-to-pile or pile-to-wallet steps. Primary-backend PC corpses enter
durable deferral, but staff NPC-corpse resurrection and any non-primary fallback
can reach the direct branches. Unmaking similarly defers PC corpses while its
non-PC path directly releases children and extracts the corpse. Corpse portal
also moves a live corpse between rooms without a typed owner result. The
legacy wallet routes now explicitly include coin-pile room publication and
extraction.

All 29 previously unmapped item sites in `src/item/enhance.c` are now linked.
Ordinary enhancement debits a fee, publishes a new item, then extracts the old
source and usually its carried donor; Chaos pouch mode retains the donor.
Modifier enhancement changes the source item's affected fields before its fee
debit and then consumes an essence. Superior enhancement debits cash, consumes
multiple materials and changes source fields in sequence. Its Chaos pouch
generated-use update happens after the fee and upgrade; failure only reports an
alert, leaving the upgrade in place without matching counter evidence. These
effects need one fenced root each. Template reads for base modifiers, material
names, target planning, descriptions and boot indexing are provisional.
Separate world-source grants issue turkey gear, NPC death essence and reset
fallback materials to NPCs.

All 31 previously unmapped item sites in `src/core/files.c` are now linked.
Serializer prototype reads are temporary, while `write_one_object` can assign a
UID to the passed live item when persistent-UID output is requested; that
assignment is an admission candidate outside a typed root. Flat-file terminal
save unloads the live inventory after authority save even if the account
character projection save fails and merely alerts. SQL terminal save stages
equipment, restores it on failure or ordinary save, and unloads after terminal
success. Legacy item restore and single-item decode can retain a newly generated
UID when the saved record lacks a UID flag, so active-epoch publication needs a
selected identity check. Pet save temporarily unequips and re-equips gear.
Compiled rent confiscation helpers have no in-tree callers; separate `#if 0`
child movement and pet extraction blocks are nonexecuting candidates.

All 27 previously unmapped item sites in `src/cmd/actmove.c` are now linked.
Breaking a tracked player key submits typed destruction and removes the live
object only in the committed callback, while the NPC or missing-UID branch
still extracts directly. Movement creates temporary Path of Frost ice in a
room, and opening a faerie bag or turkey innards grants a fresh random reward
before consuming its carried input. Rejected random templates and ice without
a valid room are unpublished candidates. All three lockpick break paths
extract held picks directly. Dragging an item, including a player corpse,
changes its room without an owner transaction. The latter direct mutations
need exact UID, source and room/owner proof before activation.

All 24 previously unmapped item sites in `src/item/storage_lockers.c` are now
linked. Enter, save, sort and rollback relink retained items through a
temporary locker character, chests and the room; those are projection
candidates requiring exact saved UID, locker/chest owner and coin-pile proof.
Chest fixtures are created from templates and later extracted, while their
destructor can spill remaining private contents into the room or limbo. A
new locker unconditionally extracts leftover objects in a reused room, and
locker exit moves room corpses to the exit room. An artifact found on the
locker floor is returned directly to the player. These are separate custody
routes, not evidence that all locker objects are temporary. The access check's
temporary restored character is unloaded without retiring selected custody.

All 18 previously unmapped unique item sites in `src/cmd/actnew.c` are now
linked. Combat disarm keeps a weapon under the same owner while changing its
equipment slot. Making a lock consumes a carried template after changing the
target container or door, and making a key changes an existing blank key and
container lock in place; those payload changes needed explicit routes because
the lexical item-call scanner does not see them. A held pick can break after
key shaping. Throwing a potion can destroy it, drop it in a room or consume it
after spell effects. The cast path unequips before the spell loop: an over-level
early return can leave a held potion detached, and successful remix retains a
carried potion. These are unresolved exact-UID and rollback requirements.

All 20 previously unmapped item sites in `src/magic/spell_conjuration.c` are
now linked. Active-PC branches for minor creation, flame blade, shield, food,
doom blade, mandrake consumption and sticks-to-snakes arrows use the shared
typed item owner, while NPC or inactive branches still mutate directly. This
conditional path is recorded in the matrix without counting the entire spell
as a qualified gameplay producer. Channeling directly creates and later
destroys a room avatar orb. Failed typed grant candidates are freed before
publication, and committed sticks-to-snakes arrows are removed from live
state after the batch result. The fallback arrow loop detaches selected arrows
first and stops consuming them if the victim dies early, potentially leaving
remaining detached objects. No spell path has a real player journey here.

All 49 current direct-SQL lexical sites are linked to named routes. Ten added
rows distinguish combat reward SQL, legacy auction pickup and compensation, collector SQL,
item repository custody/payload, corpse lifecycle, death restitution, snapshot
quarantine, and saved-item store/delete. Existing routes now link their SQL
subwrites, including auction settlement/claims, currency apply, SQL reset and
saved-item recovery retirement. These links describe the native writer and its
transaction boundary; they do not certify balanced postings or playable use.
The retained `auction_pickup_legacy` definition subtracts a pending claim before
submitting an asynchronous wallet credit; an immediate failure or later rejected
callback restores the claim with separate direct SQL. No in-tree caller reaches
that legacy definition, and it must remain closed unless exact claim, credit,
rollback and retry identity are proven under one root. The active pickup uses a
typed auction submission path, whose playable accounting proof is still pending.
Snapshot quarantine retains disputed item
custody; saved-item row deletion is not by itself item destruction. The SQL item
repository is a component of an owning item root and is not counted as another
schema-2 gameplay producer.

Seven matrix routes have a schema-2 gameplay producer and one SQL coin
component has balanced postings. That component has no qualified playable
dispatch/publication path. The matrix reports `coverage_complete=false` and
`playable_release_status=BLOCKED`; unsupported direct writers must refuse
under an active epoch. For per-route backend, authority, source/sink and
blocking details, use `writer_coverage_matrix.json`.

The sampled workload consists only of synthetic reconciliation fixtures,
component harnesses and disposable schema/backup fixtures. **Real player
journeys: none qualified.** No 1,000-root mixed workload, latency percentile,
storage-growth sample, checkpoint recovery time or release-host 32 MiB audit
memory/time measurement has been recorded. The development-host synthetic
audit-size sample above cannot certify the release-host mixed-authority budget.
The limits and measurement method are specified in
[AUDIT_OPERATIONS.md](AUDIT_OPERATIONS.md); the release performance gates remain
unverified.

## Remaining release gates

1. Finish semantic classification of all current writer candidates; attach
   executable evidence or explicit active-epoch refusal to every real route.
2. Produce complete, fenced SQL and flatfile native/evidence exports. Run the
   read-only reconciler against each backend after fresh install, upgrade,
   restore and injected evidence loss. A JSON fixture cannot attest its own
   completeness or operator access.
3. Make the `0036_economic_sql_activation_receipt.sh` verifier executable in
   Git and resolve the clean-head runtime metadata/state mismatch. Rerun the
   full MySQL and MariaDB wrappers on the integrated commit. Qualify flatfile
   journal interruption, restore, source/UID dedupe and receipt replay with
   actual domain roots. Resolve any later build or harness failure on that commit.
4. Run both server builds and the focused gameplay/fault matrix for both
   backends, including live publication, reconnect and player-visible state.
   Measure the stated operation, latency, storage, checkpoint and audit budgets.
5. Publish a report for one exact integrated commit with the actual workload,
   route results and unsupported paths. Only then can a separate deployment
   decision be considered.

Correction and restitution submission are deliberately absent from the
read-only audit tool. A later operator writer needs authenticated authority,
expected-state checks and original-operation linkage; audit exceptions never
cause an automatic balance or custody adjustment.
