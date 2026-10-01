# Discovered Zone Daily Quests Implementation Plan

**Status: implemented on `codex/discovered-zone-dailies`, based on
`experimental-accounting` commit `1bb03b465`. Local build, gameplay, and recovery
qualification passed; no merge or production rollout has been performed.**

Implementation details and current content coverage are maintained in the
[daily contract](../reference/ZONE_STORY_QUEST_DAILY.md),
[tracking contract](../reference/ZONE_STORY_QUEST_TRACKING_CONTRACT.md), and
[catalog audit](../reference/ZONE_STORY_QUEST_CATALOG.md). The baseline findings
below explain the design choices and refer to the pre-feature checkout.
Measured validation and the remaining engine coverage limitation are in the
[qualification record](DISCOVERED_ZONE_DAILY_QUESTS_QUALIFICATION.md).

Discovering an area should earn that area's discovery achievement and unlock its
quest journal. When daily quests are enabled, every reviewed repeatable quest in
that discovered area should be available each day. Completing the original quest
should update both its existing seasonal achievement progress and its daily
status through the same authoritative completion event.

The recommended design extends the existing zone story service. Daily status
resets each day; discovery and distinct quest achievements persist for the
current season. Normal quest objectives, turn-ins, rewards, and world resets
continue to supply the gameplay.

## Proposed player experience

The example below is illustrative; Ashen Hollow is an example area name.

```text
You have discovered Ashen Hollow!
Achievement earned: Discover Ashen Hollow.
Type 'quest zone Ashen Hollow' to view its quests.

> achievements zone Ashen Hollow
Ashen Hollow
  Discovery: Completed
  Story quests: 3 of 8 unique quests completed (37.50%)
  Quest milestones: 25% reached
  Today: 1 of 6 daily quests completed

> quest daily Ashen Hollow
Ashen Hollow daily quests
  [Done]      Recover the missing supplies
  [Available] Help the watch captain
  [Locked]    Deliver the sealed message - finish the captain's request first
  ...
  Daily bonus: Earned today
  Resets in: 5h 14m
```

One completion can advance both views. Repeating that quest later the same day
does not add another daily completion or another distinct seasonal completion.
Repeating it tomorrow advances tomorrow's daily checklist without increasing the
seasonal numerator.

The zone journal includes story-only quests as well as daily quests. A one-time
quest appears as story progress until content work gives it a valid replay
contract. A chain step is initially the existing tracked completion definition;
a claim that a whole storyline is complete requires an authored terminal
definition, rather than guessing from dialogue or item ownership.

## Existing systems and findings

| Existing component | Verified behavior | Planned reuse |
| --- | --- | --- |
| Static quest catalog | 2,668 distinct completion definitions from active `areas/AREA` quest sources; stable IDs deduplicate identical giver and turn-in contracts. | Definition identity, catalog generation, original quest execution, and distinct credit. |
| Zone story feature service | Character/PID and season state, frozen completion recipients, telemetry, 25/50/75/100% zone milestones, public completion ranking. | Achievement calculations, private progress, recipient semantics, and evidence collection. |
| Existing daily implementation | Disabled by default; one deterministic assignment per character/day across the world; fixed UTC days; one renown reward. | Feature switch, period calculation, evidence policy, renown balance, and reward deduplication. |
| Quest reward continuation | Retains original completion time, definition ID, zone, party context, and recipient PIDs for recovery. | A committed offering remains the source of completion evidence after restart or delayed acknowledgement. |
| Persistence adapters | A SQL aggregate state document and an atomic flat-file document with a checksum. | Initial storage route, validation, and recovery, subject to the capacity gate below. |

The catalog currently has 253 zone-number values across 220 source areas. Both
catalog builders set every static definition's `repeatable` flag to true, and
both validators require it. This establishes the current tracking scope; it
does not independently prove that every entry is suitable daily content.

An offline audit of the checked-in catalog found 108 contracts with identical
give/receive terms, one contract with no give requirements, and 527 contracts
whose giver disappears. These are review candidates, not automatic exclusions:
some may represent valid quests, and disappearance may be recoverable through
ordinary zone resets.

The audit also found 584 definitions whose current catalog zone number differs
from their source area's first zone header, using 37 catalog numbers without a
matching active source zone header. The current helper uses `giver_vnum / 100`;
an area can span several hundred-blocks. For example, Alatorin's source header
is 831 while some quest givers produce catalog numbers 832 through 835. Confirm
the intended ownership against the booted world before treating this source
audit as a final runtime diagnosis.

Discovery cannot be attached indiscriminately to `char_to_room()`. For example,
`gmcp_quest_map()` temporarily moves the player to draw a remote map and then
returns them. Negative direction flags also occur in legitimate travel paths,
so a blanket exclusion of negative directions would miss real visits.

Primary implementation references:

- [Tracking contract](../reference/ZONE_STORY_QUEST_TRACKING_CONTRACT.md),
  [catalog](../reference/ZONE_STORY_QUEST_CATALOG.md), and
  [existing daily contract](../reference/ZONE_STORY_QUEST_DAILY.md).
- [Feature service](../../src/world/zone_story_quest_feature.h) and
  [runtime adapter](../../src/world/zone_story_quest_runtime.c).
- [Production catalog](../../src/world/zone_story_quest_production.c) and
  [offline catalog builder](../../scripts/zone_story_quest_catalog.py).
- [Static quest execution](../../src/world/quest.c) and the accounting branch's
  `src/item/quest_reward_continuation.h`, which is an unmerged dependency.
- [Room placement](../../src/world/handler.c),
  [movement](../../src/cmd/actmove.c), and
  [remote quest map rendering](../../src/net/gmcp.c).

## Branch dependencies and merge order

This design was prepared from the `experimental-accounting` checkout. A check
against current `origin/master` on October 1, 2026 confirmed that the core zone
story catalog, feature service, achievements, and existing disabled daily system
are already present there. The accounting checkout additionally supplies the
durable quest reward continuation and associated offering/recovery changes
described in this plan. Those additions are dependencies for the proposed daily
receipt and recovery work, not capabilities already available on `master`.

The planning document and its documentation-index entry can merge to `master`
now as a documentation-only change. Use a separate branch based on current
`origin/master` containing only those two documentation changes; merging the
current accounting branch to publish this plan would also bring its unrelated
implementation work.

At the user's request, implementation began in an isolated worktree based on
`experimental-accounting`, with a separate `codex/discovered-zone-dailies`
branch. Integrate accounting first, then update this feature branch to its
accepted base before merging the feature to `master`. The feature commit is
kept separate from accounting qualification. Its new migration and frozen v6
continuation depend on that accounting base.

Catalog audits, content review, and design refinement can continue immediately.
A narrowly scoped discovery or catalog correction could merge earlier if it is
implemented and validated independently against current `master`, but that
would require checking its overlapping changes again when accounting merges.
The full feature should remain a separate change from accounting qualification.

## Recommended rules

| Decision | Proposed initial rule |
| --- | --- |
| Ownership | Per character PID and season, matching existing quest achievements. |
| Discovery | First genuine visit to an eligible world area; walking, following, and legitimate travel spells count. |
| Discovery achievement | A separate completed item within the area's achievement view. |
| Daily availability | All reviewed repeatable definitions in discovered zones; no random rotation or reroll. |
| Daily reset | 00:00 UTC, using the existing `floor(unix_time / 86400)` period. |
| Quest acceptance | Automatic eligibility after discovery; viewing the list is optional. |
| Daily completion | One completion per character, definition, and UTC day. |
| Distinct achievement credit | First completion per definition in the season, using the existing catalog denominator. |
| Extra reward | Initially retain one renown per character per day for the first qualifying daily completion, across all zones. |
| Original rewards | Continue through the existing quest reward authority. |
| Group credit | Retain the frozen recipient set; each recipient must independently meet daily discovery and eligibility rules. |
| Feature default | Keep `ZONE_STORY_DAILY_ENABLED` disabled until the isolated gameplay and persistence checks pass. |

The extra-reward rule is a balance proposal. All qualifying quests still receive
daily checklist credit after the bonus has been earned. Per-quest renown would
require a separate reward schedule and aggregate cap; it should not be an
accidental consequence of expanding from one assignment to thousands of entries.

Discovery does not add a fake quest to the quest-completion numerator or
denominator. A character who enters a zone with ten quests has discovered it
and still has zero of ten quests completed. A zone with no quests can grant its
discovery achievement and display that it has no quest milestones.

Use the existing season boundary for discovery in the first release. A new
account-wide or lifetime discovery rule would be an additional product choice.
New daily status must never reset seasonal progress.

## Catalog and content coverage

Use the actual world zone number as the discovery key. Build a small registry
from `zone_table` that supplies zone identity, player-facing name, and whether
the area is ordinary playable content. Include zones without quest definitions.
Exclude administrative, temporary, and other nonplay areas through reviewed
world metadata or a small explicit content policy, rather than guessing from
display names.

Replace the giver hundred-block shortcut with a shared ownership rule using the
containing world zone range. The offline builder must apply the equivalent rule
from active source zone ranges and cross-check source area ownership. Resolve
ambiguous or cross-area giver ownership explicitly at catalog construction;
moving a giver later must not silently change its quest's home zone. A player
visiting any room in a multi-block area must unlock that area's definitions.

Keep existing definition IDs when only zone ownership or presentation changes.
Changing ownership still needs a deliberate catalog revision and state upgrade
because both persistence adapters reject stale catalog revisions. Preserve
original transaction IDs and audit evidence, and rebuild zone projections from
stable definition IDs. Test pending reward continuations that contain an older
zone mapping; they must retain their original completion fact and have an
explicit compatibility path.

Separate three properties in the catalog: achievement eligibility, actual
repeatability, and reviewed daily eligibility. Update the current validators so
a valid story-only definition can contribute achievement credit without being
forced into the daily pool. Add only the access/prerequisite metadata needed
for the audited content. Daily eligibility should require a valid repeat
contract and reviewed player-facing objective.

Reuse the evidence report's attempt, accessibility, and party checks for daily
review. Telemetry may support level and faction suitability, but a numeric
racewar range does not prove access for every faction in that range. Treat
unknown access or prerequisite conditions as unavailable for automatic daily
recommendation until reviewed. Initial enablement should use reviewed content
with the existing required evidence, rather than lowering thresholds just to
fill a daily list. Successful-completion telemetry alone does not establish
failure rates or quest duration.

The phrase "all quests" needs a coverage inventory. The existing 2,668 entries
are static completion contracts, not a verified inventory of every quest-like
special procedure or every entire storyline. Inventory ordinary zone quests
implemented by NPC, room, and item procedures. Give genuine additional quests
stable catalog identities and narrow hooks at their successful completion
boundaries. Staff events and randomly assigned bartender/world quests retain
their existing lifecycle. A feature is not full coverage until every inventoried
zone quest has either an implemented adapter or a documented content reason it
cannot repeat daily.

## Discovery and daily state

Extend the current service with the following durable facts and projections.

| State | Identity | Required evidence |
| --- | --- | --- |
| Zone discovery | `season, pid, zone_number` | First committed visit time, room VNUM, and visit source. |
| Daily completion | `season, pid, period, definition_id` | Content/policy revision and original completion transaction ID. |
| Daily bonus | Existing `season:pid:period` reward key | Original qualifying transaction ID and awarded amount. |

Keep the content revision on the receipt as evidence, rather than putting it in
the daily identity. Editing a label or changing a compatible revision during
the day must not make the same quest claimable again.

Derive the available list from the reviewed catalog, the character's committed
discoveries, and applicable access/prerequisite rules. Persist completed entries
and bonus receipts, not one empty assignment per quest per character per day.
Viewing `score`, the zone journal, or a daily list should not create assignments
or rewrite the complete state document.

Hook discovery into actual arrival with explicit suppression for temporary
placement used by map/rendering operations. Validate live character identity and
destination after any room procedure that may move or extract the character.
Followed characters count even though activity telemetry suppresses automated
following. Once login or copyover has fully restored a playing character,
reconcile its actual current room idempotently. Temporary inspection and staff
test placements must not grant ordinary-player discovery.

An already-discovered area should exit through an in-memory lookup without a
database write or full-document serialization. Commit a new discovery before
announcing the achievement or enabling its dailies. A failed save must not
block movement, grant a reward, or publish uncommitted discovery; retry the
visit fact through a bounded persistence path or a subsequent real visit/login.

At the original completion boundary, capture the UTC period, season, definition
and catalog revision, applicable daily policy, and recipient-specific daily
eligibility. The existing continuation retains several of these fields, but it
does not freeze the season/catalog revision or every recipient's daily access
context. Extend its versioned terms only for the missing evidence. Do not
recompute eligibility from a changed group, a later discovery, or current
player level during recovery.

Record seasonal credit, each eligible recipient's daily receipt, and the capped
bonus in one durable state mutation. Announce success only after that mutation
commits. Retries must reproduce the same facts. A delayed acknowledgement after
midnight belongs to the original day; replaying yesterday's offering cannot
complete today's checklist. Historical or pre-enablement completions must not
mint new daily rewards.

Use one shared daily predicate for listing and completion. It should establish
that the feature and policy applied at the original completion time, the zone
had been discovered, the definition was active and reviewed at that revision,
the recipient met its access/prerequisite rules, and the authoritative event
credited that PID. Player-specific access and any carry restriction need frozen
recipient evidence, not the direct completer's level reused for everyone.

Reset status by changing the computed period, not by clearing seasonal state
or scheduling a midnight scan over characters. A discovery made during the day
unlocks that area's eligible quests immediately for the remainder of that day.

## Persistence compatibility and capacity

Introduce a versioned state document with a reader for existing version 1
records and an explicit upgrade to the new records. Keep legacy assignments and
reward keys readable; the new implementation stops creating single random
assignments. Cut over at a UTC day boundary so an old assignment and the new
list cannot create two bonus rewards for one day. Both paths share the existing
bonus key.

Add the next immutable migration for the state-version contract and any
necessary storage changes. Do not edit migration `0026`, an applied manifest
entry, or the sealed fresh baseline. Update schema/runtime compatibility and
data lifecycle manifests through the maintained workflow. Rehearse conversion,
revision remapping, pending continuations, and repeated upgrade execution on
disposable data for SQL and flat-file authorities.

Backfill discovery only from trustworthy visit evidence, such as an existing
completion transaction's room resolved to its actual world area. A credits-only
projection, item possession, or viewing a quest name is insufficient proof of
visiting the quest's home zone. If the original discovery time is unknown, mark
it as imported evidence without inventing a date. A character with no visit
history begins with its reconciled current area and subsequent visits. Backfill
never awards a daily bonus.

Reuse the aggregate repository for a bounded pilot, while measuring complete
state size and save latency at the planned daily-completion rate. The SQL adapter
currently caps its payload at 16 MiB and the flat-file adapter at 64 MiB; SQL is
the stricter limit. Existing history and telemetry also grow inside that document,
so compact new daily rows alone do not prove sufficient capacity.

Require a tested growth budget, headroom below the storage limit, and acceptable
game-loop save latency before broad enablement. Keep daily UI projections
bounded, but preserve reward deduplication and protected completion evidence.
If realistic load fails this gate, implement incremental durable records behind
the same service/repository boundaries before broad rollout. Simply increasing
the blob limit would leave whole-document write cost unresolved. This storage
decision is driven by the measured workload, not a speculative new framework.

Character deletion must erase discovery and daily projections across seasons,
retain the required deletion fences, and prevent old continuations from
resurrecting deleted progress. Copyover, rename, and reconnect preserve earned
facts through PID identity. The rollout rollback is to disable new dailies while
using a binary that can still read the upgraded state; reverting to an old
reader is not a safe rollback after version 2 has been written.

## Player commands and presentation

| Surface | Proposed behavior |
| --- | --- |
| `achievements zones` | Discovery summary and existing seasonal quest totals, with private discovered-area detail. |
| `achievements zone <area>` | Discovery achievement, distinct quest progress, existing milestones, and today's completion count when dailies are enabled. |
| `quest zone <area>` | Paginated journal for a discovered area, including daily, completed, locked, and story-only entries. |
| `quest daily` | Short summary of available and completed dailies across discovered areas. |
| `quest daily <area>` | Paginated daily list with giver, useful objective, status, and reset countdown. |
| `score` | A short reminder and existing renown balance. |

Use area names and safe display labels. Resolve unknown or ambiguous names
without defaulting to an unrelated zone. Distinguish unknown quest-state data
from a valid zero-completion result. Undiscovered areas should not expose their
quest breakdown or unlock through a read command. A locked prerequisite does
not disappear from the zone journal, and temporary daily access changes do not
redefine the seasonal quest denominator.

The current production renderer often uses the first dialogue message for all
completion blocks on one giver. Review objectives per definition so a large
daily list does not contain several indistinguishable requests. Preserve the
terminal color/plain-text conventions and paginate large zones; the current
catalog has a hundred-block with 298 definitions.

Retain private area detail and the existing delayed worldwide quest leaderboard.
Discovery achievements must not inflate unique-quest ranking. When the daily
switch is disabled, ordinary quest and score output retain the existing silence
about daily assignments. Discovery and the zone journal remain usable.

Update the tracked quest/achievement help sources and the three zone story
reference documents when implementing these commands. A new website, GMCP
protocol extension, or public discovery leaderboard is unnecessary for the
initial text-client release.

## Implementation sequence

1. **Correct zone ownership and define the content policy.** Produce the source
   and booted-world mapping audit; use one canonical ownership rule; retain
   stable definition IDs; separate achievement, repeat, and daily eligibility;
   specify revision conversion. Complete the inventory of uncovered scripted
   quests. Deliverable: a catalog whose zone keys can actually be discovered,
   with clear review status for each daily candidate.
2. **Implement discovery and its achievement.** Add the world zone registry,
   idempotent discovery facts, real-arrival hooks, temporary-placement
   suppression, restore reconciliation, and per-zone presentation. Complete
   the versioned persistence upgrade and deletion handling. Deliverable: visit
   an area once and retain its discovery achievement through reconnect,
   restart, and copyover, including areas with no quests.
3. **Replace daily assignment with the discovered-zone checklist.** Derive
   available entries, capture missing completion/recipient evidence, persist
   daily receipts and the shared bonus key, implement day-boundary cutover,
   and add the journal/daily command forms. Deliverable: multiple original
   quests can complete daily in an unlocked area without multiplying unique
   achievement credit or the proposed bonus cap.
4. **Qualify a small set of representative areas.** Choose reviewed areas
   covering both factions, solo/group play, a multi-block zone, and a
   disappearing/resettable giver. Use the existing evidence thresholds;
   exercise normal quests, access restrictions, and recovery. Measure state
   growth and persistence latency. Deliverable: a usable isolated-server pilot
   with the broad-enablement capacity decision resolved.
5. **Expand to the full inventoried quest set.** Add the missing completion
   adapters in focused slices and author repeat contracts where appropriate.
   Report remaining story-only content explicitly. Deliverable: all ordinary
   zone quests are represented, and all daily-suitable quests in a discovered
   area are available each day.

Steps 1 through 3 provide the first playable implementation. Steps 4 and 5
establish that it works for the intended world content and daily traffic.

## Validation and acceptance

Extend the existing feature, production-catalog, tracking, flat-file, and schema
harnesses. Add a focused native discovery harness and a gameplay journey that
uses the configured test character on an isolated development server.

| Scenario | Required result |
| --- | --- |
| Enter an area normally, including another hundred-block of the same area | One correctly named discovery achievement and the area's correct quest list. |
| Follow a group or arrive through valid travel | The arriving character earns its own discovery. |
| Render a remote map, inspect remotely, or fail to enter | No discovery or daily unlock. |
| Visit a zone without quests | Discovery succeeds; no quest percentage is fabricated. |
| Complete a new quest in a discovered zone | Seasonal credit and one daily receipt commit together; the first qualifying daily grants the proposed bonus. |
| Complete that quest again, or complete a second quest today | No duplicate for the first quest; the second quest gets checklist credit; the global daily bonus remains one. |
| Complete a quest tomorrow | New daily credit; prior discovery and distinct seasonal credit remain. |
| Discover a new zone during the day | Its dailies become available immediately; prior events do not gain retrospective daily credit. |
| Complete with a group whose recipients differ in discovery/access | Frozen recipients retain existing seasonal semantics; daily credit respects each recipient's captured eligibility. |
| Restart, copy over, reconnect, or replay the offering acknowledgement | Exact earned facts and reward balance remain; no duplicate payout. |
| Commit near midnight and acknowledge after midnight | The original completion belongs to the original day. |
| Change season or catalog revision with pending recovery | Defined upgrade/cutover behavior preserves audit identity and never grants a reward in an unrelated day or season. |
| Inject a save failure or corrupt/stale state | No uncommitted achievement or bonus is published; restoration/retry follows the versioned contract. |
| Delete or rename a character | PID identity, deletion fences, private progress, and ranking remain consistent. |
| Leave dailies disabled | No new daily reward, assignment, or daily heading appears in ordinary quest/score output. |
| Generate a realistic daily workload | State size and save latency pass the capacity budget for both persistence authorities. |

For C/C++ implementation, run changed-line formatting and `make -C src` after
each meaningful slice, then run the smallest relevant executable regressions.
Use the disposable-database migration/schema suites for schema changes. Validate
the native offering-recovery journey when frozen continuation terms change.
Use local evidence to finish repository work rather than waiting for CI.
Production configuration changes and migrations require the owner's permission.

Baseline planning verification: the existing offline catalog contract,
production-catalog coverage regression, and daily evidence report regression all
passed. Implementation adds native arrival and completion rollback/replay,
feature-state migration, append-tail recovery, SQL transactional rollback, and a
6,000-completion retained-history capacity regression. The real offering and
crash journeys validate discovery/daily behavior alongside original rewards.
See the implementation qualification report for final run results.

## Plan simplification

Reuse the current service, stable quest IDs, authoritative offering boundary,
UTC periods, achievement calculations, and backend interfaces. Remove a new
quest engine, acceptance/share/abandon records, random rotation, midnight reset
jobs, and a second reward currency from the initial plan. They do not satisfy
an additional requirement here.

Keep canonical zone ownership, real-visit detection, reviewed repeatability,
recipient evidence, state conversion, and capacity qualification. Omitting any
of those produces a concrete mismatch between discovery, playable quests,
earned credit, or recoverable storage. Lifetime/account aggregation, authored
whole-story replay, richer rewards, and browser surfaces can be separate later
decisions.

The proposed defaults that remain product choices are all daily-suitable quests
versus a rotation, seasonal versus lifetime discovery, first visit versus a
deeper exploration threshold, and the extra-reward cap. This plan uses all
daily-suitable quests, seasonal discovery on first visit, and the existing one
renown daily bonus as its starting rules.
