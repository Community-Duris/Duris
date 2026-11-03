# Shadamehr Keep: a floating fortress and its fallen paladin

This dossier maps the checked-in WLD, MOB, OBJ, and ZON source for active area shadamehr_keep. It has 25 rooms, six mobile prototypes, ten object prototypes, no local QST file, and no story sidecar. The zone has no native Q or QA contract. Its narrative is carried by room, mobile, and item descriptions and by the authored placement of keys, monsters, and treasure. Static source review does not establish current access, reset behavior, combat outcomes, or safe travel.

## Environmental story

The entrance at room #41200 describes a black-stone fortress floating above the Ethereal Plane. The keep is presented as an old stronghold once owned by a powerful knight. Rusted weapons and frayed tapestries frame the formal entrance halls, foyer, and small lounge. The descriptions move from faded courtly wealth into uncanny space: rooms #41210 through #41217 repeat the same magically disorienting hallway description, while #41209 is dark and rank, with shadows said to move around its visitor.

Past the metal door, room #41218 names the route as the entrance to the Unholy Chambers of the Death Knight. The nearby rooms lead into the Shadow Dragon's lair and then to the Death Knight's abode. Mobile #41201 is Merkus, described as a fallen paladin struck down by the gods. His armor still resembles a noble paladin's full plate, but it is blackened and burnt; the room text surrounds him with evil power and a hostile hiss. The lair contains old bones and half-eaten corpses, and its shadow dragon and Merkus are the named dangers in this progression.

The final north door leads to room #41223, the Death Knight's Treasure Vault. Its prose depicts piles of fabric, glowing equipment, and spoils amassed over many ages. The vault also receives several reset objects whose owners are external to this zone, so its inventory is not a self-contained Shadamehr collection.

## Authored route and key identities

1. Dreggan Woods room #41186 and Shadamehr entrance #41200 have reciprocal north/south exits. From the entrance, the local route runs through the trophy room and foyer toward the formal and magical hallways.
2. The metal north door at #41207 leads to #41218. The matching south door at #41218 returns to #41207. Both require local metal key object #41201, reset onto the undead beholder mobile #41203 in dark room #41209.
3. From #41218, the north route reaches Shadow Dragon lair room #41219. Its north ornate door and reverse at #41222 require ornate key object #41202, which the local ZON equips on Shadow Dragon mobile #41202.
4. Room #41222 houses Merkus. The local ZON gives him imported old rusted key object #41003, whose prototype is in Dreggan Woods. It fits the vault door between #41222 and #41223.
5. The six flagged door directions above are paired with six local D resets. The ZON comments describe them as closed/locked. Their exact runtime states and the availability of their keys require a live qualification.

Mobile and object VNUMs share some numbers but belong to separate namespaces. In particular, mobile #41202 is the shadow dragon while object #41202 is the ornate key; mobile #41203 is the undead beholder while object #41203 is Bloodlust.

## Reset and item ledger

The zone header is 41224 0 0 5 10 1, with reset mode 0. Do not infer supply, persistence, or a player-facing reset schedule from that header alone. The ZON source places the undead beholder in #41209, wraith groups in #41214 and #41217, a shadow at #41218, the shadow dragon in #41219, and Merkus in #41222; two banshee follow commands target #41222. Merkus is equipped with local plate #41205 and spectral sword #41210, plus the Dreggan Woods key #41003.

The vault's local floor resets include the green shimmering cloak #41207, mithril dagger #41208, and black boots #41209. Imported object resets place Heavens rune-covered stone #359 and Unique object #67219, the dark heart of Dreggan Wood, in the same vault. Another O reset names object #41002, but no active OBJ prototype with that VNUM was found. Dreggan Woods has a mobile #41002; a mobile definition does not satisfy an object reset. Preserve this unresolved typed reference for builder review.

The local cloak of shadow dragons, object #41204, is equipped on the shadow dragon rather than placed on the vault floor. Alatorin QST source removes #41204 as an input in a dragonscale-cloak recipe. The object is also referenced by compiled equipment support, so it is a cross-zone material and code dependency, not just local treasure. Object #41203, Bloodlust, is separately reset by Barovia onto Doru; it is not reset on Merkus in this zone. Its mobile/object namespace distinction matters.

The spectral sword #41210 is described as lying in the Death Knight's vault and refusing to budge, while the ZON equips it on Merkus. Object #41208 has a commented-out compiled assignment; that comment is not active special behavior. Other object properties and effects should be read from the prototypes rather than inferred from room prose.

## Quest, sidecar, and source limits

There is no areas/qst/shadamehr_keep.qst and no Shadamehr story sidecar, although areas/qst/AREA lists the stem. The coverage inventory records zero local Q/QA contracts and no shop. The story dossier therefore does not invent a native quest for defeating Merkus or the dragon. The only explicit external QST material transaction found here is Alatorin's use of local cloak #41204.

The six mobiles and their exact reset placements provide a source-backed threat map; the sources do not prove kill outcomes, key drops, or safe passage. The compiled references for local objects are object-special bindings and Chaos equipment data. No compiled room or mobile callback for the local VNUM range was found in the numeric source scan.

## Evidence reviewed

The [rooms](../../../areas/wld/shadamehr_keep.wld), [mobiles](../../../areas/mob/shadamehr_keep.mob), [objects](../../../areas/obj/shadamehr_keep.obj), [resets](../../../areas/zon/shadamehr_keep.zon), [Dreggan Woods key](../../../areas/obj/dreggan_wood.obj#L1), [Alatorin QST consumer](../../../areas/qst/alatorin.qst#L5775), and [Barovia Bloodlust reset](../../../areas/zon/barovia.zon#L336) were reviewed with active boundary and dependency registers. The repository-wide reconciliation and builder questions are in the [audit supplement](../quest-story-followups/SHADAMEHR_KEEP_AUDIT_REVIEW.md).
