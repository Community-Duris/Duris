# `dranquest` builder questions

Canonical key: `component:dranquest`. Physical coverage row 385; candidate dependency row 35. The [technical audit](DRANQUEST_AUDIT_REVIEW.md) records a reciprocal two-room stub, five mobiles, seventeen objects, one QST record, no ZON resets, and a text-level relationship to the active Heavens QST.

## Explicit builder questions and recommended resolutions

| ID | Question | Recommended resolution |
| --- | --- | --- |
| DQ-01 | Who owns `dranquest` and its editor, deployment, and persisted quest state? | Identify the map and quest owners before changing content, paths, or registration. |
| DQ-02 | Was exclusion from both `areas/AREA` and `areas/qst/AREA` intentional? | Keep the current manifest state until the world and quest-loader owners confirm the contract. |
| DQ-03 | Is the raw zone title `Quest` an intentional name or a placeholder? | Preserve the title and request the owning map identity; do not infer it from the filename. |
| DQ-04 | Is the reciprocal pair #12601/#12625 the complete intended map? | Confirm whether this two-room bookend is a stub, a test, or a deliberately minimal location. |
| DQ-05 | What source or authored route is intended to reach either room? | Keep the rooms unregistered and record owner-approved access separately from numeric references. |
| DQ-06 | Is the `dranquest` questline an alternate ending to the active Heavens QST about Rozkadrath and Joukarth? | Reconcile chronology and canon with both story owners; do not label either source a replacement. |
| DQ-07 | Should Balor #12601 ask for Joukarth #12602's head while Heavens has Rozkadrath help Joukarth pursue Blood War dominance? | Preserve both texts and obtain an explicit story decision before any quest or narrative edit. |
| DQ-08 | Is QST key #12601 intentionally attached to the Balor mobile #12601? | Confirm the typed giver and intended dialogue owner while keeping the matching VNUM types distinct. |
| DQ-09 | How is the requested head of Joukarth obtained and recognized? | Identify the intended item, reset, native contract, or code path; the candidate QST contains no numeric object ID. |
| DQ-10 | Why does the candidate ZON contain no M, O, G, or other reset commands? | Confirm whether the five mobiles and seventeen objects are intentionally unplaced or have an external placement owner. |
| DQ-11 | Are all seven seals #12601–#12607 a complete intended set for this quest? | Preserve their names and inscriptions until their puzzle and reward roles are documented. |
| DQ-12 | Are objects #12608–#12617, including Joukarth's relics and the chained goblin, intended parts of this source? | Review the full object list with the content owner; do not infer use from names alone. |
| DQ-13 | Is candidate Rozkadrath #12605 the same story character as the Rozkadrath in Heavens QST? | Confirm one character identity or an intentional alternate before merging any records. |
| DQ-14 | Do code checks for room #12613/#12614 in `albert()` refer to rooms removed from this source? | Resolve the legacy code owner and any earlier room files; those current candidate numbers are object VNUMs, not rooms. |
| DQ-15 | Was the `albert()` special intended to be assigned to any current mobile? | Confirm code ownership and assignment history before treating its room checks as live behavior. |
| DQ-16 | Do `fooquest` or `raxquest` share an authoring/test lineage with `dranquest`? | Keep their file identities separate until repository or builder evidence connects them. |
| DQ-17 | Do older editor exports or external archives contain a fuller Dranquest map or reset set? | Check available archives before deciding that the two-room WLD and empty ZON are complete. |
| DQ-18 | Who can approve activation, archival, or a future map/quest move? | Record the accountable map, quest, content, tooling, and data owners after lineage closure. |

[Coverage row 386](ZONE_COVERAGE_REGISTER.csv#L386) | [dranquest audit](DRANQUEST_AUDIT_REVIEW.md)
