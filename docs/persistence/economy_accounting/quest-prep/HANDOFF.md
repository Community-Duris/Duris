# Quest accounting prep handoff

Status: first gap/specification bundle prepared; executable fixture pack in progress.
Accounting stays inactive; no release or native journey qualification is claimed.

Branch `codex/accounting-quest-prep`, separate managed worktree. Base accounting
commit `17c033d69316b21da8598791fc95cae79baa8dc2`. PR #678 research head
`55905eac1906cf59405764407f9d22497cccfff3`. Static production catalog computed
with `scripts/zone_story_quest_catalog.py` against `areas/AREA`: 2,668 definitions,
fingerprint `04d687493b02ed27a3b32d070a0a71e9d10c1643406d5fb95fe6b1538b4bb314`.
Dynamic bartender quests are excluded and separately traced to runtime producers.

## Import order and owned files

Initial bundle: this handoff, `GAPS.md`, `CASES.md` in this directory. Exact commit
SHA will be recorded after committing; primary can inspect branch history in the
meantime. Only these docs and necessary `tests/async/quest_accounting_prep/`
additions are owned. Production source, migrations, shared tests/contracts,
manifests, registries, finish plan and Plan5 audit/restore are untouched.

## Priority and evidence

Read [GAPS.md](GAPS.md) for existing versus missing coverage and [CASES.md](CASES.md)
for QP01 exact sapphire kinds, QP02 overlap/paid fee, QP03 disappearance/birth,
QP04 dynamic fee/refund. Selected follow-on: repeated skins/cap, real mixed
Kord rewards and dynamic stale map/abandon attempt. Seven total; do not add
duplicates of the existing synthetic crash or reward-callback tests.

Source facts were checked against this pinned worktree. The latest linked native
quest/birth handoffs describe private source (including terminal pair manifest
`4b0e7821471740024042dc2be19269cbe9f68760a31c8a9ad73ce6e1f4a04d7b`), not maintained
native installation. No private tmp candidate was imported or tested here.

Results so far: source/catalog survey only. Executable commands in CASES are
being prepared, not claimed passing. No server, migrations, operational scripts,
gameplay or database qualification ran; the major-batch cadence is preserved.

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
