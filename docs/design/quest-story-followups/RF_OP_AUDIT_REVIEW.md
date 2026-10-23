# rf_op source-component audit

Canonical key: component:rf_op. Physical coverage row 440 / CSV line 441;
candidate dependency data row 89 / CSV line 90. Reviewed October 11, 2026,
Checkpoint 527, against checkout c500ac51b8cd26540943257e7ca4e9807263ef74.
This is a static source, manifest, graph, portal, and identity review. It
does not establish booted world tables, player travel, or editor ownership.

## Inventory, title, and manifest status

The source set is [rf_op.wld](../../../areas/wld/rf_op.wld#L1) with three
rooms #29510–#29512, [rf_op.mob](../../../areas/mob/rf_op.mob#L1) with two
mobile prototypes #29510/#29511, [rf_op.obj](../../../areas/obj/rf_op.obj#L1)
with two portal objects #29510/#29511, and
[rf_op.zon](../../../areas/zon/rf_op.zon#L1) with one zone header and five
reset commands. The raw title is “The Tharnadian Outpost of Rotting Flesh”;
the zone header ends at #29512 and uses reset mode 2. There is no same-stem
QST, SHP, or story sidecar.

rf_op is absent from areas/AREA, areas/qst/AREA, and the empty current
areas/RANDOM_AREA. The active [Harrow manifest row](../../../areas/AREA#L142)
claims the broad *294-296 band, but no current WLD source in that manifest
defines rooms #29505 or #29510–#29512. The similarly numbered WLD files
aod_gh, genocide_hall, and legion are also absent from the full manifest.
The expected generated areas/world.wld, world.mob, world.obj, and world.zon
files are absent from this checkout.

## Map topology and outlets

rf_op has five authored WLD exits. Four target its own room numbers and form
one weak component of all three rooms. The fifth is a one-way east exit to
#154153.

| Source room | Authored exits |
| --- | --- |
| #29510, desecrated-cathedral approach | D0 → #29511; D4 → #154153 |
| #29511, cathedral | D2 → #29510; D5 → #29512 |
| #29512, altar | D4 → #29511 |

#154153 is “The Ocean” only in unlisted Duris3.wld. No reverse exit to
#29510–#29512 was found. None of the five positive WLD targets resolves to a
room identity in the current areas/AREA table: the four local-number targets
have no listed WLD definitions, and #154153 is in unlisted Duris3. There are
no active inbound exits to an rf_op room. The same numbers occur as different
rooms in other unlisted candidate WLD files; those source-only references are
not active approaches to rf_op. The row records five unresolved positive
target numbers, zero loader-resolved targets, one weak component, and no
negative, fall-chance, or current records.

## Reused cathedral fragment and VNUM collisions

rf_op's three rooms reproduce the closing cathedral fragment of rf_gh at
different local VNUMs. After translating #28910–#28912 to #29510–#29512,
the second and third WLD records match exactly; the first differs by one
space in its description. The associated MOB #28905/#28906 records match
rf_op MOB #29510/#29511 exactly after the same header renumbering. The two
portal objects have the same names, text, and statistics as rf_gh OBJ
#28903/#28904, but their destination values changed. The reset sources also
place three of the first angel and one of the second in the corresponding
gate room. This is strong reused-fragment evidence, not proof that either
whole component replaces the other.

Same-type WLD room numbers #29510–#29512 also have different prototypes in
three unlisted files:

| VNUM | aod_gh.wld | genocide_hall.wld | legion.wld |
| --- | --- | --- | --- |
| #29510 | guarded jungle-galley entrance | blood-strewn passageway | below the tundra |
| #29511 | deeper jungle canopy | grand marble chamber | icy hallway |
| #29512 | immense hollowed tree | no #29512 room | room cloaked in mist |

Those are unlisted map identities, not active same-type definitions. rf_op's
MOB and OBJ IDs have no outside same-type header matches in the current
areas tree. No mini-area exact VNUM match was found. Followed-path history
shows only the 2009 area-tree move in this checkout; there is no explicit
predecessor, successor, or retirement decision.

## Reset and teleport dependencies

The five ZON commands are all source-local: one O reset places OBJ #29510 in
room #29512; three M resets place MOB #29510 in #29510; and one M reset
places MOB #29511 there. Same-number WLD exits and ZON room fields also
occur in unlisted aod_gh, genocide_hall, and legion; these belong to those
separate source maps and do not establish active routes to rf_op. No outside
QST, SHP, or C/C++ reference was found in that scan.

Both OBJ records have item type 25 (ITEM_TELEPORT). OBJ #29510 has
value[0] #29505; OBJ #29511 has value[0] #29512. The portal handler uses
value[0] as a room VNUM and passes it to real_room
([spell_travel.c](../../../src/magic/spell_travel.c#L991);
[ITEM_TELEPORT](../../../src/core/defines.h#L140)). The first target #29505
has two different unlisted room definitions: “The Inn of Nightmares” in
aod_gh.wld and a torchlit castle walkway in legion.wld. Neither source is in
areas/AREA, and no current listed WLD defines #29505. The second portal's
#29512 destination is present only in unlisted WLD sources too; its OBJ
record is not placed by rf_op.zon. These are source-code route values, not
WLD direction records, so they are tracked separately from the five
unresolved WLD exits.

The word “Father Benali” appears in both rf_op angel descriptions and as
active Surface MOB #500209. Surface's story catalog describes that mobile
as a class teacher with no topics; it does not identify the cathedral,
provide a portal destination, or prove these descriptions refer to one
in-world person.

## Disposition and limits

Keep all four rf_op files RETAIN IN PLACE. Resolve the copied rf_gh fragment,
the #29510–#29512 same-type room owners, the two portal destinations,
#154153's one-way Duris3 outlet, and the Father Benali story owner before any
separate activation, route change, rename, move, merge, or archive proposal.
No migration or activation clearance is implied. See the
[21 builder questions](RF_OP_BUILDER_QUESTIONS.md),
[rf_gh cross-audit](RF_GH_AUDIT_REVIEW.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md), and
[retention checkpoint](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 931: same-number records keep separate typed roles

Within `rf_op`, #29510 has separate WLD, MOB, and OBJ prototypes, #29511 has
separate WLD, MOB, and OBJ prototypes, and #29512 has a WLD room only
([WLD](../../../areas/wld/rf_op.wld#L1), [MOB](../../../areas/mob/rf_op.mob#L1),
[OBJ](../../../areas/obj/rf_op.obj#L1), [second room](../../../areas/wld/rf_op.wld#L30),
[second mobile](../../../areas/mob/rf_op.mob#L22), [second object](../../../areas/obj/rf_op.obj#L20),
[altar room](../../../areas/wld/rf_op.wld#L53)). The type-25 OBJ #29510 targets
#29505 and ZON places that object in room #29512; type-25 OBJ #29511 targets
#29512 but has no ZON object reset ([OBJ values](../../../areas/obj/rf_op.obj#L6),
[destination rows](../../../areas/obj/rf_op.obj#L7), [ZON placement](../../../areas/zon/rf_op.zon#L9)).
The four M resets place the two mobile prototypes in room #29510
([reset rows](../../../areas/zon/rf_op.zon#L11)). These source fields have
separate WLD, MOB, OBJ, reset-prototype, reset-room, and teleport-destination
roles; equal numeric values do not collapse them into one identity.

A fresh exact-value scan outside `rf_op.*` found different WLD prototypes
for #29510–#29512 only in unlisted `aod_gh`, `genocide_hall`, and `legion`
([aod_gh](../../../areas/wld/aod_gh.wld#L206),
[genocide_hall](../../../areas/wld/genocide_hall.wld#L324),
[legion](../../../areas/wld/legion.wld#L193)); no other MOB or OBJ prototype
headers, QST, SHP, story-sidecar, or C/C++ references to these numbers were
found. External WLD exit operands and ZON room fields with the same numbers
occur in those unlisted maps as source-local map/reset data
([Legion ZON](../../../areas/zon/legion.zon#L16),
[genocide_hall ZON](../../../areas/zon/genocide_hall.zon#L11)). The scan also
found Legion OBJ #29500's type-25 `value[0]` destination
#29512 ([Legion OBJ](../../../areas/obj/legion.obj#L6),
[destination row](../../../areas/obj/legion.obj#L7)). That reference belongs
to unlisted Legion source, which defines its own different WLD #29512 room
([Legion room](../../../areas/wld/legion.wld#L229)); it is not an active
inbound WLD edge or proof of a route to `rf_op`.

The same-type WLD candidates remain absent from the current active manifest;
the five `rf_op` WLD exits still have zero loader-resolved targets and zero
active inbound exits. No graph metric or replacement conclusion changes.
Keep the four files RETAIN IN PLACE and confirm the typed table inputs,
teleport destination ownership, and reset placement with the map and export
owners before activation. Question 21 in
[RF_OP_BUILDER_QUESTIONS.md](RF_OP_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions)
records the Legion #29512 source-reference distinction.
