# Plan 5: compare price projections across their shared root metadata

This independent slice uses branch `codex/accounting-plan5` and worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Its frozen
base is `c89cd26185fd6c05151f0ae4129cd374d79db1ac`, which incorporates the
primary's census repair `e1e3028cda754d9a8accbeb6a962acdea2cdd96d`. The native
tree is `9e364315847e320d0458a3e750e98aa1f721ec35`; the migrations tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`. The commit containing this report
is the result slice; its parent and exact diff identify the result without a
self-referential commit hash. This lane pushes only its own branch.

## Defect and fix

The selected-epoch operation roots and the independently captured lineage price
history describe the same operation. The auditor compared only their copper
amounts. Two rows could report the same operation, lineage, epoch and amount,
but disagree about reason, terminal outcome or result code without raising
`realized_price_scope_mismatch`. In particular, the price view preserved two
different reasons for the same operation and amount while reporting zero
exceptions and a successful CLI exit.

Two focused regressions against the unchanged base reader produce 12 failing
subtests. They cover three missed metadata comparisons at limits 0, 1 and 100,
and the full audit/price view with conflicting reasons at those limits. The
existing amount comparison remains a passing control. The direct projection
test compares different nonzero result codes on two rejected, unpriced rows;
their independently valid terminal metadata cannot substitute for agreement.

The reader now compares all four common root fields: `reason`, `outcome`,
`result_code` and `realized_price_copper`. Lineage and selected epoch remain
checked by the existing scope validation. Existing missing-row, duplicate,
receipt, price and outcome checks remain authoritative. The view still retains
conflicting display rows, and the global finding count remains visible at
output limit zero. Auditing leaves the source snapshot and database unchanged.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_reconcile_economy_accounting.py`, and this report. The existing
native/SQL proof can select an explicitly fresh child directory using
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS`; an existing path or a path outside
`bin/tests/plan5-price-view-sql` is refused. This keeps prior runs intact. No
shared interface, registry, matrix, migration, coordinator, native writer or
activation change is requested by this slice.

## Exact qualification and preserved evidence

Runs use immutable image `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
Ubuntu 24.04.4, Python 3.12.3, GCC 13.3.0 and OpenSSL 3.0.13. Docker has
`--network none`; `/workspace` is a read-only bind of this worktree and only
`/workspace/bin` is writable for test outputs. The evidence recorder also has
an explicit writable bind of `tmp/plan5`. All paths and hashes below are local
ignored evidence; none is committed as player data, credentials or logs.

| Check | Command inside the image | Result |
| --- | --- | --- |
| Focused RED | `python3 -u -m unittest -v test_reconcile_economy_accounting.ReconciliationTests.test_realized_price_scope_compares_all_shared_root_fields test_reconcile_economy_accounting.ReconciliationTests.test_price_view_reports_same_amount_conflicting_reasons_at_every_limit` | Two tests, 12 failing subtests, 0.073 s; exit 1; no skips. |
| Complete focused GREEN | `python3 -u -m unittest -v test_reconcile_economy_accounting.ReconciliationTests` | All 98 tests pass, 26.200 s; no skips. |
| Native/disposable RED | `python3 -u -m unittest -v test_reconcile_economy_accounting.NativeStakeSQLTests` | Both engines and all existing checks finish; 12 of 16 projection cases miss the required finding. The final exact-results assertion fails, exit 1, 260.005 s; no skips. |
| Native/disposable GREEN | Same native test command on the fixed reader | Pass, exit 0, 213.720 s; 217.140 s including container startup; no skips. All 16 projection cases match exact expected findings. |
| Preserved-capture CLI comparison | `python3 tmp/plan5/price-scope-captured-cli-proof.py` | Both native database captures at limits 0, 1 and 100: six silent RED admissions and six GREEN scope refusals. Every view retains five displayed price projections; global GREEN findings are visible with zero displayed rows. Both original captures and all copied input bytes stay unchanged. |
| Normal contract validity | `python3 scripts/validate_economy_accounting.py` | Pass: 14 fixtures, 886 routes, 2,843 candidate occurrences; explicitly no runtime/release claim. |
| Matrix consistency | `python3 scripts/generate_economy_writer_coverage.py --check` | Pass on the primary's refreshed census; coverage/release remain blocked. |
| Shared contract regressions | `python3 -u -m unittest -v test_economy_accounting_contract` | All 31 tests pass, 0.248 s; no skips. |
| Release refusal | `python3 scripts/validate_economy_accounting.py --release` | Expected exit 1: `writer has no executable evidence`. |
| Whitespace | `git diff --check` | Pass. No C/C++ source change requires a format or maintained-build rerun. |

Unit invocations set `PYTHONPATH=/workspace/tests/async` and a fresh
`PYTHONPYCACHEPREFIX` below `bin/tests/plan5-price-projection-scope`. The native
invocations additionally set `DURIS_RUN_STAKE_SQL_INTEGRATION=1` and
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS` to
`/workspace/bin/tests/plan5-price-view-sql/price-scope-red` or
`/workspace/bin/tests/plan5-price-view-sql/price-scope-green`, respectively.
These directories must be fresh on a subsequent invocation.

Both disposable engines start from `migrations/bootstrap_multithread_safe.sql`,
adopt `fresh_bootstrap`, execute `migration_runner.run_pending`, and verify
the exact terminal history row `56 / 0056_spell_ward_durability`. Engine
versions are MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. SELECT-only accounts deny UPDATE on each engine.
Every capture uses REPEATABLE READ plus a consistent READ ONLY transaction,
one rollback and one cursor close. Exact pre/post rows of seven source tables
agree after every capture; `economic_lineage_state.active_epoch` remains NULL.
Only the disposable fixture owner seeds controlled rows. No project `.env`,
live database, external network or production data is used.

The retained native proof still runs 90 captures, including 72 fault captures,
62 source-fault captures, 44 source-kind captures, four original-link faults,
eight price captures and 24 existing price CLI cases. The added 16 copied
projection checks cover reason, outcome, result code and amount on both engines
at both valid price capture points. The separate preserved-capture CLI proof
adds 12 executions of exactly the saved RED or current GREEN reader. Its small
copied base repository contains the unchanged RED reader and the exact registry,
so ordinary CLI execution has its normal repository path resolution.

The native fixture compiles and executes the existing codec closure in both
SQL-configured and `-D__NO_MYSQL__` modes. Flags are
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -Isrc`,
with `-lcrypto`. The generated `probe.cpp` and these exact native sources are
compiled: `economic_accounting_plan.c`, `economic_accounting_types.c`,
`economic_accounting_intent.c`, `critical_command.c`,
`item_transfer_command.c`, `craft_pouch_mutation.c`, `chaos_pouch_ledger.c`
and `player_snapshot_codec.c`, in their existing `src/` subdirectories.
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` are set for both binaries.
There are no sanitizer findings. Both modes agree on all 104 source-grammar
cases, 1,107 source-policy decisions, six original-link cases and four price
roots. These are codec builds, not complete maintained server builds.

Both modes within each run produce identical binaries and encoded bytes.
RED binary SHA-256 is
`6a52f73a45e4376362a7db2022851dbc5e12b78884fc13fc1d54d8fad01d8a7a`;
GREEN binary SHA-256 is
`3750be7efb5f0e118d734e7d7e2c05383032ea9e79e1827bd720ab0f67124f75`.
Fresh build paths are embedded in debug information, so the separate runs
have different binary hashes. All four outputs have encoded SHA-256
`1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.
RED and GREEN native test source copies have identical SHA-256
`c2cdf4f8baba2134827bfa612b266d5037167f0c051af6a6471198936c152751`.
Only the independent reader changes between those invocations.

Evidence paths are:

- `bin/tests/plan5-price-projection-scope/`: immutable RED/GREEN readers and
  test sources, unit/native/contract logs, command records, tool versions,
  recorder/proof source copies and the captured CLI proof's inputs/outputs.
- `bin/tests/plan5-price-view-sql/price-scope-{red,green}/`: generated native
  source, both binaries, encoded outputs, final exported price captures, and
  the per-engine exact `price-scope-results.json` comparisons.
- `tmp/plan5/price-scope-base-tree.txt`: the frozen Git entries for all native
  and migration inputs.
- `tmp/plan5/price-scope-evidence-start.json` and
  `tmp/plan5/price-scope-evidence.json`: initial and final SHA-256 inventories
  and explicit source/artifact preservation assertions. The final manifest
  SHA-256 is `c34fcdad100e9fdc20274e4db31b3747eeae43669886154d1ffd0102fbb62c3c`.
  The recorder verifies all 1,254 native files, all 236 migration files, and
  all 24,904 pre-existing artifacts byte-for-byte unchanged. It hashes 652
  fresh evidence files, plus the Python input inventory and exact registry.

## Remaining gates and curator handoff

These checks qualify the independent consumer on the frozen source above.
The native codec fixtures create four structurally valid price roots across
two epochs, then insert them into fresh canonical disposable schemas. They do
not execute the real commerce producers or qualify gameplay, native mutation
publication, full-world capture or accounting activation. Deliberate price
projection corruption changes only copied export data; it never repairs or
changes the SQL evidence.

The normal validator, generated matrix check and 31 shared contract tests pass
on the refreshed census input. Release validation refuses a writer without
executable evidence. Inventory completeness, synthetic roots and isolated
passes do not close the remaining Plans 1–5 or release gates.

The maintained flatfile service build defect documented in
`PLAN5_V3_SERVICE_BUILD_HANDOFF_2026-10-04.md` remains on this native tree:
`flatfile_economic_runtime.c` lacks the existing replay-ownership header needed
for `player_save_execution_guard::current_ownership_epoch()`. The primary owns
that repair; this slice does not repeat the unchanged failed build or claim
managed v3 cold boots. Complete baseline command receipt authentication still
needs retained original admission authority, as described in
`PLAN5_SQL_BASELINE_COMMAND_BINDING_SLICE_2026-10-04.md`; the normalized command
binding fix does not close that shared gate.

The primary has published `b96c85c176a4e19a75822b9b0edd5e0328d84d91`, importing
the preceding independent command-binding slice and build handoff. That refresh
does not change native source or migrations. This price slice remains based
on its own frozen `c89cd26185fd6c05151f0ae4129cd374d79db1ac` input; only the
primary may publish and qualify the combined candidate on
`experimental-accounting`.

During final preservation verification the primary also published the narrow
header repair as `c78a97055920d5e38f068e8df30805ffaa22ebf6`. That source is
not the frozen input to this price slice. The next service qualification can
adopt it and retry the maintained build and actual managed v3 boots on fresh
artifact paths; this report does not promote its isolated header syntax checks
to either result. The primary also confirmed the original baseline admission
time field and historical-NULL semantics for its forthcoming additive schema
work. Its actual transaction/migration implementation and the independent full
command verification still require their own tested slice.

The primary maintains the shared notebook locally through its curator workflow,
per the user's clarification. This report supplies the curator with the defect,
owned changes, exact source and evidence, and remaining gates; it does not claim
an independent notebook write. Accounting stays inactive, wallet-root item
exclusions and the declined inactive spell-path behavior are preserved. No
production data, deployment, activation or PR merge is involved.
