# The Underworld Shafts: jade, falling water, and hardened-fungus routes

Coverage line 269; source stem shafts, registered zone 100; source range 10000-10043, with 43 actual room records; reset mode 2. This source-comprehensive review follows the current snapshot in the coverage register. The [WLD file](../../../areas/wld/shafts.wld#L1) carries the zone's narrative. Its reset header is [here](../../../areas/zon/shafts.zon#L1).

## Story carried by the rooms

The Underworld Shafts is an environmental zone rather than a populated quest hub. Its rooms describe carved jade passages, sharp shards, old tools abandoned at a dead end, a dangerous waterfall, rock-hard fungus, and an unexplained burrow with seared roots. No local character, faction, quest giver, item, or narrative sidecar identifies who made the passages or what happened to the people who left the tools.

| Rooms | Environmental story |
| --- | --- |
| 10000-10003 | A steep, twisting shaft of carved jade with sharp wall shards, opening toward a vast cavern. |
| 10004-10011 | Smooth, difficult-to-climb jade tunnels. Room 10007 is a dead end with scattered tools. |
| 10012-10020 | A raging waterfall: the bottom at 10012, a falling-water sequence at 10013-10019, and the top at 10020. The text says water sweeps things down and describes sharp jade at the top. |
| 10021-10025 and 10036-10043 | Tunnels of dead, hardened fungus that has become rock. |
| 10026-10029 | A hard-earth tunnel bored by an unknown creature; its roots are described as seared off. |
| 10031-10035 | A separate smooth-jade tunnel. |

The declared range includes 10030, but the WLD has no room record for it. The zone has six disconnected local exit components; the room descriptions do not explain how the component boundaries arose. The hard-earth component, rooms 10026-10029, has no active cross-zone boundary and no source-only inbound edge in the scanned inactive maps.

## Hazards and authored behavior

The source has 25 F fall-marker rooms: 10000, 10002-10004, 10010-10011, 10021-10022, 10025-10029, 10031-10033, and 10035-10043. Eight C records sit on the waterfall sequence, rooms 10013-10020, each with the literal values 55 and 5. Those records and the descriptions signal hazards, but static source does not establish live movement, fall resolution, or a safe return route.

The 43 rooms contain 84 authored exit directions. All exit flag and key fields are zero; the zone file has no mobile or object reset commands. The source does not describe doors, keys, or any quest condition that would explain the disconnected branches.

## Population and quest content

The tracked mob and object files exist as empty files. There is no local QST file, local SHP file, native Q/QA contract, or story sidecar. The QST/AREA list includes the area name, but no local quest text is present. The [audit supplement](../quest-story-followups/SHAFTS_AUDIT_REVIEW.md) records the typed-reference checks and the inactive-map boundaries separately.
