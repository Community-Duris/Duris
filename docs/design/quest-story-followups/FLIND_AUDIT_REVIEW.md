# Flind audit review

This supplement records the source-only review of active contributor flind. It maps authored WLD exits separately from reset-loaded ITEM_TELEPORT objects and compiled destination tables. It does not qualify installed state or live gameplay.

## Source inventory

The full area roster lists flind at *34, zone #34, with the title “The Minor Plane of Planitude.” The active files are areas/wld/flind.wld, areas/mob/flind.mob, areas/obj/flind.obj, areas/zon/flind.zon, and areas/shp/flind.shp. The source files contain 89 rooms (#3400–#3488), 34 mobiles (#3400–#3433), and 41 objects (#3400–#3440). The zone has reset mode 2 and 125 non-S commands: 52 M, 19 E, 18 D, 14 O, nine G, seven F, and six P.

The QST roster also lists flind, but areas/qst/flind.qst is absent. There is no native Q/QA contract or story sidecar. The one SHP record is #3406. Its product list contains five local objects: #3400, #3401, #3402, #3405, and #3432. The room’s reset loads display case #3403 and smelly butcher #3406 at #3406. See the [coverage record](ZONE_COVERAGE_REGISTER.csv#L99), [shop source](../../../areas/shp/flind.shp#L1), and [local resets](../../../areas/zon/flind.zon#L81).

## Narrative and reset context

The room text describes a torture and cannibalism complex: spear-lined halls; prisoners and captives; a nursery; a butcher shop selling body parts; cages and a salt pit; an underground roasting pit; a suspended path over a void; an inverted cross; the golden passageway; a necromancer; a wagering room; and the Demon’s Walk and living-tissue hatcheries. Reset-loaded mobiles place Flind roles, prisoners, animals, demons, and minions through these scenes. No local QST source records an objective or outcome.

The WLD’s south exit from Flind #3475 reaches active underdark room #829678, which has a reciprocal north exit to #3475. The paired active records are in [exit-register rows 45 and 1756](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L45) and [the pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L144). The inactive-WLD scan found no room-number overlap. It found one source-only approach: unregistered ud_map2 room #1124533 has a down exit to #3475 ([source room](../../../areas/wld/ud_map2.wld#L53018), [exit target line](../../../areas/wld/ud_map2.wld#L53042)). Because ud_map2 is not in areas/AREA, this is not active ingress.

## Authored graph and reset-aware reach

The 183 WLD exits resolve in the active loader inventory: 182 are local and one leaves Flind for #829678. The local directed graph has four weak components sized 53, 18, 15, and 3 rooms. From the active underdark arrival at #3475, the WLD graph alone reaches and returns from 3/89 rooms. Eleven rooms carry a fall-chance value; no current fields were found.

The local zone resets 18 doors. Thirteen D commands set state 1 (closed), four set state 5 (closed and secret), and one sets state 2 (closed and locked). With those exits treated as unavailable but all six reset-loaded local teleport objects included, strict static reach and return are 8/89 from both #3475 and the compiled-route target #3404. If all authored local exits are treated as traversable and all six teleport links are included, each root reaches and returns from all 89 rooms. These are graph calculations over source records, not evidence that doors, objects, or code routes are currently available.

The golden passageway door exposes a source asymmetry for owner review. WLD room #3487 south targets #3441 with key #3436, and its ZON D reset sets state 2; the reverse #3441 north exit has a ZON D reset in state 1. The local reset gives cross-shaped key #3436 to sentinel mobile #3428 at #3487 ([door resets](../../../areas/zon/flind.zon#L40), [locked-side reset](../../../areas/zon/flind.zon#L44), [sentinel and key resets](../../../areas/zon/flind.zon#L186)). Static source does not establish how the paired door behaves after loading.

## Reset-loaded ITEM_TELEPORT routes

All six local type-25 objects have active ZON placements. The command values match the generic ITEM_TELEPORT handler, which checks the object’s command value and uses its target room ([handler](../../../src/magic/spell_travel.c#L745), [command match](../../../src/magic/spell_travel.c#L949), [command dispatch](../../../src/cmd/interp.c#L2412)).

| Object | Reset placement | Declared target | Command value |
| --- | --- | --- | --- |
| #3430 ([prototype](../../../areas/obj/flind.obj#L438)) | #3408, [reset line 87](../../../areas/zon/flind.zon#L87) | #3409 | enter (7) |
| #3431 ([prototype](../../../areas/obj/flind.obj#L450)) | #3409, [reset line 88](../../../areas/zon/flind.zon#L88) | #3408 | enter (7) |
| #3424 ([prototype](../../../areas/obj/flind.obj#L347)) | #3431, [reset line 94](../../../areas/zon/flind.zon#L94) | #3441 | worship (197) |
| #3423 ([prototype](../../../areas/obj/flind.obj#L334)) | #3441, [reset line 96](../../../areas/zon/flind.zon#L96) | #3431 | worship (197) |
| #3404 ([prototype](../../../areas/obj/flind.obj#L51)) | #3435, [reset line 95](../../../areas/zon/flind.zon#L95) | #3462 | look (15) |
| #3410 ([prototype](../../../areas/obj/flind.obj#L127)) | #3462, [reset line 98](../../../areas/zon/flind.zon#L98) | #3435 | look (15) |

These form three reciprocal pairs and connect all four raw WLD components when all six routes and authored exits are available. Source placement does not prove the objects are loaded or that a character can use them.

## Compiled route targets and typed references

Room #3404, the Vertical Tunnel, also appears in two compiled destination tables. The illithid_teleport_veil handler’s 25-room array includes #3404; active object #96402 is bound to it and reset in Ixarkon. The handler accepts “enter veil” and selects a room from that table ([target list and handler](../../../src/specs/specs.ixarkon.c#L15), [binding](../../../src/specs/specs.assign.c#L1874), [Ixarkon reset](../../../areas/zon/ixarkon.zon#L156)). Psionic spell_mind_travel has a separate 25-room destination array that also includes #3404 ([handler and array](../../../src/classes/psionics.c#L609), [skill registration](../../../src/classes/skills.c#L1012)). These are compiled source routes; access and safe return were not tested.

Typed scans found no outside active ZON reset-role reference to a Flind mobile or object, no outside active QST item reference, no outside active SHP product, and no outside active type-25 object whose target is a Flind room. No active story sidecar reference to a local mobile or object was found. The source-code scan’s other matching numbers are unrelated constants, comments, or table indices; the two destination arrays above are the typed room-target matches.

## Title, history, and disposition

After color-code removal, “The Minor Plane of Planitude” is unique among all 442 current ZON titles. Followed same-stem history reaches 2013-11-13 for WLD, 2014-08-17 for MOB, 2017-02-08 for OBJ and ZON, and 2014-08-18 for SHP. The 2009 source-tree move appears in the histories. No alternate-stem predecessor, retirement, or explicit replacement is established by the checked-in title and followed histories.

Keep active flind DO NOT MOVE. Preserve the reciprocal underdark boundary, the key and door records, all six local teleport placements, the two compiled #3404 destination arrays, the local shop catalog, and the ud_map2 approach with its existing candidate source. Do not add a Flind candidate row or migration clearance. The source-only ud_map2 subset is now 18; curated totals are 3,461 overall, 221 for Surf, and 18 for ud_map2. Underworld2 retains eight approaches.

Static source evidence does not establish reset state, current door condition, shop operation, object availability, player access, safe return, or live traversal. No test, build, server, gameplay session, database operation, migration, or operational script was run for this documentation audit.

## Checkpoint 801: current typed-dependency census

The current reset file has 125 non-S commands and 113 typed mobile/object
operands: 52 M and seven F mobile slots, 19 E, 14 O, nine G, and twelve
object slots across six P commands. All typed prototypes and explicit reset
room operands resolve locally; no local reset import or unresolved typed
operand was found. A role-aware scan of every active and source-only ZON file
found no external reset reference to a Flind mobile, object, or room.
[Local reset source](../../../areas/zon/flind.zon#L81).

Flind has no local QST file or story sidecar. The all-source QST scan found no
Flind giver header or item field; typed sidecar keys in other story files have
no reference to a Flind room, mobile, or object. Its single local shop #3406
lists five local products (#3400, #3401, #3402, #3405, and #3432). No active
or source-only SHP file uses a Flind mobile as a keeper or lists a Flind object
as stock. No same-type mobile or object duplicate was found.
[Local shop](../../../areas/shp/flind.shp#L1).

Flind declares six local type-25 objects and no type-29 object. Their targets
are the three reciprocal room pairs already recorded above. Across all 872
type-25 and 380 type-29 declarations, no external target enters a Flind room.
These destinations add no authored WLD exits or source-only approaches.
[Portal prototype](../../../areas/obj/flind.obj#L51),
[portal reset](../../../areas/zon/flind.zon#L87),
[coverage row 98](ZONE_COVERAGE_REGISTER.csv#L99).

The exact-number source scan confirms that compiled destination arrays in
Ixarkon and psionic mind travel include local room #3404; no typed direct
mobile/object call or local prototype binding was found. Other numeric matches
are unrelated constants, comments, or table data. These source matches do not
establish access or successful traversal.
[Ixarkon target table](../../../src/specs/specs.ixarkon.c#L18),
[psionic target table](../../../src/classes/psionics.c#L615).

Keep the previously recorded DO NOT MOVE disposition, six local portal
placements, compiled #3404 targets, reciprocal underdark boundary, and
non-overlapping source-only ud_map2 approach. This static typed refresh does
not change Flind's WLD totals, candidate disposition, or owner questions;
reset state, shop operation, item availability, access, and persisted-player
behavior remain unqualified.
