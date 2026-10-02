# The Town of Breale: comprehensive source story map

Reviewed October 2, 2026. Source area `breale`, zone 26, journal revision 2.
The source contains 56 rooms, 30 mobile prototypes, 86 object prototypes,
399 reset commands, six native contracts, and six addressable M responses.
All six exchanges remain distinct: five Triad stages with different rewards
and a separate dying-witch request. No exchange is excluded.

## Evidence boundary

Reviewed the complete active [Q/M source](../../../areas/qst/breale.qst),
[resets](../../../areas/zon/breale.zon), [rooms](../../../areas/wld/breale.wld),
[mobiles](../../../areas/mob/breale.mob), [objects](../../../areas/obj/breale.obj),
and all five [shop records](../../../areas/shp/breale.shp).
The [generated audit index](../../reference/zone-story-audits/breale.md)
records exact contracts, response locations, and nine literal mobile assignments
to `breale_townsfolk` in [specs.assign.c](../../../src/specs/specs.assign.c).
Reviewed that complete function in
[specs.highway.c](../../../src/specs/specs.highway.c), its periodic callback,
the shared shop wrapper, quest execution, magic-door handling, and reset/exit
loader. Other procedures in the highway file are not assigned to this area.

The catalog owns a derived room range beginning at 2533; this file's actual
rooms begin at 2600. Item, room and mobile numbers are separate identities.
For example, object bracelet 2645 and room Passage of Clarity 2645 do not
constitute a bracelet-gated room merely because their numbers match.

## Progression: a wounded witch and the concealed world below

At burial ground 2621, dying witch 2617 responds to `hi`, `hello`, or `wrist`.
One **small potion of healing 2679** earns triangular bracelet 2645 and removes
the witch through the Q disappearance outcome. Doctor 2616 at 2609 holds this
specific potion in reset equipment. No native purchase or peaceful potion-gift
contract is encoded for the doctor; ordinary custody/combat rules still apply.
A supplied potion is equally acceptable. Possession is not personal recovery.

The witch says to wear the bracelet and be led by one of her kind. Hagatha 2618
at hut 2623 looks at visitors' wrists in her prototype description. Neither has
a bracelet-checking escort procedure assigned in this source. The actual
trapdoor uses **small key 2680**, which Hagatha holds in reset equipment.
The bracelet does not replace that key, reveal a recorded route, protect against
town hostility, or prove a successful escort. Keep the narrative clue, with an
explicit builder decision to clarify it or implement the intended encounter.

The secret down exit from 2623 reaches stairway 2622 and levels 2637–2640.
The key fits the reciprocal trapdoor; its reset states are closed, locked and
secret. Stairs lead through mushroom caves 2640/2641 to a concealed east exit
at 2642, then 2643. Passage of Clarity lies south at 2644–2646; the witch
complex lies east through 2647. Celia's Spices'n'Things is south at 2648.
This is the ordinary local exploration route, not a new historical achievement.

## Progression: decipher and assist the Triad

Esmerelda 2600 is at 2652; Abigail 2601 is at 2651; Pontif 2602 is at 2654.
Their dialogue recommends the sequence below. Native admission checks the
offering, not prior conversations, the bracelet, class identity, or earlier
Triad receipts. Supplying the final recipe can complete Pontif's exchange
without completing the other four. A complete Triad campaign would need an
explicit all-stage family, while retaining those independent accomplishments.

| Exchange | Exact offering | Actual native reward / guidance |
| --- | --- | --- |
| Esmerelda: eastern hat | One dirt 2673 | Emerald bracelet 2681 |
| Esmerelda: further assistance | One fennel root 2674 | Iron gloves 2682; prose names `brontella` |
| Abigail: western wand and two western hats | One peppercorn 2671 and two fennel roots 2674 | Blue potion 2683; next broom/hat request |
| Abigail: two western brooms and eastern hat | Two wood piles 2672 and one dirt 2673 | Pink potion 2684; prose names `montra` |
| Pontif: two eastern brooms, eastern wand, two western brooms | Two red mushrooms 2675, one ash 2670, two wood piles 2672 | Triad amulet 2602 |

`hi`/`hello` introduce Esmerelda; Abigail and Pontif rebuff those greetings.
`brontella` prompts Abigail's clues, and `montra` prompts Pontif's recipe.
These responses do not save learned topics or authenticate Esmerelda/Abigail
completion. Repeating a password cannot earn another quest achievement.

### The clue vocabulary is deliberate

The Passage of Clarity encodes the ingredients in drawings. Room 2644 displays
a wand, 2645 a broom, and 2646 a hat. Odd-numbered drawings are on the east
wall; even-numbered drawings are on the west wall.

| Clue | Drawing topic / room | Depicted material | Executable ingredient |
| --- | --- | --- | --- |
| Eastern wand | `one`, 2644 | Scooping ground near a volcano | Ash 2670 |
| Western wand | `two`, 2644 | Round dark spice in a cauldron | Peppercorn 2671 |
| Eastern broom | `three`, 2645 | Collecting red fungus | Red mushroom 2675 |
| Western broom | `four`, 2645 | Chopping logs | Wood 2672 |
| Eastern hat | `five`, 2646 | Digging the ground | Dirt 2673 |
| Western hat | `six`, 2646 | Digging plants with a fork | Fennel root 2674 |

This is a source-supported riddle translation, not an unexplained mismatch to
repair by substituting wearable hats or weapon wands. The displayed scenery is
room prose, not obtainable quest equipment. The journal uses accepted materials
and explains the drawings. A future reveal policy should let builders choose
when to show the translation, with accepted examination evidence if they want
an earned puzzle milestone. Automatically extracting a candidate explanation
does not authorize exposing a solution or declaring a prerequisite.

Celia's shop stocks all six reagents, with G reset inventory for each. Five
loose mushroom reset commands also occur in the lower cave. No fee is part of
these six Q contracts; normal shop prices/custody are separate transactions.
A player may purchase, recover, or receive valid materials. Current journal
counts cannot distinguish these acquisition routes.

## Access, hazards and supporting content

Nine townsfolk prototypes (2613, 2614, 2619–2625) share `breale_townsfolk`.
While awake and not fighting, the periodic handler scans visible occupants,
checks normal protection/aggimmune rules, and attacks shamans, sorcerers,
necromancers, summoners or conjurers. `CMD_PERIODIC` and `CMD_NONE` both equal
zero, so its `if (cmd)` check correctly admits periodic execution. There is no
callback mismatch to fix. The fish vendor's shop retains this procedure as its
secondary handler; being a shopkeeper does not erase this behavior.

The handler does not check bracelet 2645, Triad amulet 2602 or conversational
passwords. Normal combat, visibility, awake state, racial/faction protection,
and giver availability remain separate from accepted recipes. The journal
states the class hazard without advertising immunity from a quest reward.

Waiting Room 2655 north to Pontif uses black key 2601, reset loose in 2655.
Pontif carries red key 2600 for the north door to Bacca's demon room 2653.
These are separate keys and routes. The demon is not a required native kill
or an additional Triad completion; room prose about destroying surface hunters
does not implement a town liberation outcome. The church, crucified-witch
circle, guards and merchants supply setting and ordinary encounters.

Other shops belong to the fish vendor, Monty, Bart and Garvin. Their food,
weapons and armor are services; vendor prose alone does not create another
quest. Ordinary equipment, including broom of air 2607, uses its actual item
type and spell values. It is not one of the Passage's ingredient clues simply
because its name contains broom. No local item or room custom procedure is
assigned in the inspected literal assignment sites.

## Reward/prose findings and implementation additions

| ID | Finding / balanced interpretation | Concrete fix or qualification |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | Active accounting suppresses ordinary item resets: keys, doctor potion, reagents, NPC equipment, shops and scenery. Recovered items can still exist. | Qualify reset generation, command occurrence and exact NPC/room/shop custody before the fresh active pilot. Do not claim a missing prototype: the small key does have a Hagatha E source. |
| ZSQ-BREALE-RIDDLE | Hat/wand/broom wording is explained by Passage drawings. | Preserve the puzzle; add builder-authored clue translations, room-examination evidence and staged hint/solution visibility. Do not silently replace executable reagents. |
| ZSQ-BREALE-ESCORT | Bracelet/leading prose and Hagatha's wrist description have no corresponding assigned escort or bracelet gate. This can be intentional narrative guidance. | Builder chooses clarified clue or an actual accepted escort/access transaction. Track exact actor/encounter/travel only if implemented; do not invent bracelet protection. |
| ZSQ-BREALE-SPELLS | Q prose promises fireball, fireshield and incendiary cloud. The first two rewards are potions, and the final reward is armor; no `R S` skill-learning grant exists. | Reconcile intended rewards with world builders. Blue potion 2683 contains armor/spirit armor (1/192); pink 2684 contains bless/vigorize light (3/64), not the promised fire spells. Preserve receipts and balance until the content decision; do not advertise learned spells. |
| ZSQ-ACCESS-STATE | Trapdoor keys and later room keys control travel, while recipe admission is independent. | Add reviewed exit/key/source metadata and concrete live access reasons, separate from durable successful travel and personal recovery. A supplied key or already-open route must remain valid. |
| ZSQ-ALL-STAGE / LEARNED-LORE | Five separate Triad rewards cannot be represented by an any-of terminal array without premature completion. Keywords and drawings are not saved lessons. | Version all-stage campaigns and topic/examination objectives with explicit repeat and reveal policy. Keep six current achievements and their original native receipts. |

Revision 2 adds optional access preparation, optional earlier Triad receipts in
Pontif's journal, exact source/clue hints, and named terminal instructions.
It does not change contracts, rewards, dailies, world behavior or receipt IDs.
Full source mapping is complete; active-world qualification remains separate
in the [shared plan](../ZONE_STORY_INTEGRATION_PLAN.md).

## Journey matrix

Qualify the doctor/potion/bracelet route; Hagatha key/trapdoor; secret Passage
and all six drawing clues; Celia purchases versus gifted/looted reagents; all
five Triad exchanges and a supplied final recipe with no earlier history.
Exercise two-of-a-kind counts, wrong similarly named potions/reagents/keys,
closed/locked/open doors, secret search, black/red key routes, and class hostility
with awake/visibility/protection variants. Confirm no bracelet immunity or
learned spell is inferred. Test all original receipt restarts/replay, independent
reset encounters, daily policy, group credit versus personally met contacts,
and read-only journals. Future all-stage campaigns must prove every stage
without rewriting the existing independent native history.
