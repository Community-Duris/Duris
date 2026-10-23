# Tribal Forest repository-wide audit supplement

This supplement reconciles the active route graph, outside references, title,
tracked source history, and retention status for the
[Tribal Forest source dossier](../zone-stories/TRIBAL_FOREST.md). The source
snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No world or gameplay
source changed.

## Active graph and world boundaries

The active WLD contains 175 rooms and 462/462 resolving exits. Of those exits,
453 target a Tribal room and nine leave the area. Three additional inbound
edges from Surface target room #42281. The local undirected WLD graph has three
weak components: 173 rooms, singleton #42297, and singleton #42374. These raw
graph counts ignore doors, switches, hidden exits, objects with travel
procedures, actor eligibility, and hazards.

From #42281, 129 rooms are reachable by directed WLD exits alone, while 172
rooms have a directed WLD path back to #42281. The counts include the start
room. They do not include the five local type-25 travel objects documented in
the dossier. Room #42297 has six authored exits to Astral Main (#19795,
#19732, #19755, #19722, #19798, and #19790) and no reverse WLD edge. The
reset-placed black rift object #42253 has its separate configured travel
destination at #42295. Room #42374, “Inside a Force Cage,” has no WLD exits and
no static WLD ingress in the scanned active or source-only files.

Room #42281 provides three reciprocal Surface boundaries: Tribal north to
#630869, south to #631669, and west to #631268. Surface returns from those
rooms by south, north, and east respectively. The
[cross-area pair register](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L614) records the
Surface side; [Tribal's rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L669) record
all three reverse pairs. The six Astral Main exits are listed at
[pair row 668](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L668). No other active or
source-only WLD exit targets a Tribal room.

## Typed dependency closure

| Reference type | Source-backed result |
| --- | --- |
| Active and source-only ZON resets | No external M/F/G/E/O/P reset-role reference places a Tribal mobile or object, and no external reset places a mobile or object into a Tribal room. [Tribal's reset](../../../areas/zon/tribal.zon) places active Heavens object #72 in room #42219. |
| Imported object and procedure | Heavens [object #72](../../../areas/obj/heavens.obj#L1000) is an active external prototype. [Tribal's reset](../../../areas/zon/tribal.zon) places it in the glade; [`specs.assign.c`](../../../src/specs/specs.assign.c#L1620) binds it to [`spell_pool`](../../../src/specs/specs.heavens.c#L963). This is separate from Tribal's own [QST exchanges](../../../areas/qst/tribal.qst). Same-number #72 records in the unregistered [`hostel`](../../../areas/obj/hostel.obj) and [`mavulsk`](../../../areas/obj/mavulsk.obj) object files are source-only; neither stem is an active `areas/AREA` contributor. |
| QST | Tribal has seven native giver prototypes and ten Q/QA contracts in [`tribal.qst`](../../../areas/qst/tribal.qst). No other QST file has a numeric reference to a Tribal room, mobile, or object VNUM. The nine meaningful native exchange outputs have no foreign native QST consumer in the current source scan. |
| Shops | The tilde-ended `#42235~` record in [`tribal.shp`](../../../areas/shp/tribal.shp) is the one local shop. Keeper mobile #42235 is assigned there; it sells objects #42231, #42246, #42248, and #42247. No other SHP file references a Tribal room, mobile, or object VNUM. |
| Story sidecars | [`tribal.story.json`](../../../areas/story/tribal.story.json) is the local sidecar. No other story sidecar has a typed integer reference to a Tribal room, mobile, or object VNUM. |
| Type-25 travel objects | The full active object scan found no outside type-25 object whose destination value targets a Tribal room. Tribal's [five local travel objects](../../../areas/obj/tribal.obj) remain mapped in the source dossier; item type 25 is [`ITEM_TELEPORT`](../../../src/core/defines.h#L140). |
| Compiled bindings and consumers | The exact direct-code scan found three numeric references: object #42235 is bound to `amethyst_orb` in [`specs.assign.c`](../../../src/specs/specs.assign.c#L1738); object #42226 appears in slot 4 of both standard and enhanceable Psionicist CHAOS starter profiles in [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L89) ([second profile](../../../src/account/chaos_eq_data.h#L341)). [`nanny.c`](../../../src/account/nanny.c#L682) creates these kits for CHAOS characters. Object #42226 is the “amulet hell nine hells” in [`tribal.obj`](../../../areas/obj/tribal.obj#L406), not a Tribal QST output. |
| Exact prototype collisions | Current active WLD/MOB/OBJ header scans found no outside room, mobile, or object with an exact VNUM in the corresponding Tribal local set; see the [local prototype sources](../../../areas/wld/tribal.wld), [mobiles](../../../areas/mob/tribal.mob), and [objects](../../../areas/obj/tribal.obj). |

The #42235 object special and #42235 apprentice mobile are separate
namespaces. Preserve the exact object binding, shop keeper identity, and
external Chaos starter item reference if any VNUM or prototype is changed.

## Title, history, and disposition

After color-token removal and case/whitespace normalization, “Tribal Forest”
has one exact owner among 442 current `.zon` titles: `tribal`. No external
active WLD/MOB/OBJ prototype collides with the area's current local VNUMs.
Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on their current
stems through the 2009 area-tree move and subsequent same-stem edits. The
story-sidecar history begins with the 2026 `tribal.story.json` addition. The
bounded history scan found no alternate-stem predecessor, rename, or whole-zone
replacement. Tribal Oasis is a distinct area titled “Tribal Oasis”
with its own dossier and coverage row; it is not a Tribal Forest alias or
replacement.

Keep active Tribal Forest **DO NOT MOVE**. Preserve its reciprocal Surface
approaches, Heavens fountain import, local item procedure, and Chaos starter
reference. No separate attic candidate or replacement source was found.

## Builder review

Checkpoint 399 records 20 explicit questions and conservative recommendations
in the [source dossier](../zone-stories/TRIBAL_FOREST.md#explicit-builder-questions-and-recommended-resolutions).
They distinguish current QST acceptance from source history, route prose from
implemented state, and static graph evidence from runtime access. No gameplay
result or repair is claimed.

## Checkpoint 854: Current boundaries and typed dependencies carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 399 source review and builder questions forward without claiming a second independent parse. Tribal Forest remains 175 rooms with 462/462 resolving exits (453 local, nine outbound), three local weak components, three reciprocal Surface pairs, and six one-way WLD exits to Astral Main. The isolated Force Cage #42374 still has no authored exits or static WLD ingress. The all-95 inactive review found no separate approach or room overlap; cumulative source-only totals remain 11,993 overall / 415 Surf.

The typed closure remains unchanged: Tribal's resets import active Heavens OBJ #72, which retains its `spell_pool` binding; Tribal's own seven giver prototypes and ten Q/QA contracts remain separate from outside consumers. The one local shop at #42235 retains its four products, the local story sidecar remains its own record, and no external typed sidecar/QST/shop/reset or type-25 target into Tribal was established. Preserve the `amethyst_orb` binding to OBJ #42235 and CHAOS Psionicist starter dependency on OBJ #42226; the same-number apprentice mobile is a separate prototype namespace. Five local travel objects, the Astral exits, and reciprocal Surface edges remain distinct source routes. The twenty builder questions from Checkpoint 399 remain source review, not gameplay qualification.

Keep active `tribal` **DO NOT MOVE**. Preserve the three reciprocal Surface boundaries, Heavens #72 import, local object procedure, and CHAOS starter reference with their current owners. No separate replacement source, attic candidate, route activation, or migration clearance follows. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L312), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-854-tribal-forest-boundaries-and-routes-remain-unchanged), and [Tribal Oasis distinction](../zone-stories/TRIBAL_OASIS.md).
