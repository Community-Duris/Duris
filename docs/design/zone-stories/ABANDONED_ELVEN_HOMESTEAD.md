# Abandoned Elven Homestead: comprehensive source story map

Reviewed October 2, 2026. Source area `elvish`, zone 358, journal revision 2.
Reviewed all 52 rooms, fourteen mobiles, 28 objects, 138 reset commands,
four native exchanges and six addressable M/MA responses. Two achievement
rows and two preparation services retain all native bindings, with no exclusions.

## Evidence boundary

Reviewed complete active [Q/M source](../../../areas/qst/elvish.qst),
[resets](../../../areas/zon/elvish.zon), [rooms](../../../areas/wld/elvish.wld),
[mobiles](../../../areas/mob/elvish.mob), [objects](../../../areas/obj/elvish.obj),
and the [special assignment source](../../../src/specs/specs.assign.c).
There is no local shop file and no literal local mob/object/room special
assignment. The [generated index](../../reference/zone-story-audits/elvish.md)
provides complete contract/response/reset references. Reviewed shared
[quest execution](../../../src/world/quest.c),
[exit/reset loading](../../../src/world/db.c),
[movement/unlock/search](../../../src/cmd/actmove.c),
[spoken magic doors](../../../src/cmd/actcomm.c), and
[item teleport handling](../../../src/magic/spell_travel.c).
Special-name extraction alone would miss these property-driven mechanics.

The catalog's derived ownership range begins at 35643; actual local rooms
start at 35800. The `A` suffix in MA/QA means room-echoed prose, not an
alignment requirement, saved branch, extra quest, or automatic group milestone.
No ambient `qc_action` response is present in this source.

## Complete progression: unlock the drider's remedy

| Stage | Exact source / accepted outcome | Journal treatment |
| --- | --- | --- |
| Enter the keep | The outer courtyard 35818 north door has key `-2`, keyword `door gate sanctum`, and locked reset state. Shared `check_magic_doors` accepts the final keyword through normal speech. | Exploration guidance. Builder dossier records `say sanctum`, then opening the door; no historical password/arrival achievement. |
| Meet Kalaban | Aging hermit 35801 occupies kitchen 35822. `spiders`, `spider`, or `key` prompts egg lore. | One encountered contact/topic family, not three separate achievements. |
| Obtain the access key | Four eggs 35812 → adamantium spider key 35824 plus nominal 30,000 experience. Five sword-spider reset instances 35802 in bedrooms 35833–35837 each carry an egg. | One request achievement. Exact count four; gifts accepted; source recovery/kill unproven. |
| Reach the drider | Key 35824 opens the locked door north from Hall of Redemption 35827 to War Room 35828. | Optional live key/access guidance. A gifted key or existing open access does not require an egg receipt. |
| Hear his history | Drider 35800 responds to `life`/`quest`, `fighter`/`great`, `help`/`past`, `yes`, and `no`. | Five authored topic families. No saved pledge, refusal or prior-talk gate. |
| Prepare two potion statues | One robin feather 35817 plus glowing ivy leaf 35818 → blood-red manticore 35814 and blue sea serpent 35816. | Visible preparation service; optional earlier receipt in terminal journal, no extra achievement/daily unit. |
| Combine the remedy | Both potion statues to Kalaban → golden engraved dragon statue 35813. | Visible preparation service and optional receipt. Combination alone does not release the drider. |
| Terminal delivery | Golden dragon statue to drider → adamantium-and-mithril spider key 35820 plus nominal 50,000 experience; giver disappears. | One release story. A supplied final statue completes it without local preparation receipts. |
| Follow the reward key | Key 35820 fits runed chest 35821 in War Room and belt pouch 35825 in master bedroom 35836. | Post-reward guidance; opening or looting them is not another Q terminal. |

Native experience policy can adjust/share awards; these are source terms, not
a promise of each character's final payout. Preserve frozen recipient/reward
terms. Personal NPC meetings and personal sourcing remain distinct from credit.

The access key's description says adamantium; the terminal reward says
adamantium **and mithril**. Both share the aliases `key spider`. They are
different prototypes with different targets. Generic keyword or display-name
matching cannot decide whether a player can unlock the door/container.

The drider's `no` response threatens a later encounter, but the native message
does not persist refusal or modify later exchange admission. His ingredient
prose asks for ivy petals; the actual accepted item is glowing ivy **leaf**.
Preparation prose mentions a dragon and manticore; the actual pair is manticore
and **sea serpent**. Final dragon production belongs to Kalaban's exchange.
Journal text follows actual item names and explains the route without silently
changing any world contract or adding a rejection branch.

## Sources, transformation, and closure semantics

The red-breasted robin 35803 at pond 35810 carries feather 35817; golden ivy
35805 at eastern glade 35814 carries leaf 35818. Ordinary NPC equipment/custody
and combat rules apply. The source does not contain a peaceful plucking or
leaf-harvest special. Delivery accepts supplied materials; possessing either
does not authenticate a personally killed robin or harvested plant.

All three small statue rewards are **ITEM_POTION**, with vial aliases and
liquid descriptions. Their recipe names do not make them ordinary sculpture
items. Quaffing them consumes a potion through shared item rules, so the journal
advises retaining them for exchange. Accounting already provides authoritative
native offering/reward transactions; a future recipe-lineage objective must
link exact committed input/output UIDs rather than infer a transformation from
these names or possession. Do not replace working exchange receipts with a
new unqualified grant path.

Final QA prose narrates the drider drinking the potion, becoming a frail old
elf, and rejoicing toward Arvandor. Native execution grants the key/experience
and removes the giver. There is no local spawned elf, authenticated quaff,
escort into Arvandor, or persistent race transformation. This can be intentional
narrated closure. Keep accepted remedy delivery as the achievement; builders
can separately choose prose clarification or a recoverable actor transformation.

Chest 35821 contains treasure 35822, a drow-skin sheath 35819, and skin armguards
35827 through P resets. The master-bedroom pouch is itself the reset object;
there is no local P reset proving additional contents. Its reward-key lock and
the chest lock are actual object data. Post-story loot does not imply another
hand-authored quest or guaranteed treasure inside every described container.

## Lore, travel and optional exploration

The sequence of wall hangings at 35824–35827 describes ancient elven harmony,
Lloth's seduction, war, exile and redemption toward Arvandor. Their moving
scenes are extra descriptions. They do not emit learned-history or witnessed
battle milestones. A builder can author optional lore stages once accepted
examination and reveal policies exist; reading all tapestries is not presently
required by any native exchange.

Three deity statues use **ITEM_TELEPORT 25**, rather than a local special:

| Object / reset location | Actual command and destination |
| --- | --- |
| Lloth 35801, glade 35814 | `grovel` (183) → local entrance 35800 |
| Corellon 35803, pond 35810 | `worship` (197) → local entrance 35800 |
| Solonar 35804, pond 35810 | `stare` (139) → room 30648 in source `solonar` |

Use the appropriate object argument and ordinary actor/teleport admission.
Each has unlimited charge value `-1`; generic travel must confirm the actual
destination before granting any future travel objective. Solonar is a cross-area
travel lead, not an automatic ownership transfer for this area's remedy story.
The active reset-source blocker also applies to these non-takeable objects.

The waterfall route continues from the pond through 35838–35850. Hidden beach
35851 lies east from pool 35849. Its key `-2` entry is reset state **5**, which
means secret and closed but **unlocked**; this differs from the locked outer
gate. Search/reveal/open rules apply. Advertising the gate's speech mechanism
as universally required for every negative-key exit would be wrong.
The charred spellcase 35826 is held by crab 35812 on the beach. Its description
mentions a keyword or magical key, but its actual type is ITEM_SPELLBOOK 33,
with no local Q consumption or assigned opening procedure. Treat it as
equipment/lore until a builder supplies a supported recipe or custom integration.

Elven protectors, forest creatures, fish followers, food supplies, the old
brazier, other sculpture/containers, spidersilk clothing and Spider Bane weapon
remain ordinary exploration/encounters. No extra kill-all-spiders, sanctuary
restoration, spellcase liberation or tapestry campaign is executable here.

## Direct area-owner questions and conservative recommendations

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Which public title and command-target aliases should identify this area? | The source title is “Abandoned Elven Homestead”; the dossier maps native aliases and computed arrival context while command ownership remains separate. See the [zone header](../../../areas/zon/elvish.zon#L2) and [evidence boundary](#evidence-boundary). | Keep the current title and `elvish` stem. Add aliases only when a live command binding and its owner are confirmed. |
| **ZSQ-RESET-AUTHORITY.** What source-generation, parent, and retry evidence should establish fresh local stock under active READY accounting? | All 78 local O/P/G/E item rows are currently refused before creation; recovered objects may still remain. Legacy P uses a global matching-container lookup, and parent/cap distribution can differ. See [stock and custody analysis](#stock-crypt-access-and-source-custody). | Qualify exact NPC/container/scenery generation, parent UID, and retry/custody dependencies before promising a live source. Do not infer renewal from declarations alone. |
| **ZSQ-ACCESS-STATE / MAGIC-DOORS.** Which spoken key/keyword, current door state, and actual arrival should count as access? | The sanctum gate uses a spoken keyword and current flags; the egg key, terminal key, and independently opened paths have distinct targets. See [the progression](#complete-progression-unlock-the-driders-remedy) and [access semantics](#stock-crypt-access-and-source-custody). | Track accepted unlock and confirmed arrival separately from recipe history. Preserve supplied keys and already-open routes; do not expose password solutions or create mandatory history gates. |
| **ZSQ-ELVISH-KEY-IDENTITY.** How should the two “key spider” prototypes be distinguished in directions and tracking? | The egg-access key and terminal reward share aliases but are different prototypes and target the room door versus chest/pouch. See [key identity notes](#complete-progression-unlock-the-driders-remedy). | Use exact prototype, instance, and target identity rather than loose keyword/name matching. Keep current optional access and preparation checks. |
| **ZSQ-ELVISH-NARRATIVE.** Which differences are loose narration, and is a lasting elf transformation actually intended? | Ingredient prose says ivy petals, but the recipe accepts a leaf; production prose mentions dragon/manticore while inputs are manticore/sea serpent; the ending describes an elf transformation without spawning one. See [native progression](#complete-progression-unlock-the-driders-remedy) and [closure semantics](#sources-transformation-and-closure-semantics). | Clarify names/prose or explicitly define a confirmed transformation. Preserve existing rewards and accepted receipts; do not infer a rejection branch or actor conversion. |
| **ZSQ-ELVISH-POTION-LINEAGE.** Should the two preparation exchanges remain services separate from the final release receipt? | Two native crafting exchanges make potion-type statues, which can also be consumed by ordinary quaff rules; each exact recipe has its own inputs and outputs. See [accepted progression](#complete-progression-unlock-the-driders-remedy) and [transformation semantics](#sources-transformation-and-closure-semantics). | Keep preparation services separate. If lineage matters later, link exact committed input/output UIDs and preserve partial consumption, gifts, and replay without adding personal-source prerequisites. |
| **ZSQ-LEARNED-LORE / SHARED-TRAVEL.** Which successful wall examination, topic response, or teleport arrival should become actor-specific history? | The journal has orientation hints but no native topic receipt; property-driven teleports require exact object/argument admission and actual arrival. See [evidence boundary](#evidence-boundary) and [travel/control analysis](#lore-travel-and-optional-exploration). | Add only accepted examination/topic and confirmed-arrival outcomes with explicit actor, repeat, reveal, and cross-area policies. Nearby echoed prose does not prove another character learned or completed a stage. |
| **ZSQ-ELVISH-SPELLCASE.** Is the charred spellcase’s magical-opening description only flavor, or should it have a supported additional interaction? | Spellcase #35826 is an ITEM_SPELLBOOK with no local Q consumption or assigned opening procedure despite its prose. See [optional exploration](#lore-travel-and-optional-exploration). | Treat it as ordinary spellbook equipment until a supported recipe or custom integration is authored. Do not label the existing item unusable or invent an opening route. |
| Should exact supplied materials, direct potion delivery, and existing unlocked access remain valid without personal source or ordered-topic history? | Native Q/A accepts supplied exact inputs; neither native endpoint requires a personal kill/source, puzzle, unlock, rescue, combat, planar-travel, or prior-topic receipt. See [exact accepted progression](#complete-progression-unlock-the-driders-remedy). | Preserve supplied-proof eligibility and independent native receipts. Keep access/preparation optional and do not make current source history a new prerequisite. |

Revision 2 makes local egg/access/statue preparation optional in the terminal
journal, preserves both achievements and both services, and explains exact
counts, source alternatives and reward-key uses. Full source mapping is complete;
active-world qualification remains open in the
[shared plan](../ZONE_STORY_INTEGRATION_PLAN.md).

## Journey matrix

Qualify outer speech unlock plus open/arrival; concealed webs/kitchen; four-egg
collection/gift and access key; supplied key and already-open War Room; all
five drider topic families and repeated `no`; feather/leaf gifts versus direct
recovery; both preparation services; supplied terminal statue without earlier
history; failed/missing/wrong similarly named offerings; consumed potion versus
retained remedy. Services alone must not complete release. Test both key
variants against door, chest and pouch, exact source output identities,
post-reward loot, all original receipts/replay/cold restart, repeat reset actors
and daily policy. Qualify deity travel and hidden-beach search separately;
confirm no personal kill, learned lore or actual elf spawning is inferred.
