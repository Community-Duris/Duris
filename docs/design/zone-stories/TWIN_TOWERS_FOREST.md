# Twin Towers Forest: comprehensive source story map

Reviewed October 2, 2026. Source area `twin_towers_forest`, zone 135,
journal revision 3. This is a comprehensive map of the stories visible in the
current area sources and assigned custom procedures. It is not certification
that every route has been played with accounting active.

## Evidence boundary

Reviewed the complete [Q/M source](../../../areas/qst/twin_towers_forest.qst),
[reset](../../../areas/zon/twin_towers_forest.zon),
[rooms](../../../areas/wld/twin_towers_forest.wld),
[mobiles](../../../areas/mob/twin_towers_forest.mob),
[objects](../../../areas/obj/twin_towers_forest.obj), and
[three custom procedures](../../../src/specs/specs.twintowers.c).
The assignment chains attach `forest_animals` to nine mobiles, `forest_corpse`
to the nine corresponding objects, and `gardener_block` to twelve rooms.
There is no local shop source. The audit below accounts for all 84 distinct
native exchanges, 58 M response blocks, 345 reset commands, and 30 special
assignments. Greetings, refused lore, and rejected offerings are accounted for
without inventing achievements.

Shared execution was also inspected: [death dispatch](../../../src/combat/fight.c#L2211),
[reset admission](../../../src/world/db.c#L3291),
[object instantiation](../../../src/world/db.c#L3059), and
[player item publication](../../../src/world/handler.c#L1941).
The complete evidence export is reproducible:

```bash
python3 scripts/zone_story_quest_zone_inventory.py \
  --area-evidence twin_towers_forest --output bin/twin-towers-evidence.json
```

## Story families and actual prerequisites

### Flowers for Alvinar

Meet Alvinar (13500), learn his family's wishes through `flowers`, and optionally
ask `towers` for the garden route. Eight alternatives are accepted: orchid sprig
13553, rose plant 13555, chrysanthemum plant 13557, tulip bulb 13559, iris bulb
13561, sunflower sprout 13563, pansy seedling 13565, or daisy plant 13567.
Any one earns identify scroll 13572. The eight corresponding cut flowers are
returned with an explanation; those responses are not successful gardening.

The garden route requires belt 13521 **equipped at the waist**, not merely carried.
Tryve (13504) is reset in room 13556 with that belt at slot 13. No Q contract
or conversation awards it. The journal must say the gardener wears it rather
than promise a friendly belt exchange. The owner clarified on October 8, 2026
that mortal players have no active steal skill: the existing source route is
killing the gardener and looting the belt, and that challenge is intentional.
A supplied belt is equally valid for access and the intended achievement;
do not add a personal-kill requirement or a friendly award as a journal repair.

`gardener_block` guards these source-room/direction pairs:

| Direction | Source rooms |
| --- | --- |
| North | 13553, 13558, 13568, 13569 |
| South | 13553, 13556, 13564, 13569 |
| East | 13570, 13558, 13557, 13555, 13556, 13571 |
| West | 13570, 13568, 13567, 13565, 13564, 13571 |

This is a directional movement predicate, not a permanent unlock or a turn-in
prerequisite. A supplied valid plant can be delivered without a belt or prior
conversation. Revision 3 marks garden access as optional preparation: its live
equipment check remains visible, but `Next:` does not demand it before a valid
plant's exchange. The daisy plant reset is in room 13588, and the other seven
acceptable plant kinds are in the garden rooms. Do not infer that all eight
alternatives require exactly the same path.

Builder decision, October 8, 2026: the fuller flower achievement should record
obtaining belt 13521 by any accepted acquisition route, wearing it at the waist,
and the accepted plant delivery. Delivery remains valid before either belt
milestone. Acquisition and wear are retained historical facts, not requirements
to still possess or wear the belt when delivering; delivery-first completion is
also valid once the other facts are recorded. Each character must obtain and
wear it. A personal gardener kill, first original-source recovery or successful
garden crossing is not an additional required achievement objective.

Durable acquisition/wear events and separate aggregate achievement eligibility
remain implementation work; the current sidecar equipment check is live
preparation only. Learned dialogue, accepted guarded movement and personal plant
recovery remain possible optional extensions requiring their own reviewed rules.
A journal read or failed movement does not record an accomplished action.

### The archers' missing arrows

Farlindel (13501) explains `practice` → `sprites` → `arrows`; Hanson (13502)
also accepts the missing arrow. VNUM 13530 is the green-shafted, blue-feathered
arrow. Reset sources include loose arrows and nested piles of leaves, underbrush,
and a bird's nest. Containers are sources, not top-level carried offerings: the
player must recover the arrow into inventory before giving it.

Farlindel pays 300 copper, Hanson 200. Either native success completes the one
request. They also buy ordinary arrows 13569, 13570, and 13571 for 100 copper
each; these three services are independent of the recovery story. Neither
conversation nor a particular source is enforced by the turn-in. Gifts work.
Personal recovery needs a committed custody event, and finding an arrow does
not prove that the player defeated a sprite.

### Restore Glor-Linda's magic

Meet sprite 13520, then ask `crying`/`matter`, `wand`, `fairy`, and `magical`.
Bring dust 13582, one wand from 13583/13589/13591, and one each of feathers
13584, 13585, 13586, and 13587. Four copies of one feather are insufficient.
The three wand routes reward earrings 13581/13588/13590 respectively and
10,000 XP, then the sprite disappears. Her normal reset provides a later return;
the story does not permanently remove the NPC from the world.

Pixies 13521 carry the dust and wands; magical birds 13522–13525 carry their
respective feathers. The reset also places those supplies directly in room
13501. This matters: a journal must not claim every successful offering proves
combat or a pixie/bird source. The three terminal contracts are alternatives,
with original distinct rewards preserved. Lore and personal source recovery
may become separate optional objectives; they are not new native admission rules.

### Fresh game, tanning, and the family clothing work

Alvinar's `sons`/`furs`, Hanson's `hides`/`father`/`mother`, and Marja's
`sons`/`clothing` connect the family. These are guidance links. There is no
global campaign state requiring the player to interview each family member.

Eight preparation services exchange a fresh animal plus coins for prepared
supplies. Both archers accept them, with several different prices:

| Fresh object | Prepared supplies | Farlindel fee | Hanson fee |
| --- | --- | ---: | ---: |
| Fox 13505 | Fox fur 13550 + one fox fillet 13535 | 200 copper | 200 copper |
| Wolf 13508 | Wolf hide 13549 + one wolf roast 13536 | 300 | 300 |
| Rabbit 13511 | Rabbit pelt 13546 + one rabbit meat 13532 | 100 | 100 |
| Raccoon 13513 | Coonskin 13548 + one jerky 13534 | 100 | 100 |
| Buck 13515 | Buckskin 13543 + two venison chops 13531 | 500 | 500 |
| Deer 13516 | Deerskin 13544 + two venison chops 13531 | 300 | 300 |
| Doe 13517 | Doeskin 13545 + one venison chop 13531 | 300 | 200 |
| Rattlesnake 13518 | Snakeskin 13547 + two steaks 13533 | 300 | 100 |

Bluejay 13519 is sold for 1,000 copper to Farlindel or 2,000 to Hanson, rather
than tanned. The journal now exposes all twelve supporting services (eight
animal preparations, three ordinary-arrow trades, and the bluejay sale) after
an archer is encountered. They contribute no story/daily achievement units.

Marja (13503) has seven independent commissions:

| Story | Exact materials | Fee | Reward |
| --- | --- | ---: | --- |
| Jacket | Four buckskins + two rabbit pelts | 3,000 copper | 13522 |
| Boots | Two deerskins + four rabbit pelts | 3,000 | 13523 |
| Pants | Four doeskins | 2,000 | 13524 |
| Belt | Two snakeskins | 1,000 | 13525 |
| Cape | Ten fox furs | 10,000 | 13526 |
| Cap | One coonskin | 1,000 | 13527 |
| Backpack | Two wolf hides | 1,000 | 13551 |

These are seven terminal stories, not seven compulsory stages of one campaign.
Supplied prepared hides are legitimate. Adding local preparation history must
not force the player to recreate hides that the quest already accepts. Prepared
skin returns from the archers, and raw/finished item returns from Marja, are
feedback rather than successful crafting. Those 32 returns plus Alvinar's eight
cut-flower returns account for all 40 excluded contracts.

### Animal creation and decay

The assigned death special handles fox, wolf, rabbit, raccoon, buck, deer, doe,
rattlesnake, and bluejay. `forest_animals` creates an object with the dying
mobile's VNUM in its room and sets `value[0]` to a periodic countdown. The
shared death caller supplies the killer as `pl`, but this special ignores that
parameter: its output is not evidence of personal kill or personal recovery.

`forest_corpse` decrements that countdown and replaces the fresh object with
13520, preserving its weight and carried/room/container placement, then retires
the original. Normal decay is attached to the rotten output. The remaining
fresh-animal prototypes for pups, squirrels, robin, chipmunk, and skunk do not
have these assignments or accepted tanning contracts. Do not invent a use for
them from their names.

The journal currently measures live supplies and existing exchanges. It cannot
prove a birth source, decay deadline, or tanning lineage. Proposed events must
carry death/reset identity, actor policy, exact input/output UIDs, freshness,
and committed retirement. Group assistance must be authored separately.

### Lore and boundaries

The mage ownership, poaching warning, Marja's heritage, and the family's history
are lore. No inspected forest special enforces hunting permission or resolves
the heritage mystery. Marja refuses that subject; Alvinar's `cloud` closes it.
The gardener and pixie Q sections contain no addressable response. The brook,
farm produce, furnishings, and ordinary wildlife supply setting and resources,
not additional proven terminal stories. Tower continuations belong to their
own area review; this map does not claim to complete their campaigns.

The forest reset also intentionally loads undead wizard 15120 in foreign room
15141 with four potions 15119, to maintain an epic-skill teacher in a non-resetting
area. Its Q source belongs to `new_cavecity`, and the intelligence teaching
entry is in `src/classes/epic_skills.c`. This is a cross-area reset/service lead,
not a forest campaign. Future reset generation must preserve source-zone
occurrence identity independently of destination and giver/story ownership.

## Blockers, inconsistencies, and fair repair plan

| ID | Evidence and impact | Proposed fix and required proof |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | `reset_zone` refuses O/P/G/E and other item-producing reset commands while accounting is active. Fresh-world garden supplies, belt, arrows, and sprite ingredients therefore cannot be assumed to exist. This is an intentional authority guard, not proof that the legacy area was broken in its original mode. | Add an accounting-backed reset generation with stable command/occurrence identity, exact UID/custody results, nested dependencies, and replay. Test room, container, and NPC equipment sources, failed commit, duplicate reset, and cold restart before the active flower journey. |
| ZSQ-ANIMAL-LIFECYCLE | Fresh creation and rotten replacement use direct `read_object`/placement/extraction and carry no story-specific committed lineage. Carried rotten publication can be refused by active ownership authority. | Qualify creation and retirement as one source/transform operation, preserving placement and freshness. Test room/carried/nested decay, restart deadlines, failure, and direct recovery versus gifts. Do not award source credit merely because the object exists. |
| ZSQ-OPTIONAL-PREPARATION | The old journal suggested obtaining the belt even when a valid plant was supplied. | Fixed in schema 3 presentation: optional equipment preparation stays visible and does not take `Next:` precedence. Native acceptance and receipt identity are unchanged. Conditional subrecipes and all-stage campaigns remain separate work. |
| ZSQ-TWIN-TERMS | Hanson prose disagrees with executable fees and meat counts for several animals; Marja quotes 20 gold for a backpack but charges 10. Farlindel's and Hanson's prices intentionally differ in some contracts. | Owner decision, October 8: align conflicting dialogue with existing executable terms and preserve prices, quantities and reward balance. Treat differences between NPC prices as alternatives. Actual text corrections remain a separately identified fix with exact term checks and PR/news wording. |
| ZSQ-MIXED-OFFERING | All tanning and clothing require item-plus-currency offerings, outside current durable offering support. | Implement one atomic input/payment/reward/evidence transaction; test every distinct count, missing coin/item, group policy, replay, and preserved UIDs. Keep honest unavailability messages until then. |
| ZSQ-LEARNED-LORE | Showing an `ask` command is not proof its response ran. | Dispatch canonical accepted topics after native success; persist idempotently. Repeated aliases, greetings, unavailable NPCs, and failed saves must not invent history. |

## Qualification matrix

Source-comprehensive mapping is complete for this revision. Native parsing,
projection, and focused source audits must pass before publication. Full gameplay
qualification remains open: a fresh world with verified active accounting;
each plant and rejected flower; correct/wrong/carried belt; valid gift;
container arrow recovery and both prices; each fairy wand and four distinct
feathers; all preparation and clothing terms; death/decay placements; failed
commit; cold reconnect/restart. Record results separately from this source review.

The [generated review index](../../reference/zone-story-audits/twin_towers_forest.md)
accounts for every native binding, dialogue alias block, literal special assignment,
and reset-command count. Regenerate it with the documented command when sources
change. It is evidence, not gameplay proof.

## Explicit builder questions and recommended resolutions

Reviewed October 11, 2026. The recommendations preserve the existing native
exchanges and the owner decisions already captured for the gardener route and
conflicting service text.

| # | Builder question | Recommended resolution from current source |
| --- | --- | --- |
| 1 | Should the fuller flower achievement require belt acquisition, waist wear, and an accepted Alvinar plant exchange? [owner decision](../quest-story-followups/TWIN_TOWERS_FOREST_MILESTONES.md) · [Alvinar Q](../../../areas/qst/twin_towers_forest.qst#L14) | Yes. The owner approved these three historical facts in any order; each character must acquire and wear the belt. Keep the plant exchange valid before or without garden access. |
| 2 | Should the existing garden challenge be weakened into a friendly belt handout or a steal route? [movement check](../../../src/specs/specs.twintowers.c#L130) · [belt reset](../../../areas/zon/twin_towers_forest.zon#L371) | No. The owner confirmed mortal players have no active steal skill and that killing the gardener for the belt is intentional. Preserve this access challenge; do not make it mandatory for Alvinar's hand-in. |
| 3 | Should every one of Alvinar's eight plant alternatives remain an independent way to earn the identify scroll? [flower contracts](../../../areas/qst/twin_towers_forest.qst#L14) | Preserve all eight exact alternatives and the one scroll reward. Keep garden entry as optional preparation; the daisy reset is outside the garden. |
| 4 | Should the eight cut-flower return responses count as completed gardening? [flower source map](#flowers-for-alvinar) | No. They are refusals with explanation. Count only an accepted plant exchange as the native flower receipt. |
| 5 | Should Farlindel and Hanson share one missing-arrow story despite paying different amounts? [arrow contracts](../../../areas/qst/twin_towers_forest.qst) | Keep one exact arrow identity and one story outcome. Preserve their separate 300- and 200-copper rewards and keep their ordinary arrow purchases independent. |
| 6 | Should an arrow still require personal sprite combat if it is recovered from a nest or supplied by another player? [arrow sources](#the-archers-missing-arrows) | No. The Q checks the arrow, not its origin. Require current loose custody after recovery from a container; do not infer a personal kill. |
| 7 | Should Glor-Linda require all three wands or four copies of one feather? [sprite exchange](../../../areas/qst/twin_towers_forest.qst) | No. Preserve one dust, any one exact wand, and all four distinct feather kinds in one offer. Retain each wand alternative's original earring and experience reward. |
| 8 | Should Glor-Linda's disappearance after a reward mean that she is permanently removed? [sprite reset](../../../areas/zon/twin_towers_forest.zon) | No. Preserve the accepted departure and normal reset return as separate events. Do not describe the reward as deleting her from the world. |
| 9 | Should the eight tanning preparations, three arrow trades, and bluejay sales award story or daily credit? [service map](#fresh-game-tanning-and-the-family-clothing-work) | Keep all twelve as supporting services. They may be shown after archer contact but remain separate from the one missing-arrow story and from Marja's commissions. |
| 10 | Should Marja's seven clothing commissions be a mandatory sequence? [commission source](../../../areas/qst/twin_towers_forest.qst) | No. Keep each exact recipe, fee, and reward independent, and continue to accept supplied prepared hides without forced tanning history. |
| 11 | Should dialogue override the executable prices or quantities where Hanson's and Marja's prose disagrees? [owner terms decision](#blockers-inconsistencies-and-fair-repair-plan) | The owner chose to align conflicting text with the current executable terms. Preserve those fees, counts, and rewards; keep intentional differences between archer prices. Handle any copy repair separately. |
| 12 | Should the animal-death special be used as proof that a character killed or personally recovered the animal? [death dispatch](../../../src/combat/fight.c#L2211) · [animal lifecycle](#animal-creation-and-decay) | No. The special ignores the killer parameter. Require a distinct committed source and custody event if a personal recovery story is later approved. |
| 13 | Should current item-reset declarations guarantee live flower, belt, arrow, and sprite stock? [reset admission](../../../src/world/db.c#L3291) · [zone resets](../../../areas/zon/twin_towers_forest.zon) | No. Preserve the active accounting authority guard. Qualify generation, publication, nested placement, and replay before promising fresh stock or enabling a daily. |
| 14 | Should family interviews, refused lore, or an unanswered gardener/pixie topic be campaign requirements? [lore boundary](#lore-and-boundaries) | No. Keep dialogue as guidance unless an addressed response and intended saved knowledge outcome are defined. Do not turn greetings, refusals, or absent M blocks into receipts. |
| 15 | Should generic healing or poison removal count as curing the altered, sick, or dying animals? [animal story families](#story-families-and-actual-prerequisites) | No. Require an observable cause, attainable treatment, eligible living beneficiary, and useful saved aftercare state before adding a cure achievement. |
| 16 | Should the keeper's dung-cleaning text or aggression flags count as an implemented cleanup service? [keeper and flags](../../../areas/mob/twin_towers_forest.mob) | No. Preserve the decoded evil/undead/outcast aggression and define a bounded task, cost, recipient, and committed habitat result before describing a working service. |
| 17 | Should ordinary or unassigned species become quest inputs because their names resemble the tanned animals? [prototype and special map](#animal-creation-and-decay) | No. Keep only the nine assigned fresh-animal types and authored inputs in the reviewed flow. Do not infer a use for pups, squirrels, robin, chipmunk, or skunk. |
| 18 | Should the New Cave city wizard reset be counted as a local forest quest or moved to New Cave city's reset file? [cross-zone reset owner](../quest-story-followups/NEW_CAVECITY_AUDIT_REVIEW.md#typed-and-compiled-references) | Keep the occurrence owned by Twin Towers Forest while preserving New Cave city as the wizard's story and service owner. Retain the four local object #15119 G rows under their current source reset. |
| 19 | Should the three reciprocal tower routes, Surface entry, or load-room links alone prove a public or safe forest journey? [active and boundary graph](../quest-story-followups/WORLD_CONNECTIVITY.md#checkpoint-407-twin-towers-forest-boundaries-and-directed-access) | No. Keep authored WLD edges separate from open doors, scripted routes, lawful arrival, return, and completed tower campaigns. Treat the two load-room approaches as source edges, not player-facing guarantees. |
| 20 | Should all 84 native exchanges be projected as 84 achievements or dailies? [exchange accounting](#evidence-boundary) | No. Preserve the authored story units and aggregation in the current map; keep greetings, refusals, rejected flowers, and service trades outside story credit. |

These recommendations document source decisions and captured owner choices.
They do not qualify active reset publication, successful movement, item-source
provenance, committed payment, treatment, cleanup, persistence, or ordinary
play through the connected towers.
