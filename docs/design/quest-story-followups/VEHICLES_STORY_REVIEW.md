# Vehicles: source story review

**Scope:** source review of the active `vehicles` area's native Q/QA
contracts and story-relevant shipboard scenes. This classifies written
contracts and their exact item terms; it does not establish runtime
availability, route safety, who can reach a ship, or that any contract is a
required step.

## Source coverage

The active area is `vehicles`, zone 470, with rooms through VNUM 47215. Its
current source set contains 216 WLD room records, 117 mobile prototypes, 53
object prototypes, one QST file with 255 QA contracts, and a ship data file.
The source has no `vehicles.story.json` journal mapping. The 255 contracts
belong to 8 givers and include no `Q`-kind contract; 19 addressable dialogue
blocks provide narrative context and transaction prompts. The contract and
prototype inventory comes from the repository's source-only area inventory
([generator](../../../scripts/zone_story_quest_zone_inventory.py#L38)).

## Finding 1: three parallel shipboard food exchanges

Wulfholm, Bruna Ironkettle, and Thokka Thunderpot each have 82 QA offers:
five land-meat payments and 77 two-fish recipes. Their dialogue distinguishes
crew needs but does not make their exchanges sequential.

| Giver and room | Land-meat payment | Fish recipes |
| --- | --- | ---: |
| Wulfholm, 47129, The Stalval's Rend-Slab | 30,000 for side of boar; 60,000 for each of olyx, caribou, theezak, or lion | 77 |
| Bruna Ironkettle, 47060, The Ironhold Grudge's Anvil's Appetite | 25,000 for side of boar; 50,000 for each of olyx, caribou, theezak, or lion | 77 |
| Thokka Thunderpot, 47186, The Crimson Fulgur's Grog Pit | 50,000 for side of boar; 75,000 for each of olyx, caribou, theezak, or lion | 77 |

The land-meat inputs are `83223` (boar), `83220` (olyx), `83116`
(caribou), `83117` (theezak), and `83201` (lion). The 12 fish prototypes
are `293` bass, `294` piranha, `295` cod, `318` pike, `319` lobster, `330`
crab, `332` shrimp, `333` oyster, `334` clam, `335` mussel, `355` trout,
and `356` perch. Each pair is written as its own two-item QA contract. Per
giver, the 77 fish contracts yield 64 charred skewers (`47100`), 10 seared
skewers (`47101`), and 3 sizzling skewers (`47102`). The exact patterns are
repeated at all three givers, while land-meat payments differ. The native
dialogue calls for feeding ship crews and cooking the offered food; it does not
claim that a player has provisioned a ship or completed a campaign
([Wulfholm](../../../areas/qst/vehicles.qst#L1), [Bruna](../../../areas/qst/vehicles.qst#L1640),
[Thokka](../../../areas/qst/vehicles.qst#L834), [food objects](../../../areas/obj/vehicles.obj#L753)).

## Finding 2: a three-ship rivalry and a second flag exchange

Three NPCs request different pairs from the same three flag objects:

| Giver | Required flags | Main item reward | Shared additional rewards |
| --- | --- | --- | --- |
| Haafgurd, jarl of the Tribe of the Whale | `47042` Crimson Fulgur + `47043` Ironhold Grudge | `47094` ancestral bracer | `55362` scroll, `55033` shopkeepers token, `400233` Exceptional equipment chest |
| Fulgur, the Bridge-boss | `47043` Ironhold Grudge + `47041` Stalval | `47092` cloak of crackling energy | Same three additional rewards |
| Kurgonn Grudgebearer, the Helm-Warden | `47041` Stalval + `47042` Crimson Fulgur | `47093` old signet ring | Same three additional rewards |

Their dialogue frames the fleets as rivals and the requests as joining or
serving a crew. In the WLD/ZON sources, the flag objects are reset into the
captains' inventories on their corresponding ships: Haafgurd at the Stalval's
High Seat (room 47104), Fulgur at the Crimson Fulgur's Bridge-Boss Steering
Platform (47157), and Kurgonn at the Ironhold Grudge's Command Platform
(47066). That placement agrees with the flag identities, but it does not
prove a safe or ordinary route to any captain, a specific combat requirement,
or a player-vs-player objective. The three pairings are alternative local
turn-ins in the source; none asks for all three flags or names another giver
as a prerequisite ([Haafgurd](../../../areas/qst/vehicles.qst#L755),
[Fulgur](../../../areas/qst/vehicles.qst#L794),
[Kurgonn](../../../areas/qst/vehicles.qst#L1605),
[captain resets and flags](../../../areas/zon/vehicles.zon#L539),
[flag prototypes](../../../areas/obj/vehicles.obj#L222)).

A shrouded stranger has one additional QA contract for each individual flag.
Each consumes one flag and returns 250,000 currency units plus one `400235`
Rare equipment bag; its contract carries the native `D` disappearance marker.
The stranger is reset aboard the Ironhold Grudge, Stalval, and Crimson Fulgur.
This is a separate one-flag exchange, not a completion step in the three
captain pairings ([stranger contracts](../../../areas/qst/vehicles.qst#L2404),
[ship placements](../../../areas/zon/vehicles.zon#L523)).

The three captain contracts each award one `55033` token, while Kjelgar's
crafting service consumes tokens. The local item-link scan therefore flags
`55033` as a **candidate** link between the flag rewards and crafting. Other
token sources and whether a player needs any captain reward to obtain a token
are outside this local contract graph; this is not evidence of a required
quest chain.

## Finding 3: Kjelgar's three crafting offers

Kjelgar's dialogue on the Stromvok describes three separate recipes, each
with five material items and 100,000 currency units. The first requires one
shopkeepers token and offers pipe `47095`; the second requires one token and
offers teleportation anchor `47009`; the crystal recipe requires two tokens
and offers the Vokstron harp `47096`. Those token counts appear in the QA
inputs and match the token multipliers stated in the dialogue.

There is one unresolved prototype mismatch. The crystal recipe's five `G I`
terms name object `400941`; no current `areas/obj/` source definition for
`#400941` appears in the active prototype inventory. The active object source
defines a moonstone crystal at `#400094` instead
([prototype](../../../areas/obj/tradeskills.obj#L753),
[active object list](../../../areas/AREA#L406)). The written recipe strongly
suggests a relationship, but the numeric identity must be confirmed by the
builder before any native gameplay change. This review does not substitute
`400094` for `400941` or claim the current recipe is completable
([Kjelgar's dialogue and contracts](../../../areas/qst/vehicles.qst#L2502)).

## Environmental story shape

The area is a transport and shipboard-faction collection rather than one
continuous quest dungeon. It includes the WaveDancer passenger ferry and
galley, the Seaspray, a rickety ferry, the heavily armed dwarven Ironhold
Grudge, the whale-bone Stalval, the storm-orc Crimson Fulgur with its
wyvern-filled Storm-Aerie, and the giant-steered Stromvok. Room text gives
the ships distinct material cultures and purposes: Ironhold's artillery,
cargo-beast hold, and Abbathor-linked crew; Stalval's whale-rib structure,
whale-hunting Tribe, and Jarl's seat; Fulgur's storm-orc crew, ram, and
grog-centered social space; and Stromvok's chain-pit steering system
([Ironhold mess](../../../areas/wld/vehicles.wld#L1204),
[Stalval ribs](../../../areas/wld/vehicles.wld#L2167),
[Stalval high seat](../../../areas/wld/vehicles.wld#L2271),
[Fulgur bridge](../../../areas/wld/vehicles.wld#L3423),
[Fulgur Grog Pit](../../../areas/wld/vehicles.wld#L4054),
[Stromvok Chain-Pit](../../../areas/wld/vehicles.wld#L4696)).

The remaining vehicles and mount prototypes cover carts, carriages, coaches,
ferry tickets, flight tickets, a ferry-ticket automat, and a heavy
teleportation anchor. These objects signal several travel systems, but static
room prose and prototype presence alone do not establish their route graph,
price rules, reachable destinations, or safe return paths. Those questions
remain part of the separate dynamic-reachability audit.

## Builder questions and recommended resolutions

1. **Are the food contracts deliberately parallel across the three crews?**
   Confirm that identical exhaustive fish recipes and the different meat
   payments are intended. Preserve the exact 82-offer family per cook while
   that decision is unresolved; do not collapse or broaden the accepted item
   set from these notes.
2. **Are the captain flag pairings intended as three optional faction
   responses?** Confirm the named flag pairs, individual gear rewards, and
   common token/scroll/chest outputs. Preserve each exact pair and output; do
   not introduce an all-three-flags objective, forced ordering, required
   ship ownership, PvP objective, or new route/access condition.
3. **Is the stranger exchange meant to coexist with the captain rewards?**
   Confirm the one-flag reward for each ship and the `D` behavior. Keep these
   as distinct contracts and do not assume one flag can be reused, respawned,
   or handed in twice without runtime evidence.
4. **Which crystal prototype should Kjelgar's third recipe accept?** Confirm
   whether `400941` is an intentionally supplied/generated object or whether
   `400094` was meant. Do not silently alias, replace, or add a prototype.
5. **Should the sidecar classify these as provisioning, rival-crew flag
   bargains, and shipboard crafting?** If approved, record those as separate
   optional story groups. Do not turn them into one required campaign without
   an explicit design decision and separate review of balance, access, and
   protected READY/accounting boundaries.

## Safe resolution summary

Keep the current source identities and contract boundaries as the evidence
baseline. Record the crystal VNUM mismatch for owner review. Treat the food,
captain, stranger, and crafting offers as distinct source families unless a
builder resolves their intended relationship. No gameplay, item, route,
economy, combat, PvP, access, zone-placement, or migration change is proposed
by this review.
