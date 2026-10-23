# Du'Maathe Castle: boundary and typed-reference supplement

Status: repository-wide static supplement to the existing
[Du'Maathe Castle dossier](../zone-stories/DU_MAATHE_CASTLE.md). It reconciles
active WLD boundaries, current unregistered WLD inputs, bounded external typed
references, and the active-title/history boundary. No zone data, story sidecar,
or gameplay code was changed or qualified.

## Active and source-only WLD boundaries

The active area entry is `mntcastl *371-376`; the canonical zone header is
`#371`. The existing dossier records 1,224 authored and loader-resolving
exits, one local weak component spanning all 506 rooms, no unresolved exits,
falls, or current fields, and two active reciprocal boundaries:

| Active boundary | Evidence |
| --- | --- |
| Castle room 37416 west ↔ Surface room 563532 east | [Castle exit](../../../areas/wld/mntcastl.wld#L7215), [Surface exit](../../../areas/wld/surface.wld#L1333667) |
| Castle room 37574 south ↔ Dracopit room 15968 north | [Castle exit](../../../areas/wld/mntcastl.wld#L11035), [Dracopit exit](../../../areas/wld/dracopit.wld#L2839) |

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored D0–D9 exits with zero parse issues. Five source-only exits target
active castle rooms:

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| `oldmap8.wld` room 188135 | West (`D3`) to castle room 37409 | [exit target](../../../areas/wld/oldmap8.wld#L170540) |
| `oldmap8.wld` room 188234 | North (`D0`) to castle room 37416 | [exit target](../../../areas/wld/oldmap8.wld#L172603) |
| `surf.wld` room 224560 | South (`D2`) to castle room 37425 | [exit target](../../../areas/wld/surf.wld#L515235) |
| `surf.wld` room 224861 | West (`D3`) to castle room 37409 | [exit target](../../../areas/wld/surf.wld#L521540) |
| `surf.wld` room 225160 | North (`D0`) to castle room 37416 | [exit target](../../../areas/wld/surf.wld#L527787) |

These records are in unregistered WLD source, not the active area input. They
are not active-world entrances and do not change the active 1,224-exit census
or establish ordinary access, safe arrival, or a return path. They increase
the curated source-only record count from 370 to 375.

## Direct external typed references

The existing dossier maps the active Alatorin totem consumer, the foreign
Surface-mini frail-man contact, Carmotee's epic-teacher binding, and the
absence of active foreign reset consumers. This exact scan confirms those
references and adds two source-only Brass reset records:

| Current source | Castle prototype and source role | Evidence |
| --- | --- | --- |
| Active Alatorin quest | Consumes local Gruneegth totem 37117 for skull-belt reward 83495 | [QST exchange](../../../areas/qst/alatorin.qst#L6327), [story mapping](../../../areas/story/alatorin.story.json#L3540), [totem](../../../areas/obj/mntcastl.obj#L249) |
| Active Surface-mini story sidecar | Maps castle mobile 37102, the frail man, as a foreign contact for the granular-potion route | [contact](../../../areas/story/surfacemini.story.json#L179), [mobile](../../../areas/mob/mntcastl.mob#L43) |
| Epic-teacher code | The table maps Carmotee mobile 37145 to `SKILL_SMELT`; the shared epic initialization binds the teacher procedure | [teacher table](../../../src/classes/epic_skills.c#L201), [binding loop](../../../src/world/epic.c#L308), [Carmotee](../../../areas/mob/mntcastl.mob#L742) |
| Unregistered Brass reset | `brass-old-1.zon` has `G` resets for black ring runes 37142 and copper earring 37144 | [ring reset](../../../areas/zon/brass-old-1.zon#L551), [earring reset](../../../areas/zon/brass-old-1.zon#L535), [ring](../../../areas/obj/mntcastl.obj#L630), [earring](../../../areas/obj/mntcastl.obj#L665) |

The Brass reset rows are in a current unregistered source group; they are not
active reset consumers or proof of current stock. The bounded active MOB, OBJ,
ZON, SHP, QST, story-sidecar, and C/H exact-token scans found no additional
direct references to the castle's mobile or object prototypes. Numeric values
in unrelated object affect/value records were not treated as prototype links.

## Active title and tracked history boundary

The normalized scan of all 442 current `.zon` titles finds one exact
“Du'Maathe Castle” owner: active area `mntcastl`, zone 371 ([active title](../../../areas/zon/mntcastl.zon#L2)).
Followed WLD/ZON history stays on the current `mntcastl` paths and names no
distinct predecessor or whole-zone replacement.

The five unregistered `oldmap8.wld` and `surf.wld` records listed above are
distinct source rooms #188135, #188234, #224560, #224861, and #225160, all
outside the Castle's *371–376 range. Their titles describe a stone bridge, a
path through the DragonClaws, Cheltenham Fields, and a slow-moving river; each
has an empty room-description field. They point only to three active boundary
rooms: #37409 “A Slow Flowing River,” #37416 “A Little Used Dirt Road,” and
#37425 “Chiltern Way.” Neither source WLD contains the literal `Du'Maathe` or
`mntcastl`. These are inactive approaches to active boundary rooms, not an
alternate Castle interior or a distinct predecessor map. This closes the
bounded semantic comparison for the five flagged records; it does not make
those exits active or establish safe travel.

[Active title](../../../areas/zon/mntcastl.zon#L2) ·
[active-area entry](../../../areas/AREA#L180) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source findings. The unregistered WLD and Brass reset
references do not establish a current route or item source. Active boundaries,
Alatorin acceptance and settlement, the Surface-mini story contact, and the
epic-teacher invocation remain separate source/runtime facts. Editor and
tooling references, persisted-player state, and indirect or dynamic consumers
remain outside this bounded closure.


## Checkpoint 286: all-source title and bounded typed closure

After stripping Duris color tokens and the title terminator, then normalizing
case and whitespace, “Du'Maathe Castle” has one owner among all 442 current
.zon files: active mntcastl. The existing followed WLD/ZON history remains
same-stem and names no distinct predecessor or replacement. The row's broader
semantic source comparison remains open; this checkpoint does not infer a
lineage from the castle name or its source-only approaches.

The active typed pass is complete for the bounded direct source categories
reviewed. It includes Alatorin's exact totem item exchange, Surface-mini's
structured frail-man contact, Carmotee's epic-teacher table binding, and the
two source-only Brass G resets for objects #37142 and #37144. No other direct
outside QST/story-sidecar item reference, active outside reset consumer, SHP
product, duplicate prototype, or type-25 target was found in the scanned
sources. The local ZON imports of Verspin amethyst #28146 and castle memory
#55171 remain documented in the existing dossier. The Brass resets are not
active sources. Editor/tooling ownership,
indirect runtime lookups, and persisted references remain outside this
bounded source result.

## Checkpoint 286: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should the four lord offerings remain independent outcomes rather than a required horn → potion → tooth → gem chain? | [Four lord contracts](../zone-stories/DU_MAATHE_CASTLE.md#exact-accepted-exchanges-and-progression), [QST source](../../../areas/qst/mntcastl.qst) | Preserve the exact accepted exchanges and supplied-item paths; add campaign ordering only with explicit authored stages and durable predicates. |
| 2 | Should each lord continue accepting its one exact item kind, with supplied matching items valid regardless of personal kills or source order? | [Exact terms](../zone-stories/DU_MAATHE_CASTLE.md#exact-accepted-exchanges-and-progression), [lord QST records](../../../areas/qst/mntcastl.qst#L70) | Keep exact prototype admission and independent receipts; no generic horn, gem, or tooth substitute. |
| 3 | Should the blue-tinged horn #37103 remain accepted even though no active exact producer is declared? | [Horn exchange](../../../areas/qst/mntcastl.qst#L70), [missing-source review](../zone-stories/DU_MAATHE_CASTLE.md#capability-gaps-and-fair-repair-proposals) | Keep supplied exact stock eligible; choose any new source in a separate builder-approved balance repair with its own origin and recovery evidence. |
| 4 | Should granular preparation continue requiring both fine sand #37106 and recipe #97903, consuming both and producing three potions #37105? | [Preparation QST](../../../areas/qst/mntcastl.qst#L237), [batch lineage review](../zone-stories/DU_MAATHE_CASTLE.md#capability-gaps-and-fair-repair-proposals) | Preserve both inputs and indexed output identities as one service transaction; one input or one potion does not complete the batch. |
| 5 | Should the granular and frost clothing quotes remain separate from the accepted copper fees of 1,000 and 500? | [Hermit exchange](../../../areas/qst/surfacemini.qst#L33), [fee discrepancy](../zone-stories/DU_MAATHE_CASTLE.md#capability-gaps-and-fair-repair-proposals) | Confirm intended pricing and repair dialogue or native fee in a separate named change; keep current item paths valid while accounting blocks payment. |
| 6 | Should frost potion #37109 continue requiring pure white horn #37108, distinct from blue horn #37103 and blackened horn #37113? | [Frost service](../../../areas/qst/mntcastl.qst#L255), [horn source review](../zone-stories/DU_MAATHE_CASTLE.md#exact-accepted-exchanges-and-progression) | Preserve exact horn identity and hidden crystal-dragon source; name and general dragon origin do not substitute. |
| 7 | Should protective preparation continue consuming golem rod #37110, while shop wand #37140 remains a distinct unrelated product? | [Protective service](../../../areas/qst/mntcastl.qst#L272), [shop source](../../../areas/shp/mntcastl.shp#L1), [typed object review](../zone-stories/DU_MAATHE_CASTLE.md#complete-source-closure) | Keep the source and shop item types distinct; do not turn wand inventory or a golem kill into the accepted rod. |
| 8 | Should home preparation keep its exact small white flower #37121 and warning about side effects, without promising a successful return journey? | [Home service](../../../areas/qst/mntcastl.qst#L286), [flower reset](../../../areas/zon/mntcastl.zon#L59) | Preserve the exact item and authored warning; qualify actual effect, movement, and survival separately from preparation. |
| 9 | Should the four potion preparations remain services rather than lord-completion receipts or guaranteed daily supplies? | [Sidecar classifications](../../../areas/story/mntcastl.story.json), [zone reset header](../../../areas/zon/mntcastl.zon#L3) | Keep preparation, source generation, lord acceptance, and renewal separate; do not promise stock from reset mode or daily metadata. |
| 10 | Should source recovery, SEARCH, transfer, and current loose-item custody remain optional history for the native item-only lord exchanges? | [Native custody rules](../zone-stories/DU_MAATHE_CASTLE.md#exact-accepted-exchanges-and-progression), [source and search review](../zone-stories/DU_MAATHE_CASTLE.md#capability-gaps-and-fair-repair-proposals) | Preserve supplied-item eligibility; track a personal source path only as a separate actor/source-generation contract. |
| 11 | Should the lich gem exchange return golden key #37100 without requiring the optional onyx-key staircase or a prescribed previous lord receipt? | [Gem exchange](../../../areas/qst/mntcastl.qst#L137), [key access review](../zone-stories/DU_MAATHE_CASTLE.md#loaded-access-keys-and-services) | Preserve the separate gem receipt and exact key reward; keep the alternative access paths optional. |
| 12 | Should the tooth exchange remain an independent source of tooth key #37115 for the foreign Pit of Dragons gate, where ordinary successful unlock consumes the key? | [Tooth exchange](../../../areas/qst/mntcastl.qst#L105), [foreign gate](../zone-stories/DU_MAATHE_CASTLE.md#loaded-access-keys-and-services) | Record accepted tooth delivery, key custody, unlock, break, and foreign arrival separately; do not make a one-time item proof of travel. |
| 13 | Should asymmetric prison, catacomb, and keyed-door reset states remain as authored until the owner selects a visibility and return policy? | [Castle access map](../zone-stories/DU_MAATHE_CASTLE.md#loaded-access-keys-and-services), [door resets](../../../areas/zon/mntcastl.zon) | Preserve reciprocal flag and key behavior while qualifying actual SEARCH, OPEN, UNLOCK, arrival, and return; do not repair based on prose alone. |
| 14 | Should the waterfall portal stay visible and open because the loader masks raw state four and no D reset closes it? | [Waterfall interpretation](../zone-stories/DU_MAATHE_CASTLE.md#loaded-access-keys-and-services), [room source](../../../areas/wld/mntcastl.wld#L453) | Confirm whether the hidden-opening prose is atmospheric or intended mechanics before a reset or visibility change; do not add an assumed SEARCH prerequisite. |
| 15 | Should described underwater passages retain indoor sector data and shared movement rules rather than gaining an inferred swim prerequisite? | [Water route review](../zone-stories/DU_MAATHE_CASTLE.md#loaded-access-keys-and-services), [passage source](../../../areas/wld/mntcastl.wld) | Preserve the current sector declarations until the builder chooses explicit mechanics; qualify admitted movement and safe return separately. |
| 16 | Should the two active reciprocal boundaries and five inactive Surf/oldmap8 approaches remain separate route facts? | [Active and source-only boundaries](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries), [candidate records](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv) | Keep active Surface and Pit pairs distinct from source-only records; no candidate exit is current ingress or proof of safe travel. |
| 17 | Should T'Sharenoa's wand shop and both literal inn bindings remain service contexts separate from lord exchanges and room-in semantics? | [Shop and inn source map](../zone-stories/DU_MAATHE_CASTLE.md#complete-source-closure), [assignment index](../../reference/zone-story-audits/mntcastl.md#literal-special-assignments), [shop source](../../../areas/shp/mntcastl.shp#L1) | Preserve exact keeper, room, and product bindings; credit only supported trade or successfully saved RENT outcomes. |
| 18 | Should the active Alatorin totem consumer and Surface-mini contact stay owned by their recipient stories instead of becoming hidden Castle prerequisites? | [External references](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#direct-external-typed-references), [Alatorin QST](../../../areas/qst/alatorin.qst#L6327), [Surface-mini sidecar](../../../areas/story/surfacemini.story.json#L179) | Preserve typed external identities and recipient ownership; separate foreign acceptance and contact history from local Castle receipts. |
| 19 | Should the dragon prophecy and revenge narrative remain incomplete lore until an explicit Empress or campaign endpoint is authored? | [Campaign limits](../zone-stories/DU_MAATHE_CASTLE.md#capability-gaps-and-fair-repair-proposals), [lord dialogue](../../../areas/qst/mntcastl.qst#L41) | Keep the four lord outcomes complete as authored; require named actors, victory rules, state changes, beneficiaries, and recovery before adding campaign progress. |
| 20 | Are Brass G resets for local black-ring runes #37142 and copper earring #37144 intentional source-only item origins, given that their reset file is unregistered? | [Brass source references](DU_MAATHE_CASTLE_AUDIT_REVIEW.md#direct-external-typed-references), [candidate plan](ATTIC_MIGRATION_PLAN.md) | Retain the typed dependency as inactive source evidence; do not count current supply or activate the reset without its source owner and loader path being established. |
