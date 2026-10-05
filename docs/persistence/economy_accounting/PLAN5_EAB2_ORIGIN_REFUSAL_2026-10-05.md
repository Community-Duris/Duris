# Plan 5 controlled EAB2 origin refusal

Malformed version-two position padding and forests escaped the independent
SQL opening-origin reader as `EvidenceError`. The retained-epoch baseline-claim
consumer catches `OriginError`; the escaping exception interrupted that scan
instead of leaving an invalid book unbound. This completed reader fix converts
both position and forest refusals at the existing origin boundary. It never
accepts, repairs, reseals or changes malformed authority.

## Source, branch and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Prior published slice: `ff545d8316e85a709b48432fc0f892b5b320a36f`.
- Tested base: `2eb8b2aff2f224fa928e15bac1e3d50069c798a4`, preserving merge of
  published primary `1e723524ca6237a0ab0bf7afa05f82886eaa8d85` into this branch.
- Native tree: `b00968beadaa72d2e126c11d41a92231017e6d27`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical 0056.
- Result/remote/ancestry receipt: `tmp/plan5/origin-refusal-delivery.json`.
- Sealed evidence manifest: `tmp/plan5/origin-refusal-evidence.json`.

The four owned tracked files are this report,
`scripts/economic_sql_audit_origins.py`,
`tests/async/test_economic_sql_audit_origins.py` and
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`.
No shared producer, coordinator, accounting contract, migration, registry/matrix
or activation file is independently changed. No native source or wire format
changes. All earlier branch work remains preserved on remote
`codex/accounting-plan5`.

The final refresh also observes primary
`eef5b646c226349e655daaac9a21d62938af1dfd`, which adds its original flat baseline
marker fixture recovery qualification. It arrived after this slice's source
freeze and is not included in these tested inputs. Its private EAB2/schema61
candidate evidence remains scoped to the primary's own report.

## Established defect and complete fix

The retained old reader has SHA256
`5a8465d21d4d691f5f15c91889582886c7bd78c881859925c05b60a2f45d999c`.
The old version-test helper accepted any `ValueError`, including the sibling
`EvidenceError`. Strengthening it to require `OriginError` and adding the
retained-claim consumer regression exposes the real boundary failure.
The red class runs all nine methods: exit 1, 30 subtest errors, zero skips,
9.857 process / 0.230 unittest seconds. Original red source and log are retained.
These are wrong exception types, not false clean audits.

The existing `decode_witness` calls to `position` and `forest` now translate
only their `EvidenceError` into
`OriginError("invalid EAB2 item position")`, preserving the cause. All original
byte, digest, length, count, owner/state, equipment and forest checks stay.
The fixed reader has SHA256
`4876556fcee643dda6fce393ff0bf95740d0a6456847b3746667361bcbc090e0`.

An old/new comparison exercises eight reference cuts: six reserved position
bytes, a noncanonical root and a missing parent. It records 24 old exception
leaks across direct decoding, capture and retained-claim lookup. The fixed
reader produces 16 controlled origin refusals in the first two paths; all eight
retained-book lookups complete with their two claims unbound. The invalid book
is cached once per epoch. Direct intact decoding is identical between readers.
All supplied rows stay unchanged, and capture rolls back once and closes its
cursor. The comparison models are explicitly reference witnesses, not native
EAB2 capture or an installation attestation.

Source comparison verifies that `decode_witness` is the only reader function
changed. Every original test/helper body remains after removing the additive
native projection block and undoing the one stronger exception assertion for
comparison. One new method protects the retained-claim consumer. Historical
EAB1 equipment/root refusals, valid EAB2 reference equipment, exact maxima and
all original native cases remain enabled.

## Native and actual database checks

The actual selector is `test_economic_sql_audit_origins.NativeSQLOriginTests`,
enabled by `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1`. A preservation wrapper
retains temporary native evidence and databases after normal shutdown. A
command observer records the original compiler invocation without changing
its arguments, fixture, flags, tests or native execution.

The native fixture compiles fresh, with zero reused objects, in 73.792 seconds.
Its actual command compiles the original C++ fixture plus 20 production units
using C++20, `-Wall -Wextra -Wpedantic -Werror`, `-O1 -g`, ASan/UBSan, frame
pointers, non-PIE and the original flatfile-test macro, crypto and pthread
policies. Complete arguments are in `native-green/native-commands.json`.
Binary SHA256:
`9e59042874cb751e8c461a9547440b32484ce7ee76da4c4c7660f55717490b9a`.

The original native fixture emits four authentic historical EAB1 witnesses,
with retained CCM1/EAI1/EAP1 bytes, across two epochs. Both fresh disposable
engines use the unchanged canonical 0056 schema and SELECT-only reader role:

| Engine | Original captures / refusals / rollbacks | New projection refusals |
| --- | ---: | ---: |
| MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 | 7 / 7 / 14 | 16 |
| MySQL 8.0.46-0ubuntu0.22.04.4 | 7 / 7 / 14 | 16 |

The added checks project a SELECT-retrieved native EAB1 row into a malformed
version-two reader input. Six padding cuts and root/parent cuts each exercise
both `economic_sql_audit_origins.capture` and
`economic_sql_audit_snapshot.capture`. All 32 observations refuse with the
fixed origin diagnostic, roll back exactly once, close the cursor and preserve
the database inventory. Only the copied SELECT projection changes; physical
witness version constraints, native capsules, schema and authority stay intact.
These negative projections are not accepted native EAB2 witnesses and do not
qualify schema61 or a full-world source capture.

Both original methods pass, exit 0, zero skips, in 96.787 process / 91.026
unittest seconds. All 1371 retained files match their original hashes. The
source remains unchanged before/after the execution.

## Other checks and exact recipes

The final pure selection runs the existing reconciliation, child identity,
item revision, origin, baseline-version, canonical-audit and restore-projection
classes: 191 methods, zero skips, exit 0, 92.868 process / 85.114 unittest
seconds. The writer/audit contract modules run 71 methods, zero skips, exit 0,
19.664 process / 12.469 unittest seconds.

Normal validation exits 0 in 10.570 seconds; matrix `--check` exits 0 in 12.391
seconds. Release validation still exits 1 in 0.113 seconds with
`writer has no executable evidence`. The 105 central rows remain unchanged.
Final green selections comprise 264 distinct methods and executions:
191 pure, 71 contract and two native. Red/setup attempts are not added to this
count. There are zero selected skips.

Exact wrappers, underlying commands and environment gates are retained:

```text
python3 -u -B tmp/plan5/run-origin-refusal.py red
python3 -u -B tmp/plan5/compare-origin-refusal.py
python3 -u -B tmp/plan5/run-origin-refusal.py pure
python3 -u -B tmp/plan5/run-origin-refusal.py contracts
python3 -u -B tmp/plan5/run-origin-refusal.py native native-green
python3 -u -B tmp/plan5/run-origin-refusal-gates.py
```

The seven final pure classes are `ReconciliationTests`, `ChildIdentityTests`,
`ItemRevisionTests`, `OriginTests`, `BaselineVersionTests`,
`CanonicalAuditTests` and `RestoreProjectionTests` in their existing modules.
The contract modules are `test_economy_writer_coverage_contract` and
`test_audit_accounting_invariants`. All run with `python3 -u -B -m unittest -v`.
Native preservation dispatches the unchanged two-method class selector through
`retain-origin-refusal-native.py` and `retain-flatfile-qualifier.py`.
No C/C++ source changed, so no new full maintained-server build is claimed.
Earlier full builds and workload measurements retain their original source scope.

## Preserved setup failures and evidence

The first native invocation mounted the leaf output directory, causing the
wrapper's freshness assertion to fail before tests. Its failure receipt is
retained; the parent namespace mount corrected that setup without weakening
freshness checks. That first mount also left an empty physical retention
directory. The next attempt reached retention but refused to overwrite it.
Its test log was not retained when the temporary container ended, so no pass,
test count or native result is claimed for that attempt. Its failure receipt
records this limitation. A fresh named attempt checks the retention destination
before tests and preserves the original empty directory.

The retained `native-complete` attempt then compiled the strict fixture but
failed both database setup paths at `isolated_database_socket_path_too_long`
because its temporary path exceeded the existing 107-byte Unix-socket bound.
Exit 1, two errors, 78.821 process seconds; all 322 files are retained and hash
verified. The final fresh `native-green` uses the existing short `/tmp` path.
No socket guard, assertion, case, sanitizer or source policy was weakened.

All executions use image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3.0, Python 3.12.3, read-only checkout/root and `--network none`.
Private executable temporary filesystems hold `/tmp` and the native artifact
namespace; outputs copy to D: after processes stop. Source maps cover `src`,
`migrations`, `scripts` and `tests` before/after each final run. Evidence stays
under `bin/tests/p5-eab2-origin-refusal-20261005` at physical root
`D:/CodexEvidence/accounting-plan5/bin/bb259-maintained-20261005`.
The sealed manifest retains attempts, commands, logs, capsules, database files,
source copies, original Git-blob preimages and artifact hashes. No live service,
`.env`, production data or authority correction is used.

## Curator and primary handoff; remaining qualification

Primary-owned registration request: add
`BaselineVersionTests.test_invalid_positions_preserve_retained_claim_origin_refusals`
to the existing `plan5_baseline_versions_pure` row in
`tests/integration_manifest.json`, preserving all eight original cases and
policies. Its stronger existing helper is consumed by the original selectors;
the native class also consumes its additive negative projection checks.
No schema, producer, accounting interface or wire-format change is requested.

Import this report through the primary's notebook curator. The notebook's
local maintenance is nonblocking. The primary's reported private native EAB2
padding failure has a corresponding owned reader repair here; repeat its
original native EAB2/schema61 audit against the published combined candidate
before claiming that larger gate closed.

Native EAB2/schema61 installation and measured sealing, historical populated
upgrades, original positive cold SHOP/ACK/recovery and flat parity, actual
writers/player journeys, complete lifecycle capture/install/erasure, measured
production budgets and R1–R8/release remain open. The SHOP `mob_index` component
link finding remains primary owned. Accounting stays inactive, wallet-root item
exclusions and the declined inactive spell-path change stay. No activation,
deployment, PR merge, production mutation or audit autocorrection occurs.
