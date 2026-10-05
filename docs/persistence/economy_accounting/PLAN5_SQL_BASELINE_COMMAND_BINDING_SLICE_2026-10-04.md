# Plan 5: independently bind SQL baseline commands

This slice starts at `c2c6107c1418d69c8eac75fbb12cb8b135fc9c27` on
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
That base refreshes published primary `152acc1cd75b5f60bb62436c652fdc10d4e66163`
and preserves the earlier Plan5 reader at
`3f496660c0ea89ea6fa313d86cc1a421cf8a48cf`. Native source tree is
`79cc72a7a21ab52233455386eaa054561f7eb8c8`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`. Result commit and publication
readback are bound by `tmp/plan5/sql-baseline-command-binding-evidence.json`.
All 1,254 native inputs and 236 migration inputs remain unchanged in this slice.
Only this lane's branch is published; the primary integrates completed slices
and qualifies the combined candidate.

## Established defect

The independent SQL baseline reader bound complete EAB1 witnesses to EBC1
domain digests and canonical EAI1/EAP1 roots, but required only a nonzero
`EAI1.command_binding`. A disposable owner changed the first binding byte,
recomputed the tagged EAI1 intent digest, inserted it into EAP1, recomputed
the plan digest and updated matching SQL digest columns. The witness, native
command, domain digest, operation identity, receipt and opening projections
remained intact. All three readers admitted this self-consistent substitution:
the SQL exporter/reconciler, independent restore evidence reader and full SQL
restore qualifier.

The RED uses the actual native SQL baseline owner, not a synthetic canonical
root. It publishes two inactive epoch books, applies two native batches with
wallet/item origins, and checks selected and retained epochs separately on
both engines. The defect reproduced in all four cases. One test completed in
288.598 seconds, with zero skips; its success means the original gap was
observed, not that corrupted evidence qualified for release. All 18 captured
tables remain unchanged across each reader invocation. Private fixture repairs
restore the original rows between cases.

## Independent fix and contract

`scripts/economic_sql_audit_origins.py::verify_baseline_root` reconstructs the
existing native normalized baseline command preimage. It imports no mutation,
native codec, persistence owner or coordinator implementation. The 116-byte
CCM1 schema-1 projection consists of the derived operation ID, command type 20,
payload version 1, source 6 (`operator_repair`), deadline 4 (`recovery`),
publication false, accepted-time sentinel 1, exactly one key, no expected
revisions, the system key `(9, 0x45434f4e42415345)` and the complete 48-byte
EBC1 payload. The payload binds the retained EAB1 size and SHA-256. No schema-2
intent length/body is present in this projection.

SHA-256 of the NUL-terminated `DURIS-ECONOMIC-COMMAND-V1` tag followed by that
projection must equal EAI1 bytes `[160:192]`. The existing native
`economic_command_binding_digest` normalizes only its binding projection to
time 1; actual admission, exact-ID equality, journal bytes and durable receipts
continue to retain real time. The independent reader does not change that
contract or substitute a timestamp for a native receipt.

Selected-book disagreement refuses with `EAB1 committed root mismatch`.
Retained-book disagreement preserves the existing `baseline_source_claim`
finding. Independent restore evidence and the full restore qualifier refuse
either scope. The original SQL snapshot/CLI schemas and native database schemas
remain unchanged. No shared interface or schema request is needed for this fix;
the normalized baseline preimage is completely reconstructible from existing
retained fields. Full command-preimage evidence for other SQL domains remains
outside this baseline fix and must be bound to each domain's native contract.

Owned files are the independent reader, the existing unit fixture/regression,
the existing native wrapper/driver, and this report. The synthetic helper now
creates a valid command binding instead of its former arbitrary nonzero bytes;
these unit/sibling fixtures remain explicitly synthetic. The native SQL owner
provides the independent positive oracle. No server, migration, shared accounting
contract, producer, writer registry/matrix, central integration manifest or
activation-owner file is edited.

## Exact qualification

The tool image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, OpenSSL 3.0.13. SQL versions are
`10.11.14-MariaDB-0ubuntu0.24.04.1` and `8.0.46-0ubuntu0.22.04.4`.
Both RED and GREEN create fresh private schema histories through canonical
`0056_spell_ward_durability`. Earlier 0055 and older-source results do not
qualify this candidate. The source mount is read-only, `bin/` is writable,
Docker networking is disabled, and private daemons disable TCP and use fresh
Unix sockets. The checkout `.env` is never loaded. The reader has SELECT only;
an UPDATE attempt is explicitly denied with error 1142.

The 13-input native fixture compiles in SQL and `__NO_MYSQL__` modes with
strict C++20 warnings and ASan/UBSan. Client-free native initialize/apply/reconcile
refuse with ENOTSUP. Frozen native/migration hashes, consumed Python/C++ helper
hashes, native binaries, engine logs and source preservation are recorded in
each run's evidence. The two new binding cuts invoke all three readers and
assert unchanged authority; the pre-existing native damage/constraint suite,
SELECT-only CLI paths, dump/import into a second fresh daemon, cold full restore
qualification and exact native replay remain required.
Both RED and GREEN consume identical SQL binary
`0d7d7dca94aa58540d4a13f35ccdc545abf5b178f9cc7ba97e5752d151071fc0`
and client-free binary
`2d624696a8b7d4a6735f8b325f305fcbe5233541ec1935073e3c31bb3f77d874`
(SHA-256). RED preserves the four Python inputs changed afterward; its 17
consumed helper hashes remain bound to those bytes. GREEN freezes 19 consumed
inputs, additionally naming the build-artifact and restore-qualifier helpers.

The wrapper now accepts an optional fresh evidence directory strictly below
`bin/tests/plan5-baseline-sql-restore`; it refuses an existing selection rather
than overwriting earlier qualification. No timeout or central required-case
registration is changed. RED and GREEN consume these exact commands inside
the read-only source mount:

```text
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_COMMAND_BINDING_RED=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/command-binding-red python3 -u tests/async/test_native_sql_baseline_audit.py
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/command-binding-green python3 -u tests/async/test_native_sql_baseline_audit.py
cd tests/async
python3 -m unittest -v test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.ItemRevisionTests
```

The focused unit selection passes 24 tests in 0.043 seconds, zero skips. Its new
regression substitutes each of the 32 binding bytes while resealing canonical
digests, and requires refusal, rollback, cursor closure, unchanged input rows
and SELECT-only statements. These are structural regressions, not native world
capture or gameplay qualification.

GREEN passes one native test with two engine subtests in 506.569 seconds, zero
skips. Each engine passes 147 damaged-baseline cuts and seven SQL constraint
refusals; all 147 damage cuts refuse independent restore evidence. The two new
binding cuts on each engine also refuse the full SQL restore qualifier with
`restore_economic_baseline_witness_mismatch`. The selected audit refuses its
root; the retained audit reports exactly one additional `baseline_source_claim`
finding. Thus all four original binding admissions are closed.

Each engine cold-imports the intact native baseline dump into another fresh
private daemon, qualifies full SQL history/reconciliation and repeats exact
native receipt replay. All 18 tables are unchanged, both books survive and
`active_epoch` remains NULL. MariaDB dump SHA-256 is
`379aeffe90e5defbc89f3142e6cf2ec38cbde48b0865e5fdbc2498eb9bf17895`
(332,977 bytes); MySQL dump SHA-256 is
`0a5bca8e434dec6b5ee64fdc68ee820ac68437eb2eeadc74e01c4aabe0d18e4b`
(341,737 bytes). The sibling SQL snapshot suite also passes source grammar,
source policy, original-link, orphan/interrupt, uint64 boundary, UID provenance,
operator views and explicit partial-capture checks on both engines.
`py_compile` for all four touched Python inputs and `git diff --check` pass.

Preserved evidence resides under `bin/tests/plan5-command-binding/` and fresh
`bin/tests/plan5-baseline-sql-restore/command-binding-{red,green}/`. Consolidated
evidence is `tmp/plan5/sql-baseline-command-binding-evidence.json`. Private logs,
SQL dumps, binaries, source snapshots and evidence JSON remain ignored and are
not committed.

## Remaining gates and notebook handoff

The intact native fixture still lacks the corresponding native world holdings
and UIDs: reconciliation deliberately retains `evidence_loss: 1`,
`missing_native_holding: 2`, and `missing_native_item: 2`. Its snapshot remains
partial. Baseline history/full SQL restore qualification here proves retained
component evidence and cold replay, not a complete world generation, authentic
cutover, production backup or accounting activation.

One narrow shared receipt-preimage request remains for complete baseline
authentication. The normalized digest deliberately omits real admission time;
the current baseline witness retains no exact `accepted_at_usec`. Therefore a
coherent rewrite of the witness and every canonical root/binding cannot yet be
checked against the original full `critical_operation_inbox.command_hash` by
this SQL reader. SQL `created_at` or `committed_at` must not substitute for the
native admission timestamp.

Proposed primary-owned field: retain
`economic_baseline_witness.command_accepted_at_usec`, the exact nonzero uint64
from the admitted baseline command, atomically with its witness, canonical root,
opening projections, source claim and committed inbox receipt. It must be
immutable on exact replay, preserved across turnover/backup/restore/retention,
and read from the same transaction cut. Unwitnessed historical rows need an
explicit compatibility state and cannot establish complete command authentication.
No migration or runtime schema change is made independently here.

Consumers: the independent SQL origin reader can reconstruct the full 376-byte
schema-2 baseline CCM1 with the retained admission time and EAI1, compare raw
SHA-256 with existing `critical_operation_inbox.command_hash`, and compare the
known nine-byte fence-key encoding's digest with existing `keys_hash`. Restore
evidence and operator provenance then consume that verified book. Required
native/disposable tests: atomically absent/present evidence around owner faults,
exact replay without timestamp changes, wrong/missing/zero admission time,
resealed witness/intent/plan/binding with unchanged inbox hash, wrong keys hash,
selected and retained epochs, both-engine cold dump/import/replay, and explicit
historical compatibility. This is a release evidence handoff, not a blocker for
the completed independent normalized-binding fix.

Primary still owns the maintained-build collector row signedness repair,
central stale lifecycle inventory assertions, reviewed writer-site census and
full executable writer evidence. Their previous exact-source failures remain
documented in the lifecycle-origin reader handoff; this Python-only slice does
not rerun or waive them. A maintained server build and managed v3 lifecycle
two-boot journey are required on a buildable combined primary candidate.
Native initialization/selection/allocation atomicity, startup/fault/gameplay,
complete authenticated holdings/items, trusted backup generation provenance,
financial/alias retention and erasure/export governance, off-site transport,
measured full workloads and combined R1-R8 release gates remain open.

The user confirmed on 2026-10-04 that the primary keeps the shared notebook
locally maintained. Notebook access is therefore not a Plan5 work blocker.
This report supplies the source, defect, fix, exact evidence and remaining gates
for that owner's required curator workflow. No independent local notebook or
curator write is claimed. This clarification supersedes the unresolved notebook
access gate in older slice reports; their evidence is preserved unchanged.

Accounting remains inactive. Wallet-root item exclusions and the declined
inactive spell-path change are preserved. No production data, audit correction,
deployment, PR merge or independent experimental-accounting push occurs.
Plan5 and release qualification remain open.
