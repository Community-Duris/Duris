# UnderDark 2011 audit supplement

**Checkpoint 552 — source narrative, graph, typed references, map identity, and followed history.** This review covers the five checked-in source files for underdark2011. It does not change game sources or establish runtime availability.

## Inventory and source status

The source set is areas/wld/underdark2011.wld, areas/mob/underdark2011.mob, areas/obj/underdark2011.obj, areas/zon/underdark2011.zon, and areas/qst/underdark2011.qst. There is no same-stem SHP, story JSON, or map-editor JSON in the checked-in areas tree.

The WLD and ZON identify zone #2000, title “Norabi Map,” top VNUM #289999, and reset mode 2. No underdark2011 entry appears in areas/AREA or areas/qst/AREA. The active underdark contributor is a separate source family at #7000–#8599 with a different title ([active manifest](../../../areas/AREA#L413); [candidate ZON](../../../areas/zon/underdark2011.zon#L1); [QST manifest entries](../../../areas/qst/AREA#L345)).

## Authored story content

The candidate WLD contains 15,482 rooms in the #200000–#289999 range. The 90 raw title strings normalize to 88 titles. The largest visible groupings are Mountains (3,027 rooms), Low-Ceiling Caverns (3,005), Massive Obsidian Walls (2,147), Rock-Filled Cavern (2,077), icy Black Mountain slopes (744), mossy stalagmite caves (613), jagged obsidian peaks (523), tropical mountain terrain (512), and giant purple-worm tunnels (449). Smaller named regions include Chief Kerzt Bloodeye’s spires, the Jade Empire, Magma Mountain, the Mountains of the Damned, Mushroom Forest, and IceCrag. Every room’s long-description field is empty; titles and exits carry the checked-in environmental framing, but this source does not provide room prose.

The 68 mobile prototypes include purple-worm life stages, driders, ropers, and other cavern inhabitants. Eight object prototypes include two ancient-amulet halves, bronze-plated traveling boots, two fine drow scepters, a flail, Kryz’kyssik’s seal, a shepherd’s hook, and a bloody roper skin ([mobile source](../../../areas/mob/underdark2011.mob#L1); [object source](../../../areas/obj/underdark2011.obj#L1)).

The sole QST giver is #200036, Kryz’kyssik. Its text asks for bloody roper skins and the two halves of an ancient amulet held by Ozman’dyzus and Haz’on’wyz. Its first completion block accepts object #200007 and has an R C 100000 reward record; the dialogue promises fifty platinum coins. Its second block accepts objects #200000 and #200001, with reward records for object #200005 and 250,000 experience ([quest source](../../../areas/qst/underdark2011.qst#L1)). In the quest parser, G records become offer goals and R records become completion rewards; the turn-in path checks offer goals against player inventory ([goal parser](../../../src/world/quest.c#L2197), [turn-in path](../../../src/world/quest.c#L1723), [reward path](../../../src/world/quest.c#L1788)). The coin denomination mapping and dialogue-to-reward contract need owner confirmation.

The area-side underdark.qst repeats this dialogue and goal shape under renumbered IDs: giver #700036, roper-skin item #700008, amulet halves #700000/#700001, and seal #700005. This source similarity is evidence of shared quest lineage, not evidence that underdark2011 replaced or currently supplies the active map. The candidate ZON has no reset commands, so it does not establish a placement for #200036. The runtime quest loader opens areas/world.qst; that generated input is absent from this checkout, and the candidate file is not in areas/qst/AREA. Checked-in QST text alone therefore does not establish a live quest ([candidate QST](../../../areas/qst/underdark2011.qst#L1); [area-side QST](../../../areas/qst/underdark.qst#L1); [loader path](../../../src/world/quest.c#L79)).

## Candidate WLD graph and boundaries

The candidate has 61,931 authored exit records: 50,697 target rooms are local; 10 point to unique active WLD owners; and 11,224 point outside the candidate to unregistered WLD identities. The 11,224 divide into 11,222 records with 6,826 distinct target VNUMs found in both surf.wld and surface2011.wld, plus two routes to Jungle. No candidate exit has a negative target, and every positive target is represented in at least one current WLD source. The candidate source is not active, so the 50,697 local records and all 11,224 unregistered-map records are unresolved against the current loader; the 10 active destinations resolve by VNUM. The static candidate graph has 26 weak components, the largest containing 4,154 rooms. No fall-chance or current fields were found.

The ten outbound exits to active areas are source-only: underdark2011 is omitted from areas/AREA, and none has a reciprocal exit in its active destination.

| Candidate source | Active target | Source evidence |
| --- | --- | --- |
| #224716 east | IceCrag #97001 | [WLD exit](../../../areas/wld/underdark2011.wld#L109751); [IceCrag review](ICECRAG_AUDIT_REVIEW.md) |
| #232202 down | Divhome #40780 | [WLD exit](../../../areas/wld/underdark2011.wld#L145799); [Divhome coverage](ZONE_COVERAGE_REGISTER.csv#L76) |
| #249233 east | Herders #94300 | [WLD exit](../../../areas/wld/underdark2011.wld#L182898); [Herders review](HERDERS_AUDIT_REVIEW.md) |
| #249534 north | Herders #94300 | [WLD exit](../../../areas/wld/underdark2011.wld#L184049); [Herders review](HERDERS_AUDIT_REVIEW.md) |
| #250432 south | Grave #8400 | [WLD exit](../../../areas/wld/underdark2011.wld#L188572); [Grave review](GRAVE_AUDIT_REVIEW.md) |
| #250731 east | Grave #8400 | [WLD exit](../../../areas/wld/underdark2011.wld#L189933); [Grave review](GRAVE_AUDIT_REVIEW.md) |
| #255552 north | Githzerai Stronghold #44400 | [WLD exit](../../../areas/wld/underdark2011.wld#L216032); [Githzerai review](GITHZERAI_STRONGHOLD_AUDIT_REVIEW.md) |
| #272717 down | Troll Caves #96900 | [WLD exit](../../../areas/wld/underdark2011.wld#L298935); [Troll Caves review](TROLL_CAVES_AUDIT_REVIEW.md) |
| #274261 up | Faang #15411 | [WLD exit](../../../areas/wld/underdark2011.wld#L305491); [Faang coverage](FAANG_STORY_REVIEW.md) |
| #278150 down | Kvarkpass Connector #54804 | [WLD exit](../../../areas/wld/underdark2011.wld#L320912); [Kvarkpass review](KVARKPASS_CONNECTOR_STORY_REVIEW.md) |

The separate two source-only exits into unlisted Jungle are #253961 south to #67005 and #255161 north to #67087 ([source records](../../../areas/wld/underdark2011.wld#L207388), [Jungle review](JUNGLE_AUDIT_REVIEW.md#candidate-wld-graph-active-targets-and-source-only-approaches)). Keep these apart from the ten active-destination records above.

Eleven active WLD exits also target VNUMs found in this candidate. Each remains positive-unresolved in the active-exit register, whose candidate column lists surf, surface2011, and underdark2011 together. The target number therefore does not identify this map as owner.

| Active source records | Candidate target VNUMs | Evidence |
| --- | --- | --- |
| Ashrumite #66149 east | #224166 | [unresolved row 253](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L253) |
| Connectorzones #53661 west; #53672 east; #53683 west; #53781 west; #53782 east | #218196, #218200, #270228, #261258, #261263 | [rows 227–228](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L227), [row 232](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L232), [rows 238–239](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L238) |
| Crypt #14568 up | #265588 | [unresolved row 39](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L39) |
| Desolate Under Fire #77301 south | #217609 | [unresolved row 260](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L260); [room-identity conflict](DESOLATE_UNDER_FIRE_AUDIT_REVIEW.md) |
| Patrols #30500 west; #30504 north/east | #209935, #218475, #222076 | [rows 154, 167–168](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L154) |

The #217609 number is especially unsafe to assign by number alone: Surf, Surface2011, and UnderDark2011 define different titles and exits there. Treat all eleven records as unresolved candidates until the map owners establish destination identity. They are not confirmed entrances into a live UnderDark2011 area.

## Typed references and code

The candidate ZON contains only its header, metadata, comments, and terminator; it has no reset commands ([ZON source](../../../areas/zon/underdark2011.zon#L1)). No active outside ZON has an exact reset-role reference to this candidate’s room or mobile records. The only numeric ZON matches are in unlisted surf.zon: 15 M-load records use mobile IDs also defined by the candidate, and 62 M commands use room VNUMs also present here. The source’s own Surf map duplicates those same IDs, and its mobile definitions differ, so these commands do not identify UnderDark2011 as their owner ([example shared mobile reset](../../../areas/zon/surf.zon#L39), [example shared room reset](../../../areas/zon/surf.zon#L20)).

Unlisted surface2011.qst has 13 giver headers whose numbers also occur in the candidate MOB range, plus five item-goal records using candidate OBJ numbers (#200003, #200004, #200005, #200006, #200007). Surface2011 defines its own different prototypes at those numbers. These are ambiguous same-number consumers, not a confirmed dependency on UnderDark2011 ([Surface2011 quest examples](../../../areas/qst/surface2011.qst#L160), [item goal](../../../areas/qst/surface2011.qst#L186)). No outside SHP product, structured story-sidecar VNUM, or type-25 object portal target into the candidate room set was found.

The exact C/C++ integer scan found no direct candidate-specific room/mobile/object binding. It did find generic range code: elfgate samples loaded world rooms but rejects room numbers below 110000 or at least 210000; its object assignments in specs.assign.c are inside a comment stating those object records do not exist ([range logic](../../../src/specs/specs.underworld.c#L1691), [unbound assignments](../../../src/specs/specs.assign.c#L1844)). The dormant spell_dark_compact body also contains a disabled 210000–214000 map range ([commented routine](../../../src/magic/spell_travel.c#L85)). These are not direct UnderDark2011 bindings. They leave a code-owner question about the generic teleport range if any part of this map is ever activated.

## Duplicate and replacement evidence

The exact room-number set for underdark2011 matches all 15,482 surface2011 room IDs and 15,481 of Surf’s 15,482 IDs. A full-record comparison found no identical room block between these three maps. All 68 candidate mobile IDs overlap both Surf and Surface2011 and have different full MOB records; all eight candidate OBJ IDs overlap Surface2011 and have different records. The normalized ZON title Norabi Map is shared by surface2011 and underdark2011. Checkpoints 534–535 record these distinct room and prototype identities; neither overlap nor common title establishes replacement ([identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-534-surf-surface2011-and-underdark2011-map-identities), [Surface2011 identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-535-surface2011-and-surf-map-and-story-identities)).

Followed history records the 2009 area-tree move and the August 2011 map-editor/renumber note for this source family. The MOB has a later 2016 Kuo-Toa race change; the OBJ and ZON have 2012/2015 field and quest-item-flag updates. The QST history includes a 2009 quest fix and the 2011 editor/renumber commit. None of the followed paths states a retirement or replacement decision. Active underdark remains separately registered and is not declared a successor here.

## Disposition and limits

**RETAIN IN PLACE.** Keep all five source files, their current IDs, the eleven unresolved active-source references, ten source-only exits to active areas, two Jungle links, and the alternate Surf/Surface2011 numeric identities. Do not activate, merge, rename, move, regenerate, or archive this component from this static review. Resolve map-editor ownership, reset and QST packaging, boundary destinations and returns, runtime range behavior, and persisted room/prototype identity with the relevant owners first.

This checkpoint used tracked source, current WLD registers, manifest membership, and followed Git history. It did not run tests, a build, server/runtime or gameplay, database work, migrations, or operational scripts. See the [builder questions](UNDERDARK2011_BUILDER_QUESTIONS.md), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-552-underdark2011-source-graph-and-boundaries), [identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-552-underdark2011-map-and-prototype-identity), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-552-underdark2011-retained-with-ambiguous-map-identities), [candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115), and [coverage row 465](ZONE_COVERAGE_REGISTER.csv#L466).

## Checkpoint 936: UnderDark2011 QST values retain their source-owned types

The sole `underdark2011.qst` giver header #200036 and its four item operands overlap Surf’s mobile-number range #200000–#200321. The header is the local QST giver key matching `underdark2011.mob`; the item operands are OBJ-typed and resolve to local prototypes in `underdark2011.obj`. The QST is absent from `areas/qst/AREA`. This numeric overlap adds no Surf QST dependency and does not change the existing RETAIN IN PLACE disposition. See the [Surf type-boundary review](SURF_AUDIT_REVIEW.md#checkpoint-936-old-surface-qst-values-do-not-identify-surf-mobiles) and the [Surface2011 reciprocal note](SURFACE2011_AUDIT_REVIEW.md#checkpoint-936-surf-range-qst-values-retain-their-source-owned-types).
