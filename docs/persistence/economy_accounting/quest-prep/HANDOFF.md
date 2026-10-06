# Quest accounting prep handoff — 2026-10-06

The seven-case independent prep pack is reviewable on
`origin/codex/accounting-quest-prep`. Accounting source:
`a5a1f4b196496d50a6f46afecfec03aba3e66190`; preserved-history merge:
`920a7bb8d7aae5400725fad0c70b505e4d263052`. Original accounting base:
`17c033d69316b21da8598791fc95cae79baa8dc2`; previous tested candidate:
`2c4e17f363ecff0f0d7eb3451ddb229abdd63d9a`; research PR678:
`55905eac1906cf59405764407f9d22497cccfff3`. Schema manifest SHA256:
`1fddd009cc67bba940efba28c2afc3d16d65ea8a476410034cd77c90a0a9aea2`.

Primary already consumed original bundles through
`3e9ce549a0a14c6414242aae231b69da64b10cce`; do not re-import them. New:

1. `6d5ec67a24dd052fd8c9e699c1a89abbf7716d9f`: recovery cut oracle/SELECT reader,
   actual shared QP06 interface check, current frontier/batch instructions.
   Metadata: `aacef7b38dd15c42b2fa5d8939f26e1c069e475d`.
2. `8b02790c5fc50fda76828d6f9b7ae94018b53b30`: supply/full-world variants,
   D/held/original-custody controls and completed batch instructions. Its parent
   is `aacef7b38dd15c42b2fa5d8939f26e1c069e475d`. Import in this order on a
   compatible integrated candidate. Merge920a7 imports upstream history only.

## Usable now

- Five static mini preparations retain exact production bytes/recipe order.
  --reward-vnum and --supply exact/shortage/spares select supplied roots without
  deleting competing recipes; QP01/QP05 wrong-kind variants are available.
  Thirty verified variants cover nine original contracts. These are supplied
  synthetic O goods, distinct from authentic reset/source custody.
- --layout world prepares unchanged production run-roots for all seven cases,
  including dynamic Quietus runtime producers; no stock, task, epoch, binding,
  UID or receipt is inserted. QP05 production huge-skin cap remains one.
- Executable native-row expectations cover exact input retirement/reward UID,
  spares, original incarnation, fees/C3000/frozen XP, receipt/source/reference/
  posting links, definite refusal, replay, linked refund, historical ACK presence,
  ordinary later room move, original D stock/cash and replacement independence,
  and stable accepted held obligation. Fourteen corruption-control unit tests
  pass; their modeled cuts provide no native authority.
- SELECT-only reader guards disposable loopback schema and explicit actual
  integrated source/binary pins, bounds rows/BLOBs and captures actual native
  tables in one read-only snapshot. Actual SQL execution is pending the batch.

## Current acceptance frontier

| Case | Current executed result | Remaining primary qualification |
|---|---|---|
| QP01 P0 exact sapphire/consumed Orb | Source/variants/current selector PASS | Authentic birth/source and four exact retirements/fresh reward/recovery |
| QP02 P0 overlapping hides/fees | Observation PASS; acceptance RED component30 | Actual first paid branch blocks backpack; native coupled fee owner |
| QP03 P0 Auriam D/reset | Replacement-generation predicate PASS; D oracle PASS on counterfactual controls | native_publish currently refuses successful disappear=true; whole original stock/cash lifetime, absent-body adoption/historical ACK/pair |
| QP04 P0 dynamic paid failure | Observation PASS; acceptance RED component30 | Active generic refund refuses injected callback; genuine debit/linked refund or visible held liability |
| QP05 P1 repeated skins | Source/sufficient/shortage/wrong-kind/selector PASS | Actual supplied custody/source; ordinary cap-one huge availability remains limited |
| QP06 P1 Kord mixed/lost ACK | Shared QP06 interface/source/selector/row controls PASS | Actual native route/XP-ACK/cold evidence; original later move and harder move-before-ACK/pair |
| QP07 P1 stale map/abandon | Observation PASS; acceptance RED component31 | Authenticate original A attempt; callback currently mutates replacement B |

The four diagnostics retain one component PASS and three RED; requirements were
not weakened. Current owner/producer files are unchanged from2c4e17. Original
selector/prototype-room seams were superseded by actual prepare_original and
observe_give in the prior refresh; that resolves the generation predicate only.
Shared QP06/--move-reward options close the old driver interface gap. Current
move occurs AFTER recovery; legacy single-ear invocation/fault boundaries cannot
qualify active native three-item handover or pair retirement.

## Exact checks and remaining shared requests

From repository root under Linux/WSL:

```bash
python3 tests/async/quest_accounting_prep/test_production_terms.py --variants
python3 tests/async/quest_accounting_prep/test_native_selectors.py
python3 tests/async/quest_accounting_prep/test_recipient_retirement.py --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py
python3 tests/async/quest_accounting_prep/test_quest_cut_checks.py
python3 tests/async/test_native_quest_recovery_context.py
```

All pass within their stated source/fixture/component scopes. Native context
uses actual providers with ASan/UBSan; retained ignored local artifacts:
`bin/tests/native-quest-recovery-context-0vb_ww_n/`. Exact failures/commands and
artifact source/binary hashes: [RESULTS.md](RESULTS.md). No server, SQL, fault,
activation or native cold journey executed by prep.

[RECOVERY_BATCH.md](RECOVERY_BATCH.md) supplies all fixture/capture/oracle and
maintained journey commands, cwd/source/binary/schema/env prerequisites, genuine
producer/reset anchors, expected effects and external evidence locations. Its
shared request table names exact files/functions, current/required behavior,
original requirements, minimal interface and reproducer: maintained runner
case/native/capture controls before cleanup; backpack selection; genuine paid
refund/attempt ownership; D whole-stock/cash; exact historical original ACK
and genuine paired return. Those capabilities require the primary's actual
integrated candidate, not prep stubs. Fresh SQL activation and Plan5 release/
audit/restore ownership remain with their existing owners.

## Exact owned diff

New executable files: `quest_cut_checks.py`, `capture_quest_cut.py`,
`test_quest_cut_checks.py`, all under `tests/async/quest_accounting_prep/`.
Updated executables: `case_data.py`, `prepare_fixture.py`,
`test_production_terms.py`. Existing diagnostic scripts are retained unchanged.
Owned docs: this file, GAPS.md, CASES.md, RESULTS.md, SOURCE_FACTS.json and new
RECOVERY_BATCH.md, all under `docs/persistence/economy_accounting/quest-prep/`.
No authored file lies outside those two paths.

Every identified independently completable fixture/assertion is implemented and
small checks executed; remaining genuine shared capabilities are specified.
The primary can import the new owned bundles and qualify at its agreed major
batch boundary. This handoff claims completion of independent preparation only,
with integrated native gameplay/persistence/activation/release still pending.

Second bundle's exact owned files: docs CASES.md, GAPS.md, HANDOFF.md,
RECOVERY_BATCH.md, RESULTS.md in the owned docs directory; executable
prepare_fixture.py, capture_quest_cut.py, quest_cut_checks.py,
test_production_terms.py, test_quest_cut_checks.py in the owned tests directory.
First bundle additionally updates SOURCE_FACTS.json and case_data.py. The final
handoff-only commit records these SHAs; published branch tip is independently
resolvable after fetch. No shared-file patch is included.
