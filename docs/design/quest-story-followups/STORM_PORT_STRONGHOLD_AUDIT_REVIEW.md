# Storm Port Stronghold audit supplement

**Checkpoint 53 — active and source-only connectivity, exact typed
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/STORM_PORT_STRONGHOLD.md) remains
the authority for the four independent contracts, local resets, access,
services, foreign recipes, payment defect, and the already-repaired outpost
direction. This supplement records bounded cross-area evidence without
changing native sources.

## Active WLD boundaries and source-only ingress

Storm Port Stronghold's 130 authored exits all resolve across nine local weak
components; the largest contains 50 rooms. Three Stronghold exits have
reciprocal Surface records:

| Stronghold exit | Surface return | Evidence rows |
| --- | --- | --- |
| #22600 north → #636281 | #636281 south → #22600 | [out](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L370); [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1666) |
| #22600 west → #636680 | #636680 east → #22600 | [out](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L371); [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1668) |
| #22634 east → #636682 | #636682 west → #22634 | [out](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L372); [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1669) |

These records describe authored links, not door state or safe player travel.

The multi-zone load room #22649 has six authored exits to Desolate #22255,
Cerebusp #22113, Mira #82540, Divine Home #40717, Storm Port #22423, and Jade
Mini #76616 ([source room](../../../areas/wld/spshold.wld#L989),
[first outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L373),
[last outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L378)). None has a
reverse-direction record, and the active WLD census identifies no incoming
exit to #22649. This is an authored load hub without a static entrance; it is
not evidence for teleporting or otherwise placing a player there.

A stateful parse of all 95 WLD files outside the active `areas/AREA` list read
2,080,773 exits from 527,791 source rooms, with zero parse issues and no
source-room VNUM overlap with the Stronghold. One source-only exit points into
the active area: unregistered `surf.wld` room #285640, “A Deep and Dark
Forest,” west to Stronghold room #22634 ([room](../../../areas/wld/surf.wld#L1797072),
[exit](../../../areas/wld/surf.wld#L1797088),
[target tuple](../../../areas/wld/surf.wld#L1797091)). Because `surf.wld` is
not listed in `areas/AREA`, this is a source-only reference, not active-world
ingress. It is the 27th individually reviewed `surf.wld` boundary record and
raises the curated source-only follow-up count from 586 to 587. The full scan
found no other source-only target into Stronghold rooms.

## Bounded typed and code references

Exact typed scans found these active references to Stronghold prototypes:

| Source | Exact reference | Context |
| --- | --- | --- |
| Vehicles reset | `G` object #22621 at line 825 and object #22631 at line 840 ([first](../../../areas/zon/vehicles.zon#L825); [second](../../../areas/zon/vehicles.zon#L840)). | The comprehensive source map traces each to a 20% gear reset for the two 25% vehicle-avatar placements at room #47197. A declared reset is not proof of current stock or access. |
| Divine Home QST | `R I 22625` ([source](../../../areas/qst/divhome.qst#L72)). | Foreign exchange reward reference. |
| Jade Mini QST | `G I 22622` ([source](../../../areas/qst/jademini.qst#L45)). | The helmsman's sea-map exchange consumes this material. |
| Mira QST | `R I 22631` ([source](../../../areas/qst/mira.qst#L142)). | Foreign exchange reward reference. |
| Winterhaven QST | `G I 22631` ([source](../../../areas/qst/wh.qst#L1176)). | Foreign equipment request consumes this material. |

No active SHP record had an exact local prototype reference. A structured scan
of the other 231 story sidecars parsed without errors. Three structured
numeric fields match Stronghold prototypes: Jade Mini's sea maps #22622,
Thetis's Hordine contact #22626, and Winterhaven's item #22631. Completion-key
strings also repeat QST item numbers in Divine Home #22625, Jade Mini #22622,
Mira #22631, and Winterhaven #22631; they mirror the native contracts rather
than add new transactions ([Divine Home](../../../areas/story/divhome.story.json#L434),
[Jade Mini](../../../areas/story/jademini.story.json#L286),
[Mira](../../../areas/story/mira.story.json#L809),
[Thetis Hordine contact](../../../areas/story/thetis.story.json#L72),
[Winterhaven](../../../areas/story/wh.story.json#L903)).

The exact C/C++ scan found only Stronghold's local bindings: object #22621 to
`master_set`, room #22648 to `crew_shop_proc`, the room's crew-shop and ship
variable entries, and the master-set adapter/list entries
([object assignment](../../../src/specs/specs.assign.c#L1341),
[room assignment](../../../src/specs/specs.assign.c#L2410),
[shop registry](../../../src/ships/ship_shop.c#L3239),
[ship variable](../../../src/ships/ship_variables.c#L235),
[master set](../../../src/specs/specs.set.c#L47),
[set adapter](../../../src/specs/specs.set_equipment.c#L419)). No additional
foreign code literal matched a local room, mobile, or object VNUM. The set
adapter includes #82559 while the master-set list omits it; the dossier keeps
that existing membership question separate from quest credit.

## Active title and tracked history

Color-code stripping, whitespace collapse, and case folding across all 351
active `.zon` titles produce one “Storm Port Stronghold” title, owned by
`spshold` ([manifest entry](../../../areas/AREA#L114),
[zone title](../../../areas/zon/spshold.zon#L2)). The tracked WLD and ZON paths
reach the August 2009 area-tree relocation. Later path history records a 2015
quest-item flag update. The October 2026 WLD change
([commit](https://github.com/Community-Duris/Duris/commit/4b25c5f177ebb11d4a2eabab286b5c2a2e5a55f9))
corrects the outside-path direction text to west; it does not change the
Stronghold exit graph. No distinct predecessor or replacement is identified.

## Scope and disposition

Storm Port Stronghold remains an active `areas/AREA` contributor. The six
one-way load-room edges and the unregistered `surf.wld` exit remain source
facts with unresolved player access and map intent; only the three reciprocal
Surface pairs have reverse WLD records. Foreign QST/reset/story references
remain owned by their source zones. Travel safety, return routes, service
transactions, reset renewal, and persisted-player outcomes remain unverified.
No migration or gameplay decision follows from this review.

This documentation-only review used source and static registers. It did not
run tests, a build, server/runtime/gameplay, database work, a migration, or an
operational script. The source-comprehensive audit queue is now **93/220
complete, 127 pending**; priority 94, The Mountain Settlement of the Harpies
(`harpyht`), is next.


## Checkpoint 369 builder-question coverage

Physical coverage line 282, `area:spshold`, already had its source map, active graph, typed-dependency closure, title/history review, and the Checkpoint 53 cross-area supplement. This checkpoint completes the coverage register's builder-question review with twenty source-backed questions and conservative recommendations. The full [Storm Port Stronghold story map](../zone-stories/STORM_PORT_STRONGHOLD.md) and this [cross-area audit](STORM_PORT_STRONGHOLD_AUDIT_REVIEW.md) remain the detailed source references.

### Builder questions and conservative recommendations

1. Should `spshold` remain the active owner of its room, mobile, object, reset, quest, story, and ship-service identities? Keep it **DO NOT MOVE**; review any owner or map-list proposal against all of these maintained bindings.
2. Should the three reciprocal Surface pairs remain the Stronghold's ordinary WLD borders? Preserve the exact #22600/#636281, #22600/#636680, and #22634/#636682 pairs; the westward prose correction in #22607 changes no route.
3. Should the six external exits from load room #22649 remain one-way, with no incoming active WLD direction? Preserve each destination and its source owner; qualify any dynamic placement or return separately.
4. Should the inactive [Surf #285640 west exit to #22634](../../../areas/wld/surf.wld#L1797088) remain source-only? Retain that exact edge on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); the source file is not active ingress.
5. Is the 100-percent F10 fall risk at #22621 intentional? Preserve the room marker while builders qualify arrival, fall handling, rescue, and return behavior.
6. Should the fixed ladders and furnace portals keep their current type-25 targets and returns? Preserve the authored #22617/#22618 ladder and #22604/#22605 furnace records; do not turn a ticket exchange into launch or arrival credit.
7. Should the captain's exact coal #22610 plus valve #22612 exchange continue to produce ticket #22613? Keep the native bundle and hidden capped sources; qualify source custody and renewed supply before relying on repeat availability.
8. Should Decker continue accepting a supplied ticket for helm #22621 without requiring the player's own captain receipt or a sailed trip? Preserve the independent exchange; do not infer travel from the ticket or the helm.
9. Should Hordine's sea-map exchange keep its separate outputs, including the imported greaves #40771? Preserve the exact local QST result and keep the competing Oberon helmsman consumption under that foreign source owner.
10. Should Aphantan begin at #22649 with no active static entrance and remain able to roam away? Preserve the authored mobile behavior; qualify a legitimate current source and item visibility before describing a fixed recovery path.
11. Should the four native exchanges be treated as repeatable daily work? Keep the current contract definitions and reset mode; verify cap-1 supply, renewal, custody, and ready accounting before making any repeatability promise.
12. Should the torn treasure map #77209 remain distinct from the sea-map packet #22622? Preserve the separate prototypes and accepted QST bundles; do not substitute one map or receipt for another.
13. Should Burgadan's Thetis-owned exchange remain an optional way to return torn map #77209 and receive spade #38037? Preserve its foreign contract and keep its receipt out of Stronghold completion requirements.
14. Should the paired silt exits #77261/#77262 remain secret and key-controlled by the deep-sea spade? Preserve both reset directions and their separate access owner; do not present `DIG` as the route mechanic.
15. Should the spade and treasure key #22633 retain their 100-percent break chance on ordinary keyed UNLOCK? Keep the authored break values and qualify replacement sources and recovery before promising repeated access.
16. Should chest #77210 remain fixed keyed treasure access after the map exchange? Preserve its container, key, contents, and foreign room owner; treat chest entry and loot as separate outcomes.
17. Should crew hiring at #22648 be enabled while `crew_shop_proc` ignores a failed `SUB_MONEY` result? Refuse active HIRE until typed payment and crew publication settle together; track the shared accounting repair separately from this documentation review.
18. Should merchant or inn prose imply a local shop or lodging service? Keep local SHP count at zero and retain the existing crew-shop procedure separately; the inn room has no `ROOM_INN` assignment.
19. Should outside QST, story, and vehicle reset references to Stronghold objects remain owned by their source areas? Preserve the exact foreign references and avoid turning them into Stronghold contracts or local stock guarantees.
20. Should Master-set membership include #82559 when the adapter and underlying master-set list disagree? Preserve the current lists pending an owner decision; keep equipment-set effects separate from the Decker exchange and story credit.

These recommendations complete the builder-question review; owner decisions remain open. The known Surf edge is already included in the source-only candidate tally, so this checkpoint adds no route record and changes no source behavior. The review is documentation-only; no build, test, runtime, database, or migration action was performed.


### Checkpoint 754: Storm Port and Stronghold identity boundaries

A normalized scan of all 442 current `.zon` titles finds one exact “Storm Port
Stronghold” owner, active `spshold`. The neighboring `stormport` contributor has
its own “Storm Port” title and separate *224–225 entries in both area rosters;
`spshold` is separately listed at *226–227. Stronghold load room #22649 has a
one-way DOWN exit to Storm Port room #22423, with no active incoming exit to
#22649. The direct connection describes complementary area identities rather
than a renamed or replaced map ([registered edge](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L377),
[Stronghold room](../../../areas/wld/spshold.wld#L989),
[Storm Port room](../../../areas/wld/stormport.wld#L491)). Other title files
sharing only the generic words `Storm`, `Port`, or `Stronghold` have distinct
titles and keys.

Jade is another separately listed contributor, titled “The Jade Empire.” Its
rooms #76631–#76632 describe Jade's leased Storm Port authority outpost and
draw a boundary between the Jade capital and that outpost
([Jade title](../../../areas/zon/jade.zon#L2),
[district gate](../../../areas/wld/jade.wld#L697),
[district interior](../../../areas/wld/jade.wld#L721)). This is a region inside
Jade's own WLD and VNUM range, not a second Stronghold map. The SurfaceKeeps
QST explicitly identifies Storm Port Stronghold as the place from which the
Hunter-Killers emerge, and a Herders mobile recalls having been a Stronghold
captain ([QST](../../../areas/qst/surfacekeeps.qst#L546),
[mobile](../../../areas/mob/herders.mob#L2104)); these are cross-area story
references, not alternate map declarations. The Stormport medallion, Storm
Port orc, traveler, and navy descriptions likewise do not define another map.

The existing all-95 inactive-WLD review found one source-only approach,
`surf.wld` #285640 west to Stronghold #22634; retain it under existing Surf
candidate row 97. The candidate register contains no `spshold` candidate key,
and no candidate plan changes here. Followed WLD/MOB/OBJ/ZON/QST histories
remain on the `spshold` stem from the 2009 area-tree move. The 2026 WLD edit
corrects the outside-path direction clue without changing exits; the story
sidecar was added in 2026. No alternate-stem predecessor or explicit
whole-zone replacement was identified. The title and semantic map-identity
review is complete; command-target alias and owner disambiguation remain
separate.

## Checkpoint 824: current typed-source reconciliation

A fresh exact scan of the current checkout reproduces Stronghold's bounded external typed closure. Active `vehicles.zon` contains the only external reset-role references: `G` loads local objects #22621 and #22631 at lines 825 and 840. Four active foreign QST item operands remain: Divine Home `R I 22625`, Jade Mini `G I 22622`, Mira `R I 22631`, and Winterhaven `G I 22631`. No external SHP record refers to a Stronghold prototype, and no local `.shp` file exists.

All 232 current story sidecars parse without errors (the 231 other than Stronghold were checked independently). External structured fields remain Jade Mini item #22622, Thetis contact mobile #22626, and Winterhaven item #22631; completion-key strings in Divine Home, Jade Mini, Mira, and Winterhaven mirror their exact QST item operands. The current C/C++ numeric scan again finds only Stronghold's local master-set, crew-shop, ship-variable, and set-list/adapter bindings; no additional foreign code literal matches a local room, mobile, or object. See the [Checkpoint 53 typed-source table](#bounded-typed-and-code-references) for source links and recipe ownership.

Across 19,149 active object prototypes, 713 are type 25 and 362 are type 29. The only type-25 destinations into Stronghold are four local portals: [#22604 to #22647](../../../areas/obj/spshold.obj#L60), [#22605 to #22641](../../../areas/obj/spshold.obj#L73), [#22617 to #22640](../../../areas/obj/spshold.obj#L247), and [#22618 to #22631](../../../areas/obj/spshold.obj#L260). No foreign fixed or random type-25 destination reaches Stronghold, and no type-29 target-room field names a Stronghold room. Exact WLD exit-tuple scans across current `.wld` files likewise find no foreign key field using a Stronghold object: the only matching key #22602 appears in two local `spshold.wld` exits ([line 275](../../../areas/wld/spshold.wld#L275), [line 745](../../../areas/wld/spshold.wld#L745)). Same-type active room, mobile, and object overlap remains zero.

A repository-wide exact range scan of external `.wld` sources confirms the three active Surface returns—#636281 south and #636680 east to #22600, plus #636682 west to #22634 ([register rows 1666, 1668, and 1669](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1666))—and one unregistered Surf exit, #285640 west to #22634. No other current `.wld` file contains a Stronghold-range match. The stateful inactive-WLD scan finds no room overlap and only that source-only approach; retain it under existing Surf candidate row 97. The current check adds no route or dependency, changes no native source, and does not qualify reset availability, service settlement, live access, dynamic travel, or persisted-player state. Keep active `spshold` **DO NOT MOVE** and leave the no-candidate disposition unchanged.
