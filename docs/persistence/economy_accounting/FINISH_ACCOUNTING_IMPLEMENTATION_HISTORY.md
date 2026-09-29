# Finish accounting implementation history

Dated checkpoints preserved from the implementation branch. The current work order and release gates are in [the active plan](FINISH_ACCOUNTING_PLAN.md). Earlier claims below describe the commits and environments named in each checkpoint.

---

# Finish accounting, saves, item custody, and death recovery

Date: 2026-09-28
Planning branch: `finish-accounting`
Planning base: `main` at `d686d4c70384af9012673fa7c4db5cba356c471f`
Status: implementation plan; accounting release remains blocked.

The checks and counts in the planning-base table are a dated baseline. The
checkpoints below record later fixes; use the current generated writer matrix
for the latest route count and release state.

## Outcome and delivery order

Deliver a reproducible server build whose supported gameplay saves survive
disconnect, copyover, database interruption, and process restart. Every admitted
item must retain its identity, payload, custody, and history; death and corpse
recovery must reach a durable, player-visible outcome without duplication or
silent loss. Complete the money and item accounting implementation with
independent reconciliation and guarded activation.

SQL is the first delivery target, consistent with the existing delivery
decision. Produce these separately identifiable milestones:

1. **Stable SQL gameplay candidate:** saves, normal item movement, quest reward
   recovery, death/corpse handling, and restart journeys pass on the candidate
   binary. Accounting stays in its existing guarded mode until its activation
   requirements pass.
2. **Complete SQL accounting candidate:** all declared supported gameplay
   writers have durable accounting, publication, and recovery proof; remaining
   unsupported features are explicitly documented and refuse before mutation.
   Core saves, items, quests, and death cannot qualify merely by refusing them.
3. **Full feature completion:** flatfile implements the same supported behavior
   and passes native recovery tests. Flatfile work follows the SQL milestones;
   a successful flatfile compile alone does not establish parity.

This document sets the execution order for this branch. The detailed
[remaining requirements](REMAINING_REQUIREMENTS.md) and
[existing domain plans](DELIVERY_PLAN.md) remain useful contracts. Their older
commit-specific gap statements must be checked against current code.

## Current evidence and concrete gaps

Checks below were run read-only on the planning base on 2026-09-28. No server
build, database migration, or gameplay journey was run for this planning task.

| Check or inspection | Current result | Consequence |
| --- | --- | --- |
| `python3 scripts/validate_economy_accounting.py` | Fails: economic writer census drift | Review changed/new sites and repair the registry before trusting coverage. |
| `python3 scripts/generate_economy_writer_coverage.py --check` | Fails: generated matrix is stale | Refresh it only after semantic review. |
| `python3 scripts/validate_economy_accounting.py --release` | Fails: writer has no executable evidence | Mapping source sites is insufficient for release. This is the first reported failure, not an exhaustive list. |
| In-memory coverage calculation | 854 registry routes; 2,784 lexical occurrences; 2,726 unique sites; 2,631 mapped at current locations; 95 unmapped | These are scanner counts, not 95 confirmed missing implementations. Source moves also affect the count. `coverage_complete=false`; release is `BLOCKED`. |
| `critical_command_repository_apply_from_pool` and reconcile | Bank, supported coin/item, collector, and shop accounting envelopes are routed | The earlier “pooled bank-only” plan statement is obsolete. Verify and extend existing dispatch only where an actual route is missing. |
| `player_snapshot_repository.c` | Existing UID/payload custody reconciliation, revisions, and topology repair are present | Build on those checks; establish runtime race and restart evidence. |
| `fight.c` and save pipeline | Death retries, terminal revision fences, disputed-disposition persistence, and release paths exist | Prove liveness and recovery; do not assume the older missing-recovery report still describes the code. |
| `quest.c` and `item_movement_transaction.c` | Item, cash, eligible skill, solo XP, and SQL-primary group XP rewards enter durable recovery with the offering operation ID; skill eligibility and per-recipient XP amounts are frozen at admission. XP receipt mask and progression snapshot commit together. | Focused synthetic retry coverage passes; SQL restart, group XP runtime qualification, and operation-safe replay of spell component effects remain open. |
| `scripts/economic_sql_audit_snapshot.py` | Exports mapped balances for wallets, banks, coin piles, open auction escrow, pending claims and shop treasuries, but remains incomplete for holding lifecycle/origins and full UID history | Finish native lifecycle and provenance reconciliation before using the export as a whole-economy certificate. |

The legacy quest reward callback still depends on finding a live quest NPC.
Durable item and cash completions use their retained continuation even if the
NPC disappears after publication; unsupported effects leave the obligation
pending with player feedback. The durable offering adapter keeps its 14-root
bound. Solo XP now uses continuation v4 and the snapshot receipt path described
below; group XP is refused before consumption in flatfile/fallback mode. SQL-
primary mode admits the frozen group award through the durable entitlement
path, including while accounting is active. Each in-world character, including
linkdead characters, receives its award during the completion pass.
The GCC 13.3 SQL server build and client-free flatfile
server build pass, and the post-publication-ack quest reward restart journey
passes on a synthetic item-and-cash completion. SQL server-level restart,
crash-before-publication, XP recovery, and save/custody race journeys
remain open. The focused player-save pipeline, worker, bandage custody contract,
and terminal-save safety tests pass. The pipeline test's obsolete expectation
for `sql_save_player_shapechanges` inside `writeCharacter` was removed because
that write now belongs to the SQL serializer; these focused checks do not close
the runtime save/custody race gate.

The historical [release report](RELEASE_REPORT_2026-09-27.md) records dual-engine
schema qualification through migration `0044` and maintained SQL/flatfile builds
on earlier commits. Preserve that evidence with its original commit identity.
It does not certify this branch's final integrated binary. The partial
[qualification checkpoint](QUALIFICATION_CHECKPOINT_2026-09-29.md) records the
quest/save build and focused journey evidence without closing the open SQL
restart or save/custody race gates.

## Step 0 checkpoint: source inventory repaired

The planning-base audit rows above record the starting failure. The current
source-reviewed registry and generated matrix list 861 routes. The latest
generated census contains 2,789 lexical occurrences across 2,731 unique sites;
all 2,731 now map to the registry. Reanchoring the bank-load projection and three
SQL season-reset deletion sites removed source-line drift without changing route
classification. Coverage remains incomplete because source mapping does not
prove runtime behavior or executable evidence.

### Reconcile moved and newly visible writer sites (2026-09-29)

Recent quest recovery edits shifted source lines and added a second disappearing
quest-NPC cleanup path. I reanchored the source census against the current
functions and attached its three new lexical hits to the existing
`quest.disappearing_npc_cleanup` route after checking both call paths. The
generated matrix now reports 2,804 lexical occurrences across 2,746 unique
sites, all mapped to the source-reviewed registry. The regular validator and
generated `--check` pass. Two focused hard-coded route assertions had drifted
with the source lines; after updating them, both focused contract cases pass.
The full source-contract suite was not rerun after that repair. Release
validation still stops at missing per-writer executable evidence.

### Export pending auction-claim source allocations (2026-09-29)

The SQL audit cut now includes bounded `economic_pending_claim_source` rows with
source operation, slot, stable pending-claim account key, beneficiary, amount
and consumer operation. It reports open/consumed totals and flags missing or
mismatched claim lifetime mappings. The reconciler now validates each source
root's committed outcome and lineage, mapped claim credit, unchanged higher
denominations, posting count and zero-sum value. It also requires open source
allocations to equal the native aggregate. Consumed allocations are checked
against their consumer roots' balanced postings and claim debit, including
cross-epoch operations. Linked post-baseline create events also supply a
revision-zero creation origin when no baseline origin exists. The SQL cut now
exports all retained item references across the selected lineage, links each to
its exact ownership event, and exports ownership-ledger events for UIDs anchored
by those references, baseline origins, or current native rows. Unreferenced
events for tracked UIDs are exported separately, while the reconciler checks
their complete post-origin revision chain and final native position. A
database-wide distinct-UID census also reports ownership histories that have no
origin, reference or native-row anchor in the selected audit. The disposable
MariaDB fixture exercises aggregate mismatch, altered source and consumer roots,
a post-baseline created UID, an unreferenced destroy event, and an unanchored UID.
Completeness across legacy claim writers, logical source-event attribution and
global UID lineage assignment remain open. The SQL audit now reads realized
prices for shop buy/sell roots across the lineage when the operation schema
provides them, checks coverage and reports a missing column or committed price
instead of estimating. The follow-up now adds the nullable price column to fresh
SQL schemas and guarded migration 0046, and records the frozen payload price on
committed SQL shop buys and sales in the same operation transaction. Rejected
shop roots retain a null price. The focused SQL shop fixture checks both cases;
dual-engine full-schema fingerprint qualification remains open. Retired mapping
lifetimes now retain their root operation, complete effects and postings across
epochs. Reconciliation requires a committed same-lineage root, exact effect and
posting agreement, zero terminal account balance and balanced value. Current-
epoch retirements are additionally tied to the opening origin and checked for
absence of a native holding. The disposable SQL fixture now also verifies that
the root evidence is exported for a prior-epoch retirement; when mapping
creation evidence is available, each retirement must also retain the mapping's
original native identity. A post-baseline
ordinary account opening now derives a zero/revision-zero origin only when its
first committed effect agrees with the retained mapping's creating operation;
the fixture covers a newly mapped wallet. Other account-opening paths remain
explicitly incomplete. A registry-backed policy census also counts committed
prior-epoch operations that lack a required source event, without importing all
historical operation rows into the current-epoch audit tables.
The old `quest.artifact_turnin` entry records the quester entry point without
claiming that its removed direct extraction still runs. The inactive legacy
`quest_completion` path consumes from NPC carrying, not player carrying.

`python3 scripts/validate_economy_accounting.py`, the generated-matrix
`--check`, the writer-site contract (2,695 checks), the existing 49 writer
route tests, and two focused new-route tests passed. The release validator still
refuses `writer has no executable evidence`; the matrix keeps
`coverage_complete=false` and `playable_release_status=BLOCKED`. Lexical mapping
does not close any runtime gate. The SQL server built with GCC 13.3 using
`make -C src -j4`. After pinning the four executable area-script checkouts to
LF in `.gitattributes`, `make world` passed in the maintained local audit
container. The focused quest-offering and shared publication retention harnesses
passed. A later checkpoint completed the process-restart reproduction below.

The normal live flatfile quest journey grants and cold-loads an item reward
with its original UID after three committed offerings. At the original baseline,
the post-publication-ack crash journey showed the separate loss window: offerings
stayed consumed while the reward was absent after restart. The recovery scaffold
below now passes that journey for one item-only flatfile completion. A read-only
inspection of issue #10's isolated SQL playtest found an active bandage custody
row absent from the rejected save snapshot. The durable bandage path now retires
custody before healing, and a disposable MariaDB save/restart journey passes. The
already stranded player's row requires a separate, reviewed recovery action; no
player data was changed. Broader quest reward recovery and accounting route
evidence remain open.

## Step 1 checkpoint: typed NPC quest turn-in

The durable static NPC item-offering route now uses
`item_transfer_reason::quest_turnin` through validation, SQL accounting,
projection retirement, publication, and the flatfile capability check. The
flatfile route accepts only an accounted player batch with a quest-offering
continuation. Durable item and cash-only rewards now share the persisted
recovery pipeline after publication; XP and skill rewards remain unsupported by
that recovery path at this checkpoint. The 2026-09-29 continuation-v3 update
below adds eligible skill recovery; XP remains unsupported.

On 2026-09-28, `python3 scripts/validate_economy_accounting.py` passed its 14
contract fixtures and 861-route inventory check. The generated matrix `--check`
passed with 2,789 lexical occurrences, 2,731 unique sites, and zero unmapped
sites; release remains `BLOCKED`. The focused quest test was attempted, but the
host lacks `g++`; the server build and `.clang-format` verification also could
not run because `make` and `clang-format` are unavailable. No C++ runtime proof
is claimed for this checkpoint.

### Frozen quest-credit and skill continuation (2026-09-29)

Quest-offering continuation version 2 now freezes the stable zone-story
definition ID, zone, direct character name/level/racewar, credited player IDs,
party size, strongest party level, room, completion time, offerings and reward
terms before durable admission. Recovery records the captured recipient set
through `record_authoritative_completion`; a later group change cannot change
the credited players. The context and recipient list are bounded at 64 members,
and a group that exceeds the bound or a trusted direct completer is refused
before offering consumption.
Version-1 continuations remain readable and keep their prior recovery behavior.

The focused quest-offering harness verifies that the recipient list survives a
group change after admission. The transfer-continuation compatibility harness
checks version-1 decoding, version-2 frozen-credit decoding and duplicate-PID
refusal; a C++20 syntax-only compile of `quest.c` passed with the available
MySQL headers. A complete local SQL `make -C src` build with GCC 12.3 and an
isolated `bin/gcc12` output tree also passed after correcting a missing aggregate
initializer warning. The maintained GCC 13 SQL build, client-free flatfile
build, and SQL process restart journey have not been rerun for this change.
Removed or changed quest definitions can still leave a frozen completion
pending because the zone-story runtime requires the retained definition ID to
exist in its current catalog; that catalog-transition policy remains open.
At the continuation-v2 checkpoint, durable XP effects remained unsupported and
were refused before active-accounting turn-ins consumed items. The later
continuation-v4 implementation note records the XP receipt work that closes
that specific admission gap for solo awards.

Continuation version 3 records whether each skill reward met its level
requirement at admission. Recovery applies an eligible skill as an idempotent
`learned = 1` state change and requests the existing player-skills snapshot; it
does not acknowledge the quest obligation until that save revision is durable.
Version 1 and 2 decoding remains available. The focused quest recovery harness
covers admission metadata, save-fence acknowledgement, and replay after the
skill is already learned. The continuation compatibility harness covers v1/v2
and v3 decoding and rejects reward flags on non-skill goals. Both focused checks
and the full isolated GCC 12.3 SQL build pass at `9a4a4ebea`. XP still needs a
separate idempotent progression receipt; SQL restart and maintained GCC 13 and
flatfile builds remain open for this change.

### Quest reward recovery scaffold (2026-09-28)

Player materialization now dispatches loaded pending obligations through the
pre-entry item grant queue. Item rewards use the same deterministic logical
source ID as the original reward path, and quest completion tracking uses a
stable transaction ID derived from the consumed offering UID. An obligation is
acknowledged only after all supported item and cash callbacks succeed and the
current quest definition matches the retained reward terms. Cash child commands
use an operation ID derived from the offering command, so retries can attach to
the same reward. Unsupported skill and XP effects stay pending; item and cash
rewards in a mixed obligation may still be delivered, but the obligation is not
acknowledged. The flatfile item-only post-publication-ack crash journey now
passes: offerings stay consumed, exactly one item reward appears after cold
restart, and the flatfile reward obligation is acknowledged. SQL crash recovery, cash crash/replay behavior, skill, XP, frozen
group-credit, and catalog-change behavior remain open.

The maintained GCC 13.3 server build and focused `test_durable_quest_offering.py`
passed. The writer census and generated matrix were refreshed to 2,801 lexical
occurrences and 2,743 mapped unique sites, with zero unmapped sites; release
remains `BLOCKED`. The focused test covers turn-in publication, item-only ack timing, deterministic
cash child IDs, and mixed-reward retention. The separate
`run_quest_reward_ack_crash.py` journey passed on the client-free flatfile server
for the committed item reward case, including persisted obligation acknowledgement. SQL process-crash recovery and the remaining
reward types still need their applicable acceptance journeys.

### Quest wallet reward accounting scaffold (2026-09-29)

Recovered quest wallet rewards now have a typed `quest_reward` issuance plan on
the existing currency command path. The deterministic reward child operation ID
is also the source-event identity; no reward payload format or repository was
added. SQL and flatfile bank transaction paths can decode, prepare, retain, and
verify the wallet, bank, and issuance accounts under the same command identity.

Live item/cash-only quest completions now pass the coordinator operation ID to
the existing recovery pipeline. This applies the same deterministic item source
IDs and typed cash child operations used after reconnect, then acknowledges the
offering only after every supported reward callback succeeds. Cash-only and
item-plus-cash turn-ins can therefore use the typed wallet path while accounting
is active. If the quest NPC disappears after offering publication, these rewards
still dispatch from the retained continuation. Unsupported reward effects leave
the obligation pending and show the player a recovery notice. Mixed XP/skill
rewards keep their previous behavior and remain unacknowledged for recovery work
that is not implemented yet.

The maintained SQL build and focused currency-adapter and gameplay-authority
tests passed. A separate client-free flatfile build also passed. The focused
post-ack crash journey now includes a 1,000-copper wallet reward and passes with
the reward obligation acknowledged after restart. Its first run exposed a
flatfile legacy guard treating an empty `economic-evidence` directory as an
initialized authority, then retrying the missing control record as `EILSEQ`.
Legacy wallet commands now consult the accounting control only when that
directory contains state; any nonempty but unreadable authority still fails
closed. This journey exercises the inactive flatfile legacy path because
flatfile accounting authority is not installed at runtime. It proves durable
quest cash recovery there, not typed double-entry publication. The callback
harness and maintained SQL build now cover the live typed route, but no SQL
process-crash journey has run. The focused currency-adapter and
gameplay-authority tests cover the typed wallet issuance plan. At this
checkpoint replay-safe XP, SQL restart qualification for skill recovery, frozen
group credit across the reward path, and catalog-change behavior remained open;
the later continuation-v4 note records solo XP progress, while SQL restart and
group credit remain open.

### Replayed quest publication owner (2026-09-29)

Coordinator replay now reconstructs the item-movement publication owner for a
durable quest-offering command from its serialized quest continuation. Once the
player is available, that owner releases the publication fence; the separately
persisted reward obligation remains responsible for item and cash delivery.
Unknown publication-required item commands fail startup closed because their
process-local callbacks do not yet have a replayable continuation. This was
superseded by the later unresolved-publication retention checkpoint below. The
focused quest-offering regression and maintained MariaDB server build passed.
The server-level SQL process-crash journey has not run, formatting could not
run because `clang-format` is unavailable in the maintained container, and
release remains blocked.

The SQL item-transfer harness now uses the live `quest_turnin` reason and
asserts durable destroyed custody. For the accepted offering it kills a separate
writer process immediately after the SQL commit, then reconnects and verifies
the exact pending reward obligation before reopening the obligation reader. This
focused harness passed against a disposable MariaDB 10.11 database with GCC
13.3. It verifies the process boundary and repository replay, but is not yet a
server/gameplay crash journey.

### Replayable item publication for forced drops and soulbind (2026-09-29)

The durable movement payload now restores forced weapon drops from their
committed room custody and restores soulbind publication from a typed
continuation that retains the replace-existing decision. Soulbind can finish
when either transfer participant loads; recipient metadata and its required
save still gate acknowledgement. The focused quest/publication source contract,
item-transfer compatibility test (including soulbind continuation round-trip
and invalid-flag rejection), and maintained MariaDB server build passed. SQL
process-crash execution remains open. Spell-component publication still carries
a process-local function pointer and effect context, so those effects still
require stable durable continuations. Until then, committed commands remain
fenced and player-visible as pending recovery rather than preventing server
startup.

### Keep unsupported item publication replay online (2026-09-29)

Replayed item commands without a typed publication restorer now reconstruct a
blocked publication owner instead of aborting coordinator startup. The original
entity fences and command record remain retained, the affected player receives
a recovery notice, and a replayed command that did not commit can safely release
its fence. This improves restart availability while preserving the unresolved
business effect for staff recovery. Pre-continuation soulbind journal records
also use this retained path; they cannot safely infer the old replace-existing
choice. This does not qualify spell-component effect
recovery or the SQL crash journey. The focused quest/publication source contract
and maintained MariaDB server build passed.

### Pending auction credit on bids (2026-09-29)

Accounted bids now apply a bidder's pending auction credit before wallet funds
and record the exact claim debit beside the escrow funding. Fully credit-funded
bids omit the invalid zero-value wallet posting. The flatfile auction repository
harness now covers spending the outbid claim source, preserving its consumer
operation, and recovering an interrupted auction money claim. The focused
flatfile repository test, bid accounting unit test, and maintained SQL server
build passed. SQL auction replay and live gameplay coverage remain open; this
does not change the blocked release status.

### Export SQL coin-pile balances in the audit cut (2026-09-29)

The bounded SQL audit snapshot now reads each coin UID's owner, revision, state
and four denomination values from the persisted item payload in the same
read-only consistent cut. The decoder checks the item codec structure, UID,
coin vnum/type, nonnegative amounts, and payload bounds. Missing payloads remain
explicit in `native.coin_pile_coverage`; malformed payloads fail the export.
These coin balances are diagnostic data only: the reconciler still does not
compare them with accounting postings, and the exporter remains incomplete for
escrow, claims, treasuries, full UID history and other listed gaps.
The guarded `run_economic_sql_audit_snapshot_mysql.py` harness passed against a
disposable MariaDB 10.11 schema, including live/tombstone payload extraction,
malformed-payload refusal, and the existing consistent-cut and fail-closed
checks.

### Export remaining mapped SQL holding balances (2026-09-29)

The bounded read-only audit cut now includes mapped coin piles, open auction
escrow, pending auction claims and non-null shopkeeper cash alongside wallets
and banks. It exports each current balance using the stable mapping lifetime as
the account key and the native revision where available. A pending claim whose
pickup row has not been created is represented as zero only if its mapped player
still exists. Missing coin-pile payloads and null treasury balances remain
invalid. The mapping census and reconciler accept coverage counters for all six
ordinary account kinds.

The focused reconciler suite and guarded disposable audit runner passed; the SQL
runner used MariaDB 10.11 and verified all six holding classes in one snapshot.
Changing a mapped pile denomination away from its opening witness raised the
expected `stale_native_balance` exception, showing the reconciler checks its
current value against the captured posting history. This still does not prove
complete pile mapping or postbaseline creation origins, lifecycle/source
attribution for escrow, claims or treasuries, or unreferenced UID history. MySQL
and live authority qualification remain open.

## Invariants to preserve

- **One authority for each state:** SQL native rows and the existing command/item
  authorities decide ownership and balances. Player snapshots, Redis, live
  objects, and saved-item projections cannot overwrite a newer committed result.
- **Durable command outcome:** native effects, required accounting evidence,
  source claims, receipt, and recovery obligation share the appropriate
  transaction. An ambiguous reply reconciles the original operation ID.
  Reusing that ID with changed intent fails.
- **Snapshot ordering:** stale save completions cannot release a newer terminal
  fence or resurrect transferred/consumed assets. A checkpoint cannot discard
  journal data before the promised durability boundary.
- **Stable items:** one admitted UID, one valid current location or a tombstone;
  acyclic containment; preserved parent/root/equipment information; full
  serializable properties. Loading a saved object is not a new issuance.
- **Balanced money:** denomination vectors remain exact, conversion is checked,
  and each value-changing root balances against real holdings or a named
  issuance/expense/opening/restitution policy. Item custody does not imply an
  invented monetary valuation.
- **Player recovery:** after a committed result, reconnect or restart can finish
  publication without the original pointer, NPC, or in-memory callback.
  Persistent conflicts retain evidence and expose an authenticated recovery
  state; retries must not silently drop the player's assets.
- **Death boundaries:** wallet conversion, corpse custody, terminal character
  state, and later resurrection must be recoverably linked. Death and a later
  resurrection are distinct operations; preserve the existing critical
  boundaries instead of promising one transaction across both events.

## Implementation sequence

Each increment includes a focused regression, relevant executable/database
proof, a maintained build after C/C++ edits, and an update to route evidence.
Use small reviewable commits on this branch. Implement the smallest missing
piece exposed by the test; keep working infrastructure.

### 0. Establish a trustworthy baseline

**Work**

- Freeze the source commit, compiler/container image, build flags, backend,
  schema version, world fixture, and test commands in a new qualification
  report. Use a disposable SQL target and synthetic test characters.
- Review the census delta, distinguishing moved anchors from new quest writers,
  real mutations, projections, staging cleanup, and unreachable code. Update
  `writers.json` and regenerate the matrix without marking unproved routes
  qualified.
- Build the maintained SQL server and tools with a supported C++20 compiler.
  Generate the world required by gameplay tests. Record any environment
  adaptation, including shell line endings and executable permissions.
- Run focused save/item/death baselines to turn suspected faults into
  reproducible cases. Existing schema evidence can be reused until relevant
  changes invalidate it; final integrated qualification is still required.

**Files:** accounting registry/matrix and report; affected test fixtures and
build scripts only if a concrete baseline failure requires a correction.

**Exit:** reproducible SQL build and boot; ordinary census/matrix checks pass;
runtime failures have named reproductions. The release check may remain red
until later phases supply all required evidence.

### 1. Close save and business-completion crash windows

**Work**

- Trace manual/autosave, camp/quit, disconnect, death, shutdown, and copyover
  through capture, journal sync, database apply, live publication, and terminal
  release. Retain exact revision fences, bounded queues, and no blocking
  database/filesystem work in game-thread capture or completion.
- Exercise a save captured before an item/coin operation but applied after it,
  delayed/out-of-order completions, journal replay, DB disconnects, and queue
  pressure. Repair actual stale projection or custody mismatches without
  weakening ownership checks. Verify payload properties as well as UIDs.
- Add a process-crash reproduction for quest consumption before reward
  completion. Record quest completion identity and the exact reward obligation
  durably with accepted consumption. Prefer the existing domain transaction;
  where reward work is already a separate operation, persist its deterministic
  continuation/entitlement in that commit and dedupe each child.
- Recover quest status, item/cash rewards, and XP consistently after reconnect or
  restart. Snapshot replay cannot award XP twice. NPC disappearance or player
  absence cannot erase a committed reward obligation. Failed admission leaves
  offerings untouched; failed later delivery remains visibly pending.
- Keep publication acknowledgement from retiring the only recovery obligation.
  Audit other business callbacks at the same seam for the identical risk;
  expand only where an actual callback carries durable business effects.

**Files:** `src/player/player_save_pipeline.c`,
`player_save_journal.c`, `player_save_worker.c`,
`player_snapshot_repository.c`; `src/item/item_movement_transaction.c`;
`src/world/quest.c` and existing quest state/reward repositories;
critical coordinator/journal only if the shared recovery seam requires it.

**Exit:** save/replay never rolls back committed money or item state; terminal
release follows its documented durability requirement; an accepted quest
consumes offerings once and completes or retains exactly one recoverable reward
obligation across every tested interruption.

### 2. Finish ordinary money and item authority

**Work**

- Qualify the real coordinator/pool path for bank, wallet, coin-pile, and item
  commands, including replay after an epoch or policy changes. Reuse current
  typed accounting owners and their native ledgers.
- Complete get/drop/give/put/equip/remove, nested containers, lockers, pets,
  room/world recovery, and same-owner topology changes. Preserve original UIDs,
  payloads, and ordered accounting references for admitted events.
- Cover coin pile creation, split/merge, pickup/drop, change, shared banks,
  player transfers and group splits with exact denominations, account
  lifetimes, owner revisions, and balanced value effects.
- Bind creation/consumption to durable logical source identities that survive
  retries with a new operation ID and process restart. Separate template
  allocation and staging cleanup from actual admission/destruction.
- Reject duplicate UIDs, conflicting claimants, impossible/cyclic topology,
  missing payload, negative/overflowing money, and changed-ID replay before
  publication. Restoration reuses identity and retains evidence on conflict.
- Complete saved-item source retirement and destination acknowledgement so
  interruption cannot both replay the source and expose a second live copy.

**Files:** `src/economy/currency_transaction.c`,
`coin_transfer_accounting.c`, `item_transfer_accounting.c`;
`src/item/item_transfer_repository.c`, `item_movement_transaction.c`;
existing SQL transaction adapters, UID allocator, and world recovery pipeline.

**Exit:** supported ordinary journeys preserve exact balances, payloads and
custody after save/reconnect/restart; each admitted item event has its required
reference and each supply event its source identity. Native rows and history
agree without snapshot-based ownership repair.

### 3. Qualify the entire death and corpse lifecycle

**Work**

- Model and test the existing progression: death admitted -> money conversion
  resolved -> corpse handoff committed/published -> exact terminal save
  acknowledged -> character released. Resume persisted obligations at each
  interruption; do not create a second corpse or retry under a new identity.
- Preserve equipment, nested inventory, item properties and coin-pile UID/value.
  A pending movement or reward must resolve before the death snapshot excludes
  its assets. Validate whole batches before publishing any live transfer.
- Test healthy death and disputed custody separately. A transient failure
  retries; a durable dispute retains the refused payload and ownership evidence,
  completes the permitted terminal disposition, and becomes visible to the
  authenticated account. Once dependencies recover, no manual staff command
  should be required merely to leave a dead character stuck online.
- Qualify corpse save/load, movement/hauling and nested storage, partial loot,
  coin loot, decay/destruction, resurrection, raise-dead/follower creation, and
  relevant arena/trusted/NPC exceptions under their existing gameplay rules.
- Resurrection returns only eligible assets still held by that corpse; it
  cannot reclaim already-looted items or consume a coin pile twice. Preserve
  corpse and death lineage through subsequent lifecycle operations.
- Test simultaneous loot/resurrection/decay, replayed spells, missing/stale corpse
  revision, and restart after each durable leg. Recovery/restitution requires
  expected-state checks and original-operation linkage.

**Files:** `src/combat/fight.c`;
`src/persistence/corpse_lifecycle_{command,repository,transaction}.c`;
`src/magic/spell_corpse_lifecycle.c`; death conflict/recovery/restitution
repositories and the player save pipeline.

**Exit:** healthy and disputed real-PC journeys reach their correct terminal
state unassisted; original fixture UIDs and money are accounted for before and
after loot/resurrection/restart. Every admitted lifecycle event has exact
native/evidence linkage, with no duplicate corpse, orphan item or unfunded coin.

### 4. Complete remaining economic producers and compound operations

**Work**

- Finish supported shop/finite-keeper, collector, auction escrow/bid/outbid/
  settlement/claims, crafting/refining, spell inputs/outputs, world/reset/loot,
  quest/boon/epic rewards, special procedures, taxes/costs, and staff grants.
- At each existing critical boundary commit all coupled native effects, money
  postings, item references, source claims and receipt together. Preserve the
  original random outcome through retry; technical failure rolls back, while
  intended gameplay failure follows the existing consumption rule.
- Give NPC holdings, finite keepers, pending claims, and interrupted game rounds
  durable lifetimes before accepting those writers. Reuse current semantics;
  explicit gameplay decisions still unresolved are recorded before implementing
  the affected route.
- Test unsupported-route guards through executable entry points. A guard is
  containment for an intentionally unsupported feature, not completion of
  normal required gameplay.

**Exit:** every reachable economic writer is semantically classified and has
runtime evidence for supported behavior or an intentional pre-mutation refusal.
A green scanner alone cannot close this increment.

### 5. Finish independent audits, lifecycle, and activation

**Work**

- Extend the read-only SQL audit export to all native money classes and complete
  UID/event/reference history, with explicit source coverage, bounds, lineage,
  revision and consistency witnesses. Derive coverage from native stores;
  evidence rows cannot certify their own completeness.
- Reconcile native holdings against balanced roots; verify exact UID locations,
  origins, retirement, source dedupe and realized trade prices. Inject missing
  postings, duplicate UIDs, orphan references, unknown origins and evidence
  loss; each must produce a specific exception without modifying state.
- Qualify fresh install, upgrade, backup/restore, migration replay, boot
  compatibility, retention and account erasure on MySQL and MariaDB. Any new
  migration is additive, guarded and re-runnable where practical.
- Reuse the lifecycle owner, maintenance fence, activation receipt and boot
  checks. Prove incomplete coverage refuses activation; a complete local
  baseline does not create gameplay assets. Test runtime recovery, pause and
  evidence loss without allowing unjournaled writes.

**Files:** existing audit/reconcile scripts, registry, SQL source snapshot and
lifecycle/activation code, relevant lifecycle manifests and migrations.

**Exit:** complete native export and independent reconciliation pass; current
route evidence permits the release validator to pass; a disposable activation
and restart/pause exercise proves the actual runtime admission state.

### 6. Qualify the release binary and complete flatfile parity

- Build from one integrated commit with the maintained toolchain; identify the
  executable hash, schema and world fixture in the report.
- Run the regression suite, relevant isolated DB suites and real gameplay matrix.
  Rerun both engine schema wrappers after schema changes. Inspect current-run
  persistence errors and all outstanding/unpublished/disputed operations.
- Measure pulse latency, save/commit latency, bounded queue occupancy, memory,
  journal growth/checkpoint recovery, and audit duration at a declared workload.
  Set thresholds from the server's pulse interval, existing timeouts and queue
  caps; require no unbounded backlog and recovery after dependencies return.
- Then bring flatfile native transactions, source/UID dedupe, publication,
  terminal saves, death/recovery, audit export, journal interruption and
  backup/restore into parity. Use the existing flatfile authority.
- Publish a report distinguishing supported routes, intentional refusals,
  unresolved failures, actual test commands/results and historical evidence.
  Local builds and executable journeys decide readiness; do not wait for CI.

**Exit:** the stable SQL and complete SQL milestones have reproducible evidence;
full-feature completion additionally has equivalent flatfile evidence.
Production deployment or migration is a subsequent owner-authorized action.

## Required journey and fault matrix

| Journey | Assertions after reconnect and cold restart | Required fault points |
| --- | --- | --- |
| Save/autosave/camp/copyover | Latest acknowledged revision, exact payloads/UIDs, no rollback of committed balances | Delayed completion, journal append/sync failure, DB unavailable, crash after commit before acknowledgement |
| Get/drop/give/equip/nested storage | One owner, valid parent/root/slot, original properties, exact references | Two claimants, stale revisions, partial batch, interrupted publication |
| Quest completion | Offerings consumed once; status, XP and each reward delivered once or durably pending | Grant admission failure, NPC gone, disconnect, crash before/after publication acknowledgement and reward persistence |
| Wallet/bank/piles | Exact denomination vectors; balanced effects; one pile identity and source | Overflow, new-ID duplicate source, lost commit reply, retry/restart |
| Death/corpse/loot | One corpse/disposition; exact surviving UIDs and funded value; terminal release | Pending transfer, failed wallet conversion, custody conflict, lost reply and crash between death stages |
| Resurrection/raise/decay | Only eligible remaining assets restored/retired; lineage retained | Concurrent loot, stale/missing corpse, replayed spell, restart after commit before live effects |
| Shop/auction/craft/rewards | All coupled effects agree; frozen outcomes and claim entitlements survive | Failure after an intermediate child, offline recipient, duplicate source, restore |
| Audit/restore | Complete native coverage and specific exceptions; replay/dedupe survive restore | Missing evidence, extra UID/posting, erased aliases, interrupted upgrade/restore |

For each accepted operation check live gameplay, native rows, immutable evidence
and restart read-back. For rejected operations verify unchanged holdings and
custody. Keep source-contract tests as guard checks alongside executable proof.

## Starting validation commands and fixtures

Run these in the maintained Linux/Docker environment. They are planned gates,
not passing results for this branch. Provision only disposable databases;
some Python journeys require a fixture wrapper and are not standalone commands.

- Census: `python3 scripts/validate_economy_accounting.py`,
  `python3 scripts/generate_economy_writer_coverage.py --check`;
  release adds `python3 scripts/validate_economy_accounting.py --release`.
- Changed C/C++ lines: `./scripts/format.sh --check`; build:
  `make -C src`. Use the maintained Docker build where host dependencies are
  unavailable.
- Save/publication: `test_player_save_pipeline.py`,
  `test_terminal_save_safety.py`, `test_stable_terminal_death_request.py`,
  `test_publication_retention_runtime.py`, `test_durable_quest_offering.py`,
  and `test_player_save_item_reconcile_mysql.py` with its disposable fixture.
- Item authority: `test_item_transfer_accounting.py`,
  `test_universal_item_transfer_accounting.py`,
  `test_economic_accounting_item_reference_mysql.py`,
  `run_saved_item_recovery_journey.py`.
- Death: `test_corpse_creation_batch.py`,
  `test_death_item_custody_contract.py`,
  `run_player_death_disposition_mysql.sh`,
  `run_corpse_lifecycle_repository_schema_mysql.sh`,
  `run_death_resurrection_mysql_journey.sh`; extend the real-PC journey with
  accounting references, process interruption and unassisted dispute recovery.
- Accounting schema: `run_economic_accounting_schema_mysql.sh` with
  `ECONOMIC_ACCOUNTING_DB_IMAGE=mariadb:10.11` and `mysql:8.0`;
  use the existing typed domain SQL wrappers for changed domains.
- Final integrated SQL candidate: `make test-all`, `make test-db`, plus the
  relevant accounting/death/journey wrappers above. The root `test-db` target
  does not currently invoke every specialized accounting or death runner.
- All named test scripts above live under `tests/async/` unless an explicit
  different path is shown. Run only relevant focused checks during each repair;
  run the aggregate gates once the integrated candidate is ready.

## Scope review and next implementation step

The plan-ablation review retains the existing coordinator, journals, SQL and
flatfile repositories, UID catalog, and lifecycle guards. It removes duplicate
dispatch work suggested by stale plans, a new general persistence framework,
a second ownership catalog, broad unrelated refactors, and inventory appraisal.
It keeps real player/restart/fault evidence because compilation and source
contracts cannot establish persistence correctness.

The first implementation increment is **baseline repair plus a failing
quest-consumption/reward restart reproduction**, followed by the smallest durable
completion fix and the save/custody race checks. Continue through ordinary item
and money stability into death/corpse recovery before expanding the remaining
accounting producers. Update this checklist and the evidence report as each
increment passes; do not relabel an unexecuted gate as complete.


## Current implementation note (2026-09-29)

Quest reward continuation v4 now freezes solo quest XP and skill eligibility at
turn-in admission. Player snapshot schema 11 carries an XP receipt keyed by the
offering operation and reward index; the SQL player save transaction records
the applied bit with the progression save. Recovery waits for that durable save
before acknowledging the obligation. This v4 continuation covers solo XP;
group XP uses the v5 recipient-entitlement scaffold described below. V1-v3 XP
continuations remain pending because they do not contain a frozen award.
Migration 0047 adds the durable applied mask. MySQL 8 and MariaDB 10.11
fingerprints and SQL restart behavior remain unqualified; see the dated
qualification checkpoint.

### Frozen group quest XP award scaffold (2026-09-29)

Quest continuation v5 and the admission code now carry the exact XP amount for
each credited recipient and XP reward, using admission-time recipient IDs and
levels. The amounts preserve existing turn-in behavior: the direct completer
is capped at one tenth of the next-level threshold, while nearby credited
group members use the full next-level threshold. The bounded in-memory
completion context grew from 512 to 768 bytes to retain those levels across
asynchronous publication. Flatfile/fallback admission refuses group XP before
item consumption. SQL-primary admission accepts the frozen group award
independently of the economic ledger; its runtime path still needs
qualification. The
focused quest-offering regression, v2-v9 item-transfer and quest-continuation
compatibility harness, and isolated GCC 12 compilation of `quest.c` and
`item_movement_transaction.c` pass. A full SQL build reaches the Redis objects
but cannot compile them with this workspace's hiredis headers.

Migration 0048 and the recovery scaffold provide durable recipient
entitlements, per-recipient save receipts, and login recovery. SQL-primary
group XP is admitted and remains subject to runtime qualification.

Migration 0048 now defines the per-recipient entitlement table, and the SQL
critical-command transaction inserts the v5 rows beside the quest reward
obligation. Retained-command verification checks the exact recipient, reward
index and amount set. Solo v5 rewards continue using the existing owner receipt
mask; the new rows are reserved for multi-recipient awards. The migration and
critical-command object contracts pass. The table is included in the runtime
schema inventory, but its MySQL/MariaDB fingerprints have not been measured.
The pending-entitlement query, save-transaction receipt update and player-load
recovery path are implemented in the current worktree. The changed translation
units pass GCC 12 syntax-only compilation, but there is no database or restart
qualification yet. Flatfile/fallback group XP remains refused before offering
consumption.
Quest participants are frozen at turn-in admission from group members whose
characters are present in the world in the actor's room. A logged-out group
member is not a participant. A link-dead character that remains in-world is
eligible and receives its frozen XP award in the same completion pass; payout
does not wait on or inspect its descriptor connection. If a frozen participant
leaves the world before payout, the durable entitlement is recovered on that
character's next login.

### XP progression and economic double-entry scope (2026-09-29)

The quest XP receipt path makes progression updates replay-safe with player
snapshots. XP progression remains outside the economic double-entry ledger;
ordinary `gain_exp` calls and other progression mutations must not be routed
through economic accounts. A quest XP receipt proves only that its progression
snapshot is durable and replay-safe. Do not describe quest XP receipts,
telemetry progression events, or a player snapshot as double-entry accounting.
SQL-primary group quest XP can run while active economic accounting is enabled
because the progression receipt and recipient entitlement are not economic
ledger entries. Its delivery path still needs runtime qualification; flatfile/
fallback modes refuse group XP before consuming the offering. A recipient who
has left the world retains a durable entitlement for recovery on the next login.
The Python-only `test_quest_xp_accounting_scope.py` now checks that both
`gain_exp` and the XP entitlement recovery path stay clear of
economic-accounting APIs; both focused cases pass. The combined quest harness
reached its compile step, but could not finish on this Windows host because
`g++` is unavailable.


### Typed spell-component retirement continuation scaffold (2026-09-29)

Spell-component destruction commands now retain a typed continuation with a
versioned fixed-endian envelope, a stable effect ID, and up to 48 bytes of
effect-specific context in the existing item-transfer payload. The six migrated
effects are faerie sight, initial and repeat spore burst, summon insects, wall
of bones, and vines. Live publication resolves the ID through an explicit
dispatcher rather than persisting a function pointer or native struct layout.
Envelope version 1 records an explicit context length and fixed-width
little-endian fields; payload validation continues to read the unversioned
envelope from the initial scaffold so a retained journal command still decodes.

The persisted ID/context is groundwork for process recovery, not permission to
replay the effect. Replayed spell-component commands still use the retained
publication path until each effect has an operation-scoped application receipt;
several effects mutate room or player state and can duplicate if applied before
a crash ahead of publication acknowledgement. Add that receipt at the effect
owner before enabling replay. Item publication callbacks and all six spell
effect callbacks now receive the stable critical operation ID, including when a
pending publication is restored from its journal command. This gives each
effect owner the identity needed to scope a receipt without persisting a
function pointer. Focused item-payload, live-publication, and forced-drop
regressions and the GCC 12 SQL build pass for the callback and context-format
scaffold; operation receipts and process-crash behavior remain open acceptance
gates.

### Coin-pile lifecycle source identity scaffold (2026-09-29)

Typed coin-transfer roots now freeze a deterministic lifecycle source event
when a root creates or retires a physical coin pile. The identity uses the
accounting lineage, the debit account type and ID, its expected revision, and
the create/retire transition flags. It does not include a newly allocated
destination pile UID, so rebuilding an unchanged debit under a new critical
operation ID and destination UID does not produce a new source event. A later
legitimate debit uses the advanced account revision and therefore has a new
identity. Successful SQL publication records the source claim in the same root
transaction and retained verification checks the exact claim. The existing
unique `(lineage, source_event)` key rejects a second root for that lifecycle
event. Ordinary coin movements without pile creation or retirement keep no
lifecycle source claim.

The focused coin-accounting and coin-item-accounting harnesses pass, the
isolated GCC 12 SQL build passes, and changed-line formatting passes. The SQL
source-claim insert now maps duplicate key to terminal `EEXIST` for typed coin
roots, matching the existing item source-claim behavior. A focused source
contract covers that classification. The SQL source-claim retry path still
needs a disposable database journey before this scaffold can count as runtime
proof; broad item/money and death qualification remains open.

### Per-character starter grant source identity (2026-09-29)

The legacy newbie kit now derives a stable source ID from the character PID and
the grant kind and carries it through the bounded preparation queue into the
single batch transfer. The CHAOS starter kit uses its own stable tag. Rebuilding
either per-character grant with newly allocated item UIDs therefore retains the
same source claim. The deferred-grant API keeps a zero default for unrelated
callers while preserving explicitly supplied source IDs on every prepared root.
Independent grants queued behind a preparing batch retain their own source
metadata and are not rejected for differing from the batch source. Focused
queue coverage and the 36,360-case starter-kit plan parity check pass; ordinary
item/money qualification remains open.

### Death retry copyover handoff (2026-09-29)

Copyover format version 17 appends a bounded death-retry marker, delay, and
corpse UID to each descriptor record. Recovery validates the appended state and
re-enters the private death hold before restoring the retry, so the player is
not exposed as standing while the terminal save and corpse handoff resume.
Versions 12-16 retain their original descriptor layout and recover without a
serialized retry. A preflight still refuses copyover before persistence drains
when a pending retry belongs to a player without an eligible preserved
descriptor, or when its retry metadata cannot be serialized. This prevents a
disconnected runtime-only obligation from being silently dropped. The focused
copyover contract and production copyover save/exec/recover custody harness pass,
as do the local GCC 12 SQL and flatfile builds. The real flatfile socket journey
passes its actual exec handoff and acknowledged post-copyover save for both
plain Telnet and MCCP. Its runner now preserves the build environment's optional
`LD_LIBRARY_PATH` so private runtime dependencies remain available inside the
isolated fixture. The runtime and custody journeys do not seed a pending death
retry; that specific recovery path, legacy-retry policy, and the wider
death/corpse acceptance matrix remain open.

### Re-anchor the writer source census (2026-09-29)

The copyover and starter-grant edits shifted 86 registry line anchors. The
baseline and current census still have identical `(path, family, excerpt)`
multisets across all 2,804 occurrences, so these were source moves rather than
new or removed candidate expressions. I reanchored the registry and regenerated
the matrix: all 2,746 unique sites map, with zero unmapped sites. The contract
validator and generated-matrix `--check` pass. Release validation still stops
at `writer has no executable evidence`; this refresh does not qualify routes.

### Partition the SQL UID ownership census by lineage (2026-09-29)

The read-only audit snapshot no longer treats every UID in the global
ownership ledger as belonging to the selected accounting lineage. It joins
ledger operations to their retained accounting operation, counts UIDs from
other known lineages separately, and reports ledger UIDs without a retained
lineage assignment as unattributed. UIDs observed under more than one category
are counted as ambiguous rather than double-counted. The selected lineage's
unanchored UID list is now derived only from its own ownership events. The
snapshot counts these populations, and the reconciler validates the scope
arithmetic while flagging selected-lineage orphans, ambiguous UIDs, and
unattributed history. Ambiguous UID values are included in the snapshot and
reported individually so an operator can identify the conflicting history.
Unattributed history remains an explicit audit gap rather than being silently
assigned.

The SQL audit journey now includes selected-lineage, foreign-lineage and
unattributed UID fixtures. The new focused UID-scope contract, reconciler suite
(23 tests), audit-origin suite (4 tests), and Python syntax compilation pass.
The database-backed audit journey has not been run against a disposable SQL
database in this environment. This narrows false cross-lineage classification
but does not establish complete global UID lineage or native-origin coverage.

### Export selected-lineage UID history and infer creation origins (2026-09-29)

The bounded UID history query now joins ownership events to the retained
accounting operation and restricts event history to the selected lineage. It
includes every UID observed in that lineage, including UIDs that lack a current
row, reference, or baseline origin, while preserving the global census counts
for foreign, unassigned, and ambiguous identities. A first revision-zero create
event from a committed operation now supplies a creation origin for that UID.
The snapshot retains the missing-reference event for reconciliation, and the
reconciler reports an ownership event whose operation is rejected or unknown.

The SQL audit journey now expects an unreferenced committed create to produce a
creation origin and to remain visible as a missing reference. Unit coverage for
the lineage-scoped query and non-committed operation rejection, the reconciler
suite (24 tests), Python syntax compilation, and diff checks pass. The SQL
journey remains unrun because this worktree has no disposable database
configuration; the schema/query integration and process-level audit still need
that specific qualification.

### Reconcile mapped coin piles to native item custody (2026-09-29)

The SQL audit cut now exports each selected-lineage active pile mapping with
its stable account key, native UID, decoded denomination vector and revision.
The reconciler links that mapping back to both the native coin payload and the
UID's current item position. It counts unmapped and multiply mapped live piles,
dangling mappings, and mappings whose holding payload is invalid. This closes
the mapping-to-current-native-payload comparison scaffold; coin-pile creation
origin and lifecycle source completeness remain open.

The disposable SQL journey now asserts the mapping row and its native balance.
The focused reconciler suite (25 tests), UID-scope test, Python syntax
compilation and diff checks pass. The SQL journey is not run without a
disposable database; database-backed creation, retirement and replay evidence
remain required.

### Reconcile coin-pile lifecycle source events (2026-09-29)

The lineage UID-root export now includes each retained operation's reason and
source-event identity. The reconciler decodes typed coin-transfer lifecycle
identities and checks that their destination-create and source-retire flags
match the operation's committed UID create/destroy references. It reports a
missing lifecycle identity when a coin transfer creates or retires a pile
without one, and reports malformed or mismatched identities. This checks the
lineage evidence already retained by the accounting and UID ledgers; it does
not claim native creation provenance or runtime SQL replay qualification.

The focused reconciler suite (26 tests), UID-scope test, Python syntax
compilation and diff checks pass. The disposable SQL journey remains unrun
because this worktree has no disposable database configuration. Native
creation-origin and source lifecycle database replay coverage remain open.

Coin-pile lifecycle reconciliation now also compares a retired source pile's
mapping identity with the UID of its destroy event. It rejects roots with more
than one create or destroy reference for a single lifecycle flag. For a
created destination pile, it compares the created UID with the native UID of
the pile mapping created by that operation. Mapping creation exports retain
the mapping's original native UID even after active custody is retired, so
both sides of the lifecycle cross-check have evidence across epochs.

The focused reconciler suite covers wrong source UID, mismatched destination
mapping, and duplicate lifecycle events. The SQL journey asserts the mapping's
original UID. Database qualification remains pending.

### Require committed roots for inferred item creation origins (2026-09-29)

All three SQL snapshot paths that infer a revision-zero item creation now use
one shared predicate: the ledger event must be a create at revision zero and
its owning operation must be committed. Rejected or unknown roots remain in
the history evidence but cannot establish an item's opening origin. This
prevents a failed write's retained legacy event from being mistaken for a
valid coin-pile or item creation.

The UID-scope suite now directly covers rejected, unknown, non-create,
nonzero-revision, committed and duplicate origin candidates. The reconciler
suite, UID-scope suite, audit-origin suite, Python compilation and diff checks
pass. Disposable SQL replay remains unqualified.

### Retain account mapping creator roots across epochs (2026-09-29)

The SQL audit cut now lists every selected-lineage mapping, its stable account
key and creator operation, then exports the creator operation and account
effects even when the operation is from an earlier epoch. For a postbaseline
mapping, a committed creator effect from revision-zero and a zero balance can
now establish its creation origin. The reconciler checks creator-root lineage,
outcome, account-effect count and the exact stable account key, and reports a
mapping whose creator root or postbaseline origin is unresolved. Baseline
accounts still use their witnessed opening origin.

The focused reconciler suite (28 tests), UID-scope suite (2 tests), audit-origin suite,
Python syntax compilation and diff checks pass. The SQL journey was extended to
check mapping-creator coverage, but was not run because this worktree has no
disposable database configuration. Cross-epoch creator-root query behavior and
full postbaseline account-origin coverage still require that database
qualification.

### Audit reverse coverage for pending-claim consumers (2026-09-29)

The SQL cut now finds every committed auction-money-claim root that debits a
pending-claim account and compares its copper debit with the source rows naming
that consumer operation. The reconciler reports a consumer with no source rows
and a source total that does not match the debit. This complements the existing
forward check that validates every stored source row against its producer and
consumer roots, making missing consumer links visible instead of checking only
the links that were recorded.

The focused reconciler suite (29 tests), UID-scope suite (3 tests), audit-origin
suite, Python syntax compilation and diff checks pass. The disposable SQL
journey was extended with a consumer-root fixture but was not run because no
disposable database is configured. Legacy claims and historical writer routes
remain an open coverage gate.

### Export unattributed UID ownership events (2026-09-29)

The global UID census now exports bounded ownership events whose operation ID
has no retained accounting root, including events on UIDs that also appear in a
selected lineage. The reconciler validates those records and reports each as
unattributed history without assigning the event to a lineage. This makes the
unassigned records inspectable while preserving the existing ambiguous-lineage
classification.

The focused reconciler suite (30 tests), UID-scope suite (3 tests), audit-origin
suite, Python syntax compilation and diff checks pass. The SQL journey already
contains a missing-operation ownership event and now asserts its export and
report. It remains unrun without a disposable database; attribution and
recovery for those events remain open.

### Expose source claims attached to baseline roots (2026-09-29)

The SQL audit no longer drops source-claim rows whose operation is the opening
baseline root. It exports the joined operation reason, and the reconciler flags
any claim attached to reason 38 as a baseline-scope exception. Gameplay
source-claim coverage continues to exclude the baseline reason, while the raw
claim row is still visible for review. With that explicit invariant in place,
the baseline source-claim scope is removed from the snapshot's open-gap list.

The focused reconciler suite (30 tests), UID-scope suite (3 tests), audit-origin
suite, Python syntax compilation and diff checks pass. Baseline claim presence
is covered by a corrupted-snapshot test; the disposable SQL journey has not
been run in this environment.

### Capture accepted auction bid prices (2026-09-29)

Auction bid roots now persist their accepted copper price in
`realized_price_copper`. Buy-now bids record the buy price selected by the
transaction planner; ordinary bids record the submitted accepted amount.
Collector purchase roots now persist their frozen catalog price as well. The
SQL snapshot exports reasons 24 and 27 alongside shop buy/sell roots, and the
reconciler checks their captured values against current-lineage operations.
This records domain prices only; XP progression remains outside double-entry
accounting.

The auction SQL harness asserts prices for both an ordinary bid and a buy-now
bid, and the collector SQL harness asserts its frozen catalog price. The
snapshot SQL fixture checks both reason 24 and 27 inclusion, while a focused
reconciler case covers both purchase reasons. The database journeys were not
run because no disposable database is configured. `realized_domain_prices`
remains open until the broader route inventory and SQL qualification are
complete.

### Scope native mapping coverage to the selected lineage (2026-09-29)

The SQL native-row census now requires a matching active mapping from the
selected lineage. A mapping from another lineage can no longer hide an
unmapped wallet, bank, pile, escrow, claim, or treasury row. A focused query
contract test verifies the lineage predicate and bound lineage value, and the
disposable SQL journey now includes a wallet mapped only in another lineage.
That SQL journey remains unrun without its explicitly configured disposable
database.


### Put realized-price route policy in the registry (2026-09-29)

The accounting registry now marks the shop buy/sell, collector purchase, and
auction bid reasons as requiring a captured realized price. SQL export and
reconciliation derive their candidate reason sets from that metadata, and the
registry validator checks its type and presence. This removes duplicated
reason-number lists; the broader trade-route inventory and domain journeys stay
open.

### Cross-check UID history event coverage (2026-09-29)

The reconciler now ties UID-history coverage totals to the exported event list
and requires the separately exposed unreferenced-event list to equal the
unreferenced subset of that history. This detects snapshots that omit or
misclassify unreferenced UID events while adjusting their coverage counters.
Focused reconciliation coverage exercises both mismatches; SQL capture
qualification remains pending.

### Authorize retired mapping account kinds (2026-09-29)

Retirement root snapshots now preserve each operation's reason. The accounting
registry declares which ordinary account kinds each reason may retire, and the
reconciler checks that policy for every retired mapping, including roots from
prior epochs. Current-epoch root reason is also cross-checked against the
retained operation row. The focused reconciler suite and registry validator
pass, including an unauthorized bank-retirement case. The disposable SQL audit
journey now asserts historical retirement reason export but remains unrun
without a configured disposable database. The native escrow census now includes
open and removed listings, deriving a held balance only when there is a winning
bidder; the focused SQL fixture represents a removed listing with retained
escrow. The source-verified trusted-removal contract intentionally retains a
winning bid in its active escrow mapping without refunding either party; the
audit now includes that held balance rather than reporting the mapping as
dangling. The separate no-bid removal and broader runtime journey remain
unqualified.

### Retire empty escrow after no-bid removal (2026-09-29)

SQL settlement now retires an auction escrow mapping whenever the terminal
account effect reaches zero. This preserves a funded escrow mapping for trusted
removal, while no-bid removal closes its empty mapping just as no-bid expiry
does. The registry authorizes auction-cancel retirement of escrow. Focused
planner and SQL harness cases cover no-bid removal; the disposable SQL harness
has not been run in this environment.

### Authorize account mapping creation roots (2026-09-29)

The native audit now checks creator-reason authorization as well as committed
effect and zero-origin evidence for postbaseline account mappings. Registry
policy permits auction-listing roots to create auction escrow mappings and
epoch-transition roots to create ordinary account mappings. Baseline roots keep
their separately witnessed opening-origin path. The focused reconciler suite
covers an allowed epoch-transition mapping and rejects an auction-listing root
claiming to create a wallet mapping. The registry validator and Python syntax
checks pass. Database-backed mapping history and epoch-transition qualification
remain open.

### Reconcile baseline installation mapping creators (2026-09-29)

Initial SQL account mappings name the lifecycle-install inbox operation as their
creator, while the witnessed opening root is a separately derived baseline
operation. The audit exporter now retains baseline witness operation IDs and
exports the lifecycle installation to baseline-root link for these mappings.
Reconciliation accepts that creator only when installation phase, committed
baseline root, lineage/epoch and witnessed baseline origin agree. This prevents
valid opening mappings from being reported as missing creator roots while
keeping postbaseline roots subject to the registry creation policy. The focused
reconciler suite (33 cases), baseline-origin suite (6 cases), Python syntax
checks and diff checks pass. The disposable SQL snapshot journey has been
updated but remains unrun without its configured database.

The baseline-install creator proof also requires the installation receipt's
selected epoch to equal its epoch and its terminal revision to be 1, matching
the lifecycle owner's phase transition. The exporter preserves both values and
requires a successful inbox receipt with no failure stage and a commit
timestamp. The reconciler rejects receipt mismatches instead of relying on
schema constraints alone. Focused UID-scope and reconciler cases cover the
receipt fields and reject corrupted values. Python syntax compilation and diff
checks pass. The disposable SQL journey remains unrun without its configured
database.

The reverse pending-claim consumer inventory now exports and validates the
consumer operation's critical inbox receipt as well. Its focused SQL UID-scope
test verifies the receipt join and captured fields, and a reconciler case
rejects a receipt without a commit timestamp. The reconciler suite (37 cases),
UID-scope suite (5 cases), Python syntax checks, and diff checks pass. The
disposable MySQL journey remains unrun without its configured database.

Realized-price root evidence now exports the accounting result and its critical
inbox receipt across epochs. Reconciliation requires a terminal, matching
receipt with no failure stage and a commit timestamp; committed price roots
must have result 0. A focused reconciler case rejects a receipt missing its
commit timestamp. The reconciler suite (38 cases), SQL UID-scope suite (5
cases), Python syntax checks, and diff checks pass. The disposable SQL journey
remains unrun without its configured database.

The SQL snapshot's repeatable-read source inventory now requires
`critical_operation_inbox` to exist as InnoDB alongside the accounting and
native source tables. This makes the transaction-engine prerequisite explicit
for the durable receipt evidence exported by the audit. Python syntax, the SQL
UID-scope suite (5 cases), and diff checks pass; the disposable database
journey remains unrun.

Every accounting-operation mapping creator root now carries and validates its
critical inbox receipt, including roots from prior epochs. Current-epoch roots
are also compared against the separate operation/effect evidence, covering the
reason, outcome, counts, source event and exact account effects. The focused
UID-scope suite (5 cases), reconciler suite (35 cases), Python syntax checks and
diff checks pass. Cross-epoch SQL receipt capture is staged in the disposable
snapshot journey, which remains unrun without its configured database.

Baseline witness origins now require the same successful, durable inbox
receipt: status 1, result 0, failure stage 0, and a non-null commit timestamp.
The origin reader refuses a witness with any of those receipt fields missing
or inconsistent. The focused origin suite, UID-scope suite (5 cases), and
reconciler suite (35 cases) pass, along with Python syntax and diff checks.

Mapping retirement roots now carry and validate their critical inbox receipt,
including prior-epoch retirements. The receipt must be successful, match the
accounting root result, have no failure stage, and include a commit timestamp.
The focused reconciler suite (35 cases), SQL UID-scope suite (5 cases), Python
syntax checks and diff checks pass. The disposable SQL snapshot journey includes
prior-epoch receipt assertions but remains unrun without its configured database.

Pending-claim source and consumer root evidence now includes each critical
inbox receipt. Export marks a source or consumer root valid only when its root
is committed and the receipt has status 1, matching successful result 0, no
failure stage, and a commit timestamp. Reconciliation checks those receipt
fields directly for current- and prior-epoch claims. The focused reconciler
suite (36 cases), SQL UID-scope suite (5 cases), Python syntax checks and diff
checks pass. Disposable SQL journey assertions cover the receipt exports but
remain unrun without the configured database.

Cross-epoch UID-reference root evidence now carries and validates its durable
critical inbox receipt, including the committed result and commit timestamp.
The disposable SQL journey checks a historical item root's receipt, and a
focused reconciler case rejects a missing commit timestamp. The reconciler
suite (37 cases), SQL UID-scope suite (5 cases), Python syntax checks, and diff
checks pass. The disposable SQL journey remains unrun without its configured
database.

Historical source-claim evidence now includes its accounting result and
critical inbox receipt. Reconciliation requires the retained claim to link to
a committed operation with a matching successful durable receipt; baseline
source claims remain reported under their separate baseline policy. The
focused reconciler suite (38 cases), Python syntax checks, and diff checks
pass. The disposable SQL journey now asserts receipt export for committed
claims but remains unrun without its configured database.

UID-history events now export the owning operation epoch, and reconciliation
cross-checks each event against its separate lineage root's epoch and outcome.
This prevents an event from borrowing a different operation's root evidence.
The focused reconciler suite (38 cases), SQL UID-scope suite (5 cases), Python
syntax checks, and diff checks pass. The disposable SQL journey remains unrun
without its configured database.

Current-epoch operation receipts now retain inbox status, result code, failure
stage and commit-timestamp presence, including rows whose inbox status is
missing or nonterminal. Reconciliation requires a terminal receipt matching
the operation result with no failure stage and a commit timestamp. The focused
reconciler suite (38 cases), Python syntax checks and diff checks pass. The SQL
journey asserts complete receipt coverage, but remains unrun without its
configured database.

Pending-claim reconciliation now cross-checks every reverse consumer root's
source-row count and amount against the separately exported forward source
rows. It also reports a source row whose consumer operation is absent from the
reverse root inventory. Focused reconciler coverage rejects both omitted
source rows and missing consumer roots; the reconciler suite (38 cases) and
diff checks pass. Database-backed legacy-claim coverage remains unqualified.

Native escrow mapping coverage now treats a removed auction as live escrow
only while a winning bidder remains. A removed no-bid auction with an active
escrow mapping is reported as dangling, matching the settlement path that
retires empty escrow. The focused SQL UID-scope suite (6 cases), reconciler
suite (38 cases), Python syntax checks, and diff checks pass. The disposable
SQL journey includes a stale empty-escrow mapping assertion but remains unrun
without its configured database.

Escrow balance export now rejects a positive auction price when no winning
bidder exists, rather than silently projecting a zero holding. A focused
snapshot contract covers valid zero-bid and funded states and rejects the
inconsistent state. The SQL journey includes the malformed native-row case.
The UID-scope suite (7 cases), reconciler suite (38 cases), syntax checks, and
diff checks pass; the disposable SQL journey remains unrun.

Pending claim sources now export both the stable and active native IDs from
their account mapping. Reconciliation requires the stable ID to match the
beneficiary for open and consumed claims, and requires the active ID to match
for open claims. The quest XP source contract also protects the rule that a
group member present in the actor's room receives the completion payout even
without an active descriptor; logged-out characters remain ineligible. The
UID-scope suite (7 cases), reconciler suite (38 cases), quest XP scope suite
(3 cases), Python syntax checks, and diff checks pass. The disposable SQL
journey remains unrun without its configured database.

The native SQL mapping census now requires locator kind to match account kind
for wallet, bank, pile, auction escrow, pending claim and treasury mappings.
Wrong-locator mappings are reported separately and cannot make a native row
appear mapped; pending-claim source joins apply the same locator check. The
reconciler rejects malformed mapping coverage. The UID-scope suite (8 cases),
reconciler suite (38 cases), Python syntax checks and diff checks pass. The
disposable SQL journey remains unrun without its configured database.

The locator identity check now covers retained retired mappings as well as
active mappings. Stable native IDs must remain valid after retirement; active
IDs must equal the stable ID while present. Unsupported mapping kinds are
reported as well. Focused helper cases cover valid active and retired
mappings, wrong locators, changed active IDs and invalid native IDs. The
UID-scope suite (9 cases), reconciler suite (38 cases), Python syntax checks
and diff checks pass; SQL lifecycle qualification remains open.

### Recheck the release census after audit increments (2026-09-29)

The current validator passes contract validation for 861 writer routes and
2,804 lexical occurrences; the generated-matrix `--check` passes with 2,746
unique sites and zero unmapped sites. Release validation still fails first at
`writer has no executable evidence`. The inventory currently has no executable
evidence attached to 849 routes: 843 legacy, three observed, two projection,
and one unsupported. Census completeness is therefore not route completion;
subsequent implementation should continue through the plan sequence and add
evidence only for behavior that the named executable journeys actually prove.

The source review reclassified `account.cleanup_temp_char`: callers load a
temporary `restoreCharOnly` PC graph for account/browser display, then free that
graph. Extracting those copied objects does not retire durable player custody,
so this is a non-writer candidate rather than an item destruction route. The
release validator now checks the generated route matrix against the inventory
and source symbols, and exempts only explicitly dormant or non-writer
candidates from executable backend evidence. The matrix remains incomplete:
14 dormant candidates, 93 non-writer candidates, three offline operational
writers, 688 runtime mutation routes, and 63 runtime projection routes. Release
validation still stops on a runtime route without executable evidence. The
quest XP contract passes: logged-out characters cannot be captured as group
participants, in-world linkdead group members receive the frozen payout in the
same completion pass, and ordinary `gain_exp` remains outside double-entry
accounting.

Account-bound item reward summons now enter the existing typed item-creation
transaction during active accounting. The item UID is retained as the
source-claim identity, so replay under a rebuilt operation ID refers to the
same admitted item; a later permitted re-summon has its own item UID and
source claim. The previous blanket active-accounting refusal is removed. This
advances the first runtime mutation route in the writer inventory, but the
account cooldown reservation and item creation remain separate operations,
and SQL/flatfile playable, crash, and replay qualification is still required.

Follow-up source review found that the legacy summon helper also deduplicates
existing rewards by directly retiring saved item custody and may normalize an
existing item's payload. The active-accounting path now skips both legacy
mutations while retaining typed creation for a new reward. Existing duplicates
are left for the dedicated item-retirement route; active summons with an
existing instance report it without rewriting its payload.

The source inventory now attributes the extraction at that boundary to
`account.reward_duplicate_cleanup` / `existing_character_instance`, not to the
read-only `cooldown_remaining` query. The route is blocked during active
accounting at both summon and dismiss entry points; its direct saved-item
retirement still needs a typed lifecycle path before it can be supported there.
The updated account-reward contract checks these gates and the active typed
creation path. `account_reward.c` syntax-compiles directly with the available
GCC 11 using the maintained warning profile minus the GCC 12-only
`-Wuse-after-free=3` flag.

The multi-claim account-reward contract, exact reward/config/cooldown contract,
player UX contract, and the focused item-admission case pass. The accounting
inventory and generated matrix pass with 2,804 lexical occurrences, 2,746
unique sites, and no unmapped sites. A full WSL build with GCC 11 compiled the
changed path but stops later in unrelated `cmd/actcomm.c`: GCC 11 rejects the
C++20 `std::atomic<std::shared_ptr<...>>` specialization used by output
profiles. The maintained build flag `-Wuse-after-free=3` is also unavailable
on GCC 11 and was omitted only for that attempt. No account-reward SQL replay
journey was run; the route remains unqualified.

### Typed account-reward dismissal (2026-09-29)

Dismissal during active accounting now submits the exact empty reward root
through the typed item-destruction transaction with `intentional_destruction`
source attribution. A versioned continuation carries the grant ID, legacy
marker allowance, and expected prototype. Its command validator binds those
facts to the single selected UID and prototype. After the durable custody
commit, publication rechecks the exact live UID, account marker, owner and
empty-root condition before extraction. Replay restores the same publication
callback from the continuation; failed or unresolved publication keeps the
operation fenced. Inactive accounting retains the existing legacy retirement
path. Active summons now retire empty duplicate roots through that same typed
route. For populated duplicates, active summons first submit a same-owner typed
move for every direct child, then submit retirement for the now-empty root.
If the process stops between those commits, the empty duplicate remains and a
later summon can submit its retirement again. Unplaceable or rejected child
promotion leaves the duplicate intact.

The continuation codec round-trip/invalid-prototype checks,
multi-claim account-reward contract, exact reward/config contract, quest XP
scope contract, inventory validator, generated matrix check, and changed-file
format check pass. The source census is synchronized at 2,811 lexical
occurrences, 2,753 unique sites, and zero unmapped sites; release remains
blocked on executable route and backend evidence. GCC 11 compiled the three
changed C++20 translation units for both MariaDB and flatfile with the
maintained warning profile except for unsupported `-Wuse-after-free=3`. The
broader writer-source contract
suite still has four errors and one failure from stale hard-coded source-line
expectations in unrelated routes. No SQL gameplay or replay journey was run,
so dismissal remains unqualified for release.

Quest eligibility remains based on the character being in the live group when
the quest is admitted. A linkdead character still present in-game is included
in the frozen award list and receives the payout in the same completion pass;
connection state does not defer it. Logged-out characters are not admitted as
participants. Ordinary XP gain remains outside double-entry accounting.

### Spell component effect completion boundary (2026-09-29)

Spell component effect callbacks now report whether publication is complete.
The publisher retains an in-process operation stage after item retirement so a
callback can be retried without extracting those items again. Existing effect
callbacks complete synchronously; this only establishes the completion
boundary needed for an owner to wait on a durable effect receipt. Committed
effect replay after process restart remains fenced until those receipts are
implemented. Focused compilation and runtime contracts could not run because
the configured WSL host did not respond and no local C++ compiler is available.

### Retain spell effects while the owner waits (2026-09-29)

The spell callback now distinguishes an ordinary retry from an effect owner
that has started work and is waiting for its durable receipt. Item movement
keeps that committed operation in a separately visible waiting state and
polls the owner without consuming the bounded retry budget. Item retirement is
remembered during those polls, so the callback can check its receipt without
repeating component extraction. The existing six effects do not yet emit this
waiting result, and restart replay remains fenced; this is the async seam for
the upcoming per-effect receipt work. Python contracts and diff checks pass,
but the C++ harness and build remain unrun because WSL is unavailable and this
host has no C++ compiler.

### Exact nested account-reward retirement continuation (2026-09-29)

The typed account-reward retirement continuation is now version 2 and carries
the exact reward UID. Its validator accepts a selected reward nested under an
outer inventory container while keeping version-1 root-only journal commands
readable. After commit, publication resolves the UID from the continuation,
so it cannot confuse the enclosing inventory root with the reward being
retired. This covers active dismissal and empty duplicate cleanup for nested
reward instances without extracting their outer container. Populated rewards
still use the child-promotion transaction before retirement.

The item-transfer continuation codec contract covers nested version-2
retirement, UID mismatch rejection, flag validation, and version-1 compatibility.
The account-reward runtime source contracts, item-transfer codec contract,
quest XP scope contract, changed-file format check, and targeted MariaDB and
flatfile object compiles pass. The generated inventory remains complete at
2,811 occurrences and 2,753 sites, with no unmapped candidates. SQL gameplay
and replay qualification remains open.

### Prevent account-reward reissue while the UID is in its death corpse (2026-09-29)

During active accounting, the legacy death hook retains a bound reward in the
corpse because its synchronous extraction path is not an item-authority
operation. Account-reward lookup now recognizes the exact marked UID inside
that character's flagged player corpse with a valid corpse save ID as an
existing instance, displays its corpse status, and refuses a second summon.
Duplicate cleanup does not try to retire a
corpse-owned item through the player-owner command. This preserves the
recoverable UID and prevents a second grant while the eventual typed corpse
cleanup path remains open.

The focused account-reward source contract and changed account-reward object
compile pass for MariaDB and flatfile. No corpse gameplay/restart journey was
run; the death/corpse writer remains unqualified.

### Restore rejected spell-component retirement callbacks safely (2026-09-29)

Replayed spell-component commands now rebuild the bounded callback context
from their retained effect envelope and selected item UIDs. A rejected command
can dispatch its existing failure callback and acknowledge publication because
no component or gameplay effect committed. A committed command remains held
with recovery feedback; the effect is not replayed until its owner has an
operation-scoped application receipt. This advances failure recovery without
weakening the duplicate-effect guard.

The focused spell-component publication harness passed under WSL, and the
changed item-movement and magic objects compile for MariaDB and flatfile. The
Windows harness invocation could not find `g++`; the same harness passed under
WSL. No committed-effect restart journey or effect receipt was added here.

### Expose item publication health to staff (2026-09-29)

The trusted persistence-health output now reports item-movement queue and
publication state, including owners waiting for a durable effect receipt,
blocked publications, and pending acknowledgments. This makes the retained
publication fence visible while spell-effect restart recovery remains open.
The focused persistence-status source contract passes. The writer registry and
generated matrix are current at 2,811 lexical occurrences and 2,753 unique
sites, with zero unmapped sites; accounting release remains blocked.

Quest group eligibility remains based on an in-game character present in the
room at admission. Reward delivery uses that character's presence in the game
world and does not require an active descriptor, so a linkdead participant is
paid in the same completion pass. A character absent from the game world is not
admitted to the quest group snapshot. Ordinary XP remains outside
double-entry accounting.

### Persist spell-effect receipts with player snapshots (2026-09-29)

Player snapshot version 12 now carries bounded operation-scoped spell-effect
receipts. The save pipeline retains them through coalescing, and the SQL
snapshot transaction writes each receipt in the same commit as the owning
player components. Player loads read and validate those receipts into the load
result. Additive migration 0049 creates the receipt table. This establishes the
durable storage path; individual effect callbacks do not yet consult receipts,
and committed spell replay remains fenced until owner application, save-ack
polling, and receipt retention cleanup are integrated. No migration was run.
