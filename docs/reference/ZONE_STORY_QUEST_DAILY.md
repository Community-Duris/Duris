# Zone-story daily quests

Daily zone-story quests are shipped disabled. The production default is
`ZONE_STORY_DAILY_ENABLED` unset or false, so ordinary quest completion never
awards daily renown and ordinary players do not see a daily heading, disabled
notice, or empty assignment. Activation is an explicit isolated-server
configuration change after telemetry review; it is not a live rollout switch
to enable casually.

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

SQL-primary deployments apply immutable migration `0026_zone_story_quest_state`
and must pass its verifier. Flat-file-primary deployments use
`<FLATFILE_ROOT>/domains/zone-story-quests.state`; the atomic file includes a
catalog revision and SHA-256 payload digest. Both backends fail closed on a
corrupt or stale state document.

## Player surfaces

Daily output is separate from bartender/random quests and is rendered only
when the feature is enabled and the character has a real assignment or
committed renown. With the shipped default, `score`, `quest`, and `quest daily`
are silent about the feature. When an assignment is present, `score` contains
only a short reminder and the renown balance (when it is above zero); `quest`
or `quest daily` contains the player-facing quest name, area name, giver name,
objective, status, reset countdown, and `Reward: 1 renown`. Definition IDs,
content revisions, completion keys, VNUMs, and numeric zone identifiers are
never shown to players.

Player completion surfaces are:

* `achievements zones` for the overall personal summary;
* `achievements zone <area>` for a private per-area 25/50/75/100% milestone view;
* `leaderboard quests [page]` for the worldwide completion ranking;
* `quest daily` for the assignment/status/reward section.

The public leaderboard shows only characters with at least one distinct quest
completion. Each row leads with the exact count of unique quests completed,
followed by a two-decimal percentage (for example, `2 unique quests (0.07%)`).
Good racewar names are gold, evil racewar names are bright red, unique-quest
counts are cyan, and percentages are white when terminal colors are enabled.
A character with no completion is not yet ranked. Staff/immortal characters are
excluded from public ranking. The list
has no area or quest breakdown; area names and per-area progress appear only in
the current character's private achievement view. Character names are
remembered by PID for durable personal progress, while stale duplicate display
names are collapsed and an unknown name is displayed as `Unknown adventurer`,
never as a PID. Displayed quest totals and percentages may lag actual
completions by up to 12 hours. This publication delay applies to the public
leaderboard; the current character's private achievement view remains current.
