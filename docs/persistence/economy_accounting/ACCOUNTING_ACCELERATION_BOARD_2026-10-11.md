# Accounting acceleration board - 2026-10-11

## Current accepting gate

ACC-01: fresh strict SQL and flat production compile/link plus maintained inactive
flat boot on frozen `6022c0ad7feab34c13e27738d4295b65b8d010ac`. Bootstrap roster
handoff and exclusive heavy-job grant/receipt precede execution. SQL startup and
schema/restore jobs must be sequenced with Plan 5; do not duplicate its mutable DB
matrix. First checkpoint or exact failure/owner handoff target: within 12 hours
of orchestrator creation (2026-10-11 00:43 MDT).

Accounting inactive; admission CLOSED; final release BLOCKED. This board records
execution, not inventory-based completion. Original Plans 1-5 and R1-R8 evidence
keeps its exact historical scope. Architecture work follows the first checkpoint.

## Owners and source pins

Human authorization verified directly in source chat `01a104b8-0bcd-7863-b720-a15cad96df41`:
five-session plan followed by "do it". Local model is GPT-6.1 Sol / xhigh.

| Lane | Chat / worktree / branch | Current ticket |
| --- | --- | --- |
| Orchestrator | `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`; `C:/Users/alexa/.codex/worktrees/5708/NewDuris Max`; `codex/accounting-accel-integration` | ACC-01 bootstrap prepared; no heavy lease yet |
| Remote primary | Non-connected RGitGoin; private session/WIP inaccessible; published `experimental-accounting` | Shared lifecycle map and implementation; acknowledgment and toolchain/storage/32MiB proof queue |
| Plan 5 | `01a104cf-6aba-7021-b5cd-56345f75cf10`; established `codex/accounting-plan5`; exact worktree/result pending roster | ACC-02 after current malformed-input slice drains |
| World/quest | Exact chat/worktree pending ROSTER_READY; `codex/accounting-accel-world` | ACC-03 inspection/existing executable proof; maintained edit awaits primary acknowledgment |
| Shop/backend | Exact chat/worktree pending ROSTER_READY; `codex/accounting-accel-shop` | ACC-04 inspection/existing executable proof; maintained edit awaits primary acknowledgment |

- Native source: `83e2e4392064667fc4f64780183d17b5acc431ef`.
- Launch source: `6022c0ad7feab34c13e27738d4295b65b8d010ac`; only charter and plan pointer added.
- Fresh published observation at 2026-10-11 00:44 MDT: launch source unchanged.
- Previous combined executable checkpoint: `3049fb7712de7ccd957c61d5d345d2c8a4cf838d`;
  cannot qualify current source.
- Schema65 MySQL8/MariaDB10.11 fingerprints: null / null; qualification false.

## Exclusive file leases

PRIMARY_ACK PENDING at observed launch source; protected WIP/exclusions undisclosed.
No elapsed-time or local observation substitutes for primary-authored published acknowledgment.

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

## Queue and proof contracts

| Ticket | Accepting result | Dependencies / next step |
| --- | --- | --- |
| ACC-01 | Both fresh production binaries, inactive flat preflight, applicable existing SQL startup result or exact assigned failure | ROSTER_READY; one exclusive heavy grant; current frozen source |
| ACC-02 | Genuine two-engine schema65 matrix and exact restore/startup histories; independent audit and native room/restore gates | Retained Plan 5 slice finishes; owned isolated databases; measured pins go to primary |
| ACC-03 | Existing genuine quest/world executable failure reproduced or gate passed; then one acknowledged connected domain slice with both backend proof | Reuse retired quest packet; exact WORLD acknowledgment before edits |
| ACC-04 | Existing genuine shop/repository executable failure reproduced or gate passed; then one acknowledged responsibility slice | Reuse retired domain packet; exact SHOP acknowledgment before edits |
| PRIMARY-01 | Connected save/coordinator/item lifecycle map then verified boundary/caller/admission closure | ACC-01 evidence; no collision with acknowledged leases |
| PRIMARY-02 | Supported toolchain/runtime/library correspondence, complete storage/frame/transitive proof, aggregate 32MiB accept/refuse | Remains explicit; object compile/source packets do not close it |
| FINAL | Gameplay/persistence/schema/backup/restore/recovery/fault/load/resource/R1-R8 gates agree on one frozen candidate | Reviewed implementation batches and per-slice architecture evidence |

Maintained commands prepared (not run):

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

No resource controller was identified in maintained script filename inventory.
Use explicit exclusive task receipts if bootstrap confirms no controller. Receipt
must identify owner chat, job, exact image/container, direct D mounts, purpose,
start and release condition. Disposable jobs use foreground `--rm`, restart no,
finally cleanup; release after evidence capture. Preserve unrelated services,
volumes, recovery containers and all evidence. No resources acquired yet.

## Preserved retirement and handoffs

Retiring quest `01a110f7-3d22-7fc2-9965-daa6428d11fc`, domain
`01a11627-5fc2-7960-b5f1-f38e5183a822`, old coordinator
`01a115ce-8ec5-7991-8cf4-56f8d0961d93`: finish only named active slice and preserve
all work/goals/failures; launch readiness owns bootstrap retirement capture.
Old coordination heartbeat PAUSED; original prompt SHA256
`472ba9b18ce9af8b9ab57eebc5ae2cb86cbb7a7cca0cab29f6b3468552b9e589` retained.
Do not resume it. Exact reusable handoff receipts await roster/bootstrap handoff.

No worker delivery reviewed/integrated yet. No primary acknowledgment accepted.
Forecast remains 3-6 more days with low confidence; 25-40% reduction is a planning
estimate, not measured. Re-estimate after current accepting SQL/startup evidence.
