# Quest accounting prep handoff

Status: usable seven-case source/fixture/component pack committed, pushed and
reviewable on `origin/codex/accounting-quest-prep`.
Accounting stays inactive; no release or native journey qualification is claimed.

Branch `codex/accounting-quest-prep`, separate managed worktree. Base accounting
commit `17c033d69316b21da8598791fc95cae79baa8dc2`. PR #678 research head
`55905eac1906cf59405764407f9d22497cccfff3`. Static production catalog computed
with `scripts/zone_story_quest_catalog.py` against `areas/AREA`: 2,668 definitions,
fingerprint `04d687493b02ed27a3b32d070a0a71e9d10c1643406d5fb95fe6b1538b4bb314`.
Dynamic bartender quests are excluded and separately traced to runtime producers.

## Import order and owned files

First gap/four-priority bundle (already pushed within the two-hour milestone):
`f1a15f1d982bf330d57428b20797beb96aa34c71`.
Second bundle (committed and pushed):
`cd6e62e331aae0b81b23ed858ec900420e70bbf5` — complete specifications,
source-fact snapshot, results and six owned Python files. This subsequent
handoff-only metadata commit records its exact SHA; it adds no fixture or
production changes. Resolve the published metadata tip with
`git rev-parse origin/codex/accounting-quest-prep` after fetching.
Import in order or review the combined owned-path diff from the accounting base.
Production source, migrations, shared tests/contracts,
manifests, registries, finish plan and Plan5 audit/restore are untouched.

Owned files:

- `docs/persistence/economy_accounting/quest-prep/HANDOFF.md`
- `docs/persistence/economy_accounting/quest-prep/GAPS.md`
- `docs/persistence/economy_accounting/quest-prep/CASES.md`
- `docs/persistence/economy_accounting/quest-prep/RESULTS.md`
- `docs/persistence/economy_accounting/quest-prep/SOURCE_FACTS.json`
- `tests/async/quest_accounting_prep/case_data.py`
- `tests/async/quest_accounting_prep/prepare_fixture.py`
- `tests/async/quest_accounting_prep/test_production_terms.py`
- `tests/async/quest_accounting_prep/test_native_selectors.py`
- `tests/async/quest_accounting_prep/test_bartender_settlement.py`
- `tests/async/quest_accounting_prep/test_recipient_retirement.py`

## Priority and evidence

Read [GAPS.md](GAPS.md) for existing versus missing coverage and [CASES.md](CASES.md)
for QP01 exact sapphire kinds, QP02 overlap/paid fee, QP03 disappearance/birth,
QP04 dynamic fee/refund, QP05 repeated skins/cap, QP06 real mixed Kord
rewards and QP07 dynamic stale map/abandon attempt. Seven total; do not add
duplicates of the existing synthetic crash or reward-callback tests.

Source facts were checked against this pinned worktree. The latest linked native
quest/birth handoffs describe private source (including terminal pair manifest
`4b0e7821471740024042dc2be19269cbe9f68760a31c8a9ad73ce6e1f4a04d7b`), not maintained
native installation. No private tmp candidate was imported or tested here.

Read [RESULTS.md](RESULTS.md) for exact commands, limitations and integration
seams. Seven source cases pass; five static mini fixtures and two isolated
full-world run-root fixtures pass preparation checks. Seven extracted-function
observation components pass under WSL Python3.10.12/g++11.4.0. Four local acceptance
assertions are RED as expected on the pin: QP02 backpack reachability,
QP03 replacement incarnation, QP04 active refund and QP07 stale map. Those
diagnostic seams must follow the primary's actual owner when it replaces an old
helper; their current RED is not an additional release gate.

All seven native journeys remain pending. No server, migrations, operational
scripts, gameplay or database qualification ran; the major-batch cadence is
preserved. No synthetic birth binding, epoch, UID, SQL root or receipt is seeded.
Fixtures explicitly distinguish authentic source bytes from supplied roots,
relocated mini room and unqualified full-world runtime setup.

The source snapshot's hashes match the actual worktree; 23 selected production
input/bartender producer files match PR #678 head exactly. `world_quest.c` has
daily/journal UI differences outside the selected economic facts. Six owned Python
ASTs and owned diff whitespace pass. Source review corrected the first bundle's
totem attribution: dark shaman16087@16161, not Altrucali16086, owns its G16080 row.

## Primary integration blockers

- Authentic reset birth/source/cash/forest binding and incarnation-aware native12
  producer/publication, cold reconstruction and callback uncertainty.
- Reachable supported recipe policy at the existing overlapping Q dispatch;
  mixed-fee settlement must retain guards until coupled authority exists.
- Bartender debit/refund and exact task-attempt settlement/recovery.
- Acknowledged historical reward proof and original parent/child pair retirement.

Primary should fetch this branch, review/import compatible owned commits, recheck
producer pins, and qualify selected SQL-first journeys on its actual integrated
binary/schema at the agreed batch boundary. Keep blocked cases pending and update
its existing native quest handoff with consumed SHAs and actual evidence.

Small prep commands from repository root on Linux:

```bash
python3 tests/async/quest_accounting_prep/test_production_terms.py
python3 tests/async/quest_accounting_prep/test_native_selectors.py
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py
python3 tests/async/quest_accounting_prep/test_recipient_retirement.py
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP06 --output /tmp/quest-prep-QP06
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP04 --output /tmp/quest-prep-QP04
```

The maintained world driver defaults to level56 at Woodseer16633/giver16553;
QP04/QP07 require real Quietus1734/1709 arrival and the actual quoted level/fee.
The level11 examples are controlled case inputs, not a claimed mortal journey.
Do not copy a real `.env`, use production authority or relax active refusals to
run these tests. Primary lifecycle ownership supplies the authentic disposable
SQL candidate and frozen runtime/binary/schema pins for final qualification.
