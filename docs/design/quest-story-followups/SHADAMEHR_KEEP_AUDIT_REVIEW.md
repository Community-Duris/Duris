# Shadamehr Keep: connectivity, dependencies, and builder review

Checkpoint 354 reconciles the source story in the [Shadamehr Keep dossier](../zone-stories/SHADAMEHR_KEEP.md) with active boundaries, the inactive WLD set, typed references, compiled object bindings, and followed file history. The evidence is static and repository-based.

## Active graph and door resets

The active WLD has 25 rooms and 57/57 loader-resolved exits. All 25 rooms form one weak component and one strongly connected directed component when every authored edge is treated as traversable. There are no negative or unresolved targets, room-range mismatches, fall fields, or current fields. From entrance #41200, raw authored reachability and returnability are 25/25. Suppressing all flagged door directions leaves 19/19 from the entrance. This is a conservative graph comparison, not live reset, key, combat, safety, or travel evidence.

| Boundary | Counterpart | Classification |
| --- | --- | --- |
| Dreggan Woods #41186 north -> Shadamehr Keep #41200 | Shadamehr #41200 south -> Dreggan Woods #41186 | Reciprocal ([inbound register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L557), [outbound register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L558)) |

Six flagged exit directions form the three documented door pairs: #41207 north <-> #41218 south (metal key #41201), #41219 north <-> #41222 south (ornate key #41202), and #41222 north <-> #41223 south (imported rusted key #41003). Each direction has an exact D reset at 100% chance, reset-state value 2. The local source comments call all three doors closed/locked. Refer to the [WLD](../../../areas/wld/shadamehr_keep.wld) and [ZON](../../../areas/zon/shadamehr_keep.zon#L8) for authored values.

The current inactive set contains 95 WLD files. An exact scan for Shadamehr room headers #41200-#41224 and exit data targeting those VNUMs found no source-room overlap and no source-only approach. The current curated source-only total remains 4,504 (Surf 267, Underworld2 9); no candidate register row is changed or added. These searches cover static WLD records only.

## Typed dependencies

Local Shadamehr ZON reset commands import or reference:

| Reference | Source evidence | Classification |
| --- | --- | --- |
| Dreggan Woods object #41003 | Given to Merkus by Shadamehr ZON; defined as an old rusted key in [Dreggan Woods OBJ](../../../areas/obj/dreggan_wood.obj#L1) | Active external object dependency and vault key |
| Heavens object #359 | Reset into Shadamehr vault; defined as a rune-covered stone in [Heavens OBJ](../../../areas/obj/heavens.obj#L3725) | Active external object dependency |
| Unique object #67219 | Reset into the vault; defined as the dark heart of Dreggan Wood in [Unique OBJ](../../../areas/obj/unique.obj#L298) | Active external object dependency |
| Object #41002 | Shadamehr has an O reset at ZON line 34, but no active OBJ prototype with VNUM #41002 was found. Dreggan Woods defines mobile #41002 only. | Unresolved object reset target; confirm intended object or remove/fix the reset in a separate authorized source change |

External consumers and producers of local Shadamehr objects:

| Local object | External reference | Finding |
| --- | --- | --- |
| #41204, cloak of shadow dragons | [Alatorin QST](../../../areas/qst/alatorin.qst#L5775) | A recipe includes R I #41204 as a removed input and its QA describes making a shadowy cloak. |
| #41203, Bloodlust | [Barovia ZON](../../../areas/zon/barovia.zon#L336) | A G reset attaches this local object to Doru in Barovia; this is not a Shadamehr placement. |

No other external QST item token or SHP product with local Shadamehr object VNUMs was found. No active foreign D reset targets a local Shadamehr room, and no external active MOB/OBJ prototype definition duplicates a local same-type VNUM. The 231 external story sidecars were scanned for structured numeric references to this room/mobile/object range; none matched.

Compiled-source matches are limited to object bindings: [object #41204](../../../src/specs/specs.assign.c#L1407) is bound to artifact_invisible and appears in six [Chaos equipment profiles](../../../src/account/chaos_eq_data.h#L107); [object #41203](../../../src/specs/specs.assign.c#L1998) is bound to unholy_avenger_bloodlust. A #41208 dagger assignment is commented out and is not active. No local room or mobile VNUM callback/reference was found in the compiled numeric scan. These matches describe code relationships, not accepted quest progress or player acquisition.

## Title, history, and disposition

After color-code removal, terminator trimming, whitespace normalization, and case folding, Shadamehr Keep is unique among all 442 current .zon titles and among the 351 active contributors. Same-type active room, mobile, and object checks show no duplicate ownership; inactive-WLD scans show no local room-number overlap.

Followed history remains on shadamehr_keep: WLD and OBJ reach 2013-11-13; MOB reaches 2026-08-26; ZON reaches 2015-10-13 and older same-stem edits to the 2009-08-12 area-tree move. There is no local QST file or story sidecar to follow. No alternate-stem predecessor, distinct prior zone, or explicit whole-zone replacement was found in these bounded paths. Keep active shadamehr_keep DO NOT MOVE.

## Builder questions and recommended resolutions

| ID | Builder question | Source-backed answer | Recommended resolution |
| --- | --- | --- | --- |
| BSQ-SHK-01 | Is the floating fortress above the Ethereal Plane intended to have only its authored Dreggan Woods entrance? | Room #41200 describes it floating above the Ethereal Plane; the active WLD boundary is reciprocal with Dreggan Woods. | Preserve the existing entrance and lore; confirm before adding a portal, plane route, or extra connection. |
| BSQ-SHK-02 | Is the keep intentionally an environmental story with no local quest contract? | The local QST file, story sidecar, and native Q/QA contracts are absent, while qst/AREA lists the stem. | Do not invent a Merkus/dragon quest from room prose; confirm whether a separate story feature is desired. |
| BSQ-SHK-03 | Are the disorienting halls (#41210-#41217) meant to function as a maze or only as magical atmosphere? | Their repeated room descriptions mention disorientation; the authored exits form a connected loop. | Keep the current layout until the builder states whether navigation itself should be a puzzle. |
| BSQ-SHK-04 | Does alcove #41224 have a deliberate player purpose? | It has an empty description and one west exit back to #41217. | Preserve the alcove as authored; ask whether its blank text and loop endpoint are intentional. |
| BSQ-SHK-05 | Should the dark room and undead beholder be the source of the metal key? | The beholder is reset in #41209 with metal key #41201; that key opens the #41207/#41218 door pair. | Keep the exact key identity and placement; do not credit the key before actual qualified acquisition. |
| BSQ-SHK-06 | Should both sides of the metal door remain closed and locked? | The room exits are flagged and both door directions have state-2, 100%-chance D resets. | Preserve the paired resets; qualify opening, reset timing, and return before making access claims. |
| BSQ-SHK-07 | Is the shadow dragon meant to guard the ornate key to Merkus? | Shadow Dragon mobile #41202 is reset at #41219 and carries object #41202, the ornate key for the #41219/#41222 pair. | Keep object/mobile identities distinct and preserve this authored progression. |
| BSQ-SHK-08 | Is the Shadow Dragon's lair intended to connect directly to Merkus's abode? | #41219 north and #41222 south are the reciprocal ornate-key door pair. | Preserve the key-gated connection; do not infer a dragon kill receipt from the key reset. |
| BSQ-SHK-09 | Is Merkus still intended as a fallen paladin? | His mobile description says he was a fallen paladin struck down by the gods, and the room retains blackened noble armor imagery. | Use this source as the current lore; confirm any expanded redemption or divine-history story with the builder. |
| BSQ-SHK-10 | Should the old rusted Dreggan Woods key open the treasure vault? | Imported object #41003 is equipped on Merkus and used by the #41222/#41223 locked door pair. | Preserve the cross-zone key dependency; qualify source, custody, and return before any migration proposal. |
| BSQ-SHK-11 | What is the intended object for the O reset of #41002? | The Shadamehr ZON resets object #41002, but no active object prototype is found; Dreggan Woods has mobile #41002 only. | Ask the builder to identify a valid object or confirm the reset is obsolete; do not silently substitute a mobile or other item. |
| BSQ-SHK-12 | Should Heavens stone #359 and Dreggan heart #67219 appear in the Shadamehr vault? | The local ZON places both external prototypes in #41223. | Keep their owning prototype sources explicit and confirm the cross-zone vault stock is intentional. |
| BSQ-SHK-13 | Should Bloodlust stay reset on Doru in Barovia? | Barovia attaches local object #41203 to Doru; it is not reset on Merkus in Shadamehr. | Preserve Barovia's producer placement unless the owner approves a separate item-placement change. |
| BSQ-SHK-14 | Is the shadow dragon cloak intended to be spent in Alatorin's recipe? | Alatorin QST has R I #41204 and describes producing a shadowy cloak from dragonscale work. | Preserve the exact input identity and external quest dependency; do not treat a held cloak as local quest completion. |
| BSQ-SHK-15 | Should the cloak retain its artifact-invisibility binding and Chaos-kit use? | Compiled assignment binds #41204 to artifact_invisible; six Chaos profiles reference the VNUM. | Keep these code and equipment dependencies visible in any separate balance or migration proposal. |
| BSQ-SHK-16 | Is the spectral sword meant to be carried by Merkus despite its immovable-room prose? | Object #41210 says it refuses to budge, while the Shadamehr ZON equips it on Merkus. | Confirm intended presentation and custody; do not remove the equipment reset by inference. |
| BSQ-SHK-17 | Are the dark-heart and rune-stone vault imports meant to be reset stock rather than quest rewards? | They are O resets in the vault and have no local QST consumer contract. | Keep environmental loot separate from quest acceptance; confirm source ownership before changing supply. |
| BSQ-SHK-18 | Should the zone's reset mode remain 0? | The ZON header records mode 0 and local door resets are authored at 100% chance. | Preserve mode 0 pending a reset/persistence qualification; do not infer stock restock behavior from the field. |
| BSQ-SHK-19 | Is the room #41218 label intended to mark entry to the unholy chambers before the dragon and Death Knight? | Its text labels the route, and the adjacent graph leads to #41219 and #41222 behind paired key doors. | Preserve the current route labels and ask before changing the order or chamber ownership. |
| BSQ-SHK-20 | Are the authored arrival, door, and return routes safe in ordinary play? | Static raw reachability is complete, but suppressing flagged doors leaves only 19 rooms from the entrance; no live access or combat check was run. | Keep runtime safety and ordinary availability unqualified until a separate gameplay journey checks them. |

This checkpoint updates design documentation and one coverage row only. No gameplay source, area source, database, migration, server, or compiled artifact was changed or exercised.
