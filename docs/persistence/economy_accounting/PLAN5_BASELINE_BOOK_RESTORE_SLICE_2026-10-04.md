# Plan 5: independent retained baseline book and witness qualification

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `2cbc41bb6674470808b199799e373f37fd23e54c`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/baseline-evidence.json` record its exact SHA after commit.
Refreshed canonical remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`.
Native source tree: `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, including
canonical migration 0056. Primary-owner unpublished fixes are outside this
source qualification. Only this delivery branch is published by this lane.

## Established defect and retained-book fix

At the slice base, the actual native encoder and storage seam staged a baseline
in a private inactive store. The existing actual restore qualifier accepted it,
then still returned zero after independent deletion of its EAB1 witness, book
head, or populated reservation index. Remaining bytes were identical before
and after each qualification. The failing assertion, all three false passes and
reproduction driver are retained in `tmp/plan5/flatfile-baseline-loss-red.log`
and `tmp/plan5/reproduce-flatfile-baseline-loss.py`. No epoch was selected,
native holdings/custody changed, or activation owner called.

The independent reader now requires the precise private witness named by every
successful typed baseline command. It checks raw size, SHA-256, EAB1 version,
reserved fields, lineage, retained epoch, preparation ID, operator, batch index,
opening key, and nonzero source/boundary/coverage digests. It derives the operation
ID independently from preparation, domain and batch. EBC1/EAI1/CCM1 must retain
the native baseline fence, source site, deadline, empty facts/result, supported
versions and exact source-event bindings. Rejected receipts do not establish
baseline origins or reservations.

The reader regenerates the entire opening plan from the witness without any
production compiler, codec, store getter or recovery call. Holdings must use
strict numeric account-key order and distinct authority lifetimes. Their four
denominations must be nonnegative and their weighted copper values fit int64.
Zero holdings retain their revision-only opening; nonzero holdings get exact
opposite opening postings. The generated account effects, event indexes,
postings, opening counterpart and counts must match every retained plan byte.
Item positions must be unchanged, canonical and ordered by numeric UID. Active
and quarantined containment must form a complete forest with consistent owner
and root identities; retained destroyed edges are not followed. Source digests
are checked for presence, not asserted to prove external native authority.

Each retained epoch's baseline namespace must contain its private head and all
16 exact reservation indexes. The reader binds full index-file hashes to the
head, checks one consecutive receipt revision per successful batch and the last
operation, and independently regenerates all holding/UID reservations from the
hash-bound witnesses. Exact index comparison rejects missing, extra, duplicate,
foreign or wrongly assigned reservations. The opening key must agree across the
head and every witness. Namespace closure rejects untracked witnesses, malformed
names and foreign lineage/epoch books. Initialized empty books remain readable;
partial loss of them refuses. Legacy absent/empty accounting storage and earlier
partial bootstrap/operation/source-claim compatibility remain preserved.

This uses the existing independent private-file reader, extracted into a bounded
raw-file helper for the second caller: EAB1 has no outer envelope. `same` and
bounded little-endian `number` now serve both readers from that same owned
helper. No native storage, wire format, public API, dependency or schema changes.

The common index format limits retained roots to 256 * 4096. Baseline descriptors
carry 56 bytes of fields per root, with vector capacity and book metadata also
bounded. Only one book's reservations are reconstructed at a time, at most
16 * 65536 entries with 32-byte fields each. A witness is at most 872144 bytes;
a full reservation index is 2097240 bytes. These are structural bounds, not
measured retained-world workload, peak-memory or latency qualification.

## Owned files and narrow shared-interface request

- `scripts/qualify_flatfile_economic_baseline.h`: independent witness, plan,
  book, reservation and namespace checks.
- `scripts/qualify_flatfile_economic_authority.h`: shared owned pure read helpers.
- `scripts/qualify_flatfile_economic_records.h`: binds successful baseline
  receipts and finishes all retained books in the existing pre/postflight gate.
- `tests/async/flatfile_restore_authority_fixture.cpp`: native empty, multibatch,
  cross-epoch, maximum-witness and full-reservation-index fixtures, all inactive.
- `tests/async/test_flatfile_restore_economic_authority.py`: both actual native
  qualifier phases, standalone ASan/UBSan reader and unchanged-evidence checks.
- `tests/async/test_persistence_backup_integration.py`: extends the existing
  economic loss case with witness/head/populated-index/empty-index loss.
- `docs/operations/BACKUPS.md` and this report: operator behavior and proof limits.

**Shared interface request: durable empty-book initialization evidence.** Native
`flatfile_accounting_baseline_storage::initialize` explicitly requires its owner
to prove never-initialized state; absence alone cannot prove that after loss.
The current control/epoch catalog supplies no independently retained baseline
initialization marker. Complete removal of a book before its first successful
receipt is indistinguishable from a never-initialized book. This lane does not
invent a marker or modify the primary-owned lifecycle/control contracts.

The requested logical fields, with encoding/version allocation left to the
primary owner, are per retained epoch: `baseline_initialization_operation_id`
(16 bytes, zero only if never initialized) and `baseline_opening_account_key`
(the exact 40-byte kind-9 lineage/account/context key, absent only with zero
initialization ID). Bind that catalog to independently retained authority/control
evidence. Initialization must atomically commit the marker, head and all 16
indexes; retries may not reinterpret missing files as fresh authority. Retain the
marker across deactivation and epoch turnover. An initialized empty head's last
operation must equal its initialization ID; every head/witness opening must
equal the marker's key. A marked epoch missing its entire namespace must refuse.
The primary owner must choose explicit compatibility for pre-marker histories.

Consumers: Plan 1 baseline/lifecycle transaction owner and flatfile authority
control/catalog encoders, activation/cutover owner, Plan 5 independent
qualifier and backup/restore capture. Required tests: native atomic initialization
and retries; complete empty-namespace deletion; head and each index deletion;
marker/init-operation/opening tampering; retained older epochs and deactivation;
legacy never-initialized compatibility; interrupted initialization recovery;
actual restore-manager refusal before boot on the combined source. This is a
narrow flatfile interface request, not a new SQL migration request.

The earlier central registration handoff remains: register this native script in
`tests/regression_manifest.json`, raise the recovery `minimum_cases` from 10 to
11, and require
`PersistenceRecoveryIntegration.test_flatfile_economic_record_loss_refuses_before_service_boot`
in `tests/integration_manifest.json`. This slice extends the same case; its suite
still has 11 methods. Shared coordinator, contracts, manifests, writer registry,
matrix, producers and activation owner are untouched.

## Exact source and validation

The final frozen executable source is pinned in
`tmp/plan5/baseline-frozen-inputs.json`:

| Input | SHA-256 |
| --- | --- |
| `scripts/qualify_flatfile_economic_authority.h` | `4353bb056301124c5d30bc050cf7b07a1e94bb7859be3ac3b5b20412927d368f` |
| `scripts/qualify_flatfile_economic_baseline.h` | `56779ac2ac7c48b4773695dc8732ad550e262f51b3e27e236b3977357c7c45ce` |
| `scripts/qualify_flatfile_economic_records.h` | `7752637579987f7e0a3371044b50d8ca8f1f1fe891acb428ca0c553d201da2d6` |
| `tests/async/flatfile_restore_authority_fixture.cpp` | `cd41974b5ee1fb641890b007570f8cf28255de64b01cf1776c86edeb9eced31a` |
| `tests/async/test_flatfile_restore_economic_authority.py` | `5028458a0a7dbff3031d31a730751c2ee3482ac5f3b89fbab6e208181fa0c432` |
| `tests/async/test_persistence_backup_integration.py` | `d6f83371c70f10eb5a07e61037217c3983bb9b765c13ffa8717ecbfb2f41522a` |

The complete ignored evidence manifest pins all 1,232 native source/header inputs,
all owned files, unchanged build/recovery helpers, migration inputs, encoder and
qualifier source lists, tools images and raw output hashes. The post-commit check
compares every owned file's actual tested bytes with its committed bytes and all
1,232 native files with the committed source tree. The branch was refreshed again
after qualification; canonical remote is still the SHA recorded above.

| Command inside `/workspace` | Final observed result |
| --- | --- |
| `python3 -u tests/async/test_flatfile_restore_economic_authority.py` | PASS: 18 positive stores, 317 corruption refusals, 670 actual native pre/postflight invocations and 335 standalone ASan/UBSan reader invocations. All retained bytes, modes and link counts unchanged. |
| `python3 tests/async/test_persistence_backup.py` | PASS: 40 tests, 16.137 seconds. |
| `DURIS_RUN_BACKUP_INTEGRATION=1 DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1 python3 -u tests/async/test_persistence_backup_integration.py -v` | PASS: all 11 methods, zero skips, 674.124 seconds. Flatfile, MySQL 8.0.46 and MariaDB 10.11.14. |
| `make -C src -j2` | PASS: maintained SQL development executable. |
| `make -C src PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_restore_flatfile -j2` | PASS: maintained flatfile development executable. |

Native positives cover a 3071-holding/6000-item maximum witness with a complete
6000-level chain, a 65536-entry reservation shard across 22 native batches,
numeric 255/256 ordering, all retained epochs, empty/zero holdings and empty
batches, full unsigned counters, tombstones, quarantine, pet and collector
positions. Negatives include every retained head/index/witness loss, each partial
empty-book loss, absent receipt history, inconsistent head/revision/terminal,
rehash-bound wrong reservations and witnesses, regenerated plan disagreement,
bad forests and nonprivate/symlink/hardlink/FIFO files. These are synthetic native
storage fixtures, not verified gameplay openings.

The restore-manager method captures nine damaged generations: the earlier three
operation-file losses, two source-claim losses, plus four baseline losses
(witness, head, populated shard and unused empty shard). File manifests verify
their remaining captured bytes, then actual native preflight refuses semantic
incompleteness. The service loader is never called, no `QUALIFIED.json` exists,
and captured/live inventories remain unchanged. Both database methods create
fresh private datadirs, bootstrap/adopt and apply pending canonical migrations
through 0056, dump/import complete schema/history/values, and actually boot their
isolated service. TCP/native AIO are disabled and no production endpoint is used.
This is fresh bootstrap/adoption evidence, not a populated old-schema upgrade.

Native fixture SHA-256:
`70619e34caa45cb4cbac11f6f7277f64f92b1f551bc3d78dbbcd10b70f7510fc`.
Actual qualifier SHA-256:
`859203da83916639db7b165df0f7ee2292507ac151365aec00e1bb27cc62231c`.
Standalone sanitized reader SHA-256:
`b87bbb9830c16b03437986064eddd7152daed05c748328d51affb24aff816c24`.
The manager's installed qualifier matches that exact final artifact. Both final
native fixture builds produce content-addressed cache artifacts with the same
SHA-256; both cached binaries are verified in the evidence manifest. The existing
fixture helper returns those cache paths rather than installing its requested
destination. The evidence recorder's initial destination-path assumption was
corrected after the complete passes; executable source and checks are unchanged.
Maintained SQL SHA-256:
`ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.
Maintained flatfile SHA-256:
`b39588923aa29ee4850b94c7c73d99c7eac1e6ce83ea45edf28e3d4854dab756`.

Linux tools are Ubuntu 24.04, Python 3.12.3 and GCC 13.3.0. The base tools image
is `duris-accounting-restore-tools:local`, image ID
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`.
The already established MySQL/MariaDB recovery image is
`duris-plan5-mysql8-recovery-tools:local`, image ID
`sha256:192535b64b6212f908bd54befe5ced5285494875d675a15202aee20c571f7058`.
It supplies MySQL `8.0.46-0ubuntu0.22.04.4` and MariaDB
`10.11.14-MariaDB-0ubuntu0.24.04.1` without user configs/datadirs. Its existing
private runtime packaging is pinned in the manifest; no repo dependency changed.

The exact native wrapper used for the command above is:

```powershell
docker run --rm --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max,target=/workspace,readonly' --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max\bin,target=/workspace/bin' --workdir /workspace duris-accounting-restore-tools:local python3 -u tests/async/test_flatfile_restore_economic_authority.py
```

The unit and make commands use the same base-image wrapper, with the command
from the table. The exact full recovery wrapper is:

```powershell
docker run --rm --cap-add SYS_ADMIN --security-opt seccomp=unconfined --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max,target=/workspace,readonly' --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max\bin,target=/workspace/bin' --workdir /workspace --env DURIS_RUN_BACKUP_INTEGRATION=1 --env DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1 duris-plan5-mysql8-recovery-tools:local python3 -u tests/async/test_persistence_backup_integration.py -v
```

Outputs use `2>&1 | Tee-Object -FilePath tmp/plan5/<log>` with Docker's exit
propagated. Formatting passed with
`./scripts/format.sh --check --file scripts/qualify_flatfile_economic_authority.h --file scripts/qualify_flatfile_economic_baseline.h --file scripts/qualify_flatfile_economic_records.h --file tests/async/flatfile_restore_authority_fixture.cpp`
from this checkout in WSL. Python compilation of both changed tests and
`git diff --check` pass. Builds/artifacts remain ignored under `bin/`.

Raw evidence paths:

- `tmp/plan5/flatfile-baseline-loss-red.log` and its reproduction driver.
- `tmp/plan5/flatfile-baseline-positive-probe.log` (four early native positives).
- `tmp/plan5/flatfile-baseline-native-green.log` (final 18/317 native proof).
- `tmp/plan5/flatfile-baseline-backup-unit.log` (40 unit tests).
- `tmp/plan5/baseline-recovery-both-engines.log` (11 full recovery methods).
- `tmp/plan5/baseline-sql-build.log`, `baseline-flatfile-build.log` and
  `baseline-format-check.log`.
- `tmp/plan5/baseline-frozen-inputs.json`, `baseline-evidence.json` and its
  ignored `record-baseline-evidence.py` verifier.

The first complete candidate passed 17 positive stores and refused 315 corruptions,
covering 664 actual native qualifier phases and 332 sanitizer-reader invocations.
Its `baseline-first-pass-inputs.json` and
`flatfile-baseline-native-first-pass.log` remain distinct from the final evidence.
The final source adds the full-shard positive, maximum-depth native chain and
stronger duplicate/orphan/noncanonical-name cases. No failed database or native
regression runs are substituted for the final complete pass.

## Remaining gates and notebook

The empty-book initialization request remains a release gate. A complete marker
implementation and qualification belongs to the primary owner; the current
reader does not claim to solve an absence that the stored format cannot prove.

External native opening/source/custody attestations, full nonbaseline financial
and item semantics, logical source entitlement, authorization, pile heads,
retention/erasure evidence, resumable sweeps, measured predeclared budgets,
populated upgrades, actual supported-route gameplay/fault journeys, the strict
production profile and current executable writer census remain open. Maximum
synthetic native fixtures and successful disposable restore checks establish this
specific component behavior, not release completion or cutover authorization.
Earlier source-specific 0055 results do not qualify canonical 0056 plus the
primary owner's unpublished integration. That owner must publish and qualify the
tested combined candidate.

`AI_CONTEXT.md` and the required curator workflow/notebook reference are still
unavailable. The pending input request remains unanswered, and no curator tool is
available in the current tool catalog. This source handoff does not substitute
for or claim a notebook update. Accounting stays inactive; wallet-root item
exclusions and the declined inactive spell-path change remain intact. No
production data, deployment, PR merge, or audit autocorrection is performed.
