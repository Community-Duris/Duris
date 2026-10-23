# Mobs_Underdark: shared dispersal rooms and a legacy Zarbonesti chain

Source-comprehensive overview of active `mobs_underdark`, titled “The
Underdark.” Its 15 rooms form a shared dispersal layer for creatures placed
across the larger world. The local native QST file also holds three linked
House Zarbonesti contracts. This contributor is separate from `underdark`,
“The Twisting Tunnels of the Durian Underdark,” and `udmini`, “The Minizones of
the Underdark.” The detailed contract and reset review is in
[`MOBS_UNDERDARK_STORY_REVIEW.md`](../quest-story-followups/MOBS_UNDERDARK_STORY_REVIEW.md);
the bounded title, graph, and typed-reference follow-up is in
[`MOBS_UNDERDARK_AUDIT_REVIEW.md`](../quest-story-followups/MOBS_UNDERDARK_AUDIT_REVIEW.md).

## Contributor and map shape

The active area owns 15 WLD rooms, 23 mobile prototypes, 19 object prototypes,
three QST givers, and 566 zone reset commands. There is no local SHP file or
`mobs_underdark.story.json` sidecar. All room descriptions are empty; the room
titles label creature-dispersal points for Ghore, Drst, water creatures, Purple
Worms, sahuagin, Myconids, Arach/Goblin bubbles, Roper/Earth, and the magma
lake. See the [active WLD](../../../areas/wld/mobs_underdark.wld#L1),
[area list](../../../areas/AREA#L122), and [zone header](../../../areas/zon/mobs_underdark.zon#L1).

The room graph has 14 weak components: thirteen singleton rooms and the
reciprocal up/down pair #24013–#24014. Four of the 98 exits resolve in the
active loader: that local pair and two outbound area links, from #24007 to
Underworld3 #30149 and from #24012 to Ixarkon #96471. The remaining 94
positive exits are unresolved; 18 target VNUMs also occur in inactive `ud`
WLD sources. There are no active inbound WLD exits. These source records do
not establish ordinary access, return, or safe travel.

## House Zarbonesti contracts

The native [QST source](../../../areas/qst/mobs_underdark.qst#L1) contains two
`Q` contracts and one `QA` contract. The item terms and their open source
questions are:

| Giver | Native terms | Source outcome |
| --- | --- | --- |
| Haz'on'wyz, #24021 | Give goblet #1515; receive amulet half #24013; `D` | The exact goblet has no current object prototype. The text says Haz'on'wyz leaves in a pillar of flame. |
| Ozman'dyzus, #24022 | Give bracelet #4660; receive half #24014 and literal `C 150000`; `D` | The Underworld source equips Bregnar with #4660. Local #24014 is a money-type object, so the modern Mushroom Caverns sidecar flags the legacy exchange for source/accounting review. |
| Kryz'Kyssik, #24023 | Give both halves #24013 and #24014; receive seal #24016, weapons #24017 and #24018, and literal `R E 35000`; `D` | The native `QA` record preserves these exact terms; this overview does not assign runtime meaning to `E 35000`. |

The amulet halves are distinct prototypes even though their descriptions are
similar. The existing [source review](../quest-story-followups/MOBS_UNDERDARK_STORY_REVIEW.md#finding-1-three-linked-house-zarbonesti-contracts)
records the full dialogue, the missing goblet prototype, the Bregnar bracelet
source, and the out-of-range giver mapping.

## Reset and cross-area ownership

The zone has 549 mobile reset rows and 17 give-to-mobile object reset rows.
Local mobile prototypes account for 313 resets across 18 of the 23 definitions.
Other mobile imports are from Underworld (187 rows), Connectorzones (22 rows),
RTun (17 rows), and Ghore (four rows). Six further mobile rows request #1301,
for which no current mobile prototype exists. The 17 object rows give
Connectorzones object #53610 fourteen times and local potion #24012 three
times. The [detailed reset inventory](../quest-story-followups/MOBS_UNDERDARK_STORY_REVIEW.md#finding-3-the-area-has-substantial-reset-and-topology-dependencies)
keeps these typed references separate.

Outside resets, active Alatorin has six M commands for local pech miner #24011
at rooms #83916 and #83920 ([first](../../../areas/zon/alatorin.zon#L4190),
[last](../../../areas/zon/alatorin.zon#L4197)). The active Mushroom Caverns
story sidecar maps the three legacy contract givers and their item terms
([contact](../../../areas/story/mushroom_caverns.story.json#L21),
[contract](../../../areas/story/mushroom_caverns.story.json#L124)); it is an
external consumer, not a local Mobs_Underdark sidecar. Compiled CHAOS kit data
also uses seal #24016 in wear slot 24 in eleven class tables
([kit entry](../../../src/account/chaos_eq_data.h#L144),
[kit selection path](../../../src/account/nanny.c#L706)).

## Qualification boundary

The normalized title “The Underdark” has one owner among all 442 current
`.zon` titles. Followed WLD, ZON, MOB, OBJ, and QST history keeps the current
stems through the 2009 area-tree move and later same-stem edits; it establishes
no alternate-stem rename or whole-zone replacement. Keep active
`mobs_underdark` DO NOT MOVE.

The source audit records reset and contract declarations only. Live giver
availability, missing-item resolution, reset timing, cross-area access,
return, player safety, and durable reward outcomes require separate
qualification.
