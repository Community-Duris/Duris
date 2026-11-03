# `theurgist` source audit: class notes and map asset

Canonical key: `component:theurgist`. Coverage data row 455 / CSV line 456;
candidate dependency data row 104 / CSV line 105. Reviewed October 12, 2026,
Checkpoint 542, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
identity, graph, and dependency review. It does not establish runtime loading
or editor consumption.

## Membership, title, and source history

The candidate WLD/MOB/OBJ/ZON files are not selected by the current
[areas/AREA list](../../../areas/AREA), and `theurgist` is not in
[areas/qst/AREA](../../../areas/qst/AREA). The candidate ZON header is #1,
title `The Horrible Tracts of the Unnamed`, with last room #4. A normalized
case-insensitive comparison against the current active ZON files found no
exact title match. The shared low zone number does not establish an alias.
There is no candidate QST, SHP, or story sidecar.

The initial four-file source commit `de954e5a6` is titled “This is not a live
zone, it is theurgist notes” (July 6, 2010). Followed file history shows later
format/data edits, including the 2012 zone-field conversion, a 2013 edit, and
a 2015 object quest-item flag update. It contains no explicit retirement,
replacement, or archive decision. The wording is strong evidence that this
was created as class documentation, but it does not authorize deleting or
moving the files.

A separate tracked [map asset](../../../areas/map/theurgist.json#L1) sets
`zoneId` to `theurgist` and assigns positions to rooms #1–#4. The file records
`lastModified` as 2025-11-30 and was added by commit `b57048496` (“pfile
migration tool”, 2025-12-28). Its coordinates match the four WLD room IDs and
layout. No in-repository reader or owner for this map format was established;
preserve the asset and ask its editor/export owner how it is consumed.

## Authored inventory and narrative scope

| Source | Inventory | Review |
| --- | --- | --- |
| [WLD](../../../areas/wld/theurgist.wld#L1) | Rooms #1–#4; six authored exits | A four-room class-reference layout; all exits stay local. |
| [MOB](../../../areas/mob/theurgist.mob#L1) | Mobiles #1–#14 | Generic `mob unnamed` records with undead, archon, angel, and golem descriptions. The ZON has no M reset to place them. |
| [OBJ](../../../areas/obj/theurgist.obj#L1) | Objects #1–#3 | `Pets`, `Innates`, and `Spells` reference pages with class-function names, explanatory prose, and editing notes; they read as documentation placeholders rather than ordinary inventory objects. |
| [ZON](../../../areas/zon/theurgist.zon#L1) | Header #1, last #4, reset mode 2; three O resets | Places OBJ #3 in room #2, OBJ #2 in room #3, and OBJ #1 in room #4. There are no M resets. |
| QST / SHP / story | None present | No local quest contract, shop catalog, or story sidecar exists. |

The text documents Theurgist pets, spells, innates, and specializations. It does
not describe a conventional adventure, populated world zone, or quest arc.
Current files also use "theurgist" as a class reference elsewhere. The
[Newbie story names Taiz](../../../areas/story/newbie.story.json#L369), and the
[Newbie room text](../../../areas/wld/newbie.wld#L740) and [Voluntown room
text](../../../areas/wld/Voluntown.wld#L583) mention the class. Those are
separate textual references; none establishes a typed route or source-file
binding to this candidate.

## Local graph and active-number aliases

All six source exits resolve within the four-room source set:

| From | Exit | To | Source lines |
| ---: | --- | ---: | --- |
| #1 | D0 | #3 | 5–8 |
| #1 | D1 | #4 | 9–12 |
| #1 | D2 | #2 | 13–16 |
| #2 | D0 | #1 | 22–25 |
| #3 | D2 | #1 | 31–34 |
| #4 | D3 | #1 | 40–43 |

The numeric graph has one weak component containing all four rooms, and a
raw directed walk from #1 reaches all four. All six targets are local; there
are no negative, positive-unresolved, cross-area, range-mismatch, fall, or
current records. Room IDs are within the ZON range #1–#4.

Three room IDs also have active WLD owners with different complete records:
#1 is active Limbo, and #3–#4 are active Heavens rooms. #2 has no active WLD
owner. A raw scan found ten active exit records targeting this low-number
band: [nine Desert exits to #1](../../../areas/wld/desert.wld#L16949) and
[one Heavens exit to #4](../../../areas/wld/heavens.wld#L859). Under the current
area membership those numbers resolve to [active Limbo room #1]
(../../../areas/wld/limbo.wld) and [active Heavens room #4]
(../../../areas/wld/heavens.wld); they are not candidate-specific ingress.
Candidate inbound active exits remain zero.

The prototype IDs are similarly non-unique. Candidate MOBs #1–#6 have
same-number records in [active `limbo.mob`](../../../areas/mob/limbo.mob#L1),
and #7–#14 in [active `heavens.mob`](../../../areas/mob/heavens.mob#L1);
all fourteen candidate bodies differ from those active records. Candidate
OBJs #1–#3 also have same-number records in [active `limbo.obj`]
(../../../areas/obj/limbo.obj#L1), and all three bodies differ. In `areas_mini`, eight candidate MOB IDs and all three candidate
OBJ IDs overlap by number, but no candidate body is an exact mini-file copy ([mini MOB file](../../../areas_mini/mini.mob#L1), [mini OBJ file](../../../areas_mini/mini.obj#L1)).
Keep the register’s mini fields as numeric-ID overlap counts, not content
matches. The shared IDs do not prove replacement or candidate use.

## Typed code and editor references

The candidate MOB numbers #1–#14 resemble values used in the class runtime,
but those values belong to different namespaces in the checked-in code.
[`pet_restore_state.h`](../../../src/player/pet_restore_state.h#L10) defines
1–14 as summoned-pet creation-kind values, not prototype VNUMs. The maintained
[necromancy implementation](../../../src/classes/necromancy.c#L75) holds the
`undead_data` class table and creates the raised mobile from VNUM #1201
([call site](../../../src/classes/necromancy.c#L563)); active
[`heavens.mob`](../../../areas/mob/heavens.mob#L2571) defines #1201. The code
then writes class-derived descriptions and stats. This does not establish use
of the candidate’s #1–#14 prototypes.

A bounded numeric-literal scan also finds unrelated low-VNUM callers: the
[training-dummy fallback reads mobile #7](../../../src/combat/training_dummy.c#L269),
[Shabo trap code reads mobile #14](../../../src/specs/specs.shabo.c#L647), and
[corpse creation reads object #2](../../../src/classes/necromancy.c#L2648).
Current active files own those same numbers in Heavens/Limbo with different
bodies from the candidate records. These literal values cannot be attributed
to `theurgist.mob` or `theurgist.obj` by number alone. Preserve the area-qualified source distinction and ask the
loader/code owners before changing it.

The map JSON is the only direct checked-in editor-style asset found for the
candidate. A repository search did not identify a reader for `areas/map/*.json`;
that is an unresolved tool-ownership question, not proof that the map is
unused. Keep the source and map together while the editor/export lifecycle is
identified. No activation, co-loading, renumbering, move, merge, or archive
clearance follows from this review.

## Disposition

Keep `theurgist.wld`, `.mob`, `.obj`, `.zon`, and `areas/map/theurgist.json`
**RETAIN IN PLACE**. Preserve their identity as class notes and a matching
four-room layout asset. The explicit 2010 “not a live zone” history, absent
full-list membership, active same-number owners, and maintained class code
support keeping the candidate out of the active zone list unless the content
and tooling owners decide otherwise. Resolve the twenty owner questions in
[THEURGIST_BUILDER_QUESTIONS.md](THEURGIST_BUILDER_QUESTIONS.md) before any
source move or disposition proposal.
