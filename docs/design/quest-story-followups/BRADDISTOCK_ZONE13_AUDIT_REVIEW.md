# Braddistock Mansion, zone 13 audit supplement

Checkpoint 775 completes the static typed-dependency inventory for the active
zone-13 Braddistock Mansion area key braddistock. It distinguishes this source
from the separately active zone-1350 Mansion area key brad, then records the
Mansion's Heavens reset imports, active and candidate inbound references,
Tharnadian Ruin's QST consumer, and direct code uses of local objects. It does
not change either active source identity or gameplay.

## Scope and source connectivity

The zone-13 source has 46 rooms in its #1300–#1399 range, 14 mobile
prototypes, 73 objects, two native Q/QA contracts, and no local SHP file. Its
WLD census has 93/93 loader-resolved exits; the registered Tharnadia boundary
is one reciprocal pair. The [coverage row](ZONE_COVERAGE_REGISTER.csv#L37)
records two local weak components, the largest with 45 rooms. The newer
zone-1350 source with the same display title remains a distinct active area;
see [the existing boundary/title audit](BRADDISTOCK_MANSION_AUDIT_REVIEW.md)
and [the zone-13 story dossier](../zone-stories/BRADDISTOCK_MANSION.md).

## Local reset imports and typed consumers

The Braddistock ZON imports two Heavens objects through P resets:

| Source record | Typed dependency | Current prototype owner |
| --- | --- | --- |
| [braddistock.zon line 123](../../../areas/zon/braddistock.zon#L123) | P reset references object #364 | Active Heavens object, iron rations |
| [braddistock.zon line 124](../../../areas/zon/braddistock.zon#L124) | P reset references object #398 | Active Heavens object, large wooden torch |

No other external mobile/object prototype is imported by the local reset
roles. Keep both objects owned by Heavens and preserve the authored container
reset context.

| External consumer | Exact reference into Braddistock | Classification |
| --- | --- | --- |
| Active Alatorin ZON | [P reset line 1469](../../../areas/zon/alatorin.zon#L1469) references Braddistock object #1310, the ring of protection. | Active cross-area container-reset reference. |
| Active Mini Zones ZON | [O reset lines 388–389](../../../areas/zon/minizones.zon#L388) references Braddistock object #1320, the small diamond. | Two active ground-reset records. |
| Active Tharnadian Ruin QST and sidecar | QST [G I #1375 at lines 80 and 138](../../../areas/qst/tharnadian_ruin.qst#L80); structured item_vnums at [sidecar line 94](../../../areas/story/tharnadian_ruin.story.json#L94). | Active foreign item consumer of Braddistock's Lord Braddistock remains #1375. Preserve the exact item owner and both contracts. |
| Unregistered Nexus56 ZON and SHP | ZON [G reset line 1378](../../../areas/zon/nexus56.zon#L1378) references Braddistock object #1335; [shop #83038 product line 501](../../../areas/shp/nexus56.shp#L501) lists the same object. | Candidate-source records only; preserve under Nexus56 RETAIN IN PLACE. |

The Tharnadian Ruin references above are the only active external QST and
story-sidecar consumers found; no other external active QST item command or
active SHP product targets a Braddistock object. No outside story-sidecar
item_vnums field targets another local object, no external QST giver or story
giver targets a local Braddistock mobile, and no other outside reset targets
the area's mobile or object set. Nexus56's current shop-loader fallback can encounter
individual SHP files in this checkout because areas/world.shp is absent, but
that source path does not establish shop boot or stock operation. See
[open_shop_stream](../../../src/economy/shop.c#L3267).

## Prototype identity, code references, and portal scan

Exact same-type scans found no second current mobile or object owner for the
local prototype IDs. The cross-area numeric matches resolve by type and source
file, not by shared numeric range:

| Compiled source | Exact local reference | Meaning |
| --- | --- | --- |
| [newbie_kit_plan.c line 1621](../../../src/account/newbie_kit_plan.c#L1621) | Object #1317 | The Skeleton Rogue starting kit includes Braddistock's sharp assassin's dagger. This is an item reference despite same-number room/mobile values in the local area. |
| [specs.assign.c line 1279](../../../src/specs/specs.assign.c#L1279) | Object #1372 | Assigns the jet_black_maul object procedure to Braddistock's jet-black maul of the titans. |

A scan of current object declarations found no external ITEM_TELEPORT
destination or ITEM_SWITCH value[1] room field into Braddistock rooms
#1300–#1399, and no locally owned ITEM_TELEPORT or ITEM_SWITCH object. Existing regular WLD travel remains the
single reciprocal Tharnadia boundary in the connectivity registers.

## Related active and candidate registers

The exact source edges are now cross-linked from this active owner's review,
the relevant active source rows, and the Nexus56 candidate plan. Keep
Heavens, Alatorin, Mini Zones, Tharnadian Ruin, and Nexus56 as distinct source
owners. Preserve Nexus56's RETAIN IN PLACE plan and its #1335 reset/shop
records; do not add an attic row for active Braddistock.

Keep zone-13 Braddistock **DO NOT MOVE** as its current active
areas/AREA contributor. Its separate same-title zone-1350 neighbor remains
the active brad key. No renumbering, merge, path move, or gameplay change is
supported by these typed links. Runtime reset admission, shop loading,
object access, and player custody remain unqualified.
