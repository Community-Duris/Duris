# The Elemental Groves: repository-wide source audit supplement

This checkpoint supplements the [Elemental Groves dossier](../zone-stories/THE_ELEMENTAL_GROVES.md). It reconciles the active WLD boundary, all unregistered WLD inputs, typed references, shared code matches, normalized title, and tracked path history. These are static source findings; they do not establish live access or player history.

## Active WLD boundary and source-only approach

The Elemental Groves has 384/384 resolving exits across 165 rooms in one local weak component. Its sole active boundary is reciprocal: Element room #3800 east enters Surface room #531681, and Surface #531681 west returns to #3800 ([Element row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L48), [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1391)).

Separately, unregistered `surf.wld` room #215024 has a west (`D3`) exit to Element room #3800 ([source room](../../../areas/wld/surf.wld#L315232), [exit](../../../areas/wld/surf.wld#L315248), [target](../../../areas/wld/surf.wld#L315251)). This is a source-only WLD approach and is distinct from the active `surface.wld` reciprocal pair.

The refreshed scan parsed all 95 unregistered WLD files: 527,791 source rooms and 2,080,773 exits, with zero parse issues and no source-room VNUM overlap with Element. No other unregistered WLD exit targets Element rooms. This route raises the curated source-only total from 593 to 594 and the reviewed `surf.wld` subset from 31 to 32.

## External typed and code references

No external native QST matched an Element prototype. All 231 story sidecars parsed without error and produced no exact structured numeric reference. A broad digit scan found 3967, 3942, and 3853 only inside string identifiers in Alatorin and Winterhaven sidecars ([Alatorin ID](../../../areas/story/alatorin.story.json#L3937), [Winterhaven IDs](../../../areas/story/wh.story.json#L969), [another Winterhaven ID](../../../areas/story/wh.story.json#L1028)); these are not prototype fields.

Two raw SHP stock-list hits use number 3902, one in active Charing and one in unregistered Sylvandawn ([Charing stock](../../../areas/shp/charing.shp#L255), [Sylvandawn stock](../../../areas/shp/sylvandawn.shp#L592)). The number is also Element room #3902 ([room](../../../areas/wld/element.wld#L2201)), but it is outside Element's object range #3800–#3837. These are numeric collisions, not stock references to an Element object. No external SHP stock hit matched an Element object VNUM.

No external ZON reset-role, MOB/OBJ prototype definition, or exact sidecar reference matched. The scan of 872 external type-25 portal declarations found no Element room destination. A broad C/C++ number scan found Element object #3833 in its local `glades_dagger` assignment ([local assignment](../../../src/specs/specs.assign.c#L1319)); it found no foreign prototype consumer. Other number hits are a strength-application table ([`str_app`](../../../src/core/constant.c#L860)), floating-point constants ([table](../../../src/classes/memorize.c#L77)), and a database telemetry error code ([case](../../../src/telemetry/telemetry_failure.c#L61)).

## Title and tracked history

The normalized title scan found one “The Elemental Groves” title among 351 active area contributors. The active key is `element`, scope `*38-39`, with the matching title in its ZON ([area entry](../../../areas/AREA#L20), [zone title](../../../areas/zon/element.zon#L2)). The duplicate/replacement register has no separate retired or replacement area for the Groves ([register](DUPLICATE_AND_REPLACED_ZONES.md)).

Tracked WLD/ZON history remains on the same `element` source paths from the 2009 area-tree move (`642da5400`), with later WLD edits in 2011/2013/2025/2026 and ZON edits in 2013/2015. It records no distinct predecessor or path replacement.

## Audit boundary

This review used repository source and static registers only. No tests, build, server/runtime, gameplay, database, migration, or operational script was run. The source-comprehensive queue is now **100/220 complete, 120 pending**; priority 101, Temple of the Earth (`earth`), is next. Source mapping remains separate from played source custody, reset issuance, room access, and return-safety qualification.

## Checkpoint 779: Caertannad typed dependency

Active Elemental Groves resets Caer Tannad object #78455 twice. See [Caertannad inbound consumers](CAERTANNAD_AUDIT_REVIEW.md#active-inbound-consumers).

## Checkpoint 794: current typed census refresh

### Active ZON prototype operands

Element has 110 prototype-reset commands: 63 M, 14 O, five G, 20 E, and
eight P. Each P row supplies both a child and container object VNUM, yielding
118 typed prototype operands. Of these, 113 resolve to local mobiles or
objects, and five are active external object imports; no operand is unresolved.

All five imports are O resets: Caertannad object #78455 twice
([first reset](../../../areas/zon/element.zon#L145)), Heavens object #358
([reset](../../../areas/zon/element.zon#L161)), White Harbor object #55450
([reset](../../../areas/zon/element.zon#L165)), and Heavens object #750
([reset](../../../areas/zon/element.zon#L166)). Each imported object has a
current same-type definition in its owner source
([Caertannad](../../../areas/obj/caertannad.obj#L820),
[Heavens #358](../../../areas/obj/heavens.obj#L3709),
[Heavens #750](../../../areas/obj/heavens.obj#L7309),
[White Harbor](../../../areas/obj/wh.obj#L5309)). Caertannad's audit already
records its two inbound rows
([consumer register](CAERTANNAD_AUDIT_REVIEW.md#active-inbound-consumers)).

### QST, story, and external consumers

The local QST has four R/G item fields (#3830, #3831, #3808, #3809), all
resolving locally ([QST](../../../areas/qst/element.qst#L395)). The structured
sidecar has 29 typed VNUM values: 20 mobile, six giver, and three item values,
all local ([sidecar](../../../areas/story/element.story.json#L23)). None of the
231 other current story sidecars contains a typed reference to an Element
room, mobile, or object.

No external QST item/giver header, ZON reset-role, SHP product, or external
MOB/OBJ definition targets an Element prototype. Element has no local
SHP/shop, and no same-type external mobile or object definition duplicates
one of its local IDs. The earlier raw SHP #3902 hits remain numeric collisions
with Element room #3902, outside the local object range.

### Item travel and compiled references

Element declares no local type-25 teleport or type-29 switch object. The scan
of 872 external type-25 and 380 external type-29 declarations found no room
target in the 165-room Element WLD. No item travel field changes its authored
WLD edge totals.

The direct compiled prototype binding remains object #3833 to glades_dagger
([assignment](../../../src/specs/specs.assign.c#L1319)). Other exact-number
hits remain the previously classified stat table, floating-point values, and
telemetry error code; no external compiled prototype consumer was found.
This closes static typed-source inventory only. Runtime reset admission,
shop operation, editor/tooling, persisted identity, and played qualification
remain unqualified. No source area, candidate disposition, gameplay, or WLD
total changed.
