# Fix confirmed gameplay, builder, and runtime regressions

This series fixes reproducible defects in ferry handling, combat and regeneration,
invasion alarms, builder commands, locker access, boot registration, epic training,
and help publication. It keeps each repair in a focused commit and adds executable
regressions around the affected behavior.

For example, completing an epic-training purchase now refreshes derived HP and GMCP
vitals after the durable epic acknowledgment. A player who is missing 30 HP remains
30 HP below the new maximum, and reaching 100 training displays a completed-mastery
message. The configured minimum level of 56 and existing prices remain unchanged.

## Behavior changes

| Area | Result |
| --- | --- |
| Ferries and wagons | Protect ferry objects from inappropriate deletion, handle their lifetime safely, correct stationary wagon `look out`, and update ferry messages, routes, and prices in help. |
| Combat utilities | Preserve slowness-poison flags, correct gradual over-cap movement decay, and apply the Swashbuckler anti-parry modifier in the intended direction. |
| Hatred and Awareness | Repair confirmed Hatred bounds, effect lifetime, watcher, and restoration defects; preserve fractional Awareness scaling in its active hamstring-defense use. |
| Control Flee | Correct the direction-control calculation and validate requested exits. The control roll remains distinct from escaping combat and any random fallback. |
| PSP regeneration | Use consistent psionic-point regeneration calculations and repair event restart, timing, and cap behavior. |
| Invasion alarms | Enable configured evil hometown alarms and evaluate accompanying NPCs through their directly linked, living player controller in the same room. Eligible enemy companions contribute arrival rolls; friendly companions and uncontrolled NPCs cannot independently trigger alarms. Existing reinforcement settings remain unchanged. |
| Kick and diagnostic messages | Correct kick race indexing/bounds and selected command, login, boot, and runtime logging defects. |
| Special procedures and boards | Remove assignments to missing authored VNUMs and register bulletin boards from board metadata without the duplicate keyword-based registration pass. |
| Lockers | Find occupied lockers through their active private-room wrappers, prevent duplicate loading, and preserve occupancy privacy when access is denied. |
| Builder listings | Bound `rlist`, `mlist`, and `olist` to the requested/current zone, cap output at 500 entries with continuation, validate ranges, and use boot-loaded prototype names without spawning entities. |
| Epic training | Refresh stats and client vitals after a successful durable purchase, distinguish improvement from mastery, and retain refusal/refund handling. |
| Help | Resolve Quickstep to Quick Step, document existing Seadog mechanics, and mark imported redirects with the category required by the SQL renderer. |

The SQL import repair was found during integration qualification: the file/catalog
tests originally supplied category 1, while the real importer stored category 0 and
displayed the literal redirect text. The cache regression now uses the importer's
actual category expression, and the database test checks the staged metadata.

## Validation

- Clean MariaDB and flat-file server builds using GCC 13.3 and the repository's
  strict warnings-as-errors configuration:
  `make -C src -j2 BIN_ROOT=<task-bin>` and the corresponding
  `PERSISTENCE_BACKEND=flatfile DMS_BINARY=<task-bin>/server/dms_flat` build.
- Combined run of 50 affected regression scripts: **49 passed**. The documentation
  contract has the same pre-existing link failure described below; its other 11
  cases passed. Coverage includes native production-function tests under ASan/UBSan,
  regeneration/event scheduling, transaction boundaries, parsing, and lifetime checks.
- A real flat-file server passed the **35-command builder journey** covering mortal
  rejection, staff access, two zones, the 500-row cap, continuation, paging cancellation,
  cold prototype names, and a subsequent mobile load.
- Generated full/mini world indexes passed the ordering audit: 17,680 full-world
  mobile prototypes, 19,149 object prototypes, and 253,261 rooms. The existing
  duplicate room VNUM remains supported by continuation offsets.
- `python3 tests/async/test_help_import_atomic.py` passed against disposable MariaDB:
  rollback, atomic publication, redirect category, and nontransactional-table refusal.
- The full-world MariaDB training/help journey passed: level-55 refusal,
  level-56/57 purchases, Human +10 HP and Illithid +5 HP per 10 training,
  full/injured health, matching GMCP packets, stable repeated refreshes,
  mastery/at-cap refusal, live reconnect, and clean restarts. Five successful
  purchases produced exactly five epic-ledger entries; restart did not add entries.
  The same game served the imported ferry, Quickstep, and Seadog help correctly.
- `./scripts/format.sh --check --rev <base-master>` and
  `git diff --check <base-master>` passed.

The SQL journey is reproducible with a generated full world, a matching MariaDB
server binary, and explicit disposable loopback `TEST_DB_HOST`, `TEST_DB_USER`,
and `TEST_DB_PASSWORD`:

```sh
python3 tests/async/test_mysql_epic_training_journey.py --server /absolute/path/dms_new
```

It creates and drops a unique schema, follows the maintained bootstrap/migration
contract, and runs the help importer from a temporary copy without reading the
checkout's `.env`.

## Known limits and issue scope

The documentation contract still reports these two missing targets from the base
branch, unchanged by this series:

- `docs/ongoing-projects/log-server-review.md#qualification-update-2026-10-14`
- `docs/ongoing-projects/plan-issue-106.md`

Focused sanitizer harnesses are not a full-world sanitizer run. This qualification
does not inject a crash between the durable epic deduction and the subsequent
character snapshot, measure production PvP balance/latency, or establish that every
symptom in the broad historical reports is fixed.

Refs [Community #624](https://github.com/community-duris/duris/issues/624).
The series also addresses confirmed, bounded portions of Luminari
[#166](https://github.com/LuminariMUD/Duris/issues/166),
[#187](https://github.com/LuminariMUD/Duris/issues/187),
[#57](https://github.com/LuminariMUD/Duris/issues/57),
[#14](https://github.com/LuminariMUD/Duris/issues/14),
[#4](https://github.com/LuminariMUD/Duris/issues/4),
[#234](https://github.com/LuminariMUD/Duris/issues/234),
[#231](https://github.com/LuminariMUD/Duris/issues/231), and
[#56](https://github.com/LuminariMUD/Duris/issues/56).
These references are for scope and traceability; the broader issues need separate
closure decisions. Web Spray is omitted because Community does not implement it.

## Deployment notes

There are no new schema migrations. Deploy the server changes and publish the
tracked help through the maintained importer for SQL-backed installations; a
server binary update alone does not replace database help pages. Flat-file help
uses the tracked content. Existing alarm reinforcement policy, training prices,
and Community's minimum epic-training level are preserved.
