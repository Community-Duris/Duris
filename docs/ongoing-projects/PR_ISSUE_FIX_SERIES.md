# Fix gameplay, locker persistence, builder, and runtime regressions

This series fixes reproducible defects in ferry handling, combat and regeneration,
invasion alarms, builder commands, locker access and persistence, boot registration,
epic training, and help publication. It adds executable regressions around the
affected behavior.

For example, completing an epic-training purchase now refreshes derived HP and GMCP
vitals after the durable epic acknowledgment. A player who is missing 30 HP remains
30 HP below the new maximum, and reaching 100 training displays a completed-mastery
message. The configured minimum level of 56 and existing prices remain unchanged.

Granted locker entry now resolves the named character to their account and faction.
Characters on that side share one locker, its room title uses a character name,
and a grant cannot admit an opposite-faction visitor. Locker saves preserve the
item tree and chest data without passing through player persistence. A failed
final locker save cancels normal shutdown or copyover and retains the live items.

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
| Lockers | Resolve character aliases to one account locker per faction, use character names in room titles, enforce faction boundaries independently of grants, and preserve occupancy privacy. Save through a dedicated locker payload without copying passwords or writing player-format files; retain failed saves and cancel shutdown/copyover if final locker persistence fails. Canonical guild names retain membership, parole, prestige, and fee checks. |
| Builder listings | Bound `rlist`, `mlist`, and `olist` to the requested/current zone, cap output at 500 entries with continuation, validate ranges, and use boot-loaded prototype names without spawning entities. |
| Epic training | Refresh stats and client vitals after a successful durable purchase, distinguish improvement from mastery, and retain refusal/refund handling. |
| Help | Resolve Quickstep to Quick Step, document existing Seadog mechanics, and mark imported redirects with the category required by the SQL renderer. |

The SQL help importer now assigns the redirect category expected by the renderer,
so Quickstep displays Quick Step instead of literal redirect text. The cache
regression uses the importer's category expression, and the database test checks
the staged metadata.

The account list also reads SQL login timestamps as epoch seconds and orders them
by most recent login, then player ID. This keeps the locker title fallback aligned
with the character menu and prevents an account save from replacing login dates
with their year values.

## Validation

- Clean MariaDB and flat-file server builds using GCC 13.3 and the repository's
  strict warnings-as-errors configuration:
  `make -C src -j2 BIN_ROOT=<task-bin>` and the corresponding
  `PERSISTENCE_BACKEND=flatfile DMS_BINARY=<task-bin>/server/dms_flat` build.
  Both backends were rebuilt successfully after the final locker changes.
- The earlier combined qualification passed 49 of 50 affected regression scripts.
  After removing two stale documentation references, the remaining documentation
  script passed all 12 cases, including at final review. Coverage includes native
  production-function tests under ASan/UBSan, regeneration/event scheduling,
  transaction boundaries, parsing, and lifetime checks. The subsequent locker
  changes received the separate qualification below; the earlier combined count
  is not a claim that the whole repository test suite passes.
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
- The final locker revision passed production-function tests for account-only
  resolution and private character-based titles, occupied/idle/pending-save
  admission, the dedicated save payload, retry
  preparation, failed-save retention, and the shutdown boundary under ASan/UBSan.
  Private-chest loading restores all five saved item-flag fields and
  material, and asynchronous saves deduplicate extra descriptions consistently
  with synchronous saves. Related item-codec, spellbook, container-weight,
  player-save, and copyover checks passed.
- The **12-stage full-world MariaDB locker journey** passed on the final server
  binary. It covers character/account grants, aliases, revocation, title privacy,
  faction refusal, reconnect, private/nested contents, and canonical guild access.
  Occupied and just-left shutdown/restart checks compare full saved item payloads,
  parent/item UIDs, affects, descriptions, and chest credentials/settings. An
  injected database write failure rolls back, cancels shutdown, and keeps the
  game running; removing that failure permits a clean save and restart. The
  journey also verifies that no locker player row or player-format file is created.
  See [the regression guide](../testing/REGRESSIONS.md#named-locker-entry).
- The root test-harness and persistence-severity regressions pass with their
  expectations aligned to the current runner and death-recovery reporter. The
  harness checks serial scheduling for connection-limit and corpse-trap journeys;
  severity checks retain success/progress/failure classifications through the
  reporter helper. Related death-recovery visibility, wallet-retry, and item-custody
  regressions also pass.
- `./scripts/format.sh --check --rev <base-master>` and
  `git diff --check <base-master>` passed.

The SQL journeys are reproducible with a generated full world, a matching MariaDB
server binary, and explicit disposable loopback `TEST_DB_HOST`, `TEST_DB_USER`,
and `TEST_DB_PASSWORD`:

```sh
python3 tests/async/test_mysql_epic_training_journey.py --server /absolute/path/dms_new
python3 tests/async/test_mysql_locker_grant_journey.py --server /absolute/path/dms_new
```

Each creates and drops a unique schema, follows the maintained bootstrap/migration
contract, and runs the help importer from a temporary copy without reading the
checkout's `.env`.

## Known limits and issue scope

Focused sanitizer harnesses are not a full-world sanitizer run. This qualification
does not inject a crash between the durable epic deduction and the subsequent
character snapshot, measure production PvP balance/latency, or establish that every
symptom in the broad historical reports is fixed.

Community's flat-file locker entry/load/save UI still contains pre-existing stubs.
The named-locker follow-up tests the real flat-file identity catalog and builds that
backend; its complete locker gameplay evidence is MariaDB only. It does not port
Luminari's newer locker engine or establish full flat-file locker support.

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
The named-locker entry, grants, faction isolation, and persistence acceptance for
Community's MariaDB implementation of Luminari #234 are covered by the locker
journey above; this does not assert a change to the upstream repository.

## Deployment notes

There are no new schema migrations. Locker rows and their existing item/chest/grant
tables are retained. New saves no longer copy the owner race/password or use a
player-format recovery file. Failed persistence retains live contents and prevents
a successful shutdown; it does not provide durability across a forced process kill
while the database is unavailable. The internal character-shaped item carrier
remains an object-movement adapter, not a player save payload.

Deploy the server changes and publish the
tracked help through the maintained importer for SQL-backed installations; a
server binary update alone does not replace database help pages. Flat-file help
uses the tracked content. Existing alarm reinforcement policy, training prices,
and Community's minimum epic-training level are preserved.
