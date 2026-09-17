# Zone-story daily quests

Daily zone-story quests are shipped disabled. The production default is
`ZONE_STORY_DAILY_ENABLED` unset or false, so ordinary quest completion never
awards daily renown and the player-facing daily section says that the feature
is disabled. Activation is an explicit isolated-server configuration change
after telemetry review; it is not a live rollout switch to enable casually.

## Evidence and policy

The runtime records stable observations with an observation ID, quest definition
ID, content revision, UTC timestamp, PID, level, racewar context, credit mask,
party context, strongest party level, duration, outcome, and accessibility. The
same observation ID is idempotent and cannot be reused with different values.

The maintainable offline report is:

```text
python3 scripts/zone_story_quest_daily_report.py \
  --observations /path/to/zone-story-observations.jsonl \
  --format text
```

It reports observed attempts, distinct observed PIDs, observed outcomes, level
and racewar ranges, access/party-context checks, and the exact configured
policy. It does not invent a player-population denominator or extrapolate
success from missing telemetry. Unmatched or stale revisions are never turned
into an assignment candidate.

The default suitability policy requires at least 20 observed attempts from 5
distinct PIDs, at least one observed success, a level range beginning at 1,
accessible evidence, no stale-revision observations, and no inaccessible
attempts. It also requires known party context and rejects an observation when
the strongest party member is more than 10 levels above the completing
character. A low-level completion while being carried therefore cannot establish
daily suitability. The policy can be made stricter in an isolated test report;
the runtime never fabricates missing party context.

## Assignment and completion contract

Assignments use one fixed UTC period (`floor(unix_time / 86400)`) per PID and
season. The assigned definition and revision, period expiry, status, reward,
and authoritative completion transaction ID are persisted. A suitable
candidate is selected deterministically from sorted evidence-backed definitions;
there is no reroll path. If no candidate is suitable, the period is persisted as
`no_eligible_candidate`.

Only an authoritative post-assignment completion of the exact assigned
definition and revision, inside the period, can award one renown. The durable
reward key is `season:pid:period`, so replaying the completion, restarting,
copying over, or reconnecting cannot award a second renown. A repeat completion
may remain in history, but it never increments the zone numerator again.

SQL-primary deployments apply immutable migration `0025_zone_story_quest_state`
and must pass its verifier. Flat-file-primary deployments use
`<FLATFILE_ROOT>/domains/zone-story-quests.state`; the atomic file includes a
catalog revision and SHA-256 payload digest. Both backends fail closed on a
corrupt or stale state document.

## Player surfaces

The daily section is separate from bartender/random quests and is visible from
`quest daily`, the score output, and the quest help path. It includes the
current renown total, assignment zone/giver/objective metadata, reward, and
expiry. It is also shown when
the legacy bartender quest command takes an early return. Player completion
surfaces are:

* `achievements zones` for the overall personal summary;
* `achievements zone <zone>` for a per-zone 25/50/75/100% milestone view;
* `leaderboard quests [zone] [page]` for exact per-PID completion rankings;
* `quest daily` for the assignment/status/reward section.

Names are remembered by PID and offline rows render as `PID <number>` until a
login or other authoritative identity refresh supplies the current name.
