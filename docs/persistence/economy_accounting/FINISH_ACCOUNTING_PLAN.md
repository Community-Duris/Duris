# Finish accounting implementation plan

Updated: 2026-10-03. **Status: accounting activation and release remain blocked.**
SQL is the first delivery target; flatfile parity follows. This plan tracks the
current work. [Implementation history](FINISH_ACCOUNTING_IMPLEMENTATION_HISTORY.md)
preserves the dated checkpoints and their original evidence.
The [experimental review checkpoint](EXPERIMENTAL_REVIEW_CHECKPOINT.md) explains
the public branch scope, publication checks, and known blockers for reviewers.
The damaged production database requires the separate
[ownership repair and accounting opening procedure](../../operations/PRODUCTION_RELEASE_AND_OWNERSHIP_REPAIR.md)
before any production cutover.

Current integration and native qualification are recorded in the
[October 3 review status](REVIEW_STATUS_2026-10-03.md), with earlier results in the
[October 2 review status](REVIEW_STATUS_2026-10-02.md) and
[October 1 review status](REVIEW_STATUS_2026-10-01.md). Canonical, staging, and
master-prefix histories now retain 55 receipts, preserving published alchemy
migration 0054 and adding exact room-item payload migration 0055. The staging
fork preserves its first 45 and appends ten; the master fork preserves its first
31 and appends 24, retaining existing runtime-state payloads. All three converge
on the pinned 226-table schema on disposable MySQL 8.0.46 and MariaDB 10.11.14 targets.
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

The independent reconciler now also refuses weighted copper overflow in parsed
native, opening and effect denomination vectors, even if their values agree and
all postings balance. The consistent malformed snapshot reproduced a false
zero-exception result; 61 reconciler tests and both SELECT-only SQL probes pass
overflow refusal, exact repair and unchanged authority. The CLI refuses the same
malformed input with a zero detail limit. These are bounded R2/R7 format checks,
not complete holding/source, workload or active-gameplay qualification.

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
same-root, same-transition bridge once, including receipted native children.
Distinct roots claiming the same revision and invalid wallet/bank transitions
refuse even when counting them could hide another missing revision. Dual-engine
SELECT-only probes and all ten native
recovery cases pass this bounded R8 repair; complete accounting/clone workload
qualification is still required. The diagnostics integration now also passes
both strict production builds, all ten native recovery cases and the nested
locker journey on both SQL engines at native source `5d6cf93...`. These bounded
proofs remain separate from the frozen failed broad runs and full R1-R8 gates.
A native economic-only shop purchase reproduced a value restore refusal despite
valid committed money revisions. The restore qualifier now walks actual native
and economic denomination before/after witnesses instead of summing only legacy
currency deltas. Both engines pass native-purchase and full buy/sell
corruption, exact-repair, bridge and later-opening probes. All ten native recovery
cases pass in 321.980 seconds. These remain native component/recovery checks,
not active-epoch player qualification. The frozen `88d3b364c` broad regression is
also in progress and does not contain this subsequent value qualifier change.

Flatfile legacy deletion now refuses active or corrupt accounting metadata under
native authority locks, including direct wallet/bank removal and empty-account
finalization. The pre-fix native fixture admitted a wallet removal operation.
ASan/UBSan deletion/recovery checks, borrowed-lock reads, both strict production
builds and the inactive real account-menu deletion/cold-restart journey pass.
All ten native recovery/restore cases also pass in 427.430 seconds with both SQL
engines, exact legacy replay and private service boot.
This is bounded R6/R8 unsupported-writer refusal; typed erasure, account-menu
pre-fence admission, complete retention/audit and release qualification remain
open. SQL deletion admission is now repaired as described below. Native source is `4968da54...`; the frozen broad
`88d3b364c` run does not contain these changes. See the October 2 review status.

SQL legacy deletion now acquires native writer admission before BEGIN. The
character guard and physical-delete boundary validate the exact same-session
held lease; a cached PID cannot bypass absent/lost authority. Both SQL engines
pass native active/staged/schema-error/reconnect/lost-lease refusal and the two
retained-death-evidence lock-order races. ASan/UBSan runtime refusal/rollback/
publication checks and strict SQL, flatfile and offline pfile builds pass.
Real inactive character-menu journeys pass lifecycle-read-error refusal,
playable repair/retry, deletion once and cold restart on both SQL engines;
flatfile's inactive journey also passes. All ten native recovery cases pass in
316.257 seconds at native source `971da564...`. Whole-account cleanup now acquires
the same gate before its transaction; that change has build/source-order proof,
not whole-account runtime qualification. Account-menu pre-fence admission,
typed erasure and full R1-R8 qualification remain open. See the October 2 status.

SQL account confirmation now acquires native admission before the irreversible
deletion fence, retains the lease through that write, then releases it before
worker-save draining. An unavailable lifecycle table reproduced permanent
fencing before backend refusal on both real SQL engines. Both engines now pass
unfenced refusal, unchanged character/mapping/items, playable reconnect/save,
and the subsequent character-deletion fault/retry/cold-restart journey.
ASan/UBSan confirmation-owner tests cover unavailable authority, outer
transaction, fence-write failure, retained fenced retry/cancel and successful
publication. Both strict server profiles and pfile pass; all ten native recovery
cases pass in 275.327 seconds at native source `30d8b4b4...`. Flatfile's existing
inactive deletion journey passes, but its pre-fence admission remains open.
Whole-account typed erasure/runtime cleanup and full R1-R8 gates remain open.

The completed frozen `88d3b364c` broad run retains three failures: two native
files violate the formatter contract, the full-world inspector compilation hit
its 180-second deadline, and the isolated publication-ACK harness omitted the
new observation bindings. The ACK harness now executes the current production
function with those bindings and asserts command/outcome traces across blocked
checkpoint, failure and retry; its focused test passes without changing native
code or checkpoint semantics. The two native formatting violations are now
repaired; the full formatting contract, strict SQL/flatfile/pfile builds and
54 writer contracts pass at native source `a132651b...`. Full-world and fresh
current-head broad gates remain open. The formatter milestone was then integrated
with remote PRs #684/#685; both strict server profiles, pfile, the full formatting
contract, account-worker/watchdog regressions and refreshed writer contracts pass
at native source `44528a2f...`. These are separate combined-source checks.
The separate unchanged-source inspector build takes 98.804 seconds;
that compile result does not replace the failed full-world journey.

The later frozen `79540e65d` broad run also finished: 826 passed, 11 skipped
and two stale source-contract assertions failed in 6,127.39 seconds. NPC cash
assignments still match the classified registry; their expected locations in
the test now match the boot-integration source, and all 52 writer contracts
pass. The help-build contract now checks the documented flatfile make target
and compiler define and passes its focused check. This remains
a failed full run plus a focused repair. The frozen `8c997b00d` run also
finished: 828 passed, 11 skipped and the same two stale source-contract tests
failed in 7,160.80 seconds. Later focused repairs remain separate evidence.

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
retain separate focused evidence. The later diagnostics merge `4fffb0748`
moved three existing census sites; the registry and generated matrix now retain
the same 864 routes and 2,756 unique sites, zero unmapped, with all 52 writer
contracts passing. This inventory refresh does not qualify those routes.
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

Remote PR #686 session-queue changes are also integrated locally. The current
formatter/native queue and writer-contract checks pass. Copied native .d targets
still name prior QA directories, so cached integrated binaries do not qualify
new descriptor headers. Fresh strict production SQL/flatfile/pfile builds pass
from empty object directories. Real inactive SQL deletion journeys pass on both
engines; the existing flatfile character deletion journey also passes. The
flatfile pre-fence admission repair now passes the real menu journey, sanitizer
owner tests, both strict backend builds and all ten native recovery checks. The committed-fence acknowledgement repair passes native sanitizer, baseline,
strict backend/pfile builds, both SQL deletion journeys and all ten recovery
checks. Real uncertain/durable publication faults pass pending recovery,
non-cancellable retry and pending-journal crash recovery through whole-account
erasure. Both complete journeys remain failed because a zone-story alias
survives whole-account deletion; this separate R8 defect and the second cold
restart remain open. Existing bank/shop/UID owner checks pass. Remote character-index, portable copyover and readiness changes through
16ce1f2fb now pass fresh strict SQL/flatfile/pfile builds, incoming owner suites,
both real SQL deletion journeys and all ten native recovery cases. Shifted
writer locations and hardcoded assertions are refreshed after exact identity
checks; all 54 contracts pass without route upgrades. Flatfile whole-account quest-alias
erasure now passes the complete uncertain/durable publication and second-restart
journeys on native 27c8bb63 after Telnet integration. Checked quest serialization
rejects allocation-truncated state. Native SQL whole-account deletion still
retains quest aliases; protected history, typed retention and other lifecycle
gates remain open. See the October 3 checkpoint for the exact proof boundary. A new integrated broad run is pinned to published 0be1cdf30, rather
than the uncommitted acknowledgement candidate. Current broad and full-feature
qualification remain open. The unchanged frozen full-world retry passes inspector
compilation but times out in its 600-second server-build stage before gameplay.
No deadline was increased or gate waived; see the October 2 review status.

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


### October 3 SQL whole-account quest erasure qualification

Native source ce7550d6 now validates and erases all-season quest aliases inside
whole-account SQL deletion and refreshes cached state after success. Real RED
probes precede both engines' malformed/stale/write-failure/rollback/retry/cache/
cold-restart journeys. Strict backend/pfile builds, full formatting, 387 native
allocation-fault sanitizer checks, combined flatfile and SQL character journeys,
maintenance owner contracts and ten recovery cases pass. See the latest review
status for exact source, artifact hashes and evidence boundaries. Protected quest
history policy, broader personal stores, typed economic erasure, ambiguous commit
cache behavior and complete R1-R8 remain open. SQL ship-coffer audit omission and
safe interrupted-build reuse are the next separate issues; captured backup
qualification still needs the requested local generation path. No inactive
spell-path behavior, activation authority or acceptance deadline was changed.


### October 3 bounded ship-coffer audit capture

The SQL independent cut now includes every persisted ships.money candidate and
reports unresolved authority/revision instead of silently omitting it. Both
engines' SELECT-only refusal, read-view, CLI and exact-restoration probes and
independent validation tests pass. These raw candidates are not mapped native
holdings and do not qualify ship gameplay, issuance, claims or enrollment.
Guilds' persisted denominations remain omitted and are the next established
native-audit gap; outcome_revision is not their monetary revision. Full R7,
writer/source completeness and all applicable R1-R8 gates remain open.

The October 3 current launcher fixture now includes its real watchdog script
and passes existing configuration/backup checks plus invalid-watchdog refusal.
Its retained frozen-suite failure is not relabeled as a broad-suite pass.

The schedule-failure fixture initializes all queue-accounting fields and now
passes strict warnings and sanitizers on the transport integration, retaining
queued command/counters on refusal. This fixes test drift without modifying
player-facing spell code. Current broad and real gameplay gates remain open.


The interrupted-build cache repair passes real compiler cancellation/resumption
and immutable-publication tests, then a 452.463-second strict production build
and full-world save/item/process-restart journey at native ce7550d6. Keep that
bounded proof separate from the incoming transport source 4180f745, captured
staging clone and measured integrated workload qualification. Current-source
broad and all full R1-R8 gates remain open; 600 seconds/-j2 are unchanged.


### October 3 bounded guild treasury audit capture

The independent SQL cut now includes every guild's persisted unsigned
money vector, distinct from mapped holdings and prestige/construction revisions.
Both engines pass read-only value/refusal/consistent-view/CLI/exact-restoration
probes; 81 focused tests pass. Guild money revision/lifetime and actual
same-root deposit/withdraw gameplay/recovery remain unqualified. Full R7 and
R1-R8 gates remain open. See the latest review status for source and failed/
passing evidence; raw capture does not qualify enrollment or activation.


The transport-integrated queue extraction fixture now includes the real
transport interface and passes strict warnings/ASan/UBSan. Eleven other bounded
transport owners and 24 watchdog fixtures pass at native 4180f745. Actual
transport/listener/accounting journeys, recovery and current-source broad
qualification remain independent open gates; no inactive behavior changed.


### October 3 transport-integrated native qualification

Native source 4180f745 passes strict backend/pfile/formatting, fresh inspector,
all transport lifecycle/fault journeys, listener budgets and ten native recovery
cases (545.753 seconds). Verified census identities are unchanged; refreshed
line anchors, 52 writer tests and contract validation pass while coverage stays
incomplete. The exact published-70aaa broad suite is running. Full-world/mixed
accounting workloads, captured staging generation, complete writers/native audit
and R1-R8 remain open. Plan 1 now distinguishes existing typed coordinator
components from missing actual coin/item lost-COMMIT reply and ordinary live
publication recovery proof. See latest review status for source/log hashes.


The maintained currency/item and legacy PA native SQL runner drift is repaired:
real strict RED links preceded current-closure/API GREEN, then exact binaries
pass complete native item and coin matrices on both engines with source4180.
This restores executable component qualification, not actual lost-COMMIT reply,
production pool, ordinary drop payload/publication or player route completion.
Those R1/R4/R8 issues and the broad/captured/workload gates remain open.


### October 3 actual SQL coin/item COMMIT reply-loss components

The previous coordinator cases synthesized ambiguity after repository success.
Their replacements hide exactly one successful native COMMIT reply, report the
lost-client error, require one broken connection replacement, then reconcile
through a fresh real session. Both MySQL and MariaDB pass the complete native
coin/item matrices with original operation/result, custody/value/evidence counts,
changed-envelope refusal, journal replay and one publication ACK/checkpoint.
Native source is pinned to 4180f745 (1,229 files, zero mismatches); no production
or inactive behavior changed. These fixture pool interfaces and manual ACKs do
not qualify production pool lifecycle, actual network disconnection, ordinary
live drop/give/pile publication or full R1/R4/R8 completion. The exact SQL room
payload prerequisite remains implementation WIP; broad/captured/workload gates
remain open. See the latest review status for hashes and additional checks.


### October 3 pooled replacement lease ownership repair

A deterministic native regression reproduced an outstanding borrower after
replacement refused during shutdown. Replacement now consumes a valid input
lease consistently, including closing/discard/factory-failure paths; a fresh
handle remains borrowed on success. Snapshot, death-conflict and locker callers
clear the old pointer even on NULL, avoiding release of a freed/reused address.
Strict/sanitized pool ordering, failure and capacity checks and both real SQL
engines pass rollback/server-session retirement/reborrow/shutdown. Strict backend
builds, pfile dependency check, formatting and all ten native recovery cases pass
at source tree c60330b58de9063dc1ad8510ced36310324d5d4c (1,229 files match).
Actual typed coordinator integration with the production pool is the next
bounded qualification; supplied factories and manual ACKs do not establish
production factory contention or live item/pile publication. Full R1-R8 remains
open. Room payload/season-fence implementation is separate unqualified WIP.


The bounded coin/item coordinator matrices now also pass both engines with
actual production sql_pool.c linked under ASan/UBSan, including successful
COMMIT reply loss, fresh replacement, clean reborrow and shutdown. Native source
c60330b58de9063dc1ad8510ced36310324d5d4c is pinned separately from room-payload
WIP. The configured factory and ACK remain supplied by the fixture; typed bank,
production boot/contention and ordinary gameplay cold-boot publication remain
open. See the October 3 status for exact binary and evidence hashes.

Six focused broad-fixture dependency/lifetime repairs now pass strict native
ASan/UBSan (corpse/input/retention/spell/dispatcher/zone). Actual runtime identity
and wakeup dependencies replace missing fixture linkage. Production inactive
spell behavior is unchanged. The frozen broad run retains its original failures;
focused repairs do not qualify a complete rerun. Details/pins are in October 3 status.
