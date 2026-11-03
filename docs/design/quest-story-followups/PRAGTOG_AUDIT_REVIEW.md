# Pragtog repository-wide source audit

Reviewed October 10, 2026, Checkpoint 328 against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static repository
audit. It covers the active files, exact references, active WLD graph,
inactive WLD sources, current normalized titles, and followed source paths.
It does not qualify live access, player inventory, editor state, runtime
population, or persistent world data. See the
[Pragtog source dossier](../zone-stories/PRAGTOG.md).

## Active inventory and authored story

The full world list at areas/AREA line 59 and QST list at areas/qst/AREA
line 53 both list Pragtog as *106. The QST list entry does not establish a
quest. Active source consists of 94 WLD rooms, 26 MOB prototypes, 59 OBJ
prototypes, and the ZON file. The local QST, SHP, story sidecar, and native
Q/QA contract are absent.

Room prose and reset actors depict a worked orc settlement with guards,
kitchen, sleeping and slaughter rooms, a slave camp, the chief's rooms,
new mining tunnels, a shaman, a basilisk, a chasm, mushroom growth, a spirit
chamber, a decorated underground wing, scorched workrooms, an explosion
crater, demon remains, and a waterfall. Mob text names captured species and
depicts forced labor, escape, and punishment. This is environmental story
evidence; the source does not state a unified player objective.

The most developed puzzle thread consists of the paper note placed inside
niche #10616 in room #10621, rune book #10618 equipped on shaman #10606 in
room #10625, the partly legible statue inscription in #10623, and guardian
spirit #10621 reset into #10648. The clue text warns about answering a
spirit's password request and says fungus may lead to treasure, but the
native sources do not state the correct password or a contract/reward. See
the [note reset](../../../areas/zon/pragtog.zon#L98),
[book reset](../../../areas/zon/pragtog.zon#L153), and
[guardian reset](../../../areas/zon/pragtog.zon#L199).
The mushroom objects' command-triggered destinations provide related
travel clues, but intent and order require the builder's account. See the
[note, book, and statue prototypes](../zone-stories/PRAGTOG.md#authored-environmental-story).

## Active graph and authored access

The active WLD loader resolves all 198 of 198 Pragtog exit records. There
are two incoming and two outgoing cross-area records, forming reciprocal
pairs with Underdark and Black Tunnels. The exact rows already exist in the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L201) and
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L356); no exit-register
rewrite is needed.

The local graph has weak components of 79 and 15 rooms and directed strongly
connected components of 56, 23, and 15. Raw authored WLD reach from either
active arrival, #10649 or #10693, reaches the same 56 rooms. The 23-room
set #10650-#10672 has a one-way WLD exit at #10666 north to #10673. The
15-room set #10619-#10623, #10626-#10628, and #10642-#10648 is separate in
the WLD graph. The main 56-room set comprises #10600-#10618, #10624-#10625,
#10629-#10641, #10649, and #10673-#10693.

Five local objects have ITEM_TELEPORT type. Three are actively reset into
rooms and provide non-WLD routes across those graph regions: #10614 is in
#10618 and targets #10619 on hug; #10615 in #10619 targets #10618 on kill;
and #10645 in #10648 targets #10650 on enter. The reset waterfall object
#10659 targets #10692 from #10691, which is already within the main WLD
reach set. Prototype #10647 targets #10600 on enter but has no local reset
or found external placement. Runtime source recognizes these command/object
matches and resolves item-teleport targets, but actual availability,
conditions, and safe return were not tested.

The ZON has 20 D resets: 13 closed, two locked and closed, three secret and
closed, and two secret/locked/closed. A static initial-open traversal that
blocks any reset closed, locked, or secret exit reaches three rooms from
#10649 and two from #10693. The traversal is intentionally conservative:
it does not model opening doors, acquiring keys, discovering secret exits,
or using the item triggers. The two #10609/#10616 directions specify key
#10611; the #10634/#10637 directions specify #10632 but reset only closed;
and #10655/#10656 reset locked with key VNUM zero. The waterfall's north
exit is reset secret while its south return is not. These are builder-review
conditions, not a conclusion of inaccessibility.

No active WLD exit has a negative or positive unresolved target. No room
record is outside the numeric zone range, no source exit comes from a
range-mismatch room, and no unregistered WLD file supplies an active target.
There are no fall-chance rooms or current markers.

## Inactive-WLD approaches and candidate plans

The all-95 inactive-WLD sweep parsed every file without issues and found no
room overlap with Pragtog. Exactly two inactive exits target #10649 from
distinct, non-overlapping source rooms:

| Existing candidate | Source exit | Status |
| --- | --- | --- |
| [ud_map1, candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113) | [ud_map1.wld room #1115029](../../../areas/wld/ud_map1.wld#L25482), up to #10649; source lines 25482, 25502, 25505 | Source-only. Preserve on current candidate plan. |
| [ud_map2, candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114) | [ud_map2.wld room #1126541](../../../areas/wld/ud_map2.wld#L80497), up to #10649; source lines 80497, 80513, 80516 | Source-only. Preserve on current candidate plan. |

These exits do not add active ingress or change either candidate's active
inbound count. ud_map1 now has two reviewed unique-source approaches
(including its prior #1117360 to DLSC #36800 approach); ud_map2 now has
20. Curated source-only totals rise by two to 4,488 overall, with listed
subtotals Surf 252, ud_map1 two, ud_map2 20, Underworld2 eight. Keep the
candidate rows RETAIN IN PLACE.

## Exact typed-reference closure

The bounded exact scan found no external QST item command, SHP product,
active ZON reset-role reference, story-sidecar numeric reference, duplicate
local MOB/OBJ prototype, or direct active C/C++ VNUM binding to Pragtog's
room, mob, or object ranges. The scan reviewed 867 external type-25
teleporter declarations and 380 type-29 switch declarations; neither set
targets a Pragtog room. No local shop source or QST contract exists.

Object #10640, a mining pick, is actively reset on a dwarf slave. The exact
C/C++ search found its VNUM inside a [commented-out macro in mining.c](../../../src/economy/mining.c#L95);
the active code uses a name and ITEM_WEAPON check instead
([generic pick check](../../../src/economy/mining.c#L100)). A separate
numeric match for 10600 in [unicode.c](../../../src/net/unicode.c#L239) is
part of the Unicode codepoint range 0x105b3-0x10736, not a zone binding.
Treat the mining macro as historical/commented evidence, not an active
hard-coded Pragtog dependency.

These are exact source scans, not proof about live object custody, generated
editor output, indirect calls, saved player inventories, or dynamic runtime
state.

## Title, followed history, and retention

After color-code removal and whitespace normalization, Pragtog's Domain
occurs once in the current 442-title scan and once among the 351 active
contributors. The followed WLD, MOB, OBJ, and ZON histories continue on the
same stem from the 2009 tree move (642da5400), 2012 area fixes
(e71c1369d, 02d4ac9be, fca1b0132), 2013 content work (71d731c5b), and the
2015 quest-item flag update (f18992b8f). The MOB path also has a
2026-08-26 clean-rebuild and boot-log entry (22fb9fdad); it does not change
the source stem. No path rename, alternate-stem predecessor, or whole-zone
replacement is established.

The active Scorched Valley is a distinct current contributor with a
different normalized title; no exact normalized room title/description
match or WLD boundary was found. Their similar underground/destruction
themes do not establish that one replaced the other.

Keep active pragtog DO NOT MOVE. Preserve the reciprocal Underdark and
Black Tunnels boundary pairs, all reset and item-trigger routes, and both
source-only inactive-WLD approaches. The audit proposes no native change.

## Checkpoint 328 builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PG-01 | What player-facing story joins the orc settlement, spirit chamber, crater, and waterfall? | Keep each authored thread intact; ask the area owner to define a common objective before adding one. |
| PG-02 | Is Chief Pragtog meant to be the area's final antagonist, or a settlement encounter within a larger journey? | Preserve the current rooms and resets until the owner identifies the intended progression. |
| PG-03 | What is the spirit's requested password, and where is the complete clue? | Retain the note, book, statue inscription, and guardian; request the intended answer and clue path before writing quest instructions. |
| PG-04 | Does "The love of fungus may lead to treasure" describe the mushroom teleports, a hidden object, or another interaction? | Keep the authored line and ask for the exact interaction and intended reward. |
| PG-05 | Are the trigger commands hug, kill, and enter intentional for objects #10614, #10615, #10645, and #10647? | Preserve each value and prototype until the owner confirms command text, object targeting, and expected player feedback. |
| PG-06 | Should unreset object #10647, the portal to #10600, be placed, removed, or retained only as an unused prototype? | Leave it unplaced and request an owner decision; do not invent an active placement. |
| PG-07 | Are the two active reciprocal world boundaries the intended arrival and return routes? | Preserve #10649/Underdark and #10693/Black Tunnels pairs while both neighboring owners confirm expected access. |
| PG-08 | Should arrivals at #10649 and #10693 lead directly through currently closed doors? | Keep the reset states and ask the builder to confirm expected starting experience after reset. |
| PG-09 | Are the 23-room #10650-#10672 decorated, shaft, and crater rooms meant to be reachable only through item #10645 or another intended path? | Preserve the one-way #10666 to #10673 WLD edge and ask the owner to define intended entry and return. |
| PG-10 | Is the 15-room mushroom and spirit set intentionally reached through the fungi item #10614 in #10618? | Preserve its hug trigger and its paired #10615 return; request a played access check before route changes. |
| PG-11 | Should the fungi items #10614 and #10615 carry the cross-set trip, including the unusual kill trigger for #10615? | Retain the source values; ask the owner to confirm risk, command, and return expectations. |
| PG-12 | Are the three WLD strong components and two weak components an intentional layout? | Keep authored links unchanged until the owner confirms which separation is deliberate. |
| PG-13 | Is the #10609/#10616 passage meant to be closed, locked, and secret in both directions? | Preserve both D resets and key VNUM #10611; confirm discovery and key custody before altering either side. |
| PG-14 | Is key #10611 intentionally reset after the lost gnome load? | Preserve the reset order and ask the owner to confirm which mobile should carry the key. |
| PG-15 | Is key #10632 meant to serve the #10634/#10637 door even though that door resets closed but not locked? | Keep the key field and reset state as authored until the builder clarifies its intended use. |
| PG-16 | Should #10655/#10656 be locked with key VNUM zero? | Preserve the paired locked resets and ask for the intended unlocking mechanism or a confirmed key owner. |
| PG-17 | Is the #10691 north waterfall opening supposed to be hidden while the south return remains visible? | Preserve the asymmetric secret flags and ask both area and route owners to confirm. |
| PG-18 | Do the NPC and reset placements communicate which captured people escaped, remain captive, or continue to mine? | Keep current descriptions and reset placements while the builder confirms intended character states. |
| PG-19 | Is the mining pick #10640 still intended as a mining interaction item? | Preserve the item and dwarf reset; the exact C++ numeric reference is commented, so request explicit mechanic ownership before relying on it. |
| PG-20 | Is the resemblance between Pragtog's scorched crater wing and active Scorched Valley intentional? | Keep both active zones separate; request evidence of shared authorship or replacement before recording a relationship. |
