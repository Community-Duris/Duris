# Vast Hidden Grove: comprehensive source story map

Reviewed October 2, 2026. Source area `solonar`, zone 306, journal revision 2.
Reviewed all 100 rooms 30600–30699, 41 mobiles, 72 objects, 15 Q contracts,
12 addressable M families, seven shops and 152 reset commands. The journal
retains three equipment stories, two bird requests and ten preparation services
in fifteen rows. All native terms, bindings, classifications and receipt IDs survive.

## Evidence boundary

Reviewed the complete active [Q/M source](../../../areas/qst/solonar.qst),
[rooms](../../../areas/wld/solonar.wld), [mobiles](../../../areas/mob/solonar.mob),
[objects](../../../areas/obj/solonar.obj), [resets](../../../areas/zon/solonar.zon)
and [shops](../../../areas/shp/solonar.shp). The
[reproducible index](../../reference/zone-story-audits/solonar.md) records all
native contracts, topics, prototypes, resets and literal assignment candidates.
The commands comprise 65 M, 53 G, 22 O and 12 D; no F, E, P or custom reset
command changes their equipment-parent interpretation. All local object/mobile
load chances are 100%, with separate live caps determining stock availability.

Reviewed shared Q admission/reward recovery in
[quest.c](../../../src/world/quest.c), source generation/room loading in
[db.c](../../../src/world/db.c), door/key rules in
[actmove.c](../../../src/cmd/actmove.c), speech doors in
[actcomm.c](../../../src/cmd/actcomm.c), scenery teleports in
[spell_travel.c](../../../src/magic/spell_travel.c), wandering in
[mobact.c](../../../src/mob/mobact.c), ordinary pickup/corpse recovery in
[actobj.c](../../../src/cmd/actobj.c),
[item_command_policy.c](../../../src/item/item_command_policy.c) and
[fight.c](../../../src/combat/fight.c), and falling in
[interp.c](../../../src/cmd/interp.c) and
[falling.c](../../../src/world/falling.c). Shop, teacher/smith tables and literal
special assignments were checked for local execution leads.

No literal reference to a 306xx VNUM or Solonar-specific implementation occurs
in maintained C/C++ source. This does not rule out computed dispatch; the
reviewed active behavior is native Q plus shared data-driven rules. The index's
`inn` assignment at 30511 is a candidate inside the derived zone range
30507–30699, but no active world file defines room 30511. It is not an executable
local inn location. `real_room0` maps a missing room to index zero, so the
unguarded assignment can affect the first world room until later assignments
overwrite it. Plan a checked assignment/diagnostic and validated target review;
do not create an inn or substitute a room merely from a range guess.

Global active Q/reset/room/object-property searches establish ingredient
consumers, one reused foreign mobile and the Homestead statue continuation.
These bounded foreign reads do not qualify the whole Homestead or Alatorin again.

## Story families and exact exchanges

The narrated route is grove exploration → elemental/plant/animal materials →
specialists → one or more equipment finales. These are independent stories.
Native Q admission does not test earlier receipts, accepted topics, class/deity,
personal kills, harvests, curse removal or visiting a particular source. Exact
supplied materials can satisfy the delivery without proving personal acquisition.

Every listed material is one copy unless the reward explicitly says two. Coins
are copper-equivalent native terms. Only the three **fees** are mixed offerings;
seven other exchanges pay **coin rewards** from item-only offerings.

| Giver / row | Exact offering | Exact reward | Classification / closure |
| --- | --- | --- | --- |
| Phonar 30600 | Shattered diamond 30661 + **4000 copper fee** | Radiant diamond 30662 | Service; giver remains |
| Aidon 30601 | Dragon tooth 30651 + dragon scale 30652 | Linked scales 30658 + 10000 copper | Service; giver remains |
| Beowulf 30602 | Panther hide 30634 + buckskin 30642 | Cured robes 30656 + 10000 copper | Service; giver remains |
| Striker 30603 | Orb 30640 + dust 30650 + cured robes 30656 + lavender thread 30657 + linked scales 30658 + **1000 copper fee** | Arch-magi robes 30671 | Equipment story; giver remains |
| Xaves 30604 | Panther hide 30634 + raw golden thread 30635 + adamantium 30636 + filament 30649 + ancient scroll 30666 + **10000 copper fee** | Piwafwi 30670 | Equipment story; giver remains |
| Umpf 30605 | Scale amulet 30660 + treant heart 30663 | Pulsating heart 30664 | Service; giver remains |
| Mogoe 30606 | Raw golden thread 30635 + filament 30649 | Lavender thread 30657 + 10000 copper | Service; giver remains |
| Nymph 30615 | Azalea 30653 + mistletoe 30654 + ginseng 30655 | Faerie dust 30650 + 15000 copper | Service; no D despite departure prose |
| Lich 30617 | Tablet 30639 + orb 30640 + quill 30641 + buckskin 30642 | **Two** ancient scrolls 30666 | Service; giver remains |
| Faerie dragon 30620 | Mandrake 30632 + nightshade 30633 + elk horn 30638 | Adamantium 30636 | Service; giver remains |
| Hawk 30631 | Dead mouse 30668 | 10000 copper | Request; D removes hawk |
| Swallow 30632 | Dead earthworm 30669 | 5000 copper | Request; D removes bird |
| Centaur 30635: quill | Air residue 30643 + magical sand 30646 | Quill 30641 + 6000 copper | Service; giver remains |
| Centaur 30635: orb | Pech rock 30644 + salamander scale 30645 + water 30647 + storm fragment 30648 | Orb 30640 | Service; giver remains |
| Valin 30638 | **Small pickaxe 30659** + radiant diamond 30662 + pulsating heart 30664 | Mage Bane 30665 | Equipment story; D removes Valin |

The centaur's shared `quill orb` response explains two different recipes; it is
not a two-item alternative checklist. Striker has two M families (`mage` and
`items`). All twelve M families have a valid representative topic among fourteen
contacts. Xaves and the birds have no addressable M family. `valin` distinguishes
the recipient from Phonar; `swallow` is an actual mob keyword whereas the bird's
long/Q descriptions call it a sparrow. Asking about an alias does not produce a
durable learned-lore achievement under the current schema.

## Three equipment routes and shared ingredients

Striker's five specialist exchanges appear as optional preparation receipts.
The final story requires the five current ingredients and native terminal
receipt. Crafting each personally is not an encoded prerequisite. The lich's
orb/quill services are also optional preparation; one of his two output scrolls
can feed Xaves' story along with the faerie dragon's stone. These services do not
add zone/daily achievements or prove all stages of a campaign.

Mogoe consumes the same golden-thread/filament pair that Xaves requires raw.
His lavender output serves Striker and **cannot** replace Xaves' golden input.
The lich and Striker consume separate orb copies; Beowulf and the lich consume
separate buckskin copies; Beowulf and Xaves consume separate panther hides.
An old preparation receipt does not make a spent ingredient available again.
Future recipe projection needs explicit consumer edges, exact quantity and UID
lineage, current availability and owned alternatives rather than a guessed
linear path or a historical-receipt shortcut.

Valin's dialogue mainly mentions the diamond and heart, but the Q additionally
requires the miner's small pickaxe. The journal states all three. Repairing a
diamond and awakening a heart are optional history. Phonar's unavailable fee
does not prevent an item-only Valin exchange using a valid already supplied
radiant diamond, if all three ingredients and the actual recipient are available.
Valin and the two birds disappear after their own exchanges. Availability belongs
to the current NPC episode/reset, not a permanent global branch closure.

Several quest props are executable consumable types: raw golden thread,
adamantium, orb and radiant diamond are wands; water and faerie dust are potions;
ginseng is food. Native Q checks their exact prototype, not pristine charges or
a verified gathering origin. Preserve ingredients for their recipes. A future
personal-source objective needs committed first acquisition and custody evidence,
with separate supplied-material policy; item type or present possession is not proof.

## Ingredient sources and actual availability

| Ingredient | Reviewed ordinary placement lead |
| --- | --- |
| Panther hide 30634 | G on panther 30630 at 30601/30623; item cap two |
| Buckskin 30642 | G on buck 30608 at 30606/30611; item cap two. Other buck placements carry hide spikes 30637 instead |
| Elk horn 30638 | G on elk 30609 at 30618; cap one. Other elks are not additional horn sources |
| Golden thread 30635 | G on mimic 30616 at 30687, below the Striker branch |
| Filament 30649 | G on cave fisher 30611 at 30673/30676/30678; cap three. Fisher at 30690 has no filament G |
| Treant heart 30663 | G on treant 30607 at 30679; cap one. Treants at 30629/30680/30681 have no heart G |
| Dragon tooth/scale 30651/30652 | G on deep dragon 30639 at 30695; each cap one |
| Shattered diamond/scale amulet 30661/30660 | Separate O floor stock at 30695; no recorded dragon kill is required to qualify their delivery |
| Small pickaxe 30659 | G on wandering miner 30636 at 30607; cap one |
| Plants/tablet | O azalea at 30600, mistletoe at 30614, ginseng at 30627, mandrake at 30680, nightshade at 30683, tablet at 30688; each cap one |
| Air/pech/sand/salamander/water/storm 30643–30648 | G on corresponding elemental 30622–30627 at 30651/30653/30654/30655/30656/30657; each cap one |
| Dead mouse 30668 | G on mouse 30633 at 30614; cap one; ordinary pickup defect below |
| Dead earthworm 30669 | G on worm 30640 at 30602/30616; cap two. Unplaced worm 30637 is not another active producer |

Fresh O/G source generation is deliberately refused under active accounting
until reset generation is integrated. Existing recovered stock can differ. A
reset line is a placement lead and cap, not confirmation of live committed stock.
The Alatorin reset at 83958 reuses fisher 30611 but provides no following filament
G. No foreign native Q uses a local Solonar item. Prototype owner, physical
placement, current carrier and story owner remain separate concepts.

Valin and the miner both load at 30607 with live cap one and `ACT_STAY_ZONE`,
without sentinel. No active ordinary incoming exit or reviewed item teleport
targets 30607. Its north exit leads to reachable 30600; south leads to exitless
30612. Generic wandering can make either NPC available in the glades or trap
it in the holding room. This is an availability gap, not a nonexistent recipient
or guaranteed permanent failure in every loaded world. Reproduce fresh/recovered
episodes, then review a return exit, wander barrier or source placement with a
builder. Do not advertise a deterministic Valin/miner route until qualified.

The dead mouse is ITEM_TRASH with wear flags **zero**, unlike the takeable dead
earthworm. Ordinary level-below-60 pickup from ground or an external corpse
requires ITEM_TAKE and refuses this item. A whole NPC corpse is likewise not an
ordinary takeable replacement. Staff pickup, existing supplied custody or a
different custom transfer can differ; no ordinary alternate mouse route was
established. Reproduce the player journey and review adding TAKE or an explicit
recoverable quest-grant route, preserving exact actor/source/UID evidence.

## Access, scenery and foreign arrival

The Homestead pond 35810 loads statue 35804: `stare statue` reaches grove room
30648. Local Solonar statue 30604 returns to 35810. Both use generic item
teleports. Arrival discovers the actual destination's own zone while accounting
is active; reading about a route or possessing a statue proves no arrival.

| Local teleport object / placement | Actual command → destination |
| --- | --- |
| Circle 30600 at 30652 | `stare circle` → 30647 |
| Pedestal 30601 at 30629 | `touch pedestal` → 30646 |
| Circle 30602 at 30647 | `stare circle` → 30652 |
| Pedestal 30603 at 30646 | `touch pedestal` → 30600 |
| Solonar statue 30604 at 30648 | `stare statue` → Homestead 35810 |
| Sehanine statue 30605 at 30658 | `stare statue` → 30645 |
| Sehanine statue 30606 at 30645 | `stare statue` → 30658 |
| Soot disc 30625 at 30627 | `tap disc` → 30663 |
| Soot disc 30626 at 30663 | `tap disc` → 30627 |
| Opening 30628 at 30692 | `enter opening` → 30693, toward the deep dragon |
| Opening 30629 at 30674 | `enter opening` → 30698, toward the lich |
| Opening 30630 at 30693 | `enter opening` → 30692 |
| Opening 30631 at 30698 | `enter opening` → 30674 |

All thirteen properties use value[2] = -1 and have valid reviewed destinations.
Statue descriptions about kissing, deity or profession are not additional
assigned eligibility checks. Commands come from actual teleport values. Four
identically aliased openings depend on the actual selected scenery object and
room. Retain destination validation/confirmed-arrival plans: shared travel can
print glow before confirming movement, and missing destinations must be guarded
before room-property lookup. No current local missing destination was found.

The Point 30622 north → 30629 is reset D6, locked/secret, key -2, last keyword
`arrow`. Speech unlocks/reveals it and its reciprocal face; ordinary opening
still follows. The reverse's last keyword is `shaft`. Inner circle 30650 south
→ 30652 is D6 with last keyword `solonor`; its reverse is D5 and unlocked.
The hawk's circle offers another route into 30652. Pale tree 30637 east → 30638
and apex 30640 up → 30649 are D5, closed/hidden but not keyed admission checks.
The passage 30684 east → 30685 is ordinarily closed. Centaur passage 30651
north → 30699 is D5 with negative key and keyword `circle`, but **unlocked**:
discover/open works without a password because shared speech requires locking.

Striker's 30686 downward exit names keyword `striker` and a world-field state of
4. `setup_dir` retains only the low two property bits; no D reset closes/locks
this face. It is currently an ordinary open exit to mimic room 30687. His Q's
vanishing-wall prose does not mutate it. Builder review must decide narrative
clarification versus intended gated access; adding a prerequisite now would
change behavior, not merely recognize an existing quest.

High branch rooms 30631/30636/30637/30639/30640 have F values 35/40/45/50/55.
These are **chance_fall**, not a required climbing skill rank. The interpreter
can roll on commands; flight/levitation, a protected mount or an active climb
catch can prevent falling under shared rules. Falling has real scheduled
movement/injury outcomes. Future journey objectives must record confirmed
arrival/hazard outcome, never a printed climbing description or one attempted move.

## Lore, shops and source defects

The lich's lost-tablet/curse account, nymph's departure, Striker's back wall and
living-heart transformation include narrative beyond the exact Q effects.
The nymph remains because her Q lacks D; lich scroll delivery does not implement
an independent curse-removal state. Review intended narrated closure versus
additional effects before adding achievement claims. Tablet floor placement is
an ordinary source; the lore's theft/plane journey is not enforced acquisition.

Seven local shops use shared shop execution. Active buy/sell remains deliberately
unavailable pending accounting support; native Q dispatch still supports the
reviewed item-only exchanges with valid durable offerings. A shopkeeper's
existence is not another story achievement. Unplaced creatures, scenery extra
descriptions and magical-wall prose are documented exploration/lore leads rather
than invented native quests. No live local inn is established by absent 30511.

## Repair and qualification plan

| Finding | Proposed repair / proof |
| --- | --- |
| Three mixed fees unavailable | Add atomic exact item multiset + identified wallet debit with frozen terms, committed rewards and safe compensation/replay. Test fees independently of item-only coin rewards |
| Fresh material/scenery generation refused | Complete existing durable reset-generation plan, preserving source occurrence, cap, NPC episode and exact object UID |
| Valin/miner access and holding | Reproduce wander/cap/current-location episodes; builder chooses placement/barrier/return repair. Keep supplied ingredients valid and D closure episode-specific |
| Dead mouse lacks TAKE | Qualify ordinary recovery refusal; review TAKE versus a dedicated committed recovery action. Prove direct and supplied custody separately |
| Missing inn target 30511 | Validate assignment target resolution against actual active rooms, skip/log missing targets, review intended owner/location. Preserve legitimate room index zero; do not silently remap |
| Prose versus reset access | Compare loaded exit state with D commands; decide Striker wall and centaur password intent before gating. Test bypass/supplied/alternate routes |
| Shared ingredient consumption / two scrolls | Model producer/consumer edges, distinct UID outputs and competing inputs. Test separate orbs/raw-thread copies, two-scroll recovery/replay and no duplicate births |
| Narrative closure and topics | Add accepted-topic/confirmed effect adapters only for authored intent. Keep supplied delivery, lore, current access and historical personal source distinct |

Qualification should cover each of the fifteen exchanges, all three fee
rejections without item loss, both centaur recipes, exact small pickaxe and
golden/lavender distinctions, two-scroll reward recovery, independent equipment
order, every scenery command/destination, locked versus merely hidden faces,
flight/climb/falling outcomes, normal mouse/worm recovery, NPC removal/reset,
holding availability, restart and replay. Current source/native projection tests
do not replace a played active-world journey. No database operation, accounting
activation or content-balance repair is part of this mapping batch.
