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

The active area has 69 rooms, 16 mobile prototypes and Vegar's shop. Vella manages
her establishment, Padh guards it, and the rooms depict patrons, private chambers,
a fountain and a treasure room. There is no same-stem QST file and no native
Q/QA hand-in. Membership in the QST area list does not create a quest.

Added an exploration/contact journal with zero invented completion contracts.
Discovering the area is still an achievement; visiting the shop is not a quest
completion. The three contacts are Vella, Padh and Vegar, revealed on encounter.
Review whether exploration-only presentation is useful before designing any new
errand. Treasure-door reset behavior and imported slot-machine objects remain
research follow-ups, with no repairs in this port.

Evidence: [room descriptions](../../areas/wld/bordello.wld),
[characters](../../areas/mob/bordello.mob), [resets](../../areas/zon/bordello.zon),
[shop](../../areas/shp/bordello.shp),
[full audit](quest-story-followups/BORDELLO_AUDIT_REVIEW.md).

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
