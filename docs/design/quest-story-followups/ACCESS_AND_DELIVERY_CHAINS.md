# Access and delivery chains

**Research record — proposed decisions only; no player-facing change has
shipped.** The exact native contract remains the authority for accepted
offerings. The evidence below does not assert live stock, a played route, or a
personal history requirement unless the native source explicitly provides it.

## QSF-BP-01 — Black Pearl campaign sources

**Priority:** P1. **Confidence:** high for the placement/contract findings;
unresolved whether the author intended a staff/event campaign or an ordinary
repeatable route. **Classification:** confirmed source or entry block, with
intent unresolved.

### What is implemented

The Black Pearl story describes a long chain of recovered fragments, named
contacts, a reconstructed weapon, revenge, and a ship endpoint. The source
records define exact item exchanges for stages of that story. For example,
Telrar's sword reconstruction consumes eight distinct fragments plus the hilt
(nine exact item roots);
the final skin contract is another exact-item transaction, not a proof that all
earlier stages happened to the same character. See the [Telrar contract and
reward](../../../areas/qst/blackpearl.qst#L216) and the existing [Black Pearl
source dossier](../zone-stories/THE_BLACK_PEARL.md#L108).

The ordinary placement declarations are a material blocker: all twenty
campaign mobiles 142215–142234 have their sole ordinary mobile placement in
room 142200, “Newbie Quest Mob Room,” and that room has no exits. The source
matrix also identifies required prototypes without an ordinary producer,
including invitation 142225 and sewer fragment 142206. The invitation's own
consuming briefing and the sewer fragment's same-kind return cannot create the
first usable copy. The source dossier lists those declarations and limitations
in its [placement table](../zone-stories/THE_BLACK_PEARL.md#L108) and explains
the [ship/entrance gap](../zone-stories/THE_BLACK_PEARL.md#L166). The room and
reset files provide the underlying [room](../../../areas/wld/blackpearl.wld#L1)
and [mobile placements](../../../areas/zon/blackpearl.zon#L327).

This establishes a static ordinary-route blocker in the reviewed data. It does
not establish that staff never move an NPC, that no historical stock exists,
or that an event has never run. Those possibilities cannot be promised as
player-available behavior from the reviewed source.

### Proposed disposition

Ask the builder to select one intent:

1. **Event/staff story:** retain the exact source and exchange rules, but make
   player-facing language say how/when an event provides access. Do not label
   the chain repeatable or currently available without an actual event source.
2. **Ordinary playable campaign:** prepare a separate source/entry proposal
   that names every required mobile, item, entrance, reset parent, cap, chance,
   and destination. The proposal must account for the invitation, sewer
   fragment, royal scepter, final ship access, and existing distinct source
   identities. A broad claim that “the ship is open” would not settle those
   requirements.

Until that intent is chosen, a journal may describe only receipts that were
actually admitted through their native contracts. It must not imply that one
receipt proves the whole revenge campaign, that the player personally killed
Ghalasax, or that the final ship route is available. Do not alter item IDs,
rewards, combat, or caps to make an assumed route convenient.

### Implementation proof boundary

Before any future campaign projection or source repair, inspect the exact
recipient, item UID, room/exit, reset parent, cap, chance, and admission
result. Distinguish an event placement from an ordinary reset. Test the actual
eligible and absent-source cases and verify the final destination in the
authored route. Any accepted native exchange remains its own fact; a new
aggregate campaign achievement needs an explicit list of receipts and branch
rules. No such aggregate is approved by this research.

**UNSHIPPED proposal — news wording if a later builder-approved source repair is
made:** “Black Pearl campaign access now follows the reviewed source plan for
[named entrances, placements, and items]. This changes [exact before/after
availability]. It does not change the campaign's item quantities, fees,
rewards, or combat, and the recorded receipts still describe only the exchanges
that actually occurred.” Replace brackets with verified details; do not publish
this draft as shipped news.

## QSF-JADE-01 — Jade princess release and token

**Priority:** P2. **Confidence:** high for the two distinct native exchanges;
the intended meaning of “daughter safe” (released, home, or reunited) is
unresolved. **Classification:** missing tracking capability plus an
unresolved arrival/reunion boundary.

### Native progression

In Jademini, princess 77240 is in the keyed brig. The captive's exact Q accepts
shackles key 77204, returns token 77205, removes the shackles in its text, and
retires that actor. The [Jademini contract](../../../areas/qst/jademini.qst#L27),
[reset placement](../../../areas/zon/jademini.zon#L92), and
[brig/corridor rooms](../../../areas/wld/jademini.wld#L918) establish the
release and access as separate facts. The token description says it proves
membership in the imperial family ([item](../../../areas/obj/jademini.obj#L71)).

The Emperor's independent Jade Q checks that exact token 77205, consumes it,
and gives Jade charm 55372 and hood 76722. Its completion lines say the
“daughter” is safe ([Emperor contract](../../../areas/qst/jade.qst#L336)). The
native predicate is the item bundle; it does not check the receiving
character's release receipt, an escort, a self-unlocking action, travel home,
or a reunion. The existing [cross-zone story map](../../../areas/story/jade.story.json#L20)
already treats the foreign token as a proof that can be supplied, and the
[Jade dossier](../zone-stories/THE_JADE_EMPIRE.md#L181) documents the distinct
foreign source/consumer boundary.

### Proposed disposition

If this chain is projected, record two independent accepted events: (1) the
shackles exchange that releases/removes the captive and issues a token; (2) the
Emperor's acceptance of a token and reward. If the token was supplied by trade
or another valid route, the Emperor's contract still qualifies. Do not require
the same character to have performed the Jademini exchange. Do not report
arrival or reunion; the reviewed native route has no such travel/encounter
receipt. Preserve the token's exact identity and both existing outputs.

If the builder wants a durable “home safe” or reunion outcome, first identify
the destination actor/room and the actual positive event that proves arrival.
The current completion line alone is not that proof. Otherwise, align the
journal copy to “freed” for the first exchange and “royal token accepted” for
the second.

**UNSHIPPED proposal — news wording if projection is later qualified:** “The
Jade story records the captive's release and the Emperor's token exchange as
separate events. A supplied token remains valid. The exchange does not claim
that the princess was escorted home or reunited.”

## QSF-TOWER-01 — Amelia and Dorthan's family proof

**Priority:** P2. **Confidence:** high for accepted item bundles and Amelia's
release exchange; the meaning of Dorthan's “daughter safe” line is ambiguous.
**Classification:** missing tracking capability and unresolved arrival meaning.

### Native progression

Amelia's Q accepts small black key 134026, gives locket 134030, and removes
Amelia from the encounter. Her dialogue suggests she can walk to her uncle;
the reviewed contract records the release/locket transaction, not an escort or
destination arrival ([Amelia Q](../../../areas/qst/lortower.qst#L135),
[reset/key source](../../../areas/zon/lortower.zon#L655), and [mobile
description](../../../areas/mob/lortower.mob#L813)).

Dorthan separately accepts five exact swords 134004 plus locket 134030 and
gives shield 134028. The [contract](../../../areas/qst/lortower.qst#L113)
checks the exact six-item bundle. It does not require that this character
obtained the locket from Amelia, personally rescued her, spoke to her, or
verified her arrival. Therefore, supplied or gifted locket remains valid
proof. “Daughter safe” is exchange dialogue; it does not add a movement or
arrival check. The [Tower dossier](../zone-stories/TOWER_OF_DARKNESS.md#L60)
records the independent contracts and [source boundaries](../zone-stories/TOWER_OF_DARKNESS.md#L69).

### Proposed disposition

Keep the facts separate: key accepted/release performed, locket currently held
or supplied, and Dorthan's exact exchange accepted. If an arrival achievement
is desired, identify and instrument a real destination event; do not infer it
from Amelia leaving the source room or from Dorthan's response. Keep the shield
exchange available for a valid supplied locket, with the same five swords and
reward. Do not add a personal kill, escort, conversation, or original-source
requirement.

**UNSHIPPED proposal — news wording if projection is later qualified:** “The
Tower story now distinguishes Amelia's release and locket exchange from
Dorthan's later item hand-in. The shield still accepts the exact five swords
and locket; the text does not claim an escort or verified arrival.”
