# Thetis's Realm repository-wide audit supplement

This supplement reconciles active and inactive WLD boundaries, typed cross-zone dependencies, normalized title ownership, and bounded source history for the [source-comprehensive Thetis dossier](../zone-stories/THETIS_REALM.md). The frozen source snapshot is 72ba3deff3bb30bde880018cdebe7c6db675b697. No area or gameplay source is changed.

## Active graph and boundary exits

The active thetis WLD contains 100 rooms and 218 authored exits. The current loader resolves 217: 215 local exits and two outbound cross-area exits. One weak component contains all 100 rooms. Raw directed traversal from beach room #38000 reaches all 100 rooms and has authored returns from all 100. There are no fall or current fields. These counts do not model doors, keys, switches, hazards, actor admission, dynamic travel, or safe traversal.

| Boundary | Current classification | Evidence |
| --- | --- | --- |
| #38000 south → Surface #618491 | Outbound only; no active reverse exit and no active inbound record to Thetis. | [Beach exit](../../../areas/wld/thetis.wld#L19); [cross-area pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L655) |
| #38075 south → Heavens #38 | Outbound only; no active reverse exit and no active inbound record to Thetis. | [Tunnel exit](../../../areas/wld/thetis.wld#L1418); [cross-area pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L654) |
| #38095 north → #104 | Positive unresolved target; excluded from the loaded graph. | [Starfish Lane exit](../../../areas/wld/thetis.wld#L1794); [unresolved register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L209) |

The unresolved register records #104 as present in inactive dalvik and mavulsk WLD sources ([Dalvik room](../../../areas/wld/dalvik.wld#L2566), [Mavulsk room](../../../areas/wld/mavulsk.wld#L2223)). Dalvik’s room is the Great Temple of Ubtao; Mavulsk’s is a quiet library. Both sources are outside areas/AREA, so neither supplies a loaded destination. Preserve the exact target as unresolved until its intended owner and route are established; do not infer a replacement room. The [Mavulsk source audit](MAVULSK_AUDIT_REVIEW.md#thetis-target-104) records the same collision and the separate #188-to-#228202 Surf candidate target.

## Inactive-WLD approaches and cumulative evidence

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits, with no parse issues or room-VNUM overlap with Thetis. It found one non-overlapping source-only approach: Surf room #251543 north → Thetis #38000 ([source room](../../../areas/wld/surf.wld#L1081539), [exit](../../../areas/wld/surf.wld#L1081543)). This edge is absent from active ingress. The original Thetis checkpoint raised its then-current source-only total from 2,615 to 2,616 and the then-current Surf subset from 155 to 156; those cumulative totals are superseded by Checkpoint 387 below. Retain surf and surface2011 pending map ownership and complete dependency/tool closure; keep active thetis DO NOT MOVE.

## Typed dependencies and scoped negatives

| Dependency surface | Source-backed result and boundary |
| --- | --- |
| Jade Mini Zones map and Thetis Q37 | Jade Mini Zones resets torn map #77209 on pirate #77219 in room #77248 ([pirate resets](../../../areas/zon/jademini.zon#L86), [map reset](../../../areas/zon/jademini.zon#L106)). Thetis Q37 references the imported map and local spade #38037 ([native contract](../../../areas/qst/thetis.qst#L50)); the source-comprehensive dossier maps the exchange and returned map. Hordine's Stronghold map exchange is a separate owner and can precede Burgadan. |
| Thetis prototypes placed in Jade Mini Zones | Active jademini.zon has two reset records for local mobile #38026 in room #77258 and two for #38011 in room #77259 ([reset records](../../../areas/zon/jademini.zon#L112)). The jademini story sidecar has typed contacts for #38011 and #38026 ([jellyfish](../../../areas/story/jademini.story.json#L23), [turtle](../../../areas/story/jademini.story.json#L30)). These placements and contacts do not make Thetis an active route prerequisite. |
| Stronghold story references | spshold.story.json has an optional giver_vnum reference to Burgadan #38037 and an item_vnums reference to the Thetis spade #38037 ([giver field](../../../areas/story/spshold.story.json#L220), [spade field](../../../areas/story/spshold.story.json#L243)). They document the distinct optional Burgadan exchange and later spade material; the numeric tokens in prose and completion keys are not additional typed references. |
| Local shop and imported prototype | Thetis has one local SHP record, Salty #38033 ([shop source](../../../areas/shp/thetis.shp#L1)); coverage row 300 now records that source instead of zero shops. The Q37 map prototype #77209 is imported from Jade Mini Zones. |
| Scoped negative results | The bounded typed scans find no outside native QST item consumer or SHP product for a Thetis prototype, no outside type-25 portal target into Thetis, and no duplicate external mobile/object prototype. The external sidecar hits are the typed roles above plus string/prose/completion-key tokens; the scan does not establish live sightings, item custody, or played quest order. |

These are independent prototype placements, story annotations, and item exchanges. They do not establish a mandatory map-to-Stronghold quest chain, source recovery, a living arrival, or safe access.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “Thetis's Realm” has one exact title owner among the 351 active .zon contributors: thetis ([title source](../../../areas/zon/thetis.zon#L2)). The bounded source history inspected stays on the thetis WLD stem and shows no alternate-stem rename, explicit predecessor, or whole-zone replacement. Keep the active thetis contributor DO NOT MOVE. Retain the unresolved #104 references and the source-only Surf approach with their source maps until builders establish intent; no automatic topology or story repair follows from these records.

## Builder recommendations

1. Preserve the Surface and Heavens exits as separate outbound-only leaves; do not imply an authored return.
2. Keep #38095 north → #104 unresolved until the intended destination and ownership are verified against source history.
3. Preserve Surf #251543 as source-only evidence and keep surf/surface2011 RETAIN.
4. Keep the optional Thetis, Jade Mini Zones, and Stronghold story roles separately owned; a shared prototype reference does not imply mandatory order or player discovery.
5. Qualify actual door state, key loss, underwater survival, current stock, foreign Hordine exchange, and persistence in a later gameplay pass before describing them as played outcomes.

## Checkpoint 387: active boundary refresh, Surf approach, and builder questions

This refresh uses the current 351-area manifest and 95-source WLD sweep. The
100-room active graph has 218 authored exit records: 215 resolve locally, two
leave the area, and one positive target (#104) is missing from the active
manifest. There are no active WLD arrivals. The graph is one weak component;
raw directed traversal from beach room #38000 reaches all 100 rooms, and all
100 can return to that root. The beach root is not an active WLD arrival.
These are static WLD results and do not qualify a player's entrance, door
choices, underwater survival, or route safety.

The two active outbounds remain #38000 south to Surface #618491 and #38075
south to Heavens #38 ([boundary table](#active-graph-and-boundary-exits)).
#38095 north to #104 remains unresolved; the VNUM exists only in inactive
dalvik and mavulsk WLD sources, outside areas/AREA. Do not infer which inactive
owner, if either, supplies the intended destination.

The all-95 inactive-WLD recheck parsed 2,080,773 exits without issues and found
no room-VNUM overlap with Thetis. It found one source-only approach:
inactive Surf #251543 north to active Thetis #38000
([source room](../../../areas/wld/surf.wld#L1081539),
[exit](../../../areas/wld/surf.wld#L1081543)). This does not create active
ingress. Keep it on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
The reviewed Surf subset increases from 291 to 292, and the curated source-only
WLD total increases from 9,473 to 9,474. Surf's 145 active unresolved
reference records, 145 count, and 17 source-area keys remain unchanged.

The normalized title “Thetis's Realm” is unique among all 442 current ZON
files. Same-type active WLD, MOB, and OBJ comparisons find no collision, and
the inactive WLD scan finds no room overlap. Followed thetis WLD and ZON
histories stay on the same stem; the bounded histories establish no
alternate-stem rename, distinct predecessor, or whole-zone replacement.
Keep active thetis DO NOT MOVE. The existing typed closure remains: three
independent native QST requests, Jade Mini Zones resets of Thetis mobiles
#38011/#38026, the imported map/spade exchange, optional Stronghold
cross-references, the local Salty shop, and no external type-25 destination
into Thetis. Those records preserve item and story owners without proving
availability or a mandatory quest chain.

The five-item Builder recommendations section above is the earlier condensed
checklist. The following twenty questions consolidate it into source-backed
owner decisions.

| # | Builder question and source evidence | Recommended resolution |
| --- | --- | --- |
| 1 | The active WLD has no inbound boundary, although all 100 rooms are directed-reachable from beach #38000. Is the missing active entrance intentional? | Confirm any authored entrance or approved alternate arrival; do not infer access from the local graph alone. |
| 2 | Beach #38000 exits south to Surface #618491 without a return. Is the one-way Surface boundary intentional? | Keep the exact outbound edge until the Surface owner confirms whether a return belongs in either map. |
| 3 | Tunnel #38075 exits south to Heavens #38 without a return. Is this an administrative arrival, a player route, or a one-way exit? | Confirm the intended users and return owner; do not add a reciprocal direction automatically. |
| 4 | Starfish Lane #38095 north targets #104, which exists only in inactive dalvik and mavulsk. Which destination, if any, did the builder intend? | Resolve its source lineage before changing #104; preserve it as an unresolved active target meanwhile. |
| 5 | Surf #251543 north leads to beach #38000 only in the inactive candidate map. Should that legacy approach remain recorded? | Keep it source-only on Surf candidate row 97; do not load Surf or surface2011 to restore the path. |
| 6 | Do any non-WLD item or script arrivals intentionally supply the missing active ingress? The external type-25 scan found no destination into Thetis. | Name and qualify an exact active route before documenting one; keep the current source-only boundary description. |
| 7 | The single WLD component gives 100/100 reach and return from #38000, but no active route enters there. Should local connectivity stay rooted at this beach? | Preserve the current internal graph unless an intended active arrival changes the builder's route plan. |
| 8 | The six reciprocal cell-door directions use the courtyard key and eligible PICK as alternate access. Is that full access model intended? | Preserve both exact key and pick routes; do not make either route a prerequisite without a builder decision. |
| 9 | Red, green, golden, and skeleton keys have 100% break rolls on successful keyed UNLOCK, while key retirement can be refused separately. Should those source semantics remain? | Keep unlock, break roll, and accepted destruction as separate events; qualify current reset stock before describing retries. |
| 10 | The throne route uses a secret/closed/locked/pickproof direction and an asymmetric return. Is the hidden direction and return design intentional? | Preserve source direction and flag values; confirm the throne path before revising its prose or route. |
| 11 | Room #38083 west to #38001 is a one-way internal return with no reverse direction. Is that deliberate? | Keep the authored one-way edge unless its room owner confirms a paired exit. |
| 12 | The existing source audit separates Hordine's Stronghold map hand-in from Burgadan's Thetis exchange. Should the requests remain optional and independently owned? | Keep the native Thetis Q37 receipt separate from foreign Stronghold Q18 and do not impose a quest chain. |
| 13 | Burgadan accepts map #77209 and returns the map prototype with spade #38037. What identity does a returned map represent? | Preserve the documented receipt as an exchange; do not claim the same UID or source recovery without an explicit receipt rule. |
| 14 | Jade Mini Zones resets Thetis mobiles #38011 and #38026 away from Thetis. Are those foreign-owned encounter placements intentional? | Preserve the Jade reset owner and coordinate any population change with the Jade area owner. |
| 15 | Shared underwater hazards apply to many Thetis rooms, but the WLD has no fall or current fields. Which authored routes require explicit breath or survival cues? | Keep terrain, generic hazards, and player survival separate; add route guidance only after a builder maps intended preparation. |
| 16 | Mask #38004 describes waterbreathing but has no selected WATERBREATH affect. Should its prose or ability be changed? | Ask the item owner to choose a truthful description or a separately balanced effect; do not infer an ability from its name. |
| 17 | Q7, Q26, and Q37 are three native requests with independent recipients and outcomes. Is their independence intended? | Retain separate item checks and receipts; do not merge them into a required three-step chain. |
| 18 | The sidecar and Stronghold card refer to Burgadan #38037 and spade #38037 as an optional bridge. How should players learn of that route? | Keep discovery visibility with each owner; do not grant Stronghold discovery or daily credit from Thetis discovery alone. |
| 19 | Thetis has a mode-1 reset zone and a local Salty #38033 shop. What stock and renewal behavior should the source prose promise? | Keep the authored reset and shop owners; verify current stock and reset episode before promising availability. |
| 20 | Captives, sirens, the clams, the purple potion giver, translation, and turtle rides appear in prose without selected Thetis completion controllers. Which outcomes are intended as quests? | Name an exact controller and outcome for any new objective, or retain the material as lore without inferring a rescue or achievement. |

No topology, prototype, QST, code, or migration change is proposed by this
checkpoint. The questions preserve the current owners and require no gameplay
or test run.

## Checkpoint 842: Current typed closure and source-only ingress refreshed

The bounded typed closure remains the one recorded at Checkpoint 387. The three
local Thetis requests keep independent native receipts; Q37's Jade Mini Zones
map #77209 and Thetis spade #38037 are a separate exchange, and Hordine's
Stronghold references remain optional story material. Jade Mini Zones still
resets Thetis mobiles #38011 and #38026 at its own rooms. The local Salty shop
#38033 remains the only Thetis shop. These contacts, placements, and item
exchanges preserve their current owners without creating an active WLD arrival
or a mandatory map-to-Stronghold chain. ([Thetis Q37](../../../areas/qst/thetis.qst#L50),
[Jade map reset](../../../areas/zon/jademini.zon#L106),
[Jade mobile resets](../../../areas/zon/jademini.zon#L112),
[Stronghold references](../../../areas/story/spshold.story.json#L220),
[local shop](../../../areas/shp/thetis.shp#L1))

The scoped negative closure is unchanged: no outside native QST item consumer,
SHP product, duplicate mobile/object prototype, or type-25 destination into
Thetis was found. Keep the active Surface and Heavens directions as outbound-
only exits, preserve #38095 north → #104 as unresolved, and retain inactive Surf
#251543 north to #38000 only on candidate row 97. This refresh does not qualify
runtime availability, underwater safety, or actual quest order. Keep active
Thetis **DO NOT MOVE**.
