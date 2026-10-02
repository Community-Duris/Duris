# Starter and hometown story coverage

The native `avail_hometowns` and `guild_locations` tables determine possible
character-creation homes and their real default/class starting rooms. The
`ZONE_TOWN` flag determines areas where `home` can work, subject to existing
character, room, faction, and accounting rules. Cover the union of these
sources so changing creation availability does not remove journal coverage.

`python3 scripts/zone_story_quest_home_coverage.py --check` verifies all 27
playable required areas have sidecars. It also checks that creation locations
exist in active room sources. The End of the World is a file/index sentinel,
not a playable town; its `$~` entry is excluded. Ailvio and the Plains of Life
are the two selectable introduction areas. Race/class switches can narrow
creation choices; this audit intentionally covers the native union.

Every listed sidecar uses schema 2 or 3 and classifies every native Q contract in
its area. Request checklists use exact item counts and authoritative receipt
terms. They are a mechanical journey baseline; they do not certify every
dialogue dependency, item source, custom special, or lore interpretation.
Areas with no native exchanges retain orientation and verified conversations.
Those conversations do not invent terminal quest achievements.

Contacts are revealed only after an identifiable NPC was physically available
to meet. The first discovery announces the zone and exact journal command;
first meetings and first receipt progress prompt the journal. The first
outstanding checklist step is marked `Next:`. NPC encounter history survives
reconnect/restart and is never backfilled from a zone visit or group credit.

## Maintained register

Update the sidecar and this register when integrating a zone. `Q` is the raw
deduplicated native contract count; requests/stories and services are projected
units; exclusions retain native execution and receipt history. A creation entry
names the native menu home whose starting rooms are owned by the area.

| Area / sidecar | Creation homes | Town flag | Q | Requests / stories | Services | Excluded Q | Contacts |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| [Braddistock Mansion](../../areas/story/braddistock.story.json) | — | Yes | 2 | 1 | 1 | 0 | 2 |
| [Fort Marigot](../../areas/story/marigot.story.json) | Marigot | Yes | 0 | 0 | 0 | 0 | 1 |
| [Ghore](../../areas/story/ghore.story.json) | Ghore | Yes | 0 | 0 | 0 | 0 | 0 |
| [Faang](../../areas/story/faang.story.json) | Faang | Yes | 0 | 0 | 0 | 0 | 0 |
| [Woodseer](../../areas/story/woodseer.story.json) | Woodseer | Yes | 7 | 7 | 0 | 0 | 7 |
| [Khildarak Stronghold](../../areas/story/khildarak.story.json) | Khildarak | Yes | 2 | 2 | 0 | 0 | 3 |
| [Githyanki Hometown](../../areas/story/gith_ht.story.json) | Githyanki Hometown | Yes | 0 | 0 | 0 | 0 | 0 |
| [The Plains of Life](../../areas/story/newbie2.story.json) | Plane of Life (Beginner Introduction) | No | 0 | 0 | 0 | 0 | 1 |
| [Ailvio, Duris Newbie Outpost](../../areas/story/newbie.story.json) | Outpost of Ailvio (Secondary Introduction) | No | 116 | 22 | 17 | 0 | 32 |
| [The Mountain Settlement of the Harpies](../../areas/story/harpyht.story.json) | Harpy | No | 3 | 3 | 0 | 0 | 3 |
| [Llzazan Ghetto of Arachdrathos](../../areas/story/llzazan.story.json) | Arachdrathos | Yes | 0 | 0 | 0 | 0 | 0 |
| [Arachdrathos - Drow City](../../areas/story/arac-web.story.json) | — | Yes | 1 | 0 | 0 | 1 | 1 |
| [Nax](../../areas/story/nax.story.json) | Nax | Yes | 0 | 0 | 0 | 0 | 0 |
| [Ugta](../../areas/story/ugta.story.json) | Ugta | Yes | 0 | 0 | 0 | 0 | 30 |
| [Charing](../../areas/story/charing.story.json) | Charing | Yes | 0 | 0 | 0 | 0 | 0 |
| [Orog Encampment](../../areas/story/orogs.story.json) | Orog Encampment | Yes | 0 | 0 | 0 | 0 | 0 |
| [The City of Winterhaven](../../areas/story/wh.story.json) | — | Yes | 221 | 135 | 84 | 2 | 91 |
| [Ashrumite Village](../../areas/story/ashrumite.story.json) | Ashrumite | Yes | 12 | 8 | 0 | 4 | 4 |
| [Graendiae](../../areas/story/sea.story.json) | — | Yes | 0 | 0 | 0 | 0 | 0 |
| [Taliccicopiid](../../areas/story/thri.story.json) | Payang | No | 0 | 0 | 0 | 0 | 0 |
| [The Town of Moregeeth](../../areas/story/goblinht.story.json) | Moregeeth | Yes | 11 | 7 | 0 | 4 | 8 |
| [Tunnels of Payang](../../areas/story/rtun.story.json) | — | Yes | 0 | 0 | 0 | 0 | 0 |
| [Kimordril](../../areas/story/kimordril.story.json) | Kimordril | Yes | 4 | 4 | 0 | 0 | 2 |
| [Ixarkon](../../areas/story/ixarkon.story.json) | Ixarkon | Yes | 3 | 3 | 0 | 0 | 3 |
| [Shady Grove](../../areas/story/shady.story.json) | Shady | Yes | 3 | 3 | 0 | 0 | 4 |
| [Tharnadia - City of Humans](../../areas/story/tharnadia.story.json) | Tharnadia | Yes | 19 | 11 | 5 | 3 | 14 |
| [Braddistock Mansion](../../areas/story/brad.story.json) | — | Yes | 5 | 5 | 0 | 0 | 5 |

## Reviewed journeys and boundaries

- Braddistock Mansion combines the key, pet rescue, recovered collar, and final
  owner delivery into one story. The intermediate rescue receipt remains visible
  in the checklist without adding another terminal achievement.
- Ailvio covers class-teacher deliveries, recipes, and multiple material counts.
  Its known herb/ingredient exchanges appear as one available preparation route,
  rather than enforced personal sourcing. Map/starter-equipment exchanges are
  services. Questions and class restrictions still belong to the native systems.
- The Plains of Life follows tutorial signs and the scripted paladin's `racewars`
  topic. The paladin is revealed only after meeting him. His guidance explains
  `enter stream` and the transition to Ailvio. The legacy newbie tag, blessing,
  sword, and stream gate still execute in `specs.newbie2.c`; this release records
  meeting the NPC, not a new tutorial-completion receipt.
- Winterhaven's reviewed dyeing, plain weapon/shield crafting, jewelry, tanning,
  silk clothing, and thread trades are services. Its other contracts retain
  exact delivery checklists while narrative grouping and source chains await
  deeper review. Tharnadia's plain-weapon improvements are likewise services.
- Unsupported mixed payments and item-type offerings stay unavailable under
  the existing accounting path. Do not interpret a journal row as evidence that
  the economic engine can execute those terms.
- Five source contracts reference missing active object prototypes: three in
  Ashrumite, one in Tharnadia, and one in Winterhaven.
  They are explicitly excluded until world data is repaired. The maps retain
  the canonical native bindings for audit; no missing item is fabricated.

Twin Towers is additionally mapped in
[`twin_towers_forest.story.json`](../../areas/story/twin_towers_forest.story.json).
Its revision 2 adds encountered contacts to the earlier ten reviewed stories,
including the waist-equipped gardener belt barrier, plants, arrows, distinct
feathers, and clothing materials. Dialogue milestones and personal animal/plant
provenance remain in the [integration plan](../design/ZONE_STORY_INTEGRATION_PLAN.md).

## Qualification and next integration pass

The source audit validates sidecar fields, exact canonical contract ownership,
complete Q classification, item prototypes, NPC prototypes/aliases, and coverage
of all required creation/town areas. The native harness parses every shipped
mapping and verifies that discovery hides contacts while real encounters reveal
them. Domain/arrival tests cover failures, old state, malformed encounters,
read-only inventory checks, and restart. The isolated native player journey
covers discovery, a later physical meeting, guided checklists, actual offerings,
one daily bonus, and a cold reconnect.

These checks do not certify every full-world quest route. Qualify each area by
playing its complete journey as its mapping is refined. Add reviewed adapters
for dialogue learning, scripted milestones, transformations, and item provenance
before claiming those historical objectives. Maintain stable story IDs across
prose/hint revisions and preserve all native receipt IDs.
