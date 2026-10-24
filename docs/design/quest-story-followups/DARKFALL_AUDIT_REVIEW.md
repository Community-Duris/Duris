# Darkfall Forest: connectivity, dependency, and history supplement

**Checkpoint 24.** This supplement reconciles source-only routes, typed
dependencies, and path history for the existing
[Darkfall story dossier](../zone-stories/DARKFALL_FOREST.md). It is a
follow-up to an existing source-comprehensive dossier, not a new whole-zone
story review. The 22 new story-review count and the 109 pending active-key
count remain unchanged. Source membership, routes, native quests, and runtime
behavior were not changed.

## Active and source-only connectivity

Darkfall remains an active full-area contributor at zone 953. The active WLD
boundary has three incoming and three outgoing records, with a reverse record
for each:

| Active pair | Source evidence | Reading |
| --- | --- | --- |
| Surface #515272 east ↔ Darkfall #95300 west | [Surface](../../../areas/wld/surface.wld#L320688); [Darkfall](../../../areas/wld/darkfall.wld#L18) | Current active pair. |
| Mira #82679 south ↔ Darkfall #95312 north | [Mira](../../../areas/wld/mira.wld#L3431); [Darkfall](../../../areas/wld/darkfall.wld#L249) | Current reciprocal bushes pair. The existing dossier traces the raw flag value `4`, loader masking, and absence of a literal reset override; it does not claim arbitrary installed state. |
| Surface #515679 west ↔ Darkfall #95348 east | [Surface](../../../areas/wld/surface.wld#L329223); [Darkfall](../../../areas/wld/darkfall.wld#L899) | Current active pair. |

An exact-target scan of all 95 current WLD files outside the active
`areas/AREA` list found two records that point into Darkfall. They are in
unregistered `surf.wld`, so they are source-only records, not active-world
ingress:

| Source-only record | Active target | Evidence |
| --- | --- | --- |
| `surf.wld` room #248948 east (`D1`) | Darkfall #95300 | [Exit](../../../areas/wld/surf.wld#L1027063); [room body](../../../areas/wld/surf.wld#L1027052) |
| `surf.wld` room #249568 west (`D3`) | Darkfall #95348 | [Exit](../../../areas/wld/surf.wld#L1040095); [room body](../../../areas/wld/surf.wld#L1040076) |

The unregistered `surface2011.wld` file also declares same-numbered room bodies
for #248948 and #249568, but those exits stay in its own old map: #248948 east
leads to #248949, and #249568 west leads to #249567
([#248948 exit](../../../areas/wld/surface2011.wld#L1027920),
[#249568 exit](../../../areas/wld/surface2011.wld#L1040948)). The bodies and
topology differ from the corresponding `surf.wld` records. This is evidence of
parallel source-map versions, not proof that either file is a complete
replacement for the other. No exact room declaration in the 95 unregistered
WLD files overlaps Darkfall's loaded #95300–#95348 rooms.

### Checkpoint 176 raw graph refresh

A fresh parse of all 351 active WLD contributors found 49 Darkfall rooms and 97/97 resolving authored exits: 94 internal and three cross-area. The local weak components contain 48 rooms and one room. Directed traversal from root #95300 reaches 48/49 rooms, and 48/49 rooms have paths back to the root; room #95310 is isolated from the ordinary local exit graph. See the [root room](../../../areas/wld/darkfall.wld#L1) and [room #95310](../../../areas/wld/darkfall.wld#L209). The all-95 inactive-WLD parse had no errors and no room-VNUM overlap, and reconfirmed the two source-only `surf.wld` inbounds listed above. These are authored graph results; dynamic routes, door/reset state, and safe player return remain separate.

The two `surf.wld` edges add to the curated source-only list in
[WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md). They do not alter its active
exit census, establish a runtime route, or justify a personal Darkfall access
or search requirement.

## Typed dependency closure

### References into Darkfall

The reviewed current `.zon` sources contain no external reset placement of a
Darkfall room, mobile, or object prototype. Exact-ID scans of current MOB, OBJ,
QST, SHP, and story-sidecar sources found no other direct reference to the
local mobile or object VNUMs. The existing dossier's bounded object scan found
no selected type-25 destination into Darkfall and no selected positive-key
consumer in its 1,318-container scan. Numeric collisions in other fields do
not establish typed dependencies.

A room-range search has one separate raw object-data hit: Mansion prototype
#3553 has the number #95345 in its cost column
([object row](../../../areas/obj/mansion.obj#L858)). The object reader assigns
that column to `obj->cost` ([reader](../../../src/world/db.c#L4680)); it is not
a reference to Darkfall room #95345.

The literal special-assignment index for Darkfall has no row, but a direct
code dependency exists outside that inventory. The Epic teacher table maps
Grellinar mobile #95304 to `SKILL_EPIC_AGILITY`
([table entry](../../../src/classes/epic_skills.c#L211)). During normal
full-mode startup, `epic_initialization` binds that table's mobiles to
`epic_teacher` ([binding](../../../src/world/epic.c#L1368),
[startup call](../../../src/net/comm.c#L1035)). That procedure handles
`PRACTICE`, with its own level, class, skill, epic-point, and coin checks; it
is distinct from the native item exchange and does not turn scroll #406 into
a skill grant.

### Native exchange inputs and other owners

The only native local contract is Grellinar #95304's QST contract at
[#37](../../../areas/qst/darkfall.qst#L37): exact items #32490, #26614, and
#402 produce #406 without disappearing. These are foreign prototype
dependencies: the heart and orb have separate producer and competing-contract
evidence in the existing dossier, while tablet #402 and scroll #406 are
defined under the active Heavens object source
([tablet](../../../areas/obj/heavens.obj#L4199),
[scroll](../../../areas/obj/heavens.obj#L4255)). Darkfall's local object file
defines only #95300–#95303; it does not supply the tablet or scroll.

The full source dossier records eight other exact-trio QST offers and two
separate competing contracts (Belial's heart and Sodolum's orb), among 11
individually compared complete Q/QA blocks. These are material ownership and
opportunity-cost dependencies, not a mandatory cross-zone campaign. Its
bounded 888 world/reset and 235 active-QST query found no active reset or QST
reward producer for tablet #402. The tablet's own lore mentions a generic
artifact, while native QST terms require this exact tablet; its intended
current supply and the clue wording remain builder questions.

The native reward and code-defined trainer also represent different outcomes.
The #406 object is raw `TRASH13` with zero values and no selected literal
effect binding; its readable claim does not prove lasting Agility. Grellinar's
separate Epic Agility lesson has its own prerequisites, price, and skill
record. Builder confirmation is needed before changing either path or
presenting them as one reward.

### Boundaries of this closure

This supplements source references found in the reviewed WLD, ZON, QST, SHP,
OBJ, and C++ scopes. It does not establish dynamic/generated/installed state,
editor or external tooling references, arbitrary numeric lookup, saved player
identities, current live inventory, or successful travel and training.
Room #95310 still has no authored ordinary exits, incoming ordinary edge, or
local reset spawn in the reviewed sources; the existing dossier asks the area
owner whether it is intentionally reserved. It is not an implied exploration
prerequisite.

## Builder questions and recommended disposition

1. **Which Surface source is authoritative for the two source-only Darkfall
   leads?** The active exits use current Surface rooms #515272 and #515679;
   only the old `surf.wld` bodies point into Darkfall, while the same-numbered
   `surface2011.wld` bodies do not. **Recommendation:** keep Darkfall active
   and retain both unregistered map families in place until their source owner
   resolves their provenance. Do not treat the two raw leads as active routes.
2. **How should the exact tablet #402 be supplied, and is the broader artifact
   clue still intended?** **Recommendation:** preserve Q37's exact three-item
   terms and ask the builder to resolve source availability and clue wording
   before any separately named repair. Do not add an inferred fourth item or
   route prerequisite.
3. **Are Grellinar's paid Epic Agility lesson and his Agility-scroll exchange
   intentionally independent?** **Recommendation:** keep them recorded as
   separate code and QST outcomes. Clarify the intended effect of scroll #406
   before changing its magic behavior or promising a lasting stat increase.

No source, list membership, candidate file, route, reward, or zone placement
changed. Dynamic route qualification and played quest, training, and
persistence behavior remain pending.
