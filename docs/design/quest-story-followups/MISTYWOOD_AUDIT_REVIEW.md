# Mistywood dependency and lineage supplement

**Checkpoint 23.** This supplement closes specific dependency, source-history,
and source-only topology questions for the existing
[Mistywood story dossier](../zone-stories/MISTYWOOD.md). It is not a new
narrative dossier and does not change the follow-up story-review totals.
Evidence is from the source snapshot recorded in the
[coverage register](ZONE_COVERAGE_REGISTER.csv#L182).

## Connectivity reconciliation

Mistywood remains an active full-area contributor. Its WLD has 203 rooms and
580 authored exits; 542 target loaded rooms and 38 target absent old-map
rooms. The active WLD boundary consists of three inbound records and one
outbound record:

| Source record | Destination | Reading |
| --- | --- | --- |
| [Pineholl room 16164, up](../../../areas/wld/pineholl.wld#L4202) | Mistywood room 95048 | Source-side staging room; does not establish ordinary player access. |
| [Krimman room 16486, east](../../../areas/wld/krimman.wld#L2175) | Mistywood room 95100 | Source-side staging room; does not establish ordinary player access. |
| [Underworld3 room 30162, up](../../../areas/wld/underworld3.wld#L3353) | Mistywood room 95045 | Paired with the Mistywood return below. |
| [Mistywood room 95045, down](../../../areas/wld/mistywood.wld#L1132) | Underworld3 room 30162 | The sole active outbound boundary. |

An independent scan of unregistered WLD sources found 53 more exit records from
surf.wld into 27 active Mistywood rooms. These belong to an unregistered
90,000-room map file and are source-only records, not active-world ingress.
They correct the previously blank source-only inbound fields in the coverage
row. D0 is north, D1 east, D2 south, and D3 west.

| Active Mistywood target | Source-only surf.wld exits |
| --- | --- |
| 95000 (2) | [229842 D2](../../../areas/wld/surf.wld#L626018); [229843 D2](../../../areas/wld/surf.wld#L626039) |
| 95001 | [231936 D0](../../../areas/wld/surf.wld#L669948) |
| 95010 | [233442 D0](../../../areas/wld/surf.wld#L701562) |
| 95087 | [231331 D1](../../../areas/wld/surf.wld#L657251) |
| 95109 (2) | [229550 D3](../../../areas/wld/surf.wld#L619898); [229849 D0](../../../areas/wld/surf.wld#L626157) |
| 95113 | [230149 D3](../../../areas/wld/surf.wld#L632449) |
| 95114 | [229849 D3](../../../areas/wld/surf.wld#L626169) |
| 95120 | [230729 D1](../../../areas/wld/surf.wld#L644609) |
| 95132 | [229245 D2](../../../areas/wld/surf.wld#L613489) |
| 95134 | [228946 D2](../../../areas/wld/surf.wld#L607218) |
| 95135 | [228647 D2](../../../areas/wld/surf.wld#L600951) |
| 95136 (6) | [228050 D3](../../../areas/wld/surf.wld#L588426); [228348 D2](../../../areas/wld/surf.wld#L594680); [228350 D3](../../../areas/wld/surf.wld#L594726); [228650 D3](../../../areas/wld/surf.wld#L601018); [228950 D3](../../../areas/wld/surf.wld#L607306); [229250 D3](../../../areas/wld/surf.wld#L613598) |
| 95137 | [229841 D2](../../../areas/wld/surf.wld#L625997) |
| 95138 (14) | [230129 D2](../../../areas/wld/surf.wld#L632025); [230139 D1](../../../areas/wld/surf.wld#L632231); [230139 D2](../../../areas/wld/surf.wld#L632235); [230428 D1](../../../areas/wld/surf.wld#L638292); [230430 D2](../../../areas/wld/surf.wld#L638338); [230430 D3](../../../areas/wld/surf.wld#L638342); [230432 D2](../../../areas/wld/surf.wld#L638380); [230433 D2](../../../areas/wld/surf.wld#L638401); [230434 D2](../../../areas/wld/surf.wld#L638422); [230435 D2](../../../areas/wld/surf.wld#L638443); [230436 D2](../../../areas/wld/surf.wld#L638464); [230437 D2](../../../areas/wld/surf.wld#L638485); [230438 D1](../../../areas/wld/surf.wld#L638502); [230438 D2](../../../areas/wld/surf.wld#L638506) |
| 95145 | [231030 D1](../../../areas/wld/surf.wld#L650930) |
| 95152 | [230449 D3](../../../areas/wld/surf.wld#L638741) |
| 95161 | [230748 D3](../../../areas/wld/surf.wld#L645016) |
| 95171 | [231632 D1](../../../areas/wld/surf.wld#L663572) |
| 95172 | [232538 D0](../../../areas/wld/surf.wld#L682586) |
| 95173 | [232539 D0](../../../areas/wld/surf.wld#L682607) |
| 95180 | [231048 D3](../../../areas/wld/surf.wld#L651316) |
| 95181 (2) | [232839 D1](../../../areas/wld/surf.wld#L688911); [233140 D0](../../../areas/wld/surf.wld#L695224) |
| 95194 (2) | [231347 D3](../../../areas/wld/surf.wld#L657595); [231646 D0](../../../areas/wld/surf.wld#L663862) |
| 95197 | [232544 D0](../../../areas/wld/surf.wld#L682712) |
| 95198 (2) | [231946 D3](../../../areas/wld/surf.wld#L670170); [232245 D0](../../../areas/wld/surf.wld#L676433) |
| 95199 | [231646 D3](../../../areas/wld/surf.wld#L663874) |
| 95201 (4) | [232544 D3](../../../areas/wld/surf.wld#L682724); [232844 D3](../../../areas/wld/surf.wld#L689024); [233144 D3](../../../areas/wld/surf.wld#L695320); [233443 D0](../../../areas/wld/surf.wld#L701583) |

## Typed dependency and QST lineage

The source-only file areas/qst/kilfray.qst defines the same five giver VNUMs
(95019–95023) as Mistywood and has three parallel Q contracts. Their giver
and offered-item identities align, with this reward difference:

| Giver and offer | Mistywood reward | Kilfray reward |
| --- | --- | --- |
| Riliatar 95019 offers claw 95000 | 15,000 coins and 25,000 experience | 15,000 coins only |
| Ascuren 95020 offers club 95001 | Potion 95004 | Potion 95004 |
| Jarnes 95022 offers mushroom 95003 | Scimitar 95002 and 10,000 experience | Scimitar 95002 only |

Daumis 95021 and Korred 95023 are lore contacts, not additional Q contracts.
The active [Mistywood QST](../../../areas/qst/mistywood.qst) and source-only
[Kilfray QST](../../../areas/qst/kilfray.qst) differ in wording, formatting,
some record ordering, and the Riliatar and Jarnes experience rewards, so this
is a parallel lineage rather than a byte-identical or reward-identical
duplicate.
Mistywood appears in both
[production area lists](../../../areas/AREA#L339) and
[the historical QST index](../../../areas/qst/AREA#L299); Kilfray appears in
neither. The production QST generator reads the top-level AREA list
([make_qst.c](../../../areas/src/qst/make_qst.c#L6)). The Kilfray file is
therefore not compiled into the normal world quest catalog.

Static typed scans found no external ZON reset/door references into Mistywood's
room, mobile, or object ranges; no external mobile or shop references to those
prototypes; and no direct compiled C/C++ or script numeric reference. The
existing dossier also records no type-25 object in Mistywood and no active
type-25 destination into the area. These are source scans; editor/tooling
ownership and indirect or dynamic lookups remain open.

Several raw number matches are values, not prototype dependencies:

- [surface.qst lines 725, 748, and 749](../../../areas/qst/surface.qst#L725)
  use 95000 for currency or experience terms.
- [surface2011.qst line 725](../../../areas/qst/surface2011.qst#L725) has the
  same currency/experience values.
- [Krethik object line 416](../../../areas/obj/krethik.obj#L416) and
  [Newhope object line 2627](../../../areas/obj/newhope.obj#L2627) use 95000
  in cost/value fields, not as an object VNUM.

## History, builder questions, and recommended disposition

Git path history records the 2009 area-tree relocation and a 2013 generic
“stuff” commit for both QST files; the 2011 Mistywood WLD edit corrects
spelling only. No commit or current list records a direct Kilfray-to-Mistywood
replacement. The Mistywood story sidecar and dossier were added separately in
2026 and do not establish QST provenance.

Builder decisions:

1. Is Kilfray an intentionally disabled earlier copy, an alternate draft, or a
   source that should eventually be retired? The files share giver and offered
   item VNUMs, but Kilfray lacks Mistywood's 25,000 Riliatar and 10,000 Jarnes
   experience rewards, and several dialogue passages differ. **Recommended
   resolution:** keep Kilfray source-only and retained until its author/tooling
   lineage and reward intent are identified. Do not add it to the production
   list or delete/move it based on similarity alone.
2. Are the 53 surf.wld edges into Mistywood and Mistywood's 38 old-map targets
   parts of one intended Surface connection, or alternate map-grid remnants?
   **Recommended resolution:** retain the source files and ask the map owner
   to identify the authoritative Surface topology before considering any
   retirement or route repair. Do not treat the 53 records as runtime access.
3. Are the Pineholl and Krimman staging entrances meant to become ordinary
   player routes? **Recommended resolution:** preserve their present active
   declarations and keep ordinary reachability unclaimed until the staging
   and access conditions are resolved.

No source, list membership, or zone placement changed. The coverage row now
records 53 source-only inbound records from surf and partial typed-dependency
closure; the global
[connectivity report](WORLD_CONNECTIVITY.md#L121),
[replacement-history report](DUPLICATE_AND_REPLACED_ZONES.md), and
[candidate-specific migration plan](ATTIC_MIGRATION_PLAN.md#L27) carry the
corresponding cross-zone decisions.

## Checkpoint 70 source revalidation

Checkpoint 70 reran the repository-source WLD, typed-reference, title, and
history checks for Mistywood. The current active scan parsed all 351 active
WLD files without issues. Mistywood still has 203 rooms and 580 authored
exits: 541 resolve internally and one additional exit resolves across the
active boundary, for 542/580 loaded targets and one 203-room weak component.

The three active inbound records remain Pineholl #16164 up to Mistywood #95048,
Krimman #16486 east to #95100, and Underworld3 #30162 up to #95045. Mistywood's
only valid active outbound is #95045 down to Underworld3 #30162, paired with
the Underworld3 inbound. The other 38 authored outbound records still point
to absent old-map VNUMs; they are unresolved records, not active routes. The
active boundary rows remain recorded in the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L286).

The all-95 inactive WLD scan parsed 527,791 rooms and 2,080,773 exits with no
issues and no source-room VNUM overlap. It again found 53 non-overlapping
`surf.wld` exits into 27 active Mistywood rooms. Their grouped room-by-room
evidence remains in [connectivity reconciliation](#connectivity-reconciliation).
Those 53 references were already counted, so the curated source-only inbound
total remains 603. The coverage row now records 53 references from `surf`
while retaining the separate 38 unresolved active outbound targets.

Typed scans reconfirm that the five exact QST item commands matching
Mistywood prototypes occur in the unregistered `kilfray.qst` file only. That
file is absent from both production area lists; its parallel contracts and
reward differences remain documented above, with no new production or
retirement decision. The 231 other story sidecars parsed without issues and
had no exact structured VNUM matches. No external SHP product, reset
role/room target, mobile/object prototype reuse, compiled C/C++ numeric
reference, or type-25 destination into Mistywood was found. Generic currency,
experience, and request-ID number matches remain non-prototype values.

The normalized active title remains unique, and the bounded source history
still identifies no distinct replacement source. Mistywood remains **DO NOT
MOVE**. The existing builder questions about Kilfray's status, Surface map
topology, and Pineholl/Krimman staging access remain open; no route, source, or
placement decision changed.

## Checkpoint 284: title, history, and bounded typed closure

After stripping Duris color codes and the trailing title terminator, then
normalizing case and whitespace, the exact title “Mistywood” has one owner
among all 442 current .zon files: active mistywood. The followed
mistywood.wld, mistywood.zon, and mistywood.qst paths remain on the same
stem from the 2009 area-tree move through later same-path edits. No alternate
stem, distinct predecessor, or whole-zone replacement was established.
Source-only kilfray.qst has parallel giver and contract identities, but is
absent from both production area lists and its followed history does not
establish replacement or precedence. Keep the active area **DO NOT MOVE**.

Checkpoint 70's bounded typed scan remains current: the only exact external
QST item consumers are five matching item commands in source-only
kilfray.qst, which is not in either production list. No outside SHP product,
active external ZON reset target, duplicate external mobile/object prototype,
compiled C/C++ numeric reference, exact structured sidecar VNUM reference, or
active type-25 target into Mistywood was found. The 231 other story sidecars
parsed without issues. This closes the direct source-typed categories scanned;
editor/tool ownership, arbitrary indirect/runtime lookups, and persisted
player/reference records remain outside this source result.

No connectivity finding is added. The 53 non-overlapping surf.wld approaches
were already counted; retain them on existing Surf candidate row 97 as
source-only records. The 38 positive unresolved active exits remain distinct.
No route or candidate row is added, and current curated totals remain 3,777
overall, 249 Surf, 18 ud_map2, and eight Underworld2. See the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L182), [connectivity report](WORLD_CONNECTIVITY.md#checkpoint-284-mistywood-source-reconciliation),
[title and history report](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-284-mistywood-title-and-bounded-history),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 284: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Riliatar, Ascuren, and Jarnes remain three independent native contracts with separate receipts? | [Three endpoints](../zone-stories/MISTYWOOD.md#three-independent-native-endpoints), [native QST](../../../areas/qst/mistywood.qst) | Preserve the three authored identities; do not add a hidden Daumis prerequisite or combine rewards. |
| 2 | Should Riliatar continue accepting exactly claw necklace #95000 for 15,000 coins and 25,000 experience without a personal scarred-bear kill check? | [Riliatar QST](../../../areas/qst/mistywood.qst#L32), [source and custody](../zone-stories/MISTYWOOD.md#source-and-custody) | Keep exact item acceptance and reward semantics; record any personal hunt history separately. |
| 3 | Should the scarred black bear remain the declared necklace source, distinct from ordinary brown bears that do not carry it? | [Bear and necklace reset](../../../areas/zon/mistywood.zon#L145) | Preserve typed source identity; do not infer the necklace from a generic bear kill or name match. |
| 4 | Should a supplied necklace, club, or mushroom remain eligible without requiring first recovery by the accepting character? | [Native custody boundary](../zone-stories/MISTYWOOD.md#source-and-custody), [three QST endpoints](../zone-stories/MISTYWOOD.md#three-independent-native-endpoints) | Keep current-custody acceptance; make source provenance a separate explicit story with its own source and actor records if desired. |
| 5 | Should Ascuren continue accepting exactly club #95001 and returning potion #95004 without a saved forest-restoration or giant-kill outcome? | [Ascuren QST](../../../areas/qst/mistywood.qst#L58), [potion prototype](../../../areas/obj/mistywood.obj#L52) | Preserve the item exchange; treat the forest protection request as lore until builders define an actual lasting outcome. |
| 6 | Should club #95001 remain cap-one floor stock at room #95075 while the giant's SCAVENGER behavior remains separate from direct equipment placement? | [Club reset](../../../areas/zon/mistywood.zon#L59), [scavenging review](../zone-stories/MISTYWOOD.md#source-and-custody) | Preserve the declared stock and shared scavenging rules; qualify current custody before calling supply absent or guaranteed. |
| 7 | Should Jarnes continue accepting only mushroom #95003 for scimitar #95002 and 10,000 experience? | [Jarnes QST](../../../areas/qst/mistywood.qst#L113), [mushroom prototype](../../../areas/obj/mistywood.obj#L44) | Keep the exact item and combined reward; unrelated fungi, potion #95004, and other rewards do not substitute. |
| 8 | Should eating the native mushroom remain a separate sickness and hit-regeneration effect that can spend an item before delivery? | [Food prototype](../../../areas/obj/mistywood.obj#L44), [consumption review](../zone-stories/MISTYWOOD.md#source-and-custody) | Preserve native food behavior and explain the competing use; do not convert eating into rescue or quest credit. |
| 9 | Should Daumis's referral dialogue stay optional because Jarnes has no accepted-turn-in prerequisite on a prior Daumis conversation? | [Daumis and Jarnes source review](../zone-stories/MISTYWOOD.md#three-independent-native-endpoints), [QST records](../../../areas/qst/mistywood.qst#L70) | Keep referral history separate from exact Jarnes item acceptance unless a new authored chain is approved. |
| 10 | Should accepting Jarnes's mushroom continue to extract him after his message, without moving him to Daumis or teleporting the player? | [Departure contract](../../../areas/qst/mistywood.qst#L121), [rescue retirement review](../zone-stories/MISTYWOOD.md#capability-and-builder-follow-ups) | Preserve the current retirement semantics; design any reunion or escort as a separately named, settled outcome. |
| 11 | Should the three mode-two contracts remain potential daily candidates without promising fresh stock, renewed NPCs, or repeatable rewards? | [Zone header and reset mode](../../../areas/zon/mistywood.zon#L3), [renewal review](../zone-stories/MISTYWOOD.md#qualification-and-proposed-repair-reporting) | Keep daily use unqualified until ready reset supply, recipient renewal, reward delivery, replay, and persistence are demonstrated. |
| 12 | Should Ascuren remain a normal recipient without SENTINEL or ACT_TEACHER status, so training is not treated as an offering prerequisite? | [Ascuren placement and behavior](../zone-stories/MISTYWOOD.md#three-independent-native-endpoints), [mobile source](../../../areas/mob/mistywood.mob#L285) | Keep contract eligibility separate from training and player-presence behavior unless the builder intentionally changes the mobile flags. |
| 13 | Should the native contract set remain three receipts while rescue, forest repair, and cure ideas remain unimplemented builder proposals? | [Unresolved stories](../zone-stories/MISTYWOOD.md#access-hazards-and-unresolved-stories), [story sidecar](../../../areas/story/mistywood.story.json) | Require an explicit beneficiary, state change, failure policy, and saved result before adding any new receipt. |
| 14 | Is Kromor's unwanted immortality meant to remain unresolved, with the book and Korred dialogue as lore only? | [Kromor and Korred review](../zone-stories/MISTYWOOD.md#access-hazards-and-unresolved-stories) | Do not invent a cure recipe or infer completion from entering the tower; design ingredients and a durable endpoint first. |
| 15 | Should the dead horse, silver arrows, trophies, broken arrow, skeletal containers, and burnt lizard remain story props without hidden recovery contracts? | [Unassigned story elements](../zone-stories/MISTYWOOD.md#access-hazards-and-unresolved-stories), [object inventory](../../../areas/obj/mistywood.obj) | Keep them as source descriptions and objects until an owner selects a distinct request, item flow, and beneficiary outcome. |
| 16 | Should the tower's locked roof access and the vegetation/cabin doors retain their authored reset-specific states and directions? | [Door resets](../../../areas/zon/mistywood.zon#L16), [access review](../zone-stories/MISTYWOOD.md#access-hazards-and-unresolved-stories) | Preserve the current key, closed, and secret states; qualify actual search/open/arrival before any balance change. |
| 17 | Should pits, no-heal rooms, rivers, bridges, and no-ground branches continue using shared movement and hazard rules despite descriptive danger? | [Hazard and passage review](../zone-stories/MISTYWOOD.md#access-hazards-and-unresolved-stories), [room source](../../../areas/wld/mistywood.wld) | Preserve current room data until a specific intended safety or fall rule is authored; do not infer F/C values from prose. |
| 18 | Should the reciprocal Underworld3 boundary stay distinct from Pineholl and Krimman's NO_MOB/NO_SUMMON staging inbounds? | [Boundary reconciliation](MISTYWOOD_AUDIT_REVIEW.md#connectivity-reconciliation), [staging-source review](../zone-stories/MISTYWOOD.md#access-hazards-and-unresolved-stories) | Keep the valid paired boundary; do not publish staging arrivals as player routes without an owner-approved admission path. |
| 19 | Do the 53 source-only Surf approaches and 38 unresolved active old-map exits represent one intended map connection or separate historical remnants? | [Grouped Surf approaches](MISTYWOOD_AUDIT_REVIEW.md#connectivity-reconciliation), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) | Preserve both source facts and ask the map owner to identify current topology before reactivation, repair, retirement, or player guidance. |
| 20 | Is source-only kilfray.qst an earlier copy, alternate draft, or unrelated parallel source given its shared giver VNUMs and reward differences? | [QST lineage comparison](MISTYWOOD_AUDIT_REVIEW.md#typed-dependency-and-qst-lineage), [production lists](../../../areas/qst/AREA#L299), [active area list](../../../areas/AREA#L339) | Retain Kilfray outside production until its author and tooling lineage are identified; do not add, delete, or replace files based on similarity alone. |
