# Alterian Wilderness source audit

Checkpoint 446 reviews unregistered `component:alterian_wilderness`. This
static review reconciles its environmental story, WLD connectivity, room and
prototype identities, reset imports, and typed code dependencies. It does not
establish that these files are loaded or that source-only routes are playable.
See the [story dossier](../zone-stories/ALTERIAN_WILDERNESS.md) and
[builder questions](../zone-stories/ALTERIAN_WILDERNESS_BUILDER_QUESTIONS.md).

## Inventory and membership

| Source | Current evidence |
| --- | --- |
| [Wilderness WLD](../../../areas/wld/alterian_wilderness.wld#L1) | 130 rooms, #4750–#4919, with 366 directional exit records. |
| [Wilderness MOB](../../../areas/mob/alterian_wilderness.mob#L1) | 17 prototypes, #4800–#4816. |
| [Wilderness OBJ](../../../areas/obj/alterian_wilderness.obj#L1) | 15 prototypes, #4800–#4815, with #4807 absent. |
| [Wilderness ZON](../../../areas/zon/alterian_wilderness.zon#L1) | Zone number 47, title “Alterian Region - Wilderness,” maximum #4919; 98 M, 15 G, 10 P, 5 O, and 8 D commands. |
| QST / SHP / map / story sidecar | No same-stem source exists. |
| Current manifests | The four files are absent from [areas/AREA](../../../areas/AREA), [areas/qst/AREA](../../../areas/qst/AREA), and the empty `areas/RANDOM_AREA`. |

The active comparison areas include [Tchan](../../../areas/AREA#L24),
[Mining](../../../areas/AREA#L25), and [bs](../../../areas/AREA#L268). The
Wilderness source is not listed with them. Exact typed scans found no QST or
SHP reference to this component's mobile and object VNUMs, and no same-stem
quest, shop, map, or story contract.

## Room and prototype identities

The Wilderness map shares 20 room numbers (#4900–#4919) with active Mining
and nine (#4801–#4809) with active Tchan. None of the 29 same-numbered room
records is an exact normalized match. Preserve each map definition as a
separate source identity. In particular, Wilderness #4919 (“Mountain Trail”)
and active Mining #4919 are different records; Mountain #4920 and active
Mining #4920 are also distinct records.

The same-type mobile and object overlaps are both at #4800 with active Tchan.
Wilderness mobile #4800 is a huge brown bear; Tchan's is its guard. Wilderness
object #4800 is a black iron key; Tchan's is a different key object. Those
numeric collisions do not establish shared runtime identity or a replacement.
Other exact room/mobile/object comparison counts are recorded in
[coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).

Zone header number 47 and the title “Alterian Region - Wilderness” are unique
among the current ZON headers. The bounded tracked-path history reaches the
2009 directory move and 2013 source revision for the WLD/MOB/OBJ files; MOB
and OBJ also show a 2012 reset-format edit. The ZON history includes the 2013
revision and a 2015 quest-item-flag update. This path history shows no
alternate-stem predecessor or whole-zone replacement; it does not establish
what an external editor or older untracked source contained. See the
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-446-alterian-wilderness-map-and-prototype-identities).

## Authored place and story

The first cluster is an underground steam-cave route. #4750 is a steep,
wet vertical cavern; #4751–#4758 describe humid passages, rising steam,
iron grates, and a large smoke-filled cavern around a boiling pool. The
paired grate exits between #4752 and #4753 encode key VNUM #4800, while the
Wilderness ZON sets both door states to open. The key reference and reset
state should remain visible as a builder decision rather than being treated
as proof of current lock behavior.

Rooms #4759–#4760 form a separate two-room lair with crude furniture and
carvings. The nearby magical portals carry stored destination rooms #4759
and #4758 and are assigned the `magic_pool` object callback in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L1820). The handler uses
an object's stored room value when it processes `CMD_ENTER`
([`teleport_pools.c`](../../../src/specs/specs.teleport_pools.c#L107)). This
is a code-level travel path distinct from a WLD exit; it only establishes a
playable route if the matching object and destination are present in the
runtime indexes.

The broad surface story follows a gravel road through forest and prairie to a
farmstead: a yard and well, cornfields, a shed, a porch, and a furnished family
home. The 17 mobiles provide wildlife and rural encounters, from bear, hawk,
mountain lion, fox, and prairie dog to buffalo, displacer beast, wolverine,
and bobcat. Farther south and east, the geography turns through bog, meadow,
foothills, and mountain trail. There is no local quest giver, shop, or
structured story sidecar.

## World connectivity and map identity

The WLD-only weak exit graph has three components: 119 rooms (#4801–#4919),
nine rooms (#4750–#4758), and two rooms (#4759–#4760). It has no negative
target records. Five positive exits leave the local WLD VNUM set:

| Source record | Stored target | Source identity |
| --- | --- | --- |
| #4750 up | #4980 | Source-only Alterian Mountains; Mountain #4980 points back down to #4750. |
| #4801 south | #6368 | Active Shadow Forest (`mist`); this source room overlaps active Tchan by number. |
| #4915 east | #229817 | Source-only `surf.wld`; Surf #229817 points west to #4915. |
| #4918 south | #6368 | Active Shadow Forest (`mist`); this source room overlaps active Mining by number. |
| #4919 east | #4920 | Alterian Mountains source record; both numbers also have distinct active Mining records. |

The outward source-only links into active areas are tracked in earlier
reviews: five non-overlapping approaches to Mining (#4897 east to #4901,
#4898 east to #4900, and #4899 north/east/south to #4901/#4908/#4900), two
approaches to Shadow Forest #6368, and four distinct-origin approaches to
Tchan. Tchan also has 16 more matching target numbers from Wilderness source
rooms #4801–#4809; those origins overlap active Tchan room numbers and do not
add 16 distinct source-only entrances. See the [Mining](MINING_AUDIT_REVIEW.md#inactive-wld-sweep-and-overlapping-sources),
[Shadow Forest](SHADOW_FOREST_AUDIT_REVIEW.md), and
[Tchan](TCHAN_AUDIT_REVIEW.md) supplements. No active WLD exit targets a
Wilderness-only room identity, so candidate row 8's active inbound count
remains zero. All these edges remain source evidence, not proof of active
ingress or safe return.

## Typed dependency census

Wilderness ZON's 98 M commands include 30 resets referencing 13 mobile
prototypes from `alterian_mountains.mob`. The reverse dependency is present
too: Mountain ZON references Wilderness mobiles #4812 once and #4813 twelve
times, plus Wilderness object #4806. Wilderness ZON also has 17 M reset room
fields in active Mining's numeric range. These source-only reset commands do
not establish loader or prototype resolution.

There are additional typed consumers beyond the paired Alterian sources:

* Active [bs.zon lines 573–574](../../../areas/zon/bs.zon#L573) each use
  object #4814, which Wilderness defines as an ear of corn. The source comment
  says that object type is not in the local `.obj` file. This is an active
  typed dependency, not proof that the source-only Wilderness prototype
  resolves in the running object index.
* [`specs.assign.c` line 623](../../../src/specs/specs.assign.c#L623) binds
  mobile #4812 to `poison`; Wilderness #4812 is the displacer beast. The code
  also binds #4800 to `newbie_guard_east` at
  [`specs.assign.c` line 600](../../../src/specs/specs.assign.c#L600), while
  active Tchan owns a different #4800 guard. Keep that callback ownership
  explicit.
* [`specs.assign.c` lines 1820–1821](../../../src/specs/specs.assign.c#L1820)
  assign `magic_pool` to object #4801 and #4802. Their stored room values are
  #4759 and #4758, and Wilderness ZON places them in #4758 and #4759.
* Exact QST/SHP scans found no references to the Wilderness mobile/object
  VNUM sets; no same-stem QST, SHP, or story sidecar exists.

Wilderness object #4800 is also carried by a Wilderness reset following a
Mountain mobile #4831 in room #4760. Its object number collides with Tchan's
different active #4800, so preserve both reset and prototype identities.

## Disposition

Keep candidate row 8, `alterian_wilderness`, **RETAIN IN PLACE**. Preserve its
130-room map, separate Mining and Tchan room records, #4800 mobile/object
identities, 15 objects, and reset dependencies with Alterian Mountains and
active `bs`. Keep the five Mining, two Shadow Forest, and four Tchan
source-only approaches distinguishable from active map access. The static
story, graph, identity, and typed-source review is complete; loader behavior,
runtime availability, external editor state, and persisted references remain
unqualified. See [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8),
[coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359), and the
[builder questions](../zone-stories/ALTERIAN_WILDERNESS_BUILDER_QUESTIONS.md).

## Bloodstone Checkpoint 778: active object consumer

Active Bloodstone ZON lines 573–574 use candidate object #4814 in two P reset rows. The current active manifest has no object owner for #4814; candidate row 7 still defines it in alterian_wilderness.obj. Preserve the source-only candidate under RETAIN IN PLACE until owners resolve the active reset dependency; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#registered-candidate-only-dependencies).

## Checkpoint 881: external source identities reconciled

An all-95 inactive-WLD sweep found exactly three exit records from other files
into this map: Alterian Mountains #4920 west to #4919 and #4980 down to #4750,
and Surf #229817 west to #4915. Each is the reciprocal counterpart of an
existing Wilderness outbound exit. No other inactive WLD source overlaps or
approaches these 130 rooms. The active-WLD candidate ingress count remains
zero for Wilderness-only room identities; Mining and Tchan own their separate
same-numbered rooms.

A role-aware scan of all active and source-only ZON files found sixteen exact
external reset operands into Wilderness prototype identities: Alterian
Mountains ZON has thirteen M resets for mobiles #4812/#4813 and one E reset
for object #4806; active Bloodstone has two P resets for object #4814. The
scan also found 36 active room operands whose numbers overlap Wilderness
rooms but resolve to active Mining (32 operands) and Tchan (four operands),
plus Tchan's M reset for mobile #4800. Those numbers resolve to Mining/Tchan
prototypes, not the distinct Wilderness records.

Typed scans found no external QST giver or item command, SHP keeper or product,
or structured story-sidecar VNUM targeting a Wilderness mobile or object. The
two raw Mining QST values #4912 and #4911 are item commands for Mining objects;
they are outside Wilderness's #4800–#4815 object set. Mining's sidecar
`mob_vnum` entries #4900–#4919 and its #4911 `item_vnums` entry likewise
resolve by role to Mining prototypes; these are room-number overlaps, not
Wilderness mobile/object references. No external object value references a
Wilderness object, and no ITEM_TELEPORT or ITEM_SWITCH value targets a
Wilderness room. Existing code callbacks for #4800, #4812, #4801, and #4802
remain the separately documented typed code uses above.

This closes the bounded tracked-source exit and typed-reference census for
this candidate. It does not qualify loader/manifest behavior, editor state,
deployment, persisted identity, runtime availability, or owner intent. Keep
candidate row 8 **RETAIN IN PLACE** until those qualifications are resolved.
See the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-880-alterian-wilderness-source-only-inbound-routes),
[candidate plan](ATTIC_MIGRATION_PLAN.md#checkpoint-881-alterian-wilderness-source-closure-and-retention),
[candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), and
[coverage row 359](ZONE_COVERAGE_REGISTER.csv#L359).
