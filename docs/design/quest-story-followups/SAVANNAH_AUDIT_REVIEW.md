# The Savannah of Broken Trusts: connectivity, dependencies, history, and builder decisions

**Checkpoint:** 346, coverage register row 258. **Disposition:** DO NOT MOVE. No gameplay, zone, or migration change is proposed.

This supplement reconciles the [comprehensive Savannah source story map](../zone-stories/SAVANNAH_OF_BROKEN_TRUSTS.md) and [generated source review index](../../reference/zone-story-audits/savannah.md) with active connectivity, reset-aware access, source-only map identity, typed prototype references, followed history, and builder decisions. It uses checked-in source only. No build, test, live travel, reset, item-source, or recipient-availability journey was run for this checkpoint.

## Source story and authored scope

The active `savannah` contributor is listed in `areas/AREA` and `areas/qst/AREA`. Its active files define 167 rooms, 59 mobiles, 39 objects, nine QST giver VNUMs, 17 native contracts, and 315 reset commands: 200 M, 73 E, 20 F, 16 G, and six D. The source review index maps the 17 exact exchanges and the raw dialogue families. There is no local `.shp` file or local literal special assignment. The zero shop count in [coverage row 258](ZONE_COVERAGE_REGISTER.csv#L258) is correct.

The source story map follows five named request families, six distinct base-instrument exchanges, one six-sword Kunji recipe, six matching epic upgrades, source-stock constraints, inaccessible or risky routes, and unfinished recipient and tribal leads. Its route-specific arithmetic for a fresh six-epic set is 42 original katana roots and six available Kunji and Lynstar appearances; supplied exact items can bypass those preparations, so this is not a universal player requirement.

## Active connectivity and reset-aware access

The active WLD has 167 rooms and 450 exit records: 449 local and one outbound. All resolve, and the local directed graph has one weak component of 167 rooms. The only active boundary is Savannah #138584 west to Surface #562072, with the reciprocal Surface east exit. The [cross-area pair register](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L416) and [exit rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1311) document the Savannah side; the reverse is in [the Surface pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L584) and [Surface exit row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1469).

| Arrival | Raw directed local reach | Raw return to arrival | Conservative post-reset reach | Post-reset return |
| --- | ---: | ---: | ---: | ---: |
| Surface #562072 → Savannah #138584 | 167/167 | 167/167 | 165/167 | 164/167 |

The conservative filter removes directions covered by 100%-chance closed or secret door resets. All six D reset records are state 5 and form three reciprocal pairs. Under that filter, #138546, #138547, #138585, and #138629 are not reachable from #138584; #138665 is reachable but has no filtered return path. These are static source-graph counts, not live results for hidden-exit discovery, door handling, falls, scripted movement, or reset timing.

| Pair | Reset records | Finding |
| --- | --- | --- |
| #138580 west ↔ #138585 east | Both state 5, key 0, 100% | Closed secret wall pair. |
| #138619 east ↔ #138665 west | Both state 5, key 0, 100% | Closed secret wall pair into Lynstar's pit. |
| #138623 south ↔ #138629 north | Both state 5, key 0, 100% | Closed secret wall pair into the Kahir chamber. |

The six D records are at [Savannah ZON lines 8–28](../../../areas/zon/savannah.zon#L8). Four rooms declare fall chances: #138545, #138546, #138547, and #138549. No room declares currents. The source story map also records two unusual access facts: the termite mound #138559 has a secret down edge to #138589 that is not closed by a reset, and Butsu's well at #138655 descends to #138665 without an up exit. The secret pit/termite pair offers another route; source topology alone does not establish a safe return journey. See the [baobab and hidden-route source](../../../areas/wld/savannah.wld#L1159) and [well room](../../../areas/wld/savannah.wld#L3351).

## Inactive WLD room-number overlap

The all-95 inactive-WLD scan found no uniquely attributable, non-overlapping source-only approach into Savannah. Inactive `Duris3.wld` declares all 167 Savannah room VNUMs, but its records have different map identities: its #138500 is titled “Mountains,” while its #138584 is “The Ocean”; Savannah #138500 is the savannah entrance and #138584 is the wide plain. Compare [Duris3 #138500](../../../areas/wld/Duris3.wld#L808501), [Duris3 #138584](../../../areas/wld/Duris3.wld#L810265), and the corresponding [Savannah entrance](../../../areas/wld/savannah.wld#L1) and [plain](../../../areas/wld/savannah.wld#L2011).

There are 668 `Duris3.wld` edge records whose targets fall in Savannah's 138500–138666 range; every target is also declared in that same inactive file. The full room-number overlap makes those target identities ambiguous, so they are not counted as approaches into active Savannah. The other 94 inactive WLD files contain no edges into the Savannah VNUM range. Preserve this source identity finding on the existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); do not add a Savannah ingress or change Surf's source-only count. Savannah has no new unique source-only approach, and the curated global total stays 4,498 (Surf 262, `ud_map1` two, `ud_map2` 20, Underworld2 eight). Surf's 145 active references are unchanged.

## Typed prototype dependencies

Savannah's native instrument routes depend on the exact six Mitashi katana VNUMs #138267–#138272, and the six epic recipes also require Retribution #138279. Exact native exchange terms and their sources are listed in the [Savannah source review index](../../reference/zone-story-audits/savannah.md#native-exchanges). Preserve the item owners and distinct kinds when reviewing those routes.

The one exact external consumer of a Savannah-owned object found in active typed QST and story data is Plane of Air's Cloudseeker exchange. Airp consumes Watcher cloak #138515 together with eight other relics, including Mitashi's North Wind #138268, to produce Cloudseeker #131650 and retires the north wind. The native QST lists #138515 among its required givens at [airp.qst line 119](../../../areas/qst/airp.qst#L119); the [story contract](../../../areas/story/airp.story.json#L495) preserves the complete nine-item requirement and its [Watcher-cloak step](../../../areas/story/airp.story.json#L590) preserves the Savannah source. This is a foreign-owned use of a local item and competes with retaining or transferring the cloak.

No local ZON room, mobile, or object reset imports an external prototype. No external mobile or room reset targets a Savannah-owned prototype, and no external type-25 object declaration targets a Savannah room (872 declarations scanned). Same-type mobile and object scans across active and source-only files found no duplicate definitions. The only outside exact active QST/story object use found was Airp's #138515 above. These source scans do not include persisted or dynamic runtime data.

The generated source index has no local shop or literal special assignment. Compiled typed item tables do reference Savannah instruments: the standard Bard kit lists all six base instruments #138533–#138538 as extras in [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L199), and the standard Shaman kit equips epic flute #138542 in slot 23 at [line 144](../../../src/account/chaos_eq_data.h#L144). The profile grants are independent sources; they do not record a native recipe history. A bounded C/C++ exact VNUM scan found no other Savannah prototype hook.

## Title, duplicates, and followed history

After color-code removal, trimming, whitespace normalization, and case-folding, “The Savannah of Broken Trusts” occurs once among 442 current `.zon` titles and once among 351 active contributors. Same-type mobile and object scans found no duplicate VNUMs. Room identity has the explicit exception above: all 167 active Savannah room VNUMs also occur in inactive `Duris3.wld`, whose records describe a separate map. This numeric overlap does not establish that Duris3 is a predecessor, replacement, or active route into Savannah.

Followed WLD, MOB, OBJ, ZON, and QST histories stay on the `savannah` paths. The tracked WLD and ZON last changed in 2025, MOB in 2014, OBJ in 2017, and QST in 2015; the story sidecar was added on 2026-10-02. No alternate-stem predecessor or explicit whole-zone replacement was identified. Keep active `savannah` DO NOT MOVE and retain the `Duris3.wld` collision as a separate, unresolved map identity on its existing candidate row.

## Builder questions and recommended resolutions

1. **Are the six cap-one Mitashi katana sources intended to support these recipes at their declared populations?** A cap describes stock when admitted, not player supply or schedule. **Recommendation:** preserve each typed source and qualify spawn, recovery, and availability before promising a daily or guaranteed route.
2. **How should the six distinct katana roots be allocated between base instruments and Kunji's all-six exchange?** **Recommendation:** keep the source map's route-specific allocation explicit and do not treat six copies of one kind as substitutes.
3. **Should builder guidance present 42 roots as the route for six locally made epics?** That arithmetic includes six base-making roots and six full sets for Retribution. **Recommendation:** label it as the fresh native route only; keep supplied items and saved receipts as separate paths.
4. **Do CHAOS profile grants count as instrument provenance or completion of native preparation?** The code grants base instruments and epic flute #138542. **Recommendation:** keep profile possession separate from accepted recipe and source history.
5. **Should Plane of Air continue consuming Savannah cloak #138515 together with North Wind #138268?** **Recommendation:** preserve the exact owners and coordinate any cloak change with Airp; do not infer that the local Kahir essence receipt is required by Airp.
6. **Should Lynstar's repeated base-exchange dialogue continue to say “lyre” in all six branches?** The exact terms accept six different matching swords and outputs. **Recommendation:** correct the prose to name the branch's actual instrument without changing native terms.
7. **Should Lynstar's current pit placement at #138665 remain at odds with prose placing her on village paths?** **Recommendation:** decide whether to update the placement or the prose after reviewing the secret pit route and Lynstar's own reset.
8. **Is Kunji's six-sword recipe meant to retire him after one exchange?** **Recommendation:** preserve the accepted one-each recipe and qualify how an intended repeat route obtains another available Kunji generation.
9. **Are all six epic upgrades meant to retire Lynstar, one matching pair at a time?** **Recommendation:** retain that identity boundary and qualify recipient return/reset behavior before describing repeated completion.
10. **Is there an intended terminal for collecting all six instruments?** The native area has no accepted full-set collection contract. **Recommendation:** either author a distinct exact six-item terminal and recipient or keep the collection as an unimplemented lead.
11. **Should Lynstar's absent Drakenwood sister remain lore, or should the lead have an active endpoint?** The referenced Hostel is not in `areas/AREA` and the retained quest source has no six-instrument consumer. **Recommendation:** choose an active recipient/reward or retire the lead intentionally before implementing a sidecar stage.
12. **Should the M'Bele god appear after a chief's death?** Prototype #138558 and claws #138532 exist, but no active god stock or reviewed death handler was found. **Recommendation:** decide whether this is lore or a feature, and define the accepted eligible death and durable closure before awarding a stage.
13. **Should the Watcher exchange record only cloak delivery and departure?** **Recommendation:** keep tribe-wide liberation, a winning tribe, and M'Bele's appearance separate until a durable outcome is authored.
14. **How should the three state-5 secret wall pairs be discovered and traversed in both directions?** Their reset keys are zero. **Recommendation:** confirm search/open flags, visibility, and return descriptions before changing the paired resets.
15. **Is the mound's secret down edge intentionally open at reset?** **Recommendation:** verify the actual door flags and intended search path; preserve the no-close-reset evidence meanwhile.
16. **Is Butsu's one-way well descent into #138665 intended?** **Recommendation:** qualify the pit exit through the termite network and avoid documenting the well as a two-way route.
17. **Are the four fall fields in the baobab intended to match their descriptions and landing routes?** **Recommendation:** review fall triggers, consequences, and safe recovery in the running world before changing any field.
18. **Should Dromis's dialogue imply player-directed healing, termite hatching, or a completed village rescue?** Those texts have no reviewed local terminal. **Recommendation:** keep narrative claims separate from persisted results until a world outcome is authored.
19. **Should the chief and Watcher dialogue families be retained by the generated source index despite apostrophe aliases?** Both have valid plain aliases as well as an apostrophe form. **Recommendation:** fix alias-family filtering to preserve usable aliases and keep ambient `qc_action` separate from learned objectives.
20. **Is NOSLEEP item #138545 intended to protect every Tukluk who carries it?** Most wear it in EYES, while one woman receives it through G; reviewed bard and spell checks inspect different equipment scopes. **Recommendation:** confirm intended NPC protection before adjusting either the reset or the sleep checks.

## Safe disposition

Keep active Savannah in place. Preserve its single reciprocal Surface boundary, three state-5 secret pairs, fall fields, local recipe identities, Airp's external cloak consumer, and CHAOS instrument references. Keep the 167-room `Duris3.wld` number collision on the existing Duris3 candidate with its 668 identity-ambiguous target records excluded from approach totals. No gameplay change, added source-only ingress, candidate addition, or migration clearance follows from this source audit.
