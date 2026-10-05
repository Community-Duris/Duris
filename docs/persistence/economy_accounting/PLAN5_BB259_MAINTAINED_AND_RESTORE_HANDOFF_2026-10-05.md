# Plan 5 refreshed maintained builds and recovery qualification

The published cold SHOP procedure-binding source now has fresh maintained SQL
and flatfile builds. This qualification consumes the primary's actual native
source together with the completed independent Plan 5 restore-reader fix.
The original native audit selections and managed SQL/flatfile recovery journeys
also pass with fresh artifacts. Their scoped results do not establish release
completion.

## Source, ownership and preservation

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Prior completed Plan 5 result: `4a075abb22757b048f065c6d1647f19978113876`.
- Refreshed primary: `bb25935985af38181080b681b083cdd84774bd52`.
- Combined tested base: `87643757621e74669e72a10a94a2a8c2ccae216f`, a preserving
  local merge of that published primary into the requested Plan 5 branch.
  The merge is published there; no PR merge or primary-branch push occurred.
- Exact native tree: `b00968beadaa72d2e126c11d41a92231017e6d27`.
- Exact migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical 0056.
- Owned tracked file for this qualification slice: this report only.
- Evidence manifest: `tmp/plan5/bb259-qualification-evidence.json`.
- Result commit/remote/preservation receipt:
  `tmp/plan5/bb259-qualification-delivery.json`.

The integrated native and migration files exactly match the primary. All three
files from the completed restore slice retain their byte hashes after the merge.
Earlier Plan 5 branches remain preserved through their committed ancestry.
No shared coordinator, producer, contract, registry/matrix or activation file
was independently edited. No interface or schema change is requested here.

All generated output uses
`D:/CodexEvidence/accounting-plan5/bin/bb259-maintained-20261005`, mounted as
`/workspace/bin`; the source checkout is mounted read-only at `/workspace`.
The pinned image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Docker network access is disabled. GCC is 13.3.0 and Python is 3.12.3.
The maintained builds use a read-only container root as well as read-only source.

## Fresh maintained builds

Both commands retain the Makefile's complete development-profile warning and
hardening policy, C++20, `TEST_MUD`, `__NO_TESTS__` and persistent transport.
Neither flags nor backend definitions are weakened. They start with new object,
binary, tool and compiler-temporary directories. No object or build cache is
reused. Source maps over `src/`, `migrations/`, `scripts/` and `tests/` are checked
before and after each build.

```text
make -C src -j2 PERSISTENCE_BACKEND=mariadb
  BIN_ROOT=/workspace/bin/tests/p5-bb259-build-20261005/bin-sql
  OBJDIR=/workspace/bin/tests/p5-bb259-build-20261005/objects-sql
  DMS_BINARY=/workspace/bin/tests/p5-bb259-build-20261005/server-sql

make -C src -j2 PERSISTENCE_BACKEND=flatfile
  BIN_ROOT=/workspace/bin/tests/p5-bb259-build-20261005/bin-flatfile
  OBJDIR=/workspace/bin/tests/p5-bb259-build-20261005/objects-flatfile
  DMS_BINARY=/workspace/bin/tests/p5-bb259-build-20261005/server-flatfile
```

`TMPDIR` selects `temporary-sql` or `temporary-flatfile` in the same namespace.
The native wrapper command is
`python3 -u -B tmp/plan5/run-bb259-build.py sql` or its `flatfile` variant.
The source, objects, complete command lines, logs, input maps, receipts and linked
servers are retained under `bin/tests/p5-bb259-build-20261005`.

| Backend | Exit | Fresh units/objects | Warnings/errors | Process seconds | Server bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| SQL | 0 | 726/726 | 0/0 | 1655.678 | 193082744 |
| Flatfile | 0 | 726/726 | 0/0 | 1615.270 | 171070920 |

```text
SQL server SHA256:
b0380cf876e48852a01d1d12ce66912b47d19764532d4163560f6f1aa19048c3
Flatfile server SHA256:
1583905ee8928ea519331bb70be7a5cd15b7fb3f3873e6c6771c92077068795f
```

These are full fresh compiles and links. Unlike the earlier f23 output-only
link retries, no result is assembled from older object sets. Compiler temporary
files and outputs are kept on D: to preserve the existing C: artifacts and avoid
its limited free capacity. The slower elapsed build times are recorded as
environment evidence, not runtime latency or workload-budget measurements.

## Contracts and release gate

The child commands are:

```text
python3 -u -B -m unittest -v
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 -u -B scripts/validate_economy_accounting.py
python3 -u -B scripts/generate_economy_writer_coverage.py --check
python3 -u -B scripts/validate_economy_accounting.py --release
```

The final 71-method suite passes, zero skips, exit 0 (15.864 process seconds,
15.539 unittest seconds). Normal validation exits 0 (16.335 seconds), matrix
validation exits 0 (17.512 seconds), and release validation exits 1
(0.299 seconds), with `writer has no executable evidence`. Current primary
registration retains 104 central rows. No route or release status is promoted.

The original `contracts` attempt had one environment error: no writable default
temporary directory in the read-only container. Its complete logs, commands and
receipt remain preserved. `contracts-complete` reruns the unchanged suite with
a task-owned `TMPDIR` on D: and preserves all original cases. This is an
environment correction, not a test or source repair.

## Native canonical audits

The original class is selected directly:

```text
python3 -u -B -m unittest -v
  test_economic_sql_canonical_audit.NativeCanonicalAuditTests
```

The wrapper `tmp/plan5/run-bb259-canonical.py` sets the existing native gate and
fresh artifact directory, with the central default/mobile/source selectors.
All original cuts, the new projection-type cuts, repaired controls, saved
projections, operator views and constraints remain enabled. Each run compiles
fresh SQL and flatfile native probes with strict warnings, ASan/UBSan, leak
detection and immediate sanitizer failure. The probes emit original native
EAI1/EAP1 bytes; projections are imported into fresh disposable canonical-0056
databases. This establishes reader behavior, not a complete gameplay producer.

The fixture's compiled outputs and private datadirs use an executable temporary
filesystem below `/workspace/bin/tests/p5-bb259-audit-20261005`, then copy to the
physical D: bin root. Every retained file is checked against its original byte
hash. `retention.json` records the original/retained paths and complete map.
The test performs its original row, schema, read-only, rollback and cursor checks
before this copy; copied datadirs are stopped and are never production data.

The first default attempt compiles the SQL probe but cannot execute it because
Docker's temporary mount defaults to `noexec`. Its failed log and artifacts are
preserved under `default`. The mount policy was reproduced directly from
`/proc/mounts`; `default-complete` explicitly enables execution and passes in
161.490 process seconds, zero skips, retaining 1205 verified files. No probe
reuse or source change is used to obtain that pass.

All three central configurations pass on both databases, zero skips. Their
results are separate configurations of the same original method; they do not
constitute three distinct test methods or integrated producer journeys.

| Configuration | Process seconds | MariaDB cases / accept / refuse | MySQL cases / accept / refuse | Verified retained files |
| --- | ---: | ---: | ---: | ---: |
| Default | 161.490 | 39 / 25 / 14 | 39 / 20 / 19 | 1205 |
| Native-mobile | 161.818 | 39 / 25 / 14 | 39 / 20 / 19 | 1205 |
| Source event | 162.883 | 44 / 26 / 18 | 44 / 21 / 23 | 1215 |

The source-event selection retains all original missing/duplicate/malformed
database-wide claim checks, including declared damaged-import cases. No source
event or mobile case is dropped to obtain the result. Each selection includes
the ten storage cuts per engine and their ten completely restored controls.
Actual SQL JSON representations remain recorded: all 15 float-valued projections
per configuration are refused, while the five MariaDB DOUBLE projections that
normalize to exact integers remain representation controls. Such altered schemas
are never counted as canonical schema qualification. Each restored control
requires original column definitions/defaults, byte-identical `SHOW CREATE TABLE`
and unchanged rows; audit API calls roll back once and close their cursor.

The three configurations total 244 API/CLI cases. Each retains the original
24 damaged saved projections and 216 API/216 CLI view/limit observations. The
aggregate 648 API and 648 CLI observations preserve saved bytes and authority.
These repeated selections are explicitly scoped to their distinct native
capsules and current reader implementation, not broader release coverage.

## Managed backup, restore and retention

The managed SQL wrapper reuses the existing two engine-specific methods in
`PersistenceRecoveryIntegration`, copying exactly 3168 required tracked inputs
from `areas_mini`, `lib`, `migrations`, `scripts`, `src` and `tests` into a fresh
private checkout. Input byte hashes, modes and Git blob IDs are recorded in
`managed-inputs.json`; native server binaries are copied only after verifying
their successful fresh-build receipts and byte hashes. No checkout `.env`,
existing database or live game is used.

The flatfile wrapper selects the existing
`FlatfileLifecycleRecoveryIntegration.test_retained_receipt_capture_restore_restart_and_generation_retention`
using the newly built flatfile server, fresh verifier/fixture outputs and its
original fault/retention cases. This class explicitly distinguishes modeled
inactive native-codec history from lifecycle source capture or installation.

The wrapper commands are:

```text
python3 -u -B tmp/plan5/launch-bb259-managed.py
python3 -u -B tmp/plan5/run-bb259-flat-managed.py
```

They run in network-disabled, read-only-source containers. Private SQL datadirs
and service boots use task-owned temporary Linux filesystems; the existing
integration test's mount and user/network namespaces require the container's
explicit privileged capability. Databases disable TCP and use distinct Unix
sockets. The read-only container root has an executable task `/tmp` mount.
Observation hooks retain exact managed generations, receipts and service logs
without changing manager, verifier, boot or refusal behavior. Failed boot logs
are retained before normal fixture cleanup as well.

The flatfile journey passes, one method, zero skips, exit 0, in 377.482 process
seconds (368.457 unittest seconds). The fresh native fixtures use zero reused
objects and their original strict/ASan/UBSan policies. The production manager
and newly built server perform two actual isolated cold boots, drain both native
journals, recover the pending authority transaction, and preserve the inactive
lifecycle receipt. The UID high-water proof advances from `[1000203,2]` to
`[2000203,3]` through the original boot reservation policy.

Both retained generations survive and the unretained generation is pruned.
Manifest loss/corruption, checksum-valid receipt corruption and the required
receipt missing before capture all retain their original pre-boot refusals.
Fault audits preserve sources and managed generations. Exact outcomes, input
hashes, binary hashes and the explicit incomplete-R8/source-capture/installation
flags are in `flat-managed/native/evidence.json`.

The managed SQL selection is exactly:

```text
PersistenceRecoveryIntegration.test_mariadb_full_dump_schema_history_values_and_isolated_service_boot
PersistenceRecoveryIntegration.test_mysql_full_dump_schema_history_values_and_isolated_service_boot
```

Both methods pass, zero errors, failures or skips, exit 0, in 644.455 launcher
process seconds (513.753 inner-wrapper seconds, 498.772 unittest seconds).
The actual disposable engines are MariaDB
`10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. Each production-manager backup succeeds and retains
four exact managed-generation files. Both restore receipts report `qualified`
with the correct engine and `schema_history_and_value_reconciliation: ok`.
The inventory and SHA256 of every retained generation file are verified before
delivery. No replica is configured in these fixtures.

The newly built SQL server performs one actual isolated cold service boot per
engine, using SHA256
`b0380cf876e48852a01d1d12ce66912b47d19764532d4163560f6f1aa19048c3`.
The original readiness, journal-drain, service-health and normal-shutdown checks
remain enabled. Full dump/import, schema history, exact restored values,
revision/identity, epic baseline and locker journal/receipt checks retain their
original controls. Each engine records 20 qualifier observations: 12 accepted
and eight refused. The 3168 source/private-copy input hashes remain unchanged
after the journeys. The flatfile binary required by private-checkout setup is
verified but is not executed in this SQL selection; its actual service boots
are established separately by the flatfile journey above.

`managed-results.json`, `managed-process.json`, `managed-checks.log` and
`managed/{mariadb,mysql}` retain process receipts, engine versions, commands,
qualifier observations, generation inventories/files, restore receipts and
service/status logs. Normal fixture cleanup removes the temporary private SQL
datadirs after retaining their managed SQL dumps and receipts. These existing
journeys use synthetic legacy account/wallet/bank/epic values and locker
receipts. They do not qualify active accounting, native EAB2 baseline
installation or SHOP producer acceptance.

The final green selections total 75 distinct test methods and 77 method
executions, zero skips: 71 contract methods, the canonical-audit method in three
configurations, one flatfile managed method and two SQL managed methods. The
initial environment failures remain preserved, and release validation remains
an explicit expected refusal.

## Curator handoff and remaining qualification

Import this report through the primary's notebook curator, retaining its exact
source and scope. The primary maintains the shared notebook locally; that
locality is not blocking work. The preceding six-method restore registration
request remains primary owned. Final integration and publication of the tested
combined candidate remain with the primary.

Native EAB2/schema61 installation and measured engine sealing, original cold
SHOP callbacks/recovery/ACK and flat parity, original writers and real player
journeys, fresh/upgrade/replay/fault coverage, measured workload budgets,
source-capture/lifecycle installation, erasure and full R1–R8 remain open.
Passing source contracts, isolated capsule probes, a maintained build or a
modeled managed history does not complete those gates. Accounting stays
inactive; wallet-root item exclusions and the declined inactive spell change
are preserved. No production access, deployment, PR merge or audit
autocorrection occurred.
