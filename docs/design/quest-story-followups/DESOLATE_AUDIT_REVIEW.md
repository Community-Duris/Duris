# Desolate typed-dependency audit

Checkpoint 786 records the bounded static typed-reference review for active
Desolate. The current area and native gameplay remain unchanged.

## Current source inventory and reset sources

Desolate is an active full-area contributor with 168 rooms (#22200–#22367),
114 mobiles (#22200–#22313), 92 objects (#22200–#22291), and eleven QST
givers. Its QST file has eleven Q contracts and one addressed M block. Its
400-command ZON has 195 M, 61 E, 60 D, 45 O, 24 G, nine F, four P, and two R
commands. The 329 M/G/E/O/P prototype-reset commands use 333 typed prototype
fields when the four P container objects are included.

Most local reset prototypes resolve within Desolate. The active imported reset
rows are:

| Owner | Imported object prototypes | Desolate ZON source |
| --- | --- | --- |
| Verz1 | #3097 and #3050 | [O row 292 and G row 494](../../../areas/zon/desolate.zon#L292) |
| Divine Home | #40768 | [G row 452](../../../areas/zon/desolate.zon#L452) |
| Heavens | #364, #384, #386, and #347 twice each; #431 and #429 | [G rows 489–492, 499–502, 570–571](../../../areas/zon/desolate.zon#L489) |
| Myrabolus | #82502, #82513, #82514, and #82501 | [G rows 504–507](../../../areas/zon/desolate.zon#L504) |

Three other G reset references, #6070, #6109, and #6110, have no current
object prototype in the source tree. The source dossier already records the
trinket-vendor stock decision; preserve these unresolved references without
inventing replacements or removing either the shop or reset rows. The reset
prototype census contains 17 imports of active external objects and these
three unresolved object references. Reset files do not establish actual
admission or availability.

## Shop census and typed products

The local SHP contains three records, with shop keys #22290, #22292, and
#22311 ([source](../../../areas/shp/desolate.shp#L1)). The coverage register's
prior zero-shop census was incorrect; it now records three shops, #22290–#22311.

| Shop | Products | Resolution |
| --- | --- | --- |
| #22290 | #3050, #6070, #6109, #6110, #22256, #364, #384, #386, #347 | Verz1 #3050; local #22256; Heavens #364/#384/#386/#347; #6070/#6109/#6110 unresolved |
| #22292 | #82502, #82513, #82514, #82501 | Myrabolus prototypes |
| #22311 | #22285, #22287, #431, #429 | local #22285/#22287; Heavens #431/#429 |

Of 17 product rows, eleven resolve to active external objects, three resolve
locally, and three remain unresolved (#6070/#6109/#6110). No outside SHP
product row references a Desolate object.

## QST, story sidecar, and compiled references

The local QST recipes use local input and reward objects except the Seraphim's
original letter #82543, which is owned by Myrabolus. The corresponding active
Myrabolus QST accepts Desolate objects #22279 and #22280 as rewards at
[lines 112–113](../../../areas/qst/mira.qst#L112); the two Mira story
completion keys repeat both objects at [lines 676 and 709](../../../areas/story/mira.story.json#L676).
No other active or candidate QST item/giver consumer targets a Desolate
prototype.

The local story sidecar contains 65 typed VNUM fields and 53 item references
inside completion keys. It uses Myrabolus mobile/giver #82569 and letter item
#82543 as external typed references; the completion keys include four
references to #82543. The other typed values resolve to Desolate prototypes.
No other active or candidate story sidecar consumer targets a Desolate
prototype.

The only active outside prototype reset found is Vehicles' G row for Desolate
Master armor #22237 at [line 834](../../../areas/zon/vehicles.zon#L834).
The bounded C/C++ scan finds #22237 in the Master-set assignment and its
legacy/adapter membership tables, plus local mandolin #22269 in the
smith_data table at [tradeskill.c line 204](../../../src/economy/tradeskill.c#L204).
The number 22222 appears once inside an encoded text string and is not a
prototype field. No external mobile/object definition duplicates a Desolate
prototype VNUM.

## Item travel and disposition

Desolate has six local type-25 ITEM_TELEPORT objects, all targeting local
rooms: #22204 → #22309, #22222 → #22273, #22233 → #22326, #22271 → #22364,
#22272 → #22365, and #22273 → #22366. Its eleven type-29 ITEM_SWITCH objects
also target local rooms through value[1]. Scans of 866 external type-25 and
369 external type-29 declarations found no destination or value[1] field into
Desolate.

Keep active Desolate **DO NOT MOVE**. Preserve its Surface approaches, typed
Myrabolus/Heavens/Verz1/Divine Home imports and Mira/Vehicles consumers, the
existing builder question for missing #6070/#6109/#6110, and the three-shop
source inventory. No candidate dependency or WLD exit total changes. The
existing 21 owner questions remain unchanged. Static declarations do not
qualify stock, reset admission, safe passage, the invasion phase, or runtime
behavior.
