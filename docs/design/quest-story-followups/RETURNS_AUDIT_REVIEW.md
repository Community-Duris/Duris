# returns isolated mobile-source audit

Canonical key: component:returns. Physical coverage row 438 / CSV line 439; candidate dependency data row 87 / CSV line 88. Reviewed October 11, 2026, Checkpoint 525, against source snapshot c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static mobile-source, text, history, membership, and reference review. It does not establish that either prototype is loaded, placed, or encountered.

## Source scope and text

The only current file is [areas/mob/returns.mob](../../../areas/mob/returns.mob#L1), with two mobile records:

| MOB VNUM | Keywords and short description | Long description |
| --- | --- | --- |
| #93400 | silent golem; “A Silent Golem” | It stands silently, watching those who enter. Its text says it was created to stand sentinel at the entry of “this tower.” |
| #93401 | diminutive cleric; “A diminutive cleric” | The cleric gazes out a window in distraction. |

No same-stem WLD, OBJ, ZON, QST, SHP, or story sidecar identifies the tower, a placement, a quest, or a relationship between these two mobiles. The filename returns is the only current group label; there is no ZON title to provide a zone alias or canonical title.

The source stem is absent from areas/AREA, areas/qst/AREA, and areas/RANDOM_AREA. The expected mobile aggregate is areas/world.mob, named in [db.h](../../../src/world/db.h#L23), but that aggregate is absent from this checkout. The per-file source does not establish full-world membership or runtime loading.

## Typed closure and identity

An exact standalone-VNUM scan of the current areas source tree found #93400 and #93401 only in returns.mob. No current outside MOB definition reuses either VNUM; no same-number WLD or OBJ record was found. No external ZON reset, QST item/mobile field, SHP keeper, story sidecar, or C/C++ source contains either exact VNUM. The matching keywords “silent golem” and “diminutive cleric” also appear only in this MOB file. The mini-area source scan found no copy. This closes the repository-bounded direct numeric/name scan, not editor exports, generated inputs, persisted state, or dynamic scripts outside the checkout.

There is no WLD file and no ZON file, so neither mobile has a source-defined room placement or reset. No active WLD destination can point into these candidate identities because the component contains no rooms, and the exact source scan found no direct outside numeric reference. This is a dependency finding, not a conclusion that the characters cannot be created through deployment-specific or dynamic inputs.

## History, disposition, and limits

The followed MOB path contains only the area-tree move commit in this checkout. No alternate-stem predecessor, explicit successor, owner annotation, or retirement decision was found. The descriptions provide two small scene cues, but not enough evidence to name the tower, place the characters, or join them to a known area story.

Keep areas/mob/returns.mob RETAIN IN PLACE. Before any activation, move, rename, merge, or archive proposal, identify the tower and mobile owners, intended placements and spawn path, aggregate-generation inputs, editor/export history, and saved or scripted VNUM references. See the [20 builder questions](RETURNS_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-525-returns-two-mobile-prototypes-without-a-map), [identity/history note](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-525-returns-isolated-mobile-source-identity), and [retention note](ATTIC_MIGRATION_PLAN.md#checkpoint-525-returns-retain-mobile-source-pending-tower-and-placement-owners).

## Checkpoint 929: both mobile VNUMs remain isolated in current sources

A fresh exact-value search across `areas/` and `src/` finds #93400 and #93401 only in [returns.mob](../../../areas/mob/returns.mob#L1). Neither number appears in another WLD, MOB, or OBJ prototype; any ZON M reset; QST giver, item, or mobile field; SHP keeper, room, or stock field; ITEM_TELEPORT or ITEM_SWITCH destination; story/map sidecar; mini-area source; or compiled C/C++ literal. The two records therefore have no current checked-in cross-type collision or direct numeric consumer, and the existing exact-name and history findings do not change.

The component remains MOB-only, with no rooms, exits, placement, or WLD inbound count to update. This source scan still cannot qualify dynamic spawning, generated `world.mob`, editor/export inputs, saved state, or use outside the checkout. Keep both records RETAIN IN PLACE and leave the 20 builder questions unchanged; RT-13 now calls out the specific typed consumer classes reviewed here.
