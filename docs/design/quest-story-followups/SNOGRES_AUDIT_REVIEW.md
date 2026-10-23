# Checkpoint 366: Valley of the Snow Ogres audit

Physical coverage line 279, `area:snogres`, is reconciled with the existing [source-comprehensive story map](../zone-stories/VALLEY_OF_THE_SNOW_OGRES.md). That map remains the detailed account of the nine native contracts, reset source quantities, interactions, and assigned procedures. This supplement closes the register's static connectivity, typed-dependency, title/history, and builder-question reviews. It does not qualify installed gameplay or repair any source behavior.

## Active graph and source identity

The active [Snow Ogres WLD](../../../areas/wld/snogres.wld) parses as 100 rooms (#87700–#87799) and 281 authored directions. Of those directions, 280 stay in the area and one leaves from #87700 up to Surface #620605. All targets resolve in active WLD sources. The local graph has one weak component; authored directed traversal from #87700 reaches all 100 rooms. Surface #620605 has the reciprocal down direction. Faerie Realm room #14210 has a separate northwest direction into smithy #87777, with no Snow Ogres return direction. The active boundary census is two inbound, one outbound, and one reciprocal pair.

The target WLD has no parse issues. The wider active-source scan also found no other direction into these room VNUMs. It recorded one unrelated malformed exit block in `northern_wilderness.wld` near line 1232; its visible target is #12270, outside this area. The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 directions without errors. It found no same-numbered Snow Ogres room and no inactive direction into the area. No candidate WLD row or source-only approach is added.

The three directions from room #87708 with exit flag 4—north to #87709, east to #87712, and south to #87713—have no matching D reset. Four other flagged directions match exact local D resets at 100 percent: #87717 west to #87718 (flag 13), #87718 east to #87717 (flag 5), #87745 south to #87746 (flag 5), and #87746 north to #87745 (flag 5). Preserve the raw flags and reset pairing; the separate story map documents the switch, secret/closed states, and movement guards. Three fall-chance room records remain part of the source census, including the slope declaration the loader clears because no down exit exists; do not turn that record into a new required fall route.

## Typed dependencies

Typed scans keep room, mobile, and object identity separate because the local ranges overlap numerically: 44 room/mobile IDs and all 38 object IDs also occur in another local type namespace. Same-type scans across active and source-only WLD, MOB, and OBJ files found no other file reusing a Snow Ogres ID.

Active external consumers and reset sources are:

- [Newbie QST line 155](../../../areas/qst/newbie.qst#L155) gives object #87700, which is the local Berserker mobile's number in the mobile namespace. Four [White Harbor QST commands](../../../areas/qst/wh.qst#L3311) give local object prototypes #87721, #87720, #87723, and #87722 to four snow-ogre shamans. The active [Newbie story sidecar](../../../areas/story/newbie.story.json#L446) and [White Harbor story sidecar](../../../areas/story/wh.story.json#L1053) repeat those exact item VNUMs in typed `item_vnums` fields. These are five distinct object consumers; equal-numbered mobiles do not change their item identity.
- The active [Surface reset at line 1735](../../../areas/zon/surface.zon#L1735) places mobile #87742, Leppts, at room #660001. This is an external physical encounter for a Snow Ogres contract owner, not a WLD boundary or proof that the local contract owner changed.
- Local [Snow Ogres resets](../../../areas/zon/snogres.zon#L206) and [line 210](../../../areas/zon/snogres.zon#L210) refer to active Heavens object #360, “monolith rune-covered,” and object #1230, “totem mastery,” on local mobile reset chains. Both object prototypes are in [heavens.obj](../../../areas/obj/heavens.obj#L3741) and [heavens.obj](../../../areas/obj/heavens.obj#L9638). Heavens also has mobile #1230, so preserve the object type when recording that reset.
### Source-only legacy reset reference

- Inactive [brass-old-1.zon line 815](../../../areas/zon/brass-old-1.zon#L815) has a G reset reference to Snow Ogres object #87725, the remorhaz hide. It is not active hide supply. Preserve this source-only typed reference on the existing [brass-old-1 candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17) while that contributor remains unregistered and incompletely reviewed.

There is no local `snogres.shp` file and no external SHP product listing for a local Snow Ogres object. A scan of all 713 active type-25 object prototypes found no fixed destination in the area. The sidecar scan found no additional external typed item references beyond the five Newbie/White Harbor entries above.

The bounded direct numeric scan of maintained C/C++ found four relevant source files. [Chaos equipment profiles](../../../src/account/chaos_eq_data.h#L82) reference local objects #87704, #87707, #87709, and #87727 across twelve profile lines; the account-entry loader consumes those profile records. [randomeq.c](../../../src/item/randomeq.c#L129) names local mobiles #87741, #87700, #87709, and #87729 in its random-equipment list. The twelve literal assignments in [specs.assign.c](../../../src/specs/specs.assign.c#L2192) and the complete [Snow Ogres procedure unit](../../../src/specs/specs.snogres.c) remain as described in the story map: the second #87734 assignment replaces the earlier block-direction binding, and the Lich's teacher flag does not add a generic teacher binding or a local epic-teacher table entry. The scan found no other direct local VNUM hit in maintained C/C++ source.

## Title, same-type identity, and followed history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “Valley of the Snow Ogres” occurs once among all 442 current ZON titles and once among the 351 active contributors. No active or source-only WLD, MOB, or OBJ header duplicates one of this area's IDs in the same file type. The inactive brass-old-1 object reset is an item-level source reference, not evidence that brass-old-1 is a Snow Ogres predecessor or replacement.

`git log --follow` for the WLD, MOB, OBJ, ZON, and QST paths reaches the 2009-08-12 areas-tree move. The latest tracked WLD edit is from 2013, MOB from 2014, OBJ from 2026-09-14, ZON from 2015, and QST from 2013. The story sidecar was added on 2026-10-03. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in the followed paths or current manifest. There is no local SHP history. Keep active `snogres` DO NOT MOVE.

## Builder questions and conservative recommendations

These twenty questions record the decisions that should stay explicit while keeping current source identities and receipts intact.

1. Should the active `snogres` contributor remain in place? Keep it DO NOT MOVE; the source map, QST, reset, and assigned procedure contracts share this identity.
2. Does the derived catalog span beginning at #87683 reserve #87683–#87699 as actual rooms? Keep the WLD range at #87700–#87799; do not create rooms from the derived span alone.
3. Should the reciprocal Surface boundary remain the ordinary valley route? Preserve #87700 up to #620605 and the Surface return until an owner specifies a replacement.
4. Should Faerie Realm #14210 remain a one-way source entrance into smithy #87777? Preserve the authored inbound edge; do not invent a reverse direction or mark it reciprocal.
5. Should a candidate map be added despite the inactive-WLD sweep finding no approach or room collision? Add none; keep the zero inactive approach result attached to the completed scan.
6. Are the three flag-4 exits from #87708 intended to have no D reset? Preserve all three source states until the builder decides whether they are closed barriers, reset-controlled doors, or something else.
7. Should the four flag-13/flag-5 exits keep their exact 100-percent D resets? Preserve all four pairings and chance values; do not change one side without deciding the full door behavior.
8. Is object #87735 intended to clear only the west blocked bit at #87717? Preserve the current switch effect and require a separate decision for revealing/opening the still secret and closed door.
9. Should pit fiend #87741 remain a separate 40-percent source and westward movement guard? Keep its independent guardian role explicit; do not treat the switch as satisfying every movement prerequisite.
10. Should the #87745/#87746 astereater doorway remain secret and closed but unlocked? Preserve the authored access state; do not create a key quest from the door description.
11. How should six-hide recipes be supplied when three active remorhaz declarations cap object #87725 at three each? Choose intended accepted source quantities and test six-copy allocation, consumption, retained stock, and replacement before changing the recipe.
12. What accepted source should issue sundew stalk #87719? Add or identify a deliberate producer before treating ordinary sundews or giant #87735 as its source.
13. Should Leppts' Snow Ogres contract continue to use his active Surface reset at public room #660001? Keep the foreign placement visible as such; qualify area discovery, encounter ownership, dispersal, and any stay-zone policy before publishing it as reliable access.
14. Should the Newbie and White Harbor consumers continue to accept the five exact Snow Ogres objects? Preserve object #87700 and the four distinct shaman totems, with their active QST and sidecar references; do not collapse equal-numbered mobile IDs into item history.
15. Should the valley's reset chains continue to supply Heavens objects #360 and #1230 to local mobiles? Preserve the typed reset commands and decide separately whether each foreign prototype is intended stock in this area.
16. Should inactive brass-old-1's G reset to hide #87725 contribute to active source quantity? Keep it on candidate row 17 as source-only; do not count it as an active reset or repair the hide supply calculation with it.
17. Should the remorhaz retaliation callback remain a journal-worthy effect? Its current `damage(ch, vict, ...)` call targets the remorhaz owner rather than the attacker; keep effect credit pending a target correction and callback-lifetime qualification.
18. What helper population and retreat behavior should the flesh golem enforce? Resolve the small/large helper-count mismatch, null periodic opponent, virtual-room placement, and replacement-pointer lifetime before qualifying bounded combat support.
19. How should the cadaver-pit repop timer behave across missing prototypes, failed creation, recovery, and repeated pulses? Add lookup/spawn/place checks and define the intended accepted generation and cap before granting any corpse-reassembly credit.
20. Should the dormant berserker throw, weapon-hit effects, or skull-leggings auto-equipment count toward progression? Keep them separate from source recovery and quest credit until combat dispatch, safe actor/object identity, and durable equipment custody are qualified; the story map retains the individual repair findings.

## Register updates

Coverage row 279 now links this supplement and closes static connectivity, title/history, typed-dependency, and builder-question review. The existing source story map remains the mapped narrative artifact. The inactive WLD review adds no candidate approach, so the curated source-only WLD total remains 4,849, including 283 Surf records. The source-only brass-old-1 reset is appended to its existing candidate row, without changing the WLD count.
