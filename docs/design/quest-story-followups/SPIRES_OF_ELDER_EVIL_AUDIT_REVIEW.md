# The Spires of the Elder Eternal Evil repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, title ownership, and bounded source history for the [source-comprehensive Spires dossier](../zone-stories/SPIRES_OF_ELDER_EVIL.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; no area or gameplay source is changed.

## Active graph and boundary records

The active `eternal` WLD has 333 rooms and 708 authored exits, all 708 resolving and local. It has no active cross-area inbound or outbound WLD records. Its weak graph has 17 components: one of 275 rooms, one of 43, and 15 single-room components. With the three source-marked blocked exits unavailable, directed traversal from #135400 reaches 264 rooms and has authored returns from 274. If those blocked edges are assumed available, it reaches 272 and has authored returns from 275. These raw scenarios do not model key custody, password doors, switches, or live actor/item placement.

Three internal exits carry the blocked flag: #135515 west → #135645, #135523 east → #135639, and #135639 west → #135523 ([source lines](../../../areas/wld/eternal.wld#L2243), [#2392](../../../areas/wld/eternal.wld#L2392), and [#4545](../../../areas/wld/eternal.wld#L4545)). The dossier maps the PULL switches and the separate reset state of their reciprocal sides. The WLD has no fall-chance or current fields. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L90) and [dossier](../zone-stories/SPIRES_OF_ELDER_EVIL.md) for the selected room routes and gates.

## Duris3 source-only approaches and map identity

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. All 333 active Eternal room VNUMs also occur in `Duris3.wld`; the source room at #135400 is titled “The Ocean,” while active `eternal.wld` #135400 is a Spires courtyard ([Duris3 room](../../../areas/wld/Duris3.wld#L743401), [active room](../../../areas/wld/eternal.wld#L1)). These same-numbered source rooms are separate map identity evidence.

The sweep found 1,332 Duris3 exit records targeting Eternal room VNUMs. Of these, 602 begin in non-overlapping source rooms and are counted as source-only approaches; the other 730 begin in the 333 overlapping source-room VNUMs and remain separately classified. The 602 source-only records consist of two 300-exit bands and two endpoint records:

| Inactive source-only Duris3 band | Evidence |
| --- | --- |
| 300 south exits from #135100–#135399 to Eternal #135400–#135699 | [First edge](../../../areas/wld/Duris3.wld#L737113); [last edge](../../../areas/wld/Duris3.wld#L743392). |
| 300 north exits from #135733–#136032 to Eternal #135433–#135732 | [First edge](../../../areas/wld/Duris3.wld#L750398); [last edge](../../../areas/wld/Duris3.wld#L756677). |
| One east exit from #135999 to Eternal #135700 | [Endpoint](../../../areas/wld/Duris3.wld#L755988). |
| One west exit from #135733 to Eternal #135732 | [Endpoint](../../../areas/wld/Duris3.wld#L750410). |

`Duris3` is not an active world contributor, so none of these records is current ingress. The 602 non-overlapping approaches raise the curated source-only total from 2,008 to 2,610; the reviewed Surf subset remains 152. Retain `Duris3` in place pending map-owner and identity review; do not infer replacement from shared VNUMs. Active `eternal` remains **DO NOT MOVE**.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Outside CHAOS equipment table | Four configured CHAOS equipment entries refer to local objects eyepatch #135404 ([definition](../../../areas/obj/eternal.obj#L78); [first table row](../../../src/account/chaos_eq_data.h#L100); [second table row](../../../src/account/chaos_eq_data.h#L177)) and MoonSword #135436 ([definition](../../../areas/obj/eternal.obj#L609); [first table row](../../../src/account/chaos_eq_data.h#L205); [second table row](../../../src/account/chaos_eq_data.h#L321)). These equipment-table references do not define a source, quest handler, or arrival route. |
| Outside QST, SHP, reset, and story references | The bounded active/unregistered scan found no outside exact QST item consumer, SHP product, typed `.zon` reset role, or story-sidecar VNUM reference to local Eternal objects, mobiles, or rooms. Local quest inputs and reset roles resolve to local prototypes; there are no imported reset objects or mobiles. |
| Shops and portals | Eternal has no local SHP source. The bounded scan found no outside duplicate mobile/object prototype definitions and no type-25 portal target into Eternal among 872 scanned declarations. |
| Item source gap preserved in the dossier | Required Kezef hide #135440 has no registered reset/native reward producer or selected compiled producer. The ordinary corpse and CARVE code paths do not create this object; keep its intended source as a builder decision ([dossier follow-up](../zone-stories/SPIRES_OF_ELDER_EVIL.md#L62)). |

Static object, quest, reset, and compiled references do not establish item custody, a live Chaos kit delivery, availability of a route, or a played outcome.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “The Spires of the Elder Eternal Evil” has one exact owner among 351 active `.zon` titles: `eternal` ([title source](../../../areas/zon/eternal.zon#L2), [coverage row](ZONE_COVERAGE_REGISTER.csv#L90)). The followed WLD, MOB, OBJ, ZON, and QST histories begin together in commit `85e5be06c` on 2010-12-15. That commit describes Eternal as a follow-up to Yuan-Ti2 with no direct game connection and for editing purposes. Later history remains on the same `eternal` stem; the story sidecar was added on 2026-10-06 in commit `8d1611158`. This records a design follow-up, not a whole-zone replacement. No alternate-stem rename or explicit predecessor appears in the bounded paths. Keep active `eternal` **DO NOT MOVE** and `Duris3` **RETAIN IN PLACE**. No source placement changed.

## Builder recommendations

1. Keep the 602 non-overlapping Duris3 records classified as source-only and preserve the 730 overlapping-source records as separate map identity evidence. Resolve map ownership before proposing a source move or room merger.
2. Keep the current Eternal WLD separate from the disconnected Duris3 map bands. The inactive records do not create an active entrance.
3. Preserve the three blocked switch-side exits and their distinct reverse/reset states until builders select any gate change.
4. Keep CHAOS kit references #135404 and #135436 documented as configured external item references, separate from Eternal's six native returns.
5. Decide the intended acquisition for Kezef hide #135440 before presenting the Issis turn-in as a complete source journey.
