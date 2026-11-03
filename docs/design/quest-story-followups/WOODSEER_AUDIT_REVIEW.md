# Woodseer: repository-wide connectivity and dependency audit

**Checkpoint 76; priority 116 of the original 220-zone queue.** This supplement
reconciles the source-comprehensive [Woodseer dossier](../zone-stories/WOODSEER.md)
with the active WLD graph, inactive WLD inputs, typed dependencies, direct
compiled references, title, and bounded source history. It is a static source
review; normal-play access, safety, and gameplay qualification remain separate.

## Active WLD graph

The 351 active WLD contributors parsed without issues. Woodseer contains 453
rooms and 1,041 exits; 1,040 targets resolve, leaving the existing positive
unresolved exit from room #16500 up to #154581. Its local weak graph has two
components, the largest with 452 rooms.

The active boundary has eight incoming records and seven outgoing records.
Six incoming/outgoing pairs are reciprocal: two Surface pairs, three Rift
Jungle pairs, and the Pineholl pair. Heavens room #25 south enters Woodseer
#16558 and Krimman room #16486 west enters #16660 without reverse exits. The
remaining outgoing edge, #16500 up to #154581, is unresolved in the active
manifest. The target exists as an ocean record in unregistered
[`Duris3.wld`](../../../areas/wld/Duris3.wld#L1146202); the normal generator
omits it and the runtime prunes the unresolved exit. The exact unresolved row
is [record 42](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L42).

| Boundary records | Evidence |
| --- | --- |
| Active Woodseer outbounds: #16891 north to Surface #568910, #16915 west to Surface #569309, #16950–#16952 east to Rift Jungle #80000–#80002, and #16960 north to Pineholl #16065 | [Surface #568910](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L296), [Surface #569309](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L297), [Rift #80000](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L298), [Rift #80001](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L299), [Rift #80002](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L300), [Pineholl](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L301) |
| Active inbounds: Heavens #25 to #16558; Krimman #16486 to #16660; Pineholl #16065 to #16960; three Rift Jungle arrivals; two Surface arrivals | [Heavens](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L5), [Krimman](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L295), [Pineholl](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L279), [Rift #80000/#80001/#80002](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L969), [Surface #568910](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1482), [Surface #569309](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1483) |

The coverage row records 1,041 authored exits, 1,040 resolved exits, eight
active inbounds, six resolved outbounds, six reverse records, and one positive
unresolved target. The unresolved Duris3 approach remains separate from the
resolved active boundary.

## Inactive WLD approaches

All 95 inactive WLD files parsed without errors: 527,791 rooms and 2,080,773
exits. No inactive source room overlaps an active room VNUM. Two
source-only `surf.wld` exits lead into Woodseer:

| Inactive source | Target | Evidence |
| --- | --- | --- |
| Room #226868 south (direction 2) | Woodseer #16891 | [`surf.wld`](../../../areas/wld/surf.wld#L563628) |
| Room #228065 east (direction 1) | Woodseer #16915 | [`surf.wld`](../../../areas/wld/surf.wld#L588733) |

These records are not active ingress. They raise the curated source-only
inbound total from 677 to 679 and the individually reviewed Surf subset from
107 to 109. They do not change Woodseer's active graph counts.

## Typed external references

| Source and role | Classification |
| --- | --- |
| [`surfacemini.qst`](../../../areas/qst/surfacemini.qst#L310), `G I 16635` | Active Surface Mini hermit QA302 item requirement for Woodseer's salmon #16635. The existing foreign Surface Mini story owns that receipt. |
| [`surfacemini.story.json`](../../../areas/story/surfacemini.story.json#L214) contact #16538 and [step material #16635](../../../areas/story/surfacemini.story.json#L547) | Two exact structured VNUM references across 231 parsed sidecars; no parse issues. The sidecar identifies Woodseer's fish vendor and salmon supply. |
| [`alatorin.zon`](../../../areas/zon/alatorin.zon#L2211), `M` mobile #16695 | Active Alatorin reset placement for Woodseer's mailman prototype. |
| [`alatorin.zon`](../../../areas/zon/alatorin.zon#L2820), `G` object #16599 twice ([second row](../../../areas/zon/alatorin.zon#L2821)) | Two active Alatorin resets for Woodseer's blackened dagger object. This object ID is distinct from Woodseer mobile #16599. |
| [`solstice_gh.shp`](../../../areas/shp/solstice_gh.shp#L19), shop #29100 product #16649; [`solstice_gh.zon`](../../../areas/zon/solstice_gh.zon#L34), `G` object #16649 | Source-only references from an unregistered guild hall, not active shop stock or resets. #16649 is Lomar's local mandolin/instrument. |

No external mobile/object prototype definition matched Woodseer, and none of
the 872 scanned external type-25 declarations targets it. The raw text search
also returned a non-VNUM substring in a foreign story identifier; it is not an
additional structured reference.

## Direct compiled references

The existing dossier classifies the code behavior. Repository numeric scans
also resolve these direct bindings:

| Reference | Source and disposition |
| --- | --- |
| CHAOS bard starter object #16530 | [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L200), repeated in the [second profile](../../../src/account/chaos_eq_data.h#L446). |
| Restricted CHAOS test room #16551 | [`chaos.c`](../../../src/combat/chaos.c#L23); the helper resolves the listed room through `real_room()`. |
| GoodHome configuration | [`justice.c`](../../../src/combat/justice.c#L61) lists report room #16512; guard rooms #16506, #16522, #16526, #16653, #16551; guard mobile #16705; and jail #16513. |
| Githzerai plane-shift room pool | [`innates.c`](../../../src/classes/innates.c#L2010) includes #16805 and selects from this race-specific list at lines 2040–2045. |
| Postmaster mobile #16695 | [`mail.c`](../../../src/cmd/mail.c#L135), in addition to its local and Alatorin reset placements. |
| Hometown and Woodseer room-placement values | [`constant.c`](../../../src/core/constant.c#L1727) lists hometown #16558; its Woodseer table at [lines 1916–1919](../../../src/core/constant.c#L1916) includes #16558, #16653, #16676, and #16675. |
| Auction room #16885 and inn #16558 | [`auction_room_registry.c`](../../../src/economy/auction_room_registry.c#L9) and [`guildhall.h`](../../../src/guild/guildhall.h#L38). |
| Bartender mobile #16553 | [`specs.assign.c`](../../../src/specs/specs.assign.c#L732) and [lines 782–785](../../../src/specs/specs.assign.c#L782) assign the same `world_quest` special five times; the existing dossier records its single secondary shop behavior. |
| Guild guard mobile #16501 and artifact object #16904 | [`specs.assign.c`](../../../src/specs/specs.assign.c#L923) assigns `guild_guard`; [line 1403](../../../src/specs/specs.assign.c#L1403) binds `artifact_invisible`. The [guard case](../../../src/specs/specs.guards.c#L107) is dormant for all 17 maintained local guard births, and the dossier documents the object's incompatible wear slot. |
| Inn room #16558 and pet shops #16886 | [`specs.assign.c`](../../../src/specs/specs.assign.c#L2537) and [the next assignment](../../../src/specs/specs.assign.c#L2538). |

## Title, history, and disposition

The color-coded active title is [`Woodseer`](../../../areas/zon/woodseer.zon#L2).
Its normalized title is unique among 351 active contributors. The global WLD
duplicate-room scan found only room #51800 shared by Hell Two and Hell Three;
Woodseer has no duplicate room ID. The curated duplicate/replacement register
contains no Woodseer predecessor or replacement entry, and bounded
`git log --follow` histories keep the WLD/ZON sources on their existing
`woodseer` paths back to the 2009 area-tree move.

The WLD history reaches the 2009-08-12 area-tree move and its latest tracked
edit is 2026-06-09; the ZON history reaches the same tree move and latest
tracked edit is 2026-08-31. These bounded paths identify maintenance on the
same source stem but no distinct successor area.

Woodseer remains a current, un-commented [`areas/AREA` contributor](../../../areas/AREA#L90)
and is **DO NOT MOVE**. The source-only approaches, compiled bindings, and
foreign references do not establish a replacement, an ordinary player route,
or safe traversal. Door/reset state, dynamic travel, editor/tooling closure,
persisted references, and played qualification remain pending.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 436: builder question review

Twenty source-backed questions and recommended resolutions now gather
Woodseer's open request, route, shop, foreign-receipt, service, and stale
binding decisions in the [builder question review](../zone-stories/WOODSEER_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The coverage inventory also records the twenty maintained local SHP records,
keyed from #16508 through #16691. The existing graph, title/history, and typed
closure findings above remain unchanged; no source or runtime behavior changed.
