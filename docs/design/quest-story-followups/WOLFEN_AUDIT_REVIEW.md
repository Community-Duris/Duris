# The Lost Realms of Shadowfall: boundary and typed-reference audit

Status: repository-wide static supplement to the
[Shadowfall source review](../zone-stories/WOLFEN_STORY_REVIEW.md). It
reconciles current WLD boundaries and directed reach, reset-backed portal
links, bounded typed references, tracked title/history, and retained ownership.
No area source, player state, or runtime behavior was changed or qualified.

## Active WLD boundary and local graph

The active `areas/AREA` entry is `wolfen *180-182`; the ZON title is “The Lost
Realms of Shadowfall.” All 632 WLD exit targets resolve. The only boundary is
the reciprocal Surface connection: Wolfen room #18000 north to Surface room
#620706, with the Surface south exit returning to #18000. The repository cross
register records both source exits. No unregistered WLD source target into the
area was found in the bounded active/candidate scan. [Wolfen exit](../../../areas/wld/wolfen.wld#L10),
[Surface exit record](../../../areas/wld/surface.wld#L2534290),
[cross-area row 305](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L305),
[reciprocal row 1626](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1626), and
[active-area owner](../../../areas/AREA#L92).

Ignoring item travel objects, the local WLD graph has two weak components:
255 rooms in the main map and the three-room tunnel (#18000–#18002). The
Surface arrival at #18000 can follow local WLD exits through those three
tunnel rooms; the WLD-only map has no edge from that component into #18003's
main component. Two zone-reset rifts bridge the components as directed
type-25 portals: object #18030 in #18003 targets #18002, and object #18029 in
#18002 targets #18003. The separately reset wagon portal #18034 in campsite
room #18031 targets wagon room #18256; the wagon's room #18256 exits west to
the campsite and room #18257 connects to its storage interior. With these
three source-declared object links added to directed WLD edges, source reach
from #18000 covers all 258 rooms. This is a bounded source graph, not proof
that the objects are present, usable, safe, or return a live player.
[Rift/wagon resets](../../../areas/zon/wolfen.zon#L25),
[rift targets](../../../areas/obj/wolfen.obj#L575),
[wagon portal](../../../areas/obj/wolfen.obj#L646),
[wagon rooms](../../../areas/wld/wolfen.wld#L6597).

The two tunnel exits are blocked by their ZON `D ... 8` state. Their reset
slabs #18031/#18032 are `ITEM_SWITCH` objects configured for `CMD_PUSH` and
the paired exit VNUM/direction. The generic item-switch handler documents
these value fields and clears `EX_BLOCKED`; the ZON reset's state bit 8 sets
that flag. The source wiring therefore describes a data-driven push-to-open
route, while actual command, object instance, and live reset behavior remain
unqualified. A separate two-sided cottage door between #18139 and #18255
resets closed with no key VNUM. [Tunnel door resets](../../../areas/zon/wolfen.zon#L8),
[slab values](../../../areas/obj/wolfen.obj#L603),
[ITEM_SWITCH definition](../../../src/core/defines.h#L144),
[CMD_PUSH](../../../src/cmd/interp.h#L305),
[switch handler](../../../src/specs/specs.object.c#L302),
[door reset implementation](../../../src/world/db.c#L7857), and
[cottage door resets](../../../areas/zon/wolfen.zon#L16).

The WLD sources record no unresolved targets, fall-chance rooms, or currents.
The road/city/forest graph and the item-linked routes still require live
qualification for access, combat safety, party behavior, reset generation,
and return. [Forest room](../../../areas/wld/wolfen.wld#L898),
[cottage door](../../../areas/wld/wolfen.wld#L3480), and
[wagon interior exit](../../../areas/wld/wolfen.wld#L6597).

## Direct typed references and source closure

The active same-type MOB and OBJ definition scan found only Wolfen as owner of
mobile prototypes #18000–#18019 and object prototypes #18000–#18039. Other
active ZON files contain no direct reset of those mobile/object prototypes;
other active SHP files contain no shopkeeper or stock reference in those
ranges; other active QST files contain no item/giver prototype reference; and
active story sidecars contain no numeric reference in the room/prototype
ranges. The single raw `R E 18000` match in `newhaven.qst` is an experience
reward: the QST loader maps `R E` to `QUEST_GOAL_EXP`, so it is not a reference
to Wolfen object #18000. [Newhaven reward record](../../../areas/qst/newhaven.qst#L139)
and [QST reward parser](../../../src/world/quest.c#L2260).

The bounded active type-25 object scan found no portal target in Shadowfall
from another area. A targeted source-code VNUM scan found these direct local
consumers:

| Local prototype | Source consumer | Static role |
| --- | --- | --- |
| Mobile #18001, Karig | [Epic teacher table](../../../src/classes/epic_skills.c#L178), [teacher-proc binding](../../../src/world/epic.c#L308) | Teacher entry for `SKILL_SILENT_SPELL` |
| Object #18000, the Answerer | [Special assignment](../../../src/specs/specs.assign.c#L2052) | `generic_riposte_proc` assignment |
| Objects #18000 and #18001 | [Chaos equipment data](../../../src/account/chaos_eq_data.h#L110), [#18001 loadout](../../../src/account/chaos_eq_data.h#L267), [#18000 loadout](../../../src/account/chaos_eq_data.h#L361), and [second #18001 loadout](../../../src/account/chaos_eq_data.h#L511) | Static equipment-table VNUMs |
| Object #18000 | [Chaos seed helper](../../../src/combat/chaos.c#L143) | Included in a pouch test-seeding VNUM list |

These code references establish prototype dependencies only; invocation,
eligibility, equipment delivery, and player outcomes are not qualified. The
local QST file's two Karig actions and the local instrument shop are described
in the [source review](../zone-stories/WOLFEN_STORY_REVIEW.md#access-objects-local-services-and-non-contract-content).

## Active title and tracked history boundary

After color-code and case normalization, “The Lost Realms of Shadowfall” is
unique among the active ZON titles. Followed WLD, MOB, OBJ, ZON, QST, and SHP
histories stay on the `wolfen` stem and reach the August 2009 areas-tree move;
no alternate-stem predecessor, renamed source, or whole-area replacement was
identified. Current-area text contains Wolfen Empire history but no matching
second owner; the active titles “The Jade Empire” and “Mitashi - Capital City
of the Jade Empire” are distinct. Command-target alias disambiguation remains
separate in coverage column 37. [Active title](../../../areas/zon/wolfen.zon#L2),
[tracked title/history register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-435-shadowfall-title-history-and-owner-boundary),
and [coverage row 348](ZONE_COVERAGE_REGISTER.csv#L348).

## Audit boundary

These findings are static source evidence. They do not establish a safe live
Surface arrival, successful slab interaction, portal invocation, reset
availability, shop stock, faction eligibility, current disease events, item
custody, quest completion, or safe return. Persisted-player state, editor and
tooling references, and indirect dynamic consumers remain outside this
bounded closure.

## Checkpoint 435: builder question review

Twenty source-backed questions and recommended resolutions now collect the
open historical, outbreak, route, shop, and authored-action decisions in
[Shadowfall builder questions](../zone-stories/WOLFEN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
This checkpoint makes no area-source or runtime behavior change.
