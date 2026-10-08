# Winterhaven: comprehensive source story map

Reviewed October 2, 2026. Source area `wh`, zone 550, journal revision 2.
The review covers 599 rooms, 314 mobiles, 483 objects, thirteen shops,
1,284 reset commands and all 571 native interaction blocks. The 221 Q/QA
contracts remain classified as 135 request rows, 84 service rows and two
exclusions. This is a source-comprehensive map; active gameplay qualification
is still pending.

## Evidence boundary

Reviewed the complete active [Q/M source](../../../areas/qst/wh.qst),
[rooms](../../../areas/wld/wh.wld), [mobiles](../../../areas/mob/wh.mob),
[objects](../../../areas/obj/wh.obj), [shops](../../../areas/shp/wh.shp)
and [resets](../../../areas/zon/wh.zon). The
[reproducible index](../../reference/zone-story-audits/wh.md) retains exact
contract terms, topic bodies, prototypes, reset arguments and literal special
assignment locations. Q/QA comprise 217 Q and four QA blocks. Dialogue comprises
205 M and 145 MA blocks: 191 addressable families and 159 ambient `qc_action`
responses. Thirty-nine addressable default bodies are empty; the journal offers
152 nonempty topic families across 92 named contacts instead of promising an
answer to the empty defaults.

Resets comprise 887 M, 126 D, 106 G, 74 E, 54 O, 36 F and one P. Reviewed all
loaded door states, object properties/extra descriptions, mobile descriptions,
load chances/live caps and M/F-selected equipment parents. F changes the current
mobile: follower-carried prisoner hearts and rare equipment do not belong to the
preceding M parent. Ground/source prose alone does not establish actual custody.

The [assignment source](../../../src/specs/specs.assign.c) contains 92 literal
assignment candidates associated with Winterhaven prototypes or its room span.
The extractor now handles whitespace and line breaks around resolvers and field
names, recovering the previously missed 21-object animal-decay chain. Macro
aliases for the three beating hearts still require manual resolution. Commented
assignments and functions remain distinct from active handlers.

Reviewed relevant native offering/reward/recovery and shared room/reset, door/key,
object pickup, scenery teleport, wandering and service dispatch. Reviewed the
active local control flow in [specs.winterhaven.c](../../../src/specs/specs.winterhaven.c),
including death births, decay, scroll transforms, gift opening, guards/janitors,
spell help and assigned reward powers; combat display calls are not new quest
stages. The Leviathan, cleric service, generic switch and artifact effects were
checked in their owning implementations. World-quest and ship service behavior
uses the shared execution reviewed in the [Quietus dossier](QUIETUS_QUAY.md).
Storage/boards and actively refused pet/gambling services are support surfaces,
not native story completion evidence. No local smith-table entry implements
Bisoti's narrated wood craft.

Global active reset/Q searches identify foreign material sources and consumers;
bounded foreign reads resolve cloak/heart births, different tribal totems,
containers and missing prototypes. These reads do not qualify every foreign zone.
Static absence of a reset or literal grant is an unresolved source lead, not proof
that recovered stock, staff placement or computed generation never exists.

## Progression families

These routes explain the stories without making earlier receipts mandatory for
native delivery. Supplied materials are accepted; a Q receipt proves the actual
exchange, not personal acquisition, every topic, access or campaign closure.

| Family | Reviewed progression and boundaries |
| --- | --- |
| Wildlife and basic supplies | Matching animal death → freshly killed animal item → either Grenthal's 1,000-copper sale or Monfit's tanning with a 100-copper fee → specific material. Grenthal has 21 sales; Monfit has 22 tanning contracts including the pregnant puffadder. The same copy cannot serve both consumers. |
| Fish and snow-ogre keys | Four separate carp skins → Enfil's fish oil → Layla's centaur semen → Mangrel's baby centaur → chief key. Each chief key breaks after one unlock; four still-locked shaman doors need four keys and hence four babies by this route. The baby exchange is independent of the chief's disappearing four-locket request. |
| Animal clothing | Monfit's exact tanned output → eighteen paid Grendilyn clothing recipes. The down coat requires both duck and duckling down. Four distinct feather pieces → ornate mask is an item-only finale; rare Jade and Venan fabrics → two raw silk threads plus 250,000 copper is a separate request. |
| Silk | Rare fabrics → Grendilyn's two raw threads → one raw thread plus one named dye at Melfist → colored thread → exact leather base and 50,000 copper at Seyb. Eight colored-thread services, eight garment services and a returned-raw-thread rejection are separate. Monfit sells the leather bases; Melfist sells dyes. Same-name colors/outcomes are not interchangeable. |
| Elemental alchemy | Each Incarnate leaves one corresponding material; Lancer's gem-studded hide exchange also supplies all five kinds. One material plus one empty bottle → one potion at Bipplewizzlille. The set of five distinct potions → two elixirs of power at Bipple, or a separate potion at Chibbleniffle. Completing both consumes two sets. |
| Prison and palace | Three distinct marks from Frenbar, Tarn and Arthel → Sir Espet's signet and immunity letter → Thyz's prison key → eight distinct prisoner hearts → Rosenthal's palace key, ancient sheath, 250,000 copper and 325,000 experience. Hearts belong to individual M/F prisoners. The receipt does not prove guards honored immunity or all prisoners were personally defeated/liberated. |
| Snow-ogre shamans | `stare painting` at Ohjal → chief's key/baby exchanges → four separately locked diagonals → matching tribal totem exchanges → four different lockets → chief's blessing → Ohjal's smoke cloak and fury → Sootfoot's Titan recipe. Each shaman has a 25% reset roll and live cap one. Their real Q recipes have no 1,000-platinum fee. |
| Thieves | Psychomia treant death → plant → Buzzbeef's bag and 100,000 copper → Thyz's invitation → hidden passage → Bojaxx's key → locked north entrance → Frenbar. Three distinct child/mother/father spirits → mark of Death, middle finger and 500,000 copper; that mark differs from the mark of the dagger Frenbar wears. |
| Timeless dragon | Xola briefing and shimmering portal → projection's time/dragon topics → Tiamat/Bahamut/Dragonnia beating hearts → three ordinary key seals → mortal Drahknalloth's heart reward: earring and sinew → Xola's ring, 2,000,000 copper and 250,000 experience. The Chronomancer's scepter → Drahknalloth's dragon-skin quiver is independent. |
| Ambassador memories | Forty-nine distinct memory deliveries each award one shopkeepers token, one strange enchantment scroll and 1,000,000 copper. Forty-eight have active reset sources; Ultarium's memory is a native engineer reward. The actual carrier/ground room and owning area are named in journal hints. Each delivery remains independent. |
| Enchanted jewelry | Strange-scroll recitation → one random attribute among nine → two matching attribute scrolls plus electrum ingot → one of nine Branay earrings. All nine different earrings plus 500,000 copper → wondrous-power earring. Random draws need repeatable, recoverable lineage; possessing an earring does not establish how it was made. |
| Blueprint improvements | Three tokens → Chibbleniffle's blueprint → exact plain weapon/armor plus named ingot → one improved piece at Kzantha/Davril. Seven weapons and eight armor pieces are separate services; each consumes another blueprint. Weapon/armor/blacksmith merchants supply the specific base, hilt/flint and metals. |
| Sootfoot artifacts | Each of fifteen independent commissions consumes its exact improved base, binding orb, weapon hilt or armor flint and named distant artifacts. Three distinct key molds at Chibbleniffle produce the orb directly; the key/chest story is not that contract's reward. Earlier crafting receipts remain optional. |
| Mystic and royalty | Six blood-red orbs worn by mighty wizards → gnomish mystic above the bazaar → three vitality potions and golden key. Rosenthal's palace key opens the eastern palace door beside the mystic. Later royal-vault access is not represented by this receipt; the currently loaded palace contains Snowman and rubble. |
| Independent distant requests | Enfil's three hearts → sacred-speed cloak; Ohnagra's moonstone and two relics → bracelet; Aevenyl's dragon visage → backsheath; Urthylyl's jade heart → Venom; Thorl's three seasoned-warrior skulls → skull sheath, which the Domain ambassador also consumes. Exact supplied trophies qualify without personal kill history. |
| Lancer and Sierra | Lancer accepts the Zarbonesti seal, Troggahn's egg, gem-studded hide and blueprint in separate contracts. Sierra accepts intact or broken Lanella hearts for two Wrath or two Truth elixirs respectively. Native egg/blueprint rewards take precedence over Lancer's unrelated wand/mask prose. His snake-necklace contract references a missing reward prototype and remains excluded. |

## Sootfoot's exact recipes

All recipes additionally consume one binding orb 55209 and their own base/hilt
or base/flint; these supplies cannot be counted twice across commissions.

| Result | Improved base | Connector | Additional proof |
| --- | ---: | ---: | --- |
| Illithid axe 55205 | Bronze axe 55160 | Hilt 55283 | Mind 55206, body 55207, soul 55208 |
| Ra 55210 | Electrum dagger 55157 | Hilt 55283 | Two different shattered-ray items 55270 and 55271 |
| DeathSeeker 55211 | Mithril spiked mace 55159 | Hilt 55283 | Mark of Death 55145 |
| Titan 55277 | Adamantium hammer 55161 | Hilt 55283 | Snow-ogre fury 55276 |
| Living Legend gauntlets 55280 | Bronze gauntlets 55162 | Flint 55284 | Living Legend story 55279 |
| Demon Slayer 55301 | Platinum bastard sword 55156 | Hilt 55283 | Euronymous/Bel/Juiblex femurs 55281, 55282, 55336 |
| Blur 55304 | Electrum shortsword 55158 | Hilt 55283 | Different same-name mists 55302 and 55303 |
| Volo 55306 | Platinum longsword 55155 | Hilt 55283 | Volo's essence 55305 |
| Fame 55307 | Adamantium plate 55163 | Flint 55284 | Immortal egos 55308 |
| Vampires 55309 | Mithril helmet 55164 | Flint 55284 | Vampire fangs 55310 |
| Abyss 55311 | Platinum boots 55165 | Flint 55284 | Darkness 55312 and another Leviathan scale 55313 |
| Storm 55315 | Mithril legs 55166 | Flint 55284 | Ten distinct maelstrom items 22631, 34541, 75856, 76050, 76066, 76243, 76634, 82553, 82554, 82555 |
| Cosmos 55318 | Mithril arms 55167 | Flint 55284 | Two copies of cosmic dust 55319 |
| Indomitable tower 55320 | Adamantium tower 55168 | Flint 55284 | Snowman essence 55321 |
| Saints 55322 | Adamantium buckler 55169 | Flint 55284 | Three saint auras 55323, 55324, 55325 |

Storm is thirteen offering roots and fits the current fourteen-root limit; no
capacity change is needed for that exchange. Duplicate cosmic dust still needs
two distinct item UIDs. Adryv's darkness recipe consumes a scale; following it
and making Abyss requires a second scale. Some artifacts (including the second
ray, mists, egos, fangs, dust and Snowman essence) lack a confirmed active static
producer in the reviewed source paths. Keep their delivery contracts and exact
proofs, but do not invent a personal acquisition route or promise a live source.

## Access, availability and custom dependencies

The painting's teleport command is 139 (`CMD_STARE`), not `enter` or a bitmask.
Its return gateway uses command 7 (`CMD_ENTER`). Shared
[teleport execution](../../../src/magic/spell_travel.c) compares the command
exactly and permits unlimited uses at value[2] = -1. Record a travel stage only
after an actual successful destination change. Merely examining an extra
description or typing a travel verb is insufficient.

The chief's key 55235 has a 100% break chance. The four diagonal front doors
are locked by D resets; reciprocal back doors start closed. The chief's four-
locket QA disappears him, so replacement-key planning precedes that finale when
needed. Supplied lockets or already open doors can bypass this preparation.
The journal's four-baby check is optional, not a historical prerequisite or an
assertion that every door is currently locked.

The three dragon hearts are actual key items. Their birth procedures set
approximately three mud days of individual lifetime and disable ordinary key
breakage. Heart admission has no family-wide first-kill timestamp or noon rule;
the narrative describes a stronger time ritual than Q admission enforces.
Decay replaces a heart with 55024 across carried, worn, ground and nested
placements. The failing output-load branch advances the counter below zero,
so it needs a retry/recovery decision. Bahamut/Tiamat also release existing
inventory/equipment before the new heart; the exact death episode, actor and
custody operations must remain distinct.

Fresh animal birth publishes a same-VNUM object to the death room and permits
the ordinary corpse as well. Only 55500–55520 have assigned decay replacements;
pregnant-puffadder, Incarnate and psychomia objects are births with duration
values but no equivalent decay assignment. Do not implement a universal timer
solely because value[0] was assigned. The animal decay routine handles inventory,
room and container placements, but has no worn branch. Native pickup uses the
TAKE bit on these fresh objects; generic corpses are a different type.

Ambassadors generally have two 100% source placements, in Alatorin's distribution
room 55005 and Winterhaven's 55400, with live cap two. Their current location and
wandering matter. The Kingdom of Torg memory has an O source, but no ordinary
active M/F reset for its ambassador 55274 was found. Volo is placed in the Surface
Realm; Adryv is in Sea Caves. Encounter and receipt ownership follow the
Winterhaven prototype/contract, while location/source ownership follows the
actual foreign room or reset.

Chibbleniffle has cap one, with placements on Evermeet and the Winterhaven roof.
No ordinary incoming exit, scenery-teleport property or maintained literal route
to room 55627 was found. The roof's contraption and gnomish chest exist, and the
chest contains an orb, but its key 55327 has no confirmed ordinary source. The
three-mold Q rewards an orb, not that key. Keep the usable Evermeet lead and ask
builders whether the city entrance/key is retired, incomplete or intended to be
restored; do not insert a guessed upward exit.

The book switch in Aevenyl's guild uses shared ITEM_SWITCH execution to clear
EX_BLOCKED on the hidden passage. It does not itself clear EX_SECRET. Journal
plans must distinguish switch activation, finding/revealing an exit and entering
it. Guard/janitor/support behavior does not make those three one achievement.

## Findings and proportionate repair plans

| Finding | Source conclusion | Planned repair/qualification |
| --- | --- | --- |
| Lancer gift | `make_gem_gift` creates and prices an object, then returns zero. `lancer_gift` retires the input first and twice publishes the null result. Checked publication rejects null; no live crash is asserted. | Return/check the created object, verify the selected gift, freeze both gem outcomes and prices, commit input retirement with both outputs, and print success after publication. Test rejection, missing prototypes, replay/crash, foreign/supplied input and wrong target. |
| Scroll outcomes | Strange enchantment and legend scrolls are retired before unchecked direct random publication. Their command handlers match generic keywords rather than independently verifying the selected exact scroll. | Use selected UID/source/context, frozen random output and committed input/output lineage. Test duplicate commands, two same-name scrolls, custody changes, failure/retry and reconnect. |
| Heart/animal lifecycle | Death and replacement lack committed actor/source/transform events. Heart missing-output retry and animal worn placement need explicit policy. | Adopt actor/NPC-generation-aware birth plus UID/deadline/placement-aware replacement; preserve intentional per-object differences and independent heart timing. Do not award personal-source credit to gifts or unknown origins. |
| Mixed payments | Forty-nine native contracts include item-plus-coin offerings: 22 tannings, 18 clothing recipes, eight silk garments and one final earring. Active durable Q admission deliberately refuses them. | Implement atomic exact-item retirement, coin debit, fee/reward distinction and all outputs, with a durable attempt/receipt. Keep the current unavailable presentation until qualified. Coin rewards on item-only requests remain supported. |
| Missing or ambiguous sources | Torg ambassador, city-roof entrance/key and several artifact inputs have unresolved ordinary routes; some displayed items are only lore or unused prototypes. | Validate actual entities, active reset parents, computed/foreign sources and recovered stock. Builder decides restore/retire/clarify; retain exact contracts without promising invented supply or world changes. |
| Prose/terms | Volo says three cloaks but QA requires two; Lancer's egg/blueprint contracts differ from wand/mask dialogue; Davril names a different helmet metal; Incarnate prose says two drops while birth creates one. Some mob/map lore describes an absent king/vault. | Compare each exact contract/handler first. Propose targeted prose corrections or reviewed content restoration, keeping valid alternatives, rewards and receipt identities. Do not uniformly rewrite native rewards or assume every lore discrepancy is a bug. |
| Optional reward effects | Several advertised powers are commented/unassigned. Living Legend's combat-periodic branch follows an earlier unconditional periodic return, so that branch is unreachable in the reviewed control flow. | Builder reviews intended active powers and resolves the unreachable branch separately, with focused effect tests. Equipment receipts remain valid without claiming that every advertised effect ran. |
| Service boundaries | Cleric, pet and gambling services explicitly refuse active accounting; money exchange directs players to BANK. Boards/storage and world quests are independent namespaces/services. | Qualify only services selected for integration, using committed fees/effects/refunds/attempts. Preserve deliberate refusals and existing shared handlers; no gambling or board interaction becomes a story daily. |
| Authoring capacity | Revision 1 was already close to the 256 KiB sidecar bound. Detailed 219-row guidance exceeds it. | Raise the source sidecar bound to 512 KiB with exact boundary regression, retaining per-text/objective/story limits. Plan district/family authoring and projection for larger areas; avoid changing native identities to save bytes. |

## Capability and verification boundary

Revision 2 preserves all 219 existing IDs, native bindings, categories, two
exclusions, production definitions and the 135-unit achievement denominator.
It adds source/location and recipe guidance, 38 optional preparation checks,
complete nonempty addressable topic leads, precise named colors/marks and
accounting-dependent availability. Reading or holding materials does not create
history. Recorded preparation cannot recreate consumed stock.

Universal follow-up needs district/family grouping, conditional recipes, shared
material reservation/counts, exact source identity, accepted topic/examination/
switch/access/travel events, expiring attempt families, recoverable random multi-
output transformations and effect/recipient closure policy. A sidecar can author
these semantics, but inference from prose alone cannot safely activate them.
The [expanded plan](../ZONE_STORY_INTEGRATION_PLAN.md) owns these adapters; this
zone should qualify representatives before builders apply them across the game.

Accounting must be genuinely active before publishing player journals, discovery,
encounters or new daily credit. Offline mapping remains available. Source/parser/
projection tests do not establish fresh active reset supplies, personal kills,
timed journeys, atomic fee execution, palace repair or every foreign campaign.
Those require actual committed gameplay/recovery cases after source authority
and the relevant adapters are ready.
