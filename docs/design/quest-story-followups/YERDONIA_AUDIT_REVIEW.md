# Drustl's Yerdonia Enslaved: repository-wide source audit supplement

This supplement reconciles the [source-comprehensive Yerdonia dossier](../zone-stories/DRUSTLS_YERDONIA_ENSLAVED.md)
with active WLD boundaries, the all-source WLD sweep, typed cross-zone
references, shared-code uses, title and replacement history. Findings come
from static repository sources; they do not establish live access, played
receipts, or persisted player history.

## Active boundaries and local graph

Raxthan has 100 rooms and 227 authored exits. All 227 targets resolve: 224
remain in Raxthan and three leave. The local WLD-only weak graph is one
component of 100 rooms. Each outbound exit has a reverse-direction record:

| Raxthan exit | Active target | Reverse exit |
| --- | --- | --- |
| #42980 east | Surface #595852 | Surface #595852 west to #42980 |
| #42980 south | Surface #596251 | Surface #596251 north to #42980 |
| #42999 south | Underdark #749737 | Underdark #749737 north to #42999 |

The outbound records are listed at
[Raxthan #42980 east](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L579),
[#42980 south](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L580), and
[#42999 south](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L581). Their reverse records
are [Surface #595852 west](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1564),
[#596251 north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1565), and
[Underdark #749737 north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1708). The static
graph does not establish ordinary access, door/reset state, safe return, or
reachability through other travel systems.

## Unregistered WLD source sweep

The all-95-file scan parsed 527,791 rooms and 2,080,773 exits with zero parse
issues. No unregistered source room overlaps Raxthan's room VNUMs. One inactive
`surf.wld` record points north into Raxthan #42980: room #266966, “The
Mountainside of the Footsteps of Ferrik Bloodyfist”
([room](../../../areas/wld/surf.wld#L1405202),
[exit](../../../areas/wld/surf.wld#L1405206),
[target](../../../areas/wld/surf.wld#L1405209)). This is a source-only
reference, not an active entrance. The curated source-only inbound total rises
from 602 to 603.

## Typed dependencies and shared code

The exact external QST item-command scan found one reference: active
`alatorin.qst` gives item #42948 in Ravi Bjornson's recipe
([command](../../../areas/qst/alatorin.qst#L6581)). The Alatorin sidecar
assigns that black Bloodbeast heart to Alatorin's two-item receipt
([contract](../../../areas/story/alatorin.story.json#L3641),
[proof and completion](../../../areas/story/alatorin.story.json#L3645)). Local
prototype #42948 is the Bloodbeast heart
([object](../../../areas/obj/raxthan.obj#L1096)). Keep the Alatorin journal and
receipt ownership foreign; the shared item does not create another Raxthan
completion.

No external SHP product, reset-role match, reset room target, mobile/object
prototype definition, or type-25 portal destination into Raxthan was found.
Among 872 external type-25 declarations, none targets Raxthan. All 231 other
story sidecars parsed without issues; the sole exact structured VNUM match is
Alatorin's #42948 proof. These negative results are bounded to the exact typed
records scanned.

Shared CHAOS equipment tables reference local spiked electrum ring #42903 in
two anti-paladin kits ([standard](../../../src/account/chaos_eq_data.h#L106),
[enhanced](../../../src/account/chaos_eq_data.h#L357)). The shared
`do_shift_prime` code also lists room #42950 in its Githyanki destination
array ([procedure and array](../../../src/classes/innates.c#L2005)). That path
requires the current zone to be Astral and a supported race, then randomly
selects among 15 destination entries
([Astral guard](../../../src/classes/innates.c#L2021),
[Githyanki/Pillithid branch](../../../src/classes/innates.c#L2033),
[Illithid branch](../../../src/classes/innates.c#L2054)). Room #42950 is
therefore a possible guarded arrival, not a WLD edge, guaranteed destination,
or Raxthan quest prerequisite.

## Title, tracked history, and disposition

The active manifest lists Raxthan as `*429`
([AREA entry](../../../areas/AREA#L203)); its normalized ZON title is
“Drustl's Yerdonia Enslaved”
([header](../../../areas/zon/raxthan.zon#L2)). The normalized title is unique
among 351 active areas, and the curated
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no
Raxthan entry. The global active-WLD duplicate scan found only #51800 in Hell
Three and Hell Two, not in Raxthan.

Tracked source paths reach the 2009 area-tree move `642da5400`; later WLD/MOB
history includes the 2012 area edits `2b50c75aa`, and the OBJ/ZON paths include
later 2015–2016 edits and `c8b1fa45f` on 2026-04-18. The story sidecar was
added at `7ae9fbdb8` on 2026-10-05. The bounded path/register review identifies
no distinct replacement source. Raxthan remains an active `areas/AREA`
contributor, so its disposition stays **DO NOT MOVE**.

## Recommended reconciliation

- Preserve Alatorin's foreign receipt ownership for Bloodbeast heart #42948.
- Preserve #42950 as a guarded, randomized `shift_prime` possibility; do not
  describe it as a deterministic route or physical Raxthan boundary.
- Builder question: Is the inactive `surf.wld` Ferrik mountainside edge to
  #42980 intended as retained alternate-map source, or is it a stale source
  record? Keep it outside active ingress counts until its status is clarified.
- Preserve Raxthan's active manifest membership. This audit supports no attic
  move or native gameplay repair.

This is a bounded static source audit. Editor/tooling consumers, indirect code
paths, persisted player data, and live access, combat, service, and return
journeys remain outside its scope.

## Checkpoint 338: Raxthan reset-aware access and builder decisions

The existing [source-comprehensive story dossier](../zone-stories/DRUSTLS_YERDONIA_ENSLAVED.md)
and this bounded dependency/history audit already cover ten independent native
endpoints, the Alatorin-owned heart receipt, shared item-switch behavior,
conditional fall handling, the guarded `shift_prime` option, and Raxthan's
distinct active identity. This checkpoint adds current reset-aware WLD reach,
reconciles the older inactive Surf edge with the existing Surf candidate, and
adds direct builder questions without changing the area source.

### Active arrival and reset-aware reach

Raxthan has 100 rooms and 227/227 resolving exits: 224 local and three outbound
in one weak component. Its three active inbounds are reciprocal. The two Surface
roots enter the same room; the Underdark route reaches the lower shaft:

| Active inbound | Local arrival | Raw directed reach | Strict post-reset reach |
| --- | --- | ---: | ---: |
| Surface #595852 west → #42980 | #42980 | 100/100 | 90/100 |
| Surface #596251 north → #42980 | #42980 | 100/100 | 90/100 |
| Underdark #749737 north → #42999 | #42999 | 100/100 | 94/100 |

The corresponding local outbounds are #42980 east/south to Surface #595852 and
#596251, and #42999 south to Underdark #749737. Their reverse records are
verified in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L579).

The four ZON D resets apply to distinct local doors:

| Source room and direction | Reset | Initial interpretation |
| --- | --- | --- |
| #42940 south | state 1 | closed, unlocked |
| #42945 north | state 1 | closed, unlocked |
| #42943 north | state 8 | open, blocked |
| #42944 south | state 1 | closed, unlocked |

There are no duplicate D-reset directions. The D handler applies these bits in
[`db.c`](../../../src/world/db.c#L7857). The strict walk excludes closed or
blocked exits before any player action; it does not simulate OPEN, symbol
touches, falling, reset timing, or the race/zone guard on `shift_prime`. From
#42980 it omits #42944–#42949 and #42996–#42999; from #42999 it omits
#42944–#42949. The closed/unlocked exits may be opened, and the blocked north
exit has a separate symbol control, so these are baseline counts rather than
absolute reachability claims.

The WLD declares seven fall chances: 69 at #42969, 83 at #42982, 53 at #42986,
81 at #42993, 9 at #42995, 18 at #42996, and 33 at #42997. No current is
declared. These fields and static exit walks do not establish safe descent,
landing, or return.

### Current inactive-WLD and retention reconciliation

The current `areas/wld/` inventory contains 446 files: 351 active and 95
inactive. All 95 inactive files were parsed without errors. No inactive file
reuses a Raxthan room VNUM. The only approach remains inactive [`surf.wld` #266966 north → #42980](../../../areas/wld/surf.wld#L1405206).
Checkpoint 68 already counted this non-overlapping source-only edge. It remains
map evidence rather than an active entrance; preserve it on the existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Current
Surf active references (145), reviewed source-only subset (253), and curated
source-only total (4,489) are unchanged.

The coverage row's SHP inventory fields were also stale: the checked-in
[Raxthan shop source](../../../areas/shp/raxthan.shp#L1) contains one shop,
#42963, matching the dealer mobile and keeper room recorded in the dossier.
The corrected shop bounds are #42963–#42963 with one record.

### Twenty builder questions and recommended resolutions

These questions carry the existing story map into owner decisions. The
recommendations retain current accepted items and area ownership until a
separately qualified change is selected.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| RY-01 | Should the ten exact Q/QA receipts remain ten independent endpoints? | Preserve every current input, quantity, reward, and recipient. Do not combine shared item themes into a single campaign completion. |
| RY-02 | Should Drustl's two requests remain alternatives even though either retires him? | Preserve both receipt identities and current retirement behavior; do not promise both acceptances from one same-visit recipient instance. |
| RY-03 | Are Trosat's nine nested strange mushrooms intentionally distinct from the seven loose common mushrooms? | Keep item identity, location, and exact count separate. Do not substitute Grobklarn's common-shroom bundle. |
| RY-04 | Should Dravkult's caption saying “two heads” remain when QA194 requires Graskal's head plus Noctule's heart? | Confirm the intended wording with the area owner, then correct prose in a separate named change if approved; preserve the native pair of inputs. |
| RY-05 | Should Noctule's heart remain accepted by both Dravkult and T'rin, with its separate Alatorin use? | Preserve the alternative consumers and separate their receipt owners. One physical heart cannot settle multiple consume-once outcomes. |
| RY-06 | Should Ravi Bjornson's Alatorin contract keep ownership of its Bloodbeast-heart receipt? | Keep the foreign QST and sidecar outcome in Alatorin; Raxthan's heart prototype does not create another local completion. |
| RY-07 | Are Noctule and the smaller slug intended to carry the later proof items after their F resets? | Preserve current follower creation and exact carried-item owners; do not attribute their proof items to the leaders. |
| RY-08 | Should Volgk's “Agama may be spared” narration remain separate from the tongue's native G stock? | Keep the item-only acceptance. Do not award a spared-life outcome without a builder-defined event and persisted state. |
| RY-09 | Should Epolon's dragon-morph narration remain text without a changed creature or world state? | Preserve the current endpoint and wording until an explicit transformation behavior and outcome are designed. |
| RY-10 | Should Drustl's and Dravkult's Yerdonia-freedom narration remain separate from a rescued-world state? | Keep current story text descriptive; do not infer liberation, rescue, or NPC replacement from item delivery. |
| RY-11 | Is T'rin's first/second-heart wording intended as flavor for one atomic two-item offering? | Preserve the current simultaneous bundle; do not create an ordered deposit stage without a selected design. |
| RY-12 | Should Grobklarn continue to require three exact common mushrooms when his potion is also shop stock? | Keep the three-item QST bundle distinct from purchase or possession; qualify shop availability separately. |
| RY-13 | Should Rethorn continue to request Morklar's scales rather than Morklar's heart? | Preserve the exact native scales input and avoid adding a kill or revenge predicate from the surrounding story. |
| RY-14 | Should chaos-kit ring #42903 and guarded `shift_prime` room #42950 remain separate code-owned references? | Preserve their current shared-code owners; do not represent either as a local QST or guaranteed physical entrance. |
| RY-15 | Should the dealer's one shop record retain its imported stock, and what is the intended fate of disabled object #6122? | Preserve shop #42963 and its authored products. Confirm #6122's intent before a separate cleanup; do not invent a replacement prototype or promise stock readiness. |
| RY-16 | Is the second symbol at #42944 intended to remain redundant after the first symbol clears the blocked north exit? | Preserve both type-29 objects and current switch code until the owner confirms intent; the switch does not open the closed return door. |
| RY-17 | Should the slab passage #42940 south ↔ #42945 north remain described as hidden when file mask 5 and reset state 1 load closed/unlocked, not secret? | Resolve the text/behavior mismatch with the owner; do not invent a SEARCH requirement or silently change reset state. |
| RY-18 | Are the current reset door states and entry-specific strict reaches intended? | Keep the three reciprocal arrivals and four D rows; qualify keys, OPEN, switch use, and travel independently before claiming ordinary access. |
| RY-19 | Are the seven fall chances intended hazards, and what return routes should be qualified? | Preserve the recorded chances and room ownership. Do not label a descent safe or require it for story completion without movement evidence. |
| RY-20 | Should the inactive Surf mountainside exit #266966 remain in the alternate map source? | Keep it on Surf's existing RETAIN row 97 as source-only evidence; do not count it as active ingress or use it to clear a move. |

This checkpoint changes documentation and inventory metadata only. No native
WLD, ZON, MOB, OBJ, QST, SHP, sidecar, membership, or gameplay behavior changed.
No build, test, server, database, migration, or gameplay session was run.

## Checkpoint 822: current typed and WLD-source reconciliation

A fresh scan of the current `areas/wld` inventory finds 446 files: every one of the 351 active area stems has a WLD file, and 95 files are inactive. The 95 inactive files parse without errors, cover 527,791 rooms and 2,080,773 exit records, contain no Raxthan room-number overlap, and contribute the same sole source-only approach: [`surf.wld` #266966 north to Raxthan #42980](../../../areas/wld/surf.wld#L1405206). The two additional repository `.wld` files outside `areas/wld` (`areas/mob/astral_main.wld` and `areas_mini/mini.wld`) add no Raxthan target. The prior “97 inactive files under `areas/wld`” count was stale; the room, exit, parse, and source-only approach findings remain unchanged.

The current graph still has 100 rooms, 227/227 resolved exits, and three reciprocal active boundary pairs: Raxthan #42980 east/south to Surface #595852/#596251, and #42999 south to Underdark #749737. The source-only Surf approach remains separate from these active boundaries.

A bounded typed refresh confirms 64 local mobiles, 59 objects, and 100 rooms, with no same-type mobile/object prototype duplicate. The 442-zone ZON scan finds no external reset token in #42900–#42999. The only external QST item command remains Alatorin `G I #42948` at line 6581; among 231 other story sidecars, only Alatorin references that VNUM, and all 232 sidecars parse. A scan of 720 SHP shop headers finds no external Raxthan product or keeper match. No external WLD door-key field references a Raxthan object.

The all-object scan covers 21,682 records, including 872 type-25 teleports and 380 type-29 switches. No type-25 fixed destination or random-zone seed targets Raxthan. The only type-29 target-room fields in range are local symbols #42954 → #42943 and #42958 → #42944; no external switch targets a Raxthan room. One nonstandard sentinel record in `end.obj` (#9999999) has no type/value record and is outside the Raxthan range. The exact C/C++ VNUM scan still finds only CHAOS kit object #42903 at two table entries and guarded `shift_prime` room #42950; no other direct Raxthan VNUM literal was found.

These checks reconfirm CP338’s source findings and do not qualify installed editor/tooling consumers, indirect or runtime behavior, persisted player data, shop/spawn availability, or safe traversal. Preserve active Raxthan **DO NOT MOVE**, the foreign Alatorin receipt ownership, and the existing Surf candidate’s **RETAIN IN PLACE** approach. No candidate or native change is warranted by this refresh.
