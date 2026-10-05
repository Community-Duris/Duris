# The Basin Wastes: comprehensive source map

Priority83 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The [journal](../../../areas/story/basin_wa.story.json) uses
schema3/revision1: seven cards bind all ten native recipes. Four potion crafts
and the larger signet ritual are distinct achievements. Four equal part sales
form one outcome, retaining all four native receipts. The heartstone refusal is
a guidance service with no achievement/daily. Eight contacts retain all fourteen
addressed aliases; fourteen optional checks separate thirteen current materials
from one earlier potion receipt. Discovery has its own achievement. All new
tracking requires **active, ready accounting**; frozen recovery stays separate.

**No native zone or quest repair ships.** Dispatch choices, source admission,
learned book clues and renewal need the explicit plans below. No recipe, reset,
blocker, item flag, TAKE, charge or mobility changes. Actual repairs require a
separate named fix, original-fails/repaired-passes proof and prominent PR/news.
The [Fields legacy hotfix follow-up](FIELDS_BETWEEN.md#builder-required-follow-up-replace-the-legacy-rift-hotfix-safely)
remains required builder design; mapping must preserve that protection.

## Complete local and bounded foreign closure

Active `basin_wa`, zone340, registry33765–34199, physically34000–34199. Native
header `34199 2 0 0 10 1` uses reset mode2. Header levels0–10 are metadata, not
safe-level advice: native creatures include level61 spider and level62 dragon.

- [All16 native blocks](../../../areas/qst/basin_wa.qst): M6/Q10, including full
  response bodies, fourteen aliases, exact inputs/rewards and D0 on every recipe.
  No MA/QA, coin-input, item-type input or oversized bundle occurs.
- [All200 rooms](../../../areas/wld/basin_wa.wld):91 complete title/prose
  families/eight headers/692 exact exits/69 relative exit patterns/78 complete
  exit-description/keyword families/one S metadata family. Every repeated
  membership, direction, flag, key and destination was reviewed. All exits have
  raw flag/key zero. No room E/F/T or ROOM_INN record was identified. Cliffs,
  wastes, swamp, two oasis contexts and the deliberately false-city cave differ.
- [All15 mobiles](../../../areas/mob/basin_wa.mob): every full description,
  keyword, header, class/race, level, money and remaining numeric body reviewed.
  Part-bearing reset instances differ from creatures of the same kind at the
  pool. The witch has ACT_TEACHER and gets the computed shared `teacher` behavior
  if no explicit procedure exists; this is not an epic-teacher assignment.
- [All32 local objects](../../../areas/obj/basin_wa.obj): all types, flags,
  values, E descriptions and affects. Six fixed type15 corpse containers start
  open (`[20,0,-1,100]`, wear0); their six books are hidden type12 objects. No
  key/type18, teleport/type25, switch/type29 or object T record occurs. Part
  prototypes include armor/weapon/light; dark elixir and four craft outputs are
  potions. Signet/heartstone/ring are type11, scale type8, witch staff type4.
  The pool is fixed type17 with liquid17 (holy water). QUESTITEM in extra2
  differs from NOLOCATE in extra flags; no capability is inferred from names.
- [All204 reset commands](../../../areas/zon/basin_wa.zon):151 exact and191
  M-parent-aware families,83 location-expanded groups; O/P/M/E/G only. All seven
  floor sources, P containment, mob-parent equipment/inventory and cap/chance
  fields reviewed. Two Milton potion P rolls each have chance20/cap2. The sole
  imported prototype is [Sakaslan’s skull67240](../../../areas/obj/unique.obj#L605).
  Its complete armor prototype/prose/affects and sole active local G source were
  reviewed; no touching native recipe or literal procedure was found for it.
- [Literal spider assignment](../../../src/specs/specs.assign.c#L2185) invokes
  the full shared [block_dir](../../../src/specs/specs.exit_barriers.c#L24): its
  `_block_east_` keyword blocks EAST for ordinary players, while trusted staff
  bypass. EAST34199→34198 is the dead end’s sole exit. This is an exit barrier,
  not an unblock-on-death quest or a newly mapped personal combat achievement.
- The global bounded scan covers native input/reward references to all32 local
  objects plus skull67240, foreign resets touching local objects/mobiles and
  all713 active teleport prototypes. All ten touching recipes are local, with
  no foreign reset source of a local ingredient and no identified incoming
  portal. The sole ordinary boundary is UP34000→612059, with reciprocal
  DOWN612059→34000 in [the surface](../../../areas/wld/surface.wld). Its full
  Khomani-Khan depression room was reviewed. These scans do not prove absence
  of spell/staff/saved-world/procedural/dynamic routes elsewhere.
- Full shared native/durable quest admission, reverse loaded recipe order,
  exact loose actor-root selection, same-kind reward replacement, source/reset
  admission, SEARCH container and READ→LOOK behavior were reviewed. Computed
  teacher registration/ASK level and the full epic-teacher table were checked;
  no local epic-teacher binding or shop file exists. Generic teaching, ordinary
  combat, light/staff/potion/water effects do not become terminal quest receipts.

The [generated audit](../../reference/zone-story-audits/basin_wa.md) preserves
every exact native binding, response and reset. Generated inventories supplement
the full source review; synthetic receipt projection is not played proof.

## Progression stories and exact sources

| Card | Exact native transaction | Guidance and boundaries |
| --- | --- | --- |
| Signet ritual | [Q35](../../../areas/qst/basin_wa.qst#L35): scale34031 + red potion34019 + signet34018 → white-gold ring34028 + E500000 | Scale carried by Sakaslan34110; signet inside Aberden’s fixed corpse34150; potion from gland craft. Supplied matching inputs bypass personal earlier craft/kill/reading. |
| Red potion | [Q49](../../../areas/qst/basin_wa.qst#L49): elixir34030 + gland34002 →34019 | Give the elixir, with the intended gland loose; red output feeds ritual but drinking consumes it. |
| Brown potion | [Q56](../../../areas/qst/basin_wa.qst#L56): elixir34030 + mandible34001 →34025 | Distinct reward, not another color of the same accepted result. |
| White potion | [Q63](../../../areas/qst/basin_wa.qst#L63): elixir34030 + shell34000 →34026 | Distinct reward; consumes the same shell kind accepted for cash. |
| Spotted potion | [Q70](../../../areas/qst/basin_wa.qst#L70): elixir34030 + horn34003 →34027 | Distinct reward; neither CARVE nor a personal beetle kill is enforced. |
| Part payment | [Q77/82/87/92](../../../areas/qst/basin_wa.qst#L77): any one exact shell/mandible/gland/horn → C25000 | One achievement/daily candidate across four branches. Give the part to select sale; all canonical receipts remain intact. |
| Heartstone refusal | [Q97](../../../areas/qst/basin_wa.qst#L97): heartstone34024 → same kind34024 | Guidance service only. Native consumption/reward does not preserve the original UID. Keeping the stone is enough; refusal is no ring prerequisite. |

The **cap-one dark elixir** starts carried by the minotaur lizard34008 at34052,
immediately before the witch’s M reset. Other minotaurs do not carry it; the
witch starts equipped only with staff34029. Glands are E-held by the ten fire
beetles at34185–34194, not by the pool’s fire beetle34088. Shells, mandibles and
horns are equipped by the corresponding bombardier/slicer/rhino instances.
Pool concentration and mere prototype names are insufficient producer evidence.
Fresh source/root/custody and actual pickup require accounting qualification.

**Native dispatch is sensitive to the offered item.** `boot_the_quests` prepends
Q blocks, so current testing order is heartstone refusal, horn/gland/mandible/
shell sales, spotted/white/brown/red crafts, then ring ritual. Durable offering
selection first requires the offered item to match a recipe and then gathers
its full supported bundle from the actor’s loose inventory. Giving a beetle
part therefore selects its cash recipe even when elixir is also carried.
Giving the elixir avoids those sales, but multiple carried part kinds can
select another craft (horn, shell, mandible, then gland precedence). Guidance
asks for only the intended part kind loose; it does not alter shared dispatch.
Legacy inactive-accounting mob inventory accumulation is separate and grants
no new journal credit. Actual offer, consumption, reward and retry behavior
still need a played, accounting-active journey.

### The expedition books are clues, not fabricated quest endpoints

| Fixed container / room | Hidden book / additional declared content | Progression meaning |
| --- | --- | --- |
| Michith34014 /34036 | Book34008; greatsword34022 | Room titled Oasis holds the corpse, not the fixed pool source. |
| Cyril34015 /34065 | Book34009; dress34021 | Escape and doomed expedition clue. Dress’s canteen E description is a copy mismatch. |
| Tomak34011 /34082 | Book34005 | Broken-leg/abandonment clue; no rescue controller. |
| Gordas34012 /34113 | Book34006; kilt34020 | Expedition betrayal context; no new accepted receipt. |
| Aberden34013 /34150 | Book34007; signet34018 | Actual signet source; witch’s unidentified-mage conjecture does not require a kill. |
| Milton34016 /34199 | Book34010; two chance20 silvery potion34017 rolls | Book has passage1/2/3, describing the failed Crystal City search; spider blocks the only ordinary return exit. |

These are fixed open containers, not newly killed corpse objects or locked key
gates. SEARCH a named container reveals hidden contents subject to find chance,
visibility and current presence. READ delegates to LOOK and displays E prose;
it does not emit a learned-word/story event. A collected book is not proof
that its passage was read. The final room explicitly says no door, magic portal,
runes or Crystal City onward route. Preserve that authored false lead and
the spider’s existing barrier until a builder deliberately chooses otherwise.

Sakaslan’s scale, heartstone and imported armor skull are three different kinds.
The heartstone’s `_noquest_` alias affects random world-quest reward selection;
it does not remove the static local Q97 refusal. The white-gold reward has
actual finger wear despite doubtful-wear prose. Mandible headwear prose disagrees
with its weapon wear mask. These are fair clue-review findings, not authority
to enable wear, change quest proof, destroy an item or add new loot.

## Builder work and universal capability additions

| Follow-up | Current evidence / implemented guidance | Required design and qualification |
| --- | --- | --- |
| **ZSQ-BASIN-DISPATCH-CHOICE** | Giving a part selects its equal cash sale; giving elixir selects crafting with reverse-order precedence among supplied parts | Builder chooses explicit offer guidance or deliberate selection UI. Preserve existing recipe identities and shared game behavior. If dispatch changes, scope it explicitly and isolate a fix; test each trigger, all partial/excess bundles, multi-part ambiguity, duplicate/nested/worn roots, concurrent offers, replay, rollback and frozen recovery. |
| **ZSQ-BASIN-LEARNED-BOOKS** | Six hidden books, separate corpse parents, three Milton passages, READ→LOOK with no durable learned milestone | Admit the actual reader, selected object/UID/root, exact E keyword/passages, text/content revision and successfully delivered response. Distinguish gift, custody, collection and reading. Design optional all-book investigation without duplicate terminal credit; handle missing/ambiguous books, visibility/search, replay, content updates and cold recovery. |
| **ZSQ-BASIN-SOURCE-RENEWAL** | One elixir serves four crafts; scale/signet cap1; mode2 reset declarations and live accounting source admission differ | Admit source generation, exact parent, successful transfer/reward lineage and donor policy. Qualify scarce supply, reset issuance, stock already held elsewhere, dead/absent actors, daily boundaries and concurrent claims. Preserve scarcity/reset mode; six static daily candidates are not a guaranteed renewable promise. |
| **ZSQ-BASIN-HEARTSTONE-REFUSAL** | Keep-it prose is implemented as consume/reward same kind, not same UID; service gives no credit | Builder decides retaining existing replacement semantics versus real no-consumption refusal. Review identity/enhancements/custody, guarded admission, rollback/retries and historical receipt compatibility before an isolated repair. Do not use the refusal to duplicate inventory or farm credit. |
| **ZSQ-BASIN-CLUE-CONSISTENCY** | Surface UP/DOWN boundary differs from city/Nizari prose; pool placement differs from Oasis title; dress has canteen prose; wear descriptions mismatch | Builder chooses precise caption/E-keyword edits after checking intended lore. Prefer clue-only clarification; no movement, wear-mask, object type, producer or reward change from prose alone. Any actual repair gets exact before/after regression and separate fix/news. |
| **ZSQ-BASIN-CAVE-ACCESS** | False-city dead end and spider EAST barrier are real; no exit key/portal/controller onward | Future personal access/combat stages need admitted actor, movement attempt/result, exact blocker lifecycle and committed arrival. Staff bypass or another player’s combat must not count as personal clearance. Do not create a Crystal City door or remove a deliberate trap to make the journal look complete. |

Presentation shows seven cards, clear exact ingredients and missing/ready loose
copies, an optional earlier red-potion receipt and recorded accepted outcomes.
Show the craft-versus-sale trigger in each potion card, not only in documentation.
Keep the refusal visibly separate from six achievements. Later learned-clue,
first-source, gift, access and investigation views need qualified committed
events before they can become progression checks or ANSI/GMCP milestones.

## Validation and practical limits

Focused source/schema regression closes all local sets, exact bindings, material
types/parents, alternative grouping, shared dispatch/teacher/blocker, boundary
and imported skull context. Python/C++ file-loader/projection journeys verify
wrong/worn/duplicate ingredients, supplied ritual proof without earlier craft,
four independent potion outcomes, any-one sale grouping, zero-credit refusal,
spent-proof versus historical receipts, ten retained canonical receipts, exact
replay and cold recovery without invented surface discovery. Full production
regression, maintained build, formatting, links and publication preservation
remain checkpoint checks. Prior100 journals/all2668 native definitions/content
revision2/fingerprint/registry and original220 queue must remain unchanged.

Catalog after this checkpoint:101 journals/1585 achievements/1441 potential
dailies/2195 rows. Grouping four sales removes three fallback achievement/daily
units; refusal additionally removes one fallback achievement. **83/220 areas
source-comprehensive,137 pending; Nakral’s Crypt (`crypt`) next.** The full goal
remains active. Synthetic receipts do not qualify played first recovery, gifts,
SEARCH/READ/GET, native dispatch, consumption/reward settlement, same-kind
replacement identity, combat, access, teacher effects, database persistence or
renewal. No accounting activation, DB/server operation, migration, deployment
or merge is part of this checkpoint.
