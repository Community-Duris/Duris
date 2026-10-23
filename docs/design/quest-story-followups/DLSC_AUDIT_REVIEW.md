# Domain of Lost Souls repository-wide audit supplement

This supplement reconciles the active Domain WLD and its boundaries, the all-file inactive-map scan, typed prototype dependencies, compiled references, and bounded title/history evidence with the [source-comprehensive Domain dossier](../zone-stories/DOMAIN_OF_LOST_SOULS.md). The frozen source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active graph and boundaries

The active [Domain WLD](../../../areas/wld/dlsc.wld#L1) has 100 rooms and 218 authored exits; all 218 resolve. There are 217 local exits and one outbound cross-area exit. The local graph is one weak component of 100 rooms. Raw directed traversal from entrance #36800 reaches 91 rooms and returns from all 100. The rooms outside raw root reach are #36883–#36891, the submerged pocket described in the dossier; the graph count does not qualify its item-based access or underwater safety.

The primary entrance is reciprocal with the Underdark: #36800 south → #784930 and #784930 north → #36800 ([Domain exit](../../../areas/wld/dlsc.wld#L14), [Underdark return](../../../areas/wld/underdark.wld#L123068), [outbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L118), [reverse pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L697)). Winterhaven has a separate one-way inbound exit: #55632 southwest → Domain room #36879 ([WH exit](../../../areas/wld/wh.wld#L13024), [inbound row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L789)). In total the active census records two inbound and one outbound cross-area exit, with one reciprocal pair. There are no unresolved or negative WLD targets, falls, or current fields.

## Inactive-map sweep

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parser errors. It found no source-room VNUM overlap with Domain rooms. It found two distinct source-only approaches to entrance #36800: unregistered `ud_map1.wld` #1117360 east → #36800 ([exit](../../../areas/wld/ud_map1.wld#L39568)) and `ud_map2.wld` #1127382 down → #36800 ([exit](../../../areas/wld/ud_map2.wld#L94634)). These are not active ingress. They add two to the curated source-only total, now 2,798 overall; the reviewed Surf subset remains 156. Existing active Shafts exits into `ud_map1` and `ud_map2` remain separate unresolved candidate references.

## Typed dependencies and compiled references

Domain's active reset imports are explicit: the magic fountain #72 and rune-covered stone #359 are Heavens objects; robe #67235 is defined in `unique.obj`; tormented spirit #55272, memory #55179, and Euronymous femur #55281 are Winterhaven objects. The [Domain reset rows for #55272](../../../areas/zon/dlsc.zon#L147), [#72](../../../areas/zon/dlsc.zon#L160), and [#67235/#359/#55179/#55281](../../../areas/zon/dlsc.zon#L503) point to owning [Heavens fountain #72](../../../areas/obj/heavens.obj#L1000), [Heavens stone #359](../../../areas/obj/heavens.obj#L3725), [robe #67235](../../../areas/obj/unique.obj#L510), [spirit #55272](../../../areas/obj/wh.obj#L3388), [memory #55179](../../../areas/obj/wh.obj#L2310), and [femur #55281](../../../areas/obj/wh.obj#L3519) definitions. Domain's own Q47 consumes three Winterhaven skulls #55037 and returns Winterhaven sheath #55038 ([quest commands](../../../areas/qst/dlsc.qst#L52), [skull](../../../areas/obj/wh.obj#L396), [sheath](../../../areas/obj/wh.obj#L404)); these prototypes remain owned by Winterhaven.

No active outside QST item command, local SHP source/shop, active foreign SHP product, active foreign ZON reset, outside story-sidecar VNUM, external mobile/object prototype definition, or outside type-25 portal target into a Domain prototype or room was found. The scoped prototype scan found no external mobile or object definition overlapping Domain's local sets; none of 870 outside type-25 declarations targets a Domain room. All 231 outside story sidecars parsed without errors or exact token hits.

An all-file reset-role scan does find one source-only dependency: unregistered `nexus56.zon` line 1374 issues a `G` reset for Domain book #36822 ([reset](../../../areas/zon/nexus56.zon#L1374), [book definition](../../../areas/obj/dlsc.obj#L350)). The same unregistered source has shop #83038 listing #36822 at line 497 ([shop row](../../../areas/shp/nexus56.shp#L497)). `nexus56` is absent from the current [world manifest](../../../areas/AREA#L1) and [QST manifest](../../../areas/qst/AREA#L1); it is not an active reset or selected stock row. This checkout has no `areas/world.shp`; `open_shop_stream()` can fall back to scanning `areas/shp/*.shp` ([shop loader](../../../src/economy/shop.c#L3267)), so the record is conditional parser exposure, not proof of an operating shop. Preserve both records while `nexus56` remains **RETAIN**.

Several compiled tables also refer to Domain prototypes. CHAOS starter profiles include shark ring #36883 for Bard ([standard profile](../../../src/account/chaos_eq_data.h#L200), [enhanceable profile](../../../src/account/chaos_eq_data.h#L446)); Ihsahn's braid #36891 for Psionicist, Cleric, Druid, and Blighter ([Psionicist](../../../src/account/chaos_eq_data.h#L93), [Cleric](../../../src/account/chaos_eq_data.h#L119), [Druid](../../../src/account/chaos_eq_data.h#L136), [Blighter](../../../src/account/chaos_eq_data.h#L271)) and also Shaman in the enhanceable profile ([Shaman](../../../src/account/chaos_eq_data.h#L394)); Skull Splitter #36894 for Mercenary ([standard](../../../src/account/chaos_eq_data.h#L196), [enhanceable](../../../src/account/chaos_eq_data.h#L442)); and Erebus cloak #36896 for Ethermancer ([standard](../../../src/account/chaos_eq_data.h#L284), [enhanceable](../../../src/account/chaos_eq_data.h#L528)). The new-character kit path reads the configured VNUMs through `read_object()` ([item loader](../../../src/account/nanny.c#L634), [profile application](../../../src/account/nanny.c#L709)); this is a source-level issuance dependency, not evidence that a particular character received an item.

The effective epic teacher table maps mobile #36849 (Ihsahn) to `SKILL_TWOWEAPON` ([teacher row](../../../src/classes/epic_skills.c#L183)); startup's `epic_initialization()` binds `epic_teacher` to mobiles in that table ([binding loop](../../../src/world/epic.c#L1368)). Object #36884 is assigned `kvasir_dagger`, and object #36894 is assigned `critical_attack_proc` ([special bindings](../../../src/specs/specs.assign.c#L1950), [proc binding](../../../src/specs/specs.assign.c#L2588)). High-drop table value #36856 identifies mobile Kyeril, not the same-numbered local contract object; the high-drop consumer loop is commented out ([mobile table](../../../src/item/randomeq.c#L125), [disabled consumer](../../../src/item/randomeq.c#L650)). These source references explain why raw numeric hits must retain prototype type and effective-binding context.

## Title, history, and disposition

After removing color tokens and normalizing whitespace/case across all 442
current `.zon` title lines, “Domain of Lost Souls” occurs once under `dlsc`
([title source](../../../areas/zon/dlsc.zon#L2)). The lexical peers “Grumbar's
Domain” under `earthp` and “Pragtog's Domain” under `pragtog` are separate
titles and source owners, not alternate names for this area
([Grumbar title](../../../areas/zon/earthp.zon#L2),
[Pragtog title](../../../areas/zon/pragtog.zon#L2)). Followed WLD, MOB, OBJ,
ZON, and QST history remains on the `dlsc` stem from the 2009 area-tree move;
there is no local SHP file. The story sidecar was added on 2026-10-06. No
alternate-stem rename, explicit predecessor, retirement, or whole-zone
replacement was established in the reviewed history.

`dlsc` remains an un-commented contributor in both [the world manifest](../../../areas/AREA#L178) and [QST manifest](../../../areas/qst/AREA#L165); keep it **DO NOT MOVE**. Keep the separate `nexus56` source group **RETAIN** while its unregistered reset/shop references and remaining owner/tooling closure stay open. See [coverage row 77](ZONE_COVERAGE_REGISTER.csv#L77), [Nexus56 candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79), [`ud_map1` candidate row 113](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L113), [`ud_map2` candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), [connectivity checkpoint](WORLD_CONNECTIVITY.md), [history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md). Static source review does not claim played qualification or establish the availability of gated routes, shops, or reset stock.

## Checkpoint 788: post-sidecar typed closure refresh

The current source still has 452 ZON commands and 438 M/G/E/O/P prototype fields: 429
resolve locally, and nine are the active external reset imports already mapped above.
Local M/O placement rooms and D room references resolve within Domain. Its two QST
contracts contain seven item fields: three local, three Winterhaven skull inputs #55037,
and Winterhaven sheath #55038 as the fourth external item field. There is no local SHP
file.

The story sidecar added on 2026-10-06 contains 24 typed VNUM fields (17 mob, four giver,
three item); all typed targets are local except one item_vnums reference to Winterhaven
skull #55037 ([field](../../../areas/story/dlsc.story.json#L212)). Its four
completion_key strings repeat the two native QST recipes at both contract and step
levels; the Q47 signature repeats three #55037 inputs and #55038 output in each copy
([first signature](../../../areas/story/dlsc.story.json#L204), [step
copy](../../../areas/story/dlsc.story.json#L226)). These duplicate records do not add
contracts or distinct prototype identities.

The full active and source-only scan found no external active QST item/giver, SHP
product, ZON prototype or room reset, typed sidecar VNUM, duplicate mobile/object
definition, or type-25/type-29 destination into a Domain prototype or room. The 231
other story sidecars parsed without errors. It covered 870 external type-25 and 378
external type-29 declarations; none targets Domain. Local travel remains two type-29
switches (#36800 to #36801 and #36801 to #36802) and two type-25 teleports (#36826 to
#36801 and #36842 to #36883), all within the area.

The only source-only consumers remain unregistered Nexus56's G reset and shop product
#36822; preserve the existing candidate row 79 RETAIN disposition. No candidate row or
disposition, WLD total, native gameplay, area source, or the dossier's twenty owner
questions changed. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L77) and the
[Nexus56 candidate record](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).
