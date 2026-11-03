# The Temple of Blibdoolpoolp repository-wide source audit

Checkpoint 208 maps active kuotoa against its authored environmental story, active and inactive WLD boundaries, reset-aware access, typed dependencies, title ownership, and followed source history. This is a static source audit. It does not inspect a live server, player state, database, editor, persisted world state, or actual gameplay. The [source dossier](../zone-stories/KUOTOA.md) summarizes the local story and builder questions.

## Source identity and authored content

The active area list identifies kuotoa with range note *882. Its zone header is #882, titled The Temple of Blibdoolpoolp, with last room #88236 and reset mode 2. The WLD contains 37 rooms #88200–#88236, the MOB contains six prototypes #88200–#88205, and the OBJ contains seven prototypes #88200–#88206. Local resets reference every local mobile and object. The source appears in both areas/AREA and areas/qst/AREA, but there is no current kuotoa.qst, SHP file, story sidecar, or native Q/QA contract ([area list](../../../areas/AREA#L310), [zone header](../../../areas/zon/kuotoa.zon#L1), [QST index](../../../areas/qst/AREA#L276), [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151)).

The authored setting is an underground Kuo-Toa settlement approached through damp green stone passages. It has a large clear pool, carved abodes, a temple vestibule and altar, and a disturbing cavern occupied by insane Kuo-Toa. Reset populations include guards, worshipers, a resting Kuo-Toa, a huge monitor, common residents, and insane Kuo-Toa. Local objects are treasure, a coral torc, a jagged sword and net, and shark-skin and coral equipment. No local request or native contract defines an objective or outcome ([rooms](../../../areas/wld/kuotoa.wld#L1), [resets](../../../areas/zon/kuotoa.zon#L122), [mobiles](../../../areas/mob/kuotoa.mob#L1), [objects](../../../areas/obj/kuotoa.obj#L1)).

The zone title spells the deity name “Blibdoolpoolp.” Local altar #88227 instead uses “Blipdoolpoolpoo” and calls it a Kua-Toan deity. Gold Hall room #40589 uses “Blipdoolpoolpoo” in its altar title and “Blipoodoopoo” in the description. Underworld room #4457 describes an altar and a large image of the Kuo-Toan god but gives no name. Underworld #4454 directly exits east to kuotoa #88200 and south to its own shrine #4457. This establishes a neighboring WLD route and parallel religious text, not a replacement or a proven shared settlement identity ([local altar](../../../areas/wld/kuotoa.wld#L459), [Gold Hall altar](../../../areas/wld/gold_hal.wld#L3958), [Underworld entry boundary](../../../areas/wld/underworld.wld#L1720), [Underworld shrine](../../../areas/wld/underworld.wld#L1797)).

Room #88228 is an authored anomaly: its generic abode description and local connection match nearby rooms, but its raw room flags and sector are 176197704 and SECT_CITY (1), while the other 36 rooms use flags 33558536 and SECT_UNDRWLD_INSIDE (15). No mobile reset targets #88228. These source values merit builder review; this audit does not infer whether they are wrong ([room source](../../../areas/wld/kuotoa.wld#L470), [sector constants](../../../src/core/defines.h#L459), [Underworld-inside constant](../../../src/core/defines.h#L473)).

## Active WLD graph and reset-state reach

The local WLD has 75/75 resolving exits: 74 local and one outbound. Its local raw graph has one weak component; all exits available, #88200 reaches all 37 rooms. Active Underworld #4454 exits east to #88200. The local entry #88200 exits west to #4454, so the only active boundary is reciprocal: one inbound, one outbound, one reverse pair ([Underworld edge](../../../areas/wld/underworld.wld#L1729), [local edge](../../../areas/wld/kuotoa.wld#L1)).

The zone declares 28 unconditional directional door resets across 14 local pairs, and each pair starts closed with no key VNUM. The source pairs are:

- #88203 east / #88213 west; #88203 west / #88214 east.
- #88204 north / #88215 south; #88204 east / #88216 west; #88204 west / #88217 east.
- #88207 west / #88231 east.
- #88208 east / #88219 west; #88208 west / #88218 east.
- #88209 east / #88220 west; #88209 west / #88221 east.
- #88210 east / #88222 west; #88210 south / #88211 north; #88210 west / #88223 east.
- #88212 east / #88224 west, the double doors to the temple entrance.

With reset-closed exits excluded, 46/74 local directed exits remain open. The strict graph has weak components sized 14, 6, 4, 2, and eleven single-room components. Directed reach from #88200 is 14/37: #88200–#88210, #88212, #88229, and #88230. This is a source graph result, not proof of command success, live door state, survival, or return. No local WLD fall-chance or current fields are authored ([door resets](../../../areas/zon/kuotoa.zon#L8), [temple doors](../../../areas/zon/kuotoa.zon#L112)).

## Inactive-WLD approaches and attic evidence

The current WLD inventory has 446 files: 351 active area WLDs and 95 unregistered WLDs. Exact room-target scanning found one external WLD edge, the active Underworld exit #4454 east to #88200. It is paired with the local west exit. The scan found no additional active boundary, inactive source room overlapping #88200–#88236, or source-only exit targeting kuotoa. No attic candidate row or source-only total changes.

## Typed dependency reconciliation

All 32 local M reset commands and five E plus two G commands reference the six local mobiles and seven local objects. Repository-wide source scans found:

| Consumer scan | Result |
| --- | --- |
| External ZON reset roles | No outside room, mobile, or object reset target among the 441 other current ZON files. |
| External QST numeric references | No exact local room, mobile, or object VNUM among 265 other current QST files. |
| External SHP products and references | No local object reference among 180 other current SHP files; kuotoa has no local shop. |
| External story-sidecar VNUMs | No exact local room, mobile, or object VNUM among 232 current story sidecars. |
| External MOB/OBJ definitions | No duplicate local mobile or object definition among 445 other MOB and 441 other OBJ files. |
| WLD door-key fields | No local object VNUM is used as a door key in any current WLD file; kuotoa has no keyed door field. |
| ITEM_TELEPORT / ITEM_SWITCH room targets | No external destination targets a local room among 872 teleport and 380 switch prototypes; kuotoa has no local teleport or switch object. |
| C/C++ direct numeric references | No exact local room, mobile, or object VNUM was found under src/. |

These are bounded source scans, not claims about editor aliases, server memory, databases, persisted references, or runtime behavior.

## Title, followed history, and disposition

After color stripping and normalization, The Temple of Blibdoolpoolp is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON histories retain the kuotoa stem through the 2009 area-tree move commit 642da5400 and later same-stem edits; a 2016 MOB edit sets the Kuo-Toa race. No alternate-stem predecessor, retirement, or whole-zone replacement is established. Gold Hall’s altar name and Underworld’s neighboring shrine are authored parallels, not zone ownership evidence.

Keep active kuotoa DO NOT MOVE. Preserve its reciprocal Underworld boundary, all room and prototype VNUMs, and the 14 reset-closed door pairs. No inactive-WLD candidate or migration clearance is added. See the [title and history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-208-kuo-toa-temple-title-and-history), [coverage row 151](ZONE_COVERAGE_REGISTER.csv#L151), and [source dossier](../zone-stories/KUOTOA.md).

## Builder follow-up

1. Confirm the canonical deity spelling and whether Gold Hall’s similar altar refers to the same deity or tradition.
2. Clarify whether kuotoa is part of the same settlement as Underworld’s shrine #4457, while preserving their separate source identities.
3. Review room #88228’s flags and sector against its generic abode description and neighboring rooms.
4. Confirm the intended access and reset state for all 14 closed door pairs, especially the temple double doors.
5. Decide whether a local story or objective is intended; current indexes and source files provide no local QST or native Q/QA contract.

## Builder questions and recommended resolutions

The earlier follow-up list identifies five broad builder decisions. These direct questions make the source-backed choices reviewable while keeping the temple's current rooms, religious text, reset values, and lack of a local contract intact.

| Question | Source evidence | Recommended resolution |
| --- | --- | --- |
| Which spelling should the local altar use for Blibdoolpoolp? | The zone title says “Blibdoolpoolp”; altar #88227 says “Blipdoolpoolpoo”; Gold Hall's altar and description use two other spellings ([source identities](#source-identity-and-authored-content)). | Confirm the deity's canonical name with the source owners. Preserve the checked-in wording until they decide whether and where to correct it; do not merge similar spellings automatically. |
| Does Gold Hall's similarly named altar belong to the same deity or tradition? | Gold Hall has parallel altar language, while the sources do not identify shared ownership or a common settlement ([source identities](#source-identity-and-authored-content)). | Keep both source identities separate. Record a shared religious reference only after the builders confirm the lore relationship. |
| Is Underworld shrine #4457 part of this temple's story or only a neighboring location? | Underworld #4454 enters the Kuo-Toa Temple at #88200 and also exits south to its own shrine #4457 ([Underworld context](../zone-stories/KUOTOA.md#green-stone-passages-and-the-temple)). | Preserve the reciprocal entry pair and separate shrine VNUM. Do not imply a mandatory story sequence or shared settlement without an authored owner decision. |
| Should room #88228 use city flags/sector despite its matching abode description? | The room has flags 176197704 and SECT_CITY, while the other 36 rooms use Underworld-inside flags/sector; no mobile reset targets it ([anomaly](#source-identity-and-authored-content)). | Confirm whether its classification is deliberate before editing. Retain the current values until a builder resolves the conflict. |
| Should all fourteen paired doorways reset closed with no named keys? | Twenty-eight directional D resets close fourteen local pairs, including the temple entrance double doors ([reset-aware graph](#active-wld-graph-and-reset-state-reach)). | Review each pair's intended party access and route order, then retain or amend exact reset values as a separate decision. Do not infer a missing key from the current zero-key fields. |
| Are the closed double doors from #88212 to #88224 meant to gate the altar? | The pair is closed in reset data; strict reset-open WLD reach from Underworld arrival #88200 reaches 14 of 37 rooms ([temple rooms](../zone-stories/KUOTOA.md#green-stone-passages-and-the-temple), [graph](#active-wld-graph-and-reset-state-reach)). | Keep the raw and strict counts distinct. Confirm the desired temple route, intended opening mechanism, and return before describing access as intended or safe. |
| Is a local quest or completion objective intended for the temple? | There is no active local QST file, SHP, story sidecar, or native Q/QA contract ([source inventory](#source-identity-and-authored-content)). | Treat the zone as an authored environmental story map until a builder specifies a contract owner, actors, exact prerequisites, accepted outcomes, and recovery behavior. |
| Should guards, worshipers, or insane Kuo-Toa grant a particular kill or rescue outcome? | Reset populations establish possible placements only; no local contract records those outcomes ([environment and reset evidence](../zone-stories/KUOTOA.md#reset-population-and-objects)). | Do not add kill, rescue, or religious-credit stages from descriptions or reset counts. Require a separately authored objective and durable receipt if one is wanted. |
| What should the disturbing cavern's down exit and terminal hole mean to the story? | Rooms #88231–#88236 contain the insane Kuo-Toa; the described slime-covered hole has a down exit to #88235 ([cavern rooms](../zone-stories/KUOTOA.md#green-stone-passages-and-the-temple)). | Preserve the authored connection and prose. Confirm any intended destination, episode, or safety rule before adding a route or completion claim. |
| Which access, stock, and survival claims need play before being treated as outcomes? | Static WLD/reset scans do not establish live door state, current population, supplies, actor survival, or safe return ([graph and audit limit](#active-wld-graph-and-reset-state-reach)). | Keep those claims unqualified until actual routes, current placements, and return/survival are observed; do not infer gameplay from source declarations. |
