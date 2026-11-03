# Cityruin: source story review

**Scope:** static review of the old-quarter source's QST messages, room and
mobile narrative, objects and resets, WLD connectivity, tracked file history,
and typed references. No QST interaction, shop behavior, route, reset outcome,
or runtime access was tested.

## Source identity and dialogue coverage

Cityruin is an active full-area contributor in numeric range 66200–66354
([area list](../../../areas/AREA#L249)). Its source defines 155 rooms, 42
mobiles, 37 objects, and two QST giver prototypes, #66228 and #66241. Both
have one `M` message; the file has no `Q`/`QA` completion contract. There is
no local SHP file, story sidecar, or earlier mapped source dossier
([zone header](../../../areas/zon/cityruin.zon#L1),
[coverage row](ZONE_COVERAGE_REGISTER.csv)).

The mage guildmaster (#66228) has a `qc_action 50` message that makes him
cackle and causes the listener's flesh to crawl. The quest routine handles
`qc_action` during its idle/timed path, separate from keyed ask/tell topics
([QST](../../../areas/qst/cityruin.qst#L1),
[timed routine](../../../src/world/quest.c#L88)).

The animated elite guard (#66241) has a message keyed `noteforimm` describing
it as intentionally low difficulty to add epic-point rewards for zone
completion and saying rarity can be adjusted. The generic QST ask/tell path
matches a message key against the supplied topic and has no `noteforimm`
special case in the inspected routine. The note therefore appears queryable
by a player who knows that literal key; its intended audience is unresolved.
Neither record defines a native `Q`/`QA` completion contract
([QST](../../../areas/qst/cityruin.qst#L7),
[ask/tell key matching](../../../src/world/quest.c#L1985)).

## Environmental story

- **A ruined old quarter behind rusted gates.** The exterior describes
  collapsing buildings, rubble, bones, and green muck. The city street opens
  into a once-busy commercial district with a royal jeweler, former bank,
  trading houses, and the mayor's meeting hall and office. The store holding
  the old royal jeweler's safe resets a ruby and gem-studded crown inside it
  ([entrance](../../../areas/wld/cityruin.wld#L1),
  [commercial street](../../../areas/wld/cityruin.wld#L182),
  [jeweler's store](../../../areas/wld/cityruin.wld#L388),
  [former bank](../../../areas/wld/cityruin.wld#L503),
  [meeting hall](../../../areas/wld/cityruin.wld#L707),
  [safe contents](../../../areas/zon/cityruin.zon#L201)).
- **A district with civic and military institutions.** The barracks and old
  armory contain undead guards and an elite guard; the reset roster also
  places the skeletal necromancer at the armory entrance. The necromancer's
  description says it is responsible for the undead in Tharnadia's old
  quarter. The old Street of Guilds leads to the Order of Steel, Shadow
  Warrior, and mage halls, whose former masters appear as undead or remains
  ([necromancer](../../../areas/mob/cityruin.mob#L50),
  [necromancer reset](../../../areas/zon/cityruin.zon#L284),
  [guild street](../../../areas/wld/cityruin.wld#L3151),
  [mage hall](../../../areas/wld/cityruin.wld#L3400)). The remains objects
  for the household master and warrior, mage, and rogue guildmasters are all
  named for those figures, but their extra descriptions each say they are the
  necromancer's remains; confirm whether that repeated text is intentional
  ([household and warrior remains](../../../areas/obj/cityruin.obj#L369),
  [mage and rogue remains](../../../areas/obj/cityruin.obj#L395)).
- **Private homes and a failed shelter.** Giant lizards occupy many wealthy
  homes, while some guest rooms hold ghosts and a former household master.
  The room called “In a Bloody Shelter” says the shelter failed to protect
  citizens; a ghost child still searches for its parents there. Nearby
  mansion basements describe shackles, chains, and many human remains, and
  reset former-prisoner ghosts
  ([intact home](../../../areas/wld/cityruin.wld#L2156),
  [shelter](../../../areas/wld/cityruin.wld#L2371),
  [basement](../../../areas/wld/cityruin.wld#L2407),
  [ghost placements](../../../areas/zon/cityruin.zon#L347)).
- **Sewers and old drainage.** Streets connect to several named sewers,
  tunnels, and drainage rooms. The ancient drainage room's text says water
  once flowed through it and out of the city into a nearby river. The WLD
  census finds no numeric current fields, so this is environmental text, not
  evidence of a current or safe travel route
  ([drainage room](../../../areas/wld/cityruin.wld#L3136),
  [cityruin exit census](WORLD_CONNECTIVITY.md)).

The sources present an occupied city before its ruin, undead raised by a named
necromancer, animal occupation of the homes, and explicit civilian deaths.
They do not give a single definitive cause or timeline for the destruction.
The mage guildmaster's description says he was transformed into a lich and is
trying to build his power; the separate reaver master has closely similar
lich-study wording. The title places the old quarter in Tharnadia, but these
sources do not establish how it relates in time or location to the larger
active `tharnadia` zone
([guildmasters](../../../areas/mob/cityruin.mob#L446),
[reaver master](../../../areas/mob/cityruin.mob#L604),
[Tharnadia area-list entry](../../../areas/AREA#L382)).

## Resets, doors, and static connectivity

The static census records 327 authored exits and 327 loader-resolving targets.
All 155 rooms fall in one local weak component. There is one active exit out
and one into Cityruin, and the two records are reverse-paired. No room has a
fall-chance field or numeric current field. These counts do not establish
ordinary access, safety, or traversability
([coverage row](ZONE_COVERAGE_REGISTER.csv)).

| Cityruin boundary | Paired source record |
| --- | --- |
| #66200 east to active Tharnadian Ruin #5588 | Tharnadian Ruin #5588 west to Cityruin #66200 ([Cityruin](../../../areas/wld/cityruin.wld#L15), [Tharnadian Ruin](../../../areas/wld/tharnadian_ruin.wld#L2254)) |

One additional source record points west into #66200 from room #262344 in
unregistered `surf.wld`. This is a source-only reference, not an active-world
entrance. It does not add an active inbound exit or alter the exit census
([`surf.wld`](../../../areas/wld/surf.wld#L1308240),
[world-connectivity record](WORLD_CONNECTIVITY.md)).

The zone resets both city-gate doors closed, while the street text says the
rusted gates have nearly fallen from their hinges and would be easily
passable. Multiple street, home, mansion, and basement passages reset closed;
several routes into the sewer network reset closed and secret, and some house
and basement doors reset locked. Those are authored initial states, not proof
of how a player discovers or opens the routes
([gate resets](../../../areas/zon/cityruin.zon#L8),
[paired gate reset](../../../areas/zon/cityruin.zon#L12),
[gate descriptions](../../../areas/wld/cityruin.wld#L20),
[secret routes](../../../areas/zon/cityruin.zon#L16),
[home and basement doors](../../../areas/zon/cityruin.zon#L160),
[basement door](../../../areas/zon/cityruin.zon#L184)).

## Typed dependencies and code references

- The armory resets import Moonshae object #26233, the lost sword of Cymrych
  Hugh, and Heavens object #359, a rune-covered stone. Both are placed with
  the local armory stock. The external source scan found no active-zone reset
  or shop that consumes a Cityruin mobile. Active `bs` does reset local object
  #66219, the black onyx statue. Current but unregistered Nexus56 zone and
  shop sources reset and list local book #66211, “The Mt.Sknak Treasure
  Hoard”; those sources are outside `areas/AREA` and do not establish that
  those shop/reset records are loaded
  ([armory imports](../../../areas/zon/cityruin.zon#L216),
  [Moonshae sword](../../../areas/obj/moonshae.obj#L498),
  [Heavens stone](../../../areas/obj/heavens.obj#L3725),
  [Cityruin objects](../../../areas/obj/cityruin.obj#L157),
  [active `bs` reset](../../../areas/zon/bs.zon#L576),
  [Nexus56 reset](../../../areas/zon/nexus56.zon#L1376),
  [Nexus56 shop stock](../../../areas/shp/nexus56.shp#L499)).
- C++ binds mobile #66208, described as a zombie noble continuing city work,
  to the `money_changer` procedure; the local reset places it in room #66233.
  Mob #66239 is described as a lich alchemist who sells ingredients, but the
  local zone has no reset for it, no SHP file, and the scanned code has no
  direct binding for that prototype. These source facts do not qualify either
  service as usable
  ([mobile #66208](../../../areas/mob/cityruin.mob#L129),
  [reset](../../../areas/zon/cityruin.zon#L261),
  [procedure assignment](../../../src/specs/specs.assign.c#L810),
  [lich alchemist](../../../areas/mob/cityruin.mob#L619)).
- Room #66201 appears in the “Ancient City Ruins” justice entry, and #66202
  appears in the “City Ruins (Undead)” hometown list. The same assignment
  file binds `undead_inn` to room #66355, but the active Cityruin WLD ends at
  #66354 and no active WLD defines #66355. The intended inn-room identity is
  unresolved. The class-specific `guild_locations` table also points into
  Cityruin rooms #66345, #66348, and #66354. Local object #66231, the steel
  ring of greater physical resistance, appears in two chaos paladin equipment
  tables
  ([justice entry](../../../src/combat/justice.c#L91),
  [hometown list](../../../src/core/constant.c#L1737),
  [guild-room table](../../../src/core/constant.c#L1967),
  [inn assignment](../../../src/specs/specs.assign.c#L2330),
  [last local WLD room](../../../areas/wld/cityruin.wld#L3400),
  [equipment tables](../../../src/account/chaos_eq_data.h#L97),
  [ring](../../../areas/obj/cityruin.obj#L456)).
- The active `tharnadian_ruin.qst` and `tharnadian_ruin.story.json` consume
  Cityruin objects. The QST records require the necromancer's remains #66201
  for one return, require the former-master and three guildmaster remains
  #66225, #66226, #66227, and #66230 for armoury clearance, and award local
  key #66200. The sidecar carries matching `item_vnums` and completion keys.
  QST `G` and `R` roles are parsed as player-given items and received rewards
  ([QST item records](../../../areas/qst/tharnadian_ruin.qst#L70),
  [armoury records](../../../areas/qst/tharnadian_ruin.qst#L81),
  [story contract](../../../areas/story/tharnadian_ruin.story.json#L81),
  [quest parser](../../../src/world/quest.c#L2202)).
- No Cityruin SHP source exists. The full exact-role source scan found no
  external reset of a Cityruin mobile and no active external shop product for
  a Cityruin object. Active `bs.zon` resets local object #66219; unregistered
  `nexus56.zon` and `nexus56.shp` reference object #66211, so those are
  source-only dependencies. No outside QST reference to a Cityruin mobile,
  duplicate external MOB/OBJ prototype, external type-25 destination,
  external type-29 room target, or structured story-sidecar `item_vnums`
  reference beyond the active Tharnadian Ruin entries was found. The active
  Tharnadian Ruin QST/story dependency, `bs` reset, Nexus56 references, C++
  equipment use, and code room/mobile assignments are the typed edges listed
  above; broader editor, saved-state, and runtime closure remains pending.

## Duplicate and replacement history

The normalized active-title scan finds no repeated title for Cityruin. Its
title is “The Ruins of Tharnadia's Old Quarter.” Tharnadian Ruin and
“Tharnadia - City of Humans” are separate active entries; Tharnadian Ruin is
the paired neighboring zone, while the source does not label either area as a
replacement. No whole-zone replacement is established by the tracked history
([Cityruin title](../../../areas/zon/cityruin.zon#L2),
[Tharnadian Ruin title](../../../areas/zon/tharnadian_ruin.zon#L2),
[Tharnadia title](../../../areas/zon/tharnadia.zon#L2),
[Tharnadian Ruin area-list entry](../../../areas/AREA#L28),
[Tharnadia area-list entry](../../../areas/AREA#L382)).

The followed source history includes `642da5400` (2009 `areas/` path migration),
`bb0d59425` (2012 field serialization), `71d731c5b` (2013 mobile/object record
changes while the WLD, QST, and ZON files are mode-only changes), and
`f18992b8f` (2015 quest-item flag update to the object file). It shows no
source rename, retirement, or replacement of Cityruin. This history does not
resolve the current reset, service, description, or code-reference questions.

## Builder questions

1. Should the city gates reset closed despite the room descriptions saying
   they are rusted and easily passable? Confirm that the active paired route
   to Tharnadian Ruin and the source-only `surf.wld` record are intentional.
2. Is the `noteforimm` QST message meant to be queryable by players, and is
   its epic-point tuning language meant to be narrative content?
3. Is mobile #66239 intended to be an active ingredient seller despite having
   no reset or formal shop source? Is #66208 the intended money changer given
   its zombie-noble description and reset placement?
4. Does `undead_inn` belong on a missing room #66355, or should its assigned
   VNUM be reviewed?
5. Should the extra descriptions on the remains of the household master and
   guildmasters (#66225/#66226/#66227/#66230) all identify them as the
   necromancer's remains?
6. Confirm the imported sword and rune-stone in the armory and the continuing
   active Tharnadian Ruin QST/story use of Cityruin remains and key, plus the
   `bs`, Nexus56, and chaos-equipment references before any dependency or
   archival decision.
7. Is the failed-shelter and imprisoned-citizen backstory intended to carry
   the current violent tone?
8. How should this ruined old quarter relate in time or location to the
   separately named active Tharnadia city zone?

This review records source text, topology, and static references only. Gate
state, secrets, the purported shops, QST presentation, `undead_inn`, reset
outcomes, and safe player routes remain unqualified.
