# The Halfling Silver Mine: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Mining source
dossier](../zone-stories/HALFLING_SILVER_MINE.md). It reconciles active and
source-only WLD edges, raw directed reach, typed reset references, same-number
room/object records, title ownership, and bounded same-stem history. Static
source review does not establish that a player can traverse a route or
complete an interaction.

## Active WLD boundaries and directed reach

Active `mining`, zone 49, has 69 rooms from #4900 through #4968. Its 153
authored exits all resolve: 152 are local and one leaves the area. The local
map is one weak component. Raw directed traversal from #4900 reaches 64 rooms
and has a return path from 68 rooms. Rooms #4964–#4968 are outside root-forward
reach, while #4968 has no authored path back to #4900. No internal exit has
flag 8. These counts describe the authored WLD graph, not runtime access.

The only active boundary is reciprocal with Surface: Mining #4900 west to
Surface #566915 and Surface #566915 east to Mining #4900
([pair rows 268 and 541](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L268), [Mining
room](../../../areas/wld/mining.wld#L1), [Surface room](../../../areas/wld/surface.wld#L1404687)).

## Inactive WLD sweep and overlapping sources

All 95 WLD files outside the active areas/AREA list parsed cleanly: 527,791
rooms and 2,080,773 exits. The sweep found 160 exit records targeting Mining
room VNUMs and 69 source-room VNUM overlaps: 49 in `alterian_mountains.wld`
and 20 in `alterian_wilderness.wld`. Those maps contain separate source room
records with the same numbers as Mining; their records are not active
ingress, and the numeric overlap does not establish that they are Mining
rooms or replacement evidence.

Nine inbound records have non-overlapping source rooms and remain in the
curated source-only set:

| Source record | Exit into Mining | Evidence |
| --- | --- | --- |
| `alterian_mountains.wld` #4969 | west to #4968 | [Source room](../../../areas/wld/alterian_mountains.wld#L1046) |
| `alterian_mountains.wld` #4981 | down to #4924 | [Source room](../../../areas/wld/alterian_mountains.wld#L1344) |
| `alterian_mountains.wld` #4986 | up to #4925 | [Source room](../../../areas/wld/alterian_mountains.wld#L1446) |
| `alterian_wilderness.wld` #4897 | east to #4901 | [Source room](../../../areas/wld/alterian_wilderness.wld#L2690) |
| `alterian_wilderness.wld` #4898 | east to #4900 | [Source room](../../../areas/wld/alterian_wilderness.wld#L2713) |
| `alterian_wilderness.wld` #4899 | north to #4901, east to #4908, and south to #4900 | [Source room](../../../areas/wld/alterian_wilderness.wld#L2736) |
| `surf.wld` #229817 | west to #4915 | [Source room](../../../areas/wld/surf.wld#L625485) |

The other 151 target matches come from source rooms whose VNUMs overlap the
Mining range and are excluded from this curated source-only count. The nine
records raise curated totals to 3,346 overall, 169 for the reviewed Surf
subset, and 15 for `ud_map2`. None is active ingress.

## Exact typed and structured references

The selected scans found no external QST item command, SHP product, typed
story field, or active reset-role reference into a Mining room, mobile, or
object. The local `mining.qst` exists, but `mining` is not listed in
`areas/qst/AREA`; the local contract is not active QST input through that
manifest. Mining has no local SHP source. The local QST and story sidecar
contain no item VNUM outside Mining's local object set.

Two unregistered ZON files contain M-reset destination fields in the Mining
numeric room range: 16 records in `alterian_mountains.zon` and 17 in
`alterian_wilderness.zon` ([Mountain example](../../../areas/zon/alterian_mountains.zon#L10),
[Wilderness example](../../../areas/zon/alterian_wilderness.zon#L153)). These
files are outside areas/AREA and are source-only. Their destination VNUMs
overlap with their respective WLD source maps, so the numeric references do
not establish active resets into Mining.

One unregistered object file defines a VNUM also used by Mining. Active
`mining.obj` #4900 is a wooden crate, while unregistered
`alterian_mountains.obj` #4900 is a separate “nothing” prototype
([Mining object](../../../areas/obj/mining.obj#L1), [Alterian object](../../../areas/obj/alterian_mountains.obj#L1)).
The external prototype scan found no duplicate mobiles. Preserve this exact
candidate collision without treating the object records as one identity.

The two numeric hits in other story sidecars occur only inside string IDs:
Alatorin `exchange-83302-e10450a4931f` contains token #4931, and Shady Grove
`request-97510-4963db257809` contains token #4963
([Alatorin ID](../../../areas/story/alatorin.story.json#L2652), [Shady ID](../../../areas/story/shady.story.json#L134)).
They are not room/mobile/object references. Two direct compiled-number scan
hits were also false positives: #4954 occurs within floating-point data and
#4904/#4925 occur in a source comment describing another file's line range
([float data](../../../src/classes/memorize.c#L83), [comment](../../../src/kingdom/kingdom_guards.c#L1223)).

No external type-25 target points into Mining among 872 scanned declarations.
All 231 other story sidecars parsed without errors; their two #4931/#4963
tokens are the string IDs above, with no typed VNUM fields found.

## Title and bounded history

The normalized title “The Halfling Silver Mine” belongs only to active stem
`mining` among the 442 scanned `.zon` files
([title source](../../../areas/zon/mining.zon#L2)). Followed `mining.wld`
and `mining.zon` histories begin with the same-stem August 2015 import and
continue through later same-stem edits, including the 2025 cleanup. The
bounded paths establish no alternate-stem rename, distinct predecessor, or
whole-zone replacement. See the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active `mining` DO NOT MOVE. Preserve its reciprocal Surface boundary,
all nine non-overlapping source-only approaches, the two Alterian ZON
reference sets, and the #4900 object collision with the unregistered
Alterian Mountains source. Keep `alterian_mountains`,
`alterian_wilderness`, and `surf` RETAIN IN PLACE; preserve their existing
candidate rows. No new candidate row is added. Editor, administrative,
installed-data, persisted-player, indirect-runtime, and actual traversal
closure remain outside this bounded scan; no gameplay, migration, or source
change is proposed.


## Checkpoint 277: direct builder and owner questions

These questions focus the remaining decisions around Mining's one exact return, the missing legacy QST index entry, inactive Alterian records, and the proposed industrial, planar, and safety stories. The recommendations preserve current source ownership and the native Q8 recipe until a specific design is selected.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Is Priest Kitan's exact whip-for-badge return intended to be the only accepted local contract? | [Q8 exchange source](../../../areas/qst/mining.qst#L11), [native exchange review](../zone-stories/HALFLING_SILVER_MINE.md#L9) | Keep one accepted exchange and treat shaft, vault, engineer, and planar story routes as optional unless a separate endpoint is selected. |
| 2 | Should supplied exact whip #4911 remain eligible without a personal kill or original recovery of Balor's equipped whip? | [Balor reset](../../../areas/zon/mining.zon#L169), [equipped whip](../../../areas/zon/mining.zon#L170), [provenance boundary](../zone-stories/HALFLING_SILVER_MINE.md#L11) | Preserve item-only acceptance; track source combat and personal recovery as separate optional results. |
| 3 | Which source and transfer rules should govern optional first-recovery credit for the whip? | [source ownership follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L63), [exact receipt boundary](../zone-stories/HALFLING_SILVER_MINE.md#L65) | Bind actual Balor generation, equipped root, actor, transfer, and custody before crediting provenance. |
| 4 | Is mining.qst meant to be loaded despite its absence from the legacy qst/AREA index? | [local QST](../../../areas/qst/mining.qst#L1), [index scan](#exact-typed-and-structured-references), [scope methodology](SCOPE_AND_COVERAGE_REGISTER.md) | Verify the production loader path; do not conclude runtime availability from this legacy index alone. |
| 5 | Should the three room dialogues in Kitan's request remain one addressed family with no separate per-word achievements? | [quest response](../../../areas/qst/mining.qst#L1), [dialogue follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L61) | Preserve one greeting family and record the accepted whip return separately from ambient or repeated words. |
| 6 | What reset and accounting conditions should support daily availability for Kitan and Balor? | [zone reset](../../../areas/zon/mining.zon#L1), [stock and daily policy](../zone-stories/HALFLING_SILVER_MINE.md#L52) | Keep daily policy disabled until actual source/giver renewal and active READY accounting are established. |
| 7 | Should the nine source-only approaches from Surf and Alterian remain separate from active Surface access? | [inactive WLD records](#inactive-wld-sweep-and-overlapping-sources), [existing candidates](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) | Preserve the reciprocal Surface route and all nine source-only records; do not expose them as active entrance hints. |
| 8 | Are the same-numbered Alterian rooms intended to remain distinct from the active Mining rooms? | [overlap census](#inactive-wld-sweep-and-overlapping-sources), [separate identities](../zone-stories/HALFLING_SILVER_MINE.md#L15) | Keep all 69 overlapping room identities separate; do not co-load, rename, or reassign VNUMs from numeric overlap alone. |
| 9 | Should the unregistered Alterian reset files continue to reference Mining-range room numbers while remaining outside the active manifest? | [reset source references](../../../areas/zon/alterian_mountains.zon#L10), [second reset source](../../../areas/zon/alterian_wilderness.zon#L153) | Preserve those source-only declarations with their map owners; do not interpret them as active Mining resets. |
| 10 | Should duplicate object #4900 in unregistered Alterian Mountains remain distinct from Mining's wooden crate? | [active crate](../../../areas/obj/mining.obj#L1), [Alterian prototype](../../../areas/obj/alterian_mountains.obj#L1) | Preserve distinct source identities and the existing candidate collision record. |
| 11 | Is the badge #4912 intended to provide any lasting guild, blessing, or planar effect after Q8? | [badge prototype](../zone-stories/HALFLING_SILVER_MINE.md#L11), [effect boundary](../zone-stories/HALFLING_SILVER_MINE.md#L64) | Qualify actual equipped-use effects and beneficiary before adding a post-quest milestone. |
| 12 | Should the engineer's key and hidden desk route remain an optional access story instead of a prerequisite for the priest's exchange? | [desk and key route](../zone-stories/HALFLING_SILVER_MINE.md#L17), [recipe boundary](../zone-stories/HALFLING_SILVER_MINE.md#L9) | Keep the native Q8 independent; record source key, desk unlock, child key, and trapdoor transitions separately. |
| 13 | What fall and heat policy should apply through the shaft and the distinct Fireplane rooms? | [shaft and heat analysis](../zone-stories/HALFLING_SILVER_MINE.md#L21), [fall follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L79) | Preserve actual sector and movement rules; qualify actor eligibility, scheduled impact, survival, and return before describing safety. |
| 14 | Are the asymmetrical vault keys, hidden openings, and locked resets intentional on both faces? | [vault door analysis](../zone-stories/HALFLING_SILVER_MINE.md#L23), [key states](../zone-stories/HALFLING_SILVER_MINE.md#L27) | Keep current flags until both directions are qualified with real key, pick, open, and shared-state behavior. |
| 15 | Should the random-grid rooms and janitor stock remain inaccessible to ordinary entrance traversal? | [ordinary graph limits](../zone-stories/HALFLING_SILVER_MINE.md#L27), [grid-stock follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L89) | Preserve existing controller ownership and restrictions; do not open a shortcut simply to expose stock. |
| 16 | Which controller or maintenance result should make the mine's carts, tracks, lift, or ventilation prose into a playable industrial story? | [industrial sources](../zone-stories/HALFLING_SILVER_MINE.md#L31), [work follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L98) | Require an actual caller, input/output, beneficiary, and persistent equipment or safety state before creating work credit. |
| 17 | Which worker welfare, leadership, or sanitation stories should receive named beneficiaries and lasting outcomes? | [worker leads](../zone-stories/HALFLING_SILVER_MINE.md#L45), [welfare follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L100) | Keep visits, kills, and generic food custody outside completion until builders select consent, care, and durable state rules. |
| 18 | Should the order's planar investigation end in a defined controller or boundary resolution? | [planar story plan](../zone-stories/HALFLING_SILVER_MINE.md#L40), [planar follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L101) | Define grounded clues, choice, actual target state, and resident safety; the whip and badge do not prove planar closure. |
| 19 | Does the vault's treasury or golden boomerang need an accepted recipient or property-restoration outcome? | [vault and container stock](../zone-stories/HALFLING_SILVER_MINE.md#L29), [property follow-up](../zone-stories/HALFLING_SILVER_MINE.md#L93) | Count only owned item retrieval and a selected beneficiary/property result; room prose does not create coins or restoration credit. |
| 20 | Should the mine remain without a local shop, inn, or paid-work service in the registered source files? | [source inventory](../zone-stories/HALFLING_SILVER_MINE.md#L31), [service boundary](../zone-stories/HALFLING_SILVER_MINE.md#L52) | Keep service claims tied to installed, owned handlers; identify a real shop or work caller before adding purchase or labor stories. |

Checkpoint 277 validation: the twenty questions link to the local QST, ZON, OBJ, unregistered Alterian files, source-comprehensive dossier, and CP162 audit. The coverage row's duplicate disposition text and unstructured dependency status are corrected, and bounded title/history and typed-source closure are marked complete. Existing CP162 route and source-only counts remain unchanged. No area or gameplay source changed; no tests, build, server, database operation, migration, or operational script was run.
