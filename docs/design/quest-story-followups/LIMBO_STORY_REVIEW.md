# Limbo: source and dependency review

**Scope:** static review of the active Limbo area’s rooms, mobiles, objects,
zone data, authored exits, tracked file history, and typed source references.
This review does not establish player access, runtime fallback outcomes, or the
intent behind the low-numbered prototype imports.

## Source identity and local content

`limbo` is an active full-area contributor with negative zone number `-1`,
title “Limbo,” rooms `#0`–`#1`, six mobile prototypes `#1`–`#6`, and nine
object prototypes `#1`–`#9` ([active list](../../../areas/AREA#L1),
[zone header](../../../areas/zon/limbo.zon#L1),
[coverage row](ZONE_COVERAGE_REGISTER.csv#L159)). Its room source has no
authored exits. The zone file has only its header and terminator; it contains
no local reset commands ([zone source](../../../areas/zon/limbo.zon),
[room source](../../../areas/wld/limbo.wld)).

The old quest index includes `limbo`, but there is no
[`areas/qst/limbo.qst`](../../../areas/qst/AREA#L1), local shop file, or story
sidecar. The active area has no QST giver or native Q/QA contract. Its negative
zone number is outside the nonnegative native-contract catalog denominator.

| Source element | Static content |
| --- | --- |
| Room `#0`, “The Void” | A short, self-referential joke describes floating in nothing ([room](../../../areas/wld/limbo.wld#L1)). |
| Room `#1`, “Limbo” | A formless void with swirling light and a pinned “No Tipping” notice ([room](../../../areas/wld/limbo.wld#L8)). |
| Mobiles `#1`–`#6` | A blank shaping template, a giant black “durian” dragon, and four colored dracoliches ([mobile prototypes](../../../areas/mob/limbo.mob#L1), [dragon](../../../areas/mob/limbo.mob#L12), [dracoliches](../../../areas/mob/limbo.mob#L26)). |
| Objects `#1`–`#9` | A skull dagger, corpse, coins, blood, blank note, unmade treasure, master spellbook, unmade body part, and buggy scrap ([object prototypes](../../../areas/obj/limbo.obj#L1), [blood](../../../areas/obj/limbo.obj#L35), [paper note](../../../areas/obj/limbo.obj#L47)). |

The map editor input places the two rooms as separate map positions, consistent
with their isolated source records; it is an editor map, not evidence of a
player route ([map input](../../../areas/map/limbo.json#L1)).

## Static exits and boundary records

Limbo has zero authored exit records, zero outgoing active cross-zone exits,
and two one-room weak components. Neither room has a fall or current field.
These are source-graph facts only.

Active Desert room `#49790`, titled “jussin load room,” has nine of its ten
direction slots pointed at Limbo room `#1`: north, east, west, up, down, and
the four diagonals. Its remaining south exit points to Desert room `#49310`
([south target](../../../areas/wld/desert.wld#L16960)).
The cross-area register records the nine inbound edges with no reverse
direction records ([Desert room](../../../areas/wld/desert.wld#L16945),
[edge records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L630),
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L113)). Desert resets place
Jussin and a heavily cloaked mercenary in `#49790`
([Jussin](../../../areas/zon/desert.zon#L1634),
[mercenary](../../../areas/zon/desert.zon#L1636)). The source calls the room a
load room; the exits alone do not establish ordinary player access or the
reason for the Limbo targets.

Thirteen additional WLD exits into `#1` occur in unregistered sources. These
records are separate from the active Desert edges and do not add active-world
ingress:

| Unregistered source records | Targets | Evidence |
| --- | --- | --- |
| `dalvik.wld` room `#2`, north (`D0`) | Limbo `#1` | [target](../../../areas/wld/dalvik.wld#L37) |
| `hostel.wld` rooms `#2` (`D2`), `#49` (`D0`), `#86` (`D1`) | Limbo `#1` | [room `#2`](../../../areas/wld/hostel.wld#L40), [room `#49`](../../../areas/wld/hostel.wld#L934), [room `#86`](../../../areas/wld/hostel.wld#L1757) |
| `Magetower.wld` rooms `#2` (`D0`) and `#64` (`D2`) | Limbo `#1` | [room `#2`](../../../areas/wld/Magetower.wld#L21), [room `#64`](../../../areas/wld/Magetower.wld#L863) |
| mavulsk.wld rooms #2 (D2) and #3 (D0) numerically target #1; Mavulsk itself defines #1 as “Before the Stone Mansion.” | Current active owner of #1 is Limbo; these source references are not confirmed Limbo approaches. | [room #2](../../../areas/wld/mavulsk.wld#L47), [room #3](../../../areas/wld/mavulsk.wld#L70), [Mavulsk identity audit](MAVULSK_AUDIT_REVIEW.md#wld-graph-and-unresolved-map-boundaries) |
| `Mentiri.wld` room `#2`, up (`D4`) | Limbo `#1` | [target](../../../areas/wld/Mentiri.wld#L21) |
| `Nexus_1.wld` room `#2`, south (`D2`) | Limbo `#1` | [target](../../../areas/wld/Nexus_1.wld#L17) |
| `theurgist.wld` rooms `#2` (`D0`), `#3` (`D2`), `#4` (`D3`) | Limbo `#1` | [room `#2`](../../../areas/wld/theurgist.wld#L25), [room `#3`](../../../areas/wld/theurgist.wld#L34), [room `#4`](../../../areas/wld/theurgist.wld#L43) |

Those seven source stems are not entries in the normal full-world
[`areas/AREA`](../../../areas/AREA#L1) list. Keep these 13 references as
source-only owner questions rather than treating them as routes.

## Typed prototype and code references

Limbo’s low prototype numbers have consumers elsewhere in the active list.
They are numeric references, so their relationship to the local prototypes
needs deliberate ownership review:

| Consumer | Static reference | Limbo source match |
| --- | --- | --- |
| Heavens reset | One `M` command references mobile `#2`; the line comments “mob type not in this .mob file.” | Limbo defines mobile `#2` as the black “durian” dragon ([reset](../../../areas/zon/heavens.zon#L286), [prototype](../../../areas/mob/limbo.mob#L12)). |
| Active ZON resets | Four `G` records and fourteen `P` records reference object `#5`. The source comments call the object absent from each local `.obj` file. | Limbo defines object `#5` as a blank paper note ([Alatorin P](../../../areas/zon/alatorin.zon#L1482), [Alatorin G](../../../areas/zon/alatorin.zon#L2113), [Harrow](../../../areas/zon/harrow.zon#L250), [Player Castles](../../../areas/zon/player_castles.zon#L23), [Tharnadia P](../../../areas/zon/tharnadia.zon#L541), [Tharnadia G](../../../areas/zon/tharnadia.zon#L1321), [Limbo object](../../../areas/obj/limbo.obj#L47)). |
| Active SHP stock | Three shop records list object `#5` in their stock: Alatorin shop `#83137`, Harrow shop `#29429`, and Player Castles shop `#11005`. | The numeric stock ID matches Limbo’s blank paper note ([Alatorin](../../../areas/shp/alatorin.shp#L10), [Harrow](../../../areas/shp/harrow.shp#L81), [Player Castles](../../../areas/shp/player_castles.shp#L14), [Limbo object](../../../areas/obj/limbo.obj#L47)). |
| Compiled mobile specials | `necro_dracolich` is assigned globally to mobile VNUMs `#3`–`#10` and `#1201`. | Limbo defines #3–#6 as its four colored dracoliches; active Heavens defines #7–#10 and #1201, so the binding spans both contributors ([assignment range](../../../src/specs/specs.assign.c#L383), [Limbo prototypes](../../../areas/mob/limbo.mob#L26), [Heavens prototypes](../../../areas/mob/heavens.mob#L1), [Heavens #1201](../../../areas/mob/heavens.mob#L2571)). |
| Compiled object special | `blood_stains` is assigned to object VNUM `#4`. | Limbo’s object `#4` is “some blood.” The procedure changes its description as it ages, sets `ITEM_NOSHOW` after 90 seconds, and suppresses its ordinary decay message ([assignment](../../../src/specs/specs.assign.c#L1311), [procedure](../../../src/specs/specs.limbo.c#L14), [object](../../../areas/obj/limbo.obj#L35)). |

The active QST scan found no giver keyed to Limbo mobile VNUMs `#1`–`#6`, and
the active SHP scan found no keeper in that range. The three stock references
above are the active shop references to Limbo object VNUMs `#1`–`#9` found in
those records. Limbo itself has no local shop or quest file.

Separate unregistered data also reuses these small numbers. A static scan of
unregistered ZON sources found 68 reset-command references to mobile VNUMs
`#1`–`#6` or object VNUMs `#1`–`#9` (30 `M`, 14 `O`, 11 `G`, and 13 `E`
records):

| Source-only ZON | Matching reset records | Example |
| --- | ---: | --- |
| `dalvik.zon` | 9 | [reset records](../../../areas/zon/dalvik.zon#L225) |
| `hostel.zon` | 26 | [reset records](../../../areas/zon/hostel.zon#L153) |
| `Magetower.zon` | 3 | [reset records](../../../areas/zon/Magetower.zon#L26) |
| `mavulsk.zon` | 24 | [reset records](../../../areas/zon/mavulsk.zon#L498) |
| `theurgist.zon` | 3 | [reset records](../../../areas/zon/theurgist.zon#L9) |
| `waterplane.zon` | 3 | [reset records](../../../areas/zon/waterplane.zon#L10) |

Unregistered `dalvik.qst` has giver headers `#2` and `#3`
([`#2`](../../../areas/qst/dalvik.qst#L1),
[`#3`](../../../areas/qst/dalvik.qst#L1263)), and `hostel.qst` has giver `#4`
([header](../../../areas/qst/hostel.qst#L1)). Those files are outside the
active list and do not establish active Limbo consumers. The separate minimal
fixture also defines object VNUMs `#1`–`#9` in
[`areas_mini/mini.obj`](../../../areas_mini/mini.obj); this is a configuration
overlap, not another full-world area owner. No current
unregistered SHP file uses stock objects `#1`–`#9` or keeper mobiles `#1`–`#6`.

The source defines room constants `RROOM_VOID = 0`, `RROOM_LIMBO = 1`, and
`ROOM_LIMBO_VNUM = 1` ([room constants](../../../src/world/vnum.room.h#L4),
[VNUM constants](../../../src/core/utils.h#L635)). Code uses room `#1` as a
recovery destination: artifact repair sends invalidly located artifacts there,
locker cleanup falls back there for invalid room locations, and the innate
resurrection path detects Limbo and returns the character to a previous room
or birthplace ([artifact recovery](../../../src/guild/artifact.c#L1291),
[locker fallback](../../../src/item/storage_lockers.c#L1582),
[resurrection handling](../../../src/classes/innates.c#L1162)). These source
uses support an administrative/system role, but do not prove which player
transitions can place a character there. The disconnect-restoration path also
returns a shapechanged character found in room #1 to its recorded prior room,
or to the hometown if no prior room is available
([restoration path](../../../src/cmd/actnew.c#L1898)). Innate resurrection has
its own prior-room/birthplace recovery branch
([resurrection path](../../../src/classes/innates.c#L1158)). These are code
transitions, not authored WLD edges. The command-aggro guard excludes room
index #1 from its ordinary check ([guard](../../../src/cmd/interp.c#L2459)).

## Tracked history and owner questions

A normalized scan of all 442 current `.zon` titles finds one exact “Limbo”
owner: active `limbo` ([title](../../../areas/zon/limbo.zon#L2)). The local
room #0 title “The Void” ([room](../../../areas/wld/limbo.wld#L1)) is also
similar to the separate, unregistered candidate title “Main Guildhall for The
Void” under `void_gh` ([title](../../../areas/zon/void_gh.zon#L2)). That
candidate is retained under its own source identity; its review records 19
different room-number collisions with active Torg and no tracked predecessor
or whole-zone replacement ([candidate dependency row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L119),
[candidate audit](VOID_GH_AUDIT_REVIEW.md)). This name similarity does not
establish that `void_gh` aliases, replaces, or descends from Limbo. The
command-target/owner question remains open separately.

Followed WLD/ZON history reaches the 2009 `areas/` relocation (`642da5400`)
and a 2013 path-history entry (`71d731c5b`). Later MOB/OBJ commits include the
blue dracolich slowness edit (`fe0541d38`) and blood levitation change
(`9f07fd9d6`). No tracked change identifies a retired Limbo predecessor or
establishes Limbo as a whole-zone replacement. This is a limit of the
available path history, not a claim about untracked or older source material.

Keep the active area in place pending owner decisions on:

1. Whether the nine Desert load-room exits into Limbo are intentional
   administrative destinations, and why that room has no authored return
   direction.
2. Whether Heavens’ mobile `#2` reset and the active reset/shop uses of object
   `#5` intentionally depend on Limbo’s generic low-VNUM prototypes.
3. Whether the unregistered WLD/QST references and the minimal object overlap
   have any intended shared-prototype history that should be preserved in a
   separate source-only record.

No room, reset, prototype, shop, or route data was changed. No live access,
runtime fallback, or persisted-identity outcome was tested.

## Checkpoint 261: builder questions and recommended resolutions

The following questions make the existing owner questions actionable while
preserving the area’s administrative role, the current low-VNUM owners, and
all source-only records until their owners resolve intent.

| # | Direct builder question | Source-backed recommended resolution |
| --- | --- | --- |
| 1 | Should negative-zone `limbo` remain an active system-support contributor despite having no local QST, shop, reset commands, or story sidecar? | Keep the active manifest entry and preserve its non-contract status; do not invent local quest receipts or services from recovery code. |
| 2 | Are room #0 “The Void” and room #1 “Limbo” intended to remain two isolated room records with no authored exits? | Preserve both room identities and the zero-exit map; the editor coordinates do not establish a player route. |
| 3 | Are all nine Desert room #49790 direction slots aimed at Limbo room #1 intentionally? | Keep the nine authored inbound records and identify the owning transition before changing any slot; do not consolidate them as one ordinary passage. |
| 4 | Is the absence of a reverse Desert-to-Limbo edge intentional for the #49790 load-room relationship? | Preserve the one-way source graph until the Desert and Limbo owners identify the intended return mechanism. |
| 5 | Should room #1 remain outside ordinary player travel even though several system paths use it as a fallback or recovery destination? | Keep code-driven recovery separate from player navigation; do not add public access or describe the room as a normal destination without an owner decision. |
| 6 | Should invalid or orphaned tracked artifacts continue to fall back to room #1? | Preserve the existing artifact recovery behavior and qualify saved-item settlement separately; do not convert it into a player-facing artifact source. |
| 7 | Should invalid storage-locker contents continue to use room #1 as their defensive fallback? | Preserve the fallback branch and its ownership boundaries; do not describe it as an ordinary chest or player-accessible room. |
| 8 | Should shapechange-disconnect restoration and innate resurrection continue to return characters from room #1 to their prior room or hometown/birthplace? | Keep those recovery transitions distinct from WLD exits and preserve their separate fallback rules. |
| 9 | Does Heavens’ reset reference to mobile #2 intentionally resolve to Limbo’s black “durian” dragon despite the reset comment? | Preserve the exact cross-area reset reference and record the mobile owner; do not change either VNUM from the comment alone. |
| 10 | Should active resets and three shops continue to reference Limbo object #5, its blank paper note? | Keep each importing reset/shop with its current owner and qualify actual resolver behavior; do not infer a local Limbo shop or quest. |
| 11 | Do comments calling object #5 absent from local files indicate intentional global prototype reuse, or an outdated source assumption? | Retain the authored consumers until each area owner reconciles its comment; do not edit imported records based on comment wording alone. |
| 12 | Is the compiled `necro_dracolich` binding intended to cover global mobile VNUMs #3–#10 and #1201 across both Limbo and Heavens? | Preserve the exact assignment and source ownership: Limbo supplies #3–#6; Heavens supplies #7–#10 and #1201. Do not describe all targets as Limbo mobiles. |
| 13 | Should `blood_stains` remain bound to Limbo object #4 with its timed description and hide behavior? | Preserve the object-special binding and its aging/decay semantics; do not treat the object as a reset stock or accepted story item. |
| 14 | Are the thirteen inactive WLD references from seven unregistered source stems into room #1 historically meaningful? | Keep them recorded as source-only references, not active approaches; resolve any map recovery only with each source owner. |
| 15 | Should the 68 unregistered ZON low-VNUM references and unregistered QST giver headers remain outside the active Limbo dependency set? | Preserve configuration boundaries and maintain them as separate owner questions; do not treat inactive records as active consumers or routes. |
| 16 | Is `areas_mini/mini.obj`’s overlap with Limbo object VNUMs #1–#9 only a separate minimal-configuration definition? | Keep the minimal fixture separate from full-world ownership; do not infer a duplicate full-world zone or renumber prototypes from this overlap. |
| 17 | Are the blank mobile template and unmade treasure/body-part objects intentionally inert prototypes with no local resets or contracts? | Keep their current prototype records and do not claim ordinary availability, stock, or a Limbo story endpoint. |
| 18 | Does the followed same-stem history still provide no evidence for a retired Limbo predecessor or whole-zone replacement? | Preserve the active contributor and its current title/VNUM identity; keep the **DO NOT MOVE** disposition unless new tracked source evidence appears. |

## Checkpoint 261 disposition

Keep active Limbo **DO NOT MOVE**. Preserve both isolated rooms, the nine
one-way active Desert edges, current recovery code, and all imported or
compiled low-VNUM references under their current owners. The full compiled
dracolich range includes Heavens-owned mobiles #7–#10 and #1201 as well as
Limbo-owned #3–#6. Keep the thirteen WLD references and low-VNUM reset/QST
records from unregistered sources distinct from the active graph. No native
area or gameplay source changed.
