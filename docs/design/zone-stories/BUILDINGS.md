# Buildings: generated outpost interiors and shared prototypes

## Source identity and authored content

`buildings` is an active full-world contributor, titled “Buildings,” at zone
978. It is listed in both the main world manifest and the separate quest
index ([`areas/AREA`](../../../areas/AREA#L352),
[`areas/qst/AREA`](../../../areas/qst/AREA#L311)). Quest-index membership
does not mean a local quest file exists.

The WLD file defines 100 rooms, #97800–#97899. Every room has the same name,
“Inside a building,” an empty description, the same room fields, and no authored
exits ([first room](../../../areas/wld/buildings.wld#L1),
[#97899](../../../areas/wld/buildings.wld#L496)). These are reusable room
slots, not 100 distinct interior scenes.

The source has two mobile prototypes and four object prototypes. The mobiles
are an outpost tower and a gate guard ([tower](../../../areas/mob/buildings.mob#L1),
[guard](../../../areas/mob/buildings.mob#L12)). The objects are rubble,
a portal-like egress, a copse of trees, and a timber stand
([rubble](../../../areas/obj/buildings.obj#L1),
[egress](../../../areas/obj/buildings.obj#L9),
[copse](../../../areas/obj/buildings.obj#L17),
[timber stand](../../../areas/obj/buildings.obj#L25)). The ZON file has no
reset commands, so these prototypes are not placed by a local reset
([`buildings.zon`](../../../areas/zon/buildings.zon#L1)). There is no
same-stem QST, native Q/QA contract, shop file, or story sidecar. The current
coverage row records those absences at
[row 44](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L44).

## Story and generated behavior

The authored text supplies a small construction-and-destruction motif: an
outpost tower, its guard, a rubble pile that marks a fallen construct, and a
vortex object whose description is filled with a destination name. It does
not describe a town, a quest chain, or a set of individually authored rooms.
The `outpost` command and the outpost module provide guild ownership, entry,
repair, defense, resource, and optional portal behavior in C++.

The distinction matters because the disk rooms are runtime templates. In the
normal non-mini startup path, the server calls `init_outposts()`, which reads
the outpost records and attempts to instantiate the configured outposts
([startup](../../../src/net/comm.c#L1003),
[loader](../../../src/world/outposts.c#L260)). The building generator takes a
free room from #97800–#97899, changes its displayed name to “Inside an Outpost
Tower,” and adds a down exit to that outpost’s exterior location
([room allocation](../../../src/world/buildings.c#L41),
[generator](../../../src/world/buildings.c#L567)). The outpost mobile’s enter
handler moves an authorized player into that generated room
([enter handling](../../../src/world/buildings.c#L295)). This is source-backed
code behavior; this dossier does not qualify live access or current persisted
outpost state.

## Builder questions and recommended resolutions

- **Are the 100 identical rooms meant to be separate interiors?** The runtime
  allocator treats the whole range as reusable slots and currently generates
  one interior for each loaded outpost. **Recommendation:** keep the generic
  room shells and preserve every VNUM in the reserved range. Do not assign
  distinct fixed-room lore or exits to them without changing and qualifying
  the allocator.
- **Should `Buildings` be reviewed as a conventional adventure zone?** Its WLD
  text is generic, its reset file is empty, and it has no local quest source;
  the meaningful building behavior is in the outpost service code.
  **Recommendation:** classify it as shared world infrastructure with a
  concise environmental note, and place any future settlement story on the
  area that owns that location rather than on a reusable interior slot.
- **Are the tower and guard meant to spawn from ZON resets?** They have no
  local reset placements; C++ reads and binds these prototypes directly.
  **Recommendation:** retain the code-owned spawn path and do not add
  duplicate reset placements. If the outpost feature is retired, reconcile
  its code and saved records before considering these prototypes unused.
- **Are the copse and timber-stand objects active resources?** Their VNUMs
  remain declared in the outpost header, but current source references stop
  at those declarations and the resource initializer call is commented out.
  **Recommendation:** keep both prototypes pending an owner review of the
  unfinished resource path; do not describe them as currently harvestable.
- **Is the outpost feature actually in service?** An old header comment says
  buildings are “not attached,” while the current non-mini boot path calls
  `init_outposts()` and the command table registers `outpost`.
  **Recommendation:** use the current call path as the source-of-truth for
  migration safety. Ask the feature owner to reconcile persisted state before
  any retirement or removal proposal; this audit does not inspect that state.
- **Should the portal object be treated as an ordinary static exit?** Its
  destination is assigned at runtime and the travel code identifies it by
  prototype VNUM. **Recommendation:** retain the portal prototype and keep
  portal routes separate from the static WLD graph until their generated
  destinations and live permissions are qualified.
- **Should `buildings` have a command-target alias beyond “Buildings”?** The
  normalized title has one owner among the active contributors; Human Outpost,
  Undead Outpost, and Newbie Outpost are separately named areas. Keep the source
  key and title, and add an alias only after the owner names its exact target
  and intended distinction.

Keep the active `buildings` source **DO NOT MOVE**. The
[repository-wide audit supplement](../quest-story-followups/BUILDINGS_AUDIT_REVIEW.md)
records the static graph, compiled consumers, dynamic map links, history, and
bounded dependency scans.
