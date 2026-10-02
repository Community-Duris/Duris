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

## Findings and implementation additions

| ID | Finding / balanced interpretation | Concrete fix or qualification |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | Fresh active-accounting item resets are suppressed, including four-egg supplies, feather/leaf, chest contents, pouch and teleport statues. Recovered objects may remain available. | Qualify exact NPC/container/scenery generation and retry/custody dependencies before an active-world journey. |
| ZSQ-ACCESS-STATE / MAGIC-DOORS | Access depends on spoken keyword, current door flags, exact key target and independently opened paths. Recipe history alone cannot represent it. | Extract reviewed property-driven leads, add live access projection and accepted-unlock/confirmed-arrival adapters. Preserve supplied keys/open routes; do not expose password solutions automatically. |
| ZSQ-ELVISH-KEY-IDENTITY | Both keys share aliases; egg key opens the drider room, terminal key opens two containers. | Revision 2 names each use and adds optional access/preparation checks. Future client/source hints use prototype/instance/target identity, not loose keyword matching. |
| ZSQ-ELVISH-NARRATIVE | Petals versus leaf, dragon versus serpent, and elf transformation go beyond accepted item/effect data. Some wording can be loose narration. | Builders clarify names/prose or explicitly add a confirmed transformation; preserve rewards and original receipts. Journal already follows executable items and actual terminal delivery. |
| ZSQ-ELVISH-POTION-LINEAGE | Two native crafting exchanges support a final delivery; potion-shaped items are also consumable. | Keep preparation services separate. Future conditional recipes and lineage project committed input/output UIDs, partial consumption, gifts and replay; no invented personal collection gate. |
| ZSQ-LEARNED-LORE / SHARED-TRAVEL | Wall examination, echoed dialogue and property-driven teleport are not historical objective events. | Add accepted examination/topic and confirmed travel adapters with explicit recipient, repeat, reveal and cross-area policies. Echoed prose cannot prove everyone nearby learned/completed a stage. |
| ZSQ-ELVISH-SPELLCASE | Magical opening prose has no assigned local quest integration. This may be flavor for ordinary spellbook use. | Builder decides whether to clarify flavor or design a real additional route. Record as unintegrated lore rather than declaring the current item unusable. |

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
