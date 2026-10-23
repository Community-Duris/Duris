# Brimeforge repository-wide source audit

This supplement reconciles the active map, inactive-WLD sweep, typed source references, compiled numeric references, normalized title, and bounded same-stem history with the [source-comprehensive dossier](../zone-stories/BRIMEFORGE.md). It uses the pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active map and connected routes

The active [Brimeforge WLD](../../../areas/wld/brimeforge.wld#L1) contains 49 rooms (#131001–#131049) and 98 authored exits. All 98 resolve; 96 are local and two cross-area exits form reciprocal pairs. The map is one weak component, has no raw flag-8 exit, and authored directed traversal from root #131001 reaches and returns from all 49 rooms.

- Brimeforge #131036 west to Connector Zones #53802 is reciprocated by Connector Zones #53802 east to #131036 ([Brimeforge exit](../../../areas/wld/brimeforge.wld#L590), [Connector Zones reverse](../../../areas/wld/connectorzones.wld#L3743), pair rows [56](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L56) and [87](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L87)).
- Brimeforge #131037 east to Mini Zones #5879 is reciprocated by Mini Zones #5879 west to #131037 ([Brimeforge exit](../../../areas/wld/brimeforge.wld#L605), [Mini Zones reverse](../../../areas/wld/minizones.wld#L4422), pair rows [57](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L57) and [269](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L269)).

These are authored reciprocal boundaries. The graph counts do not establish current door, reset, actor, or runtime state.

## Inactive-WLD sweep

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. It found 196 Duris3 records targeting Brimeforge rooms. Of those, 100 have source-room VNUMs outside Brimeforge's #131001–#131049 range and are curated source-only map approaches:

- 49 south exits from Duris3 #130701–#130749 to Brimeforge #131001–#131049 (the first source example is [Duris3 #130701](../../../areas/wld/Duris3.wld#L644734)).
- One east exit from #131000 to #131001.
- One west exit from #131050 to #131049.
- 49 north exits from #131301–#131349 to #131001–#131049 (the final band reaches [Brimeforge #131049](../../../areas/wld/Duris3.wld#L658334)).

The remaining 96 matching exits start in 49 Duris3 source rooms whose VNUMs overlap #131001–#131049: 48 east and 48 west exits. Those are separate Duris3 map-room identities, not active Brimeforge rooms or active ingress. Their source titles are 42 “The Ocean,” five “Rolling Hills,” and two “Mountains.” They are excluded from the curated approach count.

No Surf exit targets Brimeforge. Checkpoint 151 therefore adds 100 to the curated source-only total: 3,228 before this checkpoint and 3,328 now. The reviewed `ud_map2` subset remains 15 and the reviewed Surf subset remains 159. Source-only map records do not establish active access or replacement. Keep Duris3 RETAIN IN PLACE.

## Typed references and numeric-context review

The active and unregistered exact-reference scans found no external QST item, SHP product, STORY item, ZON reset-role reference, local outbound prototype reference, or direct C/C++ numeric reference involving Brimeforge's local rooms, mobiles, or objects. No external duplicate mobile or object prototype definition was found. None of 872 external type-25 declarations targets a Brimeforge room.

All 231 external story sidecars parsed without errors and had no raw numeric-token hits for Brimeforge's local prototypes. These are bounded static-source results; they do not make claims about computed database, editor, or runtime references.

## Title, history, and disposition

The normalized title “The BrimStone Forge” is unique among the 442 scanned `.zon` files and active titles; the lexical candidate scan returned none. The followed `brimeforge.wld` history reaches the 2009-09-05 addition “added brimforge xp zone for your review,” with later same-stem edits in 2010, 2011, 2013, and 2025. The followed `brimeforge.zon` history includes the same initial addition and same-stem updates through 2015. No alternate-stem rename, predecessor, or replacement was established by these paths. The [title source](../../../areas/zon/brimeforge.zon#L2) and bounded history do not imply broader identity completeness.

Keep active `brimeforge` **DO NOT MOVE** as an un-commented `areas/AREA` contributor. Preserve the source-only Duris3 approaches with Duris3 **RETAIN IN PLACE**; neither their presence nor their same-numbered map rooms clears migration or proves replacement.

The source-mapping queue is now 191/220 complete, with 29 pending. Priority 192, The Bugger Caves (`bugger`), is next.

Validation: Checkpoint 151 documentation validation checked 124 changed/added Markdown files; all 4,888 local link targets and 2,265 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.
