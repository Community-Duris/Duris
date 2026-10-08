# Plan 5 canonical 64 native and managed claim qualification — 2026-10-07

Two owned fixture defects prevented current-source qualification. The coin
fixture omitted three real providers now used by item transfer; both original
backend recipes fail to link. Once those providers are present, the full native
SQL fixture fails on both engines because it still requires and reports migration
0062. Separate commits repair the recipe and the obsolete head assertion/labels.
Both complete managed SQL methods and the full native coin/SQL entry point then
pass on the exact published native source and canonical 0064 schema.

These checks qualify modeled retained claim history, its independent readers,
managed preservation and corrupt-import refusal. They do not authenticate an
original accounting opening, actual financial producer, complete current native
capture, lifecycle installation, activation or full release.

## Branch, ownership and separate solved issues

Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Local and sole remote publication branch: `codex/accounting-plan5`.
All earlier branch tips and existing work remain preserved; no branch switch.

| Slice | Base commit | Result commit | Owned implementation |
| --- | --- | --- | --- |
| Real coin-fixture providers | `c373f04fac0ef14f734bc086c1cef01246dc70fe` | `03d9cea1a2d8de31a6dc87e75824995d4ad60256` | `tests/async/test_restore_economic_coin_effects.py` |
| Current canonical-head assertion/evidence | `03d9cea1a2d8de31a6dc87e75824995d4ad60256` | `2b111d818bbc41bbbba785e4be1daa0228edea5f` | `tests/async/run_restore_accounting_evidence_mysql.py` |

The recipe retains all 12 original sources in order, adding exactly once:
`src/item/lockpick_retirement_continuation.c`,
`src/economy/native_quest_cost.c`, and
`src/economy/native_quest_coin_give.c`. Removing those three AST nodes restores
the complete original helper/test AST, including flags, ASan/UBSan settings,
assertions and limits. No stubs, linker suppression or test filtering is added.

The second slice changes exactly four existing literals from
`0062_economic_pending_claim_consumption` to `0064_auction_custody_history`:
the head assertion and the pending-claim/canonical/coin evidence labels. Its
complete AST is otherwise unchanged. Fresh adoption, migration application,
all original corruption controls, SELECT-only enforcement and final authority
comparisons remain active.

The containing documentation commit adds this report and the remote follow-up;
its full SHA and matching remote are recorded by the post-commit delivery.
No native source, migration, accounting contract, producer, registry/matrix,
activation or maintained shared builder is independently edited.

These two source fixes target the published primary native/canonical source.
The delivery branch retains its historical native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` (0062/229 tables). It lacks the
three incoming native providers. This is an explicit primary-source handoff,
not a passing native claim for the delivery branch's older source. No conditional
provider omission or compatibility fallback hides that prerequisite.

## Exact tested source compositions

All runs freeze primary `e9e4da5a14e106ddc4e8d1b78749f7d9042c5653`, native tree
`833d3085815b396861ad18a77635412212381e4b`, migration tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, canonical
`0064_auction_custody_history` and 230-table runtime manifest. Alternate Git
indices create immutable full archives without changing the checkout branch.

| Composition | Exact tree | Archive SHA256 |
| --- | --- | --- |
| Original coin recipe, owned managed methods and strict tombstone fix | `e3cddd4fb160f6e61e9a5138a69c4153d6ee9557` | `fe9efdda75ff79ba202041cc652436f733f0e98484a8e21ee424d2bcd355672e` |
| Repaired coin recipe, unchanged old SQL head assertion | `c5bc4362b83d6af130fd0a07fd9cd84f617e88bf` | `dca6ad59c5cdde3a37c6e78e595a868cf2c8bf14adfee9e413f88b898925c752` |
| Both fixture repairs | `eddb831506c06b001744b20858b5f01bbea7284b` | `f72d2792bb50a87de1e1cffe1d4530af4c5d112683b6fd2a6f5bd147b83815cb` |

The last two compositions differ solely in the four-literal SQL fixture repair.
The final source overlays exactly four owned Python files on the primary:

- `test_persistence_backup_integration.py`, blob
  `d1ef31c19e79228e994e5c764a8553875db0f86e`: previously delivered nonempty
  claims and pending-source lifecycle fixture fix, with no new edit this turn.
- `scripts/persistence_restore.py`, blob
  `23b17a3d158ac455105de1cd1303561dcc028ad8`: the published strict tombstone fix.
- `test_restore_economic_coin_effects.py`, blob
  `f46125cca129f6e94e9f8dbf876d9e75f353bd57`: the three real provider additions.
- `run_restore_accounting_evidence_mysql.py`, blob
  `ff6293257cc7c7f27b9d732be8f6a88fed0473ab`: the four head literals.

Every archive authenticates all 6,462 regular bodies and four links against raw
Git identity. Terminal native guards check all bodies, canonical tar modes and
link targets. No generated artifact or source transport conversion is accepted.

## Native/disposable commands and observed results

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/claim64-20261007/`.
New helpers: `D:/Dev/Temp/accounting-plan5-claim64/`.
Separate direct D: binary mounts:
`D:/Dev/Builds/Duris/accounting-plan5-claim64-20261007/<stage>/bin`.
Existing immutable image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Actual toolchain is GCC 13.3.0/Python 3.12.3; database logs identify
MariaDB 10.11.14 and MySQL 8.0.46. Native fixture caching is off.

Each isolated container has no network or host ports. Databases have private
fresh datadirs and Unix sockets, with TCP disabled. The managed runs retain the
original 512 MiB candidate mounts and original namespace capabilities. Source
and temporary work use native RAM filesystems. The unchanged full native test
requires `TemporaryDirectory(dir="/")`; that container alone permits root
scratch on Docker's existing D: storage. No storage relocation or service
start/stop/reconfiguration occurs.

Actual host entry points, with full Docker argv retained in each stage:

```powershell
python -X utf8 D:/Dev/Temp/accounting-plan5-claim64/run-prereq.py
python -X utf8 D:/Dev/Temp/accounting-plan5-claim64/run-green.py managed
python -X utf8 D:/Dev/Temp/accounting-plan5-claim64/run-green.py native-full
python -X utf8 D:/Dev/Temp/accounting-plan5-claim64/run-green.py native-full-02
python -X utf8 D:/Dev/Temp/accounting-plan5-claim64/run-green.py managed-full-02
```

The observers use the standard unittest loader/runner and preserve complete
methods. `managed` selects the original full MariaDB and MySQL
`PersistenceRecoveryIntegration` methods; `native-full-02` selects the entire
`RestoreCoinEffectsTests` class. `managed-full-02` loads the complete
`test_persistence_backup_integration` module, including its lifecycle class.
Observed native/client/qualifier commands, full argv, logs and terminal receipts are
retained. Observation never changes executable deadlines or restarts a live job.

The original coin recipes fail in both modes, exit 1, after 75.907379 seconds.
The repaired recipes preserve the complete native corpus: both modes emit
identical bytes, all 32 coin cases (15 accepted) and all 3,026 decoder cases
(1,054 accepted) agree with independent Python readers under ASan/UBSan.
Five native claim roots have SHA256
`2f0c5dc917aef019c60befbd592c9d0e0a1bf6b0c6bbef828996b9ad077bd635`.
Both old-head full native engine subcases then fail at the 0062 assertion;
that whole test exits 1 with two failures and zero skips, 169.313380 seconds.

The corrected complete native entry point passes: one full method, zero
failures/errors/skips, 337.532591 seconds including guard/collection work.
Each fresh canonical 0064 engine executes 109 canonical cuts/90 expected
refusals/58 full entries, 25 pending-claim cuts/seven valid controls, original
258-source/257-consumption pagination, 30 audited coin cases/two constraint
refusals, SELECT-only denial and unchanged authority. No original case is removed.

Both complete managed SQL methods also pass, zero failures/errors/skips,
306.600015 seconds including collection. Each restores the three-root retained
book with two original credits, one partial allocation and six residual copper;
the book is equal before import, after import, after actual server boot and after
each refusal. Fourteen raw SELECT-only cuts (seven per engine) match exactly
within each engine. Two actual server boots, two qualified restores and six
corrupted-import refusals reach their intended checks. All original
wallet/bank/epic/schema-history/receipt/isolation assertions remain present.
Forty-six database qualification calls give 24 acceptances and 22 expected
refusals. All ten daemon handles and every tracked initialization/version process
are terminal. Runtime namespaces remain isolated and accounting inactive.

Managed boots reuse the exact production binaries previously clean-built from
published `62d030746265067638155314b3165bb135cb2c80`: SQL SHA256
`1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b`,
flatfile `9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be`.
All 1,318/1,320 recorded repository inputs and 28/20 external header inputs
match again inside the same pinned image; both native trees and binary bodies
are exact. Their prior 754-unit fresh Make commands/results remain bound in
`production-build-binding*.json`; this turn does not claim new Make builds.
The Python recipe/head repairs change no C++ body.

The final complete managed module also passes on the same final composition as
the corrected native SQL method: all 12 original methods, zero failures/errors/
skips, 832.929 seconds of unittest execution and 844.161866 seconds including
collection/guards. Its 35 restore attempts produce nine qualified restores;
all ten observed service-loader calls pass, plus the lifecycle case's second
direct cold boot. All original WAL/quarantine, first snapshot, missing economic
records, pending replay, interrupted bank/legacy transactions, foreign-owned
checkout, locker/spell receipts, both full SQL methods and corrupt lazy-catalog
checks remain. The lifecycle case preserves old receipts, drains pending native
journals, advances UID/revision from (1000203,2) to (2000203,3), refuses its
original corruption cuts and retains two generations while pruning the old one.
Fourteen equal claim cuts, 46 database qualifications (24 accepted/22 expected
refusals), ten stopped daemons and all tracked initialization/version processes
are verified again. Native C++ fixtures are rebuilt with cache off; six shared
compiler proposals retain the same three real providers and all original flags.

Every job is terminal, with Docker inspections showing no OOM and expected
codes: original recipes 1, old-head native entry 1, two managed methods 0,
corrected native entry 0, complete managed module 0. The source archives preserve
6,103 regular files at 0664 and 359 at 0775, plus four links. Original native
temporary inventories precede host copies; NTFS copy modes are not used as
original native mode evidence. The raw evidence seal and post-publication
delivery receipt bind the retained D: evidence and separate D: build outputs.

## Shared handoff and continuing gates

No public API/schema fields change. Two previous narrow shared recipe requests
remain: append the same three real providers exactly once to
`scripts/build_restore_qualifier.py:SOURCES` and
`tests/async/test_flatfile_accounting_store.py:SOURCES`; their inherited
restore/authority/lifecycle consumers must retain original providers, flags,
assertions and budgets. This worker applies only compiler-argv proposals in
frozen validation; it does not independently edit those maintained builders.
The primary must apply/review the recipes and run the actual integrated candidate.

Terminal refresh `76195e6d763d999214427f4d1d20924646b0a904` integrates the
complete operator package and reviewed cashout overflow controls, then assigns
its original provider-feasibility investigation to the architecture owner.
Native/migration trees match, but operator/fixture bodies differ from this
frozen qualification. This packet does not qualify that successor. The reserved
investigation and actual shared applications remain with their owners.

Full Plan 5/R7/R8/release remain open: complete independent current physical,
currency and UID census; native player/pet/treasury/history and origin coverage;
original native V2 encoder/installer/recapture; authentic erasure/retention
continuity; applicable upgrade/rerun checks on both engines; genuine financial
producer/player/fault/load journeys and predeclared budgets; and all required
checks on one primary-published combined candidate. Private producer source and
required native interfaces remain unavailable to this worker.

Accounting activation, audit correction, production modification, deployment
and merging remain absent. Inactive behavior, wallet-root money item exclusions
and the declined inactive spell change are preserved. Report/follow-up/seal/
delivery form the curator-ready packet for the nonblocking primary-local
notebook. No application, acknowledgement or cross-chat notification is claimed.
