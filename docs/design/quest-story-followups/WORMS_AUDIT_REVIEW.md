# The Caverns of the Worms: repository-wide source audit supplement

This supplement complements the [Worms dossier](../zone-stories/THE_CAVERNS_OF_THE_WORMS.md).
It reconciles active WLD boundaries, unregistered WLD approaches, exact typed references,
shared code uses, the normalized title, and tracked source history. These findings describe
repository source and static loader paths; they do not establish live access, stock, or player
history.

## Active boundaries and local graph

The active area has 100 rooms and 220 authored exits; every exit target resolves in the active
WLD set. Its local weak-exit graph is one 100-room component. Two active exits enter Worms and
two leave it, forming two reciprocal pairs: Worms room #6900 south to Underdark #811708, and
Worms room #6984 south to Connectorzones #54211 ([Worms to Underdark](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L128),
[Worms to Connectorzones](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L129),
[Connectorzones return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L727),
[Underdark return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1727)). These records describe authored
WLD topology, not door state, safe traversal, or a return journey.

## Unregistered WLD approach and unresolved Shafts target

The all-source scan parsed all 95 unregistered WLD files: 527,791 rooms and 2,080,773 exits,
with zero parse issues and no source-room VNUM overlap with Worms. One source-only exit targets
the area: unregistered `ud_map2.wld` room #1124844 exits east (D1) to active Worms room #6900
([source room](../../../areas/wld/ud_map2.wld#L57606),
[exit](../../../areas/wld/ud_map2.wld#L57614),
[target](../../../areas/wld/ud_map2.wld#L57617)). This raises the curated source-only total
from 599 to 600.

The same unregistered map contains room #1121446. Active Shafts room #10012 has a south exit
to that VNUM, but #1121446 is absent from active WLD sources and remains in the unresolved
exit register ([Shafts room](../../../areas/wld/shafts.wld#L208),
[exit](../../../areas/wld/shafts.wld#L213),
[target](../../../areas/wld/shafts.wld#L216),
[unresolved record](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L19),
[unregistered map room](../../../areas/wld/ud_map2.wld#L10911)). Within `ud_map2.wld`, a
directed 50-exit source path leads from #1121446 to #1124844, then east to Worms #6900. The
map file is not in the active manifest; this source path does not resolve the active Shafts
exit or establish active access to Worms. Door, hazard, and traversal states along the source
path were not evaluated.

## External typed and code references

The active external-source scan found no QST item commands, ZON reset commands, or external
MOB/OBJ prototype definitions that reference Worms VNUMs. None of 872 external type-25 portal
declarations targets a Worms room. All 231 external story sidecars parse, with no exact
structured room, mobile, or item VNUM references. Raw digit matches in unrelated story text
and alphanumeric story IDs were excluded.

Two unregistered `kzkrkeep.qst` item commands numerically name Worms objects: `R I 6900` and
`G I 6912` ([remove](../../../areas/qst/kzkrkeep.qst#L20),
[give](../../../areas/qst/kzkrkeep.qst#L21); corresponding current object records:
[#6900](../../../areas/obj/worms.obj#L1),
[#6912](../../../areas/obj/worms.obj#L145)). `kzkrkeep` is absent from `areas/AREA` and has
only QST and SHP files in this checkout. The quest loader opens `areas/world.qst` and
explicitly does not glob unregistered QST files ([loader](../../../src/world/quest.c#L2086),
[fallback comment](../../../src/world/quest.c#L2092)). These are unregistered source references,
not active Worms quest contracts.

Unregistered `kzkrkeep.shp` has nine product entries whose VNUMs match Worms objects: #6913–
#6920 in one shop and #6924 in another ([first list](../../../areas/shp/kzkrkeep.shp#L3),
[second list](../../../areas/shp/kzkrkeep.shp#L46); sample object definitions
[#6913](../../../areas/obj/worms.obj#L157),
[#6920](../../../areas/obj/worms.obj#L256),
[#6924](../../../areas/obj/worms.obj#L318)). The shop loader resolves positive product VNUMs
through `real_object` ([product load](../../../src/economy/shop.c#L3380)). Its fallback path
globs `areas/shp/*.shp` when `areas/world.shp` is unavailable
([shop stream](../../../src/economy/shop.c#L3267),
[glob fallback](../../../src/economy/shop.c#L3278)); `areas/world.shp` is absent from this
checkout. Thus this orphaned shop file can enter the fallback input set, but its keeper,
runtime stock, and player availability were not qualified. Its other product numbers that only
match Worms room VNUMs are not Worms object references.

The active shared-code use of Worms room #6900 is Ixarkon's `illithid_teleport_veil`. Object
#96402 is reset into room #96524 ([object](../../../areas/obj/ixarkon.obj#L29),
[reset](../../../areas/zon/ixarkon.zon#L156)) and assigned to the handler
([binding](../../../src/specs/specs.assign.c#L1874)). Its exact `ENTER veil` handler selects
among 25 code-defined destinations, including #6900
([destination table](../../../src/specs/specs.ixarkon.c#L18),
[handler](../../../src/specs/specs.ixarkon.c#L15)). This custom travel path belongs to Ixarkon;
it is separate from the two ordinary WLD boundary pairs and the Wilms commissions.

## Title and tracked history

The normalized `.ZON` title appears once among 351 active area titles. The active manifest key
is `worms`, with numeric scope `*69` ([AREA entry](../../../areas/AREA#L38)); the ZON header is
“The Caverns of the Worms” ([title](../../../areas/zon/worms.zon#L2)).
`DUPLICATE_AND_REPLACED_ZONES.md` has no separate Worms predecessor or replacement entry.
Tracked WLD, MOB, OBJ, ZON, and QST paths retain the `worms` stem back to the 2009 area-tree
move, commit `642da5400`; no distinct predecessor path was identified. The story sidecar was
added at its current path in commit `b59909733`.

## Audit boundary

This is a static source review. It does not verify live shop loading, keeper placement, dynamic
resets, safe traversal, accounting-enabled stock, or a played commission journey. No native
repair is proposed in this checkpoint.

## Checkpoint 437: builder question review

Twenty source-backed questions and recommended resolutions now gather the open
commission-selection, exact-material, service, access, and ownership decisions
in the [Worms builder review](../zone-stories/THE_CAVERNS_OF_THE_WORMS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The active source inventory has no local SHP file, so the existing zero-shop
coverage values remain accurate. No area source or runtime behavior changed.
