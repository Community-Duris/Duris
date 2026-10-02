# Torg: comprehensive source story map

Reviewed October 2, 2026. Source area `torg`, zone 289, journal revision 2.
The source contains 282 rooms, 126 mobiles, 100 objects, 15 Q contracts,
15 addressable M response families, two shops and 597 reset commands.
The journal retains twelve achievements and two services in fourteen rows;
the two fine-chisel giver alternatives share one accomplishment. All native
bindings, categories, offering terms, rewards and receipt identities survive.

## Evidence boundary

Reviewed the complete active [Q/M source](../../../areas/qst/torg.qst),
[rooms](../../../areas/wld/torg.wld), [mobiles](../../../areas/mob/torg.mob),
[objects](../../../areas/obj/torg.obj), [resets](../../../areas/zon/torg.zon)
and [shops](../../../areas/shp/torg.shp). The
[reproducible index](../../reference/zone-story-audits/torg.md) records all
native contracts, topics, prototypes, reset commands and literal assignments.
The 597 commands comprise 300 M, 41 F, 43 G, 81 E, 15 O, three P, 112 D and two R.
Reviewed all rooms 28900–29181, including the hidden and holding rooms, rather
than assuming that prototype membership follows the derived zone room bounds.

Three active literal assignments in
[specs.assign.c](../../../src/specs/specs.assign.c) attach `timoro_die` to
28961, `lanella_heart` to 29025, and `inn` to room 29103. The first two
implementations are in [specs.mobile.c](../../../src/specs/specs.mobile.c) and
[specs.winterhaven.c](../../../src/specs/specs.winterhaven.c). Room 29181's
`bersekerproc` assignment is commented out. Reviewed its bounded implementation
in [specs.room.c](../../../src/specs/specs.room.c) to distinguish unused class
conversion code from an executable Torg altar.

Dynamic/table-driven leads also matter: Tibornor 28975 is an epic teacher through
[epic.c](../../../src/world/epic.c), and Kordor 28917 appears in the smith table
in [tradeskill.c](../../../src/economy/tradeskill.c). These are not additional
literal assignments in the index. Reviewed the teacher and smith dispatch,
boot ordering in [comm.c](../../../src/net/comm.c) and
[db.c](../../../src/world/db.c), and shop assignment/secondary dispatch in
[shop.c](../../../src/economy/shop.c).

Shared review covers reset parent selection, object creation, Q admission and
durable offerings in [quest.c](../../../src/world/quest.c),
[item_movement_transaction.c](../../../src/item/item_movement_transaction.c),
key/door/teleport and follower movement in
[actmove.c](../../../src/cmd/actmove.c), speech doors in
[actcomm.c](../../../src/cmd/actcomm.c), mobile wandering in
[mobact.c](../../../src/mob/mobact.c), death dispatch in
[fight.c](../../../src/combat/fight.c), and item publication in
[handler.c](../../../src/world/handler.c). Bounded foreign reads establish the
legend-scroll transformation, Uz's terminal exchange, Sierra's two heart
contracts and the Tallin advisor's head contract. They do not establish a
comprehensive review of Winterhaven, Juiblex or Scorch Valley.

## Narrated route and actual admission

The suggested route is city access → hide/favor commissions → council secrets
and Colosseum → legendary relics → invasion → locket and equipment commissions.
These strands can be pursued independently. No local Q checks earlier receipts,
learned topics, faction membership, a personal kill, weekly participation,
invisibility, invasion completion or liberation of Torg.

All fifteen Q contracts are item-only, have no coin fee, and retain their giver.
Each offering requires the exact listed prototypes/counts together. A supplied
item qualifies for delivery without proving its acquisition source. Optional
curing, favor, rose and locket receipts explain preparation; they do not block
the next required action when the terminal materials are already supplied.
Current history establishes committed native deliveries, not personal recovery,
accepted dialogue or confirmed access. An all-stage Torg campaign needs an
authored stage policy; an any-of contract list cannot mean all stages completed.

## Hide and favor routes

| Recipient | Offering | Result | Story role |
| --- | --- | --- | --- |
| Kordor 28917 | Raw dracolich hide 28923 | Cured hide 28924 | Supporting service |
| Jeweler 28936 | Cured hide 28924 | Dracolich-hide bracelet 28911 | Independent achievement |
| Keeper 28922 | Illithid tentacle 28906 | Token of favor 28917 | Independent achievement |
| Pacing woman 28965 | Token 28917 | Rose 28955 | Supporting service |
| Zuzon 28937 | Rose 28955 | Shining golden helm 28956 | Independent achievement |

The dracolich 28916 loads beneath Torg at 28902 on a 35% reset roll; its raw
hide has its own 33% G roll and live cap of one. A present dracolich need not
carry hide. The hidden trapdoor route has an ordinary key-zero door; learning
the route is not equivalent to the curing receipt. Kordor's `dracolich` and
`rare` responses explain his material preference. The jeweler follows Zarina
28934 from 29073; the actual F command makes him the parent for subsequent G
commands. A scanner that associates every G with the last M would misidentify
his possessions as Zarina's.

The illithid 28901 at the newly dug tunnel's end, 28927, has the tentacle G
source. The Keeper's `crime` and `tentacle` responses lead to the favor exchange.
His obsidian door uses key 28918, carried by the Keeper at 29026; the token is
not a door key. The forward face 28918 north → 28925 names key 28918, while
the reverse face names key 28910. Shared unlock updates the reciprocal face;
do not infer two required keys for one ordinary forward traversal.

The woman's `story` response establishes her relationship with Zuzon. The token
exchange describes an illusion/concealment device, but the rose is ITEM_OTHER
with no reviewed special or encoded invisibility/access effect. Her service and
Zuzon's `lover` response explain the secret delivery; they do not implement an
escort, meeting, lover-state change or accepted-topic achievement.

## Colosseum and the two promise rings

The north entrance from 29027 to 29028 is a locked key-`-2` speech door.
The final exit keyword is **carnage**: ordinary `say carnage` unlocks/reveals it
through the shared magic-door handler; opening is a separate action. This is
not a native M keyword achievement. A nearby golem's scrap is a clue, not a
receipt or a material required by the gate.

The Colosseum Champion 28926 at 29043 wears laurel medallion 28930. The
medallion also unlocks the pickproof north gate from 29033 to 29062, then the
upper route leads to Zillra at 29064. Zillra 28929 accepts the medallion for
sculpted tree 28932. Use the medallion for access before consuming it in Q.
This delivery does not require a registered arena victory. Weekly games,
spectator approval and group/race references in room prose have no reviewed
custom admission handler.

Two identical names conceal different required prototypes:

| Exact item | Ordinary source | Required use |
| --- | --- | --- |
| Promise ring 28916 | P inside handsome corpse 28915, O at blood-stained corner 28919 | Together with 28938 |
| Promise ring 28938 | G on Zillra 28929 at 29064 | Together with 28916 |

Zillra's `ring`/`forgotten` response families beg the player to destroy the
rings and avoid giving them to her or Zojt. Councilor Zojt 28932 instead accepts
both distinct rings for Swiftblade 28936. Destroying either ring is not another
native Q; the journal must describe the moral choice without inventing a
supported destruction ending. Zojt's threats in reward prose do not initiate
another Q or attack through that Q body. Ordinary hostile NPC behavior is
separate. Two copies of one ring cannot substitute for the other prototype.

The pools at 29064 and 29065 are generic ITEM_TELEPORT objects 28933/28934,
linking the rooms; their council lore does not establish a membership gate.
The third pool, 28952 at Tagnor's 29081, leads to 29117 and the hidden western
return toward 29027. Ordinary D reset state determines whether that return
needs opening; a secret-looking description alone is not a live secret flag.

## The Lore Keeper's four independent commissions

The eight-legend delivery accepts exactly one of each relic 28944–28951 in a
single exchange. Its reward is shimmering curtain of fire 28954 plus Legend of
The Lore Keeper 55363. The eight distinct roots fit the durable offering limit
of eight; eight arbitrary relics or eight copies of one kind do not suffice.

| Relic | Source mobile | Reset equipment/inventory |
| --- | --- | --- |
| 28944 | LLznixor 28948 | E, slot 24 |
| 28945 | Dorn 28950 | E, slot 6 |
| 28946 | Tibor 28951 | G |
| 28947 | Elirandel 28949 | G |
| 28948 | Jonxin 28952 | G |
| 28949 | Smurp 28954 | E, slot 16 |
| 28950 | Kulnon 28955 | E, slot 9 |
| 28951 | Lanom 28957 | E, slot 3 |

All eight M resets load into 29061 at 100% with a live mobile cap of one;
their own G/E item commands are also 100%, cap one. The Keeper's `legends`
response describes elusive champions and repeated visits, but no weekly timer
or sequential arena-spawn controller is encoded here. Generic wandering permits
these non-sentinel mobiles to move up into the arena at 29048 or east/west into
29063. **Room 29063 has no exits.** Neither destination has ROOM_NO_MOB, so the
one-way holding path can leave a live cap-one source unreachable. That is a
placement/availability concern, not evidence that the relic prototype lacks a
reset source. Reproduce this path and review return/barrier/placement options
with the builder, retaining intentional variation where appropriate.

The `tranug` response names a separate adversary and three separate contracts:

| Offering from Tranug 29000 | Lore Keeper reward |
| --- | --- |
| Vecna's cloth 28983 | Blazesword 28986 |
| Bahamut's scaled strip 28984 | Firestone collar 28987 |
| Prodigy's severed hand 28985 | Multicolored-bead cloak 28988 |

Tranug loads at 29123 with three **independent 50% G rolls**. They can produce
none, one, two or three relics; this is not an exclusive three-way branch.
Rooms 29124/29127/29128 provide routes upward to 28920, where the Keeper loads.
Tranug is not sentinel and may move. Each contract accepts its own item without
a receipt for the eight legends or evidence that the delivering player killed
Tranug. The source guidance therefore describes conditional availability and
exact proof without turning prose about defeating him into a durable kill.

## The legend's cross-zone continuation

Reward 55363 is defined in Winterhaven's [objects](../../../areas/obj/wh.obj)
and has the assigned `lorekeeper_scroll` handler in
[specs.winterhaven.c](../../../src/specs/specs.winterhaven.c). Its `recite legend`
route creates **one randomly selected half**, 55364 or 55365, of Uz's tale;
one scroll does not produce both halves. Both together go to Uz 87527 through
[Juiblex's Q](../../../areas/qst/juiblex.qst) for 500,000 coins and story of a
Living Legend 55279. That is an owned foreign terminal exchange. Personally
collecting both random results may require repeated legend deliveries; trading
for the missing half remains possible. The transform is not a second Torg
achievement and possession of a half does not prove a particular Torg attempt.

The current scroll handler prints success, extracts its original input and
directly publishes the randomly chosen object without confirming durable grant
success. A committed transform must bind the exact selected input UID and
generation, freeze the one-of-two choice, validate output and delivery, then
retire the input only as part of the recoverable outcome. Reject/abort, duplicate
dispatch, lost actor, restart and recipient changes need defined behavior.
Do not redraw the random choice on recovery or grant both halves accidentally.
The current direct code does not supply this lineage or qualification.

## Dwarven invasion, locket and equipment

The path into the city/mines uses key 28901 carried by rough rakshasa commanders
28908 at 28964/28977. Further gates use the ogre lieutenant's petrified-tree key
28909 and shaman commander's key 28910. Tagnor's commander 28938 at 29080 carries
glowing key 28939 for the locked north door into 29081. Inside, Tagnor 28941
carries the wedding locket 28940 and skull-tipped spear 28953; Zravi 28942 carries
unholy divine chisel 28982. The spear unlocks the granite gate from 29090 to
29091 toward Timoro and Torg. Torg's mithril key 28999 leads from 29093 to
hidden chamber 29181. These executable gates supply access leads, not new Qs.

The death handler for Timoro 28961 looks for the first live Tibornor 28975 in
29116 and commands him `down`, toward 29092. Reset F commands place Dranar
28974, scouts/ragers and equipment master **29024** with Tibornor there. Shared
movement follows eligible members from the original room after the leader's
move; combat, position, visibility and each follower's own movement can affect
arrival. The initial room has no ordinary incoming player route. Consequently,
possessing a locket or buckle materials does not by itself make these recipients
reachable. The journal explains the invasion rather than fabricating a
possession-based availability check.

`timoro_die` prints the invasion before verifying the leader or movement and
dereferences `world[real_room(29116)]` without a missing-room guard. Its death
callback receives the killer but does not use it; no durable actor/episode event
confirms the full party arrived. It runs on Timoro's death, not necessarily a
personal kill by the player who later delivers proof. Plan a validated room,
exact leader/member generations, confirmed post-movement result and replay
policy before making invasion or liberation a historical objective. Keep partial
arrival distinct from full-force arrival and decide whether partial arrival is
intentional. Do not require killing Torg for Qs that currently accept items.

| Recipient | Exact offering | Result | Admission detail |
| --- | --- | --- | --- |
| Dranar 28974 | Tagnor's wedding locket 28940 | Blood-doused obsidian 28962 | `note` explains vengeance; no Torg-death check |
| Master 29023 **or** 29024 | Fine chisel 28959 | Obsidian hoop earring 28981 | One accomplishment, two native alternatives |
| Master **29024 only** | Obsidian 28962 **and** unholy chisel 28982 | Rage buckle 28973 | Separate from the fine-chisel commission |

The jeweler 28936 has a 20% G roll for fine chisel 28959 after his F reset under
Zarina. Fine chisel and Zravi's unholy chisel are distinct items, not successive
versions automatically upgraded by the first commission. The alternate master
29023 has no active ordinary reset found anywhere; 28994 is an unused empty
master placeholder. Preserve the valid alternative contract and history, but
direct players to the live invasion master 29024. Dranar's locket receipt is
optional preparation for supplied obsidian; neither chisel commission proves
personal acquisition, the invasion's cause, liberation or Torg's death.

## Lanella's heart and other owned foreign leads

Lanella 29025 loads at 29178. Her `lanella_heart` death special rolls exactly
one of intact heart **28997** and broken heart **28998**, validates the selected
prototype, then directly creates it on the room floor. The delivering player
is not recorded as its killer or first discoverer. These are custom births,
with no reset grant or durable source identity. Supplied hearts are accepted
by their actual foreign contracts but cannot establish personal kill credit.

Sierra 55125 in [Winterhaven's Q](../../../areas/qst/wh.qst) accepts intact
heart 28997 for two Wrath elixirs 55367, or broken heart 28998 for two Truth
elixirs 55366. Her request supplies the revenge context. Neither contract
belongs to Torg's achievement denominator. Lanella can wander through either
arena-side route or down into the same exitless 29063 holding room; availability
needs the same live-cap review as the legends.

The heart handler assigns a duration to value[0], but these ITEM_OTHER hearts
have no reviewed timer-consuming procedure or decay affect initialized by that
handler. `dragon_heart_decay` is assigned to different dragon-heart prototypes.
A duration field alone does not qualify an expiration mechanic. Confirm builder
intent, then implement a committed birth and any intended deadline/expiry with
exact UID, random branch, placement and restart semantics. Do not promise a
working one-day expiry from this field, or silently borrow another heart's rules.

Zuzon carries head 28980, used outside Torg by Tallin's demonic advisor 71236 in
[Scorch Valley's Q](../../../areas/qst/scorchvalley.qst), together with 71227
and 71009 for armor 71226. This is another owned foreign contract, not a local
Zuzon quest stage. No foreign reset for the inspected Torg quest carriers or
material prototypes was found in the active area list; bounded foreign Q links
remain distinct from producer ownership and current location.

## Services, unused content and fair repair decisions

Tibornor also teaches **Indomitable Rage** through the epic teacher table. Its
level/class/prerequisite/cost rules belong to that service; active purchases are
deliberately refused until their adapter is qualified. Teaching is not a local
Q and is not a liberation reward. The inn assigned to escape-tunnel room 29103
uses the shared terminal-save ordering already reviewed. The unusual placement
should be confirmed with the builder before moving or removing it.

Kordor's shop is assigned during `boot_db`/`assign_mobiles`; normal
`initialize_tradeskills` runs later and installs `smith` only when no mobile
function exists. His populated shop function therefore leaves the ten-recipe
smith table unattached; the shop has no earlier smith secondary to capture.
This is a source-level reachability finding, not an observed player failure.
If the forge is intended, attach it deliberately through compatible shop/secondary
dispatch and qualify the complete menu. The shared legacy smith body also tests
a numeric choice against the smith-table index and searches the smith's own
inventory for materials; review those paths before enabling this menu. Its
explicit active-accounting refusal is an appropriate guard, not evidence of
a successful active forge or a reason to remove that guard prematurely.

The hidden Gilaxi altar's berserker procedure is commented out. Its separate
handler contains permanent character changes and requirements, but room prose
alone does not activate it. Mine beams, surveillance, council ceremonies, weekly
games, lover concealment and liberation language likewise have no additional
reviewed quest completion. Builders should choose retained flavor, clarified
prose or explicit supported mechanics. No class conversion, timer, killing
requirement, source cap or economic repair is silently enabled by this journal.

## Universal capability additions and qualification

| Gap | Required capability / repair plan | Qualification before gameplay claims |
| --- | --- | --- |
| Fresh source availability | Shared committed resets for O/P/G/E, with F as actual NPC parent and exact occurrence/cap/equipment/custody | Fresh and recovered stock, F-parent supply, rare independent rolls, cap/restart/replay |
| Access and learned lore | Shared exit/key/password and confirmed travel events; accepted-topic evidence; staged clue/solution policy | Spoken unlock versus open versus arrival; supplied key; two-ring identities; no read-only history mutation |
| Holding routes | Review 29061/29178 → 29063 availability and intended return/barrier/placement | Live cap-one wandering, inaccessible survivors, ordinary versus forced reset, intentional rarity |
| Invasion | Guard destination/leader; confirm exact follower outcomes and actor/episode semantics | Missing room/leader, failed movement, fighting or missing follower, partial arrival, repeated death, restart |
| Legend transform | Commit exact scroll retirement and one frozen random-half birth with recoverable continuation | Invalid target/output, rejected grant, duplicate call, crash/replay, changed recipient, both foreign halves |
| Lanella birth / duration | Commit actor-aware one-of-two heart grant; decide and implement intended expiry policy | Both random branches, missing prototype, failed publication, floor/pickup/gift, retry/restart, deadline if authored |
| Native campaigns / foreign links | Explicit all-stage policy and owned terminal references, keeping current delivery IDs | Supplied materials, independent order, services excluded, master alternatives one achievement, foreign receipts not double-counted |
| Forge / unused altar | Builder decision plus compatible dispatch and guarded service/class adapters | Normal boot reachability, exact materials/menu, rejection/recovery; permanent conversion only after explicit reviewed design |

Journals, discovery, encounters, new achievements and zone dailies continue to
require **actual active accounting and readiness**. A feature switch cannot
bypass that requirement. This source map neither activates accounting nor
qualifies direct script grants. Readable contact descriptions and optional
preparation are deployable today; durable semantic events, staged client
projection and the above played/recovery cases remain implementation work.

Source-comprehensive means each inspected local quest and quest-like interaction
is accounted for here. It does not mean all branches are gameplay-qualified.
The next source review remains Vast Hidden Grove (Solonar), followed by
Winterhaven, preserving the original [priority queue](../ZONE_STORY_ZONE_PRIORITIES.md).
