# Tradeskills old catalog builder questions

These questions preserve the unresolved author and tool-owner decisions around
[`tradeskills.old.obj`](../../../areas/obj/tradeskills.old.obj#L1). The source
comparison is recorded in the
[audit review](TRADESKILLS_OLD_AUDIT_REVIEW.md); recommendations keep the
current active catalog and numeric bindings stable until an owner establishes
otherwise.

## Explicit builder questions and recommended resolutions

| # | Source-backed question | Conservative recommendation |
| --- | --- | --- |
| 1 | What was the intended purpose of adding `tradeskills.old.obj` in the 2013-11-18 commit that also changed `tradeskills.obj`? ([tracked history](TRADESKILLS_OLD_AUDIT_REVIEW.md#selection-story-and-history)) | Preserve the two paths and the commit evidence; do not infer intent from the `.old` suffix. |
| 2 | Was the old file meant to be a frozen snapshot, an alternate catalog, or a temporary working copy? | Keep it as source evidence until its author or current data owner identifies the role. |
| 3 | Which exact object file should future catalog edits target? ([manifest](../../../areas/AREA#L406)) | Continue edits against active `tradeskills.obj`; do not update the unselected copy as if it were a second live catalog. |
| 4 | Does any build, deployment, or recovery process outside the tracked manifest still read the old filename? | Verify generated and deployed object aggregates before changing or deleting the old path. |
| 5 | Should the old file remain in version control after its history has been recorded? | Retain it until tooling and ownership are reconciled; this audit does not authorize deletion. |
| 6 | Was the change from old #400232, an elven equipment bag, to active #400232, turkey innards, intentional? ([old](../../../areas/obj/tradeskills.old.obj#L1932), [active](../../../areas/obj/tradeskills.obj#L1936)) | Preserve current #400232 as turkey innards because maintained code binds that VNUM to `TURKEY_INNARDS_VNUM`. |
| 7 | Should any behavior or identity from the old #400232 bag survive under a current VNUM? | Do not remap it from this duplicate alone; require an owner decision and an explicit consumer review. |
| 8 | Are the changed numeric fields on #400218 an intentional update to the Netheril battle-mage robe? ([old](../../../areas/obj/tradeskills.old.obj#L1780), [active](../../../areas/obj/tradeskills.obj#L1784)) | Treat the active record as authoritative for current data and review the field meanings before changing it. |
| 9 | Why do old and active #400220 use different numeric flags while both point to #554045? ([old](../../../areas/obj/tradeskills.old.obj#L1809), [active](../../../areas/obj/tradeskills.obj#L1813)) | Keep the active corpse-portal record and its code consumers together; do not load the historical copy as a second portal prototype. |
| 10 | Is the historic type-25 destination #554045 intended to remain on the active #400220 prototype? | Preserve the current declaration and code path unless the travel owner reviews both together. |
| 11 | Were the identity updates to #400094 and #400098 intended material changes? ([old #400094](../../../areas/obj/tradeskills.old.obj#L753), [old #400098](../../../areas/obj/tradeskills.old.obj#L785)) | Keep the active definitions; do not restore earlier mineral identities based only on the old copy. |
| 12 | Are the long-description edits on #400120–#400122 and #400128/#400133 intended copy corrections? ([old steel records](../../../areas/obj/tradeskills.old.obj#L965)) | Preserve the active prose and verify any future shared-material changes against crafting output. |
| 13 | Which field-level changes on #400155, #400199, #400201, and #400216 are deliberate? | Review each changed record against its current consumer before carrying any field across. |
| 14 | Should the old #400217–#400233 event and equipment records be kept as a historical set or treated as stale duplicates? | Keep the old file intact for provenance; use only the active records for current references. |
| 15 | Do the 59 active VNUMs absent from the old file represent later catalog growth? | Keep them as active definitions; the count alone does not establish the old file's intended end date. |
| 16 | Should the 218 exact old records be considered redundant and safe to remove from the old file? | Keep exact matches in the historical copy; removing them would change its source record. |
| 17 | How should reset, quest, and shop references to shared IDs be documented? | Record the active target file as `tradeskills.obj`; numeric matches cannot prove use of `.old`. |
| 18 | Are any editor, import, or third-party tools configured to open or export `tradeskills.old.obj`? | Check those paths before renaming or archiving the file; none is established by the in-repository source scan. |
| 19 | Should the old object's #400220 → #554045 declaration be represented in any map story? | Keep it classified as a typed object destination, not a WLD edge or candidate room route. |
| 20 | Who owns the final retention or archival decision for this historic catalog copy? | Keep **RETAIN IN PLACE** until the data and tooling owners resolve provenance and aggregate-file state. |
