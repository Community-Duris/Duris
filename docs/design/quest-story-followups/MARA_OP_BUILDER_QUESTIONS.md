# Mara Outpost builder questions

Resolve these with the Mara Outpost, Mara Guildhall, Duris3, item, and
editor/deployment owners before changing the area lists, VNUMs, resets,
portal fields, or paths.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| MOP-01 | Who owns the four unlisted `mara_op` source files? | Record content and tooling owners; preserve the current files until the source role is confirmed. [Inventory](MARA_OP_AUDIT_REVIEW.md#inventory-and-story-content) |
| MOP-02 | Is `The Khomani Kan Outpost of The Marauders` the canonical area title? | Keep the literal `.zon` title and filename stem distinct until the area owner confirms aliases. [Inventory](MARA_OP_AUDIT_REVIEW.md#inventory-and-story-content) |
| MOP-03 | Should Mara Outpost remain outside `areas/AREA` and `areas/qst/AREA`? | Leave both lists unchanged until shared identities and intended loader inputs are reconciled. [Area lists](../../../areas/AREA#L1), [QST list](../../../areas/qst/AREA#L1) |
| MOP-04 | Is Mara Outpost a separate map from Mara Guildhall despite shared room/mobile/object VNUMs? | Keep the source keys distinct and have both owners designate an authoritative definition or an intentional shared-input plan. [Shared VNUMs](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition) |
| MOP-05 | Should rooms #29610–#29612 have the same text/exits but different room flags in both WLD files? | Confirm the intended flags and map ownership; do not merge the records based on matching descriptions. [Graph](MARA_OP_AUDIT_REVIEW.md#wld-graph-and-duris3-boundary) |
| MOP-06 | Which stats belong to the duplicate #29610 pillar and #29611 Rednax-Ela prototypes? | Compare both MOB files with their owners and preserve both records until the correct stat fields and identity are confirmed. [Shared VNUMs](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition) |
| MOP-07 | Are both sets of resets for #29610/#29611 intended to coexist? | Keep Mara Outpost and Mara Guildhall reset commands source-only and confirm their separate placement roles. [Resets](MARA_OP_AUDIT_REVIEW.md#resets-and-portal-references) |
| MOP-08 | Is #29610 up to Duris3 room #124408 an intended Outpost entrance? | Preserve the edge and coordinate its owner with Duris3; it is not active ingress. [Boundary](MARA_OP_AUDIT_REVIEW.md#wld-graph-and-duris3-boundary) |
| MOP-09 | Should the duplicated #29610-to-#124408 WLD direction in Mara Guildhall count as a second authored source record? | Keep both source file records distinct and do not add either to active-route totals. [Boundary](MARA_OP_AUDIT_REVIEW.md#wld-graph-and-duris3-boundary) |
| MOP-10 | Is portal #29610 intended to send users to Mara Guildhall room #29608? | Confirm the `value[0]` target with both map and item owners; the target is absent from Mara Outpost WLD. [Portal](MARA_OP_AUDIT_REVIEW.md#resets-and-portal-references) |
| MOP-11 | Is portal #29610's Good destination text compatible with its #29608 room target? | Preserve the value and description until the item owner confirms their intended relationship. [Portal](MARA_OP_AUDIT_REVIEW.md#resets-and-portal-references) |
| MOP-12 | Is portal #29611 intended to target local room #29612, the Blazing Vortex? | Confirm the numeric and descriptive destinations together; do not infer destinations from prose alone. [Portal](MARA_OP_AUDIT_REVIEW.md#resets-and-portal-references) |
| MOP-13 | Why do both portal OBJ definitions share VNUMs but differ in keywords/fields? | Keep the records separate and confirm whether their differing metadata is intentional. [Shared VNUMs](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition) |
| MOP-14 | Is the unlisted Duris3 boundary intended with no authored WLD return? | Ask the map owners to confirm access direction and preserve the current one-way source evidence until resolved. [Boundary](MARA_OP_AUDIT_REVIEW.md#wld-graph-and-duris3-boundary) |
| MOP-15 | Does the plaque's song text and the staircase imagery belong to the intended Outpost story? | Confirm narrative ownership and intended presentation without inventing a QST or progression contract. [Story](MARA_OP_AUDIT_REVIEW.md#inventory-and-story-content) |
| MOP-16 | Should this source have a local QST, shop, or story sidecar? | Ask the content owner whether those contracts live elsewhere; keep current inventory counts honest. [Inventory](MARA_OP_AUDIT_REVIEW.md#inventory-and-story-content) |
| MOP-17 | Are there external editor, export, backup, or deployment copies that define these same VNUMs? | Reconcile the tool/deployment inventories with both Mara source families before any path change. [History](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition) |
| MOP-18 | Do history and matching prose establish that either map replaced the other? | Treat the 2009 import, recode, and duplicate descriptions as limited evidence; require direct owner confirmation. [History](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition) |
| MOP-19 | Should the four source files remain together under `mara_op`? | Keep the current paths until owners confirm the authoring/export package boundary. [Inventory](MARA_OP_AUDIT_REVIEW.md#inventory-and-story-content) |
| MOP-20 | May Mara Outpost be merged, moved, or activated after review? | Wait for a coordinated owner decision covering shared VNUMs, portals, Duris3 boundary, lists, and external copies. [Disposition](MARA_OP_AUDIT_REVIEW.md#shared-vnums-history-and-disposition) |
