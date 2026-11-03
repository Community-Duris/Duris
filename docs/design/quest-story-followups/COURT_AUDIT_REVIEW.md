# Court of the Muse typed-dependency audit

Checkpoint 783 records the static typed-reference review for active `court`.
This supplement preserves the current source identity and does not change
native gameplay or zone ownership.

## Current source inventory and QST index status

Court is listed in the active full-area source with 99 rooms (#6700–#6798),
36 mobiles (#6700–#6735), and 43 objects (#6700–#6742). No same-type room,
mobile, or object VNUM overlaps another active contributor or any candidate
source.

The [QST source](../../../areas/qst/court.qst) and
[story sidecar](../../../areas/story/court.story.json) exist. The native file
defines nine exchange contracts, and its 131 structured VNUM/item/completion
references resolve to Court prototypes. However, Court is absent from
[`qst/AREA`](../../../areas/qst/AREA). Record these as authored source
declarations; do not treat the file or sidecar as proof the nine contracts
are loaded or available in a running game.
No active or candidate QST item, giver, or custom-room reference targets Court.

## Reset and shop dependencies

The [ZON has 209 commands](../../../areas/zon/court.zon): 122 M, 42 G, 23 E,
nine O, eight D, and five F. The 196 mobile/object prototype reset rows
resolve locally except for two G imports: Heavens object #359 (rune-covered
stone) and White Harbor object #55448 (memory of the Court of the Muse) at
[court.zon lines 74–75](../../../areas/zon/court.zon#L74). Preserve those
active owners. No unresolved prototype or active/candidate inbound ZON reset
to a Court mobile, object, or room was found.

The [SHP has one record](../../../areas/shp/court.shp#L1), keeper #6728,
selling local object #6706 (red apple). The coverage register's previous
zero-shop count was incorrect; it now records shop #6728 and one record.
No active or candidate shop product targets a Court object.

## Object travel, structured references, and compiled consumers

Court has five type-25 ITEM_TELEPORT objects, all with local destinations:
#6700 → #6796, #6705 → #6720, #6709 → #6770, #6718 → #6706, and #6730 →
#6713. There is no local ITEM_SWITCH. A full active/candidate object scan
found no external ITEM_TELEPORT destination or ITEM_SWITCH value[1] room
field into Court.

The sidecar's 131 structured prototype references all resolve to Court; no
active or candidate sidecar references a Court prototype. A bounded scan of
current C/C++ sources found 47 object-VNUM entries for nine Court objects in
[`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L116) and
[its later entries](../../../src/account/chaos_eq_data.h#L553): #6703, #6704,
#6710, #6721, #6725, #6726, #6728, #6736, and #6738. They appear as item VNUMs
in the generated chaos-equipment tables, not as local special assignments.
No other direct VNUM literal matching a Court mobile, object, or room was
found.

## Disposition

Keep active Court **DO NOT MOVE**. Preserve the two Heavens/White Harbor
object imports, the local shop and teleport declarations, and the QST index
absence as a separate loader-ownership finding. The existing 16 owner
questions remain unchanged. No candidate dependency or WLD exit total
changes. Static source review does not qualify QST loading, reset admission,
shop stock, or travel behavior.
