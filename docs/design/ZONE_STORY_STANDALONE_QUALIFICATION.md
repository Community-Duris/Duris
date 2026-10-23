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
- Vella's Bordello adds one exploration/contact-only map, with no quest contracts.
  The complete catalog validates 233 maps and 2,668 native contracts, producing
  1,518 story achievement units and 1,406 conservative daily candidates.
- Native area files and special assignments match the pinned master base. The
  only change to `src/world/quest.c` observes an NPC encounter after a successful
  ASK response; native offer, consumption and reward logic is unchanged.
- All 980 research documents match the SHA-256 hashes in
  [PORT_SNAPSHOT.json](quest-story-followups/PORT_SNAPSHOT.json).
- Master's existing 31 migration entries are unchanged. Migration 0032 is additive.
  No original feature/research worktree or original PR was modified.

## Executable validation

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
The tested implementation commit is `6489b7987423a9a37317c066a3c08eda357a1fa8`;
subsequent qualification/review-plan edits are documentation only. Diagnostic runs
that collect assertion failures are not counted as passing tests.

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

Evidence and build outputs are retained under the task-specific directories in
`D:/Dev/Temp/duris-zone-journals-standalone-20261022` and
`D:/Dev/Builds/Duris/zone-journals-standalone-20261022`.
The owned disposable container exited and was removed after the successful final
build; its lease was released. No development game or database from this task was
left running. Evidence/build outputs are retained; unrelated services were untouched.
Research files were preserved byte for byte, including inherited line endings and
whitespace. The standalone implementation passes the changed-line clang-format
18.1.8 check and Git whitespace checks with CRLF line endings recognized.
