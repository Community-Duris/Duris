# The Prisons of Carthapia: repository-wide connectivity and dependency audit

**Checkpoint 95; priority 135 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive dossier](../zone-stories/PRISONS_OF_CARTHAPIA.md)
with the active WLD graph, all inactive WLD sources, typed external
references, numeric C/C++ matches, normalized active title, and followed
WLD/ZON history. Authored exits and source-only map declarations do not
establish player availability, safe travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Prisons of Carthapia has
199 rooms and 455 authored exits; all 455 resolve with no parse issue. The
local weak components contain 166, 17, and 16 rooms: the largest spans
#7300–#7497, the Limbo and gateway component spans #7456–#7472, and the planar
arrival and Nexus component spans #7479–#7498. Seven fixed portal objects and
the custom Nexus route connect destinations through object handlers; those
routes do not merge the authored WLD components.

The only active inbound WLD boundary is reciprocal Surface room #636262 north
to Prison entrance #7497, with #7497 south back to #636262
([Surface source](../../../areas/wld/surface.wld#L2860950),
[Prison return](../../../areas/wld/prison.wld#L3633); [cross-area records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1665),
[return record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L174)). The 35 other active
outbound exits leave the planar rooms in seven sets of five. Every set is
one-way in the authored WLD graph:

| Prison room | Active destination | Exits | Evidence |
| ---: | --- | ---: | --- |
| #7479, Water plane | #23255, `plane_water_one` | 5 | [WLD](../../../areas/wld/prison.wld#L3207); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L139) |
| #7484, Fire plane | #25504, `plane_fire_one` | 5 | [WLD](../../../areas/wld/prison.wld#L3304); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L144) |
| #7486, Ethereal plane | #12544, `ethereal_main` | 5 | [WLD](../../../areas/wld/prison.wld#L3354); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L149) |
| #7488, Astral plane | #19728, `astral_main` | 5 | [WLD](../../../areas/wld/prison.wld#L3408); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L154) |
| #7490, Air plane | #24439, `plane_air_one` | 5 | [WLD](../../../areas/wld/prison.wld#L3460); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L159) |
| #7492, Avernus | #51032, `hell_one_avernus` | 5 | [WLD](../../../areas/wld/prison.wld#L3513); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L164) |
| #7494, Earth plane | #23905, `plane_earth_one` | 5 | [WLD](../../../areas/wld/prison.wld#L3565); [register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L169) |

There are no other active WLD inbounds. The graph has no negative or
unresolved targets, source rooms outside the numeric zone range, or exits
from range-mismatched rooms. The two fall-chance fields are F60 in shaft
#7382 and F80 in shaft #7383 ([first](../../../areas/wld/prison.wld#L1489),
[second](../../../areas/wld/prison.wld#L1508)); there are no current fields.
The only duplicate active room VNUM found globally remains unrelated #51800
in Hell Two and Hell Three.

The inactive map scan found one additional approach: `surf.wld` room #263479
north to #7497 ([room](../../../areas/wld/surf.wld#L1332039),
[exit](../../../areas/wld/surf.wld#L1332043),
[target](../../../areas/wld/surf.wld#L1332046)). `surf.wld` is not in the
active `areas/AREA` manifest, and its source room does not overlap Prison
VNUMs, so this is a source-only declaration rather than active ingress. The
all-95 inactive-WLD scan parsed 2,080,773 exits without issues, found no
source-room VNUM overlap, and found no other exit into Prison. The reviewed
Surf subset is now 123; the curated source-only reference total is 1,506.

These are authored graph relationships. Door and reset state, local fixed
portals, the Nexus's random surface destination, hazards, successful travel,
and safe return remain separate; no live outcome is qualified here.

## Typed references and external producers

No external active or source-only QST item command uses a local Prison object.
There are no active external SHP products, and no external mobile/object
prototype definition matches a local prototype.

The active ZON scan found 13 reset-role references across Alatorin, Ravenloft,
and Ravenloft2. Alatorin places local object #7353 in ten rooms
([first](../../../areas/zon/alatorin.zon#L1557),
[last](../../../areas/zon/alatorin.zon#L1633)) and loads local mobile #7365,
the Tarrasque, at room #84007 ([reset](../../../areas/zon/alatorin.zon#L4487)).
Ravenloft and Ravenloft2 each reset local portal object #7371
([Ravenloft](../../../areas/zon/ravenloft.zon#L468),
[Ravenloft2](../../../areas/zon/ravenloft2.zon#L922)). Object #7365 is also
the local Flaming Axe, a separate object-namespace prototype; the mobile reset
is not a reset of that weapon.

The unregistered source-only Nexus56 files add two ZON `G` references to
local objects: bone sandals #7356 and garland #7357
([first](../../../areas/zon/nexus56.zon#L1314),
[second](../../../areas/zon/nexus56.zon#L1322)). Its SHP declares garland
#7357 as a product in shop #83031
([product](../../../areas/shp/nexus56.shp#L204)); local object #7357 is
“garland flowers” ([prototype](../../../areas/obj/prison.obj#L533)), while
mobile #7357 is the Chaplain. Nexus56 is not in `areas/AREA`. Its possible
all-file SHP fallback exposure is already recorded in the
[Nexus56 coverage row](ZONE_COVERAGE_REGISTER.csv#L430); these source records
do not establish an operating shop or active reset.

Across 862 external type-25 declarations (703 active and 159 source-only),
none targets a Prison room. The seven local fixed portals and active external
Nexus copies are already mapped in the source dossier; the external type-25
scan does not add another destination into the area.

All 231 external story sidecars parse without errors and have no structured
Prison room, mobile, or object VNUM fields. A deeper token scan reports three
number matches, all inside opaque IDs: two Alatorin exchange IDs
([first](../../../areas/story/alatorin.story.json#L653),
[second](../../../areas/story/alatorin.story.json#L3509)) and one Winterhaven
service-request ID ([line 929](../../../areas/story/wh.story.json#L929)). These
strings do not express typed Prison dependencies.

## C/C++ numeric source matches

The exact-number scan returned 12 matching source lines in eight C/C++ files.
Context review classifies the selected references as follows:

| Source | Static reference | Classification |
| --- | --- | --- |
| [`specs.assign.c`](../../../src/specs/specs.assign.c#L1061) | Mobile #7333 | Bound to `warden_shout`; the callback and helper rules are mapped in the source dossier. |
| [`specs.assign.c`](../../../src/specs/specs.assign.c#L1324) | Object #7365 | Bound to the Flaming Axe of Azer procedure. |
| [`specs.assign.c`](../../../src/specs/specs.assign.c#L1410) | Object #7371 | Bound to the Nexus handler. Its random surface destination is not the WLD edge at #7497. |
| [`specs.prison.c`](../../../src/specs/specs.prison.c#L16) | Mobile #7359; helper IDs at [line 20](../../../src/specs/specs.prison.c#L20) | The Azer constant and Warden summon list are local combat behavior, not quest receipts. |
| [`epic_skills.c`](../../../src/classes/epic_skills.c#L187) | Mobile #7357 | KI_STRIKE teacher table entry; normal boot dynamically binds the local Chaplain as described in the dossier. |

Other raw matches are decimal constants, compatibility hashes, comments, or
unrelated saved-journal constants. Numeric coincidence is not treated as an
additional active route or binding. The scan is static and does not establish
configuration-specific runtime availability.

## Title, history, and disposition

The color-stripped title in
[`prison.zon`](../../../areas/zon/prison.zon#L2) normalizes uniquely among the
351 active contributors. Followed WLD/ZON history shows the 2009 area-tree
move, later typo and content maintenance, the 2015 quest-item flag update, and
the 2026 planar-prison center-sector fix. It identifies no distinct
predecessor or replacement. The
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no
named Carthapia entry; broader semantic comparison remains open.

The current un-commented [`areas/AREA` row](../../../areas/AREA#L41) remains
**DO NOT MOVE**. The source dossier retains the two independent accepted
returns, key and switch state, combat, escape and rescue gaps, training and
rune services, outside reset sources, and 12 builder/capability follow-ups.
This supplement adds repository-wide source evidence; it does not resolve a
builder question or change native gameplay. Any separate future attic review
must account for the reciprocal Surface route, inactive `surf.wld` approach,
source-only Nexus56 item references, and the active external producers.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 329: reset-aware access and builder decisions

The current Prison WLD, MOB, OBJ, ZON, QST, and story sidecar remain at the
recorded source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. Current
HEAD c500ac51 has documentation changes after that snapshot; the Prison
source paths have no later diff. The existing source-comprehensive dossier
and Checkpoint 95 repository audit remain valid, with this section adding
reset-aware arrival reach, a current title/neighbor comparison, and explicit
builder decisions.

### Title and neighboring zone comparison

After color-code stripping and whitespace normalization, the title
“The Prisons of Carthapia” occurs once among 442 current zone titles and once
among 351 active contributors. Followed WLD/ZON history records the 2009
area-tree move, later maintenance and quest-item flags, and the 2026
center-sector fix; it names no predecessor or replacement.

The active Prison of Fort Boyard (prisonb, #43000–#43051) has a different
normalized title. The two zones have zero exact normalized room-title matches
and zero exact normalized full-room-description matches. This comparison is
bounded to current text and followed source history; it does not claim that
the zones have no broader thematic relationship.

The declared Surface arrival is #7497, whose open north exit reaches #7496
and then the Main Foyer #7300. Raw directed local WLD reach from #7497
contains 166 of 199 rooms, matching the 166-room component. After applying
all 76 ZON D reset states and blocking closed, locked, secret, or blocked
WLD exits, open-only local reach is five rooms: #7497, #7496, #7300, #7301,
and #7302. The first main gate is #7300 north to #7303, reset closed and
locked with key VNUM #7300. The source loads that key on the Minotaur in
#7300. This is a source-state measurement; players may obtain keys, open
doors, discover secret passages, or use switches and portals.

The 76 D resets encode 66 closed/locked doors, two secret-but-open exits,
two closed/secret exits, four closed/locked/secret exits, and two blocked
exits. The strict traversal excludes the latter categories and does not
model searching, unlocking, the remote item switches, fixed portals, or the
Nexus callback. Keep the source-defined mechanics separate from proof of
played accessibility, safety, return, or persisted state. The [existing
dossier](../zone-stories/PRISONS_OF_CARTHAPIA.md) and [graph audit
sections](#active-wld-graph) document the remaining routes and boundaries.

## Checkpoint 329 builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PR-01 | Is the shadowy visitor's missing partner intended to have a rescue or recovery outcome? | Keep the coin return independent; do not record a rescue, death, or escape without a separate authored outcome. |
| PR-02 | Should Q20 continue to accept only tarnished coin #7372 for its 15,000-coin reward? | Preserve the exact item identity and accepted-return contract; ordinary money or another coin does not qualify. |
| PR-03 | Is the hidden coin inside the open hole #7349 in commode #7353 meant to require SEARCH before extraction? | Preserve ITEM_SECRET and the nested reset; distinguish discovery, extraction, first-source recovery, and loose proof. |
| PR-04 | Is Q47 intended to require all three distinct scale prototypes #7342, #7343, and #7344, which share the same visible wording? | Keep the three exact VNUMs and shield #7345 reward; do not treat three copies of one prototype as a set. |
| PR-05 | Should the scales remain reset on Smaug #7353 without a selected kill or carving handler issuing them? | Preserve current reset stock; request a separate authored provenance decision before adding a drop, kill gate, or alternate producer. |
| PR-06 | Are the two visitor requests meant to remain independent of each other and of personal combat credit? | Keep Q20 and Q47 separate; do not infer partner rescue, personal Smaug kill, or group victory from an accepted item return. |
| PR-07 | Should either world-cap-one giver or material restock when a daily eligibility window rolls over? | Preserve existing native reset and daily behavior; require owner approval and a separate durable-state design before changing stock. |
| PR-08 | Does the entrance sequence intentionally stop at the locked #7300-to-#7303 portcullis with key #7300 carried by the Minotaur? | Retain gate, key, carrier, and reset; qualify intended discovery and arrival flow before revising the route. |
| PR-09 | Are the small, golden, and large silver keys assigned to the intended minimum, cell, and maximum security gates? | Preserve each prototype and reset placement; keep key custody distinct from proof that a character opened a gate. |
| PR-10 | Is Captain #7351's obsidian key #7340 the intended route through the secret lower hatches, Smaug, and the oubliette? | Keep both secret/locked reset directions and SEARCH behavior; request confirmation of intended discovery and return sequence. |
| PR-11 | Are the two ITEM_SWITCH torches #7354 and #7355 meant to operate the distant blocked/secret passages as currently wired? | Preserve actor/object/target roles and conditional reverse-side updates; do not give ordinary torch #7353 the same behavior. |
| PR-12 | Should the planar arrivals and Nexus room remain separate WLD components connected by fixed portals and special handling? | Preserve all component identities and listed destinations until the owner confirms any additional intended physical exits. |
| PR-13 | Are the seven fixed ENTER portals meant to send players to the seven separate planar-prison branches as authored? | Keep their destinations and distinct prisoner rooms; qualify access, danger, and safe return before publishing travel guidance. |
| PR-14 | Should Nexus object #7371 continue to choose a random eligible surface room instead of fixed entrance #7497? | Preserve the current handler and exclusions; never document its object value as a guaranteed destination. |
| PR-15 | Is the key asymmetry between the #7481-to-#7498 vault door and its reverse side intentional? | Retain both reset records and the generic mirrored open/unlock behavior; do not label it a lockout from key fields alone. |
| PR-16 | Is rune #358's zone-touch reward an independent optional objective beside the two visitor contracts? | Keep its epic settlement rules separate; do not add rune completion to Q20 or Q47. |
| PR-17 | Should Chaplain #7357's KI_STRIKE lesson remain separate from story completion while active accounting refuses purchases? | Preserve current training admission and payment rules; do not activate purchases or add an automatic lesson award. |
| PR-18 | Should Warden hunts, axe proc assistance, poison, and other combat outcomes remain ambient prison behavior? | Keep combat sources separate from quest receipts; require a durable credited-outcome contract before connecting them. |
| PR-19 | Do the external Tarrasque/reset, Nexus56 item, and Winterhaven ambassador/memory records describe imports or adjacent lore only? | Preserve the exact typed dependencies and source identities; do not treat them as local quest issuers or active routes. |
| PR-20 | Is the inactive surf.wld approach #263479 north to #7497 still a historical map declaration? | Keep it on Surf candidate row 97 as source-only, not active ingress; preserve Surf RETAIN IN PLACE and all current references. |

The source-comprehensive dossier already records twelve detailed capability
follow-ups. This twenty-question table makes the owner decisions explicit
for the coverage register while retaining those implementation
qualifications and the current no-gameplay-change disposition.
