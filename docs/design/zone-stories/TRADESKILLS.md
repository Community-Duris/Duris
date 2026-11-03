# Tradeskills: source story dossier

The active `tradeskills` contributor spans zone header 4000 and WLD rooms
#400000–#400999. Its authored story is a compact utility hub called “The Path
Between Worlds,” attached to a two-room catalog ceiling. The area has no local
story sidecar, and its one-line QST file is not listed in `areas/qst/AREA` and
contains no Q/A contract. The useful story closure is therefore in its room,
shop, object, reset, and maintained-code sources. Static review does not claim
that the hub or any described service is reachable or available in a running
game.

## Place and route

Room #400000 is titled “The Path Between Worlds” and has one open DOWN exit to
#400999. The second room calls itself “Top of Tradeskills,” says it exists as
the upper VNUM limit, and returns by an open SOUTH exit. Those two records form
one raw directed component; neither has a key, door flag, or authored outside
WLD connection. The return edge is a source fact, not proof of a safe player
journey or a normal in-world entrance. See the [room pair](../../../areas/wld/tradeskills.wld#L1)
and its [zone header](../../../areas/zon/tradeskills.zon#L1).

There is a separate code-created connection: a conditional death path creates
portal object #400220, sets its destination to the victim’s room, and places it
in #400000. The portal lifecycle and travel handlers inspect that object. It
does not add an authored WLD edge or establish who can enter, leave, or safely
return through the hub. The exact binding is in the [portal creation path](../../../src/combat/fight.c#L4829),
[lifecycle handler](../../../src/magic/spell_corpse_lifecycle.c#L1023), and
[travel handler](../../../src/magic/spell_travel.c#L972).

## People, stock, and local requests

The zone reset creates only Lout (#400004) in #400000. His mobile text calls
him a demon hoarder. The matching SHP record belongs to #400004 and lists three
products: faerie bag #400217, turkey innards #400232, and exceptional-equipment
chest #400233. The reset supplies those items to Lout’s inventory. This is a
configured shop and reset, not proof of a live stock count, purchase, or
separate quest. See the [reset rows](../../../areas/zon/tradeskills.zon#L10),
[Lout](../../../areas/mob/tradeskills.mob#L47), [shop record](../../../areas/shp/tradeskills.shp#L1),
and [three products](../../../areas/obj/tradeskills.obj#L1772).

The active mobile file also defines Kannard the traveler (#400000), the
Harvester (#400001), a Death Hunter (#400002), a lesser conjured being
(#400003), a Thanksgiving turkey (#400005), a Christmas elf (#400006), and
Santa Claws (#400007). Only Lout is in this zone’s reset. Maintained code
binds #400000 as an epic store and #400001 as a PvP store; other code conjures
#400003, creates seasonal encounters, and binds item-specific effects. Their
prototype presence is not a claim that they all appear in the Tradeskills
rooms. Cross-zone resets and code bindings are reconciled in the [audit
supplement](../quest-story-followups/TRADESKILLS_AUDIT_REVIEW.md).

The local QST file is only `#400000` followed by `S`; the stem is absent from
the QST manifest. No local accepted request, completion receipt, or story
sidecar is declared.

## Object catalog and authored identity

The active object file contains 293 definitions, organized around shared
numeric families rather than one local quest:

- #400000–#400209: 42 salvage-material families with five quality tiers each.
- #400210–#400237: 28 recipe, magic, event, and utility objects, including the
  recipe scroll, portal, crafting goods, and seasonal objects.
- #400238–#400258: 21 stat and attribute essences.
- #400260–#400283: 24 ore objects in three size tiers.
- #400291–#400299: nine encrusting stones.
- #400300: the universal-material Chaos craft pouch.

The gaps #400259 and #400284–#400290 are not active object definitions. Code
defines the salvage, ore, essence, and encrusting ranges separately; keep each
range distinct from a claim that all prototypes are currently obtainable.
Representative definitions include [the first salvage tier](../../../areas/obj/tradeskills.obj#L1),
[recipe scroll](../../../areas/obj/tradeskills.obj#L1685), [stat essences](../../../areas/obj/tradeskills.obj#L2000),
[ore tiers](../../../areas/obj/tradeskills.obj#L2168), [encrusting stones](../../../areas/obj/tradeskills.obj#L2360),
and [Chaos pouch](../../../areas/obj/tradeskills.obj#L2433).

`tradeskills.old.obj` is an unlisted, object-only catalog with 234 overlapping
VNUMs. Comparing whole records found 218 exact copies and 16 changed records;
the current file adds 59 objects. Among the changed identities, old #400232
was an elven equipment bag and current #400232 is turkey innards, while the
current bag is #400235. The 2013 tracked history records a separate copy but
does not state why it was kept. Keep the older file out of the active catalog
and do not infer that its overlapping VNUMs name the current objects. See the
[old-catalog audit](../quest-story-followups/TRADESKILLS_OLD_AUDIT_REVIEW.md)
and [20 builder questions](../quest-story-followups/TRADESKILLS_OLD_BUILDER_QUESTIONS.md).

## Explicit builder questions and recommended resolutions

| # | Source-backed question | Conservative recommendation |
| --- | --- | --- |
| 1 | Is #400000 a player-facing “Path Between Worlds,” or a staging room for a specific portal service? ([room](../../../areas/wld/tradeskills.wld#L1), [corpse-portal placement](../../../src/combat/fight.c#L4840)) | Describe the authored room and conditional portal separately; do not promise general travel from the title alone. |
| 2 | Should the open DOWN exit expose the “Top of Tradeskills” VNUM ceiling as a player destination? ([exit pair](../../../areas/wld/tradeskills.wld#L5), [upper-limit text](../../../areas/wld/tradeskills.wld#L10)) | Confirm whether #400999 is a tool/catalog endpoint before giving it a player-facing purpose. |
| 3 | What journey and return policy should accompany corpse portal #400220's arrival at #400000? ([portal prototype](../../../areas/obj/tradeskills.obj#L1813), [creation](../../../src/combat/fight.c#L4829), [travel check](../../../src/magic/spell_travel.c#L972)) | Keep portal arrival distinct from authored WLD ingress and qualify its trigger, destination, ownership, and return conditions. |
| 4 | Is Lout intended to be the zone's only reset mobile and the keeper of the local shop? ([mobile](../../../areas/mob/tradeskills.mob#L47), [reset](../../../areas/zon/tradeskills.zon#L10), [shop](../../../areas/shp/tradeskills.shp#L1)) | Preserve that separation from training or quest roles unless a separate accepted interaction is authored. |
| 5 | Are #400217, #400232, and #400233 the complete intended Lout stock, given their reset limits and shop product rows? ([reset items](../../../areas/zon/tradeskills.zon#L11), [shop products](../../../areas/shp/tradeskills.shp#L3)) | Keep configured inventory, product identity, and live availability as separate facts. |
| 6 | Should Kannard's traveler text explain the code-bound epic-store role? ([mobile](../../../areas/mob/tradeskills.mob#L1), [binding](../../../src/specs/specs.assign.c#L2576), [outpost reset](../../../areas/zon/khildarak.zon#L3212)) | Clarify the interaction only after the store's access and service terms are confirmed. |
| 7 | Should the Harvester's soul-collector description explain the PvP-store callback? ([mobile](../../../areas/mob/tradeskills.mob#L12), [binding](../../../src/specs/specs.assign.c#L2577), [outpost reset](../../../areas/zon/tharnadia.zon#L1843)) | Keep the evocative text separate from a promised service until the actual menu and eligibility are qualified. |
| 8 | Is #400003 intended as the default conjured being, with its Level 21 spellbook path and lifecycle? ([mobile](../../../areas/mob/tradeskills.mob#L34), [spellbook reference](../../../src/classes/drannak.c#L1278)) | Preserve the prototype identity and specify any intended restrictions or duration in the owning spell documentation. |
| 9 | Does recipe scroll #400210 teach persistent player recipes, independently of the room and local QST stub? ([object](../../../areas/obj/tradeskills.obj#L1685), [special assignment](../../../src/specs/specs.assign.c#L1575), [recipe read](../../../src/classes/drannak.c#L836)) | Document recipe learning as its own service and persistence path; do not imply a local quest receipt. |
| 10 | Are the Christmas elf and faerie bag intended to be a conjuration/event pair? ([mobile](../../../areas/mob/tradeskills.mob#L75), [bag](../../../areas/obj/tradeskills.obj#L1772), [creation code](../../../src/classes/drannak.c#L1693)) | Describe the exact code-triggered encounter separately from static mobile availability. |
| 11 | Which seasonal conditions own the Thanksgiving turkey, its innards, and the periodic turkey wings? ([turkey](../../../areas/mob/tradeskills.mob#L61), [innards](../../../areas/obj/tradeskills.obj#L1936), [wings](../../../areas/obj/tradeskills.obj#L1988), [special binding](../../../src/specs/specs.assign.c#L1386)) | Keep seasonal trigger, food, wing effect, and Lout shop stock distinct until their shared schedule is verified. |
| 12 | Are the 42 five-tier salvage families meant to be general materials accepted across other areas? ([material range](../../../areas/obj/tradeskills.obj#L1), [tier generation](../../../src/item/randomeq.c#L567)) | Preserve each family and quality tier; do not turn a shared material reference into a provenance requirement. |
| 13 | Should the unlisted Dalvik QST file's 420 material commands remain source-only? ([QST material rows](../../../areas/qst/dalvik.qst#L6), [QST manifest](../../../areas/qst/AREA)) | Keep the file unregistered unless its giver, loader status, and current owner are deliberately reconciled. |
| 14 | How should Alatorin's exact salvage-material exchanges and equipment rewards distinguish supplied items from personally recovered ones? ([Alatorin QST](../../../areas/qst/alatorin.qst#L497), [story-sidecar sample](../../../areas/story/alatorin.story.json#L345)) | Preserve exact VNUM acceptance; do not imply source history or personal harvesting is recorded. |
| 15 | Are flux #400223 and Gnomish crafting tools #400224 meant to stay available through several foreign shops and resets? ([objects](../../../areas/obj/tradeskills.obj#L1844), [Khildarak stock](../../../areas/shp/khildarak.shp#L803), [Shady stock](../../../areas/shp/shady.shp#L330), [Tharnadia stock](../../../areas/shp/tharnadia.shp#L631)) | Keep these shared item identities stable and separate purchase from successful crafting. |
| 16 | Should the 24 ore prototypes retain their three size tiers and mined/conversion routes? ([ore records](../../../areas/obj/tradeskills.obj#L2168), [mining range](../../../src/economy/mining.c#L190), [size conversion](../../../src/classes/salchemist.c#L1371)) | Preserve the current VNUM-to-metal/size mapping; qualify output and crafting requirements independently. |
| 17 | Are #400238–#400258 the complete stat-essence set, with #400259 left unused? ([essence block](../../../areas/obj/tradeskills.obj#L2000), [range check](../../../src/item/enhance.c#L858)) | Retain the exact checked range and keep the gap reserved unless all item consumers are updated together. |
| 18 | Should encrusting stones #400291–#400299 remain paired with the #400300 Chaos pouch interface? ([stones](../../../areas/obj/tradeskills.obj#L2360), [pouch](../../../areas/obj/tradeskills.obj#L2433), [class instruction](../../../src/classes/salchemist.c#L937)) | Keep the nine-item range and pouch operation together; do not claim every stone can be used in another crafting flow. |
| 19 | Should `tradeskills.old.obj` remain an excluded older catalog despite its 234 VNUM overlaps, 16 changed records, and ambiguous copy history? ([older object source](../../../areas/obj/tradeskills.old.obj#L1), [active catalog](../../../areas/obj/tradeskills.obj#L1), [old-catalog audit](../quest-story-followups/TRADESKILLS_OLD_AUDIT_REVIEW.md)) | Retain it as separate source evidence; never merge or load it alongside the active object file without a reviewed migration. |
| 20 | Should Tradeskills be considered for separation from the active world contributor? ([active manifest entry](../../../areas/AREA#L406), [cross-zone dependency review](../quest-story-followups/TRADESKILLS_AUDIT_REVIEW.md#typed-reset-quest-shop-and-sidecar-references)) | Keep it DO NOT MOVE while the dynamic portal, active reset/shop consumers, and compiled bindings depend on these identities. |

## Qualification boundary

The active map, cross-zone references, and code provide source evidence for
these roles. They do not establish live player reachability, shop purchase,
successful crafting, material recovery, quest completion, or safe travel.
Keep Tradeskills active and preserve its shared VNUMs while those outcomes
remain unqualified.
