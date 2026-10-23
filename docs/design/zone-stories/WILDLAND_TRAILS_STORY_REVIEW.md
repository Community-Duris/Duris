# The Wildland Trails: source story review

Canonical key: `area:wildland_trails`. Physical coverage row 346. The active
zone contains 99 rooms, 25 mobiles, 13 objects, and 142 reset commands. It has
no local QST, SHP, or story sidecar. This review reads its room and prototype
descriptions as environmental storytelling and keeps live access, combat, and
item recovery unqualified.

## Geography and authored movement

The trail is presented as a worn, increasingly overgrown route through scrub
and the forest edge. Rooms describe holes, thorn plants, reduced visibility,
and stretches with an eerie absence of animal sounds. It opens into the River
Shining and banks marked by canoe and raft traffic, then broad grass fields and
the rocky Moorland Plains. A watering hole punctuates the plains. At the
outpost ruins, the prose speculates that a trail may once have been a caravan
route, perhaps between Daggerford; preserve that uncertainty as speculation.

The northward route rises into the Hills of Quistery and the Quistery Ravine.
The ravine's steep walls, runoff stream, broken rock, and cave openings
contradict the more pleasant green hills above. A twisting tunnel contains a
locked storage room and a larger cave occupied by a verbeeg and worgs. A second
track crosses a pine grove beside the river. There, a clearing and fire pit
lead into the Welgurd tribal camp, with family, fishing, hunting, and
chieftain's tents. A separate track runs deeper into the forest; the Decaying
Pit is a shallow cavity around plant roots and rotting remains.

## Welgurd camp, objects, and unresolved stories

Room prose and mobiles describe a working community rather than a quest hub:
fishermen mend nets and spear fish, hunters carry stone axes, families share
deerskin tents, and a chieftain carries an heirloom war axe. The text spells
the tribe as both “Welgurd” and “Welgund.” The bone totem depicts a salmon,
wildcat, grass field, and brush fire beneath runes the reader cannot decipher.
The spiritist's medicine pouch is described as sacred and protective by tribal
belief; that description does not establish a native service or a quest reward.

The source has no local QST or sidecar that resolves the mysterious runes,
chieftain's axe, ruined outpost, cavern occupants, or corpse in the pit. Keep
those as authored environmental leads until a builder identifies an intended
story endpoint.

The reset table loads carved wooden key #14600 in the Decaying Pit. The
Quistery storage-room door in both directions references that object VNUM as
its key. The storage-room reset also places crate #14602, with bracelet #14603
and crystal rod #14604 reset inside it. Fishermen receive fishing spears and
several reset groups place a stringer of river fish; these are fixed source
placements, not evidence of a live fishing loop or guaranteed player supply.
Mobile #14600 is a separate rotting-corpse prototype: its matching number with
key object #14600 is a type distinction, not duplicate identity.

## Special procedures and behavior boundary

The source assigns periodic poison to thorn-slinger #14601, paralysis to
strangle-vine #14602, blindness to red-musk creeper #14603, and
`barbarian_spiritist` to the Spiritist #14605 in [the assignment table](../../../src/specs/specs.assign.c#L994).
Their implementation is in [the Wildland Trails special procedures](../../../src/specs/specs.wildland_trails.c).
The three plant routines can apply their named effects after their chance and
save checks. The similarly described thick vine #14624 has no matching special
assignment in the reviewed table.

The Spiritist routine selects among spoken ritual, curse, weapon-disarm, and
cyclone branches. One branch marks a player's equipped weapon `ITEM_SECRET`
before unequipping it into the room. Static code review does not establish
whether a dropped weapon can be found or recovered under live conditions.
Likewise, `plant_attacks_paralysis` contains death cleanup for
`SPELL_MAJOR_PARALYSIS` after an earlier guard returns on any nonzero command;
`CMD_DEATH` is -1, so that cleanup branch is unreachable in the reviewed
control flow. Treat this as a source finding for owner review, not a verified
in-game result.

## Builder direction

Keep the area's place names and terrain transitions, the uncertain outpost
history, tribal spellings, and the explicit key-to-storage-door relationship
visible in review. Confirm the source-only map relationship and player routes
before describing ordinary access. Preserve the lack of native quest receipts
unless a builder chooses to author them; do not infer an acquisition path or
recovery guarantee from descriptive prose or static reset placement. The
[bounded technical audit](../quest-story-followups/WILDLAND_TRAILS_AUDIT_REVIEW.md)
records map identity, dependency, history, and retention evidence.
