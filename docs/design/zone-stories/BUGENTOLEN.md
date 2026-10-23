# Bugentolen: captive labor, ritual, and unbound town stories

Status: source-comprehensive static review of active area Bugentol. The local
source set contains 125 rooms, 27 mobile prototypes, 18 object prototypes, and
329 reset commands. It has no local QST or SHP file and no story sidecar. This
review maps authored text and source records; it does not qualify live access,
population, NPC behavior, or quest completion.

## Native story boundary

Bugentolen is authored as a poor, crowded bugbear settlement, with a dirty
Booger Street and a cleaner, busier Boggler Street. The village contains an
elder who answers questions, the shaman Boongar and his wife Bonga, a butcher,
a carpenter, and a cloaked stranger performing a ritual. Those identities,
scenes, and room descriptions are source evidence. There is no local QST
contract, shop file, story sidecar, or exact room/mobile/object-number binding
in the scanned C++ source that turns these scenes into a native objective or
persistent outcome. A price sign, appointment scribble, work order, claim of
healing, or invitation to ask a resident does not by itself establish a
transaction or quest.
[Village rooms](../../../areas/wld/bugentol.wld#L1) ·
[elder Boular](../../../areas/mob/bugentol.mob#L235) ·
[Boongar](../../../areas/mob/bugentol.mob#L250) ·
[the cloaked stranger](../../../areas/mob/bugentol.mob#L349)

## Environmental stories and source placements

| Story area | Authored scene and reset evidence |
| --- | --- |
| Streets and services | Booger Street is dirty and guano-covered; Boggler Street is cleaner and more populated. The butcher's room advertises meat prices and has a cutting board, while a tavern hosts drunk bugbears. The carpenter's room has an order pad and chalk. These are not local shop or order contracts. [butcher and services](../../../areas/wld/bugentol.wld#L552), [carpenter](../../../areas/wld/bugentol.wld#L574) |
| Children and work | Two children's spaces describe play and occasional falls. The farming fields describe youths forced to work, boars replacing oxen, creek irrigation, a scarecrow, a sign-up sheet, and a sundial. Youth prototypes load, but the local source has no boar prototype or reset. The stable prose describes diseased work animals; its direct local resets load people rather than an animal. [children's center](../../../areas/wld/bugentol.wld#L251), [stable](../../../areas/wld/bugentol.wld#L651), [fields](../../../areas/wld/bugentol.wld#L681), [second play center](../../../areas/wld/bugentol.wld#L2459) |
| Leadership and ritual | Boular's clean, model-filled hut and Boongar's bone-decorated shaman rooms contrast with the squalid streets. A room says Boongar can heal or help, but no local contract, special binding, or shop record supplies that service. The temple includes Orcus shrines, shaman training, an altar, elf skin, and prose about a top flap opened at night; there is no corresponding top exit or timed door reset. The shaman-office prose mentions a bloodied grate, but that room has no downward WLD exit. [elder hut](../../../areas/wld/bugentol.wld#L411), [Boongar's hut](../../../areas/wld/bugentol.wld#L436), [shaman rooms](../../../areas/wld/bugentol.wld#L509) |
| Town prison | A separate prison wing describes barred cells, solitary confinement, and a torture chamber. Its local resets place guards and elites, not captive prisoners in these rooms. Prose about bars is not itself a WLD door record. [prison wing](../../../areas/wld/bugentol.wld#L1120) |
| Slave Pens | The separate Slave Pens contain shackles and solitary rooms. Twelve direct reset rows place dwarf slave mobile 95816 there, in rooms including 95887, 95890, 95892, 95894, 95896, 95899, 95900, and 95914. No local QST or other selected record defines rescue, release, escort, safe destination, or a persistent result. [Slave Pens](../../../areas/wld/bugentol.wld#L1816), [slave resets](../../../areas/zon/bugentol.zon#L273) |
| Private garden and communal house | A thorny, vine-filled private courtyard has a gardener, warning sign, fountain, statue, and benches. Six nearby cell rooms have no direct local mobile reset. The vine-covered feeding-place house looks wealthy outside but describes a splattered communal food hall, storage, and toilet inside; no room-object reset places an object there. [garden](../../../areas/wld/bugentol.wld#L1404), [feeding-place house](../../../areas/wld/bugentol.wld#L1709) |
| Barracks and training ground | The barracks and training field load warriors, guards, and equipment. The local reset file outfits characters but does not define a training reward or progression contract. [barracks](../../../areas/wld/bugentol.wld#L2196), [training ground](../../../areas/wld/bugentol.wld#L2211) |

The 27 local mobile prototypes range from guards, children, youths, cleaners,
gardeners, nobles, drunks, and warriors to the dwarf slave, Boular, Boongar,
Bonga, a constable, shaman students and teacher, butcher, carpenter, and ritual
stranger. A small bat, small snake, and thick vine are also defined. These
prototypes and their direct reset rows establish authored cast and placement,
not a simulated town census or an unbound story outcome.
[Mobile definitions](../../../areas/mob/bugentol.mob#L1) ·
[elder and shaman definitions](../../../areas/mob/bugentol.mob#L235) ·
[slave prototype](../../../areas/mob/bugentol.mob#L221) ·
[ritualist](../../../areas/mob/bugentol.mob#L349) ·
[slave reset rows](../../../areas/zon/bugentol.zon#L273)

The 18 objects, from a bone leg and dwarf-hide garments to a blood-spiked mace,
sewn-flesh mask, battleaxe, blue hood, ragged-flesh belt, and studded black
boots, are equipment or carried/held stock. The reset file has 109 equipment
rows and one give row, with no room-object or put-object resets. These items
are not local QST inputs in the reviewed source.
[Object prototypes](../../../areas/obj/bugentol.obj#L1) ·
[reset file](../../../areas/zon/bugentol.zon#L1)

## Authored access and reset state

The WLD contains 253 exit records, all of whose targets resolve in the static
source set, and forms one weak component when all authored directions are
counted. Only two local door pairs are encoded. Each endpoint has a matching
zone reset at state 1 (closed); neither pair is reset locked:

| Pair | WLD description and reset |
| --- | --- |
| 95826 west ↔ 95827 east | Keyword curtain; key VNUM 0; both endpoints reset closed. |
| 95885 west ↔ 95886 east | Keyword door; key VNUM 0; both endpoints reset closed. |

The reset-state interpretation comes from the shared [door-state handler](../../../src/world/db.c#L7857); it does not establish that a reset has run or that either door is currently closed. As a static graph inference only, removing these reset-closed pairs separates the main settlement (110 rooms), the Slave Pens segment (14 rooms), and Boongar's bedroom (one room). The main gate's armor-sheeted wooden-gate prose does not match a door-coded/reset WLD exit. None of these source facts proves live route access.
[Door reset rows](../../../areas/zon/bugentol.zon#L8) ·
[Bugentolen WLD](../../../areas/wld/bugentol.wld#L1)

Bugentol has 253 authored WLD exit records, 253 statically resolving records,
one local weak component, and no fall-chance or current fields. Its active
boundary consists of a paired east exit from 95800 to Surface 589071 and one
unpaired inbound record from Wh 55402 east to 95813. The Wh source labels 55402
as a no-teleport room (ROOM_NO_TELEPORT in the [flag definitions](../../../src/core/defines.h#L409));
that flag and exit record are not a qualification of ordinary travel. No
reverse WLD record points from 95813 back to Wh.
[Bugentol gate](../../../areas/wld/bugentol.wld#L15) ·
[paired Surface return](../../../areas/wld/surface.wld#L1869864) ·
[Wh inbound record](../../../areas/wld/wh.wld#L8982) ·
[active cross-exit ledger](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1150)

A separate source-only record in unregistered surf.wld room 280727 points west
to active Bugentol room 95800. It is excluded from the active WLD census and
is not active-world ingress.
[source-only surf room](../../../areas/wld/surf.wld#L1693959) ·
[connectivity follow-up register](../quest-story-followups/WORLD_CONNECTIVITY.md)

## Boundary history and duplicate/replacement limits

The normalized active title Bugentolen is unique. Tracked WLD history shows
commit 28c8b2ab5 (2011-05-12) changing room 95800's east target from active
Surface room 589071 to 259558; commit 71d731c5b (2013-11-13) changes it back
to 589071. The old number 259558 has divergent descriptions in unregistered
surf.wld and surface2011.wld. The source-only surf.wld room 280727 points into
Bugentol, while the same-numbered surface2011.wld record describes stormy sea
and points west to 280726. This is parallel map material and a boundary edit,
not evidence that Bugentol replaced or was replaced by another active zone.
See [duplicate/replacement history](../quest-story-followups/DUPLICATE_AND_REPLACED_ZONES.md).

## Bounded typed closure

The local source inventory is WLD, MOB, OBJ, and ZON only. Exact scans found no
external ZON reset references to Bugentol's local room/mobile/object ranges, no
exact local-ID hits in external QST, SHP, or story-sidecar files, and no literal
room/prototype number binding in the scanned C++ files. A prototype-definition
inventory found no external MOB/OBJ prototypes duplicating local VNUMs;
dedicated typed scans found no external type-25 portal or type-29 switch target
in Bugentol. These are bounded source searches, not proof against indirect
references, editor or builder data, persisted player identities, generated
content, or dynamic routes. The unregistered Wh boundary and `surf.wld` record
are recorded above; source-only WLD files were screened across all 95
unregistered WLD inputs.

## Builder questions and recommended resolutions

- **Is the dwarf labor scene intended to support a rescue story?** Twelve
  resets place dwarf slave #95816 in the Slave Pens, but no local quest or
  other selected source defines a rescue action or outcome
  ([slave resets](../../../areas/zon/bugentol.zon#L273)). **Recommendation:**
  keep the scene as authored context until the area owner decides whether a
  rescue is intended. If it is, document eligibility, release authority and
  action, safe destination, persistence, death and partial-completion rules,
  and replay before designing any mechanic.
- **Should the town prison or garden cells contain residents?** Room text
  describes cells, solitary confinement, a torture room, and six courtyard
  cells; the matching local resets do not place prisoners in those rooms
  ([town prison](../../../areas/wld/bugentol.wld#L1120),
  [garden](../../../areas/wld/bugentol.wld#L1404)). **Recommendation:**
  decide whether these spaces are intentionally empty. If so, make the prose
  describe their unused or former purpose; if not, assign an explicit source
  role before adding placement.
- **How should the farm's youth and animals be represented?** The fields
  describe youths being forced to work, boars replacing oxen, and diseased
  animals, but there is no local boar reset and the stable loads people
  ([fields](../../../areas/wld/bugentol.wld#L681),
  [stable](../../../areas/wld/bugentol.wld#L651)). **Recommendation:**
  reconcile prose with the intended local population; do not treat the sign-up
  sheet or animal references as working systems without source support.
- **Are healing, work orders, and meat prices real services?** The shaman's
  rooms claim healing or help, while the butcher and carpenter scenes include
  price or order props; there is no local QST, SHP, or exact-number code
  binding for these promises. **Recommendation:** label them as atmosphere if
  they are noninteractive. If an owner wants an active service, specify its
  costs, eligibility, result, and persistence separately before implementing
  it.
- **Do the ritual and prison descriptions imply routes or access that should
  exist?** The temple mentions a top flap opened at night and a bloodied grate,
  but has no top exit or downward exit; the entrance gate has no door reset.
  The two internal door pairs are both reset closed. **Recommendation:**
  confirm which passages are meant to be navigable, then align room clues,
  exit data, and reset state with that decision. Do not infer a route from
  prose alone.
- **What do the legacy Surface records mean?** The active east exit from
  #95800 changed from Surface #589071 to #259558 in 2011 and back in 2013;
  inactive `surf.wld` also has one approach to #95800. **Recommendation:**
  keep Bugentol in place and preserve the inactive source until the map owner
  resolves provenance. Numeric overlap alone does not establish a replacement
  or safe migration.
- **Should `bugentol` have a command-target alias beyond “Bugentolen”?** The
  normalized title is unique in the current scan, and the bounded area sources
  do not establish another command alias. Keep the source key and title; add
  an alias only after the owner names its exact target and intended distinction.

Reviewed source totals: 125 room headers, 253 authored and resolving WLD exit
records, 27 mobile headers, 18 object headers, and 329 reset commands (215 M,
109 E, 1 G, and 4 D). There are no local QST/SHP/story-sidecar files, no fall
or current fields, no room-object or put-object resets, and no shop records.
No build, test, server, gameplay, database, migration, or operational command
was run for this review.
