# Southern Coastal Highway: repository-wide connectivity and dependency audit

**Checkpoint 84; priority 124 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/SOUTHERN_COASTAL_HIGHWAY.md)
with the active WLD graph, inactive WLD sources, exact typed references,
compiled numeric and symbolic matches, normalized title, and bounded source
history. Authored edges and code destinations do not establish ordinary
availability, safe travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Southern Coastal Highway has 539
rooms and 1,411 exits; 1,410 resolve to active rooms. Its internal-exit graph
has two weak components of 533 and 6 rooms. Five active outbound boundaries
and five inbounds make five reciprocal pairs:

| Highway edge | Active return | Cross-area register rows |
| --- | --- | --- |
| #41316 north to Mir #42137 | Mir #42137 south to #41316 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L559), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L564) |
| #41585 west to Mir #42152 | Mir #42152 east to #41585 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L560), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L565) |
| #41618 east to Bastine #7600 | Bastine #7600 west to #41618 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L561), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L175) |
| #41732 east to Surface #531250 | Surface #531250 west to #41732 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L562), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1389) |
| #41837 north to Surface #530849 | Surface #530849 south to #41837 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L563), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1387) |

The remaining Highway exit is #41488 down to #-1. The [unresolved-exit
register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L210) classifies it as a negative
VNUM dropped by `setup_dir` before room renumbering; it is not a positive
unresolved destination or active route. The source dossier records the
climb-down prose discrepancy separately.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. It found
two non-overlapping `surf.wld` source rooms that point into active Highway
rooms:

| Source-only record | Direction and active target | Evidence |
| --- | --- | --- |
| #238784, The Traveller's Path Through the Myrabolan Forest | South to #41837 | [exit](../../../areas/wld/surf.wld#L813740) |
| #239989, The Traveller's Path Through the Myrabolan Forest | West to #41732 | [exit](../../../areas/wld/surf.wld#L839041) |

No inactive source-room VNUM overlaps an active Highway room. These map records
are not active ingress. They raise the curated source-only total from 1088 to
1090 and the reviewed Surf subset from 118 to 120.

## Typed references and compiled numeric matches

External typed scans found these existing cross-zone records:

| Source | Exact local targets | Classification |
| --- | --- | --- |
| Active `bastine.qst` | #41388, #41407, #41327, #41408, #41375, #41304, #41411, #41394, #41398, #41397 | Ten item-command matches in Bastine's six royal returns (including the cursed-hide exchange) and Victor's signet/key/ring chain. Bastine's source dossier owns those outcomes; these are not extra Highway receipts. See [QST records](../../../areas/qst/bastine.qst#L33), [Victor's exchange](../../../areas/qst/bastine.qst#L199), and [Highway dossier](../zone-stories/SOUTHERN_COASTAL_HIGHWAY.md#royal-trophies-and-custom-world-context). |
| Active `bastine.story.json` | #41388, #41407, #41327, #41408, #41375, #41411, #41397 | Seven structured `item_vnums` hits in the same Bastine-owned chain; no other parsed sidecar had an exact structured hit. See examples at [#41388](../../../areas/story/bastine.story.json#L25), [#41397](../../../areas/story/bastine.story.json#L647). |
| Active `alatorin.zon` | Mobile #41362 at room #83361, M reset | One active source for the sturdy horse prototype, already documented in the dossier ([reset](../../../areas/zon/alatorin.zon#L2814)). |
| Active `highway.zon` and Mir prototypes | Object #41915 is O-reset in Highway room #41540; #41917 is P-reset inside chest #41353. Both are defined in Mir. | Active Highway reset imports; preserve the Mir prototypes and their bindings ([Highway resets](../../../areas/zon/highway.zon#L164), [nested reset](../../../areas/zon/highway.zon#L176), [Mir object definitions](MIR_AUDIT_REVIEW.md#exact-typed-and-structured-dependencies)). |

No external SHP product matched. The all-95 scan parsed 231 other story
sidecars without errors; the seven Highway item hits are all in Bastine. No
external mobile or object prototype definition matched. The Highway area has
no local SHP file. The external type-25 scan found no Highway destination among
872 declarations from other object files: 713 active and 159 source-only.

A context-reviewed C/C++ scan found these additional typed references:

| Local target | Static reference | What the source establishes |
| --- | --- | --- |
| Mobile #41327, Bishop | [`epic_skills.c`](../../../src/classes/epic_skills.c#L175) | The epic-teacher table associates this mobile VNUM with `SKILL_DEVOTION`. The table alone does not establish ordinary training availability. |
| Room #41767 | [`innates.c`](../../../src/classes/innates.c#L2010), used at [#2040](../../../src/classes/innates.c#L2040) | `do_shift_prime` selects it as one random Githzerai destination when the race shifts from Astral. This is a computed arrival, not a WLD edge. |
| Mobile #41302, Kearonor Beast | [`specs.highway.c`](../../../src/specs/specs.highway.c#L538) | The cursed-hide proc resolves this local mobile as a morph target; already covered by the dossier. |
| Objects #41304, #41349, #41350 | [`specs.assign.c`](../../../src/specs/specs.assign.c#L1736) | Existing local bindings for the cursed hide, mystical organ, and wonder wand; the dossier covers their handlers. The wand's modern action also names #41350 in [`wonder_actions.c`](../../../src/item/wonder_actions.c#L21) and its device setting at [#294](../../../src/item/wonder_actions.c#L294). |
| Object #41375, black ankh | [`vnum.obj.h`](../../../src/world/vnum.obj.h#L115), referenced by [`specs.highway.c`](../../../src/specs/specs.highway.c#L444) | The symbolic VNUM is used in `blackness_sword`, whose source comment says the proc is not in game; the function has no active literal assignment. It is not counted as an active binding. |
| Mir objects and helper mobiles | [`specs.assign.c`](../../../src/specs/specs.assign.c#L1742), [`specs.highway.c`](../../../src/specs/specs.highway.c#L571) | Highway binds Mir objects #41915 and #41917 to their custom weapon handlers. Its spider/fire procedures inspect Mir web/fire objects #41900, #41901, #41907, and #41908, and its wyrm routines use Mir mobile helpers #42172–#42174 ([Mir audit](MIR_AUDIT_REVIEW.md#compiled-references-and-external-prototype-checks)). |

The raw numeric pass also encounters IDs shared across local room, mobile,
and object namespaces. Those number collisions were typed by source API and
are not treated as interchangeable prototypes.

## Title, history, and disposition

The color-stripped title from [`highway.zon`](../../../areas/zon/highway.zon#L2)
normalizes to “Southern Coastal Highway” and is unique among 351 active
contributors. The only global active-WLD duplicate room VNUM is #51800 in Hell
Two and Hell Three; none is in Highway. The inactive scan found no room-number
overlap with the Highway room set.

The followed WLD history contains the 2011 Tharn Rifts preparation change, a
2013 generic source edit, and the 2009 `areas/` path move. The ZON history
contains the 2015 quest-item-flag pass, the 2013 source edit, and the same path
move. No distinct predecessor or replacement appears in the curated
[replacement register](DUPLICATE_AND_REPLACED_ZONES.md). The active,
un-commented [`areas/AREA` row](../../../areas/AREA#L198) remains **DO NOT MOVE**.

The existing dossier covers the five active boundary pairs, the dropped
negative exit, hazards, resets, custom behaviors, and selected foreign
ownership. This repository-wide scan adds two inactive Surf approaches and
classifies the typed and compiled matches above. Tooling/editor closure,
indirect and persisted references, dynamic route availability, door/reset
state, safe travel, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 230: builder review and current cross-area dependencies

The source-comprehensive dossier now includes eight explicit builder questions
and recommended resolutions. They cover the dropped #41488 descent, two
source-only Surf approaches, the computed Githzerai arrival at #41767, the
horse-merchant text and stock, separate statue/organ switch verbs, the
unplaced scrying organ, trap values and triggers, and the source-wand flag
effect shared by the legacy and modern gem-spray paths. These questions identify owner decisions; they do not claim a
live route, sale, trap effect, or repair.

The active Highway contributor remains **DO NOT MOVE**. Its existing five
reciprocal active boundary pairs and two source-only Surf approaches remain
classified as before. The current typed closure also preserves Highway's
active resets of Mir objects #41915 and #41917, their compiled custom weapon
bindings, and Highway code references to Mir web/fire objects and mobile
helpers. No new attic candidate row or active WLD edge was found. See the
[Highway dossier](../zone-stories/SOUTHERN_COASTAL_HIGHWAY.md#open-builder-questions-and-recommended-resolutions),
[Mir supplement](MIR_AUDIT_REVIEW.md), and
[Highway coverage row](ZONE_COVERAGE_REGISTER.csv#L128).

This checkpoint used static repository sources and documentation only. No
tests, build, server, gameplay, database, migration, or operational script
was run.
