# Ako Village (`ako`) source story dossier

This dossier maps the environmental story and visible NPC/item hooks in the
active zone source. It does not add a quest or claim an outcome that the files
do not implement. The [Checkpoint 181 audit supplement](../quest-story-followups/AKO_AUDIT_REVIEW.md)
records the source graph, cross-area references, title/history review, and
archive disposition.

## Story shape in the authored rooms

### From quiet hills to the village

Ako opens as a country walk. The entrance sign calls it suitable for newer and
lower mid-level characters and warns about food and water. The path passes
birds, insects, trees, a creek, rolling hills, plains, a small desert patch, a
geyser, and abandoned farmland before reaching the settlement
([entrance and sign](../../../areas/wld/ako.wld#L1),
[geyser text](../../../areas/wld/ako.wld#L417)). Wildlife ranges from gentle
birds and small insects to buzzards, vultures, and other hostile creatures.
The special procedures add periodic animal behavior; they do not create an
area quest or resolve a story lead ([Ako mobile procedures](../../../src/specs/specs.ako.c#L14),
[procedure assignments](../../../src/specs/specs.assign.c#L214)).

The contrast is deliberate in the room text: the quiet stream and picnic
clearing give way to a failing farm, a worn dirt road, and a poor settlement
([farm approach](../../../areas/wld/ako.wld#L439),
[road into Ako](../../../areas/wld/ako.wld#L506)). The geyser description says
it erupts every 30 minutes, but the scanned Ako procedure file contains no
geyser event handler. Treat that sentence as authored description until its
runtime behavior is separately confirmed.

### Poverty and daily work

The village is busy but materially poor. The source describes an unfinished
bakery, damaged or abandoned housing, markets, a farmer, beggars, a hungry
child, a branded villager, and a thin vagabond. Market and resident text gives
the settlement a daily life, while the descriptions of poverty and exclusion
suggest that the community is under strain ([village entry and market rooms](../../../areas/wld/ako.wld#L540),
[branded villager](../../../areas/mob/ako.mob#L424),
[hungry residents](../../../areas/mob/ako.mob#L335)).

The room descriptions call several NPCs vendors, but Ako has no local `.shp`
file. The files reviewed therefore provide no local shop inventory to connect
to those vendor descriptions. The area also has no local QST file and no story
sidecar; no accepted delivery, purchase, dialogue, or journal outcome is
defined in Ako's native quest/story files.

### Council, trial, and executions

The strongest story is the village's treatment of people accused of
witchcraft. The council room leads to a chamber where the source calls the
accused innocent, a detention room holding condemned people, a burning ground
with people being burned, and a nearby child crying for his mother
([council room](../../../areas/wld/ako.wld#L776),
[trial and detention rooms](../../../areas/wld/ako.wld#L809),
[burning ground](../../../areas/wld/ako.wld#L793),
[weeping child](../../../areas/mob/ako.mob#L590)). The prose presents a clear
moral conflict, but no local Q contract, mapped special procedure, or reset
transition records an investigation, release, judgment change, or aftermath.

Two authorities appear: the armed council leader at the trial and the village
leader described as a figurehead in a comparatively comfortable home. The
source does not explain their relationship ([council leader](../../../areas/mob/ako.mob#L551),
[village leader](../../../areas/mob/ako.mob#L603),
[leader's home](../../../areas/wld/ako.wld#L859)).

## Reset-loaded story objects

Ako's six local objects are placed as ordinary reset stock, not as recorded
quest rewards:

- The farmer uses a local garden hoe near the abandoned farm.
- The village grandpa carries a crooked, softly glowing cane in the market.
- The village grandma carries a softly glowing knit scarf.
- The council leader carries a long, thin blade in the trial chamber.
- Guards use a local jacket and stone dagger; the same guards also receive
  Heavens object #283, a steel long sword.

The local reset lines attach the cane to grandpa and scarf to grandma
([cane reset](../../../areas/zon/ako.zon#L113),
[scarf reset](../../../areas/zon/ako.zon#L130)); the local object
descriptions are at [cane](../../../areas/obj/ako.obj#L46),
[scarf](../../../areas/obj/ako.obj#L63), and
[trial blade](../../../areas/obj/ako.obj#L80). The external sword import is
documented in the [audit supplement](../quest-story-followups/AKO_AUDIT_REVIEW.md#typed-cross-area-references).

An invitation-like note prototype #22808 exists in
[`newbie2.obj`](../../../areas/obj/newbie2.obj#L128), with text about bringing
it to its author for an amulet. The Plains of Life review found
no active reset, quest, or typed story reference that makes it an Ako route;
its [existing note](PLAINS_OF_LIFE.md#L61) treats it as an unintegrated lead.
Do not describe the note or an amulet as obtainable through Ako based on the
current source.

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Is the witchcraft trial meant to be an unresolved tragedy or a player-facing story? | The rooms assert innocence, confinement, and burning, with a separated child; no local outcome state or QST is present. | If it is intended as an adventure, define evidence, player choices, release/judgment outcomes, and consequences before adding a quest. If it is scenery, make sure descriptions do not promise an actionable rescue. |
| Are the two leaders meant to share power? | The council leader conducts the trial, while the other leader is called a figurehead. | Clarify the distinction through source dialogue or room text so the authority behind the executions is legible. |
| Are the cane, scarf, and note related? | The cane and scarf are reset on elders and have magical-glow descriptions; the note is in another area's prototype set and has no active Ako consumer. | Confirm whether they are flavor/equipment or an intended story chain. Keep each item an ordinary prop until a producer, recipient, and outcome are specified. |
| Does the geyser actually erupt on a timer? | The room description promises a 30-minute eruption; the scanned area procedure file has no geyser handler. | Mark it as flavor or identify the actual intended event owner before promising a timed interaction. |
| Is the village's intended arrival path active? | Ako exits west from #3751 to Surface #544136, but the active graph has no WLD edge into Ako. One inbound-looking Surf exit exists only in an inactive source file. | Decide whether to provide a current inbound route and return route, then qualify it in the loaded map before calling the village ordinarily reachable. |
| Do the elder combat templates fit the sign's suggested audience? | The entrance sign advertises newer/lower mid-level play, while grandpa and grandma have exceptional numeric combat fields in the mobile source ([grandpa](../../../areas/mob/ako.mob#L361), [grandma](../../../areas/mob/ako.mob#L373)). | Confirm those source fields against the engine's stat meaning and intended protection role before changing any balance. This static review makes no encounter-safety claim. |
| Should `ako` have a command-target alias beyond the unique ZON title “Ako Village”? | The active area key is `ako`; normalized title review found “Ako Village” unique among 442 current ZON titles and no lexical title candidates. No separate command-target alias is established in the reviewed area sources. | Keep `ako` and “Ako Village” as the registered key and display identity. Add another alias only after its exact target and intended distinction are confirmed by the owner. |

Until those choices are made, preserve the existing room and NPC prose as
source evidence, do not invent a rescue, delivery, or amulet reward, and keep
Ako in the active map. The technical archive recommendation is in the
[Checkpoint 181 audit supplement](../quest-story-followups/AKO_AUDIT_REVIEW.md#migration-disposition).
