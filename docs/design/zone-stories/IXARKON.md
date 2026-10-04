# Ixarkon: comprehensive source map

Priority 74 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The existing [journal](../../../areas/story/ixarkon.story.json)
is upgraded to schema three, revision two, preserving all three story identities.
It classifies two independent stories and one supporting paid preparation,
with 16 contacts, all 15 addressed topic families and 22 aliases. Four optional
checks cover three current materials and one earlier banker receipt. The two
stories are potential daily candidates; the paid preparation has no achievement
or daily credit. Discovery retains its own achievement. All new discovery,
encounter, journal, achievement and daily credit requires **active, ready
accounting**. Frozen obligations retain separate recovery.

One actual native repair ships independently in
[7baa78c3c](https://github.com/Community-Duris/Duris/commit/7baa78c3c8ee67a2059c314e6372f5ae1843e989):
12 direction words in 11 room descriptions now match reciprocal exits.
No route, key, lock, reset, prototype, quest contract or server behavior changes.
The corrected directional variants increase full title/prose groups from 195 to 198.
Other findings below remain plans, rather than completed repairs.

## Complete source closure

The active `areas/AREA` entry is `ixarkon *964-967`; zone 964 has registry range
96296–96600 and reset mode two. Physical membership is the 201 rooms 96400–96600.
The wider registry interval does not imply that missing lower rooms exist.
The review covers:

- [All 18 native blocks](../../../areas/qst/ixarkon.qst): 15 addressed M and three
  Q, with no ambient MA, QA, scripted quest branch or disappearing recipient.
- [All 201 complete rooms](../../../areas/wld/ixarkon.wld): 198 full title/prose
  groups, 15 numeric header families, the skeleton extra description at 96486,
  and all 468 exits/468 numeric families/259 description-and-keyword families.
  The review includes gardens, towers, meeting room, priestly halls, ceremonial
  clearing, no-exit veil chamber, public markets, no-exit pet storage, mansions,
  hidden banker cave, mushroom forest, water and foreign boundaries.
- [All 51 mobiles](../../../areas/mob/ixarkon.mob) and
  [44 objects](../../../areas/obj/ixarkon.obj): complete descriptions, flags,
  values, affects, worn and hidden inputs, switches, bags, scrolls, light items,
  ritual equipment and coins. No loaded object is silently equated with an
  ordinary CARVE result or a personal source kill.
- [All 385 reset commands](../../../areas/zon/ixarkon.zon): D36/O6/M280/E34/G29,
  302 exact and 323 parent-aware families. No F/P/R reset occurs. Repeated
  placements preserve their own M parent; the first red-cap elder and the
  one spore-bearing myconid are distinguished from their same-kind neighbours.
- [All five shops](../../../areas/shp/ixarkon.shp): Axonokar's weapons,
  Ixaruuk's scrolls, provisions including raft/bags/water/imported consumables,
  Xelrelinak's ringmail/goggles and the clothing merchant's robe. None is
  another quest acceptance, gardener belt bargain or rescue endpoint.
- All literal assignments in [specs.assign.c](../../../src/specs/specs.assign.c):
  mob 96449 `money_changer`, room 96549 `pet_shops`, object 96402
  `illithid_teleport_veil`, and room 96537 `inn`. Automatic
  [creation bindings](../../../src/world/db.c) add type-29 switches 96400/96401
  and ACT_TEACHER mobile 96450. Imported board 90, counter 3097 and bandages
  363/368/369 were reviewed with their complete prototypes and active bindings.
  The commented `illithid_sack` assignment is not an active race restriction.
- Relevant shared [command dispatch](../../../src/cmd/interp.c),
  [object selection](../../../src/world/handler.c),
  [SEARCH and admitted object recovery](../../../src/cmd/actobj.c),
  [door and movement execution](../../../src/cmd/actmove.c),
  [switches](../../../src/specs/specs.object.c),
  [quest selection, allocation, rewards and recovery](../../../src/world/quest.c),
  [production classification](../../../src/world/zone_story_quest_production.c),
  [pet/inn/GithyankiCave procedures](../../../src/specs/specs.room.c),
  [teacher guidance](../../../src/classes/epic_skills.c),
  [storage-locker entry](../../../src/item/storage_lockers.c),
  [board dispatch](../../../src/cmd/boards.c),
  [restoration](../../../src/cmd/staff_character_recovery.c),
  [mind travel](../../../src/classes/psionics.c), justice and reward settlement.
  Local source inspection and generated audit coverage are separate evidence.

Bounded active foreign closure finds no required missing item and no foreign
recipe producing or consuming an Ixarkon kind. Two complete
[Ixxillikor reset groups](../../../areas/zon/ixxillikor.zon) load slave fighter
96433 at room 4305; that [full room](../../../areas/wld/ixxillikor.wld) and the
local actor were reviewed. They do not provide a quest material or new outcome.
Five complete foreign boundary rooms cover all nine directed edges:

| Foreign room | Local room | Source meaning |
| --- | --- | --- |
| 4503, [Underworld](../../../areas/wld/underworld.wld) | 96588 | Reciprocal secret rubble approach, raw flags four, no keyed door |
| 15412, [Faang](../../../areas/wld/faang.wld) | 96474 | Reciprocal ordinary approach; local debris lore does not remove the exit |
| 24012, [Underdark dispersal](../../../areas/wld/mobs_underdark.wld) | 96471 | One-way inbound dispersal, not a reciprocal public entrance |
| 54238, [Connectors](../../../areas/wld/connectorzones.wld) | 96600 | Reciprocal ordinary eastern boundary through water |
| 828097, [Underdark](../../../areas/wld/underdark.wld) | 96471 | Reciprocal ordinary southern water boundary |

All 713 active type-25 portal prototypes were scanned: none targets these local
rooms. This does not exclude custom movement. The veil's 25 actual targets all
exist and their complete bodies were read; two are local hovels. The separately
assigned GithyankiCave room 19890 is absent from active world data and from the
world-room header search. It must not be advertised as a usable entrance.

## The two stories and supporting preparation

| Role | Exact native offering | Output and consequence | Player guidance |
| --- | --- | --- | --- |
| Ixaruuk: component story, Q20 | I96431, one large mushroom spore | C25000 and E50000; D0, recipient remains | Bring the actual hidden loaded spore to Ixaruuk upstairs in his garden tower; Ixaraak's separate study does not accept it |
| Pacing elder: talisman return, Q63 | I96434, one small spider amulet of Lloth | E5500 and I96435, bone-white skullcap; D0 | Identify the pacing elder in the meeting room. Supplied exact amulets skip the banker route. Acceptance records the token return, not global peace |
| Drow banker: supporting preparation, Q116 | I96414, one red skullcap, plus C1000000 together | I96434 and E50000; D0 | Native price is 1,000 platinum. Fresh coin/mixed input is unavailable under active accounting; retain historical evidence and supplied-amulet alternatives without granting service achievement/daily credit |

The [native cost dialogue](../../../areas/qst/ixarkon.qst#L107) describes
revenge against the captor and money for a bribe. The actual acceptance checks
the red cap and fee, with no personal-kill, sibling liberation, escort, departure,
successful bribe or return-to-Menzoberranzan predicate. All three Q recipients
persist. The elder says the return will *hopefully* avert a war; no relation or
world-state change establishes that larger campaign outcome. These are useful
story motivations whose missing endpoints should be explicitly designed.

The banker service retains its exact binding and previous story ID. The amulet
story adds one optional earlier receipt; that history never replaces the current
amulet. A supplied amulet can complete the native return without prior banker
acceptance. A worn, nested, spent or handed-away amulet remains missing now.
The spore and amulet are separate outcomes, not successive mandatory stages.
The existing daily metadata still admits both item-only stories. It does not
promise renewed cap-one stock, a present recipient or a fresh paid amulet route.

## Exact materials and access

- **Spore:** six myconids load in forest rooms 96592/96593/96596/96597/96598/96599.
  Only the M96444 at 96597 is followed by G96431, cap one. Its ITEM_SECRET flag
  requires actual discoverable custody/recovery; neither defeating any myconid
  nor finding decorative mushrooms proves acquisition. The exact object is a
  light item, not a generic anatomical part. The forest is reached through the
  eastern canyon's downward crack, or connected local forest/water approaches.
- **Red cap:** the first M96422 in meeting room 96500 receives E96414, HEAD,
  cap one; the second same-kind elder does not. M96423 is the distinct pacing
  recipient in the same room. The high priest at 96517 wears black cap 96415;
  the return produces bone-white cap 96435. Those are not interchangeable.
  An admitted transfer or supplied exact copy is valid without a personal kill.
- **Amulet:** 96434 is produced by Q116. It is not ordinarily reset into the
  banker's possessions and no local shop sells it. A receipt says an exchange
  occurred; it does not prove current custody or surviving reward settlement.
- **Ixaruuk's tower:** from the garden path 96443, open the eastern gate to
  96444, then climb through 96445 to study 96446. The other tower leads to
  Ixaraak at 96455 and board 90. Gardeners do not dispense a required belt and
  no magical garden barrier is wired into these contracts.
- **Banker:** from canyon 96582 go south into cave 96583, SEARCH the cracks,
  OPEN the concealed southern door to 96584. Both sides use raw flags five
  and key zero; D resets close the door. Recovery, search, opening and passage
  are distinct accepted actions, not automatic achievements for reading prose.
- **Water and rubble:** prepare water travel on the southern/northeastern
  approaches. The northern rubble boundary is secret but not an ordinary
  locked door. Journal prose names useful travel actions without claiming an
  accepted search, shared opening, swim, fall, theft or combat transaction.

## Custom mechanics and service limits

The [veil procedure](../../../src/specs/specs.ixarkon.c) handles ENTER for a
live actor, an object and the exact `" veil"` argument. It has no illithid race
check and no generic selected-object identity comparison. After removing the
actor it repeatedly selects an existing target, moves there, calls self-RESTORE
with command -4, emits arrival text and applies wait three. A handled boolean
does not independently prove admitted movement, actual restoration, safe arrival
or a chosen destination. Normal argument dispatch and alternate spacing must be
qualified before interpreting the literal as a bug.

The complete target list is 2368, 3404, 4108, 4109, 4437, 6900, 11545,
12528, 12535, 12536, 12540, 12541, 15273, 19022, 19275, 23805, 23812,
25458, 25459, 25484, 36171, 96563, 96569, 96803 and 96909. Full target bodies
cover Myconid Village, Flind, Skelenak, Underworld, Worm Caverns, Ghore,
Ethereal, Faang, Gagga'Joba, Highmoor, Earth, Fire, Arac, Ixarkon, Dirkn and
Troll Caves. Some are planar, water, air or confined terrain; existence is not
a survival or return guarantee. Plan bounded target selection before removal,
accepted actual-arrival and restoration receipts, selection/actor identity and
failure/retry qualification. All 25 targets exist today, so an all-targets-missing
loop is a resilience concern rather than a present missing-target claim.

Veil room 96524 has no ordinary exits or inbound local exit. The ceremonial
clearing 96523 describes illithids fading through a veil, but has only an upward
exit to 96522. The assigned `GithyankiCave` entry uses DOWN for illithids or staff
and moves directly to 96524, yet its room 19890 is absent. `real_room0` returns
index zero on a missing room, so the literal assignment also needs a guarded
binding audit; do not assume it harmlessly vanishes or relocate it arbitrarily.
Builder should choose the intended ceremonial ingress and any race restriction,
then implement a guarded assignment and a tested return/access policy separately.
Ordinary psionic mind travel can reach local hovel 96563 but does not supply an
entrance to the sealed veil room or earn this zone's quest acceptance.

The bridge's switches 96400/96401 use PULL340 with targets 96423/east and
96428/west. Their reciprocal exits are raw zero and no reset blocks them.
`item_switch` only removes EX_BLOCKED; an already open target says “Nothing
happens.” The bridge is presently traversable and the levers do not retract it
as prose suggests. Choose clearer wording or an explicit reversible controller
with reset/recovery rules before repairing intended bridge behaviour. Do not
create a mandatory lever step or close a currently public bridge based on lore.

The rest room 96537 has a literal `inn` procedure despite no ROOM_INN flag.
RENT's admitted terminal save, home change, rejected save rollback and extraction
are separate from the ACT_TEACHER actor loaded there. Receptionist 96429 exists
as a prototype but has no reset placement; that does not disable the room
procedure. Teacher 96450 offers class-appropriate ASK level guidance, not another
native Q. Confirm builder intent before adding an innkeeper or changing flags.

Pet-shop room 96549 lists the scrawny goblin and rothe from adjacent real-index
storage room 96550. It checks level, money, pet count and ownership; fresh BUY
and RENT are refused under active accounting. This is not a slave-rescue quest.
Imported counter 3097 delegates ENTER locker to the shared locker procedure;
paid locker entry is also refused while accounting is active. The drow's banker
description does not make that counter an amulet purchase implementation.
Mob 96449's retired money-changer procedure redirects LIST/EXCHANGE to Royal
Bank services without a local exchange mutation. Justice, auctions, merchant
stock, board feedback and priestly lore remain their own systems.

Object 96443 is also the [CHAOS starter bag](../../../src/account/nanny.c#L684),
created and granted through the starter coordinator. That foreign grant is not
a zone quest reward or personal source recovery. The old `illithid_sack` routine
is unbound; do not advertise its race-dependent PUT restriction as active.

## Actual repair and news handoff

Commit [7baa78c3c](https://github.com/Community-Duris/Duris/commit/7baa78c3c8ee67a2059c314e6372f5ae1843e989)
changes only direction words and adds a focused regression:

| Room | Before → after | Verified native route |
| --- | --- | --- |
| 96426 and 96427, slave pens | east → west | D3 to 96421/96422, reciprocal D1 |
| 96448, fungal-garden path | north → south | D2 to 96447, reciprocal D0 |
| 96453, other tower gate | eastern → western | D3 to 96451, reciprocal D1 |
| 96489, camp approach | south → north | D0 to 96490, reciprocal D2 |
| 96491, camp approach | west → east | D1 to 96490, reciprocal D3 |
| 96492, camp approach | north → south | D2 to 96490, reciprocal D0 |
| 96495, sandstone passage | western → eastern | D1 to 96475, reciprocal D3 |
| 96512, Rockspire tunnel | east → west | D3 to 96511, reciprocal D1 |
| 96531, dwelling and cavern | east/west → west/east | D3 to 96530 and D1 to 96532, both reciprocal |
| 96600, water cavern | west → southwest | D7 to 96470, reciprocal D8 |

Player trigger: LOOK in these rooms previously pointed travellers opposite or
away from the matching exit. The corrected descriptions agree with the unchanged
routes. [Focused regression](../../../tests/async/test_ixarkon_direction_clues.py)
fails all 12 clue checks on the original source, passes the corrected clues and
reciprocal destinations/door metadata, and exact scope checks preserve every
other room field and native file. Live LOOK/traversal remains unqualified.

**News sentence:** “Ixarkon's slave pens, fungal gardens, towers, camp approaches,
Rockspire tunnels and water cavern now give directions that match their exits.”

This repair does not announce a restored veil entrance, retractable bridge,
working paid amulet route, rescued slave or averted war. Those remain proposals.

## Capability expansion and qualification

| Gap | Required addition and qualification |
| --- | --- |
| Mixed exact item and fee | A single admitted wallet-plus-root offering with selected recipient/generation, frozen fee/denominations, all-or-none debit, indexed amulet/XP settlement, rejection, busy, duplicate, partial, retry and cold recovery. Preserve current guards until qualified; do not make the service a daily candidate |
| Source identity and first acquisition | Distinguish exact hidden G root and worn E root, admitted search/corpse/steal/other transfer, sender/donor and reset generation. Current possession does not prove first recovery or personal defeat; supplied branches stay valid |
| Optional earlier receipt | Preserve historical service evidence without forcing it for supplied amulets or restoring absent custody. Retain three identities across revision migration; source generation and reward receipts require their own facts |
| Custom sealed/racial/random access | Builder chooses ceremonial ingress/race/return intent; guard missing assignments, validate selected object and target before removal, record accepted actual movement/restoration separately, qualify missing-target/alternate-argument/death/fall/cold cases |
| Bridge control | Define retract/extend/reset semantics or clarify prose. Already open movement must remain usable; handled PULL is not accepted state change or personal passage |
| Lore campaign endpoints | Explicit revenge, sibling rescue, paid escape/escort and diplomatic state predicates, including supplied proof and competing actor outcomes. Neither every keyword nor either item return establishes those world changes |
| Service availability and truthful cards | Show the paid banker, hireling and locker limitations beside useful supplied-material alternatives; identify pacing recipient and distinguish two sages/cap colours. Service state and current material readiness are not completed story badges |
| Renewal and persistence | Qualify actual source/cap/recipient reset episodes, XP policy and per-output settlement, frozen recovery, ledger/pfile persistence and daily admission. Repeated Q metadata alone is not renewed availability |

Focused Python/C++ projections should cover stable identities, all topics and
bindings, exact/wrong/worn materials, supplied amulet without banker history,
historical paid receipt without story credit, spent amulet after history,
canonical ownership, two independent outcomes, replay and cold recovery.
Source checks pin the three contracts, full physical/reset/binding closure and
all 25 existing targets versus absent entry. Full production regression,
maintained build, formatting, source links and publication/preservation checks
are required. Synthetic receipts do not qualify played offers or transactions.

No accounting activation, DB/server operation, migration, deployment or merge
occurred. Queue: **74/220 source-comprehensive, 146 pending; Du'Maathe Castle
(`mntcastl`) next**. The full goal remains active.
