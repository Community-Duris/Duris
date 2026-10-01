# Experimental accounting review continuation

Reviewed baseline: `8ef7e9d243d3ccb14477a88d142c17743fba78a0` on
`Community-Duris/Duris:experimental-accounting`, 2026-09-30.
Local continuation: `codex/accounting-review-fixes`.

## Review scope and readiness

Current community master `e1357a30acf258356a5f269ce00496457e3a240a`
is an ancestor of the review baseline. The branch adds 72 commits and changes
1,138 files relative to that master. This continuation reviews the documented
gates and SQL quest recovery budget boundary; it is not an independent audit of
every changed file. GitHub returned no pull request headed by
`Community-Duris:experimental-accounting` at this checkpoint.

The active [completion plan](FINISH_ACCOUNTING_PLAN.md) and executable gates
control readiness. Issues #474, #475, and #476 are closed on GitHub; #490 remains
open. Closed issue metadata does not supersede the branch's failing release
validator. Source integration/review and production activation have distinct
gates. No live access, production repair, deployment, or activation was performed.

## Resolved finding: bounded SQL quest recovery reads

Previously, pending-obligation loading ran one native item/cash witness query
per economic reward slot while reporting only the obligation SELECT. Up to
64 obligations with 64 slots each could hide 4,096 reads. XP entitlement
continuations were also omitted from the load byte metric.

The repository now selects all item witnesses together and all currency
witnesses together, matching each result to its frozen source or derived
operation ID. The read takes three SELECT executions including the obligation
query, independent of slot count. The separate XP entitlement query takes one.
The existing player-load budget includes the two additional witness reads.
Recovery metrics include native witness rows and payload bytes, obligation
metadata/continuations, and XP metadata/continuations. Costs are aggregated even
when validation fails; invalid recovery output is withheld and the load stays
degraded. Existing exact payload, recipient, amount, revision, source, and
duplicate-receipt checks remain enforced.

## Verification

The native fixture uses connection-private temporary tables on fresh,
explicitly disposable loopback databases. It compares reported query counts to
each engine's `Com_select` counter rather than inferring them from source.

| Check | Result |
| --- | --- |
| Native maximum workload, MariaDB 10.11.14 | PASS: 64 obligations, 4,096 slots, three queries, 4,160 rows, 466,984 bytes; 36,628 microseconds in the final local run |
| Native maximum workload, MySQL 8.0.46 | PASS: same queries/rows/bytes; 60,986 microseconds in the final local run |
| Native recovery fault cases, both engines | PASS: missing ledger, duplicate witness, 65th obligation, conflicting XP amount, failed SQL preparation, and retained prior output |
| Actual player-load recovery blocks | PASS: aggregate query/row/byte costs on success and refusal; refuse recovery publication on errors |
| Player-load query admission, load pipeline, durable quest offering | PASS |
| SQL production build, strict maintained warning profile | PASS; binary SHA-256 `e70429ce2311721e4b0d9446ec689c2ece7db168c9d0ae4dd026c22b3686ef61` |
| Flatfile production build, strict maintained warning profile | PASS; binary SHA-256 `2db34bc85c376c03f66714e53cc4c406b1e45827ffee9ea62ddb2d8f57d147e0` |
| Changed-line C++ formatting and `git diff --check` | PASS |
| Normal accounting validator and generated matrix `--check` | PASS: 862 routes, 2,812 lexical occurrences, 2,754 unique sites, zero unmapped sites |
| Accounting validator `--release` | BLOCKED: `writer has no executable evidence` |

The maximum workload asserts the maintained three-second load deadline and
snapshot row/byte limits. These are local component measurements on synthetic
data, not production concurrency or full player-login latency qualification.
The item ledger has no production index on `(reason_type,reason_id)`; the
fixture does not add one. Batched reads bound round trips and returned rows,
not the number of historical rows examined. Large-history query-plan and
latency qualification remain open.
The WSL validation distribution needed task-local linker script shims for its
missing `/lib/x86_64-linux-gnu` alias; no repository warning or linker policy was
relaxed. Docker is unavailable locally. `make test-all`, `make test-db`, the
full-world clone, and integrated live gameplay/restart fault matrix were not
qualified by this continuation. Earlier branch results remain historical.

Reproduce the focused checks:

```sh
python3 tests/async/test_quest_recovery_load_budget.py
python3 tests/async/test_player_load_query_budget.py
python3 tests/async/test_player_load_pipeline.py
python3 tests/async/test_durable_quest_offering.py
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
```

For each disposable SQL engine, supply `DB_HOST=127.0.0.1`, `DB_PORT`, `DB_USER`,
`DB_PASSWD`, a newly created `DB_NAME` beginning `economic_schema_test_`, and
`TEST_DB_DISPOSABLE=1`, then run
`python3 tests/async/test_quest_recovery_read_budget_mysql.py`. The test refuses
other targets and modifies only connection-private temporary tables. It is
excluded from the no-argument generic runner.

## Remaining work

1. Qualify integrated quest, spell, save/custody, disconnect, database-fault,
   copyover, and cold-restart paths on a reproducible candidate. Component query
   bounds do not establish concurrent publication/save correctness.
2. Finish executable native evidence for every supported writer and backend,
   independent monetary/origin/UID reconciliation, lifecycle qualification,
   and guarded activation. Retain the failing release gate until these pass.
3. Complete flatfile gameplay/recovery parity beyond this build compatibility
   check. A compiled backend is not a qualified economic journey.
4. Keep production backup, ownership repair, witnessed opening, and cutover
   under their separately authorized operational procedure.

Sources: [branch checkpoint](EXPERIMENTAL_REVIEW_CHECKPOINT.md),
[remaining requirements](REMAINING_REQUIREMENTS.md),
[parent issue](https://github.com/Community-Duris/Duris/issues/474),
[#475](https://github.com/Community-Duris/Duris/issues/475),
[#476](https://github.com/Community-Duris/Duris/issues/476), and
[#490](https://github.com/Community-Duris/Duris/issues/490).
