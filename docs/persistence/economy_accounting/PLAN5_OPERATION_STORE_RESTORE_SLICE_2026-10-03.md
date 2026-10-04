# Plan 5: retained operation-store restore qualification

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `5deba7a87f040a899c399efd703fcc77ba7a5d03`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/operation-store-evidence.json` record its exact SHA after commit.
Refreshed remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`; native sources
remain unchanged from canonical `7d2f8e8153f637c38e19054cf1202436d3f9a28a`,
with migration 0056. This slice does not incorporate the primary owner's
unpublished fixes or qualify its future combined candidate.

## Established defect and complete physical-storage fix

The native encoder created an inactive store retaining 23 operation records,
including a successful bank plan, rejected receipts, a sealed segment, an active
segment and an empty initialized bucket. The actual native qualifier accepted
the intact store, then still returned success after `bucket-01-0.eas` was deleted.
The remaining economic bytes were unchanged. The executable RED is retained in
`tmp/plan5/flatfile-operation-loss-red.log` and its ignored reproduction driver.

The qualifier now independently scans every initialized operation bucket and
every sealed/active segment after candidate recovery. The existing independent
authority check still runs first. The two readers share the owned bounded,
read-only frame reader; no mutation/storage/recovery codec is called by either
reader. The operation scan requires:

- Every declared index and segment, exact private regular files and complete
  bounded frames, hashes, lineage/bucket identity and reserved fields.
- Unique ordered operation IDs, declared byte totals, bounded entry/segment
  counts, contiguous segment numbers and dense, non-overlapping record ranges.
- Exact indexed record hashes and no untracked or malformed `bucket-` filenames,
  including stale segments and indexes for uninitialized buckets.
- ECR2 and CCM1 framing/size/result consistency, matching stored operation IDs,
  bounded canonical entity keys/revisions and valid envelope fields.
- EAI1 lineage, retained epoch and operation identities; immutable command and
  domain bindings; matching EAP1 metadata/digests and declared physical row size.

Command timestamp normalization follows the existing versioned binding preimage:
schema 1, publication false, accepted timestamp 1, with the NUL-delimited hash
tags. It changes only a local byte projection for hashing. Retained bytes are
never rewritten. Rejected receipts remain opaque bounded result bytes with no
invented successful plan. Empty initialized evidence before any epoch is a valid
partial inactive bootstrap. Full unsigned durable receipt revisions are preserved.

The scan holds one bounded index and segment at a time. Per-index capacity is
4096 records, per-segment capacity 8 MiB, and per-bucket byte capacity 256 MiB.
This is physical storage qualification, not proof of native effects, policy
authorization, complete item semantics, source entitlement or activation readiness.

## Owned files and narrow shared handoff

- `scripts/qualify_flatfile_economic_records.h`: independent operation-store reader.
- `scripts/qualify_flatfile_economic_authority.h`: factors the existing frame reader
  for its second real caller; authority checks remain unchanged.
- `scripts/qualify_flatfile_restore.cpp`: invokes the combined read-only gate after
  existing candidate authority recovery in preflight and postflight.
- `tests/async/flatfile_restore_authority_fixture.cpp`: native-encoded retained
  records and partial inactive evidence fixtures; never selects an active epoch.
- `tests/async/test_flatfile_restore_economic_authority.py`: actual native qualifier,
  independent sanitized reader and corruption fixtures; shared fixture build helper.
- `tests/async/test_persistence_backup_integration.py`: actual backup/restore manager
  proof that missing index/sealed/active files refuse before service boot.
- `docs/operations/BACKUPS.md` and this report: operator behavior and evidence limits.

No shared accounting field, interface, schema, wire format, writer, coordinator
or activation-owner change is requested. The primary owner should register the
native script in `tests/regression_manifest.json` and update the existing recovery
entry's `minimum_cases` from 10 to 11. Add
`PersistenceRecoveryIntegration.test_flatfile_economic_record_loss_refuses_before_service_boot`
to `required_cases` of the `persistence_backup_integration` entry in
`tests/integration_manifest.json`. Consumers: central native and privileged
recovery runners. Invariants: execute the read-only assertions; missing retained
operation files must block before service boot and `QUALIFIED.json`, without
changing the source or captured generation. Shared manifests were not edited.
The earlier native shop harness linkage request remains in the SQL restore report.

## Exact tested source

All native source bytes retain tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`. The ignored evidence manifest pins
all 1,232 native inputs, build helpers, executable sources and raw output hashes.
Owned executable input pins:

| File | SHA-256 |
| --- | --- |
| `scripts/qualify_flatfile_economic_authority.h` | `567a28bf7413f20803c155a4f71fcc071f5cf15ec3994759fe6e64683048f041` |
| `scripts/qualify_flatfile_economic_records.h` | `cbd84bc55d108f2ef331fad2278c5d574862d2d778861376c11c57ee83cf805e` |
| `scripts/qualify_flatfile_restore.cpp` | `7b74ac8a63fbb6d1915018e40591f288590f3f332b9632a0cf27991868508c22` |
| `tests/async/flatfile_restore_authority_fixture.cpp` | `fa5bfdc74b1247b98a577f46a612390ff4ecc0636b26a43a5a079d0a25e4c0ed` |
| `tests/async/test_flatfile_restore_economic_authority.py` | `a05443088c1cdcfa1360ecf10c09899423a8aa9129918710a140dc08a82a867c` |
| `tests/async/test_persistence_backup_integration.py` | `3c64dcd8bbf467ac3dec3ba7130af6bf54057aba3cf0c5384065bd3c4270d08e` |

Executable SHA-256:

- Native qualifier: `70d77e6fa0906e304b150e11ab5972bb82def6897738f11f515b84c74b711d9f`.
- Native encoder fixture: `0c3a5b2f530bf4988c7cab07143f3a1a7eece65f52e5482eeca27016379766e2`.
- Independent sanitized reader: `72c078c84e24828cf6323ba6625a4740cb3dcd76ea936128acc4e5e31947a941`.
- Maintained SQL/development server: `ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.
- Maintained flatfile/development server: `b39588923aa29ee4850b94c7c73d99c7eac1e6ce83ea45edf28e3d4854dab756`.

## Commands, backends and evidence

The checkout is mounted read-only at `/workspace`, with only this worktree's
`bin/` mounted writable for build artifacts. Functional/native tests use
`duris-accounting-restore-tools:local`, pinned to
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`,
with Ubuntu 24.04, Python 3.12.3 and GCC 13.3.0:

```sh
python3 tests/async/test_flatfile_restore_economic_authority.py
python3 tests/async/test_persistence_backup.py
make -C src -j2
make -C src PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_restore_flatfile -j2
```

The native regression passes ten intact-store cases and 110 corruption refusals:
240 actual preflight/postflight qualifier invocations and 120 independent
ASan/UBSan reader invocations. Each checks retained economic bytes, permissions
and link counts are unchanged. Rehashed/rebound corruption cases exercise
semantic identity and binding checks beyond checksum detection. The native
encoder is also sanitized. Qualifier builds use the existing strict C++20 flags;
the independent reader links only the owned readers and libcrypto. Backup
filesystem regressions pass 40 cases. Both maintained builds pass with strict
repository warning/hardening flags. Formatter file checks, Python compilation
and `git diff --check` pass.

For the full recovery suite, the derived local tools image
`duris-plan5-mysql8-recovery-tools:local` is pinned to
`sha256:192535b64b6212f908bd54befe5ced5285494875d675a15202aee20c571f7058`.
It preserves the base tools image and adds the installed WSL Ubuntu MySQL
8.0.46 public binaries, message/plugin files and required runtime libraries.
The ignored `tmp/plan5/mysql8-toolchain/Dockerfile`, runtime archive and build
log describe the packaging; no database files, credentials or server configuration
are copied. MySQL is `8.0.46-0ubuntu0.22.04.4`; MariaDB is
`10.11.14-MariaDB-0ubuntu0.24.04.1`. MySQL clients are also 8.0.46.
The container has `CAP_SYS_ADMIN` and unconfined seccomp for isolated namespaces:

```sh
DURIS_RUN_BACKUP_INTEGRATION=1 DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1 \
  python3 -u tests/async/test_persistence_backup_integration.py -v
```

The final full suite passes all 11 cases with no skips in 590.672 seconds. This
elapsed time records the run; it does not establish a workload or latency budget.
Its new manager case captures three deliberately damaged generations, verifies
their manifests and then checks that missing index/sealed/active files refuse
before service boot, leave no qualification receipt and preserve source/generation
inventories. Existing cases exercise native replay, interrupted transactions,
first snapshots, WAL corruption/quarantine, receipt/catalog checks, foreign-owned
checkout isolation and actual SQL dump/import/schema/history/value qualification
with isolated service boot. Every daemon uses a fresh private datadir and Unix
socket, with TCP and native AIO disabled. Fresh bootstrap adopts and applies
canonical migrations through 0056; this is not an upgraded existing installation.

Raw evidence under ignored `tmp/plan5/`:

- `flatfile-operation-loss-red.log`: the exact missing sealed-segment false success.
- `flatfile-record-store-native-green.log`: final native and sanitizer regression.
- `operation-store-recovery-both-engines.log`: final full recovery run.
- `operation-store-recovery-integration.log`: preceding unchanged integration driver
  pass, nine executable cases with one explicit MySQL skip; not the final qualification.
- `flatfile-record-store-backup-unit.log`, `flatfile-maintained-build.log`,
  `operation-store-sql-build-check.log`, `flatfile-record-store-format-check.log`.
- `operation-store-frozen-inputs.json`, `operation-store-evidence.json` and
  `mysql8-recovery-tools-build.log`: input/toolchain/artifact pins.

## Remaining gates and blockers

This solves physical retained-operation loss and its restore-consumer defect.
It does not qualify full account/posting policy, item topology, actual native
holdings/custody/openings, source-claim entitlement/closure, pile heads, baseline
witnesses, erasure, allocator continuity, publication/pause authority or activation.
The semantic audit/export and missing-external-origin gates remain required.

Full financial retention/rotation horizons, measured workload/latency/growth and
checkpoint budgets, real accounting gameplay/fault journeys, upgraded populated
databases, production-profile qualification, writer-census coverage and the
primary owner's combined candidate remain open. The fixtures and recovery
service boots are not full-feature release completion. Earlier 0055 evidence is
not used to qualify this source. No production database, configured game, `.env`,
accounting activation, PR merge, deployment or audit correction is involved.
Wallet-root item exclusions and the declined inactive spell change remain untouched.

The required curator notebook update still needs the missing `AI_CONTEXT.md`
and curator workflow/notebook reference. The earlier request is pending; this
source report does not substitute for or claim a notebook update.
