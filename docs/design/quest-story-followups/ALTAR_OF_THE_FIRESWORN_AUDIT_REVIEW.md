# The Altar of the Firesworn: repository-wide connectivity and dependency audit

**Checkpoint 92; priority 132 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive dossier](../zone-stories/THE_ALTAR_OF_THE_FIRESWORN.md)
with active WLD topology, all inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Static exits
and code references do not establish ordinary availability, safe travel, or
played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Altar of the Firesworn has
71 rooms and 259 authored exits; all 259 resolve. Its internal WLD graph has
components of 44 and 27 rooms. The 44-room component contains #135101–#135143
and #135171; the separate 27-room component is #135144–#135170. There are no
active WLD inbound or outbound boundaries, negative or unresolved exits,
room-number range mismatches, fall-chance fields, or current fields.

The existing dossier maps local object portal #135101 from Maelshem at room
#135131 to dream room #135157, which crosses between those WLD components;
portal object #135131 from the projection at room #135154 targets room #135171
in the 44-room component ([Maelshem portal #135101](../../../areas/obj/firesworn_altar.obj#L1),
[projection portal #135131](../../../areas/obj/firesworn_altar.obj#L333)).
These are locally declared object portals, not WLD edges or external entrances.
Actual admission, item/actor custody, `ENTER`, and safe return remain
unqualified. No external fixed/random-zone portal declaration among the
scanned type-25 objects targets a Firesworn room.

## Inactive WLD sources

The all-95 inactive-WLD scan parsed 2,080,773 exits without issues. It found
284 `Duris3.wld` exit records whose targets are Firesworn rooms. The inactive
file also contains 71 same-numbered room records, #135101–#135171; those
overlapping source VNUMs account for 140 of the target matches. The other 144
records originate in non-overlapping map rooms and cover all 71 Firesworn
rooms:

| Inactive map-source records | Destinations | Evidence |
| --- | --- | --- |
| 71 south (`D2`) exits from #134801–#134871 | One per room, #135101–#135171 | [`#134801`](../../../areas/wld/Duris3.wld#L730834) to [`#134871`](../../../areas/wld/Duris3.wld#L732304) |
| 71 north (`D0`) exits from #135401–#135471 | One per room, #135101–#135171 | [`#135401`](../../../areas/wld/Duris3.wld#L743426) to [`#135471`](../../../areas/wld/Duris3.wld#L744896) |
| Two endpoint exits: #135100 east (`D1`) and #135172 west (`D3`) | #135101 and #135171 | [`east`](../../../areas/wld/Duris3.wld#L737109), [`west`](../../../areas/wld/Duris3.wld#L738629) |

`Duris3.wld` is not in the active [`areas/AREA` list](../../../areas/AREA),
so these map declarations are source-only and do not add active ingress. The
144 non-overlapping records raise the curated source-only reference total to
1,237; the reviewed Surf subset remains 122. This excludes the 140 additional
numeric matches that originate on same-numbered `Duris3.wld` room VNUMs.

## Typed references and external producers

Typed scans found no external QST item command or SHP product matching a local
Firesworn object. One active foreign ZON record resets local object #135129 in
the Heavens bookcase: object #40 in room #1204 ([reset](../../../areas/zon/heavens.zon#L201),
[local object](../../../areas/obj/firesworn_altar.obj#L272), [room](../../../areas/wld/heavens.wld#L917)).
It is a book supply, not an accepted quest material or an external entrance.
No external mobile or object prototype definition matched a local Firesworn
VNUM.

Across current active and source-only object files there are 872 type-25
declarations (713 active and 159 source-only), including the two local
Firesworn portals; the other 870 declarations are external. The 380 type-29
declarations (362 active and 18 source-only) are also external to Firesworn.
None of these external type-25 or type-29 destinations targets a Firesworn
room. All 231 external story sidecars parsed without errors and none contains
a structured Firesworn room, mobile, or object VNUM reference.

## Compiled numeric references

The namespace-aware direct-number scan found no exact Firesworn room, mobile,
or object VNUM in the scanned C/C++ sources. The existing dossier separately
reviews the generic packed combat and item-action paths, local object portals,
and the absence of literal local callbacks; generic handlers do not create a
static numeric binding or prove a played effect. No compiled exact continuation
for the hilt or blade fragment was found in the reviewed source.

## Title, history, and disposition

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“The Altar of the Firesworn” owner: `firesworn_altar`. Both current indexes
list `firesworn_altar *1351` ([full-area entry](../../../areas/AREA#L387),
[QST entry](../../../areas/qst/AREA#L338)). Fire-themed neighbors have
distinct titles and source owners: `plane_fire_one` is “Plane of Fire”,
`brass` is “Plane of Fire, Brass”, `desolateinv` is “Desolate Under Fire”,
and `temple` is “Temple of Flames” ([Plane of Fire](../../../areas/zon/plane_fire_one.zon#L2),
[Brass](../../../areas/zon/brass.zon#L2), [Under Fire](../../../areas/zon/desolateinv.zon#L2),
[Temple](../../../areas/zon/temple.zon#L2)). These title similarities do not
identify an alternate owner or replacement for the Firesworn altar.

Followed WLD/ZON history begins with the current `firesworn_altar` paths added
in commit `d2c6cfda5` on 2010-10-05; the commit message says “Renumbers and
uploads.” Later tracked changes remain on those paths. No alternate-stem
predecessor, retirement, or whole-zone replacement was found in the reviewed
repository history. The story sidecar was added on 2026-10-06. Keep the current
un-commented [areas/AREA row](../../../areas/AREA#L387) **DO NOT MOVE**. The
same-numbered inactive map records are documented above and remain separate
source-only history; they do not establish a replacement area.

The existing dossier maps the exact essence exchange, access/key differences,
local dream portals, finite reset supply, generic combat behavior, and selected
outside source. This repository-wide scan adds the active graph, the inactive
map-source bands, the Heavens book supply, and the negative external numeric
reference findings. Dynamic access, reset admission, combat/effect outcomes,
builder intent, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 799: current typed census refresh

The local reset file has 95 typed operands: 45 M, 2 F, 10 O, 12 G, 24 E,
and two object operands in P slots. All 95 resolve to the 21 local mobiles or
33 local objects; there are no foreign imports or unresolved operands. One
active outside reset consumes Firesworn object #135129: Heavens places it in
bookcase #40 at room #1204 with a P reset. No source-only reset consumer was
found.
[Heavens reset](../../../areas/zon/heavens.zon#L201),
[book](../../../areas/obj/firesworn_altar.obj#L272),
[Heavens bookcase](../../../areas/obj/heavens.obj#L459),
[room](../../../areas/wld/heavens.wld#L917).

The two local QST givers have seven typed item fields: G inputs #135106,
#135107, #135108, and #135109, and R outputs #135110, #135119, and #135120.
All resolve locally. The local story sidecar's typed mobile and item roles
also resolve locally, and no external QST item field, QST giver header, SHP
product, or outside story-sidecar VNUM targets the altar. No local shop,
same-type external mobile/object duplicate, or exact room/mobile/object VNUM
in compiled C/C++ source was found.
[QST fields](../../../areas/qst/firesworn_altar.qst#L95),
[story contacts](../../../areas/story/firesworn_altar.story.json#L18),
[story receipt](../../../areas/story/firesworn_altar.story.json#L96).

The two local type-25 items target local rooms: object #135101 targets
#135157, and object #135131 targets #135171. Both are authored item portals
between local WLD components. There is no local type-29 item, and no external
type-25 or type-29 destination targets a Firesworn room across the current
active/source-only declarations. These item portals do not add WLD exits or
change the 259-exit graph.
[First portal destination](../../../areas/obj/firesworn_altar.obj#L7),
[second portal destination](../../../areas/obj/firesworn_altar.obj#L339),
[item type constants](../../../src/core/defines.h#L140).

No area data, typed owner, WLD count, candidate disposition, or builder
question changed. Runtime access, portal use, editor/tooling consumers, and
persisted-player identity remain unqualified.
