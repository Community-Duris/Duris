# Crystalspyre Mountains: glacier frontier and temporary keep encounter

Canonical key: `area:vrzawild`. Physical coverage row 341. The source title is
“Crystalspyre Mountains,” zone 370, with 100 rooms (#37000–#37099), 17 mobile
prototypes (#37000–#37016), and 15 object prototypes (#37000–#37014). The active
source files are [WLD](../../../areas/wld/vrzawild.wld),
[MOB](../../../areas/mob/vrzawild.mob), [OBJ](../../../areas/obj/vrzawild.obj),
and [ZON](../../../areas/zon/vrzawild.zon). There is no local QST, SHP, or
story-sidecar file. `vrzawild` does appear in [areas/AREA](../../../areas/AREA#L179)
and the [QST area list](../../../areas/qst/AREA#L166), but the QST list entry
does not supply a `.qst` contract.

## The authored story

The room text frames the mountains as a glacial frontier: an extensive crystal
canopy gives way to bare spines, deep chasms, a barrier ridge, lichened tundra,
a warming hut, a rose-lit ice dome, and a smooth tunnel. The 48-room canopy
(#37000–#37047) leads into the descending spines and snowfields (#37048–#37064).
The Barrier Wall and cliff trail (#37065–#37068) then descend toward the
Crystal Tundra, Valley of Eternal Snow, and warming-hut approach (#37069–#37090).
The [room source](../../../areas/wld/vrzawild.wld#L1) contains the full route
prose and its directional links.

The entry at #37090 and the durithalite slab reset there carry the clearest
plot note. The slab's engraved message calls Crystalspyre a planned road to the
fabled city of Verzanan, says a high-level temporary encounter will occupy the
end of the road until that city is completed, and warns that the road unfairly
challenges characters below level 20 while the final encounter is designed
for level 40 and above. The message is signed “Xyzom” ([slab prototype and
message](../../../areas/obj/vrzawild.obj#L18), [room #37090](../../../areas/wld/vrzawild.wld#L2252)).
The source describes an intended relationship to Verzanan; it does not define
a current WLD passage to the separate `verz1` rooms.

The environmental climb has three explicit 33% fall fields at rooms #37065,
#37066, and #37067 ([#37065](../../../areas/wld/vrzawild.wld#L1761),
[#37066](../../../areas/wld/vrzawild.wld#L1783),
[#37067](../../../areas/wld/vrzawild.wld#L1805)). The associated room text
describes an exposed switchback trail and a wall dropping toward the tundra.
Below it, #37085–#37089 form the warming-hut knoll. The hut contains a reset
fire and is guarded by three reset Bloodguards of Verzanan. Four D1 resets
start the hut doors closed ([door resets](../../../areas/zon/vrzawild.zon#L8),
[Bloodguard prototype](../../../areas/mob/vrzawild.mob#L156)). The closed state
is not a lock or a key requirement.

The optional upper route passes through the rose ice dome (#37091), a polished
ice tunnel (#37092–#37096), and three end rooms marked “TE”: the end of the
tunnel, the Foyer of the Keep of Zikata, and Zikata's throne (#37097–#37099).
The room prose describes a boundary into an outerworld keep and a throne with
positive and negative energy imagery ([keep entrance](../../../areas/wld/vrzawild.wld#L2408),
[foyer](../../../areas/wld/vrzawild.wld#L2431), [throne](../../../areas/wld/vrzawild.wld#L2452)).
The object note and `(TE)` titles both support the temporary-encounter reading;
their exact current design status remains a builder question.

## Encounter cast and reset placements

The local mobiles form environmental encounters rather than native quest
contracts. Dire wolves (#37000–#37004) inhabit the glacial route. A fiery ice
dragon (#37005) is reset at #37043 and receives the ice shield #37000. Frost
worms (#37006–#37010) occupy the tundra and lower snow route. The Bloodguard
(#37011) is described as a guardian of Verzanan's frontier; the ZON loads one
and two followers at the warming hut, with three equipment sets. The zone
uses reset mode 2 and has 66 M, 25 E, 4 D, 4 G, 2 O, and 4 F commands (105 in
total). Zikata (#37012)
is the two-faced demon lord at #37099. Yin (#37013) and Yang (#37014) are reset
as his followers; six demon minions (#37015–#37016) are placed in the foyer and
throne and carry tentamort spines (#37014). See the [mobile source](../../../areas/mob/vrzawild.mob#L156)
and [reset groups](../../../areas/zon/vrzawild.zon#L28).

The local object roster includes a scene-setting slab and fire, equipment worn
by the Bloodguards, the crown of opposition #37013, and minion spines #37014.
The crown's description says it belonged to Zikata, and the reset equips it on
him; no local QST accepts it. Zikata also receives imported object #358, a
rune-covered stone owned by Heavens. `specs.assign.c` binds #358 to the shared
`epic_stone` callback ([Heavens prototype](../../../areas/obj/heavens.obj#L3709),
[assignment](../../../src/specs/specs.assign.c#L1603)). Preserve that import as
a separate dependency; it does not create a Crystalspyre quest receipt.

The zone has no local QST file, shop file, or story sidecar. The QST index
membership alone does not describe a quest. No source-backed accepted
contract, completion flag, player reward, or “city reached” outcome is present
in these four local area files.

## Access and review limits

The active WLD entrance is reciprocal with Surface: Crystalspyre #37090 east
to Surface #523536, and Surface #523536 west back to #37090. Raw directed
traversal of all 280 resolving internal WLD exits, treating every authored
edge as traversable, reaches all 100 rooms from #37090 and returns to #37090
from all 100; the local graph has one strongly connected component. This is a
source-topology result only. It does not apply door reset state, the three
fall fields, actor condition, route choice, reset timing, or runtime scripts.
The inactive `surf.wld` also declares room #210777 north to #37090
([source-only approach](../../../areas/wld/surf.wld#L226153)); that old-map
record is not a current ingress or a proved return route.

The [repository-wide audit](../quest-story-followups/CRYSTALSPYRE_MOUNTAINS_AUDIT_REVIEW.md)
records the bounded title/history, external-reference, and source-only WLD
scans. The [builder questions](CRYSTALSPYRE_MOUNTAINS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
keep the Verzanan link, temporary encounter, area hazards, follower resets,
and imported stone under explicit owner review. This static source review does
not qualify installed availability, safe travel, active accounting, or a
played outcome.
