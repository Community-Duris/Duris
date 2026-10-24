# map8 empty MOB path builder questions

Resolve these with the source/data and tool owners before reclassifying,
moving, or removing the tracked empty file. Keep it unchanged and outside the
area lists until its purpose and any external copies are known.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| M8-01 | Is zero-byte `areas/mob/map8.mob` an intentional placeholder? | Preserve the current path until an owner confirms its role. [Inventory](MAP8_AUDIT_REVIEW.md#inventory-and-empty-source-finding) |
| M8-02 | Was `map8` ever intended to identify a game area or mobile set? | Do not infer an area identity from the filename; record an owner-confirmed identity or retain it as an unresolved empty source path. [Inventory](MAP8_AUDIT_REVIEW.md#inventory-and-empty-source-finding) |
| M8-03 | Do the zero-byte `Players/Declined/...` and `Players/justice_list` history paths identify the same logical asset? | Treat rename-follow matches as ambiguous because identical empty blobs do not show purpose or ownership. [History](MAP8_AUDIT_REVIEW.md#path-history-and-identity-limits) |
| M8-04 | Does map8 share an identity with the separately tracked empty `map1.mob`? | Keep the source keys separate; require evidence independent of the reused empty historical paths before relating them. [History](MAP8_AUDIT_REVIEW.md#path-history-and-identity-limits) |
| M8-05 | Is `oldmap8.wld` a companion, predecessor, or unrelated map? | Preserve its separate source identity; the similar filename alone does not establish a relationship. [History](MAP8_AUDIT_REVIEW.md#path-history-and-identity-limits) |
| M8-06 | Is the absence of same-stem WLD, OBJ, ZON, QST, SHP, story, or editor files expected? | Reconcile repository, editor, backup, and deployment inventories; the tracked component currently consists only of an empty MOB path. [Inventory](MAP8_AUDIT_REVIEW.md#inventory-and-empty-source-finding) |
| M8-07 | Should map8 remain outside `areas/AREA` and `areas/qst/AREA`? | Keep both lists unchanged unless an owner supplies a valid source set and intended area identity. [Area lists](../../../areas/AREA#L1), [QST list](../../../areas/qst/AREA#L1) |
| M8-08 | Are there external build, editor, export, or deployment references not present in tracked content? | The bounded areas/src/scripts/tests token and exact-path scan found no in-checkout reference; ask external tool and deployment owners to identify exact copies and inputs before any path change. [CP915 path scan](MAP8_AUDIT_REVIEW.md#checkpoint-915-exact-path-reference-refresh) [Reference limits](MAP8_AUDIT_REVIEW.md#references-and-area-list-status) |
| M8-09 | Did a former mobile prototype set or numeric identity disappear before this snapshot? | Request source-control or external asset evidence; the present empty file supplies no VNUMs for reference matching. [Reference limits](MAP8_AUDIT_REVIEW.md#references-and-area-list-status) |
| M8-10 | May the zero-byte file be removed, renamed, or reclassified after review? | Wait for the owner's explicit disposition and reconcile external copies before a separate cleanup decision. [Disposition](MAP8_AUDIT_REVIEW.md#disposition-and-owner-questions) |
