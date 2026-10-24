# VoDsGH source audit

Canonical key: `component:vodgh`. Coverage data row 468 / CSV line 469;
candidate dependency data row 117 / CSV line 118. Reviewed October 12, 2026
(Checkpoint 555) against checkout
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static source, graph,
typed-reference, identity, and followed-history review. It does not establish
live access, portal behavior, shop state, or route safety.

## Source inventory and authored story

The same-stem sources are [vodgh.wld](../../../areas/wld/vodgh.wld#L1),
[vodgh.mob](../../../areas/mob/vodgh.mob#L1),
[vodgh.obj](../../../areas/obj/vodgh.obj#L1),
[vodgh.zon](../../../areas/zon/vodgh.zon#L1), and
[vodgh.shp](../../../areas/shp/vodgh.shp#L1). There is no same-stem QST or
story sidecar. `vodgh` is absent from [areas/AREA](../../../areas/AREA) and
`areas/qst/AREA`; the current `areas/RANDOM_AREA` is empty. The ZON header is
#303, title “VoDsGH,” top VNUM #30313, reset mode 0.

The WLD defines 14 rooms #30300–#30313, all with non-empty long descriptions
and 14 distinct normalized room titles. Rooms #30300–#30309 describe a
Dragon/Veil of Darkness cave and guildhall: a marked entrance after a wyrm's
slaying, a main gathering cavern, barracks, treasure, torture and meditation
rooms, and a controlled rift. Rooms #30310–#30313 describe a guarded jungle
valley, canopy, hollowed tree, and waterfall. The two room groups have no
local WLD path between them; that topology does not establish whether they
were authored as one story package or combined for another reason. No formal
quest contract or separate story brief is present in the same-stem sources.

The MOB file contains five prototypes #30300–#30304: four golem variants and
the Orc hermit #30301. The OBJ file defines two type-25 portal objects
#30300/#30301. The ZON includes five object resets importing Player Castles
objects #11061, #11111, and #11013; two local portal-object resets; nine M
commands across the two room groups; and eleven G inventory-object references
for the hermit. Object #6089 is one of those G/shop references but has no
prototype in the checked-in OBJ sources. The SHP file has one shop keyed by
mobile #30301 at room #30306, with eleven inventory entries. These are source
declarations only while `vodgh` remains outside the loader manifest.

## Authored WLD graph and map boundaries

The WLD contains 26 exit records: 24 target another `vodgh` room, one exits up
from #30300 to unregistered `ud_map2` room #1123336, and one exits up from
#30310 to active Magma room #142034. The #30300 destination has no return exit
from that exact `ud_map2` room. The Magma destination has no active WLD return
to #30310. All targets are positive and resolve to a room in a checked-in WLD
source; there are no declared exit flags or key fields, fall-chance fields, or
current fields.

The local graph has two weak components of ten and four rooms. From #30300,
raw directed traversal reaches all ten rooms in its component, and reverse
traversal also reaches all ten. The strongly connected component sizes are
10 and 4; each weak component is strongly connected. This is authored
topology only, not evidence that the unregistered map loads or that the exits
are safe or usable.

No active WLD exit targets #30300–#30313. One unregistered WLD exit from
`ud_map2` #1126542 down (`D5`) targets shared room number #30300
([source exit](../../../areas/wld/ud_map2.wld#L80541)). `bb_gh.wld`,
`venom.wld`, and `vodgh.wld` all define #30300, so this source record cannot
identify the intended map. It numerically pairs with `bb_gh.wld` #30300 up to
#1126542, while `vodgh.wld` #30300 points up to the different `ud_map2` room
#1123336. This is not active ingress.

The #30310 up exit to active Magma #142034 is the Vodgh source-only route
already recorded in Checkpoint 145. It is not a new route for the curated
global tally. The other outbound direction ends in unregistered `ud_map2`,
and the ambiguous `ud_map2` inbound target is also source-only. No route is
added to the current 11,993 overall / 415 from Surf totals.

## Room and prototype identity

All 14 `vodgh.wld` room VNUMs #30300–#30313 also occur in `bb_gh.wld`, which
defines 44 rooms #30300–#30343. The first ten `vodgh` room numbers also occur
in `venom.wld`. Full WLD room-block comparisons found no identical records
across these sources. Thus #30310–#30313 are unique relative to Venom but still
share their VNUMs and competing room identities with BB GH.

`vodgh.mob` #30300–#30302 also occur in both `bb_gh.mob` and `venom.mob`; no
full mobile records are identical. `vodgh` #30303/#30304 do not overlap those
two mobile files. `vodgh.obj` #30300/#30301 also occur in `bb_gh.obj`, with no
identical full object records. The three ZON files use header #303, while the
normalized title “VoDsGH” occurs once among the 442 current ZON titles.
Numeric overlap is not evidence that these authored maps or prototypes are
one source or that one replaced another.

Followed same-stem history reaches the 2009 area-tree move, a 2011 WLD typo,
misflag, and stat update, a 2012 reset-format update to MOB/OBJ/ZON, and 2013
updates to all five source files. No tracked alternate-stem predecessor,
successor, rename, retirement, or whole-zone replacement was found. No
`vodgh` entry was found in the checked-in full-area or QST manifests.

## Typed references and bounded dependency closure

| Source or role | Finding | Classification |
| --- | --- | --- |
| Active WLD inbound | No active WLD direction targets #30300–#30313. | Candidate-specific active WLD ingress is zero. |
| WLD boundaries | `vodgh` #30300 up to `ud_map2` #1123336 and #30310 up to Magma #142034; `ud_map2` #1126542 down to shared #30300. | Both `vodgh` exits are one-way in checked-in WLD data. Magma is an already-counted source-only active destination; the `ud_map2` pair is source-only and the inbound map identity is ambiguous. |
| ZON object imports | Five O resets reference Player Castles #11061/#11111/#11013 at rooms #30300/#30302/#30303/#30311/#30312. | Preserve the five imported reset rows and their external prototype ownership. |
| Local portal objects | Object #30301 is reset at #30309 and targets #30313; object #30300 is reset at #30313 and targets #30309. | The authored pair is reciprocal by number, but #30309 is shared by BB GH/Venom/VoDs GH and #30313 is shared by BB GH/VoDs GH. It does not prove destination identity or runtime behavior. |
| Other type-25 targets | Sunhall object #30202 targets #30302; BB GH portals also target numbers in the overlapping room range. | These references do not identify which competing room record owns a target number. |
| ZON mobile resets | Nine M commands reference local mobile IDs #30300–#30302; repeated golem commands appear at both #30300 and #30310. | Preserve all reset records while zone ownership and reset semantics remain unresolved. |
| Hermit inventory | Eleven G commands refer to object IDs 108, 109, 111, 3050, 377, 398, 412, 53, 364, 346, and 6089. | Ten IDs have checked-in OBJ definitions; #6089 has none. Some resolved numbers have multiple OBJ definitions. |
| SHP | Shop key #30301 is located at #30306 and lists eleven object IDs, including #6089. BB GH has a separate #30301 shop at #30302. | The key is a mobile VNUM shared by three map sources; do not infer a live keeper/shop association from these files alone. |
| Compiled room binding | [`specs.assign.c`](../../../src/specs/specs.assign.c#L2455) binds an inn callback to raw room #30303. | That room number occurs in all three competing maps; no live loader identity was checked. |
| Quest/story files | No same-stem QST/story sidecar or exact QST room reference was found. | No formal quest contract was identified in this bounded source scan. |

## Disposition

Keep `vodgh` **RETAIN IN PLACE** outside `areas/AREA` and `areas/qst/AREA`.
Preserve its five source files, both disconnected WLD clusters, both boundary
exits, the ambiguous `ud_map2` target, all imported Player Castles resets,
mobile/object/shop identities, and the compiled #30303 binding. Do not
activate, merge, regenerate, move, or archive until map and guild ownership,
the shared #303 VNUM family, cross-map routes, portal/shop targets, imported
objects, editor/export authority, deployment, runtime, and persisted identity
are resolved. See [builder questions](VODGH_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-555-vodgh-source-graph-and-boundaries),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-555-vodgh-shares-room-and-prototype-numbers-with-two-guildhalls),
[coverage row 468](ZONE_COVERAGE_REGISTER.csv#L469), and
[candidate dependency row 117](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L118).
