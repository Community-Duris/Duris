# Plan 5 exact baseline projection representations

The independent SQL origin reader accepted Python float, boolean and Decimal
values equal to native integers. A malformed imported schema could therefore
receive a clean origin capture despite returning noncanonical representations.
The reader now compares exact types and values against independently decoded
original capsules, without correcting the findings or changing native authority.

## Source and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `a0491e0a385cecee2842733718fd92da1967a5eb`, the preceding maintained
  build handoff. Executable reader inputs were pinned on its parent
  `d18f8de63d2f7f7605bd37567d64435b6cf86650`; only the build report separates
  these commits. The input maps and owned source hashes bind this pending fix.
- Tested primary: `f23aaa3c725c720f70ab2182a0942da511da90dd`.
- Native tree: `139556ad49b39d006588572015dc5006e8186a59`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, public
  canonical0056. Original native baseline bytes are EAB1; the EAB2 controls in
  this slice are explicitly reference models.
- Owned files: `scripts/economic_sql_audit_origins.py`,
  `scripts/reconcile_economy_accounting.py`,
  `tests/async/test_economic_sql_audit_origins.py`, and this report.
- Evidence manifest: `tmp/plan5/baseline-type-evidence.json`; result commit and
  verified remote: `tmp/plan5/baseline-type-delivery.json`.

No shared coordinator, producer, accounting contract, schema, registry/matrix or
activation file changes. No interface or schema request is needed for this fix.
All earlier owned slices and original branch heads remain preserved on the
requested remote branch. There is no notebook locality blocker.

## Established defect and complete origin-reader fix

The immutable preimage reader SHA256 is
`e80a77138c4f585a8d17a4fb1587ff6b6a014a8ee099cab20242aa04e2b58c9d`.
An initial 80-cut model reproduction finds 80 false-clean captures: equal floats
or booleans in root metadata, revisions, counts, inbox metadata/status, account
effects, coin postings or reservations. Every cut retains the altered value,
rolls back once and closes the cursor. Full original source and model results
remain under `bin/tests/p5-baseline-types-20261005/red-models/`.

The four new methods run against that unmodified old reader fail with 156
subtest failures and 6 uncaught-type errors, zero skips. The error cases are
malformed inputs the old reader did not convert into its controlled refusal.
These expected RED results are preserved, not treated as qualification passes.

The saved-plan reconciler's existing recursive exact-type comparison is moved
to module scope and extended to tuples for its second real caller. Its original
list behavior stays; no duplicate decoder or mutation dependency is added.
The origin reader uses it for root metadata and SQL projection families,
persisted counts, inbox fields and status/revision controls. Standalone root
verification also requires a positive unsigned 64-bit book revision and an exact
integer inbox revision, protecting the restore reader that reuses this verifier.
SQL-projected binary IDs/digests must retain their original bytes representation.

Audit bounds now require nonnegative exact integer results. Both read-only SUM
queries cast their nonnegative aggregate to SQL UNSIGNED, because normal
MySQL/MariaDB SUM returns Decimal even for integer arguments. No stored column
is cast or coerced by the audit. Projection rows and byte sizes remain bounded
by the existing 100,000-row and 32-MiB limits; capsule layouts, domains, root
derivation and refusal diagnostics stay versioned and unchanged.

The existing tests cover all integer projection fields and root controls with
float/Decimal/string/None/boolean substitutions, bytearray substitutions,
standalone revision bounds and malformed source bounds. They require controlled
refusal, rollback, cursor closure and unchanged inputs. Existing EAB1/EAB2 reader
controls and saved-plan range/representation tests remain in the selected suite.

## Exact validation

Image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
GCC 13.3.0, Python 3.12.3; read-only checkout and no network. Because the host's
C: drive lacked space, generated artifacts use a writable `/workspace/bin` bind
mount on D:. The complete evidence root is retained at
`D:/CodexEvidence/accounting-plan5/bin/baseline-types-20261005`; each path below
is relative to that physical `bin/` root. All inputs remain unchanged over every
recorded run. The original C: red model directory is copied here and kept in place.

```sh
PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_economic_sql_audit_origins.OriginTests \
  test_economic_sql_audit_origins.ItemRevisionTests \
  test_economic_sql_audit_origins.BaselineVersionTests \
  test_reconcile_economy_accounting.ReconciliationTests \
  test_plan5_child_identity.ChildIdentityTests \
  test_economic_sql_canonical_audit.CanonicalAuditTests \
  test_economic_restore_mobile_grammar.MobilePositionTests
```

All 187 methods pass, zero skips, 66.754 seconds in unittest /71.633 including
startup and capture overhead. This protects both actual callers of the reused
comparison. Exact commands and input maps are in `tests/p5-baseline-types-20261005/pure/`.

The actual guarded native selection is
`test_economic_sql_audit_origins.NativeSQLOriginTests`. Its preservation wrapper
executes that original two-method class, builds the production flatfile baseline
fixture freshly under ASan/UBSan, and runs `baseline-rich`: four native witnesses
in two inactive epochs, holding IDs 255/256/512/UINT64_MAX and original capsules.
The native binary SHA256 is
`9e59042874cb751e8c461a9547440b32484ce7ee76da4c4c7660f55717490b9a`.

```sh
python3 -u -B tmp/plan5/run-baseline-type-checks.py native
python3 -u -B tmp/plan5/run-baseline-type-checks.py native-proof
```

These captured runner commands set `PYTHONPATH=tests/async`,
`DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1`, `DURIS_PLAN5_PRESERVE_ORIGINS=1`, and
the respective preserved-output path. Their child commands are
`python3 -u -B tmp/plan5/retain-flatfile-qualifier.py` and
`python3 -u -B tmp/plan5/retain-baseline-type-native.py`. All helpers, runtime
receipts and actual sources are retained in evidence. The committed class can
also run directly with the explicit integration gate.

| Run | Methods/skips | Process seconds | Per-engine new-reader result |
| --- | --- | ---: | --- |
| Native standard | 2/0 | 262.007 | 7 captures, 7 refusals, 14 rollbacks |
| Native old/new proof | 2/0 | 255.227 | Same original selection repeated; 4 additional old-reader false-cleans |

Both runs freshly bootstrap canonical0056 on MariaDB
`10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL `8.0.46-0ubuntu0.22.04.4`.
Private daemons disable TCP, use task-owned Unix sockets and never consume
checkout credentials. The SELECT-only audit user is denied UPDATE with 1142.
Each original native holding-order corruption remains refused. For the new
negative cuts, a separate owner changes `after_silver` on account effects or
`delta_silver` on postings to DOUBLE or DECIMAL(20,0). Their values 0/2/-2 remain
exact, while the driver returns float/Decimal. The fixed reader refuses all
four; the old reader falsely accepts the same four on each engine. The old
reader's extra transactions use that same SELECT-only role.

Canonical BIGINT columns are restored after each cut and complete database-row
snapshots match their controls; every repaired control passes again. Altered
schemas are explicitly negative fixtures, not canonical schema qualification.
All selected new-reader transactions roll back and close their cursor; every
read leaves database rows and native evidence bytes/modes/link counts unchanged.
`active_epoch` remains NULL. Private datadirs, migration histories, native
capsules, binary/cache metadata, both preimages and failed attempts are retained.
The repeated native selection does not double the number of distinct cases.

## Gates and curator handoff

This closes baseline-origin representation comparisons at the recorded source.
It is not a claim about every independent restore projection comparison, live
watermarks, measured budgets or an entire release. Full gameplay/persistence,
original writer routes, backup/restore/retention and erasure journeys, migration
upgrades, workload/fault checks and R1–R8 remain open. No selected green test is
skipped; no external blocker prevents this owned fix.

The fetched primary `9665df324ab9751408d2cb9b735797f2d13b3ef8` integrates the
earlier versioned readers and publishes a reviewed private schema61 interface.
It does not publish native EAB2 or measured migrations beyond 0056. This slice
does not relabel prior EAB1 results as native EAB2 qualification or change any
historical 0056 fixture to an unavailable successor. The baseline admission-time
and schema 61 package must be consumed and checked only when its actual source
and measured engine metadata are published.

Import this completed slice from `codex/accounting-plan5` through the primary's
notebook curator workflow. Register only its exact source and scope; central
registration and publication of the tested combined candidate stay primary
owned. Preserve inactive behavior, wallet-root item exclusions and the declined
inactive spell-path change. No accounting activation, production mutation,
deployment, PR merge or audit autocorrection occurred.
