# Discovered zone daily quests qualification

Date: October 1, 2026. Feature branch: `codex/discovered-zone-dailies`.
Base: `experimental-accounting`, commit `1bb03b465`.

The implementation has passed local builds, focused regressions, native gameplay,
and crash/restart recovery on flat-file authority and isolated MySQL 8 authority.
The feature branch has not been merged. Daily activation remains disabled by
default; no production database, settings, or player data were used.

## Implemented behavior

- A genuine first visit earns a seasonal discovery achievement and opens the
  area's private journal, including areas without tracked quests.
- The journal includes every tracked static quest in a discovered area. Every
  suitable repeatable contract is considered daily, subject to the existing
  evidence, level, faction, party, and prerequisite policy.
- Original NPC turn-ins and rewards drive story and daily progress. Distinct
  story credit persists for the season; daily entries reset at midnight UTC.
  The first qualifying completion grants one renown per PID per day.
- V6 accounting continuations freeze season, catalog revision, original time,
  credited recipients, and the daily-eligible subset. Recovery and duplicate
  replay preserve those terms without awarding again.
- Private journal, daily overview, area achievements, score, paging, and help
  are connected to the existing game commands.
- Legacy state conversion preserves credit and backfills only proven visits.
  Failed writes restore memory; deletion removes the PID's facts while retaining
  other recipients' own credit and delayed public completion times.

Catalog revision 2 retains all 2,668 static contract identities, with 349
discoverable playable areas, 2,659 achievement-eligible contracts, and 2,133
potential daily candidates. See the [catalog audit](../reference/ZONE_STORY_QUEST_CATALOG.md)
for exclusions and the separate scripted-content boundary.
The audit also excludes 286 otherwise repeatable contracts that the current
accounting offering path cannot accept; they remain in the full story journal.

## Validation

| Validation | Result and scope |
| --- | --- |
| SQL and flat-file server builds | Passed in Ubuntu 22.04 under WSL, C++20, `g++-12`, repository warning/error flags. Initial full builds and final incremental builds succeeded. |
| Domain and native arrival harnesses | Passed: genuine/temporary visits, empty areas, staff/arena/ship suppression, discovery persistence and rollback, multiple daily entries, UTC rollover, party eligibility, frozen subsets, replay conflicts, disabled mode, corrupt state, deletion, public delay. |
| Catalog, production catalog, tracking, repository, state schema, and daily report contracts | Passed, including active-world ownership ranges and stable identities. Lazy prototype names are read without creating NPCs/items or changing loader positions. |
| Item-transfer compatibility | Passed for legacy continuation versions and the new V6 frozen daily subset; truncation and invalid subsets are rejected. |
| Flat-file storage | Passed for legacy conversion, current snapshots/deltas, checksum rejection, torn append recovery, restart, and physical erasure. |
| Real SQL storage | Passed on disposable MySQL 8.0.46: legacy conversion, large compressed snapshots, corrupt payload rejection, multi-bucket rollback, repeated updates/deletions, idempotent replay, and erasure. |
| Native daily gameplay | Passed: discovery/journal, two different real NPC turn-ins, two daily entries, one renown, original item reward, save, and cold reconnect with the same reward UID. |
| Offering crash recovery | Passed on flat-file and MySQL authority: kill after committed offering, recover original item/cash/XP and daily credit, then two cold restarts without duplicate awards. |
| XP acknowledgement crash recovery | Passed on flat-file and MySQL authority, including original XP and daily credit after two cold restarts. |
| Migration and runtime compatibility | Passed: 22 migration-runner tests, 10 runtime boot contract tests, manifest validation, and actual canonical migrations through 0051 with native SQL server boot. The supported staging history also completed through 0051 on disposable MySQL, and its schema/history verifier passed. |
| Data lifecycle and reward ACK retry | Passed: 14 lifecycle inventory tests and the focused ACK retry contract. |
| Formatting and diff whitespace | Passed `scripts/format.sh --check` for changed lines and whole touched files, and `git diff --cached --check`. |

Build commands used `BIN_ROOT=../bin` because the worktree path contains a space:

```sh
make -C src -j6 CC=g++-12 BIN_ROOT=../bin
make -C src -j6 CC=g++-12 BIN_ROOT=../bin PERSISTENCE_BACKEND=flatfile \
  DMS_BINARY=../bin/server/dms_zone_daily_flatfile
```

The focused tests live in `tests/async/`, especially
`test_discovered_zone_daily_journey.py`, `test_discovered_zone_daily_crash.py`,
`test_zone_story_quest_arrival.py`, `test_zone_story_quest_sql_store.py`, and
`test_zone_story_quest_capacity.py`. SQL tests require an explicitly disposable
database. The `mariadb` crash-test backend name selects the server's SQL authority;
the engine used in this qualification was MySQL 8.0.46.

## Retained-history capacity

The capacity test retains 25 characters completing four quests every day for 60
days. It saves every completion, then reconstructs the service from storage and
compares the complete state, including all 6,000 events and daily awards.
The logical document is 29,862,893 bytes, exceeding the old aggregate MEDIUMTEXT
ceiling. SQL uses 255 bounded, compressed buckets; flat-file authority uses a
checksum-protected append journal and bounded snapshot frames.

Final measurements, with server builds and other database journeys stopped:

| Authority | Average completion/save | Slowest completion/save | Physical storage after 6,000 events |
| --- | ---: | ---: | ---: |
| MySQL 8 | 17.51 ms | 666.88 ms | 3,319,749 bytes across 255 buckets; largest compressed bucket 17,087 bytes. |
| Flat-file | 7.34 ms | 18.71 ms | 78,816,896-byte journal, including frame overhead and un-compacted updates. |

The largest uncompressed bucket was 362,293 bytes. Full logical serialization
took 74.41 ms in the SQL run and 68.30 ms in the flat-file run; ordinary saves
do not serialize the whole global document. The SQL result includes an isolated
long durable-save outlier. SQL saves use the guarded main-thread connection, so
target-machine storage latency should be measured before broad activation.

These are local development measurements from the optimized capacity harness,
not a production load guarantee. Much larger populations and indefinite history
still need a retention and storage budget. The feature does not discard recovery
or accounting history to keep the benchmark small.

## Merge and rollout

Merge accounting first, then update this feature branch to the accepted accounting
base and repeat focused build/gameplay/recovery validation. Migration
`0054_discovered_zone_daily_state` is required by both the canonical and supported
staging histories. It is additive, re-runnable, preserves v1 state, and allows v2
records without rewriting player facts during migration.

MariaDB 10.11 engine integration was not run because that engine and Docker were
unavailable in this environment. The existing MariaDB metadata fingerprint is
unchanged; 0051 changes CHECK expressions rather than columns, indexes, or foreign
keys. The dual-engine staging/tamper suite remains a useful check on a host with
those engines. No CI wait or full repository burn-in was used for this focused
feature qualification.

Enable `ZONE_STORY_DAILY_ENABLED=true` deliberately after installing the accepted
accounting/feature code and migration. Discovery and ordinary story tracking work
with daily activation off. Production migration and activation require the
owner's deployment instruction. Older binaries cannot read V6 continuations or
version-3 flat-file state after those have been written.
