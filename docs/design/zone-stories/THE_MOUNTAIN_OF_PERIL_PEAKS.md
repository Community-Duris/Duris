# The Mountain of Peril Peaks: comprehensive source story map

**Source-comprehensive, revision one — October 4, 2026. Gameplay qualification
remains open.** The [journal](../../../areas/story/nexus.story.json) covers every
one of the ten native deliveries as ten independent outcomes. Twenty-eight
contacts retain all eleven addressed dialogue families. Sixteen optional checks
comprise thirteen current-material checks and three earlier-exchange histories.
Three two-stage errands and four independent outcomes explain the progression;
no native contract is omitted, merged into an entire campaign or newly excluded.

**This checkpoint ships journal guidance and evidence, with no actual native
zone or quest repair.** Gooran's ring-versus-book promise, misleading object
aliases and dormant prototypes are pending builder decisions. The companion
investigation needs a designed continuation. Future actual repairs need separate
clear `fix` commits and prominent PR/news entries naming the player trigger,
before/after behavior, validation, remaining limits and a truthful news sentence.

Active, ready accounting is mandatory for discovery, encounters, journals and
new credit. Frozen obligations retain their separate persistence/recovery paths.
All ten native contracts are item-only, consume at most three objects and leave
the recipient present. Their potential daily shape is retained; it does not
guarantee fresh supply, accessible recipients, safe travel or a playable repeat.
Exact supplied proof fits without personal producer history. A receipt records
its own delivery; it does not prove first recovery, a personal kill, learned
dialogue, healing, access, companion fate or every earlier campaign step.

## Evidence and review boundary

Reviewed every [native block](../../../areas/qst/nexus.qst): ten Q and eleven
addressed M, with no QA/MA or ambient/default families. The empty section for
mobile 57512 has no quest. The [reproducible source index](../../reference/zone-story-audits/nexus.md)
retains exact bindings, aliases, dialogue and reset references.

Reviewed all 196 [rooms](../../../areas/wld/nexus.wld), physically 57500–57695:
143 exact prose groups, 41 numeric headers, 42 non-exit metadata groups and
all 236 exit families. The catalog's broader registered lower boundary is not
an extra room count. Non-exit metadata includes the citadel drawing, current
and falling properties; no separate room script supplies a quest endpoint.
Reviewed all 162 [mobile prototypes](../../../areas/mob/nexus.mob), all 85
[object prototypes](../../../areas/obj/nexus.obj) and both complete
[shop definitions](../../../areas/shp/nexus.shp).

Reviewed every one of 297 [resets](../../../areas/zon/nexus.zon), across 236
families: 140 M, 58 D, 55 E, 25 G, ten F, seven O and two P. Reserved arguments
are zero. Every local native input/reward and reset reference resolves. A global
active producer scan covers all native materials, intermediate outputs, keys
and controls. No foreign material is required for these ten deliveries.
Both ordinary boundary routes reciprocate with Mountain Tracks: riverbank
57637 west to 21015, and cliff 57536 down to 21144. The neighboring cliff has
its own falling property; adjacency is not safe-arrival evidence.

There is no local literal binding in [special assignment](../../../src/specs/specs.assign.c).
The neighboring `nexus_uc` procedures belong to a different area and are not
Peril Peaks bindings. Automatic switches use [object creation](../../../src/world/db.c#L3166)
and [item_switch](../../../src/specs/specs.object.c#L302); travel uses
[check_item_teleport](../../../src/magic/spell_travel.c#L932) through the
[interpreter](../../../src/cmd/interp.c#L2410). Reviewed those shared handlers,
normal exit/reset flags and key handling, command/object dispatch, NPC wandering,
containers, shop stock and relevant fall/current behavior. These mechanisms
explain prerequisites but do not emit accepted quest objectives for this area.

The loader clears stale `ITEM_PROCLIB` bits and reinstates them only for actual
`_proclib_` declarations. No such local description adds a procedure here.

## Ten deliveries and their progression

The following exact native contracts remain distinct achievement/daily units.
Reward or ingredient names that resemble each other do not substitute for the
listed prototype. The journal shows useful prose instead of exposing these IDs.

| Outcome | Exact input → native reward | Story relationship |
| --- | --- | --- |
| [Foreman's shipment](../../../areas/qst/nexus.qst#L12) | Silver bag 57522 → stud 57523 | Miner supplies the bag; stud can be consumed by Roxon |
| [Roxon's jewelry](../../../areas/qst/nexus.qst#L202) | Stud 57523 → tribal bracelet 57583 | Shipment history is optional; supplied stud fits |
| [Spirit's reptile trophies](../../../areas/qst/nexus.qst#L36) | Python scale 57535 + serpent scale 57536 + anaconda pair 57534 → snakescales 57537 | Three distinct kinds; anaconda pair is one object |
| [Spirit's tentacles](../../../areas/qst/nexus.qst#L46) | Plant tentacle 57555 + kraken tentacle 57566 → enchanted tentacle 57519 | Two different kinds; reward is a third kind |
| [Spirit's draco trophy](../../../areas/qst/nexus.qst#L55) | Pair of eyes 57540 → flaming-draco bracelet 57569 | Eyes pair is one object; independent of the other spirit requests |
| [Snow-bear fur](../../../areas/qst/nexus.qst#L99) | Fur 57545 → 5,000 experience | Independent of the sasquach arm; no meat delivery |
| [Sasquach trophy](../../../areas/qst/nexus.qst#L104) | Right arm 57561 → parchment 57548 | Opens a supplied-material route to Gooran |
| [Gooran's parchment](../../../areas/qst/nexus.qst#L119) | Parchment 57548 → prayer book 57511 | Arm-delivery history optional; actual reward differs from promised ring |
| [Troll's revenge](../../../areas/qst/nexus.qst#L184) | Ancient twisted skeleton head 57554 → eye 57549 | Eye can be consumed by injured human; regeneration not an objective |
| [Injured traveler](../../../areas/qst/nexus.qst#L217) | Troll eye 57549 → rune medallion 57504 | Medallion is a key; companion investigation has no further Q |

The hunter spirit's `quest/hi` family explains a failed mission against the
undermountain beasts and offers to enchant recovered parts. His three outcomes
retain story classification because that hunting mission supplies their narrative,
rather than introducing generic material-for-fee services. They do not award
crafting-skill, personal hunting or all-beasts completion. The seven recipients
have eleven addressed families; the tribal barbarian's four families distinguish
task, prize and father, and the troll's two distinguish the forest from regeneration.
No keyword grants its own achievement.

## Actual material sources, custody and stock

The wandering dwarven miner 57544 starts at 57578 and carries the silver bag;
the foreman 57548 is in the mining hut 57622. Roxon and Gooran are separately
placed at the Gray Owl Ale House 57605. The tribal barbarian is at snowy trail
57565, and the spirit is at the underground jungle entrance 57619.
Meeting one does not reveal every other recipient or prove their continued presence.

Reptile sources share nest 57635: anaconda 57549, python 57546 and serpent 57545
have different scale kinds. The serpent can wander. Plant 57520 starts at 57574;
kraken 57531 at 57535; fire draco 57521 at 57577; sasquach 57573 at 57589;
ancient twisted skeleton 57586 at 57513. Sources are reset declarations, not a
guarantee about the current instance or a required personal killing route.
Exact matching proof handed over by another player fits the same native contract.

Snow-bear 57570 has three mobile placements but only two fur G declarations
under a global item cap of two. This is a supply/contention limitation, not proof
the fur quest is broken. It needs a live depletion/reset journey before a daily
availability promise. The miner, head and other parts also have shared caps.
An intermediate receipt does not recreate an already consumed stud, parchment
or eye. Current inventory checks count loose carried proof and do not include
worn eyes, the worn troll eye or nested/corpse material.

The native bag's split `fu ll` aliases are awkward; the stud's usable aliases
include `earring` rather than `stud`, and the kraken tentacle uses `tentacly`.
The journal preserves workable guidance. Correcting aliases needs an additive
builder-reviewed change that retains old command compatibility, with a separately
named fix and command-matching regression if shipped.

## The medallion, emeralds and tower access

Medallion 57504 is `ITEM_KEY` with value[1] zero: persistent rather than
automatically destroyed on unlocking. It keys the reciprocal locked white-washed
gates 57505 north / 57507 south. Unlocking and opening remain distinct actions.
The gate-side dwarf 57511 has boots and a dagger, not a medallion declaration;
the injured traveler's eye exchange is the actual declared medallion producer.

The gardener 57527 at 57524 holds emerald 57558. Touching its exposed object
controls 57507 north toward courtyard 57663. The librarian 57528 at 57523 holds
same-named emerald 57567, controlling 57663 north toward tower room 57525.
Both use command 320 / TOUCH, direction zero and switch-moves mode one; all
target and reciprocal exits exist. Their D8 resets make the controls blocked/open,
while the reverse D0 resets are open. Clearing the blocks does not require
inventing a search/open step for a closed door.

The emeralds have no TAKE wear bit and initially reside on NPCs. The interpreter
dispatches the actor's equipment/inventory and room objects, not another NPC's
inventory, and `item_switch` resolves only those same visible object scopes.
Guidance therefore requires an exposed room emerald. Do not mark an NPC encounter
as switch operation or pretend the NPC's carried switch is directly touchable.
Both holders use the ghost race. The [no-corpse predicate](../../../src/core/utils.h#L815)
and [death handler](../../../src/combat/fight.c#L969) release their contents directly
into the room after death, explaining how the untakeable controls become exposed.
Native combat, custody, release and exposure need a qualified gameplay journey;
same-named objects must preserve exact object identity and target exit.

Additional keys are separate: shaman 57541's bridge key 57564 has break value
100 for the 57549/57550 gate; sentry 57551 supplies defense gate key 57577
with break value one for 57602/57657; Wong 57556 supplies council-cell key
57530, persistent but displayed as silver with `stone key` aliases. Bear chief
57553 supplies persistent key 57526 for chest 57525, whose container flags
include locked and pickproof. The chest's declared contents are plate 57527
and visor 57572. Do not confuse these similarly named keys or grant all access
when the medallion is received.

Small iron key 57556 has no active producer. It is named on 57511 south and
57525 north, but those exits are already-open non-doors and do not require it.
The tower is not established as inaccessible by that unused key reference.
Builder review can remove misleading references or intentionally design a
locked route, but must not silently activate a new gate or reward.

## Portal commands, maze and environmental routes

Four local teleport objects have active room O declarations and valid destinations:

| Object / placement | Actual command and destination | Scope |
| --- | --- | --- |
| Magical twilight 57568 at 57500 | LOOK 15 → observatory 57570 | Looking at the matching object triggers travel |
| Silver sphere 57502 at 57570 | TOUCH 320 → oval chamber 57500 | Return counterpart |
| Black gate 57512 at 57511 | ENTER 7 → aether storm 57515 | Not a guaranteed safe shortcut |
| Snow-covered tent 57582 at hot spring 57628 | ENTER 7 → dragon-fighter tent 57692 | Ordinary north exit returns to the spring |

All have unlimited value[2] -1; no last-charge consumption is asserted. Shared
travel still applies its arrival/arena rules. Commands and actual accepted arrival
are different evidence, especially when movement is rejected or an environmental
effect runs during placement. No local travel receipt currently exists.

Unused teleport prototypes 57500, 57506 and 57508 have no active local/global
typed producer in the reviewed data. The unused cave exit uses MREPORT 103,
not a normal leave command. The cave itself has ordinary connections, so this
does not prove the cave is inaccessible. Unused plaque 57573 contains a tower
riddle; no active declaration makes it a guaranteed present clue.

The elemental route 57664 → east 57665 → south 57666 → west 57527 is a
concrete path while several false turns send a traveler to 57509. Other tower
branches lead to elemental wardians, the dark forest and magi. The journal
explains observed hazards and progression without inventing a terminal victory
contract. Snowy cliffs and the neighboring Mountain Tracks route have falling
properties; room 57529 has current metadata. Review accepted movement, swimming,
flight, current, fall, interruption and safe return before claiming traversal.

Both shops are real: ale-house keeper 57537 has brew 57528, and curios keeper
57560 has potions 57533/57539/57584. The weapon-store prose and weapon master
57559 do not create a third shop; no selling/service achievement is inferred.
The hot-spring tent is a travel object rather than a fountain. Actual fountain
57552 is placed in the courtyard. Decorative fire and prose do not mint quest items.

## Companion investigation and implementation expansion

The traveler explicitly asks for his friends' fate after delivering the medallion.
Five displaced companions 57513/57515/57512/57510/57514 have ordinary M resets
at connected LOAD ROOM 57551, whose exits lead east to FAILED LOAD ROOM 57572
and south to gate approach 57505. They are not F followers or missing mobiles.
Their non-sentinel flags allow independent wandering; the old magician has no
stay-zone flag. Aether dragon 57508 also begins in a connected load room and
can wander. Staging labels alone do not establish inert or broken sources.

There is no later Q for finding, rescuing or reporting those companions. A zone
builder must decide whether success means locating all five, a surviving subset,
recovering a keepsake, learning fates, escorting survivors or defeating a threat.
Preserve these choices as pending design, rather than implying a playable final
rescue. Likewise the troll's regeneration and the human's narrated healing do
not carry explicit persistent native state transitions.

Extend the existing [builder sidecar](../../guides/ZONE_STORY_BUILDING.md) and
[universal plan](../ZONE_STORY_INTEGRATION_PLAN.md) incrementally:

- **Causal item sources:** attach item UID, source instance and committed transfer
  receipt to first recovery; distinguish native corpse/drop, barter/reward, container,
  shop and player handoff. Loose current possession remains useful supplied-proof
  readiness, not historical source evidence.
- **Accepted controls and access:** capture emerald identity, exact target exit,
  successful block removal, reset generation, actor, custody and actual subsequent
  arrival. Medallion grant, key use, opening and entering are distinct endpoints.
  Preserve actor-owned versus room-exposed controls and scarce/destructible keys.
- **Scoped campaigns:** describe the three linked errands as causal episodes with
  explicit AND prerequisites when the builder requires a personal sequence. Existing
  schema completion arrays mean alternatives; putting all stages there would be wrong.
  Supplied proof can remain an allowed branch without fabricating earlier receipts.
- **Companion encounters and resolution:** identify runtime instances, observed fate,
  survival, movement and attempt scope. Commit a designed report/rescue outcome only
  after its real native endpoint. Wrong companion, already reported attempt, dead or
  moved NPC, group ownership and cold recovery need rejection/replay coverage.
- **Truthful effects and travel:** healing, regeneration, maze travel and final guardian
  defeat need actual successful effect/arrival/kill endpoints with interruption and
  restart proof. Dialogue and reward prose alone cannot complete them.

Each zone can continue to own a dedicated `.story.json` file. Add any deeper
prerequisite declarations only after their parser/runtime/event/persistence
semantics are implemented; do not ship inert fields that look enforceable.
Dynamic extraction can suggest bindings, producers, doors and item links. It
cannot settle ring-versus-book intent or design the missing companion finale.

## Pending repairs and balanced builder decisions

| Finding | Assessment | Planned repair/qualification |
| --- | --- | --- |
| Tribal prize dialogue and parchment promise the Bel'Rok ring; Gooran gives prayer book 57511 | Confirmed player-facing discrepancy, not a missing reward prototype | Builder chooses intended reward. Prefer coherent clue correction if the book is intended; any reward change needs balance and exact-binding/revision review. Separate fix and news entry |
| Split bag aliases, stud/earring and kraken `tentacly`; council key displayed silver but matched stone | Existing usable words remain; confusing command discovery | Add clear aliases while preserving old ones; original-failing command lookup tests and separate content fix/news |
| Unused cave/oval portals, MREPORT cave-exit command, unused plaque/small iron key | Dormant or misleading definitions, not proof working routes fail | Builder confirms whether to remove, correct or deliberately place them. Test actual source, correct command, destination and return before enabling content |
| Human asks for companion fate without a continuation | Confirmed unimplemented story endpoint | Design desired investigation/report/rescue and required source/effect/world events; publish as a feature with honest scope, not a repaired existing finale |
| Same-named NPC-carried untakeable emeralds, global proof caps and independently wandering companions | Real prerequisite/custody/contention complexity, not demonstrated missing sources | Qualify exposure/loot/control, depletion/reset, interruption, causal source and surviving-recipient journeys under active accounting |

No migrations, native source/content repairs, database changes, account activation,
live server operations or merge occur in this checkpoint. Static reference and
native C++ journal tests verify guidance and recording semantics; they cannot
qualify actual emerald exposure, live hunting, companion rescue or world effects.
Progress advances to **49 of 220 comprehensive source maps; 171 remain**.
Next in the original queue: **Crakkaros' Liar (`crakkaro`)**.
