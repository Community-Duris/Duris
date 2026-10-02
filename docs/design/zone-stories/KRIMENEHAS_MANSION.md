# Lord Krimeneha's Mansion: comprehensive source story map

Reviewed October 2, 2026. Source area `krimman`, zone 164, journal revision 2.
The complete local source has 90 rooms, 34 mobile prototypes, 65 objects,
nine Q contracts, nine addressable M responses and 176 reset commands.
Eight rescues/final requests remain achievements; preparing a staff fragment
is one supporting service. All native bindings are retained, without exclusions.

## Evidence boundary

Reviewed the complete active [Q/M file](../../../areas/qst/krimman.qst),
[rooms](../../../areas/wld/krimman.wld), [mobiles](../../../areas/mob/krimman.mob),
[objects](../../../areas/obj/krimman.obj) and [resets](../../../areas/zon/krimman.zon).
The [reproducible audit index](../../reference/zone-story-audits/krimman.md)
records exact native terms and response locations. There is no local shop file
and no local procedure found in the literal assignment and named-procedure
scan of [specs.assign.c](../../../src/specs/specs.assign.c) and `src/specs/`.
Computed assignments remain a manual audit limitation of the inventory tool.

Reviewed shared Q disappearance/reward semantics, ordinary key/lock handling
in [actmove.c](../../../src/cmd/actmove.c), reset/exit loading, mobile wandering
in [mobact.c](../../../src/mob/mobact.c) and relevant flag definitions.
Inspected Eckraldu's three foreign first-hop destinations and the competing
Quietus staff contract. These dependency reads are not comprehensive dossiers
for those other areas.

## Progression: learn what happened to the garden

The younger gardener 16426 starts at indoor fountain sitting room 16462. `pool`, `monster` and
`lord` explain the creatures emerging from the poisoned pool. The elderly
gardener 16427 starts at overgrown garden 16423. His `lord` response identifies
Eckraldu as the poisoner and summoner. Both prototypes share the alias
`gardener` and the short description "a gardener's ghost"; journal labels now
distinguish their ages and starting locations. Starting placement is not a
promise of current location or an exact live NPC identity.

Lord Krimeneha's ghost 16422 starts at office 16459. `pool` explains his
suspicions; `eckraldu` identifies the needed staff. Crying ghost 16421 starts
upstairs at 16475 and accepts the `crying`/`tears`/`sob`, `leave`/`exit` and
`lord`/`krimeneha` topic families. The journal displays one usable representative
per family. These nine responses print lore; they do not enforce an ordered
investigation or save learned topics. Staff and keepsake offerings work without
earlier conversations. No keyword becomes an achievement merely by being asked.

The courtyard, garden monsters and dark pool explain the disaster and support
exploration. The mentioned gate to hell is narrative origin, not an assigned
portal objective or a required demon-kill counter in this area's source.

## Progression: obtain and prepare the staff

Eckraldu 16423 is equipped with **staff 16450** in reset 315. His spawn room
16486 is a load room with no normal local incoming route. Its outgoing exits
lead to Mistywood hill 95100, Twin Towers garden 13595 and Woodseer private
quarters 16660. His action flags include hunter, memory and scavenging;
they do not include sentinel, patrol or stay-zone. The shared ordinary wander
handler permits him to leave through allowed exits into other zones. All
three first-hop rooms exist and do not set `ROOM_NO_MOB`.

This is a real source route with a roaming adversary, not a confirmed missing
staff. It does not justify instructing players to walk into the load room or
guaranteeing they can find him in the mansion. Qualify the live spawn episode,
movement, hostility/access and recovery route before promising a local journey.
Cross-area tracking should keep his prototype/source owner separate from his
current room and encounter generation.

One staff offered to Lord Krimeneha produces **one fragment 16451**, despite
the prose describing several pieces. This preparation service leaves the lord
present. No fee, extra fragments, prerequisite kill, family receipt or learned
topic is encoded. The staff is also accepted by giver 1736 in
[Quietus](../../../areas/qst/quietus.qst) for cloak 1751. That is a competing
consumer, not an alternative way to create a rescue fragment. Custody and
consumption must prevent one exact staff from completing both transactions;
another valid staff or gifted fragment may support a separate route.

## Progression: free the sane spirits

Each rescue accepts one fragment. Each supplies another fragment and removes
its own giver through the native D outcome. A single fragment type can therefore
continue the sequence, but the Q engine consumes the input and creates a new
reward object. **Same VNUM does not mean the same UID was returned.** Future
lineage must record exact retired input, new output and NPC episode, including
the rescues with no additional tangible reward.

| Spirit / starting room | Offering | Additional native reward | Meaning |
| --- | --- | --- | --- |
| Lady 16419 / bedroom 16484 | Fragment 16451 | Silver broach 16454 | One family keepsake and actual giver disappearance |
| Serving girl 16420 / wide hallway 16450 | Fragment 16451 | None | Complete rescue, even though the only item award is another fragment |
| Crying ghost 16421 / upstairs 16475 | Fragment 16451 | Elegant key 16436 | Rescue plus access to the Lady's rooms |
| Boy 16424 / bedroom 16476 | Fragment 16451 | Signet ring 16452 | One family keepsake |
| Young girl 16425 / bedroom 16478 | Fragment 16451 | Broach 16453 | One family keepsake |
| Younger gardener 16426 / fountain sitting room 16462 | Fragment 16451 | None | Complete rescue, with no separate keepsake |
| Elderly gardener 16427 / garden 16423 | Fragment 16451 | Metal band key 16422 | Rescue plus pool treasure access |

Serving-girl/younger-gardener exchanges are not wrong-input returns or
briefing-only services. Conversely, non-addressable mad ghost prototypes
16428–16431 have no equivalent rescue contract. Do not claim that every ghost,
servant or guest in the mansion can be released through this recipe.

The crying ghost's elegant key is a useful earlier route to the Lady, but a
supplied key, another source, or an accessible door can work. Journal revision
2 makes staff preparation optional and explains exact access and rewards.
It adds no new native admission rule or personal-source condition.

## Progression: release the lord and use his reward

The final offering is exactly **one boy's ring 16452, one girl's broach 16453
and one Lady's silver broach 16454**. Lord Krimeneha supplies **heavy key 16445
and silver bastard sword 16455**, then disappears. Other servants' rescues
are encouraged but are not required inputs and do not trigger another encoded
blessing/reward. A gifted set of keepsakes can complete this exchange without
the completer's three family rescue receipts.

The final journal shows optional office access, staff preparation and the three
earlier family exchanges, followed by required live keepsake counts and the
lord's terminal delivery. Completion remains that single final receipt;
it does not retroactively award all seven rescue achievements. A future
"personally released the whole household" campaign requires an explicit
all-stage policy, with separate treatment of repeat reset episodes and gifts.

## Access, keys and secondary exploration

| Access / source | Exact use and limitation |
| --- | --- |
| Steel key 16427: crying ghost reset inventory; also mad guest 16431 in load room 16489 | Locked, pickproof office door from library 16458 to 16459. Its source is equipment/inventory, not the crying ghost's Q key reward. No peaceful gift/purchase is encoded for that steel key. |
| Elegant key 16436: crying ghost Q; also insane ghost 16430 reset inventory | Locked, pickproof upstairs east door 16480 → 16482, on the route to Lady 16484. Distinct from the steel office key. |
| Metal band key 16422: elderly gardener Q; also mutating ogre 16412 inventory at 16427 | Locked, pickproof stone chest 16421 in underwater treasure room 16445. Pool exploration, water access and secret exit 16444 → 16445 remain separate. |
| Heavy key 16445: lord's final Q | Locked, pickproof iron vault **object 16444** in vault room 16481. It does not unlock the separate secret room door 16480 → 16481, which uses key 0. |
| Wooden key 16441: Lady's reset inventory | Wooden box 16440 in her bedroom; pearls 16442 inside are ordinary treasure. Rescue does not guarantee recovery of her prior inventory as a Q reward. |
| Dull iron key 16433: wandering kenku 16432 at 16430 | Wooden chest 16432 at guest room 16461. Another dull iron key 16405 exists as loose object at 16409; matching aliases do not establish the same target. |

The outer gate, children's bedroom doors and several ordinary inner doors
are reset locked with key 0. `has_key` matches exact prototype numbers;
none of these maps to a named ordinary key here. Shared lock picking permits
nonnegative keys when the exit is not pickproof and the player has the skill,
held pick and suitable state. Hidden exits first need ordinary discovery.
This may be deliberate skill/party access. Document and qualify it; do not
invent a quest key or call it broken solely because key 0 is used.

The office desk 16428 has a trap record and a note; it is an exploration
hazard, not a Q prerequisite. Pool breathing equipment, cellar supplies,
pearls, garden treasure and vault loot use normal object/room mechanics.
No local Q records accepted clue examination, unlocking, opening a container,
traversing the pool, or recovering a specific personal treasure UID.

## Blockers, proposed improvements and balanced repairs

| Finding | Evidence / impact | Planned action |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | 48 D, 58 M, 31 O, 23 P, ten G and six E commands; fresh active item/scenery/nested sources lack generation authority. Recovered items may exist. | Qualify durable O/P/G/E generation and parent dependencies, including roaming NPC equipment and vault/pool containers. |
| ZSQ-REUSED-REWARD-LINEAGE | Seven rescues retire a fragment and publish another of the same prototype. | Record exact input/output lineage, recipient and NPC removal episode in committed outcomes; replay once. Same-item returns alone cannot classify stories. |
| ZSQ-ROAMING-SOURCE / COMPETING-CONSUMERS | Eckraldu can leave an isolated load room into three other areas; Quietus also consumes his staff. | Add source owner/current location/episode projection and safe competing consumption tests. Confirm intended roaming route before any world change. |
| ZSQ-KRIMENEHA-ALL-STAGES | Staff service, seven rescues and lord finale are distinct; prose suggests an additional servants' blessing with no encoded condition/reward. | Builder decides whether that line is flavor or an intended bonus. Add a separate all-stage household achievement only with explicit requirements; preserve independent receipts. |
| ZSQ-ACCESS-STATE / LEARNED-LORE | Exact key targets, keyless locks, two gardener identities and accepted lore are not historical objective kinds. | Use reviewed live access reasons and exact NPC episodes. Qualify accepted topics/examinations and lock/arrival events; do not infer them from visible names or possession. |

No native quest or world balance is changed in this mapping pass. The missing
servants' bonus and roaming/access intentions need a builder decision before
being labeled defects or repaired. The existing returning-fragment rescues are
meaningful supported outcomes.

## Qualification matrix

Require active accounting for all new discovery, encounter and quest evidence.
Exercise normal roaming staff recovery and the gifted-staff/fragment routes;
office-key and Lady-key sources; key 0 discovery/picking failures; each rescue,
including both fragment-only rewards; all three keepsakes and wrong/missing
inputs; supplied keepsakes without invented personal rescues; seven rescue
receipts versus the separate finale; two concurrent staff consumers; giver
disappearance/reset return; exact fragment UID replacement; pool/vault access
and container contents; interrupted publication and two cold restarts.

Native regression covers optional preparation, service exclusion, independent
family completion and durable receipt projection. It does not prove the live
roaming source, unsupported reset generation, historical access/lineage or
additional blessing. This dossier is source-comprehensive; gameplay
qualification remains pending.
