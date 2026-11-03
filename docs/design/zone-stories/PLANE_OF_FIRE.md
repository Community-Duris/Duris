# Plane of Fire

Reviewed October 10, 2026 for the active `area:plane_fire_one` contributor,
zone 254, VNUMs #25401–#25525. Its source files are
[`plane_fire_one.wld`](../../../areas/wld/plane_fire_one.wld),
[`plane_fire_one.mob`](../../../areas/mob/plane_fire_one.mob),
[`plane_fire_one.obj`](../../../areas/obj/plane_fire_one.obj),
[`plane_fire_one.zon`](../../../areas/zon/plane_fire_one.zon), and
[`plane_fire_one.qst`](../../../areas/qst/plane_fire_one.qst). The area has
125 room records, eight mobile prototypes, and seven object prototypes. It
has no story sidecar or local player Q/QA contract; its short QST file is a
Brass gate hook described below.

This active contributor is the current 254-range Plane of Fire. The separate
[Plane of Fire, Brass](PLANE_OF_FIRE_BRASS.md) dossier covers the City of
Brass and its shared gate story; its WLD is not this area's map.

## Environmental story in the rooms

The map is almost entirely one environment: 123 rooms repeat a description of
fire, flames, molten rock, and elemental gravity. Two rooms stand apart:

- Room #25406 is an edge around a cooler silver sphere of air and describes
  itself as part of the Plane of Air.
- Room #25433, “A Jet of Multi-Colored Flame,” describes chromatic flame and
  popping rocks. Negative Plane object #26659 also names this room as its
  destination.

The mobile catalog defines iron golem gate guardians, fire elementals, large
fire elementals, efreeti scouts, flame salamanders, Imix, Prince of Fire, and
magmen. Fire's reset file distributes elemental inhabitants around the map;
the guardian prototypes also serve in the City of Brass. These mobile
descriptions establish the setting and cast, not a player objective or proof
that an encounter is available.

## Residents, resets, and objects

The local mobile prototypes are #25400 and #25401 (iron golem gate guardians),
#25410 (fire elemental), #25415 (large fire elemental), #25420 (efreeti
scout), #25430 (flame salamander), #25440 (Imix), and #25450 (magman). The
zone file resets the elemental residents around the map and loads Imix in
#25450. It equips Imix with local #25403 (cloak of eternal fire), #25405
(white hot mask), and #25402 (flaming earring), plus imported objects #67210,
#55199, #359, and #14121. These are reset definitions, not a live inventory
or proof of how a player obtains the items.

Local objects #25400 and #25401 are a flaming potion and a grey potion of
stone skin. The remaining objects are the flaming earring, cloak, damaging
grey color pool, white hot mask, and smoke-and-ash portal. The pool is reset
in room #25422 and the portal in #25464. Their code routes are described in
the [audit review](../quest-story-followups/PLANE_OF_FIRE_AUDIT_REVIEW.md#code-and-object-routes).

## The QST hook and the City of Brass

The local [QST file](../../../areas/qst/plane_fire_one.qst#L1) contains only
giver #25401 and the message command `qc_unblock 139000 north~`. Compiled
[mobile assignment](../../../src/specs/specs.assign.c#L1254) binds #25401 to
`unblock_on_death`; the [handler](../../../src/specs/specs.exit_barriers.c#L93)
reads this QST command when the mobile dies and clears the blocked flag on the
named exit. Active Brass resets both
guardian mobiles #25400 and #25401 at its room #139000, and the Brass story
sidecar records them as gate contacts. This is a shared Brass gate mechanic,
not a local Fire quest receipt; it does not define a player Q/QA objective.

The active Heavens QST separately requires Fire objects #25402 and #25403.
Its nearby prose first asks for wearable items of fire and mentions Imix, then
describes turning over Imix's remains and moves on to Demogorgon's helmet
before the fixed #25402/#25403 requirement lines. Keep this owner and text
relationship as a builder question; the source does not settle the intended
stage ordering or live acquisition.

## Title, history, and retention

After color-code removal and whitespace normalization, “Plane of Fire” is
unique among all 442 current `.zon` titles and among the 351 active area
contributors. The current files use the `plane_fire_one` stem and range
#25401–#25525. Historical comparison shows that this content came from the
former `plane_fire_three` contributor: the current WLD, OBJ, ZON, and QST
contents match those pre-cleanup files, while six of the eight current mobile
records match and the two guardian records were edited. The older
250-range `plane_fire_one` map and its associated prototypes are no longer
the active contributor. See the [followed history](../quest-story-followups/DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-323-plane-of-fire-title-and-followed-history)
and [audit review](../quest-story-followups/PLANE_OF_FIRE_AUDIT_REVIEW.md#title-followed-history-and-retention).

Keep active `plane_fire_one` **DO NOT MOVE**. Preserve its current VNUM
ownership, the separate City of Brass map, both object-portal directions, the
Ethereal pool destination and damage field, the Heavens item identities, and
the gate-guardian references. No route, reset, story, or migration change is
proposed.
