# Githyanki Fortress: source story review

**Scope:** static review of the Fortress QST messages, room and mobile
narrative, objects and reset placements, authored exits, tracked file history,
and typed source references. No dialogue, rare-load behavior, reset, shop,
route, or runtime access was tested.

## Source identity and dialogue coverage

`githyanki_fortress` is an active full-area contributor, zone `1320`, with
rooms `#132000`–`#132068`, 20 mobile prototypes, 25 object prototypes, and two
QST giver prototypes: the imprisoned fire giant `#132013` and frost giant
`#132014`. The local QST file contains eight keyed `M`/`MA` response records,
four per giant, but no `Q`/`QA` completion contract. The active coverage row
records no local shop file, authored story sidecar, or older mapped story
dossier ([area entry](../../../areas/AREA#L380),
[zone header](../../../areas/zon/githyanki_fortress.zon#L1),
[QST file](../../../areas/qst/githyanki_fortress.qst#L1),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L106)).

The local QST records are keyed messages, not native quest steps. The loader
reads `M` and `MA` as keyed messages and stores the `A` suffix as the
echo-to-room setting; the generic ask/tell path matches the supplied topic
against the message key ([message loader](../../../src/world/quest.c#L2163),
[key matching](../../../src/world/quest.c#L1985)). This describes source
handling only; it does not qualify the giants' availability or ordinary
access.

| Giver | Keyed responses | Source story |
| --- | --- | --- |
| Fire giant `#132013` | `hi hello`, `battalion`, `theft loki invasion`, `sword` | He says he was the last survivor of the Smoking Beard battalion, sent toward Jotunheim to attack frost giants. He blames Loki for stealing Surtr's crown of Muspelheim and sword, and calls the sword “Twilight,” a weapon tied to Ragnarok. ([records](../../../areas/qst/githyanki_fortress.qst#L2)) |
| Frost giant `#132014` | `hi hello help`, `despair`, `loki`, `sword` | He says Jotunheim may fall while Thrym is distracted and frost giants defect to Kostchtchie. Loki learned of the death prophecy that starts Ragnarok, then stole and hid Surtr's sword. Surtr has sent Glammad Firebeard and an army to raze Jotunheim and recover the sword and crown. ([records](../../../areas/qst/githyanki_fortress.qst#L39)) |

The source spells the destination “Jotenheim” in one fire-giant response and
“Jotunheim” elsewhere. The active `jotun` area is titled Jotunheim, but the
spelling difference and narrative reference do not create a WLD route
([Jotunheim title](../../../areas/zon/jotun.zon#L1),
[active-area entry](../../../areas/AREA#L344)).

## Environmental story

- **A guarded stronghold on a floating Astral landmass.** The approach
  describes a mile-long mass of black rock suspended in the silvery void. Its
  gates, high walls, towers, patrols, and military training spaces establish
  the Fortress as a defended base, not an open settlement
  ([approach](../../../areas/wld/githyanki_fortress.wld#L1),
  [outer wall](../../../areas/wld/githyanki_fortress.wld#L327),
  [silvery gates](../../../areas/wld/githyanki_fortress.wld#L300)).
- **A history centered on Gith and the illithids.** The large sacrificial hall
  displays Vlaakith's statue, a blood-stained altar, and tapestries depicting
  the Githyanki's enslavement by the illithids, Gith's uprising, and the
  division between Githyanki and Githzerai. The local objects also include
  illithid corpses and remains, a flayed tentacle, and a desk made of magically
  fused illithid bones holding a star crystal
  ([hall](../../../areas/wld/githyanki_fortress.wld#L920),
  [statue and altar](../../../areas/zon/githyanki_fortress.zon#L113),
  [illithid corpses](../../../areas/obj/githyanki_fortress.obj#L1),
  [bone desk](../../../areas/obj/githyanki_fortress.obj#L177),
  [nested star crystal](../../../areas/obj/githyanki_fortress.obj#L205),
  [desk reset](../../../areas/zon/githyanki_fortress.zon#L115)).
- **Prison, coercion, and the two giant captives.** A warded, locked route
  leads from the training grounds into a guarded row of cells. The hallway
  description mentions dried blood; the reset places a frost giant and fire
  giant in separate cells. Their mobile descriptions depict burns, flaying,
  and shackles, while the room and object sources place a bloody slab,
  flayed illithid remains, a severed tentacle, and burning or icy shackles in
  the cell area ([prison approach](../../../areas/wld/githyanki_fortress.wld#L545),
  [cell row](../../../areas/wld/githyanki_fortress.wld#L561),
  [giants' descriptions](../../../areas/mob/githyanki_fortress.mob#L212),
  [frost giant placement](../../../areas/zon/githyanki_fortress.zon#L150),
  [fire giant placement](../../../areas/zon/githyanki_fortress.zon#L151),
  [cell objects](../../../areas/zon/githyanki_fortress.zon#L108)).
- **Military and magical leadership.** The sources place a Chief Knight,
  blacksmith, advisor, arch-mage, grand warlock, worshippers, Supreme Leader,
  and an emissary of Tiamat in the fortress. The warlock and arch-mage share a
  richly furnished study with maps, magical tomes, and a bone desk containing
  the star crystal. The Supreme Commander's chambers contain meeting maps and
  a mercury-colored pool; its authored resets also place two foreign objects
  there (see dependencies below)
  ([warlock study](../../../areas/wld/githyanki_fortress.wld#L1041),
  [audience hall](../../../areas/wld/githyanki_fortress.wld#L1258),
  [inner study](../../../areas/wld/githyanki_fortress.wld#L1277),
  [leadership and emissary resets](../../../areas/zon/githyanki_fortress.zon#L164)).
- **Labeled rare-load rooms.** Rooms `#132066`, `#132067`, and `#132068` are
  titled “Rare Load Trap Room” and “Rare Load Starting Room” variants. Zone
  commands place Malachite in `#132067`, and Ujani plus the astral projection
  of Alenien in `#132068`. Each starting room has one authored exit toward a
  prison cell and its other exits point to the room named as the trap. These
  labels and placements do not establish spawn frequency, a player path, or
  how the trap behaves ([room records](../../../areas/wld/githyanki_fortress.wld#L1295),
  [Malachite placement](../../../areas/zon/githyanki_fortress.zon#L183),
  [Ujani placement](../../../areas/zon/githyanki_fortress.zon#L185),
  [Alenien placement](../../../areas/zon/githyanki_fortress.zon#L187),
  [Ujani description](../../../areas/mob/githyanki_fortress.mob#L279),
  [Malachite description](../../../areas/mob/githyanki_fortress.mob#L301)).

Taken together, the source presents an Astral military fortress whose
religious and political identity is tied to Gith's rebellion against the
illithids. Its prison and the giants' dialogue add a separate conflict among
Surtr, Loki, Thrym, and the powers contesting Jotunheim. The QST text supplies
that connection, but the Fortress has no direct WLD boundary to the active
Jotunheim area.

## Static connectivity, resets, and doors

The static census records 159 authored exits, all 159 loader-resolving, and
one local weak component containing all 69 rooms. There is one inbound and one
outbound active boundary, and the two WLD records are reverse-paired: Fortress
room `#132000` exits down to Astral Plane room `#19798`, whose reverse record
returns to `#132000` ([Fortress exit](../../../areas/wld/githyanki_fortress.wld#L18),
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L155),
[reverse record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L321)). No room has a
numeric fall-chance or current field ([coverage row](ZONE_COVERAGE_REGISTER.csv#L106)).

The reset comments mark the outer gate closed and locked, and several
interior passages closed, locked, or secret. Those are authored reset states;
the exit census does not prove a player can discover or traverse them. A
separate source-only screen found 276 exits from unregistered `Duris3.wld`
pointing into the Fortress. They form four uniform 69-room bands. Most source
rooms are titled “The Ocean”; 272 of 276 have that title. Four are titled
“Mountains”: room `#132026` on `D1` and `D3`, and rooms `#131725` and
`#131726` on `D2`
([east](../../../areas/wld/Duris3.wld#L672558),
[west](../../../areas/wld/Duris3.wld#L672566),
[south](../../../areas/wld/Duris3.wld#L666241),
[south](../../../areas/wld/Duris3.wld#L666262)). Since `Duris3` is outside
`areas/AREA`, these records are not active-world entrances; exact bands and
source lines are in
[World connectivity](WORLD_CONNECTIVITY.md) and the
[unregistered-component row](ZONE_COVERAGE_REGISTER.csv#L387).

Fortress `.zon` still contains reset commands, but the tracked history records
that on 2010-10-02 the zone's header field was changed from `1` to `0` in a
commit titled “Githyanki Fortress will no longer repop.” Treat the command
list as authored placement data, not evidence that the occupants currently
repopulate ([current header and resets](../../../areas/zon/githyanki_fortress.zon#L1);
history: commit `d76ab704b`).

## Typed dependencies and code references

- The inner study reset imports object `#64` and object `#359`, neither of
  which is defined in the Fortress object file. The active Heavens object
  source defines `#64` as a glowing pool and `#359` as a rune-covered stone.
  These are source-level foreign placements; the Fortress reset comments
  themselves identify both as absent locally
  ([Fortress reset for `#64`](../../../areas/zon/githyanki_fortress.zon#L118),
  [reset for `#359`](../../../areas/zon/githyanki_fortress.zon#L119),
  [Heavens pool](../../../areas/obj/heavens.obj#L904),
  [Heavens stone](../../../areas/obj/heavens.obj#L3725)).
- `src/account/chaos_eq_data.h` names Fortress objects `#132024` (astral
  threads), `#132015` (severed illithid tentacle), and `#132022` (the holy
  warhammer “Discipline”) in equipment data. This is a direct code-side
  prototype dependency, not evidence of who receives or wears the objects
  ([equipment references](../../../src/account/chaos_eq_data.h#L167),
  [tentacle reference](../../../src/account/chaos_eq_data.h#L345),
  [warhammer reference](../../../src/account/chaos_eq_data.h#L368),
  [local tentacle](../../../areas/obj/githyanki_fortress.obj#L149),
  [local warhammer](../../../areas/obj/githyanki_fortress.obj#L213),
  [local threads](../../../areas/obj/githyanki_fortress.obj#L249)).
- A current-source scan of `.zon` and `.shp` files found no external reset or
  shop consumer of Fortress mobile/object prototypes. The code scan found no
  other direct numeric room or mobile reference for this area. There is no
  local SHP file; runtime services, procedures, and editor dependencies remain
  outside this static review.

## Checkpoint 808 current typed dependency census

The Fortress `.zon` contains 83 typed prototype operands: 57 mobile operands,
all resolving to its 20 local mobiles, and 26 object operands. Twenty-four
object operands resolve to 24 local prototypes; the other two are active
Heavens objects #64 and #359
([reset imports](../../../areas/zon/githyanki_fortress.zon#L118),
[Heavens #64](../../../areas/obj/heavens.obj#L904),
[Heavens #359](../../../areas/obj/heavens.obj#L3725)). All 20 local mobiles
and 24 of 25 local objects are reset-used; object #132010 is not. The zone
header remains the authored no-repopulation setting, so this is placement
data, not evidence of current population. Object #64 is also present in
unregistered `hostel` and `mavulsk` object sources; its active full-area
definition is Heavens, while those source copies remain outside the active
area list ([Hostel](../../../areas/obj/hostel.obj#L839),
[Mavulsk](../../../areas/obj/mavulsk.obj#L1047),
[active Heavens membership](../../../areas/AREA#L2)).

The two local QST givers have eight keyed `M`/`MA` messages and no `Q`/`QA`
completion contract or item field. No outside active or unregistered ZON,
QST giver/item, SHP keeper/product, or typed story-sidecar consumer of local
Fortress prototypes was found. There is no local SHP or authored story
sidecar. The local objects contain no type-25/type-29 item, no external
type-25/type-29 target enters a Fortress room, and no external WLD door key
field targets a Fortress object.

Ten local WLD door-key fields form five reciprocal pairs, using local keys
#132001 on rooms #132025/#132026, #132002 on #132017/#132038, #132003 on
#132013/#132016, #132008 on #132017/#132057, and #132009 on
#132064/#132065 ([keyed exit records](../../../areas/wld/githyanki_fortress.wld#L310)).
The maintained equipment table has six numeric entries across three local
object prototypes: #132024, #132015, and #132022. An exact source scan found
no other direct C++ room or mobile reference in the Fortress VNUM range
([equipment table](../../../src/account/chaos_eq_data.h#L167)).

The static world census remains 159/159 resolved exits, with the same one
reciprocal Astral boundary. The 276 `Duris3.wld` exits into this area remain
unregistered source-only references, not active entrances. No area source,
registration, reset, or WLD graph change is proposed. Runtime, editor, and
persisted behavior remain unqualified.

## Duplicate and replacement history

The normalized active-title scan finds no exact repeated `.zon` title for
this area. The history does contain an explicit but incompletely resolved
replacement statement. Commit `9004f09cb` (2010-07-31) added the Fortress
WLD/MOB/OBJ/QST/ZON files and said they were “the replacement for old gith
fort”; its message also says the area had not yet been added to `areas/AREA`.
Commit `cc7d194ae` (2010-08-04) replaced a commented `*lortower *1320` list
entry with active `githyanki_fortress *1320`. Commit `93e77d517` (2010-09-08)
is titled “Readding gith fort” and adds it to the list again.

The pre-addition tree already contained active `gith_ht` (“Githyanki
Hometown”), whose own rooms describe streets through a Githyanki fortress and
whose Astral boundary is room `#19794`. The current list continues to include
`gith_ht` at zone `194` and this Fortress at `1320`; they have different
titles and different Astral boundary rooms (`#19794` and `#19798`). The
tracked pre-addition source tree and path history do not identify a distinct
predecessor file or map the phrase “old gith fort” to `gith_ht`. Therefore the
author's replacement claim is recorded, but its predecessor identity remains
unresolved; it is not evidence that the current Hometown is superseded
([Hometown review](GITH_HT_STORY_REVIEW.md),
[Hometown entry](../../../areas/AREA#L97),
[Fortress entry](../../../areas/AREA#L380),
[Fortress boundary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L155),
[Hometown boundary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L154)).

## Direct area-owner questions and conservative recommendations

### Builder questions

| Question | Source evidence | Conservative recommendation |
| --- | --- | --- |
| **GITHFORT-Q01.** Which source or VNUM range does the 2010 statement "replacement for old gith fort" identify, and does it refer to a predecessor distinct from current Hometown? | The tracked claim names no source range; the pre-addition tree already contained active Hometown and both current areas remain active with different Astral exits. See [replacement history](#duplicate-and-replacement-history) and [Hometown review](GITH_HT_STORY_REVIEW.md). | Keep both current contributors and all source rows in place; do not infer that Hometown is superseded or move/merge either area until the predecessor is identified. |
| **GITHFORT-Q02.** Is the zone's no-repopulation header still intentional alongside its authored reset roster? | The header's current value is `0`, while the tracked 2010 change says the Fortress will no longer repop. See [reset and door review](#static-connectivity-resets-and-doors), [zone header](../../../areas/zon/githyanki_fortress.zon#L3), and [reset roster](../../../areas/zon/githyanki_fortress.zon#L150). | Treat reset rows as placement data, not proof of current population; preserve the no-repopulation setting unless its owner separately decides otherwise. |
| **GITHFORT-Q03.** Are the Surtr, Loki, and Jotunheim captive accounts intended to remain dialogue-only context, and should "Jotenheim" or the sword name "Twilight" receive a separate text review? | Fire- and frost-giant messages establish the story, spelling variation, and sword name; no local Q/QA contract is present. See [source identity and dialogue](#source-identity-and-dialogue-coverage), [fire-giant records](../../../areas/qst/githyanki_fortress.qst#L2), and [frost-giant records](../../../areas/qst/githyanki_fortress.qst#L39). | Preserve the current messages as keyed dialogue; do not infer a local quest, route to Jotunheim, or item behavior from narrative wording. Make any spelling/name change only as a separate content decision. |
| **GITHFORT-Q04.** Are the Rare Load Starting Room placements and their exits toward the named trap room and prison cells intentional? | The source names three rare-load rooms and resets Malachite, Ujani, and Alenien among them; the WLD records do not establish load frequency or safe player travel. See [environmental story](#environmental-story), [room records](../../../areas/wld/githyanki_fortress.wld#L1295), and [resets](../../../areas/zon/githyanki_fortress.zon#L183). | Preserve current topology and placements until load conditions and access are separately established; do not advertise these as ordinary entrances or infer trap behavior from room labels. |
| **GITHFORT-Q05.** Are Heavens objects #64/#359 intentional imports, and should the three code-side equipment references to Fortress objects #132024, #132015, and #132022 remain bound to those exact prototypes? | The reset file imports Heavens objects; maintained equipment tables refer to three local Fortress objects. See [typed dependencies](#typed-dependencies-and-code-references), [imports](../../../areas/zon/githyanki_fortress.zon#L118), [Heavens definitions](../../../areas/obj/heavens.obj#L904), [equipment references](../../../src/account/chaos_eq_data.h#L167), and [additional equipment references](../../../src/account/chaos_eq_data.h#L345). | Preserve exact active Heavens ownership and local prototype IDs until their respective owners decide otherwise; do not archive or reassign by number alone. |
| **GITHFORT-Q06.** Should the 276 `Duris3.wld` exits into this area remain historical source references rather than active ingress? | All 276 references are in unregistered `Duris3.wld`; they do not create active-world entrances. See [static boundary review](#static-connectivity-resets-and-doors), [source exits](../../../areas/wld/Duris3.wld#L672558), and [unregistered component record](ZONE_COVERAGE_REGISTER.csv#L387). | Retain the bounded source finding and current active boundaries; do not add these records to active entrance totals or advertise access without registration and route-owner review. |


Keep this active area, its reset rows, imported objects, and code-referenced
items in place. Record the old-fort predecessor as unresolved and keep
`gith_ht` distinct. This review does not propose a gameplay, topology, or
archive change.
