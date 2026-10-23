# Rift Valley Jungle: comprehensive source map

Reviewed October 4, 2026. Zone 800, `rftjngle`; roadmap priority 53.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready economic accounting is required for discovery, encounters, journals
and new achievement/daily credit. Frozen obligation recovery is a separate concern.

The [schema-three journal](../../../areas/story/rftjngle.story.json) classifies
all 28 native exchanges as twelve story outcomes, twelve supply services and three
excluded referral/refusal contracts. The woodcarver and crystal-dragon contracts
are alternatives for one outcome. Forty-nine contacts and thirty optional checks
(28 material/count checks, two earlier receipts) explain what current code can
prove. Seventeen native daily shapes remain unchanged; eleven authored story
candidates remain after service classification and alternative grouping. The
fifteen-feather batch and seven mixed payments stay guarded under active accounting.

## Reviewed evidence and boundaries

- All [65 native blocks](../../../areas/qst/rftjngle.qst): 28 Q and 37 addressed
  M families. All aliases, narratives, inputs, rewards and retire flags reviewed.
- All 470 [rooms](../../../areas/wld/rftjngle.wld), 325 exact prose groups,
  34 numeric-header groups, 69 non-exit metadata groups and 267 exit families.
  Jungle floor/canopy, thieves, rivers, caves, Moonhollow, elemental cages,
  connected load rooms, mountain pass and giant caves were all read.
- All 220 [mobiles](../../../areas/mob/rftjngle.mob), 220
  [objects](../../../areas/obj/rftjngle.obj), eight
  [shops](../../../areas/shp/rftjngle.shp), and 828
  [resets](../../../areas/zon/rftjngle.zon) in 499 families: M411, E139,
  O121, G96, D42, F11, P8. No R-mounted loads are declared locally.
- Both literal [world-quest hosts](../../../src/specs/specs.assign.c#L762),
  their [procedure](../../../src/specs/specs.world_quest.c), shared
  [assignment/reward engine](../../../src/world/world_quest.c), native
  [offering and extraction](../../../src/world/quest.c#L1517),
  [search](../../../src/cmd/actobj.c#L9653),
  [item portals](../../../src/magic/spell_travel.c#L934),
  [teacher binding](../../../src/world/db.c#L2764),
  [teacher behavior](../../../src/classes/epic_skills.c#L424), movement,
  falls/current data, reset ownership and active foreign boundaries reviewed.
- [Generated review index](../../reference/zone-story-audits/rftjngle.md)
  is navigable evidence, not a substitute for this semantic review.

Catalog interval 78851–80469 follows the registry; physical rooms are
80000–80469. Zone reset mode two and header `80469 2 0 25 35 1` are preserved.
Three Woodseer approaches are reciprocal (16950/16951/16952 to
80000/80001/80002). The southeastern trail 80469 ↔ current surface 568516
is reciprocal. Mountain pass 80398 goes DOWN to Pine Hollow 16008; no return
exit from that target to 80398 was found. This alone does not establish that
the route is erroneous or that a new reverse exit should be created.

Connected staging room 80444 has foreign exits N25405 (fire), E23250 (water),
S24501 (air), W23805 (earth) and up/down jungle routes. The elemental ranger
is not sentinel or stay-zone, so those loads permit ordinary wandering between
owners. His prototype/source ownership remains Rift; his actual location can
be foreign. Other connected floor/canopy rooms seed ordinary wanderers; they
are not missing spawn scripts merely because their names mention loads.

The 80460 SOUTH exit targets absent active room 228371. That room exists in
inactive historical `surf.wld` and `surface2011.wld`, and historically returns
NORTH to 80460. Current `areas/AREA` loads `surface`, not either historical
file. This is a concrete stale boundary reference, separate from the valid
current surface entrance. Confirm the intended modern coordinate before a
native route repair; do not infer a replacement by proximity alone.

## Exact native recipes and classifications

All listed coin amounts are native base units (1,000 per platinum). D1 means
the accepted recipient departs; D0 means it stays. Alternatives are OR, not
two required deliveries. No source ownership, recipe or native identity changed.

| Q line / giver | Actual inputs → actual rewards | D | Journal classification |
|---|---|---|---|
| 29 / 80070 | 3× 80067 an elongated couatl egg + 80065 a couatl-hide wrist guard → 80066 a burnished couatl-hide bracer | 1 | story / couatl-stolen-eggs |
| 65 / 80071 | 80067 an elongated couatl egg → 80067 an elongated couatl egg | 0 | excluded / non-credit |
| 113 / 80080 | 2× 80021 a tiger skin + C: 10000 → 80023 a full-suit of tigerskin armor | 0 | service / rei-tiger |
| 100 / 80080 | 2× 80077 the skin of a red alligator + C: 10000 → 80080 a suit of red alligatorskin armor | 0 | service / rei-red-alligator |
| 89 / 80080 | 2× 80078 skin of a giant alligator + C: 10000 → 80079 an alligatorskin tunic | 0 | service / rei-giant-alligator |
| 125 / 80080 | 80024 skin of a white tiger → 80024 skin of a white tiger | 0 | excluded / non-credit |
| 135 / 80134 | 80134 some thin, soft bark → 80135 a blanket of soft bark | 0 | service / bark-blanket |
| 153 / 80136 | 80137 some bright red flower petals → 80138 a red warmask | 1 | story / painter-red-pigment |
| 187 / 80147 | 80144 a strange parchment → 80145 an odd-looking brass tool | 0 | story / engineer-lost-plans |
| 234 / 80148 | 80024 skin of a white tiger → 80025 white tigerskin boots | 0 | service / leather-white-tiger |
| 219 / 80148 | 3× 80077 the skin of a red alligator + 3× 80078 skin of a giant alligator → 80139 some azurian hide armor + 80140 some azurian hide sleeves + 80141 some azurian hide leggings | 0 | service / leather-mixed-skins |
| 251 / 80149 | 80058 the poisonous skin of a dartfrog → 80059 a vial of poison | 0 | service / shaman-dart-poison |
| 242 / 80149 | 80142 some crumpled green leaves → 80143 a green potion | 0 | service / shaman-green-potion |
| 279 / 80155 | 80154 Summerstorm's silver amulet → 80155 a couatl-hide mask | 1 | story / chief-summerstorm-proof |
| 333 / 80156 | 80157 a large, twisted staff with a crystal head → 80196 a crystalline bow | 1 | story / dragon-crystal-staff |
| 350 / 80157 | 80157 a large, twisted staff with a crystal head → 80196 a crystalline bow | 1 | story / dragon-crystal-staff |
| 366 / 80158 | 80172 a flaming medallion → 80173 a flaming javelin | 1 | story / release-fire |
| 382 / 80159 | 80170 a watery amulet → 80171 some odd-shaped water drums | 1 | story / release-water |
| 400 / 80160 | 80166 a strange amulet of swirling air → 80167 a light and windy cloak | 1 | story / release-air |
| 417 / 80161 | 80168 an earthen amulet → 80169 a mask of earth and stone | 1 | story / release-earth |
| 495 / 80175 | C: 5000 + 80028 a chunk of mithril ore → 80199 a small mithril hammer | 0 | service / teglaron-mithril |
| 474 / 80175 | C: 5000 + 80060 the thick hide of an elder couatl → 80061 a suit of couatl hide armor | 0 | service / teglaron-elder-hide |
| 485 / 80175 | C: 5000 + 80062 hide of a young couatl → 80063 couatl-hide boots + 80064 a couatl-hide eyepatch | 0 | service / teglaron-young-hide |
| 507 / 80175 | C: 5000 + 80075 the shell of a giant turtle → 80076 some turtle shell plate armor | 0 | service / teglaron-turtle |
| 615 / 80209 | 5× 80053 a quetzel feather + 5× 80054 a quetzel feather + 5× 80055 a quetzel feather → 80057 a quetzel feather cloak | 0 | story / weaver-quetzel-cloak |
| 647 / 80210 | 80149 an ancient, silver sword → 80177 a green longsword named "Orcslayer" + C: 25000 | 1 | story / mistwalker-ancient-sword |
| 707 / 80211 | 80150 golden rod of the lizardman chieftain → 80203 a gold leaf | 1 | story / scout-chief-proof |
| 699 / 80211 | 80153 the bloody head of the lizardman chieftain → 80153 the bloody head of the lizardman chieftain | 0 | excluded / non-credit |

## Progression, source recovery and story meaning

**Couatl eggs.** Elder 80070 starts in canopy 80180 and wants three eggs
80067 plus drow wrist guard 80065. Egg O placements are 80130 (25%), 80145
(70%), 80152 (50%), 80193 (25%), 80200 (99%), with global kind cap five.
Three matching eggs suffice; three distinct caches are not required. The
drow hunter wears the guard in connected load 80441. Eggs and guard are secret
prototypes: search/reveal as appropriate precedes recovery. The young couatl
accepts and returns the same egg *kind*, referring to the elder. That is optional
history, not a second achievement or proof of the same UID. Elder-hide crafting
is a separate paid service and is not a prerequisite to helping the elder.

**Cave artisans.** Bark 80134 O80046 becomes the old woman's blanket service.
Painter petals 80137 O80220 are a real canopy item, unlike decorative flowers.
His mask exchange ends his presence but does not repaint room descriptions.
Engineer parchment 80144 O80333 is secret in Moonhollow guard quarters despite
his “somewhere in these caves” clue. No theft/transport event explains that
placement; journal guidance names the real source without rewriting plot intent.
Leaf 80142 O80023 has 30% admission/cap one; dartfrog skin 80058 comes from
canopy frog loads. Shaman conversions do not prove applying either potion.

**Skin crafts.** Reiwiggy's three ten-platinum crafts are guarded mixed recipes.
Two ordinary tiger skins differ from the white kind. His white-skin rejection
returns that kind; cave leather-worker acceptance is independently valid.
The six-skin service needs 3×80077 and 3×80078, not six interchangeable skins.
Rewards call themselves azurian hide even though actual inputs are alligator
skins; this mismatch needs builder confirmation, not an invented azurian input.
Red/giant alligator skins come from their respective river mobile G loads;
ordinary tiger skins from 80033/80035 floor G loads; white skin from 80034
in connected floor load 80431. Shared aliases do not make kinds interchangeable.

**Opposing requests.** Chief 80155 accepts Summerstorm's silver amulet 80154,
not a head. Summerstorm G admission is 40%. Scout 80211 accepts golden rod
80150 (chief E admission 30%). Actual bloody mangled head 80153 G admission
100% is refused and returned. Asking for heads and accepting tokens is a
presentation/intent mismatch; no personal kill proof is currently required.
Both branches are independent. Helping the chief is not a prerequisite to
helping the scout; neither requires betrayal history or enforces allegiance.
Chief's narrative plans to raid do not actually launch a raid. Source chance,
current custody and the accepting recipient's availability govern either branch.

**Dragon's staff.** Moonhollow lieutenant 80173 G80157 starts at 80323.
Woodcarver 80156 M80152 is the reachable recipient. Both woodcarver Q333 and
dragon 80157 Q350 exchange that exact staff for crystalline bow 80196, D1.
Crystal dragon 80157 has no declared active reset anywhere in the current
inventory and no assigned transformation procedure. Group the native alternatives
as one outcome while explicitly describing that missing live route. The native
exchange extracts the recipient; it does not spawn a dragon or relocate one
to another plane. Granite carving 80156 describes another world without a
causal trigger. The ordinary broken staff 80033 is not valid proof.

**Four elemental rescues.** Ranger 80212 M80444 receives four distinct G
amulets 80166/80168/80170/80172 at 100%, cap one each; his equipped javelin
80173 is ordinary loot, even though it also matches the fire quest reward kind.
The ranger can wander to foreign planes. Current item possession does not prove
which source or player supplied it. Each silent imprisoned elemental accepts
its matching amulet, creates its own reward and is extracted. No dialogue M
family, four-rescue campaign terminal or actual home-plane relocation exists.

Portal O/control pairs are:

| Element | Jungle origin / item / command → cage | Paired cage return |
|---|---|---|
| Fire | 80061 / 80158 / UP (5) → 80162 | 80159 / DOWN (6) → 80061 |
| Water | 80072 / 80160 / UP (5) → 80164 | 80161 / DOWN (6) → 80072 |
| Air | 80060 / 80162 / ENTER (7) → 80161 | 80163 / ENTER (7) → 80060 |
| Earth | 80071 / 80164 / UP (5) → 80163 | 80165 / DOWN (6) → 80071 |

They are ITEM_TELEPORT, infinite charges (-1). Direction portals are keywordless;
ENTER uses the actual named object. The portal handler checks matching command,
charges/arena rules and target, then calls the shared arrival path. It does not
require a rescue receipt or simulate home-plane movement. Native plane sector
and movement/survival conditions still require live qualification. Decorative
iron amulet 80206 is ITEM_TRASH, with no opening, four-piece assembly or Q
dependency despite its compartment lore. Do not claim opening it releases anyone.

**Teglaron's services.** Each asks five platinum with its one material. Elder
hide 80060 G80070; young hide 80062 only in one 80071 G load at 80430, not
every young couatl instance; mithril 80028 O80201 at 10%/cap one; turtle shell
80075 G80076. These are guarded crafting services, not story dailies. His wall
axe 80200 is untakeable ITEM_TRASH with no code-word procedure; the cookbook,
dragon skull, weapons and shop stock are lore/ordinary commerce, not quests.

**Feather cloak.** 5×green 80053 + 5×yellow 80054 + 5×red 80055 yields 80057.
Three bright-quetzel 80053 G loads have green feathers and two have yellow;
three red-quetzel 80054 G loads have red, all in connected canopy load 80440.
Each feather kind cap is ten. Names all say “a quetzel feather”; aliases and
long descriptions reveal color. One reset's live stock need not supply five
of each, so renewal/shared supply matters. This is a named unfinished weaving
request; the fifteen roots exceed the fourteen-root durable maximum. Display
three separate counts, retain the recipe, and keep acceptance/daily eligibility
guarded until the entire batch can settle. No partial-progress crafting is native.

**Ancient sword.** The chief's F-loaded aid 80154 wears ancient silver sword
80149 in throne room 80266. Mistwalker 80210 starts at connected 80442 and
can wander; Q647 gives Orcslayer 80177 plus 25 platinum and extracts him.
Ghost-wife reunion appears in native narrative; no live ghost creation or escort
endpoint was found. This proves delivery/departure, not resurrection, speech
comprehension, restored family or active sword effects. Supplied exact sword fits.

**Lore and roles.** All 37 M families are guidance. The memorial names, historic
battle, halfling aid, bereavement, school, bard songs and Summerstorm's sadness
have no independent completion endpoints. Teachers are 80170, 80175, 80176, 80186,
80187, 80195, 80196, 80197, 80198, 80199, 80205, 80210; DB binds generic teacher
if no existing function, and ASK level is class-sensitive. Named schoolteacher
80178 has no ACT_TEACHER bit. Bartenders80123/80194 instead have explicit
world_quest functions. Two shops can host procedural tasks; ordinary buy/sell
still uses shopkeeper paths, not additional static Q achievements.

Procedural quests use persisted actor/start/giver/target/type/zone and kill
counts, with policy-selected foreign targets, paid generation/map and abandonment
callbacks. ASK and death dispatch use matching target state. Reward commit
rechecks source/player/item publication before SQL finish and reset. The ordinary
quest command displays these alongside zone journals; generated assignments
remain excluded from static story denominators. Mercenary coin rewards above
level 24 are currently guarded under active accounting. Their integration needs
accepted assignment/target/count/payout/abandonment events with episode identity;
merely asking a bartender's keyword cannot produce a stable local story receipt.

## Capability plan and balanced pending findings

1. **Full batch settlement:** extend root capacity across request/continuation,
   item movement, serialization, durable capture, result/recovery and catalog
   bounds together. Qualify 15-root success, 14/16 boundary, mixed kinds, duplicate
   UID, busy/rejected roots, disconnect/crash/replay and conservation. Do not just
   raise the report limit or split one native recipe into partial credit.
2. **Mixed payment services:** one atomic wallet + all item trees + reward roots
   + recipient retirement + receipt; failures leave both fee and materials intact.
   Qualify all seven paid crafts, source supply and repeated purchases. These
   remain services, even after payment support is added.
3. **Source and reveal provenance:** committed first acquisition tracks actor,
   UID/revision, source kind/instance/reset placement, owner zone and causal
   source/transfer chain. Separate successful secret reveal, world/corpse recovery,
   rewards and another player's handoff. Three matching eggs are not automatically
   three different caches. Feather source is instance-sensitive. Render possession
   readiness separately from recorded personal milestones.
4. **Accepted dialogue and role events:** actual addressed recipient/topic,
   response/result, speaker identity and role-specific policy; aliases do not
   grant one achievement each. Native M, computed teacher and procedural host
   namespaces must stay distinct. Optional referral/refusal receipt means kind
   exchange, not same physical item or mandatory learning.
5. **Access, travel and actor effects:** accepted hidden-route search/open/pass,
   exact portal command/arrival/return, world generation and survival policy.
   Qualify actual dragon transition and elemental/ghost lifecycle before authoring
   transform/free/restore/escort milestones. Departure alone cannot prove destination.
6. **Scoped campaigns and branches:** builder-declared four-element AND campaign,
   source/recipient ownership, attempt/reset episodes and optional faction choices.
   Choose whether opposing requests coexist or are exclusive; current code lets
   them coexist. Native OR binding must not pretend to implement AND campaigns.
7. **Procedural assignment lifecycle:** bind paid assignment, target/count,
   completed payout and abandonment to its own actor/start episode, preserving
   generated target ownership and ordinary SQL/reward authority. No duplicate
   static local daily or keyword credit. Mercenary mixed payout support needs
   its own transactional qualification.
8. **Intent-dependent native repairs:** confirm whether head/token dialogue should
   change, whether alligator→azurian output is intended, where the engineer lost
   his plans, how the dragon alternative should become reachable, whether the
   decorative iron amulet/axe should gain interactions, and which modern surface
   room replaces historical 228371. Existing reachable contracts are preserved;
   no claim that all jungle quests are broken or that new plot code already ships.

## Shipped native repair — separate fix commit and news handoff

`f5d5b2a5ccfbb9fccc8f5570079dd3394c74ed54` — **`fix: correct five Moonhollow route descriptions`**.


Only five room-description direction words changed:

| Room / interaction | Before | Actual exit / after |
|---|---|---|
| 80325 wall walkway, cave approach | Second clause says north into cliff cave | South D2→80326; first north walkway clause stays correct |
| 80327 Moonstone's office, return | Doorway says east to guardroom | West D3→80326; eastern desk placement stays unchanged |
| 80335 armory exterior, entrance | Doorway says south | East D1→80336; south path remains D2→80334 |
| 80337 forge, return | Armory doorway says east | South D2→80336 |
| 80346 narrow branch, home approach | First clause says home west | North D0→80347; later north clause now agrees |

The [focused regression](../../../tests/async/test_rftjngle_moonhollow_directions.py)
checks prose against actual reciprocal exits; it fails on original room 80325
and passes after repair. Verification additionally checks that exactly five scoped
word replacements changed native bytes. No exits, locks, mobs, recipes, resets
or rewards changed. Other plot/boundary findings above are pending proposals.

**News:** “Moonhollow's wall, guard office, armory, forge and canopy-home directions now match their actual exits.”


Live LOOK and route traversal remain unqualified. The PR must prominently list
this fix commit and news sentence, retaining previously shipped Desolate, Hall and
Halfcut repairs. Journal guidance and pending capability work are separate entries.

## Qualification and continuation

Focused source assertions cover all native bindings/counts/rewards, secret flags,
source instances, source probabilities, portal pairs, independent faction proofs,
unspawned dragon, procedural hosts and computed teachers. Actual C++ journal
journeys check encounter gating, exact counts/kinds, loose versus worn proof,
optional histories, excluded referrals/refusals, service credit, supplied proof,
alternative grouping, read-only rendering, independent receipts and cold recovery.
Full catalog/daily, tracking/runtime/accounting gates, maintained build, changed-line
formatting, whitespace and local source links are checkpoint checks.

This validates source mapping and journal behavior, not live recovery, crafting,
search, portal survival, roaming, ghost/dragon effects, repeat supply or guarded
payments. No DB migration, accounting activation, live server operation, generated
area output, credentials or merge is part of this checkpoint. Catalog: 74 maps,
1,638 achievement units, 1,465 potential daily units, 2,212 rows; all 2,668 native
definitions, content revision 2, fingerprint/registry and prior 73 maps unchanged.
53/220 source dossiers complete, 167 pending. The Transparent Tower is next;
the full roadmap goal remains active.
