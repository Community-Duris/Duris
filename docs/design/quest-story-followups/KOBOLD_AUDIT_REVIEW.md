# Kobold Settlement audit review

This checkpoint reconciles the active `kobold` area with the broader world
connectivity, typed-reference, title-history and attic registers. The existing
[comprehensive dossier](../zone-stories/KOBOLD_SETTLEMENT.md) remains the room,
mobile, object, reset, shop, native-exchange and behavior review. Keep active
`kobold` **DO NOT MOVE**. No native area or gameplay source changed.

## Directed map, reset states and boundaries

The 147 active [WLD rooms](../../../areas/wld/kobold.wld), #1400–#1546, contain
337 resolving exit records: 335 local and two outward. The local graph has one
147-room weak component and one 147-room strongly connected component. With
all authored local exits treated as traversable, each boundary arrival (#1400,
#1402 and #1542) reaches all 147 rooms, and every room has an authored path
back to each arrival. This is WLD topology; it does not apply door resets or
guardian, switch, accounting or player state.

| Boundary | Authored WLD edge | Pairing |
| --- | --- | --- |
| Surface | #1400 south to #622130; #622130 north back to #1400 | Reciprocal; cross-area rows 22 and 1629 ([outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L22), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1629)). |
| Underdark | #1542 east to #816080; #816080 west back to #1542 | Reciprocal; rows 23 and 1734 ([outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L23), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1734)). |
| Winterhaven | #55402 southwest to Kobold #1402 | One authored arrival with no ordinary reverse WLD exit; row 756 ([Winterhaven source](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L756)). Preserve Winterhaven's entry under its own source identity. |

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse errors or a room-VNUM overlap with Kobold. It found one exact approach:
unregistered `surf.wld` room #207232 north to active Kobold #1401 ([source room](../../../areas/wld/surf.wld#L151796), [exit](../../../areas/wld/surf.wld#L151800), [target](../../../areas/wld/surf.wld#L151803)). That source-only edge stays with Surf candidate row 97 and does not add an active arrival.

The [zone file](../../../areas/zon/kobold.zon#L8) has 16 unconditional D resets.
Thirteen leave doors closed: seven locked and six closed without a lock. Three
apply the blocked bit at #1449, #1463 and #1469; the local switches and their
compiled assignment are detailed in the [source dossier](../zone-stories/KOBOLD_SETTLEMENT.md#source-clue-access-and-guardian-routes).
The reset handler applies state 1 as closed and unlocked, states 2/3 as closed
and locked, and the separate blocked bit from state value 8 ([reset handling](../../../src/world/db.c#L7868), [switch behavior](../../../src/specs/specs.object.c#L1382)).

For a strict edge-filter view, dropping every local exit reset closed or
blocked and performing no key, door, speech or switch action leaves 319 local
exit records. That filter reaches 6 rooms from Surface arrival #1400 and 121
rooms from either #1402 or #1542. It is a static lower-bound model; actual
access depends on the authored key, switch, language and guardian behavior.
The area has no F fall-chance or C current records. The custom priest and demon
movement is code-driven and remains separate from the WLD graph ([assignments](../../../src/specs/specs.assign.c#L410), [handlers](../../../src/specs/specs.kobold.c#L102)).

## Bounded typed-reference closure

The active reset file imports 11 distinct external object prototypes from
Heavens, Verz1 and White Haven. The imports include Heavens stone #358, which
uses the shared epic handler, and White Haven memory #55440 ([Kobold reset](../../../areas/zon/kobold.zon#L103), [Heavens prototypes](../../../areas/obj/heavens.obj#L3036), [Verz1 prototypes](../../../areas/obj/verz1.obj#L548), [White Haven memory](../../../areas/obj/wh.obj#L5209)). The memory is placed in Tomb of Tyxru and remains the input for Winterhaven Q3652, whose reward and acceptance belong to Winterhaven ([QST input](../../../areas/qst/wh.qst#L3657), [story record](../../../areas/story/wh.story.json#L1072)).

External typed scans distinguish mobile, object and room roles rather than
matching bare numbers. They found 13 outside ZON reset references to Kobold
object prototypes: 10 active Alatorin O resets of scattered bones #1460, one
active Surface Keeps O reset of #1460, one active Charing G reset of bread
#1403, and one source-only Sylvandawn G reset of #1403. Charing's active shop
also lists #1403. Sylvandawn's unregistered shop does the same ([Alatorin resets](../../../areas/zon/alatorin.zon#L1692), [Surface Keeps reset](../../../areas/zon/surfacekeeps.zon#L20), [Charing reset](../../../areas/zon/charing.zon#L558), [Charing shop](../../../areas/shp/charing.shp#L247), [Sylvandawn reset](../../../areas/zon/sylvandawn.zon#L636), [Sylvandawn shop](../../../areas/shp/sylvandawn.shp#L588), [Kobold object #1403](../../../areas/obj/kobold.obj#L55), [Kobold object #1460](../../../areas/obj/kobold.obj#L954)).

Keep the active imports with Alatorin, Surface Keeps and Charing. Preserve the
two source-only Sylvandawn references with its existing RETAIN candidate row
101; they do not establish active reset placement or stock. Charing's audit
already records its active Kobold imports; this checkpoint adds the exact
object and shop rows. Kobold mobile #1403 (baby kobold) and object #1403 (dark
bread) are different typed prototypes. Mobile #1433 (the statue) and object
#1433 (the frames) are also separate identities.

The bounded current-catalog scan found no external QST item command or
structured story giver/item reference to Kobold prototypes, no outside mobile
or object prototype definition in the local VNUM sets, and no external type-25
portal destination into a Kobold room. Kobold's own four exchanges use its
local item prototypes; the existing dossier records their exact inputs and
outcomes. The checked compiled assignments remain the five Kobold mobile
handlers, three local switches and guard-post inn handler ([assignment table](../../../src/specs/specs.assign.c#L410), [switches](../../../src/specs/specs.assign.c#L1809), [inn](../../../src/specs/specs.assign.c#L2512)).

## Title, source history and related identities

After stripping color codes and normalizing case and whitespace, the active
title “Kobold Settlement” has one owner among the 351 current active `.zon`
titles ([title](../../../areas/zon/kobold.zon#L2), [active list](../../../areas/AREA#L4)). Followed WLD, MOB, OBJ, QST, SHP and ZON histories remain on the `kobold` stem. The tracked 2009 change is the area-tree relocation; later records are same-stem text, loader, quest-item-flag and equipment maintenance. The April 2026 ambassador and rare-load updates are content additions under the same owner. The structured source-story journal was added on 2026-10-04 ([journal](../../../areas/story/kobold.story.json#L1)). No alternate-stem predecessor, title rename or whole-area replacement was found in these followed paths.

Winterhaven's ambassador from Kobold Settlement and its memory exchange are
explicit cross-area relationships, not duplicate ownership. The single
inactive Surf approach is also a source link rather than replacement evidence.
No inactive WLD room collides with #1400–#1546. Preserve Kobold's source
identity and the separate Winterhaven, Surf and imported-object owners.

## Builder questions and recommended resolutions

| Question | Source evidence | Recommended resolution |
| --- | --- | --- |
| Should the silver smelting, shield and gem-inspection exchanges remain supporting services? | The journal classifies Q65, Q52 and Q85 as services and Q93 as the spectacles story ([journal](../../../areas/story/kobold.story.json#L158), [native blocks](../../../areas/qst/kobold.qst#L52)). | Keep each existing contract identity. Do not make a service receipt a mandatory story stage without an explicit owner decision. |
| Should the mixed coin-and-item exchanges accept payment with active accounting? | Q65, Q52 and Q93 combine coin and item goals; the existing implementation keeps those offers guarded ([QST](../../../areas/qst/kobold.qst#L52), [dossier](../zone-stories/KOBOLD_SETTLEMENT.md#native-progression-and-current-limits)). | Keep the guard until an accepted operation settles fees, all item roots, reward creation and recovery together. |
| What fair source or cap should support the eight-nugget smelting recipe? | The recipe wants eight #1447 nuggets; normal resets declare a five-live-copy cap. | Qualify ordinary and forced resets, saved-item accumulation, reload and consumption before changing source counts or limits. |
| How should Szxvu choose between gem inspection and spectacles when the inputs overlap? | Q93 and Q85 both use #1431; with matching frames present the guarded coin goal can block inspection selection ([QST](../../../areas/qst/kobold.qst#L85), [branch order](../zone-stories/KOBOLD_SETTLEMENT.md#native-progression-and-current-limits)). | Preserve the current order and optional inspection unless a builder selects a separate, tested branch rule. Do not silently consume the gems or award a failed exchange. |
| Does inspection of #1431 have to preserve the original item identity? | Q85 returns the same object VNUM; equal kinds do not prove that the original UID survives. | Keep inspection optional and qualify exact UID, custody and reward behavior before requiring it. |
| Should supplied exact materials continue to skip personal source and producer history? | The sidecar marks source and earlier-service checks optional; its guidance accepts supplied copies ([journal](../../../areas/story/kobold.story.json#L1)). | Preserve delivery eligibility for supplied copies. Separate provenance evidence from possession and accepted exchange evidence. |
| Which local outcomes own discovery, achievement and daily credit? | Discovery has its own achievement; Q93 retains one guarded achievement candidate; the three services and all four contracts have no daily candidate. | Keep those credit owners and exclusions explicit; do not award new progress from a question, material, scenery or unaccepted payment. |
| Should the four Rod of Rulership pieces remain lore, or should a complete acquisition and reassembly route be authored? | The dossier found no local reset, QST or smith row that supplies or reassembles the pieces ([dossier](../zone-stories/KOBOLD_SETTLEMENT.md#source-clue-access-and-guardian-routes)). | Keep the existing owner boundary until builders map every source, custody rule, reassembly and accepted outcome. Do not infer a recipe from item text. |
| Do the outer gate, Gwark's house and mound doors use the intended keys and holders? | #1419 is carried by Gwark; #1415 is carried by the shaman; the reset closes the paired doors and the WLD assigns those key VNUMs ([reset](../../../areas/zon/kobold.zon#L16), [key placements](../../../areas/zon/kobold.zon#L169)). | Review key, holder, pair, closed/locked state and reverse passage together. Preserve the VNUMs until a route decision is made. |
| Are the north and south fence switches meant to clear both blocked directions? | Root switches #1421/#1425 at #1463/#1469 use TUG341; the reset applies blocked state to both directions. | Confirm the two controls and their reciprocal target data; credit an accepted opening and crossing separately. |
| Is the bar switch at #1449 meant to open the east route to #1470? | Object #1427 uses PULL340; the zone resets the exit blocked and separately closes its west return. | Preserve the handler and destination; qualify the acknowledged switch action, door state and return path before describing a completed passage. |
| Does the bone-door phrase record language learning or only unlock the door? | The altar door uses keyword `i>|uub`; the shared SAY behavior clears matching lock/secret state but does not record learned language. | Keep clue, correct speech, unlock, opening, entry and any language-learning receipt as distinct events. |
| Which temple guardian actions count as useful story outcomes? | The corrected handlers act at altar #1481, tomb #1482, pit #1484 and ledge #1483; their assignment and movement are code-bound ([assignment](../../../src/specs/specs.assign.c#L411), [handlers](../../../src/specs/specs.kobold.c#L102)). | Define accepted source, actor, target, movement, combat, survival and return evidence. Do not credit presence, warning text or an unaccepted pull as completion. |
| Should statue death continue to use `stone_crumble` as the player's recovery route? | The #1433 special creates container #1438 and transfers contents; the handler has failure exits and is outside ordinary corpse creation ([handler](../../../src/specs/specs.kobold.c#L28)). | Qualify pile creation, item and money transfer, save/restart, extraction and failed creation before promising durable recovery. |
| What counts as a completed spectacles outcome when source materials are supplied? | The static frames and statue gems have different source paths; Q93 has an exact native completion contract and guarded fee. | Record only an accepted exchange under its current owner. Do not require an unrecorded kill or corpse episode unless builders deliberately add it. |
| Should the Winterhaven arrival #55402 remain one-way? | Winterhaven's WLD points southwest to #1402, with no ordinary reverse exit; Kobold's other surface and underdark boundaries are reciprocal. | Keep the current edge under Winterhaven and name the intended return route before describing the arrival as safely reversible. |
| Are the reciprocal Surface and Underdark boundaries meant to remain open at the zone edge? | #1400 and #1542 have reciprocal external WLD records; the local reset file does not close those outward directions. | Preserve both pairs and keep local doors, dynamic guardian travel and outer-area return as separate checks. |
| Should the inactive Surf approach to #1401 ever be activated? | Only unregistered `surf.wld` #207232 north points to #1401. | Preserve it on Surf candidate row 97 as source-only. Any activation needs its own owner, route and return decision. |
| Should the imports from Heavens, Verz1 and White Haven stay with their current prototypes? | Kobold resets import their objects; #358 has an epic procedure and #55440 is used by Winterhaven Q3652. | Retain the imported prototype owners and exact references; do not copy them into Kobold or annex their foreign outcomes. |
| Should Charing, Alatorin and Surface Keeps continue using Kobold objects? | Active Charing uses bread #1403; Alatorin and Surface Keeps reset scattered bones #1460. | Keep each consumer under its current zone and preserve the Kobold object identities if either owner later proposes a move. |
| Should source-only Sylvandawn continue to list Kobold bread #1403? | Its unregistered ZON resets the item and its SHP product row lists it; both are recorded on RETAIN candidate row 101. | Preserve those records on Sylvandawn's existing candidate row. They do not establish current shop operation or active reset stock. |
| Should guard post #1444 keep its additional inn handler? | `specs.assign.c` binds `inn` to #1444 although the room lacks the inn flag; regular inn #1443 has its own keeper and flag. | Confirm the guard post's rent/home policy and recovery behavior before changing or crediting that extra binding. |

## Attic disposition and audit limits

Keep `kobold` **DO NOT MOVE** and add no Kobold candidate row. Preserve its
Surface and Underdark pairs, Winterhaven-owned arrival and memory completion,
foreign object imports, active consumers, and native code assignments. Keep the
new #207232 approach on Surf candidate row 97 and the #1403 source-only reset
and shop references on Sylvandawn candidate row 101. No move or activation is
authorized by these source findings.

The map, resets and typed catalogs establish authored relationships. This
checkpoint does not establish current item stock, successful fee settlement,
live door traversal, guardian availability, safe travel, durable death
recovery, saved progress or a played return journey.
