# EOO (Valhalla) source audit

Canonical key: `component:eoo`. Physical coverage row 387 / CSV line 388;
candidate dependency row 37. Reviewed October 11, 2026, Checkpoint 474. This is
a static source, graph, history, typed-reference, and loader-path review. It
does not establish runtime shop behavior, player access, or permission to
activate or move the area.

## Source scope, story, and map graph

The candidate consists of [WLD](../../../areas/wld/eoo.wld),
[MOB](../../../areas/mob/eoo.mob), [OBJ](../../../areas/obj/eoo.obj),
[ZON](../../../areas/zon/eoo.zon), and [SHP](../../../areas/shp/eoo.shp).
There is no `eoo` row in full
[`areas/AREA`](../../../areas/AREA) or
[`areas/qst/AREA`](../../../areas/qst/AREA), and there is no
`eoo.qst` or same-stem story sidecar. Its ZON title is `Valhalla`,
the only exact matching title among the tracked ZON headers; the AREA
annotation is blank. The WLD has ten rooms, #29200–#29209.

The text identifies #29200 as Gladsheim, land of Valhalla. It names Odin,
Yggdrasil, Bifrost, Ragnarok, Hugin, Munin, Jormungandr, Fenris, and the
Einherjar. Gunnar’s MOB prose says he failed to free Brunhilde and that Gudrun
did so in his shape; no QST or quest reset implements that story in this
component. The existing Jotun review already treats the EOO Valhalla source as
a separate identity and Jotun’s Valhalla wording as thematic lore, not a merge
instruction ([Jotun review](JOTUN_AUDIT_REVIEW.md#checkpoint-242-current-catalog-tool-and-semantic-revalidation)).

The WLD has 19 directions: 18 stay within the ten EOO VNUMs, and one goes up
from #29200 to #179748 in inactive Duris3 (“The Ocean”)
([EOO exit](../../../areas/wld/eoo.wld#L17),
[Duris3 target](../../../areas/wld/Duris3.wld#L1674709)). No target room
record points back to #29200. All ten EOO rooms are in one weak component and
are forward-reachable from #29200 when source directions are treated as
available. The external Duris3 target is absent from the full active room
table. This source graph does not establish a current connection or safe
return.

## Reused room, mobile, and object identities

Every EOO room VNUM also has at least two other tracked WLD definitions:
`dark_gh` and `genocide_gh@ have all ten #29200–#29209 numbers, and
active `newbie` has #29201–#29209. That is 29 same-number room pairs
across these owners; none of the paired complete room blocks is identical.
All nine Newbie room overlaps have different normalized titles from EOO.
The 22 active Newbie directions that numerically target #29201–#29209 belong
to Newbie’s own active map identities. No active WLD direction targets
EOO-only room #29200, so the candidate-inbound count is zero. Preserve the
previously reviewed EOO source-only approach from #29200 north to active
Ailvio #29201; it is one of the three inactive approaches already counted in
the Newbie review, not a new total
([Newbie connectivity review](NEWBIE_AUDIT_REVIEW.md#active-graph-and-inactive-source-only-approaches)).

The five EOO mobiles have ten same-number prototype comparisons across
`dark_gh`, `genocide_gh@, and `newbie`; none is an identical
complete record. Four EOO mobile VNUMs, #29201–#29204, have active Newbie
owners with different prototypes. For example, EOO Gunnar #29204 is distinct
from active Newbie’s Lady Laurel #29204
([Gunnar](../../../areas/mob/eoo.mob#L142),
[Lady Laurel](../../../areas/mob/newbie.mob#L51)). EOO object #29201 is a
cinnamon-apple pastry, while active Newbie object #29201 is its map; EOO
object #29200 also shares a number with a different `dark_gh` portal.
None of these exact same-number comparisons is identity or replacement
evidence.

## Resets, shop stock, and conditional loader path

The ZON has five M resets, seven O resets, and fourteen G stock resets. It
places all five EOO mobiles, including Gunnar #29204 in room #29204. Its
seven O records refer to #11051, #29211, #11101, #11005, #11013, #29212, and
#29213; none is defined in `eoo.obj`. The four #110xx objects are the
existing Player Castles imports recorded in Checkpoint 325 and must remain
attached to this candidate’s source-only reset history. The other three IDs
(#29211–#29213) are EOO-portal prototypes in unregistered
`eoo_op.obj`, while active Newbie owns the same numbers as pickled
herring, flax basket, and fish-scale hat. The ZON comments explicitly mark
these as objects outside `eoo.obj`. Preserve the intended typed import
decision; current active numeric ownership does not load the EOO or EOO_OP
prototype.

The SHP contains one valid record keyed #29204, with keeper #29204, shop room
#29204, and fourteen stock VNUMs. The two EOO stock prototypes are #29200 and
#29201; twelve other VNUMs are listed. Eleven of those twelve have active
prototype owners, but #29201 resolves numerically to Newbie’s map object,
#29200 has no active object owner, and #6089 has no matching OBJ definition
in any tracked current source. This separates the authored stock list from
what the current active object table can identify.

There is no `areas/world.shp` in this checkout. In non-mini mode,
`open_shop_stream` first opens that file and otherwise globs all
`areas/shp/*.shp@, which includes unregistered `eoo.shp@. The parser
maps the shop keeper VNUM through `real_mobile` and positive stock VNUMs
through `real_object`; shop-room checks later use `real_room`
([fallback and shop parser](../../../src/economy/shop.c#L3267),
[stock lookup](../../../src/economy/shop.c#L3380),
[keeper lookup](../../../src/economy/shop.c#L3493),
[room lookup](../../../src/economy/shop.c#L3092)). Under the current active
tables, those numbers identify Newbie’s Lady Laurel and walkway at #29204,
not EOO’s Gunnar and Valhalla shop. This is a conditional source-code risk;
the audit did not boot the server or establish which shop records are present
in a running process.

The numeric code references found for #29201/#29202 are Newbie-specific:
character creation checks room #29201, Newbie startup and travel use its
active room, Ailvio code reads Newbie object #29201, and the room #29202
special binding is assigned to active Ailvio
([creation check](../../../src/account/nanny.c#L848),
[object use](../../../src/specs/specs.ailvio.c#L44),
[room binding](../../../src/specs/specs.assign.c#L2453),
[Newbie2 return](../../../src/specs/specs.newbie2.c#L134)). They do not
establish a consumer of EOO’s same-number records.

## History and disposition

Followed histories reach the 2009 area-tree move and 2012–2013 same-stem
updates to the current files, with no alternate-stem predecessor or explicit
whole-area replacement. The title `Valhalla` and the source texts provide
identity clues, not ownership or replacement proof. Keep the EOO and
`dark_gh`, `genocide_gh@, `eoo_op`, and active Newbie records
distinct until their builders reconcile the shared room/prototype numbers,
external resets, narrative, and shop-loading path.

Keep candidate row 37 **RETAIN IN PLACE**. Preserve its ten room records, five
mobiles, two objects, seven imported O resets, fourteen stock references,
the one SHP record, its Ailvio approach classification, and its one-way
Duris3 edge. Typed source, editor, deployment, player-state, and persisted
identity closure remains open. See the
[24 builder questions](EOO_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-474-eoo-valhalla-map-identity-and-history),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-474-eoo-valhalla-connectivity-and-typed-boundaries),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-474-eoo-retained-pending-import-shop-and-map-identity-review),
[candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37), and
[coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387). No area/source files,
tests, builds, runtime, server, database, or container were changed or used.

## Checkpoint 886: role-aware reference closure and shop-stream qualification

This pass classified matching numeric fields by their loader roles. `renum_zone_table()` converts each ZON command's mobile, object, and room operands separately; a matching VNUM alone does not identify which colliding source definition a reset uses ([role mapping](../../../src/world/db.c#L1612)).

The WLD result is unchanged: there is no active inbound exit to EOO-only #29200, and the 22 active WLD directions matching #29201-#29209 are Newbie-to-Newbie routes. No approach or curated connectivity count is added. This pass found 25 external ZON room-role operands into the same numeric #29200-#29209 band: ten in `dark_gh.zon`, eight in `genocide_gh.zon`, and seven in active `newbie.zon`. Their placements belong to those source maps. The 26 external ZON prototype-role matches in the band likewise occur only in `dark_gh`, `genocide_gh`, and `newbie`, which define colliding prototypes of their own; they do not establish consumers of EOO's distinct records. EOO has no QST or story sidecar. `newbie.qst` and `newbie.story.json` references to #29201 are Newbie quester/item roles, while the `newbie zone` table in `constant.c` also names #29201; none is an EOO consumer ([Newbie QST](../../../areas/qst/newbie.qst#L1), [Newbie story sidecar](../../../areas/story/newbie.story.json#L429), [Newbie zone table](../../../src/core/constant.c#L1991)).

The external OBJ type-25 room targets are source-specific: `dark_gh` #29200 -> #29206 and #29202 -> #29209; `eoo_op` #29210 -> #29206; and `eoo_op2` #29220 -> #29202 and #29221 -> #29206. Those room numbers are active Newbie identities under the current room table and also overlap inactive EOO-family definitions; the object records do not establish an EOO approach. No type-29 target into the EOO numeric room band was found. Keep these portal targets distinct from EOO's one-way #29200 -> Duris3 #179748 edge.

The all-shard SHP scan applied the current `N`-record grammar. Of 179 non-terminator `.shp` files, 712 records parsed as `N`; the remaining eight records are legacy `O` records in `luiren.shp`. Three other parser-valid shops mention the EOO number band: `dark_gh.shp` and `genocide_gh.shp` each use keeper #29201 or #29202 in room #29204, and active `newbie.shp` uses keeper and room #29201. Those are source-local shop identities. No other parser-valid shop stock list names EOO objects #29200 or #29201. EOO's own record is keyed #29204, names keeper #29204 and room #29204, and lists fourteen products ([EOO shop record](../../../areas/shp/eoo.shp#L1); [other same-number records](../../../areas/shp/dark_gh.shp#L1), [Genocide Guildhall](../../../areas/shp/genocide_gh.shp#L1), [Newbie](../../../areas/shp/newbie.shp#L1)).

The shop loader first tries `SHOP_FILE`; in non-mini mode its fallback concatenates `areas/shp/*.shp`, including `eoo.shp`, and then parses the aggregate. Product VNUMs resolve through `real_object`, the keeper through `real_mobile`, and the shop room is checked through `real_room` ([fallback](../../../src/economy/shop.c#L3267), [product lookup](../../../src/economy/shop.c#L3374), [keeper and room fields](../../../src/economy/shop.c#L3489), [room check](../../../src/economy/shop.c#L3092)). `areas/world.shp` is absent in this checkout, but the aggregate also contains eight `O` records from `luiren.shp`; the current parser accepts only `N` and calls `fatal_boot_error` for an old-format record ([format gate](../../../src/economy/shop.c#L3360), [legacy records](../../../areas/shp/luiren.shp#L1)). Therefore the source glob includes EOO but does not prove that the full fallback catalog can load successfully. No shop boot or runtime behavior was tested.

Keep candidate row 37 **RETAIN IN PLACE**. Preserve all role-separated reset and portal references, the source-local shop/map identities, the conditional fallback risk, and the existing unresolved owner, tooling, deployment, story, and persisted-state questions. This supplement changes no history finding and adds no WLD approach count. See the [builder questions](EOO_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-886-eoo-role-aware-connectivity-and-shop-source-closure), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-886-eoo-retains-role-separated-identities-and-shop-input), [candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37), and [coverage row 387](ZONE_COVERAGE_REGISTER.csv#L388). No area/gameplay source, tests, builds, server, database, migration, or runtime was changed or used.
