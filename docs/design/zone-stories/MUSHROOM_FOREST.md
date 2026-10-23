# The Mushroom Forest (mush1): static source review

Zone 101, source stem mush1, contains 128 rooms (#10100–#10227), 14 mobile prototypes, and seven object prototypes. Its active WLD graph has 1,001 authored exit declarations; the loader resolves 1,000 and drops one negative target. This is a static source map. It does not establish a player's actual route, the spawn behavior of its dispersal rooms, or live reset stock.

The normalized title scan finds one exact owner among all 442 current .zon files: mush1, “The Mushroom Forest.” Myconid (“Myconid Mushroom Forest,” stem myconid) and Mushroom Caverns (stem mushroom_caverns, zone 241) are separate active areas. Keep their source identities distinct absent an owner-confirmed relationship.

## Source identity and files

| Source | Reviewed evidence |
| --- | --- |
| [Active world manifest](../../../areas/AREA#L57) | Lists mush1 as a current full contributor, area note *101-102. |
| [QST index](../../../areas/qst/AREA#L51) | Lists mush1, but there is no local mush1.qst file. No local shop file or story sidecar is present. |
| [Zone header and resets](../../../areas/zon/mush1.zon#L1) | Zone #101, reset mode 2, 146 commands: 132 M, six E, four O, two D, and two F. |
| [Rooms](../../../areas/wld/mush1.wld#L1) | All 128 records from #10100 through #10227, with 1,001 D0–D9 exit declarations. |
| [Mobiles](../../../areas/mob/mush1.mob#L1) | Fourteen prototypes: #10100–#10109, #10111–#10114. There is no #10110 prototype. |
| [Objects](../../../areas/obj/mush1.obj#L1) | Seven prototypes #10100–#10106. |
| Compiled room behavior | No direct numeric assignment or local room procedure reference for the mush1 room/mobile/object VNUMs was found. |

## Forest, secret cave, and dispersal rooms

The room descriptions form five exact families across all 128 WLD records:

| Rooms | Description family | Source meaning |
| --- | --- | --- |
| #10100 | An opening in the tunnel | Introduces an enormous violet, blue, and white mushroom forest, abundant food, many Underdark residents, and the risk of getting lost. Its east exit reaches UnderDark #856059. |
| #10101–#10222 (122 rooms) | Inside the Great Mushroom Forest | The repeated prose describes a large river, extreme mushroom sizes, thick ground fungus that quickly overtakes paths, and occasional corpse- or armor-shaped growths as landmarks. |
| #10223 and #10226 | Within / Back of a Secret Cave | Both repeat the story of a battle-scarred drow exile, his protection of the area, and many charred skeletons. The prose says he killed “the dragon above.” |
| #10224 | Mob Dispersal Room 1 | Explicitly named as a mob dispersal room. |
| #10225 | Mob Dispersal Room II | Explicitly named as a second mob dispersal room. |
| #10227 | Disperal Room | Misspelled title, empty description, and exits into the forest grid. |

The local WLD graph is one weak component. From entrance #10100, authored directed edges reach 125/128 rooms when door state is ignored; all 128 rooms have an authored directed path back to #10100. The three rooms outside forward traversal are the two mob dispersal rooms and #10227. This is consistent with reset load rooms feeding outward toward the forest, but does not prove how NPCs move or whether players can enter those rooms.

The route into the two-room drow cave is a pair of WLD door exits: #10173 to #10223 and #10223 back to #10173; #10223 also connects to #10226. The zone resets both door directions to state 1, which means closed and unlocked in [door reset handling](../../../src/world/db.c#L7857). The raw exits do not carry the secret flag or explanatory exit text. The “Secret Cave” room titles alone do not make the entrance mechanically secret.

Room #10178 has an authored northeast exit with target -1. The exact [unresolved-exit register row](../quest-story-followups/WORLD_UNRESOLVED_EXIT_REGISTER.csv#L21) records that setup_dir rejects it before world renumbering. No room in this area has a fall or current record.

## Creatures and reset layout

The 14 mobile prototypes establish an inhabited forest and a drow-cave story; reset commands, not prose alone, determine which prototypes are placed:

| Mobile | Authored description role | Reset evidence |
| --- | --- | --- |
| #10100 myconid | Cares for the forest and carries herbs. | Nineteen M rows in dispersal rooms #10224/#10225. |
| #10101 troll-ogre mutant | Described as a peaceful forest wanderer with a bag of bones. | Six M rows in #10224/#10225; three E rows equip other local objects in the same reset sequence. |
| #10102 flaming spore ball | Rumored to be a drow food and cultivated delicacy. | Sixteen M rows in #10225. |
| #10103 svirfneblin | Supposedly sent from Valinhaven for spore balls; mentions a demon in the mines. | Fourteen M rows in #10225. |
| #10104 lost orc | Wears Black Order of Shady Grove ceremonial garb and has lost the way out. | Eleven M rows in #10225. |
| #10105 ugly grell | Lost from its hive. | Twelve M rows in #10225. |
| #10106 rothe | Grazes on the forest mushrooms. | Nine M rows in #10225. |
| #10107 spider | Black spider that spins a web among the mushrooms. | Fourteen M rows in #10224. |
| #10108 drow warrior | Exile and dragon-slayer who performs Lolth sacrifices and animates slain dragons. | One M row in cave room #10223. |
| #10109 undead dragon | Text says it is magically controlled by the warrior. | Two F follower rows target #10223. |
| #10110 | No prototype in the file. | No reset row found. |
| #10111 bugbear | Former Bugentolen resident. | Fourteen M rows in #10224/#10225. |
| #10112 cannibalizing fungus | Predatory fungus described as paralyzing prey. | Eleven M rows in #10224/#10225. |
| #10113 huge mushroom creature | Described as a sentinel that blocks the way and cannot communicate. The duergar also refers to it. | Prototype exists, but no local or external typed M reset use was found. |
| #10114 duergar mercenary | Refugee from Khildarak who feels safer behind the huge mushroom sentinel. | Five M rows in #10224. |

The two dispersal rooms receive repeated M reset rows rather than direct placements throughout the forest. There are 132 M rows across 12 loaded mobile prototypes; the row counts and limits are reset instructions, not live stock counts. No special assignment for the local room VNUMs was found.

## Objects and scripted-looking lore

| Object | Prototype and placement |
| --- | --- |
| #10100 mushroom stalk | Scenery/landmark item; three O resets place it at #10102, #10173, and #10222. Its extra description says it grew around the remains of a large, non-native dragon. |
| #10101 bloody ogre spine | Equipment object appearing in the troll-ogre reset sequence. |
| #10102 smooth batskin gloves | Equipment object appearing in the troll-ogre reset sequence. |
| #10103 polished bone collar | Equipment object appearing in the troll-ogre reset sequence. |
| #10104 gleaming mithril longsword | Equipment reset in the drow sequence. |
| #10105 small statue of a dark dragon | Equipment reset in the undead-dragon sequence. |
| #10106 shrine to Lolth | O reset at #10226 in the back of the secret cave. |

The drow mobile prose mentions a small staff containing a magical diamond; the local object list and reset file contain no staff prototype. The zone instead has a sword and repeated small dragon-statue equipment resets around the drow/follower group. The drow-cave WLD exits have key VNUMs 5 and 1, neither defined in the local object file; same-number object prototypes occur in unrelated area files. Because reset state 1 closes but does not lock these doors, those references do not establish an active key requirement.

## Active boundaries and source-only approach

Mush1 has one active outbound WLD edge: #10100 east to UnderDark #856059. UnderDark has the reciprocal #856059 west return ([cross-area pair summary rows 308 and 721](../quest-story-followups/WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L308); [outbound register row](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L196); [return row](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1785)).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no source-room VNUM overlap with mush1 and one non-overlapping source-only approach: unregistered [ud_map2.wld room #1124761](../../../areas/wld/ud_map2.wld#L56451) goes north to #10100. It is not active ingress. The existing ud_map2 candidate record is updated to preserve it separately from active UnderDark access.

## Bounded typed-reference closure

Mush1 has no local QST, SHP, or story sidecar despite its QST/AREA registration. Exact bounded scans found no outside active/unregistered QST item command, shop product, typed ZON reset consumer, story-sidecar item_vnums reference, or duplicate foreign mobile/object prototype for local VNUMs #10100–#10114 and #10100–#10106. The sweep of 872 parsed type-25 object prototypes found no fixed destination inside mush1. Numeric matches in C/C++ were limited to Unicode codepoint ranges and a source comment with a line number; no compiled room or prototype consumer was identified.

The local reset file contains no imported mobile prototype. The area has encounter and lore material, but no source-defined item exchange, quest giver, local payment contract, or story-credit handler. This static closure does not qualify indirect tooling, player access, live resets, or persistent gameplay state.

## Title history and review status

The current normalized title has one exact owner among all 442 .zon files. Same-stem WLD, MOB, OBJ, and ZON history reaches the 2009 areas-tree move, with later edits in 2010, 2012, 2013, and 2015. No alternate-stem predecessor, tracked rename, or whole-zone replacement claim was found in those paths. No local QST, SHP, or story-sidecar file/history exists.

Keep active mush1 in place. The [CP297 audit supplement](../quest-story-followups/MUSHROOM_FOREST_AUDIT_REVIEW.md#checkpoint-297-all-442-title-and-direct-builder-questions) records the title/history and dependency findings plus 20 owner questions. No source, route, or active archive-candidate change is proposed.
