# Plan 5: mapping-creation audit work and near-limit budget evidence

Delivery branch: `codex/accounting-plan5`, separately published from
`experimental-accounting`. Worktree:
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base: `c3a948594336653c9ef7a693a24308b1ec04994d`. The result is the commit
containing this report; its exact SHA is recorded after commit in
`tmp/plan5/mapping-audit-evidence.json`.

Canonical remote refreshed before this slice:
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source remains tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, all 1,232 tracked inputs,
with canonical migration 0056. Primary-owner unpublished integration is not
qualified by this delivery branch.

## Established scaling defect and independent fix

The mapping-creation audit previously iterated the entire selected effects
dictionary once for every selected accounting creator root, although each root
needed only its own effects. Valid unchanged synthetic fixtures establish the
quadratic scan: 200/400/800/1,600 roots with one effect per root inspect
40,000/160,000/640,000/2,560,000 effects. Exception counts remain zero.
The driver and RED result are `tmp/plan5/reproduce-mapping-audit-scan.py` and
`mapping-audit-scan-red.log`. The permanent deterministic regression also
fails at the base: 1,200 creators inspect 1,444,800 entries, including four
unrelated key shapes, against a linear 2,408-entry bound
(`mapping-audit-unit-red.log`).

The reader now builds a shallow operation/index lookup in one pass, preserving
references to existing effects. Each selected creator normalizes and compares
only its own effects. This preserves field comparisons, duplicate/count
refusals, authoritative selected-root matching, prior-epoch behavior and safe
handling of unrelated key/value shapes. It neither modifies inputs nor imports
mutation code. It introduces no audit-format, producer, storage or schema change.

The regression checks a valid 1,200-root full synthetic cut, then directly
counts dictionary entry inspections for clean and corrupt root copies. Both
scan at most twice the effects count; corruption retains exactly
`invalid_mapping_creation_root`. Unrelated malformed value placeholders are
not normalized, and every input remains unchanged.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_reconcile_economy_accounting.py`
- this report

Shared coordinator, accounting contracts, producers, registry/matrix,
migrations and activation-owner files remain untouched. Accounting stays
inactive. Wallet-root item exclusions, the declined inactive spell change and
active blackjack refusal remain preserved.

## Measured workload and explicit proof boundary

The existing audit contract declares <= 32 MiB input, <= 100,000 rows per
collection, <= 100 detailed outputs, <= 30 seconds and <= 256 MiB peak process
memory for reconciliation. Those limits are unchanged.

The maintained opt-in `AuditBudgetTests` now builds a bounded synthetic
mapping-creation workload near the byte ceiling. It contains 15,245 independent
reason-42 wallet creators, one zero-balance effect and native holding per root,
0-to-1 native revisions, revision-zero creation openings, distinct operation
and source IDs, matching retained creator roots and durable inbox receipts.
It has no item events, children or postings. This is an account-creation audit
shape, not real player/producer admission or complete native capture evidence.

The encoded input is 33,551,932 bytes, 2,500 bytes below the 32 MiB ceiling.
The corrupt counterpart changes only the first retained creator's after-vector;
selected effect/native authority remain original. Clean and corrupt files have
these SHA-256 values, identical across base and indexed measurements:

- Clean: `826eb3b3dd5467a60a9f0daae378eb63cf171e043c5bbb5b750535e8d14b6d65`
- Corrupt: `5519a9ab799c2a3c967ca83f0f02253197ad8afb6f6a626f412da7d27ea3e183`

Each measurement runs the actual CLI in a fresh process at output limits
0/1/100. A fresh wrapper has one child; Linux `RUSAGE_CHILDREN.ru_maxrss` is
converted from KiB to bytes, so a prior compiler's peak cannot contaminate the
CLI memory value. Wall time includes CLI startup, JSON read/decode, complete
audit and output. Clean cuts exit 0; corrupt cuts exit 1 with one precise
exception; detail count is bounded by the limit and input bytes stay identical.
The maintained test also refuses valid JSON padded one byte over the byte
ceiling, at limits 0 and 100, before parsing with exit 2 and unchanged input.

| Same six CLI workloads | Elapsed range | Peak RSS range | Result |
| --- | --- | --- | --- |
| Base reader | 8.667-9.443 seconds | 138,137,600-138,633,216 bytes | Numeric container budget PASS; quadratic scan demonstrated independently |
| Indexed reader, separate measurement | 0.800-0.897 seconds | 138,276,864-138,653,696 bytes | PASS, identical counts/files |
| Indexed reader, final maintained suite | 0.747-0.925 seconds | 137,609,216-138,600,448 bytes | PASS, 90 tests total with zero skips |

The base already passes the numeric budget on this container. The solved
defect is unnecessary quadratic work; it is not presented as a demonstrated
30-second breach. The roughly tenfold observed improvement provides headroom
for this measured shape. These are container component measurements, not
release-host, mixed historical workload, query-plan, storage-growth or commit
latency qualification. Other 32 MiB shapes and the integrated host remain gates.

## Native and disposable SQL checks

The existing native fixture is unchanged except for its new artifact directory
`bin/tests/plan5-mapping-audit-sql`, which preserves all earlier binaries/output.
It compiles eight production sources in SQL and flatfile modes with C++20,
strict warnings as errors, ASan/UBSan and no PIE. Native EAI1/EAP1 bytes, all
104 source grammar decisions, 1,107 source-policy decisions and six original
link decisions retain their independent agreement. This is existing metadata
and reader-regression proof, not a native CPU workload benchmark.

Both canonical engines use fresh private datadirs/Unix sockets, TCP disabled,
bootstrap adoption and all migrations through `0056_spell_ward_durability`.
The SELECT-only reader is denied UPDATE (1142), FK/CHECK constraints remain
enabled, and all 82 canonical cuts retain exact exception counts, rollback/
cursor-close checks and unchanged seven-table authority. These checks preserve
existing stake/evidence behavior; they do not produce mapping gameplay or
qualify active gambling.

The broader existing guarded SQL runner passes both native engines using its
explicit minimal fixture DDL. Its native mapping creation/selected-root
comparisons, corruption/restoration, source, orphan, UID provenance, operator
and interrupted-cut checks retain their exact expectations. All 21 compared
tables remain unchanged across reads. Its original final count describes the
54 grammar faults; separate lines retain 132 kind-policy and four original
self-link faults. The private loopback driver creates only new isolated
instances inside an unexposed container and strips checkout credentials.

## Commands, pinned inputs and evidence

All Linux jobs use `duris-plan5-origin-sql-tools:local`, immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It provides Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL package
1.0.2-2ubuntu1.1, MySQL 8.0.46-0ubuntu0.22.04.4 and
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1.

Common invocation: `docker run --rm --mount
type=bind,source=<worktree>,target=/workspace,readonly --workdir /workspace
<image> <command>`. Native/artifact jobs also mount `<worktree>/bin` read-write
at `/workspace/bin`. Qualification gates are passed with Docker `--env`.
No ports are published or capabilities added. Only ignored synthetic fixture
and compiled artifact paths are written; no credentials/real player data are
captured or committed.

| Command inside container | Result | Evidence |
| --- | --- | --- |
| `DURIS_RUN_AUDIT_BUDGET=1 DURIS_RUN_STAKE_SQL_INTEGRATION=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` | 90 PASS, zero skips, 201.980 seconds; six budget CLI cases, oversized-input refusals, both native modes and canonical engines through 0056, 82 read-only cuts | `tmp/plan5/mapping-audit-qualified-green.log` |
| `python3 -u tmp/plan5/qualify-source-event-snapshot.py` | PASS both engines; retained mapping/root, corruption, provenance, source and operator checks | `tmp/plan5/mapping-audit-snapshot-green.log` |
| `python3 -u tmp/plan5/measure-mapping-audit-budget.py base` | Six base CLI cases PASS; baseline work issue established separately | `tmp/plan5/mapping-audit-budget-base.log` |
| `python3 -u tmp/plan5/measure-mapping-audit-budget.py green` | Six indexed CLI cases PASS, identical encoded inputs/counts | `tmp/plan5/mapping-audit-budget-green.log` |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py` | PASS on host | evidence manifest |
| `git diff --check` | PASS | evidence manifest |

The first fast suite collected 90 tests: 88 PASS, two explicitly gated
native/SQL and budget skips, 3.452 seconds (`mapping-audit-unit-first.log`).
The final enabled run supersedes those skips. `mapping-audit-frozen-inputs.json`
pins both executable changes before final tests. The post-commit recorder
verifies raw committed file/native source identity, prior artifact preservation,
all measured results and logs, then records result SHA, image, toolchain,
commands, migrations, drivers and artifact SHA-256 values in
`mapping-audit-evidence.json`. Existing input and output limits remain fixed.

## Shared registration request and remaining gates

No interface/schema request is introduced by this fix. The primary coordinator
owner should register
`AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance`
with `DURIS_RUN_AUDIT_BUDGET=1` on Linux, plus the existing native SQL gate
`DURIS_RUN_STAKE_SQL_INTEGRATION=1` and broader guarded snapshot runner. The
maintained budget case always labels its workload synthetic component evidence;
the combined release report must separately pin the release host and real
supported-route workloads. This lane does not edit central registration.

The previous durable-header and retained-baseline-marker handoffs remain open.
Complete audit capture, metadata/actor/writer authority, out-of-scope original
history, flatfile parity, populated upgrades, retention/erasure/export governance
and native journeys, mixed real roots, storage growth, commit/publication latency
and checkpoint/restart budgets remain required. Isolated budget/component passes
do not qualify the primary's unpublished combined candidate or complete release.

Full backup/restore, lifecycle protocol suites, auxiliary origin/invariant suites
and maintained server builds were not rerun for this Python-only lookup fix.
Their source inputs remain unchanged; the relevant audit and actual database
checks run here. Prior source-specific evidence retains its original scope.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable; the existing input request is unanswered. This report is an
evidence handoff, not a notebook update. Notebook maintenance remains a specific
external-input dependency. No activation, production data access/mutation,
automatic correction, merge or deployment occurred. Only the delivery branch
is published by this lane.
