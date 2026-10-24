# Llyrath Forest: source and dependency review

**Scope:** static review of active Llyrath room, mobile, object, zone, and
tracked-file history, including story cues, authored exits, and typed references.
This review does not establish ordinary access, live door state, teleport
usability, swimming or fall outcomes, or safe traversal.

## Source identity and local story

`llyrath` is an active full-area contributor with zone number `949`, 93 rooms
`#94900`–`#94992`, 16 mobile prototypes `#94900`–`#94915`, and seven object
prototypes `#94900`–`#94906` ([active list](../../../areas/AREA#L338),
[zone header](../../../areas/zon/llyrath.zon#L1),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L161)). Its sources contain no local
shop file, QST file, or story sidecar. `llyrath` appears in the separate
separate [`areas/qst/AREA`](../../../areas/qst/AREA#L298) index, but there is
no `areas/qst/llyrath.qst`; that index entry does not define a local contract.
There is no `areas/map/llyrath.json` editor map.

| Source element | Static story cues |
| --- | --- |
| Road and forest paths | The Moonshae approach enters on a quiet, well-traveled dirt road. Southward it narrows to grass-covered paths, dense woods, roots, and leaf-covered trails ([entry](../../../areas/wld/llyrath.wld#L1), [paths](../../../areas/wld/llyrath.wld#L35)). |
| Quiet clearing and concealed trail | Rooms `#94916`–`#94918` repeat a small clearing where only leaves and animals disturb the quiet. Room `#94916` describes thinner leaves northwest; its west exit is described as “branches” into narrow trail `#94919` ([clearing](../../../areas/wld/llyrath.wld#L294), [trail](../../../areas/wld/llyrath.wld#L356)). |
| Lagoon and river | The waterfall feeds a dark lagoon with a northwestern outlet. The waterway passes rocky shallows and a moss-covered log, then enters a narrow, deep forest trench with changing current; farther north it accelerates toward a gully ([lagoon](../../../areas/wld/llyrath.wld#L1285), [log and river](../../../areas/wld/llyrath.wld#L1596), [fast section](../../../areas/wld/llyrath.wld#L1701)). |
| Chauntea grove | The secluded, circular grove has one described opening to the east and a clear pool. Seulith, whose mobile text names Chauntea, is reset there with the local holy symbol and green-and-white robe ([grove](../../../areas/wld/llyrath.wld#L1933), [Seulith](../../../areas/mob/llyrath.mob#L1), [reset and equipment](../../../areas/zon/llyrath.zon#L58)). |
| Woodsman’s shack | Kalrath is described as a woodsman relaxing in a humble shack. The one-room interior has a table, chair, and fur-covered bed; his green boots and forest-green cloak are equipped by local resets ([shack](../../../areas/wld/llyrath.wld#L1920), [Kalrath](../../../areas/mob/llyrath.mob#L54), [reset and equipment](../../../areas/zon/llyrath.zon#L55)). |
| Gully and cave | The river reaches a sinkhole and natural cave. Cave prose says the stream enters from the south and disappears into the north wall; an onyx pillar stands in the river to the ceiling ([gully](../../../areas/wld/llyrath.wld#L1946), [cave](../../../areas/wld/llyrath.wld#L1965)). |
| Wildlife and scenery | The local mobile set supplies forest animals and fish, while the object set includes a waterfall, pool, Chauntea symbol, clothing, and the onyx pillar ([mobile prototypes](../../../areas/mob/llyrath.mob#L1), [object prototypes](../../../areas/obj/llyrath.obj#L1)). |

The environment sketches a forest walk that branches into a quiet clearing,
grove, and woodsman’s shelter, while the lagoon outlet leads through a current-
marked river to the cave. No local QST contract or native `Q`/`QA` block
expresses a quest arc. None of the seven objects has the native quest-item bit
set ([flag definition](../../../src/core/defines.h#L252)). The area has no
mapped narrative sidecar, and source text alone does not establish that its
scenes or transitions are reachable by ordinary players.

## Static exits and boundary records

All 234 authored exit records resolve in the frozen active room table. The
93-room local exit graph forms one weak component; it includes one active
Moonshae boundary pair, no fall-chance records, and four current-marked rooms.
The component count treats linked directions as undirected and says nothing
about door state, swimming, survival, or permission to travel.

Moonshae room `#26257`, “The Great Corwell Road,” has a south exit to Llyrath
`#94900`; Llyrath `#94900` has the north return exit. Both records are listed
in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L406)
([reverse record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1141); [Moonshae room](../../../areas/wld/moonshae.wld#L1123),
[Llyrath entry](../../../areas/wld/llyrath.wld#L1)). No unregistered WLD file
adds a source-only inbound exit to `#94900`–`#94992`.

| Static condition | Source evidence and limit |
| --- | --- |
| Quiet-clearing door | The WLD labels the west connection from `#94916` as “branches”; the zone comments call that exit closed/secret and also reset the reciprocal exit at `#94919` as closed ([zone records](../../../areas/zon/llyrath.zon#L8)). These records do not prove the live state after reset. |
| Shack door | The west/east door pair between `#94987` and `#94988` is marked closed by zone reset comments ([zone records](../../../areas/zon/llyrath.zon#L16)). The room prose describes the shack and its east door ([interior](../../../areas/wld/llyrath.wld#L1920)). |
| Cave direction | Cave prose says the river dominates the entrance and makes it one way, but the WLD also authors a south exit from cave `#94991` back to gully `#94990` ([cave prose and exit](../../../areas/wld/llyrath.wld#L1965)). The static prose/edge mismatch needs builder interpretation; neither text nor edge proves actual movement behavior. |
| Height and fall | Room `#94992` describes a clear fall from roughly twenty feet above a forest path and has a down exit to `#94912`; its record has no fall-chance field ([room](../../../areas/wld/llyrath.wld#L1980)). No fall result was exercised. |
| Current fields | The WLD records currents at `#94972` (`2 3`), `#94976` (`2 0`), `#94977` (`2 0`), and `#94990` (`5 0`) ([river records](../../../areas/wld/llyrath.wld#L1631), [fast river](../../../areas/wld/llyrath.wld#L1701), [gully](../../../areas/wld/llyrath.wld#L1946)). The loader reads current values from room data ([loader](../../../src/world/db.c#L1395)); this review does not infer their runtime effect. |

## Typed prototype and code references

The main local typed room reference is object `#94906`, the onyx pillar. Its
prototype is type `25` (`ITEM_TELEPORT`) and its first value is room `#94992`;
the zone resets it into cave room `#94991`
([pillar](../../../areas/obj/llyrath.obj#L75),
[reset](../../../areas/zon/llyrath.zon#L27),
[`ITEM_TELEPORT`](../../../src/core/defines.h#L140)). The shared item handler
reads `value[0]` as the fixed destination and resolves that room number
([handler](../../../src/magic/spell_travel.c#L991)). This is a source-defined
local teleport destination, not a tested claim that the object can currently
be used or that the full route is safe.

| Consumer or input class | Static scan result |
| --- | --- |
| Other active and unregistered ZON files | No reset references to Llyrath mobile/object prototypes `#94900`–`#94915` / `#94900`–`#94906`, and no room-placement or door-reset references to `#94900`–`#94992`. The local zone’s own commands refer to local prototypes and rooms. |
| Active and unregistered SHP files | No `949xx` stock, keeper, or room references; Llyrath has no local shop file. |
| Active and unregistered QST files | No `949xx` giver or content references; Llyrath has no local QST source despite its historical quest-index row. |
| Other object prototypes | A type-aware scan of 21,683 current object prototypes found no external `ITEM_TELEPORT` whose destination is in `#94900`–`#94992`; the only match is Llyrath’s own pillar-to-`#94992` reference. |
| Compiled source | No direct numeric reference to Llyrath room, mobile, or object VNUMs was found in `src`. The generic `ITEM_TELEPORT` handler is type behavior, not an area-specific binding. |
| Other WLD sources | The active Moonshae record is the sole external WLD connection found; unregistered WLD sources add no inbound exit. |

A broad numeric search also finds `94903` in `hall.obj` object `#77729`. In
that prototype it is the cost field following eight ordinary object values,
not a Llyrath object reference: the object parser reads `value[0]`–`value[7]`
before weight, cost, and condition ([object record](../../../areas/obj/hall.obj#L506),
[numeric fields](../../../areas/obj/hall.obj#L511),
[parser](../../../src/world/db.c#L4678)).

## Tracked history and owner questions

The normalized active `.zon` title scan found no repeated Llyrath title. The
tracked path history reaches the 2009 `areas/` relocation (`642da5400`). Later
changes on these paths are field-layout updates in mobile/object data, the
2015 zone-editor version string, a file-mode change, and 2011 WLD spelling
edits (`677a9f877`). None of the reviewed path commits identifies
Llyrath as replacing or retiring another active area. This is bounded to the
tracked current paths and their available history; it is not a conclusion
about untracked material or older design intent.

Keep the active area in place pending owner clarification on:

1. Whether the onyx pillar’s fixed destination at `#94992` is the intended
   route from the river cave, and what player-facing use of that object is
   intended.
2. Whether cave `#94991` is meant to be one way despite its authored return
   edge to `#94990`.
3. Whether the clearing and shack door reset comments match intended access,
   and how the current-marked river rooms and elevated `#94992` are meant to
   be approached safely.
4. Whether Llyrath is intentionally an environmental area without a local QST
   contract or authored story sidecar.

No room, exit, reset, prototype, or route data was changed. No live access,
teleport, current, fall, combat, or persistent outcome was tested.

## Checkpoint 263: directed WLD reachability

The active WLD has 93 rooms and 234 resolving exits: 233 local and one
outbound to Moonshae #26257. The local graph is one 93-room weak component.
From entrance #94900, raw directed traversal reaches 92/93 rooms; all 93 rooms
have authored local paths back to #94900. The strongly connected components
contain 92 rooms and singleton #94992. That elevated room is outside raw WLD
forward reach, but its down exit reaches #94912 and returns through the local
map. The source-defined onyx-pillar type-25 object #94906 targets #94992 from
cave #94991; it is dynamic object travel, not a WLD edge. See the
[connectivity report](WORLD_CONNECTIVITY.md#checkpoint-263-llyrath-directed-wld-reachability).

## Checkpoint 263: builder questions and recommended resolutions

These questions make the existing environmental and access decisions explicit.
Recommendations preserve the source map and wording until the owner chooses a
separately scoped change.

| # | Direct builder question | Source-backed recommended resolution |
| --- | --- | --- |
| 1 | Should Llyrath remain an environmental forest area with no local QST contract or story sidecar? | Preserve the current non-contract status; do not credit exploration, visits, or scenic encounters as receipts. |
| 2 | Does the historical `areas/qst/AREA` row, without a `llyrath.qst` file, remain catalog history rather than an active local contract? | Keep the index entry and absent source file distinct; do not infer a giver or accepted item command. |
| 3 | Is the west connection from quiet clearing #94916 into narrow trail #94919 intended to stay a closed, secret branch discovered through native reveal handling? | Preserve the authored exit and reset flags; qualify SEARCH/reveal, OPEN, and actual passage separately. |
| 4 | Do the two reciprocal clearing-door reset comments express the intended closed/secret state after reset? | Keep both reset records until the area owner confirms state; prose and reset comments alone do not establish the current door state. |
| 5 | Is the shack door pair between #94987 and #94988 intended to remain closed without a key or personal quest prerequisite? | Preserve the authored close state; do not infer a lock, key, guardian defeat, or service from the door prose. |
| 6 | Is Seulith's Chauntea grove intended as a devotional setting without a local rite or item-exchange contract? | Keep the grove, symbol, robe, and NPC text as environmental evidence; do not invent a blessing, offering, or achievement. |
| 7 | Are Seulith's equipped holy symbol and robe intended to stay ordinary reset equipment rather than recoverable quest proof? | Preserve their exact reset placement and do not assign them a story role without a new native contract. |
| 8 | Should Kalrath remain a woodsman in a one-room shack without a local shop or accepted service transaction? | Preserve the current room and equipment details; do not advertise trade or shelter services absent an implemented owner. |
| 9 | Are the four current-marked river rooms (#94972, #94976, #94977, and #94990) intended to remain hazards whose movement and swimming outcomes need runtime qualification? | Keep the exact WLD fields and avoid promises about current direction, resistance, safe crossing, or survival until the movement path is qualified. |
| 10 | Should player guidance avoid calling the lagoon-to-cave river a safe route while its current fields are present? | Preserve the written geography and report water hazards plainly; do not infer a safe passage from the weakly connected static map. |
| 11 | Is cave #94991 meant to be one way even though its WLD record authors a south return to gully #94990? | Keep the current prose and edge documented as a mismatch; the owner should choose a copy correction or separately reviewed map change before either is treated as authoritative. |
| 12 | Is onyx pillar #94906, reset in cave #94991, intended to teleport to room #94992? | Preserve the exact type-25 destination and reset; qualify usable command, admission, successful arrival, and return separately. |
| 13 | Is #94992 intentionally unreachable from the normal #94900 WLD traversal and entered through the pillar's dynamic destination? | Keep raw WLD reachability separate from object travel; do not add a physical entrance based only on the isolated forward set. |
| 14 | Should #94992's twenty-foot height prose remain descriptive when it has a down exit to #94912 and no fall-chance field? | Preserve the authored exit and absence of a fall record; do not promise a fall, damage, or safe descent from prose alone. |
| 15 | Should Moonshae #26257 south to #94900 and the reciprocal north return remain the only active outside WLD boundary? | Preserve the pair with both area owners and keep the local pillar travel separate from cross-area WLD routing. |
| 16 | Should the all-95 inactive-WLD result remain negative, with no source-only inbound candidate or room overlap? | Keep the scan result and add no candidate or ingress claim unless new source evidence is found. |
| 17 | Should the wildlife and scenery objects/mobile descriptions remain environmental content without added achievements? | Preserve the current source identity and ordinary behavior; any new story outcome needs an explicit accepted task and durable contract. |
| 18 | Does the unique normalized Llyrath title and followed same-stem history still provide no predecessor or whole-area replacement evidence? | Keep active `llyrath` **DO NOT MOVE** and its title/VNUM ownership unless new tracked source evidence changes that conclusion. |

## Checkpoint 263 disposition

Keep active `llyrath` **DO NOT MOVE**. Preserve the reciprocal Moonshae pair,
the reset-controlled clearing and shack doors, the current-marked river, the
cave prose/return-edge mismatch, and the local onyx-pillar destination under
their existing owners. The raw WLD graph gives #94992 no ordinary forward
arrival from #94900, while the pillar declares a dynamic destination there;
neither fact qualifies live use or safe travel. The inactive-WLD scan found no
approach or overlap, so no candidate is added. No native area or gameplay
source changed.
