# Northern Lakes and Settlements: comprehensive source map

Reviewed October 4, 2026. Zone 752, `nlakes`; roadmap priority 64.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligation recovery is separate.

The [schema-three journal](../../../areas/story/nlakes.story.json) maps all
six accepted native outcomes as independent stories, with thirteen contacts,
twenty addressed topic aliases and nine optional checks (seven current-material
checks and two earlier delivery receipts). The central progression is Maur's
heart → Tamara's packaging → Aerin's receipt → Tamara's return-note reward.
Alongside it, the lost human needs Syria's scroll, the green dragon needs the
Ruzdo shaman's quest vial, and Artek needs two scales plus the demon's amulet.
Exact supplied items work without personal source, kill or producer history.

**Separate shipped repair:** [5a2b93d6d](https://github.com/Community-Duris/Duris/commit/5a2b93d6d387e192ea501d6d6e5bbf71796e927b)
changes exactly two words in pile-of-bones room 75263: the cathedral exit now
says east and the pasture exit west, matching actual reciprocal routes.
News: **“Northern Lakes' pile-of-bones exits now correctly point east to the
cathedral and west to the pasture.”** Original clues fail the focused source
regression; corrected clues, reciprocal targets and exact native-byte scope
pass. Live LOOK/traversal remains unqualified. This is separate from journal
and implementation-plan work; no quest recipe, actor or balance repair is claimed.

## Reviewed evidence

- All fourteen [native blocks](../../../areas/qst/nlakes.qst): eight M and
  six Q. Seven addressed response families retain all twenty aliases. Tein's
  `qc_action 45` block is periodic ambient narration, not an addressed topic.
  All six outcomes belong to the five local recipient kinds; no additional
  owned terminal or foreign physical giver was found.
- All 219 consecutive [rooms](../../../areas/wld/nlakes.wld), 75200–75418:
  138 exact prose groups, seventeen headers, twenty-eight non-exit metadata
  groups, 139 numeric exit families and 264 text/keyword pairs (263 nonempty).
  Full descriptions, properties, exit texts and every family membership were
  reviewed. No room has an empty description. The broader registry bounds
  75172–75418 do not create additional physical rooms or source evidence.
- All sixty-three [mobile prototypes](../../../areas/mob/nlakes.mob), eighty-
  two [objects](../../../areas/obj/nlakes.obj) (75266 is absent), and all 414
  [resets](../../../areas/zon/nlakes.zon), in 309 exact argument families and
  226 parent-aware families: M166/E115/D50/P31/G23/O20/F5/R4. Both full raw
  resets and parent/equipment/source relationships were reviewed. Header
  75418 2 0 20 30 1 preserves reset mode two. All checked local reset and
  ordinary exit targets resolve; no positive door key is declared.
- Both [shops](../../../areas/shp/nlakes.shp), Bron at 75219 and Melbh at
  75283, plus imported canoe/raft/rations/water/bandage prototypes and stock.
  All five native recipient kinds are sentinel/stay-zone. Rabbit 75252 and
  leopard 75253 can leave the zone. No literal local special, actual teacher
  flag or inn/arena terminal adds a quest. ACT_TEACHER32768 registers the
  shared teacher; ACT_SPEC_TEACHER2147483648 is an aggression property, not
  another teacher-registration path.
- Five ordinary reciprocal foreign boundaries, with full foreign rooms:
  Winterhaven 55297↔75410, Teka extension 75546↔75264 and Surface
  537871↔75418, 537889↔75337, 539488↔75363. Bounded global material producer,
  consumer, reset, shop and literal-assignment scans found no other local
  producer or foreign quest for these local input kinds. The one foreign
  consumer of Artek's reward is Aevenyl's Winterhaven Q2333. All 713 active
  teleport prototypes were scanned; no foreign fixed destination enters these
  physical rooms. No local fixed teleport object was found.
- Shared quest allocation/ownership credit/retirement, parent-aware reset
  admission, ordinary boat movement, door setup/reset state, command-time
  falling, current movement on arrival and command, item type/equipment effects
  and random-world-quest exclusion were considered. Lack of a local special
  alone is not proof of lack of shared behavior. Exact contracts and daily
  classification remain in the [generated index](../../reference/zone-story-audits/nlakes.md).

## Exact stories and courier progression

| Recipient/block | Exact native offering → reward | Journal treatment |
| --- | --- | --- |
| Lost human 75239, [Q17](../../../areas/qst/nlakes.qst#L17) | I75252 → I75263; D1 | Scroll-delivery story; mace reward and recipient departure. No escort, actual recall spell or home-arrival endpoint. |
| Artek 75254, [Q44](../../../areas/qst/nlakes.qst#L44) | Two I75271 + I75215 → C250000 + I55287; D0 | Three-root combined dragon/demon story; visage continues to foreign Aevenyl. Empty native Q response is documented below. |
| Green dragon 75255, [Q64](../../../areas/qst/nlakes.qst#L64) | I75274 → I75273; D1 | Exact quest vial earns a dagger and recipient departure; flight/curse language is scene closure, not a separate tracked event. |
| Tamara 75260, [Q101](../../../areas/qst/nlakes.qst#L101) | I75268 → I75281; D0 | Heart-order preparation; packaging acceptance completes only this stage. |
| Aerin 75261, [Q126](../../../areas/qst/nlakes.qst#L126) | I75281 → I75280; D0 | Independent middle delivery, with optional Tamara preparation receipt. |
| Tamara 75260, [Q110](../../../areas/qst/nlakes.qst#L110) | I75280 → I75279 + C10000; D0 | Independent final return-note reward, with optional Aerin receipt. |

The order matters to the narrative, but supplied exact bottles and notes remain
valid. A packaging receipt never restores a consumed bottle or completes the
middle/final requests; a middle receipt never restores a spent note or grants
the final earring. Existing story contracts use any accepted bound definition
for completion, so combining all three contracts into one story would falsely
complete the whole chain after the first handoff. Keep six independent credited
units and explain their relationship. Future parent campaign summaries require
explicit all-stage semantics and must preserve legitimate supplied-item routes.

The native requests do not require recorded expedition rescue, shaman death,
blue-dragon death, personal scale extraction, scroll casting, dragon flight or
Aerin's experiment. Native narrative can explain those intentions without
inventing additional accepted gameplay. Addressed eligible visible dialogue
helps the player understand requirements; aliases are not separate achievements,
and periodic Tein narration is not credited learning.

All six definitions remain repeatable and potential daily candidates under the
existing all-item/reset-two heuristic. This does not prove renewal, admitted
supply, cap availability or recipient presence. The lost human and green dragon
leave after acceptance; the other recipients stay. Recipient extraction/reset,
equipped input recovery and durable offering interruption need actual journeys.

## Declared material sources and overlapping roles

| Exact kind | Source/reset | Practical distinction |
| --- | --- | --- |
| Scroll of recall 75252 | Syria 75249 at ice sheet 75292, M455/E457, hold18, cap1/100 | TREASURE8, zero spell values, quest delivery identity; it is equipped on the source and must become a loose exact offering. |
| Demonic amulet 75215 | Red demon 75206 at vault 75213, M294/E295, neck3, cap1/100 | Actual ARMOR9; the hidden cellar route leads to the vault. Preloaded possession does not establish a personal demon kill. |
| Dragon heart 75268 | Maur 75209 at lair 75216, M306/G308, cap1/100 | TRASH13 preloaded heart; held blue egg 75218 is separate. Blue-dragon/expedition prose does not create personal kill provenance. |
| Green scale 75271 | Green dragon 75255 at 75393, M539/E540 shield11; old dragon 75256 at cave75411, M561/G562; cap2/100 | SHIELD37, same exact kind on both sources. Native allocation needs two different roots, not one verified original from each dragon. |
| Quest golden vial 75274 | Ruzdo shaman 75244 at hut75372, M503/E505, hold18, cap1/100 | TRASH13 with zero potion values. Same targeting words as ordinary golden vial75225, which is POTION10 in bookcases; only quest75274 is accepted. |
| Bottled heart75281 / note75280 | Tamara Q101 / Aerin Q126 only in reviewed producers | TRASH13 / TREASURE8 with zero values. Neither experiment completion nor editable player-written paper is required. |
| Mace75263 / dagger75273 / earring75279 | Human Q17 / dragon Q64 / Tamara Q110 | Rewards do not fabricate the other requests, original-source proof or travel readiness. |
| Dragon visage55287 | Artek Q44; imported [Winterhaven prototype](../../../areas/obj/wh.obj#L3579) | Its `_noquest_` targeting alias excludes random-world-quest selection; it does not prevent the explicit Aevenyl native recipe. |

Green dragon75255 and old dragon75256 share `green dragon` keywords, but have
different native roles and locations. Helping the green dragon retires a declared
scale carrier. Killing that recipient to recover its scale also prevents using
that instance for the vial request. Current quantity and optional history cannot
resolve that real actor/source conflict. Plan source order and reset availability;
do not silently duplicate the scale, remove disappearance or demand a kill.
Builder review must determine whether recovery/reset or an intended peaceful
scale route should resolve the conflict. No impossibility or repair is claimed.

`P` resets stock local containers/bookcases; their items are not additional native
terminals. `F` loads a follower, sets `tmp_mob` and groups it with the last M
leader; `R` loads a mount and mounts `tmp_mob` or the last M leader without changing
the stored M leader. It does change current `mob`; all four local R declarations
are followed by M/F before any G/E, so no item is implicitly assigned to a mount. Shared [execution](../../../src/world/db.c#L3952)
uses the actual current `mob` for G/E. All nine follower/mount declarations were
reviewed in context; Tein's guard/mount and other scene groups remain supporting
world content, not escort/mount achievements.

## Access, current movement and falling

Most lake/river rooms use WATER_NOSWIM: eighty-three rooms. Other sectors are
inside61, field38, city23, forest13 and water-swim1. Actual
[ordinary movement](../../../src/cmd/actmove.c#L966) recognizes carried/worn
ITEM_BOAT22, suitable character/mount flight/levitation and other supported
alternatives. Bron declares canoe429; Melbh canoe429/raft431. One-person and
primitive-raft prose is not a sea-going ship occupancy or capsize contract.
Boat presence does not block current exposure. Existing current speed also
permits ordinary movement into water under the shared movement check.

All seventeen current properties are on water-noswim rooms. Creek75234–75237
uses C45 south. River75309/75311 C29 east,75310 C29 down,75312 C35 down,
75313 C30 east; river75331/75333 C30 south,75332/75334/75336 C30 down,
75335/75337 C30 west,75338 C15 west. Both
[arrival](../../../src/world/handler.c#L1516) and
[command dispatch](../../../src/cmd/interp.c#L1909) can attempt current movement.
They test water/current/chance and fly/levitate state; the command path also
excludes petition. The warning precedes `do_move`, so text alone does not prove
accepted swept arrival or survival. Qualify blocked exits, recursion/multiple
currents, mounts, interruption and publication before a typed current objective.

Three properties start chance falling at command time: ladder75266/75267 F10
and well75275 F23. The [loader](../../../src/world/db.c#L1337) reads F/C fields
and disables falling when no usable downward target exists. Actual fall admission,
climb catches, flight/mount alternatives and interrupted survival remain separate
from visiting a room. Bridge75392 is SINGLE_FILE8192; vestibule75264 is
DARK+HEAL131073. No local room is SAFE, INN or ARENA. Healing is recovery support,
not a guarantee of safe passage or a quest endpoint.

Door pairs75202N↔75203S,75211D↔75212U,75248D↔75249U and75403E↔75404W use
worldkind5 and D-state5: closed/secret without a declared key lock. The well
exit75274N→75414 is closed/secret, while75414S→75274 uses worldkind1/D-state1,
closed/unlocked without the same secret declaration. Worldkind5 is parsed with
state&3; it is not automatically pickproof. D-reset5 adds EX_SECRET and low-bit
closed state. [Door reset](../../../src/world/db.c#L4062) does not clear secret
flags merely because a later declaration omits them; actual lifecycle still
needs qualification. Finding/opening, selected side, accepted entry and return
are separate; old quest receipts cannot establish current access.

The five ordinary foreign boundaries preserve neighboring ownership. The frozen
passage reaches Surface537871; Winterhaven and Teka extension routes do not
create local courier acceptance. Aevenyl55132 at Winterhaven55161 has
[Q2333](../../../areas/qst/wh.qst#L2333): I55287→I55040+E250000, D0. Her
addressed Artek hint explicitly sends the player here. The prior Winterhaven
journal and exact request remain unchanged. Artek's local receipt, visage
custody or cross-zone travel does not complete her foreign request.

## Capability gaps and balanced repair plans

| Evidence or limitation | Plan and acceptance boundary |
| --- | --- |
| Three independent courier handoffs; current story completion is any bound terminal | Preserve this six-unit map. Design optional parent campaign/all-stage display after actual mixed supplied/personal journeys; historical receipt must never refill current materials or grant skipped credit. |
| Shared scale kind versus dialogue's one-from-each intent; overlapping recipient/source | Qualify accepted acquisition, actor/item UID/origin/zone, direct source versus handoff and cap/reset admission. Builder decides whether distinct-source or peaceful-source integration is intended; preserve current exact supplied delivery compatibility. |
| Native D retires human/dragon while prose narrates recall/cure/flight | Existing accepted offering is sufficient for these stories. New typed actor actions need committed item consumption, live recipient identity, extraction/effect/arrival and interruption/replay evidence; text or disappearance alone cannot establish every narrated action. |
| Current warnings precede attempted movement; chance falls happen at commands | Qualify accepted cause/entry/exit/blocked/reset/return/current effect and survival episodes before travel objectives. Current-item or earlier reward checks cannot express alternative effect support or successful travel. |
| Artek Q44 response is empty | Builder-confirm a concise accepted-bundle/reward/visage-continuation response after testing actual generic reward feedback. Recipe and reward exist; empty bespoke prose is not proof that the quest fails. No wording repair ships. |
| Tamara's blue-dragon/expedition narrative versus heart preloaded on Maur | Qualify actual source/body identity; review intended story wording or source design. Preserve exact accepted heart and supplied routes. Do not replace it with a new blue-dragon kill requirement or claim a rescue implementation. |
| Melbh shop data says Kelbh in its opening message | Opening-message dispatch is commented out in [shop code](../../../src/economy/shop.c#L2315). Data naming mismatch is inactive intent evidence, not a live opening bug. Correct/re-enable only with an isolated builder-approved wording/announcement task and actual audience/hours qualification. |
| Daily supply and actor renewal are static classifications | Exercise all six accepted durable offerings, equipped/loose conversion, cap2 scales, D1 extraction/recipient reset and interrupted/cold recovery with active accounting. Recovery must not activate disabled discovery or grant new credit. |

The separate two-word direction repair is the only native change in this
checkpoint. All other proposals remain pending, with no unqualified claim that
custom quest code is broken or impossible. Actual later repairs need clear
isolated fix commits, prominent PR trigger/before-after/validation limits and
accurate news wording; keep them distinct from story mapping and plans.

## Builder questions and conservative recommendations

| ID | Direct builder question | Source evidence | Conservative recommendation |
| --- | --- | --- | --- |
| NLAKES-Q01 | Should Artek's two-scale request require one scale from each dragon, or remain satisfied by any two exact scale items? | Artek's native bundle accepts two copies of item 75271 ([Q44](../../../areas/qst/nlakes.qst#L44-L51)); the green and old dragons carry the same item at separate resets ([source rows](../../../areas/zon/nlakes.zon#L539-L562)). | Preserve the two exact-item requirement and supplied-item eligibility. Keep distinct-source or peaceful-source credit separate unless an owner approves a specific accepted source event. |
| NLAKES-Q02 | What stock and recipient renewal policy, if any, should support six repeatable or daily candidates? | The zone uses reset mode 2 ([ZON header](../../../areas/zon/nlakes.zon#L3)); the lost human and green dragon contracts retire their recipients ([Q17](../../../areas/qst/nlakes.qst#L17-L25), [Q64](../../../areas/qst/nlakes.qst#L64-L74)), while scale supply is shared between capped sources ([material/source review](#declared-material-sources-and-overlapping-roles)). | Keep all six as potential candidates only. Do not promise or enable daily availability until actual source, stock, recipient reset, and interrupted recovery policies are explicitly approved and qualified. |
| NLAKES-Q03 | What accepted success response should Artek give for the three-item bundle, and should it mention the visage's Winterhaven continuation? | The native Q44 response body is empty although its exact inputs and outputs are defined ([Q44](../../../areas/qst/nlakes.qst#L44-L51)); Aevenyl's visage exchange is a separate Winterhaven contract ([Q2333](../../../areas/qst/wh.qst#L2333)). | First confirm generic reward feedback. If bespoke text is approved, describe only the accepted local bundle and its actual reward; keep Aevenyl's separate request explicitly unfinished. |
| NLAKES-Q04 | Is Tamara's blue-dragon and expedition wording intended as background, or should the heart source/story be clarified? | Q101 accepts heart 75268 ([Q101](../../../areas/qst/nlakes.qst#L101-L110)); Maur carries that exact heart in the local reset ([Maur and heart](../../../areas/zon/nlakes.zon#L306-L308)), and the source conflict is documented in the [material review](#declared-material-sources-and-overlapping-roles). | Preserve the current accepted heart and lawful supplied route. Do not add a blue-dragon kill, rescue, or personal-source requirement from dialogue alone; clarify the wording only after the owner selects its intent. |
| NLAKES-Q05 | Does the lost-human story end at the accepted scroll handoff, or should actual casting or home arrival be a separate objective? | The local Q17 accepts the exact scroll and retires the human ([Q17](../../../areas/qst/nlakes.qst#L17-L25)); the current contract boundary is summarized in [exact stories](#exact-stories-and-courier-progression). | Keep the existing handoff as the local completion. Track any cast, escort, destination, or arrival outcome separately, with an accepted effect and safe endpoint; do not infer it from the reward or disappearance. |
| NLAKES-Q06 | Should any local courier or expedition story require a current or falling route, and what successful travel result would count? | Current movement can be attempted on arrival or command, and falling is command-time chance behavior ([current arrival](../../../src/world/handler.c#L1516), [current command path](../../../src/cmd/interp.c#L1909), [fall setup](../../../src/world/db.c#L1337)); the local movement limits are summarized in [access and movement](#access-current-movement-and-falling). | Keep the exact native item exchanges independent from unapproved travel gates. A new route story needs accepted arrival, effect, survival, return, and usable alternatives, not only a warning or room visit. |
| NLAKES-Q07 | Should the three Tamara/Aerin handoffs remain separate outcomes, or is an all-stage parent campaign intended? | The native mapping has three independent handoffs and six accepted outcomes ([contract table](#exact-stories-and-courier-progression)); their current completion boundary warns that a parent story would need explicit all-stage semantics. | Keep all six credited units independent. Add a parent summary only with an explicit stage-order and supplied-item policy that cannot grant stages the player skipped. |
| NLAKES-Q08 | If Melbh's shop announcement is enabled in the future, should its opening text say Melbh rather than Kelbh? | The shop data announces Kelbh ([SHP](../../../areas/shp/nlakes.shp#L68)), while the shop handler's opening-message dispatch is commented out ([shop code](../../../src/economy/shop.c#L2315)) and the mobile identifies the vendor as Melbh ([MOB](../../../areas/mob/nlakes.mob#L605)). | Treat the mismatch as dormant data, not a live announcement defect. If an owner enables that message, confirm its audience and hours and correct the intended name in a separately scoped content change. |

## Title, nearby areas, and tracked history

A color-stripped, whitespace-normalized scan of all 442 current `.zon` titles
finds one exact “Northern Lakes and Settlements” owner: active `nlakes`
(*752–754; [title](../../../areas/zon/nlakes.zon#L2), [world index](../../../areas/AREA#L270),
[QST index](../../../areas/qst/AREA#L241)). Similar current titles are
separate contributors: “Northern Wilderness” (`northern_wilderness`, *122–123),
“The Dark Stone Tower of the Northern Realms” (`teka2`, *755), and the distinct
Harpy, Kobold, and Shaughin settlements ([Northern Wilderness](../../../areas/zon/northern_wilderness.zon#L2),
[Teka](../../../areas/zon/teka2.zon#L2), [Harpy index](../../../areas/AREA#L151),
[Kobold index](../../../areas/AREA#L4), [Shaughin index](../../../areas/AREA#L34)).
Shared northern or settlement wording does not identify an alternate Nlakes
contributor.

A scan of all 446 current WLD files found all 219 Nlakes room VNUMs only in
`nlakes.wld`. Type-scoped scans found the 63 local mobile VNUMs only in that
MOB file and the 82 local object VNUMs only in that OBJ file; no other source
file defines those room or prototype identities.

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `nlakes` stem
from the 2009 `areas/` relocation. Their later edits remain on those paths:
QST through 2013, MOB/SHP through 2014, OBJ/ZON through 2015, and WLD through
2026, including the October 2026 two-word direction-clue correction. The
revision-one story sidecar was added on 2026-10-04. No alternate-stem
predecessor, tracked rename, or whole-zone replacement appears in the bounded
history. No `nlakes` candidate appears in the attic dependency register; keep
the active contributor in place.

## Validation and remaining qualification

Focused exact-source checks protect six native bindings, separate courier
ownership, quantity-two scales, quest/ordinary vial identity, source equipment,
ambient topic exclusion, physical boundaries and the unchanged foreign recipe.
Actual C++ projection journeys exercise discovery/encounter visibility, worn
versus loose readiness, supplied final notes without earlier receipts, missing
current items after historical stages, six independent outcomes, wrong-owner
rejection, replay and cold recovery. They exercise journal behavior; they do
not execute native quest offerings, currents or NPC extraction.

Full production catalog/source and all-map/schema/file-loader regressions,
accounting tracking/feature gates, daily projection, maintained build,
formatting/whitespace and local/source links are required. Preserve all 2668
native definitions, fingerprint/content revision two, registry and earlier
eighty-four maps. Catalog: 85 maps/1625 achievements/1459 potential dailies/2207
rows. Original queue: 64/220 complete,156 pending; Kobold Settlement next.

Actual material acquisition/handoff, three-root committed offerings, courier
mixed routes/rewards, peaceful scale sourcing, D1 extraction/renewal, original
source versus supplied items, boat/current/fall access and survival, Aevenyl's
foreign completion and daily renewal remain unqualified. No DB/account/server
operation, migration, deployment or merge is part of this source checkpoint.
