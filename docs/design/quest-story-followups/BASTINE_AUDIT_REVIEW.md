# Bastine Castle audit supplement

Checkpoint 773 completes the static typed-dependency inventory for active
Bastine Castle. It reconciles the external items named by native contracts,
the single candidate-source reset and shop product into Bastine, a cross-area
story-giver reference, compiled uses of local objects, and the portal scan.
This is a source review and does not establish live loading, item availability,
or player travel.

## Scope and map connectivity

Bastine is the active full-area contributor with 129 rooms (#7600–#7728), 39
mobiles, 80 objects, 428 reset commands, and 14 native Q/QA contracts. It has
no local SHP file. Its static WLD graph has 259/259 loader-resolved exits in
one 129-room weak component, one inbound and one outbound cross-area exit
forming one reciprocal pair, and no positive unresolved or unregistered WLD
targets. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L28),
[world connectivity report](WORLD_CONNECTIVITY.md), and
[comprehensive story dossier](../zone-stories/BASTINE_CASTLE.md).

## Native contract imports and outputs

The Bastine QST names foreign objects as both commission inputs and rewards.
The matching structured sidecar lists the 12 foreign commission materials and
the optional signet #41397 as typed item VNUMs. Prototype ownership resolves
as follows:

| Current object owner | Bastine QST references | Contract role |
| --- | --- | --- |
| Highway | Inputs #41388 (line 33), #41407 (41), #41327 (58), #41375 (123), #41411 (140), and #41397 (201); outputs #41304 (139), #41394 (199), and #41398 (200) | Royal commission materials, Victor's signet, replacement hide, key, and apprentice ring. |
| Skulldrach | Input #12802 (line 50) | Blessed shield commission. |
| Forest of Mir | Inputs #41924 (line 68), #41922 (100), and #41920 (108) | Ebony staff, face of Mythra, and Xort hide commissions. Mir's audit already cross-links these exact active consumers. |
| Breale | Input #2607 (line 80) | Broom of air commission. |
| Test of Knights | Input #70970 (line 156) | Final Bastine Order commission. |

These are declared item references; they do not prove a source reset, personal
acquisition, or campaign ordering. The Bastine dossier records the local
contracts as independently accepted exchanges. See the [Bastine QST](../../../areas/qst/bastine.qst#L33),
[story sidecar](../../../areas/story/bastine.story.json#L159), and
[Mir's typed-dependency table](MIR_AUDIT_REVIEW.md#L43).

## Inbound typed consumers and candidate references

| Source | Exact reference | Classification |
| --- | --- | --- |
| Active Highway story sidecar | Line 136 declares giver mobile #7603, Victor the Apprentice, for the optional accepted-history step before Victor's ring. | Cross-area story continuity. Highway owns the story's later Morlanthra contract; this field points to the local Bastine giver rather than transferring quest ownership. |
| Unregistered Nexus56 ZON | Line 1370 G resets Bastine object #7627. | Candidate-source reset only. |
| Unregistered Nexus56 SHP | Shop #83037 lists product #7627 at line 458. | Candidate-source shop record only; no active stock is established. |

No external active QST item command, external active ZON reset, or active SHP
product targeting a Bastine object or mobile was found. No other external
story-sidecar item VNUM targets a Bastine object. Preserve Nexus56's reset and
shop record under its existing RETAIN IN PLACE plan; both references are
explicitly recorded in the [Nexus56 audit](NEXUS56_AUDIT_REVIEW.md).

The checkout has no areas/world.shp. In non-mini mode,
[open_shop_stream](../../../src/economy/shop.c#L3267) falls back to globbing
individual SHP files if it cannot open the combined file, so the candidate
SHP has a possible reader path. That does not establish successful shop
boot or operation. No server or shop runtime was started.

## Local resets, duplicate prototypes, and code references

All Bastine ZON reset-role references resolve to its local mobile/object
sources; no external prototype import was found. Exact same-type scans found
no other current mobile or object owner for Bastine's local prototype IDs.
The only inbound candidate reset/product references identified above target
local object #7627.

Compiled code contains three further exact object references:

| Source | Local Bastine prototype | Use |
| --- | --- | --- |
| [chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L135) lines 135 and 385 | #7677, Mask of Battles Past | Listed in two CHAOS equipment kit tables. |
| [chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L166) lines 166 and 415 | #7678, Greaves of Avoidance | Listed in two CHAOS equipment kit tables. |
| [player_death_restitution_locker.h](../../../src/player/player_death_restitution_locker.h#L6) and [locker implementation](../../../src/player/player_death_restitution_locker.c#L22) | #7670, generic bag | Restitution-bag identification requires this VNUM together with ITEM_TRANSIENT and the restitution marker keyword; the number alone is not enough. |

A global scan of current object declarations found no external ITEM_TELEPORT
destination or ITEM_SWITCH value[1] room field into Bastine rooms #7600–#7728.
Bastine owns no local ITEM_TELEPORT or ITEM_SWITCH object. Bastine's route remains its one reciprocal
regular WLD boundary, already recorded in the connectivity report.

## Identity and disposition

The [CP705 title/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-705-bastine-castle-title-and-tracked-history)
keeps the active source identity under Bastine. The typed references do not
indicate a duplicate or replacement area. Keep Bastine **DO NOT MOVE** as an
active areas/AREA contributor. Keep Nexus56 **RETAIN IN PLACE**, preserve its
source-only reset and shop product for #7627, and do not add a Bastine attic
candidate row or change native gameplay. The existing 16 area-owner questions
remain in the [story dossier](../zone-stories/BASTINE_CASTLE.md#direct-area-owner-questions-and-conservative-recommendations);
the newly reconciled dependency evidence does not require another owner
decision. Live access, reset/shop availability, and persisted-player state
remain outside this static review.

## Checkpoint 777: Breale QST item

Active Bastine QST line 80 has G I #2607, a Breale object. Preserve its exact typed ownership; see the [Breale typed-dependency audit](BREALE_AUDIT_REVIEW.md#inbound-active-consumers).
