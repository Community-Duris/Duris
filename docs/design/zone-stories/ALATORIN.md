# Alatorin: district progression draft and audit in progress

Priority 26, source area `alatorin`, canonical zone 831. **This is an interim
source review, not a comprehensive dossier or a deployed journal.** All native
Q/M blocks and the shop file have been reviewed. The remaining rooms,
prototypes, reset families, shared handlers and foreign supply paths must be
reviewed before publishing the sidecar and marking this zone complete.

Player discovery, encounters, journals, achievements and new daily eligibility
require active, ready economic accounting. Several prerequisites deliberately
refuse that mode today. Describing a route does not make its sources or payment
supported. Native item delivery proves an accepted exchange; it does not prove
personal hunting, allegiance, travel or every earlier preparation step.

## Reviewed evidence and remaining boundary

The [native source](../../../areas/qst/alatorin.qst) contains 961 blocks:
482 QA, thirteen Q, 285 MA and 181 M. These yield 495 distinct native
contracts across 82 recipients, including 93 contracts with a coin offering.
There are 296 nonambient addressed dialogue blocks. The conservative inventory
reports 291 because five families contain an apostrophe alongside usable
aliases: Odeth's `ardgral`; Ragmor and Wikzor's `strak`/`ash`/`blackrock`;
Ulster's other named proofs; and Gimabon's `grtak`/`leaders`.
Preserve usable commands in authored contacts and the raw source distinction
in the index. An addressed response still needs a separate accepted learned-topic
adapter before it can be historical progress.

The local files contain 952 rooms, 425 mobile prototypes, 602 object prototypes,
3,596 reset commands and fifteen shops. Resets comprise 1,085 M, 856 E,
511 G, 352 D, 342 O, 270 F, 164 P and sixteen R. Their complete review remains
pending; raw inventory counts are not reviewed family coverage.

Completed additional reads at this checkpoint:

- [Rooms](../../../areas/wld/alatorin.wld): 83100–83273 and 83776–83790,
  including every description, field and exit in those ranges: 189 rooms.
- [Mobiles](../../../areas/mob/alatorin.mob): 83100–83209 and 83414: 111
  prototypes, including the separate Steelgrip smelter and depot merchant.
- [Objects](../../../areas/obj/alatorin.obj): 83245–83250, 83457–83458,
  83530–83533, 83605–83607, 83626–83630 and 83677–83698: 42 prototypes.
- [All fifteen shops](../../../areas/shp/alatorin.shp), including stock,
  keepers, rooms, trading types, prices, schedules and restriction fields.
- Both procedures and the completion helper in
  [the Alatorin source](../../../src/specs/specs.alatorin.c); the shared
  attribute-scroll, wondrous-ring, osquip birth/decay and paid-cleric functions;
  the parry handler; the ship-shop dispatcher and relevant mining refusal.
  Their wider dispatch, economic publication and recovery paths still need
  qualification. Existing Winterhaven, Quietus and Jade dossiers provide
  previously reviewed shared-service context.
- Bounded `surfacekeeps` road connections, low-numbered blank paper in the
  loaded `limbo` file, and selected global stock declarations. This does not
  complete the source map of those foreign areas.

The five literal local assignments are miner's helmet 83457, quarterstaff
83605, wondrous ring 83698, ship yard 83786 and doctor 83414. The computed
`VMOB_ALATORIN_STEELGRIP` assignment resolves to mobile 83187 and installs
`smelter`; a literal-only inventory misses it. Foreign osquip mobile/object
120051 use `wh_corpse_to_object` and `wh_corpse_decay`.

## District stories in proposed player order

This is a presentation order, not an enforced campaign. Keep independent
accepted requests visible after encountering their recipient, with services
and optional source routes explained alongside them. Full-family completion
and district pagination remain planned capabilities.

| Order / family | Reviewed progression and exact constraints | Journal treatment |
| --- | --- | --- |
| 1. Inn kitchens and everyday provisions | Four cooks convert particular haunches into three portions plus XP. Stonebrew's clam, apple, garlic and green herb produce its royal dish; BrightAxe uses pike, mandrake root, garlic and orange; the guild soup uses root, garlic, faerie dust and boar meat; Hammerhelm uses garlic, dragon blood, banana and boar meat. | Separate preparations with exact ingredients and outputs. Fish, forage and supplied ingredients are different source routes. Fresh-sounding prose is insufficient for a personal/freshness objective. |
| 2. The royal banquet | Each of four distinct prepared dishes pays the King's attendant for one culinary commendation token. Ten culinary tokens pay Modan for a medal of honor. The attendant also buys the ancient wedding band for 333,000 copper. | Explain a complete banquet route, while each actual dish receipt remains independent. A first dish is not proof that all four courses were served. The band also pays Taark and cannot fund both exchanges with one root. |
| 3. Collecting and the workshop | Dweefniggle has 223 exchanges. Each of 209 salvaged kinds 400000–400208 pays one collecting fragment; nebula material 400209 adds 200,000 copper. Vellum recipe 400210 pays three fragments; ten fragments make one collecting token. Strange object 1250 instead pays a full token and treasure. | A supporting collecting service may share proven equivalent one-item alternatives. Keep different rewards and ten-fragment assembly separate. A second native recipe consumes ten **tinkerer's tokens 83458** for one fragment; its source is still under investigation. |
| 4. Professional commendations and honor | Modan has seven exchanges: ten collecting, dragonslaying, mining, culinary, stealth or siege tokens each pay a medal, with family-specific extra gifts; ten medals pay 500,000 XP and key of honor 83338. Dweefniggle separately buys four medals for gnome currency 55033. | Name every physical token kind precisely. Explain competing uses and optional preparation history. Do not treat a collecting token, a tinker's token, a medal and gnome currency as interchangeable. The key's targets require the remaining room/object audit. |
| 5. Brewery, militia and guild rivalries | Dweefniggle accepts six brewer badges; Barlow accepts six tinker badges; Grendar accepts six barbed swords; Grem accepts six apprentice robes; Maggeynel accepts six hoods. Sturb accepts six silver axes and has twelve additional proof exchanges. Brudo requests Clund's head; Clund requests Brudo's sword. | Distinct requests and actual six-root counts. Several briefings say five. Native proof possession does not prove kills, enlistment or an exclusive side; journal metadata must not invent those conditions. |
| 6. Miboli's ten arcanums | Blank paper plus two of the **same** named component make each arcanum type. Several sylvan, lithic and sanguine materials are alternatives for the same output. One of each of the ten arcanums pays an enchantment scroll and gnome currency. | Preserve each arcanum identity and the ten-kind assembly. A combined live count across alternative materials must not report a mixed pair as a valid recipe. |
| 7. Attributes and wondrous power | Reciting enchantment scroll 55362 outside combat, through its `scroll` command match, replaces it with a random attribute scroll 55352–55360. Electrum ingot plus two **matching** attribute scrolls make one corresponding ring. All nine rings plus 500,000 copper make wondrous ring 83698. | Separate preparation services, nine attribute outputs and the finale. A fresh route needs at least eighteen random scroll outputs, but eighteen draws do not guarantee two of every kind. Supplied matching scrolls/rings remain valid. The paid finale is currently unavailable with active accounting. |
| 8. Mining and master crafting | Smidrin accepts either item 251 or a vellum recipe for a mining token. Harmon needs six **different large ore kinds** for a mining token and another gift. Gimbrin has 55 commissions: six of one exact tempered material, the matching armor part and 50,000 copper; premium metals also need gnome currency. | Distinguish current mining ore, salvaged tempered material and ready armor. Mining and smelting are deliberately unavailable with active accounting. Cold iron and steel can be terminal alternatives for equal equipment, but mixtures are not accepted recipes. |
| 9. Body enhancements, dragonscale and Xamora | Dweefniggle's five enhancements each need the matching armor, two gnome tokens, drug bag and 50,000 copper. Mundorno has 27 contracts, including 22 paid ones, competing common/chromium/silver scales, wings, shadow scales and a Noroth-head reward. Xamora's six commissions need eight of one exact material, two tokens and 200,000 copper. | Equipment services retain exact recipes and different outputs. Review Mundorno's overlapping wing-breastplate recipes before grouping; one demands a gnome token and the other does not. Xamora's material/output prose also needs a builder decision. |
| 10. Libraries, gardens and private royal requests | Three specific books pay the librarian for **wisp of deadly mist 83267**, despite medal prose. Baron Helgrim separately consumes Jedd's book for a medal and treasure. The mystic gardener requires all four named flowers together. The ancient wedding band has two recipients; Noroth's head has Taark, Queen Bellethra and Mundorno alternatives with different outcomes. | Keep exact outputs and competing allocation explicit. There is no receipt-backed requirement that the player personally visited every flower source or completed every royal errand. Royal keys are different physical kinds with different targets. |
| 11. Guild proofs and opposed outposts | Two rogue districts contain separately owned FrostSharn-heart, five-king-weapon, paired-heart, two-banner and map requests. Khoralator and Helgor each accept the other's head and retire. Ulster's independent proof requests, Ravi's two hearts, Locke's insignia, the prisoner key and the lion cub have their own outcomes. | Equal recipes or gifts at different recipients do not establish a shared story identity. Retiring proof recipients need runtime availability and attempt evidence. Narrated membership, healing and escape are separate from what the exchange actually implements. |
| 12. Deramuth Port and competing supplies | Bimk buys psychomia plants for money and drug bags. Naltem accepts five different barrels together for a stealth token, or buys drug bags separately. Jenk makes elemental potions, elixirs and elemental might; his signet-ring mission retires him, potentially ending those services for that appearance. Lundeen's egg soups compete with Modo, Dezik and the glutton. The clerk buys two Jade shipping-crate kinds at different prices. | Name quantity, contents/disposition, source and competing consumer. Brino's eleven buybacks, crystal sales and glutton food exchanges are commerce/services whose grouping needs explicit semantics, rather than a fabricated campaign. Ship and doctor services remain separate from native Q receipts. |
| 13. The First Mountain and divine gifts | Shard 83626 is declared as ground stock at Moradin's bust, room 84045. Two priests return a replacement shard without another reward; seven named gods each return a replacement shard and their own gifts, retiring after the exchange. The priest's sacred-key request has mithril and cash alternatives. Meshadan has token and cash tithe alternatives. | Track actual independent gift receipts and exact old/new shard lineage. A same-kind return does not preserve the old UID. Divine narration is not proof of every temple visit or a single finished pilgrimage. Key-access routes and recipient availability remain under review. |

Other closures need explicit classification in the final map. Odeth's brew
uses faerie dust, dragon blood, strange stone, gnome currency and nebula
material, with **no native coin fee**, despite his quoted price. Jedd trades
four pyrolisk glands and a gnome token for two elixirs. K'tharz accepts a
physical coin object, which differs from wallet currency. Gruneegth's totem,
Taigorus's weapon/heart and Bimk's Eye of the Serpent have separate outcomes.
Borik returns two rejected bandages; his tourniquet exchange grants XP and
retires him, but its entire bandaging sequence is narration and describes
failure. It must not become a successful-healing objective. Several guide,
peacekeeper, flask and potion claims have no matching local terminal contract;
review source execution before declaring them unfinished.

## Access and source findings

Alatorin's interior rooms connect into the separately loaded
[`surfacekeeps` world](../../../areas/wld/surfacekeeps.wld), listed as
1200–1238 in [AREA](../../../areas/AREA). For example, 83100 joins 120667,
and 83103 joins 122103, with real reverse connections. These are not missing
destinations. Native contracts remain owned by their recipient's catalog area;
the future city-family presentation must not award duplicate discoveries or
receipts merely because a path crosses those area boundaries. The client
currently treats that outer range as wilderness and hides ordinary room numbers.
Qualify both canonical discovery and useful city navigation before adding a
district map.

Reviewed interior locks already distinguish the Hammerhelm barracks' key
83198, inner citadel key 83105, private chamber key 83279 and depository vault
key 83630. Queen's reward 83307 and Modan's honor key 83338 are further kinds,
not aliases of those keys. A key offering, live possession, accepted unlock
and actual arrival need separate evidence. Supplying a finale item does not
require replaying the optional route that produced it.

Some useful alternatives are ordinary stock: key 83198 is also carried by its
guard; collecting token 83246 is in a chance-limited container; wedding band
83249 and military letter 83272 are nested supplies; a separate outer-city
carrier has a medal. Alatorin's fragment 83245, honor key 83338, depository key
83630 and enchanted helmet have no selected reset declarations, but native
recipes produce them. **No reset declaration is not the same as no source.**
Tinker's token 83458 needs the remaining producer/custom-source review.

Blank paper 5 exists in active `limbo.obj`, although the playable-area prototype
inventory omits it. Do not describe that as a missing prototype or replace it
with another paper kind. Build inventory/prototype resolution from loaded data,
while keeping administrative areas ineligible for player achievements.

## Required universal capabilities and repair proposals

| Finding | Required implementation and qualification |
| --- | --- |
| ZSQ-ALA-RECIPES: exact alternatives | Add a versioned recipe predicate with explicit any-of complete recipes and all-of exact per-kind counts. Allocate concrete unused root UIDs; never let the same root satisfy two slots. Tests must reject one of each alternative when two/six/eight identical kinds are required, yet preserve legitimate gifted supplies and any supported terminal alternative. Until then use exact single-kind checks or text guidance without aggregate readiness. |
| ZSQ-ALA-DISTRICTS: bounded large journals | Preserve the 256-row, 32-step, 64-item-check and 512-KiB bounds. Inventory safe equivalences before choosing rows; the 209 ordinary one-item fragment trades are an example, while differing rewards, recipients and retirement are not automatically equivalent. Add family/district expansion and pagination to a shared terminal/client projection; encounter visibility and receipt identity remain authoritative. Increase a bound only if a faithful final map actually requires it. |
| ZSQ-ALA-SERVICES: settlement and staged stock | Keep smelting, mining and paid-cleric refusal. A smelter adapter must admit two exact NPC/player-held ores, frozen price, actor, payment ownership and output together, with recoverable refund/publication and no mixing between helpers. Qualify both pay-first and ore-first sequences. Review detached consumed ore cleanup and unchecked output allocation before any re-enable. Cleric purchase and actual spell/resurrection outcome need separate settlement/effect proof. |
| ZSQ-ALA-RANDOM: scroll and power publication | Freeze one random attribute result and consume its exact source scroll only with a recoverable output grant. Gifted outputs need custody, not invented recitation history. Wondrous-ring power changes and helmet `paydirt` use are separate effects; a timer or utterance alone proves no successful power. Test failures, replay, restart, donation and changed equipment. |
| ZSQ-ALA-DECAY: osquip preparation | The osquip death special creates item 120051 in its room, then initializes decay; the periodic consumer replaces expired remains with a generic corpse. Add accepted birth/expiry/replacement lineage and a precise episode clock before optional personal/fresh cooking objectives. Test donation, nested storage, expiry during delivery and reconnect. Retain native exact item acceptance. |
| ZSQ-ALA-CITY: ownership and map visibility | Resolve interior/outer-city membership deliberately. Keep contract owner, current recipient room, visited area and authored district separate. Test movement across both directions, discovered/undiscovered neighboring areas, hidden room-number client behavior and duplicate receipt projection. |
| ZSQ-ALA-CONTENT: builder decisions | Decide the intended librarian gift, five-versus-six badges, attendant/banquet rewards, Xamora prose and Mundorno's overlapping paid breastplate recipes from actual outputs. Quantify supply paths before selecting a source repair for tinker's tokens or foreign proofs. Existing dialogue could be stale rather than gameplay broken; preserve rewards and balance until that decision. |
| ZSQ-ALA-PILGRIMAGE / ALLOCATION | Specify all-stage pilgrimage/banquet/profession campaigns separately from independent terminals. Confirm exact competing item use, replacement shards, retiring Jenk/proof recipients and reset generations. A final gifted item can complete its request while leaving personal journey history unearned. |

The earring-named handler on the wondrous **ring** is not a naming-based defect:
it changes two distinct maximum attributes every ten minutes while worn and
blocks encrusting broadly. Its intended encrust scope needs a targeted dispatch
test before changing behavior. Likewise, actual `earthen`/`flaming` emissary
deliveries do not implement the world transformations described in dialogue.

## Next audit checkpoint

Finish the remaining 763 rooms, 314 mobile prototypes and 560 object
prototypes; review all reset families and complete selected source paths;
resolve every gate, timed/random effect and quest-like orphan. Then classify
all 495 contracts into faithful story/request/service/exclusion rows, author
all usable contacts, publish the sidecar and reproducible index, and add
focused native/source regression cases. Keep the existing catalog and completed
25-zone count unchanged until that work is complete.

Native-file review baseline SHA-256:
`5b2ab0b2587ebaf17cc2b010e1d6e00e18e8984ae15e029df0a33fa8fea08913`.
The full [roadmap](../ZONE_STORY_ZONE_PRIORITIES.md) remains active; Newhaven
follows Alatorin. Played active-world journeys are a separate qualification
stage and have not been run at this checkpoint.
