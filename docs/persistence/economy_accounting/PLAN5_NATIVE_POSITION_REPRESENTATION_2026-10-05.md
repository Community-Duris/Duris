# Plan 5 native item-position representation qualification

The independent saved-snapshot reader accepted integer-valued floats and
Booleans in current native UID positions and retained item openings. It could
report zero exceptions and a verified original plan after such a projection
changed. This completed fix validates position representations before UID
indexing and preserves existing custody, topology and lifetime checks.
Malformed positions refuse every CLI view without changing the input.

## Exact source and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Tested base: `921943400fc422453eebcb288fd8a347f4fbb2d9`.
- Refreshed primary at that base:
  `b754e2962390811b13cde820674684aef9098986`.
- Native tree: `b00968beadaa72d2e126c11d41a92231017e6d27`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical 0056.
- Result SHA/remote/ancestry receipt: `tmp/plan5/native-position-delivery.json`.
- Evidence manifest: `tmp/plan5/native-position-evidence.json`.

The five owned tracked files are:

```text
scripts/reconcile_economy_accounting.py
tests/async/test_plan5_child_identity.py
tests/async/test_economic_sql_canonical_audit.py
docs/persistence/economy_accounting/AUDIT_OPERATIONS.md
docs/persistence/economy_accounting/PLAN5_NATIVE_POSITION_REPRESENTATION_2026-10-05.md
```

No shared coordinator, producer, accounting contract, registry/matrix,
migration or activation file is independently edited. No native source or
wire-format change occurs. The native compiler probe text and all pre-existing
child-test/helper bodies remain unchanged. The canonical native method retains
all its original cuts and gains only the saved-position refusal block.

The unchanged native tree already has fresh full SQL/flatfile maintained builds
in the preceding `df9df046269add89a13407c8d22817190eed7c34` qualification.
Those process executions remain bound to their original report; no new full
server build is claimed for this Python-only fix. The current slice instead
compiles fresh strict native probes and executes fresh disposable SQL checks.

## Established defect, red results and fix

The unmodified reader's SHA256 is
`fa8a3d51ad9be8b0e81eebb90d3cd4b8d9687c437699bc12e2f3a4c921296440`.
Original source, tests, probes and observations remain under
`bin/tests/p5-native-position-20261005/red`.

Across four existing synthetic factories and the retained MariaDB/MySQL saved
models, the old reader observes 92 distinct equal-value alias cuts at limits
0/1/100: 276 observations. Eighty cuts produce a false clean result at all
three limits, 240 observations. The remaining 12 cuts already produce other
findings. These counts are not 276 distinct defects or gameplay journeys.
Original plans and unaffected projection bytes remain unchanged.

Three new focused methods execute against the old reader before the fix:
exit 1, 328 subtest failures and three errors (27.415 process seconds,
21.316 unittest seconds). The three errors are old unhashable UID/parent paths;
the new early validator turns them into the fixed diagnostic refusal. The raw
red result also includes the initially misordered boundary control described
below; its count is not used to inflate the independently observed false cleans.

The cause is ordinary Python equality and dictionary indexing: a float or
Boolean can compare equal to the original integer UID, root, parent or owner
member. In particular, `True` and integer `1` can become the same UID key before
the existing topology/history checks see the malformed representation.

The reader now validates both `native.items` and `item_origins` before any UID
index is constructed:

| Field | Required existing native representation |
| --- | --- |
| `uid` | Exact integer in 1..UINT64_MAX |
| `root` | Exact integer in 0..UINT64_MAX |
| Required `parent` | Null or exact integer in 1..UINT64_MAX |
| `owner` | Three-element list |
| `owner[0]` | Exact integer native owner type in 0..12 |
| `owner[1:3]` | Exact integer identity/context in 0..UINT64_MAX |

The existing unsigned integer helper is reused. Missing fields, invalid list
shape, wrong widths, floats and Booleans raise `SnapshotError` with the fixed
`invalid item position` diagnostic. Absent-creation placeholders and typed
zero/unknown values still pass through the original semantic checks; this
parser does not grant them live custody or repair a finding. Original-plan
authentication, revision validation, topology, tombstones, unknown origins,
inactive behavior and wallet-root exclusions remain separate and unchanged.

The fixed reader's SHA256 is
`f51be75aeb7a5591cddd9b317a86c15c0e103625a754a85b008e95976a48dfdf`.
An old/new comparison reruns all 276 observations: all six intact controls
remain clean and all 276 malformed position observations refuse. Only the
relocated old module's registry path is redirected to the unchanged canonical
registry; its source bytes are not modified. The comparison is scoped to the
original native-capsule saved models and synthetic authority projections,
not source-complete native capture or real gameplay.

## Native and disposable-database proof

The existing method runs directly:

```text
python3 -u -B -m unittest -v
  test_economic_sql_canonical_audit.NativeCanonicalAuditTests
```

The wrapper `python3 -u -B tmp/plan5/run-position-canonical.py default` enables
the existing native gate and a fresh artifact directory. Both SQL and flatfile
probes compile and execute, zero reused objects/probes, with their unchanged
C++20, `-Wall -Wextra -Wpedantic -Werror`, `-O1 -g`, ASan/UBSan,
non-PIE, frame-pointer and crypto policies. Flatfile remains client-free with
`-D__NO_MYSQL__`. Both linked binaries have SHA256
`0d217e859a20a9cc40a1734c9e1d893c9eea0df25f0526e9a6584f0dc61b49eb`;
their complete actual compiler commands are in `native/native-builds.json`.

Fresh disposable canonical-0056 databases run on MariaDB
`10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. The actual server startup logs retain these versions.
The class passes, one method, zero skips, exit 0, in 271.708 process seconds
(262.661 unittest seconds).

| Engine | Original API/CLI cases / accept / refuse | New position cuts | New CLI refusals |
| --- | ---: | ---: | ---: |
| MariaDB | 39 / 25 / 14 | 20 | 420 |
| MySQL | 39 / 20 / 19 | 20 | 420 |

The 40 new position cuts exercise current native and opening fields after
SELECT-only retrieval of the original native EAI1/EAP1 evidence. As in the
existing saved-projection cases, the surrounding native authority/openings are
explicit fixture models; no full-world capture is inferred. Each cut refuses
the audit API at all three limits and refuses all seven CLI views at each
limit: exceptions, holdings, supply, prices, routes, provenance and operation.
These are 120 new API refusals and 840 new CLI refusals. CLI status is 2,
stdout is empty, and stderr contains only the fixed diagnostic. Source JSON
bytes, private aliases, original capsules and database inventory are preserved.

All existing storage-type cuts, complete repaired schema/default/row controls,
source-claim checks, original 24 damaged saved projections and 216 API/216 CLI
saved-view observations remain enabled. Each actual canonical SQL audit is
SELECT-only, rolls back once and closes its cursor. The ten storage cuts per
engine retain their exact restored `SHOW CREATE TABLE` controls. MariaDB's five
DOUBLE-to-integer JSON normalization controls remain representation controls,
not altered physical-schema qualification.

The fresh native selection is default. The preceding report retains its
separate mobile/source executions at that earlier exact source; those variants
are not relabeled as fresh runs of this expanded method. Native EAB2/schema61,
producer acceptance and gameplay remain unqualified by this proof.

## Pure tests, workload bounds and preserved failed control

The final pure command selects these original classes:

```text
python3 -u -B -m unittest -v
  test_reconcile_economy_accounting.ReconciliationTests
  test_plan5_child_identity.ChildIdentityTests
  test_economic_sql_audit_origins.ItemRevisionTests
  test_economic_sql_audit_origins.OriginTests
  test_economic_sql_audit_origins.BaselineVersionTests
  test_economic_sql_canonical_audit.CanonicalAuditTests
  test_economic_sql_canonical_audit.RestoreProjectionTests
```

All 190 methods pass, zero skips, exit 0 (82.952 process / 77.655 unittest
seconds). The three new methods cover every position member and both sources,
missing fields, list shapes, integer bounds, null parents, creation controls,
maximum uint64 UIDs and all seven CLI views. Audits preserve their input.
The pure CLI method adds 105 fixed-diagnostic refusal observations.

The initial fixed-reader pure attempt retained one failed control among all
190 methods (85.588 process seconds). The new boundary model assigned its two
maximum UIDs in descending order, violating original native plan ordering.
`boundary-control` proves that the descending model reports only
`invalid_original_plan`, while the ascending model passes. The fixture is
corrected to preserve the original event order; no reader bound, assertion,
case, decoder or sanitizer is weakened. Original red/failed/control/green
sources, logs and receipts remain separately retained.

The original workload classes also run explicitly:

```text
python3 -u -B -m unittest -v
  test_reconcile_economy_accounting.AuditBudgetTests
  test_plan5_child_identity.ChildIdentityBudgetTests
```

Existing gates `DURIS_RUN_AUDIT_BUDGET=1` and
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1` select three actual methods; the child
artifact directory is fresh. All pass, zero skips, exit 0 (24.495 process /
18.408 unittest seconds). Their original 30-second/256-MiB bounds remain.

| Synthetic workload | Input bytes | Scope | Largest CLI seconds | Reported peak |
| --- | ---: | --- | ---: | ---: |
| Mapping creation | 33551795 | 9948 roots; intact/corrupt, limits 0/1/100 | 1.082 | 129556480 bytes |
| Lineage prices | 33552384 | 100000 captured roots; intact/conflict, limits 0/1/100 | 0.786 | 150806528 bytes |
| Child identities | 33554432 | 500 roots, 32000 children/postings; limits 0/1/100 | 0.895 | 179944 KiB cumulative child RSS |

Each retains source bytes, exception-count invariance, exact commands and its
explicit `synthetic_component`/incomplete release-host qualification. The
child RSS is the original cumulative-child metric, not a claimed independent
per-case peak. These sampled component workloads do not establish real-root
commit latency, storage growth, checkpoint/restart or release-host budgets.

## Contracts, environment, evidence and curator handoff

The writer/audit contract command runs the existing two modules: all 71 methods
pass, zero skips, exit 0 (18.635 process seconds). Normal validation exits 0
(14.581 seconds), matrix `--check` exits 0 (17.558 seconds), and release
validation exits 1 (0.132 seconds) with `writer has no executable evidence`.
The 105 central rows and all writer/source policies remain unchanged.

Final green selections comprise 265 distinct methods: 190 pure, three workload,
71 contract and one fresh native method. All 265 pass with zero skips. Failed
red/control attempts are retained separately and are not added to that count.

All executions use image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3.0 and Python 3.12.3, read-only source/root and `--network none`.
Pure/workload output uses an explicitly executable private `/tmp` mount.
Native output/private datadirs use an executable three-GiB temporary filesystem
under `/workspace/bin/tests/p5-native-position-audit-20261005`, then copy to
the physical D: bin root. All 1247 retained native files match their original
hashes. Processes/databases are stopped before retention. Source input maps
over `src`, `migrations`, `scripts` and `tests` match before/after every final run.
No `.env`, existing game, production database or source correction is used.

Physical evidence root:
`D:/CodexEvidence/accounting-plan5/bin/bb259-maintained-20261005`.
Slice evidence namespaces:
`bin/tests/p5-native-position-20261005` and
`bin/tests/p5-native-position-audit-20261005/default`.
The sealed manifest includes all source pins, commands, attempts, results,
native outputs, database logs, budget records and artifact hashes.

Primary-owned registration request: require these three new pure methods in
the existing `plan5_child_identity_pure` row while preserving all prior policies:

```text
ChildIdentityTests.test_native_and_origin_positions_require_exact_integer_representations
ChildIdentityTests.test_native_and_origin_positions_preserve_widths_and_nullable_controls
ChildIdentityTests.test_native_position_cli_refuses_every_view_without_changing_input
```

The existing canonical native selector already consumes its added inline cuts.
No accounting interface, schema or registry change is requested. Import this
report through the primary's notebook curator; the shared notebook's locality
remains nonblocking. All earlier branch work and follow-ups stay preserved on
remote `codex/accounting-plan5`.

The retained SHOP owner `mob_index` link finding remains primary owned.
Native EAB2/schema61 installation and measured sealing, original positive cold
SHOP/recovery/ACK and flat parity, original writers/player journeys, lifecycle
capture/installation, erasure, full measured budgets and R1–R8/release remain
open. Accounting stays inactive. Wallet-root exclusions and the declined
inactive spell-path change are preserved. No activation, deployment, PR merge,
production mutation or audit autocorrection occurs.
