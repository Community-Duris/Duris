# Documentation cleanup audit — 2026-10-09

Status: confident documentation corrections completed on `codex/docs-cleanup`,
then integrated for the user-authorized publication to `experimental-accounting`.
The original audit boundary and results below are historical; the publication
integration is recorded separately at the end. Runtime qualification and
production operations remain outside this documentation work.

## Review boundary

The audit started from `7f3da9c3a1b2b423da85a24abab603d8cdbee149`.
The architecture investigation separately screened 113 recent repository chats
and read 17 relevant conversations, then inspected pinned source and committed
evidence. Its branch boundaries and design recommendations are recorded in the
[October 7 architecture assessment](../design/ARCHITECTURE_REVIEW_2026-10-07.md).

The focused review closed the 49-document queue of indexed and supporting guides.
Additional status corrections covered the legacy importer, baseline preparation
and storage, SQL lifecycle integration, and chat colorization. Repository-wide
triage covered the 198 tracked Markdown documents under `docs/` present before
this report, plus the root onboarding/security documents and the two diagrams.
The documentation contract also discovers current untracked Markdown proposals;
passing their local links does not approve or integrate those proposals.

Review depth differs by document. Current guidance was checked against its owning
source, manifests and focused contracts. Historical records were checked for
reference and status consistency while preserving their dated results.
Repository-wide link/status triage is not a line-by-line runtime qualification
of every historical claim or proposed feature.

## Completed corrections

| Area | Result | Maintained reference |
| --- | --- | --- |
| Architecture and ownership | Describes this checkout's world thread, worker inputs/results, SQL and native flatfile authority, durability versus publication, and distinct lifecycle drains. Newer branch behavior remains pinned to its source revision. | [Architecture](../reference/ARCHITECTURE.md), [module map](../reference/CODEBASE.md) |
| Schema and imports | At the original audit base, the runtime contract was 225 tables at canonical head `0053_craft_progression`; accepted branch histories are explicit. Legacy import includes character baseline readiness and attempted rollback limits. Dated import counts remain unchanged. | [Runtime compatibility](../persistence/RUNTIME_COMPATIBILITY.md), [legacy import](../persistence/LEGACY_DUMP_IMPORT.md) |
| Configuration and operation | Removes unimplemented example settings, corrects ports/environment rules, actual log paths, launcher ordering, Redis authentication/recovery boundaries, and diagnostic limitations. | [Configuration](../operations/CONFIGURATION.md), [runbook](../operations/RUNBOOK.md) |
| Recovery and receipts | Distinguishes RAM admission, synced journal records, domain commit, retained publication and transport delivery. Corrects format versions, item limits, quarantine and restore proof boundaries. Typed payloads remain protected even when health output is metadata-only. | [Player saves](../persistence/PLAYER_SAVE_PIPELINE.md), [critical commands](../persistence/CRITICAL_COMMAND_PIPELINE.md), [world recovery](../persistence/WORLD_RECOVERY_PIPELINE.md) |
| Lifecycle and accounting status | At the original audit base, the inventory covered 225 database and 50 non-database stores. Implemented private baseline/lifecycle callers are linked without claiming game-wide accounting activation. Archive timing is caller-supplied synthetic evidence. | [Lifecycle inventory](../persistence/DATA_LIFECYCLE.md), [SQL lifecycle owner](../persistence/economy_accounting/SQL_LIFECYCLE_OWNER.md) |
| Development and security | Documents build/output locations, diagnostic wrapper guards, maintained test scope, formatting policy and CI scan limitations. Runtime and migration source were not changed by this audit. | [Building](../guides/BUILDING.md), [testing](../guides/TESTING.md), [security baseline](../operations/SECURITY_BASELINE.md) |
| Feature guidance | Corrects implemented telemetry consumers, snapshot version history, controlled pet custody, studio action counts/bounds, structured chat delivery and equipment behavior conditional on accounting authority. | [Telemetry contract](../telemetry/CONTRACT.md), [output preferences](../guides/OUTPUT_PREFERENCES.md), [item commands](../reference/BATCH_ITEM_COMMANDS.md) |
| Historical records | Labels dated qualification and compliance records; preserves original numbers, failures, engine versions and deployment observations. No final-sweep document deletion was justified. | [Readiness record](readiness-report.md), [deployment record](../operations/PRODUCTION_DEPLOYMENT.md) |

## Verified implementation follow-ups

These findings were established at the original audit base. The documentation
describes that boundary; repairing the implementation requires separately scoped
work. Their complete status was not requalified against the 810 intervening
implementation commits during publication integration.

| Finding | Evidence and next action |
| --- | --- |
| Launcher configuration preflight accepts a fully configured mixed fallback mode that the server rejects. | Compare [`cycle_mud.sh`](../../scripts/cycle_mud.sh) with [`persistence_mode.c`](../../src/persistence/persistence_mode.c); align the preflight with runtime validation and add a focused regression. |
| Valgrind/GDB guards use hardcoded port 7777 and do not qualify database authority. | [`valgrind_mud.sh`](../../scripts/valgrind_mud.sh) and [`gdbdms`](../../scripts/gdbdms); validate the configured production port and target authority before diagnostic execution. |
| Production installation listener guard uses hardcoded 7777 and skips the check for an active production unit. | [`install-production-service.sh`](../../scripts/install-production-service.sh); define and test the actual replacement/listener boundary. |
| Reboot recording and email messages overstate successful delivery. | [`cycle_mud.sh`](../../scripts/cycle_mud.sh) suppresses SQL client errors, checks `sendemail` but executes `sendEmail`, and checks an absolute `/logs` attachment path. Verify result codes and helper/path spelling before reporting success. |
| Hook instructions conflict with repository policy. | [`pre-commit`](../../scripts/git-hooks/pre-commit) recommends `--no-verify`, and its test preserves that message. The guide follows `AGENTS.md`; reconcile the hook and test policy in a separate tooling change. |

## Deferred verification and design decisions

| Topic | What is unresolved | Evidence required before a stronger claim |
| --- | --- | --- |
| Current deployment state | September topology, certificates, tunnel ownership, units and helper locations are dated observations. Source templates cannot establish the running host's current state. | Owner-approved current inventory and sanitized host verification. Keep the dated records until supplied. |
| DurisWeb behavior | Website console refresh/UI behavior and website-only hook handling require the corresponding website source and revision. Server hooks and transports do not prove the UI behavior. | Review the matching DurisWebApp checkout and UI/integration evidence. |
| Historical database results | Past MySQL/MariaDB matrix runs and imported-source row/value comparisons were not recreated during this docs pass. | Original sanitized receipts or an authorized isolated rehearsal of the relevant exact source/schema pair. Do not rewrite old counts to today's head. |
| Fresh container toolchain | The current server includes `hiredis/hiredis_ssl.h` and links `hiredis_ssl`; Docker/metapackage declarations list `libhiredis-dev`. This review did not prove a fresh image supplies the complete SSL build surface. | Qualify a clean image/toolchain. This is an unverified dependency gap, not a demonstrated fresh-build failure. |
| Capacity and production telemetry | Phase 03's representative 200-account/four-hour execution remains deferred. Offline telemetry performance fixtures and focused source tests do not replace it. | The isolated representative gate and target-environment workload/teardown evidence, with approved policies and all required cases. See [Phase 03](../gates/PHASE03_READINESS.md) and [telemetry performance](../telemetry/PERFORMANCE_GATE.md). |
| Archive/export/erasure activation | Controller and disclosure policy remains pending; synthetic archive execution does not implement a live database deadline adapter or complete restore/erasure propagation. | Reviewed policy, production adapters and isolated end-to-end lifecycle/restore qualification. See [archive limits](../persistence/LIFECYCLE_ARCHIVE.md). |
| Accounting and architectural redesign | Private owners and selected routes exist, but complete writer/source coverage, activation and combined-candidate qualification remain separate gates. Interactive load admission, publication latency, maintenance measurement and RAM-authority conversion need design decisions. | Choose a combined implementation revision, review the [architecture recommendations](../design/ARCHITECTURE_REVIEW_2026-10-07.md) and [accounting delivery plan](../persistence/economy_accounting/DELIVERY_PLAN.md), then qualify each authorized implementation. |
| Removal of retained evidence | Age or a superseded Markdown journal does not authorize deleting backups, quarantine, player/account data, migration receipts or runtime game data. | An explicit retention/disposition decision. Keep `docs/legacy/` as inherited reference and `docs/lib/` as runtime data. |

## Approved removal follow-up — 2026-10-09

After the initial audit, the user approved removing the obsolete Windows SQL
setup note (`docs/legacy/src/howto_sql_win.txt`) and consolidating the SQL lifecycle
gap/status document (`docs/operations/ECONOMIC_SQL_LIFECYCLE_OWNER_GAP.md`) into the
[maintained owner guide](../persistence/economy_accounting/SQL_LIFECYCLE_OWNER.md).
The guide retains the implementation status, source/schema pointers,
receipt-ordering fixture and historical pre-owner RED evidence. Both test-support
CSV references now point to the owner guide.

The initial sweep's no-deletion result and final audit receipt counts describe
the state before this follow-up; this batch removes two documents. Accounting
archive candidates remain in place.

## Validation and scope

During the autonomous completion batches, 42 distinct focused check programs
passed in their final recorded runs. Coverage includes the repository-wide
documentation contract, connection/runtime metadata contracts, backup and import
planners, lifecycle policy boundaries, telemetry/report fixtures, output parser
and snapshot codec harnesses, item/phase contracts, and security/reference checks.
The contract suite has 14 checks and discovers maintained Markdown dynamically.

Compilation fixtures used the existing network-isolated tools image. New output
parser/preference and header artifacts were directed to a task-specific `bin`
directory on D:. Permission-sensitive synthetic scratch used private Linux
scratch; logs and receipts remained under the D: evidence root.
Two initial fixture runs failed because the read-only source mount hid required
scratch/build destinations; the isolated destinations were supplied and the
affected checks passed. A stale help-wording contract was resolved in the guide
and its rerun passed. These are focused checks, not a full server burn-in.

The original local cleanup performed no production operation, production database
migration, deployment, push, PR or merge. Actual credential files were not opened. The full branch also includes the
earlier approved `.env.example` setting cleanup, SBOM generator path correction
and documentation/security/reference test updates; it is not strictly Markdown-only.
`src/` and `migrations/` remain unchanged from the audit base.

Existing unrelated work remains outside the cleanup commits: the daily-quest
index addition, three untracked planning documents and `output/`.
Only the approved index descriptions were staged; the daily-quest addition
remains in the working tree. Historical qualification data was preserved.

Detailed before/after manifests, commands, logs and local commit receipts live in
`D:\Dev\Temp\duris-docs-cleanup-20261008` and
`D:\Dev\Temp\duris-docs-cleanup-20261009`.
Final counts and the exact final Git revision are recorded in the latter's
`final-audit-receipt.json`, so this report does not require a self-referential commit hash.

## Publication integration - 2026-10-09

The user authorized pushing the completed documentation changes to
`experimental-accounting`. Its fetched head was
`43807ab01fce32d10e6976739772d454618233c1`, with 810 implementation commits
since the original audit base. The five consolidated cleanup commits were
integrated in a separate worktree on top of that head, preserving the intervening
implementation and documentation changes. Conflict review covered architecture,
README setup, backup lock ordering and help behavior. Current references retain
the newer persistent transport, bounded `poll()` turns, account-load worker and
help-catalog behavior. Copyover now writes portable version 18; native 12-17
readability is limited to the compatible legacy ABI. The lifecycle inventory now
contains 230 database tables and 51 other store contracts, 281 entries in total.

During validation, the remote advanced by 12 additional commits to
`626e338461cbd803d69b64cccb2d3849b01a5fff`. The five publication commits were
rebased onto that head without conflicts, retaining all 12 incoming commits.
Current publication references use this final base. The original cleanup and
pre-squash backup branches remain available locally.

The immutable manifests end at `0065_zone_reset_item_birth_origin` (sequence 65),
while the compiled runtime gate and sealed metadata manifest still identify
`0064_auction_custody_history` (sequence 64) and 230 tables. The schema-alignment
contract must continue to reject that mismatch. The guides disclose the pending
alignment rather than claiming an accepted source/schema pair. This existing
implementation condition requires separate candidate qualification; the docs
publication does not repair or approve it.

Five maintained accounting documents each contained one invalid UTF-8 byte: a
Windows-encoded em dash. Only that byte was converted to the equivalent UTF-8
character in each file, preserving the remaining text and valid UTF-8. Original
bytes, hashes and publication validation receipts are retained under
`D:\Dev\Temp\duris-doc-publish-20261009`.

The publication review checks the integrated cleanup and the affected current
claims; it is not a new full investigation of every document or every runtime
change added since the original audit. Runtime source and migration files remain
identical to the fetched accounting head. The original checkout's uncommitted
planning work remains outside these commits. No production database operation,
deployment or pull request is part of this publication.

Final publication validation ran the integrated result on the rebased accounting
head. The documentation contract passed 13 of 14 tests, including maintained
Markdown links/anchors, configuration, diagram accessibility and encoding. The
remaining schema-alignment test failed on the existing `0065` versus `0064`
manifest/runtime heads, after verifying the guides describe both actual pins.
The event-reference/legacy-cleanup and security/dependency baseline checks passed.
All 544 maintained Markdown documents decoded as UTF-8. Whitespace checks
passed, and the original checkout's HEAD, index, index document and unrelated
working-tree status were verified unchanged. Detailed logs and the baseline
schema-mismatch receipt are under the publication evidence root above.
