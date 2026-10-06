# Quest accounting prep � continued recovery stream, 2026-10-06

Current source: `a5a1f4b196496d50a6f46afecfec03aba3e66190`; preserved-history
merge: `920a7bb8d7aae5400725fad0c70b505e4d263052`.
Earlier bundles through `3e9ce549a0a14c6414242aae231b69da64b10cce` are already
consumed by primary. The new work below is additional; do not re-import the old pack.
The stream is active: negative/sufficient fixture variants, QP03 D/held recovery
checks and complete native batch instructions remain the next independent work.

## First substantive recovery checkpoint

New owned files: `tests/async/quest_accounting_prep/quest_cut_checks.py`,
`capture_quest_cut.py`, `test_quest_cut_checks.py`.
Changed existing files: `case_data.py`, `test_production_terms.py`; owned source
snapshot and this handoff/results/gaps/cases are refreshed to the new pin.
The exact bundle SHA is recorded by its following metadata commit.

The new oracle asserts exact production kinds/counts, input retirement and new
reward UIDs, spares, mixed fees/C3000, native event/accounting reference links,
committed inbox/source claims, balanced postings, original frozen XP entitlement,
refusal/replay, exact original-debit restitution, historical ACK and a later
room move followed by stable cold recovery. These are captured-state predicates;
canonical native command/context/world/ACK/pair proof remains separate. Missing
evidence fails rather than selecting a successful stub.

The SELECT-only reader captures real native rows in one repeatable-read read-only
transaction, checks InnoDB sources and rolls back before writing a new private
artifact. It creates no database, epoch, task, UID, receipt or binding. Its actual
SQL execution is pending the integrated disposable batch. Counterfactual unit
cuts exercise error detection and supply no authority or native qualification.

Small commands from repository root under Linux/WSL:

```bash
python3 tests/async/quest_accounting_prep/test_quest_cut_checks.py
python3 tests/async/quest_accounting_prep/test_production_terms.py --case QP06
```

The first command passes ten unittest methods with parameterized corruption
controls. The second passes through the actual shared driver's QP06 fixture API,
verifying its ear/scalp/toe/Kord/dagger/C3000 return terms and production bytes.
AST/whitespace checks pass. Collector CLI refuses non-disposable configuration
before touching a server or output. No DB/server/fault/cold execution is claimed.

The frontier is reconciled for all seven cases: native selector/observe_give,
bartender callbacks and selected producer data are unchanged from the last tested
pin. Retained QP02/QP04/QP07 failures still apply to those exact owners; QP03's
resolved generation predicate remains. The shared crash driver now supports
`--quest-case QP06` and `--move-reward`; that previous hook is closed. Its move is
AFTER recovery and its breakpoints still select legacy complete_quest_offering
and quest_reward_recovery_save_acknowledged. Active SQL GIVE needs three genuine
item handovers, its original native child boundary and authentic setup/receipt
cuts; the existing single-ear legacy invocation cannot qualify that route.

See current additions in RESULTS.md and RECOVERY_BATCH.md for complete capture,
check and shared-hook requests. The historical refresh below retains its exact
pins/results and import order.

# Quest accounting prep handoff — refreshed 2026-10-06

The seven-case pack is refreshed for published accounting candidate
`2c4e17f363ecff0f0d7eb3451ddb229abdd63d9a` and remains on
`origin/codex/accounting-quest-prep`. Original base:
`17c033d69316b21da8598791fc95cae79baa8dc2`. Research PR #678:
`55905eac1906cf59405764407f9d22497cccfff3`.

Upstream merge `a28763fcb66a88a286136df1503bd8cc0ed2f72e` preserves original
prep commits `f1a15f1d982bf330d57428b20797beb96aa34c71`,
`cd6e62e331aae0b81b23ed858ec900420e70bbf5` and
`a7c7efb269bdeb3e7d0ef803ac3d2c441fecaf26`.
Refresh fixture/test/documentation bundle:
`2ec64e8b0f04331291d1be0ff049e57a35470ad7` (ten owned files changed).
Its following handoff-only metadata commit records this exact SHA; resolve that
published tip with `git rev-parse origin/codex/accounting-quest-prep` after fetch.
Import the owned refresh bundle on a compatible candidate; the merge imports
upstream history and is not a prep-authored production change. For a primary that
has not yet consumed the original pack, retain original bundle order first.

## Current evidence and priorities

Production AREA/catalog, quest area data, dispatch assignments and bartender
callback inputs are unchanged from the original accounting base. All 23 selected
production input/bartender producer files still agree with the research pin.
Catalog: 2,668 static definitions, fingerprint
`04d687493b02ed27a3b32d070a0a71e9d10c1643406d5fb95fe6b1538b4bb314`.
The candidate installs native birth/quest owners. Static SQL GIVE now reaches
submit_native_quest_give and prepare_original; the old selector and prototype/room
recipient seam have been replaced for that route. Dynamic services retain their
runtime producer and are outside the static catalog.

| Priority/case | Current small-check result | Remaining acceptance |
| --- | --- | --- |
| P0 QP01 exact sapphire kinds + consumed Orb | Source/fixture and native selection slice PASS | Authentic birth/source, four exact retirements and reward/replay |
| P0 QP02 three-hide backpack/paid overlap | Observation PASS; acceptance RED code30 | New owner's paid first branch refuses before availability; current pulse blocks, so supported backpack remains unreachable |
| P0 QP03 disappearing Auriam/reset recipient | Current observe_give acceptance PASS | Replacement-generation predicate resolved at component scope; actual D stock/cash, birth/cold recovery/ACK/pair retirement pending |
| P0 QP04 dynamic failed paid creation/refund | Observation PASS; acceptance RED code30 | Actual unchanged ADD_MONEY refuses injected active refund; genuine active debit/refund/held recovery owner pending |
| P1 QP05 repeated skins/cap-one supply | Source/fixture and native selection slice PASS | Real supplied custody/birth and ordinary huge-skin availability; preserve cap/quantity |
| P1 QP06 Kord item/C3000/XP/lost ACK | Source/fixture and native selection slice PASS | Real mixed reward, later custody change, historical ACK and pair retirement |
| P1 QP07 dynamic stale map/abandon | Observation PASS; acceptance RED code31 | Callback mutates replacement task; exact paid attempt, recovery/history/quota ownership pending |

Seven source cases, five static mini fixtures, two full-world fixture preparations
and seven observation components pass on this pin. Four diagnostic commands now
produce one PASS and three RED. Six Python ASTs, snapshot raw hashes and whitespace
checks pass. No native journey, server boot, DB operation or gameplay batch ran.
Native qualification remains on the primary's actual integrated binary/schema at
its agreed major-batch boundary.

## Exact small commands

From repository root under Linux/WSL:

```bash
python3 tests/async/quest_accounting_prep/test_production_terms.py
python3 tests/async/quest_accounting_prep/test_native_selectors.py
python3 tests/async/quest_accounting_prep/test_recipient_retirement.py --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP02 --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP04 --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP07 --acceptance
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP06 --output /tmp/quest-prep-QP06
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP04 --output /tmp/quest-prep-QP04
```

The three RED commands exit1 intentionally; components return30/30/31. These
predicates are diagnostics, not additional release gates. Static selection runs
only the current availability/selection slice with constructed NPC holdings;
QP03 executes observe_give with lookup/census stubs. Bartender debit is injected
hypothetically: this pack does not prove that an active player was charged.
Fixtures seed no epoch, birth binding, UID or receipt. Static mini O roots are
supplied goods; full-world setup creates no task. Quietus needs real1734/1709
arrival/quoted fees, whereas the shared driver defaults to Woodseer and level56.

## Owned files and shared blockers

Owned documentation: [GAPS.md](GAPS.md), [CASES.md](CASES.md),
[RESULTS.md](RESULTS.md), [SOURCE_FACTS.json](SOURCE_FACTS.json), this HANDOFF.md.
Owned executable paths under `tests/async/quest_accounting_prep/`:
`case_data.py`, `prepare_fixture.py`, `test_production_terms.py`,
`test_native_selectors.py`, `test_recipient_retirement.py`,
`test_bartender_settlement.py`. Source facts retain both accounting pins, research
pin, exact terms/reset declarations, raw hashes and current runtime owners.

Primary owns recipe/coin policy; bartender debit/refund/task-attempt authority;
authentic native birth/source/custody/publication/cold ACK and pair retirement;
and the shared crash driver's real fixture/VNUM/alias selection. The driver fixes
synthetic22802–5/C1000; QP06 needs29262/3/4→29237/C3000 and later legitimate reward
custody change. Required changes are detailed in RESULTS.md. Shared production,
migrations, tests/contracts, manifests, registries and finish plan have no prep
edits; Plan5 remains independently owned. Primary can fetch/review/import the
compatible owned bundle and record consumed SHAs in its native handoff before
integrated qualification. All original R1–R8 and SQL-first release gates remain.
