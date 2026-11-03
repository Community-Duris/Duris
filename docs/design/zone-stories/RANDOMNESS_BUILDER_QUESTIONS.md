# Randomness: builder questions and conservative recommendations

The coverage register lists `area:random` as an active full-area contributor:
10,000 rooms, 199 mobile prototypes, no local object prototypes or QST givers,
no mapped source dossier, and an empty `areas/RANDOM_AREA` file. A static census
of `random.wld` finds all 10,000 room records named `Unnamed` with empty
descriptions. The register records 39,600 loader-resolved exits in one local
component. These are source facts; they do not establish intended player-facing
content or live generation behavior. See the [coverage row](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L247),
[room source](../../../areas/wld/random.wld), and
[zone source](../../../areas/zon/random.zon#L1).
The source-comprehensive room, mobile, reset, shop, and generator review is in
[RANDOMNESS.md](RANDOMNESS.md).

## Open area-owner questions

- **RANDOMNESS-Q01 — Are the connected, unnamed blank rooms intended to remain
  structural map records, or should players encounter an authored Randomness
  map there?** The source contains 10,000 blank-named records; the generator
  allocates dynamic rooms only through #105000 while the source area extends to
  #109999. **Conservative recommendation:** preserve the current room records
  and exits as structural data until the owner defines any intended player-facing
  layout; do not invent names, descriptions, or destinations. See the
  [room source](../../../areas/wld/random.wld),
  [coverage row](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L247), and
  [random room range](../../../src/world/random.zone.h#L27).

- **RANDOMNESS-Q02 — What role, if any, should the empty `areas/RANDOM_AREA`
  file retain?** The source loader for that file is inside an `#ifndef
  RANDOM_ZONES` block, while the shared define currently sets `RANDOM_ZONES` to
  `1`. This source relationship does not verify deployment or runtime behavior.
  **Conservative recommendation:** keep the file empty and do not describe it as
  an active generation schedule unless the owner identifies the intended build
  configuration and entries. See the [empty file](../../../areas/RANDOM_AREA),
  [file loader](../../../src/item/randomeq.c#L540), and
  [feature define](../../../src/core/defines.h#L1396).

- **RANDOMNESS-Q03 — Are the 197 same-stem mobile prototypes without a local
  reset intentional catalog content, and which systems are meant to consume
  them?** The local zone resets only merchants #100013 and #100015. The generic
  random-mob routine shown in source reads mobile prototypes #1255 or #1256,
  which does not by itself account for every same-stem prototype. **Conservative
  recommendation:** retain all prototypes and do not add resets or label them
  unused until their intended consumers are identified. See the
  [mobile catalog](../../../areas/mob/random.mob#L14),
  [local resets](../../../areas/zon/random.zon#L10), and
  [random mobile selection](../../../src/world/random.mob.c#L585).

- **RANDOMNESS-Q04 — Are the two fixed merchants at rooms #105301 and #105302
  intended services, and are their imported goods the intended source for
  objects #6008 and #9225?** Their reset rooms sit above the generator's
  `RANDOM_END_ROOM` value of #105000; the zone and shop sources configure the
  merchants and list imported stock. **Conservative recommendation:** preserve
  both reset and shop records, and do not infer present availability, alter
  placement, or duplicate imported prototypes without an owner decision. See
  [merchant resets and stock](../../../areas/zon/random.zon#L10),
  [shop #100013](../../../areas/shp/random.shp#L1),
  [shop #100015](../../../areas/shp/random.shp#L38), and
  [random room range](../../../src/world/random.zone.h#L27).

- **RANDOMNESS-Q05 — What exact buyer condition is meant by each shop's
  refusal, “I do not sell to your people!”?** Both shop records contain this
  refusal, but the text alone does not identify a race, faction, or other
  eligibility rule. **Conservative recommendation:** preserve the authored
  message and leave its predicate unspecified until the owner names the intended
  buyer group. See the [first refusal](../../../areas/shp/random.shp#L36) and
  [second refusal](../../../areas/shp/random.shp#L71).

- **RANDOMNESS-Q06 — Are the lost peasant and travelling gnome descriptions
  ambient character flavor, or should either begin a rescue, escort, or return-
  home story?** Their profiles mention being lost and finding a way home, while
  the coverage register records no local QST giver or story sidecar.
  **Conservative recommendation:** treat the text as atmosphere unless the owner
  defines an attainable objective and outcome; do not imply a rescue endpoint
  or completed return. See the [peasant and gnome profiles](../../../areas/mob/random.mob#L14)
  and [coverage row](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L247).

- **RANDOMNESS-Q07 — Is the `random` entry in `areas/qst/AREA` intentional
  even though `areas/qst/random.qst` is absent and the coverage row records no
  local QST givers?** **Conservative recommendation:** preserve the manifest
  entry and the current lack of local quest text until the owner clarifies the
  intended index membership; do not invent a quest or create an empty QST file.
  See the [QST manifest entry](../../../areas/qst/AREA#L323) and
  [coverage row](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L247).

These questions record unresolved owner intent. They do not assert that the
generator or either shop is currently running.
