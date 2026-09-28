# Finish accounting, saves, item custody, and death recovery

Date: 2026-09-28
Branch: `finish-accounting`
Planning base: `main` at `d686d4c70384af9012673fa7c4db5cba356c471f`
Status: implementation plan; accounting release remains blocked.

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
| `quest.c` and `item_movement_transaction.c` | Offering consumption commits; publication is acknowledged and pending state erased before the completion callback invokes quest tracking/rewards | Source inspection identifies a crash window between consumption and reward. Reproduce it with a restart test, then make the business outcome recoverable. Existing focused tests do not prove that boundary. |
| `scripts/economic_sql_audit_snapshot.py` | Explicitly exports an incomplete cut; missing coverage includes coin piles, escrow/claims/treasuries and full UID history | Finish native export coverage before using reconciliation as a whole-economy certificate. |

The quest path also depends on finding a live quest NPC, logs a tracking failure
and proceeds with rewards, and can refuse an item grant after offerings are
consumed. Its current durable offering adapter handles item-only goals with a
14-root bound and refuses cash rewards during active accounting. These limits
need a documented support decision and executable cases.

The historical [release report](RELEASE_REPORT_2026-09-27.md) records dual-engine
schema qualification through migration `0044` and maintained SQL/flatfile builds
on earlier commits. Preserve that evidence with its original commit identity.
It does not certify this branch's final integrated binary.

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
