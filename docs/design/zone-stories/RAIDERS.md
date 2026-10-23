# Ogre Raiders: setting and source story

**Checkpoint 335; coverage row 246.** This dossier records the active raiders source files and the separate inactive Surf map approach. Reset declarations, descriptions, and static exits are source evidence; they do not establish live actor counts, behavior, or a played quest.

## Area and travel setting

The active area has 21 rooms, six mobile prototypes, and three object prototypes. It has no local QST, native Q/QA contract, shop, or story sidecar. The WLD starts at [room #8900](../../../areas/wld/raiders.wld#L1), a narrow rock opening with loose footing and sharp angles. Its prose offers two possible origins for the fissure: a lightning strike or a small avalanche. Mounted travel is described as impossible. The active entrance is reciprocal with Surface room #593506, titled "Along a Crumbling Cobblestone Road" in the [Surface WLD](../../../areas/wld/surface.wld#L1963007).

The crevice runs south through #8901 and #8900; from #8902 its north exit opens into camp room #8916. The three crevice descriptions emphasize loose stone and increasingly narrow, hazardous footing. They do not define a special movement or damage rule.

## Camp, occupants, and local props

Rooms #8903 through #8918 use the same camp description: dirty straw, smoke, animals, dung, unwashed bodies, stained walls, and broken camping gear. This is a 16-room grid around the leader's hut and corral approaches.

Room #8919 is the Leader's Hut, assembled from cart flooring, sawed barrels, mud, broken crates, a ratty bearskin carpet, and a smoky torch. The Ogre Leader (#8900) surveys his band as if checking their loyalty. The ZON places him in the hut and equips the [dull black two-handed sword #8900](../../../areas/obj/raiders.obj#L1). Its edge is described as keen, though the sword has been used as a spit for animals.

Two half-orcs are described as natives of Sarmiz'Duul whose presence here is a mystery. The priest (#8901) looks for possible assassins and positions himself between the leader and the band. The shaman (#8902) bears ritual scarring associated with the BeastLord clan. The ZON loads both as followers in the hut and equips the [crude robe #8901](../../../areas/obj/raiders.obj#L16) and [gold leggings #8902](../../../areas/obj/raiders.obj#L33), respectively. The robe is sewn from former magical garments and retains a little power; the leggings appear torn from a larger set and held by crude straps.

The party also includes orc raiders (#8903), dirty bugbears (#8905), and Ratou (#8904), described as rat-and-hippopotamus crossbreeds that search for food. The bugbear's origin is unknown in its description, which calls it a fine fighter. Six repeated Ratou reset rows place them in the corral, room #8920. That room's prose describes a fence keeping its animals out of the living area and a floor covered in dung, rotting food, cloth, and decomposing bodies.

## Reset and story boundaries

Two ZON door resets close the fence exits between camp room #8910 and corral #8920. The source marks the doors closed and unlocked. This accounts for #8920 being outside the strict post-reset static walk from the Surface arrival; it does not qualify live door interaction or actor presence. Mobile reset rows record placements and source caps, not guaranteed live populations.

The separate [inactive Surf WLD source](../../../areas/wld/surf.wld#L1618817) has room #277145 leading north to active Raiders room #8900. This is map-source evidence only; it is not a second active entrance.

The active [SurfaceKeeps reset](../../../areas/zon/surfacekeeps.zon#L233) places local sword #8900 inside its enormous stitched hide sack, object #120028. Preserve this cross-area typed reference along with the sword's local leader equipment reset. It does not by itself establish a quest or current live placement.

No local quest contract or sidecar connects the half-orcs, BeastLord clan, sword, robe, leggings, party, or corral to progression. The source describes an inhabited raider camp and leaves several origins and relationships unexplained. The [audit review](../quest-story-followups/RAIDERS_AUDIT_REVIEW.md#checkpoint-335-builder-questions-and-recommended-resolutions) records owner questions without assigning new behavior or history.
