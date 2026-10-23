# City of Brass typed-dependency audit

Checkpoint 776, reviewed October 12, 2026. This static source review closes
the active Brass typed-reference inventory and reconciles the previously
missing SHP census. It records declared dependencies; it does not establish
live resets, shop operation, quest loading, item availability, or player
access. No native gameplay or area source was changed.

## Active source inventory and qualification

Active Brass is the full-area contributor at [areas/AREA line 397](../../../areas/AREA#L397).
Its current source contains 357 WLD rooms, 147 mobiles, 170 objects, 779 ZON
commands, seven Q/QA exchange blocks in brass.qst, and 18 new-format SHP shop
records (#139041–#139127). The [story dossier](../zone-stories/PLANE_OF_FIRE_BRASS.md)
owns the narrative and contract interpretation.

The seven brass.qst blocks are not listed in [areas/qst/AREA](../../../areas/qst/AREA).
They remain source-defined exchanges; this review does not claim that the
index loads them. Likewise, this checkout has no areas/world.shp. In non-mini
mode [open_shop_stream](../../../src/economy/shop.c#L3267) can fall back to
individual SHP files, but a reader path does not prove successful shop boot
or operation.

## Outbound active reset references

The Brass ZON has 34 external M, O, G, and P reset commands outside its local
MOB/OBJ catalogs, covering 26 distinct VNUMs owned by 14 active contributors.
The exact active reset rows are:

| Current prototype owner | Brass reset references |
| --- | --- |
| Guildhalls | Object #48012, O at [line 458](../../../areas/zon/brass.zon#L458). |
| Heavens | Object #60, O at [line 463](../../../areas/zon/brass.zon#L463); objects #97, #96, #77, #73, and #54, G at [lines 555–559](../../../areas/zon/brass.zon#L555); object #1240, G at [line 818](../../../areas/zon/brass.zon#L818); objects #430 and #360, G at [lines 1055–1056](../../../areas/zon/brass.zon#L1055). |
| Unique | Object #67202, O at [line 464](../../../areas/zon/brass.zon#L464); object #67244, P at [line 467](../../../areas/zon/brass.zon#L467); object #67245, G at [line 1031](../../../areas/zon/brass.zon#L1031). |
| Plane of Fire | Mobiles #25400 and #25401, M at [lines 472–473](../../../areas/zon/brass.zon#L472). Their procedures are assigned in [specs.assign.c](../../../src/specs/specs.assign.c#L1053) and [line 1254](../../../src/specs/specs.assign.c#L1254). |
| Undermountain | Object #92046, G at [line 560](../../../areas/zon/brass.zon#L560). |
| Githzerai Stronghold | Object #44504, G at [line 561](../../../areas/zon/brass.zon#L561). |
| Plane of Earth | Mobile #43540, M at [lines 765, 768–769, 771, and 951](../../../areas/zon/brass.zon#L765). |
| Minotaur Pass | Mobile #94740, M at [lines 766–767, 770, and 948, 950](../../../areas/zon/brass.zon#L766). |
| Tharnadia | Object #132509, G at [line 819](../../../areas/zon/brass.zon#L819). |
| Winterhaven | Object #55150, G at [line 820](../../../areas/zon/brass.zon#L820). |
| Faerie Realm | Object #14030, G at [line 821](../../../areas/zon/brass.zon#L821). |
| Sea | Object #67359, G at [line 822](../../../areas/zon/brass.zon#L822). |
| Ashrumite | Objects #66048 and #66045, G at [lines 885–886](../../../areas/zon/brass.zon#L885). |
| Ravenloft | Object #58331, G at [line 887](../../../areas/zon/brass.zon#L887). |

All listed VNUMs resolve by type to an active prototype owner. Heavens is the
current manifest owner for #54, #60, #73, #77, #96, and #97; unlisted
dalvik.obj, hostel.obj, and mavulsk.obj also define some of those numbers.
Those inactive same-number files do not create a second active owner. These
reset rows preserve item and mobile ownership; they do not guarantee reset
admission or player acquisition.

Every G/R I item command in the seven brass.qst blocks resolves to a local
Brass object; no native exchange input or reward imports a foreign object.
The Brass story sidecar also types Fire mobiles #25400 and #25401 as contacts
at [line 122](../../../areas/story/brass.story.json#L122). Its item_vnums
resolve to local Brass objects. These fields corroborate the reset and Fire
QST relationship; they do not prove that the contracts or contacts load.

## Outbound active shop products

The corrected SHP scan finds 12 externally owned products in two Brass shops.
The other 16 shop records list local Brass products only.

| Keeper and room | External product references |
| --- | --- |
| Fingers #139077, room #139030 | Undermountain object #92046 at [brass.shp line 73](../../../areas/shp/brass.shp#L73); Githzerai object #44504 at line 74; Heavens objects #54, #73, #77, #96, and #97 at lines 75–79. |
| Tila Banto #139126, room #139182 | Heavens object #1240 at line 581; Tharnadia #132509 at line 582; Winterhaven #55150 at line 583; Faerie Realm #14030 at line 584; Sea #67359 at line 585. Line 586 also lists local Brass object #139102. |

Every external SHP product above also has an active Brass ZON import listed
in the preceding table. Treat shop products as configured source stock,
separate from reset-carried objects. No outside active or retained-candidate
SHP product record was found to target a Brass local object.

## Inbound consumers and QST relationship

The one external active QST coordinate reference found is
[plane_fire_one.qst line 3](../../../areas/qst/plane_fire_one.qst#L3):
mobile #25401's block declares qc_unblock room #139000 north. Room #139000
is Brass's entrance gate. This pairs with Brass's active reset of Fire-owned
mobile #25401 and its assigned unblock_on_death procedure; it does not prove
that the hook is loaded or that the gate opens in a running world.

The static scans found no outside active ZON reset into a Brass mobile or
object, no outside active QST G/R item command or giver for a Brass prototype,
no outside active SHP product for a Brass object, and no outside active story
sidecar item_vnums or giver/mobile VNUM targeting Brass. Brass owns no local ITEM_TELEPORT or ITEM_SWITCH object, and no active or
retained-candidate ITEM_TELEPORT destination or ITEM_SWITCH value[1] room
field targets a Brass room.

The retained-candidate scans found no other cross-area ZON, QST, SHP, or
story-sidecar reference into a Brass prototype. brass-old-1 has same-number
reset commands for prototypes in its own archived MOB/OBJ catalog; those are
candidate self-resets, not active inbound uses of the Brass family.

The QST indexes need owner reconciliation before runtime conclusions:
brass.qst is absent from areas/qst/AREA, and [areas/qst/AREA lines 118–120](../../../areas/qst/AREA#L118)
still list plane_fire_one *250-251, plane_fire_two *252-253, and
plane_fire_three *254-255, while the active full-area manifest assigns
*254-255 to [plane_fire_one](../../../areas/AREA#L127). Preserve the exact
source hooks and contracts while this index identity is unresolved.

## Same-number source identities

The unregistered Duris3.wld defines all 357 Brass room VNUMs #139000–#139357.
None of the paired room titles match and none of the complete WLD blocks is
identical. For example, active Brass #139000 is “The Mighty Gates Of The City
Of Brass,” while [Duris3 #139000](../../../areas/wld/Duris3.wld#L819001) is
“The Ocean.” Duris3 is not in areas/AREA. Preserve that map under candidate
row 36; the numeric overlap is not an exit, active owner, or replacement.

The archived brass-old-1 source shares 257 room VNUMs, 47 mobile VNUMs, and
70 object VNUMs with active Brass. It is the already documented +100
predecessor crosswalk, not a second active prototype family. Keep
brass-old-1 **RETAIN IN PLACE** on candidate row 17 and preserve the distinct
active Brass source; see the [Old 1 audit](BRASS_OLD_1_AUDIT_REVIEW.md) and
[identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-454-brass-old-1-is-the-archived-100-brass-predecessor).

## Compiled references and disposition

The exact C/C++ VNUM scan found these local Brass bindings:

| Source | Typed reference | Meaning |
| --- | --- | --- |
| [specs.guards.c line 229](../../../src/specs/specs.guards.c#L229) | Room #139000 | Gate-room case in the compiled guard logic. |
| [specs.assign.c line 2313](../../../src/specs/specs.assign.c#L2313) | Room #139078 | Inn callback binding. |
| [specs.assign.c line 1963](../../../src/specs/specs.assign.c#L1963) and [shaman.c lines 88–89](../../../src/classes/shaman.c#L88) | Object #139004 | Holy mace special assignment and the matching Shaman weapon check. |
| [specs.assign.c line 1392](../../../src/specs/specs.assign.c#L1392) | Object #139138 | artifact_hide object-procedure assignment. |

Keep active Brass **DO NOT MOVE**. Keep brass-old-1 candidate row 17 and
Duris3 candidate row 36 under their existing **RETAIN IN PLACE** plans.
Resolve the missing Brass QST index entry, the stale Plane of Fire QST
identity, and intended external shop stock with their owners before making
any loader or content change. This review authorizes no gameplay, area,
candidate, or migration edit.
