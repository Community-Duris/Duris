# Finish accounting implementation plan

Updated: 2026-10-02. **Status: accounting activation and release remain blocked.**
SQL is the first delivery target; flatfile parity follows. This plan tracks the
current work. [Implementation history](FINISH_ACCOUNTING_IMPLEMENTATION_HISTORY.md)
preserves the dated checkpoints and their original evidence.
The [experimental review checkpoint](EXPERIMENTAL_REVIEW_CHECKPOINT.md) explains
the public branch scope, publication checks, and known blockers for reviewers.
The damaged production database requires the separate
[ownership repair and accounting opening procedure](../../operations/PRODUCTION_RELEASE_AND_OWNERSHIP_REPAIR.md)
before any production cutover.

Current integration and native qualification are recorded in the
[October 2 review status](REVIEW_STATUS_2026-10-02.md), with earlier results in the
[October 1 review status](REVIEW_STATUS_2026-10-01.md). Canonical, staging, and
master-prefix histories now retain 53 receipts. The staging fork preserves its
first 45 and appends eight; the master fork preserves its first 31 and appends
22, retaining the existing runtime-state payloads. All three converge on the
pinned 225-table schema on disposable MySQL 8.0.46 and MariaDB 10.11.14 targets.
The writer inventory covers 864 routes, 2,815 occurrences and 2,756 unique sites
with zero unmapped sites. These counts do not replace route qualification.

Independent reconciliation now checks every native item's parent edge even when
lineage history replaces its epoch-local history or its opening origin is
missing. Corruption/recovery probes pass through the read-only SQL exporter on
both engines. Lineage and epoch-local history also share creation and irreversible
UID-retirement checks; memoized native topology bounds ancestor work, and 59
reconciler tests pass. Explicit creation/destruction must also match live/tombstone
custody, a corrupt destruction cannot erase the UID retirement fence, and
a second destruction of an already retired UID is reported. SQL lineage
references and history cuts now derive prior UID revisions from the immutable
item revision, independently of aggregate owner counters; impossible revision
zero refuses before filtering. The independent audit now also rejects boolean,
negative and overflowing item revisions in origins, native custody, references
and all event scopes, while admitting the full native uint64 range. A witnessed
UINT64_MAX transition, missing-reference detection and exact baseline recovery
pass through SELECT-only exports on both engines. Money revisions use the same
native unsigned range for holdings, ordinary effects, opening origins,
creation/retirement roots and pile mappings. Boolean/overflow evidence refuses;
full-range wallet/pile SQL cuts detect stale revisions and restore their exact
original snapshot. Ten exporter tests and both SQL probes pass. Complete native source/origin
and writer qualification remain open. Restore qualification also rejects
unwitnessed epic revisions and gaps that conserve aggregate value; both SQL
component probes and the nine-case native recovery suite pass this repair. See the October 2 review status for scope.

Native isolated SQL restore now supports an explicit `restore_database_engine`
policy choice, retaining MariaDB by default and admitting an installed MySQL 8.0
executable only after a version check. All ten native recovery cases pass,
including full MySQL and MariaDB dump/import and isolated server boot. Both SQL
cases also pass with direct source/candidate version readbacks; 40 policy tests
and seven provisioning tests pass. Captured-clone, complete accounting, erasure,
remote backup custody and measured workload qualification remain open.
Restore now also refuses unwitnessed wallet/bank revisions and missing native
revision pairs even when all denomination totals remain unchanged. It accepts
both currency-ledger and committed economic-effect witnesses, counting their
same-revision bridge once. Dual-engine SELECT-only probes and all ten native
recovery cases pass this bounded R8 repair; complete accounting/clone workload
qualification is still required.

The later frozen `79540e65d` broad run also finished: 826 passed, 11 skipped
and two stale source-contract assertions failed in 6,127.39 seconds. NPC cash
assignments still match the classified registry; their expected locations in
the test now match the boot-integration source, and all 52 writer contracts
pass. The help-build contract now checks the documented flatfile make target
and compiler define and passes its focused check. This remains
a failed full run plus a focused repair; the frozen `8c997b00d` run continues.

The frozen candidate `fbd9f5035` completed `make test-all`: 822 passed, 11
skipped and one live Redis fixture failed because its Unix socket path exceeded
the platform limit under a long qualification TMPDIR. The repaired fixture
passes the same native authentication/TLS/database checks under that TMPDIR.
The strict SQL server, area editor and world generators built successfully.
This is a failed broad run plus a separate focused repair pass; a current-head
integrated run and skipped external-service checks remain qualification gates.
The skipped help-import fixture now honors a validated disposable connection;
its rollback, atomic publication and nontransactional refusal test passes on
both SQL engines. The 79540e65d run completed with the failure totals above; later fixture fixes
retain separate focused evidence.
The optional native item-provenance fixture now links and honors terminal
source-reuse refusal; its full transaction/replay/epoch/concurrency probe passes
on both engines. Native load/recovery and telemetry schema/factory checks also
pass separately on both engines. These leave player-journey, backup/restore,
real telemetry-role grants and Docker integration gates open.
Guarded development combat has bounded variant passes on both engines,
but the first MariaDB boon attempt has an unexplained owner-revision failure.
The fixture now retains exact acknowledged source rows before assertions for
that investigation. Five more MariaDB boon repeats pass with durable ACK,
self-scoped conflict readback and restart stability, without explaining the
original RED. Production conflict-release attempts remain RED; no gate
or inactive selector is changed.
The current ten-case native disposable backup/restore suite passes on the
same strict SQL/flatfile source, including both SQL engines, journal recovery,
isolated server boot and corruption refusals, with direct daemon version
readbacks. Captured-clone/full-world, erasure propagation, remote custody and
active-accounting lifecycle gates remain open.

The disposable playtime journey now honors a validated `TEST_DB_PORT` in both
native connections and SQL clients. Its outdated temporary-table probe is
repaired to retain the migrated runtime-state foreign key. The complete native
save/death/crash/copyover journey and item/spell/XP receipt probes now pass on
both SQL engines using the strict frozen-candidate executable. These qualify
the measured inactive-accounting routes, not complete economic accounting.

The character-deletion journey honors its selected disposable SQL port. Its
shortened random namespace fits MySQL's named-lock limit without changing native
exclusion locks. The no-specials quest-state boot dependency is repaired in
`b401a8521`; real refusal/rollback/playable-retry/deletion/cold-restart journeys
pass on both SQL engines. These bounded results leave economic identity and
complete alias-erasure qualification open.

Stopped quarantine recovery from upstream `5c157f693` is integrated with the
review fixes. Version-2 archives retain original frames and frozen grant
commands; native commit proofs gate resolution and later-save revalidation.
Synthetic native archive/flatfile restore checks and actual commit/crash tests
on disposable MySQL 8.0.46 and MariaDB 10.11.14 pass. This does not establish
recoverability of historical live PIDs. The stopped recovery restrictions and
proof requirements are in the [recovery contract](../PLAYER_QUARANTINE_RECOVERY.md);
the captured-clone gameplay and integrated workload gates below remain open.

Material downgrade now freezes one original input and two lower-tier outputs in
one native craft operation (`0cd04f40b`). Missing prototypes and refused admission
preserve the original. Its real mortal journey passes on flatfile, MySQL 8.0.46
and MariaDB 10.11.14 through copyover and two cold restarts with exact output
UIDs and one original retirement. The enclosing active-accounting refusal is
retained; ordinary salvage and further material families remain open. The writer
inventory separates this batch from the remaining ordinary salvage grants and
direct retirement rather than qualifying the entire salvage command from it.

Ordinary salvage also preflights its selected material and eligible recipe
templates before granting rewards or consuming tools (`c5ea78c95`). The actual
command previously dereferenced missing templates. Its native ASan/UBSan
regression now preserves source and tools on those refusals, retains successful
salvage and ineligible-recipe behavior, and both production profiles pass. This
does not couple ordinary salvage's independent grants, retirement or progression.

Salvage reward thresholds and tool-assisted recipe scores are also bounded
(`d4af6fac3`). Very large finite multipliers preserve certain outcomes without
out-of-range casts or signed score overflow; invalid settings refuse before
mutation. Native whole-command sanitizers cover large values and fractional
roll boundaries. Both production profiles pass; compound salvage remains open.

## Delivery gates

| Gate | Required outcome |
| --- | --- |
| Stable SQL gameplay candidate | Saves, item movement, quest rewards, death, and corpse recovery survive disconnect, database interruption, copyover, and restart on one reproducible build. Accounting stays guarded. |
| Complete SQL accounting candidate | Every supported money and item writer has durable native effects, balanced evidence, publication, and recovery proof. Explicitly unsupported routes refuse before mutation; ordinary core gameplay cannot qualify by refusal. Independent reconciliation and guarded activation pass. |
| Full feature completion | Flatfile reaches the same supported behavior and passes native replay, audit, and recovery journeys. |
| Production data repair | Two verified backups, frozen journals, protected case register, clone rehearsal, reviewed holds, and a new witnessed accounting opening are complete before SQL reopening. |

## Current state

| Area | Implemented | Still required |
| --- | --- | --- |
| Player journal and staging fences | Staging's archive/PID policy, replay-prefix draining, and ACK retry are integrated with full-payload death/XP/spell proofs. Ordinary checkpoints retain operation frames. Unproven or terminal groups remain archived and fenced across restarts; transient/ambiguous results retain active frames. Allocation failures refuse initialization without marking valid frames corrupt. Native restore and lifecycle checks preserve archives and policies. Both production builds and SQL/flatfile offering/XP-ACK cold crash journeys pass after integration. | The completed staging generation is now privately captured and checksum-verified; its SQL restore/migration and native copied recovery-file qualification pass on MariaDB 10.11.14. The exact SQL candidate also reaches healthy persistence and shuts down normally with all captured recovery bytes unchanged in the existing isolated mini-world service test. Full-world boot and existing-character login/save on that clone, a fresh rollback generation, the dedicated staging restore mount, and integrated disconnect/database interruption/copyover qualification remain. The migration runner now retains its advisory lock on one non-reconnecting connection, rolls back a receipt when the state comparison fails, and passes native connection-loss, timeout, SQL-error, allocation, and cancellation faults on both supported engines. The explicit staging 0045 fork manifest now appends eight steps without changing the first 45 receipts; both disposable engines converge on the canonical 0053 schema and pass shell/compiled history, state, and expression checks. The earlier 0052 transition also passes on the verified staging-derived clone with all original 45 receipts and checked native data preserved; remote rollout remains pending. These local proofs do not open production or clear ownership holds. |
| Writer inventory | The generated matrix covers 864 routes, 2,815 lexical occurrences, and 2,756 unique sites with zero unmapped sites. Material downgrade has its own batch route; ordinary salvage remains separately classified. The helper, its call sites, its declaration, and four temporary material-probe cleanup sites are classified. | Executable route evidence and full native coverage; the release validator remains blocked. The scanner alone is not release proof. |
| Physical crafting | Schema-2 craft admission now records one crafting source from the consumed input lifetime, exact native input retirement and output admission, and matching item references. Native MySQL/MariaDB and flatfile tests pass success, nested output state, deliberately empty output, replay and rollback; flatfile also passes separate-process interrupted authority recovery. Publication keeps its operation fence until outputs can be restored and the original actor can receive them. Poison mixing, physical and virtual Encrust, Chaos-pouch collection and Harvester shard exchange use this owner. Retained pouch counters share the native craft commit without changing UID or custody; native dual-engine SQL rollback/stale-counter tests and flatfile interrupted recovery pass. Real server pouch collection/virtual Encrust passes copyover and cold reload on both SQL engines and flatfile. Mortal Craft/Forge using one frozen leather recipe also passes physical and retained-pouch input retirement, fresh output identity, exact saved XP, copyover and two cold restarts on all three backends. | Complete active-epoch server journeys, other recipe/forge variants, paid enhancement writers, and their progression/publication recovery. Existing component/native proofs and legacy server journeys do not qualify every active player route. |
| Quest rewards | Durable offering continuations, item/cash recovery, eligible skills, and SQL-primary XP entitlements use stable operation IDs. Admission freezes in-game group members present in the room. A linkdead character still present receives its reward in the same completion pass. Live quest XP completion requires the exact successful revision. Journal replay can retire an obsolete non-death attempt only after the repository verifies every attached operation receipt; a newer player revision alone remains insufficient. SQL verifies the applied entitlement or solo reward mask, and flatfile verifies its catalog markers and immutable spell receipts. Recovery now matches operation, recipient, reward index, and amount from a successful save completion instead of inferring receipt success from a later player revision. Flatfile ordinary saves now commit exact frozen XP award markers with the player snapshot, load owner masks and peer entitlements, and reject missing/conflicting replay markers. Native solo/group fault tests include mixed spell/XP commits and peer recovery after owner acknowledgment. Applied live XP is retained before save admission and collected by later ordinary or death saves with its progression components. Schemas 15/16 carry XP and optional spell receipts through immutable death requests and conflict evidence. Flatfile death custody and XP markers share one catalog after-image. SQL accepts an exact already-applied peer receipt after owner acknowledgment and refuses a new application under an acknowledged obligation. Group XP admission and linkdead completion are enabled on both backends and pass the live helper harness. | The isolated SQL offering-publication crash journey now passes recovery and a second cold restart with original offering UID tombstones, one reward UID, exact cash, retained XP, and a closed obligation. The flatfile offering crash also passes recovery and a second cold restart after fixing an inactive-accounting boon fence. The XP-commit/lost-completion journey now passes on both backends through a second cold restart after load verifies original native item/cash delivery and skips paid frozen slots. SQL checks native inbox/ledger witnesses; flatfile formats 8/4 retain creation and cash provenance, and ambiguous older obligations stay held for review. The SQL recovery budget boundary is now fixed and natively qualified: three fixed obligation/witness SELECTs plus one XP entitlement SELECT, with complete query/row/byte metrics on success and refusal. Disposable MySQL 8.0.46 and MariaDB 10.11.14 tests pass 64 obligations and 4,096 slots within the maintained load budget; see [review continuation](REVIEW_CONTINUATION_2026-09-30.md). Integrated production workload and concurrent publication/save recovery remain unqualified. Group, copyover, database interruption, and further save/custody race journeys remain. This is partial qualification. |
| Item publication | Quest, forced-drop, and soulbind owners have replay paths. Actor-owned `vines` and player-target `faerie sight` hold component publication until the effect owner's affect and operation receipt share an acknowledged player snapshot. Faerie sight records a stable target PID, survives runtime-ID changes, and records a fizzle when the target moves away. Failed saves reopen the request without repeating the effect; replay consults the loaded receipt for the pending operation. The loader fetches receipts for both source and effect owners of pending spell publications, so retained historical receipts do not exhaust its row budget. Old self-target faerie commands remain recoverable; old other-target commands without a stable PID remain held. Other committed spell effects retain their publication fence. Snapshot schema 12 and migration 0049 store receipts in the owning player save transaction; death schema 13 and conflict schema 14 retain those receipts through terminal saves and immutable retries; SQL replay verifies the exact receipt. A disposable MariaDB fixture proves affect/receipt atomicity and rejects missing or conflicting receipts. Flatfile player saves now retain immutable per-operation spell receipts in the same recoverable authority transaction as affects and, for death, disposition and custody. Native fault tests prove separate-process recovery, scoped receipt loading, exact replay, and corruption/conflict refusal. | End-to-end server restart and save/custody race journeys for `vines` and player-target `faerie sight`. NPC, room, and event effects need their own durable application policy. Integrated flatfile spell gameplay/restart qualification remains pending. |
| Money and audit | Typed currency/item operations and a read-only SQL cut cover mapped wallets, banks, coin piles, auction escrow, pending claims, and shop cash. UID lineage, sources, and realized prices have partial reconciliation. The repair topology classifier retains every duplicate physical payload row; the read-only ownership reconciler checks post-baseline revisions and detects missing event revisions. A separate custody-history classifier records anomalous current rows and normalized death-custody evidence for a frozen-clone case packet. The verified staging-derived clone preserves 417 custody cases and 1,901 physical topology findings through migration; its protected custody artifact is tied to the SQL dump checksum. | Complete native origin, source, holding, and UID-history coverage; protected cross-backup/journal repair evidence; prove every supported writer and activation guard. |
| Death and corpse | Terminal fences, conflict evidence, copyover retry scaffolds, and account-reward corpse custody guards exist. The current strict SQL binary completes real combat/death and resurrection on disposable MySQL 8.0.46 and MariaDB 10.11.14, returning 12 original fixture UIDs and the exact wallet while retiring the death coin pile. | The inactive-accounting acceptance test remains RED: 27 item events and two currency operations lack linked roots/postings. Active-epoch death/resurrection, loot, decay, dispute recovery, and restart still need qualification with exact original UIDs and value. |
| Flatfile | Selected quest and item recovery paths, player spell receipt persistence, and ordinary/death quest XP receipt persistence exist. Live receipt fallback and group XP helper qualification pass. | Integrated gameplay/restart qualification and equivalent authority, accounting, audits, lifecycle, and failure recovery remain. |

**Scope rule:** ordinary XP gain is outside economic double-entry accounting.
Quest XP receipts make progression replay-safe; they are not ledger postings.
A logged-out character cannot join a live quest group. A linkdead character
remaining in the game can participate and is paid without a descriptor check.

The nested SQL locker journey now also passes a cold reload after withdrawal
on both supported engines. Original root/child UIDs, exact item revisions
5 -> 6 -> 7, immutable transfer operations and successful durable inbox receipts
survive both reloads and the following saves. Native nesting, extra descriptions
and affects are preserved. This is inactive-accounting gameplay qualification
with a synthetic player and seeded items; active locker fees remain guarded,
and crash-point, active-epoch and flatfile parity gates remain open.

## Work order

Ordinary bandage consumption now passes a real mortal SQL journey on both
supported engines with exact surviving original UIDs, one unchanged retirement
operation/revision, save, and two cold restarts. This inactive-accounting fixture
does not qualify active accounting or a crash between retirement and publication;
see the [October 2 evidence](REVIEW_STATUS_2026-10-02.md).

1. **Close coupled save and publication gaps.** Finish spell-effect owner
   receipts and acknowledgements. Pending applied player effects now carry their
   receipt identity into later ordinary and terminal saves after capture or
   admission failure. Death schema 13 and conflict schema 14 preserve the same
   receipts in the terminal transaction and immutable retry. Quest XP uses schemas
   15/16, with optional spell receipts, and remains pending through failed initial
   save admission. Repository rollback/replay and live helper tests pass. Qualify these paths
   across an integrated server restart and save/custody races.
   Prove that a committed quest consumes its
   offerings once and delivers or visibly retains every reward across process
   crashes, missing NPCs, disconnect, and restart. Repair stale save/custody
   races at their native authority boundary.
2. **Qualify ordinary SQL money and items.** Exercise get, drop, give, equip,
   nested storage, pets, room recovery, banks, coin piles, transfers, and group
   splits. Preserve each item UID and payload, exact denominations, source
   identity, owner revision, and accounting reference through retry and replay.
3. **Qualify death and corpse recovery.** Link wallet conversion, corpse handoff,
   terminal save, and character release. Recover healthy and disputed cases
   without a second corpse or silent asset loss. Verify later loot, decay, and
   resurrection against surviving custody.
4. **Finish compound writers.** Complete shops, collectors, auctions, crafting,
   spell inputs/outputs, world loot/resets, rewards, and staff actions. Couple
   each operation's native changes with its money postings, item references,
   source claims, and recovery obligation. Keep unsupported routes guarded
   before mutation.
5. **Finish independent audit and activation.** Derive holdings and UID custody
   from native stores, reconcile every root and source, and fail specifically on
   missing or conflicting evidence. Qualify schema, backup/restore, deletion,
   retention, and activation on disposable MySQL and MariaDB targets.
6. **Qualify the integrated binary, then flatfile.** Run the full gameplay and
   fault matrix on one build, record its hash and schema, and finish equivalent
   flatfile journeys. Production migration and deployment require separate
   owner authorization.

The no-specials quest-state boot dependency is repaired and the native SQL
deletion refusal/rollback/retry/cold-restart journey passes on both engines for
the combined source including help PR #679. The real flatfile account-menu
journey also passes after establishing its empty authority catalogs through
native repository APIs. Missing authority retains the saved character and quest
alias; playable retry, deletion once, quest-alias erasure and cold restart pass
without changing production fences. Retained economic identity, all other alias
paths and the wider R8 gates remain open; the October 2 status records hashes.

## Nonnegotiable invariants

- Native authority decides balances and custody; stale snapshots and live
  projections never overwrite newer committed outcomes.
- A command's native effect, required accounting evidence, source claim, and
  recovery obligation share its appropriate durable boundary. Changed-intent
  replay under the same operation ID fails.
- Money postings balance exact value and denominations against real holdings
  or named issuance, expense, opening, or restitution policies. Item custody
  does not acquire an estimated market value.
- Each admitted item UID has one valid live owner or a tombstone, acyclic
  topology, original payload, and traceable creation and retirement.
- A committed result remains recoverable after lost replies, disconnect,
  copyover, and restart. An unresolved conflict retains evidence and a
  player-visible recovery state.

## Proof gates

During implementation, keep checks focused: changed-line formatting,
`make -C src`, the relevant focused executable or disposable SQL fixture, and
`python3 scripts/validate_economy_accounting.py` plus the generated-matrix
`--check`. Run the broader gameplay, crash-point, dual-engine schema, audit,
`make test-all`, and `make test-db` gates on the integrated candidate. The
release validator must pass before accounting activation. Record unsupported
checks as open gates; do not infer runtime correctness from a codec or source
contract alone.

See [remaining requirements](REMAINING_REQUIREMENTS.md) for R1-R8,
[delivery plans](DELIVERY_PLAN.md) for domain contracts, the
[qualification checkpoint](QUALIFICATION_CHECKPOINT_2026-09-29.md) for dated
build and journey evidence, and the
[current writer matrix](writer_coverage_matrix.json) for inventory state.
