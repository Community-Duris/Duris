# Pragtog's Domain

Reviewed October 10, 2026, Checkpoint 328 against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This dossier covers the
current active WLD, MOB, OBJ, and ZON sources, the active and inactive WLD
boundaries, exact typed references, and followed file history. It records
source facts and owner decisions; it does not qualify live access or change
native gameplay.

## Source inventory

| Source | Current evidence |
| --- | --- |
| Full world list | Listed as zone *106 in areas/AREA line 59. |
| QST list | Listed as *106 in areas/qst/AREA line 53; this is list membership only. |
| Zone header | Pragtog's Domain, zone 106, reset mode 2, interval 15-25, rooms #10600-#10693. |
| WLD | 94 non-empty room descriptions (70 unique after text normalization) and 198 authored exits; all 198 targets resolve in the active world loader. |
| MOB | 26 local mobile prototypes. |
| OBJ | 59 local object prototypes. |
| ZON | 20 door resets plus 122 active object, mobile, equipment, container, and group reset commands. |
| Local quest/shop/story contract | No same-stem QST, SHP, or story sidecar; no native Q/QA contract. |
| Random-area list | Current areas/RANDOM_AREA is empty. |

Sources: [full area list](../../../areas/AREA#L59),
[QST area list](../../../areas/qst/AREA#L53),
[WLD](../../../areas/wld/pragtog.wld#L1),
[MOB](../../../areas/mob/pragtog.mob#L1),
[OBJ](../../../areas/obj/pragtog.obj#L1), and
[ZON](../../../areas/zon/pragtog.zon#L1).

## Authored environmental story

The surface of the story is an orc settlement cut into glowing black rock.
Rooms describe narrow worked tunnels, guard posts, a kitchen, sleeping area,
slaughter room and bloodied passage. The main halls open into Pragtog's
furnished domain and a slave compound. Room text and active resets place
Chief Pragtog, Rekrok the slavemaster, guards, cooks, an old shaman, and
captured derro, goblins, gnomes, dwarves, and duergar in the settlement.
Several descriptions present forced labor, escape attempts, and punishment;
the dwarf reset carries a mining pick into the newly carved tunnels. See the
[approach and settlement rooms](../../../areas/wld/pragtog.wld#L1),
[Chief's rooms](../../../areas/wld/pragtog.wld#L650),
[slave and work tunnels](../../../areas/wld/pragtog.wld#L623), and
[mobile prototypes](../../../areas/mob/pragtog.mob#L29).

A separate branch has a blue-lit alcove, a humming dark statue, a shaman's
quarters, skulls and bone furnishings, a deep chasm, and mushroom growth
around a spirit chamber. The sources supply a small clue chain: the reset
paper note, placed inside niche #10616 in room #10621, warns that a spirit
asked for a password and attacked the narrator; rune-covered book #10618,
equipped on shaman #10606 in room #10625, says, "The love of fungus may lead
to treasure"; the statue's inscription is partly unreadable; and a guardian
spirit is reset in the chamber with floor runes and an ethereal-plane mural.
The source does not provide a complete password, a native quest, or an
explicit reward condition. See the [alcove and statue](../../../areas/wld/pragtog.wld#L369),
[note](../../../areas/obj/pragtog.obj#L174),
[rune-covered book](../../../areas/obj/pragtog.obj#L194),
[statue inscription](../../../areas/obj/pragtog.obj#L257), and
[spirit chamber](../../../areas/wld/pragtog.wld#L863).

The deeper rooms change to decorated halls, a round hole and smooth shaft, a
storage room, an abandoned home, tunnels described as ripped by claws,
scorched workrooms, and a crater left by an explosion. A demon skeleton,
trapped ghost, umber hulk, and other threats or treasure-bearing creatures
are among the local prototypes and resets. The WLD has 23 rooms in this
branch, #10650-#10672; its only WLD edge to the jagged maze continuing at
#10673 is the one-way north exit from #10666. The use of "scorched" inside
these rooms does not identify the separate active Scorched Valley zone as a
predecessor. See the [decorated and shaft rooms](../../../areas/wld/pragtog.wld#L896),
[scorched rooms and crater](../../../areas/wld/pragtog.wld#L1121),
[local reset block](../../../areas/zon/pragtog.zon#L184), and
[prototype inventory](../../../areas/mob/pragtog.mob#L308).

The connected waterfall rooms describe multicolored rock, mist, a concealed
opening, and a clear stream. The waterfall exit from #10691 to #10692 is
reset secret in one direction; a separate clear-waterfall object in #10691
is a type-25 item with target #10692 and enter command. See the
[waterfall rooms](../../../areas/wld/pragtog.wld#L1678) and
[waterfall object](../../../areas/obj/pragtog.obj#L627).

The 94 room descriptions are not accompanied by authored text that ties the
settlement, spirit puzzle, demon crater, and waterfall into one explicit
objective. The active files show environmental narrative and several
interactive-looking props, but do not establish quest completion, intended
sequence, or reward.

## Object-triggered travel and puzzle clues

Five local objects are ITEM_TELEPORT (type 25). Their values name target
room, command number, and unlimited uses (-1); active ZON placements are:

| Object | Target / command | Active placement | Source interpretation |
| --- | --- | --- | --- |
| #10614, faintly glowing fungi | #10619 / hug (49) | Room #10618, ZON line 95 | A source-authored route from the reachable main WLD set into the separate 15-room set. |
| #10615, glowing green fungi | #10618 / kill (9) | Room #10619, ZON line 96 | A reverse item-triggered route back to the main set. |
| #10645, glowing rune circle | #10650 / enter (7) | Room #10648, ZON line 121 | A source-authored route from the 15-room set into the 23-room decorated/crater branch. |
| #10647, glowing portal | #10600 / enter (7) | No local ZON reset found | Prototype exists, but no active placement or external typed placement was found. |
| #10659, clear waterfall | #10692 / enter (7) | Room #10691, ZON line 128 | Targets a room already in the main WLD-reachable set. |

The runtime defines ITEM_TELEPORT as type 25 and checks a matching command
against an item found in the room, inventory, or equipment before resolving
the destination and calling teleport_to. The command interpreter calls this
check before normal command dispatch. This confirms a code path exists; it
does not prove that a player can reach a placed item, satisfy any other game
condition, complete a puzzle, or safely return. See
[ITEM_TELEPORT definition](../../../src/core/defines.h#L140),
[travel handler](../../../src/magic/spell_travel.c#L932), and
[interpreter hook](../../../src/cmd/interp.c#L2410).

## Active graph and authored arrival

Active WLD routes cross the zone boundary at two reciprocal pairs:

| Pragtog endpoint | Neighbor endpoint | Pair |
| --- | --- | --- |
| #10649 up to Underdark #822668 | #822668 down to #10649 | Reciprocal active pair. |
| #10693 north to Black Tunnels #10700 | #10700 south to #10693 | Reciprocal active pair. |

Both arrival rooms are in the same 56-room strongly connected WLD component.
Ignoring door state, directed WLD reach from either arrival reaches 56 of 94
rooms. The local directed graph has three strongly connected components of
56, 23, and 15 rooms; its two weak components have sizes 79 and 15. The
main 56-room set consists of #10600-#10618, #10624-#10625, #10629-#10641,
#10649, and #10673-#10693. The 23-room set is #10650-#10672 and has one WLD
edge to the main set: #10666 north to #10673. The isolated WLD set has 15
rooms: #10619-#10623, #10626-#10628, and #10642-#10648. It has no WLD edge
to the other sets.

The WLD-only 56-room reach result omits those latter 38 rooms, while the
reset objects add potential non-WLD links: #10614 is placed in main-set room
#10618 and targets #10619; #10645 in #10648 targets #10650. These item
routes connect the three authored regions if the relevant rooms and
command-triggered objects are usable. This is a static source relationship,
not an installed-play result.

Twenty ZON door resets set 13 doors closed, two closed and locked, three
closed and secret, and two closed, locked, and secret. A conservative
initial-open traversal that treats those states as blocked reaches three
local rooms from #10649 and two from #10693. Closed doors may be opened,
keys may be carried, and secret passages or item routes may be discovered;
the small initial-open counts are not a player-access judgment. Particular
owner checks are useful for the two #10609/#10616 locked-secret directions
(key VNUM #10611), the #10634/#10637 closed directions (key VNUM #10632
but reset closed, not locked), and the #10655/#10656 locked directions
(reset key VNUM 0). The #10691 to #10692 north exit is reset secret, while
the reverse south exit is not. See the
[door reset block](../../../areas/zon/pragtog.zon#L8) and
[exit flags](../../../areas/wld/pragtog.wld#L159).

The area has no fall-chance rooms or current markers, no negative exits,
no unresolved active WLD targets, and no room-number range mismatch.
The exact active pair inventory is already present in the
[cross-area exit register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L201)
and [pair summary](../quest-story-followups/WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L356).

## Inactive-WLD approaches and existing candidates

All 95 inactive WLD files parsed without issues. None has a room-VNUM
overlap with #10600-#10693. Exactly two non-overlapping source-only exits
target Pragtog #10649:

| Inactive source | Exit | Target | Existing plan |
| --- | --- | --- | --- |
| ud_map1 room #1115029 | up, source line 25505 | #10649 | Existing ud_map1 candidate row 113; source-only and not active ingress. |
| ud_map2 room #1126541 | up, source line 80516 | #10649 | Existing ud_map2 candidate row 114; source-only and not active ingress. |

Preserve both records on their existing candidate plans. They do not change
either candidate's active-inbound count, currently one reference from Shafts
per row. The reviewed source-only subset becomes two ud_map1 approaches
(including the earlier DLSC approach) and 20 ud_map2 approaches. The overall
curated total becomes 4,488: Surf 252, ud_map1 two, ud_map2 20, and
Underworld2 eight among the listed subtotals. See the
[ud_map1 room and exit](../../../areas/wld/ud_map1.wld#L25482),
[ud_map2 room and exit](../../../areas/wld/ud_map2.wld#L80497),
[candidate row 113](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113),
and [candidate row 114](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

## Title, history, and retention

After removing game color codes and normalizing whitespace, "Pragtog's
Domain" occurs once among 442 current zone titles and once among 351 active
contributors. The followed WLD, MOB, OBJ, and ZON histories remain on the
same source stem through the 2009 area-tree move, 2012-2013 area maintenance,
and 2015 quest-item flag update. No alternate-stem rename, predecessor, or
whole-zone replacement is established in the followed paths.

The current active Scorched Valley is a separate contributor, has a distinct
normalized title, no exact normalized room title or description match with
Pragtog, and no direct WLD boundary to this zone. Shared "scorched" subject
matter does not establish replacement or shared ownership.

Keep active pragtog DO NOT MOVE. Preserve the four local source files, both
reciprocal WLD boundaries, reset states, item-triggered routes, and the two
inactive source-only approach records on the existing candidate plans. No
native gameplay edit or attic migration is proposed.

## Owner questions

The accompanying [audit review](../quest-story-followups/PRAGTOG_AUDIT_REVIEW.md)
records 20 source-backed builder questions and recommended resolutions.
The main decisions concern the intended spirit clue and reward, access and
return through the item triggers, the key/door relationships, the two
non-WLD-connected region sets, and the design separation from Scorched Valley.
