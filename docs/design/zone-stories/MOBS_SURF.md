# Mobs_Surf: prototype pockets and unresolved Surface map exits

Source-comprehensive review of active contributor mobs_surf. The file title is
“The Surface World”; the inventory records its room and mobile sources and
cross-zone reset ownership. There is no local quest contract or story sidecar.
This review does not establish live access, current stock, or safe travel.

## Contributor inventory

The active areas/AREA entry is mobs_surf *314, with zone header #314 and reset
mode 2. The WLD contains 24 rooms, #31401–#31424, and 64 authored exits. Forty
targets resolve locally; the remaining 24 point to rooms absent from the
active world. The local undirected graph has four components containing 11,
5, 5, and 3 rooms.

| Local room group | WLD label | Rooms | Local graph size |
| --- | --- | --- | ---: |
| Shady/Goblin/Faang | SHADY/GOBLIN/FAANG aggro dispurse | #31401–#31411 | 11 |
| Bugbear | AROUND BUGBEAR aggro disburse zone | #31412–#31416 | 5 |
| Faang spiders | NW FAANG SPIDER AREA | #31417–#31421 | 5 |
| Faang map load | FAANG MAP SPIDER LOAD ROOM | #31422–#31424 | 3 |

All 24 unresolved exit targets appear in both inactive surf.wld and
surface2011.wld. The same-numbered room records have different titles and
descriptions in those source maps. They are unresolved map references, not
active boundaries or safe player routes.

The local MOB file defines nine prototypes:

| Mobile | Source identity |
| ---: | --- |
| #31401 | Ice elemental |
| #31402 | Ice golem |
| #31403 | Living crystal |
| #31404 | Ice pile |
| #31405 | Yeti |
| #31406 | Frost giant |
| #31407 | Ice golem |
| #31408 | Ice golem |
| #31409 | Frost wyrm |

The repeated ice-golem names are separate VNUMs with identical local short
descriptions and numeric source records. Preserve those prototype identities.
The local OBJ file exists but is empty. There is no local SHP file or story
sidecar. The QST/AREA manifest lists mobs_surf, but areas/qst/mobs_surf.qst is
absent, and there are no local Q or QA contracts.

## Reset ownership and cross-area references

The local ZON file has twenty M commands for Bloody Plains mobile #33500 in
room #31412. Each repeats the same level/population fields and 100% chance;
#33500 is defined in bloody_plains.mob as a mature male leucrotta, not in the
local mobs_surf.mob file. The repeated resets are active declarations, not a
guarantee that twenty creatures will be present.

Active external reset consumers use the nine local Mobs_Surf prototypes:

| Reset owner | Direct M commands | Source |
| --- | ---: | --- |
| surface | 54 placements covering #31401–#31409 | [First](../../../areas/zon/surface.zon#L83), [last](../../../areas/zon/surface.zon#L142) |
| multiclass_temple | 2 placements for #31402 and #31403 in room #52909 | [golem](../../../areas/zon/multiclass_temple.zon#L20), [crystal](../../../areas/zon/multiclass_temple.zon#L21) |

Inactive surf.zon has 25 further M commands for the same local prototype set
at old map rooms. Those commands are source-only and are not current stock.
This reset relationship is documented separately from the 24 unresolved WLD
exits.

## Source and qualification boundary

Direct typed scans found no external QST item or giver reference, structured
story-sidecar VNUM reference, SHP product or keeper reference, duplicate
external mobile prototype, exact compiled C/C++ VNUM reference, or portal or
switch target into the local room range. The active and inactive ZON reset
references above are the identified direct mobile consumers. The broader
map-owner decision for the old Surface targets remains open.

The audit supplement records title ownership, followed source history, exact
old-map targets, typed reset consumers, and twenty direct builder and owner
questions. Shared movement and population behavior, dynamic reset timing,
editor/tooling ownership, and persisted state remain outside source
qualification.
