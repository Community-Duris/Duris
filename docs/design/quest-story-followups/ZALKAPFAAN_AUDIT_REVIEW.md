# Zalkapfaan repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, title ownership, and bounded source history for the [source-comprehensive Zalkapfaan dossier](../zone-stories/ZALKAPFAAN.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; no area or gameplay source is changed.

## Active graph and boundary records

The active `headless` WLD has 60 rooms and 132 authored exits, all resolving: 130 local and two outbound. The weak graph is one 60-room component. With authored local exits treated as available, traversal from #2730 reaches 54 rooms and 59 rooms have an authored path back to #2730. The six staging rooms #2784–#2789 are outside that root traversal; #2789 also has no authored return to the root. The dossier separately models door keys and spoken passwords and describes conditional mobile roaming, which the raw WLD graph does not prove.

Both outbound edges have reciprocal Surface returns:

| Zalkapfaan boundary | Reciprocal record |
| --- | --- |
| #2730 north → Surface #596968 | Surface #596968 south → #2730. See [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L43), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1566), and [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L169). |
| #2738 south → Surface #597768 | Surface #597768 north → #2738. See [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L44), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1571), and [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L169). |

There are two active inbound records, two outbound records, and two reverse records, forming two reciprocal pairs. The WLD has one current field and no fall-chance fields. The route-specific 40-, 52-, and 54-room scenarios in the dossier account for doors, usable keys, and passwords; they remain conditional source analysis rather than live availability or safety findings.

## Inactive-WLD approach and attic evidence

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. No inactive source room overlaps the 60 active Zalkapfaan room VNUMs. It found one non-overlapping source-only approach:

| Inactive source-only approach | Evidence |
| --- | --- |
| `surf.wld` #221513 west → Zalkapfaan #2730 | [Source exit](../../../areas/wld/surf.wld#L451337). |

The approach is not active ingress because `surf.wld` is not an active world contributor. It raises the curated source-only approach total from 2,007 to 2,008 and the reviewed Surf subset from 151 to 152. Preserve the edge with its source map and keep `surf` / `surface2011` **RETAIN** pending map ownership and remaining dependency/tool closure. Active `headless` remains **DO NOT MOVE**.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Imported reset objects | The local reset file imports broken earth #193 into room #2747 ([reset](../../../areas/zon/headless.zon#L146), [Heavens definition](../../../areas/obj/heavens.obj#L2294)); pool liquid #67 into #2781 ([reset](../../../areas/zon/headless.zon#L149), [Heavens definition](../../../areas/obj/heavens.obj#L940)); a rune-covered stone #359 onto Kal-Zerr ([reset](../../../areas/zon/headless.zon#L309), [Heavens definition](../../../areas/obj/heavens.obj#L3725)); and the Zalkapfaan memory #55189 onto him ([reset](../../../areas/zon/headless.zon#L310), [White Harbor definition](../../../areas/obj/wh.obj#L2446)). Their compiled mining, pool, and epic-node behavior is mapped in the dossier; the imported memory has no selected special or local native return. |
| Outside typed consumers and reset references | The bounded active and unregistered scan found no outside exact QST item consumer, SHP product, `.zon` reset role, or story-sidecar typed VNUM reference to a local Zalkapfaan object, mobile, or room. The local QST and sidecar item inputs resolve to local prototypes. |
| Other sidecar number matches | The only raw foreign sidecar number hits are #2788 embedded in an Alatorin string ID ([line](../../../areas/story/alatorin.story.json#L2010)) and #2787 embedded in a White Harbor request ID ([line](../../../areas/story/wh.story.json#L874)); neither is a typed VNUM field. |
| Portals and duplicate prototypes | The bounded scan found no external duplicate mobile/object prototype definitions and no type-25 portal targeting Zalkapfaan rooms among 872 scanned declarations. Zalkapfaan has no local SHP source. |
| Compiled numeric match | The only direct C/C++ numeric match in the bounded query is the Commodore #2733 row in the epic ship-damage-control teacher table ([source](../../../src/classes/epic_skills.c#L218)), which the dossier identifies as an installed service binding rather than a story objective. |

The source-defined optional mining, charisma-pool, epic-node, and teacher interactions are recorded in the dossier. Typed source references and compiled service behavior do not establish current actor/item stock, service success, or a played outcome.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “Zalkapfaan, City of the Headless Horde” has one exact owner among 351 active `.zon` titles: `headless` ([title source](../../../areas/zon/headless.zon#L2), [coverage row](ZONE_COVERAGE_REGISTER.csv#L120)). Followed WLD, ZON, MOB, OBJ, and QST histories remain on the `headless` stem from the 2009 area relocation through same-stem edits. The WLD and OBJ histories include later 2026 edits; the story sidecar was added on 2026-10-06 in commit `500462537`. No alternate-stem rename, explicit predecessor, or whole-zone replacement claim appears in these bounded paths. Keep active `headless` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. No source placement changed.

## Builder recommendations

1. Preserve both reciprocal Surface pairs and keep their endpoints distinct from the six staging rooms outside raw traversal from #2730.
2. Preserve `surf.wld` #221513 as source-only map evidence. It does not make Zalkapfaan reachable through the active loader or clear a Surf path move.
3. Keep the imported object identities #193, #67, #359, and #55189 attached to their current reset roles and prototype owners until the source-map intent is resolved.
4. Keep spoken-password, held-key, ordinary roaming, and optional service routes separate from guaranteed live access or story completion; the dossier records their specific gates and conditions.
