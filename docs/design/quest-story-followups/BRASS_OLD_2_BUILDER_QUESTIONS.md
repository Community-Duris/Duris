# Brass Old 2 Builder Questions

Checkpoint 455 questions for the former 250-range `plane_fire_one` source,
now retained as the unregistered `brass-old-2` component. Each recommendation
preserves the checked-in evidence and identifies the owner decision needed
before any content or path change. See the [audit review](BRASS_OLD_2_AUDIT_REVIEW.md).

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| BO2-01 | Does `brass-old-2` represent the former `plane_fire_one *250-251` source preserved by the 2025 relabel? | Record that identity from the four matching Git blobs, and keep it distinct from current `plane_fire_one *254-255`, `brass-old-1`, and `brass-old-3`. |
| BO2-02 | Which builder or team owns the Efreeti palace and its named hierarchy: Shadam Quilen, Padashaw, Amir, Caliph, Kossuth, the Arch-Magi, and High Priest? | Name the content owner and confirm the relationships described across the rooms before proposing a story rewrite or archive change. |
| BO2-03 | Are Potions #30705 D1–D5 and #30706 D5 still intended to lead to Old 2 room #25087? | Preserve all six source exits; have the Potions and Brass owners select a loaded destination or approve route retirement in a separate change. |
| BO2-04 | Is Realm #14210 D9 southeast to #25087 still intended? | Preserve the seventh source exit and have Realm and Brass owners resolve it separately from the six Potions routes. |
| BO2-05 | Should all six Potions directions converge on the same Efreeti treasure room? | Keep the exact directions and ask the route owners to decide one by one; do not infer a new direction map. |
| BO2-06 | Should the `areas/qst/AREA` row `plane_fire_one *250-251` remain when the current QST file at that path came from `plane_fire_three.qst` and begins at #25401? | Ask the QST index owner to reconcile the stale-looking range and stem labels. Do not create a QST for Old 2 by inference. |
| BO2-07 | Is the absence of an Old 2 QST file, SHP file, and story sidecar intentional? | Keep the current inventory facts; add any future quest, shop, or story contract only through its own content review. |
| BO2-08 | Are the 57 Old 2 ZON M resets to `brass-old-3` mobiles #25100–#25210 intentional? | Preserve them as source-only cross-component references and resolve any prototype ownership or retirement with the Old 2 and Old 3 builders. |
| BO2-09 | Are the nine E/G resets to Old 3 objects #25101, #25110, #25117, #25118, and #25120 intentional? | Keep these separate from Old 2's local objects and retain Old 3's source until the owners close the dependency. |
| BO2-10 | What should happen to the two M resets of #25019 at `brass-old-2.zon` lines 210 and 225? | Treat them as unresolved mobile references: Old 2 #25019 is an OBJ key, and no current mobile prototype owns #25019. Do not change the command type or VNUM without the builder. |
| BO2-11 | Is Old 2's O #60 reset meant to use active Heavens object #60, “glowing pool,” or a same-number object in an unlisted OBJ source? | Preserve the reset and identify its intended prototype with the owner; numeric VNUM alone does not distinguish the different #60 objects. |
| BO2-12 | Is Old 2's G #67245 reset meant to import the active Unique object? | Preserve the reference and confirm its intended use with the Old 2 and Unique owners. |
| BO2-13 | Should code still bind `guild_guard` to Old 2 mobile #25000? | Have the code and area owners decide whether to update the VNUM binding or restore an intended loaded prototype in a separately reviewed change. |
| BO2-14 | Should `ring_elemental_control` and Firep's rub behavior still name Old 2 object #25080? | Preserve the code references and ask the behavior owner whether the archived object remains the intended identity. |
| BO2-15 | Should the room-based guard behavior in `specs.guards.c` still name Old 2 rooms #25001 and #25086? | Preserve both room IDs and have the guard/code owner confirm their current purpose before changing room cases. |
| BO2-16 | Does `randomeq.c`'s #25040 “Brass Sultan” entry still have a purpose? | Treat it as an inactive reference because its lookup loop is commented out; do not count it as live stock or an active consumer. |
| BO2-17 | Do any editor, map-generator, or source-management projects still open the old `plane_fire_one` paths? | Identify and close tooling dependencies before a separate path decision. |
| BO2-18 | Are there persisted room, mobile, object, or player-owned item references tied to the 250xx identities? | Obtain the owner-approved data inventory before considering a VNUM remap or source retirement. |
| BO2-19 | Are all 105 Old 2 rooms meant to remain preserved as a separate historical map? | Keep all source files in place until the area owner confirms whether the map is archive material, active design material, or retired content. |
| BO2-20 | Is any live access, reset behavior, or player return route intended for this unlisted component? | Do not infer runtime access from the source graph; require a separately approved gameplay journey before describing a live route. |
| BO2-21 | Can any part of this component be moved or deleted now? | No. Keep candidate row 18 **RETAIN IN PLACE** until its WLD, ZON, code, QST-index, editor, persisted-data, and owner dependencies have separate closure. |
