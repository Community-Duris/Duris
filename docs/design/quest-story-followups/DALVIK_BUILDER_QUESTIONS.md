# Dalvik’s Dockyard Builder Questions

Checkpoint 466 source-backed questions for the unregistered, low-VNUM Dalvik component. Recommendations preserve the current source identities until map, quest, and external ownership are confirmed.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| DL-01 | Who owns the five tracked Dalvik source files and the editor/deployment data associated with them? | Identify the source, map, quest, and tool owners before any path or content decision. |
| DL-02 | Was the component’s absence from both `areas/AREA` and `areas/qst/AREA` intentional? | Confirm the disabled/list-excluded state; do not activate or merge it from source presence alone. |
| DL-03 | Is “Dalvik’s Dockyard” the intended title for the full #1–#476 map, including its jungle, Manshaka, temple, arena, and harbor districts? | Record the owner-confirmed title and scope without splitting the current file identities. |
| DL-04 | How do the Dalvik and Manshaka names relate within the authored map? | Request the builder’s map history; do not infer aliases or zone boundaries from prose alone. |
| DL-05 | Is the map related to active Surface port #635260, its #635660 arrival room, or the separate #11006 boatseller? | Confirm each identity with the source owner; retain the name/VNUM matches as evidence, not equivalence. |
| DL-06 | Should room #298’s south exit to Surface #635660 remain in the inactive source? | Preserve the source-only direction until its route owner confirms intent and return requirements. |
| DL-07 | Which room #104 did Thetis #38095 north intend: Dalvik’s Great Temple of Ubtao, Mavulsk’s library, or another destination? | Keep the Thetis edge unresolved until its author or source history identifies the target. |
| DL-08 | Are the 34 weak components and the 65 rooms not reached from raw seed #1 intentional? | Ask the mapper to identify separate islands, one-way staging areas, and deliberate unreachable rooms. |
| DL-09 | Are the current door flags, secret exits, and exit pairings meant to produce the directed paths recorded in the source graph? | Review each route with the builder; static traversal does not qualify player access or safe return. |
| DL-10 | Are the seven fall fields and six current fields, including the overlapping #250–#255 hazards, still intended? | Confirm the environmental fields and their intended destinations before any map change. |
| DL-11 | Is raw zone header #1 with last VNUM #476 correct for this entire source? | Preserve the current range until the area owner confirms its namespace and reset bounds. |
| DL-12 | Are 54 door resets, only two mobile resets, and eleven object resets the complete intended ZON? | Compare against the owner’s map/reset records before changing or regenerating commands. |
| DL-13 | Are mobile #162 in room #162 and trade guild leader #2 in room #470 the intended placements? | Confirm the two reset roles with the builder; retain both local source IDs meanwhile. |
| DL-14 | Why is QST giver mobile #3 present in `dalvik.mob` and `dalvik.qst` but absent from this ZON’s mobile resets? | Ask whether it is intentionally unplaced or expected to be supplied elsewhere. |
| DL-15 | Are the eleven object resets complete, and are the two nonlocal imports intentional? | Confirm local placements plus the imported Heavens #72 and Tharnadia #132581 identities. |
| DL-16 | Does object #72 intentionally refer to the active Heavens fountain, and does #132581 refer to the active Tharnadia counter? | Preserve the numeric imports until both source owners confirm the intended prototypes. |
| DL-17 | How should the 50 same-number room overlaps with active WLDs be treated if the map is edited or moved? | Keep Dalvik’s room identity distinct; require explicit collision resolution before any VNUM remap. |
| DL-18 | Are Dalvik mobiles #1–#3 intentionally different from active Limbo mobiles with the same VNUMs? | Preserve the separate source records and identify the intended owner of each number. |
| DL-19 | Are Dalvik objects #1–#63 intentionally separate from the active Limbo #1–#9 and Heavens #10–#63 prototypes? | Do not merge or reassign these records based only on matching numbers. |
| DL-20 | Which source owner should maintain Dalvik’s sapphire bracelet #53 when active Heavens #53 is a rope? | Confirm both definitions and every reset or external reference before making a change. |
| DL-21 | Are the 210 QA entries under giver #2 and the 210 under giver #3 still the intended contract sets? | Review both complete lists with the quest owner; do not infer runtime behavior from the file alone. |
| DL-22 | Are the QST item references #400000–#400209 intentionally tied to active Tradeskills objects? | Confirm the shared item contract and keep the exact prototype ownership documented. |
| DL-23 | How are `dalvik.qst` records expected to enter the quest aggregate when the file is absent from `qst/AREA`? | Ask the quest-loader and packaging owner; do not add a glob or manifest entry without a separate design decision. |
| DL-24 | Is the absence of a same-stem SHP file intentional for the storefronts named in the room descriptions? | Confirm whether these rooms are descriptive only or whether shop data lives elsewhere. |
| DL-25 | Is the title “Dalvik’s Dockyard” the correct repository-facing name when current code also uses “Dalvik” for a Surface port? | Record an owner-confirmed alias map; do not conflate the Surface port with this candidate. |
| DL-26 | Do external editors, deployment scripts, or persisted records refer to these five paths or low VNUMs? | Obtain a repository-plus-toolchain reference map before any migration discussion. |
| DL-27 | Does the tracked history before the visible source-file import identify an earlier map owner or a replacement? | Request upstream records; the current checkout history is too short to establish that lineage. |
| DL-28 | Who can approve a later path, activation, or archival proposal after all dependencies are closed? | Identify the accountable owners and make any such proposal separately from this audit. |

[coverage row 380](ZONE_COVERAGE_REGISTER.csv#L380) · [Dalvik source and identity audit](DALVIK_AUDIT_REVIEW.md).
