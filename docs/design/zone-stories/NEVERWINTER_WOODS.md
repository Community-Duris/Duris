# Neverwinter Woods

Priority 17, area `moria`, zone 990. Revision 1 of the
[journal](../../../areas/story/moria.story.json) is **source-comprehensive**;
active-world gameplay remains **unqualified**. Accounting must be active for
player journals, discovery, encounters, achievements and new daily eligibility.
Preserve committed-receipt recovery and frozen earlier eligibility independently.

Reviewed all seven native blocks (five Q, two M), 335 rooms, 38 mobiles,
75 objects, three shops, 424 resets and 28 literal mobile assignments. All
1,516 lines of the local special implementation, including three maze functions,
and relevant shared quest loading/matching, movement, switch, object parsing and
shop assignment were reviewed. The [reproducible index](../../reference/zone-story-audits/moria.md)
keeps exact native bindings and source lines. Reset mode is 2, ordinary timed
scheduling; command counts are 213 M, 157 E, 44 G, eight D and two O.
All 120 grouped reset families, their parents, chances and caps were reviewed.

## The five runes are one recovery story

Malchor 99028's five Q contracts all require **one each of 99002–99006**:
amethyst, sapphire, diamond, ruby and emerald. Rotating their G-line order does
not change the offering multiset. Each contract has a different single-item
reward and leaves Malchor present. Their canonical identities remain distinct.

| Source-file order | Reward | Current matching |
| ---: | --- | --- |
| 1 | Emerald-power helm 99073 | Shadowed by the later equal offering |
| 2 | Sapphire potion 99075 | Shadowed |
| 3 | Amethyst snake ring 99074 | Shadowed |
| 4 | Diamond insignia ring 99071 | Shadowed |
| 5 | Ruby eyepatch 99009 | First complete match after native load |

Native loading prepends each Q to the giver's linked list
(`src/world/quest.c`, `boot_the_quests`). Both the durable offering loop and
legacy completion loop stop at the first complete match. Durable matching
selects exact unused carried objects, irrespective of which rune was offered
or the order of the five item kinds. No reward-choice parameter exists here.
The current source therefore selects the **ruby eyepatch**, rather than five
player-selectable reward routes. Actual committed gameplay still needs qualification.

The journal projects all five bindings into **one story, one achievement and
one potential daily unit**, retaining every variant for existing receipt history
and deliberate future integration. Multiple historical variants complete the
family once. It does not advertise a choice or exclude old legitimate receipts.
Five exact live item checks distinguish the kinds; five copies of one rune do
not substitute. One optional bridge-wand check explains access without making
it personal history. Seven contacts include the five masters, Malchor and Agatha;
both native addressable topic families are fully covered.

Delivering supplied runes is valid. The receipt proves the offering and reward,
without proving first acquisition, personal victories, learned dimensions,
every journey or an actual restored forest/museum artifact effect. The native
finale awards equipment; it sets no reviewed world-restoration flag.

## Exact sources and routes

| Rune | Carrier | Initial room | Ordinary item cap |
| --- | --- | ---: | ---: |
| Amethyst 99002 | Master 99003 | 99017 | 1 |
| Sapphire 99003 | Master 99004 | 99043 | 1 |
| Diamond 99004 | Master 99005 | 99070 | 1 |
| Ruby 99005 | Master 99006 | 99158 | 1 |
| Emerald 99006 | Master 99007 | 99185 | 1 |

All five are ordinary G stock with chance 100 after cap-one master M loads;
the master flags include sentinel. Their item type is OTHER and their quest
flag is not evidence of personal kill or birth. No other reviewed active reset
or Q producer supplies these exact runes. Their only native Q consumer is
Malchor. Fresh active legacy G/E/O supply still lacks qualified committed reset
generation. Chance 100 and timed reset do not guarantee a new daily set when
global live caps retain stock elsewhere.

Malchor has a cap-one M placement in museum **99266**. Pond route 99084 east
→ 99332 → 99333 east → invisible bridge 99251 east → tower 99252, followed
by the tower ramp to 99271 west → museum, is separate from the mirror maze.
Pond-ring rooms use a different terrain sector from the rock approach; actual
movement prerequisites and return/retrieval need qualification.

Guardian golem 99015 stands at 99251 and carries wand of entry 99072. This is
an automatically bound **ITEM_SWITCH**, with values `172, 99251, 1, 0`.
Command 172 is **use**; wave is 143. Its inscription's wave instruction is
inconsistent with its actual trigger. Shared `item_switch` opens the indicated
blocked exit when the item and actor satisfy its location rules, without
moving the player or recording a quest objective. The golem's own procedure
only emits periodic guard complaints; expecting an offering in its description
does not implement a paid/permission exchange.

The bridge's raw east exit is blocked, but its **D reset sets state 0**, and
the tower's west reverse also resets state 0. The normal loaded route is open.
Consequently the wand is optional preparation, not a mandatory present access
gate. Builder intent must decide whether the reset or advertised barrier should
change; do not silently impose new quest cost or require the golem's death.

The dome's hole 99141 west → 99199 resets concealed/closed/unlocked (state 5),
with its reverse closed/unlocked. Malchor's bedroom 99282 west and ramp reverse
have asymmetric concealment resets, without a locked key. Ansal's door is
closed/unlocked. No mandatory physical key exchange is invented from these
descriptions. Other region entry links at 99028 east and 99335 west retain
foreign world ownership, not a local travel completion.

## Mirror maze and Agatha

Mirroids 99016 have twenty placements within rooms **99201–99236**, shared
mob cap twenty and sentinel/stay-zone flags. Nine mirror equipment resets have
a shared cap nine. The assigned handler deliberately chooses opening, closing
and movement modes, including combat flight. It changes EX_BLOCKED and can
attempt movement; it awards no rune, learned clue or maze-completion receipt.

All 36 maze rooms' raw destinations remain **inside the maze**. Entrance
99200 west points into 99234; Agatha room 99237 east points into 99202, but
neither connection has an inbound reverse from the maze. There are no local D
commands establishing a reciprocal exterior connection. The handler's comments
describe migrating entrances/exits, while the reviewed implementation's opening
path only unblocks an existing edge; it does not install the new exterior
destination. This is an ordinary-walking access/recovery gap, separate from
Malchor's obtainable tower route. Teleport, staff or recovered world state may
change a played case; do not claim a live crash or guarantee Agatha access.

Specific repair findings from the complete implementation:

- Boundary-search loops vary `i` but inspect **EXIT(ch, direction)** each time,
  rather than each candidate room. They can identify the wrong boundary slot.
- `e_room` stores a virtual room number, then passes it directly to
  `nw_reset_maze`, which indexes `world[room]` as a real room index. Convert and
  validate the identity before mutation; this is not evidence of a played crash.
- Reset-loop endpoints do not consistently match the intended 6×6 reciprocal
  topology; compare all edges against the authored world before choosing repairs.
- Block/unblock assumes valid reciprocal exit pointers and targets. Confirm
  destination, bounds, reverse existence and actual reciprocity before writing.
  Preserve intentional one-way edges separately.
- The paired-display helper's loop visits the source room only despite its
  both-rooms comment. Mode-four closure follows `do_move` without checking
  confirmed arrival, so failed movement can still close the original edge.

A focused deterministic maze fixture should exercise every boundary slot,
blocked/unblocked and zero/four-exit modes, departure failures, alternate real
indices, missing/nonreciprocal reverse edges and repeated ticks. Freeze intended
entrance/exit pairs, restore the old loop and install the new reciprocal edge
as one checked topology transition. Keep existing probability/difficulty unless
the builder explicitly revises it. Record successful player passage using actual
origin/destination and topology/NPC episode; a toggle alone is not travel credit.

Agatha's periodic comments mention Malchor's debt/news and golden horseshoes.
Neither her local M/Q data nor assigned procedure implements news delivery,
payment, horseshoe acceptance or rescue. Museum horseshoes are room projections,
without a successful grab script. Actual horseshoes **99061** are an ordinary
ground O at **99001**, cap one, distinct from that illusion. Wicks 40716's
owned **Divine Home QA** accepts them for **40,000 copper**. This is a separate
foreign consumer, not an Agatha completion, and has its own availability/
accounting qualification. Optional exploration must not become a fabricated
prerequisite for the rune finale.

## Other reviewed content and balanced additions

The remaining **27 assigned mobile routines** emit periodic dialogue/socials.
They do not implement crop harvest, logging grants, worker rescue, cattle/egg
birth or healing objectives. Eggs are ordinary chicken G stock. Lost-human,
half-breed and wood-elf flavor does not implement escort or diplomatic closure.
Conyberry and Thundertree signs, tower books/study and museum projections are
exploration/lore; accepted examination and learned-topic events remain shared
capabilities to author deliberately.

All three shops place Brock, Merthol and Vitnor at their ordinary stores/pub.
Later shop assignment preserves their prior ambient procedures as secondary
callbacks. Ordinary purchases/materials remain economic services, without local
story receipts or invented trade/logging achievements. The amethyst ring's extra
numeric fields are supported optional affect masks in the object parser, not
proof of malformed prototype data.

1. **Make reward intent explicit.** Choose an authored reward selector or an
   intentional fixed reward with shadowed variants retired/documented. A selector
   must freeze selected binding/output and attempt, validate the same five owned
   objects, and preserve historical/frozen receipts across revisions. Keep one
   recovery-family achievement unless the builder authors distinct objectives.
2. **Qualify source and travel separately.** Admit committed reset stock,
   source-versus-gift/transform evidence, actual passage and retrieval, current
   recipient episodes and cap-one replenishment before a played pilot.
3. **Repair maze/access deliberately.** Implement checked topology transitions
   and failure behavior with the focused fixture, then qualify Agatha's round
   trip. Decide bridge reset-versus-barrier and use-versus-wave prose together.
4. **Decide absent story endpoints.** Specify Agatha debt/horseshoe gameplay or
   retain it as lore. Do the same for farm/logging/escort/healing promises; do
   not turn every ambient line or keyword into a quest accomplishment.
5. **Keep shared presentation honest.** Explain exact current readiness, optional
   access, one family and actual reward; reserve historical-source, learned lore,
   branch/choice and topology progression for durable authored events.

## Qualification matrix

| Journey | Source/projection evidence | Required gameplay evidence |
| --- | --- | --- |
| Five runes | Exact kinds, cap-one sources, single family and native variant ownership | Committed stock, source versus gifts, all ingredient orders, missing/spent kinds, current Malchor and native selected reward |
| Reward variants | Reverse native load and first-complete-match semantics | Fixed/choice builder decision, frozen selected terms, retry/recovery, older variant counted once |
| Bridge/tower | Actual open reset, use-triggered wand and museum route | Source admission, blocked/open and wrong-command cases, actual move/return; optional item does not prove access |
| Maze/Agatha | Complete topology/procedure findings and separate lore | Deterministic repairs, current topology episodes, safe round trip/failure/restart; no inferred debt/rescue |
| Other towns/foreign route | Signs, shops/ambient roles and Wicks-owned horseshoes | Accepted examination/learning, economic service support and owned foreign receipt; no invented local completion |
| Discovery/history/dailies | Accounting gate and one projected unit across five bindings | Active/inactive/reconnect and committed repeat/recovery journeys; frozen recipients/eligibility preserved |

Maintain the [implementation plan](../ZONE_STORY_INTEGRATION_PLAN.md) and
[execution register](../ZONE_STORY_ROADMAP_EXECUTION.md). Continue with the
Clawed Caverns. No accounting activation, migration, DB/server operation or
merge belongs to this source-mapping checkpoint.
