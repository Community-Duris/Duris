# Swamp One audit review

**Checkpoint 376 — source-comprehensive setting, static connectivity, bounded
typed dependencies, title/history, and builder questions.** The active source is
`area:swamp_one`, zone 260, rooms #26001–#26036. Keep it **DO NOT MOVE**.

## Review boundary

The [story dossier](../zone-stories/SHADOW_SWAMP.md) covers all 36 WLD rooms,
12 mobile prototypes, 16 object prototypes, and the local reset table. The
active `areas/AREA` entry is at [line 130](../../../areas/AREA#L130). `qst/AREA`
also lists the stem at [line 122](../../../areas/qst/AREA#L122), but no local
QST file, native Q giver, story sidecar, or SHP file is present. The quest index
is not the production world list. Local shop count zero is therefore accurate.

## Static connectivity and source-only approach

| Measure | Source result |
| --- | --- |
| Active WLD | 36 rooms; 84 authored directions, all 84 resolving; 82 local and two outbound. |
| Active boundaries | Surface #608724 west enters #26001 and #26001 east returns. Swamp Two #26100 north enters #26019 and #26019 south returns. Two inbound, two outbound, two reciprocal boundary records. |
| Raw local graph | One weak component of 36. Directed traversal from the two inbound targets reaches all 36; all 36 have a directed path to an outbound boundary. This calculation ignores door state, secret search, bog traversal, and runtime movement rules. |
| Hazards and malformed targets | No negative target, unresolved WLD target, numeric-range mismatch, fall-chance marker, or current marker was found in this area’s source census. This does not establish environmental safety. |
| Inactive WLD source | `surf.wld` #267604 west points to active #26001 (source room line 1418596, target line 1418615). It is source-only, not loaded ingress; retain the record on existing Surf candidate row 97. |

The numeric source room #267604 in inactive `surface2011.wld` has a different
Storm Sea title and its exits lead to #267304, #267605, #267904, and #267603;
none points into this swamp. Preserve each file’s room identity. The Surf approach
raises the reviewed Surf source-only subset from 285 to 286 and the curated
source-only total from 7,516 to 7,517. Surf’s 145 active numeric references do
not change. See [connectivity checkpoint 376](WORLD_CONNECTIVITY.md#checkpoint-376-swamp-connectivity-and-inactive-approach)
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

The #26017/#26036 hidden exit should not be reduced to an exit-info bitmask.
The WLD loader masks the WLD state field and creates a door; ZON state 5 closes
and marks only the east side secret, while the west return is reset closed and
unlocked without the secret bit. Neither reset locks it. The source comments say
“closed/secret” east and “closed” west. Keep this source-level distinction and
ask the builder to decide whether discovery should be directional; do not infer
that the secret is discoverable or that the route is traversable.

## Typed dependency closure

| Source | Dependency and disposition |
| --- | --- |
| `swamp_two.zon` | 26 mobile resets import Swamp One mobiles #26003–#26006 and #26011; four object resets import #26011–#26014. Preserve the Swamp Two reset owner. This creates no local reset for those mobs/items. |
| `swamp_two.wld` and `.zon` | Dingy key #26005 opens the #26100/#26101 Quicksand boundary; dirty key #26007 opens #26101/#26102; bent key #26011 opens #26132/#26133. These are typed key VNUM declarations, not proof of an obtainable key. The resets place #26005 on Phyla, #26007 with the hidden-room corpse, and #26011 with a guard in the troll complex. |
| `caves_skelenak.qst` and story sidecar | Optional Caves exchange references object #26013. Its sidecar completion key says give sphere #26013, receive #4031, disappear; this remains Caves-owned. |
| Maintained C++ | `specs.assign.c` binds object #26014 to `nightbringer`; the handler source contains 1/30 sleep and 1/40 darkness branches. |
| Bounded negatives | No same-type external WLD/MOB/OBJ collisions, external QST giver collision, other typed story consumer, external SHP product, local SHP, or type-25 destination to a local room was found. Local #26001–#26012 mobile/object number overlaps are cross-type and are not prototype collisions. |

See the dossier’s [typed reference table](../zone-stories/SHADOW_SWAMP.md#typed-external-references).
The Caves item reference and Swamp Two resets do not justify adding a native
Swamp One quest or a local stock claim.

## Title, same-type identity, and followed history

After removing color codes and `.zon` terminators, normalizing whitespace, and
case-folding, “Swamp” has one exact match among 442 current ZON titles and belongs
to one of 351 active contributors. The active/inactive WLD, MOB, and OBJ scans
found no same-type VNUM collision with another contributor. The local overlap
between mobile and object IDs #26001–#26012 is cross-type.

Followed tracked source history stays on the `swamp_one` stem: WLD and OBJ edits
through 2013-11-13, MOB and ZON edits through 2015-10-13, and the tracked path
move from 2009-08-12. No alternate-stem rename, explicit predecessor, or
whole-zone replacement was established. Because no local QST, SHP, or story
sidecar exists, there is no local tracked history for those types. Checkpoint
757 completes the bounded semantic-name comparison; command-target alias and
owner disambiguation remain separate. See [identity/history checkpoint
376](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-376-swamp-title-same-type-identity-and-followed-history)
and the [semantic-name review below](#checkpoint-757-semantic-title-and-source-name-comparison).

## Checkpoint 757: Semantic title and source-name comparison

The normalized current title remains uniquely “Swamp.” “Shadow Swamp” is local
room and reset wording for `swamp_one`, not a second active contributor. The
other related current titles belong to separate rostered identities:
`swamp_two` is “The Lair of the Swamp Troll King,” `lizard` is “The Lizardman
Swamps of Clavikord,” `pods` is “Pharr Valley Swamp,” and `lab` is “The Swamp
Laboratory of Khul’Lor.” `bogentok` (“BogenTok”) appeared only in a broad
`bog` substring scan and is not a swamp-name match. The `qst/AREA` line without
a local QST file remains builder question 2; it is not evidence of another
quest source, predecessor, or replacement.

The unregistered `surf.wld` source contains 22 room labels “The Shadow Swamp
Lake” and room #267604, “A Shadowy Swamp Filled with Rotted Trees,” whose west
exit points to active room #26001. These records are source-only and remain
under the existing Surf candidate row 97 **RETAIN IN PLACE** plan. The same
numeric room #267604 in `surface2011.wld` is “The Azure Waters of a Stormy
Sea,” a separate retained source identity. Neither wording nor VNUM coincidence
establishes a second active map or a replacement. Followed `swamp_one` history
remains on the current stem with no predecessor or whole-zone replacement;
command-target alias and owner disambiguation remain outside this static name
comparison.

## Twenty builder questions and recommended resolutions

The recommendations retain current source identities and defer runtime or owner
choices that the source cannot settle.

| # | Builder question | Evidence and recommended resolution |
| ---: | --- | --- |
| 1 | Is “Swamp” the intended display title for this contributor? | It is unique after normalization, while `swamp_two` has a separate title and range. Keep the current title unless the area owner supplies a specific replacement name; do not rename the stem from this audit. [Dossier](../zone-stories/SHADOW_SWAMP.md#identity-history-and-builder-handoff) |
| 2 | Why does `qst/AREA` list `swamp_one` without a local QST file? | The index entry exists but no local contract file or giver does. Confirm whether the historical index line should remain; do not fabricate a quest file to satisfy the listing. |
| 3 | Should the Surface entrance remain reciprocal? | #26001 east and Surface #608724 west form the active pair. Preserve both authored directions and review arrival safety only in a separate gameplay pass. [Connectivity](#static-connectivity-and-source-only-approach) |
| 4 | Should inactive Surf #267604 become a real ingress? | It is an inactive `surf.wld` source-only west exit to #26001, and a same-number room in `surface2011.wld` has a different Storm Sea identity. Keep the source-only record on Surf row 97; do not activate it from numeric coincidence. |
| 5 | Does “Quicksand” intentionally connect the trail to Swamp Two? | #26019 south to #26100 is reciprocated north; the local sign warns of quicksand. Preserve the pair and ask the owner to verify the warning against actual travel behavior. |
| 6 | Is the Death Bog meant to be traversable along every authored exit? | Prose says water becomes very deep and several directions are impassable, but the source has no fall/current markers. Keep hazards and safety unqualified until a gameplay route review. [WLD](../../../areas/wld/swamp_one.wld#L360) |
| 7 | Should the #26017 east passage be a searchable secret door? | The WLD loader makes it a door; reset state 5 closes and marks it secret. Confirm intended discovery/opening policy in a separate fix if needed. Do not treat source comments as proof of operation. |
| 8 | Should the #26036 west return also be secret? | Its reset state 1 closes it without the secret bit; the east side gets state 5. Decide whether directional concealment is intentional and keep either change symmetric with the two-way route design. |
| 9 | Is Phyla’s dingy key #26005 intended for the first Swamp Two lock? | Phyla is reset wearing #26005; Swamp Two cites it as the key on #26100/#26101. Preserve those object and reset owners; separately qualify acquisition. |
| 10 | Is the hidden-room dirty key #26007 the intended next progression item? | It is reset with the corpse at #26036 and referenced by the #26101/#26102 tunnel lock. Preserve the clue and cross-area key reference unless the owner defines a replacement progression. |
| 11 | Is the bent key #26011 intentionally placed with a troll guard before the king chamber? | Swamp Two resets #26011 with a guard in #26132 and uses it on the #26132/#26133 locked pair. Keep this identity and do not move the reset without updating the lock source. |
| 12 | Should Swamp Two continue to own the imported troll-family and gear resets? | Its ZON imports local mobiles #26003–#26006/#26011 and objects #26011–#26014. Preserve those typed foreign reset roles; avoid duplicate Swamp One placements without an owner decision. |
| 13 | What is the intended role of #26009, #26010, and #26015? | The protection cloak, ancient belt, and troll tooth have no placement in the bounded active reset/shop/quest scan. Confirm whether they are legacy prototypes or have another documented producer; do not infer global orphan status. |
| 14 | Should the dragonfly keyword typo be corrected? | Mobile #26009’s keyword is `drgon fly`, while its short description says “dragon fly.” Correct the searchable keyword while retaining compatibility aliases if builder tooling supports them. [MOB](../../../areas/mob/swamp_one.mob#L96) |
| 15 | Should Mog’s sanctuary text say he is gone while the reset still places him nearby? | Swamp Two #26101’s static text says “now that Mog is gone,” while its ZON resets Mog at #26100. Confirm this is intended post-encounter wording or revise the static line; no dynamic text update is established here. [Room](../../../areas/wld/swamp_two.wld#L18), [reset](../../../areas/zon/swamp_two.zon#L34) |
| 16 | Do the Death Bog descriptions require movement restrictions beyond WLD exits? | Room prose describes depth and impassable directions, but no room fields establish swim checks or blockers. Confirm the intended environmental rule and test it separately before changing prose or exits. |
| 17 | Is the Caves of Mt. Skelenak sphere exchange still intended to consume #26013? | The Caves QST/sidecar records an optional sphere #26013 exchange for #4031. Keep receipt and reward with Caves; do not call it a Swamp One quest or promise sphere supply here. |
| 18 | Should Nightbringer’s current object handler remain attached to #26014? | The maintained assignment binds #26014 to `nightbringer`; the source has sleep and darkness branches. Preserve this code binding unless a separately reviewed behavior change is authorized. |
| 19 | Is zero local shop stock intentional? | No `swamp_one.shp` exists and no external shop product row points to local objects. Keep the zero-shop inventory unless the owner explicitly designs a vendor and stock source. |
| 20 | Should a native quest be authored for the existing environmental story? | No local QST or story sidecar exists. Keep the setting documented as environmental text; any new contract needs its own content, reward/source, and lifecycle review rather than an inferred extension of the Caves exchange. |

## Retention recommendation

Keep active `swamp_one` **DO NOT MOVE**. Preserve both reciprocal active
boundaries, the #26017/#26036 hidden-room reset asymmetry, the local key/item
prototype identities, Swamp Two’s typed reset ownership, and the optional Caves
sphere consumer. Retain the Surf approach only as source-only evidence on
existing candidate row 97. No gameplay fix or migration is cleared by this
review; see [retention checkpoint 376](ATTIC_MIGRATION_PLAN.md#checkpoint-376-swamp-in-place-retention-and-typed-dependencies).

## Checkpoint 831: Current typed source roles

A fresh exact-VNUM pass against the unchanged pinned `areas/` and `src/` snapshot confirms the bounded typed closure and clarifies the source roles:

- **Reset ownership:** [`swamp_two.zon`](../../../areas/zon/swamp_two.zon#L34) contains 26 mobile reset rows for Swamp One mobiles #26003–#26006 and #26011, plus four `G` object reset rows for #26011–#26014. Swamp Two remains the owner of those placements.
- **Keyed doors:** [`swamp_two.wld`](../../../areas/wld/swamp_two.wld#L16) pairs key #26005 across rooms #26100/#26101; key #26007 across #26101/#26102; and key #26011 across #26132/#26133. Swamp One's ZON places #26005 on Phyla and #26007 in room #26036; Swamp Two has the `G` reset for #26011. These declarations identify key VNUMs and reset owners, not guaranteed availability or traversal.
- **Caves consumer:** [`caves_skelenak.qst`](../../../areas/qst/caves_skelenak.qst#L203) has one `G I 26013` operand. Its sidecar records #26013 in `item_vnums` and `give=I:26013;receive=I:4031;disappear=1`; the repeated nested signature is in the same Caves-owned story. No other active QST item operand, QST giver, or structured story-sidecar role matched Swamp One's local IDs.
- **Maintained object binding:** [`specs.assign.c`](../../../src/specs/specs.assign.c#L2014) binds object #26014 to `nightbringer`; [`specs.underworld.c`](../../../src/specs/specs.underworld.c#L1363) retains the 1/30 sleep and 1/40 darkness branches.
- **Shop scope:** Swamp One has no local SHP file, and the active shop sources contain no product row for its objects. The previously bounded external-prototype and portal-target negatives are unchanged.

The optional sphere exchange remains Caves-owned, and Swamp Two remains the reset and keyed-route owner. This refresh changes no gameplay behavior and does not turn the `qst/AREA` listing into a local Swamp One quest.

