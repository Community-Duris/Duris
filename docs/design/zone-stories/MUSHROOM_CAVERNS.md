# Mushroom Caverns: comprehensive source map

Reviewed October 4, 2026. Zone 241, mushroom_caverns; roadmap priority 61.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligation recovery is separate.

The [schema-three journal](../../../areas/story/mushroom_caverns.story.json)
explains all three owned native contracts as three named outcomes, with ten
contacts and five optional preparation checks. It also covers the forest,
abandoned mines, river/pool routes, aboleth habitat and mining-disaster lore.
The intended legacy amulet progression has concrete source and accounting
blockers. Authored coverage and potential daily classification do not establish
a currently playable or renewable campaign.

## Reviewed evidence

- All sixteen owned blocks in [shared quests](../../../areas/qst/mobs_underdark.qst):
  thirteen M, two Q and one QA. The generated inventory indexes eleven M families
  because its identifier filter drops whole families containing punctuation.
  The journal supplies all twenty-two representable aliases, including valid
  aliases from the mixed house-name family. Literal `haz'on'wyz` and `sa'zarn`
  remain explained in prose; the current topic schema cannot encode them.
- All 132 [rooms](../../../areas/wld/mushroom_caverns.wld), 24101–24237 except
  24201–24204 and 24224: 74 exact prose groups, eleven headers, twelve non-exit
  metadata groups, 75 numeric exit families and four exit-description/keyword
  groups. Full prose, metadata, direction/target membership and exit text were
  reviewed. Registry range is 24016–24237; [header](../../../areas/zon/mushroom_caverns.zon#L3)
  24237 2 0 40 50 1 retains reset mode two.
- All eight local [mobiles](../../../areas/mob/mushroom_caverns.mob), nine local
  [objects](../../../areas/obj/mushroom_caverns.obj) and 54
  [resets](../../../areas/zon/mushroom_caverns.zon), in 33 exact families:
  M39/O8/D4/E2/F1. All twelve imported physical mobile prototypes, three imported
  quest actors, foreign bracelet source/actor and seven native input/output kinds
  were traced. No local shop file or literal local special assignment was found.
- The nineteen physically reset mobile kinds have no active native Q/M block.
  Imported piercer 4530 is bound to the actual shared
  [procedure](../../../src/specs/specs.underworld.c#L1614). Absence of a local
  assignment does not exclude imported, type, flag or shared behavior. Imported
  Haz’on’wyz has ACT_TEACHER and receives the shared teacher automatically.
- Bounded active reset/producer/consumer/shop scans cover the legacy actors and
  all seven item kinds. No foreign legacy item consumer or additional native
  half producer was found. All local reset references and ordinary room targets
  resolve. No local teacher or inn/arena flag supplies another quest terminal.
- Five incoming and five outgoing ordinary boundary edges were checked against
  their actual Khildarak/Underdark rooms. All 713 active teleport-object prototypes
  were scanned; no foreign fixed destination into these physical rooms was found.
  Dynamic/admin travel and played accessibility remain unqualified.
- Shared addressed quest dispatch, exact bundle/root allocation, credit ownership,
  item versus currency rewards, active-accounting item policy, money pickup/drop,
  portal selection, effective door resets, movement/falling and follower parenting
  were reviewed. The [generated index](../../reference/zone-story-audits/mushroom_caverns.md)
  preserves exact owned sources and classification.

## Exact amulet progression

| Recipient/block | Exact offering → outcome | Treatment and current limit |
| --- | --- | --- |
| Haz’on’wyz 24021, Q25 | I1515 → I24013; D1 | Stolen-half exchange. Offering prototype 1515 is absent from active data; no declared source. |
| Ozman 24022, Q89 | I4660 → I24014 + C150000; D1 | Bracelet exchange. Currency is a 150-platinum reward, not an input fee. Actor has no active reset. |
| Kryz 24023, QA127 | I24013 + I24014 → I24016 + I24018 + I24017 + E35000; D1 | Two different halves, one final bundle. Actor has no active reset; money-typed half prevents durable acceptance. |

The first two exchanges remain independent intermediate outcomes; the final
exchange is one two-root bundle rather than two delivery achievements. Both
half kinds display “a half of an ancient amulet.” Two copies of either cannot
substitute for one of each. Earlier producer receipts are optional guidance;
supplied exact proof does not require this player's personal assassination,
infiltration, house membership, original recovery or earlier exchange.

All thirteen raw keyword families were read. Haz’on’wyz explains Draknah/master/
weakness; Ozman discusses houses, rejects other houses, describes Zarbonesti,
asks for Bregnar's bracelet and denies knowledge of Haz’on’wyz; Kryz explains
his hunt and the stolen amulet. The native handler checks actual addressed
recipient, mutual visibility, wakefulness and combat state, then publishes an
encounter after a matching response. It does not enforce house membership,
record learned topics, select an exclusive allegiance branch or punish telling
Ozman about Haz’on’wyz. Greeting refusals are dialogue, not separate completion.
QA changes response audience; it does not prove all observers completed a campaign.

Haz’on’wyz's ACT_TEACHER automatically installs the shared
[teacher](../../../src/classes/epic_skills.c#L424). For CMD_ASK containing `level`
and a matching player class it describes one/all live next-level runestone
locations; it grants no item, lesson or native completion. Its actor special is
dispatched before the separate qst_func. Unlike the addressed native M path,
this shared function does not resolve the named recipient or mutual visibility.
It also appends live zone names into a fixed 512-byte buffer with unchecked
strcat. These are concrete recipient and bounded-output hardening leads, not a
reproduced live crash or a repaired teacher. No journal lesson credit is added.

The three contracts currently retain Mushroom credit ownership by giver range,
despite declaration in shared Underdark files. Runtime continuation capture uses
the same range owner independently of physical room. D1 retires the selected
recipient after the accepted exchange; flame/shadow/magic prose is not an actual
new portal or a durable rescued-house state. Catalog repeatability/potential
daily metadata uses owner reset mode two and currently does not inspect missing
prototypes, physical actor availability or money-object admission.

## Physical actors and exact supply

| Required/source fact | Reviewed declaration | Implication |
| --- | --- | --- |
| Haz’on’wyz 24021 | Sole [M100/cap one](../../../areas/zon/mobs_underdark.zon#L567) at room 24015 | No local cavern placement; shared loading/dispersal area reset mode two. |
| Magma Lake Disp 24015 | [Five exits](../../../areas/wld/mobs_underdark.wld#L443) to 329339, 329042, 332030, 331125, 332935 | All five destination rooms are absent from active world. Ordinary dispersal is broken in the audited source; other travel is unqualified. |
| Ozman/Kryz 24022/24023 | Prototypes and native contracts exist; no active M/F placement or literal custom assignment | Do not manufacture local spawns or promise roaming availability from prose. |
| Goblet 1515 | No active object prototype, reset, native producer or shop mention | First recipe cannot use an ordinary active source. A same-number mobile/room is not this item. |
| Bracelet 4660 | [E100/cap one, slot 15](../../../areas/zon/underworld.zon#L275) on Bregnar 4680 at 4525 | Current proof is an exact loose bracelet. His body/corpse and access need separate source/transfer/survival evidence. |
| Half 24013 | Haz’on’wyz Q25 only; ITEM_TREASURE | Missing goblet blocks normal native production. |
| Half 24014 | Ozman Q89 only; ITEM_MONEY, values [0,100,0,0,…] | Currency representation conflicts with quest-proof intent. It is not an ordinary durable offering. |
| Seal 24016, scepter 24017, flail 24018 | Kryz QA127 only in bounded native/reset scan | Three distinct reward kinds under one outcome; not another historical quest stage. Administrative chaos gear tables are not personal quest-source proof. |

The [item policy](../../../src/item/item_command_policy.c#L10) excludes
ITEM_MONEY. The [durable quest bundle](../../../src/world/quest.c#L1517) requires
every selected root to use that policy. Starting with ordinary half 24013 finds
the recipe but rejects money half 24014; starting with 24014 falls through to
the active-accounting guard. The finale is therefore unsupported under current
active-accounting execution even though its static goal kinds are both I.
Preserve the guard; do not call a synthetic model receipt a live successful offer.

Ordinary money pickup routes through committed coin settlement; these values
represent 100 silver/1,000 copper when the wallet can accept them. Money drops
can merge piles and erase the individual quest-proof identity. Ozman's I reward
goes through item-grant recovery, whose money-object compatibility also needs
qualification. Decide intended proof type before any corrective data change,
including existing-object/version compatibility and recovery of pending rewards.

## A different, active Underdark campaign

The similarly named actors [700034–700036](../../../areas/mob/underdark.mob#L516)
and [Kryz's contracts](../../../areas/qst/underdark.qst) are separate identities.
Haz’on’wyz and Ozman each have M50/cap one at 847218 with first half 700000 and
second half 700001 equipped in slot 3; Kryz has M100/cap one in the same
[reset family](../../../areas/zon/underdark.zon#L978). Kryz accepts those two
halves for seal 700005 and E250000, stays, and separately accepts roper skin
700008 for C100000. His addressed prose says fifty platinum, while that reward
is one hundred; this foreign mismatch is a bounded review lead for the Underdark
dossier, not a shipped repair or an additional Mushroom binding.

The modern seal has the same displayed name as legacy 24016.
[Winterhaven Lancer](../../../areas/qst/wh.qst#L2612) requires 700005, not 24016.
Modern halves are armor objects rather than the two legacy types. Native
identities, credit owners and receipts must remain separate. No automatic
same-name replacement, cross-zone credit migration or substitute proof is added.

Draknah 500231 actually has golden goblet 500119 in slot 18 at Surface loading
room 660003. That goblet is a drink container, not legacy I1515. It is a plausible
builder investigation lead, not evidence that changing the old recipe to it is
correct. The foreign loading room's accessibility/renewal remains outside this
local qualification.

## Cavern routes and quest-like lore

- Khildarak edges 24108 north↔17746 south and 24188 up↔17000 down resolve.
  Wider Underdark edges 24101 down↔820288 up, 24187 down↔823870 up and
  24237 down↔825082 up also resolve. No historical access receipt is invented.
- Fallen-mushroom passage 24144 south↔24168 north has world kind/reset 9:
  closed and blocked, not a missing key. No local switch or native qc_unblock
  clears it. Confirm permanent obstacle versus intended clearing puzzle before
  any repair. The route network offers other paths; blocked does not prove the
  entire zone inaccessible. Rocks 24148 east↔24151 west reset 1, closed/unlocked.
- Pools 24103 at 24108 and 24104 at 24102 are reciprocal ITEM_TELEPORT destinations,
  CMD_ENTER 7 and unlimited -1 charges. Object 24104 is floating (8192), not
  hidden/secret. Its “returning … impossible” text conflicts with the declared
  reverse portal; qualify actual targeting/return and builder intent first.
- Pools 24108 at 24218 and 24109 at 24229 similarly connect the lake/river.
  Both have ITEM_NOSHOW; the latter also floats. Their `lake`/`pool` aliases need
  actual generic-find selection tests. Ordinary list lookup explicitly permits
  NOSHOW by keyword; this flag does not prove these pools unusable. A matching
  carried/equipped object can win lookup before the room portal. Nonnumeric look
  or attempted entry does not prove travel. They are not automatically triggered
  by arrival.
- Shaft rooms 24206/24207/24208 use no-ground sector 18 and F30/F30/F100.
  Underwater flags protect the distinction between aquatic routes and dry
  scenery; shared actual falling/breathing/arrival needs surviving-player tests.
  No local room uses SECT_LAVA 39: river-of-lava scenery and demon heat prose
  do not independently dispatch the fire-sector effect.
- Broken wagon 24101, ladder 24102, melted chain 24105 and lava 24107 are
  ITEM_TRASH scenery. The ladder's down/up shaft connection is an ordinary
  exit, not a ladder receipt or switch. The wheel/chain ruins describe an
  ancient lift; no repair/operating mechanism or ore-delivery endpoint exists.
- Adult and two young aboleths start at 24178. The second young F follows the
  immediately preceding young M, not the adult; admitted F also becomes sentinel.
  That is a concrete parenting discrepancy to discuss with a builder, not proof
  of a broken quest. Piercer's actual one-attempt periodic attack is an encounter
  hazard; no accepted survival, brood, demon-banishment or mine-restoration story
  terminal is declared.
- Grell 24103 is an unplaced level-one prototype with an empty description.
  Seventeen physical rooms have empty body descriptions, including named
  mine/foreman/depot rooms and Unnamed rooms. Complete scenery only after builder
  intent; the omission does not establish unfinished native quest recipes there.

## Expanded capability and balanced repair plan

| Requirement/gap | Implementation and acceptance plan |
| --- | --- |
| Owner versus physical affiliation | Preserve immutable Mushroom receipts; explicitly reference shared loading area, physical encounter, cross-zone campaign and discovery policy. Test encounters outside owner rooms without discovering the owner. |
| Availability versus static daily policy | Add independent per-recipe offering support and source/recipient availability, including missing prototype, money kind, unplaced actor and broken dispersal. Qualify actual spawn/retirement/reset episodes before daily selection; freeze pending obligations separately. |
| Exact same-name material semantics | Reserve one root of each half once; preserve source, producer, handoff and spent history. Schema current readiness must not imply acceptance or conflate modern/legacy actors/seals. |
| Money-typed quest proof | Builder selects treasure/armor/money intent and prototype/version policy. Qualify old live objects, reward issuance, drop/merge/get, custody and offering recovery. Keep current guards until authoritative settlement supports the selected semantics. |
| Missing goblet/actor/dispersal repair | Confirm intended legacy maintenance versus retirement/referral to modern campaign. If maintained, select exact goblet source, actor homes and current world destinations with real access/renewal tests. Retain recipe/receipt compatibility and use separate fix/news commits. |
| Literal dialogue and progression | Separate a stable topic ID from safely escaped literal command tokens; represent apostrophes without shell/client injection or silent family loss. Accepted topic receipt requires addressed visibility/response success; native house dialogue is not a branch predicate. |
| Computed teacher role | Preserve separate native qst_func and flag-assigned teacher. Harden addressed recipient/visibility and bound live runestone output, with multiple teachers, mismatched class, zero/many runestones and maximal zone-name tests; no lesson receipt from guidance. |
| Pool, obstacle, shaft and scenery intent | Run actual portal target/perception/arrival/return, effective obstacle clearing and falling/survival journeys. Builder chooses return text, NOSHOW policy, permanent obstacle versus mechanism, follower parent, descriptions and mine-restoration endpoints. |
| Full-stage campaign presentation | Keep three existing accepted outcomes; future optional campaign references can explain both source routes and reunion without requiring personal history where native acceptance only requires items. Add shared ANSI/GMCP availability and provenance states. |

**No native zone or quest repair ships in this checkpoint.** Missing goblet,
unplaced actors, five stale dispersal destinations, money-proof conflict, shared
teacher targeting/output bounds, pool
return/visibility policy, aboleth parent and unfinished scenery are pending
builder-selected repairs or qualification. Do not announce them as fixed. Any
implemented repair needs a clearly named separate fix commit, prominent PR
affected-zone/trigger/before-after/validation/limits and a precise news sentence.

## Validation and limits

Focused source contracts and actual C++ journal journeys cover exact bindings,
same-name halves/modern exclusions, optional history versus supplied proof,
worn versus loose readiness, explicit accounting/source warnings, physical
encounter versus owner discovery, independent outcomes, spent supplies, replay
and cold recovery. Model receipts exercise journal projection, not impossible
live goblet/money offerings. Full production catalog/source and all-map schema/
loader regressions, accounting gates, daily projection, maintained server build,
changed/staged formatting, whitespace and documentation links are checked.

Catalog: 82 maps/1,629 achievements/1,463 potential daily units/2,207 rows.
All 2,668 native definitions, source fingerprint, revision two, registry and
previous 81 mappings remain unchanged. Original queue: 61/220 comprehensive
source maps, 159 pending; Para-Elemental Plane of Smoke is next. Live generation,
source/handoff, money reward/offer/get/drop, actual perception/roaming, travel/
obstacles/falling, combat, retirement/renewal and played persistence are
unqualified. No DB/account activation/server operation, migration, deployment
or merge is performed for this checkpoint.
