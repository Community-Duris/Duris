# Jotunheim: repository-wide connectivity and typed-closure supplement

Status: static audit supplement to the existing source-comprehensive
[Jotunheim story dossier](../zone-stories/JOTUNHEIM.md). This review reconciles
its local findings with the frozen full-area census, all current unregistered
WLD inputs, current typed consumers, and tracked title/history evidence. It
makes no gameplay or source-data change and does not qualify live access.

## Active WLD boundary and negative target

The active Jotunheim WLD has 746 authored exit records, 745 loader-resolving
records, one local weak component of 296 rooms, and no fall or current fields.
It has no active cross-zone WLD inbound or outbound edge. The single dropped
record is the down exit from room 96004 to VNUM -1; the world loader rejects
negative targets, so this is neither a route to room 0 nor a missing positive
zone destination. The exact target remains in the unresolved-exit ledger.
[Jotunheim room and exit](../../../areas/wld/jotun.wld#L80) ·
[negative target](../../../areas/wld/jotun.wld#L115) ·
[unresolved-exit row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L315) ·
[coverage row](ZONE_COVERAGE_REGISTER.csv#L140)

Checkpoint 28 parsed every authored D0–D9 exit in all 95 current unregistered
WLD files: 2,080,773 records, with zero target VNUMs in 96000–96295. At that
checkpoint Jotunheim added no source-only inbound record and the curated list
remained 360. Checkpoint 56 repeated the all-95 scan: it again found no target
in rooms 96000–96295, no source-room overlap, and no parse issue. The curated
source-only total is now 592 after later zone reviews. Object-based Astral
portals described in the main dossier are a separate authored system and do
not add an edge to this WLD census.

## Typed dependencies reconciled with the existing dossier

The main dossier covers all 15 local QST contracts, the two foreign recipes
that touch Jotunheim prototypes, the assigned mobile/object procedures and
shared handlers, imported objects, and 713 active type-25 portal declarations.
The repository-wide numeric cross-check confirms these specific external
connections:

- Active Astral object 19701 is placed in room 19733 and carries an effect
  whose destination is Jotunheim room 96004. Jotun object 96004 provides the
  authored return destination to Astral room 19733. This is documented portal
  wiring, not a WLD crossing or proof of a live trip.
  [Astral object](../../../areas/obj/astral_main.obj#L15) ·
  [portal destination](../../../areas/obj/astral_main.obj#L21) ·
  [Astral reset](../../../areas/zon/astral_main.zon#L10) ·
  [Jotunheim dossier](../zone-stories/JOTUNHEIM.md#actual-material-and-access-dependencies)
- Active Shabo reset commands G-carry Jotun outputs 96077 and 96078 from the
  foreign mobile placement. They are the only active external ZON reset group
  with direct local item IDs found in the scan.
  [Shabo reset rows](../../../areas/zon/shabo.zon#L371) ·
  [Shabo giver](../../../areas/mob/shabo.mob#L396)
- The foreign Fearfrost and Mundorno recipes use Jotun items as inputs or output;
  the main dossier records their complete accepted-material and reward sides.
  [Airp inputs](../../../areas/qst/airp.qst#L244) ·
  [Alatorin result](../../../areas/qst/alatorin.qst#L5710) ·
  [foreign-ownership review](../zone-stories/JOTUNHEIM.md#rare-actors-services-lore-and-execution)

There is no external shop or mobile prototype with a direct Jotun item VNUM in
the source scan. The external object-file scan found the Astral destination
field for room 96004, not a second object prototype definition, and a numeric
match for room 96234 only in the cost field of Nexus56 object #83170
([cost record](../../../areas/obj/nexus56.obj#L2621)). No local SHP file exists;
the Jotun dossier separately documents the assigned table-driven smith.

One additional numeric reference belongs to a source-only family: unregistered
brass-old-1.zon has two G commands for Jotun object prototypes 96071 and 96076.
The file is outside the current full-area list, so these are not active reset
consumers. Preserve the Brass source family until its broader reset, QST, shop,
tooling, and history closure is complete.
[Brass source-only reset rows](../../../areas/zon/brass-old-1.zon#L813) ·
[Brass retention entry](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv)

## Title and replacement-history boundary

The normalized active title Jotunheim has one owner, area Jotun at zone 960.
Tracked path history records the old areas-tree move and generic source updates;
no manifest entry or reviewed file-history record identifies Jotunheim as a
replacement for a distinct active or retired zone. Its single dropped negative
exit and the object-mediated Astral portal are not evidence of duplicate or
replacement lineage. Keep the area active and retain the source-only Brass
references until an owner makes a separate disposition decision.
[Active title](../../../areas/zon/jotun.zon#L2) ·
[active-area entry](../../../areas/AREA#L344) ·
[duplicate and replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Checkpoint 56 typed-reference revalidation

The current exact scan reconfirmed the two external QST consumers: Fearfrost's
Airplane recipe uses Jotun items #96000, #96012, and #96055, while Alatorin's
Mundorno exchange returns #96021 ([Fearfrost contract](../../../areas/qst/airp.qst#L244),
[Mundorno contract](../../../areas/qst/alatorin.qst#L5710)). All 231 other
story sidecars parsed without errors; the three exact numeric fields are the
Airplane sidecar records matching Fearfrost's inputs
([story records](../../../areas/story/airp.story.json#L633)). No external SHP
or mobile prototype reference matched.

The only active external reset group remains Shabo Jabulanth carrying local
outputs #96077 and #96078
([reset rows](../../../areas/zon/shabo.zon#L371)). The two #96071/#96076 G
commands in brass-old-1 remain source-only because that source is excluded from
the active area list ([Brass rows](../../../areas/zon/brass-old-1.zon#L813)).
Direct code matches still cover the assigned Jotun procedures, epic teacher,
and table-driven smith already described in the main dossier. The refreshed
numeric scan adds a CHAOS starter-kit reference: object #96012, the huge
adamantium mace named Mistweave, appears in both standard and enhanceable Druid
profiles ([object](../../../areas/obj/jotun.obj#L201),
[standard Druid profile](../../../src/account/chaos_eq_data.h#L134),
[enhanceable Druid profile](../../../src/account/chaos_eq_data.h#L384)).
The CHAOS new-character kit builder selects the profile and adds listed items
([kit builder](../../../src/account/nanny.c#L682)); the existing object
procedure assignment remains the one mapped in the dossier.

The normalized active-title scan still finds one Jotunheim title among 351
active contributors. Tracked WLD history reaches its 2013 edit and 2009
area-tree relocation; ZON history reaches the 2015 quest-item flag update, 2013
edit, and relocation. This refresh found no distinct predecessor or
replacement, no external SHP/mobile match, and no additional WLD approach.

## Checkpoint 242: current catalog, tool, and semantic revalidation

The current catalogs define 445 mobile and 442 object prototypes; none defines
a second owner for a Jotunheim mobile or object VNUM. Across all
872 current type-25 declarations, the only outside object destination into a
Jotunheim room is active Astral Main object #19701 to room #96004. Jotun object
#96005 supplies the authored return to Astral Main room #19733. There is no
external SHP product, and the current QST scan adds no foreign item-command
consumer beyond the already documented Fearfrost and Mundorno contracts.
All 231 outside story sidecars parsed; the three exact outside Jotun object
values are Fearfrost's existing inputs. The active Shabo resets and source-only
Brass resets remain the only outside ZON consumers found.

Jotunheim's own resets import the four currently defined outside objects
#72/#359/#55175/#67241, as catalogued in the dossier. A repository-wide exact
numeric scan outside area data and audit documents found the known assigned
procedures, epic-skill row, Chaos Druid profiles, and smith menu binding in
production code, plus the existing Jotun fixtures in `tests/async/`; no
additional checked-in editor or command-tool reference was found. These are
source references, not evidence about installed state or player records.

The normalized title remains unique. `stormht` is a separate contributor;
its WLD says the Storm Giants left Jotunheim. `cldgt` is separately titled
Cloud Giant Kingdom. The unregistered `eoo` source is titled Valhalla and has
no WLD approach or room overlap with Jotunheim; Jotun object #96044's Valhalla
artifact wording is a lore relationship. None of these checked-in records
identifies a duplicate owner, predecessor, replacement, or merge.
[Jotun title](../../../areas/zon/jotun.zon#L2) ·
[Storm Giants migration text](../../../areas/wld/stormht.wld#L2294) ·
[Storm Giants title](../../../areas/zon/stormht.zon#L2) ·
[Cloud Giant Kingdom title](../../../areas/zon/cldgt.zon#L2) ·
[Valhalla source](../../../areas/wld/eoo.wld#L2) ·
[Jotun Valhalla artifact](../../../areas/obj/jotun.obj#L1056)

This closes the bounded static review of current checked-in area catalogs,
story sidecars, code, and test/tooling references. Persisted account/player
records were not queried and remain outside this source review. No editor
session, portal trip, reset, quest, or gameplay journey was run.

## Audit boundary

These remain static source findings. Actual portal use, reset execution, quest
availability, item custody, reward publication, door state, race/level access,
staff paths, and persisted user/account records were not tested or queried.
The full Jotunheim dossier contains its builder requirements, source
identities, and gameplay qualification plan; no route, quest, or accounting
behavior was exercised here. The source-comprehensive audit queue is now
**96/220 complete, 124 pending**; priority 97, Temple of Flames (`temple`), is
next.
