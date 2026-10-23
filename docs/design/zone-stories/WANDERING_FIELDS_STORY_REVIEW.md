# The Wandering Fields: Llzazan's mobile staging network

Canonical key: area:wander. Physical coverage row 342. The active title is
“The Wandering Fields,” zone 366, with 14 rooms (#36600–#36613), 27 mobile
prototypes (#36600–#36626), and one object (#36600). The checked-in files are
[WLD](../../../areas/wld/wander.wld),
[MOB](../../../areas/mob/wander.mob),
[OBJ](../../../areas/obj/wander.obj), and
[ZON](../../../areas/zon/wander.zon). The active manifest lists wander at
[areas/AREA](../../../areas/AREA#L176); the [QST index](../../../areas/qst/AREA#L163)
also lists it, although no local wander.qst, SHP file, or story sidecar exists.
The index entry alone does not define a local contract.

## The authored purpose

Room #36600 is titled “Brads Wandering Mobile Zone.” Its description says it
loads wandering mobiles “in a somewhat random fashion,” identifies probability
rooms and loading rooms, and says this section is used for Llzazan Ghetto of
Arachdrathos, City of the Drow ([room #36600](../../../areas/wld/wander.wld#L1)).
The file therefore documents a mobile distribution layer. It does not author a
player-facing quest, a local town, or a completion outcome.

Six rooms name the intended destination bands. Each has six open WLD exits into
the active Llzazan contributor:

| Wander room | Authored role | Cross-zone exits |
| --- | --- | --- |
| #36601 | Rothe Area | #36280, #36281, #36278, #36288, #36283, #36276 |
| #36602 | Mushroom Grove | #36312, #36265, #36267 twice, #36263, #36313 |
| #36603 | Svirfneblin Slave Camp | #36210, #36212, #36214, #36205, #36207, #36209 |
| #36604 | Llzazan fungus farms | #36293, #36294, #36296, #36239, #36297, #36295 |
| #36605 | Lower Llzazan Ghetto | #36378, #36377, #36366, #36331, #36340, #36325 |
| #36606 | Upper Llzazan Ghetto | #36343, #36317, #36355, #36363, #36353, #36350 |

The full edge list is in [cross-area exit rows 493–528](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L493).
There are 36 directed links to 35 distinct Llzazan rooms; the duplicated
#36267 target is authored twice from #36602. None has a reverse WLD record.
These links give the source graph and ordinary mobile movement a route into
Llzazan, but no WLD evidence establishes a player entrance back into Wander or
a safe return to the staging rooms.

## Probability chain and terminal room

The other rooms form a one-way chain toward the loader and an exitless endpoint:

| Room | Title and authored chance | Local exits |
| --- | --- | --- |
| #36607 | 50 percent | Up to loader #36600; down to #36608 |
| #36608 | The Cosmic Toilet; “Mobs in this room wil not make it into game this boot.” | No exits |
| #36609 | 25 percent | Up to #36607; down to #36608 |
| #36610 | 12.5 percent | Up to #36609; down to #36608 |
| #36611 | Title says 6.25 percent; room text says 6.5 percent | Up to #36610; down to #36608 |
| #36612 | Title says 3.125 percent; no explanatory room text | Up to #36611; down to #36608 |
| #36613 | Title says 1.0625 percent; short text says it will not load often but should be powerful | Up to #36612; down to #36608 |

The values in #36611 and #36613 disagree internally or with the preceding
halving sequence, and #36612 has no explanatory text. The prose and exits show
the intended topology; they do not establish actual reset odds or prove that a
particular mob enters the game. Do not silently correct the displayed values.
See the [probability rooms](../../../areas/wld/wander.wld#L211) and
[reset placements](../../../areas/zon/wander.zon#L103).

## Residents and environmental story

The mobile text supplies the setting around the staging map:

| Mobile group | Source story |
| --- | --- |
| Gentle rothe #36600–#36603 | Small underworld cattle that graze on fungus and show little interest in other creatures ([prototype](../../../areas/mob/wander.mob#L1)). |
| Spore balls #36604–#36606 | Giant-mushroom spores looking for substrate in the southern Llzazan cavern ([prototype](../../../areas/mob/wander.mob#L57)). |
| Svirfneblin slaves #36607–#36610 | Deep gnomes condemned to Drow labor in the Arachdrathos region ([prototype](../../../areas/mob/wander.mob#L99)). |
| Drow commoners #36611–#36614 and exiled clerics #36615–#36618 | Ghetto residents scraping by after theft and exile from Lolth's clergy ([commoner](../../../areas/mob/wander.mob#L159), [cleric](../../../areas/mob/wander.mob#L211)). |
| Acrola #36619 | A feared underworld parasite said to attack Drow and Rothe; its description calls extermination a priority in Llzazan ([prototype](../../../areas/mob/wander.mob#L263)). |
| Rebellious svirfneblin #36620–#36624 | Enslaved workers who have decided to fight rather than remain captive ([prototype](../../../areas/mob/wander.mob#L279)). |
| Zarbon #36625–#36626 | Former Drow warriors transformed by House Zarbonesti into rib-cage-limbed creatures ([prototype](../../../areas/mob/wander.mob#L349)). |

The one local object is an adamantine barbed scimitar with hooked and serrated
edges ([object](../../../areas/obj/wander.obj#L1)). The ZON equips it on
Zarbons. The zone has reset mode 2 and 109 reset commands: 85 M, 10 E, and 14
G. Its environmental cast and equipment do not create a local quest or reward.

## Static graph and mobile-movement limits

All 54 authored WLD exits resolve: 18 are local and 36 leave Wander for Llzazan.
The 14 rooms form one weak component. Under an open-edge local graph, #36600
reaches itself and the six destination-selector rooms #36601–#36606. The
probability rooms #36607 and #36609–#36613 can each reach #36600, but none is
reachable from #36600; #36608 is an exitless sink. The six destination rooms
have no local return edge after they leave for Llzazan. There is no active WLD
exit into any Wander room and no inactive WLD file in the full source sweep
that targets one.

The shared NPC wandering code chooses a candidate direction and checks that an
exit can be used, the destination admits mobiles, and the destination is not a
no-ground sector. ACT_STAY_ZONE prevents an NPC from crossing a zone boundary.
The local mobile act fields do not set ACT_SENTINEL or ACT_STAY_ZONE, and none
of the 36 Llzazan target rooms is marked ROOM_NO_MOB. This source/code
combination supports the authored mobile-distribution role; it does not measure
live movement frequency, reset lifecycle, the probability text, or player
access. See [mobile wandering](../../../src/mob/mobact.c#L8018),
[movement gate](../../../src/cmd/actmove.c#L1339), and
[mobile flags](../../../src/core/defines.h#L1165).

No fall or current fields are authored, and no local door-reset commands are
present. That does not supply the missing inbound WLD route or turn the
one-way cross-zone edges into a return path.

## Review links

The [technical audit](../quest-story-followups/WANDER_AUDIT_REVIEW.md) records
exact typed dependencies, bounded title/history findings, and the retention
decision. The [twenty builder questions](WANDERING_FIELDS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
keep the probability text, route intent, source owners, and reset design with
the area owner.
