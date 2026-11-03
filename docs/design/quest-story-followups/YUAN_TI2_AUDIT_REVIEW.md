# Village of the Damned repository-wide source audit

Checkpoint 205 maps active `yuan_ti2` against its authored story, active WLD
boundaries, reset-aware graph, inactive-WLD approaches, typed dependencies,
title, and followed source history. This is a static source audit. It does not
inspect a live server, player state, database, editor, persisted world state,
or actual gameplay. The [source dossier](../zone-stories/YUAN_TI2.md)
summarizes the environmental story and builder questions.

## Source identity and authored content

The full area list identifies `yuan_ti2` with range note `*806`; the `.zon`
header is #806, reset mode 0, title “Village of the Damned.” The current room
range #80600–#80678 contains 79 room records. The mobile file has 37
prototypes #80600–#80636, the object file has 26 prototypes #80600–#80625,
and the local shop file has one shop, #80628 in room #80656. The area also
appears in `areas/qst/AREA`, but has no local `.qst` or story sidecar and no
native Q/QA completion contract ([area list](../../../areas/AREA#L286),
[zone](../../../areas/zon/yuan_ti2.zon#L1), [QST
index](../../../areas/qst/AREA#L255), [coverage row
352](ZONE_COVERAGE_REGISTER.csv#L352)).

The descriptions form a dark village story from jungle approach through
gated settlement. Recent human-sized and serpent-like tracks, a crushed
orchid, a tortured elf with an anti-invader message, a staked and burned
victim, blood-splattered rooms, and a bloody campsite mark several possible
conflicts. The village is visibly stratified; its mayor is appointed by a
“living god.” The text leaves the victims’ identities and the causes of the
violence unresolved. An elven bard and vulnerable human merchant provide
outsider perspectives. An unreset poor-human prototype describes men without
serpentine features being forced to bring back slaves for breeding and
sacrifice; whether players can encounter that prototype is not established.

The reset data places a Yuan-Ti noble and two Githyanki—one following the
other—in the mayor’s quarters. Their descriptions suggest tension but do not
explain their purpose. The secret fence route from #80671 enters active
neighboring area `yuan_ti` at #80554, titled “An Overgrown Jungle Path.” The
neighbor’s `.zon` title is “The Temple to Skrentherlog.” A note from the
campsite mentions a temple guardian, “Skren,” a dragon, a hammer, divine
anger, and trapped souls, while explicitly stating the note is elven and not
written by a villager. The neighboring boundary and the textual hints do not
settle whether these story threads are connected.

Other authored interactions include a gold key #80612 carried by an angry
human and a chest in the same room whose object record names that key. The
chest contains two blue silk bolts and platinum. The key field is not used by
any WLD exit. A hidden golden eye is reset inside a muddy hole in an abandoned
hut. One local shop belongs to mobile #80628 in room #80656 and sells Heavens
objects #364, #347, and #353. Its configured refusal line says “I do not sell
to your people!” but the source scan does not establish which buyer condition
triggers it.

## Active WLD graph and reset-state reach

The local WLD contains 167 resolving exits: 165 local and two outbound. There
are no negative or unresolved exits. All 79 rooms form one local weak
component when authored exits are treated as available. The active arrivals
and their reciprocal boundaries are:

| Neighbor | Active inbound edge | Paired outbound edge |
| --- | --- | --- |
| Surface | Surface #600250 west (`D3`) to #80600 ([Surface](../../../areas/wld/surface.wld#L2104647)) | `yuan_ti2` #80600 east (`D1`) to Surface #600250 ([local edge](../../../areas/wld/yuan_ti2.wld#L14)) |
| `yuan_ti` | `yuan_ti` #80554 south (`D2`) to #80671 ([neighbor](../../../areas/wld/yuan_ti.wld#L1355)) | `yuan_ti2` #80671 north (`D0`) to #80554 ([local edge](../../../areas/wld/yuan_ti2.wld#L1606)) |

Both boundary pairs resolve in the active WLD set. From either active arrival,
all-edges-available directed traversal reaches 77/79 local rooms. Rooms #80677
and #80678 are the two exclusions: no scanned active or inactive WLD points
into either room. Room #80677 has only two exits to #80678 and a down exit to
the mayor’s quarters #80665; #80678 has no exits. The reset table nevertheless
places one true-blood mobile #80627 in #80677 and equips it with sword #80617,
“Snakebite.” Both rooms’ descriptions tell a player to report the error to a
god. An exact source scan found no compiled numeric VNUM reference to these
rooms. The generic `snakebite()` code is invoked by the race-based innate
attack path; the scan found no binding from that handler to object #80617
([room](../../../areas/wld/yuan_ti2.wld#L1706), [reset and
sword](../../../areas/zon/yuan_ti2.zon#L111), [object](../../../areas/obj/yuan_ti2.obj#L290),
[innate handler](../../../src/classes/innates.c#L1643) and [race dispatch](../../../src/classes/innates.c#L1762)). These are source
anomalies and owner questions, not a live-access finding.

The `.zon` contains four unconditional directional door resets, closing two
pairs without locking them:

| Door pair | Reset state | Source |
| --- | --- | --- |
| #80638 north / #80671 south, fence route | #80638 side closed and secret; #80671 side closed | [zone lines 8 and 12](../../../areas/zon/yuan_ti2.zon#L8) |
| #80656 north / #80657 south, shop quarters | Both sides closed | [zone lines 16 and 20](../../../areas/zon/yuan_ti2.zon#L16) |

After those resets, 161/165 local directed exits are open, non-secret, and
non-blocked. The strict reset-open graph has three weak components sized 77,
1, and 1. Directed reach is 75/79 from Surface arrival #80600 and 1/79 from
`yuan_ti` arrival #80671. The closed shop door isolates #80657 in this graph;
the closed fence door isolates #80671 from the village. The #80671 arrival
still has its open north exit back to paired neighbor room #80554. These counts
exclude exit-open commands, secret searches, dynamic handlers, reset timing,
and actual safe-return qualification.

Rooms #80672–#80676 use sector 26 (`SECT_SWAMP`). There are no authored WLD
fall-chance or current records. Sector assignment and prose do not establish
movement safety ([sector definition](../../../src/core/defines.h#L484),
[loader fields](../../../src/world/db.c#L1395)).

## Source-only inactive-WLD approaches

The all-95-file inactive-WLD scan found two non-overlapping approaches into
active room #80600. They are source-only, not active ingress:

| Inactive source | Exit | Evidence |
| --- | --- | --- |
| `oldmap8.wld` #187735 | West (`D3`) to #80600 | [Room, exit, and target](../../../areas/wld/oldmap8.wld#L162128) |
| `surf.wld` #281432 | West (`D3`) to #80600 | [Room, exit, and target](../../../areas/wld/surf.wld#L1708752) |

Neither source file declares a room VNUM overlapping `yuan_ti2`. Preserve the
Surf record on candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
and the oldmap8 record on candidate row
[81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L81). These add two to the
curated source-only total, now 3,455 overall and 218 for Surf; `ud_map2`
remains 17. The source-only records do not change the active graph.

## Typed dependency reconciliation

All 26 local object prototypes appear in local resets. Thirty-five of 37
local mobile prototypes are reset; #80619 (poor human) and #80633 (true-blood
noble) are unplaced by the local `.zon`. The reset table also imports Heavens
objects #364 (rations), #347 (waterskin), and #353 (venison jerky) for the
shopkeeper, matching its shop inventory ([resets](../../../areas/zon/yuan_ti2.zon#L81),
[shop](../../../areas/shp/yuan_ti2.shp#L1), [Heavens
objects](../../../areas/obj/heavens.obj#L3600)).

| Consumer scan | Result |
| --- | --- |
| Active and unregistered ZON reset roles | No outside reset targets a local room, mobile, or object. Local reset object imports are #364, #347, and #353. |
| External QST numeric references | No token reference to a local room, mobile, or object among 265 current QST files. |
| External SHP products | No shop product targets a local object among 179 other current shop files. |
| External story-sidecar VNUMs | No exact room, mobile, or object VNUM among 232 other story sidecars. |
| External MOB/OBJ definitions | No duplicate definition of a local mobile or object VNUM. |
| External type-25 portals and type-29 switches | No target to a local room among 872 portal and 380 switch declarations. |
| C/C++ exact local VNUM references | None found under `src/`; the generic race-based snakebite attack has no local object binding. |

These are bounded current-source scans. They do not cover editor aliases,
server memory, databases, persisted references, or runtime behavior.

## Title, followed history, and disposition

After color stripping and normalization, “Village of the Damned” is unique
among the 442 current `.zon` sources. Followed WLD, MOB, OBJ, ZON, and SHP
histories retain the `yuan_ti2` stem through the 2009 area-tree path move
(`642da5400`) and later same-stem edits. No alternate-stem predecessor,
retirement, or whole-zone replacement is established. The direct boundary to
active `yuan_ti` #80554 establishes a neighboring area, not a duplicate or
replacement.

Keep active `yuan_ti2` **DO NOT MOVE**. Preserve its two active reciprocal
boundaries, local VNUMs, shop and chest dependencies, and reset placements.
Keep Surf candidate row 97 and oldmap8 candidate row 81 **RETAIN IN PLACE**
with their respective source-only approaches. No `yuan_ti2` attic candidate
row or migration clearance is added.

## Builder follow-up

1. Clarify the victims, “invaders” message, bloodshed, and intended relationship among the village’s violent scenes.
2. Confirm whether poor-human mobile #80619 is intentionally unreset and how its slavery text is meant to function.
3. Identify the intended roles of the elven bard, human merchant, and the Githyanki pair in the mayor’s quarters.
4. Explain the garden, caustic vines, crushed orchid, hidden eye, and the chest/key encounter.
5. Confirm whether the note’s “Skren” and temple references connect to Skrentherlog or another story.
6. Clarify why #80677/#80678 are load-error rooms and why #80627 resets into #80677 with “Snakebite.”
7. Confirm whether unplaced mobile #80633 is intentional and whether the shop refusal has a specific race or group rule.
8. Confirm why `areas/qst/AREA` lists `yuan_ti2` without a local `.qst` source.

The audit records authored files and bounded source references. It does not
qualify live dialogue, shop access, door commands, traversal, safety, or return.

## Checkpoint 439: builder question review

Twenty source-backed recommendations now gather the outstanding character,
clue, door, shop, and ownership questions in the [Village of the Damned builder
review](../zone-stories/YUAN_TI2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The existing connectivity, typed-reference, title/history, and DO NOT MOVE
findings remain unchanged; no source or runtime behavior changed.
