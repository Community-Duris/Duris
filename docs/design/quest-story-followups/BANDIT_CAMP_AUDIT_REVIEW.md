# Bandit Camp: world boundary and source-history audit

**Checkpoint 106 · 2026-10-09 · active source `banditca`, zone 148.** This supplement adds the repository-wide connectivity, outside-reference, title/history, and archive-disposition review to the [Bandit Camp story dossier](../zone-stories/BANDIT_CAMP.md). It does not change gameplay or move any source.

Sources: [`areas/AREA`](../../../areas/AREA#L81), [`banditca.wld`](../../../areas/wld/banditca.wld#L18), [`surface.wld`](../../../areas/wld/surface.wld#L976475), [`wh.wld`](../../../areas/wld/wh.wld#L12635), [`banditca.zon`](../../../areas/zon/banditca.zon#L222), [active cross-area exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L266), [cross-area pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L37), and the [coverage row](ZONE_COVERAGE_REGISTER.csv).

## Active connectivity and authored reachability

All 351 active WLDs parsed without a Bandit Camp parse issue. `banditca.wld` defines 127 rooms and 311 exits: 310 local exits and one outbound exit. All 311 targets resolve in the active WLD room set. The local undirected projection is one weak component containing all 127 rooms. A directed breadth-first search from entrance #14800 reaches all 127 Camp rooms using authored local exits. This graph check does not simulate closed doors, reset timing, combat, player permissions, or safe return.

The ordinary Surface boundary is reciprocal:

| Source | Edge | Matching edge | Register |
| --- | --- | --- | --- |
| Surface #546515 | north → Camp #14800 | Camp #14800 south → Surface #546515 | [exit rows 266/1435](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L266), [pair rows 37/467](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L37) |

There is one additional active inbound source edge: `wh.wld` room #55614 down → Camp room #14854 ([exit row 768](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L768), [pair row 781](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L781)). Its source room is titled “Zone dispersal room for Incarnates.” The edge has no WLD return to `wh`; from #14854, Camp's directed local graph reaches the main entrance #14800 and its Surface exit. This is a one-way authored dispersal reference, not evidence of an ordinary player route from WH or a safe return to WH. The all-area scan found no direct WLD edge between `banditca` and the separately active `bandit` area.

## Inactive-WLD approach

All 95 inactive WLD files parsed: 527,791 rooms and 2,080,773 exits, with no parse issues and no source-room VNUM overlap with Bandit Camp. One source-only approach was found: inactive `surf.wld` #220524 (“A Dark and Shadowy Forest”), north → Camp #14800 ([source exit](../../../areas/wld/surf.wld#L430580)). It is not active ingress and does not alter the active graph.

This adds one record to the reviewed Surf subset, raising it from 132 to 133, and raises the curated source-only approach total from 1,519 to 1,520. Preserve the edge with the inactive Surf records; it does not establish current map ownership, a player route, or safe travel. The `surf` / `surface2011` candidate remains **RETAIN**.

## Exact typed references

The all-area typed scans found these outside references into Bandit Camp:

| Type | Source and record | Qualification |
| --- | --- | --- |
| Active ZON reset | `tharnadian_ruin.zon:177`, G-load of object #14821 ([source](../../../areas/zon/tharnadian_ruin.zon#L177)) | The blood-red tear insignia is an accepted proof item; this is a real outside source and does not prove a Camp kill. |
| Active ZON reset | `wh.zon:695`, G-load of object #14800 ([source](../../../areas/zon/wh.zon#L695)) | This is the bottle of ale, not quest proof. |
| Active shop stock | `wh.shp:495`, shop #55185 product #14800 ([source](../../../areas/shp/wh.shp#L495)) | Ale stock only; not a quest item source. |

No external active QST item command targets a Camp object. No other active SHP product or ZON reset-role row targets a Camp object. No external active MOB/OBJ prototype duplicates any local Camp mobile/object. Among 872 external type-25 portal declarations, none targets a Camp room. No maintained C/C++ direct numeric binding to a Bandit Camp VNUM was found.

All 231 outside story sidecars parsed with zero exact structured numeric VNUM matches. A broad numeric-token scan flagged the digit sequence `14910` in `wh.story.json`, `$.stories[66].id` (`request-55115-e14910a42cdc`); that string is a request identifier, not a typed VNUM field or reference.

The Camp has one reviewed outbound reset dependency: `banditca.zon:222` G-loads object #67262, whose prototype is in `areas/obj/unique.obj:972` and is titled “the wondrous mace of the Sea.” This imported object is the source of Isoril's mace / sword-prose mismatch already recorded in the dossier; preserve the foreign prototype identity.

## Active identities and history

The normalized active `.zon` title “Bandit Camp” has one owner among 351 non-empty active titles: `banditca`, zone 148, with physical rooms #14800–#14926. The active `bandit` source is a separate contributor, zone 985, titled “The Bandit Canyons,” with physical rooms #98500–#98575 ([active list](../../../areas/AREA#L355)). Their stems, zone numbers, VNUM ranges, and WLD networks are distinct; the current active WLD scan found no direct edge between them.

Followed Git history shows separate same-stem paths for `banditca.wld` / `.zon` and `bandit.wld` / `.zon`, with distinct later edits. `banditca` has its own 2011/2013 WLD edits and 2013/2015 ZON edits; `bandit` has separate later history, including its 2016 Olat keyword change. The followed records and active manifest contain no explicit replacement claim or distinct predecessor for Bandit Camp. The reviewed 2009 repository-wide area move adds both current WLD paths at the new paths; it does not establish whether an earlier path or untracked predecessor existed. Similar subject matter is not evidence that Bandit Canyons replaced Bandit Camp.

See [duplicate and replacement history](DUPLICATE_AND_REPLACED_ZONES.md) and the [Bandit Camp coverage row](ZONE_COVERAGE_REGISTER.csv).

## Disposition

`banditca` remains an active full-area contributor and **DO NOT MOVE**. Keep its reciprocal Surface boundary, separate one-way WH dispersal edge, imported object dependency, and the inactive Surf approach classified as above. `surf` remains **RETAIN**. The distinct active `bandit` identity also remains in place. No area, story-sidecar, quest, world, reset, or runtime source was changed.