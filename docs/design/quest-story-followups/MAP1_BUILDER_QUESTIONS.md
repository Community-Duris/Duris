# map1 empty MOB path artifact builder questions

Resolve these with the source/data owner before reclassifying, moving, or
removing the tracked empty file. Keep it unchanged and outside the area lists
until the file's purpose and external copies are known.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| M1-01 | Is zero-byte `areas/mob/map1.mob` an intentional placeholder? | Preserve the current file until its owner confirms its purpose. [Inventory](MAP1_AUDIT_REVIEW.md#inventory-and-empty-source-finding) |
| M1-02 | Was `map1` ever intended to identify a game area or mobile set? | Do not infer a zone identity from the generic filename; record an owner-confirmed identity or keep it classified as an unresolved empty source path. [History](MAP1_AUDIT_REVIEW.md#path-history-and-identity-limits) |
| M1-03 | Do the zero-byte paths in the rename-follow history represent one logical file? | Treat the history as ambiguous because identical empty blobs can match without sharing a purpose; require independent source-owner evidence. [History](MAP1_AUDIT_REVIEW.md#path-history-and-identity-limits) |
| M1-04 | Is the 2009 `areas/mob/map1.mob` destination still required by a build or editor? | Ask the responsible tool owner to identify exact project, export, and deployment inputs before any path change. [Current file](../../../areas/mob/map1.mob) |
| M1-05 | Are there paired map, mobile, object, zone, quest, shop, story, or coordinate files outside this checkout? | Reconcile repository, editor, deployment, and backup inventories; the tracked checkout currently contains only the empty MOB path. [Inventory](MAP1_AUDIT_REVIEW.md#inventory-and-empty-source-finding) |
| M1-06 | Should the component remain unlisted in `areas/AREA` and `areas/qst/AREA`? | Keep both lists unchanged unless an owner supplies a complete, valid area identity and source set. [Area lists](../../../areas/AREA#L1), [QST list](../../../areas/qst/AREA#L1) |
| M1-07 | Is `map1` meant to be related to the separate `ud_map1` source family? | Keep the keys and files distinct; require explicit owner evidence before relating their maps, routes, or prototypes. [Distinct source](../../../areas/wld/ud_map1.wld#L1) |
| M1-08 | Do `MAP1_FILE` or the neighboring `MAP1A_FILE` constants identify this component? | Treat both as unrelated information-file paths (`lib/information/map1` and `lib/information/map1a`), not references to the area/MOB source. [MAP1_FILE](../../../src/world/db.h#L36), [MAP1A_FILE](../../../src/world/db.h#L39) |
| M1-09 | Did a former mobile prototype set or numeric identity disappear before this snapshot? | Request any source-control, editor, or deployment evidence from the owner; the current file contains no VNUMs to use for reference matching. [Reference limits](MAP1_AUDIT_REVIEW.md#references-and-area-list-status) |
| M1-10 | May the zero-byte file be removed or reclassified after review? | Wait for the owner's explicit disposition and verify any external copies before a separate cleanup decision. [Disposition](MAP1_AUDIT_REVIEW.md#disposition-and-owner-questions) |
