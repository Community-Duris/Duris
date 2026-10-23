# Starter journal review queue

Source baseline: master `2a1604e4eedf55034c8183c5cfc28cb683ef88cd`.
Status: source review and player-copy cleanup; live mortal journeys and user
story decisions remain open. Native world files are unchanged. These areas have
familiar early-game locations, but their deeper encounters are not uniformly
low-level: Turolopolis includes level 44â€“50 heroes and a level 50 Willem.

## 1. Ruins of Turolopolis (`willem`) and Turolopolis Zoo (`turolzoo`)

The city has six accepted outcomes. The public story begins with Lothrell's guilt:
he fought Willem's slavery with the Ring of Glory, miscast a spell and escaped,
then believed his five companions dead. He requests their colored badges for a
memorial. Room/mobile descriptions instead show the companions alive but broken
inside mirrored chambers, and Willem confirms trapping them in the statues.
The executable outcome is the five-badge memorial exchange. There is no verified
rescue/reunion endpoint; the journal attributes the belief to Lothrell and does
not claim that returning badges saves the heroes.

| Story | Existing progression and reward | Review boundary |
| --- | --- | --- |
| Corwyck's fountain research | Unicorn horn â†’ iron bunker key and experience; Corwyck leaves to work | His dialogue says he will treat the fountain. No persistent cured-fountain/animal state is recorded. |
| Lothrell's memorial | Five different colored Glory Badges together â†’ lesser bloodsaber and experience; he leaves | His white badge is not one of the five. Badge collection does not prove a rescue. |
| Kurtukr's craft | Lesser bloodsaber + caecilia stinger â†’ greater bloodsaber | Lothrell is an optional earlier route; a supplied lesser blade is valid. Sprecken supplies context. |
| The drow emissary's news | Minotaur's timeworn letter â†’ ribbon and experience; emissary leaves | Minotaur carries the letter; his ooze reward does not give it to the player. |
| Minotaur's experiments | Blue ooze â†’ gauze wrappings and experience; minotaur leaves | Letter recovery and ooze completion may require different resets or supplied items. |
| The slave's proof | Willem's named skull â†’ coins; slave departs | Record the proof hand-in, not a new global liberation or palace-field change. |

The city park joins the zoo at city room 7123 â†” zoo entrance 53000. The zoo has
one Q contract: hunter 53003 accepts saber tooth 53001 plus gorilla tooth 53003,
gives bark helmet 53004 and leaves. Its four ASK aliases select one response.
The teeth are loaded on the tiger and gorilla; matching supplied teeth are valid.
The keeper's house hides a lower-cage route. A keeper, altered animals and ruined
habitats create atmosphere but do not implement healing or zoo-restoration quests.
The player journal now describes the tooth quest without promising those outcomes.

Evidence: [city quests](../../areas/qst/willem.qst),
[city characters](../../areas/mob/willem.mob),
[city rooms](../../areas/wld/willem.wld),
[city resets](../../areas/zon/willem.zon),
[zoo quests](../../areas/qst/turolzoo.qst),
[zoo rooms](../../areas/wld/turolzoo.wld),
[zoo characters](../../areas/mob/turolzoo.mob),
[zoo resets](../../areas/zon/turolzoo.zon).
Full research remains in [Turolopolis](zone-stories/RUINS_OF_TUROLOPOLIS.md) and
[the zoo](zone-stories/TUROLZOO.md), including hypotheses not implemented here.

User decision, 2026-10-23: **no reunion or rescue story**. The badge bearers are
aggressive; direct recovery requires killing them and looting their badges.
Present the existing outcome as badge recovery for Lothrell's memorial. The
journal does not require a personal kill when another player supplies matching
badges. Earlier research suggestions about a reunion are superseded by this
decision, not an outstanding builder question. Preserve the minotaur's two
outcomes across resets or supplied materials; one NPC instance need not support
both routes in one visit.

Live proof still needed: walk both discovery boundaries as a mortal; ask aliases;
try incomplete and duplicated tooth pairs; accept a supplied exact pair; complete
the badge route and supplied-blade shortcut independently; reconnect and verify
records; confirm hidden-route entry and return without staff movement.

## 2. Vella's Bordello (`bordello`)

**Correction, 2026-10-23:** the initial exploration-only classification was too
narrow. The area has scripted paid follower services, and roaming creatures
supply materials for quests completed in Winterhaven. Absence of a same-stem
QST file does not mean absence of quest or service gameplay. The user's recalled
interaction is the paid follower service, not the roaming-material connection.

### Paid temporary followers

Three local mobile prototypes have assigned C++ callbacks with the same offer:

| Provider | Callback | Existing offer and outcome |
| --- | --- | --- |
| Sex crazed prostitute #93602 | `sex_crazed_prostitute` | At least 500 copper value in one payment; establishes a pet and follows the payer. |
| Well built prostitute #93603 | `well_built_prostitute` | Same payment and temporary pet outcome. |
| Sleazy prostitute #93611 | `sleezy_prostitute` | Same payment and temporary pet outcome. |

Each callback compares the NPC's money immediately before and after `do_give`.
A positive increase below 500 produces the underpayment response; the money is
kept and separate payments do not accumulate toward a hire. At 500 or more the
callback calls `setup_pet(ch, pl, 10, 0)`, stops an existing follower relationship
if present, and calls `add_follower(ch, pl)`. Standard player charm duration is
ten minutes. Existing Unholy Alliance/globe behavior can make that duration
indefinite; the journal does not alter these pet rules. These callbacks have no
item or experience reward and do not execute a native QST contract.

The `topless_prostitute` handler also exists, but no assignment to local #93604
was found. Keep that unassigned handler separate from the three configured offers;
adding a binding would be a native behavior change requiring separate review.

Current journal revision two explains the price, underpayment behavior and
follower outcome, with three service-provider contacts revealed on encounter.
**It does not yet record a hire completion.** The existing native QST observer
cannot see the custom success path. The map stays `coverage: "partial"` while
scripted outcome tracking and live journeys are outstanding. Service visibility
must not be mistaken for a completed integration.

The next implementation should observe a successful hire after the pet-owner
link and follower relationship are both established, using a stable scripted
service identity for each configured provider. Do not use a handled GIVE command
or an NPC money increase alone as proof. `setup_pet` can return -1 for both a
rejected target and a valid indefinite duration, so its return sign alone is also
insufficient. Reuse the journal's persistence and failure isolation, credit the
payer, and retain service classification unless an achievement policy is chosen.
The observer must never retry the payment or pet creation to repair journal state.

Acceptance cases: exact payment, overpayment, underpayment, two smaller payments,
wrong recipient, item gift, fighting provider, rejected pet setup, ordinary
expiry, existing indefinite-duration effects, rehire, journal storage failure,
and reconnect. Verify the original money/pet outcomes as well as the journal
record. No extra reward or daily credit is implied by recording a service.

Evidence: [assigned callbacks](../../src/specs/specs.assign.c#L352),
[hire implementation](../../src/specs/specs.grove.c#L439),
[pet setup and duration](../../src/classes/necromancy.c#L211),
[local characters](../../areas/mob/bordello.mob).

### Roaming sources for Winterhaven's quests

The shared dispersal room #55614 in `wh.wld` has an up exit to Vella's fountain
hall #93610. Its resets declare the five Incarnates #55550-#55554 and psychomia
treants #55246. Their assigned death callback creates one object with the same
VNUM in the room. The dispersal-room prose says two ingredients; the callback
creates one. This is source evidence of the intended route, not a live population
or renewal guarantee. Incarnates are level 45 and treants level 24, so these
visitors do not make a uniformly easy beginner quest set.

| Source and progression | Existing outcome | Journal treatment |
| --- | --- | --- |
| Incarnate remains plus one empty potion bottle, brought to Bipplewizzlille | Fire/flame -> fiery potion; Frost/snowflake -> frosty potion; Stone/granite -> granite potion; Air/wind -> acceleration potion; Mischief/bubble -> protection potion | Vella's explains the connection and reveals the roaming contacts on encounter. Winterhaven retains the five actual recipe checklists and completion owners. |
| Psychomia plant brought to Buzzbeef; his small plastic bag then brought to Thyz | Buzzbeef gives 100,000 copper and the bag; Thyz gives a super secret invitation | Explain both steps without making personal treant hunting or a prior personal Buzzbeef completion mandatory. |
| Psychomia plant, nightshade and two gnomish shopkeepers tokens brought to Branay | Small nightshade splinter | Explain the independent use of the same plant; retain exact extra ingredients. |

The three original local contacts (Vella, Padh and Vegar), three hire providers,
and six wandering sources bring the map to twelve contacts. Encounter history is
by mobile prototype, not proof that the encounter happened in Vella's. Winterhaven
retains its existing completion identities and now names Vella's in the matching
ingredient hints. Vella's uses prose referrals; it does not yet embed another
zone's live quest checklist or grant duplicate completion/daily credit.

Evidence: [shared dispersal room](../../areas/wld/wh.wld#L12606),
[shared resets](../../areas/zon/wh.zon#L1622),
[death conversion](../../src/specs/specs.winterhaven.c#L70),
[Incarnate recipes and dialogue](../../areas/qst/wh.qst#L749),
[Buzzbeef](../../areas/qst/wh.qst#L1322),
[Branay's recipe](../../areas/qst/wh.qst#L1557),
[Thyz's invitation](../../areas/qst/wh.qst#L2038).
The active `wh.zon` title is **The City of Winterhaven**; older research uses
White Harbor for the same source key.

The area still has 69 rooms, 16 local mobile prototypes and Vegar's shop.
Treasure-door resets and imported slot-machine objects remain research follow-ups.
No native repair is included. The preserved
[historical audit](quest-story-followups/BORDELLO_AUDIT_REVIEW.md) remains unchanged;
this current review corrects the earlier journal classification.

## 3. Harrow (`harrow`)

A girl loses the ring her father made. Returning it earns a token and she leaves.
Alorka accepts that token plus two ingredients for each of four distinct crafts:
glass horn, robe of many colors, wand of light, or lucky alchemist sack. Each
outcome is an achievement. Four outcomes require four tokens; additional resets
or supplied tokens are intended possibilities, not a broken quest.

Bom wants a near-finished painting for his cabin and gives three fish. The
fishbowl contains a fish-food exchange. A leprechaun accepts the whale's pot of
gold for a clover and coins, then departs. These bring the total to eight native
outcomes. No normal incoming route to the leprechaun's area has yet been verified;
keep that uncertainty visible and investigate before treating it as a daily
journey. Do not invent a luck, pipe or clover access requirement.

Alorka's dialogue calls the light reward a sphere/staff; object 29418 is actually
a wand. The journal names the actual reward; native prose and rewards are intact.
Buying a finished lucky sack does not complete its craft, while buying or
receiving the exact ingredients can prepare it.

Evidence: [quests](../../areas/qst/harrow.qst),
[rooms](../../areas/wld/harrow.wld), [characters](../../areas/mob/harrow.mob),
[objects](../../areas/obj/harrow.obj), [resets](../../areas/zon/harrow.zon),
[full audit](quest-story-followups/HARROW_AUDIT_REVIEW.md).

## How to close a review

Record source evidence, a mortal journey result, the user's story decisions and
remaining questions. Update the player map only with supported facts. Keep an
actual native fix in a separate named commit and PR/news entry. A source-valid
map is not proof of live reachability, reset renewal or a completed rescue.

## Suggested next journals

1. **Woodseer:** seven independent local errands: a bard's broken mandolin, honey
   for a farmer's birthday dinner, green pigment for an artist, beeswax for a
   woodworker, a bee egg for a mage, a hummingbird feather for a wizard, and raw
   turtle meat for the maitre d'. This is the next candidate for checking concise
   item hints and readable independent outcomes. Confirm source levels and routes
   before calling the whole set suitable for a new character.
2. **Kobold Settlement:** silver smelting and a shield commission support a more
   complex crafting review; statue gems and matching frames lead to spectacles.
   Keep coin fees, exact quantities and supplied-material shortcuts. Investigate
   actual stock-cap/reset behavior rather than declaring scarcity a broken quest
   or promising that one reset supplies everything.
3. **Town of Moregeeth:** connect Moreg's blackmail request, Glub's return of the
   killer's sword and Tala's assassin-letter investigation without inventing a
   required order. Gimbatul's component pouch supplies a portal-room key, while
   four planar components earn a crown. Moreg also wears Glub's requested sword,
   so review giver availability across resets and supplied-item alternatives.
   The paid dart and bat-skull necklace recipes remain supporting services.

These maps are preserved already. This queue proposes their next careful review;
it does not mark their live journeys complete or introduce native repairs.
