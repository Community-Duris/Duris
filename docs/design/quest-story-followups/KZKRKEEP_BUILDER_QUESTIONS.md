# Kzkrkeep QST/SHP builder questions

These source-backed questions separate unresolved author intent from the
current numeric references. Recommendations preserve both source files until
the affected quest, area, shop, and deployment owners answer.

| ID | Builder question | Conservative recommendation |
| --- | --- | --- |
| K-01 | Are `kzkrkeep.qst` and `kzkrkeep.shp` intentionally absent from both `areas/AREA` and `areas/qst/AREA`? | Keep the current list membership unchanged until the area owner confirms which files, if any, belong in each production input. |
| K-02 | What is the canonical display name, location, and area identity behind the filename `kzkrkeep`? | Do not infer a Kzkr Keep map or a Worms identity from the filename alone. |
| K-03 | Who owns the QST and SHP sources, their editor/export workflow, and deployed copies? | Record owners and exact paths before proposing a source or list change. |
| K-04 | Is `Lirthandarn` supposed to have a MOB prototype, and if so, what is its intended VNUM and source file? | Keep the giver header #6917 unchanged until the mobile owner identifies the matching prototype. |
| K-05 | Is QST giver #6917 intended to be the same NPC as SHP keeper #6917? | Ask the quest and shop owners to confirm the shared identity; current active sources define no MOB #6917. |
| K-06 | Is SHP keeper #6924 intentional, and where is its MOB prototype maintained? | Do not assume OBJ #6924 or room #6924 substitutes for a missing mobile of the same VNUM. |
| K-07 | Should the first shop be located in active Worms room #6915, the smooth tunnel? | Confirm whether this is an intentional Worms placement or a stale location before changing the `in_room` field. |
| K-08 | Should the second shop be located in active Worms room #6919, the crumbly tunnel? | Confirm the intended room and area; the candidate has no WLD file of its own. |
| K-09 | Is the QST’s requested troll-leader amulet intentionally represented by Worms item #6900, a strip of grey wormskin? | Keep the existing reference until the quest author confirms the intended item identity. |
| K-10 | Is the QST’s “object of great value” intentionally Worms item #6912, a strip of purple wormskin? | Ask the quest and item owners to reconcile the text with the current OBJ prototype. |
| K-11 | Are `R I 6900` and `G I 6912` intended to bind the active Worms OBJ items, given that #6900 is also a Worms room used by Ixarkon's compiled teleport table? | Keep object and room namespaces separate. Ask the quest/item owners to reconcile the grey/purple wormskin objects with the amulet story before retargeting either command. [Role-aware refresh](KZKRKEEP_AUDIT_REVIEW.md#checkpoint-906-cross-source-typed-role-refresh). |
| K-12 | Should the first shop stock Worms products #6913-#6920? | Preserve the stock list until the Worms and shop owners confirm its intended area and availability. |
| K-13 | Why does the first shop list #6958, which is a Worms room and has no OBJ prototype? | Confirm the intended object VNUM; do not convert the room collision into an item. |
| K-14 | Should the second shop list #6968 and #6969, also Worms rooms without OBJ prototypes? | Resolve both stock entries against the item owner before exposing or editing them. |
| K-15 | Are the second shop’s thirteen Heavens object products intentionally sold from the #6919 Worms room? | Ask the shop owner to verify those cross-area product references and the chosen location together. |
| K-16 | Should these two SHP records be compiled from `areas/AREA`, remain only as fallback inputs, or be retired? | Compare the generated `areas/world.shp` used in deployment with the runtime fallback behavior before deciding. |
| K-17 | Is `areas/world.shp` normally present in the intended deployment, or does production boot use the `areas/shp/*.shp` fallback? | Obtain the deployed artifact and startup configuration; the absent generated file in this checkout does not answer the production question. |
| K-18 | Are there external or persisted quest identities for the #6917 contract? | Have the persistence owner check stored quest progress before any giver, contract, or path change. |
| K-19 | Does an editor, export job, or backup contain the missing Kzkrkeep WLD/MOB/OBJ/ZON sources? | Locate those copies and confirm their owner before treating this checkout as the complete historical source. |
| K-20 | After ownership and item/keeper references are resolved, should either source remain under the `kzkrkeep` stem? | Keep both files **RETAIN IN PLACE** until a separate, evidence-backed disposition is approved. |

See the [Kzkrkeep audit](KZKRKEEP_AUDIT_REVIEW.md),
[Worms typed-reference findings](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md),
[retention plan](ATTIC_MIGRATION_PLAN.md), [candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57),
and [coverage row 407](ZONE_COVERAGE_REGISTER.csv#L408).
