# Plan 5: captured lineage prices in the bounded operator view

This independent slice is based on
`87424d5a7ee56d6078652fb46daee7055e836870` in branch
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
The result commit and exact committed blobs are bound by the local manifest
`tmp/plan5/price-view-evidence.json`. This branch is published independently;
the Plan 5 lane does not push `experimental-accounting`.

The refreshed canonical remote is
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. All 1,232 native C/C++ inputs and
the migration inputs match the preceding qualified slice. The native source
tree remains `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, from canonical base
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Canonical fresh disposable
databases are qualified through `0056_spell_ward_durability`; no 0055-only
result qualifies this slice or the primary owner's combined candidate.

## Defect and complete scoped fix

The SQL exporter already captures `native.lineage_realized_prices` and
`native.realized_price_coverage` across epochs. The independent reconciler
already validates those retained rows and terminal inbox receipts. However,
`--view prices` read only selected-epoch `operations`. It silently omitted
retained prices and conflicts between the selected and retained projections.
Its global status was visible after the preceding slice, but the operator
could not tell whether lineage price history had been captured.

Four new focused regressions against the unmodified base reader established
the defect: 20 failures and 10 missing-coverage errors. In particular, one
selected price was returned when four captured prices were expected, and the
conflicting five-projection case still returned only one. The RED test source,
base reader and log remain preserved locally.

The view now combines the selected-epoch roots with the paired captured
lineage price records. It displays only committed roots with exact integer
prices in 0..INT64_MAX, preserving zero and the upper boundary. Invalid,
missing, rejected and unknown values still produce their existing audit
findings and global count; they do not become displayed prices. Existing
selected-epoch snapshots without the paired history retain their price rows.

Each displayed row contains `epoch`, `operation_id`, numeric `reason`, and
`price_copper`. The exact tuple `(epoch, operation_id, reason, price_copper)`
is deduplicated; different projections remain visible. Lexical epoch/ID order
and numerical reason/price order are deterministic, without claiming epoch
chronology. Full count and capture coverage remain visible at limits 0/1/100.
Aliases, frozen command data, receipts and canonical blobs are not displayed.

Price coverage preserves all existing global fields and adds:

- `lineage_history_available`: true only when the retained price list and its
  coverage object were both captured.
- `realized_price_coverage`: a copy of the existing exporter's
  `column_available`, `candidate_rows` and `missing_price_rows`, or null when
  history is unavailable. Candidate rows count captured roots before price
  filtering and exact-projection deduplication.

Unavailable history, captured empty history, an unavailable persisted price
column and missing committed prices remain distinguishable. These fields
describe the captured input and confer no release or repair authority.

Only the reader's `view` function changes. AST comparison confirms that the
entire `Reconciler` class is unchanged. The snapshot input contract, SQL
exporter, migrations, registry/matrix, coordinator, producers and activation
owner are unchanged.

## Native and canonical database proof

The existing native fixture now emits four additional structural EAI1/EAP1
root pairs for shop buy, shop sell, collector purchase and auction bid
(reasons 21/22/24/27). They span a selected epoch and an earlier ordinal epoch.
All four are deliberately zero-effect structural roots. Native metadata,
intent digest, plan encoding and plan decoding pass in SQL and
`__NO_MYSQL__` modes under C++20, strict warnings, ASan and UBSan. Persisted
prices are beside the SQL plan, not a field inferred from native plan bytes.
This does not prove a real producer, writer capability, locked native
authority, money/item mutation or gameplay price binding.

The original held/payout plans, 104 source grammar decisions, 1,107 metadata
policy decisions and six original-operation decisions remain checked. The
new artifact directory is `bin/tests/plan5-price-view-sql`; every preceding
slice's native binaries and output bytes remain preserved.

For each private MariaDB/MySQL instance, the runner bootstraps the canonical
schema, adopts the fresh bootstrap and applies pending migrations through
0056. FK and CHECK enforcement remain enabled. It inserts the native roots,
terminal inbox receipts and successful committed source claims, with one
selected root duplicated by the lineage price projection. Persisted prices
exercise 3000, 125, zero and INT64_MAX.

Each of four additional cuts is captured through the existing exporter using
a SELECT-only reader in a read-only repeatable-read transaction: clean prices,
a missing historical committed price, a rejected historical root carrying a
price, and a clean restored fixture. Canonical rejection has no committed
plan and no successful source claim. Only the disposable fixture owner changes
data between cuts; the reader never repairs a finding. All seven economic
tables are byte-value equivalent before/after each capture, with exactly one
rollback and cursor close. The reader's UPDATE attempt is denied with 1142.
`active_epoch` remains NULL throughout both instances.

The total is 90 read-only cuts, 45 per engine, including eight new price cuts.
The new cuts invoke the real CLI at limits 0/1/100: 24 database-derived CLI
cases. Exact count, deduplication, price coverage, global finding count, exit
status, row bounds and snapshot-file preservation are checked. Clean captured
cuts also retain full price count and two global findings after explicitly
marking the diagnostic snapshot incomplete and unfenced. Native encode-mode
agreement and database read behavior do not make that synthetic authority
snapshot a complete native exporter or a release candidate.

The first qualification attempt exposed two errors in the new test fixture:
compact rows reached the 100,000-row cap before the requested byte size, and
the proposed rejected root violated canonical terminal-outcome constraints.
The failed source, logs, captured file and binaries remain preserved under
`price-view-attempt1-*` and `bin/tests/plan5-price-view-sql-attempt1`. The final
fixture uses valid trailing JSON whitespace at the row bound and the canonical
rejected-root representation. Constraints were never disabled. The entire
final qualification is rerun against the frozen final source.

## Exact tested source and commands

The frozen executable sources have SHA-256:

- `scripts/reconcile_economy_accounting.py`:
  `798c840da54a2c03bbcf78080cc94c73980f7eaa2a0cc6f3429cec1efc7b98d3`.
- `tests/async/test_reconcile_economy_accounting.py`:
  `07b478476d958dbdd178b3a0d93b9f8831584b18ef98914f0d69df16615a8310`.

The immutable container image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04, Python 3.12.3, GCC 13.3, PyMySQL 1.0.2-2ubuntu1.1,
MariaDB 10.11.14-0ubuntu0.24.04.1 and MySQL 8.0.46-0ubuntu0.22.04.4.
The checkout is mounted read-only, `bin/` is writable, the databases use fresh
private datadirs and Unix sockets, and no ports are published. Checkout DB
credentials are stripped by the existing private-database helper.

The native SQL and flatfile probe binaries both hash to
`7cac470a0638272bd0b223cb4c95fdb0b4e4743b2ee383deb2af47c8b86fdb91`;
their identical length-framed output hashes to
`1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.

| Command/check | Result | Local evidence |
| --- | --- | --- |
| Four new `ReconciliationTests.test_prices_*` / `test_price_cli_*` regressions at the base reader | RED, 20 failures and 10 errors | `price-view-unit-red.log`, `price-view-red-tests.py`, `price-view-base-reconciler.py` |
| Same four focused regressions after the view fix | PASS, four tests and 12 actual CLI cases | `price-view-focused-green.log` |
| `python tests/async/test_reconcile_economy_accounting.py -v` | PASS, 97 tests; three explicit Linux/disposable integration skips; 12.160 seconds | `price-view-fast-green.log` |
| `DURIS_RUN_STAKE_SQL_INTEGRATION=1 DURIS_RUN_AUDIT_BUDGET=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` in the isolated container | PASS, 97 tests, zero skips; 212.872 seconds; both native modes and both canonical engines through 0056 | `price-view-qualified-green.log` |
| Six price-view CLI budget cases, clean/conflicting, limits 0/1/100 | PASS, 100,000 captured roots; 33,552,384 bytes; 0.504–0.624 seconds; peak 137,764,864 bytes | `PRICE_VIEW_BUDGET` records in the qualified log |
| Six existing mapping-audit budget cases | PASS, unchanged 15,245 creators and input digests; limits 0/1/100 | `AUDIT_BUDGET` records in the qualified log |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py` and `git diff --check` | PASS | `price-view-diff-check.log` and manifest |
| `python tmp/plan5/record-price-view-evidence.py --committed` | PASS exact committed owned/native blobs, frozen sources, unchanged helpers/migrations, image, prior artifacts and logs | `price-view-evidence.json` |

The price budget fixture reaches the per-collection row cap before the byte
cap; valid trailing whitespace exercises the 32 MiB input boundary. It retains
100,000 unique price projections, or 100,001 when a selected/retained conflict
is injected. These are separate synthetic component measurements on this
container. They do not qualify mixed native workloads, p95/p99 writer latency,
growth, restart/checkpoint behavior or the release host.

The manifest hashes each native input, migration, unchanged helper, owned file,
probe artifact and evidence log and binds the result commit/tree. Binaries,
datadirs, captured files and logs are ignored and are not committed. No server
build is repeated because production C/C++ is unchanged; the native fixture
compiles the unchanged production libraries in both modes. Broader minimal-DDL
snapshot, restore, native deletion and populated-upgrade suites are not rerun
for this view-only change.

## Narrow primary-owner handoff and remaining gates

The additive operator output fields are `rows.epoch`,
`coverage.lineage_history_available`, and
`coverage.realized_price_coverage` (the existing three-field object or null).
Invariants are exact-projection deduplication with conflicts retained, numeric
copper bounds, paired capture availability, candidate counts before filtering,
unchanged global coverage/status and deterministic bounded output. Consumers
are the CLI JSON operator reader, existing SQL snapshot reader and Plan 5
retention tools. Tests are the four new direct/CLI regressions, canonical
native/SQL cuts and explicit price budget test. Existing consumers should
accept these additive output fields and use the epoch when interpreting prices.
No snapshot/schema, accounting contract or producer interface is requested.

The coordinator remains the primary owner's file. Register the maintained
test with the existing `DURIS_RUN_STAKE_SQL_INTEGRATION=1` and
`DURIS_RUN_AUDIT_BUDGET=1` Linux gates on the tested combined candidate. Its
publication and combined qualification remain that owner's responsibility.

Plan 5 and R1–R8 remain open: the primary owner's unpublished combined fixes,
native header/authority binding and retained empty-baseline marker handoffs,
writer executable evidence, real supported economic/UID journeys, populated
upgrades, complete flatfile capture, governance/export/retention decisions,
mixed workloads and release-host budgets still require evidence. No inventory,
synthetic fixture or isolated passing suite establishes release completion.

Notebook reconciliation remains pending because `AI_CONTEXT.md`, the notebook
reference and required curator workflow are unavailable. The existing request
has no answer and no curator capability is exposed. This report is an evidence
handoff, not a notebook update. No accounting activation, auto-correction,
production mutation, merge or deployment is performed. Inactive behavior,
wallet-root item exclusions, the declined inactive spell change and the active
blackjack refusal remain preserved.
