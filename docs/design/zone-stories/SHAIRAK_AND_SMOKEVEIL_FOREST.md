# Shairak and Smokeveil Forest: comprehensive source story map

Reviewed October 2, 2026. Source area `smokev`, zone 202, journal revision 1.
The review covers all 100 rooms, 74 mobiles, 59 objects, one shop, 435 resets
and 43 native interaction blocks: ten Q, one QA and 32 addressable M families.
The journal classifies all eleven contracts as ten achievements and one service,
with nine contacts and all 32 topic families. This is a source-comprehensive
map; active gameplay qualification is pending.

## Evidence boundary

Reviewed the complete active [native interactions](../../../areas/qst/smokev.qst),
[rooms](../../../areas/wld/smokev.wld), [mobiles](../../../areas/mob/smokev.mob),
[objects](../../../areas/obj/smokev.obj), [shop](../../../areas/shp/smokev.shp)
and [resets](../../../areas/zon/smokev.zon). The
[reproducible index](../../reference/zone-story-audits/smokev.md) retains exact
terms, all topic bodies, prototypes and reset arguments. Actual local rooms
are 20200–20299; the catalog's derived zone span begins at 20148 and is not
evidence that the preceding rooms belong to this area's files.

Resets comprise 254 M, 116 E, 27 G, 26 O and twelve D commands. Reviewed every
reset's identity, chance, cap and parent, including all trophy and recipient
sources. All local item reset rolls are 100%; live caps still matter. Reviewed
full room descriptions/exits/extra descriptions and every prototype's flags,
values and effects. There are no local key, container or item-teleport prototypes.
The native contract inputs have TAKE; these ordinary proofs do not share the
pickup problem found in the Vast Hidden Grove's dead mouse.

No literal special assignment is associated with the actual local prototypes
or room span in [specs.assign.c](../../../src/specs/specs.assign.c). That result
does not mean there are no custom/support roles: the smith and epic teacher
tables register Dorno and Keebo separately. Reviewed their initialization and
dispatch, native offering/reward/removal/recovery, shared reset/pickup/door
execution, shop supply and accounting refusal boundaries. Maintained-source
searches also identify the unused coral-golem handler and CHAOS starter-kit
uses of local equipment. This review keeps those paths distinct from native
quest completion.

Global active reset and Q searches cover the 21 distinct local input/reward
prototypes. Bounded foreign reads cover Grishnak's actual reset carrier in
Tezcat and Ravi's Alatorin continuation with the Raxthan Bloodbeast source.
Those reads do not qualify the complete foreign zones. Static source searches
cannot prove the absence of recovered stock, staff placement or computed
generation.

## Progression families and exact exchanges

Each listed native exchange is an independent terminal. Supplied materials
qualify; its receipt proves delivery, not personal acquisition, every topic,
physical access or the enacted resolution of a narrated conflict.

| Family | Actual progression and reward | Boundary |
| --- | --- | --- |
| Village tribute | Ivar's Sanrabb/Dolgar/lottery topics → heart of Sanrabb 20209 and heart of Dolgar 20200 **together** → limestone bracelet 20207 | Prose asks for hearts in sequence, but the contract consumes both in one offering. Ivar disappears; no earlier-heart stage is implemented. |
| Ivar's revenge | Svartalo's talon 20244 → Ivar → blood stained mask 20249 | Separate from the heart request. Either reward removes Ivar, despite the heart response suggesting an immediate talon follow-up. |
| Dorno's dragon commissions | Dargast's pile of scales 20208 → boots 20206; Dolgar's large scale 20214 → warvisor 20215 | Two independent item-only trophy commissions, with no coin fee. Dorno disappears after either. The named dragon hunt makes these request achievements, consistent with Pine Hollow's trophy commissions; they are distinct from his ore-forging service. |
| Oystein's betrayal | Euronymous topic → exact head of Grishnak 20255 → necklace 20256 and quiver 20257 | The active reset carrier is Diabolus 98961 in Tezcat, not a local mobile named Grishnak. Oystein's receipt belongs to Shairak. |
| Keebo's hunt | Xertrath topic → forest burrow → Xertrath's head 20210 → lugged spear 20238 | The bunny master's reset already carries the head. Native delivery does not prove a personal kill or a special death-item birth. |
| Tarlator and Raltron | Shaman's curse/ingredient topics and room parchment → Tentabeast heart 20253 plus talon 20244 → skull-crested helm 20252 and 181000 XP → Raltron → earring 20258 and 33333 XP | Two separate receipts. Optional producer history explains one helm route without barring a supplied helm. The code does not enact a human transformation, create a separate letter item or confirm a reunion. |
| Ilvorntas's reward | Tentabeast topic → heart 20253 plus talon 20244 → sleeves 20254 | Same inputs as Tarlator, different recipient/reward. The same copies cannot fund both. Completing one never completes the other. |
| Forvos's supplies and family lore | Dragons/Tentabeast/Dorno topics → exact whiskey bottle 20245 → 181 copper | Repeatable supply service; Forvos stays. Drinking, curing alcoholism or reconciling father and son is not a completion predicate. The 181 copper is a reward, not an offering fee. |
| Azcatlipoca's meal | Food/dragons/Dargos topics → actual dead animal 20211 → marble snake 20250 and 9999 XP | Ground source in forest room 20216. It is not a generic corpse, any killed animal, or the dead-animal extra description at 20207. No freshness requirement or local death-item handler is assigned. |
| Ravi's distant vengeance | Ravi's Ivar/hunter topics in Alatorin → another Tentabeast heart 20253 plus Bloodbeast heart 42948 → Baphomet signet 83447, 100000 copper and 85000 XP | Alatorin owns this receipt. His narration identifies Ivar as his father; it does not implement reunion or confer a Smokeveil terminal. Reserve another Tentabeast heart. |

No local native request is omitted or classified as rejection. The ten
achievement rows preserve their exact independent bindings. Forvos's service
retains its native receipt and journal entry, while adding no achievement or
daily unit. The two Dorno commissions remain achievements; crafting language
alone is insufficient to demote a named trophy request to a generic service.

## Sources, access and availability

| Proof | Reset source | Actual limits |
| --- | --- | --- |
| Dolgar heart 20200 and large scale 20214 | Dolgar 20233 at 20249 | Each G has cap one, 100%; mobile cap one. Both proofs are carried stock, not scripted births. |
| Sanrabb heart 20209 | Sanrabb 20235 at 20241 | G cap one, 100%; mobile cap one. |
| Dargast scales 20208 | Dargast 20236 at 20231 | G cap one, 100%; mobile cap one. |
| Bloody talon 20244 | Svartalo 20237 at 20244 | G cap one, 100%; mobile cap one; three local consumers. |
| Xertrath head 20210 | Xertrath 20200 at 20290 | G cap one, 100%; mobile cap one. Aplon is another warren inhabitant, not the head source or a new request. |
| Tentabeast heart 20253 | Tentabeast 20224 at 20296 | G cap one, 100%; mobile cap one; two local consumers plus Ravi. Its water room requires ordinary water-travel preparation. |
| Dead animal 20211 | O ground source at 20216 | Cap one, 100%. Description at 20207 is separate scenery. |
| Whiskey bottle 20245 | Otsurd's shop at 20277 and carried stock on some tavern drinkers | G cap eight, 100%. Not every reset instance of a same-named drinker receives a bottle. Shop inventory includes this exact prototype. |
| Grishnak head 20255 | Tezcat F-selected Diabolus 98961 at 98974 | G cap one, 100%. F changes the current equipment parent; preceding Bard Faust/Vorphalack are not its carrier. |
| Message-bearing helm 20252 | Tarlator's native reward | No separate note object or native message-state test. Raltron accepts the exact helm regardless of the player's producer history. |

Recipients have mobile cap one and spawn at Ivar 20269, Dorno 20268,
Oystein/Forvos 20279, Keebo 20261, Raltron 20272, Tarlator 20267,
Ilvorntas 20265 and Azcatlipoca 20237. All except Forvos have disappearing
contracts. Shared [quest execution](../../../src/world/quest.c) retires a
disappearing recipient and its remaining inventory/equipment after native
publication; it does not move the NPC to the narrated family destination.
Do not promise that multiple contracts at one giver can be completed in a
single encounter or that a removed giver immediately returns.

The talon has three local consumers, so all three requests require three copies
across available episodes/stock. The Tentabeast heart needs two copies for the
local requests and a third for Ravi. Live cap one can prevent another reset
copy while a looted copy remains in circulation. Consumption may free capacity;
it does not guarantee a reset now, an accessible NPC or a valid new accounting
source generation. These are availability/episode policies, not fabricated
time limits on the hearts. Unlike Winterhaven's scripted beating hearts, the
local heart prototypes have zero values and no assigned decay handler.

The twelve D resets establish six ordinary closed-door pairs: inn, Keebo's
cottage, rotten house, Ivar's house, shaman house and shop. No quest key or
password is required for those closed doors. The rabbit hole's raw world state
is 4 with no D reset: [setup_dir](../../../src/world/db.c) masks the field to
its lower two bits, so this is not a loaded secret-door prerequisite. Do not
invent a search/unlock objective from the number or burrow prose alone.
Forest room 20201 and lake room 20297 link to Surface rooms 614187 and 614586.
Actual arrival owns discovery; an exit description does not.

Tarlator's parchment at 20267, Ivar's tapestry, room tomes, paintings, statues,
fountain and shrine descriptions offer lore. They are not carryable proof,
scenery teleports or additional native terminals. The local fountain is drink
scenery and plants are ordinary food. Rushdie's inn name does not establish an
assigned inn special. Record accepted examination only through a qualified
event if a builder later makes it a historical objective.

## Bounded foreign continuations

[Tezcat resets](../../../areas/zon/tezcat.zon) select Diabolus with F 98961 at
98974 immediately before giving head 20255. Reviewed his full prototype,
the catacomb room and the surrounding reset parent chain. The head's Grishnak
identity and Diabolus carrier differ; there is no reviewed Grishnak death birth.
Retain the accepted trophy, show its actual source, and ask the builder whether
the carrier is intentional before moving it or rewriting betrayal content.

[Alatorin's Ravi interaction](../../../areas/qst/alatorin.qst#L6552) and
prototype describe Ivar's injured hunter son, placed at tavern corner 83762.
His QA consumes Tentabeast heart 20253 and Bloodbeast heart 42948 together,
then awards signet 83447 plus coins/experience and removes him. Reviewed the
recipient, room, reward prototype and exact contract. The matching
[Raxthan reset](../../../areas/zon/raxthan.zon#L280) places Bloodbeast 42943 at
42966 carrying 42948, cap one/100%. Reviewed its complete mobile, room and
heart. Raxthan's T'rin also consumes that Bloodbeast heart with demon heart
42940 for cloak 42947 and 60000 XP; this is another independent consumer.
None of these foreign deliveries is added as a mandatory local prerequisite
or automatically completed when a Smokeveil request succeeds.

## Support roles and balanced repair decisions

Dorno 20240 has a twelve-entry legacy smith menu (forge indices 11–22) in
[tradeskill.c](../../../src/economy/tradeskill.c). Normal startup invokes
`initialize_tradeskills` after world assignment and installs `smith` when the
mobile function is empty. Dorno has no earlier shop/literal assignment blocking
that installation, unlike Kordor in Torg. His Q function is separate and retains
the dragon commissions. Smith purchases deliberately refuse active accounting.
That refusal is an appropriate boundary, not a journal implementation defect.

Reviewed the twelve declared templates in
[forge_items.c](../../../src/item/forge_items.c), including their class/wear,
skill/chance and randomized effect fields. The declared ore recipes are:

| Menu / forge index | Declared result | Iron ore pieces |
| --- | --- | --- |
| 1 / 11 | Coarse mantle | One medium, two small |
| 2 / 12 | Coarse sabatons | Three small |
| 3 / 13 | Sturdy gauntlets | Two medium, one small |
| 4 / 14 | Sturdy great helm | Two medium, two small |
| 5 / 15 | Magnificent breastplate | Three large, one medium |
| 6 / 16 | Sturdy visor | One medium, three small |
| 7 / 17 | Magnificent greaves | Two large, two medium |
| 8 / 18 | Magnificent vambraces | Two large, two medium |
| 9 / 19 | Magnificent sabatons | Three large |
| 10 / 20 | Magnificent visor | Two large, one medium |
| 11 / 21 | Sturdy tail ring | One medium, one small |
| 12 / 22 | Sturdy knuckles | Two medium, one small |

The shared price table declares 5000/15000/30000 copper for two/three/four
pieces respectively. These are inactive service terms to qualify, not twelve
new quest achievements or a claim that every menu choice currently succeeds.

The shared smith code checks numeric choice against the smith-table index
instead of this NPC's menu length and looks for ore in the NPC's inventory
while comments describe the player. For Dorno, the table index is four despite
twelve recipes. Before enabling purchases, validate the complete bounded menu,
take exact player-owned ore UIDs, and coordinate coin payment, material
consumption, result grant and recovery. These are source-level defects in the
inactive legacy path; no player-loss incident was observed and no economic
guard was removed. Modern player recipe forging is a separate command path.

Keebo 20242 is registered by [epic initialization](../../../src/world/epic.c)
and the [teacher table](../../../src/classes/epic_skills.c) for Improved Track,
with Track 95 as its prerequisite and a teacher ceiling of 100. Level, class,
prerequisite, increment ceiling and price rules still apply. Purchases explicitly
refuse active accounting until supported settlement. Teaching is not the bunny
quest's reward and should need a successful teaching event if ever journaled.

The unassigned [seas_coral_golem](../../../src/specs/specs.mobile.c) body uses
rooms 20200/20201 and item 20202, with an abalone-earring comment although
the active 20202 is spiked leather armor. Its south command and east-facing
arrival prose also disagree. Only its declaration and definition reference the
handler in maintained source. Treat it as stale inactive content, not a magical
Smokeveil access gate. Confirm its intended owning area and typed item/slot/
destination, then repair and qualify committed travel before any assignment.

[CHAOS equipment profiles](../../../src/account/chaos_eq_data.h) can grant
local hammer, water-breathing ring and sleeves independently of these stories.
The [kit implementation](../../../src/account/nanny.c) uses a coordinated
batch starter grant. Possessing sleeves cannot prove Ilvorntas's reward;
possession-based readiness and committed quest history must stay separate.

For prose/mechanics discrepancies, preserve the real contracts and clarify the
journal now. Builder decisions can later select a persistent giver, explicit
replacement episode, enacted cure/reunion, or retained flavor. Align Ivar's
sequential-heart/immediate-talon language with chosen actual behavior. Do not
delete trophies, move the Grishnak head, manufacture personal-kill credit, or
change recipient disappearance based on a static audit alone.

## Capability additions and qualification

Keep these prerequisites in the shared plan:

1. Committed O/G/E source admission and shop replenishment with durable reset/
   NPC generation identity; fresh active legacy item resets currently refuse.
   Qualify looted cap-one stock, consumption, renewed supply and absent givers.
2. Accepted topic and extra-description events, canonical alias identity and
   actor/room/recipient episode binding. Merely offering topics is not learning
   history; repeat reads and unrelated aliases must not create stages.
3. Explicit all-stage family completion and branch/attempt policy for Tarlator
   then Raltron, and the shared Ivar/Dorno disappearing recipients. Any-terminal
   lists cannot express the full cure or two independent reward obligations.
4. Exact source versus gift lineage and material allocation across three talon
   consumers and local/foreign heart consumers. Bind the actual F carrier, not
   the trophy's name; distinct starter-grant authority must not imply quest history.
5. Availability-aware presentation and explicit builder-owned closure events.
   Receipt history may explain a route; live stock and actual recipient presence
   decide the next possible action. Native removal is not a narrated NPC reunion.
6. Table-driven assignment evidence and deliberate service qualification. Resolve
   missing/overridden handler targets and legacy menu/custody defects before
   introducing forge or teaching stages under active accounting.

Focused checks must cover supplied helm without Tarlator history, spent helm
after producer history, the simultaneous heart pair, independent same-input
rewards, exact animal identity, service exclusion, unchanged foreign progress
and cold state reload. Actual gameplay needs active reset/shop source support
and failure/reconnect/recovery journeys; source/native harness results are not
a completed live expedition.
