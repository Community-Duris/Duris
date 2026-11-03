# Tempest Court (`airp`) audit reconciliation

Reviewed October 12, 2026 against the current checkout and the source snapshot
recorded in [coverage row 2](ZONE_COVERAGE_REGISTER.csv#L3). The
[Tempest Court source dossier](../zone-stories/TEMPEST_COURT.md) is the primary
story inventory; this supplement closes the remaining identity, connectivity,
replacement-history, typed-dependency, and builder-question reviews. It is a
source audit, not live gameplay or runtime qualification.

## Identity and disposition

The current contributor is `areas/airp.*`, registered in the full world as
`*1316-1317` and in `areas/qst/AREA` as `airp *254-255`. Its color-stripped,
case- and whitespace-normalized ZON title is **The Tempest Court**. That title
is unique among all 442 current ZON titles. It is a distinct zone identity from
`plane_air_one` / **Plane of Air**; the two meet through one reciprocal WLD
boundary. The current source-history paths for WLD, MOB, OBJ, ZON, QST, and the
story sidecar remain under the `airp` stem. No tracked alternate-stem rename,
same-title active duplicate, or whole-zone predecessor/replacement was
established. The historical commit-message phrase “AP Tempest Courts” does not
by itself establish an alias or a second area identity.

Keep the current full area **DO NOT MOVE**. It is an active registered area,
not an attic candidate. Preserve its VNUM ownership, the boundary with Plane
of Air, and the source-backed story/prototype identities. No zone, quest,
reset, route, combat, or accounting change is proposed here.

## Story and environmental-story scope

The source-comprehensive [story dossier](../zone-stories/TEMPEST_COURT.md)
already inventories 26 native Q/M/MA blocks, 200 rooms, 52 mobiles, 63
objects, 307 resets, exact dialogue families, reset ordering, movement and
portal behavior, and the assigned wind-dagger procedure. It reconciles eight
native contracts as seven named outcomes, preserving Al'Hajib's two giver
identities as alternatives for one outcome. It separately distinguishes
accepted deliveries from access, source history, actor departure, combat,
visibility, divine effects, and a wider planar campaign. The room and actor
prose is environmental/story evidence; it does not create missing native
travel or persistent state transitions.

The dossier's exact foreign-proof table supplies the typed quest edges:

- Cloudseeker accepts locally made fragment #131647 and cloak #131617 plus
  seven foreign proof kinds: wisp #55553, living breeze #8735, four-winds
  boots #70976, imperceptible cloak #88304, ethereal-winds shroud #97066,
  North Wind katana #138268, and ageless-wind cloak #138515.
- Fearfrost's hammer accepts the locally made fragment plus Jotunheim proofs
  Frostbite #96000, Mistweave #96012, and allegiance token #96055.
- The dossier traces the declared producers and competing foreign consumers
  of the shared wisp and katana. It finds no foreign Q consumer for the local
  smoke key, medallion, essences, fragment, eyepiece, or named exchange
  rewards in its bounded active scan. Supplied matching proofs remain valid;
  the audit does not add personal-source prerequisites.
- Code and object routes remain separate typed dependencies. The ordinary
  Airp–Plane of Air boundary is an authored WLD edge. The eight internal
  portals, the inactive-command Troll Hills portal prototype, the air-plane
  movement restrictions, and other code-driven planar routes are described in
  the source dossier with their current qualifications and limits.

This closes the static typed-dependency review for the current area. It does
not establish live item admission, replenishment, reset-generation ownership,
safe travel, or successful foreign journeys.

## World graph and reachability boundary

The current 200-room WLD has 1,074 source exit records, all 1,074 resolving in
the audited loader census. Its local weak graph has eight components; the
largest contains 121 rooms. There is one active inbound and one active
outbound cross-area edge, and they are reciprocal:

| Source | Direction and target | Reciprocal edge |
| --- | --- | --- |
| `plane_air_one.wld` #24422 | Up to `airp.wld` #131600 | `airp.wld` #131600 Down to #24422 |

The exact records are [outbound pair row 4](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L4)
and [inbound pair row 348](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L348), with the
individual records in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L396)
and [its reciprocal record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1300).
The static census records no negative or unresolved positive targets, numeric
range mismatches, unregistered WLD target, fall-chance room, or current room.
The [Plane of Air audit](PLANE_OF_AIR_AUDIT_REVIEW.md) independently records
the same reciprocal boundary and distinguishes it from its dynamic spell
arrival paths.

The resolved-exit and connectivity counts describe authored source data. They
do not prove ordinary player access to every component, door state after a
reset, flight or mount eligibility, actual portal dispatch, safe return, NPC
availability, or repeatable/daily suitability. Those limits remain explicit
in the [source dossier](../zone-stories/TEMPEST_COURT.md) and the builder
questions below.

## Builder questions and conservative recommendations

The questions identify decisions that source inspection cannot answer. They
do not authorize an implementation or imply that an owner has answered them.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| TEMP-01 | Is `airp` meant to have any command-target alias in addition to its unique display title, **The Tempest Court**? | Keep `airp`, The Tempest Court, and `plane_air_one` / Plane of Air as separate identities. Add an alias only if its exact target and intended distinction are confirmed. |
| TEMP-02 | Is Al'Hajib's promised eyepiece upgrade meant to be an active recipe using prototypes #131659–#131662? | Keep the established medallion-to-eyepiece exchange as the only proven recipe. Before adding an upgrade, specify accepted gems, exact transformations, sources, balance, and the credited action. |
| TEMP-03 | Should cloud portal object #131641 actually transport a player to Troll Hills #1823? | Its command and charges are both zero. Keep it unavailable as an ordinary ENTER route until the intended command, charge policy, arrival, and return are specified and qualified. |
| TEMP-04 | Should the eight cloud lightning generators trigger when a player moves through them? | They lack the MOVE1 trigger required by the shared movement-trap dispatch. Preserve current behavior until the builder chooses the hazard, damage, audience, accounting/loss policy, and balance. |
| TEMP-05 | Which sealed or empty rooms and cells are intended staging, and which promise an actual route or captive? | Decide the intended access for rooms #131797–#131799, the key-zero southern cell, and the described but absent Chamber of Choices east route. Add only the selected route, actor, or transition; keep prose from implying an unimplemented path. |
| TEMP-06 | Which events, if any, should complete the wider Akadi/Yan-C-Bin planar campaign? | Define selected endpoints, actors, episode, participants, retries, and durable effects before crediting liberation, restoration, dispersal, or a divine transition. Keep the existing epic-touch result and payout as the authoritative event where applicable. |
| TEMP-07 | Should `dagger_of_wind` keep its current deflection behavior around combat stop, actor movement, and room iteration? | Preserve the callback pending targeted lifecycle qualification. The source review found a concrete iteration/lifetime question but no reproduced live crash; do not add a story event to the combat effect. |
| TEMP-08 | Which of the five repeat-shaped outcomes, if any, are suitable for daily assignment with current access and supplies? | Keep daily readiness separate from native recipe shape. Qualify actual actor/item admission, replenishment after consumed proof, and safe access first; retain the two disappearing-recipient outcomes as story-only unless their replay model changes deliberately. |

These eight questions augment the detailed source findings in the main
[Tempest Court dossier](../zone-stories/TEMPEST_COURT.md). Existing route,
reset, story-identity, and source-provenance questions remain cross-linked
there rather than duplicated as separate fixes.
