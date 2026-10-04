# Plan 5: retained ordinary-root lifecycle namespace during SQL restore

Branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base `559dbd6fe6217615ced7bb6aa4c6162408806363`; result commit and exact
committed inputs are bound by `tmp/plan5/retained-namespace-evidence.json`.
Canonical remote refreshed before this slice:
`f7d26eaa721cd3b675c0b0c65009a2535813f400`.

## Defect and owned boundary

An ordinary root's valid EAI1/EAP1 capsules, balanced projections, source claim,
native currency history and successful receipt cannot replace its retained
lineage and epoch namespace. The independent restore check previously did not
explicitly join every ordinary root to those authority rows. Its baseline-book
checks covered baseline namespaces, leaving ordinary histories without books
outside that check.

The new regression uses the existing native coin-transfer fixture, whose
ordinary reason is 3, with nonzero wallet/bank postings and a rejected root.
It removes the inactive lineage-state row through the private fixture owner,
then exercises the actual restore CLI entry. A second case models a damaged
import by deleting the retained epoch while the fixture owner briefly disables
its own FK checks and immediately reenables them. Readers never disable FK
checks. No native monetary state or canonical capsule is changed.

The first reproduction admitted corrupt restored authority after lineage loss
on both MariaDB 10.11.14 and MySQL 8.0.46: one test collected, two engine
failures, no skips, 272.967 seconds. After the lineage check was added, a second
run refused lineage loss but admitted the missing epoch on both engines:
one test collected, two engine failures, no skips, 224.524 seconds. Their exact
inputs, output and artifacts remain separately preserved. These are failed
qualification runs, not transport/setup failures or passing component evidence.

## Fix and proof

The existing independent canonical-evidence helper checks every retained
root, including rejected roots and inactive/historical epochs, for its exact
lineage-state row and composite lineage/epoch row. Missing authority refuses
with `restore_economic_lineage_mismatch` or
`restore_economic_epoch_mismatch`. Empty root histories remain eligible, and
baseline-book checks keep their existing precedence. No active epoch is selected.

The existing restore damage helper verifies the full entry point refuses
without changing captured authority, restore the private fixture, and verify
exact original rows, with both fixture-owner and reader FK checks explicitly
verified on. Existing canonical, malformed projection, source, receipt,
pagination, decoder, old money-history and empty-history cases remain required.
The native C++ fixture and all server sources remain unchanged. The pure
decoder/class ASTs remain unchanged; the new helper queries use SELECT only.

Owned files are `scripts/economic_restore_evidence.py`,
`tests/async/run_restore_accounting_evidence_mysql.py`,
`tests/async/test_restore_economic_coin_effects.py` and this report. There is no
schema or shared contract change. Central registration and the combined release
report remain with the primary owner.

## Evidence and open gates

The final canonical suite passes on both engines in 902.527 seconds, one test
collected/passed, zero skips. Both new namespace cases refuse through the full
entry point without changing captured authority. The existing 39 canonical
cuts per engine (36 refusals, three diagnostic admissions, six full-entry cuts),
259-root pagination, 8 KiB intent and 4 MiB plan bounds, 30 audited native coin
cases and two native/SQL constraint refusals per engine remain intact. The
native codec corpus still has 3,026 cases (1,054 accepted, 1,972 refused), with
independent agreement in SQL and `__NO_MYSQL__` compilation modes.

The unchanged native baseline suite also passes on both engines in 596.939
seconds, one test collected/passed, zero skips. Each engine retains its 145
cuts, seven constraint refusals, 17 damaged-import cases, and prior exact
diagnostic/refusal results. Cold dump/import, the full qualifier and exact
native replay pass for two retained nonempty books, with all 18 captured tables
unchanged and `active_epoch` NULL. This does not prove complete empty-book loss
detection or complete native/world capture.

The component suite collected 124 tests in 13.416 seconds: 119 passed and five
were skipped (two audit budgets, two native-origin gates and one native-stake
gate). These skipped gates are not promoted by either native suite.

All 1,232 native inputs and 236 migration inputs remain unchanged, at native
tree `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, from native base
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Both suites use fresh schemas,
adopt canonical bootstrap and apply migrations through `0056_spell_ward_durability` with
FK/CHECK constraints enabled. The ordinary native SQL/flatfile codec binary
remains `faf3038738747058d6d1273c31cb986351e3ae34a768d92c7ad6190cd8565edc`.
The native baseline SQL binary is
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
its client-free counterpart is
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
Native fixtures use C++20, strict warnings and ASan/UBSan. No native C++ source,
compiler flag, server build, gameplay writer or decoder policy changes here.

New canonical artifacts use `bin/tests/plan5-retained-namespace-canonical/`,
preserving earlier baseline/restore evidence. The first reproduction's input
copies and hashes are `tmp/plan5/retained-namespace-red-*`; its log is
`tmp/plan5/retained-namespace-red.log`. Artifacts were copied to separate
failure directories only after the native process is terminal and before a
subsequent run reuses the new working artifact directory. Completed lineage and
epoch failures are in `bin/tests/plan5-retained-namespace-lineage-red/` and
`bin/tests/plan5-retained-namespace-epoch-red/`.

The unchanged native baseline wrapper/worker use their original container path
`/workspace/bin/tests/plan5-baseline-sql-restore`. An additional nested writable
bind mount places these artifacts in the separate physical directory
`bin/tests/plan5-retained-namespace-baseline/`, preserving the original baseline
artifacts without changing that tracked tooling. Its log is
`tmp/plan5/retained-namespace-baseline.log`. Both final suites consume eight
frozen test/helper/fixture inputs at
`tmp/plan5/retained-namespace-frozen-inputs.json`.

Use pinned `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL 1.0.2-2ubuntu1.1,
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL 8.0.46-0ubuntu0.22.04.4.
Daemons are fresh, private and socket-only; no host ports are published and
checkout credentials are stripped. Source is read-only and `bin/` separately
writable. The baseline invocation has the additional artifact mount above:

```text
DURIS_RUN_RESTORE_COIN_INTEGRATION=1 DURIS_PLAN5_CANONICAL_EVIDENCE=1 python3 -u tests/async/test_restore_economic_coin_effects.py -v
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v
python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v
python -m py_compile scripts/economic_restore_evidence.py tests/async/run_restore_accounting_evidence_mysql.py tests/async/test_restore_economic_coin_effects.py
git diff --check
git diff --cached --check
python scripts/validate_economy_accounting.py --release
python tmp/plan5/record-retained-namespace-evidence.py --committed
```

The release validator still exits 1 with `writer has no executable evidence`;
later release gates were not reached. The source/diff checks pass. The curator
workflow is still unavailable in the current callable tool inventory, and the
required `AI_CONTEXT.md` is still absent.

The recorder pins committed source, all native/migration inputs, the image,
compiled binaries, native output, both failure-stage inputs/artifacts, final
engine results, every preserved prior artifact, and the release validator's
exact inputs. Final logs are `tmp/plan5/retained-namespace-green.log` and
`tmp/plan5/retained-namespace-baseline.log`. Sample timings describe these
synthetic/native qualification commands, not operation-latency or release-host
workload budgets. Generated logs, dumps, binaries and manifests remain local
and ignored.

No shared interface/schema change is needed. For primary-owned registration,
retain the canonical invocation above and require one exact full-entry
`restore_economic_lineage_mismatch` and one
`restore_economic_epoch_mismatch` refusal per engine, in addition to the
existing matrix. Invariants use `economic_accounting_operation.lineage/epoch`,
`economic_lineage_state.lineage` and the composite
`economic_epoch(lineage,epoch)` identity; no active-epoch or outcome filter is
permitted. Consumers are the existing independent SQL restore entry, retained
SQL history qualifier, central test registration and combined release report.
Tests are the two full-entry namespace damage cases on each canonical engine,
plus unchanged baseline/canonical/native reference matrices. Registration and
the tested combined candidate remain with the primary owner.

Native/synthetic restore fixtures do not qualify the unpublished combined
candidate, full native capture, real supported writer journeys, complete typed
erasure, full-world service restore, populated upgrades or workload budgets.
The release validator's missing executable writer evidence, retained empty-book
marker, governance/export/retention policy and primary integration remain open.
`AI_CONTEXT.md`, the notebook reference and the required curator workflow remain
unavailable; no notebook update is claimed. Accounting stays inactive and
wallet-root exclusions, the declined inactive spell change and active blackjack
refusal remain unchanged. No activation, production access, correction, merge
or deployment is performed.
