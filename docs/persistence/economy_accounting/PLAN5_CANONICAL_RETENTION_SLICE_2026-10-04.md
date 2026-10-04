# Plan 5: canonical history at the native SQL erasure boundary

Branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Slice base `3b076856aa63a9ee7ae83050e2e3ffa8e54dd081`; the result commit and
committed source hashes are bound by
`tmp/plan5/canonical-retention-evidence.json`. Publish only this lane's branch.
The refreshed integration prefix remains
`f7d26eaa721cd3b675c0b0c65009a2535813f400`.

## Qualification gap and scope

The existing inactive native deletion observer compared retained rows before
and after erasure, failures, retry and cold restart. It did not invoke canonical
economic qualification. Preserved bytes alone did not establish the EAI1/EAP1
metadata, digest, receipt, source, count or projection invariants.

An initial reason-mismatch hypothesis was disproved by native evidence:
`economic_reason::coin_transfer` is 3, the emitted EAI1/EAP1 metadata is 3, and
the SQL rows already used 3. The incorrect value was the previous restore
diagnostic's hard-coded `ordinary_reason: 1`. That independent display defect
was corrected in the base commit, with native-artifact proof at
`tmp/plan5/restore-reason-diagnostic-evidence.json`. No valid SQL retention
fixture was changed to another reason. The successful pilot is preserved under
its original `canonical-retention-red.log` name and an identical pilot copy;
it is not a failed run relabeled as a pass.

The owned observer now calls the existing independent
`qualify_database_restore.require_economic_evidence_integrity` for every
consistent SELECT-only cut. It reuses the production independent decoder and
SQL qualification functions, with a cursor adapter that permits SELECT only.
No new wire interpretation or native mutation logic is introduced.

After each native deletion/restart journey, the private fixture owner changes
one retained root's actor ID to another legal SQL value. The observer must
refuse with exactly `restore_economic_metadata_mismatch`, preserve every
captured history row, and leave its successful-capture count unchanged. The
fixture owner restores the intentionally damaged fixture and verifies exact
original rows before normal qualification continues. The observer provides no
correction operation. SQL/transport errors are not accepted as semantic refusals.

The existing history collector serves both the normal observer and the fault
assertion. Rollback, cursor close and SELECT/transaction-declaration restrictions
are now asserted in cleanup for successful and refused cuts alike.

## Exact source and native evidence

All 1,232 native source inputs and 236 migration inputs remain unchanged, at
native tree `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`. Four fresh synthetic
schemas use bootstrap/adoption and canonical migrations through
`0056_spell_ward_durability`, with FK and CHECK constraints enabled.

The native server is the strict production-profile SQL executable
`bin/server/plan5-baseline-restore-dms_new`, SHA-256
`b8393c48e6180ae98b9e1bfed51aa4fbf69e1d7402ec9dbc007d68b98675b0aa`.
Its full build and exact input provenance are preserved in
`tmp/plan5/sql-baseline-restore-evidence.json`. This slice changes Python test
tooling; it does not claim another server build. The original server remains
byte-identical.

The unchanged generated C++ probe and eight unchanged native sources compile
in SQL and `__NO_MYSQL__` modes with C++20, strict warnings, ASan and UBSan.
Both probe binaries have SHA-256
`91718265688ccd03fadf176151b0b1c0e25e17863a262224f123949eb8105cfe`.
Six 1,040-byte native cases agree across modes and retain the previous exact
encoded output hashes. Binary hashes differ from the older artifact directory
because the generated source/debug path changed; generated C++ bytes are
compared directly, not inferred equivalent from outputs.

The history has one committed zero-effect root with a source claim and one
rejected root with an original-operation link per actual target PID. Each schema
has an inactive lineage and retained epoch. There are twelve roots and six
claims across the four schemas, covering six erased identity instances. This
is seeded history observed through real native account/character menu paths;
it does not exercise an admitted economic writer, nonzero financial postings,
typed active erasure, UID custody or a complete native capture.

| Engine | Native path | Target PIDs | Canonical normal cuts | Canonical refusals | Cold restarts |
| --- | --- | --- | --- | --- | --- |
| MariaDB 10.11.14 | Whole-account deletion | 1 | 4 | 1 | 2 |
| MariaDB 10.11.14 | Account deletion, name reuse, character deletion | 1, 2 | 7 | 1 | 3 |
| MySQL 8.0.46 | Whole-account deletion | 1 | 4 | 1 | 2 |
| MySQL 8.0.46 | Account deletion, name reuse, character deletion | 1, 2 | 7 | 1 | 3 |

The final invocation exited 0 with all four native journeys complete, 22
canonical normal cuts, four exact semantic refusals, ten cold restarts and
zero skips. All 26 observer transactions rolled back and closed their cursors.
The successful pilot lacks the four added fault assertions and remains
separate evidence. No total elapsed time or workload budget was measured.

Every normal snapshot remains incomplete and unfenced, retaining exactly
`evidence_loss: 1` and `unfenced_snapshot: 1`. Operation views at limits 0/1/100
retain global exception count 2 and unchanged input. The SELECT-only observer
is denied a no-op UPDATE with error 1142. Every cut proves `active_epoch` NULL
and no staged installation in phase 1/2. Accounting is never activated.

## Commands, artifacts and handoff

Pinned image `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL 1.0.2-2ubuntu1.1;
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL 8.0.46-0ubuntu0.22.04.4.
The checkout is read-only at `/workspace`, with `bin/` separately writable.
Fresh private daemons use verified ephemeral container-loopback ports; no host
port is published, no existing schema is reused, and checkout credentials are
stripped. All generated output is local and ignored.

```text
DURIS_RUN_PLAN5_RETENTION_INTEGRATION=1 python3 -u tests/async/run_plan5_retention_journeys.py --server /workspace/bin/server/plan5-baseline-restore-dms_new
python -m py_compile tests/async/run_plan5_retention_journeys.py
git diff --check
git diff --cached --check
python scripts/validate_economy_accounting.py --release
python tmp/plan5/record-canonical-retention-evidence.py --committed
```

Native evidence: `tmp/plan5/canonical-retention-green.log` and
`bin/tests/plan5-retention-canonical-native/`. The pilot's exact source,
outputs and successful log remain separate. Thirteen consumed source/helper
inputs are frozen before the final native run. The manifest pins their exact
bytes, native/migration maps, image, binary/build provenance, native output,
normal/fault cuts and committed result. No total elapsed time or economic
workload budget was measured by this runner.

Two owned files: `tests/async/run_plan5_retention_journeys.py` and this report.
No canonical schema, accounting contract, coordinator, producer, matrix,
registry or activation-owner file changes. There is no shared schema request.
The primary-owned runner should require four `RETENTION_JOURNEY` records,
22 `RETENTION_CAPTURE` records with `canonical_economic_integrity: true`,
four `RETENTION_CANONICAL_FAULT` records with the exact refusal above, and
per-journey `canonical_qualified_captures` of 4/7 and `canonical_refusals: 1`.
Consumers are central test registration and the combined release report; tests
are the two engines' account and character journeys. The source-specific server
path and explicit integration gate above must be preserved. Registration
remains with the primary owner.

The release validator exited 1 with `writer has no executable evidence`.
`tmp/plan5/canonical-retention-release-gate.log` preserves that refusal; the
manifest pins the validator, registry, golden fixtures, writer inventory and
coverage matrix inputs along with the unchanged native source map. This is
the first reported release gate, not an exhaustive result for later gates.
The primary owner must register and qualify writer evidence and rerun the
combined candidate. No shared registration or contract file was edited here.

Complete native capture, real economic writer journeys, typed active erasure,
flatfile parity at the combined candidate, populated upgrades, full-world
backup/service restore, governance/export/retention policy, workload budgets
and the shared empty-book initialization marker remain open. Native/synthetic
fixture success does not establish release completion. No production access,
activation, audit correction, merge or deployment occurred. Wallet-root item
exclusions, the declined inactive spell path and active blackjack refusal remain
unchanged. `AI_CONTEXT.md`, the notebook reference and curator workflow remain
unavailable; this report is a handoff, not a claimed notebook update.
