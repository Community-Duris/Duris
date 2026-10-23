# CBL Outpost 2 Builder Questions

Checkpoint 461 source-backed questions for the unregistered `cbl_outpost2` component. Recommendations preserve the current source while map, area, code, and data owners identify canonical ownership and intended behavior.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| CO2-01 | Who owns the `cbl_outpost2.wld`, `.mob`, and `.zon` files and any external editor data? | Identify the source and tooling owners before changing membership, paths, or room and mobile IDs. |
| CO2-02 | Is “CBL Guildhall Monster Island” the intended title for `cbl_outpost2`? | Confirm the canonical title and source identity with the original area owner. |
| CO2-03 | What is the relationship between `cbl_outpost2` and unregistered `Monsteri`, which has the same title, room/mobile VNUMs, and reset group? | Keep both sources separate until the owners document whether one is a copy, predecessor, or independent version. |
| CO2-04 | Is either unregistered source the canonical map for the Monster Island content now present in active CBL Hall? | Ask the CBL and source owners to identify lineage; shared text alone does not establish which source replaced another. |
| CO2-05 | Should active CBL Hall continue to own its current #29820–#29824 Monster Island rooms? | Preserve those active identities unless the area and persisted-data owners approve a documented change. |
| CO2-06 | Why does the unregistered map use room VNUMs #1000–#1004 while the corresponding active CBL rooms use #29820–#29824? | Record the intended crosswalk and owner for both maps before any renumbering, merge, or activation proposal. |
| CO2-07 | Should active Heavens continue to own mobile VNUMs #1000–#1003 for its different “unkillable bastard” and peasant prototypes? | Preserve current Heavens prototype identities and resolve the numeric collision with the four golems before any candidate loading. |
| CO2-08 | Are candidate mobile IDs #1000–#1003 intended to be separate from Heavens IDs with the same numbers? | Obtain an explicit typed VNUM decision; matching IDs do not imply matching mobiles. |
| CO2-09 | Do any active or unregistered resets, quests, shops, scripts, or editor tools rely on either set of #1000–#1004 values? | Complete a source-wide typed-reference and tooling check with each owner before changing IDs or paths. |
| CO2-10 | Are the ZON header and WLD room fields of 1000 intentional for rooms #1000–#1004? | Confirm whether this is legacy numbering or a range mismatch; retain the raw source values until then. |
| CO2-11 | Should the candidate's five rooms be assigned a VNUM range under zone 10 according to the current VNUM/100 convention? | Have the map owner settle the numbering scheme and persisted references before any correction. |
| CO2-12 | Is the generic #1000 “First Room” title and empty description intentional? | Confirm whether this room is a placeholder, an arrival room, or a preserved blank record before editing it. |
| CO2-13 | Is the #1001 torture chamber's explicit blood and torture imagery still intended? | Obtain the content owner's decision on the horror intensity before a separate prose revision. |
| CO2-14 | Does the #1002 pit and #1003 stairway form the intended downward exploration route? | Confirm the map path and any hazard, fall, or return contract without inferring one from the descriptions. |
| CO2-15 | Does room #1004's “ticket off the island” text imply a portal feature or only environmental prose? | Ask the area and gameplay owners to identify the actual exit or handler contract; preserve the authored local exit meanwhile. |
| CO2-16 | Is the current #1004-to-#1003 WLD exit the intended return path from Portal Cave? | Confirm arrival and return behavior with both map owners before changing the direction or destination. |
| CO2-17 | Should all four golems reset into room #1000, one of each prototype per zone cycle? | Confirm the four M commands, room placement, and population caps with the builder; do not assume reset execution. |
| CO2-18 | Are the four candidate golem prototypes intended to correspond to active CBL Hall mobiles #29820–#29823? | Ask the mobile owner to compare behavior and prototype fields; similar roles do not establish exact equality. |
| CO2-19 | Should `cbl_outpost2` remain without an OBJ file, SHP, QST, or story sidecar? | Confirm whether this is intentionally a map and mobile source only, or whether another owner holds the missing contract. |
| CO2-20 | Should the five room descriptions and reset layout remain a second source copy beside `Monsteri` and active CBL Hall? | Preserve all copies until the canonical source and historical purpose are documented. |
| CO2-21 | Is the active Heavens #1000 mobile intended to have any reset or code use beyond its prototype definition? | Confirm active mobile consumers without treating the prototype's presence as proof of a spawn or game behavior. |
| CO2-22 | Does tracked history identify `cbl_outpost2` as a rename or predecessor of `Monsteri` or active CBL Hall? | Record only a documented rename or replacement; same text and reset commands are not sufficient. |
| CO2-23 | Should this map remain absent from the full area and quest manifests? | Confirm the intended loading and quest-list status with the area owner after VNUM and content questions close. |
| CO2-24 | Who owns persisted room/mobile identity and any future path-only archival decision? | Identify data and editor owners and request separate authorization only after typed and tooling closure. |
