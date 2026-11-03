# Arena of the Elder Brain: hidden pit and quiet fungus

Repository-wide boundary, reachability, special-procedure, typed-reference and
tracked-history findings are in the [Illithid Arena audit supplement](../quest-story-followups/ILLITHID_ARENA_AUDIT_REVIEW.md).
This source-comprehensive environmental review covers the current 18-room
contributor. It has no local QST, SHP, or story sidecar, so its narrative comes
from room and mobile prose, resets, two weapon prototypes, and one room
procedure.

## Environmental story

The arena is an underground ritual site. Its ring surrounds a tall stone
plateau, with a stone vat and an amorphous Elder Brain at the center. The
surrounding prose describes ritual violence, damp stone, water runoff, a head
sculpture, a strangely cool southwest corner, and an old combat plateau. The
Elder Brain appears in room descriptions and in the compiled room message for
the WILL command; no Elder Brain mobile or local quest giver is defined here.

The ring opens into a round, smooth dark tunnel and then a featureless black
room. Three short branches end in bare rooms; a quiet, carnivorous fungus is
reset at the far end of the narrowing branch. Its brief description says it
looks upset when disturbed. The fungus supplies the only local mobile. These
sources establish an environmental encounter and a solitary resident, with no
accepted reward or rescue outcome.

## Local route and hidden pit

Room #11200 is the arena center. The ring runs through the east, northeast,
north, northwest, west, southwest, south, and southeast rooms; an up exit leads
to the plateau, which returns down to the center. The west wall opens to the
tunnel and the black room. The three branches from the black room return to it;
one contains the quiet fungus.

The southwest corner #11208 mentions a faint outline or seam. It has a down
exit to pit #11212; the pit description mentions a slab and a reachable ledge.
The paired exits begin closed and secret under the local reset file. They form
an optional hidden route when discovered and opened. The text alone does not
prove a successful SEARCH, door mutation, climb, or live arrival.

The two rooms titled “Arena Combatant Room 1” and “Arena Combatant Room 2” are
outside forward reach from center #11200 in the authored WLD graph. Each has a
single down exit to the center. The reset file places a twisted sword in each
room, but loads no mobile there and defines no local WLD entrance to either
room. See the [directed graph review](../quest-story-followups/ILLITHID_ARENA_AUDIT_REVIEW.md#static-connectivity-and-hidden-arrivals).

## Resets and compiled behavior

The zone resets one twisted sword prototype in the pit, two of a second twisted
sword prototype across the combatant rooms, and one quiet fungus in the far
tunnel. The two sword prototypes share keywords, short description, and room
description, while their item values and affects differ. Their separate VNUMs
and reset placements remain meaningful even though their visible names match.

Startup binds the `squid_arena` room procedure to every room #11200–#11217. Its
current behavior consumes WILL commands and says the Elder Brain stops the
player from enforcing their will. It does not supply a local QST receipt or
record an arena victory. A separate direct code scan also found two old guard
cases for room #11208; the current room has no south or west exit, and no local
guard mobile is reset there. Keep those cases distinct from the hidden pit
route and the fungus reset.

The center, tunnel, and pit use indoor or underground prose. The local WLD
contains no fall-chance or current records. Steep slopes and difficult footing
describe the scene; they do not establish a mechanical hazard.

## Related areas and source limits

The current title “Arena of the Elder Brain” belongs to active
`illithid_arena`. The active `arena` contributor is titled “CTF Tournament.”
Ixarkon and Ixxillikor are separate illithid city contributors whose prose also
mentions Elder Brains. This Arena has no active or inactive WLD approach, no
shared room VNUM, and no direct checked-in type-25 target or typed reset/QST/SHP
dependency into its local prototypes. Similar subject matter does not identify
the vat's Elder Brain as one of those other source-defined actors.

No local QST, sidecar story, shop, or explicit player-versus-brain contract is
defined. The room names, swords, and environmental text do not establish a
challenge result, brain defeat, or campaign completion.

## Builder questions and recommended resolutions

| Question for the builder | Recommended resolution from current source |
| --- | --- |
| Is the vat's Elder Brain a prose-only presence, or should this area contain a distinct live actor or encounter? | Preserve the current room narrative and `squid_arena` WILL response as separate source facts. Add an encounter only with an owner-defined actor, lifecycle, accepted outcome, and evidence; keep the Ixarkon and Ixxillikor brain identities separate until their relationship is explicitly authored. See the [central room](../../../areas/wld/illithid_arena.wld#L1) and [local mobile file](../../../areas/mob/illithid_arena.mob). |
| Are #11216 and #11217 intended as dynamic combat staging rooms despite having no authored entrance or mobile reset? | Confirm their intended caller, arrival, return, and sword use before adding routes or credit. Until then, retain their current source-only one-way exits and do not describe their reset stock as a live combat encounter. See the [staging rooms](../../../areas/wld/illithid_arena.wld#L275) and [object resets](../../../areas/zon/illithid_arena.zon#L17). |
| Should the pit route remain hidden and closed at reset, and are the seam and slab clues sufficient for that design? | Keep both paired D5/D4 exits, secret flags, and reset state distinct from an observed reveal/open/arrival. Clarify prose only after the builder confirms the intended discovery and return. See the [southwest corner](../../../areas/wld/illithid_arena.wld#L162), [pit room](../../../areas/wld/illithid_arena.wld#L233), and [door resets](../../../areas/zon/illithid_arena.zon#L8). |
| Should WILL remain blocked in every room in the 18-room range, including the tunnel and unreachable combatant rooms? | Preserve the current room binding and describe its effect narrowly as a WILL refusal. Any scope or message change should be reviewed as a separate compiled behavior decision, not treated as victory or control of the Elder Brain. See the [room assignment](../../../src/specs/specs.assign.c#L2282) and [procedure](../../../src/specs/specs.room.c#L1407). |
| Are the two #11208 guard cases still intended when the room has no south or west exit and no local guard mobile? | Treat them as legacy or dynamically dependent code until their owner confirms a use. Coordinate any cleanup with the guard owner; do not infer an extra Arena route from those cases. See [`guild_guard`](../../../src/specs/specs.guards.c#L28) and [`guardian`](../../../src/specs/specs.guards.c#L375). |
| Should players be able to distinguish the two twisted swords whose visible names match but whose values and affects differ? | Preserve both prototypes and placements while intent is unresolved. If they are meant as separate rewards or encounters, clarify their identity in names or descriptions without merging VNUMs. See the [first prototype](../../../areas/obj/illithid_arena.obj#L1) and [second prototype](../../../areas/obj/illithid_arena.obj#L13). |
| Is the quiet fungus intended only as an atmospheric resident? | Keep its room and reset separate from any Elder Brain story. If a role or outcome is intended, define it through an explicit source-backed action rather than the mobile's presence or brief reaction. |
| Should the area stay an environmental vignette, or should builders author a local quest or challenge? | Preserve the current no-QST/no-sidecar state until there is a defined giver, accepted action, outcome, and return path. Do not award quest, rescue, or combat progress for finding a sword or entering the arena. |
| Are the steep plateau and smooth, difficult tunnel meant to stay descriptive, given the absence of local fall/current records? | Keep the narrative hazards descriptive unless the builder chooses a specific mechanical effect and qualification path. Do not infer falls or unsafe movement from the adjectives alone. |
| Should copy errors in the room prose be corrected as a text-only cleanup? | The source has spellings such as “plateu,” “scuplture,” and “strethces.” Correct copy only after confirming the intended wording; keep prose repair separate from topology, combat, and room-flag changes. |

These questions record design decisions without changing native source or
claiming a played route, fight, or reward.
