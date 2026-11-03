# Surface Minizones audit review (Checkpoint 375)

This supplement reconciles `area:surfacemini` with the existing
[comprehensive source story map](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md)
and the current world, dependency, and title/history registers. The source
snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay
source was changed. `surfacemini` is a separate contributor from `minizones`
(zone 57, title “Mini Zones”); the similar names do not establish a shared map
or replacement.

## Identity and source inventory

`surfacemini` is an active, full-area contributor at zone 979, through room
#98018. It has 119 rooms (#97900–#98018), 25 mobile prototypes (#97900–#97924),
42 object prototypes (#97900–#97941), seven native QST giver IDs, and 25 Q/QA
contracts. Its WLD, MOB, OBJ, ZON, QST, and story sidecar are present. There is
no local SHP file; the register’s zero-shop count is correct.

The existing dossier already maps all 119 rooms, all 25 mobiles and 42 objects,
145 reset commands, five native story outcomes, and sixteen supporting service
families. Its static inventory separates accepted exchanges from actual item
generation, movement, payment settlement, safe returns, equipment use, and
longer story episodes. This audit adds repository-wide graph, typed-reference,
identity/history, and builder-question reconciliation without replacing that
dossier.

## Active graph and boundaries

The active WLD declares 261 directions from 119 rooms; all 261 resolve in the
loaded world, with 250 local directions and eleven outward directions. Eight
active directions enter from Surface rooms, eleven leave for Surface, and seven
outbound records have reverse-direction edges. There are no negative targets,
positive unresolved targets, range-mismatch rooms, or unregistered-WLD targets.
The active graph has ten weak components (55, 44, 6, 6, 2, 2, 1, 1, 1, 1 rooms).

The exact Surface boundaries are:

| Direction | Boundary records |
| --- | --- |
| Inbound | #612685 north → #97900; #578911 south → #97908; #579310 east → #97931; #579312 west → #97951; #590430 up → #97959; #567418 north → #97953; #519153 and #518753 down → #98014 |
| Outbound | #97900 south → #612685; #97906 south → #564065; #97908 north → #578911; #97931 west → #579310; #97951 east → #579312; #97952 south → #578863; #97953 south → #567418; #97959 down → #590430; #98014 up → #519153; #98016 south → #522869; #98017 south → #626598 |

Four outward boundaries (#97906, #97952, #98016, and #98017) have no ordinary
reverse WLD direction. The crater #98014 has two Surface inbounds: the one from
#519153 has a return, while the one from #518753 does not. In the raw directed
WLD graph, the seven distinct inbound target rooms reach 112 of 119 rooms;
#97906, #97907, #97952, #98015, #98016, #98017, and #98018 are outside that
authored traversal. Reverse traversal from all outward source rooms reaches 118
rooms; #98018 has no ordinary exit and no path to an outward WLD boundary.
These are static direction results, not proof of playable entry or safe return.

Active Surface type-25 objects #500002, #500035, #500051, and #500052 also name
rooms #97906, #97952, #98016, and #98017. The inactive `surface2011.obj` source
contains corresponding type-25 declarations; those are source evidence only,
not active inventory. Keep item-mediated arrivals separate from WLD exits.
The dossier records ten rooms with fall chances and no current markers, along
with the controls, door/reset states, and fall metadata details. Movement,
mounted arrival, fall protection, and safe-return behavior remain unqualified.

The all-inactive-WLD scan found no source-only approach into a SurfMini room and
no inactive room VNUM overlap. No attic candidate row or source-only route is
added, and the curated source-only total remains 7,516 (Surf subset 285).

## Typed dependencies

No same-type WLD, MOB, or OBJ VNUM overlap with another area was found. Within
SurfMini itself, all 25 mobile VNUMs #97900–#97924
are also declared as object VNUMs. These are distinct prototype types and must
remain type-qualified in command, reset, sidecar, shop, and procedure review.

The bounded repository scan found these outside references into SurfMini:

| Source | Exact typed reference | Interpretation |
| --- | --- | --- |
| [`alatorin.qst`](../../../areas/qst/alatorin.qst#L1096) | Two `R I` lines for object #97910 | Active QST object references at lines 1096 and 1097. |
| [`mntcastl.qst`](../../../areas/qst/mntcastl.qst#L254) | One `G I` line for object #97903 | Active QST item reference. |
| [`mntcastl.story.json`](../../../areas/story/mntcastl.story.json#L359) | Giver mobile #97901 and item #97903 | Structured source references; they do not establish a live giver or delivery. |
| [`alatorin.zon`](../../../areas/zon/alatorin.zon#L4277) | Five `G` reset records for object #97934 | Active reset declarations at lines 4277, 4281, 4285, 4289, and 4293. |
| [`surfacekeeps.zon`](../../../areas/zon/surfacekeeps.zon#L246) | One `G` reset for object #97917 | Active reset declaration. This VNUM is also a local mobile, so the reset’s object type matters. |
| [`surfacekeeps.shp`](../../../areas/shp/surfacekeeps.shp#L241) | Object #97917 in shop #120009 | Active stock declaration, not evidence of guaranteed stock. |
| [`surface.obj`](../../../areas/obj/surface.obj#L41) | Four type-25 destinations: #500002 → #97906, #500035 → #97952, #500051 → #98016, #500052 → #98017 | Active item routes into four rooms without ordinary reverse WLD entries. |
| [`surface2011.obj`](../../../areas/obj/surface2011.obj#L41) | Four corresponding type-25 destinations | Inactive source evidence, not an active route or same-type collision. |

There is no external QST giver prototype with a SurfMini mobile VNUM, no other
foreign typed sidecar reference into its local prototypes, and no foreign SHP
product reference beyond #97917. The local QST has 79 external item-command
references across 30 distinct object VNUMs. Its story sidecar names 30 distinct
external contact mobiles and 29 distinct external item prototypes across its
typed `item_vnums` fields; those references and the five native outcomes are
already traced in the source dossier. Local ZON commands declare 22 resets for
six outside mobile prototypes and nine resets for five outside object
prototypes. Their presence is reset-source evidence, not proof of admission or
loaded supply.

Maintained code assigns `elemental_wand` to object #97923 and the flame/frost
collar handlers to #97931/#97932 in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L2115). Winterhaven code
also creates an object selected from #97924–#97929
([procedure reference](../../../src/specs/specs.winterhaven.c#L4180)). The
dossier describes the guarded paths and their cleanup/payment gaps; code
bindings do not qualify installed behavior.

## Title, same-type identity, and tracked history

After color-code and terminator removal, whitespace normalization, and case
folding, “The Minizones of the Surface” has one exact match among 442 current
ZON titles and is used by one of 351 active contributors. Active and
source-only WLD, MOB, and OBJ scans found no same-type VNUM collision with
another contributor. The cross-type mobile/object overlap within this zone is
listed above; it is not evidence of duplicate ownership or replacement.

Followed tracked histories stay on the `surfacemini` stem: WLD through
2009-10-25, MOB through 2015-04-22, OBJ through 2016-08-15, ZON through
2015-10-13, and QST through 2026-06-09. The story sidecar was added
2026-10-04. There is no local SHP history because no SHP source exists. These
paths and the current manifest show no alternate-stem rename, distinct
predecessor, or whole-zone replacement. They cannot rule out untracked history.
The normalized-title and bounded tracked-history review is complete;
command-target alias/owner disambiguation remains pending.

## Twenty builder questions and recommended resolutions

The source dossier supplies the detailed evidence for these questions. The
recommendations preserve current accepted exchanges and keep unresolved
location, supply, and history choices explicit.

| # | Builder question | Evidence and recommendation |
| ---: | --- | --- |
| 1 | Should the frost collar clue name the Ice Prophet’s ring rather than Fire? | The implemented input is Frosty ring #500046. Correct the clue to match the source; do not change the recipe. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 2 | Should the flame-collar room messages name flame and its debug label name flames rather than frost? | The current flame path emits ice wording while the debug label says frost. Correct the targeted text and verify both handler branches. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 3 | Should narrow-room rejection happen before the flame collar creates its mobile, or should every rejected mobile be extracted? | The flame path allocates before rejecting and does not extract on that branch; frost does. Fix and prove the original failing branch while preserving successful summon timing. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 4 | How should either collar handle a failed `char_to_room` result or an actor whose room becomes NOWHERE during setup? | Both paths ignore placement failure before later mutation/follow work. Validate actor, item, and pet placement before setup or cooldown; test rejection and cleanup. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 5 | Should flame and frost continue to share one follower-eligibility policy? | Their present predicates match, so the described null-mobile path is conditional rather than a current failure. Keep the policy aligned and add a guard if the predicates change. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 6 | Should the elemental wand validate its random replacement before consuming the worn wand? | The procedure extracts first and does not check replacement creation/placement. Create and validate first, then consume once and preserve one-of-six output balance. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 7 | Should `earth` remain a valid alias for the Ethereal wand result? | The alias set overlaps the Earth result name. Choose a single clear alias per variant and update help/clues with the exact handler behavior. [Dossier](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 8 | Should the Observatory, humble-hermit, laboratory, and Mugflog pockets gain ordinary Surface entries? | Their outbound directions lack reverse WLD entries, although four active portal objects target four of those rooms. Confirm intended walking versus item travel and add only authored, testable routes. [Boundaries](#active-graph-and-boundaries) |
| 9 | Is the second Surface entrance into crater #98014 intentional without a reverse direction? | #518753 enters the crater without a return; #519153 has a reciprocal route. Confirm placement and preserve the existing map until resolved. [Boundaries](#active-graph-and-boundaries) |
| 10 | Should the final igloo #98018 remain isolated, or should it receive a documented route and return? | It has no ordinary exits or reviewed incoming link and cannot reach an outward WLD edge. Keep it dormant until location and safe arrival/return are decided. [Room and prototype review](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#access-foreign-hosts-and-non-quest-content) |
| 11 | Should the loader-cleared fall declaration at room #97999 be repaired or removed? | The room declares falling without a DOWN exit, which the loader clears; ten rooms otherwise have fall chances. Confirm intended hazards and qualify flight, mounts, and return safety. [Room and prototype review](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#access-foreign-hosts-and-non-quest-content) |
| 12 | Should ice elementals #97918/#97919 and Ishaz #97921 remain dormant or receive active reset episodes? | No active M/F/R declaration was found for these prototypes. Do not infer an Ishaz quest or enable content without owner intent and a reviewed source. [Room and prototype review](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#access-foreign-hosts-and-non-quest-content) |
| 13 | Should Chibbleniffle’s local laboratory duplicate and global cap remain shared with Winterhaven? | The mobile and apparatus use shared cap one, while the three contracts belong to Winterhaven. Preserve ownership or explicitly design a separate host; do not count three new Surface outcomes. [Room and prototype review](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#access-foreign-hosts-and-non-quest-content) |
| 14 | Should the local chest #55326 receive contents and an active source for key #55327, and should the mold exchange award the narrated key or its current orb? | No local chest contents or active key producer was found; the exchange narrates a key but awards an orb. Resolve the container, key, and foreign-host flow together. [Room and prototype review](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#access-foreign-hosts-and-non-quest-content) |
| 15 | Should the Astral Tiamat essence path restore missing mobile #19921 or remove that incomplete alternative? | The default Demogorgon source exists, so this is an incomplete alternative rather than a blocked whole quest. Preserve default supply while the owner chooses. [Pending repairs](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 16 | Should source history for death-produced materials stay optional when a player supplies valid matching proof? | The current recipes accept supplied items. Keep provenance separate from delivery; bind generation, custody, first recovery, and transfer only to deliberate history outcomes. [Progression stories](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#supply-and-progression-stories) |
| 17 | How should mixed item-and-coin offers commit fees, consumed proof, and output together? | Several offers have guarded mixed payments. Keep exact recipes and qualify wallet readiness, debit, input reservation, and output recovery as one operation before changing settlement. [Native exchanges](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#every-native-exchange) |
| 18 | Should retiring producers and shared sphere, scepter, plant, token, and material inputs receive a deliberate order or remain independent offers? | Multiple services compete for finite inputs and some producers retire. Preserve the documented recipes and do not imply one source supplies every commission; qualify the actual chosen sequence. [Native exchanges](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#every-native-exchange) |
| 19 | Should wand transformation, collar summoning, and support-service receipts add any personal story or daily credit? | Services currently add no story or daily credit; transformations and pets have no accepted lifecycle endpoint. Keep these exclusions until actual equipment, ownership, arrival, removal, and recovery events are authored. [Pending repairs](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#pending-repairs-and-universal-capability-expansion) |
| 20 | Should griffon and key-assembly narration gain real mount/key endpoints, or remain clues? | No native accepted endpoint proves the narrated mount transport or key assembly. Keep them as clues until a committed actor, room, and world-state episode is designed and qualified. [Access and foreign hosts](../zone-stories/THE_MINIZONES_OF_THE_SURFACE.md#access-foreign-hosts-and-non-quest-content) |

## Disposition

Keep active `surfacemini` **DO NOT MOVE**. Preserve its exact boundary and
portal routes, typed object/mobile identities, reset and shop consumers,
source-comprehensive story map, and open repair decisions. The inactive-WLD
scan adds no candidate edge. Source counts and static references do not qualify
live payment, reset, route, hazard, pet, or persistence behavior; no gameplay or
migration decision is implied.

### Checkpoint 756: related Minizones titles and cross-area story reference

A normalized scan of all 442 current .zon titles again finds one exact “The Minizones of the Surface” owner: active surfacemini, listed as *979–980 in areas/AREA and *979 in areas/qst/AREA. The closest title identities are separately rostered minizones, “Mini Zones” (*57–59), and udmini, “The Minizones of the Underdark” (*885–886). The larger “The Surface Realm of Duris” (surface, *5000–6600) is another separately listed area. The shared terms identify related world regions, not alternate SurfMini titles or maps.

Woodseer’s story sidecar says its fish merchant’s salmon steak is sought by a hermit in the Surface minizones and explicitly assigns that return to the hermit’s zone. SurfMini’s own QST and story sidecar contain the hermit’s requests. This is a cross-area story/source relationship; it does not create another map owner. The bounded exact-name scan found no external SurfMini map declaration.

The existing same-type census found no active or source-only WLD/MOB/OBJ VNUM collision; the all-inactive-WLD scan found no SurfMini room overlap or approach. The followed histories remain on the surfacemini stem, with no alternate-stem predecessor, rename, or whole-zone replacement. The attic register has no surfacemini candidate row, and no candidate plan changes here. Keep surfacemini DO NOT MOVE. The title and semantic map-identity review is complete; command-target alias and owner disambiguation remain separate.

## Checkpoint 787: Divine Home reset imports

Active `surfacemini.zon` resets Divine Home object #40743 once and #40746 five times: O
row 60, G rows 160, 162, 178, 180, and 181. Preserve Divine Home as their current
prototype owner; reset declarations do not establish stock or runtime availability. See
the [Divine Home typed audit](DIVHOME_AUDIT_REVIEW.md#active-external-consumers).


## Checkpoint 830: current-source item roles and completion signatures

The current `areas/` and `src/` trees still match pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. The active 119-room graph, 25 mobile and 42 object prototypes, 25 native contracts, 20 builder questions, CP756 title/history findings, and no-candidate disposition remain unchanged.

A complete exact `G I` / `R I` parse of local `surfacemini.qst` finds 79 external item-command roles: 74 `G` inputs and five `R` outputs. They reference 34 distinct external object VNUMs (29 distinct `G` objects plus five `R`-only objects), all resolved to active object files. This corrects Checkpoint 375’s distinct-VNUM total of 30; the role count remains 79. The sidecar still has 49 external `item_vnums` entries over 29 VNUMs and 30 distinct external `mob_vnum` contacts. It also contains 13 external `giver_vnum` field occurrences over nine mobile VNUMs, including Surface prophets #500116 and #500117.

A role-aware parse of `completion_key` item operands finds 34 distinct external signatures containing 55 distinct external object VNUMs. Twenty-nine of those VNUMs overlap the sidecar’s typed `item_vnums` list; 26 occur only in completion signatures. All 55 resolve to active object owners. This includes the two Lancer signatures for Dungeon object #93011 already recorded at CP791, plus Surface-owned heads, rings, and potions in the Fire/Ice prophet signatures; `C:` and `E:` amounts are excluded as currency and experience. Outside sidecars also point back to SurfMini: Alatorin has one distinct completion signature for #97910, and Mount Castle has two for #97903; Mount Castle’s typed fields additionally name giver mobile #97901 and item #97903. All 232 story sidecars parse without errors.

The local ZON still has 22 reset roles across six outside mobile VNUMs and nine object reset roles across five outside object VNUMs. The five objects resolve to Connector Zones (#53624), Divine Home (#40743 and #40746), and White Harbor (#55033 and #55246); the six mobiles resolve to Connector Zones (five) and White Harbor (one). Current inbound resets are Alatorin’s five G rows for object #97934 and SurfaceKeeps’ one G row for cross-type object #97917. Preserve the six Divine Home rows from CP787. The only external SHP product is SurfaceKeeps shop #120009’s #97917 stock row; SurfMini has no local SHP.

The active external type-25 items remain Surface #500002/#500035/#500051/#500052 to rooms #97906/#97952/#98016/#98017. `surface2011.obj` retains four matching source-only declarations; they are not active travel items. No external type-29 target or WLD key field points into SurfMini. Local type-29 objects #97901 and #97911 name rooms #97903 and #98000; #97906 has no destination. The existing direct code bindings to #97923, #97931, #97932, and Winterhaven’s #97924–#97929 creation range remain as recorded above.

The current graph remains 261/261 resolved directions, with 250 local, 11 outbound, eight inbound, and seven reciprocated outbound records. Ten weak components and the existing 112/119 forward and 118/119 reverse static reach results remain. No inactive WLD source adds an approach or room overlap. Keep active `surfacemini` **DO NOT MOVE**, retain the separate `minizones`, `udmini`, `surface`, and inactive `surface2011` source identities, and add no candidate row or migration disposition.