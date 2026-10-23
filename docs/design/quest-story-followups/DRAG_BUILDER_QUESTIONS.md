# `drag` empty-source questions

Canonical key: `component:drag`. Physical coverage row 384; candidate dependency row 34. The [technical audit](DRAG_AUDIT_REVIEW.md) records two zero-byte files and their bounded path history.

## Explicit builder questions and recommended resolutions

| ID | Question | Recommended resolution |
| --- | --- | --- |
| DRG-01 | Who owns the `drag` QST and SHP paths, and which editor or tool creates them? | Identify the source and tool owners before changing either path. |
| DRG-02 | Are both files intentionally zero bytes? | Keep them in place until the owner confirms placeholder or retirement intent. |
| DRG-03 | Why are they absent from both `areas/AREA` and `areas/qst/AREA`? | Preserve the current manifest state until the relevant loader owners confirm its purpose. |
| DRG-04 | What did the historical `Players/Declined/...` path represent? | Treat it only as a path-history clue until a builder or archived source explains it. |
| DRG-05 | What was the `Players/justice_list` path used for? | Do not infer that the name identifies a zone; request owner or editor context. |
| DRG-06 | Did older exports or external repositories ever contain QST or SHP content at these paths? | Check available source archives before concluding that the files were always placeholders. |
| DRG-07 | Do build scripts, editors, deployment packages, or persisted records key on either filename? | Complete external-tool and persisted-path review before archiving or deleting. |
| DRG-08 | Should the empty `drag.shp` remain in the shop directory while non-mini fallback reads every `*.shp` file? | Preserve it until the loader owner confirms whether an empty path marker is intentional. |
| DRG-09 | Does any external quest system refer to a `drag` source identity even though the file has no key? | Resolve external quest and persistence state separately; do not invent a QST VNUM. |
| DRG-10 | Is another current source stem the intended home for any former `drag` content? | Keep current files unchanged until an explicit lineage or replacement is documented. |
| DRG-11 | Who can approve a later archive, deletion, or path move? | Record the accountable owner after editor, archive, and persisted-state closure. |

[Coverage row 385](ZONE_COVERAGE_REGISTER.csv#L385) | [drag audit](DRAG_AUDIT_REVIEW.md)
