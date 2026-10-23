# Mavulsk builder questions

These questions are source-backed prompts for a builder or map owner. They
record unresolved decisions; they do not imply that gameplay or policy answers
have been supplied.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| MV-01 | Is “Mavulsk — the Minotaur Settlement” the canonical title, and are there any owner-confirmed aliases for this source stem? | Keep the decoded .zon title as the current source label; record any alias only after the owner identifies its use. |
| MV-02 | Is the five-file WLD/MOB/OBJ/ZON/QST package intentionally absent from both areas/AREA and areas/qst/AREA? | Confirm its owner and intended loading/index status before changing either list. |
| MV-03 | How should the 233 room, 98 mobile, and 110 object VNUMs be reconciled when every same-type VNUM also exists elsewhere? | Preserve Mavulsk as its own source identity and resolve the complete collision set before considering activation or renumbering. |
| MV-04 | Should any of the 94 exit targets that currently resolve to active room numbers be treated as external boundaries even though their VNUMs also lie in Mavulsk’s own #1–#233 band? | Treat targets within Mavulsk’s own band as local unless the map owner identifies a deliberate external link. |
| MV-05 | Which destination did Thetis #38095 north → #104 intend: Mavulsk’s Quiet Library, Dalvik’s Great Temple of Ubtao, or another room? | Keep the Thetis exit unresolved until source history or its author identifies the intended owner. |
| MV-06 | Does Mavulsk #188 east → #228202 intentionally leave the map, and which source owns its destination: surf.wld, surface2011.wld, or neither? | Preserve the exact source edge and both #228202 identities; do not infer an alias or replacement from their shared VNUM. |
| MV-07 | Should either #228202 destination have a return route to Mavulsk #188? | Ask both map owners to confirm boundary direction and return ownership before adding or changing an exit. |
| MV-08 | Are the four WLD weak components (228, 3, 1, and 1 rooms) intentional, given the local ITEM_TELEPORT fields? | Review the WLD graph together with the six local teleport objects; do not treat WLD-only directed reach as the complete travel graph. |
| MV-09 | Are the six ITEM_TELEPORT value[0] targets #154, #153, #176, #153, #226, and #225 correct for objects #79, #82, #83, #88, #92, and #95? | Confirm each destination and its intended object placement; preserve the source fields until the item owner decides. |
| MV-10 | Is room #180 (“Room Without Function”) meant to remain isolated, and what does its “THIS ROOM SHOULD BE !PC” note mean to the map owner? | Keep the room source record distinct and clarify its intended status before changing its flags, exits, or reset use. |
| MV-11 | Are the two F=50 fields on mining-shaft rooms #127 and #128 deliberate fall chances? | Preserve both fields pending confirmation of their intended hazard values and destination behavior. |
| MV-12 | Do the 122 D commands still describe the intended door states across the map’s locked, hidden, and ordinary directions? | Compare each reset with its paired room descriptions and preserve the authored default state until the builder approves a correction. |
| MV-13 | Is reset mode 1 and the 484-command reset program still intended for this zone, including its population and equipment placements? | Keep the ZON source-only; confirm reset timing and counts with the owner before any loader-list change. |
| MV-14 | Which current source should own low-number room, mob, and object definitions, and which external reset or teleport references are intended for Mavulsk? | Keep source records separate; resolve active Heavens M #2 against active Limbo #2 and separate Mavulsk/Mentiri #2 prototypes, then reconcile imported object reset numbers and the 17 active ITEM_TELEPORT fields to #2/#9/#50/#100/#150/#200 before assigning ownership. |
| MV-15 | Should the active Limbo references to #1 be treated as Mavulsk-local links? Mavulsk #2 and #3 both target its own room #1. | Preserve these as map-local references unless the author confirms an actual Limbo boundary. |
| MV-16 | Does the wife-minotaur QST #18 intentionally ask the player to find the missing child and route a letter/reward through objects #103 and #102? | Confirm the intended giver, child placement, item sequence, and handoff before presenting the contract as complete. |
| MV-17 | Are Alizalin #22 and Walgor #23 intended as one three-stage hide-processing chain? | Confirm the required and returned objects at each handoff and keep the stages distinct unless the builder approves a combined contract. |
| MV-18 | How should Doctor #25’s troll-shaman account and One-Eye #36’s lost-eye story relate, and which item chain is intended to connect them? | Preserve their separate giver records and confirm the intended sequence and rewards with the quest owner. |
| MV-19 | Are Captain #48’s fire investigation and troll-chief thread still intended, and is the mob’s PLUSONE flag meant to remain? | Keep the code note and source flag as evidence; confirm both narrative threads and the prototype requirement with the relevant owners. |
| MV-20 | Does Baron #58’s missing-miners investigation lead into the mine and item chain discussed by Drablin #82, or are these separate story threads? | Record the owner-confirmed relationship; do not infer a continuous quest from shared dialogue topics. |
| MV-21 | Is Advisor Ivlor #51’s aquarium/crown exchange meant as a standalone trade? | Preserve the current item references and confirm whether it is intended to connect to another contract. |
| MV-22 | Should Orc emissary #52’s trouble dialogue remain a lore-only giver with no Q clause, and what are the intended roles of the remaining QST topics on the hysterical woman, Griber, and disguised dwarf? | Keep dialogue-only content separate from contracts unless the owner identifies missing or intentionally removed Q clauses. |
| MV-23 | Merchants, a restaurant, and an armory appear in the room/mobile source, but there is no same-stem SHP file. Are these dialogue-only services or are shop records owned elsewhere? | Identify any external shop source and keeper VNUMs before promising stock or creating a new SHP. |
| MV-24 | How should the 18 Q clauses and 39 R/G item references be indexed when mavulsk.qst is not listed in areas/qst/AREA? | Confirm whether the QST index is intentionally absent and preserve object ownership until the index owner responds. |
| MV-25 | Do the followed same-stem histories reflect one maintained map, and are there editor, deployment, or archived copies not represented in Git? | Preserve current paths and ask the source owner to reconcile external copies before a separate migration decision. |

See the [Mavulsk source audit](MAVULSK_AUDIT_REVIEW.md) for the inventory,
graph, typed references, collision evidence, and file links behind these
questions.
