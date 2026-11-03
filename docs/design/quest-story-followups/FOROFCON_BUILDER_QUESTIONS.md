# forofcon builder questions

Canonical key: component:forofcon. Checkpoint 482; coverage row 395 / CSV
line 396; candidate dependency row 45 / CSV line 45. These are source-backed
decisions, not implementation instructions. Keep the component RETAIN IN PLACE
until map, quest, reset, and identity owners resolve the questions below.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| FC-01 | Who owns the five forofcon data files and the tracked `.capmob`, `.capobj`, `.capqst`, and `.capzon` artifacts? | Record map, reset, quest, editor, and deployment owners; preserve the byte-identical backups and editor-version difference until their role is confirmed. |
| FC-02 | Is “Skull City and the Fortress of Conclusion” the canonical title when the raw AREA annotation is blank? | Preserve the unique ZON title and blank annotation until the area owner provides a title/alias record. |
| FC-03 | Should the WLD/ZON and QST be added to `areas/AREA` and `areas/qst/AREA`, respectively? | Have world and quest owners decide each manifest independently; absence from the lists does not resolve ownership or intended availability. |
| FC-04 | Are header #1, top VNUM #140302, and 403 rooms #139900-#140302 intentionally outside header #1's conventional #100-#199 range? | Preserve the raw fields and require a map-owner explanation before any header or VNUM change. |
| FC-05 | What is the intended relationship between all 403 candidate room VNUMs and the same-number inactive Duris3 rooms? | Keep both typed WLD sets; require an explicit lineage or generator record because every compared room block and title differs. |
| FC-06 | How should the first 53 rooms be kept distinct from active Smoke rooms #139900-#139952 with those same VNUMs? | Preserve Smoke's active identity and the separate forofcon descriptions; never treat same-number exits as candidate-identity ingress. |
| FC-07 | Is the 403-room graph's nine-component structure and 102-room directed reach from #139900 intentional? | Preserve the 817 authored exits and component metrics while the map owner confirms intended connectivity and entry points. |
| FC-08 | Is #139900 D3 with target -1 an intentional void direction? | Keep the negative destination as authored until the map owner identifies its meaning. |
| FC-09 | Are the five unique-source directions to Smoke IDs #139952, #139906, #139917, #139943, and #139941 still intentional? | Preserve these five already-counted source-only approaches on candidate row 45; do not add them to curated totals again. |
| FC-10 | Which exit-match count should the Smoke cross-reference use, given CP364 said 111/106 while a direct parse of the current 817 exits finds 109/104 plus the same five unique-source edges? | Use the CP482 direct parse, retain the five established source-only records, and keep curated totals unchanged. |
| FC-11 | How should forofcon's 19 M, 21 D, and one O reset placements into Smoke-numbered rooms be distinguished from active Smoke's own reset and portal roles in the shared #139900-#139952 band? | Preserve each source owner: forofcon's placements remain unregistered candidate resets, while Smoke's 78 M/F/O room operands, four D room operands, and two portal targets resolve to active Smoke rooms. Resolve both map identities and the #140030→#139903 candidate portal collision before any manifest change. |
| FC-12 | Is the duplicate coordinate reset at room #139945 direction D2 intentional in both Smoke and forofcon ZON sources? | Preserve each source line and have map/reset owners decide whether the coordinate collision is deliberate. |
| FC-13 | Are reset mode 2 and all 766 commands (386 M, 166 D, 57 O, 24 P, 67 G, 58 E, 8 F) intended? | Review the full ZON command set and its room/object/mobile roles as a unit; do not interpret command counts as a runtime result. |
| FC-14 | Should all 14 QST giver mobiles have M placements in the candidate ZON? | Preserve the exact 14 M-reset placements and confirm each intended room with the quest and reset owners. |
| FC-15 | Are the eight native Q contracts for Payvin, Fred, Sather, Elagor, Elanor, Bracos, and the two Baldzor records complete? | Preserve each exact G/R item exchange and review the associated dialogue with the quest owner. |
| FC-16 | Should the two separate Baldzor givers #140040 and #140049 offer the same blade-for-crystal-key exchange? | Keep both typed giver records and confirm whether the duplicated contract is intentional. |
| FC-17 | Are the eight QST item VNUMs all meant to resolve within candidate OBJ, with no external QST, story-sidecar, shop, or same-type MOB/OBJ consumer or prerequisite? | Preserve the current all-local QST closure; the cross-source role pass found no such external match. Keep compiled room-number consumers, editor/tooling, dynamic routes, and persisted identities under separate review. |
| FC-18 | Are eight WLD fall-chance records and eight current records intended in this map? | Preserve the F/C room records and ask the map owner to verify their chance, speed, and direction values. |
| FC-19 | Is the `plushit.c` source comment's PLUSFIVE inventory for candidate mobiles #139930, #139975, and #140039 still accurate? | Verify the comment against the candidate mobile flags; treat it as source documentation, not a compiled VNUM binding. |
| FC-20 | Are the overlapping WLD/MOB/OBJ VNUM ranges intentionally separate record types? | Keep all references typed; do not merge room, mobile, and object identities that share a number. |
| FC-21 | Are the three byte-identical `.cap` backups and the `.capzon` editor-version-only difference understood? | Preserve the artifacts and document their editor/generator role before cleanup or regeneration. |
| FC-22 | Is there an alternate-stem predecessor or whole-component replacement not established by the followed file history? | Retain the current files and require affirmative owner/history evidence before labeling any source a replacement. |
| FC-23 | Which editor, deployment, dynamic, or compiled path—if any—creates access to the candidate rooms and mobiles, and how should code references to #140000 be typed? | Record the owners and route. `olympus_portal` uses #140000 as a zone-bound lookup base, and encounter checks use `real_room(140000)` through `real_room(149999)`; current active-room resolution and candidate identity are not established. The #140030→#139903 portal also collides numerically with active Smoke. |
| FC-24 | What evidence would permit future activation, move, rename, archive, or merge proposals? | Require map/quest/reset ownership, manifest/editor/deployment review, typed-reference closure, and persisted-ID signoff; retain in place until then. |

See the [static audit](FOROFCON_AUDIT_REVIEW.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-482-forofcon-skull-city-and-fortress-map),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-482-forofcon-map-identity-and-history).
