# Bugger Caves repository-wide source audit

This supplement reconciles the active map, inactive-WLD sweep, typed references, compiled numeric references, normalized title, and bounded same-stem history with the [source-comprehensive dossier](../zone-stories/BUGGER.md). It uses the pinned source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. No area or gameplay source is changed by this audit.

## Active map and connected routes

The active [Bugger WLD](../../../areas/wld/bugger.wld#L1) contains 28 rooms (#6400–#6427) and 56 authored exits. All 56 resolve; 54 are local and two cross-area exits form reciprocal pairs. The map is one weak component, has no raw flag-8 exit, and authored directed traversal from root #6400 reaches and returns from all 28 rooms.

- Bugger #6400 north to UnderDark #807099 is reciprocated by UnderDark #807099 south to #6400 ([Bugger exit](../../../areas/wld/bugger.wld#L10), [UnderDark reverse](../../../areas/wld/underdark.wld#L162509), pair rows [65](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L65) and [692](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L692)).
- Bugger #6426 up to Ixxillikor #4380 is reciprocated by Ixxillikor #4380 down to #6426 ([Bugger exit](../../../areas/wld/bugger.wld#L505), [Ixxillikor reverse](../../../areas/wld/ixxillikor.wld#L4030), pair rows [64](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L64) and [207](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L207)).

These are authored reciprocal boundaries. The graph counts do not establish current door, reset, actor, or runtime state.

## Inactive-WLD sweep

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no inactive source-room VNUM overlap with Bugger and no exit targeting active rooms #6400–#6427. No source-only approach is added. The curated source-only total remains 3,328 overall, including 15 reviewed ud_map2 approaches and 159 for Surf. The absence of an inactive-map edge does not clear active routes or other dependencies for migration.

## Typed references and numeric-context review

The active and unregistered exact-reference scans found no external QST item, SHP product, STORY item, ZON reset-role reference, or local outbound prototype reference involving Bugger's local rooms, mobiles, or objects. No external duplicate mobile or object prototype definition was found. None of 872 external type-25 declarations targets a Bugger room.

The direct C/C++ numeric scan returned one match: [room #6405 in do_shift_prime](../../../src/classes/innates.c#L2009), in the githyanki_room destination list. This is a room-number use; the same VNUM in Bugger's object namespace is a separate prototype. All 231 external story sidecars parsed without errors and had no raw numeric-token hits for Bugger's local prototypes. These are bounded static-source results; they do not make claims about computed database, editor, or runtime references.

## Title, history, and disposition

The normalized title “The Bugger Caves” is unique among the 442 scanned .zon files and active titles; the lexical candidate scan found no other owner. Followed bugger.wld history records the 2009-08-12 areas/ path relocation. Followed bugger.zon history records that relocation and the 2015 quest-item-flag update. No alternate-stem rename, predecessor, or replacement was established by these paths. The [title source](../../../areas/zon/bugger.zon#L2) and bounded history do not imply broader identity completeness.

Keep active bugger **DO NOT MOVE** as an un-commented areas/AREA contributor. Preserve its two active reciprocal WLD boundaries; no inactive-WLD approach is identified. Neither the static map nor the bounded history clears migration.

The source-mapping queue is now 192/220 complete, with 28 pending. Priority 193, Dirk’nspire Stronghold (dirkn), is next.

Validation: Checkpoint 152 documentation validation checked 126 changed/added Markdown files; all 4,916 local link targets and 2,281 line anchors passed. The coverage register is rectangular at 476 data rows x 60 columns and the attic candidate register at 121 x 10. `git diff --check` passed, and `areas/` is unchanged. No tests, build, server, gameplay session, database operation, migration, or operational script was run.
