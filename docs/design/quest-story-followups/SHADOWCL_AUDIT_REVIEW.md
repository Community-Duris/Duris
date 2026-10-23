# Shadowclave: connectivity, dependencies, and builder review

Checkpoint 355 reconciles the source story in the [Shadowclave dossier](../zone-stories/SHADOWCLAVE.md) with current active boundaries, inactive WLD sources, QST/SHP/ZON references, compiled behavior, and followed source history. No gameplay or runtime claim is made.

## Active graph and dynamic arrival

The active WLD has 53 rooms and 108 authored exits. One exit is negative: #93918 east to -1; the other 107 exits resolve. All 53 rooms form one weak and one strongly connected directed component when authored edges are traversable. There are no positive-unresolved exits, range mismatches, fall fields, or current fields. From #93900 and #93952, raw authored reachability and returnability are 53/53. Suppressing all flagged directions leaves 5/5 from #93900 and 20/20 from #93952. These are static comparisons and do not establish door timing or safe travel.

| Active boundary | Reverse edge | Classification |
| --- | --- | --- |
| Shadowclave #93900 north -> Surface #545307 | Surface #545307 south -> #93900 | Reciprocal ([outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1128), [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1431)) |
| Shadowclave #93900 east -> Surface #545708 | Surface #545708 west -> #93900 | Reciprocal ([outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1129), [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1433)) |
| Shadowclave #93952 up -> Surface #544905 | Surface #544905 down -> #93952 | Reciprocal ([outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1130), [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1427)) |

Twenty flagged directions make ten paired doors/passages. All twenty match local D resets at 100% chance and state value 1; ZON comments call them closed. The negative #93918 east record is not counted as a destination. The authored river prose calls its flow fast and dangerous, but there is no numeric current field.

A compiled route adds one possible non-WLD arrival. In [innates.c](../../../src/classes/innates.c#L2011), room #93919 is one of fourteen Githzerai destinations for do_shift_prime. The function requires an Astral origin, Githzerai race, no active fight, and an available innate timer; it randomly selects a loaded destination. Room #93919 is the small pond. This conditional teleport does not add a WLD boundary record and was not exercised in game.

## Inactive map approaches

All 95 inactive WLD files were searched for exact Shadowclave room headers #93900-#93952 and exit destinations in that range. There is no inactive source-room VNUM overlap. Two source-only Surf approaches were found:

| Inactive source | Exit | Classification |
| --- | --- | --- |
| surf.wld #212935, Large Watering Hole | down -> Shadowclave #93952 | Source-only, not active ingress ([room/exit](../../../areas/wld/surf.wld#L271427)) |
| surf.wld #215655, forest path | west -> Shadowclave #93900 | Source-only, not active ingress ([room/exit](../../../areas/wld/surf.wld#L328443)) |

Preserve both on existing Surf candidate row 97; no new candidate is added. Surf's reviewed source-only subset rises from 267 to 269; the curated global total rises from 4,504 to 4,506. Surf's 145 active references and Underworld2's nine reviewed references are unchanged. See the [candidate register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Typed dependencies

The local shop at SHP VNUM #93906 is owned by Iliyad, local mobile #93906, and offers local products #93914, #93915, and #93916. The local ZON gives Iliyad one of each. This active SHP record corrects the coverage row's former zero-shop count.

External typed references to Shadowclave prototypes include:

| Source | Typed reference | Classification |
| --- | --- | --- |
| [Malch QST](../../../areas/qst/malch.qst#L17) | Completion actions issue #93901 three times after dialogue asks for three badges. No matching R I #93901 removal occurs in this block. | Source text/action mismatch; builder review needed before claiming badge consumption |
| [Newhaven QST](../../../areas/qst/newhaven.qst#L84) | Two crafting actions issue #93901 as an item output after removing other materials. | External badge output |
| [Shipy QST](../../../areas/qst/shipy.qst#L871) | Dialogue names bubbling brown potion #93914 as a requested Shadowclave ingredient; the visible result block removes #43144 and issues #93914. | QST text/action mismatch; no #93914 removal is shown |
| [Alatorin ZON](../../../areas/zon/alatorin.zon#L3646) | Gives #93914 to a dealer; a later reset gives #93901 to a cloaked visitor. | Active external producers |
| [Shipy ZON](../../../areas/zon/shipy.zon#L636) | Gives #93914 to a troll shopkeeper. | Active external producer |
| [Alatorin SHP](../../../areas/shp/alatorin.shp#L533), [Shipy SHP](../../../areas/shp/shipy.shp#L264) | Both list #93914. | External shop supply |

The external Malch, Newhaven, and Shipy sidecars have four structured item references to #93901 or #93914. No external same-type MOB/OBJ prototype definition, external room D reset, or SHP product outside those listed was found.

Compiled references include [fishing pole #93913](../../../src/economy/tradeskill.c#L574), which the generic fishing code recognizes; [potion #93915](../../../src/account/chaos_eq_data.h#L577), in Chaos support-consumable data; and dynamic destination room [#93919](../../../src/classes/innates.c#L2011). These are distinct code relationships and do not establish local quest progress.

## Title, history, and disposition

After color stripping, terminator trimming, whitespace normalization, and case folding, Shadowclave is unique among all 442 current .zon titles and the 351 active contributors. Same-type active VNUM scans found no room, mobile, or object overlap; inactive WLD searches found no room-number overlap.

Followed history remains on shadowcl: WLD through 2013-11-13; MOB through 2014-08-17; OBJ through 2016-03-05; ZON through 2015-10-13; and SHP through 2014-08-17. There is no local QST file or story sidecar to follow. No alternate-stem predecessor, distinct prior zone, or explicit whole-zone replacement was found in these bounded paths. Keep active shadowcl DO NOT MOVE.

## Builder questions and recommended resolutions

| ID | Builder question | Source-backed answer | Recommended resolution |
| --- | --- | --- | --- |
| BSQ-SC-01 | Is the Shadow Circle meant to be a hidden druid refuge? | Room #93922 calls it a hidden cavern with druids, animals, and Shadow Circle weapons; its members wear matching badges. | Preserve the hidden-circle framing unless a builder documents a different faction story. |
| BSQ-SC-02 | Are the two forest entries intended to share the same rough-path room? | Surface rooms #545307 and #545708 both connect reciprocally to #93900. | Keep the two distinct Surface approaches and avoid collapsing their identities. |
| BSQ-SC-03 | Is the east exit from #93918 to -1 deliberate? | The authored WLD target is negative and does not resolve to a room. | Confirm whether this is intentional void behavior or stale data before proposing a route change. |
| BSQ-SC-04 | Should Githzerai shifting be able to land at the small pond? | do_shift_prime selects #93919 from a list of fourteen destinations for eligible Githzerai in the Astral. | Preserve the code-defined route in dependency maps; have the owning builder confirm destination intent before editing it. |
| BSQ-SC-05 | Are the several bush passages intended to be closed? | Paired flags and D resets mark concealed bush routes closed. | Keep the paired authored state; qualify actual access and reset behavior before describing how players pass. |
| BSQ-SC-06 | Is the huge hollow tree a magical sanctuary? | Room #93915 describes a magically enlarged interior, with ladder and hidden routes nearby. | Retain the text and route as environmental story; do not invent a new quest gate. |
| BSQ-SC-07 | Do the forest druids intentionally wear the same Shadow Circle badge? | ZON equips object #93901 across several cloaked druids; the cavern describes Shadow Circle weapons. | Preserve the shared insignia and type identity, while separating badge stock from membership credit. |
| BSQ-SC-08 | Should Snarg's Malch task consume three badges or award three? | Snarg asks for three; the completion block issues three #93901 items and shows no matching removal action. | Resolve the QST/dialogue mismatch with the QST owner before treating it as a valid badge hand-in. |
| BSQ-SC-09 | Should Newhaven crafting continue to output the Shadow Circle badge? | Two Newhaven QST craft actions output #93901 after other item removals. | Preserve those external outputs until the owning builder reviews faction meaning and supply. |
| BSQ-SC-10 | Is bubbling brown potion #93914 a Shipy ingredient or output? | Shipy's text asks for the potion from Shadowclave, while its result block removes #43144 and outputs #93914. | Reconcile the prose and QST item commands before claiming a Shadowclave turn-in. |
| BSQ-SC-11 | Should Alatorin and Shipy continue to source Shadowclave objects? | Both ZON sources reset #93914; Alatorin also resets #93901. | Preserve the cross-zone producers and include them in any independent supply or migration review. |
| BSQ-SC-12 | Should multiple shops sell potion #93914? | Shadowclave, Alatorin, and Shipy SHP sources list the same local potion VNUM. | Keep item ownership and each shop's stock source explicit; do not duplicate or move prototypes casually. |
| BSQ-SC-13 | Are animal clippers and fur meant to be obtainable quest items? | Verra asks for clippers and a harvest bag, and Grelim asks for fur; objects #93911/#93912 exist but have no reset or shop source. | Confirm whether these are hint-only lines or whether a separate supply/quest is intended. |
| BSQ-SC-14 | Should the two fishing NPCs have separate roles? | Belim holds a dead trout at the pond; the dock fisherman carries a fishing pole and complains of hunger. | Preserve their distinct source placements and confirm any intended trading or fishing story separately. |
| BSQ-SC-15 | Should generic fishing recognize Shadowclave's pole? | Compiled trade-skill code explicitly recognizes object #93913. | Keep this prototype reference in typed dependency records; qualify actual fishing availability separately. |
| BSQ-SC-16 | Is the local potion shop intended to be owned by Iliyad? | SHP #93906 lists three local potions and names owner mobile #93906, reset in workshop #93929. | Preserve the shop/mob relationship and review live stock with the builder if supply changes are proposed. |
| BSQ-SC-17 | Are the bears meant to be training animals in the closed rooms? | Both training-room descriptions mention wild beasts and a young druid; bear resets populate each room. | Retain the environmental encounter placement unless a builder requests a separate combat-scope change. |
| BSQ-SC-18 | Should the river prose imply a current or forced travel effect? | The descriptions warn of fast flow and jagged rocks, but there are no current fields or current callbacks found. | Keep the text as atmosphere and qualify any actual current/safety claim in a separate runtime review. |
| BSQ-SC-19 | Are the 20 flagged routes intended to reset closed? | Every flagged direction has a state-1, 100%-chance D reset; ZON comments call them closed. | Preserve paired reset entries; do not infer guaranteed startup state without a live reset observation. |
| BSQ-SC-20 | Is local QST absence intentional despite qst/AREA membership? | No local qst file or story sidecar exists and the native Q/QA contract count is zero. | Keep this dossier source-only; add a local quest only after the builder specifies actual story requirements and reward authority. |

This checkpoint updates design documentation, the coverage row, and the existing Surf candidate record only. No area, gameplay, database, or compiled source was changed; no build, test, server, or gameplay journey was run.
