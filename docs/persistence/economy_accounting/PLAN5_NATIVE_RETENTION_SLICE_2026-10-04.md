# Plan 5: inactive native SQL deletion and retained economic evidence

Delivery branch: `codex/accounting-plan5`, separately published from
`experimental-accounting`. Worktree:
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base: `32bbbd26ea8439dc06a2e73a74a707f72adafaa1`. The result is the commit
containing this report; its exact SHA is recorded after commit in
`tmp/plan5/retention-evidence.json`.

Canonical remote refreshed before this slice, and again during qualification:
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source remains tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, all 1,232 tracked inputs,
including canonical migration `0056_spell_ward_durability`. Primary-owner
unpublished integration is not qualified by this delivery branch.

## Established qualification gap and complete observer coverage

The maintained native SQL account and character deletion journeys exercise
actual menu requests, persistence refusals, repaired retries, rollback, name
reuse and cold restart. They previously did not observe economic history
carrying the deleted actor IDs. Pure lifecycle manifest tests and
financial-table inventory did not establish retention across those boundaries.

The new guarded Plan 5 runner reuses those unchanged native journeys and adds
an independent SELECT-only economic observer. It seeds native-compatible
history in each disposable schema after the target character is created,
reads the evidence before deletion and every subsequent cold start, and checks
the final state immediately before the existing fixture cleanup drops the
schema. The observer checks both decoded reader output and all stored columns
of the retained financial rows. It asserts that every target PID is absent
after deletion, with the expected account outcome from each native journey.

The pilot revealed a coverage defect in the initial observer: the character
runner first deletes an account, then reuses its name with a new PID before
ordinary character deletion. Observing only the first PID did not establish
ordinary character retention. The complete runner now records both distinct
PIDs, allocates distinct operation IDs per PID, checks the first history before
adding the second fixture, verifies that all old rows remain unchanged during
that explicit append, then compares the complete history through ordinary
deletion and restart. Non-target observer characters are excluded.

The pilot's source, log and binaries are preserved in
`tmp/plan5/retention-journeys-first-source.py`,
`tmp/plan5/retention-journeys-first.log` and
`bin/tests/plan5-retention-native`. Its source SHA-256 is
`5816292e49b17c21bb9c97323f56288e92384f36967da82db9f444df79bab010`.
It is superseded scope evidence, and does not qualify retention for the
second PID. Qualified artifacts use a separate directory,
`bin/tests/plan5-retention-qualified-native`.

Owned files:

- `tests/async/run_plan5_retention_journeys.py`
- this report

No mutation implementation, shared coordinator, accounting contract, producer,
writer registry/matrix, migration or activation-owner file is changed.
The production audit reader and reconciler retain their independent read-only
design. Mutation imports belong only to the disposable test's existing menu
journeys. Accounting remains inactive; wallet-root item exclusions, the
declined inactive spell change and active blackjack refusal remain preserved.

## Exact evidence shape and native qualification boundary

Eight unchanged production codec/command sources are compiled in SQL and
`__NO_MYSQL__` flatfile modes with C++20, strict warnings as errors, ASan/UBSan,
no PIE and OpenSSL. Each probe encodes and decodes a native EAP1 plan and emits
EAI1/EAP1 bytes for the actual target PID. Both modes agree exactly for every
case, with no sanitizer findings. The probe represents structural native
encoding, not writer admission or authority qualification.

Each target PID receives two zero-effect historical roots: one committed root
with its source claim and canonical plan, and one rejected terminal root linked
to the first by `original_operation_id`. Native intent, domain and committed
plan digests, source identities, actor IDs, version fields and inbox receipts
are stored in canonical economic tables. One retained epoch and an inactive
lineage are seeded per schema. There are no postings, money/account vectors,
UID events, children or baseline witnesses in this fixture. It establishes
retention of this metadata/history shape only.

The native server is the existing qualified SQL binary
`bin/server/dms_new`, SHA-256
`ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.
Its earlier full build is recorded in `tmp/plan5/operation-store-evidence.json`.
All 1,232 current native inputs are compared against that build manifest and
the committed source tree. No server rebuild is claimed for this slice;
native server source is unchanged. The new fixture binaries are compiled
during this qualification, each with SHA-256
`b4e44c9f7ada997e3a6dea9167b0317a486de7ed89e47ed49842881a2e2d916e`.

Each engine has a fresh private datadir; each native invocation creates a new
synthetic schema with bootstrap/adoption and all migrations through 0056.
Foreign keys and CHECK constraints remain enabled. The existing native server
connects through a verified ephemeral loopback port inside the container;
no ports are published or capabilities added. Checkout environment credentials
are stripped. Only new private fixtures and ignored binaries/evidence are
written.

The SELECT-only observer is denied even a no-op UPDATE with native error 1142.
Each capture uses one repeatable-read consistent read-only transaction, then
exactly one rollback and cursor close. Executed observer statements are
restricted to SELECT and transaction declarations. It compares:

- Every column of `economic_lineage_state` and `economic_epoch`.
- Every column of `economic_accounting_operation` and
  `economic_accounting_source_claim`, including actor IDs, original links,
  canonical bytes, digests and outcomes.
- Every column of the seeded epoch creator's and retained roots'
  `critical_operation_inbox` receipts. Unrelated native deletion commands are
  deliberately excluded from this historical receipt comparison.
- The independent SQL reader's decoded evidence cut.

Every capture explicitly proves no active epoch and no staged installation in
phases 1/2. The diagnostic snapshot is declared incomplete and unfenced, with
empty native holdings/items. Reconciliation therefore retains exactly one
`evidence_loss` and one `unfenced_snapshot`; it never labels these cuts clean or
complete. Operation views at limits 0/1/100 preserve global exception count 2,
incomplete coverage and unchanged snapshot input while bounding detail rows.
Deletion does not auto-correct audit findings or financial evidence.

## Native menu and database results

| Engine | Native journey | Target PIDs | Retained roots / claims | Consistent captures | Cold restarts | Result |
| --- | --- | --- | --- | --- | --- | --- |
| MariaDB 10.11.14 | Whole-account deletion | 1 | 2 / 1 | 4 | 2 | PASS |
| MariaDB 10.11.14 | Account deletion, name reuse, ordinary character deletion | 1, 2 | 4 / 2 | 7 | 3 | PASS |
| MySQL 8.0.46 | Whole-account deletion | 1 | 2 / 1 | 4 | 2 | PASS |
| MySQL 8.0.46 | Account deletion, name reuse, ordinary character deletion | 1, 2 | 4 / 2 | 7 | 3 | PASS |

The four unchanged native journeys pass their own accurate-refusal, rollback,
repaired-retry, cache publication, aliases and restart assertions. The retained
observer passes 22 consistent cuts and ten cold restarts. The financial rows
remain exact through failures and successful deletion; each explicit new-PID
fixture append preserves every earlier row. Across four schemas the fixture
contains twelve retained roots and six source claims, covering six deleted
identity instances. Six emitted native cases are each 1,040 length-framed bytes.
This is actual native menu retention with seeded history, not a real economic
writer journey, governance export, complete personal-data erasure or full R8.

## Commands and pinned evidence

All Linux jobs use `duris-plan5-origin-sql-tools:local`, immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Toolchain: Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL package
1.0.2-2ubuntu1.1, MySQL 8.0.46-0ubuntu0.22.04.4 and
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1.

Exact qualified container invocation, with `<worktree>` set to the absolute
delivery worktree above:

```text
docker run --rm --mount type=bind,source=<worktree>,target=/workspace,readonly --mount type=bind,source=<worktree>/bin,target=/workspace/bin --workdir /workspace --env DURIS_RUN_PLAN5_RETENTION_INTEGRATION=1 duris-plan5-origin-sql-tools:local python3 -u tests/async/run_plan5_retention_journeys.py --server /workspace/bin/server/dms_new
```

| Command | Result | Evidence |
| --- | --- | --- |
| Container invocation above | PASS both canonical engines through 0056, four native journeys, two codec modes, 22 captures, ten cold restarts | `tmp/plan5/retention-journeys-qualified.log` |
| `python -m py_compile tests/async/run_plan5_retention_journeys.py` | PASS | Host command output |
| Host invocation without explicit integration gate | Expected refusal before database imports or native work, exit 1 | `tmp/plan5/retention-gate-disabled.log` |
| `git diff --check` | PASS | Host command output |
| `python tmp/plan5/record-retention-evidence.py --committed` | PASS source/blob, migration, helper, artifact, prior binary and server provenance checks | `tmp/plan5/retention-evidence.json` |

The executable source SHA-256 is
`aebaf442ba5131667ba990781893b49caf614c4f2fa5a90c7c7322aed60c69d9`.
`retention-frozen-inputs.json` pins the runner and all unchanged journey,
reader, reconciler, restore and migration helpers. The evidence manifest pins
all 1,232 native inputs, migrations, immutable image, fixture outputs, compiled
binaries, logs, pilot artifacts and prior Plan 5 artifacts. Post-commit checks
compare owned and native raw bytes with committed Git blobs. Evidence is local
and ignored; the maintained runner and report are published on the delivery
branch. No credentials, logs, player data or generated artifacts are committed.

## Handoff and remaining gates

There is no shared interface or schema request from this slice. A narrow
coordinator registration request goes to the primary owner: register
`tests/async/run_plan5_retention_journeys.py` on Linux with
`DURIS_RUN_PLAN5_RETENTION_INTEGRATION=1` and
`--server <exact tested SQL server under workspace/bin>`. Both disposable
engines must pass; the server binary must come from the candidate's exact
source. Coordinator registration is not edited independently here.

This delivery closes an inactive SQL retention evidence gap. Release completion
still requires the primary owner's tested combined candidate containing its
local fixes and canonical 0056. Remaining gates include active typed erasure,
flatfile native retention, real nonzero financial/account/UID lifetimes,
complete independent capture, controller-approved export/erasure and retention
horizon, populated upgrades on both engines, supported producer gameplay and
fault/replay/publication journeys, mixed workloads and release-host numeric
budgets. Earlier isolated passes, seeded fixtures and inventory cannot satisfy
those gates. The prior retained-baseline initialization marker and metadata
header handoffs remain with the primary contract owner.

The broad unchanged audit, backup/restore, lifecycle protocol and full server
build suites are not repeated in this slice. New native codec compilation and
the four actual native menu journeys provide the focused evidence for this
change. The flatfile codec mode is encoding agreement only; it does not qualify
flatfile deletion or recovery retention. Production is never accessed.

`AI_CONTEXT.md`, the project notebook reference and the required curator
workflow remain unavailable in the supplied checkout/tool context. The existing
request for that workflow has no answer. This report is an evidence handoff,
not a notebook update; notebook reconciliation remains a specific external
dependency. Accounting activation, merge, deployment and release completion
are not authorized or claimed.
