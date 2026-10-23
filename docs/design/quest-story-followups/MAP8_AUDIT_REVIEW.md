# map8 empty MOB path review

Canonical key: component:map8. Physical coverage row 417 / CSV line 418;
candidate dependency row 66 / CSV line 67. Reviewed October 11, 2026,
Checkpoint 504, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static file-inventory,
path-history, and bounded-reference review. It does not establish the intent
of the empty file, an external editor or deployment copy, or persisted use.

## Inventory and empty-source finding

The only tracked same-stem path is the zero-byte
[areas/mob/map8.mob](../../../areas/mob/map8.mob). Its Git blob and
working-tree file are both empty. No other same-stem WLD, OBJ, ZON, QST, SHP,
story-sidecar, or map-editor source is tracked. Neither
[areas/AREA](../../../areas/AREA) nor [areas/qst/AREA](../../../areas/qst/AREA)
lists `map8`.

The file defines no mobile VNUMs, prototype records, room records, exits,
shop stock, quest contract, or narrative content. Its path alone does not
establish a game-area identity or make it safe to remove.

## Path history and identity limits

`git log --follow` connects `areas/mob/map8.mob` to
`Players/justice_list`, and then to `Players/Declined/...`, across commits
`642da5400` (2009-08-12), `5000f6cb8` (2006-11-25), and `4e5f49ef1`
(2005-10-09). Each observed blob is zero bytes. The apparent 100-percent
rename matches arise from empty files; the history cannot identify what the
paths were for or whether they shared an area role.

The same old `Players/` paths also appear in the rename-follow history for
empty `map1.mob`. This is ambiguous path similarity, not proof that map1 and
map8 are one component, that one replaced the other, or that either ever
named a game area. The separately named `oldmap8.wld` is another source path;
its similar filename does not establish that it is a companion or predecessor
to `map8.mob`. No direct rename, retirement, or whole-area replacement
decision is established.

## References and area-list status

The current file has no prototype VNUMs or room identities that resets, QST
givers, shops, compiled assignments, or exits could match. A bounded,
case-insensitive text scan of tracked `areas/` and `src/` content found no
direct `map8`, `map_8`, or `map-8` reference. The scan cannot qualify
untracked editor projects, deployment exports, backups, or persisted data.

There is no WLD input, so connectivity metrics are not applicable and should
remain blank. Candidate inbound active exits are zero because the component
has no room identity to target. This is not a runtime loader or cleanup-safety
finding.

## Disposition and owner questions

**Disposition: RETAIN IN PLACE pending owner decision.** Preserve the tracked
zero-byte file at its current path and outside both area lists while its
purpose and history are identified. Do not recreate a mobile set, treat
`oldmap8.wld` or `map1.mob` as the same component, or infer that deletion is
safe from the empty blob. Ask the original source/data and tool owners whether
the path is an intentional placeholder or a legacy path artifact before any
removal, rename, or reclassification. See the
[10 builder questions](MAP8_BUILDER_QUESTIONS.md),
[coverage row 417](ZONE_COVERAGE_REGISTER.csv#L418), and
[candidate dependency row 66](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L67).

## Checkpoint 915: exact path-reference refresh

The tracked [areas/mob/map8.mob](../../../areas/mob/map8.mob) remains a
zero-byte file. A case-insensitive bounded scan for the exact tokens `map8`,
`map_8`, and `map-8` across `areas/`, `src/`, `scripts/`, and `tests/` found no
references. A separate exact-path scan for `areas/mob/map8.mob` across the same
source trees also found none. Because the file contains no VNUMs or room
identity, there are no candidate keys for typed reset, QST, shop, story,
compiled VNUM, or WLD-route scans.

This closes only the in-checkout path and token search. It does not qualify
external editors, exported or deployed copies, backups, or persisted data.
The zero-byte `map8.mob`, empty `map1.mob` history, and `oldmap8.wld` remain
separate unresolved source identities. Preserve the current **RETAIN IN PLACE
pending owner decision** disposition and all 10 builder questions; the scan
provides no removal, rename, or reclassification decision.
