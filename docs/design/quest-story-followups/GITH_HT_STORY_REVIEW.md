# Githyanki Hometown (`gith_ht`): source story review

**Scope:** source review of 91 WLD rooms, 52 mobile prototypes, 42 local
object prototypes, one `.shp` record, zone resets, and the authored story
sidecar. This maps static source facts and dependencies; it does not qualify
live shop, guild, special, route-safety, or ordinary versus staff behavior.

## Story coverage state

`gith_ht` is active in `areas/AREA` as zone `194`, with 91 rooms, 52 mobiles,
and 42 objects. The coverage register has no local `gith_ht.qst` file and
records zero native Q/QA contracts, although its QST-area membership field is
`YES` ([coverage row](ZONE_COVERAGE_REGISTER.csv#L105),
[AREA entry](../../../areas/AREA#L97)). Its sidecar says
[`coverage: complete`](../../../areas/story/gith_ht.story.json#L5), but has
empty [`contacts`, `stories`, and `exclusions`](../../../areas/story/gith_ht.story.json#L11).
The introduction promises that players can meet residents and “uncover local
requests,” which the sidecar does not map. That source combination records no
local story contact or objective; it does not prove that no live service or
special behavior exists.

## Environmental story

A source-text grouping finds 48 distinct room title/description pairs across
the 91 room records. The fortification and guild-room descriptions repeat in
purposeful-looking runs, but identical or similar prose alone does not prove
that any rooms are duplicates.

| Setting | Source evidence | Story cues |
| --- | --- | --- |
| Astral ledge and fortress perimeter | [Ledge above the Astral Plane](../../../areas/wld/gith_ht.wld#L2), [iron gates](../../../areas/wld/gith_ht.wld#L23), [outer wall](../../../areas/wld/gith_ht.wld#L52) | The fortress stands on a black, star-specked astral island whose edge drops into the Astral Plane. An iron gate relief depicts Gith’s rebellion against the illithids; high walls and posted Githyanki guards establish a defended military settlement. |
| Streets, rally space, and courtyard | [narrow street](../../../areas/wld/gith_ht.wld#L392), [formation street](../../../areas/wld/gith_ht.wld#L415), [rune-protected courtyard](../../../areas/wld/gith_ht.wld#L517) | The cramped street is described as tactical space, while the broader street serves as a place to gather and plan raids. The courtyard’s protective runes and Gith statue tie the built environment to the fortress’s anti-illithid history. |
| Prison and coercive training | [guard room](../../../areas/wld/gith_ht.wld#L441), [holding cells](../../../areas/wld/gith_ht.wld#L468), [blood-covered training room](../../../areas/wld/gith_ht.wld#L1645) | Captives from the Prime Plane are held for information and then used in warrior and anti-paladin training. The cells describe runes against magic and psionics; a hidden room combines interrogation implements with trainee weapons. Static resets place a Human from Tharnadia, a Drow from Arachdrathos, a Githzerai, and a Gnome from Ashrumite in the prison complex ([captives](../../../areas/zon/gith_ht.zon#L60)). |
| Military command and class guilds | [anti-paladin war room](../../../areas/wld/gith_ht.wld#L758), [warrior planning room](../../../areas/wld/gith_ht.wld#L832), [psionicist meditation chamber](../../../areas/wld/gith_ht.wld#L887), [High Mage laboratory](../../../areas/wld/gith_ht.wld#L940), [lord’s antechamber](../../../areas/wld/gith_ht.wld#L1018) | The fortress contains distinct anti-paladin, warrior, psionicist, necromancer, and sorcerer spaces. Maps, planning tables, ritual signs, chemical work, and the High Mage’s laboratory support a military hierarchy organizing raids on the Prime Plane and resistance to the illithids. |
| Bard and Reaver guilds | [bard students](../../../areas/wld/gith_ht.wld#L1484), [Master Bard’s chambers](../../../areas/wld/gith_ht.wld#L1504), [Reaver foyer](../../../areas/wld/gith_ht.wld#L1660), [Reaver proving ground](../../../areas/wld/gith_ht.wld#L1680), [Master Reaver’s room](../../../areas/wld/gith_ht.wld#L1698) | Music students and instruments add a civilian craft and teaching space. The Reaver guild is explicitly organized around proving trainees and coordinating forward scouts for raids. These rooms and resets do not establish player training or acceptance mechanics. |
| Beast pens, banking, and trade | [beast path](../../../areas/wld/gith_ht.wld#L1518), [breeding pen](../../../areas/wld/gith_ht.wld#L1541), [grazing area](../../../areas/wld/gith_ht.wld#L1559), [bank](../../../areas/wld/gith_ht.wld#L1572), [Herkol’s trade house](../../../areas/wld/gith_ht.wld#L1586), [Flayed Illithid Inn](../../../areas/wld/gith_ht.wld#L1600) | The fortress breeds succubi, keeps shadow daemons and smoke elementals, and has a bank described as recently built to handle increased raid activity. Herkol’s shelves hold a broad stock; the inn’s Illithid-hide furnishings make the raiding economy visible in its ordinary setting. |

## Residents and authored services

Resets place Githyanki commoners, knights, warriors, psionicists, anti-
paladins, necromancers, a High Mage, a lord and their followers throughout
the fortified settlement. The sidecar and QST sources define no local request
steps for this cast. The prison population and reset locations above are
static placements, not a claim about live dialogue or combat outcomes.

One `.shp` record belongs to Herkol `19439` in room `19483`
([shop record](../../../areas/shp/gith_ht.shp#L1),
[shopkeeper reset](../../../areas/zon/gith_ht.zon#L154)). The shop stocks local
Githyanki equipment and captive-derived goods, plus foreign item prototypes
`363`, `368`, and `369`. Those three objects are defined in active Heavens
sources ([gauze bandage](../../../areas/obj/heavens.obj#L3783),
[tourniquet](../../../areas/obj/heavens.obj#L3836),
[medicated bandage](../../../areas/obj/heavens.obj#L3850)). This maps the
declared shop and stock only; it does not qualify shop access, pricing,
restocking, or live sales.

## Reset and cross-area dependencies

The zone file has 181 typed commands: 94 `M`, 22 `F`, 29 `G`, 23 `E`, 8 `D`,
4 `O`, and 1 `P`. The four room-object placements are a fountain, a statue of
Gith, a bookcase, and bank counter `3097`; local mystic book `19408` is nested
in the bookcase ([fountain and statue](../../../areas/zon/gith_ht.zon#L41),
[bookcase and nested book](../../../areas/zon/gith_ht.zon#L43),
[bank counter](../../../areas/zon/gith_ht.zon#L45)). The counter resolves to
active `verz1.obj` ([prototype](../../../areas/obj/verz1.obj#L1199);
[AREA entry](../../../areas/AREA#L15)).

Two object-assignment commands point to IDs with no current object prototype
definition under `areas/obj/`: `G #19443` is reset with the High Mage and `E
#19442` equips the Githyanki lord
([High Mage reset](../../../areas/zon/gith_ht.zon#L101),
[`G #19443`](../../../areas/zon/gith_ht.zon#L103),
[lord reset](../../../areas/zon/gith_ht.zon#L115),
[equipment line](../../../areas/zon/gith_ht.zon#L200)). The local object file
ends at `19441` and has 42 prototypes
([object inventory](../../../areas/obj/gith_ht.obj#L518)); the full current
object-source search has no `#19442` or `#19443` record. Although those
numbers name local rooms and mobile prototypes as well, that does not supply
the missing object definitions. The matching WLD rooms are recorded at
[`19442`](../../../areas/wld/gith_ht.wld#L831) and
[`19443`](../../../areas/wld/gith_ht.wld#L847), and the matching mobile
prototypes at [`19442`](../../../areas/mob/gith_ht.mob#L595) and
[`19443`](../../../areas/mob/gith_ht.mob#L609). Preserve the reset rows and
ask the owner whether these are stale IDs, externally supplied items, or
intentional omissions; do not substitute gear or infer the intended item.

There are additional current-source dependencies in both directions:

| Direction | Source evidence | Review boundary |
| --- | --- | --- |
| `gith_ht` shop stock → Heavens objects `363`, `368`, `369` | [shop stock](../../../areas/shp/gith_ht.shp#L1), [Heavens prototypes](../../../areas/obj/heavens.obj#L3783) | Heavens is active in [`areas/AREA`](../../../areas/AREA#L2); retain the foreign stock IDs pending owner review. |
| `gith_ht` bank placement → Verz1 object `3097` | [reset](../../../areas/zon/gith_ht.zon#L45), [counter](../../../areas/obj/verz1.obj#L1199) | Verz1 is active; this is a cross-area object placement, not evidence of the bank’s live service behavior. |
| Current unregistered `nexus56` resets → Gith objects `19429`, `19434`, `19439` | [first reference](../../../areas/zon/nexus56.zon#L1348), [second](../../../areas/zon/nexus56.zon#L1366), [third](../../../areas/zon/nexus56.zon#L1388) | `nexus56` is outside the full `areas/AREA` list ([register row](ZONE_COVERAGE_REGISTER.csv#L430)). Treat these as current source references requiring closure, not active full-world placements. |

## Static connectivity and replacement boundary

The source exit census records 185 raw exits, all 185 loader-resolved. All 91
rooms are in one weak local component; the row records one incoming and one
outgoing cross-area exit, a reverse record, no fall-chance rooms, and no
currents ([coverage row](ZONE_COVERAGE_REGISTER.csv#L105)).

Room `19400` exits east to Astral Plane room `19794`, and that room has a
westward reverse record ([pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L154),
[outbound exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L311),
[reverse exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L320)). This is a reciprocal
static source connection, not a statement that ordinary players can enter,
leave, or return safely under current conditions.

The full `areas/AREA` list contains both `gith_ht` (zone `194`) and the
separate `githyanki_fortress` contributor (zone `1320`)
([Hometown entry](../../../areas/AREA#L97),
[Fortress entry](../../../areas/AREA#L380)). Their source exits reach different
Astral Plane rooms (`19794` and `19798` respectively)
([Hometown pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L154),
[Fortress pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L155)). The current title
scan records no exact repeated active title in the Hometown row
([coverage row](ZONE_COVERAGE_REGISTER.csv#L105)). Commit `9004f09cb`
(2010-07-31) describes the new Fortress files as “the replacement for old
gith fort,” but does not identify `gith_ht` as that predecessor. The tracked
pre-addition tree already contains the active Hometown, and the later active
list retains both zones. Record the replacement claim while leaving its
predecessor unresolved; it does not establish that the current Hometown is
superseded ([Fortress history review](GITHYANKI_FORTRESS_STORY_REVIEW.md)).

The eight `.zon` door declarations form four pairs: the fortress entrance,
the prison-to-secret-training route, and two beast-pen/grazing gates. The
comments mark the entrance and pens closed, and the prison route closed and
secret ([entrance](../../../areas/zon/gith_ht.zon#L8),
[secret prison route](../../../areas/zon/gith_ht.zon#L16),
[pen gates](../../../areas/zon/gith_ht.zon#L24),
[second pen gate](../../../areas/zon/gith_ht.zon#L32)). The static graph alone
does not establish which routes are normally open or safe.

## Checkpoint 807 current typed dependency census

The zone has 174 typed reset operands: 116 mobile operands (94 `M` and 22
`F`, all resolving to local prototypes) and 58 object operands. The object
operands include 52 local occurrences across the 42 local objects, active
imports Heavens #363/#368/#369 and Verz1 #3097, and one unresolved object
operand each for #19442 and #19443. The missing operands are object-role
failures even though both numbers also identify local WLD rooms and mobile
prototypes. The reset file still has 181 typed commands overall because that
command count also includes door and other non-prototype commands
([zone resets](../../../areas/zon/gith_ht.zon#L1)). Forty-nine of 52 local
mobiles are reset-used; #19407, #19418, and #19419 are not. All 42 local
objects are reset-used.

There is no outside active reset consumer of a Gith_ht-local prototype. The
unregistered, source-only `nexus56` source has object reset references to
#19429, #19434, and #19439, and shop stock for those same local objects
([resets](../../../areas/zon/nexus56.zon#L1348),
[shop](../../../areas/shp/nexus56.shp#L1)). No outside active shop or QST
consumer, typed story-sidecar consumer, or outside WLD key field targets a
Gith_ht prototype. The local Herkol shop stocks local objects and active
Heavens items #363/#368/#369; the complete local sidecar has empty contacts,
stories, and exclusions. No local type-25 or type-29 object and no incoming
outside type-25/type-29 room target was found.

The compiled numeric matches resolve to distinct room/mobile roles, not a
missing object definition: #19428 is the Chaos `githyanki` destination and a
home-location VNUM ([Chaos route](../../../src/combat/chaos.c#L39),
[home table](../../../src/core/constant.c#L1739)); #19442 appears as a guild
room VNUM in `guild_locations` ([table](../../../src/core/constant.c#L1977));
and #19400 appears in the Githyanki Fortress justice row as guard-room and
guard-mobile data plus the Demogorgon helper list. The helper list is
compared with target mobile VNUMs by `shout_and_hunt`
([helper list](../../../src/specs/specs.demogorgon.c#L101),
[mobile-VNUM filter](../../../src/combat/justice.c#L648)). No
object-specific compiled numeric binding was found. These source roles do
not qualify live teleport safety, guild availability, justice behavior, or
special-procedure outcomes.

The typed census leaves the static graph unchanged: all 185/185 exits still
loader-resolve, with the single reciprocal Astral boundary already recorded.
Editor/runtime behavior and persisted-reference closure remain pending; no
area sources, active registrations, or source-only ownership are changed.

## Builder questions and recommendations

1. **Is the empty sidecar intentional?** Confirm whether `coverage: complete`
   with no contacts, stories, or exclusions is the intended record for this
   military hometown, given the generic “local requests” introduction. If no
   local story is intended, record that environmental-only scope explicitly;
   do not invent requests or rewards.
2. **What are reset item IDs `19442` and `19443`?** Resolve whether the High
   Mage’s `G #19443` and lord’s `E #19442` should point to extant objects,
   deliberate foreign objects, or be removed. Each ID also names a local WLD
   room and mobile prototype, but neither has a current OBJ definition. Those
   other typed identities do not fulfill the object reset role; preserve both
   rows until the builder confirms.
3. **Are the fortress gate, prison passage, and beast-pen doors intentional
   for ordinary access?** Review the four door pairs and the hidden training
   route before proposing changes. Do not infer reachability from the weak
   component count.
4. **Are the imported shop and bank objects intentional?** Preserve Heavens
   stock `363`, `368`, and `369`, and the Verz1 counter `3097` until the
   respective owners confirm their roles.
5. **Which predecessor does the 2010 “old gith fort” replacement claim
   identify?** The commit does not map that phrase to `gith_ht`. Keep the
   Hometown and Fortress as distinct active contributors until a builder
   identifies the predecessor; their current area entries and Astral exits
   differ.

## Safe resolution summary

Keep `gith_ht`, the distinct active Fortress, and their current area sources in
place. Preserve the Heavens shop goods, Verz1 bank counter, current-source
`nexus56` references, and all existing reset rows, including the two item IDs
without current object prototypes. Record the fortress environmental story
and builder questions while leaving the sidecar, shop stock, doors, object
assignments, and routes unchanged. No gameplay or migration change is
proposed.
