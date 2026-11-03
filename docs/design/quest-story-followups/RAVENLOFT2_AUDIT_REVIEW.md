# The Ravenloft Catacombs: repository-wide source audit supplement

The existing [source dossier](../zone-stories/THE_RAVENLOFT_CATACOMBS.md) and
[review index](../../reference/zone-story-audits/ravenloft2.md) map this area's
authored story, native QSTs, prototypes, resets, sidecar, and reviewed shared
code. This supplement closes the broader
connectivity, typed-dependency, identity/history, retention, and builder-review
register items. It does not claim an installed or played journey.

## Story and source boundary

The dossier covers 37 native exchanges across twelve stories, thirteen
requests, and eight support services; it distinguishes 25 achievement units
from eighteen potential daily candidates. The source review counts 135
addressed response blocks and 27 ambient responses, alongside 400 rooms, 98
mobiles, 327 objects, 1,457 reset commands, and both shop records. It documents
the independent Emil, Exethanter, Thredra, Kasimir, Rahadin, Blinsky, Izek,
three-recipient Strahd-skull, and service outcomes, including outcomes that are
only narrated or not implemented.

No local literal special assignment or data procedure binding was found in the
reviewed files. Shared shopkeeper, switch, and teleport behavior still applies.
The dossier separately records the behavior and limits of those shared paths,
foreign-owned inputs, container selection, and accounting readiness.

## Active boundaries and local graph

The active [WLD source](../../../areas/wld/ravenloft2.wld) has 400 rooms and
961/961 resolving exits: 952 local and nine outbound. Its weak components have
392, 4, 1, 1, 1, and 1 rooms. The small
four-room component is #59169, #59171, #59182, and #59184; the singletons are
#59186, #59355, #59356, and #59398. #59398 still has three outbound WLD exits.

Both active WLD inbounds come from Castle Ravenloft, and the corresponding
local exits are reciprocal:

| Castle Ravenloft source | Catacombs arrival | Local return |
| --- | --- | --- |
| [#58538 down → #59057](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L811) | #59057 | #59057 up → #58538 |
| [#58542 south → #59352](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L812) | #59352 | #59352 north → #58542 |

The nine local outbounds are recorded in the
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L816): the two
Castle returns above; #59355 down to Barovia #91121; #59356 down to Barovia2
#58958; #59359 east and west to Barovia #91165 and #91164; and #59398 north to
Barovia2 #58811, east to Castle Ravenloft #58305, and south to Barovia #91010.
Only the first two have reciprocal WLD edges. The latter routes retain their
current source owners and do not by themselves prove usable arrival or return.

## Reset-aware arrival baseline

The [Catacombs ZON](../../../areas/zon/ravenloft2.zon) contains 194 D resets
addressing 192 distinct local room/direction pairs. Two door directions are
reset twice: #59145 west and #59149 east, both with state 5. State values are
applied by
[`db.c`](../../../src/world/db.c#L7857), where low bits set closed/locked state
and bits 4 and 8 set secret and blocked flags.

| ZON state | Door interpretation | Count |
| ---: | --- | ---: |
| 0 | open | 1 |
| 1 | closed, unlocked | 4 |
| 2 | closed, locked | 103 |
| 4 | open, secret | 2 |
| 5 | closed, secret | 6 |
| 6 | closed, locked, secret | 2 |
| 8 | open, blocked | 73 |
| 9 | closed, unlocked, blocked | 1 |
| 14 | closed, locked, secret, blocked | 2 |

Raw directed reach and a strict source-only baseline that excludes closed,
locked, secret, or blocked exits are:

| Active arrival root | Raw directed reach | Strict post-reset reach |
| --- | ---: | ---: |
| Castle Ravenloft #58538 → #59057 | 83/400 | 47/400 |
| Castle Ravenloft #58542 → #59352 | 383/400 | 1/400 |

These counts combine WLD exit flags with current D-reset states. They do not
simulate switch commands, keys, secret discovery, travel objects, actor/mobile
placement, reset timing, or a successful return. The source declares 80
fall-marked rooms and no currents; neither count qualifies safe movement.

## Inactive WLD source sweep

All 97 WLD files present under `areas/wld/` whose stems are not active entries
in `areas/AREA` were parsed. None reuses a Catacombs room VNUM or has an exit
targeting a Catacombs room. There is no new source-only approach or attic
candidate. The curated source-only count remains 4,489 overall: Surf 253,
`ud_map1` two, `ud_map2` 20, and Underworld2 eight.

## Typed references and ownership

The 1,457 local reset commands contain 145 references to 46 foreign mobile or
object prototypes; every referenced VNUM resolves to a current prototype
source. In the reverse direction, [Castle Ravenloft's ZON](../../../areas/zon/ravenloft.zon#L519)
has one reset of local object #59326, the counterpart used by the paired handprint
transition. A scan
of other native QST item commands, SHP products, and story-sidecar
`item_vnums` found no consumers of Catacombs object prototypes. An exact source
scan found no `59000`–`59399` numeric literals in `src` C/C++ files.

The local QST does accept externally owned item prototypes, including the
dracolich skull #58397, Madam Eva's crystal ball #58803, and old catacomb key
#58419. Its foreign reward prototypes include the saints' buckler #55322,
enchantment scroll #55362, token #55033, and equipment chest #400233. Their
source ownership remains with their defining areas; accepted possession does
not establish a player's source, kill, or travel history.

Three local type-25 objects have distinct declarations:

| Object | Source placement or outcome | Declared target |
| --- | --- | --- |
| [#59081](../../../areas/obj/ravenloft2.obj#L1227) | Reset in #59169 | Astral Main #19721 |
| [#59082](../../../areas/obj/ravenloft2.obj#L1239) | Reset in #59169 | Juiblex #87528 |
| [#59310](../../../areas/obj/ravenloft2.obj#L5146) | Jander's QST reward | Interspace #99999; its activation value is also #99999 |

All three target VNUMs exist in active WLD sources. The first two remain
separate portal objects in their shared #59169 placement. The reward object's
declared destination does not qualify its usable activation or a successful
journey.

[`areas/AREA`](../../../areas/AREA#L242) lists `ravenloft2` as an active
full-area contributor, while [`areas/qst/AREA`](../../../areas/qst/AREA) does
not. The tracked [quest-catalog generator](../../../scripts/zone_story_quest_catalog.py#L33)
documents that production `make_qst` reads top-level `areas/AREA`;
`areas/qst/AREA` is a narrower historical list. Preserve this distinction
rather than treating the historical list as evidence that the current QST
source is unowned.

The coverage row also had stale shop inventory fields. The two checked-in
[SHP records](../../../areas/shp/ravenloft2.shp#L1) are #59081 (Gadof Blinsky;
local toy stock #59261–#59266) and [#59097](../../../areas/shp/ravenloft2.shp#L40)
(Rahadin's animated cloak; local scroll stock #59281, #59282, #59289, and
#59300). The corrected shop VNUM bounds are 59081–59097 with two records.
These are shopkeeper VNUMs; the same numeric VNUMs also identify separate local
object prototypes in the shared mobile/object namespace.

## Title, duplicate, and followed history

After color-token removal, whitespace normalization, and case folding, the
title in the [current ZON source](../../../areas/zon/ravenloft2.zon#L2), “The
Ravenloft Catacombs,” is unique among all 442 current `.zon` titles and among
the 351 active area titles. It remains distinct from Castle Ravenloft,
Barovia, and Barovia2. The available followed WLD, MOB, OBJ, ZON, and QST
history stays on the `ravenloft2` source stem; no alternate-stem predecessor or
explicit whole-zone replacement appears in the reviewed title and history
records. The available repository history for these paths begins with the
April 2026 import/refactor history, so this is a bounded source-history result.

## Checkpoint 337: Ravenloft 2 reset-aware access and builder decisions

Keep active `ravenloft2` **DO NOT MOVE**. Preserve its two reciprocal Castle
boundaries, seven additional one-way outbounds, three typed portal declarations,
the paired handprint ownership, 37 native QST exchanges, story sidecar, and two
SHP records. The static reach, typed links, and declared targets do not qualify
live access, stock, accounting settlement, or safe return. No candidate,
area membership, or gameplay behavior changes.

### Checkpoint 337 builder questions and recommended resolutions

These questions focus on decisions that belong to area owners; the
recommendations retain current source behavior until a separately qualified
change is selected.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| RC2-01 | Should the Catacombs retain a distinct active identity from Castle Ravenloft, Barovia, and Barovia2? | Preserve the unique “The Ravenloft Catacombs” title and current VNUM ownership; no predecessor or replacement claim is supported by reviewed history. |
| RC2-02 | Are the six weak components intentional, including the four-room group #59169/#59171/#59182/#59184 and singleton rooms #59186, #59355, #59356, and #59398? | Preserve the WLD as authored while checking each portal, load-room, and scripted transition under its own source owner. Do not add an ordinary exit from component counts alone. |
| RC2-03 | Is strict initial-open reach of 47/400 from cellar arrival #59057 intended? | Keep the current Castle boundary and reset states; qualify keys and local controls separately before describing the rest of the area as ordinarily open. |
| RC2-04 | Should Kingsmen arrival #59352 remain a one-room strict baseline even though its raw directed component reaches 383 rooms? | Preserve the incoming boundary and reset states while the owner confirms the expected opening or puzzle sequence. Do not treat the raw graph as immediate player access. |
| RC2-05 | Are the repeated state-5 resets at #59145 west and #59149 east intentional? | Confirm the duplicate reset rows with the area owner. Keep their secret/closed state until any cleanup is selected and its reset behavior is qualified. |
| RC2-06 | Are the 73 state-8 blocked exits and other secret/blocked combinations controlled by the authored switches and travel interactions? | Preserve the distinct EX_BLOCKED, secret, closed, and locked flags; document each verified controlling action before proposing an ordinary route. |
| RC2-07 | Are the 80 fall-marked rooms intended hazards, and are their return routes clear? | Keep the current fall declarations. Qualify movement and return behavior separately; no current is declared in the room source. |
| RC2-08 | Are Castle Ravenloft arrivals #59057 and #59352 meant to remain the two reciprocal WLD entry pairs? | Preserve both pairs under their respective areas and do not collapse them into one entrance or infer safe return from reciprocity alone. |
| RC2-09 | Are the Barovia, Barovia2, and Castle exits from #59355, #59356, #59359, and #59398 intended as one-way source transitions? | Preserve all seven one-way outbounds and their destination owners; verify any intended return through the receiving area's gateways or other controls. |
| RC2-10 | Should reset portal object #59081 continue to target Astral Main room #19721 from #59169? | Preserve the declared destination and Astral/Main ownership; qualify actual portal use and return separately. |
| RC2-11 | Should reset portal object #59082 continue to target Juiblex room #87528 from the same #59169 placement? | Keep it distinct from #59081 and preserve Juiblex ownership; do not treat the shared room placement as one combined portal. |
| RC2-12 | Is Jander's type-25 reward #59310 intended to target Interspace #99999 with activation value #99999? | Preserve the authored object and destination; verify the usable activation path before promising that the reward transports its owner. |
| RC2-13 | Should the Castle and Svalich Woods remain the source owners for accepted skull #58397, crystal ball #58803, and old key #58419? | Keep those objects under their existing areas and preserve the Catacombs' accepted-item contracts without crediting source, kill, or travel history. |
| RC2-14 | Are paired handprints #59325 and #59326 intended to retain their current cross-area push-teleport mapping? | Preserve the Castle reset of #59326 and the local counterpart; qualify both directions and item ownership before describing a safe round trip. |
| RC2-15 | Should the Ezmerelda, Rictavio, and Jander skull deliveries remain three independent endings? | Keep each exact recipient, reward, and retirement outcome separate; do not add a personal-kill, forced-order, or liberation predicate from narration. |
| RC2-16 | Are Dostron's ashes #59150 intended to support separate Thredra and Exethanter receipts? | Preserve independent outcomes and require separate physical copies when both are pursued; do not collapse the two giver records. |
| RC2-17 | Should the legacy P resets continue choosing a matching container by prototype rather than exact parent generation? | Keep the current behavior documented as unqualified placement; decide any parent-binding change separately and compare repeated containers and reset timing first. |
| RC2-18 | Should Dayheart and Sunsword references remain optional lore without a local campaign-completion outcome? | Preserve current prose and item ownership until builders define an explicit cross-area story, its source/use events, and its terminal outcome. |
| RC2-19 | Should both local shops remain active source records with their current item lists under the accounting gate? | Preserve Gadof and the animated cloak's SHP identities and stock; qualify active purchase readiness independently from their native item-giving exchanges. |
| RC2-20 | Should the historical `areas/qst/AREA` list be updated to mirror top-level `areas/AREA` membership for this active area? | Keep top-level `areas/AREA` as the documented production QST input. Change the historical list only if its separate maintenance purpose calls for it. |

This is a documentation-only audit. No native area, quest, sidecar, WLD,
reset, membership, or gameplay source was changed. No build, test, server,
database, migration, or gameplay session was run.
