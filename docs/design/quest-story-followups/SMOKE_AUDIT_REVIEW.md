# Checkpoint 364: Para-Elemental Plane of Smoke audit

Physical coverage line 277, `area:smoke`, is reconciled against the existing [source-comprehensive story dossier](../zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md). This supplement closes the register's static connectivity, typed-dependency, title/history, and builder-question reviews. It does not qualify live travel or gameplay.

## Active source graph

The active [Smoke WLD](../../../areas/wld/smoke.wld) has 153 rooms (#139800–#139952) and 649 directions. Every target is a local Smoke room. There are no active cross-area directions, negative targets, unresolved positive targets, fall markers, or current markers. Exit flags are 645 zero, two state 3, and two state 1; 647 keys are zero and the two state-3 vault directions require #139818. The four flagged directions are the reciprocal vault pair (#139941 north/#139942 south) and reciprocal forge pair (#139942 north/#139945 south); all four have matching local D resets in [smoke.zon](../../../areas/zon/smoke.zon).

The raw local weak graph has components sized 147, 4, 1, and 1. Administrative room #139800 has no authored exit. Fixed type-25 destinations are recorded separately below and in the story dossier; they are not WLD directions.

## Inactive WLD approaches and VNUM collisions

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions without errors. It found two source files whose room VNUMs overlap Smoke. The overlapping VNUM sets are not additive: `forofcon`'s 53 rooms (#139900–#139952) are inside Duris3's 153-room overlap (#139800–#139952).

Inactive [Duris3.wld](../../../areas/wld/Duris3.wld) reuses all 153 Smoke room VNUMs with different room titles (150 are “The Ocean,” two “Mountains,” and one “Grassy Plains”). Its 612 directions that target Smoke divide into 302 directions starting at those colliding Duris3 room IDs and 310 directions from source VNUMs with no active WLD owner. Preserve the 310 distinct source-only directions on the existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36):

- 153 north directions from #140100–#140252 to #139800–#139952;
- 153 south directions from #139500–#139652 to #139800–#139952;
- four endpoints: #139799 east to #139800, #140199 east to #139900, #139953 west to #139952, and #139600 west to #139899.

The 302 directions from same-numbered Duris3 source rooms remain identity-conflicted and are not counted as additional approaches.

Inactive [forofcon.wld](../../../areas/wld/forofcon.wld) reuses 53 Smoke room VNUMs, with different titles such as “A Forest Path,” “Cartographer's Tent,” and “A Supply Store.” Of 109 directions that target Smoke, 104 begin at colliding source IDs and five begin at source VNUMs with no active WLD owner. CP482 re-parsed all 817 current forofcon WLD exits and corrected the earlier 111/106 count; the five unique-source approaches are unchanged. Preserve the five on the existing [forofcon candidate row 45](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L45): #139953 D7 to #139952, #139963 D3 to #139906, #139966 D1 to #139917, #139970 D5 to #139943, and #140020 D7 to #139941. The D numbers are retained as authored, including D7.

Together, Duris3 and forofcon contribute 315 source-only direction records. Their 312 distinct source VNUMs have no active WLD owner. The directions remain pruned with their inactive sources; they are not active ingress and do not change Smoke's zero active WLD inbound count. The curated source-only total rises from 4,533 to 4,848; Surf remains at 282 reviewed records.

The unregistered [forofcon.zon](../../../areas/zon/forofcon.zon) also has 19 M placements to 13 room VNUMs in the Smoke range, 21 D resets to 16 such room VNUMs, and one O placement of object #139928 at #139951. These are reset records from the same source stem as the conflicting forofcon WLD, whose rooms #139900–#139952 have their own different titles. Keep these reset roles with forofcon's inactive source identity; they are not active Smoke mobile/object producers or proof of loaded Smoke-room use.

## Typed dependencies

The existing dossier maps all fourteen local QST blocks and the native item relationships. Bounded exact scans found no outside QST item command targeting a Smoke object, no shop product entry for a Smoke object, and no external story sidecar with an exact Smoke object VNUM. No active external reset role creates a Smoke mobile or object. The `qst/AREA` index does not list `smoke`; no local SHP source exists.

The scan of all 713 active type-25 object prototypes found five fixed destinations into Smoke: three local portal prototypes and exactly two foreign ones. Active [Plane of Air object #24407](../../../areas/obj/plane_air_one.obj#L97), reset at #24405 in [plane_air_one.zon](../../../areas/zon/plane_air_one.zon#L9), targets #139832. Active [Plane of Fire object #25406](../../../areas/obj/plane_fire_one.obj#L83), reset at #25464 in [plane_fire_one.zon](../../../areas/zon/plane_fire_one.zon#L10), targets #139894. The local return portals and route limits remain documented in the [Smoke dossier](../zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md#planar-travel-and-hazards).

Maintained Chaos starter-kit data also refers to three local Smoke objects in seven profile entries across six lines: #139805 at [chaos_eq_data.h:75, 93, 311](../../../src/account/chaos_eq_data.h#L75), #139806 at [lines 84 and 254](../../../src/account/chaos_eq_data.h#L84), and #139826 twice at [line 126](../../../src/account/chaos_eq_data.h#L126). The account-entry path selects these profile entries and loads their VNUMs through `append_chaos_kit_item` in [nanny.c](../../../src/account/nanny.c#L706). This is a maintained code dependency independent of the local quest recipes.

## Title, same-type identity, and tracked history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “The Para-Elemental Plane of Smoke” occurs once among all 442 current ZON titles and once among the 351 active contributors. The nearby active titles “The Para-Elemental Plane of Magma” and “Shairak and Smokeveil Forest” are distinct identities. Same-type scans across current active and source-only files found only the Duris3 and forofcon WLD room overlaps described above; no MOB or OBJ VNUM overlap was found.

Followed WLD, MOB, OBJ, and ZON histories remain on the `smoke` stem and trace the area to the 2012-12-29 zone-addition commit. The local QST path appears in the 2015 quest-item-flag update; the story sidecar was added on 2026-10-04. No alternate-stem rename, distinct predecessor, or whole-zone replacement claim appears in the bounded paths or current manifest. Keep active `smoke` DO NOT MOVE.

## Builder questions and conservative recommendations

The following twenty questions record source-backed decisions for a builder; the recommendations preserve current VNUMs and recipes until each decision is answered.

1. Is administrative room #139800 intended as a player-facing entrance? Keep its empty, exitless source record administrative unless an owner specifies its use.
2. What purpose do the other unfinished or holding rooms serve? Record their intended use before adding encounters, exits, or rewards.
3. Should the four weak WLD components be connected? Keep their authored graph separate unless a route plan names each new direction.
4. Which room is the intended ordinary arrival for the smoke grid? Name an explicit arrival and return route; do not infer one from an inactive edge or a portal destination.
5. Is the Air vortex arrival at #139832 still intended? Preserve the fixed destination from portal #24407 and its documented return pairing.
6. Is the Fire vortex arrival at #139894 still intended? Preserve the fixed destination from portal #25406 and its documented return pairing.
7. Should the local type-25 links count as ordinary map adjacency? Keep them distinct from WLD exits in maps, counts, and reachability claims.
8. What safe return behavior is intended for the exodus room #139944? Specify the live arrival/return contract before changing either portal.
9. Should the smoke, ash, visibility, and breathing prose create mechanics? Keep the prose descriptive until a specific effect and safe-return rule are approved.
10. Should the described cool breeze change the heat or smoke state? Treat it as scenery until a mechanical transition is defined.
11. Should both vault doors and the forge doors continue resetting closed/locked? Preserve the four current flagged directions and matching 100%-chance D resets until an owner approves another state.
12. Should key #139818 break, persist, or be renewable after the vault passage is opened? Define the key lifecycle before adding consumption or repeat-access behavior.
13. What source and proof history should qualify the Ehkahk-heart delivery? Keep the current exact QA28 offer/outcome and do not infer personal combat credit.
14. Should returning the spectacles equip them, cure blindness, or start an escort? Keep QA71's current exact exchange; add any such effect as a separate approved endpoint.
15. Is a captive-rescue campaign intended after Ehkahk's death or Rijak's departure? Do not imply a release, escort, or homecoming until its actors and state changes exist.
16. Should Erk's ring/earring and equipment conversions remain services without achievement or daily credit? Preserve their current service classification unless a distinct progression rule is approved.
17. Does the great forge need a terrain or heat effect? Define its flags and exposure rules before representing the current room as mechanically hazardous.
18. Should Fire Elemental #139824 keep its copied Air description? Approve zone-specific prose while preserving the existing mobile identity and reset.
19. Which permanent-stat bundle, if any, should greatsword #139831 deliver? Specify the actual stat changes before changing its current recipe.
20. What wear slot should earring #139823 use, and which displayed names/aliases should be corrected (`Korlia`, `Charzzlk`, and `Rojlik`)? Make each approved text or targeting change explicitly; retain current reset slot and aliases meanwhile.

This documentation-only review did not run builds, tests, a server or gameplay session, database work, or operational scripts. The [existing dossier](../zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md) retains the detailed story and previously recorded gameplay-verification boundaries.
