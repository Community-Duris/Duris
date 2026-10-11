# Accounting acceleration board - 2026-10-11

## Current accepting gate

ACC-01 terminal PASS at 2026-10-11 01:18 MDT: fresh strict SQL and flat
production compile/link (789 objects each), maintained inactive flat boot/shutdown,
and 10/10 runtime source-contract tests on frozen
`6022c0ad7feab34c13e27738d4295b65b8d010ac`. Source unchanged; owned container
absent, receipt RELEASED and original heavy lock removed. Detailed evidence:
`ACC-01_COMBINED_BUILD_2026-10-11.md`. SQL mutable startup remains assigned to
Plan 5; this checkpoint does not qualify that gate.

ACC-02 attempt 1 FAILED before migrations at 2026-10-11 01:25 MDT. The
maintained readiness loop exhausted after MySQL exited with code 1; no successful
version query, schema history, measurement, MariaDB or QA execution. Private
frozen candidate `a60f5dcce0c6496e687a0806ee7d0c559579e4fb` remained unchanged;
owned inventory succeeded and was empty. Failure sealed and lease released;
reviewed report imported from Plan 5 `774d4422f`:
`PLAN5_ACC02_MYSQL_READINESS_FAILURE_2026-10-11.md`. Foreground commissioning
01 captured the startup error below; one-variable commissioning02 PASSED
actual TCP readiness/version8.0.46 and clean shutdown. Restoring original image
Linux `/tmp` is verified for this exact profile. Full unchanged two-engine matrix
RETRY-02 FAILED at 02:00 MDT: MySQL8.0.46 completed canonical/staging/master65
convergence and native session faults; MariaDB10.11.19 completed canonical65,
then staging upgrade failed applying `0060_native_mobile_item_owner`. Retained
daemon log reports an InnoDB rename with missing `item_owner_revision.ibd`.
Both daemons were running and OOMKilled=false before guarded cleanup; this is
not a proved migration-logic or storage root cause. Native schema65 accepting
boot/tamper and final two-engine contract update were NOT_RUN. All 6901 source
files and private contracts remained unchanged. Final owned inventory exit0/
empty; evidence sealed and matching global lock RELEASED at02:09 MDT.
Reviewed delivery `d7571a7e1`, detailed report
`PLAN5_ACC02_MARIADB_STAGING_FAILURE_2026-10-11.md`; independent verification
passed all6967 sealed members/550329309bytes and fresh owned absence.
No full matrix or schema65 acceptance claimed. This follows the exclusive
01:22 MDT grant of at most two 512MiB databases and one serial 2GiB QA worker.
No WORLD/SHOP heavy grants. First checkpoint target (within 12 hours of
orchestrator creation at 00:43 MDT) met; SQL qualification remains pending.

WORLD ACC-03-RC FAILED at final link on frozen6022; six distinct unresolved
symbols have three real definition providers omitted from its source list.
Runtime/sanitizers NOT_RUN, source unchanged, sealed/RELEASED02:25 MDT. Reviewed
three-line recipe repair integrated and verified by native RETRY-02 on
frozen6bfa0f7eb: compile79.410s/runtime0.418s exit0 under ASan/UBSan, original
assertions/flags/20s deadline, component-only PASS. Sealed/RELEASED02:50:35 MDT.
Plan5 FS-01 terminal paired result: Windows-data control reproduced0060 rename
failure; fresh native Linux volume passed unchanged original staging65 checks.
Storage profile verified for that slice, mechanism not independently isolated;
fullmatrix/native65 startup unqualified. Sealed/released; passive data retained.
Full RETRY-03 RUNNING since03:10:55 MDT under the sole heavy lease;
MySQL canonical/history checks progressing; MariaDB/final accepting65 gates pending.
Both accepting profiles and native component
recipe repair are established; this is not full two-engine qualification.

Accounting inactive; admission CLOSED; final release BLOCKED. This board records
execution, not inventory-based completion. Original Plans 1-5 and R1-R8 evidence
keeps its exact historical scope. Architecture work follows the first checkpoint.

## Owners and source pins

Human authorization verified directly in source chat `01a104b8-0bcd-7863-b720-a15cad96df41`:
five-session plan followed by "do it". Local model is GPT-6.1 Sol / xhigh.

| Lane | Chat / worktree / branch | Current ticket |
| --- | --- | --- |
| Orchestrator | `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`; `C:/Users/alexa/.codex/worktrees/5708/NewDuris Max`; `codex/accounting-accel-integration` | Reviewed/integrated recipe PASS and FS-01 profile; full matrix RETRY-03 live under verified lease |
| Remote primary | Non-connected RGitGoin; private session/WIP inaccessible; published `experimental-accounting` | Shared lifecycle map and implementation; acknowledgment and toolchain/storage/32MiB proof queue |
| Plan 5 | `01a104cf-6aba-7021-b5cd-56345f75cf10`; `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`; `codex/accounting-plan5` | Clean dc1cc3ddf; full RETRY-03 frozen6bfa RUNNING sole heavy lease; FS-01 sealed/RELEASED, restore queue retained |
| World/quest | `01a129b4-a57a-76e0-baec-42b3dd2fe62f`; `C:/Users/alexa/.codex/worktrees/2c0d/NewDuris Max`; `codex/accounting-accel-world` | Clean f7228f3fe; ACC-03-RC original link failure preserved; RETRY-02 PASS_COMPONENT_ONLY/sealed/RELEASED frozen6bfa; runtime lease inactive |
| Shop/backend | `01a129b4-a553-78e0-85c2-69e34d8e0d59`; `C:/Users/alexa/.codex/worktrees/969b/NewDuris Max`; `codex/accounting-accel-shop` | ACC-04-TX queued, no heavy grant; proposed runtime lease inactive |

- Native source: `83e2e4392064667fc4f64780183d17b5acc431ef`.
- Launch source: `6022c0ad7feab34c13e27738d4295b65b8d010ac`; only charter and plan pointer added.
- Fresh published observation after ACC-01 terminal: primary runtime delta
  `757c3f6fe` plus launch-readiness identity clarification
  `9d88a46684dfa40538a14a589b35e6a5b79c7abf`; bounded source review below.
  These newer bytes are outside frozen ACC-01 and ACC-02 evidence.
- Previous combined executable checkpoint: `3049fb7712de7ccd957c61d5d345d2c8a4cf838d`;
  cannot qualify current source.
- Schema65 MySQL8/MariaDB10.11 fingerprints: null / null; qualification false.

## Exclusive file leases

PRIMARY_ACK PENDING at published `54fef6421`; charter unchanged from
`9d88a4668`, protected WIP/exclusions undisclosed.
No elapsed-time or local observation substitutes for primary-authored published acknowledgment.
The primary's `757c3f6fe` explicitly retains proposed leases pending direct
authorization. The docs-only `9d88a4668` addresses RGitGoin / the remote Plans 1-4
owner by name and requests any need for a private human instruction. Awareness
and local clarification do not establish acceptance. No maintained worker edits.

| Owner | Paths | Status |
| --- | --- | --- |
| WORLD | `src/world/db.c`, `src/world/quest_mobile_native_birth.c`, its private header | PROPOSED; no maintained edits |
| SHOP | `src/economy/shop_trade_transaction.c`, its private header, `src/sql/sql_player.c`, `src/flatfile/flatfile_item_repository.c`, its private header | PROPOSED; no maintained edits |
| PRIMARY | save pipeline, critical coordinator, common item movement, shared headers/contracts, schema/registry/matrix, activation and all undelegated files | Retained |
| PLAN5 | Established independent operator/audit/backup/restore paths | Retained; exact active slice pending handoff |
| Orchestrator | This board; reviewed integration deliveries on own branch | Active docs lease; no runtime implementation lease |

Primary acknowledgment request: publish exact accepted paths, source/result pin,
protected WIP/pending edits/exclusions, and release or renewal rules in the charter.
Primary remains sole writer of `experimental-accounting`; local owners push only
their assigned helper branches. No additional sessions or subagents authorized.

Exact additional WORLD request: `src/world/quest.c` owns
`submit_native_quest_give` -> `quest_native_gameplay_owner::begin/pulse` ->
`quest_native_completion_owner::prepare_original`, continuation restore/pair ACK
and reward. Its flat preparation/phase2 refuses; SQL requires authentic runtime
NPC/item/cash references and accepting authority. Proposed first genuine QP06:
29262/29263/29264 -> 29237/C3000/E2500. Primary must acknowledge that additional
path/source/WIP before WORLD edits it. Legacy inactive mini-world ACK runner
does not qualify genuine native birth/admission.

Exact shared SHOP request remains PRIMARY-owned: `src/economy/economic_command_admission.c`
and `.h` (real bounded schema2 SHOP selection; currently unavailable),
`src/player/player_save_pipeline.c` and `.h` (factory-issued original operation/PID,
generation/epoch, full forests/revisions, one hold, authenticated current result,
once-only publication/cancellation/guarded ACK),
`src/persistence/economic_sql_shop_trade_transaction.c` and `.h` (same root session
lock/current/execute/receipt plus different-PID keeper fencing),
`src/flatfile/flatfile_accounting_shop_transaction.c` and `.h` (same authority
lock/current/apply/verify, uncertain outcome and journal compatibility).
Current consumers are retained SHOP preparation/native checkpoint, coordinator,
accounted dispatch and cold/live publication. These paths are requests to PRIMARY,
not additional active SHOP leases. Require exact-ID uncertainty, committed-unpublished,
lost completion, keeper contention, cold full forests/absence, repeated ACK and
cleanup proof in existing transaction/retention/native restore journeys. Do not
override availability or invent tokens/receipts for accepting tests.

## Queue and proof contracts

| Ticket | Accepting result | Dependencies / next step |
| --- | --- | --- |
| ACC-01 | Both fresh production binaries and inactive flat preflight PASS; runtime source contracts 10/10 PASS | Terminal frozen6022 proof; mutable SQL startup assigned ACC-02/Plan5; newer native delta needs follow-up |
| ACC-02 | Genuine two-engine schema65 matrix and exact restore/startup histories; independent audit and native room/restore gates | Both commissioning repairs verified, failures preserved/raw MySQL FP unqualified; unchanged full RETRY-03 RUNNING on frozen6bfa |
| ACC-03-RC | Existing `test_native_quest_recovery_context.py` actual-provider ASan/UBSan component or exact first failure | Original link failure retained; three-provider recipe repair verified by18-TU ASan/UBSan component PASS/sealed/RELEASED; runtime acknowledgment still required |
| ACC-04-TX | Existing `test_shop_trade_transaction.py` component or exact first failure, then retention/repository gates | Queued after schema65 job; private publish/admission capabilities abort in retention harness and are unqualified |
| PRIMARY-01 | Connected save/coordinator/item lifecycle map then verified boundary/caller/admission closure | ACC-01 evidence; no collision with acknowledged leases |
| PRIMARY-02 | Supported toolchain/runtime/library correspondence, complete storage/frame/transitive proof, aggregate 32MiB accept/refuse | Remains explicit; object compile/source packets do not close it |
| FINAL | Gameplay/persistence/schema/backup/restore/recovery/fault/load/resource/R1-R8 gates agree on one frozen candidate | Reviewed implementation batches and per-slice architecture evidence |

Maintained ACC-01 commands (terminal PASS):

```sh
make -C /source/src -k -j2 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production \
  BIN_ROOT=/source/bin OBJDIR=/source/bin/objects/sql/production \
  DMS_BINARY=/source/bin/server/dms_new
make -C /source/src -k -j2 PERSISTENCE_BACKEND=flatfile BUILD_PROFILE=production \
  BIN_ROOT=/source/bin OBJDIR=/source/bin/objects/flatfile/production \
  DMS_BINARY=/source/bin/server/dms_restore_flatfile
python3 tests/async/test_flatfile_boot_preflight.py \
  --server /source/bin/server/dms_restore_flatfile
```

SQL restore startup contract: `test_persistence_backup_integration.py` requires
both binaries at `ROOT/bin/server`, builds real native fixtures under `ROOT/bin`,
uses explicit `DURIS_RUN_BACKUP_INTEGRATION=1` and MySQL opt-in, private sockets,
Linux namespaces and isolated tmpfs. The selected full-dump service-boot methods
remain Plan 5 coordinated mutable jobs. Full schema65 matrix uses
`test_staging_migration_fork_mysql.py --update-contract --report <owned-json>`;
both engines must execute in one qualification. Schema constants remain primary-owned.

## Resources and evidence

Bootstrap coordination receipts: `D:/Dev/Temp/accounting-acceleration-bootstrap-20261011`.
ACC-01 outputs: `D:/Dev/Builds/Duris/accounting-accel-integration/ACC-01/bin`.
ACC-01 export/logs/results: `D:/Dev/Tests/Duris/accounting-accel-integration/ACC-01`.
Temporary files/cache: `D:/Dev/Temp`, `D:/Dev/Cache`.

Inspected before any job: no `duris-build-a/b` containers; seven existing running
services belong to unrelated owners and remain untouched. Pinned known QA image:
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
Toolchain must be observed in the owned job; old image label is not compiler proof.
Measured free Windows memory: 4.58 GiB of 31.75 GiB at bootstrap. One heavy job
at a time; no second slot without resource grant and measured memory.

No project controller was identified in maintained scripts or bootstrap receipts.
Explicit exclusive task receipt `ACC-01-lease.json` and `heavy-job.lock` acquired at
2026-10-11 00:49 MDT. Job `ACC-01-6022c0ad7`, attempt 2, disposable `duris-build-a`
ID `cee234bd9277fda9fd5874c760509140cd86c754e6b69990ba4dbc105a2dffb7`, exact
image above, read-only D source plus direct D bin/evidence/cache mounts, restart no,
network none, CPU2/memory3GiB. GCC13.3.0 observed; both strict production builds PASS.
Attempt 1 failed before compiler: nested `/source/bin` mountpoint missing under
read-only export. Failure receipt preserved; empty directory added, no source-byte
or flag change; first lease released before retry. Scratch `/t` is private 128MiB
tmpfs for POSIX ownership checks; persistent evidence/build/cache stays on D.
Frozen archive SHA256 `74774296e26462008de621c727752c4631a75337b3510620f3db29600caea8d0`;
src tree `df568142d3c57d74683bed76da2a0eec67d4a326`.
Receipt
must identify owner chat, job, exact image/container, direct D mounts, purpose,
start and release condition. Disposable jobs use foreground `--rm`, restart no,
finally cleanup; release after evidence capture. Preserve unrelated services,
volumes, recovery containers and all evidence. ACC-01 released in host finally
after terminal result and exact-owned-container cleanup; seven unrelated services
remain untouched. ACC-02 resource grant and matching exclusive lock bind Plan
5/source a60f5dcc, reviewed wrapper SHA256
`dbf53eeb1ddd56423da18b3f0621a5786d254f0a2c7e53ad33f872b6e11d8efb`,
QA image above, MySQL image
`sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b`
and MariaDB image
`sha256:ce66c7be32a03aabe7241d0a10993a2db827ef652a35d25727d92a832ac8ef73`.
Grant SHA256 `f1dc01974c4fa592a0f7d5d7e1f1ac1dfc5dd9eada9360018fd669fb11e6afb8`.
Measured free Windows memory exceeded 5.6GiB before granting the 3GiB cohort.
Matrix evidence `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-a60f5dcc`; direct
D build `D:/Dev/Builds/Duris/accounting-plan5/ACC-02-a60f5dcc/bin`, backend
data/tmp and host scratch isolated under evidence. Release requires terminal
sealed evidence and successful empty exact-owned container inventory; failed
inventory keeps lease unresolved. WORLD/SHOP remain queued. Attempt1's detached
DB auto-removal erased its server log before maintained finally; Docker die/
destroy events survive but do not attribute the cause. Preserve the failed
private datadir/source/transport logs; diagnostic must capture foreground server
output before an evidence-driven retry. No runner predicate is weakened.

Attempt1 immutable evidence manifest SHA256
`8efacec5bc181b68a3fe9b8a86d20d0338c4a0ac2a1decca06bd581aa041c77e` binds
6919 files/549560141 bytes; generated daemon data/keys remain private outside
the published manifest. Independent ownership inventory was successful/empty;
matching lease was released only after sealing and recheck.

ACC-02-COMMISSION-01 (fresh foreground diagnostic, same pinned MySQL512MiB/CPU1,
network none/restart no/default image entrypoint and args) reproduced exit1 in
31.609s. Temporary mysqld8.0.46 reached Unixsocket/port0; timezone-table import
then emitted `Unable to resize binlog IOCACHE auxiliary file` and
`ERROR 14 (HY000): Can't change size of file (OS errno 2 - No such file or directory)`.
No successful TCP readiness, schema or migration. Exact CID
`8d421be82de5c040f901942a8ef1d08f1babe88f05104127eca14bb63c91093d` automatically
removed; final owner/job inventory succeeded/empty. Diagnostic output
`D:/Dev/Tests/Duris/accounting-plan5/ACC-02-COMMISSION-01`; result SHA256
`e29eea10319dabd4d86901a15b11987ad6d7a6eea2f6021d9b0e2eacc595c750`,
daemon stderr SHA256 `2f74682bb6104f4ae672711c4ad0b5fabbc1e30d9697866e6ee16d720da9a9bf`.
This establishes the startup error, not a proven filesystem cause. The next
bounded comparison removes only the added WindowsD `/tmp` bind, restoring
the original image Linux `/tmp` on Docker's registered D virtual disk. Keep
the same direct D datadir, image, args and resource caps; preserve failed scratch.
Full matrix retry remains separate, after commissioning proof and transport review.

ACC-02-COMMISSION-02 PASSED, with only the added WindowsD `/tmp` bind removed
and fresh task identity/scratch. Actual TCP `SELECT VERSION()` returned8.0.46,
31 readiness attempts, total42.515s including owned graceful shutdown, exit0.
Same pinned image512MiB/CPU1/default entrypoint/args/directD datadir; Linux
temporary filesystem uses Docker's registered D disk. Exact CID
`70a4c34ab8ff1c7b8a5c89b476b42a3f48b145979911b78dbb5c389bbb71615d` inspected
with matching image/owner/job before stop; foregroundexit0, final inventoryexit0
and empty. Result SHA256
`de5ec9b0f19a07a481503a10dfde3ee49088ef540679163c25a8fada76742cfb`, output
`D:/Dev/Tests/Duris/accounting-plan5/ACC-02-COMMISSION-02`. Paired reproduction
establishes the added `/tmp` bind as the startup-failure trigger for this tested
profile; schema65/MariaDB/native startup/release remain unqualified. Preserve
both failed and successful scratch/logs. Retry transport will restore original
Linux `/tmp` and remove the added detachedDB `--rm`, so original guarded finally
captures stopped daemon logs before exact-owned removal. No maintained predicates,
server config/defaults, resource caps or source behavior will change.

Reviewed paired report imported from Plan 5 `4516f495e`:
`PLAN5_MYSQL_TEMPORARY_FILESYSTEM_REPAIR_2026-10-11.md`. Positive seal SHA256
`38d3c6092f1d8c3a07ad141f1155f23f12c2b7a04d294117bff8259986209203`;
negative seal `3f5da42d3f9297ac13fc455f9acf2e69ded3cc45db3899c23d2d14cfb979e643`.
Both diagnoses' matching leases released after exact-owned cleanup and sealing.

RETRY-02 exclusive job `ACC-02-schema65-RETRY-02` retains sourcea60,
6901 source files/inventory `b2f03f7c9042eb4d5f48f7d33330a7b7e6b909f0db3a01d155f31d37cf8c4ff6`,
unchanged runner `908ab9a227cf732b86bd3503aa0d3ccec07e0d2f35998568ea41d56543ba723d`.
Fresh evidence/source `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-a60f5dcc-retry02`,
build `D:/Dev/Builds/Duris/accounting-plan5/ACC-02-a60f5dcc-retry02/bin`.
Reviewed wrapper SHA256
`3221900217ab0ef54a64613d3c50ffce8a7bf59a396a67b7ce456d3d88e475a4`;
actual final preparation SHA256
`6f9993608635d996a4a13e73292e867143118465d8553b9b512baae9f1667b1f`.
Preparation annotation replaced obsolete pending bootstrap text after initial
hash `775e08f9`; no source/dependency bytes changed. Resource grant SHA256
`cae20131b920a12e78b6d62e94c95039b2d39d8322dbb0faab61323d670ed752`,
free Windows memory 9.79GiB; unchanged two512MiB DB plus one serial2GiB QA cap.
DB default Linux `/tmp`; exact stopped daemon logs captured before original
guarded finally; persistent data/build/evidence directD. Measured writes allowed
only to the two private contract files; PRIMARY owns maintained constants/pins.
Release requires sealed terminal evidence and successful empty owned inventory;
other workers have no heavy grant. Later primary5418 bytes remain outside freeze.
Session69068 terminal exit1; actual MySQL CID
`3e12e22f8ee41aef7114d6879ac45de1d321dfe2c9cf08915cc608f212c809af`,
generated `duris_268_66809a13c6_canonicaltest` and serial foreground QA steps
executed the original runner. Owner/source/job/image and final preparation hash
recorded in its receipt. Retained raw MySQL convergence fingerprint
`0cb5c5bdbf0862e115a933dc03b84e46a4a0855edb83cdd2c791ed66b42469a1`
is measured history/metadata evidence, not an accepted schema65 runtime contract.
The last QA migration command exited2 after 4.906s; daemon stderr records the
rename error at07:59:54Z. Docker transport journal has603 calls. Both databases
were removed by guarded finally, source_changes={} and final inventoryexit0/
empty. Plan5 sealed retained client/daemon/transport/source evidence and released
the matching global lock after rechecking owned absence. Evidence manifest
SHA256 `4d0e5992f40dc1f8277154c0bf5268574ca96f02aed22c68aca98e6eacd415a7`;
delivery result SHA256
`c8d18e59dfc7c3786feaa5b2e22a008c80c8458e7f1c04d869468041ede34367`.
Orchestrator independently hashed all6967 members/550329309bytes, matched released
lease and fresh owned `docker ps -a` exit0/empty with global lock absent:
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-02-RETRY-02-review/review.json`,
SHA256 `5e8d7a7b75f89e52984a8f0459b616af7e7fccd25b9169dd862bba88ea8d992b`.
Import only the reviewed124-line failure report from clean published Plan5
`d7571a7e11459695f0e5e68cdef0bbc3388e2c35`; additive curator follow-up remains
on Plan5 branch. ACC-01, both commissioning and RETRY-02 allocations are released.
Next independent WORLD allocation is recorded below. Prepare the smallest
paired original MariaDB staging upgrade with fresh isolated Linux data storage
on Docker's registered D: disk; Windows data-bind interaction is a hypothesis
until executable comparison. Preserve immutable0060, original histories/checks,
resource caps, SQL defaults and failed data. Do not restart the failed database.

Plan5 comparison `ACC-02-MARIADB-FS-01` terminal CONTROL_FAIL/LINUX_PASS,
sealed/RELEASED. Exact fresha60/
6901files; helper SHA256
`aa427c4bdfa2b5f9bb690de0bb88b00e759f7f435dd2a3d4713c1858563a1e24`,
preparation SHA256
`e815dd82bcd0e2a2f39d75a849f395a66e404ff4195e5a79a5bba26ae1e77c3a`.
Orchestrator independently ran `--check-preparation`: PASS, zero resources.
Exact maintained staging block lines454-480 and history/state predicates; two
serial arms, fresh directD data and fresh labeled Linux Docker volume on its
registered D: disk. At most one512MiB/CPU1 DB and one serial2GiB/CPU2 QA;
unchanged images/defaults/checks, no contract writes or full65 boot/qualification.
Control must reproduce a comparable failure before any causal claim. Verify
exact ownership, capture logs/state, graceful stop, retain private data/passive
volume, remove only owned containers, require successful empty inventory and
sealed evidence before matching lease release. Distinct grant SHA256
`8c936928645c1c9a7a1ecafdb5fb73824f0440050891753f57bfdd4f5e06c146`
at02:29:31 MDT, freeWindows8.72GiB, prior WORLD sealed/released and compatible
slots/arm containers/volume absent; only seven unrelated services before start.
Grant explicitly permits the fresh labeled passive Linux volume, retained after
diagnosis. Own global lock RUNNING SHA256
`cfef55713d4b158c1324741d3f2968bdd0a7f9fee84365530975768a841b352d`;
actual control CID
`e9a160c88cdfcca0b0b5f97e226d0efc861b4d37e8ce5b0f17a74a12429f1f33`.
Review SHA256 `f540ca5567fc126580f4f585866c9f8776704c414a78059ec614b6d7839acc30`.
Evidence `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-MARIADB-FS-01`.
Both actual10.11.19 with identical SQLmode/check enforcement/entrypoint/options;
controlv9fs/automaticcase2 vs Linuxext2/ext3/automaticcase0. Native Linux arm
passed original staging prefix/selector refusal/append/rerun/protected history/
data/count and registered state `9e81f924...534f97`. Terminalexit1 retains the
failed control; no cleanup/source failure. Both graceful shutdowns/daemon exits0,
owned inventoryexit0/empty; no active volume consumers. Comparison qualifies only
the storage profile for this exact slice, not the case/kernel mechanism or65
fullmatrix/boot/restore. No immutableSQL/default/check/predicate/cap change.
Seal SHA256 `0dc00da9c1240b365ff62ec8e4594b2da3511ac2380c321cf0e2f203fb198961`;
orchestrator verified all6946members/550808899bytes, released lease and fresh
owner/volume-consumer empty inventory/globalabsence; terminal review SHA256
`3e48965ec432218e10f0484a46184886e95ae6c7417b80cc79beea4b3a9ce550`.
Import only125-line report from clean published Plan5
`1e26f584314e74963a617381e209673891509625`:
`PLAN5_MARIADB_DATA_STORAGE_REPAIR_2026-10-11.md`.
Passive labeled local volume `plan5-acc02-maria-fs01-linux-data` and private
stopped-data copy retained as diagnostic scratch, not qualified backup/restore.
Full unchanged RETRY-03 follows below; only the Maria added Windows data bind is
replaced by fresh Linux volume, with guarded graceful evidence capture/retention.

WORLD `ACC-03-RC-6022c0ad7` terminal FAILED/RELEASED02:25:25 MDT. Receipt
`D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-03-RC-resource-grant.json`
SHA256 `e87ecac7a1830fe272a70b19bac7d1d29df923975ec12a33e84c6becea36014e`.
Prepared receipt SHA256
`b1566034b74da232003499a97fc456efa4b2e3151377954fc1c561436064ca92`;
review SHA256 `c620283b1b5b808341565b2196358e95f2d0a602161a5cb3eb247973def4b094`.
Independently verified1422 frozen source/fixture files, original Git runner/test,
archive/helpers; source readonly, directD bin/evidence/cache, private Linux128MiB
tmpfs. Runner has no BIN_ROOT override; original ROOT/bin/tests layout retained.
Actual GCC13.3.0/sanitizer library pins and cgroup limits checked before invocation.
Exact owned `duris-build-a` CID
`dbc76721b8b99ad2f7ee2f9fb6dc635897bb77cb9d7bb4d894970ea5e9bafa49`,
image74b699, token `fd09940a975647deb6948ef68279ae8c`. Foreground--rm/restart:no;
oneCPU/1536MiB/no swap/networknone,900s outer bound and original20s native runtime.
FreeWindows9.32GiB and only seven unrelated services before grant; no compatible
build slot existed. Helpers preserve existing flags/sanitizers/predicates, guard
cleanup by CID/image/owner/job/token and retain terminal evidence. Successful
empty owned inventory/sealing preceded matching lock release. No maintained
runtime edits. Evidence:
`D:/Dev/Tests/Duris/accounting-accel-world/ACC-03-RC/attempt-01`.
Original compiler driver exit1 after76.958s, six undefined real provider symbols;
zero linked binaries/runtime attempts, outer92.027s. Artifact seal SHA256
`32dbb848d0d4f5334bfb41f7cf7795df235087b2178218ceda7d81afd39f1d61`.
Orchestrator independently verified all13 members/783455bytes and released lease,
fresh exact-owned empty inventory/global absence; terminal review SHA256
`3ca0c51531fe66c6e02b5603059091d67b40e7728ffdfcf9232a8c09a530bdb2`.
Import only99-line report from clean published WORLD
`6d9c57e8439988205886769790cd1e073a57229f`:
`ACC-03_RC_RECOVERY_CONTEXT_2026-10-11.md`.
Reviewed patch SHA256
`7d8ff2f8df062a525c9c1001d31b27c038afea9ad63d78edf9e43841ff36811b`
adds exactly three real SOURCES entries to the existing test runner:
`native_quest_cost.c`, `native_quest_coin_give.c`,
`lockpick_retirement_continuation.c`. Definitions and original production recipe
independently matched. Patched runner SHA256
`f719c84ce0a450eb565f6d6bf861f836e3e96fbb4cc11049cf6dfa70c53f9e0c`;
18translation units, removing only those lines recreates original15-unit bytes.
Original assertions/flags/link/sanitizers/20s deadline unchanged; Python syntax
and diff checks PASS. Actual native retry result below verifies component closure.
Candidate recipe integrated at `6bfa0f7ebdd5f2f46841047fdd10522e4d27dfea`;
no runtime/authority fix or component pass claimed before actual retry result.

WORLD `ACC-03-RC-RETRY-02` PASS_COMPONENT_ONLY/sealed/RELEASED02:50:35 MDT,
started02:48:58 MDT as sole heavy grant after FS-01
seal/release. Same1422-source export graph, frozen6bfa/patched f719 runner;
source/helper identity-only diff independently verified. Prepared SHA256
`ec6d35338cafb24f28dd2e0ef9f588fba303e19ad3226d5ad5b8f89c9ede80d5`;
grant SHA256 `b0b589cf55b32fccb5a8e4dd6e843546ef1212e459c2f5af4614ae23c79fd75d`,
freeWindows6.96GiB, exact slots absent and seven other services preserved.
Owner token `a9c13fd02db1418695435e1aa3b3aa1f`; actual duris-build-a CID
`4f46030568426543c33217e8d4dbe0bea12f875fc600e7690c486e6e16ea847a`.
Same image74b699/oneCPU1536MiB/noswap/networknone/900s outer/original20s runtime,
directD source/bin/evidence/cache and private Linux scratch. Exact retained
flags/sanitizers/assertions; foreground--rm, guarded finally seals and checks
owned empty inventory before matching release. No SQL or other heavy grant.
Evidence `D:/Dev/Tests/Duris/accounting-accel-world/ACC-03-RC-RETRY-02/attempt-01`;
review SHA256 `f8fbf4286e9a5bcfcf8357977deb7b17e8090a5b397a55a1bd2848f4606b086c`.
Actual18-TU compileexit0/79.410s, one native runtimeexit0/0.418s, outerexit0/
96.080s; no sanitizer diagnostic. All1422source files unchanged, no predicate,
flag or deadline change. Terminal seal SHA256
`d7fa9fadb6c18e1c7e96db493e6c7aa6d3b624a513f7ad4342ac9b53f1b2e198`.
Orchestrator independently verified all15members/35054108bytes, released lease,
fresh exact-owned empty inventory/globalabsence; terminal review SHA256
`0de384548c83d6eda690a750a42c30c35efe6ace61dfe35c97fedef42d28e990`.
Import only100-line report from clean published WORLD
`f7228f3fe733863784d8f9526efd2de4596d3000`: `ACC-03_RC_RETRY_02_2026-10-11.md`.
Inputs remain synthetic pure values using real providers; no live coordinator,
native lifetime/source authentication, SQL/gameplay/publication/ACK/crash-recovery
journey or32MiB whole-host/ABI/release qualification. All748 preexisting container
IDs/seven live services preserved; initial postcheck total-vs-running count error
and correction logs retained separately, original failed seal unchanged.

Full `ACC-02-schema65-RETRY-03` sole heavy lease RUNNING, source
`6bfa0f7ebdd5f2f46841047fdd10522e4d27dfea`, fresh6906 files/archive SHA256
`e035e193b73bd993d03b412b90c5673993fd216265874d7bc9fb728dd8699a85`.
Orchestrator independently ran `--check-preparation`: PASS6906/zero resources;
critical8 matrix/schema/runner/compiled input pins match frozena60, src/migrations
trees unchanged. Preparation SHA256
`bbd5db88d02cc546117ae5caa3a397d3a2af7d023c19eb78c0305cc729780839`;
helper SHA256 `70c849d19c443d6b0a11d9e3920a8e0bae458552effb6604677f0cfcfc2f9c7c`;
reviewed exactdiff SHA256
`d9e63c6896434c90a58390f90fb571ffff57db7fae30c78b1afd6907781e601e`.
Grant SHA256 `7329c409cbb438525cda8a4cc1db8233e48c5b1609f2b835bc8c29e397cccad3`;
review SHA256 `ab158fad0ef66965ee61dabe0fee3c528d1ad04df99b3d890af65fd66a407712`.
FreeWindows6.27GiB/freeD1527.59GiB, exact buildslots/global lease absent before
grant, seven live services preserved. One cohort, at most two512MiB/CPU1 DBs and
one serial2GiB/CPU2 QA, same pinned images. MySQL tested directD data, original
Linux/tmp on both, fresh labeled native Linux Maria volume never reuses FS-01.
Full original matrix/predicates/flags/defaults/migrations unchanged: both engines,
all original/canonical/staging/master histories/selector/refusal/rerun/protected
data/metadata parity/session-fault/accepting65 shell+compiled boot/tamper/finalreport.
Only actual measurements may write the two private manifest/header files; no
qualified public adoption from partial/failed results. Guard CID/image/owner/job/
source before graceful stop/log/state/private snapshot; retain passive volumes,
data and failures. Source/result evidence sealed plus successful empty owned and
volume-consumer inventories precede matching lock release; failure keeps recovery
container/lease for investigation. No WORLD/SHOP heavy grant or runtime lease.
Evidence/source `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-6bfa0f7e-retry03`;
bin `D:/Dev/Builds/Duris/accounting-plan5/ACC-02-6bfa0f7e-retry03/bin`.
Launch03:10:55 MDT; Plan5 clean preparation publication
`dc1cc3ddf14dc022757a3aa291a59213608fbeb1`. Exact running lease entry SHA256
`a71204607dec8f500a7e53df2c437cce18c17f043bc0462cbcc3541d901d54bc`
retained as `initial-running-lock.json`. Live MySQL CID
`064e0e888f2ece37f884337d039f549cd83c4f595f0a8e1a96d5e7ad89552232`
independently inspected: exact owner/job/source/image,512MiB/CPU1/restartno,
fresh directDdata mount. Authoritative native exec session `36854` remains live;
Plan5 reports canonical/staging/master transitions and reruns passed, expected
wrong-selector refusals passed, authentic original65 routes progressing.
Historical refusal QA nonzero exits are maintained
controls, not terminal matrix failure. MySQL first-engine history checks live;
MariaDB/full accepting65 boot/tamper/result remains pending. No partial promotion.

Exact PRIMARY handoff for the native recipe: adopt only the three `SOURCES`
entries from integration `6bfa0f7ebdd5f2f46841047fdd10522e4d27dfea` in
`tests/async/test_native_quest_recovery_context.py`: `src/economy/native_quest_cost.c`,
`src/economy/native_quest_coin_give.c`, `src/item/lockpick_retirement_continuation.c`.
Resulting file SHA256
`f719c84ce0a450eb565f6d6bf861f836e3e96fbb4cc11049cf6dfa70c53f9e0c`;
refresh only applicable existing registry/matrix source pins, preserving existing
case/argument controls, original fixture/flags/20s deadline and component proof
limits. Reviewed terminal report `ACC-03_RC_RETRY_02_2026-10-11.md`, worker
`f7228f3fe733863784d8f9526efd2de4596d3000`, independent seal review
`0de384548c83d6eda690a750a42c30c35efe6ace61dfe35c97fedef42d28e990`.
This request grants no runtime domain paths; primary remains sole writer there.

Later restore source precheck (not an executed failure): retained
`tests/async/run_restore_accounting_evidence_mysql.py:596` requires0062 although
the current parser returns65/defaulthead `0065_zone_reset_item_birth_origin` or
the three nullable heads `0065_zone_reset_item_birth_origin_nullable_default`.
Do not schedule unchanged0062 fixture as65 qualification. Future owned fixture
ticket must select exact registered route/checksums/history, preserve native
oracle/freshnamespace/socket/read-only guards and qualify actual both-engine
controls. Any shared qualifier/profile interface changes remain PRIMARY-owned.
Private precheck retained at Plan5 FS-01/restore-head-precheck.json; no edits/gate
closure from this observation.

## Preserved retirement and handoffs

Retiring quest `01a110f7-3d22-7fc2-9965-daa6428d11fc`, domain
`01a11627-5fc2-7960-b5f1-f38e5183a822`, old coordinator
`01a115ce-8ec5-7991-8cf4-56f8d0961d93`: finish only named active slice and preserve
all work/goals/failures; launch readiness owns bootstrap retirement capture.
Old coordination heartbeat PAUSED; original prompt SHA256
`472ba9b18ce9af8b9ab57eebc5ae2cb86cbb7a7cca0cab29f6b3468552b9e589` retained.
Do not resume it. All three helpers verified IDLE, no active owned jobs carried.
Quest packet `D:/Dev/Temp/quest-retirement-handoff-20261011/HANDOFF.md` SHA256
`d494508008b7bebd574e089a39c2f676da85296f2b4f73fad90f8b5d88f5f731` and domain
packet `D:/Dev/Temp/accounting-retirement-01a11627-20261011/HANDOFF.md` SHA256
`807d48644ad8984751a6d681f6e3144497201af6742eb0d2e0fa3fc65b96347b` independently
matched; dirty quest overlays and all stopped resources remain preserved.

Reviewed Plan5 output integration: import only the three-file `ed57174e` ->
`0446c40d` delta (four production lines in snapshot/origin main plus 62 existing
test-module lines); preserve launch snapshot's older capture semantics. Complete
encoded JSON plus newline now refuses over-limit before output creation; existing
output remains exclusive/protected. Actual current-source command
`python -B tests/async/test_economic_sql_uid_scope.py -v` passes 14/14 tests,
zero failures/errors/skips, 0.063s; new method covers both exporters/16 controls.
Windows CLI/file execution uses mocked SQL capture/client; POSIX mode and native
SQL authority unqualified. Evidence/patch/result:
`D:/Dev/Tests/Duris/accounting-accel-integration/PLAN5-output-integration`.
PRIMARY registration request: append
`SnapshotCliTests.test_output_byte_limit_includes_final_newline_before_file_creation`
to `plan5_recent_uid_scope` required_cases and arguments before `-v`; preserve
all existing controls. Registry/matrix refresh remains PRIMARY-owned.
Native/migration source bytes unchanged, so ACC-01 stays at its original freeze.
Reviewed minimal parser integration on `e153862c9`: Plan5 patch SHA256
`dc40da057902e80ed0cb6a68407abb4c80e7bdc5c927d8e5e1691200446f80d9` ports only
`os/stat`, main's bounded regular-file reader/duplicate-field decoder/recursion
refusal, and four original regression methods. Historical dependencies:
`0a16ba2187ecfc77b6306c0e3f9a3be5d85eeb4e`,
`f7203b4c438b35da1f1d1fcf3267d3e740a65ada`, final fix `fcefe3a1e`;
portable handoff published `25f23de1479769c37953c182906529db82eeb972`.
Current whole module command
`python -B tests/async/test_reconcile_economy_accounting.py -v` collects 153,
passes 149, skips 4, zero failures/errors, 113.597s. Windows evidence:
`D:/Dev/Tests/Duris/accounting-accel-integration/PLAN5-reader-integration`.
The Windows POSIX FIFO skip passes separately on Linux in the current owned QA
container (one existing named method, 1.359s, no skips); exact source/test hashes
and result at `ACC-01/reader-posix`. Two explicit near-limit Linux audit-budget
methods and native two-engine stake method remain NOT_RUN/required in Plan5 queue.
Original audit definitions and fixtures preserved; no wholesale branch merge.
PRIMARY registration request: append
`ReconciliationTests.test_cli_refuses_nonregular_snapshot_without_waiting`,
`ReconciliationTests.test_snapshot_growth_after_size_check_cannot_bypass_byte_limit`,
`ReconciliationTests.test_cli_rejects_duplicate_snapshot_fields_before_any_view`,
`ReconciliationTests.test_cli_refuses_excessive_json_nesting_before_any_view`
to applicable offline audit row without changing its existing controls/pins.
Resolved by primary `54fef64218e049ae7583d918676d23857f361e8b`: same production
reader bytes, all four methods registered, test-only nested fixture strengthened
to16384 depth plus explicit actual parser RecursionError assertion. Published
Linux153-method150PASS/3optional and Windows149PASS/4skips remain primary-reported;
its private execution packet is not independently requalified here.

## Newer published primary delta: bounded observation

`b192710248aa5887080d22303e9d7c083be482cb` adopts the snapshot exporter's
two-line final byte/newline bound and registers its existing regression case.
Its published evidence explicitly scopes real CLI/file execution with mocked
SQL capture/client; it reports Windows/Linux component passes, not SQL authority.
Our integration additionally covers origins output and the minimal reader
refusal regressions. Those remaining narrow deliveries/registration refreshes
stay queued for PRIMARY; its test/pin bytes differ from our two-exporter module.

`757c3f6fe3c1e7b69d38ae3831fff9acb148c1a9` changes exactly two native files,
`shop_trade_recovery_manifest.c/.h`: standalone forest shape/encode/decode
Source/supplement/INITIAL/query providers. Exact Git bytes preserve the entire
original C prefix and header prefix/closing guard. New Source-provider symbols
occur only at their definitions/declarations; selecting caller integration is
still absent. Source hashes agree with that commit's published registry. These
observations do not authenticate the primary's private native/ABI/frame packet
or qualify its calculations, whole-host storage, actual 32MiB accept/refuse,
linked crypto/libc correspondence or gameplay/recovery. Published primary docs
report two affected objects compiled in both backends and retain those limits.
Observation JSON: `D:/Dev/Tests/Duris/accounting-accel-integration/primary-observation-757c3f6fe`,
SHA256 `c65cffc27c796c475c4bf180def1f6df2619841e4095d79a1131b88a3e3c5152`.
No native source was imported or relabeled into frozen ACC-01/ACC-02. Follow-up
combined build/boot is queued on the newer candidate after the schema boundary.
Published primary `5418d88ca` adds native mobile validation providers in
`src/item/item_transfer_command.c/.h` and docs/pins. Newest `2206f446c` adds native
recovery encoding/decoding/validation Source providers to those same two files
and docs/pins. Primary `9d288f175fbedaec7f439805a17e32a2fe8ddbd7` additionally
adopts the origin exporter's final byte/newline bound, expands existing two-exporter
controls and refreshes pins. Newest primary
`54fef64218e049ae7583d918676d23857f361e8b` adopts the minimal reader and four
registered refusal cases; production reader byte-identical to our reviewed repair.
Origin/reader handoffs resolved; new native-context recipe source-pin refresh
remains queued. Charter has no new
acknowledgment. Those additional native deltas
remain unreviewed/unqualified here; maintained migrations and matrix are
unchanged from frozen a60. The primary's published object-compile reports do not
qualify whole Item/ROOT selection, runtime/ABI, architecture or release.

No primary acknowledgment accepted. No runtime worker implementation active.
Forecast remains 3-6 more days with low confidence; 25-40% reduction is a planning
estimate, not measured. Re-estimate after current accepting SQL/startup evidence.
