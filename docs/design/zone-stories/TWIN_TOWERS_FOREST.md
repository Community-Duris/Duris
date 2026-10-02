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
than promise a friendly belt exchange. Review combat/theft/transfer admission
before prescribing a personal acquisition route.

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

Future history: optional learned request, successful guarded movement, personal
plant recovery under an explicit source policy, and the existing terminal receipt.
The first three are currently untracked. A read or a failed movement earns nothing.

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
| ZSQ-TWIN-TERMS | Hanson prose disagrees with executable fees and meat counts for several animals; Marja quotes 20 gold for a backpack but charges 10. Farlindel's and Hanson's prices intentionally differ in some contracts. | Record every actual term (above); obtain a deliberate world-content decision before changing prose or prices. Treat price differences as alternatives, not automatically defects. Add exact reward/fee regressions for any chosen repair. |
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
