# Fishermans Wharf title and history audit supplement

This supplement records all-442 normalized title comparison and followed source history alongside the [source-comprehensive Fishermans Wharf dossier](../zone-stories/FISHERMANS_WHARF.md). It changes no native area data.

## Title, history, and disposition

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Fishermans Wharf” owner: `fishermans_wharf`. Both current indexes list `fishermans_wharf *889` ([full-area entry](../../../areas/AREA#L315), [QST entry](../../../areas/qst/AREA#L281)); the title source is [fishermans_wharf.zon](../../../areas/zon/fishermans_wharf.zon#L2). Similar dock and port wording belongs to distinct current titles: “Dalvik's Dockyard” (`dalvik`), “The Docks of Quietus Quay” (`quietus`), “Storm Port Stronghold” (`spshold`), “Storm Port” (`stormport`), and “The Ruins of Port Skythic” (`PortSkythic`). These are separate source owners rather than alternate Wharf titles. See their [title lines](../../../areas/zon/dalvik.zon#L2), [Quietus title](../../../areas/zon/quietus.zon#L2), [Stronghold title](../../../areas/zon/spshold.zon#L2), [Storm Port title](../../../areas/zon/stormport.zon#L2), and [Port Skythic title](../../../areas/zon/PortSkythic.zon#L2).

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories remain on the `fishermans_wharf` stem from the 2009 area-tree move through later same-stem edits. Recent source changes in 2025–2026 update the existing Wharf files; they do not establish a rename or whole-zone replacement. The story sidecar was added on 2026-10-04. No alternate-stem predecessor, retirement, or whole-zone replacement was found in the reviewed repository history. Keep active `fishermans_wharf` **DO NOT MOVE**. The dossier's command-target alias and owner-disambiguation question remains open; no alias is inferred from generic dock/port wording. See [coverage row 97](ZONE_COVERAGE_REGISTER.csv#L98) and the [duplicate/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-729-fishermans-wharf-title-and-history).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad resets White Harbor object #55377 once. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

## Checkpoint 789: Dream skull reset and prose reference

Active `fishermans_wharf.zon` G row 138 resets Dream skull #31320
([reset](../../../areas/zon/fishermans_wharf.zon#L138),
[prototype](../../../areas/obj/dream.obj#L293)). Its story contact description includes
#31320 as prose, not as an `item_vnums` dependency
([sidecar](../../../areas/story/fishermans_wharf.story.json#L146)). Preserve Dream as the
item's prototype owner and classify the text occurrence separately from typed item
references.
See [audit](DRIFTING_REALM_AUDIT_REVIEW.md#checkpoint-789-shop-and-sidecar-closure-refresh).

## Checkpoint 800: current typed census refresh

The local reset file contains 113 typed operands: 78 M, 19 O, 10 G, and 6 E.
Of these, 112 resolve locally. The one imported object is Dream skull #31320
in a G reset; no operand is unresolved. One active outside ZON reset also
loads local fishing pole #88903 from Winterhaven. No source-only reset
consumer was found.
[Dream skull reset](../../../areas/zon/fishermans_wharf.zon#L138),
[Dream prototype](../../../areas/obj/dream.obj#L293),
[Winterhaven reset](../../../areas/zon/wh.zon#L760),
[fishing pole](../../../areas/obj/fishermans_wharf.obj#L58).

The five local QST givers contain 24 item fields, 19 G inputs and 5 R outputs;
all resolve to local objects. The only active outside QST item consumer found
is Newhaven's G input for snorkel pipe #88905. Its sidecar also maps local
frog-spawn jelly #88909 and eager fisherman #88907 as the optional producer
route for that pipe. The Wharf's own typed sidecar roles resolve locally. The
Dream skull #31320 in its Razhl contact description remains prose, not a typed
item-VNUM role. No outside QST giver header, additional typed sidecar consumer,
source-only QST consumer, or same-type duplicate mobile/object prototype was
found.
[local QST fields](../../../areas/qst/fishermans_wharf.qst#L27),
[pipe reward](../../../areas/qst/fishermans_wharf.qst#L118),
[Newhaven consumer](../../../areas/qst/newhaven.qst#L182),
[Newhaven recipe mapping](../../../areas/story/newhaven.story.json#L291),
[Wharf prose mention](../../../areas/story/fishermans_wharf.story.json#L146).

The local Wharf shopkeeper mobile #88903 has three declared products: cast net
#88906, fishing pole #88903, and paddle boat #88901. Winterhaven's active SHP
also lists pole #88903 as a product. No other outside SHP product reference to
a Wharf object was found.
[Wharf product list](../../../areas/shp/fishermans_wharf.shp#L3),
[last Wharf product](../../../areas/shp/fishermans_wharf.shp#L5),
[Winterhaven product](../../../areas/shp/wh.shp#L452).

Compiled CHAOS arrays reference local objects #88906, #88917, and #88919;
the fishing command's pole selector also recognizes local rod #88903. No exact
local room or mobile VNUM appears in compiled C/C++ source. No local type-25
or type-29 item destination exists, and no external type-25 or type-29
destination targets a Wharf room. These item and inventory references add no
WLD edges or source-only approach.
[CHAOS #88917 reference](../../../src/account/chaos_eq_data.h#L125),
[CHAOS #88919 reference](../../../src/account/chaos_eq_data.h#L158),
[CHAOS #88906 reference](../../../src/account/chaos_eq_data.h#L555),
[pole selector](../../../src/economy/tradeskill.c#L577).

No area source, WLD count, candidate disposition, or owner question changed.
Shop stock, reset admission, quest settlement, item custody, and runtime or
persisted-player behavior remain unqualified.

