# fooquest builder questions

Canonical key: component:fooquest. Checkpoint 480; coverage row 393 /
CSV line 394; candidate dependency row 43. These are source-backed decisions,
not implementation instructions. Keep this component RETAIN IN PLACE until
the quest, map, code, manifest, and saved-state owners resolve it.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| FQ-01 | Who owns the WLD/MOB/OBJ/ZON/QST files, the extra WLD-directory mobile file, editor data, and deployment path? | Record each source, quest, code, editor, and deployment owner before changing loader membership or paths. |
| FQ-02 | Is FooQuest the canonical title, and what title or aliases should replace the blank raw AREA annotation? | Preserve the ZON spelling and blank annotation until the owner provides a documented title and alias crosswalk. |
| FQ-03 | Should the candidate remain absent from both full areas/AREA and areas/qst/AREA despite its QST and compiled special bindings? | Ask the world and quest owners to decide WLD and QST registration separately; do not infer load state from file presence. |
| FQ-04 | Are header #636, top VNUM #65050, and all 17 room VNUMs outside #63600-#63699 intentional? | Preserve the fields and have the owner explain the range scheme before changing headers or VNUMs. |
| FQ-05 | Which mobile set is intended: the extra #63600-#63611 pirates in areas/wld/fooquest.mob or the #65000-#65011 set in areas/mob/fooquest.mob? | Keep both files intact and require the source/tooling owner to identify the authoritative typed set. |
| FQ-06 | Why are the first twelve pirate records exact copies after changing VNUMs by 1,400, and what is the role of the WLD-directory file? | Record explicit lineage or generator evidence; do not call either set a replacement based only on matching text. |
| FQ-07 | Is the sixteen-room captain-ship graph complete, including the Goodie and Evil ship branches? | Preserve all 30 local directions and request a map-owner crosswalk before adding or removing passages. |
| FQ-08 | Is the disconnected room #65050 intended to be reachable from the ship map? | Confirm whether code or another world source is meant to enter it; preserve its current no-exit WLD record meanwhile. |
| FQ-09 | Are the 13 QST givers and other quest mobiles meant to load into room #65050, which has no WLD route? | Confirm the intended access and party flow with the quest and map owners before changing placements. |
| FQ-10 | Does any active route enter this FooQuest identity, given that the current active-WLD scan found zero inbound exits? | Keep candidate-identity inbound at zero until a source-backed route and target owner are identified. |
| FQ-11 | Are the 13 QST giver records #65015-#65034 and their matching ZON resets complete? | Preserve the QST and M records as separate typed data and verify every giver/reset pair with the quest owner. |
| FQ-12 | Is the 31-contract QST sequence the intended full version of Warthehr's eight-fragment, hilt, reforging, and Ghalasax story? | Preserve its 24 QA and seven Q directives, and document any omitted stages or external quest files before editing it. |
| FQ-13 | Are the story references to Caer Tannad, Turopolis, Pharr Valley, Evermeet, Jindonium, Verspin, Moonshaes, Corwell, Newhaven, and Kimodril still accurate? | Ask narrative and area owners to verify each named destination and the reward handoffs; prose alone does not create WLD links. |
| FQ-14 | Should Abal's QST consume active Moonshae key #26201 as well as candidate key #65035? | Preserve the exact typed Moonshae dependency until both quest owners confirm the item identity and return path. |
| FQ-15 | Why was unnamed OBJ #65004 removed in 2016, and is the missing prototype intentional? | Keep the source deletion and current VNUM gap intact unless the object owner supplies its purpose and a safe identity plan. |
| FQ-16 | Are the two keys/chests, eight fragments, hilt, reward items, and dragonslayer sword correctly represented in the 46-prototype OBJ file? | Cross-check all QST R/G item fields against typed candidate OBJ prototypes before any item renumbering or removal. |
| FQ-17 | Are the 320 consecutive G resets for tooth #65041 after chipmunk #65035 intentional, and should mobile #65036 remain unreset? | Preserve the raw reset records and obtain explicit reset-owner answers for the repeated teeth and unused healer prototype. |
| FQ-18 | Are the 37 M, 72 F, 330 G, and two E commands intended with reset mode 2? | Review the full reset policy and follower/equipment counts as a unit; do not treat command totals as a runtime result. |
| FQ-19 | Are overlapping room, mobile, and object VNUM ranges #65000-#65050 intentional across typed tables? | Keep room, mobile, and object namespaces separate and require typed references for every proposed change. |
| FQ-20 | Should code bind #65012/#65013 to fooquest_mob/fooquest_boss and dynamically use #65014? | Resolve procedure owners and prototype availability; preserve source bindings until the code and map owners approve a coordinated plan. |
| FQ-21 | Should the 13 quest-giver mobiles keep the compiled newbie_quest level gate despite the missing QST AREA entry? | Confirm the intended ask/level behavior and registration relationship with both code and quest owners. |
| FQ-22 | Is #65050 the intended dragonslayer sword prototype for the compiled special, despite sharing a VNUM with the separate quest-room WLD record? | Preserve both typed records and confirm the object special and its expected loaded prototype with the code owner. |
| FQ-23 | Does the trusted transform branch intentionally create #65013 and grant object #21 or #22 from current Heavens prototypes? | Confirm the transform flow, racewar choice, and item owner; preserve the external typed item references meanwhile. |
| FQ-24 | What evidence would permit a future activation, move, rename, merge, or archive proposal? | Require quest and map ownership, complete typed QST/reset/code closure, manifest and editor/deployment review, and persisted-ID signoff; retain in place until then. |

See the [static audit](FOOQUEST_AUDIT_REVIEW.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-480-fooquest-ship-map-and-quest-room),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-480-fooquest-title-history-and-prototype-sets).