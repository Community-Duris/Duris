# Standalone zone journals on master

Status: implementation on `codex/zone-journals-standalone`, based on master
`2a1604e4eedf55034c8183c5cfc28cb683ef88cd`. This document is the current runtime
contract for this port. Historical accounting-branch research remains useful
source evidence, but its accounting activation requirements and prior repair
claims do not describe this branch.

Review: [draft PR #724](https://github.com/Community-Duris/Duris/pull/724).
Implementation checks pass; the live zone-by-zone review remains open.

## What ships

- Discovery on genuine mortal arrival, private journals, visible NPC encounters,
  seasonal story achievements, and optional discovered-zone daily checklists.
- 232 preserved mappings plus a mapping for Vella's Bordello, including its
  paid temporary follower services and connections to Winterhaven's quests.
  The native catalog contains 2,668 contracts; 1,406 grouped daily candidates
  survive the existing conservative selection policy. Candidate status does not
  guarantee a reachable recipient or renewed materials.
- Optional `areas/story/<area>.story.json` files. Native `.qst`, `.zon`, `.mob`,
  `.obj`, `.wld`, `.shp`, special assignments, costs, rewards, routes and resets
  remain those of the base master commit.
- Existing SQL or flatfile persistence, independent of the experimental economic
  accounting lifecycle. Journals do not require accounting to be installed or
  active. A future accounting integration can supply an idempotent completion ID
  through the existing completion API; it must not gate ordinary journal access.
- Story grouping, alternatives and preparation checklists. Carried/worn checks
  describe current possessions. They do not prove first acquisition, who killed
  a source, or whether the item was looted, traded or gifted. Native acceptance
  remains sufficient for completion; optional preparation never adds a native
  prerequisite. Dialogue aliases are prompts, not one achievement per keyword.

`quest zone <area>` opens a discovered journal; `achievements zones` and
`achievements zone <area>` show discovery and story progress. Contacts appear
through observed encounters. Dailies stay off by default. Setting
`ZONE_STORY_DAILY_ENABLED=true` enables their independent policy, still subject
to discovery, cohort/access evidence and party limits. It adds journal renown,
not extra native gold, experience or item rewards. `ZONE_STORY_SEASON_ID` selects
the tracking season. No live setting is changed by this branch.

## Compatibility and failure boundaries

The normal native quest bootstrap loads journals. The development `no_specials`
boot mode retains master's behavior, including no journal bootstrap.

A missing sidecar retains the native-contract fallback. A malformed or stale
sidecar is quarantined for its area, with a diagnostic: no invented fallback
achievement/daily classification is substituted for that area's reviewed map.
Other areas continue loading. Native contract identities and completion records
remain available. Builder validation stays strict and rejects the bad file.

Invalid identity/ownership metadata or an unusable store can disable tracking;
this does not veto the original native quest exchange. Arrival, encounter and
native-completion observation catch unexpected exceptions and log failure.
Normal storage errors restore the tracking checkpoint. Native-completion failures
return diagnostics to the existing quest observer; failed discovery/encounter
saves remain eligible for a later attempt.

**Journal records and native rewards are not one atomic transaction on master.**
The pre-existing native observer runs in the accepted-hand-in path before rewards.
A crash between the two can leave a recorded hand-in without its later reward;
a failed journal save can leave a successful native exchange unrecorded. No
standalone durable retry/outbox or reward replay is claimed. Adding one requires
a separate design that cannot repeat rewards or introduce a gameplay gate.
Do not describe this port as economic settlement or full acquisition provenance.

Migration `0032_discovered_zone_daily_state` follows master's unchanged 31-entry
history. It broadens only the existing journal table's CHECK constraints for v2
bounded record buckets. It preserves v1 rows and applies idempotently; application
conversion follows a validated load. Existing column/index fingerprints and the
204-table inventory are unchanged. Flatfile state converts through its existing
locked persistence path. Back up state before rollout: an old binary does not
understand a converted v2 document. Rollback needs a compatible journal reader or
restoration of the pre-port journal backup, not merely reversal of the CHECKs.

## Research preservation

The original feature source is commit `2d6f5c96083351f0234b06be7c80bc7c87d6fb20`.
The independent research checkout was snapshotted from commit
`c500ac51b8cd26540943257e7ca4e9807263ef74` including its then-uncommitted documents.
[PORT_SNAPSHOT.json](quest-story-followups/PORT_SNAPSHOT.json) records 980 paths
and hashes. Both original worktrees remain untouched. Historical dossiers contain
proposals, hypothetical progression stories and accounting-specific limitations;
none is permission to change the native world. The snapshot is a point-in-time
copy, not a claim that the separate research chat has finished every investigation.

No native zone fixes are included in this port. Prior fixes remain on the original
branch. Any future zone repair must have its own named fix commit, supporting
source/journey evidence and a clear PR/news entry, separate from journal copy.

## Sequential journal review

Follow [the starter review](ZONE_STORY_STARTER_REVIEW.md): Turolopolis and its zoo,
then Vella's Bordello, then Harrow. Each review checks room and character prose,
actual offerings/rewards, item sources, travel, reset scarcity and alternative
acquisition. Source review and a passing catalog are not a completed live journey.
Record the player decisions, reproduce disputed paths, and only then promote
unresolved research into a supported journal claim.

After these, review Woodseer first: its seven independent errands have concrete
items and rewards. Then consider Kobold Settlement's silver commissions and gem
spectacles, followed by the Town of Moregeeth. These are candidates to inspect,
not a promise about their level or access. Confirm entry routes, source combat
levels and material renewal before choosing the next player test.

## Remaining universal work

1. Run mortal gameplay journeys for each reviewed area, including supplied items,
   multi-reset outcomes and reconnect persistence; keep results with the dossier.
2. Add an optional native event adapter for custom scripted outcomes only after
   identifying the actual success point. Never infer completion from prose or a
   command merely being handled.
   The first concrete integration is Vella's three paid follower services:
   observe the established pet-owner link and follower after the accepted payment.
   Their current journal explains the service but does not yet persist a hire
   completion. See the [Vella review](ZONE_STORY_STARTER_REVIEW.md#2-vellas-bordello-bordello)
   for exact hooks and acceptance cases.
3. Design acquisition provenance and first-obtained/worn milestones independently
   of accounting, with stable event identities and clear source-versus-gift rules.
   Current item readiness is intentionally not historical provenance.
4. Add explicit builder-authored access hints and optional milestones for hidden
   routes and equipment gates. Do not infer mandatory dependencies from shared
   object IDs; traded materials and multiple reset visits must continue to work.
5. Review conservative daily exclusions (mixed fees, large offerings, services)
   separately from journal availability. Do not remove a native cost to qualify a
   quest, and do not promise daily renewal from reset metadata alone.
6. Design failure recovery for the observer without making native quests depend
   on journal availability. Keep economic accounting an optional integration.
7. Add builder-authored references to quests completed in other zones. Vella's
   supplies Incarnate and psychomia materials for Winterhaven's requests; the
   current maps explain this through guidance and ingredient hints. A future
   shared checklist should reuse the owning quest's completion, preserve
   encounter/discovery visibility, accept supplied materials and avoid duplicate
   achievements or daily credit. Do not move ownership to the material source.

Validation evidence for this port is recorded in
[the standalone qualification record](ZONE_STORY_STANDALONE_QUALIFICATION.md).
