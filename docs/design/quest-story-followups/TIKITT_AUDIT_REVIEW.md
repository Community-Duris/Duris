# Lost Temple of Tikitzopl repository-wide audit review

Checkpoint 391 completes the source-level graph, typed dependency, title/history
and builder-question review for active tikitt. The existing
[Temple story dossier](../zone-stories/LOST_TEMPLE_OF_TIKITZOPL.md) maps all
29 native exchanges and now contains twenty explicit builder questions with
conservative recommendations. This audit does not qualify a live player
journey, item availability, or safe return.

## Active map and directed reach

The active Temple WLD contains 234 rooms (#44101–#44334) and 616 authored
directions: 615 local and one outbound. All 616 targets resolve. The local
graph is one weak component. When authored exits are treated as open, directed
reach from entrance #44101 covers all 234 rooms; 233/234 can return to that
entrance through ordinary WLD directions. High Altar #44333 is the local WLD
sink, with no ordinary outward exit. There are no negative or unresolved
targets, no exits from out-of-range source rooms, no falls or currents, and no
nonlocal room-VNUM definition collision.

Two active WLD arrivals enter the Temple:

| Source to Temple | Reverse | Source-backed reading |
| --- | --- | --- |
| Lost City #44039 down → #44101 | Temple #44101 up → City #44039 | Reciprocal boundary. |
| Winterhaven #55632 east → #44333 | None | One-way WLD arrival at the High Altar. |

The sole active Temple WLD outbound is #44101 up to City #44039. See active
exit-register [rows 618–619](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L618) and
[Winterhaven row 781](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L781), with pair
summary [rows 660–661 and 820](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L660).

The altar's graph sink has a separate item route: reset array of lights #44190
at #44333 is type 25 and targets Lost City Snake Pit #43704. It uses enter and
has unlimited charges; this does not add an ordinary WLD exit or establish
availability, successful use, or safe onward travel. The
[object declaration](../../../areas/obj/tikitt.obj#L1461),
[Temple reset](../../../areas/zon/tikitt.zon#L200), and
[City destination](../../../areas/wld/tikit.wld#L88) record the three ends.

The bounded reset review includes the flaming, wooden and gem keys, the switch
target, seven speech doors, and their authored lock/secret states. In
particular, the switch at #44290 targets only the down exit; it does not clear
the reverse direction's lock or secret state. Speech unlocks the matching
near/reverse pair, while opening and passage remain separate actions. The
Temple dossier records each route and its exact key or keyword. Static reach
does not model any of those conditions.

All 95 inactive WLD files parsed without errors: 527,791 room records and
2,080,773 exit records. None declares a Temple room VNUM and none points into
a Temple room. These source-only maps add no current arrival and no candidate
record.

## Typed prototype ownership and compiled references

The local QST owns 29 Q contracts and nine addressed M blocks. Its six
equipment recipes consume fifteen Lost City object VNUMs:
#43703, #43705, #43706, #43707, #43710, #43714, #43715, #43717, #43718,
#43719, #43720, #43723, #43739, #43752 and #43753. All fifteen are defined by
active tikit, and the Temple story sidecar carries matching item_vnums
references. Representative exact QST inputs appear in the
[MadMaN slasher recipe](../../../areas/qst/tikitt.qst#L233),
[ring recipe](../../../areas/qst/tikitt.qst#L262) and
[sapphire recipe](../../../areas/qst/tikitt.qst#L274). Keep those prototypes,
their sources and stock with Lost City; their use does not transfer contract
ownership.

The Temple ZON has one additional imported object reset: Heavens object #360,
the rune-covered monolith, is placed at #44333. Its definition remains in
[Heavens OBJ](../../../areas/obj/heavens.obj#L3741). This is separate from
Temple-owned container #44165 and portal object #44190.

There is no local SHP file. Typed scans of all current QST files found no
external item command targeting a Temple object. Typed ZON reset-role scans
found no external reset of Temple rooms, mobiles or objects. The 180 other SHP
files have no exact Temple object product references; external story sidecars
have no typed Temple VNUM references; and no external type-25 object points
into a Temple room. External QST giver headers have no #44101 collision.
Same-type WLD, MOB and OBJ scans found no external duplicate Temple prototype
definitions. These results close source-declared prototype ownership within
those reviewed formats; runtime stock and reset availability were not tested.

Maintained code contains five local object-procedure bindings:

- OBJ #44170 → artifact_hide.
- OBJ #44179 → madman_mangler.
- OBJ #44172 → madman_shield.
- OBJ #44188 → mentality_mace.
- OBJ #44165 → unmulti_altar.

See the [artifact assignment](../../../src/specs/specs.assign.c#L1396),
[MaDMaN assignments](../../../src/specs/specs.assign.c#L1549), and
[altar assignment](../../../src/specs/specs.assign.c#L2591).
CHAOS starter-kit arrays separately name four local objects—#44173
([profile](../../../src/account/chaos_eq_data.h#L252)), #44174
([profile](../../../src/account/chaos_eq_data.h#L72)), #44188
([profile](../../../src/account/chaos_eq_data.h#L91)) and #44194
([profile](../../../src/account/chaos_eq_data.h#L98)). The random-equipment
high-drop table names mobile #44172 (King Arkan'non)
[in its table](../../../src/item/randomeq.c#L121), but its bonus lookup is
commented out ([lookup](../../../src/item/randomeq.c#L650)). VNUM #44172 also identifies
the local shield object in the typed object-procedure binding; do not conflate
the mobile and object identities.

## Title, map identity and followed history

After color-token and terminator removal, whitespace normalization, and case
folding, “Lost Temple of Tikitzopl” is unique among all 442 current ZON titles
([source title](../../../areas/zon/tikitt.zon#L2)).
“Lost City of Tikitzopl” belongs to tikit; “The Lost Temple of the North”
belongs to multiclass_temple. Their reciprocal edge and shared crafting chain
do not imply duplicate ownership or replacement.

Same-type current WLD/MOB/OBJ comparisons find no duplicate Temple VNUM. The
all-95 inactive-WLD scan also finds no Temple room overlap. Followed
tikitt WLD, MOB, OBJ, ZON and QST histories retain their same stem through the
2009 areas-tree move and later edits; the WLD history includes a 2010 same-stem
edit mentioning Tiki portable rooms. The story sidecar first
appears in the 2026-10-02 source-history commit dafd48924 (“Map Tikitzopl temple
journals and recipe integration boundaries”). These bounded paths show no
alternate-stem predecessor, explicit rename or whole-zone replacement.

## Builder questions and retention

The twenty direct design questions and recommended resolutions are in the
[Temple dossier](../zone-stories/LOST_TEMPLE_OF_TIKITZOPL.md#explicit-builder-questions-and-recommended-resolutions).
They cover target aliases, unfinished resistance, the empty crypt, the
cap-one Orb and competing recipes, exact keys and doors, both WLD entrances,
the item return, foreign item ownership, branch outcomes, configured powers,
starter items and text alignment. Questions frame owner decisions; they do not
claim the related outcomes already work.

Keep active tikitt **DO NOT MOVE**. Preserve the reciprocal City boundary, the
one-way Winterhaven arrival, all fifteen City-owned recipe inputs, Heavens
object #360, and Temple object #44190's destination #43704. The all-95 scan
adds no source-only WLD route, so no attic candidate is added or changed. See
[coverage row 304](ZONE_COVERAGE_REGISTER.csv#L304), the
[Lost City audit](TIKIT_AUDIT_REVIEW.md), and the current
[retention plan](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 846: Current item inputs and ownership rechecked

The current typed closure remains the one recorded at Checkpoint 391. Six
Temple QST equipment recipes and the matching sidecar fields consume fifteen
exact Lost City OBJ prototypes. Keep those inputs and each native recipe with
their existing owners; shared prototypes do not make the Temple and City one
area or imply that the Temple supplies the source items.
([recipe inputs](../../../areas/qst/tikitt.qst#L233),
[ring recipe](../../../areas/qst/tikitt.qst#L262),
[sapphire recipe](../../../areas/qst/tikitt.qst#L274),
[Temple sidecar](../../../areas/story/tikitt.story.json),
[Lost City audit](TIKIT_AUDIT_REVIEW.md#checkpoint-845-current-cross-area-typed-closure-refreshed))

The Temple ZON still imports Heavens OBJ #360 at High Altar #44333. Temple OBJ
#44190 remains a type-25 declaration to City room #43704, placed at that altar;
this item route is separate from the reciprocal City WLD boundary and from the
one-way Winterhaven WLD arrival. No external native QST item command, SHP
product, ZON reset, typed sidecar VNUM, same-type prototype, or type-25 target
into the Temple was found. Maintained item procedures and Chaos-kit entries
remain as listed under Checkpoint 391. Numeric MOB/OBJ #44172 retains separate
roles: the mobile high-drop entry's bonus lookup remains commented out, while
the local shield OBJ has its own procedure binding. ([Heavens object](../../../areas/obj/heavens.obj#L3741),
[Temple reset](../../../areas/zon/tikitt.zon#L200),
[type-25 declaration](../../../areas/obj/tikitt.obj#L1461),
[mobile table](../../../src/item/randomeq.c#L121),
[disabled bonus lookup](../../../src/item/randomeq.c#L650))

The two active arrivals, sole outbound, 234-room graph, and all-95 no-approach
result remain unchanged. Keep active `tikitt` **DO NOT MOVE**; this refresh adds
no candidate or migration clearance.
