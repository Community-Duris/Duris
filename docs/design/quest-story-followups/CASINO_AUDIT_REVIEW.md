# Casino Audit Review

Reviewed October 11, 2026, Checkpoint 460. This is a static source, narrative, WLD graph, followed-history,
and typed-reference review of the unregistered `casino` component. It does not establish active loading,
player access, gambling mechanics, object reset behavior, or permission to move files.

## Source scope and identity

The current source family contains [WLD](../../../areas/wld/casino.wld), [MOB](../../../areas/mob/casino.mob),
[OBJ](../../../areas/obj/casino.obj), and [ZON](../../../areas/zon/casino.zon). The MOB file is empty.
The zone header is #778, titled “CasinoZone,” with last VNUM #77815 and reset mode 2. The WLD defines
15 rooms #77801–#77815, the OBJ defines four fixture prototypes #77801–#77804, and the ZON has no
M or G resets. It is absent from the current [full area manifest](../../../areas/AREA) and
[QST manifest](../../../areas/qst/AREA); there is no same-stem SHP, QST, or story sidecar.

## Environmental story and game fixtures

Room #77801 is the casino entrance. Rooms #77802–#77810 describe a dark, smoky gaming floor with
bells, dice, cards, and machines; room #77811 is a velvet-rope divider; and #77812–#77815 describe
a higher-end platinum gaming area ([entrance](../../../areas/wld/casino.wld#L1),
[main floor](../../../areas/wld/casino.wld#L10),
[velvet rope](../../../areas/wld/casino.wld#L204),
[high-rollers rooms](../../../areas/wld/casino.wld#L221)). The text gives atmosphere and pricing flavor.
It does not define a wager, odds, payout, or player-facing game contract.

The four local objects are named roulette table, wheel of fortune, high-roller slot machine, and solitaire
table ([roulette table](../../../areas/obj/casino.obj#L1),
[wheel](../../../areas/obj/casino.obj#L9),
[slot machine](../../../areas/obj/casino.obj#L17),
[solitaire table](../../../areas/obj/casino.obj#L25)). The WLD/ZON set contains no mobile, shop, or quest
source that supplies an operator, stakes, or rewards. Tobacco smoke appears in the environmental prose.
No native QST or sidecar extends it into a casino activity contract.

## Active Hall ownership of all casino room VNUMs

The active [Hall of the Ancients](../../../areas/zon/hall.zon#L1) covers zone range *777–779 in the
[full area list](../../../areas/AREA#L280). Its WLD defines every numeric room VNUM #77801–#77815 with
zone fields set to 777. Those same numbers identify the unregistered Casino's rooms with zone fields
set to 778, and their descriptions and exits differ. For example, Casino room #77801 is its doors,
while active Hall #77801 is on the Topmost Ring of Land; Casino #77803 is a gaming-floor room, while
active Hall #77803 is the stone house exterior
([Casino entrance](../../../areas/wld/casino.wld#L1),
[Hall #77801](../../../areas/wld/hall.wld#L2319),
[Casino floor](../../../areas/wld/casino.wld#L32),
[Hall #77803](../../../areas/wld/hall.wld#L2360)). Casino’s #77812 high-rollers room also collides
with a Hall waterfall branch ([Casino room](../../../areas/wld/casino.wld#L221),
[active Hall room](../../../areas/wld/hall.wld#L2521)).

The active Hall WLD has 23 authored exit records whose targets are in the #77801–#77815 numeric
range. Those records already target active Hall rooms. They do not establish active access to the
unregistered Casino identity, so candidate-specific active inbound exit count remains zero. Active
Hall object #77714 is an entrance-to-waterfall portal record with destination value #77813; Hall's
active #77813 is the waterfall room, while Casino calls that number a high-rollers room
([object destination](../../../areas/obj/hall.obj#L265),
[Hall room #77813](../../../areas/wld/hall.wld#L2539),
[Hall story map](../zone-stories/THE_HALL_OF_THE_ANCIENTS.md)). Preserve this active identity when
classifying the old Casino map.

## Static WLD graph

The Casino WLD has 15 rooms and 38 authored directions. All 38 targets are within its local
#77801–#77815 set. The local graph is one weak component, and directed traversal from #77801 reaches
all 15 rooms. All room fields are zone 778, consistent with their VNUM range. There is no cross-zone
outbound exit, positive unresolved target, negative exit, fall-chance field, or current field.

All local target numbers currently resolve in the full active world to Hall WLD rooms with different
identities. This numeric resolution does not load or reach the Casino map. No candidate-specific active
inbound exit or new eligible inactive-WLD approach was found; global totals remain 11,987 overall /
415 Surf. See the [active Hall owner](../../../areas/wld/hall.wld#L2319) and the
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-460-casino-graph-and-hall-vnum-overlap).

## Reset and typed references

The ZON contains two D door-state commands and ten O object placements. Its comments describe both
sides of the casino entrance door as closed, while the raw D state values are 2 for room #77801
south and 0 for room #77802 north ([door resets and comments](../../../areas/zon/casino.zon#L8)).
Keep the authored comments and numeric state values distinct until the area owner confirms the intended
initial door state. The active Hall ZON separately configures door records on its same-numbered rooms.

Eight O commands place local Casino fixtures #77801–#77804. Two O commands import external object
#45 into rooms #77803 and #77804; the ZON comments say this object is absent from casino.obj
([Casino resets](../../../areas/zon/casino.zon#L17)). The active Heavens catalog defines object #45
as a slot machine ([prototype](../../../areas/obj/heavens.obj#L525)), which matches the source's
intended gaming fixture description. If the Casino ZON were ever included, its target room numbers
would resolve to the active Hall house rooms. The four local Casino object prototypes are not present
in the active object catalog. The object #45 reference remains an external source dependency, not an
active Casino reset.

No exact #77801–#77815 C/C++ VNUM reference was found in the bounded source scan. The source data
therefore describes room atmosphere and physical fixtures without an identified compiled Casino
handler, current mobile, SHP, or QST contract.

## Followed history and replacement review

Followed paths show casino.wld at the same stem from the 2009 areas-tree move; casino.mob is an empty
tracked file; casino.obj has same-stem changes through 2013; casino.zon has a same-stem edit in 2015.
No alternate-stem rename or explicit replacement decision was found in these paths. The active Hall
map currently owns every overlapping room VNUM with different environmental identities. That active
numeric ownership is current map evidence; the bounded histories do not establish that Casino was
renamed to Hall or formally replaced by it.

## Safe disposition

Keep `casino` **RETAIN IN PLACE**. Resolve the active Hall room-number overlap, the two external object
#45 imports, the entrance door reset values and comments, intended gambling mechanics, editor/tooling,
and persisted VNUM identity before proposing a separate path change. No area, code, object, reset,
manifest, or world-list change is authorized by this audit. The full owner-question list is in the
[Casino builder questions](CASINO_BUILDER_QUESTIONS.md).

## Checkpoint 816: Hall typed closure confirms Casino room operand identity

The active Hall typed-closure review confirms the existing numeric ownership
finding: Casino's twelve `D`/`O` room-number operands are two door-source room
fields (#77801/#77802) and ten object-placement destinations (#77803/#77804,
#77805, #77808–#77810, #77812–#77815). These VNUMs identify different rooms in
active Hall. The Casino file remains unregistered; its room fields do not become
active Hall resets or graph edges, and Hall's unchanged WLD census does not
establish Casino ingress. The two Casino imports of active Heavens object #45
remain separate typed dependencies. Keep candidate row 23 **RETAIN IN PLACE**
and preserve the room identities, persisted ownership question, and activation
boundary. See the [Hall typed closure](HALL_OF_THE_ANCIENTS_AUDIT_REVIEW.md#checkpoint-816-current-typed-dependency-closure),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-816-hall-item-dependencies-remain-separate-from-the-wld-graph),
[attic plan](ATTIC_MIGRATION_PLAN.md#checkpoint-816-hall-and-casino-retain-separate-source-identities),
and [candidate dependency row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L23).
