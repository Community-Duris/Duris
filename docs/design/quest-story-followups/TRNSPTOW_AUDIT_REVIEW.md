# The Transparent Tower audit supplement

Reviewed October 11, 2026 against source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This supplement reconciles the
active WLD graph, outside source references, title/history, and retention for
the [Transparent Tower source dossier](../zone-stories/TRNSPTOW.md). No world
or gameplay source changed.

## Active graph and world boundaries

The active WLD contains 100 rooms and 248/248 resolving exit records: 247
local and one outbound. Its local undirected graph has four weak components of
63, 27, 8, and 2 rooms. With all authored exits treated as passable, #16228
reaches 63 rooms and 30 rooms can return to it; #16271 reaches and is reachable
from 27; #16212 reaches 28 and 43 can return to it. These are raw directional
WLD counts, not proof that doors, hidden routes, speech gates, traps, reset
availability, or actor restrictions permit those journeys.

Tower's drawbridge #16212 has a south (`D2`) exit to ConnectorZones #54167;
ConnectorZones has a reciprocal north (`D0`) exit. Realm #14209 south enters
Tower #16271, with no reverse WLD edge. See the active
[Realm record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L253), [Tower outbound
record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L287), [ConnectorZones return
record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L720), and their
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L98). The Tower dossier also
records an ordinary route from #16228 through #16229 south to #16212; it
bypasses the mist-key closet and must remain distinct from the claimed escape
story.

One inactive `surf.wld` record is a source-only approach: room #245010,
“Clearing on an Island of Mist,” exits north (`D0`) to #16212. It is not active
ingress because `surf.wld` is not in the active area list. Preserve it as
candidate map evidence, separate from the two active inbound edges.

## Typed dependency closure

| Reference type | Source-backed result |
| --- | --- |
| Local resets importing external objects | [Tower reset for epic rune #359](../../../areas/zon/trnsptow.zon#L146) and [unique sword #67276](../../../areas/zon/trnsptow.zon#L156) place them in closet #16249; [reset for ioun #906](../../../areas/zon/trnsptow.zon#L238) places it on Aceralde. Preserve their external prototype identities and the existing epic-stone/Aceralde behavior. |
| Active outside resets consuming Tower objects | Four active rows target local prototypes: [Barovia reset](../../../areas/zon/barovia2.zon#L396) gives mace #16242; [Future Ceothia reset](../../../areas/zon/ceofutur.zon#L313) gives globe #16262; [Shaboath resets](../../../areas/zon/shabo.zon#L366) give leg plates #16222 and earring #16248 (line 367). These are outside reset suppliers, not Tower-local reset placements. |
| Foreign QST and sidecar consumers | [ConnectorZones QST](../../../areas/qst/connectorzones.qst#L175) has three Adventurer alternatives: mage consumes #16210/#16269/#16224/#16249/#16232; warrior consumes #16210/#16269/#16248/#16249/#16234; priest consumes #16210/#16269/#16244/#16235/#16224. Its [story sidecar](../../../areas/story/connectorzones.story.json#L305) types the same nine local item VNUMs. No foreign recipe was found for the scepter #16241, token #16264, or mist key #16258; the existing dossier's narrower statement about those three remains accurate. Preserve all three alternatives and their exact materials together. |
| Active procedure bindings and compiled references | [`specs.assign.c`](../../../src/specs/specs.assign.c#L380) binds Aceralde #16205 to `transp_tow_acerlade`; [stone bindings](../../../src/specs/specs.assign.c#L1373) cover objects #16263/#16268; [globe #16262](../../../src/specs/specs.assign.c#L1722) uses `trans_tower_shadow_globe`; and [mace #16242](../../../src/specs/specs.assign.c#L2172) uses `zion_light_dark`. The #16217, #16269, and #16244 object references in the [CHAOS equipment tables](../../../src/account/chaos_eq_data.h#L91) are equipment-profile dependencies in both standard and enhanceable tables. [`randomeq.c` lists #16205](../../../src/item/randomeq.c#L141) in `highdrop_mobs`, but its only matching loop is [commented out](../../../src/item/randomeq.c#L650); treat that numeric list entry as dormant, not as an active drop rule. |
| Shared room-keyed procedure | [`guild_guard` has a compiled case](../../../src/specs/specs.guards.c#L199) for home room #16283 that blocks south for non-shamans. [Tower reset](../../../areas/zon/trnsptow.zon#L287) places black abishai #16222 there; the bounded assignment scan found no local mobile bound to `guild_guard`. This is a conditional shared-code room collision, not a verified live Tower gate. |
| Shops | Tower has no local SHP. The unregistered, source-only `luiren.shp` has six old-format `O` records whose `in_room` fields overlap Tower rooms: #16254 ([line 57](../../../areas/shp/luiren.shp#L57)), #16277 ([line 88](../../../areas/shp/luiren.shp#L88)), #16276 ([line 119](../../../areas/shp/luiren.shp#L119)), #16275 ([line 150](../../../areas/shp/luiren.shp#L150)), #16255 ([line 183](../../../areas/shp/luiren.shp#L183)), and #16207 ([line 245](../../../areas/shp/luiren.shp#L245)). It has no active `luiren` WLD/MOB/ZON membership. The [shop loader fallback](../../../src/economy/shop.c#L3267) can glob `areas/shp/*.shp` if `areas/world.shp` is absent, but the [boot parser](../../../src/economy/shop.c#L3322) rejects old `O` records. These lines are neither evidence of functioning Tower shops nor a reason to activate Luiren; preserve the separate source/loader warning also documented by the [Pine Hollow review](PINE_HOLLOW_AUDIT_REVIEW.md#exact-typed-dependencies). |
| Type-25 objects and prototype collisions | The full active object scan found no outside type-25 destination into a Tower room. Tower has eleven reset-placed type-25 portals plus unused #16200. Current active WLD/MOB/OBJ headers have no same-kind external VNUM collision with the Tower's local sets. |

The repository-wide CHAOS references and ConnectorZones recipes mean that
changes to the relevant Tower object VNUMs or identities have consumers beyond
the local quest. Keep object, reset supplier, reward, and procedure ownership
separate; matching numeric values alone do not merge their roles.

## Title, history, and disposition

After color removal and whitespace/case normalization, “The Transparent
Tower” has one owner among 442 current `.zon` titles: `trnsptow`. The bounded
WLD, MOB, OBJ, ZON, and QST `--follow` histories stay on their present stem
through the 2009 area-tree move and later same-stem maintenance. The
`trnsptow.story.json` sidecar was added October 4, 2026. No alternate-stem
rename, distinct predecessor, or whole-zone replacement is established by
this history review. Preserve the active area **DO NOT MOVE**. Keep the Realm
one-way arrival, reciprocal ConnectorZones crossing, source-only Surf edge,
external item recipes, reset suppliers, compiled equipment references, and
source-only Luiren SHP room-number overlap distinct. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L313),
[identity/history register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-400-transparent-tower-title-and-bounded-history),
[connectivity log](WORLD_CONNECTIVITY.md#checkpoint-400-transparent-tower-boundaries-and-directed-access),
and [retention log](ATTIC_MIGRATION_PLAN.md#checkpoint-400-transparent-tower-retained-in-place).

## Checkpoint 855: Current graph, Surf approach, and typed closure carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 400 source review and builder recommendations forward without claiming a second independent parse. The Transparent Tower remains 100 rooms with 248/248 resolving exits (247 local, one outbound), in four weak components. The active boundaries remain the reciprocal ConnectorZones drawbridge and one-way Realm arrival. Inactive Surf #245010 north to Tower #16212 stays source-only on existing candidate row 97 and is already counted in the current 11,993 overall / 415 Surf total; the earlier Checkpoint 400 count of 9,480 / 298 is historical.

The recorded typed closure remains unchanged: local resets import Heavens OBJ #359, Unique OBJ #67276, and OBJ #906; active Barovia2, Ceofutur, and Shabo resets supply Tower objects #16242/#16262/#16222/#16248; ConnectorZones QST and sidecar consume nine Tower equipment VNUMs across three alternatives; and CHAOS tables plus direct procedures retain their existing object references. Keep the source-only `luiren.shp` old-format room collisions separate; its `O` rows are rejected by the parser and do not establish Tower shops. No outside type-25 target or same-kind active prototype collision into Tower was established. The twenty builder questions from Checkpoint 400 remain source review, not gameplay qualification.

Keep active `trnsptow` **DO NOT MOVE**. Preserve the ConnectorZones and Realm boundaries, the Surf approach on existing candidate row 97, external recipes/reset suppliers, compiled references, and the Luiren loader warning with their current owners. Keep `surf` / `surface2011` **RETAIN IN PLACE** pending ownership and tooling closure. No source activation, relocation, or migration clearance follows. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L313), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-855-transparent-tower-boundaries-and-surf-approach-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
