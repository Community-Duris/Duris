# `cript` SHP Builder Questions

Checkpoint 465 source-backed questions for the zero-byte, unregistered `cript` shop-file candidate. Recommendations keep the tracked path intact while its purpose and external ownership are clarified.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| CR-01 | Who owns the tracked `areas/shp/cript.shp` path and any editor-side data? | Identify the shop, area, and tooling owners before changing or deleting the file. |
| CR-02 | Is the zero-byte file intentional? | Ask whether it is a placeholder, legacy marker, or accidental truncation; preserve the current file until confirmed. |
| CR-03 | Was a shop record, keeper, room, product list, or message ever intended for this source? | Require a source-backed record from the owner rather than constructing a shop from the filename. |
| CR-04 | Is `cript` an area alias, a typo, or only a legacy SHP filename? | Record a title or alias only after the owner identifies its meaning. |
| CR-05 | Should this empty path remain tracked in the repository? | Keep it RETAIN IN PLACE until the source and external-data owners approve a separate disposition. |
| CR-06 | Should the non-mini fallback continue to encounter this path through its `areas/shp/*.shp` glob? | Review the fallback and source contract separately; do not infer that manifest exclusion protects the file from the glob. |
| CR-07 | Do any external editors, deployment tools, or manual packaging steps expect this file? | Obtain a complete toolchain reference map before considering a path or file change. |
| CR-08 | Are there persisted or operational references that do not appear in the repository? | Ask the relevant owners to search their records before treating the file as unused. |
| CR-09 | Does the historical path move through the Players tree describe a real area lineage? | Preserve the tracked history and record only owner-confirmed lineage; the current empty file has no content establishing one. |
| CR-10 | Who can approve a later archival or deletion decision for this path? | Identify the source, editor, and deployment owners and request a separate decision after closure. |
