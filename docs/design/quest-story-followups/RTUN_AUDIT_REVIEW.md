# Tunnels of Payang (rtun): comprehensive source audit

**Checkpoint:** 343, coverage register row 255.  
**Disposition:** DO NOT MOVE. No gameplay, zone, or migration change is proposed.

This audit reconciles the existing [environmental story dossier](RTUN_STORY_REVIEW.md) with reset-aware local reach, all active and inactive WLD edges, normalized title/history, typed imports and consumers, and explicit builder decisions. It is based on checked-in source; it does not qualify live doors, shop behavior, ordinary-player access, scripts, or safe travel.

## Environment and story record

The crypt source describes dwarven masonry repurposed into an undead fortress, with ancient tombs, a shrine and armory, a laboratory, and newer corpse-digging tunnels. Existing room descriptions group into 41 distinct title/description pairs among 87 rooms; repeated prose alone is not treated as duplicate-room evidence. See the [existing environmental review](RTUN_STORY_REVIEW.md).

The [story sidecar](../../../areas/story/rtun.story.json#L5) says coverage is complete, while its [contact, story, and exclusion lists](../../../areas/story/rtun.story.json#L11) are empty. Its introduction promises that meeting residents will uncover local requests. There is no areas/qst/rtun.qst, although [the QST area index lists rtun](../../../areas/qst/AREA#L284). Static source therefore documents no local Q/QA objective or authored contact; it does not rule out behavior implemented elsewhere.

Several rooms need an explicit builder disposition. Rooms 90381–90384 are titled “King Load”; 90381 has text “Doowap Doowap” and links outward to 90382–90384 and 90337, while 90382–90384 have no exits. Room 90385 has no description but resets a lurking shadow beast. Banker room 90387 also has no description, despite resetting Nyroclex and a bank counter. These rooms are not reachable from any of the three active arrivals in the raw local graph.

## Active connectivity and reset-aware reach

The [active exit census](ZONE_COVERAGE_REGISTER.csv#L255) records 87 rooms and 172 exits: 170 local exits and two resolved outbound Surface exits. All 87 rooms are in one weak component. Three active inbound records are present:

- Heavens room 30 exits north to rtun room 90346; there is no direct reverse edge in the active census ([pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L179), [exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L10)).
- Surface room 574593 exits down to rtun room 90376. Room 90376 exits back up to 574593 ([pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L412), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1112), [reverse](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1496)).
- Surface room 576997 exits west to rtun room 90380. Room 90380 exits back east to 576997 ([pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L412), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1112), [reverse](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1504)).

From each arrival, 83 of 87 rooms are both reachable and able to return to that arrival in the raw directed local WLD graph. The other four, rooms 90381–90384, have no incoming path from the reachable graph.

The zone resets mark six door pairs at 100%: five closed pairs and one closed/secret ladder pair. The pairs are 90311/90366, 90319/90320, 90325/90326, 90340/90344, 90341/90380, and 90348/90349 ([reset declarations](../../../areas/zon/rtun.zon#L8), [shrine and ladder](../../../areas/zon/rtun.zon#L16), [ladder and side tunnel](../../../areas/zon/rtun.zon#L24), [burial plot](../../../areas/zon/rtun.zon#L32), [crypt tombstone](../../../areas/zon/rtun.zon#L40), [upper tombs](../../../areas/zon/rtun.zon#L48)). Treating state 1 as closed and state 5 as closed/secret, only 53 rooms are mutually reachable with arrival 90346 and only 53 with arrival 90376. Arrival 90380 reaches only itself while the closed tombstone door to 90341 remains shut. These are conservative reset-state graph counts, not proof that the doors are locked or that players cannot open them.

Rooms 90326 and 90327 each declare a 30% fall chance; their ladder description says the shaft may buckle ([90326](../../../areas/wld/rtun.wld#L497), [90327](../../../areas/wld/rtun.wld#L517)). No local room declares a current field. Do not describe the ladder or any arrival as safe from these records alone.

## Inactive map approaches

The all-95-file inactive-WLD sweep found no room-number overlap with rtun and two source-only approaches in surf.wld:

- Surface source room 263253 exits west to rtun 90380 ([source room](../../../areas/wld/surf.wld#L1327293)).
- Surface source room 270444 exits down to rtun 90376 ([source room](../../../areas/wld/surf.wld#L1478212)).

Neither source room belongs to the active loader set, so these records are not active ingress. Preserve them on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). This raises the reviewed Surf source-only subset to 259 and the curated source-only total to 4,495 (Surf 259, ud_map1 two, ud_map2 20, Underworld2 eight). Surf's active inbound reference count remains 145.

## Typed dependencies and code hooks

Local rtun.zon resets and the four local shop records depend on prototypes owned by other active areas:

| Local use | Prototype owner and evidence |
| --- | --- |
| Piper instruments 36245–36248 | Llzazan objects ([object source](../../../areas/obj/llzazan.obj#L653)); also listed by the local piper shop. |
| Gravedigger stock/reset objects 53, 500, 377, and 3050 | Active Heavens owns 53 ([object source](../../../areas/obj/heavens.obj#L788)), 377 ([object source](../../../areas/obj/heavens.obj#L3933)), and 500 ([object source](../../../areas/obj/heavens.obj#L5119)); active Verz1 owns 3050 ([object source](../../../areas/obj/verz1.obj#L850)). |
| Battle-forger stock/reset objects 822–828 and 835, plus shovel 191 | Heavens object definitions ([822–828 and 835](../../../areas/obj/heavens.obj#L7715), [shovel](../../../areas/obj/heavens.obj#L2274)); stock and reset roles are recorded in the local [shop](../../../areas/shp/rtun.shp#L76) and [zone resets](../../../areas/zon/rtun.zon#L145). |
| Bank counter 3097 in room 90387 | Verz1 object ([object source](../../../areas/obj/verz1.obj#L1199)); the local room reset is in [rtun.zon](../../../areas/zon/rtun.zon#L66). |
| Weaponsmith 90325 | The local shop record has no listed goods ([shop](../../../areas/shp/rtun.shp#L117)). |

Llzazan, Heavens, and Verz1 are active full-area contributors at [areas/AREA lines 174, 2, and 15](../../../areas/AREA#L174). The referenced owners and source-only duplicate declarations are mapped in the [existing dependency dossier](RTUN_STORY_REVIEW.md#reset-and-cross-area-dependencies).

No outside active QST item command, SHP product, or structured story-sidecar reference targets a local rtun object, mobile, or room. The other 441 active or unregistered ZON files have 17 mobile reset rows targeting local carrion crawler 90305, all in active mobs_underdark.zon at rooms 24006, 24007, 24009, and 24011 ([first cluster](../../../areas/zon/mobs_underdark.zon#L146), [later cluster](../../../areas/zon/mobs_underdark.zon#L459)); no F or R reset role targets an rtun prototype. No outside same-type mobile or object prototype definition duplicates an active rtun prototype. A scan of 872 external type-25 object declarations found no target into an rtun room.

Object 53 also has definitions in source-only dalvik, hostel, and mavulsk component files ([Dalvik](../../../areas/obj/dalvik.obj#L681), [Hostel](../../../areas/obj/hostel.obj#L695), [Mavulsk](../../../areas/obj/mavulsk.obj#L873)). Those components are outside areas/AREA ([coverage rows 380, 403, and 421](ZONE_COVERAGE_REGISTER.csv#L380)); these declarations are not evidence of loaded active owners.

Direct C/C++ numeric references add several service and legacy hooks:

- Payang's justice table uses guard room 90341 and mobile 90313 with chaos justice and no jail ([justice table](../../../src/combat/justice.c#L94)).
- money_changer is assigned to mobile 90387 ([special assignment](../../../src/specs/specs.assign.c#L225)); the same file explicitly assigns no mobile special to 90306 and 90328 ([assignments](../../../src/specs/specs.assign.c#L441)).
- An old NPC-only IS_RACEWAR_EVIL path sets home, birthplace, and original birthplace to room 90363 ([legacy assignment](../../../src/cmd/actoth.c#L8305)).
- No outside same-type prototype, external type-25 portal, or scanned QST/SHP/story reference requires a local room, mobile, or object to move. These bounded source scans do not cover persisted data, dynamic scripts, or untracked runtime configuration.

## Title, duplicate, and replacement history

After color-code removal, trimming, whitespace normalization, and case-folding, “Tunnels of Payang” occurs once among 442 current ZON titles and once among 351 active areas/AREA contributors. Same-type WLD, MOB, and OBJ number scans found no external prototype overlap. No alternate-stem predecessor, semantic duplicate, or replacement area was identified.

git log --follow keeps each tracked source on the rtun path from the 2009 area-tree move (642da5400). Later same-stem edits include OBJ/MOB maintenance in 2012 and 2013 (bb0d59425, 71d731c5b), MOB/SHP shop cleanup in 2014 (1c59fb690), and ZON overhaul in 2015 (f18992b8f); the story sidecar was added on 2026-10-02 (335552eec). This bounded tracked history shows continuity, not a distinct retired predecessor. The inactive component definitions of object 53 are recorded separately above and do not establish an active replacement.

## Builder questions and recommended resolutions

1. **Is the empty story sidecar intentional?** The sidecar says complete and promises local requests, but its three story arrays are empty. **Recommendation:** confirm whether the area is intentionally environmental-only; if so, align the sidecar introduction and scope without inventing quests or rewards.
2. **Is the QST area-index entry stale?** areas/qst/AREA names rtun, but no local QST file exists. **Recommendation:** have the owner reconcile the index and source file set after confirming the intended quest scope.
3. **Are rooms 90381–90384 public content or load/test rooms?** They are unreachable from all three active arrivals. **Recommendation:** confirm their intended status before changing room descriptions or graph links.
4. **Is the 90381-to-90337 exit intentional?** Room 90381 has an outbound down edge but no inbound path from the active graph. **Recommendation:** identify its intended caller and destination before preserving it as a public route.
5. **Should room 90385 remain undescribed?** A shadow beast resets there. **Recommendation:** confirm whether the blank description is deliberate; preserve the reset until the room's purpose is known.
6. **Is banker room 90387 meant to be a blank alcove?** Nyroclex and the Verz1 bank counter reset there. **Recommendation:** confirm the room presentation and banking location together before any change.
7. **Is the Heavens arrival meant to lack a direct return?** Heavens 30 exits north to 90346 without a direct reverse record. **Recommendation:** verify the intended return route in ordinary play before proposing an edge.
8. **Is the Bog of Death arrival at 90376 intended to be the main open approach?** Its active Surface edge is reciprocal and reaches the 53-room closed-door subgraph. **Recommendation:** preserve the pair and confirm intended arrival flow.
9. **Is the tombstone arrival at 90380 meant to land behind a closed door?** The paired up/down door to 90341 is reset closed. **Recommendation:** confirm how ordinary players open it before changing either area.
10. **Are all five state-1 door pairs intentional?** They separate the lab, shrine/armory, side tunnel, crypt entrance, and upper tombs. **Recommendation:** confirm keys, lock flags, and intended access in the live server before changing reset state.
11. **Is the state-5 secret ladder pair intentional?** It is closed and secret at 90325/90326. **Recommendation:** confirm the discovery method and return path before changing visibility or state.
12. **Are both 30% fall chances intended?** Rooms 90326 and 90327 share the ladder description and fall field. **Recommendation:** confirm that the risk and description agree; do not remove either field based only on static prose.
13. **Are the two inactive Surf approaches still historical map records?** They enter 90380 and 90376 only from source-only rooms. **Recommendation:** retain them on Surf row 97 and let the map owner decide their fate; do not treat them as loaded ingress.
14. **Are the piper instruments meant to remain Llzazan-owned?** Local stock and resets reference Llzazan objects 36245–36248. **Recommendation:** coordinate any replacement with Llzazan's object owner and preserve these IDs until then.
15. **Which active object 53 owner is intended for the gravedigger?** Heavens is active; three other component files define source-only object 53. **Recommendation:** confirm the Heavens object is the loaded stock/reset target and keep the inactive declarations separate.
16. **Are the merchant and battle-forger Heavens/Verz1 imports deliberate?** Stock and resets use Heavens 53, 377, 500, 191, 822–828, 835 and Verz1 3050. **Recommendation:** preserve their owners and reset/shop IDs unless both areas approve a coordinated change.
17. **Is the empty weaponsmith stock record intentional?** The shop has no listed goods. **Recommendation:** confirm whether it is a service stub or unfinished stock; do not infer a live shop offering from the record.
18. **Should Underdark continue resetting crawler 90305?** Seventeen active rows across four rooms consume the local mobile. **Recommendation:** treat 90305 as shared content and preserve its identity until all reset owners coordinate.
19. **Are the Payang justice and legacy birthplace references still intended?** Code names 90341/90313 for the justice guard and assigns birthplace 90363 on a legacy NPC-only path. **Recommendation:** confirm those hooks against the intended hometown roster before editing any referenced room or mobile.
20. **Are mobile special assignments correct?** Mobile 90387 receives money_changer; 90306 and 90328 have explicit null assignments. **Recommendation:** verify the live service handlers and preserve these numeric assignments until their owners approve changes.

## Safe disposition

Keep the active area and all prototypes in place. Preserve the three active inbound paths, two reciprocal Surface exits, imported local reset/shop objects, the 17 inbound crawler resets, and code hooks. Keep the two inactive Surf approaches on Surf's existing RETAIN IN PLACE row. No attic move, gameplay adjustment, or migration is supported by this audit.
