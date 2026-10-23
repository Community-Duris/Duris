# Durian Underdark repository-wide audit supplement

This checkpoint reconciles the [source-comprehensive Durian Underdark dossier](../zone-stories/DURIAN_UNDERDARK.md) with the full active-area title, graph, inactive-WLD, typed-dependency, and bounded history passes at source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. It documents source evidence and open decisions; it does not change native gameplay or move a zone.

## Active graph and traversal limits

The current `underdark` area has 13,587 rooms and 53,661 authored exits, with no parse errors. The full active world uses 351 WLD contributors. Loader qualification resolves 46,848 exits: 46,747 stay within the area and 101 cross to other active areas. Another 6,813 raw targets are absent sparse-map cells and are pruned during target resolution ([`renum_world`](../../../src/world/db.c#L1531)). The 101 inbound and 101 outbound cross-area records form reciprocal pairs; inbound records come from 53 area owners and land in 101 local rooms.

The 13,587-room local graph has six weak components of 13,192, 274, 102, 14, 4, and 1 rooms. No local room record has a fall-chance or current field. The existing dossier's terrain-aware traversal starts at distributor #847218 and the active inbound leaves; it applies the mountain map-entry restriction in [`actmove.c`](../../../src/cmd/actmove.c#L842). Those source-graph counts do not establish safe, ordinary-player, or played reachability. The random Underdark portal, dynamic relic paths, room hazards, NPC circulation, occupancy, and other runtime conditions remain separate.

## Inactive-WLD sweep and the Underdark 2011 candidate

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse errors. It found no room-VNUM overlap with the active `underdark` area and no inactive-WLD exit into its active rooms. This adds no source-only approach, so the reviewed `surf` subset stays at 142 and the curated inactive-WLD approach total stays at 1,532.

That negative result does not clear `underdark2011`. Its candidate row still records 11 active WLD exits from five area owners into VNUMs in that unregistered source. The candidate also contains the unresolved #217609 room-identity difference across `surf`, `surface2011`, and `underdark2011`. No approach into current `underdark` resolves those separate references or the disputed room identity. Keep `underdark2011` **RETAIN** in place pending dependency, map-ownership, and tooling closure; see the [coverage row](ZONE_COVERAGE_REGISTER.csv#L466) and [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115).

## Typed cross-area dependencies

| Evidence | Source-backed result |
| --- | --- |
| Active WLD boundary | 101 reciprocal inbound/outbound pairs touch the current sparse map; the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) carries each directional record. Kobold room #1542 enters local #816080, and the Kobold orientation note names that same local room ([active exit](../../../areas/wld/kobold.wld#L1542), [sidecar](../../../areas/story/kobold.story.json#L15)). |
| External QST item use | White Hills Q2605 consumes local bronze Zarbonesti seal #700005 for ring #55054, 250,000 copper, and 250,000 experience; the giver stays ([WH QST](../../../areas/qst/wh.qst#L2612), [story request](../../../areas/story/wh.story.json#L1020)). This is a separate receipt and does not require local amulet history. |
| External reset reuse | Alatorin has three active reset references to local mobile #700066 at lines 4185, 4186, and 4189 ([Alatorin ZON](../../../areas/zon/alatorin.zon#L4185)). |
| Similar-number false positive | SurfaceKeeps' `R C 700000` command is a currency reward, not an item reference to local object #700000. Its matching sidecar currency tokens are monetary. |
| Bounded absence checks | No external SHP product for local objects, outside duplicate active mobile/object prototype, type-25 declaration targeting this area, or typed outside sidecar item reference was found. The local area has no SHP file and no local QST item command imports outside prototypes. |

The code and data range review also found broad system dependencies that do not represent additional quest receipts: Underdark map-range handling in [`map.h`](../../../src/world/map.h), mining range tables ([`mining.c`](../../../src/economy/mining.c), [`mining_config.c`](../../../src/economy/mining_config.c)), kingdom harvest placement ([`kingdom_harvest.c`](../../../src/kingdom/kingdom_harvest.c)), and legacy map generation ([`random.zone.c`](../../../src/world/random.zone.c)). Enabled purple-worm mobile #700004 is bound to its swallowing and corpse handler ([assignment](../../../src/specs/specs.assign.c#L580), [handler](../../../src/specs/specs.underworld.c#L1491)). The enabled Underdark portal chooses a live destination dynamically ([`ud_portal`](../../../src/specs/specs.heavens.c#L1471)). A former Khildarak map-distance reference is inside a commented block; current guild admission does not use it. Mining, kingdom-node, guild, worm, and portal behavior still need outcome-specific review where a future story depends on them.

## Title, history, and disposition

After removing color tokens and normalizing case, “The Twisting Tunnels of the Durian Underdark” has one exact owner among the 351 active non-empty zone titles: `underdark`. The other lexical title matches, “The Underdark” (`mobs_underdark`) and “The Minizones of the Underdark” (`udmini`), are different titles and source identities.

Followed `underdark.wld`, `.zon`, `.mob`, `.obj`, and `.qst` histories stay on the same stem after the 2009 `areas/` relocation. The story sidecar was added in commit `26fd4a945` on 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these bounded paths. This does not establish the absence of an untracked predecessor. Preserve active `underdark`; it remains **DO NOT MOVE**. Keep the separate `underdark2011` candidate **RETAIN**.

## Builder follow-ups

The existing dossier carries twenty source-specific follow-ups and its associated builder questions. Open evidence includes the ordinary admission of 50-percent reset stock, active-accounting authority for fresh item issuance, the selected source for exact skin #700008, and the dialogue/native bounty amount mismatch. Dynamic portal and relic state, purple-worm custody/release, mine and regional-node outputs, guild outcomes, NPC circulation, and terrain-conditioned travel need their own committed outcome evidence before they can support persistent progress. No source or gameplay repair is implied by this audit.

## Checkpoint 411: explicit builder-question coverage

Physical coverage row 324 now has exactly twenty numbered, direct-source
builder questions with conservative resolutions in the [Durian Underdark
dossier](../zone-stories/DURIAN_UNDERDARK.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the two native material contracts while keeping reset
issuance, source recovery, terrain-conditioned travel, worm custody, the
dynamic portal, relic mutations, and outside White Hills acceptance separate.
No source or gameplay repair changed.
## Checkpoint 866: sparse-map and candidate boundaries carried forward

CP866 carries forward the Checkpoint 119 source audit against the unchanged pinned area/code snapshot. Active `underdark` still has 13,587 rooms and 53,661 authored exits; loader qualification resolves 46,848 (46,747 local, 101 outbound) and prunes 6,813 absent sparse-map cells. Its 101 inbound and 101 outbound active boundary records form reciprocal pairs. The local graph retains six weak components sized 13,192, 274, 102, 14, 4, and 1, with no authored fall/current fields. Mountain terrain admission, dynamic portals, mining/harvest systems, hazards, and other runtime conditions remain separate from this static reach evidence.

The all-95 inactive-WLD sweep still finds no room overlap or approach into active `underdark`. That does not resolve the distinct `underdark2011` candidate: 11 active WLD records from five owners still target its unregistered VNUMs, including #217609 whose identity is ambiguous across `surf`, `surface2011`, and `underdark2011`. Keep those references on existing `underdark2011` candidate row 115; the active map's negative sweep does not clear that candidate.

The bounded typed closure remains unchanged: White Hills Q2605 consumes local seal #700005; Alatorin has three reset rows reusing local mobile #700066; Kobold's sidecar orientation matches its active boundary; no external SHP product, outside duplicate active prototype, type-25 target, or typed outside sidecar item reference was found. Keep active `underdark` **DO NOT MOVE** and candidate `underdark2011` **RETAIN IN PLACE**. The older 1,532 overall / 142 Surf source-only aggregate at CP119 is historical; current cumulative source-only totals remain 11,993 overall / 415 Surf. See the [source dossier](../zone-stories/DURIAN_UNDERDARK.md), [current candidate closure](#inactive-wld-sweep-and-the-underdark-2011-candidate), and [coverage row 324](ZONE_COVERAGE_REGISTER.csv#L324).
