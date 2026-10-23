# Braddistock Mansion: comprehensive source story map

Reviewed October 2, 2026. Source area `braddistock`, zone 13, journal revision 2.
This old mansion is separate from source area `brad` (zone 1350), which has
the same display name. Do not combine their mobs, quest receipts, access rules,
or difficulty solely because the names match.

## Evidence boundary

Reviewed complete active [Q/M source](../../../areas/qst/braddistock.qst),
[resets](../../../areas/zon/braddistock.zon),
[rooms](../../../areas/wld/braddistock.wld),
[mobiles](../../../areas/mob/braddistock.mob), and
[objects](../../../areas/obj/braddistock.obj). There is no local shop source.
Two distinct native contracts, one addressable M response, two ambient
`qc_action` responses, 186 reset commands, fourteen mobile prototypes, and
73 object prototypes are accounted for. The source includes a bookend room;
it does not create another story.

The [assignment index](../../reference/zone-story-audits/braddistock.md) binds
object 1372 to `jet_black_maul`. Reviewed the complete
[procedure file](../../../src/specs/specs.braddistock.c),
[assignment site](../../../src/specs/specs.assign.c), and shared
[quest dispatch](../../../src/world/quest.c). The procedure named `braddistock`
is assigned to mobile 135014 in `brad`, not this area's ghost 1314. Its level-14
northward barrier is therefore not a prerequisite for this mansion story.

## Progression story: find Slippers and quiet the lord

| Stage | Source / actual outcome | Journal treatment |
| --- | --- | --- |
| Speak with the lord | Ghost 1314 at road 1300 responds to `hi`, `hello`, or `help`. He believes you are from Tharnadia Pest Control and asks about missing pet Slippers. | Encountered contact and conversation guidance. No learned-topic or employment achievement. |
| Follow mansion clues | Library 1303: old tome 1303 contains damp note 1304, with surviving words “beyond skeletons.” Search the wine cellar and hidden passages. | Exploration guidance; reading a note and finding exits are not recorded objectives. |
| Find a release key | Rat Lord 1310 at deepest cavern 1345 carries silver key 1334 and has followers. | Optional live key preparation. Personal kill and source recovery are not proven by possessing the key. |
| Release Slippers | Jaguar 1316 in hidden laboratory 1338 accepts one silver key 1334, gives collar 1340, and disappears. | Separate service row `release-slippers`; durable native receipt. Also optional preparation in the final story. No additional achievement/daily bonus for the intermediate service. |
| Return proof | Lord accepts one collar 1340, gives remains 1375 and 150,000 experience, then disappears. | One terminal story `quiet-the-mansion`. A supplied collar is accepted without key possession or a rescue receipt. |

The pet Q prose says the freed jaguar runs through the house and kills rats.
The native exchange rewards and removes the giver; it does not supply an
authenticated rat-purge event. Lord's text promises payment to a pest-control
company tomorrow, but there is no second delayed payout encoded in this Q.
Record actual experience/remains and narrative closure without inventing later
money, all-rat kills, a cleared house, or a permanent rescue state.

`contracts` on the final story contains only Lord's terminal receipt. A rescue
receipt alone cannot complete it. Intermediate preparation is optional because
native execution accepts externally supplied collar/key materials. Repeated
resets make the givers available again; a disappearing actor is not automatically
a one-time story. Existing eligibility/telemetry rules still control daily credit.

## Access, alternate exploration, and optional equipment lore

The front double doors are closed. The scullery leads down to wine cellar 1335;
its concealed south passage reaches occupied cellar 1336. A barred concealed
east passage reaches 1337, whose concealed east passage reaches the laboratory.
The cellar's south connection reaches cavern 1340; routes through 1341–1344
lead to the Rat Lord in 1345. Search/open mechanics and normal combat support
this route. No custom journal stage or qualifying movement receipt is present.

The upper east bedroom door at 1326 uses scratched key 1316, a different item
from the pet's silver key 1334. The scratched key is available among mansion
loot; that bedroom is optional exploration, not a fabricated rescue gate. Other
containers use their own lock/key data. A room or mobile number equal to a key
prototype is not evidence of a dependency by itself.

The abandoned library books, hidden crystal/light, jewelry, intact gold laboratory
objects, Philosopher's Stone book 1335 and pebble 1342 are exploration loot/lore.
No local Q or assigned custom transmutation consumes them. The alchemist's
scroll 1343 is a detect-magic scroll inside table 1341, not a recipe contract.
The note, skeletons, newer clothing and occupied cellar suggest an investigation;
they do not establish an authored all-stage murder or alchemy campaign.

Warrior corpse container 1324 at 1345 holds plate, weapons, supplies, an emerald,
and titan maul 1372. The maul's description hints at naming its creators.
`jet_black_maul` accepts `say titan` when the item is in the main WIELD slot
and at least sixty seconds have elapsed since its timer. It casts strength and
enhanced strength at level 50 and updates the object timer. Merely carrying it
or using the offhand slot does not satisfy the trigger. Removal of a matching
item or `all` clears the strength effects through this handler. Spell success
and timer persistence must be qualified before adding an equipment activation
milestone; no quest reward is granted by this special.

Diseased-rat object 1376 and lightning sword 1374 use ordinary object properties
and combat messages. Their impressive prose does not add another quest terminal.
The lord's ambient exorcism scene and pet's meow/chain scene are printed through
`execute_quest_routine`; neither proves a cleric action, release, or earned lore.

## Findings and implementation additions

| ID | Finding / balanced interpretation | Fix or qualification |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | Active accounting suppresses nested corpse/table loot, the silver key carried by the Rat Lord, and other item resets. A recovered live world can still contain them. | Qualify reset generation and exact NPC/container custody before playing this route under active accounting. Preserve conditional reset dependencies and instance ownership. |
| ZSQ-BRADD-PREPARATION | Previous journal made the key/rescue steps mandatory `Next:` even with a supplied collar, and classified the pet only as an excluded intermediate. | Fixed using schema 3 optional steps and a displayed service. Validate supplied collar, supplied key, rescue-only progress, terminal delivery, and original receipt recovery. |
| ZSQ-BRADD-OWNERSHIP | Identically named `brad` and `braddistock` sources have different assigned ghost procedures. | Keep source-area identity in audits and author tools. The level barrier belongs to `brad`; review it when that area is touched, without importing it here. |
| ZSQ-BRADD-NARRATIVE | Rat-purge and later-payment prose are not backed by corresponding native effects. This may be intentional narrative closure. | World builders can clarify prose or add actual reset-instance purge/delayed-payment transactions. Require exact participants and recovery before displaying these as earned stages. Do not silently add rewards or change balance. |
| ZSQ-BRADD-MAUL | Activation is equipment/context/cooldown dependent; removing it clears strength spells without distinguishing their origins. | Treat as optional lore now. Test actual cast/stacking/removal behavior and timer persistence; if builders want an activation stage, add an accepted-effect event tied to the exact item and actor. Consider effect ownership separately from quest credit. |

The deployable journal now has one achievement story and one non-achievement
rescue service. Both native bindings are preserved, with no exclusions. Full
source mapping is complete for this revision. Search, combat, personal key
recovery, actual rat purge, maul activation, and source admission remain separate
qualification work in the [shared plan](../ZONE_STORY_INTEGRATION_PLAN.md).

## Direct area-owner questions and conservative recommendations

These questions keep zone13 `braddistock` separate from zone1350 `brad` and
record the few story decisions that remain beyond the deployed native contracts.
Recommendations preserve both local story IDs and the existing item exchanges
until an area owner selects a specific change.

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| How should players distinguish this zone13 `braddistock` mansion from active zone1350 `brad` when both display as “Braddistock Mansion”? | The normalized title is identical, but the source keys, room ranges, histories, bound ghost procedures, quest receipts, and Tharnadia routes differ. | Choose a distinct public label or explicit disambiguation in player-facing listings and journals. Keep both keys, routes, and receipt identities; the shared name does not make one a replacement. |
| Is Slippers' release intended to remain a non-achievement service separate from the Lord's terminal story? | The jaguar accepts silver key 1334 and gives collar 1340; the Lord separately accepts the collar and awards remains/experience. The journal records a service and a distinct final story. | Preserve the two outcomes unless the owner defines an explicit sequence or shared ending. Do not grant a second achievement for the rescue alone. |
| Should freeing Slippers produce a recorded rat-purge result or the later pest-control payment promised in dialogue? | The jaguar's text says it kills rats and the Lord mentions payment tomorrow, but neither a house-wide purge event nor delayed payout is implemented. | Keep these as narrative closure or clarify the prose. Add an outcome only with an actual durable purge/payment trigger and a defined recipient; do not infer it from collar delivery. |
| Should the Lord continue to accept a supplied collar without the player finding the key, releasing Slippers, or proving a personal kill? | Native acceptance checks the exact collar 1340; key/rescue/source history is optional and does not gate the terminal receipt. | Preserve supplied-item eligibility unless a separate owner-approved prerequisite is added with its own receipt. Do not infer personal combat or first-source history from possession. |
| Should reading the note, searching hidden passages, or defeating the Rat Lord become recorded story milestones? | These actions guide exploration, but no local receipt records them; supplied key/collar routes can complete the current exchanges independently. | Keep them as optional guidance unless the owner defines a successful event and persistent result. Failed attempts, room access, or a held key alone should not complete a story. |
| What source and renewal policy should govern the Rat Lord's silver key, nested mansion loot, and repeat givers under active accounting? | Active accounting suppresses these item resets; recovered worlds may still contain stock, and recurring giver availability is separate from actual source renewal. | Preserve reset ownership and existing caps while qualifying actual source generation, custody, and reset episodes. Do not duplicate stock or promise repeat availability from static declarations. |
| Should `say titan` maul activation remain optional equipment behavior, or count as a journal outcome? | The exact maul special changes equipment effects/timer but awards no quest receipt; main-hand slot, item presence, cooldown, cast result, and removal behavior all matter. | Keep it outside story credit unless the owner defines an actor/item-specific successful effect and separate recovery semantics. Qualify the existing equipment behavior independently. |

## Journey matrix

Qualify the ordinary hidden-cellar/key/rescue/collar route; supplied key and
supplied collar routes with no prior history; rescue without Lord completion;
wrong key/collar variants; original receipt restart/replay; repeat reset and
independent encounter generations; duplicate delivery and daily group behavior.
Confirm optional exploration is never required for native turn-in admission.
Exercise all hidden exits/locks, corpse loot, Rat Lord followers, and maul slots,
cooldown, failed cast/removal/reconnect independently. Persisted history and live
inventory checks must agree without journal reads writing accomplishments.
