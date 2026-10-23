# Tharnadian Causeway (Aopal) source audit

Checkpoint 448 reviews `component:aopal`, physical coverage row 361. The
candidate files are source-only: the `aopal` entry is commented out in both
the full-world and QST manifests. This audit records the authored story,
static map, competing room identities, reset imports, and compiled patrol
callback. It does not establish live availability or approve a source move.
See the [story dossier](../zone-stories/AOPAL.md) and
[builder questions](../zone-stories/AOPAL_BUILDER_QUESTIONS.md).

## Inventory and loader status

| Source | Current evidence |
| --- | --- |
| [Aopal WLD](../../../areas/wld/aopal.wld#L1) | 44 rooms, #43301–#43344, and 94 authored directions. |
| [Aopal MOB](../../../areas/mob/aopal.mob#L1) | 15 prototypes, #43301–#43315. |
| [Aopal OBJ](../../../areas/obj/aopal.obj#L1) | Five prototypes, #43301–#43305. |
| [Aopal ZON](../../../areas/zon/aopal.zon#L1) | Header #433, title “The Tharnadian Causeway,” maximum #43344; 12 D, 3 O, 37 M, 1 G, 1 E, and 2 F commands. |
| QST / SHP / map / story sidecar | No same-stem file, native QST, shop record, map file, or structured story sidecar. |
| Current manifests | The commented `*aopal` lines occur in [areas/AREA](../../../areas/AREA#L206) and [areas/qst/AREA](../../../areas/qst/AREA#L189); neither loads the candidate. |

The AREA comments describe a road connected to Tharnadia. That note is
editorial context; Aopal's three exits outside its own room set point to
Shadow Forest, an ambiguous unlisted Surface VNUM, and Ancient Bridge. The
static source scan did not find an exit into active Tharnadia.

## Authored setting and story

The map presents a protected eastern causeway outside Tharnadia, with white
brick road, merchants, travelers, and the city's Outer Keep as its first line
of defense. The bastion contains a drilling watch, archer slits, guard posts,
upper landings, and a patrol captain. An aqueduct and reservoir sit above the
road; the source prose pairs their engineering setting with rats and dogs in
the water. The second story thread is the well: ghostly Samara is trapped
beneath it, and the reset gives her a transparent, soaking wet white dress.

The captain's station, patrol shop, and spawn room connect the fortress story
to a highway-defense service. The authored NPC list also includes a Highlord
guard, local militia, causeway guards, a traveling merchant from Woodseer,
animals, and elite highway personnel. No local QST gives Samara or the patrol
service a quest contract; the source tells these stories through room prose,
mob descriptions, equipment, and resets.

## World connectivity and reachability

The 44-room source graph has 94 directions and two weak components: a
43-room component and isolated room #43343 (“Spawn Room”). There are no
negative destinations. From #43301, directed local traversal reaches 42
rooms; #43342 and #43343 are outside that forward reach. From patrol staging
room #43342, 43 rooms are reachable, but #43343 remains separate. The
isolated spawn room has one north exit to Ancient Bridge #85606.

Three direction records leave Aopal's local VNUM set:

| Aopal source | Target | Source identity and qualification |
| --- | --- | --- |
| [#43301 west](../../../areas/wld/aopal.wld#L1) | Shadow Forest #6368 | Active target number, but Aopal #43301 has a different room record from active Ship Yards #43301. The source edge is not loaded ingress. |
| [#43323 east](../../../areas/wld/aopal.wld#L450) | #230412 | The source-only `surf.wld` record is “The Eastern Road Just Outside The Tharnadian Causeway” and returns west to #43323; `surface2011.wld` defines #230412 as a storm sea room with a different west exit. Neither map is active. See [Surf](../../../areas/wld/surf.wld#L637948) and [Surface2011](../../../areas/wld/surface2011.wld#L638653). |
| [#43343 north](../../../areas/wld/aopal.wld#L817) | Ancient Bridge #85606 | Active destination VNUM, but the Aopal source room is isolated inside its own WLD graph. The earlier bridge review records this source-only approach. |

Four additional Aopal directions from non-overlapping source rooms target
Ship Yards VNUMs: [#43329 down to #43319](../../../areas/wld/aopal.wld#L560),
[#43330 down to #43321](../../../areas/wld/aopal.wld#L583),
[#43334 up to #43324](../../../areas/wld/aopal.wld#L662), and
[#43342 north to #43324](../../../areas/wld/aopal.wld#L808). These destination numbers also name
Aopal rooms, so the source map itself treats these directions as local. They
remain distinct inactive-source approaches to the active Ship Yards identity
and do not merge the maps.

Aopal shares 28 room VNUMs (#43301–#43328) with active `shipy`; every paired
room record differs. Of the 62 Aopal directions whose target numbers fall in
the Ship Yards range, 58 originate in those same-numbered room records and
four originate in the non-overlapping rooms listed above. Active WLD files
have 30 numeric target matches into #43301–#43344: 29 Ship Yards directions
to its own rooms and one Malch exit from [room #23760](../../../areas/wld/malch.wld#L3294)
to #43320, an active Ship Yards room.
Ship Yards room. No active WLD edge targets Aopal-only room IDs #43329–#43344.
Keep candidate inbound count at zero; numeric matches do not establish
ingress to Aopal's map identity.

Across the full inactive-WLD scan, Aopal's six already recorded
target-specific source-only approaches are the Shadow Forest exit, the four
Ship Yards exits, and the Ancient Bridge exit. No additional Aopal approach
was found. They remain inactive declarations, not current travel or evidence
of safe return.

## Title, numeric identity, and tracked history

The current ZON header is #433 and its title is “The Tharnadian Causeway.”
The repository's current ZON scan found no other #433 header or normalized
title match. The `aopal` manifest comments are the only repository alias
annotation identified. A bounded `git log --follow` review of WLD, MOB, OBJ,
and ZON reaches the 2009 directory move. MOB and OBJ also have the 2012
format edit and a 2013 same-stem revision; the tracked 2013 WLD and ZON
diffs are empty. The ZON has a 2015 quest-item-flag edit. These paths show no
alternate-stem predecessor or whole-zone replacement. They cannot identify
external editor aliases or persisted room ownership.

The same-numbered mobile and object definition scan found no other current
MOB prototype for #43301–#43315 or OBJ prototype for #43301–#43305. This
differs from the 28 separate Ship Yards room identities: same VNUMs do not
mean same rooms.

## Typed dependency census

The ZON contains 37 mobile reset commands. Thirty-four use Aopal prototypes;
three refer to other mobile sources:

| Reset | Evidence | Qualification |
| --- | --- | --- |
| Mobile #6116 at room #43301 | [Aopal ZON line 65](../../../areas/zon/aopal.zon#L65) | No mobile prototype #6116 was found in the repository's MOB sources. |
| Mobile #445 at room #43301 | [Aopal ZON line 66](../../../areas/zon/aopal.zon#L66) | Defined in active `heavens.mob` as a Tharnadian highway patrolman ([prototype](../../../areas/mob/heavens.mob#L1679)); this Aopal reset is still source-only. |
| Mobile #445 at room #43322 | [Aopal ZON line 87](../../../areas/zon/aopal.zon#L87) | Same active Heavens prototype; source-only reset. |

The three O commands place the wagon and two signs. The G command gives
object #43304, Samara's wet white dress, to local mobile #43307; the E
command equips local mobile #43313 with object #43305, the Highlords mantle.
Two F commands place elite bastion guards following the captain. The 12 D
resets match the 12 WLD directions with nonzero door-state flags: ten reset
to state 1 and the well's down/up pair reset to state 5; all are authored at
100 percent. Every WLD key field is zero.

No external QST item command, SHP product, structured sidecar `mob_vnum` or
`item_vnums` field, or same-type mobile/object prototype consumer was found
for Aopal's local VNUM sets. There is no Aopal `.shp` record.

Compiled code binds `patrol_shops` to numeric room #43341 in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L2333). The function
lists mobiles in `ch->in_room + 1` and, on purchase, loads a new copy into
`pet_room + 1` before calling `do_flee` and `mobPatrol_SetupNew`
([function](../../../src/specs/specs.room.c#L770)). With the authored
assignment at #43341, those are staging room #43342 and spawn room #43343;
the ZON places patrolman #43314 and marshall #43315 in #43342. This is a
compiled source relationship, not proof that the unlisted rooms are loaded
or that the service currently works.

## Safe disposition

Keep candidate row 10 **RETAIN IN PLACE**. Preserve the two source-manifest
comments, 28 distinct Ship Yards room records, all six source-only outward
approaches, the Surf/Surface2011 #230412 identity conflict, 37 mobile resets
(including missing #6116 and active-Heavens #445 imports), the door states,
and the #43341 patrol callback. Do not merge room records or move the files
based only on the Tharnadia annotation or numeric overlaps. Any later path
decision still needs the map owner's loader intent and external editor,
persisted-state, and live runtime evidence.

The earlier source-only checkpoint totals are inconsistent: existing notes
report 4,529, 9,480, and 1,507. Checkpoint 448's full recount defines the
target-map identity rule and replaces those cumulative snapshots; it does
not change Aopal's candidate inbound count. See the
[current connectivity recount](WORLD_CONNECTIVITY.md#checkpoint-448-aopal-source-graph-and-current-inactive-wld-recount),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-448-aopal-tharnadian-causeway-title-history-and-room-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-448-aopal-retained-with-source-only-approaches),
[candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10), and
[coverage row 361](ZONE_COVERAGE_REGISTER.csv#L361).
