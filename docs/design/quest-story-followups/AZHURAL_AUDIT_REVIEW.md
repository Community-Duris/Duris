# The Lair of Tiamat repository-wide audit supplement

This supplement reconciles active and inactive WLD records, the explicit Azhural source swap, typed dependencies, current QST source ownership, and bounded title/history evidence for the [source-comprehensive Azhural dossier](../zone-stories/AZHURAL.md). The frozen snapshot is 72ba3deff3bb30bde880018cdebe7c6db675b697. No area, quest, or gameplay source is changed.

## Active graph and local portal boundaries

The active [azhural WLD](../../../areas/wld/azhural.wld#L1) contains 89 rooms and 216/216 loader-resolved exits, all local. Its weak components contain 80, 8, and 1 rooms. Raw directed traversal from entrance #135201 reaches and has authored returns from 80/89 rooms; all 216 exits have no blocked flag 8. There are no active cross-area WLD inbound or outbound exits, falls, or currents. This raw graph excludes keyed-door traversal and item portals.

The local source-comprehensive dossier separately maps two local type-25 objects: gateway #135230 targets room #135275, and fiery portal #135231 targets #135202 ([gateway prototype](../../../areas/obj/azhural.obj#L257), [fiery portal prototype](../../../areas/obj/azhural.obj#L265)). Their successful ENTER paths are item travel, not ordinary WLD boundaries. Room #135283, the Gates of Malsperanze, remains isolated in the full local WLD graph ([room](../../../areas/wld/azhural.wld#L1668)). The dossier's key/ward/portal scenarios are conditional source models, not proof of a live destination, a safe journey, or a played encounter.

## Inactive-WLD approaches and identity overlaps

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues. It found 172 file-specific source-room VNUM overlaps with active Azhural: 89 in Duris3.wld and 83 in tiamats_lair.wld. Across those inactive sources, 556 exit records target current Azhural room VNUMs: 356 from Duris3 and 200 from tiamats_lair. The 200 legacy tiamats_lair records start from rooms whose VNUMs overlap the active area, so they are retained as source/map identity evidence rather than counted as new approaches.

The scan also found 180 non-overlapping Duris3 approaches into active Azhural rooms: 89 south exits from #134901–#134989 into #135201–#135289, 89 north exits from #135501–#135589 into the same room band, plus #135200 east → #135201 and #135290 west → #135289 ([south-band sample](../../../areas/wld/Duris3.wld#L732934), [north-band sample](../../../areas/wld/Duris3.wld#L747374), [east endpoint](../../../areas/wld/Duris3.wld#L739209), [west endpoint](../../../areas/wld/Duris3.wld#L741107)). They are source-only and do not create active ingress. The curated source-only total rises from 2,616 to 2,796; the reviewed Surf subset remains 156. Preserve Duris3 and the separate legacy map files with their own source identities.

## Typed dependencies and loaded QST ownership

| Dependency surface | Source-backed result and boundary |
| --- | --- |
| Current QST manifest | [areas/AREA](../../../areas/AREA#L388) selects azhural (#1352), while [areas/qst/AREA](../../../areas/qst/AREA#L339) selects tiamats_lair (#1352) and does not list azhural. The QST builder reads the qst/AREA list and loads each named qst file ([builder list](../../../areas/src/qst/make_qst.c#L6), [file selection](../../../areas/src/qst/make_qst.c#L52)); the runtime reads generated areas/world.qst ([runtime file](../../../src/world/quest.c#L79)). Thus the two native contracts are currently sourced through tiamats_lair.qst even though the active world area is azhural. |
| Content equivalence today | [azhural.qst](../../../areas/qst/azhural.qst#L1) and [tiamats_lair.qst](../../../areas/qst/tiamats_lair.qst#L1) are byte-for-byte identical 97-line files (SHA-256 5be007ebd8ae737d0647405d23ff58339f222986c29b8ce4541269ace2ead41b). Both declare givers #135219 and #135220. The legacy QST path is a live source-list dependency, with exact item references in [the first contract](../../../areas/qst/tiamats_lair.qst#L20) and [the second contract](../../../areas/qst/tiamats_lair.qst#L78), but has no current content divergence; a future edit to only azhural.qst would not enter the generated QST until qst/AREA is reconciled. |
| Legacy resets and prototype copies | tiamats_lair.zon is not in the active areas/AREA world list, but its source-only resets still name local room/mobile/object VNUMs ([first reset](../../../areas/zon/tiamats_lair.zon#L9), [later consort resets](../../../areas/zon/tiamats_lair.zon#L21)). The [legacy MOB](../../../areas/mob/tiamats_lair.mob#L1) and [OBJ](../../../areas/obj/tiamats_lair.obj#L1) sources also define 20 mobile and 38 object VNUMs that overlap current Azhural prototypes. These are not active ZON/MOB/OBJ placements; retain them as legacy source evidence. |
| Legacy portal declarations | The source-only tiamats_lair.obj repeats portal object VNUMs #135230 and #135231, targeting current rooms #135275 and #135202 ([gateway](../../../areas/obj/tiamats_lair.obj#L257), [fiery portal](../../../areas/obj/tiamats_lair.obj#L265)). The active objects are the azhural.obj prototypes above; the unregistered duplicate OBJ records are not a separate active external portal owner. |
| Compiled binding and scoped negatives | Active object #135218 receives the sphinx_prefect_crown binding ([object](../../../areas/obj/azhural.obj#L153), [binding](../../../src/specs/specs.assign.c#L1330)). Bounded scans find no external SHP product, no active external ZON reset into Azhural, no outside typed sidecar VNUM among 231 parseable sidecars, and no local shop. Exact outside QST hits belong to the qst/AREA-listed tiamats_lair file; no other active QST consumer was found. |

The identical QST copies preserve current native contract content, but file ownership is split between the active world manifest and the active quest manifest. Do not archive tiamats_lair files merely because the WLD/ZON version was replaced: the QST manifest still selects the legacy path.

## Title and explicit replacement history

The normalized active title scan finds one exact owner for “The Lair of Tiamat”: azhural among 351 active .zon contributors ([active title](../../../areas/zon/azhural.zon#L2)). The unregistered [tiamats_lair title](../../../areas/zon/tiamats_lair.zon#L2) matches it. Commit 50b400005d0bfc19adb51cda0d74414e940a7b51, dated 2012-08-07, explicitly says it is “Swapping zone files for the new version of Tiamat's Lair,” names Azhural.zon as the new file, updates AREA, and comments out the un-updated version. That commit adds areas/wld/azhural.wld and areas/zon/azhural.zon while the old tiamats_lair WLD/MOB/OBJ/ZON/QST sources remain in the current tree. This is explicit version-swap evidence, separate from the distinct active area named Tiamat.

Keep active azhural DO NOT MOVE. Preserve the 180 Duris3 approaches and the tiamats_lair source family. Reconcile the QST manifest and finish typed, editor/tooling, persisted-identity, and map-owner closure before any separate archive proposal; no source move or QST-list repair is shipped here.

## Builder recommendations

1. Keep the Azhural WLD, its local portals, and the isolated #135283 room as distinct authored structures; do not infer ordinary boundary travel.
2. Preserve the 180 non-overlapping Duris3 approaches and the 172 file-specific room-VNUM overlaps, distinguishing map history from active ingress.
3. Preserve tiamats_lair as an explicit former version with a live QST-manifest role; decide a separately named QST ownership update before archiving any legacy QST path.
4. Keep the active Azhural portals local to the current OBJ list; record the old duplicate portal declarations as source-only.
5. Qualify reset-generation supplies, batch item custody, doorway/key breakage, ENTER arrival, the finale and vault reward persistence in a later gameplay pass.
