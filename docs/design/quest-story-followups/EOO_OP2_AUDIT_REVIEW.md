# `eoo_op2` Niflheim and Hel source audit

Canonical key: `component:eoo_op2`. Physical coverage row 389 / CSV line
390; candidate dependency row 39. Reviewed October 11, 2026, Checkpoint 476.
This is a static source, graph, identity, history, and typed-reference review.
It does not establish runtime loading, intended portal behavior, safe travel,
or permission to activate or move the component.

## Source scope, title, and story

The candidate comprises [WLD](../../../areas/wld/eoo_op2.wld),
[MOB](../../../areas/mob/eoo_op2.mob), [OBJ](../../../areas/obj/eoo_op2.obj),
and [ZON](../../../areas/zon/eoo_op2.zon). It is absent from full
[areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); there is no
`eoo_op2.qst`, SHP, story sidecar, or editor-map file in this stem.
The raw AREA annotation is blank. Its generic ZON title,
`The Horrible Tracts of the Unnamed`, is also used by tracked
`astral`, `mtncastl`, `theurgist`, and
`ud-inside` ZON files. All five stems are absent from the current full
area and QST manifests, so the repeated title does not identify an alias or
common map owner.

The prose describes Niflheim as the realm of the dishonored dead, Hel as its
ruler, the gates of Eliudnir, and a plan to use a secret portal to challenge
Asgard. Mob text also names Loki, Angrboda, Fenris, and Jormungand. The
component has no QST or story sidecar establishing a canonical quest sequence
or linking these passages to EOO. Preserve this Norse-myth material and the
source identity while its owner confirms the relationship.

## WLD graph and repeated passage text

The WLD has six rooms, #29220–#29225, and nine directions. They form two
separate reciprocal three-room chains:

- #29220 north ↔ #29221 south; #29221 east ↔ #29222 west.
- #29223 north ↔ #29224 south; #29224 east ↔ #29225 west.

There are two weak components of three rooms each. Directed WLD traversal
from #29220 reaches only its own three-room chain. The second entry room,
#29223, also points up to #151855, “Grassy Plains,” in inactive
[Duris3.wld](../../../areas/wld/Duris3.wld#L1088956). That Duris3 room has
no active owner and its four exits stay within Duris3; none returns to #29223.
There are no fall or current records.

The title and prose for #29220/#29223 (Niflheim), #29221/#29224 (the gates),
and #29222/#29225 (the Inner Sanctum) match pairwise. These are repeated
passage descriptions, not identical complete room records: the VNUMs and exit
targets differ, and only #29223 has the Duris3 boundary. Preserve both chains
until the builder explains whether they are intentional parallel entrances,
separate portal destinations, or duplicated source content. The WLD-only
graph remains disconnected even though type-25 portal fields describe
source-level links to EOO.

Every one of the six candidate room VNUMs also belongs to active
[newbie.wld](../../../areas/wld/newbie.wld), with a different complete room
block; #29220–#29222 also occur in inactive
[dark_gh.wld](../../../areas/wld/dark_gh.wld), again with different blocks.
The current full-area table therefore resolves all eight local eoo_op2 exit
targets to active Newbie identities, while #151855 is unresolved. A scan of
active WLD records found 16 Newbie directions whose raw targets are
#29220–#29225. They are Newbie-to-Newbie routes. Candidate-identity inbound
remains zero because none of the candidate room VNUMs is unique to eoo_op2;
do not count the 16 shared-number matches as eoo_op2 ingress or add a
source-only approach to the curated totals. Those totals remain 11,987
overall and 415 from Surf.

## Same-number prototypes and portal pairs

The six eoo_op2 rooms have nine complete same-number WLD comparisons: six with
active Newbie and three with inactive dark_gh. None is identical. Mobiles
#29220–#29223 and objects #29220–#29221 each have different complete
same-number active Newbie prototypes.

The object collisions have direct Newbie consumers. Active
[newbie.obj](../../../areas/obj/newbie.obj#L341) #29220 is a salt box and
#29221 is a vial of basilisk tears, while eoo_op2 defines both numbers as
black-pool portals. [specs.ailvio.c](../../../src/specs/specs.ailvio.c#L81)
uses `read_object(29220, VIRTUAL)` and
`read_object(29221, VIRTUAL)@ to supply those Newbie items.
Active [newbie.qst](../../../areas/qst/newbie.qst#L201) and
[newbie.story.json](../../../areas/story/newbie.story.json#L472) also use
the same item VNUMs. These references belong to Newbie’s salt-box and vial
identities; preserve them separately from the inactive portal prototypes.

Both eoo_op2 objects are type 25
([ITEM_TELEPORT](../../../src/core/defines.h#L140)). The stored
`value[0]` room VNUM is resolved through `real_room` ([lookup](../../../src/magic/spell_travel.c#L995));
`value[1]` selects the command and `value[2]@ stores charges
([spell_travel.c](../../../src/magic/spell_travel.c#L991),
[charge handling](../../../src/magic/spell_travel.c#L1173)).
Both store command 7, `CMD_ENTER` ([interp.h](../../../src/cmd/interp.h#L42)), and `value[2] = -1`; the
current code decrements only positive charge values.

| Source placement | Portal object and stored target | Paired return pool | Current active-table targets |
| --- | --- | --- | --- |
| EOO ZON places eoo_op #29211 in EOO #29202; #29211 targets #29222 | “Undead Continent” → eoo_op2 #29222, first Inner Sanctum | eoo_op2 #29220 is placed in #29222 and targets #29202 | #29222 is active Newbie “An Intersection of Walkways”; #29202 is active Newbie “Pathway of Cobbled Stones” |
| EOO ZON places eoo_op #29213 in EOO #29206; #29213 targets #29225 | “Goodie continent” → eoo_op2 #29225, second Inner Sanctum | eoo_op2 #29221 is placed in #29225 and targets #29206 | #29225 is active Newbie “A Turn in the Walkway”; #29206 is active Newbie “Inside The Shaman’s Guild” |

These are two reciprocal-looking numeric portal pairs between EOO and
eoo_op2; they are source facts, not proof of current player access or the
intended winning room records. EOO #29202/#29206 are themselves unregistered
rooms (“A Strange Pink Alcove” and “A Bright Hallway within Valhalla”).
Inactive dark_gh separately defines portals #29203 → #29222 and #29204 →
#29221 for its own same-number map. Keep those object and room identities
separate from the EOO/EOO_OP2 pairs.

The tracked type-25 scan found no active object whose stored target is
#29220–#29225. Ravenloft2 object #59182 is a text-only apparent hit: its
description and ZON comment mention #29224, but its stored target field is
#59224. It is not a stored portal target to this candidate under current
source data.

## Resets, history, and disposition

eoo_op2.zon places portal pool #29220 in room #29222, imports Player Castles
object #11051 into #29223, and places portal pool #29221 in #29225. It resets
all four candidate mobiles, #29220–#29223, once in each Niflheim entry room,
#29220 and #29223. Preserve the two pool placements and the external #11051
import recorded in Checkpoint 325
([reset](../../../areas/zon/eoo_op2.zon#L9),
[Player Castles import review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers)).

Followed WLD, MOB, and OBJ histories reach the 2009 area-tree move. The ZON
history reaches that move and a 2025 UTF-8 encoding conversion. No
alternate-stem predecessor or explicit whole-component replacement appears
in those paths. The title and source history do not identify an owner or
establish that another titled zone replaces this map.

Keep candidate row 39 **RETAIN IN PLACE**. Resolve the two repeated Hel
passages and reciprocal EOO portal pairs; the Duris3 boundary; all Newbie and
dark_gh same-number room/prototype identities; Newbie QST, story, and compiled
Ailvio consumers; the Player Castles import; source-manifest, editor, code,
deployment, and persisted-VNUM closure before any activation, move, rename,
archive, or merge proposal. See the
[24 builder questions](EOO_OP2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-476-eoo-op2-niflheim-map-and-reciprocal-portal-pairs),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-476-eoo-op2-map-identity-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-476-eoo-op2-retained-pending-portals-map-and-prototype-owners),
[candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39), and
[coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390). No area/source files,
tests, builds, runtime, server, database, or containers were changed or used.

## Checkpoint 888: EOO-OP2 typed-role boundary closure

This pass distinguishes EOO-OP2's room IDs #29220-#29225 from its mobile IDs #29220-#29223 and object IDs #29220-#29221. The scan uses the role fields resolved by [`renum_zone_table`](../../../src/world/db.c#L1612) and treats a VNUM's source owner as part of its identity.

The WLD result remains six rooms in two separate three-room components, nine directions, and no candidate-identity inbound exit. The 16 active directions that target #29220-#29225 remain Newbie-to-Newbie. External ZON room-role operands include eight source-only `dark_gh.zon` placements (four O and four M fields, landing in dark_gh #29220-#29222) and six active `newbie.zon` room operands (one D field at Newbie #29224 and five M placements in Newbie #29221/#29222/#29224). These are not EOO-OP2 approaches. The same Newbie ZON also resets Newbie mobiles #29220-#29223 in five M rows and Newbie objects #29220/#29221 in two O rows. EOO-OP2's two black-pool objects therefore remain distinct from Newbie's salt box and basilisk-tear vial; the EOO portal placements and targets remain as the two previously documented source-level pairs.

External OBJ type-25 targets into the EOO-OP2 room-number band are four source-owned portals: `eoo_op` #29211 -> #29222 and #29213 -> #29225 form the two documented reciprocal-looking EOO/EOO-OP2 pairs; inactive `dark_gh` #29203 -> #29222 and #29204 -> #29221 remain dark_gh source targets. No external type-29 target was found. These numeric destinations do not resolve the intended portal map owner or create a WLD edge ([eoo_op portal targets #29211/#29213](../../../areas/obj/eoo_op.obj#L15), [dark_gh portal targets #29203/#29204](../../../areas/obj/dark_gh.obj#L31)).

Role review also separates quest items from rooms. The numeric QST hits in `newbie.qst` are `I` item fields, and the story-sidecar matches are Newbie `item_vnums` / `I:` completion-key fields. #29220/#29221 are the candidate-object collision with Newbie's salt box and vial; other matches such as #29222-#29224 identify Newbie objects, not EOO-OP2 room destinations. Ailvio's only direct code matches are `read_object(29220)` and `read_object(29221)`, which supply the Newbie ingredients ([QST](../../../areas/qst/newbie.qst#L201), [story sidecar](../../../areas/story/newbie.story.json#L472), [compiled reads](../../../src/specs/specs.ailvio.c#L81)). No other direct C/C++ room consumer was found. No parser-valid N-format SHP keeper, room, or product field matched the candidate's room/mobile/object sets; see Checkpoint 886 for the global fallback's legacy-format limitation.

Keep candidate row 39 **RETAIN IN PLACE**. Preserve both Hel passage chains, the one-way #29223 -> Duris3 #151855 edge, the Player Castles #11051 import, the two EOO/EOO-OP2 portal pairs, the dark_gh portal pair, and Newbie's QST/story/code object consumers under their own identities. Canon, map ownership, editor/tooling, deployment, and persisted-state decisions remain open; no WLD count or approach total changes. See the [24 builder questions](EOO_OP2_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-888-eoo-op2-typed-boundaries), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-888-eoo-op2-retains-role-separated-map-and-portal-identities), [candidate row 39](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L39), and [coverage row 389](ZONE_COVERAGE_REGISTER.csv#L390). No source, runtime, tests, builds, server, or database was changed or used.
