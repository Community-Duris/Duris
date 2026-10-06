# Prep verification and integration commands — 2026-10-06

Inputs: accounting `17c033d69316b21da8598791fc95cae79baa8dc2`, research PR #678
`55905eac1906cf59405764407f9d22497cccfff3`. Tools: WSL Ubuntu22.04,
Python3.10.12, g++11.4.0, C++20 `-O0 -Wall -Wextra -Werror`. Compiled extracted
components use private temporary directories under `bin/tests/` and clean up
after execution. No maintained server build, full gameplay batch, SQL database,
schema change, migration, accounting activation, live service or native cold
journey ran. The integration candidate is still primary-owned.

## Results and exact commands

Run these from the repository root on Linux. Each command selects only the prep
additions; no central manifest/test registry change is included.

| Command | Result | Evidence scope |
| --- | --- | --- |
| `python3 tests/async/quest_accounting_prep/test_production_terms.py` | PASS seven source checks and five static mini fixtures | Production `areas/AREA` catalog, literal terms/prototypes/reset declarations; synthetic room/supplied roots are explicit |
| `python3 tests/async/quest_accounting_prep/test_production_terms.py --case QP04` | PASS | Actual dynamic assignment plus isolated full-world run-root/TLS/journals; no seeded task or native fee |
| `python3 tests/async/quest_accounting_prep/test_production_terms.py --case QP07` | PASS | Same full-world fixture construction for stale service scenario |
| `python3 tests/async/quest_accounting_prep/test_native_selectors.py` | PASS QP01/QP02/QP05/QP06 | Actual extracted durable selector with real Q/G/R order; submission/credit/reward capture seams stubbed |
| `python3 tests/async/quest_accounting_prep/test_bartender_settlement.py` | PASS QP04/QP07 observations | Actual payment callback and ADD_MONEY; hypothetical debit delivery and quest/native currency boundaries stubbed |
| `python3 tests/async/quest_accounting_prep/test_recipient_retirement.py` | PASS QP03 observation | Actual lookup/D cleanup; reward/tracking/extraction authority stubbed |
| `python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP02 --acceptance` | Expected RED, exit1, component30 | Three-hide supported backpack remains unreachable behind unsupported paid gloves |
| `python3 tests/async/quest_accounting_prep/test_recipient_retirement.py --acceptance` | Expected RED, exit1, component32 | Old prototype/room lookup accepts reset replacement incarnation |
| `python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP04 --acceptance` | Expected RED, exit1, component30 | Active refund helper refuses credit after refund prose |
| `python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP07 --acceptance` | Expected RED, exit1, component31 | Old map callback mutates replacement task |
| `git diff --check` | PASS | Owned diff whitespace |

The all-case terms command initially ran before dynamic fixture support was
added; the two selected commands subsequently verify those new full-world
fixtures. All static fixture additions and final component source were checked.
Windows Python cannot import the maintained journey's Linux `fcntl` dependency;
WSL resolves that constraint. Linux git cannot directly follow a Windows worktree
gitdir pointer; source facts therefore use the explicit base/research pins and
actual raw source hashes, rather than manufacturing an integrated HEAD claim.

## Preparing reviewable fixtures

```bash
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP01 --output /tmp/quest-prep-QP01
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP02 --output /tmp/quest-prep-QP02
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP03 --output /tmp/quest-prep-QP03
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP04 --output /tmp/quest-prep-QP04
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP05 --output /tmp/quest-prep-QP05
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP06 --output /tmp/quest-prep-QP06
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP07 --output /tmp/quest-prep-QP07
```

Outputs must be new/empty. Static outputs retain complete selected native
prototype/Q bytes, all four overlapping QP02 contracts in file order, actual
recipient G/E stock and one extra supplied root per kind. They relocate the
recipient to mini room22800 and supply loose inputs via synthetic O rows. Those
O rows deliberately do not prove production admission, caps or birth origins.
QP05's supplied two huge skins do not claim ordinary cap-one availability.

Dynamic outputs use `run_world_quest_dual_backend.setup_run_root`, with full
production area links, isolated lib/journals/local TLS, exact Quietus commands and
pending-hook metadata. They create no static QST surrogate, player, task, epoch
or database. Full `make world` and real SQL candidate qualification remain at the
primary batch boundary. The maintained driver defaults to level56 at
Woodseer16633/giver16553; use genuine arrival/level/quoted fees for Quietus1734/1709.
Do not infer a level11 journey from a level56 character factory.

Each generated `quest-prep-provenance.json` records base/research pins, actual
source hashes, source line numbers, native terms/reset declarations, prototype
locations, fixture adaptations and missing hooks. Generated fixtures, certificates,
logs and native outputs are not committed. The tracked [SOURCE_FACTS.json](SOURCE_FACTS.json)
is a bounded source-fact snapshot, not a generated area output or a registry.
Regenerate for explicit source review with:

```bash
python3 tests/async/quest_accounting_prep/case_data.py --output docs/persistence/economy_accounting/quest-prep/SOURCE_FACTS.json
```

## Source comparison and handoff checks

The selected production AREA, QST, mobile/object/zone inputs, Quietus SHOP data and
`specs.world_quest.c` agree byte-for-byte with research head for the bounded facts
used here. `git diff <research> HEAD -- <selected-paths>` is empty for those inputs.
`world_quest.c` differs only in the daily/journal UI section; the selected payment,
reward identity and native callback facts agree. No daily/journal feature is imported.
Actual secondary dispatch is `interp.c` qst_func at2836/2837 for static quests;
Quietus runs `func.mob=shop_keeper`, whose `SHOP_FUNC` dispatch retains world_quest.

Source tracing corrected the early QP03 note: dark shaman16087@16161 carries
totem16080 (`pineholl.zon:444–445`), rather than Altrucali16086. Sapphire43703,
43752 and43753 have identical short display names but distinct exact prototypes;
their E parents are worshiper43710 at43712/43836/43798. Orb44164 is P stock in
altar44165 at44333, cap1. These are source declarations, not witnessed admitted
world holdings. SOURCE_FACTS records the original reset rows and parent declarations.

## Primary-owned integration seams

1. **Fixture selection in maintained crash journey.**
   `run_quest_reward_ack_crash.py::run` currently calls the synthetic
   `quest.quest_fixture`, uses acorn/branch/feather and22802/3/4, asserts reward22805
   and C1000. Reuse its isolated SQL bootstrap, tombstone/obligation/XP checks,
   GDB offering/XP-ACK faults and two restarts for QP06 with Kord's actual fixture,
   aliases ear/scalp/toe, input29262/3/4, reward29237 and C3000. Add the later
   reward transfer/destruction and authentic terminal pair observation. Primary
   owns any modification to that shared driver.
2. **Native birth, stock and recipient incarnation.** Import these fixtures only
   after the authentic birth/native12 owner is integrated. Observe actual
   constructor/source/cash, stock forest/equipment order and immutable birthplace;
   require genuine receipt/binding witnesses. QP03 lookup/D component does not
   implement or certify that owner. Primary owns callback/alchemist/tail/cold
   reconstruction, adoption and paired retirement; no saved pointer/flag or
   synthetic event may stand in for proof.
3. **Bartender service ownership and fault control.** Reuse the maintained
   full-world disposable runtime pattern for QP04/QP07, with real SQL debit and
   attempt-bound task/history capture. Fault after debit commit/before callback,
   after refund commit/before reply and before/after exact ACK/retirement. The
   extracted old callback's RED predicate is a reproducer; adapt the seam to the
   primary's real durable service owner when it replaces that callback.
4. **Bounded native observation.** Capture actual operation/source/account lifetimes,
   wallet denomination vectors, UID/payload/current owner/tombstone/event/reference,
   obligations and authentic parent/child frames. Verify missing/corrupt historical
   receipts remain held and later legitimate reward custody does not trigger
   reissuance. Do not write synthetic accounting rows to make an oracle pass.

Primary records imported commit SHAs and actual integrated SQL/MySQL/MariaDB
journey results in its existing native quest handoff at the agreed major-batch
boundary. Plan5 keeps audit/restore ownership. All original gates and active
refusals remain; green source/component results here are not same-root native
writer, full Plan, activation or release qualification.
