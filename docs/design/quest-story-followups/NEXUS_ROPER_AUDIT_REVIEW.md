# The Reliquary Nexus of the Roper Den: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Roper Den
source dossier](../zone-stories/NEXUS_ROPER.md). It reconciles active and
source-only WLD edges, typed QST/story/reset dependencies, compiled prototype
references, title ownership, and bounded same-stem history. Static source
review does not establish runtime access or gameplay outcomes.

## Active WLD boundary and directed reach

Active nexus_roper has 25 rooms from #130400 through #130424 and 51/51
resolving exits: 50 local and one across an area boundary. Its local graph is
one weak component. Raw directed traversal from #130400 reaches all 25 rooms
and all 25 have a path back; no internal exit has flag 8.

The sole active boundary is reciprocal with Underdark: Roper Den #130400
south to Underdark #811645 and Underdark #811645 north to #130400 ([pair rows
322 and 725](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L322), [Roper Den
room](../../../areas/wld/nexus_roper.wld#L1), [Underdark
room](../../../areas/wld/underdark.wld#L173718)). These figures describe
authored WLD edges, not dynamic access or player traversal.

## Inactive WLD sweep and Duris3 overlaps

All 95 WLD files outside the active areas/AREA list parsed cleanly: 527,791
rooms and 2,080,773 exits. The sweep found 100 exit records targeting Roper
Den room VNUMs, all in inactive Duris3.wld. Twenty-five Duris3 source rooms
also use VNUMs #130400–#130424, which are the active Roper Den rooms. Preserve
those as distinct map records: their same-number source identities are not
active ingress or replacement evidence.

Fifty-two of the 100 target records start in non-overlapping Duris3 source
rooms and remain in the curated source-only set:

| Duris3 source records | Source-only exits | Evidence |
| --- | --- | --- |
| #130100–#130124 | south to #130400–#130424, in matching numeric order | [First room](../../../areas/wld/Duris3.wld#L632101), [last room](../../../areas/wld/Duris3.wld#L632605) |
| #130399 | east to #130400 | [Source room](../../../areas/wld/Duris3.wld#L638380) |
| #130425 | west to #130424 | [Source room](../../../areas/wld/Duris3.wld#L638926) |
| #130700–#130724 | north to #130400–#130424, in matching numeric order | [First room](../../../areas/wld/Duris3.wld#L644701), [last room](../../../areas/wld/Duris3.wld#L645205) |

The other 48 target matches start in the 25 same-numbered Duris3 source
rooms #130400–#130424. They are excluded from the curated source-only count
because their source rooms overlap the active target VNUM set. The 52
non-overlapping records raise curated source-only totals to 3,404 overall;
Surf remains 172 and ud_map2 remains 15. None is active ingress.

Keep Duris3 RETAIN IN PLACE with its existing candidate row
[36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36); no candidate row is added.

## Exact typed and structured dependencies

The selected external scans found no QST item command, SHP product, structured
story item_vnums field, or ZON reset-role reference into Roper Den rooms,
mobiles, or objects. All 231 other story sidecars parsed without errors and
had no numeric-token matches into Roper Den prototypes. Its local QST/story
item fields and ZON reset roles stay within the two local objects, eight
mobiles, and 25 rooms. Roper Den has no local SHP source.

No external duplicate mobile or object prototype was found. Across 872
external type-25 declarations, none targets Roper Den. A literal C/C++ scan
found no direct numeric prototype reference into Roper Den. These bounded
scans do not exhaust arbitrary computed, editor, administrative, or installed
references.

## Title and bounded history

The normalized title “The Reliquary Nexus of the Roper Den” belongs only to
active stem nexus_roper among 442 scanned .zon files
([title source](../../../areas/zon/nexus_roper.zon#L2)). Followed
nexus_roper.wld history reaches the 2009 areas/ move. nexus_roper.zon history
adds a 2010 title fix and 2015 quest-item flag update after that move. The
reviewed paths establish no alternate-stem rename, distinct predecessor, or
whole-zone replacement. See the [duplicate and replacement
review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active nexus_roper DO NOT MOVE. Preserve its reciprocal Underdark
boundary and all 52 non-overlapping source-only Duris3 exits. Keep Duris3
RETAIN IN PLACE and preserve the separate 25-room overlap/48-exit
classification. No new candidate row is added. Editor, administrative,
installed-data, persisted-player, indirect-runtime, and actual traversal
closure remain outside this bounded scan; no gameplay, migration, or source
change is proposed.
