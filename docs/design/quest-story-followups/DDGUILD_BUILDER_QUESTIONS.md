# `ddguild` shop-source questions

Canonical key: `component:ddguild`. Physical coverage row 383; candidate dependency row 33. The [technical audit](DDGUILD_AUDIT_REVIEW.md) covers the single SHP record, its Asunwa source references, the shared `player_guilds` keeper-room pair, and the conditional full-mode loader risk.

## Explicit builder questions and recommended resolutions

| ID | Question | Recommended resolution |
| --- | --- | --- |
| DDG-01 | Who owns the `ddguild` stem, its shop record, and its editor/deployment state? | Identify the map and shop owners before changing the record or its path. |
| DDG-02 | Was omission from both `areas/AREA` and `areas/qst/AREA` intentional? | Keep the manifest state unchanged until the loader owner confirms the intended contract. |
| DDG-03 | Does `ddguild` identify a shop within Asunwa, Player Guilds, or another source? | Keep each source identity separate until its builders document lineage and planned use. |
| DDG-04 | Is keeper #35300 intended to be Asunwa's massive jaguar-man guard? | Preserve the typed reference and resolve its owner and full-area availability. |
| DDG-05 | Why is the configured shop room #35303 when Asunwa's source ZON places keeper #35300 in #35300, #35301, #35304, #35307, #35311, and #35316? | Confirm intended keeper placement and shop room together; do not infer or change either from the numeric match. |
| DDG-06 | Is object #35301, Asunwa's jaguar claw armor, intended as the sole `ddguild` product? | Preserve the product reference and confirm it with the Asunwa and shop owners. |
| DDG-07 | Should the `ddguild` catalog contain only #35301 while the `player_guilds` #35300 record also lists #5725, #3050, #6089, and #364? | Resolve the two product lists with both source owners before consolidating or editing either file. |
| DDG-08 | Are the two `#35300~` labels in `ddguild.shp` and `player_guilds.shp` deliberate separate records? | Preserve both records while the loader and data owners document their intended identity. |
| DDG-09 | If both records are read, how should the same keeper #35300 and room #35303 be distinguished? | Have the runtime owner qualify the shared keeper-room pair and the resolver's ambiguous result before relying on either catalog. |
| DDG-10 | Should the non-mini loader fallback include this unregistered file when `areas/world.shp` is absent? | Resolve the aggregate/fallback contract with the loader owner; do not treat the missing aggregate as proof that this file is inert. |
| DDG-11 | How should full-mode boot handle keeper #35300 when the only tracked same-type definition is outside `areas/AREA` and `real_mobile()` returns -1? | Resolve the negative keeper lookup and assignment path before exercising the fallback with the current manifests. |
| DDG-12 | How should product #35301 be handled when its only tracked object definition is in unlisted `asunwa.obj`? | Resolve the absent active object and intended stock before claiming the product can be loaded or sold. |
| DDG-13 | Are zero trade-category entries and one listed product the intended catalog configuration? | Confirm whether the record is stock-only and preserve the zero-entry list pending owner review. |
| DDG-14 | Are the seven `master` transaction templates intended for this shop and its audience? | Preserve the exact wording until the shop/content owner confirms its context. |
| DDG-15 | Is `I do not sell to your people!` intended with racist flag `N` and race 0? | Ask the content owner to confirm the message/flag combination; do not infer its effect from the literal text. |
| DDG-16 | Are the raw hours, non-roaming setting, magic flag, and killable flag intended? | Preserve the recorded fields and have the shop owner verify them as a set. |
| DDG-17 | Do editor files, deployment scripts, persisted shopkeeper records, or upstream history identify another owner or consumer? | Complete external-tool and persisted-identity closure before an archive or move proposal. |
| DDG-18 | Who can approve a later archive, move, manifest, or loader decision? | Record the accountable map, shop, content, and runtime owners after the identity questions are resolved. |

[Coverage row 384](ZONE_COVERAGE_REGISTER.csv#L384) | [ddguild audit](DDGUILD_AUDIT_REVIEW.md)
