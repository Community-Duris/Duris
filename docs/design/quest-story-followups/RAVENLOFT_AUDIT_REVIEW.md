# Castle Ravenloft: repository-wide source audit supplement

This supplement complements the [Castle Ravenloft dossier](../zone-stories/CASTLE_RAVENLOFT.md).
It reconciles active WLD boundaries, the all-source unregistered WLD scan, exact typed
references, shared code uses, the normalized title, and tracked source history. These are
static repository findings; they do not establish live access, runtime stock, or player history.

## Active boundaries and local graph

Castle Ravenloft has 269 rooms and 689 authored exits. Every exit target resolves in the active
WLD set; 686 exits stay within Ravenloft and three leave it. The local weak-exit graph has five
components with 223, 43, 1, 1, and 1 rooms. Six active exits enter Ravenloft:

| Source exit | Ravenloft target | Evidence |
| --- | --- | --- |
| Barovia #91167 down | #58568 | [cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1120) |
| Barovia Continued #58958 south | #58567 | [cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L815) |
| Catacombs #59057 up | #58538 | [cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L816) |
| Catacombs #59352 north | #58542 | [cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L817) |
| Catacombs #59398 east | #58305 | [cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L823) |
| The Realm #14208 southeast | #58369 | [cross-area record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L251) |

Three Ravenloft exits lead back to the Catacombs or Barovia Continued, and each closes a
reciprocal pair: #58538 down↔Catacombs #59057 up, #58542 south↔Catacombs #59352 north, and
#58567 north↔Barovia Continued #58958 south ([Ravenloft exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L811),
[Ravenloft exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L812),
[Ravenloft exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L813)). The other three arrivals are
one-way authored exits. This describes source topology, not door state, safe passage, or a
successful return journey.

## Unregistered WLD source sweep

The all-95-file scan parsed 527,791 rooms and 2,080,773 exits from unregistered WLD sources,
with zero parse issues. No source room shares a VNUM with Ravenloft, and no unregistered exit
targets one of its rooms. This scan adds no Ravenloft approach. The current curated source-only ingress total remains 4,489 (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight); these files are not active contributors through the current manifest.

## Exact typed references and external ownership

Five active external QST item commands use four Ravenloft objects. Barovia gives Gertruda
#58412 to Mad Mary; Barovia Continued gives Perganan's note #58416 to Urik; the Catacombs give
the dracolich skull #58397 to Rahadin; and the Catacombs' Gevin handoff references spectral key
#59288 and old key #58419, then returns #58419
([Barovia](../../../areas/qst/barovia.qst#L645),
[Barovia Continued](../../../areas/qst/barovia2.qst#L49),
[Rahadin](../../../areas/qst/ravenloft2.qst#L1723),
[Gevin's item commands](../../../areas/qst/ravenloft2.qst#L2318),
[old-key removal](../../../areas/qst/ravenloft2.qst#L2319),
[old-key return](../../../areas/qst/ravenloft2.qst#L2320)). The four local object definitions are
[Gertruda](../../../areas/obj/ravenloft.obj#L1852),
[Perganan's note](../../../areas/obj/ravenloft.obj#L1909),
[the dracolich skull](../../../areas/obj/ravenloft.obj#L1595), and
[the old key](../../../areas/obj/ravenloft.obj#L1961). These are foreign-owned delivery
workflows; each source, local carry, onward acceptance, and returned-key custody remains a
separate event.

All 231 external story sidecars parsed without issues. Four exact structured
`item_vnums` entries match the same objects: [Gertruda](../../../areas/story/barovia.story.json#L744),
[the note](../../../areas/story/barovia2.story.json#L104),
[the skull](../../../areas/story/ravenloft2.story.json#L818), and
[the old key](../../../areas/story/ravenloft2.story.json#L1778). These are object-typed fields;
numeric overlap with room VNUMs does not change their identity. No external SHP product VNUM
matches a local object, and no external MOB/OBJ prototype definition reuses a Ravenloft mobile
or object VNUM. No external ZON room-placement reset targets a Ravenloft room; the one
external portal destination into Ravenloft is described below.

The bounded cross-zone reset review classified 36 complete groups that place or equip Castle
prototypes in other zones across active and unregistered sources. Examples include Alatorin
container and equipment groups
([nested items](../../../areas/zon/alatorin.zon#L1468),
[gem bundle](../../../areas/zon/alatorin.zon#L2647),
[worn gems](../../../areas/zon/alatorin.zon#L3342)), Brass equipment
([active reset](../../../areas/zon/brass.zon#L887)), Earth Plane equipment
([reset](../../../areas/zon/earthp.zon#L118)), a Heavens room placement
([reset](../../../areas/zon/heavens.zon#L248)), Catacombs' Castle actors and equipment
([Catacombs reset](../../../areas/zon/ravenloft2.zon#L1332)), and Surface Keeps' nested
change ([reset](../../../areas/zon/surfacekeeps.zon#L234)). An unregistered
`brass-old-1.zon` retains an additional old Brass reset for #58331
([unregistered row](../../../areas/zon/brass-old-1.zon#L897)); it is separate from the active
`brass.zon` row. These reset declarations belong to their owning zones and do not make those
items Ravenloft quest rewards or guaranteed sources.

The external portal scan covered 868 type-25 declarations. One targets local room #58456:
Catacombs handprint #59325 is placed at #59065 and declares destination #58456
([object](../../../areas/obj/ravenloft2.obj#L5549),
[destination](../../../areas/obj/ravenloft2.obj#L5555),
[reset placement](../../../areas/zon/ravenloft2.zon#L847),
[Ravenloft room](../../../areas/wld/ravenloft.wld#L3332)). This is a foreign Catacombs
travel control, distinct from the Castle's four local type-25 objects and from an exchange
receipt. None of the other scanned external type-25 objects points into Ravenloft.

## Shared code references

The shared Chaos equipment tables contain several Ravenloft object VNUMs, including #58417,
#58379, #58365, #58324, #58367, and #58399
([equipment tables](../../../src/account/chaos_eq_data.h#L89),
[shaman table](../../../src/account/chaos_eq_data.h#L140),
[sorcerer table](../../../src/account/chaos_eq_data.h#L150),
[reaver table](../../../src/account/chaos_eq_data.h#L251),
[reaver entry](../../../src/account/chaos_eq_data.h#L254)). These are object slots;
some numbers also exist in the mobile namespace, so they are not typed mobile references.
The random-equipment high-drop list names Obox-Ob #58379 and Strahd #58383, but the loop that
would apply that bonus is commented out
([list](../../../src/item/randomeq.c#L147),
[disabled lookup](../../../src/item/randomeq.c#L644)).

Current assignments bind artifact #58424 to its shadow-shield special
([assignment](../../../src/specs/specs.assign.c#L2176)), bell #58413 to `ravenloft_bell`
([assignment](../../../src/specs/specs.assign.c#L2179)), Bram #58387 to the Vistani shout
([assignment](../../../src/specs/specs.assign.c#L2180)), and sword #58400 to its shimmer
special ([assignment](../../../src/specs/specs.assign.c#L2182)); the Strahd charm binding is
commented out ([assignment](../../../src/specs/specs.assign.c#L2181)).
[The helper list](../../../src/specs/specs.ravenloft.c#L57)
includes local Vistani mobile #58382. The old
`do_descend` path also
mentions tome #58424, but returns the disabled message before its item checks and consumption
([disabled command](../../../src/cmd/actoth.c#L7887)). Copyover and extraction matches for
#58424 are debug logging checks, not another item source or quest contract
([copyover logging](../../../src/persistence/copyover.c#L2091),
[extraction logging](../../../src/world/handler.c#L5846)). A raw match inside the unrelated
SplitMix hash constant `0xbf58476d1ce4e5b9` is not a VNUM reference.

## Title, duplicate review, and tracked history

The normalized ZON title appears once among 351 active area titles. The active manifest entry
is `ravenloft *583-587` ([AREA entry](../../../areas/AREA#L240)); its title is “Castle
Ravenloft” ([ZON header](../../../areas/zon/ravenloft.zon#L2)). The duplicate/replacement
register contains no separate Ravenloft or Strahd predecessor or replacement entry
([register](DUPLICATE_AND_REPLACED_ZONES.md)).

Tracked WLD, MOB, OBJ, ZON, and QST history remains on the current `ravenloft` paths back to
the 2009 area-tree move, commit `642da5400`. The story sidecar was added at its current path
in commit `7c4f028df`. No distinct predecessor filename or replacement source path was
identified.

## Audit boundary

This is a static source review. It does not qualify loaded portal reachability, runtime reset
stock, successful foreign delivery, player custody, safe traversal, or accounting settlement.
No native repair is proposed in this checkpoint.

## Checkpoint 336: Castle Ravenloft reset-aware arrivals

The existing dossier and source audit already cover the Castle's story, foreign quest workflows, special behavior, title, and followed history. This checkpoint adds a reset-aware static walk from each of its six active inbound roots. The active WLD has 269 rooms and 689/689 resolving exits (686 local, three outbound) in five weak components sized 223, 43, 1, 1, and 1.

The six inbound roots each reach 140/269 rooms through raw directed WLD exits. The current ZON has 110 D rows and no duplicate room/direction reset. Source reset state is applied in [db.c](../../../src/world/db.c#L7857): 32 exits are open, 46 closed/unlocked, 14 closed/locked, four closed/secret, and 14 open/blocked. Excluding reset-closed, locked, secret, or blocked exits gives the following strict baseline:

| Active arrival root | Raw directed reach | Strict post-reset reach |
| --- | ---: | ---: |
| Barovia #91167 to #58568 | 140/269 | 14/269 |
| Barovia Continued #58958 to #58567 | 140/269 | 1/269 |
| Catacombs #59057 to #58538 | 140/269 | 14/269 |
| Catacombs #59352 to #58542 | 140/269 | 3/269 |
| Catacombs #59398 to #58305 | 140/269 | 14/269 |
| The Realm #14208 to #58369 | 140/269 | 5/269 |

Four fall rooms are recorded (#58504, #58515, #58559, and #58563); no currents are declared. A closed, secret, or blocked WLD edge can be changed by the castle's controls, item switches, specials, and puzzle sequence. This strict baseline does not include successful puzzle interaction, portal use, actor placement, safe falling, or a return journey.

The all-95 inactive-WLD scan still finds no Ravenloft room overlap or source-only approach. The current curated source-only total stays 4,489 (Surf 253, ud_map1 two, ud_map2 20, Underworld2 eight). No candidate or area membership changes.

### Checkpoint 336 builder questions and recommended resolutions

These questions focus owner decisions in the existing comprehensive dossier and source audit. Recommendations retain source and accepted outcomes unless a builder selects a separately qualified change.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| CR01 | Are all six active entry points intended alternatives into the Castle? | Preserve the three Catacombs entrances, the Barovia and Barovia Continued entries, and the Realm entry as separate active sources; do not force a single default route. |
| CR02 | Are the three reciprocal outbound edges meant to provide the current authored returns? | Preserve each paired exit and its owning area; do not infer return safety from reciprocity alone. |
| CR03 | Are the five weak components (223, 43, 1, 1, 1 rooms) intentional alongside the travel controls and switches? | Keep WLD connectivity distinct from type-25 travel and command-controlled changes until the owner identifies a missing ordinary exit. |
| CR04 | Is the very limited strict reach after resets intended for each of the six arrivals? | Retain current door reset states and puzzle controls; treat the strict counts as a baseline, not a repair requirement. |
| CR05 | Are the 14 open/blocked reset states meant to be cleared by their linked item switches or other controls? | Preserve the source control relationships and do not count blocked exits as ordinary paths before the required interaction. |
| CR06 | Should the secret, closed, and locked doors keep their different key, speech, search, and special behavior? | Retain the current distinct door states and access methods; do not replace them with generic open exits. |
| CR07 | Are the four falling rooms and their 65, 90, and 100 values intended hazards? | Keep the source fall values and room descriptions; do not describe them as safe or remove them without a builder-approved movement design. |
| CR08 | Are the fixed PUSH, PULL, RUB, BLEED, GET, and READ controls intended as independent exploration routes? | Preserve each command and its exact exit effect; credit story progress only after its defined state transition is qualified. |
| CR09 | Should the four local type-25 routes and the foreign Catacombs handprint remain separate travel controls? | Keep each source owner and destination distinct; portal travel does not establish a hand-in or puzzle completion. |
| CR10 | Are the past and future Obox-Ob forms intended to supply independent essences without a personal kill or time-travel sequence? | Preserve both source alternatives and current accepted bundle; do not add a personal chronology gate. |
| CR11 | Is the repeat bell behavior intended to consume another mallet after the case is already open? | Retain current behavior pending owner choice; any guard or repeat change needs a separately named fix/news item and an accounting-qualified transaction. |
| CR12 | Should bell completion leave the same case prototype closed and pickproof while changing its state and descriptions? | Preserve case #58430 and its recorded state changes; do not substitute the unused bright-case prototypes. |
| CR13 | Are the two local essence stories and supplied alternatives meant to remain independent from boss appearances and player history? | Preserve the five accepted receipts and separate source issuance from player-supplied custody. |
| CR14 | Are foreign acceptances in Barovia, Barovia Continued, and the Catacombs separate delivery events from the Castle hand-ins? | Keep source acquisition, local delivery, onward acceptance, and returned-key custody under their respective area owners. |
| CR15 | Should the four exact story-sidecar item_vnums continue to mirror the same native object workflows? | Preserve object identity and existing receipts; do not create an additional stage from numeric match alone. |
| CR16 | Are inner and outer Megosh distinct independently placed actors, without a Strahd transformation or spawn chain? | Preserve both placements and their separate quest chains; keep transformation wording descriptive unless a new behavior is designed. |
| CR17 | Should You Strahd Me at Hello retain its current native kill order, solo/group rules, and epic reward? | Preserve native ordering and party eligibility; qualify settled death and reward publication before adding journal credit. |
| CR18 | Should the old do_descend command remain disabled, and should its old item-consumption code stay inactive? | Keep the unconditional disabled response; do not restore the old tome/book/orb transformation path during story mapping. |
| CR19 | Are the unplaced Ferrik book and return-control prototypes intentionally unplaced? | Preserve the prototypes and current routes; do not add reset stock until the owner specifies their intended use. |
| CR20 | Should Perganan's help keep one verified alias visible per response, and is its D0 availability intentionally a potential daily? | Preserve native recognition and hand-in; do not promise renewable stock, and retain the display limit until the builder chooses a paginated topic design. |

This is a documentation-only update to an existing source-comprehensive dossier. No native source, quest, WLD, reset, area membership, or gameplay behavior changed. No build, test, server, database, migration, or gameplay session was run.
