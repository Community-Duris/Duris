# Forest of Mir: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Forest of Mir
source dossier](../zone-stories/FOREST_OF_MIR.md). It reconciles the active
WLD graph, inactive-map references, typed QST/story/reset dependencies,
compiled prototype references, title ownership, and bounded same-stem
history. Static source review does not establish runtime access or gameplay
outcomes.

## Active WLD boundaries and directed reach

Active Mir has 281 rooms from #41900 through #42180 and 558/558 resolving
exits: 556 local and two across area boundaries. Its local graph has weak
components of 271 and 10 rooms. Raw directed traversal from #41900 reaches
237/281 rooms; 271/281 rooms have a path back to #41900. The 44 rooms outside
root-forward reach are #41943–#41944, #41947–#41949, #41963–#41964, #41967,
#41977–#41979, #41982–#41983, #41993–#41999, #42013–#42019, #42033–#42038,
#42052, and #42166–#42175. Rooms #42166–#42175 also have no authored return
path to #41900. These figures describe static WLD edges; they do not resolve
doors, resets, scripts, or runtime access.

Both cross-area boundaries are reciprocal with Highway:

| Mir exit | Highway reverse | Pair summary |
| --- | --- | --- |
| #42137 south to #41316 | #41316 north to #42137 | [Rows 194 and 276](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L194) |
| #42152 east to #41585 | #41585 west to #42152 | [Rows 194 and 276](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L276) |

The exact active records are also recorded in the [connectivity register](WORLD_CONNECTIVITY.md#priority-203--checkpoint-163--forest-of-mir).

## Inactive WLD sweep

All 95 WLD files outside the active areas/AREA list parsed cleanly: 527,791
rooms and 2,080,773 exits. None of their rooms overlaps Mir, and none of their
exits targets a Mir room. No source-only approach is added. Curated totals
remain 3,346 overall, 169 for the reviewed Surf subset, and 15 for ud_map2.

## Exact typed and structured dependencies

The manifest-active QST and story scans identify four external item
references into Mir objects:

| Source | Typed source record | Mir target |
| --- | --- | --- |
| Active Bastine QST | G I #41924 at [line 68](../../../areas/qst/bastine.qst#L68), #41922 at [line 100](../../../areas/qst/bastine.qst#L100), and #41920 at [line 108](../../../areas/qst/bastine.qst#L108) | Ebony staff #41924, face of Mythra #41922, and Xort beast hide #41920 |
| Bastine story sidecar | Structured item_vnums #41924, #41922, and #41920 at [lines 169](../../../areas/story/bastine.story.json#L169), [277](../../../areas/story/bastine.story.json#L277), and [313](../../../areas/story/bastine.story.json#L313) | Same three local Mir objects |
| Active Surface Mini QST | G I #41916 at [line 448](../../../areas/qst/surfacemini.qst#L448) | Woundhealer scimitar #41916 |
| Surface Mini story sidecar | Structured item_vnums #41916 at [line 1640](../../../areas/story/surfacemini.story.json#L1640) | Same local Mir object |

The QST files are listed in [qst/AREA](../../../areas/qst/AREA). Sidecar token matches outside the typed
item fields are IDs or completion strings that echo these numbers; they are
not additional prototype fields. All 231 scanned story sidecars parsed without
errors.

The active and unregistered SHP scan found no product entry targeting a Mir
object.

Active Highway resets reference two Mir objects: O reset #41915 in Highway
room #41540 at [highway.zon line 164](../../../areas/zon/highway.zon#L164),
and P reset #41917 inside object #41353 at [line
176](../../../areas/zon/highway.zon#L176). Mir's own active ZON has an O reset
for Heavens object #359 in Mir room #42174 at [mir.zon line
59](../../../areas/zon/mir.zon#L59); active [heavens.obj #359](../../../areas/obj/heavens.obj#L3725)
is the rune-covered stone. These are typed source reset references, not
claims about current runtime stock or player transfer.

Mir's external QST/story references and the active Highway resets resolve to
local object prototypes in [mir.obj #41915](../../../areas/obj/mir.obj#L201),
[mir.obj #41916](../../../areas/obj/mir.obj#L218),
[mir.obj #41917](../../../areas/obj/mir.obj#L234),
[mir.obj #41920](../../../areas/obj/mir.obj#L310),
[mir.obj #41922](../../../areas/obj/mir.obj#L338), and
[mir.obj #41924](../../../areas/obj/mir.obj#L366). Mir has no local SHP source,
and its selected local QST/story item references do not point outside its own
object set.

## Compiled references and external prototype checks

The compiled tables and assignments contain the following typed Mir
references:

| Compiled source | Exact references | Static context |
| --- | --- | --- |
| [specs.assign.c lines 303–310](../../../src/specs/specs.assign.c#L303) | Mobiles #41900, #42166, #42167, and #42172–#42174 | Assigns Mir spider, amphisbean, and wyrm shout procedure functions. |
| [specs.highway.c lines 571–581](../../../src/specs/specs.highway.c#L571) | Objects #41900 and #41901 | Spider behavior checks for and creates web objects. |
| [specs.highway.c lines 653–700](../../../src/specs/specs.highway.c#L653) | Objects #41900, #41901, #41908, and #41907 | Fire procedure checks for web objects, creates smoke #41908, and its callback reads fire #41907. |
| [specs.highway.c wyrm helper procedures](../../../src/specs/specs.highway.c#L710) | Mobiles #42172–#42174 | Helper arrays passed to the shout-and-hunt routines. |
| [specs.assign.c lines 1740–1748](../../../src/specs/specs.assign.c#L1740) | Objects #41907, #41911, #41912, #41913, #41914, #41915, #41916, #41917, and #41918 | Binds the local weapon behaviors and Mir fire behavior to their object prototypes. |
| [chaos_eq_data.h lines 81 and 334](../../../src/account/chaos_eq_data.h#L81) | Object #42169 | Appears in two Chaos equipment table rows; local prototype is the flexible hellfire leggings ([mir.obj](../../../areas/obj/mir.obj#L550)). |

No external duplicate mobile or object prototype was found. Across 870
external type-25 declarations, none targets Mir. The bounded numeric scans
classify matches by room, mobile, object, and string context; raw number
occurrence alone is not treated as a prototype dependency.

## Title and bounded history

The normalized title “Forest of Mir” belongs only to active stem mir among
442 scanned .zon files ([title source](../../../areas/zon/mir.zon#L2)).
Followed mir.wld and mir.zon histories show the 2009 areas/ move and later
same-stem changes, including 2013 edits and 2016 spelling/bug fixes. The
reviewed paths establish no alternate-stem rename, distinct predecessor, or
whole-zone replacement. See the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active mir DO NOT MOVE. Preserve both reciprocal Highway boundaries, the
Bastine and Surface Mini QST/story references, the Highway reset references,
Mir's Heavens object reset, and the compiled prototype bindings. No source-only
candidate row is added. Editor, administrative, installed-data,
persisted-player, indirect-runtime, and actual traversal closure remain
outside this bounded scan; no gameplay, migration, or source change is
proposed.


## Checkpoint 280: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should the fallen priest continue accepting exact scroll #41929 without requiring a prior greeting, source kill, or lair visit? | [Native contract](../../../areas/qst/mir.qst#L10), [scroll reset](../../../areas/zon/mir.zon#L153) | Preserve the exact supplied-item contract; keep arrival and discovery as separate story events. |
| 2 | Should optional source provenance distinguish the male thorny worm's cap-one scroll row from a supplied scroll or the female worm? | [Source and reset records](../../../areas/zon/mir.zon#L153), [story boundary](../zone-stories/FOREST_OF_MIR.md#native-acceptance-and-source-identity) | Track actual source lineage only for an explicitly chosen optional story; do not make it a prerequisite to Q10 acceptance. |
| 3 | How should the accepted priest exchange remain recoverable after the scroll is consumed or token #41930 is lost, traded, or absent after relog? | [QST reward](../../../areas/qst/mir.qst#L10), [journal completion](../../../areas/story/mir.story.json#L221) | Retain an idempotent accepted receipt and exact recipient identity; recovery must not repeat experience or fabricate item custody. |
| 4 | Should the native exchange remain Mir's only potential daily candidate, while reset mode and cap-one sources do not imply renewed supply? | [Zone header and resets](../../../areas/zon/mir.zon#L3), [native acceptance](../../../areas/qst/mir.qst#L10) | Keep daily policy disabled until active READY source generation, reward delivery, persistence, replay, and renewal are demonstrated. |
| 5 | Should the optional loose-scroll readiness view stay transient and never become remembered discovery, first-source history, or an acceptance gate? | [Mir story sidecar](../../../areas/story/mir.story.json), [native source boundary](../zone-stories/FOREST_OF_MIR.md#native-acceptance-and-source-identity) | Keep material visibility separate from personal history and the accepted receipt. |
| 6 | Should Johan's vellum map story require an actual read, an accepted choice to help, and a surviving beneficiary's arrival at a defined destination? | [Johan and map resets](../../../areas/zon/mir.zon#L82), [map object](../../../areas/obj/mir.obj#L265), [story flows](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Keep map custody, examination, consent, travel, and saved beneficiary arrival as distinct events. |
| 7 | Can periodic spider-created webs count as player conservation, or should credit require an admitted player action that changes a persistent world state? | [Spider procedure](../../../src/specs/specs.highway.c#L571), [story boundary](../zone-stories/FOREST_OF_MIR.md#custom-callers-access-and-hazards) | Treat autonomous generation as environmental state; award player credit only for a defined action and verified lasting result. |
| 8 | Which occupied-fire damage, empty-room spread, and cleanup outcomes are intended to count toward a player-controlled forest story? | [Fire procedure](../../../src/specs/specs.highway.c#L653), [fire callback](../../../src/specs/specs.highway.c#L700), [conservation story](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Preserve current harm and spread behavior until builders define a useful intervention and actual recipient/state evidence; diagnose factor ordering and detached-smoke cleanup before any repair. |
| 9 | Should Mythra's face and Xort's hide remain carried trophy objects, separate from any rescue, transformation, or aid outcome? | [Xort mobile](../../../areas/mob/mir.mob#L123), [Mythra mobile](../../../areas/mob/mir.mob#L138), [Xort hide](../../../areas/obj/mir.obj#L310), [Mythra face](../../../areas/obj/mir.obj#L338), [story boundary](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Do not infer rescue or carving from kill, corpse, or loot; define any living-beneficiary outcome separately. |
| 10 | Should orc-warband and werewolf stories require distinct source, faction, and surviving-beneficiary outcomes instead of a generic kill or loot event? | [Warband reset](../../../areas/zon/mir.zon#L100), [local mobile sources](../../../areas/mob/mir.mob), [story flows](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Keep reset followers and carried items as source facts; record a lasting faction or threat change only when its controller and beneficiary are defined. |
| 11 | Should the 34-room forest component stay outside ordinary exit reach pending confirmation of its intended magical route and return path? | [Active graph](MIR_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach), [maze story](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Preserve the current graph while qualifying dynamic access and return; do not add an assumed grid connection. |
| 12 | Are the raw-4 openings at #42051 and #42168 intended to remain masked non-doors, or should a real hidden-door mechanism be authored? | [#42051 exit](../../../areas/wld/mir.wld#L3041), [#42168 room](../../../areas/wld/mir.wld#L5336), [access review](../zone-stories/FOREST_OF_MIR.md#custom-callers-access-and-hazards) | Confirm intended discovery and traversal before changing flags or adding door resets; prose alone does not establish a secret door. |
| 13 | Should both type-25 mercury-pool routes remain part of intended island access, including their current trigger values and return direction? | [First pool object](../../../areas/obj/mir.obj#L381), [second pool object](../../../areas/obj/mir.obj#L394), [pool resets](../../../areas/zon/mir.zon#L48), [world routes](../../../areas/wld/mir.wld#L515) | Qualify actual visibility, ENTER behavior, destination, actor admission, and saved return before treating either declaration as usable travel. |
| 14 | What should count as completed river travel or rescue when thirteen current records can drift or refuse movement based on actor state? | [Current and hazard review](../zone-stories/FOREST_OF_MIR.md#custom-callers-access-and-hazards), [Mir world source](../../../areas/wld/mir.wld) | Record actual actor, direction, landing, survival, and beneficiary result; preserve the current hazards until a balanced outcome is selected. |
| 15 | Should the two HILLS fall records and the HEAL refuge remain separate from SAFE-room status and from a successful rescue endpoint? | [Hazard review](../zone-stories/FOREST_OF_MIR.md#custom-callers-access-and-hazards), [Mir world source](../../../areas/wld/mir.wld) | Qualify descent, exemptions, impact, recovery, refuge restrictions, and return separately; do not infer safety from healing or room prose. |
| 16 | Are the teleport traps with a missing MOVE bit and the pool/item traps with unsupported trigger or damage values intentionally dormant? | [Teleport trap](../../../areas/obj/mir.obj#L121), [pool/item traps](../../../areas/obj/mir.obj#L409), [trap behavior review](../zone-stories/FOREST_OF_MIR.md#custom-callers-access-and-hazards) | Preserve their current behavior until the builder confirms data format, intended trigger, effect, and balanced admission; do not silently activate a hazard. |
| 17 | Should the secret bone key remain NORENT and have its authored 100% break chance after unlocking the casket? | [Key reset and casket](../../../areas/zon/mir.zon#L124), [key prototype](../../../areas/obj/mir.obj#L507) | Preserve intentional consumability and rent restrictions; record acknowledged unlock and arrival separately from key possession. |
| 18 | Should the gold-key trapdoor, wyrm encounter, and FIREPLANE vault remain one explicit access chain with child loot, coin credit, heat survival, and return tracked separately? | [Treasure reset group](../../../areas/zon/mir.zon#L140), [trapdoor and vault](../../../areas/wld/mir.wld#L5427), [key prototype](../../../areas/obj/mir.obj#L715) | Preserve the authored locks and stock probabilities; do not treat key receipt or vault entry as recovered property or a safe return. |
| 19 | Which successful effects should count for the eight relic procedures, while preserving their distinct class, alignment, custody, cooldown, and foreign-stock requirements? | [Compiled bindings](../../../src/specs/specs.assign.c#L1740), [Lightning reset](../../../areas/zon/highway.zon#L164), [Golden Flame reset](../../../areas/zon/highway.zon#L176), [compiled dependency review](MIR_AUDIT_REVIEW.md#compiled-references-and-external-prototype-checks), [story boundary](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Keep Lightning and Golden Flame under their Highway reset ownership and distinguish each actual recipient effect from relic possession or command text. |
| 20 | Should rune-covered stone #359 remain an epic transaction separate from the priest exchange and wider forest stories? | [Mir reset](../../../areas/zon/mir.zon#L59), [stone prototype](../../../areas/obj/heavens.obj#L3725), [epic outcome review](../zone-stories/FOREST_OF_MIR.md#progression-stories) | Require the actual eligible touch, committed participant award, and publication; a visit or pending message is not a new Mir receipt. |
