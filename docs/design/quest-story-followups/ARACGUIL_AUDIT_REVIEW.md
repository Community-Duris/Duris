# Arachdrathos Guilds repository-wide audit review

This supplement completes the source-backed story, graph, typed-reference,
title, and bounded-history review for active area `aracguil`. It accompanies
the [Arachdrathos Guilds story dossier](../zone-stories/ARACHDRATHOS_GUILDS.md).
The findings describe repository source; they do not qualify current runtime
availability or live traversal. No area or native gameplay source was changed.

## Active source and story inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 367 is
titled `Arachdrathos Guilds` and contains 78 rooms (#36700–#36777), 52 mobiles
(#36700–#36751), and 67 objects (#36700–#36767, with no object #36761). Active
WLD, MOB, OBJ, and ZON files are present. The stem is listed in `areas/qst/AREA`,
but `areas/qst/aracguil.qst` is absent; there are zero native Q/QA contracts,
no SHP file, and no story sidecar. The ZON header sets reset mode 0. Its reset
records comprise 167 M, 108 E, 30 G, 19 O, 9 P, 8 F, and 44 D commands.

The rooms and mobile descriptions portray six Drow houses with competing
ambitions. Zarbonesti's slavers sell contracts for vendettas and its clerics
transform Drow males into Zarbon; Ilith seeks to rule after Zarbonesti; Slin's
assassins raid surrounding communities and slaughter captured children; and
Sa'Zarn's necromancers use failed experiments and preserve their history in
Zargenenon's throne room mural. The mural links the Jewel held in Zarbonesti's
lost shrine with ten switches reset in the five other houses. The [story
dossier](../zone-stories/ARACHDRATHOS_GUILDS.md) reviews those authored threads.

## Active WLD graph and city boundaries

All 150/150 authored exits resolve: 143 are local and seven lead to Arac-Web.
The active local graph has six weak components of 25, 12, 11, 10, 10, and 10
rooms. The largest component contains Zarbonesti and the five-room tunnel
chain to the lost shrine; the other components correspond to the other house
interiors, with the Zochra alcove and Sa'Zarn dungeon rooms included by local
links. From #36700, raw directed traversal over all authored exits reaches and
returns from 25/78 rooms. When the ten WLD exits flagged `EX_BLOCKED` are
removed, root reach falls to 19/78 while return paths still cover 25/78. This
is a static WLD calculation: it does not simulate reset states, keys, object
placement, or player commands.

Each boundary is reciprocal with Arac-Web:

| Guild entrance | Reciprocal Arac-Web entrance | Exit-register rows |
| --- | --- | --- |
| #36700 south ↔ #36427 north | House Zarbonesti | [482](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L482), [529](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L529) |
| #36710 south ↔ #36431 north | House Zarbonesti | [483](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L483), [530](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L530) |
| #36720 south ↔ #36444 north | House Olipth | [484](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L484), [531](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L531) |
| #36730 south ↔ #36447 north | House Zochra | [485](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L485), [532](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L532) |
| #36740 south ↔ #36506 north | House Ilith | [488](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L488), [533](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L533) |
| #36750 south ↔ #36453 north | House Slin | [486](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L486), [534](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L534) |
| #36760 south ↔ #36482 north | House Sa'Zarn | [487](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L487), [535](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L535) |

The [Arac-Web → Arachdrathos Guilds](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L17)
and [reverse](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L23) summaries each contain
seven matching exits. All 95 WLD files outside the active `areas/AREA` list
parsed without errors: 527,791 rooms and 2,080,773 exits, with no room-VNUM
overlap or source-only approach into `aracguil`. Curated source-only totals
remain 3,439 overall, 205 for Surf, and 15 for `ud_map2`; no candidate row is
added.

## The ten reset switches

The ten reset `ITEM_SWITCH` objects use `CMD_PUSH` and target the ten local
exits whose WLD and ZON records set `EX_BLOCKED`. The compiled generic handler
removes that bit and, for nonsecret passages, also removes it from the reverse
exit. A switch placed on the floor can target another room; the handler reports
a distant rumble and still opens the configured exit. All ten switch values
select the wall/floor/ceiling movement message. These source rules explain how
the authored network can join the separated houses, but they do not establish
live puzzle behavior.

| Switch | Reset room | Target exit | Initial blocked reset |
| --- | --- | --- | --- |
| [#36721](../../../areas/obj/aracguil.obj#L313) | #36729, Olipth roof | #36737 east → #36738 | [ZON 198](../../../areas/zon/aracguil.zon#L198); [door 72](../../../areas/zon/aracguil.zon#L72) |
| [#36722](../../../areas/obj/aracguil.obj#L321) | #36738, Celestial Soldier room | #36747 up → #36748 | [ZON 205](../../../areas/zon/aracguil.zon#L205); [door 88](../../../areas/zon/aracguil.zon#L88) |
| [#36723](../../../areas/obj/aracguil.obj#L329) | #36748, Window on the Demonweb | #36754 up → #36758 | [ZON 207](../../../areas/zon/aracguil.zon#L207); [door 104](../../../areas/zon/aracguil.zon#L104) |
| [#36724](../../../areas/obj/aracguil.obj#L337) | #36758, Slin forgotten room | #36768 up → #36769 | [ZON 209](../../../areas/zon/aracguil.zon#L209); [door 136](../../../areas/zon/aracguil.zon#L136) |
| [#36725](../../../areas/obj/aracguil.obj#L345) | #36769, Sa'Zarn crypt | #36719 east → #36770 | [ZON 211](../../../areas/zon/aracguil.zon#L211); [door 24](../../../areas/zon/aracguil.zon#L24) |
| [#36732](../../../areas/obj/aracguil.obj#L432) | #36725, Olipth council chambers | #36770 east → #36771 | [ZON 197](../../../areas/zon/aracguil.zon#L197); [door 144](../../../areas/zon/aracguil.zon#L144) |
| [#36733](../../../areas/obj/aracguil.obj#L440) | #36736, Zochra landing | #36771 east → #36772 | [ZON 204](../../../areas/zon/aracguil.zon#L204); [door 152](../../../areas/zon/aracguil.zon#L152) |
| [#36734](../../../areas/obj/aracguil.obj#L448) | #36745, Ilith apartment | #36772 east → #36773 | [ZON 206](../../../areas/zon/aracguil.zon#L206); [door 160](../../../areas/zon/aracguil.zon#L160) |
| [#36735](../../../areas/obj/aracguil.obj#L456) | #36757, Slin observation deck | #36773 east → #36774 | [ZON 208](../../../areas/zon/aracguil.zon#L208); [door 168](../../../areas/zon/aracguil.zon#L168) |
| [#36736](../../../areas/obj/aracguil.obj#L464) | #36768, Sa'Zarn throne | #36774 east → #36709, lost shrine | [ZON 210](../../../areas/zon/aracguil.zon#L210); [door 176](../../../areas/zon/aracguil.zon#L176) |

The generic [item_switch handler](../../../src/specs/specs.object.c#L309)
returns “Nothing happens” when the target exit is not flagged `EX_BLOCKED`.
Object #36740, described as a lever at the adamantine gate, is an outlier: it
uses `CMD_PULL`, targets #36730 north, and that WLD exit has no blocked flag or
door-reset row. The object has no local ZON reset placement, and no external
reset-role reference was found for it. Its source definition therefore does
not establish a working gate lever; confirm the intended placement and target
before relying on it.

## Typed reset and compiled dependencies

Repository-wide exact scans found these inbound and outbound typed links:

| Source | Reference | Classification |
| --- | --- | --- |
| `aracguil.zon` lines 237 and 240 | One G reset gives external object #359 to mobile #36711, the Mentenozan shade in room #36709. Object #359 is the rune-covered mana stone in `heavens.obj`. | Active reset import from Heavens; preserve the object definition and this placement. See [shade/item reset chain](../../../areas/zon/aracguil.zon#L237), [shade](../../../areas/mob/aracguil.mob#L168), and [Heavens object](../../../areas/obj/heavens.obj#L3725). |
| `epic_skills.c` | The epic-teacher table maps mobile #36720 to `SKILL_SCRIBE_MASTERY`; this mobile is the house scribe reset in Olipth's archives. | Direct compiled mobile reference and callback binding; preserve the prototype and intended archive context. See [table](../../../src/classes/epic_skills.c#L176), [epic handler binding](../../../src/world/epic.c#L310), [mobile](../../../areas/mob/aracguil.mob#L296), and [reset](../../../areas/zon/aracguil.zon#L318). |
| `specs.assign.c` | Object #36744 is bound to `shimmering_longsword` and equipped on the celestial warrior in room #36738. | Direct compiled object callback. Its function can cast a level-30 lightning bolt on a live melee victim on a 1-in-30 roll. See [binding](../../../src/specs/specs.assign.c#L1710), [function](../../../src/specs/specs.grove.c#L1204), [object](../../../areas/obj/aracguil.obj#L566), [mobile reset](../../../areas/zon/aracguil.zon#L389), and [equipment reset](../../../areas/zon/aracguil.zon#L391). |
| `specs.assign.c` / `specs.aracguil.c` | Object #36760 is bound to `rod_of_zarbon` and reset on the shade with the Jewel name. | Direct compiled object callback. When wielded by a Drow female cleric, it has a 1% branch that tries to create curse object #36761; no object prototype #36761 exists in the scanned object sources. A separate nominal 3% branch can deal negative damage and siphon hit points and vitality. See [binding](../../../src/specs/specs.assign.c#L1711), [callback](../../../src/specs/specs.aracguil.c#L16), [Jewel prototype](../../../areas/obj/aracguil.obj#L792), and [reset](../../../areas/zon/aracguil.zon#L241). |

No external QST item command, SHP product, typed story `item_vnums` field, or
other external reset-role reference targets local prototypes; the single
external reset input is Heavens object #359 above. The area has no local SHP
file or story sidecar. The bounded direct C/C++ VNUM scan found only the
teacher/callback references above and the #36761 object lookup. No external
mobile or object prototype duplicates a local VNUM, and none of 871 scanned
external type-25 portal declarations targets an Arachdrathos Guilds room. All
231 external story sidecars parsed without errors or local VNUM hits. The
all-object scan parsed 21,682 prototypes; its single parse exception was the
expected truncated `end.obj` #9999999 terminator. An ITEM_SWITCH scan found
ten placed switches and the unplaced #36740 lever described above.

## Title and bounded history

After color-code normalization, `Arachdrathos Guilds` is unique among 442
scanned current `.zon` titles. Related titles—`Arachdrathos - Drow City`
(`arac-web`), `Arachdrathos Wilderness` (`arac_wild`), and `Llzazan Ghetto of
Arachdrathos`—name neighboring contributors rather than title duplicates.

Followed WLD/MOB/OBJ/ZON history stays on the `aracguil` stem through the
2009 move into the `areas/` tree and subsequent same-stem edits. The September
2009 history toggles whether the Drow guilds pop; the 2013 commit changes MOB
and OBJ records but leaves WLD/ZON content unchanged; and the 2015 pass updates
quest-item flags. In 2025, the ZON header changes to reset mode 0, matching the
commit note that the Drow guilds should never reset. These are same-stem source
and reset-policy changes. The bounded title and history review found no
alternate-stem rename, distinct predecessor, or whole-zone replacement.

## Migration disposition and follow-up

Keep `aracguil` **DO NOT MOVE** as a current un-commented `areas/AREA`
contributor. Preserve its seven reciprocal active Arac-Web boundaries, ten
reset switch/blocked-exit pairings and 44 door resets, the Heavens #359 reset
import and local Jewel #36760, the Scribe Mastery teacher #36720, and compiled
callbacks for objects #36744/#36760. Resolve the unplaced lever #36740 and
missing object #36761 only in a separately scoped owner review. No inactive
WLD approach or new attic candidate row applies to `aracguil`; keep `surf`
**RETAIN IN PLACE** with existing candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
for its previously mapped approaches. See coverage row
[11](ZONE_COVERAGE_REGISTER.csv#L11), the [world connectivity review](WORLD_CONNECTIVITY.md),
the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md), and
the [story dossier](../zone-stories/ARACHDRATHOS_GUILDS.md).
