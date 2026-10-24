# The Dark Guildhall: map, identity, and shop audit

Reviewed October 11, 2026, Checkpoint 468, against source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static review of the unregistered `dark_gh` component. It does not establish that its map, resets, or shop operate at runtime.

## Source scope and story

The candidate consists of [`dark_gh.wld`](../../../areas/wld/dark_gh.wld), [`dark_gh.mob`](../../../areas/mob/dark_gh.mob), [`dark_gh.obj`](../../../areas/obj/dark_gh.obj), [`dark_gh.zon`](../../../areas/zon/dark_gh.zon), and [`dark_gh.shp`](../../../areas/shp/dark_gh.shp). It is absent from both the full [`areas/AREA`](../../../areas/AREA) and [`areas/qst/AREA`](../../../areas/qst/AREA) lists and has no QST file or `areas/story` sidecar. Its raw AREA annotation is blank. The ZON title is “the Dark Guildhall,” header #1, last VNUM #29242.

The WLD contains five separate pieces: a ten-room guildhall and prismatic maze, a three-room unholy circle and vault, a three-room moss-covered cavern and moonwell, a three-room ravine and staircase by Krethik Keep, and a three-room roper hive. Other descriptions name a gate “to the Real World,” a Dope Deposit, an undead ward, and several themed portals. The source has no exit or connecting local path between those five pieces.

## Static WLD graph and active Newbie collisions

| Measure | Result |
| --- | --- |
| Rooms / authored directions | 22 / 38 |
| Candidate-local directions / outward directions | 34 / 4 |
| Weak components / largest | 5 / 10 rooms |
| Plain directed reach from #29200 | 10/22 candidate rooms |
| Current active-table resolutions | 34/38 directions: 33 local targets in Newbie and #123619 in Surface Keeps |
| Positive targets not in the active table / negative targets | 4 / 0 |
| Active inbound directions to a Dark-unique room identity | 0 |
| Fall / current fields | 0 / 0 |

Twenty-one Dark room VNUMs also name active [`newbie.wld`](../../../areas/wld/newbie.wld) rooms: #29201–#29212, #29220–#29222, #29230–#29232, and #29240–#29242. Each complete Dark room block differs from the same-number Newbie block. Dark #29200 has no active Newbie room with that VNUM. All 34 source-local directions stay within the Dark file, but one of them targets its unique #29200; the other 33 targets resolve numerically to Newbie rooms under the current active WLD set. They are not evidence that Dark is loaded. The active [`newbie.wld`](../../../areas/wld/newbie.wld) also has 50 directions targeting these shared numbers; those directions belong to Newbie’s own active room blocks and do not establish ingress to Dark.

Dark’s four outward directions are one-way in the tracked target room records:

| Source direction | Tracked target definition | Current active resolution |
| --- | --- | --- |
| #29200 up → #153032 | Inactive `Duris3.wld`, “Grassy Plains”; no reverse to #29200 | None |
| #29210 up → #212674 | Inactive `surf.wld`, “A Dark Forest,” and inactive `surface2011.wld`, “The Azure Waters of a Stormy Sea”; neither has a reverse to #29210 | None |
| #29220 up → #163940 | Inactive `Duris3.wld`, “Mountains”; no reverse to #29220 | None |
| #29230 up → #123619 | Inactive `Duris3.wld`, “Thick Forest,” and active `surfacekeeps.wld`, “A Huge Arch of Mystic Runes”; neither has a reverse to #29230 | Surface Keeps #123619 |

There are four source-external target references and three distinct targets absent from the active table. All four target numbers also occur in unregistered WLD files (`Duris3`, `surf`, or `surface2011`); #123619 additionally resolves to active Surface Keeps. No active WLD direction targets Dark’s unique #29200. The #29230 source room itself overlaps active Newbie, so its one-way direction to Surface Keeps is not a confirmed Dark approach and does not add a non-overlapping inactive-WLD approach to the curated total. The source has no negative targets, range mismatch, fall field, or current field. Static traversal ignores door state and runtime conditions.

## Prototype, reset, and quest identity

All three Dark mobile VNUMs that overlap active [`newbie.mob`](../../../areas/mob/newbie.mob)—#29201, #29230, and #29240—have different complete records from their active counterparts. Dark #29201 is described as a drugdealer; active Newbie #29201 is Burbul the halfling. Dark #29230 is a pillar of gore; active Newbie #29230 is a young mother centaur. Dark #29240 is an evil presence; active Newbie #29240 is a paladin teacher. Dark #29200 has no active Newbie mobile with that VNUM.

Seven Dark object VNUMs, #29201–#29207, also collide with different active [`newbie.obj`](../../../areas/obj/newbie.obj) records. The Dark records are glowing portals to different realms; the active records include a map, instruments, wine, a decanter, a tome, and robes. Dark object #29200 has no active Newbie counterpart. Its #29207 underdark portal is defined but not placed by this ZON.

The ZON has 21 M resets, 18 O resets, and 15 G resets. Eleven O resets import Player Castles objects #11051 (magic mouth), #11101 (board), and #11013 (bonding altar) repeatedly into the five map sections; seven O resets place Dark portal objects #29200–#29206. The 15 G resets equip the #29201 drugdealer with shop stock. Under current active numeric ownership, six of those seven placed object VNUMs and three of the mobile VNUMs map to Newbie prototypes, not the Dark descriptions. The active [`newbie.zon`](../../../areas/zon/newbie.zon) contains 39 typed reset-slot references to the overlapping room, mobile, and object numbers; these belong to Newbie’s existing active records.

There is no `dark_gh.qst`. Active [`newbie.qst`](../../../areas/qst/newbie.qst) contains records #29201 and #29240; its #29201 record references item #29201, and another reward references item #29206. These IDs overlap Dark mobile/object definitions but currently belong to Newbie’s active records. No tracked QST or story evidence ties them to the unregistered Dark file.

## Conditional SHP fallback and exact numeric consumers

`dark_gh.shp` has one record keyed by #29201. Its keeper is #29201, its room is #29204, and its 15 products are #377, #3050, #398, #6089, #364, #412, #53, #347, #108, #109, #111, and #491–#494. In the current active object table, fourteen products resolve: #377, #398, #364, #412, #347, #108, #109, #111, and #491–#494 belong to Heavens; #3050 belongs to Verz1; and #53 resolves to Heavens’ hemp rope despite same-number inactive objects in Dalvik, Hostel, and Mavulsk. Heavens also owns #108 and #109 actively, while Mavulsk has inactive same-number definitions. No tracked OBJ file defines #6089.

There is no `areas/world.shp` in this checkout. The non-mini [`open_shop_stream`](../../../src/economy/shop.c#L3267) falls back to globbing `areas/shp/*.shp` at line 3278, so it can encounter `dark_gh.shp` despite the missing AREA entry. Active [`newbie.shp`](../../../areas/shp/newbie.shp#L1) also has a #29201 record. Both shop sources name keeper #29201, while Dark’s shop room #29204 means “Dope Deposit” in its WLD and “A Turn in the Walkway” in active Newbie. Current code maps a keeper through [`real_mobile`](../../../src/economy/shop.c#L3493), a shop room through [`real_room`](../../../src/economy/shop.c#L3092), and products through [`real_object`](../../../src/economy/shop.c#L3380). The wildcard and same-key records are a conditional source-code risk; this audit does not infer which record would win or how either shop would operate.

Both SHP files also carry the response “I do not sell to your people!” (`dark_gh.shp` line 63; `newbie.shp` line 38). Preserve this shared player-facing wording for owner/editorial review rather than silently changing either shop during migration work.

## Title, history, and retention

After color-token, tilde, and whitespace normalization, “the Dark Guildhall” is unique among the 442 tracked ZON titles. `dark.zon` has the similar but distinct title “guildhall of the dark” and a separate #29400–#29409 room band. Similar wording does not establish aliasing, merger, or replacement.

Followed histories for the five `dark_gh` paths reach the 2009 area-tree move. Later same-stem edits include WLD/MOB/OBJ maintenance through 2013–2014, the 2015 ZON item-flag pass, the September 2026 ZON/SHP update, and a 2025 WLD encoding change. No alternate-stem rename, predecessor, or explicit whole-zone replacement is visible in those tracked paths. This bounded history does not establish earlier upstream lineage.

Keep candidate row 31 **RETAIN IN PLACE**. Resolve the 21 active Newbie room collisions, mobile/object prototype collisions, 50 Newbie numeric arrivals, all four one-way boundaries, Player Castles imports, reset ownership, active Newbie QST consumers, the conditional SHP fallback and duplicate #29201 key, the missing #6089 object, and content/tool/deployment/persisted-data ownership before any separate move or activation proposal. See the [32 builder questions](DARK_GH_BUILDER_QUESTIONS.md), [coverage row 382](ZONE_COVERAGE_REGISTER.csv#L382), [candidate row 31](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L31), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-468-the-dark-guildhall), [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-468-dark-guildhall-map-identity-and-history), and [migration plan](ATTIC_MIGRATION_PLAN.md#checkpoint-468-the-dark-guildhall-retained-pending-identity-decisions).
