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
owned inventory succeeded and was empty. Plan 5 is sealing the failure and
preparing one bounded foreground commissioning diagnostic to capture the actual
startup error; no retry or cause assertion yet. This follows the exclusive
01:22 MDT grant of at most two 512MiB databases and one serial 2GiB QA worker.
No WORLD/SHOP heavy grants. First checkpoint target (within 12 hours of
orchestrator creation at 00:43 MDT) met; SQL qualification remains pending.

Accounting inactive; admission CLOSED; final release BLOCKED. This board records
execution, not inventory-based completion. Original Plans 1-5 and R1-R8 evidence
keeps its exact historical scope. Architecture work follows the first checkpoint.

## Owners and source pins

Human authorization verified directly in source chat `01a104b8-0bcd-7863-b720-a15cad96df41`:
five-session plan followed by "do it". Local model is GPT-6.1 Sol / xhigh.

| Lane | Chat / worktree / branch | Current ticket |
| --- | --- | --- |
| Orchestrator | `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`; `C:/Users/alexa/.codex/worktrees/5708/NewDuris Max`; `codex/accounting-accel-integration` | ACC-01 terminal checkpoint; review newer published primary delta |
| Remote primary | Non-connected RGitGoin; private session/WIP inaccessible; published `experimental-accounting` | Shared lifecycle map and implementation; acknowledgment and toolchain/storage/32MiB proof queue |
| Plan 5 | `01a104cf-6aba-7021-b5cd-56345f75cf10`; `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`; `codex/accounting-plan5` | ACC-02 readiness failure sealed; commissioning diagnostic preparation, established audit/restore queue retained |
| World/quest | `01a129b4-a57a-76e0-baec-42b3dd2fe62f`; `C:/Users/alexa/.codex/worktrees/2c0d/NewDuris Max`; `codex/accounting-accel-world` | ACC-03-RC queued, no heavy grant; proposed runtime lease inactive |
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

PRIMARY_ACK PENDING at published `9d88a4668`; protected WIP/exclusions undisclosed.
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
| ACC-02 | Genuine two-engine schema65 matrix and exact restore/startup histories; independent audit and native room/restore gates | Attempt1 MySQL readiness FAILED before migrations; commissioning logs next, no measurements or pass |
| ACC-03-RC | Existing `test_native_quest_recovery_context.py` actual-provider ASan/UBSan component or exact first failure | Queued after schema65 job; component scope only; exact WORLD acknowledgment before runtime edits |
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

No primary acknowledgment accepted. No runtime worker implementation active.
Forecast remains 3-6 more days with low confidence; 25-40% reduction is a planning
estimate, not measured. Re-estimate after current accepting SQL/startup evidence.
