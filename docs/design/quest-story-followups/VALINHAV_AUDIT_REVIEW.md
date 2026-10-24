# Valinhaven static dependency review

Checkpoint 873 closes the bounded tracked-source dependency review for active valinhav against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. The comprehensive [Valinhaven story dossier](../zone-stories/VALINHAV.md) and its 20 [builder questions](../zone-stories/VALINHAV_BUILDER_QUESTIONS.md) remain the narrative and unresolved design references. The existing title and followed-history decision remains at [checkpoint 768](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-768-valinhaven-and-source-only-map-context).

## Active graph and reciprocal WLD boundaries

The active map contains 159 rooms and 320 authored exits; all 320 resolve in the loader census, and the local exit graph has one weak component. Valinhaven has two outbound active WLD records, each paired with a reverse record:

| Valinhaven room | Active neighbor | Reverse edge | Evidence |
| --- | --- | --- | --- |
| #90800, Along a Mysterious River | East to Underdark #798725 | Underdark #798725 west to #90800 | [Valinhaven exit](../../../areas/wld/valinhav.wld#L18), [Underdark return](../../../areas/wld/underdark.wld#L143962), [cross-area row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1116) |
| #90810, Back of the Cave | North to Dirk’nspire #96800 | Dirk’nspire #96800 south to #90810 | [Valinhaven exit](../../../areas/wld/valinhav.wld#L207), [Dirk’nspire return](../../../areas/wld/dirkn.wld#L18), [cross-area row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1117) |

The active WLD graph remains the authored topology. These static edges do not establish current actor reachability, door/reset state, safe travel, or return under every state.

## Source-only WLD approaches

The all-95 inactive-WLD sweep found two authored exits into Valinhaven and no room-VNUM overlap:

| Unregistered source | Authored exit | Evidence |
| --- | --- | --- |
| ud_map1.wld, room #1113221 | North (D0) to #90800 | [room](../../../areas/wld/ud_map1.wld#L16872), [north exit and target](../../../areas/wld/ud_map1.wld#L16876) |
| ud_map2.wld, room #1123855 | East (D1) to #90800 | [room](../../../areas/wld/ud_map2.wld#L43063), [east exit and target](../../../areas/wld/ud_map2.wld#L43071) |

Both source maps are outside active WLD membership. These records remain source-only, not active ingress. Preserve the references on the existing candidate dependency data rows 112 and 113 ([physical line 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), [physical line 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114)). No Valinhaven candidate or source-only count is added; the current curated totals remain 11,993 overall and 415 from Surf.

## Code-selected possible arrival

The compiled do_shift_prime innate has a possible destination at Valinhaven room #90803. Its destination array contains #90803; the function only proceeds while the character is in Astral, and its Githyanki/Pillithid and Illithid branches select from that array ([function and destination](../../../src/classes/innates.c#L2005), [Astral gate](../../../src/classes/innates.c#L2021), [race branches](../../../src/classes/innates.c#L2033)). This is a conditional, code-selected arrival, not a WLD edge or a general access promise. The bounded source review does not qualify actual command availability, destination safety, or return behavior.

## Typed prototype and numeric-reference scan

A token-aware scan of current ZON commands found no outbound foreign mobile or object reset in valinhav.zon, and no active external ZON reset targeting any of Valinhaven’s 20 mobile or nine object prototypes. Exact tracked-source scans found no external QST, SHP, or story-sidecar prototype references. A census of all 713 active type-25 teleport objects found no destination in Valinhaven’s room set.

One raw object-data number falls in the Valinhaven room range: unrelated OBJ #38252 in forestb.obj has #90882 as the final value on its data line. The object reader assigns that field to anti2_flags, so it is flag data rather than a room destination ([object data](../../../areas/obj/forestb.obj#L805), [parser assignment](../../../src/world/db.c#L4660)). An exact numeric scan of C/C++ source found only the #90803 do_shift_prime destination match in the local room range.

No same-type active WLD, MOB, or OBJ VNUM overlap was found. These bounded scans describe tracked source references; they do not establish deployed resets or live prototype availability.

## Identity and disposition

Checkpoint 768 records one exact current title owner for Valinhaven and no distinct followed same-stem predecessor or replacement. The ud_map1 and ud_map2 routes remain separate source-map identities.

Keep active valinhav **DO NOT MOVE** and add no attic candidate. Preserve its two reciprocal active boundaries, the conditional #90803 code destination, and both source-only approaches on their current owners. Dynamic reachability, editor/tool closure, deployment/runtime state, persisted room identity, and owner intent remain unqualified. No move, merge, archive, or gameplay change is cleared by this source review.
