# Tharnadia Rifts: source environmental map

## Evidence boundary

This dossier maps the active tharnrifts contributor from its [WLD](../../../areas/wld/tharnrifts.wld#L1), [ZON](../../../areas/zon/tharnrifts.zon#L1), reset
prototypes, source indexes, existing story maps, and bounded code references.
It records authored room labels, exits, and reset declarations. It does not
establish a live reset episode, actor access, route safety, mining availability,
or a played journey.

The active WLD has 5,783 rooms in the #110000–#119999 band and 22,252 authored
directions. Every room description is blank, and the file has no extra
descriptions, fall fields, or current fields. Its 41 color-stripped title
families carry nearly all of the local environmental text. The ZON header is
“Tharnadia Rifts” with reset mode 0. The active area list includes the WLD,
empty MOB/OBJ files, and ZON; the QST index also lists tharnrifts, but there
is no tharnrifts.qst file or local story sidecar.

## Room-title map

Counts below are title occurrences, not unique landmarks or proven geographic
boundaries. The map uses repeated labels across broad groups of rooms.

| Environmental group | Color-stripped room title | Rooms |
| --- | --- | ---: |
| Blackrock | The Blackrock Mountain Peaks | 886 |
| Blackrock | The Jagged Floor of Blackrock Basin | 295 |
| Blackrock | The Molten Blackrock Flow | 251 |
| Blackrock | A Flow of Molten Blackrock | 144 |
| Blackrock | A Searing Flow of Molten Blackrock | 107 |
| Blackrock | The Dark Steppes of the Blackrock Mountains | 68 |
| Blackrock | A Mountain Trail Around the Blackrock Spire | 14 |
| Blackrock | An Island of Shiny Blackrock | 14 |
| Blackrock | At the Top of a Blackrock Staircase | 1 |
| Sea and islands | The Sea of Twin Worlds | 847 |
| Sea and islands | A Grassy Stretch of the Island | 178 |
| Sea and islands | A Rocky Island Overlook | 35 |
| Sea and islands | A Fruitful Tropical Forest | 30 |
| Sea and islands | A Sandy Beach | 27 |
| Sea and islands | A Long Crystal Reef | 22 |
| Sea and islands | A Rocky Ocean Island | 15 |
| Sea and islands | The Island of Everlasting Bliss | 4 |
| Shaeradim and highlands | The Twisted Peaks of the Shaeradim Valley | 601 |
| Shaeradim and highlands | Rocky Floor of the Shaeradin Valley | 492 |
| Shaeradim and highlands | A Rocky Hillside | 310 |
| Shaeradim and highlands | The DragonMaw Mountains | 405 |
| Shaeradim and highlands | The Snow-Covered Mountains | 326 |
| Shaeradim and highlands | A Snow-Covered Hillside | 211 |
| Shaeradim and highlands | A Steep Grassy Incline | 24 |
| Shaeradim and highlands | Mountains | 37 |
| Shaeradim and highlands | A Mountain Trail | 6 |
| Shaeradim and highlands | Rolling Hills | 2 |
| Straka and Bren'Shan | The Snowy Steppes of Straka | 33 |
| Straka and Bren'Shan | The Frozen Steppes of Bren'Shan | 28 |
| Straka and Bren'Shan | A Massive Snowbank | 22 |
| Straka and Bren'Shan | Mount Straka | 12 |
| Straka and Bren'Shan | Mount Bren'Shan | 9 |
| Alteran | The Alteran Valley | 198 |
| Jungle and forest | A Thick Jungle | 47 |
| Jungle and forest | A Dense, Humid Jungle | 25 |
| Jungle and forest | A Dismal Forest of Faded Everblooms | 13 |
| Jungle and forest | A Steaming Hotspring | 7 |
| Jungle and forest | A Fiery Volcano | 17 |
| Named landmarks | Zone Entrance | 18 |
| Named landmarks | A Monolith of the great Fotenak Hellhammer | 1 |
| Named landmarks | A Small Patch of Silverleaf Trees | 1 |

The sea, island, mountain, snow, volcanic, valley, and jungle labels describe
the source's intended environmental palette. The 18 “Zone Entrance” titles do
not by themselves identify which boundary or route each room serves. Room
#110000, “The Island of Everlasting Bliss,” is a singleton with one active
arrival from Heavens and no local WLD exits. Room #112545, “A Monolith of the
great Fotenak Hellhammer,” is another singleton, with no known active arrival
or local WLD exit. Their authored isolation is recorded for builder review;
this dossier does not add implied connections.

## Active routes

Seventy-four active WLD directions enter the area: 73 from Caertannad and one
from Heavens. The Heavens direction is #1204 down to #110000
([cross-area register row 18](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L18)).
Twenty-five directions leave: 23 return to Caertannad along reciprocal WLD
boundaries, while #110099 east and #119900 south lead one way to SurfaceKeeps
#120000 ([outbound row 1190](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1190)
and [second SurfaceKeeps edge](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1214)).
The other 50 Caertannad arrivals have no reciprocal local WLD direction; the
central cross-area register preserves each inbound source room, direction, and
return flag.

Two more local directions target #116411 and #116439. Those room numbers occur
only in inactive Duris3.wld, so the full active-world load leaves these
positive targets unresolved and prunes them
([first record](../quest-story-followups/WORLD_UNRESOLVED_EXIT_REGISTER.csv#L319),
[second record](../quest-story-followups/WORLD_UNRESOLVED_EXIT_REGISTER.csv#L320)).
They are not active returns.

The broader Tharnadia story map describes four item-teleport pairs associated
with the Twin Keeps. Those object routes use separate prototype and room
references and should not be conflated with the 110000-series Tharnadia Rifts
WLD contributor. The Rifts map's own named story connections are the imported
silverleaf and hidden-crevice placements described below.

## Reset ownership and story connections

All 62 ZON reset commands place prototypes owned by other source areas. The
local MOB and OBJ files are empty, so no local mobile or object prototypes are
declared here.

| Reset placement | Source owner and locations | Source role |
| --- | --- | --- |
| Eight #78457 half-eaten corpses | caertannad.obj; at #110726, #111123, #111335, #111717, #111840, #112233, #112316, and #112639 | Scenery reset alongside the spike-boar sites. |
| One #78503 hidden crevice | caertannad.obj; at #113375 | Type-25 object destination is Caertannad room #78848. The Caertannad dossier identifies the destination as the hidden cave containing the pirate chest and ether crystal. |
| Five #78409 skeletal remains | caertannad.obj; at #115679, #116065, #116160, #118296, and #119486 | _noquest_ scenery at the angry-vulture sites. |
| One #78455 silverleaf | caertannad.obj; at #116025, 65% reset chance | Material source named by the Caertannad story map for Mungir's service. That sidecar says the mixed silverleaf-and-coin offer is currently unavailable under active accounting. |
| Eight #94720 “bits bones” | minopass.obj; at #116757, #116859, #116956, #117258, #117361, #117455, #117659, and #117856 | Imported scenery; the Deep Ravine source audit records these as contextual remains, not a local quest proof. |
| Twenty-four #78467 starving spike boars | caertannad.mob; three repeated M commands at each of the eight corpse sites | Imported regional population. |
| Fifteen #78412 angry vultures | caertannad.mob; three repeated M commands at each of the five skeletal-remains sites | Imported regional population. |

The exact reset declarations are in
[tharnrifts.zon](../../../areas/zon/tharnrifts.zon#L10). Prototype definitions
remain owned by Caertannad OBJ #78457 ([line 844](../../../areas/obj/caertannad.obj#L844)),
#78409 ([line 126](../../../areas/obj/caertannad.obj#L126)), #78455
([line 820](../../../areas/obj/caertannad.obj#L820)), and #78503
([line 1505](../../../areas/obj/caertannad.obj#L1505)); Caertannad MOB #78412
([line 183](../../../areas/mob/caertannad.mob#L183)) and #78467
([line 1023](../../../areas/mob/caertannad.mob#L1023)); and Minopass OBJ #94720
([line 318](../../../areas/obj/minopass.obj#L318)). The Rifts crevice reset and
destination are described in the [Twin Keeps dossier](TWIN_KEEPS_OF_DEVASTATED_THARNADIA.md#access-nested-sources-and-physical-evidence);
the imported bones are covered by the [Deep Ravine audit](../quest-story-followups/DEEP_RAVINE_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches).

The tharnrifts entry in
[areas/qst/AREA](../../../areas/qst/AREA#L324) has no matching local QST file.
The source does not establish whether that index entry is intentional future
work or stale membership. It remains a builder question, as does the intended
reset schedule for a zone whose header mode is zero.

## External code and source identity

The mining configuration names a “Tharnadia Rift” region spanning
#110000–#119999. This is a system range used for mining configuration, not a
WLD boundary or a travel edge. Two special-procedure names can also be
misread as Rifts routes: mobile #500129, “Baltazo,” has an old “speed wipe
entry to Tharn Rifts” comment (the mobile is reset at Surface #568439,
[ZON line 660](../../../areas/zon/surface.zon#L660)) but its active handler only emits cosmetic text
and rejects a few attack commands; its old death-and-teleport block is
commented out. Object #500055 is reset in the Surface Realm at #568439
([reset](../../../areas/zon/surface.zon#L32)) and is a shadowrift whose active
handler sends an entrant to Tharnadian Ruin #5569 or #5583 by race-war side,
not into this map ([binding](../../../src/specs/specs.assign.c#L1277),
[handler](../../../src/specs/specs.zion.c#L37)).

The color-stripped ZON title is unique among the 442 current ZON files.
Current active same-type room IDs do not overlap this contributor. Inactive
Duris3.wld declares all 5,783 Rifts room numbers, but every normalized room
title differs; these remain distinct map-source identities. Followed WLD and
ZON history stays on the same stem and establishes no separately named
predecessor or whole-area replacement. Keep the active contributor in place.
