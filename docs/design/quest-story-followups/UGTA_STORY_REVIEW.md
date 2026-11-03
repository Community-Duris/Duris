# Ugta: source story review

**Scope:** source review of Ugta's 242 rooms, 31 mobile prototypes, 69 object
prototypes, 441 zone commands, seven shop records, local QST text, and story
sidecar. This maps environmental narrative, static exits, and selected typed
references. It does not verify live door state, shop behavior, special
procedures, safe player traversal, or whether outside access is intended.

## Story coverage state

Ugta is an active full-world contributor (`*391-399`, zone header 391), appears
in both the production [`areas/AREA`](../../../areas/AREA#L190) list and the
separate historical [`areas/qst/AREA`](../../../areas/qst/AREA#L175) index, and
has the expected WLD, MOB, OBJ, ZON, QST, SHP, and story-sidecar files. The
coverage row records 242 rooms, 31 mobiles, 69 objects, 31 local QST giver
records, no native Q/QA contracts, and seven shops
([coverage row](ZONE_COVERAGE_REGISTER.csv#L323)).

The QST file contains topic messages as well as a notable `quest` topic on
mobile 39112: it says “I hate twinks,” but defines no objective or reward
([QST response](../../../areas/qst/ugta.qst#L77)). Mobile 39130's `qc_action
70` entry emits the announcement that Ugta's gates are closed and outsiders
are no longer welcome ([guardian QST entry](../../../areas/qst/ugta.qst#L189)).
In the quest handler, `qc_action` is handled as a `CMD_NONE` room action, and
`has_quest_ask` explicitly skips it; this distinguishes the timed announcement
from an ask/contract entry ([action handler](../../../src/world/quest.c#L88),
[ask check](../../../src/world/quest.c#L2438)).

The sidecar nevertheless says `coverage: complete` and invites players to
uncover local requests. It has 30 contacts, but empty `stories` and
`exclusions` arrays, and it does not map mobile 39130
([coverage and introduction](../../../areas/story/ugta.story.json#L5),
[contacts](../../../areas/story/ugta.story.json#L11),
[empty story/exclusion lists](../../../areas/story/ugta.story.json#L284)).
That is a source-mapping gap to confirm with the area owner; the sidecar alone
does not establish a local quest chain.

## Environmental story

| Setting | Source evidence | What the authored text establishes |
| --- | --- | --- |
| Frozen tundra and river crossings | [narrow trail and bridge](../../../areas/wld/ugta.wld#L247), [frigid river room](../../../areas/wld/ugta.wld#L5459), [northern trail](../../../areas/wld/ugta.wld#L6199) | Permafrost preserves wagon tracks and footprints, lichen and snow-covered Akma peaks define the landscape, and the river cuts through the camp. A simple, portable log bridge spans the fast-flowing water. |
| Nomadic camp | [ordinary tent](../../../areas/wld/ugta.wld#L4787), [dining tent](../../../areas/wld/ugta.wld#L5370), [tool seller](../../../areas/mob/ugta.mob#L158) | Caribou-skin tents, campfires, cots, animal fat, and stone tools make the camp materially spare but inhabited. A much larger dining tent has ordered tables, a raised head table, and space for gatherings after hunts or visiting VIPs. |
| Leadership and craft | [Atgu's tent](../../../areas/wld/ugta.wld#L5634), [shaman's tent](../../../areas/wld/ugta.wld#L5417), [Atgu](../../../areas/mob/ugta.mob#L272), [elder shaman](../../../areas/mob/ugta.mob#L243) | Atgu's tent has tapestries, a raised mahogany chair, and a red-carpeted dais. The shaman's larger tent holds writings about mixtures and calling spirits; the elderly shaman is also a shopkeeper. Guards are stationed at bridges, entrances, and the king's household. |
| Trade and travel | [food seller](../../../areas/mob/ugta.mob#L132), [arms seller](../../../areas/mob/ugta.mob#L145), [clothing seller](../../../areas/mob/ugta.mob#L301), [boat seller](../../../areas/mob/ugta.mob#L328) | Food, weapons, tools, shaman goods, warm clothing, containers, and boats are sold inside the camp. The boat shop is a travel cue in the source, not evidence that a usable water route exists. |
| Gate and confinement | [gate approach](../../../areas/wld/ugta.wld#L6120), [guardian reset](../../../areas/zon/ugta.zon#L458), [guardian message](../../../areas/qst/ugta.qst#L189), [Atgu greeting](../../../areas/qst/ugta.qst#L123), [bone-gated cell](../../../areas/wld/ugta.wld#L6325) | The guardian stands on a trail beside a massive gate and announces that outsiders are unwelcome. Atgu's greeting asks how the visitor got inside. A small cave and cell are divided by a caribou-bone gate. The source presents both civic access control and a prison, but does not explain the intended visitor route or prisoner release. |

These scenes create a coherent survival settlement with a distinct social
hierarchy and guarded boundary. They also leave an explicit access question:
the area is active in the full world, its residents have conversation entries,
and its guardian says the gates are closed to outsiders.

## Residents, resets, and shops

The seven shop records each match a reset placement for the same keeper in the
configured shop room:

| Keeper | Role | Room | Shop record | Keeper reset |
| --- | --- | ---: | --- | --- |
| 39109 | Food seller | 39275 | [shop](../../../areas/shp/ugta.shp#L1) | [reset](../../../areas/zon/ugta.zon#L276) |
| 39110 | Arms seller | 39278 | [shop](../../../areas/shp/ugta.shp#L38) | [reset](../../../areas/zon/ugta.zon#L284) |
| 39111 | Tool seller | 39279 | [shop](../../../areas/shp/ugta.shp#L86) | [reset](../../../areas/zon/ugta.zon#L298) |
| 39117 | Elder shaman | 39299 | [shop](../../../areas/shp/ugta.shp#L126) | [reset](../../../areas/zon/ugta.zon#L361) |
| 39121 | Clothing seller | 39311 | [shop](../../../areas/shp/ugta.shp#L174) | [reset](../../../areas/zon/ugta.zon#L404) |
| 39122 | Container seller | 39316 | [shop](../../../areas/shp/ugta.shp#L213) | [reset](../../../areas/zon/ugta.zon#L423) |
| 39123 | Boat seller | 39325 | [shop](../../../areas/shp/ugta.shp#L256) | [reset](../../../areas/zon/ugta.zon#L450) |

The ZON file contains 441 typed commands: 136 M, 116 O, 30 P, 76 E, 79 G, and
four D commands. No foreign mobile prototype is used by Ugta's mobile resets.
The outbound object references in ZON and SHP resolve to local Ugta objects or
to object prototypes from active Heavens and Verz1 sources
([Heavens entry](../../../areas/AREA#L2),
[Verz1 entry](../../../areas/AREA#L15)). The external set is
Heavens 90, 105–109, 111, 353, 363, 368–369, 376–377, 385, 388–389, 429, 431,
603, 646, and 667–676, plus Verz1 3097. Representative definitions are
[Heavens 90](../../../areas/obj/heavens.obj#L1171),
[Heavens 667](../../../areas/obj/heavens.obj#L6736), and
[Verz1 3097](../../../areas/obj/verz1.obj#L1199). Heavens 90 and 105–109 also
have unregistered Mavulsk source copies
([example](../../../areas/obj/mavulsk.obj#L1469)); the normal active source list
supplies the Heavens definitions. Preserve those numeric identities while that
source ownership is resolved.

The static typed scan found no active ZON/SHP references into Ugta's local
mobile or object ranges and no numeric references to those ranges in other
current QST sources. These are source scans, not a claim that every code,
editor, or saved-data dependency has been traced.

## Doors and static connectivity

The full-world register counts 481 authored direction records, all with targets
present in the active room source set. Ugta has one active cross-area edge in
each direction: room 39335 leads south to Surface room 543298, whose north exit
returns to 39335 ([Ugta edge](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L549),
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1422)). There are no
negative targets or unresolved positive targets in the Ugta register row
([coverage row](ZONE_COVERAGE_REGISTER.csv#L323)).

The static local weak-exit graph has five components. Its largest contains 238
rooms; four rooms are singletons: 39300 (barren tundra), 39301 (the frigid
Ugta River), 39302 (tundra whose description says tents stand to the south),
and 39303 (inside a caribou-skin tent)
([39300–39303](../../../areas/wld/ugta.wld#L5439)). These may be deliberate
loading or decorative rooms, or may lack intended route records. The graph does
not show actual player reachability.

There is also an unregistered, source-only inbound WLD record: `surf.wld` room
209731 has a north exit (`D0`) to Ugta room 39335
([source-only `surf` exit](../../../areas/wld/surf.wld#L204203)). It is not in
the active-world cross-area count. This candidate-to-active reference is kept
separate from the live Surface pair and is added to the `surf` retention note in
the [attic plan](ATTIC_MIGRATION_PLAN.md#L27).

The gate approach room 39332 has a closed northward `gates massive` door to
39331; its room record's key field is 0, and the ZON commands set both sides
closed ([gate room](../../../areas/wld/ugta.wld#L6120),
[39331 reset](../../../areas/zon/ugta.zon#L8),
[39332 reset](../../../areas/zon/ugta.zon#L12)). The cave/cell exits are authored
closed and locked on both sides, with key VNUM 0, and the reset commands also
set both sides closed and locked ([cave door](../../../areas/wld/ugta.wld#L6325),
[cell door](../../../areas/wld/ugta.wld#L6346),
[39340 reset](../../../areas/zon/ugta.zon#L16),
[39341 reset](../../../areas/zon/ugta.zon#L20)). The area census records zero
current rooms and zero fall-chance rooms even though the river prose calls the
water fast-flowing. These are source-state comparisons only; no door, key,
current, fall, or route behavior was exercised.

## History and disposition

The normalized all-442 `.zon` title scan finds one exact active “Ugta” title.
Followed `ugta.wld` history reaches the August 2009 `areas/` path migration and
the November 2013 `stuff` change; `ugta.zon` remains on the same stem through
the 2015 quest-item flag pass. No distinct predecessor or whole-zone
replacement appears in these bounded current-source histories.

### Checkpoint 763: title and regional-source comparison

The unregistered `surf.wld` grid has one “UGTA HOMETOWN” tile, room #209431
([label](../../../areas/wld/surf.wld#L197920)). Its south exit targets source
room #209731, “Stone Road in a Northern Valley”; that room's north exit targets
active Ugta room #39335 ([tile exit](../../../areas/wld/surf.wld#L197934),
[source-only road](../../../areas/wld/surf.wld#L204199),
[inbound target](../../../areas/wld/surf.wld#L204206),
[active trail room](../../../areas/wld/ugta.wld#L6199)). The candidate WLD also
uses “The Ugta River” in 54 case-insensitive room-text lines, alongside 19
matching lines in active Ugta WLD. This is a regional surface-grid depiction
and candidate-to-active source route, not a second active Ugta area or active
ingress.

Representative active-source mentions describe Ugta as a homeland or culture:
an Alatorin traveler calls it his native home, Surface's reset names Keira as
Ugta's shamaness, and Willem object text refers to its Jiji-root dye
([traveler](../../../areas/mob/alatorin.mob#L2107),
[Surface reset](../../../areas/zon/surface.zon#L514),
[Willem dye](../../../areas/obj/willem.obj#L180)). Those references do not
identify another active map title. Preserve the exact source-only grid and
approach on Surf candidate row 97 **RETAIN IN PLACE**; keep it out of Ugta's
active ingress count ([candidate plan](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)).

Ugta is an un-commented active `areas/AREA` contributor and is **not an attic
candidate**. Keep its source files in place. The `surf` note above is a
candidate-to-active route and keeps `surf` on the retain-pending-closure list;
it does not change Ugta's active status or the active exit ledger. Runtime
command-target alias ownership and pre-2009 lineage remain outside this bounded
title/source comparison.

## Explicit builder questions and recommended resolutions

Reviewed October 11, 2026. These questions keep Ugta's civic access cues,
conversation text, static routes, shop/reset declarations, and imported object
owners tied to their authored sources.

| # | Builder question | Recommended resolution from current source |
| --- | --- | --- |
| 1 | Should the massive gate remain closed to ordinary visitors, as the guardian says? [guardian message](../../../areas/qst/ugta.qst#L189) · [gate room](../../../areas/wld/ugta.wld#L6120) · [gate resets](../../../areas/zon/ugta.zon#L8) | Preserve the authored closed gate and exclusion message in source summaries. Confirm the intended visitor policy before changing the gate state or presenting the city as open. |
| 2 | How should the guardian's exclusion line fit with Atgu asking how a visitor got inside and the reciprocal Surface path at #39335? [Atgu greeting](../../../areas/qst/ugta.qst#L123) · [active Surface pair](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L549) | Keep dialogue and WLD boundary records separate. The active route does not explain lawful admission or resolve the story cue; do not infer player access or a contradiction-free route. |
| 3 | Does `qc_action 70` announce the gate policy, or should it enforce access? [guardian QST entry](../../../areas/qst/ugta.qst#L189) · [action handler](../../../src/world/quest.c#L88) · [ask check](../../../src/world/quest.c#L2438) | The handler treats this as a `CMD_NONE` room action, and the ask check skips it. Preserve it as an announcement unless a separate access mechanism is authored; do not call it an ask/contract entry. |
| 4 | What does the sidecar's `coverage: complete` cover when it has 30 contacts but empty `stories` and `exclusions` arrays? [coverage and contact list](../../../areas/story/ugta.story.json#L5) · [story/exclusion arrays](../../../areas/story/ugta.story.json#L284) | Scope “complete” to the contact inventory only if that was intended. Keep the source-mapping gaps visible until actual story and exclusion records are confirmed. |
| 5 | Should the sidecar introduction promise that players can uncover local requests? [introduction](../../../areas/story/ugta.story.json#L7) · [QST inventory](../../../areas/qst/ugta.qst#L1) | Keep the promise only if the owner can identify the corresponding request sources. The current reviewed text does not establish a local request chain or reward. |
| 6 | Should mobile #39130, whose QST text is the gate announcement, be included among the sidecar contacts? [guardian entry](../../../areas/qst/ugta.qst#L189) · [contact list](../../../areas/story/ugta.story.json#L11) | Confirm whether the 30-contact list aims to cover every local QST giver. If so, map #39130 as a civic announcement and keep it distinct from a quest contact. |
| 7 | Is mobile #39112's `quest` topic (“I hate twinks”) intended as flavor conversation only? [QST topic](../../../areas/qst/ugta.qst#L77) | Treat the response as dialogue. It defines no objective or reward, so do not derive a quest or achievement from the topic label alone. |
| 8 | Should the 31 local QST giver records be presented as 31 actionable quests? [QST source](../../../areas/qst/ugta.qst#L1) · [coverage row](ZONE_COVERAGE_REGISTER.csv#L323) | Keep conversation records separate from native Q/QA contracts. The reviewed inventory has zero native contracts; add objectives and receipts only when an owner defines their actual sources and outcomes. |
| 9 | Are the four single-room WLD components at #39300–#39303 intentional? [room records](../../../areas/wld/ugta.wld#L5439) · [connectivity review](#doors-and-static-connectivity) | Preserve the measured five-component graph and flag these rooms for builder review. Do not add exits or describe them as reachable based on prose alone. |
| 10 | Should #39302's text that a tent stands directly south correspond to an authored south exit? [room #39302](../../../areas/wld/ugta.wld#L5472) | Keep the prose and exit records distinct until the builder confirms whether a route is missing or the description needs correction. Do not invent a connection. |
| 11 | How is room #39303, described as inside a caribou-skin tent, meant to be entered while it remains a singleton? [tent room](../../../areas/wld/ugta.wld#L5486) | Preserve the interior description and graph result. Record access as unresolved until an authored route or intended isolated-scene purpose is confirmed. |
| 12 | Should the cold-river and log-bridge prose promise safe crossing behavior? [river room](../../../areas/wld/ugta.wld#L5459) · [bridge description](../../../areas/wld/ugta.wld#L247) · [coverage row](ZONE_COVERAGE_REGISTER.csv#L323) | Keep environmental prose separate from movement mechanics. The static census records no current or fall fields; do not infer current strength, swim safety, or guaranteed crossing outcomes. |
| 13 | What destination or travel service is meant by the boat seller telling visitors they need a boat? [seller dialogue](../../../areas/mob/ugta.mob#L328) · [boat shop](../../../areas/shp/ugta.shp#L256) | Treat the line as a travel cue, not proof of an available water route. Confirm destination, activation, and return behavior before describing boat travel as usable. |
| 14 | How is the north gate between #39332 and #39331 meant to be opened when both sides reset closed and the key fields are 0? [gate record](../../../areas/wld/ugta.wld#L6120) · [resets](../../../areas/zon/ugta.zon#L8) | Preserve the authored initial closed state. Confirm whether opening is intentional, scripted, or unavailable before adding a key or claiming the gate is passable. |
| 15 | What release action is intended for the prisoner behind the caribou-bone cell gate? [cell room](../../../areas/wld/ugta.wld#L6346) · [cell reset](../../../areas/zon/ugta.zon#L20) | Preserve the source's prisoner-release cue and the locked/closed reset state. No release action is established by these records; do not invent one. |
| 16 | Is simple iron key #39167 meant to open that cell even though the door key fields are 0? [key object](../../../areas/obj/ugta.obj#L990) · [key reset](../../../areas/zon/ugta.zon#L422) · [door record](../../../areas/wld/ugta.wld#L6346) | Keep the key's reset location and the door's key field as separate facts. Do not assign the key to the prison gate unless a matching source reference is confirmed. |
| 17 | Should the unregistered `surf.wld` north exit to #39335 count as an active Ugta approach? [source-only exit](../../../areas/wld/surf.wld#L204203) · [active exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L549) | Keep the inactive-source route out of active ingress counts and preserve it in the existing Surf retention note. It does not establish live access to Ugta. |
| 18 | Should Ugta's reset and shop references to Heavens and Verz1 objects retain those external prototype owners? [Heavens source](../../../areas/obj/heavens.obj#L1171) · [Verz1 source](../../../areas/obj/verz1.obj#L1199) · [Ugta reset examples](../../../areas/zon/ugta.zon#L368) | Preserve the exact imported VNUM references and named source owners while their ownership is reviewed. Do not create local duplicates or transfer ownership by implication. |
| 19 | Do the seven shop records and matching keeper resets promise that each service is available to players now? [shop records](../../../areas/shp/ugta.shp#L1) · [food keeper reset](../../../areas/zon/ugta.zon#L276) · [boat keeper reset](../../../areas/zon/ugta.zon#L450) | Preserve the static keeper/shop/room matches. A reset and shop record do not establish live stock, open hours, actor presence, or player access. |
| 20 | Should Atgu's raised dais, the shaman's writings, and the dining tent imply a specific ceremony or political event? [Atgu's tent](../../../areas/wld/ugta.wld#L5634) · [shaman's tent](../../../areas/wld/ugta.wld#L5417) · [dining tent](../../../areas/wld/ugta.wld#L5370) | Preserve the described leadership and craft cues. The source suggests gatherings after hunts or VIP visits but does not define a named rite, quest, or political event. |

These recommendations describe the current source records. They do not
establish lawful entry, active QST outcomes, live shop state, cell release,
boat travel, or safe river crossing.
## Checkpoint 865: current source-only Ugta approach carried forward

CP865 carries forward Ugta's Checkpoint 763 title/history comparison and the source-backed map/dependency review against the unchanged pinned area/code snapshot. The active WLD remains 242 rooms and 481/481 resolving exits with five weak components: a 238-room component and four singleton rooms #39300–#39303. The only active boundary is reciprocal with Surface at Ugta #39335 / Surface #543298. Gate and cell-door reset states remain closed (cell doors also locked with key fields 0); the source records no fall or current fields.

The single unregistered `surf.wld` approach remains room #209731 north to Ugta #39335, downstream of the source-only “UGTA HOMETOWN” tile #209431. It is not active ingress and remains on existing Surf candidate row 97. The surrounding tile/road path is regional candidate-source evidence, not a second active Ugta map. Current cumulative source-only totals remain 11,993 overall / 415 Surf.

The typed review remains bounded to its mapped static sources: retain the exact Heavens and Verz1 imported-object owners, the reciprocal Surface edge, seven source-defined shop/reset pairs, and the source index/file discrepancy (no local `ugta.qst` despite QST/AREA membership). CP763's unique-title and same-stem WLD/ZON history result remains complete. Keep active Ugta **DO NOT MOVE** and add no Ugta attic candidate. Live gate operation, lawful entry, prisoner release, shop availability, safe river crossing, code/editor dependencies, and runtime command-target aliases remain outside this static qualification; preserve the source-only Surf chain without treating it as active access.
