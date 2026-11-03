# The Great Shaboath: repository-wide connectivity and dependency audit

**Checkpoint 91; priority 131 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive dossier](../zone-stories/THE_GREAT_SHABOATH.md)
with active WLD topology, all inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Static exits and
code references do not establish ordinary availability, safe travel, or played
qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Great Shaboath has 116 rooms
and 243 authored exits; all 243 resolve. Its internal graph has components of
115 rooms and one room. The isolated room is Shaboath Control Room #32800. Three
active areas contribute inbound edges; one boundary pair is reciprocal:

| Active boundary | Counterpart | Classification |
| --- | --- | --- |
| ConnectorZones #54012 north to Shaboath #32803 | No reverse exit from #32803 to #54012 | Inbound only ([register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L704)) |
| Winterhaven #55633 southeast to Shaboath #32916 | No reverse exit from #32916 to #55633 | Inbound only ([register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L797)) |
| Underdark #710327 north to Shaboath #32919; Shaboath #32919 south to #710327 | Both directions are authored | Reciprocal ([Underdark inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1690), [Shaboath outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L439)) |

There are no negative or unresolved exits, no room-number range mismatches, one
fall-chance field, and no current fields. The unrelated global active-WLD
duplicate room #51800 belongs to Hell Two and Hell Three. Door/reset state,
Winterhaven staging, the isolated room's intended role, route safety, and
successful travel remain separate from this authored graph.

## Inactive WLD sources

The all-95 inactive-WLD scan parsed 2,080,773 exits without issues and found no
source-room VNUM overlap with Shaboath and no source-only approach. The curated
source-only reference total remains 1,093 and the reviewed Surf subset remains
122. Inactive declarations do not add current ingress.

## Typed references and external producers

Typed scans found no external QST item command or SHP product matching a local
Shaboath object. Two active foreign ZON reset groups do reset local objects:
Alatorin resets #32857 and Ravenloft resets #32856 ([Alatorin](../../../areas/zon/alatorin.zon#L2100),
[Ravenloft](../../../areas/zon/ravenloft.zon#L683); both prototypes are defined
in [Shaboath objects #32856](../../../areas/obj/shabo.obj#L590) and [#32857](../../../areas/obj/shabo.obj#L598)).
No external mobile or object prototype definition matched a local Shaboath
VNUM.

Among 872 external type-25 declarations (713 active and 159 source-only), none
targets a Shaboath room. All 231 external story sidecars parsed without errors
and none contains a structured Shaboath room, mobile, or object VNUM reference.

## Compiled numeric references

The namespace-aware source scan found Shaboath VNUMs in the following compiled
contexts. The existing dossier traces the local handlers and their limits; the
raw numeric match alone is not treated as an active route, reward, or objective.

| Source context | Classification |
| --- | --- |
| [Literal assignments](../../../src/specs/specs.assign.c#L1041) | The source dossier inventories 28 enabled literal mobile/object assignments. These bind Shaboath combat, artifact, chest, hazard, and actor procedures; assignment does not prove a particular player effect or completed encounter. |
| [Computed room assignments](../../../src/specs/specs.assign.c#L2253) | Three `assign_rooms` loops bind seven room callbacks in ranges 32907–32909, 32910–32911, and 32901–32902. They are not represented by the literal VNUM assignments. |
| [Shaboath procedures](../../../src/specs/specs.shabo.c#L55) and [mobile helpers](../../../src/specs/specs.mobile.c#L117) | Local room, mobile, helper, and object IDs occur in encounter procedures. The dossier distinguishes active checks from commented code and dormant dispatch. |
| [CHAOS equipment profiles](../../../src/account/chaos_eq_data.h#L83) | Local object #32816 appears in static profile rows. A profile reference does not establish kit eligibility, issuance, or receipt. |
| [Object constant](../../../src/world/vnum.obj.h#L113) | `VOBJ_HOLYSWORD_CHAOS` names local object #32822, which has a local `holy_weapon` assignment. |

These source-context matches complement the external ZON reset findings above.
They do not establish that finite stock is admitted, that transformed or
temporary actors persist, or that a source action was personally completed.

## Title, history, and disposition

The color-stripped title in [shabo.zon](../../../areas/zon/shabo.zon#L2)
normalizes to “The Great Shaboath” and matches exactly one of the 351 active
contributors. Bounded followed WLD/ZON history includes older maintenance and
the 2009 area-tree move; it identifies no distinct predecessor or replacement.
The current un-commented [areas/AREA row](../../../areas/AREA#L159) remains
**DO NOT MOVE**.

The existing dossier maps both local accepted returns, exact proof identities,
services, doors, finite reset supply, custom handlers, and selected outside
sources. This repository-wide scan adds the active boundaries, inactive-WLD
closure, two foreign object resets, and compiled source-context classifications.
Dynamic route conditions, finite-stock admission, effect/custody recovery,
builder intent, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

### Checkpoint 353: refreshed world, history, and typed-dependency reconciliation

This refresh uses the existing source-comprehensive [Great Shaboath dossier](../zone-stories/THE_GREAT_SHABOATH.md) and updates the older Checkpoint 91 repository-wide census. Its inactive-source aggregate is superseded by the current all-95 sweep: 527,791 rooms and 2,080,773 exits parsed without errors, with no Shaboath room-number overlap or source-only approach. The current curated source-only total is 4,504, including 267 Surf and 9 Underworld2 records; Shaboath adds none. Earlier 1,093/122 totals are historical CP91 totals, not current counts.

The active manifest and exact endpoints are retained in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv): ConnectorZones #54012 north → #32803; Winterhaven #55633 southeast → #32916; Underdark #710327 north → #32919; and Shaboath #32919 south → Underdark #710327. The two former edges are inbound only; the Underdark pair is reciprocal. All are authored static exits. They do not prove ordinary availability, safe staging, or successful travel.

The local WLD has 116 rooms and 243/243 loader-resolved exits. Its weak components have 115 rooms and one room; #32800 is isolated and is described as a control room not intended for player access. The other 115 rooms form one strongly connected directed component. From each boundary room #32803, #32916, and #32919, raw authored reachability and returnability are 115/115. There are no negative or positive-unresolved targets, range mismatches, or current fields, and one room carries a fall-chance field.

The WLD has 64 nonzero exit-flag directions, and all 64 match local D reset commands at 100% chance. Their recorded reset-state values are 1×54, 2×7, 6×2, and 13×1. As a conservative static comparison only, suppressing every flagged direction leaves 6 reachable/returnable rooms from #32803, 1/1 from Winterhaven arrival #32916, and 6/6 from Underdark arrival #32919. This is not a live door-state, reset-timing, key, access, or safety result. The [local reset source](../../../areas/zon/shabo.zon#L3) and [room source](../../../areas/wld/shabo.wld) remain authoritative.

The refreshed static typed scan retains two active foreign ZON reset groups that consume local objects: Alatorin resets #32857 and Ravenloft resets #32856. No external QST item command, SHP product, MOB/OBJ prototype definition, or type-25 reset target matched Shaboath. The external type-25 closure covers 872 declarations (713 active and 159 source-only); 231 external story sidecars parsed and none contains a structured Shaboath VNUM. Compiled references remain classified by context in CP91: 28 literal mobile/object assignments, seven computed room callbacks in three assign_rooms loops, Shaboath handlers/helpers, CHAOS equipment profile use of #32816, and VOBJ_HOLYSWORD_CHAOS for #32822. Numeric matches are not treated as player routes, rewards, or completion.

After color stripping, terminator trimming, whitespace normalization, and case folding, “The Great Shaboath” is unique among all 442 current zone-title files and the 351 active contributors. Same-type prototype review found no active room, mobile, or object VNUM overlap; the inactive-WLD sweep found no room overlap. Followed history stays on the shabo stem: WLD through 2013-11-13; MOB through 2026-08-26; OBJ through 2026-09-14; ZON through 2016-06-14; QST history reaches the 2009-08-12 area-tree move; and the story sidecar was added 2026-10-06. No alternate-stem predecessor, distinct prior zone, or explicit whole-zone replacement was found in those bounded histories. Shaboath remains an un-commented areas/AREA contributor; retain it in place.

#### Builder questions and source-backed recommendations

These questions complement the dossier’s 16 detailed ZSQ follow-ups. Findings below describe repository source only; the recommended dispositions preserve the current authored behavior unless a builder confirms a specific change.

| ID | Builder question | Source-backed answer | Recommended resolution |
| --- | --- | --- | --- |
| BSQ-SHABO-01 | Should players enter Control Room #32800? | Its room text says it contains zone procedures and nobody should be able to get there; the graph has no exits from it. | Keep it isolated and inaccessible unless a builder documents a player-facing purpose. |
| BSQ-SHABO-02 | Should the zone automatically repopulate? | The Shaboath reset header records reset mode 0; limited stock and actor lifecycle matter to both local quest chains. | Preserve reset mode; qualify actual stock and reset behavior before proposing any supply change. |
| BSQ-SHABO-03 | Are the city gate and interior gate intended to need their declared keys? | The dossier traces key #32809 and compound key #32811 to their authored gate paths. | Keep both key routes and confirm key availability/staging with the builder before altering either lock. |
| BSQ-SHABO-04 | Should the savant retain trusted or NPC bypasses? | The entrance savant blocks ordinary player directions but source checks permit trusted/NPC bypass behavior. | Preserve the source distinctions; do not turn the bypass into ordinary player access without an explicit design decision. |
| BSQ-SHABO-05 | Should notebook Q41 remain information-only? | Q41 consumes notebook #32812 for information with no item or XP reward and teaches Darlakanand. | Keep the exact accepted return; document phrase learning separately from speech, entry, and an access receipt. |
| BSQ-SHABO-06 | Should the exterior and interior speech phrases be normalized? | The exterior checks Darlakanand while the reverse door keyword is Darkaland. | Ask the builder to confirm this asymmetry; preserve both authored keywords until then. |
| BSQ-SHABO-07 | Should learning the phrase be a hard gate for entering the complex? | Current door speech has no notebook-history or learned-phrase receipt requirement. | Do not add a new quest gate by inference; decide separately whether future access attribution is desired. |
| BSQ-SHABO-08 | Does the spirit require four different tower essences? | Q87 names four exact sphere VNUMs, each from a different tower; four copies of one type do not satisfy the recipe. | Preserve all four identities and source towers; distinguish supplied proof from a personal kill or first recovery. |
| BSQ-SHABO-09 | Should the spirit’s four-sphere quest count as a daily quest? | Q87 is marked story-only and excluded from daily quests; it is independent of notebook Q41. | Keep that exclusion and separate quest chains unless the builder explicitly changes the design. |
| BSQ-SHABO-10 | Does key #32847 expire after use? | Its declared break chance is 100; no wall-clock expiry is implemented in the reviewed source. | Describe the break chance as authored; do not invent a timed lifetime or priest-release completion. |
| BSQ-SHABO-11 | Should necromancy move player corpses as well as other corpses? | Its handler relocates every ITEM_CORPSE to #32912, including PC corpses. | Confirm intended corpse scope and custody/recovery first; do not label the movement a rescue or personal kill. |
| BSQ-SHABO-12 | Should the enchantment hazard affect every room occupant? | The handler returns inside its first occupant iteration, including when that occupant is an NPC. | Reproduce occupant-order cases and get builder intent before changing hazard scope. |
| BSQ-SHABO-13 | What is the intended summoning cap and status condition? | The count check can pass at nine and then create two actors; the status comparison has a suspicious negation. | Confirm cap and status rules, then qualify an isolated fix separately; the audit does not silently repair it. |
| BSQ-SHABO-14 | Should trap or shout helpers grant personal encounter credit? | Directional handlers and helper combat callbacks create effects/assistance, but no corresponding accepted Shaboath quest return is declared. | Record actor, target, source, and result separately; do not infer personal defeat from shared help. |
| BSQ-SHABO-15 | Should butler transformations include servants outside the local encounter? | The transformation helper acts on shared servants, transfers gear, and extracts old actors. | Confirm helper scope and actor/gear custody before any change; keep shared transformation distinct from a player victory. |
| BSQ-SHABO-16 | Should Pallistren’s shared state and duplicate allocation be repaired? | Its question/timer state is function-static and the transformation reads mobile #32847 twice, overwriting the first pointer. | Qualify multiple instances, failure, and restart cases; treat any repair as a separate named change. |
| BSQ-SHABO-17 | Is Petrechella’s standard periodic behavior intentionally dormant? | SET_PERIODIC returns false and nonzero combat dispatch is rejected; the legacy body is not normally dispatched. | Preserve the dormant behavior until intended dispatch and replacement-actor custody are confirmed. |
| BSQ-SHABO-18 | Does the obelisk reveal, unlock, open, or traverse the stair? | PUSH command 270 clears BLOCKED on the secret UP exit; SECRET and CLOSED remain. | Keep switch, reveal, door state, movement, and any completion evidence as separate facts. |
| BSQ-SHABO-19 | Do imported reset objects or chest/holy-weapon stock add a native quest reward? | External Alatorin/Ravenloft resets and local chest/item procedures exist, but no extra accepted QST recipe was found. | Preserve producer dependencies and distinguish stock/custody from QST acceptance or reward. |
| BSQ-SHABO-20 | Are boundary routes safe and normally available from their arrival rooms? | Static graph reachability is broad, but conservative suppression of flagged exits sharply reduces routes; runtime access was not tested. | Confirm staging, reset/door state, return route, and safe traversal in a separate builder/gameplay review. |

This checkpoint changes documentation and the coverage register only. It does not change native gameplay, migrate the area, run tests/builds, start a server, or claim live qualification.

### Checkpoint 752: semantic names and area identity

A fresh normalized scan of all 442 current `.zon` titles again finds one exact
“The Great Shaboath” owner: active `shabo`, separately listed in both the world
and QST rosters as *328–329. The matching `zoneId` in `areas/map/shabo.json`,
the story sidecar's `source_area`, and Pallistren's QST dialogue are attached to
that same `shabo` identity.

The bounded repository-wide name scan found two kinds of external text, neither
of which identifies another Shaboath map: the active Underdark guard's warning
to stay out of the city (also copied in retained `underdark2011` mobile source),
and Surface grid room labels “connector to shabo” at #511781 and #513384. Those
labels belong to Surface WLD records; the active Shaboath boundary register
continues to show only ConnectorZones #54012, Winterhaven #55633, and Underdark
#710327 as authored cross-area edges. No alternate current title or
candidate-map identity emerged. This semantic comparison leaves the separate
command-target alias/owner question open and does not change any attic candidate
plan.
