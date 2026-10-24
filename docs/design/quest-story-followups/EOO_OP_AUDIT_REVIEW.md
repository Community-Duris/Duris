# `eoo_op` Muspelheim source audit

Canonical key: `component:eoo_op`. Physical coverage row 388 / CSV line
389; candidate dependency row 38. Reviewed October 11, 2026, Checkpoint 475.
This is a static source, graph, identity, history, and typed-reference review.
It does not establish runtime loading, intended portal destinations, safe
traversal, or permission to activate or move the component.

## Source scope, title, and story

The candidate comprises [WLD](../../../areas/wld/eoo_op.wld),
[MOB](../../../areas/mob/eoo_op.mob), [OBJ](../../../areas/obj/eoo_op.obj),
and [ZON](../../../areas/zon/eoo_op.zon). It is absent from full
[areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); there is no
`eoo_op.qst`, SHP, story sidecar, or editor-map file in this stem.
Its raw AREA annotation is blank. The ZON title is `Eye of Odin Outpost`,
the only exact match among tracked ZON headers.

WLD/MOB prose describes Muspelheim, Surt, a throne room, a wall of living
flame, the nine worlds, a flame of life, and the sword Twilight. The mobiles include Surt, a mass of burning
flames, Pyros the lava dragon, and an eldritch fire serpent. The prose
connects thematically to EOO’s Valhalla material, but there is no quest record
or sidecar here that defines a canonical sequence, owner, or playable
questline. Preserve the spelling, lore, and candidate identity for review; do
not infer that a shared mythological setting makes these files one map.

## Candidate WLD graph and active-table names

The WLD has three rooms, #29210–#29212, and five directions. Four are local:
#29210 north to #29211 and #29211 south to #29210; #29211 east to #29212
and #29212 west to #29211. From #29210, the candidate-only directed graph
reaches all three rooms and forms one weak component. The fifth direction is
#29210 up to #123225, “The Ocean,” in inactive [Duris3.wld](../../../areas/wld/Duris3.wld#L487726);
that room has no active owner and no return exit to this candidate. There are
no WLD fall or current records.

Every eoo_op room VNUM also occurs in both inactive [dark_gh.wld](../../../areas/wld/dark_gh.wld)
and active [newbie.wld](../../../areas/wld/newbie.wld), with different
complete room blocks. The current active table therefore resolves all four
local candidate exits to Newbie room records sharing the same numbers; the
exit to #123225 does not resolve. That numeric result does not make the
candidate’s rooms Newbie rooms or show how these source files would behave if
added to the manifest.

A full active-WLD scan found seven Newbie directions whose raw targets are
#29210, #29211, or #29212:

| Active Newbie source | Direction | Raw target | Active Newbie target room |
| --- | --- | ---: | --- |
| #29208 | east | #29210 | “A Turn in the Walkway” |
| #29210 | north | #29212 | “Rogues Alley” |
| #29212 | south | #29210 | “A Turn in the Walkway” |
| #29213 | west | #29212 | “Rogues Alley” |
| #29214 | south | #29212 | “Rogues Alley” |
| #29234 | south | #29211 | “An Alchemist’s Garden” |
| #29279 | west | #29210 | “A Turn in the Walkway” |

These are Newbie-to-Newbie directions. Since all three candidate room numbers
already have active Newbie owners, candidate-identity inbound remains zero;
do not count the seven raw numeric matches as EOO Outpost ingress or add a
source-only approach to the curated totals. Those totals remain 11,987 overall
and 415 from Surf.

## Same-number prototypes and type-25 portals

The three eoo_op rooms have six complete same-number WLD comparisons across
Newbie and dark_gh; none is identical. Its four mobiles #29210–#29213 each
have a different same-number active Newbie MOB record. Its four objects with
those IDs each have a different same-number active Newbie OBJ record. The
Newbie records are the currently active walkways, garden, Rogue’s Alley, fish,
and fish-related objects; they are not evidence that the Muspelheim rooms,
creatures, or portals were merged or replaced.

All four eoo_op objects are type 25 ([ITEM_TELEPORT](../../../src/core/defines.h#L140)). In
[spell_travel.c](../../../src/magic/spell_travel.c#L991), `value[0]` is
resolved as a room VNUM ([lookup](../../../src/magic/spell_travel.c#L995)), `value[1]` must match the entered command, and
`value[2]` must be nonzero. `CMD_ENTER` is 7
([interp.h](../../../src/cmd/interp.h#L42)). Each candidate portal stores
command 7 and `value[2] = -1`; the current code decrements only positive
charge values ([charge handling](../../../src/magic/spell_travel.c#L1173)), so the negative value is not consumed. Confirm that charge
setting and each destination with the object owner.

| Object | Description | Stored target VNUM | Current active-table room owner | Other tracked candidate identity |
| ---: | --- | ---: | --- | --- |
| #29210 | Wall of Flames | #29206 | Newbie “Inside The Shaman’s Guild” | EOO #29206, “A Bright Hallway within Valhalla”; dark_gh also defines #29206 |
| #29211 | Portal to Undead Continent | #29222 | Newbie “An Intersection of Walkways” | eoo_op2 #29222, Hel’s Inner Sanctum; dark_gh also defines #29222 |
| #29212 | Portal to Khomani-Kan | #29212 | Newbie “Rogues Alley” | This component’s #29212 is its Wall of Flames room; dark_gh has a different #29212 |
| #29213 | Portal to Goodie continent | #29225 | Newbie “A Turn in the Walkway” | eoo_op2 #29225, a second Hel Inner Sanctum room |

The prose labels and stored numbers do not establish intended destinations.
In particular, the #29211 and #29213 targets also have eoo_op2 Hel-map
definitions, while the current active table selects Newbie rooms at those
numbers. Preserve the target VNUMs and descriptions as separate facts until
their source identities and map owners are confirmed.

## Resets, cross-component references, and history

eoo_op.zon resets all four mobiles into #29210. It also places object #29210
in #29212 and imports object #11051 into #29210; the Player Castles audit
already records #11051 as a source-only external object import
([reset](../../../areas/zon/eoo_op.zon#L9),
[prior import review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers)).
The three other eoo_op portals, #29211–#29213, are not reset by eoo_op.zon.
EOO’s separate source-only [ZON](../../../areas/zon/eoo.zon#L10) imports
#29211 into #29202 and #29212–#29213 into #29206. Preserve this explicit
cross-component object dependency; under active Newbie’s same-number OBJ
table, those VNUMs instead identify Newbie prototypes. There is no SHP or QST
source file in the eoo_op stem.

Followed WLD and ZON histories reach the 2009 area-tree move and a 2013
same-stem commit whose diff for those two files is zero lines. MOB and OBJ
history reaches the 2009 move. No alternate-stem predecessor or explicit
whole-component replacement appears in those paths. This bounded history does
not identify the current owner, the map/editor source of truth, or whether the
component is a retained alternate.

Keep candidate row 38 **RETAIN IN PLACE**. Resolve the EOO/EOO_OP/EOO_OP2 map
and lore relationship, the four portal destinations, the #123225 Duris3
boundary, all Newbie and dark_gh same-number identities, the Player Castles
import, external tools, code, deployment, and persisted-VNUM closure before
any activation, move, rename, archive, or merge proposal. See the
[24 builder questions](EOO_OP_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-475-eoo-op-muspelheim-map-and-portal-targets),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-475-eoo-op-muspelheim-map-identity-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-475-eoo-op-retained-pending-map-portal-and-owner-review),
[candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and
[coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389). No area/source files,
tests, builds, runtime, server, database, or containers were changed or used.

## Checkpoint 887: typed-role boundary closure

This pass classified references to EOO-OP's #29210-#29212 rooms, #29210-#29213 mobiles, and #29210-#29213 objects by the fields the zone loader actually resolves ([`renum_zone_table`](../../../src/world/db.c#L1612)). Same-number hits are not assigned to the candidate when an active Newbie or inactive dark_gh source owns the source record.

The active WLD finding remains seven Newbie-to-Newbie directions to shared #29210-#29212 rooms and zero candidate-identity inbound exits. The external ZON scan found eleven room-role operands in the shared room band: seven `dark_gh.zon` placements (three O room fields and four M room fields, the latter in #29210) and four active `newbie.zon` placements (one O in #29212 and three M room fields in #29211-#29212). These are placements in dark_gh/Newbie map identities, not EOO-OP ingress. Eight other active Newbie M reset rows use mobile VNUMs #29210-#29213; the active Newbie table owns those same-number fish mobiles. The three EOO ZON O resets of #29211-#29213 remain explicit external object-prototype references to EOO-OP portal IDs, but the active Newbie OBJ table has different same-number prototypes. Preserve the source import intent without resolving it by the current active numeric owner ([EOO ZON imports](../../../areas/zon/eoo.zon#L10), [Newbie resets](../../../areas/zon/newbie.zon#L310), [dark_gh placements](../../../areas/zon/dark_gh.zon#L14)).

A full external OBJ type-25/type-29 scan found two additional numeric room targets, both in source-only `dark_gh.obj`: #29201 -> #29212 and #29206 -> #29211. Those portals belong to dark_gh source objects, and both destination numbers also have separate Newbie and EOO-OP room definitions. No type-29 target was found. The EOO-OP four type-25 portal records and their existing value[0] targets remain as documented in Checkpoint 475; the dark_gh references do not establish an EOO-OP return path ([dark_gh portal target #29201](../../../areas/obj/dark_gh.obj#L15) and [#29206](../../../areas/obj/dark_gh.obj#L55)).

No external QST line, story-sidecar typed value, parser-valid N-format SHP keeper/room/product field, or direct C/C++ numeric reference matched #29210-#29213. EOO-OP still has no QST, SHP, or same-stem story sidecar. The earlier narrative, portal target, Player Castles #11051, and one-way Duris3 findings are unchanged. Keep candidate row 38 **RETAIN IN PLACE**, preserve every separate map/prototype identity and the three EOO reset imports, and leave owner, canon, tool/editor, deployment, and persisted-VNUM decisions open. No WLD count or approach total changes. See the [24 builder questions](EOO_OP_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-887-eoo-op-typed-boundaries), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-887-eoo-op-retains-role-separated-map-and-portal-identities), [candidate row 38](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L38), and [coverage row 388](ZONE_COVERAGE_REGISTER.csv#L389). No source, runtime, tests, builds, server, or database was changed or used.
