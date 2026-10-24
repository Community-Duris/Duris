# Standalone journal port qualification

Date: 2026-10-23. Branch: `codex/zone-journals-standalone`.
Base: master `2a1604e4eedf55034c8183c5cfc28cb683ef88cd`.
The [current contract](ZONE_STORY_STANDALONE_PORT.md) describes the runtime and
its failure boundaries. Historical accounting qualification documents describe
another branch and are not evidence of native repairs or accounting requirements
in this port.

## Preservation

- All 232 imported maps retain their source areas, native contract bindings,
  categories, optional steps, quantities, item identities, contacts and topics.
  Revisions and player-facing explanations were adapted for standalone gameplay.
- Vella's Bordello adds one map with paid follower-service guidance, local
  contacts and roaming material sources for Winterhaven's quests. Its
  revision-three map records three scripted hire outcomes as services. The
  complete catalog validates 233 maps, 2,668 unchanged native contracts and
  three separate scripted definitions, producing 1,518 story achievement units
  and 1,406 conservative daily candidates. Hires add no achievement or renown.
- Native area files and special assignments match the pinned master base. The
  only change to `src/world/quest.c` observes an NPC encounter after a successful
  ASK response; native offer, consumption and reward logic is unchanged.
  The three assigned Vella hire callbacks observe success after the existing
  payment, pet setup and follower operations. An additional `setup_pet` overload
  exposes whether setup was applied while preserving the four-argument API,
  return values, duration, rejection paths and effects.
- All 980 research documents match the SHA-256 hashes in
  [PORT_SNAPSHOT.json](quest-story-followups/PORT_SNAPSHOT.json).
- Master's existing 31 migration entries are unchanged. Migration 0032 is additive.
  No original feature/research worktree or original PR was modified.

## Executable validation

The current Vella revision adds schema-four declarations and a reusable scripted
success observer. The arrival regression executes all three production hire
callbacks and the real pet-setup implementation with controlled transfer,
follower and persistence surroundings. It covers exact payment, overpayment,
non-accumulating underpayments, rejected transfers, wrong targets, combat,
rejected pet setup/renewal, indefinite charm, personal credit, repeated hires,
loss of the pet, reload, disabled tracking, failed saves and unexpected storage
exceptions. A second-zone non-pet story proves reuse, physical-zone discovery
and rollback without inventing discovery of a remote owning zone. These are
executable adapter tests; a full live hire/expiry journey remains outstanding.

Schema tests exercise version 1-3 compatibility, version-four service projection,
atomic rejection of duplicate/invalid/unbound declarations, and local-only
scripted completion references. The production snapshot adds only three scripted
definitions; all 2,668 native definitions, existing bindings and achievement/daily
counts are unchanged. No new persistence format or migration is introduced by
this scripted-outcome addition.

Focused commands for the scripted-outcome addition:

```bash
python3 tests/async/test_zone_story_quest_arrival.py
python3 tests/async/test_zone_story_quest_story.py
python3 tests/async/test_zone_story_quest_production.py
python3 tests/async/test_zone_story_quest_production_catalog.py
python3 tests/async/test_zone_story_quest_source_area.py
python3 tests/async/test_flatfile_zone_story_quest_state.py
python3 tests/async/test_zone_story_quest_owners.py
python3 tests/async/test_zone_story_quest_turolzoo.py
make -C src BIN_ROOT=/output/bin -j2
```

The disposable build used image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
g++ 13.3, C++20, and task-specific D: output/scratch mounts. No accounting module
is linked into the arrival/native-completion harness.

Passed focused regressions:

- Production loader: strict builder validation and per-area quarantine for bad
  mappings/prototypes; unaffected journals and native identities remain available.
- Arrival: genuine discovery, temporary-placement suppression, visible encounters,
  ordinary native completion without accounting, persistence failure rollback,
  optional external receipt replay and frozen context.
- Feature, serialization and capacity: discovery, grouping, daily eligibility,
  replay, seasonal state and bounded records.
- Flatfile store and disabled-runtime erasure: legacy conversion, interrupted
  persistence recovery, failure handling and character-state erasure protections.
- SQL store against an isolated private MariaDB database: legacy conversion,
  large record buckets, atomic rollback, replay and erasure.
- Production catalog, source ownership, source-area identity and Turolopolis Zoo
  source-contract checks. The generated catalog must equal the current sources.
- Strict authored-story regression across the mapped catalog: optional preparation,
  exact ingredient counts, alternatives, independent outcomes, service exclusion,
  daily projection, receipt replay, legacy schema compatibility and invalid maps.
- Immutable migration runner and runtime compatibility checks.

The full maintained server build and final incremental build passed with
`make -C src BIN_ROOT=/output/bin -j2`. The final arrival/native-observation and
disabled-runtime erasure regressions also passed after the erasure-guard edit.
The initial standalone implementation was qualified at commit
`6489b7987423a9a37317c066a3c08eda357a1fa8`. The later Vella scripted-outcome addition
has separate build and focused regression evidence described above and below.
Diagnostic runs that collect assertion failures are not counted as passing tests.

## Database proof

An isolated `zone_daily_*` database was created inside the owned disposable
container. The proof applied the old journal-table migration, inserted a v1
marker, applied 0032 twice, ran its verifier twice, and checked that the legacy
row and column/index metadata were preserved. The SQL harness then exercised
real transactions. The private database server was shut down by the proof's trap.

This proves the tested MariaDB path. MySQL 8 and production rollout were not
exercised. No production database, player records or live settings were changed.
Converted v2 journal state needs a compatible reader or a pre-port state backup
for rollback; an old binary is not a sufficient rollback plan by itself.

## Review findings resolved

- Removed accounting activation gates from ordinary journal access and events.
- Replaced an accounting-branch-only SQL include with master's transaction API.
- Kept master's `no_specials` boot behavior.
- Isolated invalid sidecars so one area's journal failure does not disable every
  other area's journal or alter native quest loading.
- Removed inherited player claims that native paid offerings, large batches,
  shops or training require accounting. Daily eligibility remains a separate,
  conservative policy; it does not restrict native exchanges.
- Updated prose/revision expectations and added a generated-catalog freshness
  check so tests cannot silently use outdated journal explanations.
- Restored the pre-erasure serialization guard: failure to capture rollback state
  returns before changing the character's journal history.
- Recorded the user's Turolopolis decision: hostile badge holders, direct recovery
  through combat and looting, no rescue/reunion endpoint, and supplied badges valid.
- Distinguished rejected pet setup from successful indefinite charm without
  changing either result; rejected renewals cannot reuse existing pet ownership
  as evidence of a new successful hire.
- Recorded actual physical-zone discovery before the scripted giver encounter,
  with both changes in the completion save/rollback boundary. Generic outcomes
  work when no earlier arrival record exists and never discover a remote owner.
- Preserved the existing four-argument pet-setup interface for all other callers.
- Rejected foreign scripted completion steps rather than accepting them according
  to sidecar load order; existing native QST cross-area references remain valid.
- Updated the zone evidence inventory to expose declared scripted outcomes
  separately from native Q blocks, preserving native offering/source analysis
  and corrected-giver ownership instead of trying to parse a script as a Q block.

## Limits and next proof

These are source, projection, persistence and build checks, not live mortal
journeys through all zones. The [starter review](ZONE_STORY_STARTER_REVIEW.md)
lists the Turolopolis/zoo, Vella's and Harrow journeys still needed. Other preserved
maps remain research-backed drafts pending their individual player/source review.

Inventory readiness is current possession, not first acquisition or provenance.
The journal cannot distinguish loot from a gift and does not claim a personal
kill. ASK aliases provide guidance rather than separate keyword achievements.

The existing native completion observer runs before rewards and has no standalone
outbox. Journal persistence and native rewards are not atomic: a failed save can
miss a successful exchange, and a crash can separate an accepted-hand-in record
from the later reward. No reward replay or economic recovery is claimed.
Scripted observers run after native success. They can also miss a completed
interaction on save failure or crash; callers must never replay native effects
or gate payment/rewards on the observer's return value.

Evidence and build outputs are retained under the task-specific directories in
`D:/Dev/Temp/duris-zone-journals-standalone-20261022` and
`D:/Dev/Builds/Duris/zone-journals-standalone-20261022`.
Vella scripted-outcome evidence is under
`D:/Dev/Temp/duris-vella-scripted-20261023/evidence`, using the same chat's build
directory and pinned image with a new exclusive container lease.
The owned disposable container exited and was removed after the successful final
build; its lease was released. No development game or database from this task was
left running. Evidence/build outputs are retained; unrelated services were untouched.
Research files were preserved byte for byte, including inherited line endings and
whitespace. The standalone implementation passes the changed-line clang-format
18.1.8 check and Git whitespace checks with CRLF line endings recognized.
