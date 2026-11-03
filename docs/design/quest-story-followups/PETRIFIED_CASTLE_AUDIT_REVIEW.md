# Petrified Castle source audit

Canonical key: component:petrified_castle. Physical coverage row 432 / CSV
line 433; candidate dependency row 81 / CSV line 82. Reviewed October 11,
2026, Checkpoint 519, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static narrative,
connectivity, identity, and typed-reference review. It does not establish
runtime room loading, quest/shop boot state, editor ownership, or safe
migration.

## Package, membership, and competing names

The six same-stem source files are [petrified_castle.wld](../../../areas/wld/petrified_castle.wld#L1),
[petrified_castle.mob](../../../areas/mob/petrified_castle.mob#L1),
[petrified_castle.obj](../../../areas/obj/petrified_castle.obj#L1),
[petrified_castle.zon](../../../areas/zon/petrified_castle.zon#L1),
[petrified_castle.qst](../../../areas/qst/petrified_castle.qst#L1), and
[petrified_castle.shp](../../../areas/shp/petrified_castle.shp#L1). No
same-stem story sidecar or coordinate/map file is present. The package is
absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); the current
[RANDOM_AREA](../../../areas/RANDOM_AREA) is empty.

The ZON record names zone #938 “Tower of Testing” and ends at room #93886
with reset mode 2. That normalized title is unique among the 425 source ZON
titles scanned. The WLD and QST instead describe a decaying castle and a
kingdom under attack. Filename, zone title, room prose, and quest text are
not reconciled by an AREA annotation or manifest entry; these are competing
labels that need an owner’s explanation, not proof that this is only a test
map.

## Story-bearing content and quest contract

The WLD defines 187 rooms, #93700–#93886, with 98 distinct raw title strings;
70 rooms are literally titled “Unnamed.” The authored text describes a once
proud keep built against a cliff, a ruined gatehouse, towers and guard
passages, and a closed castle interior. For example, room #93724 describes
the long-forgotten castle and its collapsed upper structures
([WLD lines 424–436](../../../areas/wld/petrified_castle.wld#L424)).
The source therefore contains substantial authored setting even though its
loader membership is absent.

The QST file has two headers: #93704 is followed by S (no quest); #93705
contains one M/Q contract. Its requester is described as a sculpture of a
regal man with a forlorn expression and the weight of a kingdom
([mobile #93705](../../../areas/mob/petrified_castle.mob#L61)). The quest
text says a creature turned the kingdom’s survivors into statues and asks
for that creature’s head
([QST lines 3–15](../../../areas/qst/petrified_castle.qst#L3)).

There is an apparent item mismatch that needs an owner decision. The QST
records R I 93730 and G I 93731. The object prototypes identify #93730 as
glowing keys and #93731 as a bloody Medusa head
([object #93730](../../../areas/obj/petrified_castle.obj#L363),
[object #93731](../../../areas/obj/petrified_castle.obj#L371)). The quest
loader stores G clauses under quest_complete.give and R clauses under
quest_complete.receive ([quest.c lines 2202–2282](../../../src/world/quest.c#L2202));
that is opposite the head requested by the narrative if these clauses are
intended as the quester’s delivered item and reward. The ZON also gives a
head to Medusa (#93727) at room #93861
([ZON lines 351–353](../../../areas/zon/petrified_castle.zon#L351)).
Meanwhile the keys have no local ZON reset or shop stock entry; the QST
receive clause is their only package-level availability reference. Do not
change either item clause or reset until the owner confirms the intended
contract and source of the keys.

The QST file’s existence does not establish that this contract is loaded.
The quest source names areas/world.qst and explicitly declines to glob
areas/qst/*.qst when that aggregate file is unavailable
([quest file selection](../../../src/world/quest.c#L79),
[no-glob fallback](../../../src/world/quest.c#L2087)). The candidate is not
listed in qst/AREA, and areas/world.qst is absent in this checkout.

## Local graph and route gaps

The WLD has 395 directional exits: 83 north, 76 east, 85 south, 76 west,
31 up, 32 down, and three in each diagonal direction. There are two weak
components: one of 186 rooms and singleton #93723, “Guards lounge,” which
has no exits and receives no ZON reset. Of the 395 exit records, 394 target
rooms in this WLD; #93724 south targets -1
([WLD lines 424–437](../../../areas/wld/petrified_castle.wld#L424)).
There are no positive external destinations and no active WLD exits into
the candidate’s room IDs.

Six plain, non-door directions inside the main graph lack the opposite
direction tuple:

| Source direction | Target | Source line |
| --- | ---: | ---: |
| #93702 down | #93704 | [68](../../../areas/wld/petrified_castle.wld#L68) |
| #93732 west | #93731 | [561](../../../areas/wld/petrified_castle.wld#L561) |
| #93761 east | #93757 | [929](../../../areas/wld/petrified_castle.wld#L929) |
| #93798 east | #93797 | [1431](../../../areas/wld/petrified_castle.wld#L1431) |
| #93809 west | #93783 | [1586](../../../areas/wld/petrified_castle.wld#L1586) |
| #93860 south | #93859 | [2339](../../../areas/wld/petrified_castle.wld#L2339) |

These may be deliberate one-way passages, but their repeated presence beside
ordinary interior rooms also makes missing return links plausible. Verify
them against the editor or an owner walkthrough before changing directions.
WLD room #93702 ([block at line 55](../../../areas/wld/petrified_castle.wld#L55))
has one fall-chance field of 60 ([F field at line 80](../../../areas/wld/petrified_castle.wld#L80));
no current fields were found. This WLD F field is separate from the ZON F
follower-reset commands.

## Reset, population, and shop records

The ZON has 288 commands: 40 D door-state commands, 10 O, 9 P, 87 M mobile,
97 E equipment, 19 F follower, and 26 G give commands. Every reset room
reference is inside #93700–#93886; all reset mobile/object references resolve
to the candidate prototypes. All 40 D commands match a WLD door/direction
record. The raw WLD door fields and ZON state values should not be compared
as identical encodings: db.c interprets D reset states as bit fields. The
singleton #93723 has no reset. Mobile #93718 has a blank short description
and no reset use; object #93733 has no ZON, shop, or QST reference.

The SHP source contains one shop header, #93704, with seven local products:
#93700, #93701, #93706, #93708, #93711, #93732, and #93724. The shopkeeper
prototype is mobile #93704, “shop keep.” The source shop loader first opens
its aggregate SHOP_FILE and has a fallback that concatenates areas/shp/*.shp
([shop.c lines 3276–3318](../../../src/economy/shop.c#L3276)); the fallback
does not prove which stream a particular runtime uses.

## Same-type ownership and typed dependencies

No current active WLD, MOB, or OBJ file owns any of the candidate’s
same-type VNUMs; no other source WLD/MOB/OBJ file scanned defines a
same-type collision for the 187 rooms, 42 mobiles, or 49 objects. The active
WLD target scan finds zero entries into #93700–#93886. Static typed-reference
scans find zero active ZON room/mobile/object references, zero active
QST/AREA references, zero other active SHP numeric matches, and zero active
ITEM_TELEPORT value[0] destinations in the candidate range; item type 25 is
ITEM_TELEPORT ([defines.h line 140](../../../src/core/defines.h#L140)).
No direct petrified_castle literal was found in the scanned C/C++ source.
The candidate’s own ZON, QST, and SHP references are local as described
above. This is static source closure only; it cannot see editor projects,
external imports, deployed aggregates, dynamic code, or persisted identities.

## Tracked history and disposition

Bounded same-path history shows the WLD and MOB first present in the
2011-07-31 “Hello nr. 2.” commit; the WLD later received a 2011 typo edit and
the 2012 zone-field update. The OBJ has 2011, 2012, and 2016 content/format
edits and a 2026 ANSI fix. The ZON history includes a 2011 stub and a 2015
quest-item-flag update. The QST path begins in the 2011-07-31 commit. The SHP
path appears in the 2009 area-tree move and the 2011-07-31 commit. No explicit
successor, whole-package replacement, or retirement decision was found in
the tracked paths or the curated replacement register.

**Disposition: RETAIN IN PLACE.** The unlisted package is story-bearing and
has local reset, shop, and quest data, alongside a disconnected room, one
negative exit, six missing reverse tuples, and an apparent quest-item
mismatch. Keep the source intact until the owner resolves the name/test
relationship, negative and one-way route intent, QST item exchange, shop and
quest import path, editor provenance, and persisted/deployed references.
No activation, merge, renumber, move, or archive is cleared by this audit.
See [20 builder questions](PETRIFIED_CASTLE_BUILDER_QUESTIONS.md),
[coverage row 432](ZONE_COVERAGE_REGISTER.csv#L433),
[candidate dependency row 81](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L82),
[connectivity checkpoint 519](WORLD_CONNECTIVITY.md#checkpoint-519-petrified-castle-isolated-keep-and-one-negative-exit),
[identity checkpoint 519](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-519-petrified-castle-story-package-with-no-same-type-collision),
and [retention checkpoint 519](ATTIC_MIGRATION_PLAN.md#checkpoint-519-petrified-castle-retain-pending-owner-and-reference-closure).
