# Pine Hollow: comprehensive source story map

Reviewed October 2, 2026. Source area `pineholl`, zone 160, journal revision 2.
The complete local source has 166 rooms, 88 mobile prototypes, 86 objects,
seven Q contracts, ten addressable M responses and 352 reset commands.
Both dragon exchanges and all five named clothing commissions remain separate
achievements. All native bindings and categories are retained, without exclusions.

## Evidence boundary

Reviewed the complete active [Q/M file](../../../areas/qst/pineholl.qst),
[rooms](../../../areas/wld/pineholl.wld), [mobiles](../../../areas/mob/pineholl.mob),
[objects](../../../areas/obj/pineholl.obj) and [resets](../../../areas/zon/pineholl.zon).
The [reproducible audit index](../../reference/zone-story-audits/pineholl.md)
records exact terms, dialogue locations, reset counts and literal assignments.
There is no local shop file or assigned local procedure in the literal assignment
and named-procedure scan. Computed assignments remain a manual extractor limit.

Reviewed shared ordinary keys/locks and movement in
[actmove.c](../../../src/cmd/actmove.c), liquid handling in
[actobj.c](../../../src/cmd/actobj.c), data-driven hazards in
[trap.c](../../../src/combat/trap.c), Q reward/disappearance in
[quest.c](../../../src/world/quest.c), and reset/exit loading and follower
generation in [db.c](../../../src/world/db.c). Mobile wandering in
[mobact.c](../../../src/mob/mobact.c) and [flags](../../../src/core/defines.h)
explain the foreign source routes. Also inspected the six foreign exits from
the quest load rooms and the three external consumers of Pine Hollow items.
Those bounded dependency reads do not qualify the other zones comprehensively.

## Progression: hear the opposing dragon accounts

The cruel warrior 16005 begins at wooded valley room 16081. His addressable
families are `search`/`searching`/`hunt`/`hunting`, `lance`, `auriam`, and
`children`/`dragons`. Auriam 16006 begins at wooded valley room 16077;
`scar`/`scars`/`battle` explains her wound, the dark warrior and Clamphorl.
These five responses print the conflict and requests without recording learned
topics, alignment choice, kills, first acquisition or an ordered investigation.
The journal supplies one useful representative per family.

The warrior's child-name prose says **Atrucali**; the actual mobile is
**Altrucali 16086**, with keyword `altrucali`. The journal uses the verified
identity. His reference to a smith repairing the lance has no separate local
repair Q, shop service or assigned procedure. Deliver the broken lance as the
native request specifies. The final rewards do not include a repaired lance.
Review the spelling and repair narrative through a content decision rather
than silently changing native receipt terms or inventing an intermediate goal.

Both valley encounters have ordinary local forest routes. The later mine and
duergar refuge do not provide the dragon children or Clamphorl. Treating that
exploration as a mandatory prerequisite would give players an incorrect route.

## Progression: help Auriam

Offer one **black sword 16013**, one **full suit of black platemail 16014** and
one **totem of dark spirits 16080**. The warrior is equipped with the sword
and platemail in resets 267 and 266. Clamphorl carries the totem in reset 445.
The inputs must be carried for the offering; wearing the platemail is not the
same current inventory state as carrying it for trade.

Auriam awards **gold dragon scale 16015** and **gold dragon horn 16075**, then
the native D outcome removes her. Her remaining equipment/inventory is extracted
by the shared disappearance path. In particular, her reset-held **broken
mithral lance 16016** is not an extra quest reward and should be secured first
if another route needs it. No saved earlier conversation, personal kill, racial
membership or local source history is required by the native Q.

This is an independent delivery achievement. It does not prove who defeated
the warrior or shaman, even when the dialogue describes their death. Supplied
valid equipment works. The scale reward is a real material link to the warrior's
request, but not evidence that Auriam was killed or that the other three inputs
have been obtained.

## Progression: fulfill the warrior's trophy demand

Offer one each of **gold dragon scale 16015**, **broken mithral lance 16016**,
**green-hued scale 16076** and **brown-hued scale 16077**. Auriam carries the first
two through resets 260/259. Nuriem 16085 carries the green scale through reset
440; Altrucali 16086 carries the brown scale through reset 443. The three scales
have distinct types even though their aliases overlap. Auriam's own reward
also creates a valid gold scale; gifts and existing valid stock are accepted.

The warrior awards **dragonscale earring 16082**, **golden dagger 16081**,
**dragonscale collar 16084**, **sword of the blessed 16085** and **gold dragon
scale 16015**, then disappears. The nightmare/mount departure is narrated;
there is no encoded mount reward or separate riding objective. The offered
gold scale is consumed and a new scale is generated as a reward. Same VNUM
does not establish the same UID or uninterrupted custody.

The exchanges accept their inputs independently. They do not save a mutually
exclusive faction decision. They can still compete for availability: completing
Auriam's exchange removes her remaining lance, and completing the warrior's
exchange removes his remaining equipment. Prior recovery, supplied stock or
another valid NPC/reset episode may allow both. A future branch model must
explicitly choose allegiance, irreversible choice and attempt rules; it cannot
derive them from an item delivery, alignment-themed prose or one receipt.

## Roaming sources and a holding-room availability concern

Nuriem, Altrucali and Clamphorl all start at **load room 16161** in resets
438/441/444. Their action flags include hunter and memory, without sentinel,
stay-zone or patrol. Ordinary wandering can enter the connected load rooms
16162–16164 and leave into other areas. Their exits lead to:

| Load room | Foreign destination / prototype owner |
| --- | --- |
| 16162 | Twisted Wood sharp-grass field 16308; Twin Towers garden 13595 |
| 16163 | Breale main road 2604; Twisted Wood interior 16344 |
| 16164 | Asylum muddy path 9287; Mistywood Kromor Tower roof 95048 |

All destinations exist, have ground sectors and do not set `ROOM_NO_MOB`.
These are executable source leads, not guaranteed present locations. No normal
local entrance leads from the mine or valley into these load rooms. Keep the
source/reset owner separate from current room, encounter and NPC generation.

There is also a downward exit from 16161 into **holding room 16165**, which
has no outgoing exits and does not block ordinary mobile entry. This permits
a wandering source to enter a dead end. The inspected ordinary wander path
offers no return exit; additional combat/AI behavior and deployed placement
remain unqualified. This is a source-supported availability concern, not proof
that every live source is currently stranded. Confirm intended use of the
holding room, reproduce the idle path, then choose an explicit mobile barrier,
return route or spawn relocation as a reviewed content repair. Account for
existing NPC episodes; do not fabricate a currently obtainable trophy.

## Progression: Darlene's five clothing commissions

Darlene 16080 begins at **Trading Post 16110**, west of village road 16109.
The room's `sign` extra description lists her goods and says she trades rather
than sells. Her five M families explain the exact requests. Each exchange
consumes the required material roots together, awards one garment, has **no
coin fee**, and leaves Darlene present. No tanning service, crafting skill,
first garment or earlier clothing receipt is an admission requirement.

| Topic | Exact carried materials | Reward | Active source leads |
| --- | --- | --- | --- |
| `jacket` | Three brown bear skins 16019 | Jacket 16048 | Brown bear 16019; six equipped-skin resets across forest rooms 16000, 16001, 16025, 16027, 16039, 16051 |
| `leggings` / `legging` | Three black bear skins 16020 | Leggings 16049 | Black bear 16020 at 16023, 16029, 16036; equipped skins |
| `coat` | Two huge bear skins 16021 | Heavy coat 16050 | Large black bear 16021 at 16040; one carried-skin reset |
| `boots` / `boot` | Three badger pelts 16024 | Boots 16051 | Badger 16024 at 16018, 16026, 16044, 16048 |
| `stole` | Three red fox pelts 16025 | Stole 16052 | Red fox 16026 at 16074, 16079, 16090, 16093 |

The skin and bear prototypes share some numbers; their roles remain distinct.
Native repeated G entries require distinct input objects, not repeated use of
one UID. Gifts satisfy delivery without proving personal hunts or skinning.
These five named end commissions retain their existing request achievements;
they are not silently reclassified into preparation services.

The **coat supply has a confirmed count/limit conflict**: the only active
16021 source is `G 1 16021 1 ...` after a single large bear with mobile limit
one. Ordinary shared G resets admit a new item only below the live prototype
count limit unless `force_item_repop` bypasses it. Holding the first live skin
therefore suppresses the ordinary second skin, while the Q needs two together.
No alternative active reset or Q producer was found. Forced repopulation,
recovered stock or other historical circumstances can yield multiple inputs;
this finding does not establish universal impossibility in an existing world.
After reset generation is qualified, reproduce fresh-world accumulation and
review raising the skin cap to at least two or adding an intended source.
Preserve the two-skin contract until that balance decision is made. The journal
states the exact demand and asks players to confirm a supply of both skins.

## Progression: uncover the village's mine and duergar refuge

This is a coherent exploration story, with one real foreign quest material.
It has no local Q rescue or liberation terminal. Heavy green key **16060**
opens the locked/pickproof north mine door **16114 ↔ 16116**. Its reset carriers
are the guard captain 16060 at 16113, village guard 16061 at 16114, fat guard
16063 at 16115 and burly guard 16064 at 16116. Dull iron key **16061** opens
the locked/pickproof smelter door **16122 ↔ 16123**; the captain and hefty
mercenary 16065 at 16122 carry it. Identical `key` aliases do not identify
the target, and smelter access does not open the mine.

The down route from mine entrance 16116 reaches the brightly lit slave cavern.
Three concealed, reset-closed but unlocked passages lead onward: down through
`rubble` at 16143, down through `rocks` at 16153, then east through `wall secret`
at 16155. These use D state 5. Discover/open the passages rather than inventing
a password, third key or required kill count. The westbound sides are ordinary
closed exits. The refuge contains guards, escaped slaves, a blacksmith, maps,
guard schedules and a vengeful clan leader. The leader's stone door at 16159
is closed/unlocked, with key zero. Maps/schedules are room lore without a
durable accepted-examination or rescue event.

Nine F commands attach slave/guard followers to overseer M loads, group them
and add sentinel through the shared reset code. Chain equipment, follower
relationships, death and charm are ordinary game state; they do not define a
player-owned release, safe escort, destination, faction reward or persistent
liberation. A builder can design such a story, but needs an explicit participant,
NPC generation, liberation condition, hostile/allied outcome and final policy.

Two actual property-driven hazards matter. **Trap 16062** is reset in entrance
16116 with effect 261: down-movement plus room-wide fire damage, one charge,
level 20. The movement check applies the trap and returns before movement;
triggering it is not successful descent. **Stone desk 16072** in leader room
16160 is closed/locked, key zero, not pickproof, with a room-wide acid-on-open
trap (effect 516, one charge, level 30). Its child P reset contains **false
marble eye 16071**. Opening and surviving is distinct from retrieving the eye.
Prototype **spear trap 16073** has no active reset/grant found in the source scan;
do not advertise it as a confirmed live obstacle.

The eye is accepted by **giver 4038 in Caves of Skelenak** for **100,000 coins**,
with no D outcome in that contract. His eye/help dialogue explains the need.
That is a real cross-zone continuation owned by
[caves_skelenak.qst](../../../areas/qst/caves_skelenak.qst), not a new local
achievement or proof of restored sight. Current fallback preserves the foreign
receipt; future linked journals need reveal/ownership and accepted access/source
events without double-counting it in Pine Hollow.

## Other cross-zone consumers and lore closure

The [Twisted Wood faerie guard](../../../areas/qst/twstwd.qst) 16311 accepts
**black platemail 16014 + broken lance 16016** for **token 16313**, without D.
His guard/dark-man/Auriam topic families explain the connection. Auriam's own
Q does not require that token or receipt, even though the faerie suggests a
later reward. The faerie's offering competes for the armor Auriam needs and the
lance the warrior needs; it is not a mandatory preparation recipe.

[Ice Crag Masha](../../../areas/qst/icecrag.qst) 97006 uses one **fox pelt 16025**
alongside six other inputs (15252, 315, 303, 11519, 6551, 66058) for item 97110
and three copies of 97136. Keep that seven-input foreign request owned there;
a pelt supplied to Darlene cannot also be spent in Masha's same exact transaction.

The ancient fountain 16000, static doe/sparrow corpses 16001/16002, ruined
logger corpse/broken axe 16003/16004, animated trees, castle-like illusion by
the pool, village herb garden and duergar smith describe the wider setting.
No local Q or assigned procedure makes investigating the corpses, dispelling
the castle, gathering a herb, repairing the axe or forging blue steel a tracked
terminal. Fountain values encode infinite ordinary water with poison value
zero; the ominous prose and nearby static deaths do not execute a poison/death
or corpse-creation lifecycle. Those are candidates for a builder-authored
investigation, not proof of an implemented cure or ritual. Unloaded ordinary
loot prototypes 16074/16083 need content intent before becoming promised rewards.

## Blockers, proposed improvements and balanced repairs

| Finding | Evidence / impact | Planned action |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | 163 M, 95 E, 43 G, 28 D, 11 O, nine F and three P commands; all local quest materials depend on item resets. | Qualify active generation, exact reset/NPC episodes, equipment and nested scenery. Preserve recovered stock; no fresh-world qualification is claimed. |
| ZSQ-ROAMING-SOURCE / HOLDING-ROOM | Three trophy carriers wander from off-route load rooms toward six foreign exits and a one-way holding room. | Project current source location separately from owner; reproduce the holding path and choose a reviewed return/barrier/placement repair. Qualify idle, fighting, absent and renewed episodes. |
| ZSQ-RESET-SUPPLY-CAP | Coat needs two live huge skins; only producer's ordinary item cap is one. | Reproduce accumulation after generation support; review cap/source change with builders. Test exact count, live stock, forced repop and recovery without weakening the quest implicitly. |
| ZSQ-BRANCH-AVAILABILITY / COMPETING-CONSUMERS | Independent dragon Qs remove NPC inventory; faerie also consumes armor/lance, and Masha consumes fox pelt. | Author optional routes and explicit branch/attempt policy if desired. Commit exact custody/consumption and NPC disappearance once; no delivery implies a faction or personal kill. |
| ZSQ-CROSS-ZONE-OWNERSHIP / ACCESS-HAZARDS | Hidden duergar desk provides Skelenak eye; key targets and trap effects are shared data mechanics. | Add owned links, current access and confirmed movement/open/retrieval events; retain hostile hazards and distinguish blocked move from arrival. |
| ZSQ-PINE-NARRATIVE / UNINTEGRATED-LIBERATION | Altrucali spelling differs; lance repair/mount, duergar liberation, fountain deaths and illusion have no corresponding local terminal. | Clarify verified names now; builders choose prose clarification or explicit committed objectives. Preserve native contracts, rewards and existing hostile behavior until intent is reviewed. |

## Qualification matrix

Under active accounting, qualify both complete/wrong/supplied dragon offerings;
each distinct scale and sword/armor type; source versus gifted recovery; both
completion orders and their cleanup; already absent and replacement NPC episodes;
same-type scale replacement; the faerie's competing armor/lance and Masha's pelt;
all five correct/wrong/insufficient skin recipes with exact roots; two-skin
fresh-world accumulation; repeated commissions; wandering sources and holding
entry; both keys, already opened access, all three hidden passages; trap-triggered
failed descent versus successful later arrival; desk opening versus eye recovery;
Skelenak's own coin reward and recipient policy; interruption, retry and two cold
restarts. Reads must create no learned topic, hunt, rescue, faction or source credit.

Focused native tests cover independent supplied dragon receipts, exact live skin
counts/types, next actions and recovery. They do not execute NPC wandering,
item generation, trap effects or the foreign eye journey. This source-comprehensive
map therefore remains separate from active-world gameplay qualification.
