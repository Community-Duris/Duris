# Zone-story quest tracking contract

Tracking is per character PID and season. It covers stable static zone/story Q
contracts; bartender/random world quest assignments have their own lifecycle.
There is no acceptance, sharing, abandonment, or midnight world-reset record in
this tracker. An original quest turn-in supplies the authoritative completion.

## Definitions and discovery

A definition has a stable identity, canonical owning area, source, completion
contract, content revision, and explicit repeatability/daily metadata. Identical
giver/turn-in contracts are alternative routes to one distinct accomplishment.
A genuine arrival records the area's discovery achievement independently of
quest completion. Discovery opens the private journal even in an empty area;
it never adds a quest to the denominator. The registry includes areas with no
Q blocks and uses booted zone ranges rather than `giver_vnum / 100`.

Seasonal quest percentage is:

```text
distinct active eligible definitions credited to this PID in this season
/
active eligible definitions in this area at the current catalog revision
```

The first completion of a definition contributes once. Later completions remain
history. One-time definitions can contribute story credit without becoming
repeatable daily content. Catalog revision 2 preserves revision-1 stable IDs
while correcting area ownership; old transaction audit fields are not rewritten.

## Completion credit

- `PERSONAL`: direct completer.
- `SOLO`: the transaction had exactly one credited PID.
- `GROUP_PARTICIPANT`: a credited recipient on a multi-recipient transaction.
- `LEADERSHIP`: the direct completer on a multi-recipient transaction.

Recipients are captured at completion time from eligible same-room characters.
Recovery never scans the current group to reconstruct that set. Account-wide or
lifetime aggregation is outside the current storage contract. Staff characters
are excluded from player discovery, daily eligibility, and public ranking.

Transaction schema 1 remains readable. Schema 2 adds the daily policy revision
and a unique subset of the credited PIDs that passed daily eligibility at the
original turn-in boundary. V6 native reward continuations additionally freeze
season and catalog revision. The original completion time determines the UTC
day. Stable offering identity supplies the transaction ID for recovery.

The same transaction ID and payload is idempotent; reuse with different terms
is a conflict. A committed daily receipt marks that definition's daily entry
and grants at most one renown using `season:pid:period`. No daily credit is
inferred from a later eligibility check or read command. Existing normal item,
coin, skill, and XP rewards continue through the accounting reward obligation.

## Durable state

Feature state `ZSQF|3` persists completion facts, identities, credit masks, first
completion times, discovery, daily-period projections, reward keys, telemetry,
NPC encounter records, and deletion/exclusion markers. A single cutover marker bounds legacy daily
conversion. V1 documents load into a candidate service, preserve stable credit,
backfill only proven completion-room visits, and start dailies next UTC day.
Parsing or semantic failure leaves live state unchanged.

V2 documents still load without manufactured NPC meetings. Encounter `M`
records retain season, character, prototype VNUM, physical room, and first
time. The first delta writes the V3 header in the same atomic batch. SQL bucket
schema and flat-file framing versions remain unchanged; an older binary needs
its pre-upgrade state restored for a planned rollback.

Changed records are persisted atomically through SQL bucket transactions or
flat-file journal frames. Completion and discovery mutate bounded recipient
state; failed writes restore that state and its telemetry/dirty-record index.
Reads never persist. Erasure produces a full filtered snapshot, retaining other
recipients' private facts and original first completion times for public delay.

See [daily policy, persistence, and player commands](ZONE_STORY_QUEST_DAILY.md)
and [production catalog coverage](ZONE_STORY_QUEST_CATALOG.md) for operational
and content details.
