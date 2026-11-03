# Guildhall of the Infernal Empire inactive-source audit

Canonical key: `component:infernal_gh`. Physical coverage row 404 / CSV line
405; candidate dependency row 54 / CSV line 54. Reviewed October 13, 2026,
Checkpoint 903 (baseline source audit Checkpoint 491), against source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static source,
connectivity, numeric-identity, reset, shop, story, and bounded-history review.
It does not establish live loading, gameplay, actual shop boot behavior, saved
world ownership, or permission to activate, rename, move, or archive any file.

## Source inventory, naming, and competing identities

The unlisted component has [WLD](../../../areas/wld/infernal_gh.wld#L1),
[MOB](../../../areas/mob/infernal_gh.mob#L1),
[OBJ](../../../areas/obj/infernal_gh.obj#L1),
[ZON](../../../areas/zon/infernal_gh.zon#L1), and
[SHP](../../../areas/shp/infernal_gh.shp#L1) files. It has no same-stem QST
or story sidecar, its AREA annotation is blank, and neither
[areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists `infernal_gh`. The ZON header
is #294, titled “Guildhall of the Infernal Empire,” with top #29422 and reset
mode 0. The WLD has 16 rooms (#29400–#29412 and #29420–#29422), the MOB has
three prototypes (#29400–#29402), the OBJ has four prototypes (#29400–#29403),
and the SHP has one shop record headed #29401.

The active [areas/AREA entry for Harrow](../../../areas/AREA#L142) owns the
same *294–296 numeric band. All 16 Infernal room VNUMs, all three mobile
VNUMs, and all four object VNUMs have same-type records in active Harrow; every
compared record is different. In particular, the candidate main hall and
outpost rooms have different identities from Harrow's current rooms, candidate
demon slaves and shopkeeper differ from Harrow's vegetables and gardeners, and
all four candidate portals differ from active Harrow objects #29400–#29403.
The active definitions remain the current numeric owners.

The unregistered source family is also crowded: `dark.wld` reuses candidate
rooms #29400–#29409; `darkop.wld` reuses #29410–#29412 and #29420–#29422; and
`sky_gh.wld` reuses #29400–#29404. The candidate #29400–#29402 mobile records
also occur in `dark.mob` and `sky_gh.mob`; candidate objects #29400 and #29401
also occur in `dark.obj`. These files have distinct text and themes. The
overlaps establish competing source identities, not a confirmed alias,
predecessor, or replacement. Harrow's existing audit records 26 numeric WLD
target matches from `infernal_gh.wld`, all into Harrow's active room VNUMs; see
the [Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches).
That audit also records Harrow's direct item-slot values for active object
#29401 and its special function binding to active room #29403. Keep those code
consumers with Harrow's current definitions rather than assigning them to the
different Infernal bag/portal records.

## Candidate graph and active numeric matches

The candidate WLD has 29 exit records: 26 local exits within the candidate
room set and three D4 up exits to unlisted `Duris3.wld` rooms:

| Candidate exit | Source-only target room | Target room title |
| --- | --- | --- |
| #29400 up | #154534 | Rolling Hills |
| #29410 up | #124233 | Grassy Plains |
| #29420 up | #187352 | The Ocean |

The three target room records exist in `Duris3.wld`, which is not in the
current full area list. Those target records lead to neighboring Duris3 VNUMs
and do not point back to the candidate rooms. The exits therefore remain
source-only, one-way map references; they do not resolve through today's
active room table.

Within the candidate WLD alone, there are three weak components of sizes 10,
3, and 3. Directed traversal from #29400 reaches ten rooms. The four type-25
portal objects and their ZON placements author cross-component links, but those
item teleports are not WLD exits and are not included in this graph count.
There are no negative targets, range-mismatch rooms, fall chances, or current
fields.

All 26 local exit targets happen to resolve numerically in the current active
room table, but each resolves to the different Harrow room with that VNUM. The
only active cross-area numeric approaches into the candidate range are Surface
#547275 east and #547676 north to #29400. Both are reciprocal Surface–Harrow
routes; their current destination is active Harrow #29400, “Approaching the
Arctic Colony of Harrow,” not the unlisted Infernal tunnel. They provide no
Infernal ingress. No active route or reverse pair was found for the three
candidate-to-Duris3 exits. See the [Harrow boundary review](HARROW_AUDIT_REVIEW.md#active-wld-boundary),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-491-infernal-guildhall-inactive-source-graph),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-491-infernal-guildhall-source-identities),
[candidate row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

## Resets, portal links, and external objects

The candidate ZON has 39 commands: 11 O object loads, 13 M mobile loads, and
15 G inventory loads. It also has no D, E, F, or P commands. All reset room
arguments are among the 16 candidate rooms, and all three candidate mobile
prototypes are referenced by M resets.

Four O commands place the candidate's four type-25 portals:

| Object and placement | Authored destination | Description text |
| --- | --- | --- |
| #29400 in #29412 | #29409 | light |
| #29401 in #29409 | #29412 | Lush Jungles |
| #29402 in #29406 | #29422 | Mountain Castle |
| #29403 in #29422 | #29406 | light |

These stored destinations pair the three WLD components through the rooms
named as the KK and Du'Maathe outposts. Two descriptions do not name their
stored destination in the same terms, so preserve the VNUMs and ask the map
owner to confirm the intended labels and direction of travel. In the current
active object table, #29400–#29403 resolve to Harrow's sack, large bag, faerie
dust, and dagger, rather than these portal records.

The other seven O commands import three active Player Castles objects:
#11052, the mouth; #11013, the bonding altar; and #11102, the board. The
[Player Castles audit](PLAYER_CASTLES_AUDIT_REVIEW.md) confirms these are
separate object prototypes and documents the #11013 guildhome handler. Keep
those imports attached to their current Player Castles definitions; the source
comments “object not in this .OBJ file” describe split prototype ownership,
not an absent dependency.

All 15 G resets use the same products listed in the SHP record. Fourteen have
current active object definitions: #3050 belongs to Verz1 and the other 13
(#377, #364, #347, #53, #412, #108, #109, #111, #398, and #491–#494) resolve
to Heavens. No checked-in OBJ file defines #6089. Candidate ZON line 37 added
#491–#494 in a September 6, 2026 edit, so the source's stock set was still
being changed after its old WLD/MOB/OBJ sources were established. The
[bounded dependency review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers)
also records the unresolved #6089 product in another shop; it does not supply
an object definition.

## Shop source and conditional loader path

`infernal_gh.shp` contains one shop record headed #29401. Its keeper VNUM is
#29401 and its room VNUM is #29407. Those numbers currently resolve to active
Harrow mobile #29401 (a tomato) and active Harrow room #29407 (Fountain
Square), rather than the candidate ogre shopkeeper and “Grandeur Shop” room.
The record header #29401 is also present in unregistered
[`dark.shp`](../../../areas/shp/dark.shp#L1), with different shop text and
stock. Neither record is named in `areas/AREA`.

This checkout has no `areas/world.shp`. In non-mini mode,
[`open_shop_stream`](../../../src/economy/shop.c#L3267) tries that file and,
when it cannot open it, concatenates files matched by `areas/shp/*.shp`
([fallback glob](../../../src/economy/shop.c#L3276)). That conditional path
can include both unlisted `dark.shp` and `infernal_gh.shp`, so shop-file input
is not gated by full AREA membership. The code maps stock products through
`real_object` and keeper/room VNUMs through the current world tables. This is
a source-code path finding only; no shop loader or game session was run.

## Narrative, history, and disposition

The map is story-bearing despite having no QST or sidecar: it describes an
underground Infernal palace, worship halls for Lucifer and Beelzebub, enslaved
Forsaken guards, a shop, and the KK and Du'Maathe outposts. No reviewed quest
consumer or active story continuation was found for this source.

Tracked file history reaches the 2009 `areas/` tree relocation; the WLD and
SHP also have a 2013 edit. On September 6, 2026, commit
`ed94862f0d40e0d6257e5acaf793825d92d16ab2` added four Heavens stock items to
`infernal_gh.zon`. A search of tracked `areas/AREA` history found no literal
`infernal_gh` membership entry, and the bounded same-stem history review found
no rename or whole-area replacement. These findings do not resolve who owns
the old map, the later reset edit, its exports, or any saved state.

**Disposition: RETAIN IN PLACE.** Keep all five files at their current paths
and out of `areas/AREA` pending resolution of Harrow's active ownership of all
candidate room/mobile/object VNUMs; the shared `dark`/`darkop`/`sky_gh` source
identities; the one-way Duris3 exits; the Player Castles imports; the missing
#6089 object; and the conditional duplicate SHP records and Harrow keeper/room
mapping. Do not treat same-number Harrow routes as access to Infernal, or its
unlisted state as permission to move or archive it. See the
[30 builder questions](INFERNAL_GH_BUILDER_QUESTIONS.md),
[source identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-491-infernal-guildhall-source-identities),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-491-infernal-guildhall-retained-pending-map-shop-and-prototype-ownership),
[candidate dependency row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).

## Checkpoint 903: typed cross-source role closure

The CP491 room graph, source inventory, shop fallback finding, and **RETAIN IN
PLACE** decision remain unchanged. The role-aware recount confirms that raw
VNUM matches around #29400–#29422 belong to active Harrow or separate
unregistered Dark/Darkop/Sky Guildhall records unless a typed field resolves
to an active Harrow owner.

### WLD and reset roles

Outside-source WLD directions with targets in the Infernal room-number band
classify as 44 Harrow-local, 36 `dark`-local, 11 `darkop`-local, and eight
`sky_gh`-local edges. Two active Surface directions and one unregistered Surf
direction also target numeric #29400; those resolve to active Harrow's room,
not Infernal. The active Surface routes remain the reciprocal
Surface–Harrow boundaries ([register rows 1436 and 1438](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1436));
the unregistered Surf source-only record is at
[`surf.wld` line 305645](../../../areas/wld/surf.wld#L305645). The candidate itself keeps
three one-way source exits to unlisted Duris3. No active ingress into an
Infernal-only identity or return edge to the candidate was found.

Across other ZON files, 86 typed reset fields match candidate-number roles,
and each is local to its own same-stem area definitions: active Harrow has 39
room, five mobile, and four object fields; unlisted `dark` has 14 room, five
mobile, and two object fields; `darkop` has 12 room fields; and `sky_gh` has
four room and one mobile field. These reset values do not import Infernal
rooms, mobiles, or objects. Candidate ZON's seven Player Castles imports and
the active Heavens stock additions #491–#494 remain separate outbound
dependencies.

### Quest, story, shop, and code roles

Active `harrow.qst` has two `G I` item commands using #29400 and #29402
([lines 80–81](../../../areas/qst/harrow.qst#L80)). The active Harrow OBJ
owners are a sack and faerie dust ([#29400](../../../areas/obj/harrow.obj#L1),
[#29402](../../../areas/obj/harrow.obj#L29)), not the candidate's portal #29400
and portal #29402. The active [`harrow.story.json`](../../../areas/story/harrow.story.json#L398)
likewise contains two typed item VNUM fields ([#29400](../../../areas/story/harrow.story.json#L431),
[#29402](../../../areas/story/harrow.story.json#L442)) and four `I:` completion-key
occurrences for those Harrow items; it does not consume the unlisted Infernal
portal identities. Preserve the active Harrow quest and sidecar references
with Harrow.

The outside N-format shop scan found five role fields in the overlapping band:
active Harrow SHP uses keeper #29400, room #29401, room #29408, and product
#29402 ([shop records](../../../areas/shp/harrow.shp#L1) and
[#29402 stock](../../../areas/shp/harrow.shp#L80)); unlisted `dark.shp` uses
keeper #29401 ([record](../../../areas/shp/dark.shp#L1)), local to Dark's
source mobile. The Infernal record is also headed #29401. Since the fallback
path can read both unlisted shop files while keeper and room numbers resolve
through current world tables, preserve the duplicate records and Harrow owner
mapping; actual loader behavior remains unverified.

No external type-25 teleport destination or type-29 switch target points into
the candidate room range. The direct compiled-call scan found one same-number
room binding: `real_room0(29403)` assigns the inn handler to active Harrow
room #29403 ([`specs.assign.c`](../../../src/specs/specs.assign.c#L2454)). It
does not bind the separate Infernal room record. No additional direct
room/mobile/object callsite was found.

These scans add no candidate ingress or activation evidence. Keep the shared
low-VNUM source identities, active Harrow QST/story/shop/code consumers,
conditional unlisted-shop fallback, Player Castles imports, Heavens stock,
Duris3 exits, and #6089 unresolved stock intact. Resolve the story and source
owners, reset/export copies, shop-input policy, dynamic paths, and persisted
state before any activation, merge, archive, or path proposal. See the
[updated 30 questions](INFERNAL_GH_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-903-infernal-numeric-routes-remain-harrow-owned),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-903-infernal-keeps-harrow-and-sibling-identities-separate),
[retention note](ATTIC_MIGRATION_PLAN.md#checkpoint-903-infernal-retains-typed-shop-and-map-boundaries),
[candidate dependency row 54](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L54), and
[coverage row 404](ZONE_COVERAGE_REGISTER.csv#L405).
