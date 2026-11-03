# The Genocide Guildhall source audit

Canonical key: component:genocide_gh. Physical coverage row 397 / CSV line
398; candidate dependency row 47 / CSV line 47. Reviewed October 11, 2026,
Checkpoint 484, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, history, and typed-reference review. It does not establish
runtime loading, travel, shop behavior, editor ownership, persisted world
state, or permission to activate or move these files.

## Source inventory and area-list status

The current component consists of [WLD](../../../areas/wld/genocide_gh.wld#L1),
[MOB](../../../areas/mob/genocide_gh.mob#L1), an empty
[OBJ file](../../../areas/obj/genocide_gh.obj), [ZON](../../../areas/zon/genocide_gh.zon#L1),
and one [SHP record](../../../areas/shp/genocide_gh.shp#L1). WLD has ten
rooms #29200–#29209; MOB has four prototypes #29200–#29203; OBJ has no
prototypes. ZON header #2920 is titled “The Genocide Guildhall,” ends at
#29209, and uses reset mode 0. SHP is keyed to keeper #29202, places the shop
in room #29204, and lists thirteen products. There is no same-stem QST,
story sidecar, or editor-map file among the current area files.

No full areas/AREA or areas/qst/AREA entry names genocide_gh, and the raw
annotation field is blank. By contrast, the active areas/AREA entry for
newbie covers the #292–#293 room bands
([area list](../../../areas/AREA#L141)). The exact title “The Genocide
Guildhall” occurs once among 442 tracked ZON titles. A similarly named,
separate genocide_hall source has ZON title “Genocide Guild Hall,” header
#295, and rooms #29500–#29511; it is not the same source range.

## Authored map and source-only boundaries

All ten genocide_gh WLD description fields are empty; its authored content is in
the room titles, mobile text, and shop dialogue. The map titles name an
abandoned guildhall entrance, Halls of Persecution, an executioners' resting
place, a Destructors' stash, a hall of glowing blood, a cellar stair, an
ancient library, an attic, and a passage to a dark catacomb. The four mobiles
are three golems described as guards and a tired merchant said to serve
Genocide. The current files do not explain the guild's identity, history, or
relationship to the separate genocide_hall source.

The ten rooms contain 19 direction records: 18 local targets and one D4 from
#29200 to #157677, “The Ocean,” in
[Duris3.wld](../../../areas/wld/Duris3.wld#L1211218). The local exits form
one weak component; directed traversal from #29200 reaches all ten rooms.
There are no negative targets, fall records, or current records. Duris3 is
not in the full area list and has no WLD return to #29200, so this source
exit is not a qualified current route.

The first nine genocide_gh room numbers #29201–#29209 are also active Newbie
rooms with different titles and blocks. All ten numbers are additionally
defined by unregistered dark_gh and eoo maps, with different room blocks.
Seventeen genocide_gh directions target #29201–#29209 and thus numerically
resolve to current Newbie room identities; the one local direction to
#29200 and the Duris3 direction are unresolved in the current full list.
Newbie's own directions to #29201–#29209 remain Newbie-to-Newbie edges, not
genocide_gh ingress. No active WLD direction targets room #29200. The genocide_gh
exit #29200 north to Newbie #29201 is the previously counted third
source-only approach in the [Newbie review](NEWBIE_AUDIT_REVIEW.md#active-graph-and-inactive-source-only-approaches);
preserve it without adding another approach or changing the curated
11,987 overall / 415 from Surf totals.

## Resets, imports, and shop stock

The ZON has 21 commands under reset mode 0: three O imports, five M
placements, and thirteen G placements. Its O commands import Player Castles
objects #11059 into #29200, #11109 into #29201, and #11013 into #29202.
The M commands place four golems in #29200 and a tired merchant #29202 in
#29204. The thirteen G resets and the single SHP record list the same
thirteen products:

| Product | Current OBJ definition owner(s) |
| --- | --- |
| #3050 | verz1 |
| #377 | heavens |
| #108 | heavens and mavulsk |
| #109 | heavens and mavulsk |
| #111 | heavens |
| #412 | heavens |
| #6089 | No current OBJ declaration |
| #53 | dalvik, heavens, hostel, and mavulsk |
| #398 | heavens |
| #491–#494 | heavens |

The genocide_gh OBJ file is empty, so none of these products is declared locally.
The intended identity is ambiguous for duplicate #53/#108/#109 definitions;
#6089 has no current OBJ declaration. The 2026 kingdom commit added gathering
bags #491–#494 to both genocide_gh and genocide_hall shop/reset lists. Keep
that parallel stock update as provenance for both sources, not replacement
evidence.

The shop record's keeper is #29202, which currently has an active same-number
Newbie mobile, Fleurita; its room #29204 is also an active Newbie room, “A
Turn in the Walkway.” genocide_gh instead defines a tired merchant at #29202 and
“Hall of Glowing Blood” at #29204. The configured areas/world.shp file
([db.h](../../../src/world/db.h#L22)) is absent. In non-mini mode,
open_shop_stream() falls back to globbing areas/shp/*.shp when that file
cannot be opened ([shop.c](../../../src/economy/shop.c#L3276)). The shop
loader maps the keeper through real_mobile()
([lookup](../../../src/economy/shop.c#L3492)); if this fallback includes
genocide_gh, its keeper number maps to the active Newbie prototype rather
than the unlisted genocide_gh merchant. assign_the_shopkeepers() attaches the
shop special to the resolved mobile
([assignment](../../../src/economy/shop.c#L3627),
[startup call](../../../src/specs/specs.assign.c#L1261)). This is a
conditional source-code risk, not evidence of a runtime shop load.

Compiled assignment code also binds an inn handler to
real_room0(29202) in
[specs.assign.c](../../../src/specs/specs.assign.c#L2453). On the current
full-area list, #29202 is Newbie's “Pathway of Cobbled Stones”; the separate
genocide_gh room #29202 is the executioners' resting place. This numeric code
binding does not establish that the unlisted genocide_gh room is loaded or owns
the handler. The competing room identities remain an owner decision.

## Same-number identities, history, and disposition

genocide_gh, dark_gh, and eoo all define rooms #29200–#29209; active Newbie
defines #29201–#29209. Same-number room text is different. genocide_gh
mobiles #29200–#29203 have different same-number definitions in eoo; #29200
and #29201 are additionally defined in dark_gh, and #29201–#29203 have
different active Newbie mobile owners. Newbie's reset, quest, and compiled
references remain Newbie-owned; numeric matches do not make them
genocide_gh consumers. See the [EOO identity review](EOO_AUDIT_REVIEW.md#reused-room-mobile-and-object-identities),
[Dark Guildhall audit](DARK_GH_AUDIT_REVIEW.md), and
[Newbie audit](NEWBIE_AUDIT_REVIEW.md#active-graph-and-inactive-source-only-approaches).

Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the genocide_gh
stem through the 2009 area-tree move, 2013 “stuff” commit, and later
same-stem edits. The 2026 kingdom commit updates the two separately named
Genocide guildhall shop/reset sets in parallel. No alternate-stem rename or
explicit whole-zone replacement appears in these paths. The similarly named
genocide_hall map uses header #295 and a separate #29500–#29511 range; that
similarity is not proof that it replaces genocide_gh.

Keep candidate row 47 **RETAIN IN PLACE**. Preserve the ten-room map and its
already-counted Newbie approach, Duris3 edge, four mobile definitions, three
Player Castles imports, empty OBJ file, thirteen matching shop/reset products,
and all same-number Newbie, dark_gh, and eoo identities. Resolve map and
header ownership, the #29202 inn binding, fallback shop keeper/room mapping,
stock prototypes, manifest/editor state, and persisted IDs before any
activation, move, rename, archive, or merge proposal. See the
[24 builder questions](GENOCIDE_GH_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-484-genocide-guildhall-and-newbie-boundaries),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-484-genocide-guildhall-identities-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-484-genocide-guildhall-retained-pending-map-and-shop-ownership),
[candidate row 47](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L47), and
[coverage row 397](ZONE_COVERAGE_REGISTER.csv#L398). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed
or used.

## Checkpoint 896: role-aware Newbie, Dark Guildhall, and EOO closure

The audited source tree is unchanged from snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697` through current HEAD
`c500ac51b8cd26540943257e7ca4e9807263ef74`; no `areas/` or `src/` changes
occur between them. The role scan across all 442 ZON files found matches only
in active `newbie.zon` and unregistered `dark_gh.zon` and `eoo.zon`. Newbie's
M resets use its own mobiles #29201–#29203; its room-role fields point to its
active rooms #29201, #29205, #29206, and #29209. Dark Guildhall's matching M
resets use its own #29200/#29201 mobiles, and its O/M room fields belong to
its separate map. EOO's M resets use its own #29200–#29203 mobiles; its room
fields likewise belong to the unregistered EOO map. These numeric matches do
not establish Genesis reset producers. No other ZON source matches the
candidate room/mobile roles. See [Newbie resets](../../../areas/zon/newbie.zon#L214),
[Dark Guildhall resets](../../../areas/zon/dark_gh.zon#L9), and
[EOO resets](../../../areas/zon/eoo.zon#L9).

The 265 QST files and 232 story sidecars contain exact numeric matches only
in Newbie sources. `newbie.qst` has the #29201 Burbul giver record and R/I
item commands for #29201 and #29206. The sidecar identifies contact/giver
mobile #29201 as Burbul; its completion keys use item #29201 for the map
exchange and item #29206 for Esera's spellbook exchange. Newbie defines the
map as OBJ #29201 and the tome as OBJ #29206. These are active Newbie roles,
not Genesis OBJ or mobile dependencies; Genesis OBJ is empty. No other QST or
sidecar role matched the candidate set. See [Newbie QST](../../../areas/qst/newbie.qst#L1),
[map reward](../../../areas/qst/newbie.qst#L154), [second item reference](../../../areas/qst/newbie.qst#L2616),
[Burbul sidecar contact](../../../areas/story/newbie.story.json#L16),
[Burbul contract](../../../areas/story/newbie.story.json#L435),
[Newbie map OBJ](../../../areas/obj/newbie.obj#L1), and
[Newbie tome OBJ](../../../areas/obj/newbie.obj#L112).

All 442 OBJ files were checked by object type and destination field. Five
external type-25 teleports target numbers in the Genesis room range; no
external type-29 switch room field does:

| Source object | Stored destination | Source owner and room identity |
| --- | --- | --- |
| `dark_gh.obj` #29200 | #29206 | Unregistered Dark Guildhall; Dark Guildhall also defines that room number |
| `dark_gh.obj` #29202 | #29209 | Unregistered Dark Guildhall; Dark Guildhall also defines that room number |
| `eoo_op.obj` #29210 | #29206 | Unregistered EOO-OP; current active owner of #29206 is Newbie |
| `eoo_op2.obj` #29220 | #29202 | Unregistered EOO-OP2; current active owner of #29202 is Newbie |
| `eoo_op2.obj` #29221 | #29206 | Unregistered EOO-OP2; current active owner of #29206 is Newbie |

The Dark Guildhall and EOO-OP zone files reset these objects in their own
source rooms. Newbie is the current active owner of #29201–#29209, while Dark
Guildhall and EOO each have separate unregistered rooms at those numbers.
These object targets are not Genesis WLD edges or evidence that Genesis is
loaded. Loader and destination identity remain conditional if any duplicate
map is later registered. See the [Dark Guildhall portal values](../../../areas/obj/dark_gh.obj#L7),
[second Dark Guildhall portal](../../../areas/obj/dark_gh.obj#L23),
[EOO-OP portal](../../../areas/obj/eoo_op.obj#L7),
[EOO-OP2 portals](../../../areas/obj/eoo_op2.obj#L7), and
[second EOO-OP2 portal](../../../areas/obj/eoo_op2.obj#L15).

The outside SHP keeper/room matches are also source-owned: Dark Guildhall has
keeper #29201 and room #29204; Newbie has keeper/room #29201; EOO has a
#29204 shop room, while its keeper #29204 is outside Genesis's mobile range.
Newbie's second shop uses keeper #29206 in room #29269; its stock #29202 and
#29203 are Newbie object prototypes, not Genesis mobiles. Genesis's own shop
remains keeper #29202 / room #29204. In the absent-`world.shp` fallback,
keeper #29202 still numerically maps to active Newbie mobile Fleurita and
room #29204 to the active Newbie walkway; none of these source records proves
which shops load. See [Dark Guildhall shop](../../../areas/shp/dark_gh.shp#L1),
[EOO shop](../../../areas/shp/eoo.shp#L1), [Newbie shop](../../../areas/shp/newbie.shp#L1),
[Newbie second shop](../../../areas/shp/newbie.shp#L40), and
[Newbie stock objects #29202/#29203](../../../areas/obj/newbie.obj#L44).

The exact C/C++ scan found four executable role references in this number
band. `nanny.c` compares the current room's VNUM to #29201 when preparing
Newbie starter choices; `specs.newbie2.c` looks up room #29201 for its Newbie
return; `specs.assign.c` binds the inn handler to `real_room0(29202)`; and
`specs.ailvio.c` reads object #29201 as Burbul's Newbie map. The first three
room uses currently point into active Newbie identities, while the object
lookup is a separate OBJ role. Genesis #29201/#29202 remain competing room
identities for the two room consumers and #29202 handler if the map is ever
loaded. A repeated #29201 in `constant.c` is table data, not a VNUM
reference. See [Newbie starter-room check](../../../src/account/nanny.c#L848),
[Newbie return lookup](../../../src/specs/specs.newbie2.c#L134),
[inn assignment](../../../src/specs/specs.assign.c#L2453), and
[Burbul map object lookup](../../../src/specs/specs.ailvio.c#L44).

These role matches add no WLD edge or approach count. Keep candidate row 47
**RETAIN IN PLACE**. Preserve the Newbie-owned reset, QST, sidecar, shop, and
code references separately from unregistered Dark Guildhall/EOO identities;
retain the five outside portal destinations as conditional source references.
Resolve Newbie/Dark/EOO/Genesis map-range ownership, the #29201 room and
#29202 inn consumers, fallback shops, manifests, tooling, and persisted IDs
before activation or any path proposal. The [24 builder questions]
(GENOCIDE_GH_BUILDER_QUESTIONS.md) now record the portal and code-role
findings.
