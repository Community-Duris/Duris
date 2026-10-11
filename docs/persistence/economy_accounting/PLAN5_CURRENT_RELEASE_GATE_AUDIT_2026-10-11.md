# Plan 5 current release-gate audit — 2026-10-11

The original contract validator passes on frozen current primary
`9c871c875b7d794434e0f636515e3fb7caf44f78`; its original `--release` command
refuses with **writer has no executable evidence**. All current lexical sites
are mapped, but current declarations and executable evidence do not meet the
release contract. This is independent source-contract evidence, not native
route, backend, gameplay, restore or release qualification.

The original refusal is retained with exact primary-owned evidence fields for
the acceptance queue. Domain implementation and shared metadata keep their
existing owners; full Plan5 completion remains unproven.

## Branch, exact source and evidence

- Branch/remote destination: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Producer base: `02898093fa20cce30d48f057bce684dab7d9a98c`.
- Frozen primary: `9c871c875b7d794434e0f636515e3fb7caf44f78`, tree
  `7b255a9ef47472934a4f481de13f65d33aee257c`.
- Owned maintained changes: this report and additive remote follow-up.
  No shared writer registry/matrix, coordinator, runtime, migration, contract,
  test registration or activation change. No new interface/schema field proposed.
- Result/verified remote commit is bound after publication in
  `delivery/publication.json` under the evidence root below.

Evidence root:
`D:/Dev/Tests/Duris/accounting-plan5/Plan5-current-release-check-9c871c875-20261011`.
The snapshot contains complete tracked `src`, `scripts`, `tests` and accounting
documentation directories, including every referenced writer and test-candidate
path: **3,709 regular files**. Each extracted file is verified against its exact
Git blob. Raw source, source inventory and archive are retained; all input bytes
and the owned worktree remain unchanged after execution.

| Artifact | SHA256 |
| --- | --- |
| `source-inventory.json` | `0822f12383dc35a0c1def1373c045471995afeb617adc0681bf7ebccd2cb6cd4` |
| `source.tar` | `73b64e60b7d6c609aeec2d482971a11c8f7f592852b65d480b362a8467175128` |
| `result.json` | `7c4559b504a40e52931f2f56bc6f795faabfc9069da91bccaf2c5276114c79f9` |
| `source-check-seal.json` | `24235710ad9e000ae45729ca095f3985d7491a4adf39dd14881bdd9a63d47a66` |

The verified seal covers **3,724 files / 141,703,264 bytes**, including the source,
original outputs, corrected terminal receipts and preserved collector failure.
Later publication/curator metadata lives separately in `delivery`.

## Original commands and actual results

Windows Python **3.12.10**, MSC v.1943, 64-bit AMD64. No database or native
backend is executed. Both original validator calls are unmodified:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Tests/Duris/accounting-plan5/Plan5-current-release-check-9c871c875-20261011/source/scripts/validate_economy_accounting.py --root D:/Dev/Tests/Duris/accounting-plan5/Plan5-current-release-check-9c871c875-20261011/source
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Tests/Duris/accounting-plan5/Plan5-current-release-check-9c871c875-20261011/source/scripts/validate_economy_accounting.py --root D:/Dev/Tests/Duris/accounting-plan5/Plan5-current-release-check-9c871c875-20261011/source --release
```

| Check | Actual terminal result |
| --- | --- |
| Contract validation | Exit **0**, 6.547 seconds; 14 fixtures, 931 routes, 2,911 candidate occurrences; no drift or contract error |
| Release validation | Exit **1**, 0.093 seconds; empty stdout, exact diagnostic `economy accounting contract error: writer has no executable evidence` |
| Final collector session | `33679`, terminal exit **0** |
| Input/worktree preservation | All 3,709 inputs unchanged; branch/base/worktree unchanged before report edits |

The first collector, actual session `41260`, ran both original commands and then
raised `KeyError: writers` because the matrix's actual key is `routes`.
That helper and both original output pairs are preserved in `collector-failure`
and the original log files. The first collector did not retain subcommand exit
codes before its exception; those are not inferred from its output. The corrected
collector repeats only these two unchanged source checks once, immediately saves
each terminal receipt, and uses `routes` for the subsequent metadata observations.
No validator logic, input, assertion, native test or release rule changes.

Original collector SHA
`5aceb6085dbf2fbdfde5c2a3ad28300d62bb30e6ad0ace77f17ab17b3e9419d9`;
corrected collector SHA
`ce90ee2d51d1c1f1fc0929e6db217e2ca41b24c6944d00c03abb14e079282cc4`.
Helpers remain under `D:/Dev/Temp/accounting-plan5-release-check-20261011`;
scratch uses its separate D: directory. No container, heavy lease, native build,
database, game, production access or mutation is started. No cleanup is pending.

## What the current declarations establish

- 2,911 lexical occurrences / **2,853 unique sites**, all mapped; zero unmapped
  lexical sites. Mapping does not establish reviewed reachability or real writer
  qualification.
- Registry status **draft**; `writers.census_complete=false`;
  `writer_coverage_matrix.coverage_complete=false`.
- Coverage declarations: 918 legacy, 6 observed, 4 enforced, 2 projection,
  1 unsupported.
- Dispositions: 699 runtime mutation, 92 runtime projection, 4 offline operational,
  19 dormant candidates and 117 nonwriter candidates.
- Of the 795 rows excluding dormant/nonwriter candidates, **772** lack executable
  evidence, **788** lack a release-eligible coverage state, and **791** lack
  qualifying evidence/status across all three backends under the validator's
  existing rules. These are metadata observations, not 791 demonstrated runtime
  failures and not permission to relabel routes.

## Narrow primary handoff

The actual command's diagnostic does not name a route. Source-order examination
of the original release rules and declarations identifies the first missing
evidence row as `account.item_reward`, **primary / #481**:

- Consumer: `scripts/validate_economy_accounting.py:validate_inventory` and its
  generated route in `writer_coverage_matrix.json`.
- Source: `src/account/account_reward.c`, symbol `summon_one`.
- Existing fields: `reason=item_create`, `coverage=legacy`, `evidence=[]`;
  each of `backends.mysql`, `backends.mariadb`, `backends.flatfile` has
  `status=unverified` and `evidence=[]`.
- Existing invariant: the accepted reward source event, selected backend,
  expected native revisions and receipt precede publication; one admitted UID
  has exact source attribution and replay identity. Its current support text
  explicitly leaves backend and playable qualification unverified.
- Existing test candidates: `tests/async/test_account_bound_reward_multi_contract.py`
  and `tests/async/test_pa_item_admission.py`. Their existence is validated here;
  they are not executed or promoted to actual same-root/backend/gameplay proof.

Primary should supply current same-root executable and applicable player/replay/
refusal proof before updating that row's existing `evidence`, `coverage` and
per-backend `status/evidence`, and its generated matrix projection. Runtime
mutation and evidence-policy decisions remain primary-owned. No new field,
schema, independent writer classification, test registration or activation is
requested; the 772/791 row lists remain available in the exact result artifact
for the finite primary acceptance queue.

## Full-scope completion audit

The original numbered Plan5 work and R6–R8 remain the scope, including both
backends and deliberately corrupted native/disposable evidence. They are not
redefined around this check or the earlier isolated component passes.

| Requirement | Current authoritative conclusion |
| --- | --- |
| Work 1 / R6: complete real-writer classification and executable support/refusal | Incomplete: original release validator refuses; declared registry/census/matrix completion flags are false. Lexical mapping alone cannot close this requirement. |
| Work 2 / R7: independent reconstruction of every admitted holding and UID; specific corrupt-evidence exceptions | Existing reader/component evidence keeps its recorded scope. No current combined native authority, all-holding/all-UID or real-producer proof is established by this turn. Remains OPEN. |
| Work 3 / R7: bounded operator views, alias restrictions, preserved non-personal identity and guarded corrections | Earlier bounded reader/export components remain preserved. Complete current operator/authority and approved correction/retention policy evidence is not established here; domain correction remains primary-owned. Remains OPEN. |
| Work 4 / R8: fresh/upgrade/runtime, backup/restore, retention, deletion/erasure and export continuity | Current retention/restore/stake entry points retain older schema61/62 expectations while default manifest is65. Full original schema65 matrix and coherent current-schema journeys remain prerequisites. Canonical destructive erasure remains disabled. Remains OPEN. |
| Work 5 / R8: operation/latency/storage/checkpoint/reconciliation budgets, faults/restart/replay and real gameplay on both backends | ACC-05 audit-budget proof is a synthetic component; ACC-06 is a zero-effect link/codec prerequisite on055+3. New9c changes an ACC-06 native input. Neither establishes current combined workload, gameplay, recovery or release qualification. Remains OPEN. |
| Independent acceptance: deliberately corrupted disposable evidence, zero unclassified real writers, SQL fresh/upgrade/replay on both engines, flat interruption/restore/retention/dedupe and both builds/gameplay/fault matrix | No whole-current-candidate evidence closes these requirements. Original matrix failure and focused schema65 diagnostics retain their separate scopes. Remains OPEN. |
| Release report / activation boundary | This report records exact source, commands, actual results, source coverage and limits. Full release remains BLOCKED; separately authorized activation is not performed. |

This is additive curator input for the primary-local notebook and coordinator
acceptance queue. Actual primary/notebook adoption is not claimed. Accounting
remains inactive/CLOSED, wallet-root item exclusions and declined inactive spell
behavior are preserved, and the full Plan5 goal remains ACTIVE. No production
mutation, deployment, PR merge, activation, direct experimental-accounting push
or audit autocorrection occurs.
