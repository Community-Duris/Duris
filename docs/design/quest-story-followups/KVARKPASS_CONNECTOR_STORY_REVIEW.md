# Kvarkpass Connector: source story review

**Scope:** static review of Kvarkpass Connector's QST messages, room and mobile
text, objects, resets, shops, typed references, local topology, and WLD history.
No route, reset, shop, or quest behavior was tested live.

## Source identity and quest-story coverage

Kvarkpass Connector is an active full-area contributor in numeric range
546–549 ([area list](../../../areas/AREA#L233)). Its source defines 245 rooms,
35 mobiles, 21 objects, four QST giver prototypes, zero native Q/QA contracts,
and two formal shop records. The QST file has one `M` response record for each
of mobiles #54630–#54632 and #54634; #54634 has two message lines. No story
sidecar or earlier mapped source dossier was present before this review
([QST source](../../../areas/qst/kvarkpass_connector.qst#L1),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L152)).

These four records are timed room actions. They describe a distant troggler's
roar, two pupa behaviors, and the nest matron expelling ooze
([roar action](../../../areas/qst/kvarkpass_connector.qst#L3),
[first pupa action](../../../areas/qst/kvarkpass_connector.qst#L9),
[second pupa action](../../../areas/qst/kvarkpass_connector.qst#L15),
[matron action](../../../areas/qst/kvarkpass_connector.qst#L21)). The generic
quest routine emits `qc_action` text to the room during `CMD_NONE`; the ask
check skips these entries, so they do not define addressed topics or task
completion ([routine](../../../src/world/quest.c#L101),
[ask check](../../../src/world/quest.c#L2446)). The area has no local Q/QA
contract or reward chain in the source census.

## Environmental story

- **A dangerous divide and battlefield.** The pass through Kvark's Divide is
  described as the used trail between undead and evil lands, with steep,
  slippery rock and erosion. An orc refuge sits off the trail; a mountain pool
  descends into an extremely dark underwater cavern whose prose mentions a
  northbound current. The trail then opens onto the Great BloodFields, a
  corpse-covered front in the continuing war between undead and evil races.
  A magical rock keeps the field in twilight, and a blood-caked statue of
  Kvark bears the plaque “King of MetaGrunka”
  ([pass](../../../areas/wld/kvarkpass_connector.wld#L1),
  [orc refuge](../../../areas/wld/kvarkpass_connector.wld#L121),
  [pool](../../../areas/wld/kvarkpass_connector.wld#L241),
  [battlefield](../../../areas/wld/kvarkpass_connector.wld#L611),
  [statue](../../../areas/wld/kvarkpass_connector.wld#L693)).
- **An exposed and broken mountain route.** A second trail crosses a narrow
  cliffside, a collapsed rope bridge with “only air” rooms, and a landslide
  that spills toward the open wilderness of the undead hordes. The source
  includes 13 fall-chance rooms and no numeric current fields. That differs
  from the pool room's descriptive current; neither the prose nor the static
  field count establishes actual travel or hazard behavior
  ([trail and bridge](../../../areas/wld/kvarkpass_connector.wld#L2060),
  [collapsed approach](../../../areas/wld/kvarkpass_connector.wld#L2169),
  [bridge room](../../../areas/wld/kvarkpass_connector.wld#L2190),
  [landslide route](../../../areas/wld/kvarkpass_connector.wld#L2292),
  [coverage topology](ZONE_COVERAGE_REGISTER.csv#L152)).
- **Shatter Stone's expanding outpost.** The newer settlement is a duergar
  mining camp founded by workers who finished tunnels around the underground
  river. Its descriptions say that work repeatedly stops as miners go to fight
  undead near the BloodField. The route grows from miners' homes and a temple
  still being excavated into Clan Shatter Stone's guarded military outpost:
  a war room, barracks, lieutenant and tactician rooms, a great hall, and
  Commander Glonvx's throne room. Mobiles identify the clan's home as
  Khildarak. Nearby Rothe fields provide food that the duergar harvest
  ([settlement](../../../areas/wld/kvarkpass_connector.wld#L2332),
  [war room](../../../areas/wld/kvarkpass_connector.wld#L2987),
  [great hall](../../../areas/wld/kvarkpass_connector.wld#L3106),
  [throne room](../../../areas/wld/kvarkpass_connector.wld#L3144),
  [Rothe fields](../../../areas/wld/kvarkpass_connector.wld#L3218),
  [temple](../../../areas/wld/kvarkpass_connector.wld#L3910),
  [guard](../../../areas/mob/kvarkpass_connector.mob#L65),
  [lieutenant](../../../areas/mob/kvarkpass_connector.mob#L151), and
  [commander](../../../areas/mob/kvarkpass_connector.mob#L215)).
- **Misty giant homes and a balancing grove.** The winding, torch-lit mist
  caves contain mountain giant homes, a family, and an elder. Above the
  landslide trail, a small hidden grove is described as a druid's work,
  watched over by Baum to preserve balance during the battlefield's conflict
  ([giant home](../../../areas/wld/kvarkpass_connector.wld#L839),
  [elder's room](../../../areas/wld/kvarkpass_connector.wld#L4076),
  [hidden grove](../../../areas/wld/kvarkpass_connector.wld#L4201),
  [grove keeper](../../../areas/mob/kvarkpass_connector.mob#L304)).
- **A fungal underworld and insect nursery.** Mushroom caverns, Rothe pasture,
  pools, slime-covered shafts, and a long drafty Underdark shaft form the
  lower route network. Cold caves explain that the pupae secrete a frosty
  liquid and spend a year developing into carnivorous beetles; the matron
  feeds the young with ooze. The QST actions animate these descriptions but do
  not script a growth or quest outcome. Room #54833 repeats the vertical-larvae
  description but has no authored exits or ZON reset placement and forms a
  one-room local weak component. A nearby hidden chamber contains a corpse and
  unsettling presence;
  its reset mobile is a “ravaged soul” whose text asks for help, with no local
  QST record for it
  ([cold nursery](../../../areas/wld/kvarkpass_connector.wld#L4553),
  [isolated larvae room](../../../areas/wld/kvarkpass_connector.wld#L4949),
  [hidden corpse chamber](../../../areas/wld/kvarkpass_connector.wld#L4974),
  [pupa prototypes](../../../areas/mob/kvarkpass_connector.mob#L452),
  [matron](../../../areas/mob/kvarkpass_connector.mob#L523),
  [ravaged soul](../../../areas/mob/kvarkpass_connector.mob#L509)).

## Resets, doors, shops, and static connectivity

The WLD census has 566 authored exits and all 566 targets resolve. The local
weak graph has three components: 220 rooms, a 24-room larvae/cave component
in the #54811–#54835 block, and isolated room #54833. The component calculation
treats authored exits as undirected links. It does not prove that a locked or
secret edge opens, that a room is ordinarily reachable, or that a route is
safe or reversible.

The active area has ten WLD exits to other numeric zones: four paired Surface
links (#54600, #54803, #54804, #54844), three paired Underworld links (#54805,
#54810, #54841), one paired Lab link (#54690), and one paired Connectorzones
link (#54834). Room #54811 also exits to UDmini room #88515; no reverse WLD edge
is recorded for that boundary. The paired Connectorzones link is the current
outside connection to the isolated 24-room nursery component. Five more WLD
records from unregistered sources point into this area—four in `surf.wld` and
one in `underdark2011.wld`; they are not part of the normal active `areas/AREA`
world. The exact active and source-only records are summarized in
[world connectivity](WORLD_CONNECTIVITY.md), the
[exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L737), and pair rows for
[Connectorzones](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L236),
[Lab](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L237),
[Surface](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L238),
[UDmini](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L239), and
[Underworld](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L240).

Several doors are explicitly reset closed, locked, secret, or blocked. At the
drow post, the locked up trapdoor uses shiny black key object #54602, which is
given to the drow scout in room #54630. The return trapdoor is also reset
closed. A second trapdoor between the statue and shaft has a visible handle
clue and a secret reset on the statue side. The broader route system also has
closed/secret passages at a reflecting pool, a pitch-black refuge, the bone
trail, a hidden settlement alcove, and the hidden chamber in the pupa caves
([WLD door definitions](../../../areas/wld/kvarkpass_connector.wld#L648),
[door resets and key placement](../../../areas/zon/kvarkpass_connector.zon#L32),
[statue handle](../../../areas/wld/kvarkpass_connector.wld#L693),
[secret door resets](../../../areas/zon/kvarkpass_connector.zon#L8)).
The reset file is the source for the initial door states; live access was not
tested.

Two formal shops are declared: mobile #54600, the traveling orc merchant in
the refuge at #54605, and mobile #54625, the ghost shopkeeper in the
pitch-black cave at #54681. Both shop records list the local backpack object
#54600 and potion object #54601 as stock
([orc shop record](../../../areas/shp/kvarkpass_connector.shp#L1),
[ghost shop record](../../../areas/shp/kvarkpass_connector.shp#L45),
[merchant reset](../../../areas/zon/kvarkpass_connector.zon#L261),
[ghost reset](../../../areas/zon/kvarkpass_connector.zon#L288),
[backpack](../../../areas/obj/kvarkpass_connector.obj#L1),
[potion](../../../areas/obj/kvarkpass_connector.obj#L9)). Same-numbered
mobile, object, shop, and room identifiers are distinct typed records.

## Typed cross-area dependencies

- The local reset file imports Underworld mobile #4430, a shambling mound, into
  local room #54806 ([reset](../../../areas/zon/kvarkpass_connector.zon#L400),
  [mobile prototype](../../../areas/mob/underworld.mob#L124)).
- Alatorin resets Kvark object #54605, the fire pit, in room #83517
  ([foreign reset](../../../areas/zon/alatorin.zon#L1639),
  [object](../../../areas/obj/kvarkpass_connector.obj#L57)).
- Connectorzones nests Kvark objects #54617 and #54616 inside its large chest
  #53658 and spider web #53657 respectively
  ([chest reset](../../../areas/zon/connectorzones.zon#L380),
  [web reset](../../../areas/zon/connectorzones.zon#L384),
  [first locket](../../../areas/obj/kvarkpass_connector.obj#L167),
  [second locket](../../../areas/obj/kvarkpass_connector.obj#L175),
  [spider web](../../../areas/obj/connectorzones.obj#L762),
  [chest](../../../areas/obj/connectorzones.obj#L770)).
- Object #54618 is named “wand of aging,” but its long description calls it a
  glowing locket. No active reset or shop placement for #54618 was found in
  the scanned area sources. Confirm whether this is an unused prototype or a
  description mismatch ([object](../../../areas/obj/kvarkpass_connector.obj#L183)).

The static typed scan found no cross-area QST file or numeric code reference
to a local mobile prototype. These reset and shop declarations describe
source links, not proof that every stock, reset, or route behaves as expected
at runtime.

## Duplicate and replacement history

The normalized active area-title scan finds no repeated active title for
Kvarkpass Connector. The followed WLD history has a concrete endpoint
correction: commit `71d731c5b` (2013, `stuff`) changed room #54804's upward
target from #278150 to active Surface room #629815. The current Surface room
returns to #54804. The old #278150 remains in unregistered `surf`,
`surface2011`, and `underdark2011` WLD sources; the latter still has a
source-only exit from
#278150 to #54804. Those legacy files are outside the current `areas/AREA`
list. The 2011 WLD diff (`677a9f877`) for this area consists of spelling edits,
despite that commit's broader message about Tharn Rifts preparation. The 2009
history entry (`642da5400`) is a path migration. These records support an old
route-target correction, not a whole-zone retirement or replacement mapping
([Kvark side](../../../areas/wld/kvarkpass_connector.wld#L4396),
[active Surface room](../../../areas/wld/surface.wld#L2725580),
[active Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1651),
[legacy `surf` room](../../../areas/wld/surf.wld#L1639906),
[legacy `surface2011` room](../../../areas/wld/surface2011.wld#L1641151),
[legacy reciprocal source edge](../../../areas/wld/underdark2011.wld#L320912),
[source history summary](WORLD_CONNECTIVITY.md)). Kvarkpass remains an
active full-area contributor and should not be moved.

## Builder questions

1. Is room #54833 intentionally an isolated vertical larvae chamber, or is a
   missing route/reset part of its design?
2. Should the one-way WLD boundary from #54811 to UDmini #88515 have a return
   edge? Confirm that the 2013 Surface target correction and the surviving
   source-only `underdark2011` edge reflect the intended current routes.
3. Confirm the drow key/trapdoor and statue handle/secret-door behavior, along
   with the reset-closed passages. The source establishes state declarations,
   not which clues or actions reveal them.
4. Should the pupa and matron remain ambient ecology only, and should the
   ravaged soul have no addressed interaction? The existing QST entries are
   room actions rather than player tasks.
5. Correct or retire the unused-looking wand-of-aging/locket prototype if the
   description mismatch is unintended.

This review records authored source and static references. Player-facing
reachability, fall behavior, shop stock, reset outcomes, and safe return remain
unqualified.

## Builder questions and recommended resolutions

The existing five builder questions identify source anomalies and open route intent. These direct questions add source-backed resolutions while preserving the current mapped story and active owner boundaries.

| Question | Source evidence | Recommended resolution |
| --- | --- | --- |
| Is room #54833 intentionally an isolated larvae chamber? | It repeats the vertical-larvae description but has no authored exits or ZON reset placement ([room](../../../areas/wld/kvarkpass_connector.wld#L4949)). | Keep it isolated unless a builder identifies the intended connection and placement. Do not create a new route from adjacent prose or numeric proximity. |
| Should the one-way exit from #54811 to UDmini #88515 have a reverse route? | One active outbound WLD record reaches UDmini and no reverse WLD edge is registered ([boundary summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L239)). | Keep the authored edge under its current owner. Add a return only after both area owners define the destination, route, and safety intent. |
| Does the 2013 Surface correction or the surviving inactive edge establish a current route to #278150? | Current #54804 returns to active Surface #629815; old `surf`, `surface2011`, and `underdark2011` retain differing source records ([history](#duplicate-and-replacement-history)). | Treat the correction as same-area endpoint history and inactive edges as source-only. Keep them on their existing map owners; do not count them as active ingress or a replacement. |
| How should the drow scout's shiny black key and two trapdoors work together? | The scout carries #54602 for a locked up trapdoor; the return trapdoor is separately reset closed ([door/reset evidence](#resets-doors-shops-and-static-connectivity)). | Preserve the exact key holder, target, and return state. Qualify key acquisition, admitted unlock, opening, passage, return, and key loss independently before changing access. |
| Is the statue handle clue meant to reveal and open the shaft passage? | The statue mentions a handle; the matching passage is reset secret on one side ([statue](../../../areas/wld/kvarkpass_connector.wld#L693), [resets](../../../areas/zon/kvarkpass_connector.zon#L8)). | Keep clue, search/reveal, door-state change, and actual passage distinct. Confirm the intended action and return before adding a completion claim. |
| Should the pupa and matron QST messages count as player progress? | Their four QST records are timed room actions emitted during `CMD_NONE`; ASK skips them, and no local Q/QA contract exists ([QST actions](#source-identity-and-quest-story-coverage)). | Keep them as ambient ecology. Add a contract only with an addressed actor, exact prerequisites, accepted outcome, and recovery policy. |
| Should the ravaged soul's plea create a rescue objective? | The mobile asks for help, but no local QST or completion record handles it ([hidden chamber](../../../areas/wld/kvarkpass_connector.wld#L4974), [mobile](../../../areas/mob/kvarkpass_connector.mob#L509)). | Preserve the current environmental text. Do not award rescue or interaction credit until a builder defines an owned response and outcome. |
| Is object #54618 an unused wand, a locket, or a retired prototype? | Its name says “wand of aging,” while its long description calls it a glowing locket; the bounded reset/shop scan found no placement ([object](../../../areas/obj/kvarkpass_connector.obj#L183)). | Confirm intended identity and ownership. Then make a specific text correction or retire the unused prototype without inferring a quest item or reset. |
| What do the two formal shops guarantee about backpack and potion availability? | Both local shops list objects #54600 and #54601, with separate keeper and reset records ([shop sources](#resets-doors-shops-and-static-connectivity)). | Keep stock declarations under the current shop owner and treat live availability, restock, price, and transaction success as unqualified until observed. |
| Should the prose about a northbound current through the dark pool imply current movement? | Room prose mentions a northbound current, but the WLD has no numeric current fields; thirteen other rooms have fall-chance fields ([environmental story](#environmental-story)). | Preserve the prose as atmosphere unless builders select a real movement mechanic. Do not infer current behavior or safe travel from the description or field count. |
| Should the settlement, battlefield, and nursery be one mandatory campaign? | Environmental text links the Shatter Stone outpost, undead conflict, giant homes, grove, and beetle nursery, but the four local QST records are timed room actions ([environmental story](#environmental-story), [QST coverage](#source-identity-and-quest-story-coverage)). | Keep them as connected environmental narratives. Add progression only with explicit owners, route prerequisites, state changes, and completion receipts. |
| Which doors, falls, stock, and cross-zone paths need play before the dossier describes them as usable? | Static scanning does not execute reset generation, secret/key access, shop transactions, fall effects, or return journeys ([connectivity and audit limits](#resets-doors-shops-and-static-connectivity)). | Keep all route and safety claims unqualified until current stock and actor-owned passage/return outcomes are observed; do not change native source from static counts alone. |
