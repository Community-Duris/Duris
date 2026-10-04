# Plan 5: retained ordinary stake accounts in independent reconciliation

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `75bc0c4d33d367cc1060775c67b4f076194a1e17`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/stake-account-evidence.json` record its exact SHA after commit.
Refreshed canonical remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`.
Native source tree: `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, including
canonical migration 0056. Primary-owner unpublished fixes are outside this
source qualification. Only this delivery branch is published by this lane.

## Established defect and complete classification fix

The current native enum and reviewed registry retain account kind 11,
`gambling_stake`, as an ordinary finite holding keyed by table UID and round
sequence. A strict ASan/UBSan probe compiled against six production sources
in both SQL and flatfile modes emits its valid 40-byte key with UINT64_MAX
identity/context. Native encode/decode and ordinary classification succeed.
At the slice base, the independent Python decoder rejects this key and its
ordinary-kind set excludes it. Both failures are retained in
`tmp/plan5/native-stake-key-red.log`; the reproduction driver is
`tmp/plan5/reproduce-native-stake-key.py`.

The reader now accepts all current native kinds 1 through 11 and applies its
ordinary opening, revision/history, effect/posting, nonnegative balance,
native disagreement and terminal-zero checks to kind 11. Persistent mapping
creation and retirement continue to allow only kinds 1 through 6. A retired
finite stake no longer requires a persistent mapping-retirement row; it still
requires the existing committed retirement reference, zero terminal balance
and absence of a retired native holding. This fixes the additional false
mapping obligation exposed by the terminal-stake regression.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_reconcile_economy_accounting.py`, and this report. No product
interface, schema, coordinator, accounting contract, producer, writer registry/
matrix or activation-owner file changes. The active blackjack refusal and the
declined inactive spell-path change remain preserved. Reading retained structural
stake evidence does not enable a gambling writer.

## Regression and native/SQL proof

Five new focused methods cover every native kind, unsigned identity/context
boundaries, unknown kinds, held and terminal stake balances, unauthorized
counterparties, missing openings/native holdings, stale balances, broken
revisions, posting disagreement, negative holdings, nonzero retirement and a
retired native row. Creation/retirement mapping fixtures still refuse kind 11.
Bounded exact-key operator views at limits 0/1/100 omit aliases and preserve
global refusal for an incomplete snapshot; the CLI leaves its input unchanged.

The existing test file also retains an explicitly enabled native SQL case.
It compiles eight production sources plus a generated fixture under strict
warnings and ASan/UBSan in SQL and flatfile modes. Both modes produce identical
native EAI1 intents and EAP1 plans for a held stake and its return. Native plan
decode succeeds and the EAP1 intent digest matches the exact EAI1 bytes.

Each database starts with a fresh private datadir/Unix socket, TCP disabled,
reviewed bootstrap adoption and pending canonical migrations through
`0056_spell_ward_durability`. A separate fixture owner inserts native intent/
plan bytes, metadata, effects, postings, source claims and fixture receipts.
The independent SQL exporter reads those rows with a SELECT-only account in
a consistent read-only transaction. The separate reconciler consumes that
cut with known fixture opening/holding anchors. A private UPDATE attempt is
denied with SQL error 1142 on each engine.

There are four reads per engine: held stake, terminal stake, a corrupted nonzero
terminal effect, and a restored known fixture. Every read ends with exactly one
rollback and cursor close; statements are SELECT or transaction configuration/
start. All seven compared SQL tables retain identical rows before/after each
read. Each corrupted terminal effect produces `account_effect_posting_mismatch`
and `retired_nonzero_holding`. The fixture owner restores known test data;
neither audit reader makes a correction. SQL active_epoch remains null.

The EAI1 structural fixture uses fixed nonzero command-binding/domain digests,
and inbox command/keys hashes are test constants. Opening and native holding
anchors are supplied by the fixture. This does not authenticate a gameplay
command, source entitlement, durable table/round store, publication or replay.
It does not execute a native SQL mutation/lifecycle owner, obtain a live round,
or qualify the full SQL snapshot/activation path. Both backends' structural
codec checks and SQL reader checks are component evidence only.

| Exact command | Result | Evidence |
| --- | --- | --- |
| `DURIS_RUN_STAKE_SQL_INTEGRATION=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` | 75 PASS, zero skips, 182.097 seconds; native SQL/flatfile modes, MySQL 8.0.46-0ubuntu0.22.04.4 and MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 through canonical 0056 | `tmp/plan5/stake-account-qualified-green.log` |
| `python3 -u tests/async/test_economic_accounting_types.py` | PASS under ASan/UBSan: golden coin/item effects, 5,000 transfers, 3,000-item forest, limits/rejections | `tmp/plan5/stake-account-native-types.log` |
| `python3 -u tests/async/test_economic_accounting_plan.py` | PASS in both native modes under ASan/UBSan: reference bytes/digests, bindings, malformed inputs, limits and 14 goldens, including stake/payout/loss/interruption | `tmp/plan5/stake-account-native-plans.log` |
| `python3 tests/async/test_economic_sql_audit_origins.py -v` | 15 PASS; two explicitly gated origin SQL cases SKIP, not rerun for this slice | `tmp/plan5/stake-account-origins.log` |
| `python3 tests/async/test_audit_accounting_invariants.py -v` | 16 PASS, 0.216 seconds | `tmp/plan5/stake-account-invariants.log` |
| `python3 -u tmp/plan5/qualify-native-stake-key.py` | Both verified original native binaries execute successfully; unsigned-max kind 11 is readable/ordinary; kind 11 as an EAB1 opening still refuses | `tmp/plan5/native-stake-key-green.log` |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py` and `git diff --check` | PASS | Exact source hashes below |

The unittest invocations above contain **106 PASS and two explicit skips**.
The new native SQL case itself qualifies both selected engines without skips.
The default fast invocation of the reconciliation file skips its new native SQL
case unless explicitly enabled. Standalone native/probe executions are reported
separately from unittest counts.

The explicitly enabled invocation uses `duris-plan5-origin-sql-tools:local`,
immutable image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Pure and maintained native checks use the unchanged base tools image
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`.
Toolchain: Ubuntu 24.04, Python 3.12.3, GCC 13.3.0 and PyMySQL package
1.0.2-2ubuntu1.1. Each invocation mounts this worktree read-only at `/workspace`;
native jobs also mount this worktree's ignored `bin` writable at `/workspace/bin`.
No container capability is added and no existing DB, .env, game, Redis or
production data is used. Only these new private database processes are stopped.

Equivalent native/SQL wrapper:

```powershell
docker run --rm --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max,target=/workspace,readonly' --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max\bin,target=/workspace/bin' --workdir /workspace --env DURIS_RUN_STAKE_SQL_INTEGRATION=1 duris-plan5-origin-sql-tools:local python3 -u tests/async/test_reconcile_economy_accounting.py -v
```

Exact frozen executable source SHA-256:

| File | SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `eb630c64ce86d97914da427ff998b0da6f845695b0dbae0189dec5f4ab1e6899` |
| `tests/async/test_reconcile_economy_accounting.py` | `b86b7601eee2ad55a340a91201415e6ff70c83ec73e1d0da680b257c7257c9de` |

The final native SQL fixture binaries in both modes have SHA-256
`fc0973cf87d1d3bcfd153aef67cb2a897eb15b5fdf43167290999dd89ced7728`.
Their identical framed EAI1/EAP1 output has SHA-256
`f9a2c41a10e483e0cd241cef797fa2726f541b2aa50033379b1046b8fe58ed7e`.
The original native key probe in both modes has SHA-256
`c23f510086ba6b70c2ad888ee4ead5f104a56eddb46790e7447d5beab65765d8`.
The local evidence manifest pins owned/raw native inputs, helpers, migrations,
generated C++ fixtures, actual binaries/output, image IDs and original/final logs.
Post-commit checks require committed owned/native Git blobs to match those inputs.

The first unit run, `tmp/plan5/stake-account-unit-first.log`, remains a failed
run: it exposed the incorrect mapping obligation for a terminal stake and two
test assertions comparing returned tuples with lists. The corrected standalone
unit run passes 74 cases. An earlier ignored native SQL proof also passed; the
published case retains that proof in the existing test file, and the final
182.097-second run tests its exact committed-source candidate. Earlier logs do
not substitute for the final source run.

## Shared registration and remaining gates

No product interface/schema request is needed. The primary coordinator owner
should register `NativeStakeSQLTests.test_native_stake_sql_both_engines` from the
existing reconciliation file with `DURIS_RUN_STAKE_SQL_INTEGRATION=1`, both
database/server tools, g++, libcrypto and PyMySQL available. Relevant shared
fields are `path`, `arguments`, `environment`, `required_cases`, `provider`,
`engines` and timeout/runtime prerequisites. This one case executes both engines;
the required invariant is a completed pass with both actual versions, canonical
0056 and zero skips. Consumers are the central integration runner and combined
release report. The existing fast regression entry alone does not satisfy that
native/SQL requirement. Shared manifests/runtime packaging remain untouched.

The earlier flatfile/origin SQL registration and retained baseline-initialization
marker requests remain open. All R1-R8 release gates, the primary owner's exact
combined source, populated upgrades, actual gameplay/fault/replay journeys,
source/authority binding, retention/erasure/export and measured workload budgets
remain required. This reader-only slice does not rerun or transfer earlier full
backup/restore or server-build results to a combined candidate. Maintained C++
server sources are unchanged; generated fixture builds are not server builds.
Inventory and synthetic/component passes do not establish release completion.

`AI_CONTEXT.md` and the required curator workflow/notebook reference are still
unavailable. The pending input request is unanswered; this source report does
not substitute for or claim a notebook update. Accounting remains inactive,
wallet-root item exclusions and the declined inactive spell-path change remain
preserved, and only the Plan 5 delivery branch is published by this lane.
