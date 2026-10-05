# Finish accounting implementation plan

Updated: 2026-10-04. **Status: accounting activation and release remain blocked.**
SQL is the first delivery target; flatfile parity follows. This plan tracks the
current work. [Implementation history](FINISH_ACCOUNTING_IMPLEMENTATION_HISTORY.md)
preserves the dated checkpoints and their original evidence.
The [experimental review checkpoint](EXPERIMENTAL_REVIEW_CHECKPOINT.md) explains
the public branch scope, publication checks, and known blockers for reviewers.
The damaged production database requires the separate
[ownership repair and accounting opening procedure](../../operations/PRODUCTION_RELEASE_AND_OWNERSHIP_REPAIR.md)
before any production cutover.

## Current issue tracking (2026-10-03)

[#490](https://github.com/Community-Duris/Duris/issues/490) is the single
remaining implementation and qualification tracker. It retains all R1-R8 and
domain-plan acceptance, including the outstanding scope consolidated from
#487, #488, #489, #568 and #661. Those issue closures are supersession records;
they do not complete or remove the remaining requirements.

| #490 section | Remaining acceptance owner |
| --- | --- |
| 1. Ordinary money and item routes | Native authority, sources, UID custody, publication/replay and executable writer coverage |
| 2. Compound gameplay and recovery | Commerce, remaining crafts/enhancements, quest/spell rewards, death and corpse recovery |
| 3. Initialization and sourced NPC issuance | Former #568 baseline/starter grants and #661 active-accounting vial source/decision/replay |
| 4. Independent audit and reward projection | Former #487 reconciliation, protected bounded queries and canonical reward compatibility |
| 5. Audited corrections | Former #488 preview/apply, authorization, expected-state guards and linked recovery evidence |
| 6. Lifecycle, retention and verified restore | Former #489 deletion/reset, retained history, scoped exports and coherent restore |
| 7. Baseline, activation and safe pause | Consistent openings, guarded admission, observation/enforcement and reversible pause |
| 8. Integrated candidate and measured budgets | Current candidate, all supported backends, gameplay/fault matrix and declared workload budgets |

[#664](https://github.com/Community-Duris/Duris/issues/664) remains separately
open for historical quarantined-case reconciliation. Grant/save prevention and
restricted stopped recovery tooling are delivered; historical case eligibility
and authorized resolution require retained private evidence. Technical feature
completion does not establish historical recovery or production reopening.

[#551](https://github.com/Community-Duris/Duris/issues/551) is completed through
PR #693. Preserve the scoped active-accounting poison, Encrust and Harvester
[qualification evidence](ALCHEMY_ACTIVE_RECOVERY_2026-10-03.md). Older dated
statements about those routes being unfinished remain historical, not new work.

SQL is the first delivery target; MySQL, MariaDB and flatfile remain required
for full completion. Technical acceptance uses disposable databases and a
personal local server. Production repair, opening, migration, deployment and
restitution retain their separate evidence and authorization requirements.
Historical phase IDs in writer metadata continue to identify requirement slices;
their closed state is not evidence of qualified coverage. Link focused PRs and
current proof to the appropriate #490 section, preserving file-owner handoffs,
backend limitations and the tested revision before checking off acceptance.

## Current implementation and qualification checkpoint

Plan1 independent acceptance is published in8586ba589; Plan5 baseline-marker
readers and its lifecycle handoff are imported in db682ade5. Reviewed31-file
coin/collector/shared-owner source integration now follows that qualified base.
Those new inputs are unqualified; testing stays batched at major-plan readiness.
Flat active bootstrap and remaining money/item/domain writers are still required.


The [October 4 review status](REVIEW_STATUS_2026-10-04.md) records integrated
remote histories and primary production source through `9fabe54bb`, plus the
qualified ordinary-drop equipment-census correction. Both strict server builds,
focused components, 35 worker/journal guard cases per backend, bank/coin/item
coordinator/pool, lifecycle, 22 checked-release, 14 startup and 18 recovery-session
cases pass within their recorded input scopes. The actual ordinary drop now passes
guarded ACK and two complete native cold boots on each SQL engine. The maintained
gameplay regression now passes both engines against the current source, and
Plan 1's original independent acceptance is complete within those recorded scopes.
Plans 2–4, combined Plan 5, activation-owner integration and full R1–R8 release
acceptance remain unfinished. The
[consolidated save/recovery report](SAVE_RESIDENCE_INTEGRATION_2026-10-04.md)
retains the exact slices, source-established gaps and evidence limits.

Save residence, native synchronous/core ownership, reserved journal replay and
restored ordinary-drop guarded publication are connected. Covered ordinary frames
reuse locked native save revision; no additional snapshot receipt ledger or gate
is required. `1d6043f57` adds the live literal-checkpoint drop producer;
`480f20ce3` adds full owned lifecycle drain/close. `2078eb7d0` enables the existing
ownership epoch only after verified selected active SQL authority and preparation,
before critical replay and save workers. `cec4bd369` settles exact definitive
rejections without moving or reconstructing inventory. Unsupported enabled
maintenance, rename and copyover retain pre-mutation refusal; inactive behavior
and the declined spell path are preserved. No production accounting was activated.

`97fef7a09` aligns ordinary-graph admission with existing recovery eligibility
before mutation. Independent source review found no remaining production blocker
to starting Plan 1's existing qualification batch. Existing flatfile bank/item
parity, real SQL coordinator/pool and guarded lifecycle checks now enter that batch.
The existing native lifecycle APIs and stopped/disposable harness compose guarded
install, incomplete-manifest refusal, activation, pause and reactivation; a new
production CLI or all Plans2–4 writers are not independent Plan 1 test gates.
Real production activation-owner/verifier integration still belongs to full-release
R6 and cannot select an epoch before every plan supplies complete evidence.

Plan5 projection audit `a5d26b658` is imported as `cd0fbebb6`, after its claim/root
binding slices. Existing central native registrations cover those updated owners.
Zero-effect and orphan-reservation slices `e694798f6`/`bbb72ecff` are integrated
as `47aa60de0`/`4fdbb7033`. Orphan coverage now requires the fifth reservation
count; existing four-count snapshots must be recaptured by the updated exporter.
Peer evidence qualifies its own consumed inputs; intact native world holdings/items
remain absent and exports partial. Marker-v2 and mandatory `.elr` independent
inventory/restore/export/lifecycle consumers, command-preimage binding and full
release qualification remain Plan5 handoffs. Primary retains native formats,
cutover/complete-source authority and all remaining Plans1–4 writer integration.

The combined primary implementation remains unqualified as a major plan.
Historical semantic registry/matrix anchors remain unpromoted. Testing stays
deferred until each major plan is ready; Plan 1's batch is now running with the
partial passing results stated above. No qualified milestone push has been made.
Preserve original BEFORE sources, oracles, budgets and
all applicable R1–R8 requirements, including day-one routes and full backend parity.

## Current implementation and qualification

Current integration and native qualification are recorded in the
[October 4 review status](REVIEW_STATUS_2026-10-04.md), with historical results in the
[October 3 review status](REVIEW_STATUS_2026-10-03.md), the
[October 2 review status](REVIEW_STATUS_2026-10-02.md) and
[October 1 review status](REVIEW_STATUS_2026-10-01.md). Canonical, staging, and
master-prefix histories at the last qualified 0055 snapshot retain 55 receipts, preserving published alchemy
migration 0054 and adding exact room-item payload migration 0055. The staging
fork preserves its first 45 and appends ten; the master fork preserves its first
31 and appends 24, retaining existing runtime-state payloads. All three converge
on the pinned 226-table schema on disposable MySQL 8.0.46 and MariaDB 10.11.14 targets.
The current local integration includes incoming migration 0056 spell-ward durability;
its combined native, migration and player qualification remains pending. The writer
inventory now classifies 872 routes, 2,818 occurrences and 2,759 unique sites with
zero unmapped lexical sites. Four added semantic rows separately classify offline
payload repair and dispel object/timer changes outside the lexical scan. Their
backend status remains unverified. These counts do not replace route qualification.
The shared ownership and pending Plan 5 qualification interface are recorded in
[the stream handoff](SHARED_STREAM_HANDOFF.md).
The user's current execution cadence batches tests when each major plan is ready.
Continue implementation and code review, preserve immutable BEFORE sources and
regression fixtures, and defer new test runs until those milestones. Mark written
changes as implemented but unqualified until the required checks pass. Retain all
R1-R8, native/gameplay/persistence/recovery and final candidate acceptance gates.

Restored SQL ordinary-drop obligation registration and clean authoritative
hydration now have bounded component qualification on both engines. Original
operation release is actor independent and allocation-free, but production
critical replay/publication ACK callers remain unwired. Current strict inactive
flatfile creation/save/restart/relog journeys pass; initial production build
deadline failures and component fixture setup failures remain preserved. The
October 3 review status pins this milestone and its limits.

Successful historical ordinary-drop receipt verification now checks immutable
full-literal payload and native ledger/reference proof. Both engines pass actual
repository/pool corruption refusal, retained coordinator fences/journal through
retry exhaustion, exact repair/restart and later native movement/season history.
Strict production backends and current inactive creation journeys pass; the
original 300-second native compile gate passes in 287.557 seconds with the
same binary used for both-engine fault/recovery checks. These source-specific
results exclude incoming remote changes (then-current remote b84693f96). Local
history integration was blocked at that checkpoint by the retained no-merge
instruction and automatic approval review. The user subsequently authorized this
local merge; current combined-source qualification and normal publication remain
pending.
Complete ordinary-drop producer/replay/copyover and all R1-R8 gates remain open.
The bounded coin physical-publication/ACK and null-replay retention repair below
is qualified separately; production native reconstruction remains open.

Coin publication/ACK retention has a locally qualified bounded safety repair:
missing typed physical proof retains replay and refuses new schema-2 pile
admission; physical success and the original receipt survive ACK retry and
partial projection exceptions. The inactive composite path remains unchanged.
The native publisher/reconstructor and gameplay/replay/save/copyover gates remain
open. The qualified definite-admission repair is described below. See the October
3 review status for before failures, intermediate evidence and final checks.
Fourteen native cases per mode, both strict builds, 106 currency retention cases,
both queue modes and actual inactive creation/restart/three pickup journeys pass.
This is not full active accounting, native replay or current-remote qualification.

Definite admission refusal now has an explicit in-process disposition. Only
proven never-admitted commands bypass the impossible publication ACK; uncertainty,
ordinary ACK failure, malformed receipts and contradictory dispositions stay held.
The actual journal/coordinator and currency/item/craft owners pass 140 sanitizer
scenarios across both backend modes. Existing admission/capacity/fault/recovery,
106 currency retention, both queue, item publication, progression/restore and
14 coin-retention cases per mode pass. Both strict production builds pass their
original 600-second budgets, with all 1,232 source inputs verified. Actual inactive
creation/save/coldrestart/relog and three area pickup commands pass unchanged
budgets. Exact pins and preserved setup/driver failures are in the October 3
review. This bounded cleanup issue is solved locally; publication still awaits
local Git integration authorization and combined-source qualification. Separate
craft ACK-retry completion/progression cleanup is also solved locally. The
original ACK now precedes independent cleanup and business notifications; effects
and the original authoritative receipt survive retries without duplicate work.
Thirty valid before-source semantic failures become60 native passes across both
backend modes, with1990 assertions and16 supplied-artifact guards. Sixty-eight
existing admission cases, eight maintained owner suites and both strict
production builds pass. Actual inactive mortal Craft/Forge preserves pouch,
material/tool UIDs and exact XP through copyover and two cold restarts; inactive
creation/relog and three area-pickup controls also pass. These bounded results do
not qualify injected saved() hooks as SQL persistence completion, arbitrary
pre-ACK progression-hook throw/reentry, active gameplay, combined remote source
or a full route. Ordinary-drop replay/save integration and every full R1-R8 gate
remain open. Exact pins and preserved invalid fixture attempts are in October3
review; local Git integration authorization still blocks publication.

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
| Writer inventory | The generated matrix covers 868 routes, 2,817 lexical occurrences, and 2,758 unique sites with zero unmapped sites. Material downgrade has its own batch route; ordinary salvage remains separately classified. The helper, its call sites, its declaration, and four temporary material-probe cleanup sites are classified. | Executable route evidence and full native coverage; the release validator remains blocked. The scanner alone is not release proof. |
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

### Exact SQL room source-properties refusal

Native preparation now refuses a NULL physical properties projection instead of
accepting it as evidence for captured extra2/dynamic effects. Fresh actual-pool
MySQL and MariaDB rollback, real COMMIT reply-loss, exact-ID reconciliation/ACK
and cold component reads pass; both strict production backend builds pass. The
October 3 review status records the reproduced failure and source/binary hashes.
Ordinary player-drop literal checkpoint, held publication and replay integration
remain unfinished. The current inactive behavior and activation gates remain.

### Scoped full-literal checkpoint prerequisite

The opt-in literal scope now preserves selected complete bytes through ordinary
recapture/coalescing and exact worker database ACK. Original-operation holds
retain capture, terminal/death and target-login obligations while permitting dirty
marks. Fresh actual-pool MySQL/MariaDB native pipeline/repository/journal/identity
qualification and both strict production builds pass; see the October 3 review
for exact source/binary evidence. The four existing save/creation/death owners pass against final source, including
the real flatfile refusal guards. Flatfile begin/poll/hold refuse before side
effects; an ordinary local journal completion cannot grant a database ACK. Plan 1 records remaining actual drop preparation,
handler/action preflight, retained publication, ACK and restored lifecycle work.
No gameplay route has been promoted and no accounting activation is authorized.


Published fixture milestones f787dc9be and 5f83fbc78 repair exact 0055/226-table
inventory and real room-payload native linkage; focused inventory, immutable
history, sanitizer and direct SQL gates pass. Published 009085e12 retains bounded
redacted creation failure evidence before cleanup, qualified with three controlled
failures on the verified frozen native binary. The original failed broad run is
preserved. Actual starter-kit and unchanged-budget build/inspector reproduction,
current-head integrated qualification and all full R1-R8 gates remain open.
See the October 3 review for exact scope, source and evidence pins.


The read-only pending item-action reference query now passes the complete native
scheduler/runtime sanitizer owner, six assertion-rejected guard/reference/mutation
negatives and both strict production backend builds. It refuses before a drop
could trigger resource-changing departure cleanup, but no drop caller uses it
yet. Plan 1 retains actual preparation/publication/replay work and the separate
restored-obligation hydration design. A repeat of the frozen failing creation
scenario passes unchanged waits; intermittent/current-head qualification remains
open. Exact evidence and scope are in the October 3 review status.


Final dependent player-save/locker teardown now retains their operation owners
when critical shutdown refuses, preserving successful shutdown and pwipe rules.
The new native conditional-fragment test reproduces refusal loss before the
fix and passes afterward, alongside all existing contracts, both strict
production builds and real minimal flat boot/shutdown. Restored-drop obligation
registration/hydration/publication remains open.

The frozen e58bb3296 broad run is complete: 840 PASS/11 SKIP/10 FAIL. Six focused
schema/link-owner fixes are published; serial original-source recovery/pickup/
creation/network follow-ups pass with unchanged runtime/build assertions and
verified artifact reuse. The failed broad report remains unchanged, intermittent
starter-kit readiness and current-head integrated qualification remain open,
and no R1-R8 release gate is promoted. See October 3 status for exact proof pins.

The legacy partial SQL item-save dependency now has measured before-source proof
on both supported engines: eight semantic failures per engine, with existing
full/legacy, valid equipment replacement and two true postwrite rollback controls
passing. The complete partial forest/deletion-boundary/restitution guard is being
implemented and remains unqualified. Real pooled ordinary-drop and barrier-driven
save/drop/FK serialization checks are separate from the direct-client fixture;
none of these partial results completes a route or opens accounting. October3
review pins actual before evidence and preserved fixture/permission setup failures.

The partial-save prerequisite now has paired actual native qualification on both
engines:34 direct contracts/51 allocation ordinals and six real pooled-drop/direct
save serialization cases per engine pass. Selected-root custody/native closure,
guarded physical deletion and partial restitution scope preserve opposite payload
and legacy placement; new allocation failures roll back through query results.
Both strict production builds and the maintained inactive SQL save/death/crash/
restart/copyover journey pass. Exact declaration71055b7b and source/test pins are
in October3 review. Captured-journal inputs, full-save/pool-allocation coverage,
ordinary-drop native publication/replay/restored-save integration, broad current
source and full R1-R8 acceptance remain open. Inventory evidence is not promoted.

The next restored-save prerequisite follows Plan1's admission/release contract:
opt-in restoration census before startup replay, per-PID apply ownership, explicit
worker parking and selective exact-frame replay, affected-PID hydration fences,
and publication ACK only after clean save ownership or durable recovery handoff.
The worker-only parking/resume primitive is qualified: seven paired before-source
semantic failures and one real ACK-repair control become eight passing cases per
SQL-header and flatfile mode. Both strict builds and unchanged inactive SQL
save/death/crash/copyover plus flatfile creation/save/reload journeys pass. Final
declaration c1b7d3fc and October3 review retain exact pins and failed setup attempts.
The new protocol uses a controlled repository callback; normal pipeline callbacks
do not return deferral. Selective journal replay now has bounded paired native
qualification:16 before-source semantic failures and three controls become19
passes per backend mode. Held PIDs retain exact frames and lose earlier proofs;
unrelated PIDs checkpoint, and replay_deferred retains the global load fence.
Preapply collection allocation failure also refuses without losing frames. Both
strict production builds and unchanged inactive SQL save/death/crash/copyover
plus flatfile creation/save/coldrestart/relog journeys pass. Final declaration
5b86b898 and October3 review pin source, artifact and owned-teardown evidence.
Startup/registration, checkpoint-owned permits, affected-PID hydration, retained
wakes, independent ACK/checkpoint fencing and original publication release remain
pending. A per-pass marker does not establish a resident or cold-restart hold.
Worker allocation gaps are separate work; the later admission milestone below
closes retained submission only. Retry/promotion/result delivery remain open.
Unresolved callback proof withdrawal and bad_alloc classification now have
bounded paired qualification:19 before failures/four terminal controls become23
passes per mode; unchanged19-case deferral owners also pass per mode. Both strict
incremental builds, nine maintained owners,14 validations/30 contracts and actual
inactive MariaDB/MySQL plus flatfile restart/relog journeys pass. Final declaration
9ef8c4b2 pins exact source, native cases, gameplay and owned service teardown.
Ordinary SQL transaction/pool-lease exception cleanup, general postcallback
allocation safety and worker retry/promotion/completion allocation remain
separate; retained admission is closed by the later bounded milestone below. A newly confirmed typed snapshot-mask/exact-journal identity
race also remains open; native custody and stale-frame safety must be preserved. See Plan1 and October3 review for exact scope.
This does not authorize ordinary-drop wiring, active accounting or gate promotion.

The retained-admission allocation prerequisite now has paired component evidence:
16 native before failures/four controls become20 after passes per SQL-header and
flatfile mode. Empty owners and cancelable readiness are staged before moves or
revision claims; newer uncaptured marks refuse before replacing original state.
Eight unchanged parking regressions also pass per mode. Both strict incremental
builds and nine maintained owners,14 validations/30 contracts and nonmutating
matrix pass. Declarationfd0e36bf retains exact native inputs and case artifacts.
Actual inactive MariaDB/MySQL journeys and native follow-ups pass, as does
flatfile creation/save/cold restart/relog in131.800 seconds. Final declaration
20a99f56 reverifies native inputs, component cases, strict binaries/logs, gameplay
and owned schema/server teardown/port rebind. This bounded admission prerequisite
is solved locally. Scheduling/retry/promotion, result delivery, typed mask identity,
SQL lease cleanup and restored-save integration remain separate. Fresh remote
3dbb8bc83 adds ward0056; combined-source qualification and history integration
are pending under the no-merge restriction. No R1-R8 or coverage gate is promoted.

Worker retry/pending promotion now has paired component qualification separate
from admission:11 before failures/four controls become15 passes per backend mode.
Queue growth is staged before consuming results or mutating retries, revisions,
receipts or counters; allocation refusal retains the original front. The same
frozen cases assert exact later delivery and real journal ACK. Unchanged20-case
admission/eight-case parking owners also pass per mode (86 AFTER cases overall).
Component declaration36ec8996 and source manifest2576febf pin artifacts and all
1,232 production inputs, with only worker.c changed. Both strict incremental
server builds, nine maintained owners,14 validations/30 contracts and nonmutating
matrix pass. Actual inactive MariaDB/MySQL save/death/crash/copyover journeys and
native follow-ups pass; flatfile creation/save/restart/relog passes133.956 seconds.
Final declaration1c6b2a43 pins exact source, binaries, logs, cases and owned SQL
teardown/port rebind. This bounded scheduling issue is solved locally.
Results-push allocation, typed exact journal identity, SQL
transaction/lease cleanup, restored-save integration and current-remote/broad
qualification remain open. No R1-R8 requirement is marked complete.

Ordinary SQL cleanup now has actual paired BEFORE evidence:13 cases on each
private MariaDB/MySQL engine reproduce12 genuine failures/one consumed-null
replacement control. Unsafe borrowed settings are admitted; rollback/lease and
resource ownership fail at measured allocation seams. Failed conflict cleanup
loses original custody evidence and reuses an unsafe pool session, while its
existing idle gate prevents a second transaction. Declaration8e986daf and October3
review retain immutable source/owner/binary/case/teardown evidence and the first
strict fixture compile failure. Implementation/AFTER checks remain pending; this
does not complete any R1-R8 gate or qualify incoming0056 integration.

Worker completion delivery after a real exact journal ACK is now solved locally.
The allocating result deque is replaced by a fixed 256-entry FIFO under the same
mutex, capacity/backpressure and receipt ownership. Nothrow moves preserve
completion delivery when allocation is unavailable after the journal frame is
removed. Five proven BEFORE aborts/seven controls become twelve AFTER passes per
backend, including FIFO wrap, partial/full capacity, shutdown/reopen, exact typed
receipts, real ACK failure/repair and unrelated-PID progress. Unchanged scheduling,
admission and parking owners contribute another 86 AFTER passes (110 total).
Both strict incremental server builds, nine maintained owners,14 validations/30
contracts and nonmutating matrix pass. Actual inactive MariaDB/MySQL save, death,
crash and copyover journeys plus native follow-ups pass; flatfile creation/save/
cold restart/relog passes. Final declaration edd75c103ad576d8d8b0c994696a0108b09914e0533725db536a8cfeec4d9678 verifies all1,232 production
inputs, component artifacts, binaries/logs and owned SQL teardown/port rebind.
Preserved fixture failures remain evidence, not production failures. Ordinary SQL
cleanup, typed exact journal identity, restored-save integration, incoming0056 and
broad qualification remain open. No R1-R8 gate or coverage status is promoted.

### SQL cleanup candidate integration and native qualification

The reviewed five-file SQL cleanup candidate is integrated after committed worker
completion milestone dfc879598. Borrowed idle/autocommit/reconnect-disabled
sessions report original-session cleanup proof; pooled owners retire unconfirmed
connections, preserve consumed replacement ownership and ambiguous COMMIT, and
retain original custody evidence when rollback is unconfirmed. Separate retained
transaction cleanup precedes writer-fence release. Native buffer/result ownership
is scoped by RAII. Recovery_apply and sustained lifecycle lock allocation remain
separate work.

Manifest `tmp/sql-cleanup-production-inputs.local.json`, SHA-256
`efcb65d1faa86b6bd95eac6c576cf3b336033d26a01d0a16b5f46b475425de52`,
freezes1,233 source inputs: four changed repository source/headers and one new
SQL-only helper. All actual dependency-recorded header consumers are forced:
47 SQL/46 flatfile objects. Source preparation first refused Windows path
separator mismatch, then src-relative dependency parsing; both failed setup
attempts are archived before source/native object mutation. Corrected setup
matches exact dependency tokens and retains every consumer check.

Unchanged ordinary13 and retained4 owners are registered as explicit manual SQL
probes. Frozen AFTER drivers preserve original300/120 bounds, canonical0055
schema settings, all source/binary/owner/case pins, three rejection guards and
owned teardown/identity/port rebind. Strict builds and native AFTER qualification
are pending. Source integration is not a solved SQL issue or R1-R8 acceptance.

The user has now explicitly authorized the local Git merge needed to integrate
experimental-accounting. Finish the current bounded SQL cleanup qualification
and commit before integration; qualify the combined source before a normal push.
GitHub PR merge, deployment and production activation remain unauthorized.

Bounded ordinary/retained SQL transaction cleanup, original-session retirement,
replacement lease ownership and native buffer/result lifetime are solved locally.
The corrected source passes both native builds,34 actual engine fault cases,18
maintained terminal invocations,14 maintained owners, both inactive SQL journeys
and flat creation/save/restart. Final declaration0a0b5c5a is pinned in October3
review. Recovery proof/lease and lifecycle exclusion/release remain open.

Current two-stream ownership: primary owns Plans1-4 and shared coordinator,
contracts, producer integration, writer registry/matrix and activation owner.
The second user-coordinated agent owns Plan5 independent reconciliation, audit
tooling, backup/restore evidence and release qualification. Integrate separately
committed slices through narrow interface handoffs on one tested candidate.

## October 4 delivery priority

Complete Plan 1's documented independent authority/cutover acceptance first,
with Plans 2–4 progressing on independent producer work. Plan 1 need not wait
for every writer or flatfile coin implementation; guarded activation must still
refuse incomplete coverage, and actual activation/full release remains gated
on every plan. Prioritize production SQL recovery integration over further
isolated helper layers, followed by native baseline/cutover/pause and existing
flatfile bank/item parity. Testing stays deferred until the major plan is ready.

Source prerequisites241e54162 and5766555f8 now supply the replay ownership leaf
and full indexed SQL boot prototype catalog. Reconstruction consumes the latter.
Neither closes production ownership/ACK/replay or current-candidate qualification.
No defensible remaining duration or completion percentage has been measured.

Current source `1d6043f57` connects the actual ordinary SQL drop producer to
checkpoint, held native publication and separate ACK/release; maintenance slice
`c542a2642` refuses unsupported enabled deletion/reset before mutation. Plan5
root-binding slice `45c895ff4` is integrated in `24766ebff`, with independent
evidence restricted to peer inputs. All remain subject to combined qualification;
no new primary compiler/tests/native journeys ran or unqualified push occurred.
Direct core metadata ownership is source-integrated in `1192d08c5`, and clean
owned lifecycle drain/close in `480f20ce3`. Both are independently source-reviewed,
with qualification deferred. Ownership enable, remaining rejected recovery/graph
coverage, stopped-runtime maintenance connection, flatfile parity and major-plan
qualification remain. The existing lifecycle harness composes independent
install/refusal/activate/pause/reactivate acceptance; an online handover or new
baseline format is unnecessary. Preserve the group's scope without additional work.

Production source9fabe54bb now passes both strict builds, all eleven maintained
component owners, 35 worker/journal guard cases per backend, real pooled bank,
coin and item matrices and maintained SQL lifecycle/cutover checks on both engines.
Solved fixture issues fdd27c681,1dc242ed5,80565e8a7 preserve all original cases,
limits and production behavior. All22 prepared checked-release cases also pass
both engines; all14 prepared startup/held-save cases also pass both engines.
All18 current-source recovery-session cases now pass both native SQL engines;
actual active ordinary-drop publication/two cold boots remain open.
Existing guarded lifecycle APIs suffice
for the permitted disposable synthetic-coverage fixture; no new production CLI
or Plan5 waiver is needed. Plan1 remains incomplete. Continue original Plans2-4
producer work in parallel; private collector and inert-money proposals remain
unintegrated/unqualified and do not promote writer coverage or release readiness.


### Current flat runtime/origin source handoff

After the source-only 6379c713a shared-owner milestone and peer lifecycle-reader
import 621ba722b, native v3 initialization origin, protected lifecycle-file
registration, retained flat startup/current-source census and trusted shutdown
are integrated as source-only work. Matching independent v3 consumption and
original major-plan native/gameplay/recovery qualification remain pending.
Central flat coin admission stays closed. No required route, inactive safety
behavior, declined spell-path decision or original acceptance gate is changed.


### October 4 accounted shop source prerequisite

The [shop contract source milestone](SHOP_ACCOUNTED_CONTRACTS_2026-10-04.md)
freezes v6 original status and exact item after-image facts and the agreed shared
sink/issuance capability. Source review and formatting pass; native/runtime
qualification is deferred to major-plan readiness. Complete native payload
loading/saving, migration, producer/admission/publication and cold recovery remain
open; active shop refusal remains. No complete writer or release proof is claimed.
Plan5 backup/cold-restart slice94e81480b is imported and pushed in4c2abb329, with
three exact peer blobs and Python AST checked; its native evidence remains tied
to the peer's frozen older source. Matching v3 discovery and combined qualification
remain pending.
