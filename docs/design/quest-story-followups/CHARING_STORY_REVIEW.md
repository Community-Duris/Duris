# Charing: source story review

**Scope:** source review of 210 WLD rooms, 95 mobile prototypes, 58 local
objects, 496 zone commands, 14 shop records, and the authored story sidecar.
This maps environmental narrative, static routes, and selected typed
references. It does not qualify live shop behavior, special procedures,
transport, or safe player traversal.

## Story coverage state

Charing is an active full-world source with the AREA note “*450–452 (new elf
hometown)”
([AREA entry](../../../areas/AREA#L212)). The frozen register records zone
header 450, 210 rooms, 95 mobiles, 58 objects, no native Q/QA contracts, and
14 shops ([coverage row](ZONE_COVERAGE_REGISTER.csv#L55)). Charing also appears
in the separate quest index
([index entry](../../../areas/qst/AREA#L195)), but there is no
areas/qst/charing.qst file in the active source set.

The story sidecar marks coverage complete and invites players to meet Charing's
people to uncover local requests, while its contacts, stories, and exclusions
arrays are empty ([coverage](../../../areas/story/charing.story.json#L5),
[introduction](../../../areas/story/charing.story.json#L8),
[contacts](../../../areas/story/charing.story.json#L11),
[stories](../../../areas/story/charing.story.json#L12),
[exclusions](../../../areas/story/charing.story.json#L13)). The reviewed source
therefore maps environmental setting and resident hooks, but no local QST
contract or objective. The sidecar wording should not be read as evidence of
quests that are absent from the native source.

## Environmental story

| Setting | Source evidence | What the room and resident text establishes |
| --- | --- | --- |
| Hometown streets and forest approach | [city center](../../../areas/wld/charing.wld#L1), [gate approach](../../../areas/wld/charing.wld#L425), [Way of the Royals](../../../areas/wld/charing.wld#L18), [forest trail](../../../areas/wld/charing.wld#L3917) | The center is a meeting place for elves and visiting centaurs, with a mermaid fountain and maintained streets. The named roads and trails lead through an elven forest; gate prose says only people “of proper origin” may pass, without defining that rule in the reviewed static map. |
| Druid grove, temples, and peace | [grove entrance](../../../areas/wld/charing.wld#L566), [druid sanctuary](../../../areas/wld/charing.wld#L588), [Temple of Nature](../../../areas/wld/charing.wld#L605), [Ancient Temple of Faith](../../../areas/wld/charing.wld#L659), [Shrine of Peace](../../../areas/wld/charing.wld#L776) | Druid elders maintain an ancient sanctuary and a living-tree grove dedicated to Tvulia Barha. A rose-quartz temple serves the clerics, while the shrine describes a protected place of calm beneath an observing oak. Hasira, the great druid, and Mevian, the high cleric, teach residents; their source profiles do not define local quest outcomes ([Hasira](../../../areas/mob/charing.mob#L86), [Mevian](../../../areas/mob/charing.mob#L319)). |
| Royal household and civic order | [palace entrance](../../../areas/wld/charing.wld#L1200), [queen's chamber](../../../areas/wld/charing.wld#L1418), [throne room](../../../areas/wld/charing.wld#L1474), [jail](../../../areas/wld/charing.wld#L1864), [judge's chamber](../../../areas/wld/charing.wld#L3210) | The quartz palace is home to King Solirum and Queen Selistra, with court, royal chambers, a library, guards, and treasury. A courthouse and jail present a formal law system that includes captives awaiting execution. The queen, king, judge, and clerk are reset residents, not mapped objective givers in the reviewed quest files ([Solirum](../../../areas/mob/charing.mob#L117), [Selistra reset](../../../areas/zon/charing.zon#L417), [judge](../../../areas/mob/charing.mob#L1264)). |
| Guilds, trade, and performing arts | [Warriors' Guild](../../../areas/wld/charing.wld#L884), [Rogues' Den](../../../areas/wld/charing.wld#L2655), [Rangers' Guild](../../../areas/wld/charing.wld#L2788), [Grovetree Theatre](../../../areas/wld/charing.wld#L2989), [Grovesinger's practice room](../../../areas/wld/charing.wld#L3087) | The city supports warrior, rogue, ranger, and bard communities alongside merchants, mount sellers, instrument makers, bars, and guild shops. Diam is identified as master thief, Gendel Ginzi as great ranger, and Aliel Lehnalia as the Grovesinger; these are social and training roles in source text, not proof of quest chains ([Diam](../../../areas/mob/charing.mob#L336), [Gendel](../../../areas/mob/charing.mob#L983), [Aliel](../../../areas/mob/charing.mob#L999)). |
| Necromancy, danger, and local dumping | [necromancy tower](../../../areas/wld/charing.wld#L2920), [lab of flesh](../../../areas/wld/charing.wld#L2973), [city dump](../../../areas/wld/charing.wld#L2761) | A dead-wood tower and a lab containing dead elves, humanoids, and animals are presented as a blight on the otherwise serene city. The dump contains a glowing portal, but its destination or special behavior is not mapped by these WLD descriptions. |
| Great Tree and river forest | [Great Tree base](../../../areas/wld/charing.wld#L3617), [outside the tree](../../../areas/wld/charing.wld#L3637), [river](../../../areas/wld/charing.wld#L4033), [forest route](../../../areas/wld/charing.wld#L3956) | The Great Tree is a carved and inhabited landmark above an interlocking canopy. Forest trails carry elf and centaur tracks. River prose describes a strong current, but the coverage census records zero configured current rooms and zero fall-chance rooms; neither prose nor census alone qualifies live hazard behavior. |

The residents and room text include further personal hooks: the Oracle's
unsettling gaze, Morlander and Morlyntra's magical training, an out-of-place
visitor from Tharnadia, a lost human, twins described as abandoned, and
shackled drow and orc captives
([Oracle](../../../areas/mob/charing.mob#L217),
[Morlander](../../../areas/mob/charing.mob#L276),
[Morlyntra](../../../areas/mob/charing.mob#L1233),
[visitor](../../../areas/mob/charing.mob#L1048),
[drow prisoner](../../../areas/mob/charing.mob#L1033),
[abandoned child](../../../areas/mob/charing.mob#L1343)). These descriptions
set tone and suggest questions for the area owner; they do not define player
objectives or outcomes.

## Residents, resets, and shops

The zone file contains 496 typed commands: 275 M, 95 G, 59 E, 58 D, 8 O, and
1 F. The M, G, and E placements populate civic, temple, palace, guild,
commercial, forest, and household spaces. One example is the palace sequence:
elite guards are placed in the queen's outer rooms, with an opal key carried by
the guard in room 45063; another guard in room 45066 carries a glowing emerald
key ([palace resets](../../../areas/zon/charing.zon#L411)).

The shop file contains 14 records. Their configured rooms match the local
keeper reset placements:

| Keeper | Shop room | Source record |
| --- | ---: | --- |
| Armorer, 45014 | 45046 | [shop](../../../areas/shp/charing.shp#L1) |
| Weaponsmith, 45017 | 45038 | [shop](../../../areas/shp/charing.shp#L50) |
| Shanya, 45018 | 45035 | [shop](../../../areas/shp/charing.shp#L90) |
| Rogue bartender, 45028 | 45128 | [shop](../../../areas/shp/charing.shp#L133) |
| Elven bartender, 45037 | 45040 | [shop](../../../areas/shp/charing.shp#L170) |
| Alkatia, 45053 | 45153 | [shop](../../../areas/shp/charing.shp#L207) |
| Antwin, 45054 | 45138 | [shop](../../../areas/shp/charing.shp#L247) |
| Elven ranger bartender, 45055 | 45135 | [shop](../../../areas/shp/charing.shp#L288) |
| Kazoon, 45058 | 45110 | [shop](../../../areas/shp/charing.shp#L325) |
| Ishtari, 45059 | 45112 | [shop](../../../areas/shp/charing.shp#L362) |
| Guild shopkeeper, 45060 | 45041 | [shop](../../../areas/shp/charing.shp#L403) |
| Tricky-looking storekeeper, 45061 | 45130 | [shop](../../../areas/shp/charing.shp#L443) |
| Renim, 45062 | 45149 | [shop](../../../areas/shp/charing.shp#L477) |
| Nelanna, 45063 | 45144 | [shop](../../../areas/shp/charing.shp#L518) |

The shop records and reset stock import object prototypes from active
contributors. The combined local ZON and SHP references include Heavens objects
54, 191, 203, 204, 256, 283, 361, 363, 364, 368, 369, 376, 390, and 396;
Verz1 objects 3001–3004, 3021, 3023–3025, 3040–3046, 3054, 3069–3071,
3075–3076, 3080, 3085, and 3097; Kobold object 1403; Valley Crushk object
2020; Plane of Fire One object 25400; and Orrak object 9017. Representative
definitions are [Heavens 191](../../../areas/obj/heavens.obj#L2274),
[Verz1 3040](../../../areas/obj/verz1.obj#L778),
[Kobold 1403](../../../areas/obj/kobold.obj#L55),
[Valley Crushk 2020](../../../areas/obj/valley_crushk.obj#L205),
[Plane of Fire One 25400](../../../areas/obj/plane_fire_one.obj#L1), and
[Orrak 9017](../../../areas/obj/orrak.obj#L196). Object 54 also occurs in
unregistered source files for Dalvik, Hostel, and Mavulsk; Heavens is the
active-list definition, while the alternates remain source-only
([Heavens](../../../areas/obj/heavens.obj#L800),
[Dalvik](../../../areas/obj/dalvik.obj#L693),
[Hostel](../../../areas/obj/hostel.obj#L707),
[Mavulsk](../../../areas/obj/mavulsk.obj#L893)).

Six reset or shop item VNUMs have no matching prototype definition in the
current areas/obj sources: 3032, 3902, 5214, 12581, 12584, and 18280. They are
referenced in the shop records for Shanya, Antwin, and Ishtari and in reset
commands; the source comments themselves mark several as absent from the local
object file ([Shanya stock](../../../areas/shp/charing.shp#L90),
[Antwin stock](../../../areas/shp/charing.shp#L247),
[Ishtari stock](../../../areas/shp/charing.shp#L362),
[reset references](../../../areas/zon/charing.zon#L323),
[later reset references](../../../areas/zon/charing.zon#L479)). Preserve these
numeric references until their intended owners or generation source are
confirmed.

## Doors and static connectivity

The register records 467 authored direction slots: 465 target rooms resolve
in the active source world, and two positive targets are absent from that
world. The unresolved exits are room 45185 north to 209060 and room 45205 south
to 209660; each target occurs only in the unregistered surf and surface2011
WLD sources and is pruned by the normal full-world loader
([unresolved entries](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L216),
[second unresolved entry](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L217)). This is
distinct from the current Surface route: room 45183 leads west to room 513692,
which returns east to Charing. Surface room 513693 also has an east exit to
45183 but no reverse record
([Charing exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L622),
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1332),
[one-way Surface edge](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1333)).

The row therefore records two inbound cross-zone exit records, one outbound,
and one reverse-direction match. Its local weak-exit graph has three
components: 208 rooms in the largest and two isolated source rooms,
45037 and 45151. Room 45037 is labeled a pet-shop loading room. Room 45151 is
labeled as the mount-sale stable and receives resets for a horse, unicorn, and
pegasus, but has no WLD exits
([pet-shop room](../../../areas/wld/charing.wld#L861),
[stable room](../../../areas/wld/charing.wld#L3204),
[mount resets](../../../areas/zon/charing.zon#L606)). The connected room 45150
describes a stable entrance and one exit to room 45157
([entrance](../../../areas/wld/charing.wld#L3187)). These room records may rely
on commands or special behavior that this static pass did not trace; the graph
does not prove player reachability or inaccessibility.

The zone resets mark the Shrine-to-Palace boundary locked. Its WLD key field
names object 45004, an elven runed key reset on the traveler mobile in room
45161. The queen's and throne-room doors name keys 45026 and 45028, reset on
guards immediately outside those chambers; the jail-cell door names key 45006,
reset on the jailkeeper in room 45100
([palace door resets](../../../areas/zon/charing.zon#L72),
[opal key reset](../../../areas/zon/charing.zon#L416),
[emerald key reset](../../../areas/zon/charing.zon#L422),
[jail key placement](../../../areas/zon/charing.zon#L467),
[traveler key placement](../../../areas/zon/charing.zon#L635),
[runed key prototype](../../../areas/obj/charing.obj#L37),
[opal key prototype](../../../areas/obj/charing.obj#L250),
[emerald key prototype](../../../areas/obj/charing.obj#L270),
[jail key prototype](../../../areas/obj/charing.obj#L62)).
These are authored state and placements only; reset execution, ordinary access,
and key acquisition have not been tested.

The Great Tree retains an asymmetric hidden-door declaration. The current WLD
exit from the base, room 45168, is closed but not marked hidden; the reverse
exit from room 45169 is closed and secret
([base exit](../../../areas/wld/charing.wld#L3627),
[outside exit](../../../areas/wld/charing.wld#L3649)). Commit
227adc0f82c277a9eb779c8c0fd866b03df756b2, dated 2014-08-05, changed the
Charing-side exit from hidden to non-hidden while its message explicitly said
the other entrance remained hidden. The reset file still describes the
outside route as secret ([reset](../../../areas/zon/charing.zon#L228)).

The WLD room for the Grovesinger's practice chamber has one north exit, but a
bare literal D1 appears at the beginning of that exit's description before
“A heavy door allows access”; it is description text, not a second exit
record ([source lines](../../../areas/wld/charing.wld#L3087)). Confirm whether
the extra token is an editing artifact.

## Typed dependency and history boundary

An active shipyard source has two reset commands that place Charing object
45018 ([first reset](../../../areas/zon/shipy.zon#L516),
[second reset](../../../areas/zon/shipy.zon#L517)). The unregistered
nexus56 sources also reference Charing object 45037 in a reset and shop stock;
these are source-only leads, not active full-world inputs
([reset](../../../areas/zon/nexus56.zon#L1367),
[shop stock](../../../areas/shp/nexus56.shp#L455)). The typed active reset scan
found no external M command placing a Charing mobile. No local QST source
exists; QST numeric operands that resemble VNUMs include currency or
experience values rather than mobile/object references.

The active title scan finds no repeated Charing title, and the
duplicate/replacement report records no Charing-specific retirement or
replacement ([history review](DUPLICATE_AND_REPLACED_ZONES.md)). The 2014
Great Tree change is the notable route-history clue. It does not establish
that another hometown or forest zone replaced Charing. Keep Charing in its
current active source location; no attic move is supported.

## Builder questions and safe working resolution

1. **Is the sidecar's empty complete scope intentional?** Decide whether
   Charing is meant to be environmental-only. If so, align its introduction
   and coverage label; otherwise map only verified contacts and objectives.
   Do not infer quests from the Oracle, royalty, guild masters, or other named
   residents.
2. **What is the intended access rule at the hometown gate and palace?**
   Confirm what “proper origin” means in the gate prose and whether the
   runed key on the traveler in room 45161 is intended to open the
   Shrine-to-Palace door.
3. **Are the isolated pet-shop and stable rooms intentional loading rooms?**
   Confirm whether room 45037 and the mount room 45151 are reached through
   commands or special behavior and whether their descriptions need to explain
   that route.
4. **Which object sources own the unlisted reset/shop values?** Confirm the
   six VNUMs with no current object prototype, the overlapping source copies
   of object 54, and whether Charing's Heavens, Verz1, Kobold, Valley Crushk,
   Plane of Fire One, and Orrak imports are intentional.
5. **Should the Great Tree and river prose match configured route state?**
   Confirm the asymmetric hidden entrance, the two pruned forest targets, the
   unexplained portal description, and the river-current prose against intended
   movement behavior. Do not promise a route or hazard from prose alone.
6. **Is the present tone intended?** Review the execution and captive-prison
   text, the necromancy lab, and the abandoned children with the area owner.
7. **Should the literal D1 in the Grovesinger-room door description be
   removed?** It is included in the WLD exit description and is not another
   direction slot.

**Safe working resolution:** retain Charing as an active elf hometown and
preserve its rooms, resets, shops, and imported numeric references. Record its
current Surface edge, the two pruned legacy exits, empty sidecar/QST boundary,
isolated loading rooms, and unresolved door/source questions. Make no gameplay
change and do not move any source while builder intent and typed references
remain open.

## Compiled quest and staging follow-up

The compiled-source follow-up found a `world_quest` binding on the ranger
bartender mobile #45055 ([assignment](../../../src/specs/specs.assign.c#L766),
[prototype](../../../areas/mob/charing.mob#L783)). This is the shared
world-quest handler, not a local QST contract or populated sidecar story. The
handler can offer the shared quest interaction; the current offer pool and
live availability were not inspected. Keep “no local native contract” distinct
from “no quest-related runtime behavior.”

Room #45037's pet-shop staging role is also confirmed in code: the
`pet_shops` procedure on room #45036 reads hirelings from the next real room
(`ch->in_room + 1`), which is #45037 in the current Charing WLD order
([binding](../../../src/specs/specs.assign.c#L2335),
[procedure](../../../src/specs/specs.room.c#L863)). The player remains in the
shop room during listing and purchase; #45037 is not a player WLD passage.
Room #45151 remains isolated, has reset-loaded mounts, and has no direct
compiled room-VNUM reference in the bounded scan. Its intended role is still a
builder question.
