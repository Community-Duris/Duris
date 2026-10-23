# Randomness: source-comprehensive story review

The coverage register identifies `area:random` as an active full-area
contributor: 10,000 rooms, 199 mobile prototypes, no local object prototypes or
QST givers, and no story sidecar. The stem appears in both the [full-area
manifest](../../../areas/AREA#L365) and [QST manifest](../../../areas/qst/AREA#L323),
but `areas/qst/random.qst` is absent. Seven direct owner questions and
conservative recommendations are in the [builder-question dossier](RANDOMNESS_BUILDER_QUESTIONS.md).
See also [coverage row 247](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L247).

## Authored rooms and mobile catalog

A static census of all room records finds #100000–#109999 named `Unnamed` with
empty descriptions. The coverage register records 39,600 loader-resolved local
exit records in one weak component. The WLD therefore supplies a connected
room scaffold without authored room descriptions; connectivity does not
establish intended room names, player-facing destinations, or runtime access.
See the [room source](../../../areas/wld/random.wld) and [coverage row](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L247).

All 199 same-stem mobile prototypes were reviewed. Their prose is a broad
collection of wandering characters, tradespeople, class novices, wildlife, and
fantasy creatures rather than a local quest sequence. Specific vignettes include
a peasant and travelling gnome trying to find their way, a young cleric asking
for directions to Verzanan, merchants supporting their families, elemental
creatures separated from former masters, lost undead, raiders, and a half-orc
described as outcast from Shady Grove and Tharnadia. Examples appear in the
[travellers](../../../areas/mob/random.mob#L14),
[cleric](../../../areas/mob/random.mob#L703),
[elementals](../../../areas/mob/random.mob#L892),
[lost undead](../../../areas/mob/random.mob#L1186),
[merchant](../../../areas/mob/random.mob#L172), and
[half-orc](../../../areas/mob/random.mob#L1749) records. These descriptions do
not define local objectives or accepted outcomes.

Other profiles scatter place and livelihood references through the catalog: a
cleric asks for the way to Verzanan, a bugbear is compared with those from
Bugentolen, a wemic scout comes from Wellvolen, and other entries mention
Vella's, Kimordril, and the Astral plane. These are disconnected setting
references in mobile prose; no same-stem quest text connects them into a local
route or story. See the [Verzanan traveler](../../../areas/mob/random.mob#L703),
[Bugentolen reference](../../../areas/mob/random.mob#L1057),
[Wellvolen reference](../../../areas/mob/random.mob#L1161),
[Vella's reference](../../../areas/mob/random.mob#L1303),
[Kimordril reference](../../../areas/mob/random.mob#L1567), and
[Astral-plane reference](../../../areas/mob/random.mob#L1712).

The same-stem object file is empty. No local QST file or story sidecar supplies
a quest outcome for the mobile vignettes. Although `areas/qst/AREA` lists the
`random` stem, that index entry alone does not establish a missing quest's
intended content. Keep the manifest/file mismatch visible in the owner questions
and do not infer a quest from it.

## Resets, shops, and generated-zone source

`random.zon` has two same-stem mobile placements: merchant #100013 in room
#105301 and merchant #100015 in room #105302. Their reset stock and shop records
refer to object prototypes outside the empty local object file. Shop #100013
lists #384, #364, #347, and #53; shop #100015 lists #6008 and #9225. Both shop
records use the refusal “I do not sell to your people!” The text does not define
the intended buyer group or prove that either shop is currently available. See
the [resets](../../../areas/zon/random.zon#L10),
[first shop](../../../areas/shp/random.shp#L1), and
[second shop](../../../areas/shp/random.shp#L38).

The native generator source describes a separate generated-room path. Its room
allocation bounds are #100000–#105000; `create_zone` copies from numbered theme
room templates and connects generated rooms; `create_random_mob` reads mobile
templates #1255 or #1256. Those code references do not show that all 199
same-stem mobiles are part of the generated spawn pool. The static zone resets
place only the two merchants. See the [room bounds](../../../src/world/random.zone.h#L27),
[theme templates](../../../src/world/random.zone.c#L82),
[room creation](../../../src/world/random.zone.c#L184),
[mobile selection](../../../src/world/random.mob.c#L606), and
[local resets](../../../areas/zon/random.zon#L10).

The `areas/RANDOM_AREA` file is empty. The source path that reads this file is
inside `#ifndef RANDOM_ZONES`, while `src/core/defines.h` sets that macro to 1.
This is a source-configuration observation, not runtime verification. Preserve
the file and its manifest relationships until the owner confirms their intent;
do not describe its contents as a current generation schedule. See the
[empty file](../../../areas/RANDOM_AREA),
[conditional reader](../../../src/item/randomeq.c#L496), and
[shared define](../../../src/core/defines.h#L1396).

## Title and tracked identity

A fresh normalized scan of all 442 current `.zon` titles finds one exact
“Randomness” owner, active `random`. Both rosters list the `random` key with
their random-encounter note ([world entry](../../../areas/AREA#L365),
[QST entry](../../../areas/qst/AREA#L323),
[title](../../../areas/zon/random.zon#L2)). The 10,000 local WLD room numbers
are #100000–#109999. The empty `areas/RANDOM_AREA` file is a configuration
input path, not another zone title or map; `random.qst` and a Randomness story
sidecar are absent.

The related unregistered candidate `map22` is a separate, retained 10,000-room
WLD grid at #900000–#909999. Its rooms are all named “A labyrinth in the
grave,” it has no ZON or manifest entry, and a separate conditional
`reset_lab`/`create_lab` code path can refer to it as a random-lab input
([map22 audit](../quest-story-followups/MAP22_AUDIT_REVIEW.md),
[candidate row 64](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65)).
That source-code dependency does not make map22 a second Randomness map or a
whole-zone predecessor: the two WLD ranges and titles differ, and their
followed histories remain on their own stems. Preserve map22's existing
**RETAIN IN PLACE** plan without activating or merging either source.

Followed current `random` WLD, MOB, OBJ, ZON and SHP paths remain on the same
stem. The WLD history reaches the 2009 area-tree move and 2013 edit; MOB reaches
the same move and its 2026 ANSI cleanup; OBJ history starts in 2005 and has only
same-stem history; ZON reaches its 2016 edit; SHP reaches its 2014 loader
cleanup. No alternate-stem rename, distinct predecessor, or whole-zone
replacement appears in those paths. The exact normalized title/history review
is complete; command-target alias and owner questions remain separate.

## Review boundary

This review covers the authored WLD, MOB, OBJ, ZON, SHP and QST-index text,
resets, and the cited generator source. It does not establish live generation,
shop availability, buyer eligibility, external stock ownership, or the
intended use of the QST manifest entry. The connected room graph and source
prose are recorded without inventing a story for the blank rooms or treating
mobile flavor as an implemented quest.

## Checkpoint 821: encounter selection and map identity

The compiled random-encounter selector in [`encounters.c`](../../../src/mob/encounters.c#L68) chooses from #100001–#100168 outside the Underdark and #100300–#100330 in Underdark sectors; these ranges match all 199 prototypes in `random.mob`. This direct encounter selector is separate from the theme-based `create_random_mob` generator, which uses templates #1255 and #1256 ([`random.mob.c`](../../../src/world/random.mob.c#L585)). The room helper in [`random.zone.h`](../../../src/world/random.zone.h#L27) recognizes only #100000–#105000, while active Randomness WLD spans #100000–#109999.

Every active Randomness room VNUM is also present in unregistered Duris3, but the paired titles differ (“Unnamed” versus “The Ocean”) and Duris3 retains a separate candidate map identity. The source-only Duris3 exits that numerically target this range do not establish an active route. See the [Checkpoint 821 typed audit](../quest-story-followups/RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure); preserve the current **DO NOT MOVE** and **RETAIN IN PLACE** dispositions.
