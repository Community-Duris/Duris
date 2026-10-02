# Production zone-story quest catalog

The checked-in [production catalog](ZONE_STORY_QUEST_PRODUCTION_CATALOG.json) is
built from active static quest files listed in `areas/AREA`, following the area
compiler's Q-block boundary. Revision 2 contains 2,668 stable contracts, of which
2,659 are eligible for zone achievements and 2,133 are daily content candidates.
Its area registry has 350 entries, with 349 positive area headers. Actual
playable rooms can award discovery without contributing a quest denominator;
the end-of-world sentinel has no playable room and earns no discovery.

Optional `areas/story/<area>.story.json` mappings project raw contracts into
named stories without changing native receipt IDs. Twin Towers replaces 84
contracts with 10 achievement units and 3 daily groups, excluding 40 returned
offerings and displaying 24 supporting contracts as 12 non-achievement services.
The global projection now has
2,359 achievement units and 1,962 daily candidate units after adding all 27
starter/town mappings and eight further journals, equipment/crafting services,
intermediate outcomes, and explicit missing-item exclusions. There are 36
area sidecars and 2,509 projected units including services and administrative
content. Other areas
retain native-contract fallback; these counts do not claim full semantic review.
Ailvio now projects its 78 equivalent two-fish recipes into one family-feeding
story, leaving 22 achievement rows and 17 services. Braddistock displays its
intermediate pet rescue as a service while preserving one final achievement.
Breale preserves its Passage drawing riddle and independent Triad rewards.
The Homestead now displays optional access/statue preparation and explains the
two spider keys, while retaining two achievements and two services.
Pine Hollow retains seven independent achievements with exact source/material
guidance; its source dossier records roaming proof availability, competing
consumers, the coat supply-cap conflict and the foreign eye route.
Quietus now has optional briefing receipts and exact proof/reward guidance,
four mission achievements and seven supporting services. Its comprehensive
dossier records all topic families, rare-load availability, filled-badge
replacement and distinct world-quest/ship service settlement gaps.
Torg retains twelve achievements and two services across fifteen contracts.
Optional curing, rose and locket receipts preserve supplied-material routes;
its dossier explains all topic families, actual speech/key access, distinct
rings/chisels, eight legends and three separate Tranug commissions. Invasion
arrival, holding availability, direct random legend/heart grants and owned
foreign terminals remain separate qualification work.
Vast Hidden Grove retains three equipment stories, two bird requests and ten
services across fifteen contracts. Optional specialist receipts preserve
supplied ingredients; exact small pickaxe, raw versus lavender thread,
competing ingredient copies and two-scroll reward are explained. Its dossier
records thirteen scenery routes, loaded door/falling behavior, Valin/miner
availability, ordinary mouse pickup and an invalid inn assignment target.
Three mixed fees remain unavailable; item-only coin rewards retain native
accounting. Twelve source-comprehensive areas remain distinct from played
active-world qualification.
All raw definitions, receipt identities, zone registry and source fingerprint
remain unchanged. Comprehensive source dossiers for these areas are tracked in
the [execution register](../design/ZONE_STORY_ROADMAP_EXECUTION.md).
The [priority roadmap](../design/ZONE_STORY_ZONE_PRIORITIES.md) and
[complete active inventory](ZONE_STORY_ZONE_INVENTORY.md) distinguish reviewed
story proposals from provisional static candidates. Player journals and new
discovery/encounter/daily eligibility require active economic accounting.
The [starter/town register](ZONE_STORY_STARTER_HOMETOWN_COVERAGE.md) identifies
the native selection sources, exact mapped areas, and remaining journey work.
See the [builder guide](../guides/ZONE_STORY_BUILDING.md) and
[integration register](../design/ZONE_STORY_INTEGRATION_PLAN.md).

## Ownership and identity

Area ownership follows sorted active zone header ranges (`previous.top + 1`
through `top`), matching the booted world. Multi-block areas are one canonical
area: Alatorin's 495 definitions belong to area 831 through room 84055. Runtime
and offline builders use those ranges, including givers outside their area's
first hundred-block. Administrative area zero remains catalogued for normal
quest execution but does not contribute discovery, daily candidates, or player
quest percentages.

Each definition ID is derived from giver VNUM and canonical sorted
`give/receive/disappear` terms. All revision-1 stable IDs are retained. Identical
contracts at the same giver are deduplicated regardless of prose/order; goal
changes create different identities. Revision-1 completion facts retain their
historical zone and revision while current progress projects their stable IDs
onto the corrected registry. Player output uses area/giver/objective text and
never exposes definition IDs, revisions, keys, or VNUMs.

## Daily content review

A contract can be a daily candidate when its owning playable area is valid,
it has a positive item offering, it can repeat through ordinary gameplay/reset,
and it does not return an offered item. A disappearing giver in a resettable
area can repeat; disappearance in a non-resetting area is story-only. The
snapshot records `daily_eligible`, `daily_exclusion`, and `prerequisites`.

| Exclusion | Contracts | Treatment |
| --- | ---: | --- |
| Administrative content | 9 | Original execution; no player achievement/daily contribution. |
| Returned offering/item exchange | 131 | Story tracking; no daily bonus. |
| No replay through normal reset | 70 | Story-only progress. |
| No positive item offering | 39 | Story tracking; no daily candidate. |
| Unsupported native offering shape | 286 | Story tracking; no daily candidate until the accounting turn-in path supports the contract. |

Daily candidates must also pass observed level, exact faction, party, access,
and prerequisite checks at admission. Suitability metadata does not promise
availability to every character. Existing static contracts declare no separate
prerequisite IDs beyond their actual item requirements. A whole-story terminal
achievement needs authored content; the tracker does not infer one from prose.

The current accounting offering context supports at most 14 exact item goals.
Three otherwise suitable contracts exceed that bound; another 283 use mixed
currency or item-type goals that this native path rejects. Both the runtime
catalog and offline snapshot exclude these from dailies. The full story journal
still lists them. This feature does not extend the accounting offering engine.

## Scripted-content inventory

Supported native Q turn-ins are connected to authoritative completion and recovery,
including quests in areas that also use specials. The following quest-like
specials were reviewed separately; none supplies another durable ordinary
Q-contract completion that can safely be added as a daily turn-in.

| Special/source | Observed behavior | Coverage decision |
| --- | --- | --- |
| `smelter`, `finish_smelt` in `specs.alatorin.c` | Paid ore conversion/trade; unavailable under active accounting. | Trade, no daily completion or bonus. |
| `monk_remort` in `specs.mobile.c` | Paid permanent class conversion. | Character transformation, no replayable daily. |
| `berserker_proc_room` in `specs.room.c` | Permanent battlerager transformation and skill reset. | Character transformation, no replayable daily. |
| `fooquest_mob`, `fooquest_boss`, `newbie_quest` in `specs.fooquest.c` | Staff-run transformation, encounter combat, event behavior. | Encounter/event mechanics; no invented quest terminal fact. |
| `akckx` and related HoA specials in `specs.hoa.c` | Death spawns, components, encounter mechanics. | Existing Q turn-ins are tracked; spawns are not completion receipts. |
| Pirate/room/item specials in `specs.raxquest.c` | Encounter, pet, and environmental behavior. | Existing Q turn-ins are tracked; no extra daily completion event. |
| Bartender/random world quests | Generated assigned-run content with separate rewards/resets. | Existing world quest system; outside the static zone-story denominator. |

This inventory does not turn repeated speech, item creation, or an encounter
spawn into a rewarded accomplishment. New scripted quests can join the tracker
by declaring a stable definition, replay/prerequisite contract, and durable
terminal event with frozen recipients through the existing completion API.

Regenerate and validate the snapshot with:

```text
python3 scripts/zone_story_quest_catalog.py \
  --source-root . --content-revision 2 \
  --production-output docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json --check
```

Runtime construction follows `boot_the_quests()` and validates the registry
and definitions before use. The snapshot coverage test compares all active Q
contracts and canonical owners, including area 831. A missing/invalid catalog
fails closed for tracking; an empty eligible quest set renders N/A without
suppressing the separate discovery view.
