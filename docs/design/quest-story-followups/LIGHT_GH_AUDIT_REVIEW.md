# The Light's Guildhall inactive-source audit

Canonical key: component:light_gh. Physical coverage row 410 / CSV line 411;
candidate dependency row 60 / CSV line 60. Reviewed October 11, 2026,
Checkpoint 497, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, numeric-identity, reset, shop, story, and bounded-history review.
It does not establish live loading, gameplay, actual shop boot behavior, saved
world ownership, or permission to activate, rename, move, or archive any file.

## Source inventory, naming, and active Underworld3 ownership

The unlisted component has [WLD](../../../areas/wld/light_gh.wld#L1),
[MOB](../../../areas/mob/light_gh.mob#L1),
[OBJ](../../../areas/obj/light_gh.obj#L1),
[ZON](../../../areas/zon/light_gh.zon#L1), and
[SHP](../../../areas/shp/light_gh.shp#L1) sources. It has no same-stem QST,
story sidecar, or raw AREA annotation. Neither [areas/AREA](../../../areas/AREA)
nor [areas/qst/AREA](../../../areas/qst/AREA) lists light_gh. The ZON header
is #300, titled “The Light's Guildhall,” with top #30022 and reset mode 0.
The source defines 16 rooms (#30000–#30012 and #30020–#30022), four mobiles
#30000–#30003, four objects #30000–#30003, and one shop record headed #30001.

The normalized title “The Light's Guildhall” is unique among the tracked ZON
titles, but the header #300 is shared with active Underworld3. The full area
list includes Underworld3 across *300–303; its 283-room active WLD defines
every one of Light Guildhall's 16 room VNUMs. The records are different map
identities:

| VNUM | Unlisted Light Guildhall | Active Underworld3 |
| --- | --- | --- |
| #30000 | Entrance to the Light | Vertical Shaft |
| #30004 | Room of Halfling Lore | Bottom of a Vertical Shaft |
| #30010 | Ice-filled doorway | Mud-filled cavern |
| #30022 | Abandoned Dragon's Lair | Mushroom-filled tunnel |

The same-type mobile and object numbers also have active Underworld3
definitions, with different content:

| VNUM | Unlisted Light Guildhall | Active Underworld3 |
| --- | --- | --- |
| Mobile #30000 | Heavenly beam of light | Scuren |
| Mobile #30001 | Yariena, halfling shopkeeper | Lizardman |
| Mobiles #30002–#30003 | Heavenly beam variants | Green and blue slime pools |
| Object #30000 | Glowing portal | Ring made of bark |
| Object #30001 | Glowing portal | Tharnadian Guard longsword |
| Object #30002 | Glowing portal | Durithalite headband |
| Object #30003 | Glowing portal | Jodani wood armor |

Underworld3's active ZON contains 33 M commands for its mobiles #30000–#30003
and four E commands for its objects #30000–#30003. Those references belong to
the active Underworld3 prototypes, not the unlisted Light Guildhall
definitions. No other same-type mobile or object definition in the scanned
source files was found for these four IDs. The overlap establishes competing
numeric identities, not a duplicate active room or a confirmed replacement.
See the [Underworld3 audit](UNDERWORLD_AUDIT_REVIEW.md#underworld3-room-overlap-and-the-compiled-inn-binding)
and [source-backed review](../zone-stories/UNDERWORLD3.md).

## Candidate graph, active route, and portal objects

The Light Guildhall WLD has 28 authored direction records: 26 local edges and
two D4 exits to unlisted Duris3 Ocean rooms:

| Candidate exit | Source-only target |
| --- | --- |
| [#30000 up](../../../areas/wld/light_gh.wld#L14) | [#157434, The Ocean](../../../areas/wld/Duris3.wld#L1206115) |
| [#30010 up](../../../areas/wld/light_gh.wld#L197) | [#179053, The Ocean](../../../areas/wld/Duris3.wld#L1660114) |

The two target rooms lead to neighboring Duris3 rooms, not back to
#30000–#30022. Duris3 is not in the full area list, so both endpoints are
unresolved by the current active room table. Within Light Guildhall, the
26 local edges form three weak components of sizes 10, 3, and 3. Directed
traversal from #30000, #30010, and #30020 reaches every room in its component.
There are no negative targets, source rooms outside the #300 numeric zone
range, range-mismatch source rooms, fall chances, or current fields.

All 26 local targets numerically resolve to active Underworld3 rooms because
that contributor owns the same room VNUMs. This is active-table resolution to
the Underworld3 map identity; it does not load Light Guildhall's source exits.
No active exit enters the Light Guildhall identity. The one other WLD source
with a numeric exit into this room band is active
[Moonshae #26272 down to #30000](../../../areas/wld/moonshae.wld#L1485); the
cross-area register classifies that route as Moonshae-to-Underworld3, and
Underworld3 #30000 has the reciprocal up exit. Do not count it as Light
Guildhall ingress. No other WLD source was found targeting #30000–#30022.

The four type-25 object prototypes encode destinations separate from WLD
edges. The ZON resets three of them:

| Object placement | Stored destination | Source relationship |
| --- | --- | --- |
| #30000 in room #30007 | #30012 | Reciprocal source-declared pair with #30001 |
| #30001 in room #30012 | #30007 | Reciprocal source-declared pair with #30000 |
| #30002 in room #30022 | #30005 | One-way placed portal in the source resets |
| #30003 has no Light Guildhall O reset | #30022 | Prototype could be a return, but its placement is not established |

The first object pair bridges two WLD components. The second pair is not
complete in the ZON: object #30002 is placed, while object #30003's apparent
return value is not reset by light_gh.zon. Treat those OBJ values as authored
intent only; no portal callback, live availability, destination access, or
return behavior was qualified. Do not add the portals to WLD edge totals.
See the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-497-light-guildhall-inactive-map-and-active-underworld3-boundary).

## Reset imports, stock, and shop resolution

The candidate ZON header uses reset mode 0 and contains 38 commands: 10 O,
13 M, and 15 G. Seven O commands import active Player Castles objects:
#11051 in rooms #30000, #30010, and #30020; #11101 in #30003, #30011, and
#30021; and #11013 in #30009. The remaining three O commands place local
portal objects #30000, #30001, and #30002. The 13 M commands place candidate
beams of light and Yariena in candidate rooms. All of these Light Guildhall
reset commands are source-only while the component is unlisted. Preserve the
seven exact external imports and their Player Castles ownership; the #11013
home handler is not a map exit. See the
[Player Castles dependency review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers).

The 15 G commands on Yariena match the 15 products listed in the SHP record:

| Product | Current checked-in OBJ owner |
| --- | --- |
| #3050 | Verz1 |
| #53, #108, #109, #111, #347, #364, #377, #398, #412 | Heavens |
| #491–#494 | Heavens gathering-bag prototypes |
| #6089 | No checked-in OBJ prototype |

Fourteen products therefore have active OBJ definitions; #6089 does not.
Commit ed94862f0, dated September 6, 2026, added Heavens gathering bags
#491–#494 to both light_gh.zon's G list and light_gh.shp's stock list. This
shows a recent reset and stock edit to an unlisted source; it does not prove
the shop is part of the active production input.

The candidate SHP record is keyed #30001, names keeper #30001, and configures
shop room #30004. Those are candidate Yariena and a candidate lore room, but
active Underworld3 owns the same mobile and room numbers as a lizardman and
the bottom of a vertical shaft. The standard
[shop compiler](../../../areas/src/shp/make_shp.c#L6) reads its AREA list and
excludes light_gh. This checkout has no areas/world.shp. In non-mini mode,
[open_shop_stream](../../../src/economy/shop.c#L3267) falls back to globbing
areas/shp/*.shp if the generated aggregate cannot be opened, which can include
light_gh.shp. Shop code resolves its configured keeper and room numbers
against current world tables. This is a conditional loader-path finding, not
confirmation of a usable shop; no runtime load or shop interaction was tested.

## Narrative, typed-reference scan, history, and disposition

The source depicts a guildhall of Light with race-lore rooms, a sleeping room,
an ice-filled passage and cavern, and a crimson cavern containing an
abandoned dragon's lair. Its mobiles include heavenly beams and Yariena, and
its shop stock is a local typed dependency set. The source is story-bearing
even without a QST or separate story sidecar.

A bounded scan found Underworld3's 33 active mobile reset references and four
active equipment references to the same-number Underworld3 prototypes.
Other SHP files and external type-25 objects have no exact typed reference to
the candidate IDs or destination room band. Exact 30000 matches in other QST
and story-sidecar text occur as C or E currency/experience amounts, not as
item VNUM references. No direct compiled C/C++ VNUM binding to #30000–#30022
was found.

Followed WLD history reaches the 2009 area-tree move and 2013 same-path edit;
MOB and OBJ histories reach the 2009 move. ZON and SHP histories also reach
those paths and include commit ed94862f0's September 2026 addition of
#491–#494. The bounded histories contain no direct rename, retirement, or
whole-area replacement decision. The recent stock edit makes editor/export
ownership especially material.

**Disposition: RETAIN IN PLACE.** Keep the five candidate files at their
current paths and out of both area lists while the owner resolves the full
Underworld3 room/mobile/object collision, the Moonshae/Underworld3 boundary,
the two Duris3 exits, portal placements, Player Castles imports, #6089 stock,
the conditional shop fallback, and editor, deployment, story, and
persisted-ID state. Preserve Underworld3's current active prototypes,
33 mobile resets, four equipment resets, and its reciprocal Moonshae route.
Do not treat number overlap, active-table resolution, or the 2026 stock edit
as permission to activate, merge, renumber, move, or archive Light Guildhall.
See the [25 builder questions](LIGHT_GH_BUILDER_QUESTIONS.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-497-light-guildhall-room-and-prototype-collisions),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-497-light-guildhall-retained-pending-underworld3-and-shop-ownership),
[candidate dependency row 60](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L60), and
[coverage row 410](ZONE_COVERAGE_REGISTER.csv#L411).

## Checkpoint 909: typed role refresh

The exact-number recheck leaves the CP497 route, prototype, shop, and history
findings unchanged. The only active external WLD boundary remains Moonshae
#26272 to #30000, which belongs to active Underworld3 and its reciprocal shaft
room; no active exit enters the separate Light Guildhall identity. The
candidate's 26 local exit records still numerically match Underworld3 rooms,
and its two Duris3 exits remain source-only references.

Exact ZON hits occur in `light_gh.zon` and `underworld3.zon`. Underworld3's 33
M commands for mobile IDs #30000–#30003 and four E commands for object IDs
#30000–#30003 remain reset consumers of its active Scuren, lizardman, slime,
ring, sword, headband, and armor prototypes. Two additional Underworld3 M
commands use #30004 and #30005 as room operands; both are active Underworld3
rooms, not Light Guildhall room sources. The other same-number matches in
Underworld3 M prototype fields are mobile roles, not candidate room references.
The candidate's own 38 reset commands, seven Player Castles imports, three
portal placements, and recent #491–#494 stock/reset additions remain
source-only.

The exact SHP scan found only Light Guildhall's own record in #30000–#30023: its
keeper is candidate mobile #30001 and its configured room is candidate room
#30004. Both collide numerically with active Underworld3's lizardman and shaft
room. No outside SHP header, keeper, room, or product uses the candidate band.
The possible non-mini shop-file fallback remains conditional because
`areas/world.shp` is absent; no shop runtime was checked.

All external exact-number QST and story-sidecar hits are `R C 30000` currency
amounts or `R E 30000` experience amounts; no candidate item-VNUM consumer was
found. The four type-25 destination matches are the candidate's own portal
objects #30000–#30003; no external type-25 destination or type-29 room target
into the candidate band was found. The compiled-source scan found no direct
VNUM lookup or binding for #30000–#30022. Its raw 30000 matches are numeric
thresholds, hit-point/pricing/property/timeout values, and map-band arithmetic
in `map.c`, not a lookup for a Light Guildhall identity.

Keep Light Guildhall and active Underworld3 **RETAIN IN PLACE** as distinct
source identities. The 16-room/28-direction candidate graph, the Moonshae to
Underworld3 boundary, active Underworld3 resets, conditional shop-input
uncertainty, recent bag edit, and CP497 history conclusion remain unchanged.
The 25 builder questions remain open.
