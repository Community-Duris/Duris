# The Mountain Settlement of the Harpies audit supplement

**Checkpoint 54 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/THE_MOUNTAIN_SETTLEMENT_OF_THE_HARPIES.md)
remains the authority for three native exchanges, custom feather dispatch,
allegiance changes, hidden sources, shops, services, and dormant transformation
content. This supplement records bounded cross-area evidence without changing
native sources.

## Active WLD boundary and source-only ingress

All 357 authored exits resolve. The local graph has one weak component of 161
rooms, with twelve fall fields and no current fields. The only active boundary
is the reciprocal pair Harpy room #31100 south to Surface #620042 and Surface
#620042 north to #31100 ([outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L434),
[return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1624)). Static reciprocity does
not establish door state or safe player travel.

A stateful parse of all 95 WLD files outside the active `areas/AREA` list read
2,080,773 exits from 527,791 source rooms, with zero parse issues and no
source-room VNUM overlap with Harpy rooms. One source-only exit targets the
active area: unregistered `surf.wld` room #249084, “A Rocky Canyon Floor,”
north to Harpy room #31100 ([source room](../../../areas/wld/surf.wld#L1029908),
[exit](../../../areas/wld/surf.wld#L1029912),
[target tuple](../../../areas/wld/surf.wld#L1029915)). Since `surf.wld` is
excluded from `areas/AREA`, this is a source-only reference, not active ingress.
It is the 28th individually reviewed `surf.wld` boundary record and raises the
curated source-only follow-up count from 587 to 588. No other source-only WLD
target into Harpy rooms was found.

## Bounded typed and code references

Exact active typed scans found three foreign QST references to local objects:

| Source | Exact reference | Context |
| --- | --- | --- |
| Cosmic QST | `G I 31115` ([source](../../../areas/qst/cosmic.qst#L97)). | The foreign engineer exchange consumes the siege-golem plans and gives a head; the plans remain foreign story input. |
| Cosmic QST | `G I 31117` ([source](../../../areas/qst/cosmic.qst#L146)). | A separate exchange consumes the tail and returns two copper ores. |
| Divine Home QST | `G I 31105` ([source](../../../areas/qst/divhome.qst#L276)). | Wicks's foreign request accepts the wyvern egg for 25,000 copper. |

No active or unregistered ZON reset group, SHP record, or additional QST
reference matched a local prototype. A structured scan of the other 231 story
sidecars parsed without errors and found three numeric fields: Cosmic's plans
#31115 and tail #31117, and Divine Home's egg #31105. Those sidecar records
mirror the three QST contracts; they do not establish local ownership of the
foreign outcomes ([Cosmic plans](../../../areas/story/cosmic.story.json#L480),
[Cosmic tail](../../../areas/story/cosmic.story.json#L720),
[Divine Home egg](../../../areas/story/divhome.story.json#L1227)).

The exact C/C++ scan found local Harpy bindings and constants: the four active
mobile special assignments, hometown constant #31177, and the
`VOBJ_HARPY_CHOOSE_FEATHER` definition for #31112
([money changer](../../../src/specs/specs.assign.c#L801),
[gargoyle](../../../src/specs/specs.assign.c#L826),
[queen](../../../src/specs/specs.assign.c#L827),
[khan](../../../src/specs/specs.assign.c#L828),
[commented gate assignment](../../../src/specs/specs.assign.c#L2166),
[hometown constant](../../../src/core/constant.c#L1741),
[feather constant](../../../src/world/vnum.obj.h#L111)). The object assignment
for `harpy_gate` is commented out and is not an active binding. No additional
foreign code literal matched a local room, mobile, or object VNUM. The
comprehensive dossier remains the authority for procedure order, dispatch,
and the custom feather path.

## Active title and tracked history

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“The Mountain Settlement of the Harpies” owner: `harpyht` ([full-area entry](../../../areas/AREA#L151),
[QST entry](../../../areas/qst/AREA#L140),
[zone title](../../../areas/zon/harpyht.zon#L2)). The parenthetical `Alachest`
AREA note appears on five separate current contributors: `harpyht *310-312`,
`roguerai *758-759`, `cosmic *760-762`, `jade *766-771`, and
`airshipgrave *775-776` ([Roguerai](../../../areas/AREA#L273),
[Cosmic](../../../areas/AREA#L274), [Jade](../../../areas/AREA#L276),
[Airshipgrave](../../../areas/AREA#L279)). The shared note does not by itself
identify a unique title owner, Harpy alias, predecessor, or replacement.

The tracked WLD and ZON paths reach the August 2009 area-tree relocation; later
history includes a 2011 WLD edit and the 2015 quest-item flag update. The story
sidecar was added on 2026-10-05. No distinct predecessor or whole-zone
replacement is identified. The normalized-title result does not settle every
command-target alias or owner question.

## Scope and disposition

Harpyht remains an active `areas/AREA` contributor. The one active Surface
pair remains the only registered WLD boundary. The unregistered `surf.wld`
route is a separate source-only record and does not establish active ingress.
Foreign egg, plans, and tail exchanges retain their source-zone ownership.
Player reachability, hidden-source recovery, custom dispatch outcomes,
allegiance persistence, and reset renewal remain unverified. No migration or
gameplay decision follows from this review.

This documentation-only review used source and static registers. It did not
run tests, a build, server/runtime/gameplay, database work, a migration, or an
operational script. The source-comprehensive audit queue is now **94/220
complete, 126 pending**; priority 95, The Behemoth Herders (`herders`), is
next.
