# The Village of New Hope: repository-wide source audit supplement

This supplement reconciles the [source-comprehensive New Hope dossier](../zone-stories/THE_VILLAGE_OF_NEW_HOPE.md)
with active WLD boundaries, the all-source WLD sweep, typed cross-zone
references, title and replacement history, and shared-code use. Findings come
from static repository sources. They do not establish live access, keeper
availability, played receipts, or persisted player history.

## Active boundaries and local graph

New Hope has 228 rooms and 550 authored exits. The active WLD scan resolves all
550 targets: 548 exits stay in New Hope and two leave. Its local WLD-only weak
graph is one component of 228 rooms. Both outbound edges start at walkway
#89217 and lead to Surface:

| New Hope exit | Surface target | Reverse exit | Boundary records |
| --- | --- | --- | --- |
| #89217 east | #618434 | Surface #618434 west to #89217 | [New Hope](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1104), [Surface](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1620) |
| #89217 south | #618833 | Surface #618833 north to #89217 | [New Hope](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1105), [Surface](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1621) |

These are the area's two active inbounds and two outbounds; both pairs are
reciprocal. The static graph excludes the local locked doors, hidden exits,
breathing, combat, scripted death, and imported mindstone travel reviewed in
the New Hope dossier. WLD topology alone does not establish ordinary access,
return safety, or successful service use.

## Unregistered WLD source sweep

The all-95-file scan parsed 527,791 rooms and 2,080,773 exits with zero parse
issues. No unregistered source room shares a VNUM with New Hope, and no
unregistered exit targets a New Hope room. There is no source-only WLD ingress;
the curated source-only total remains 602.

## Typed dependencies and source ownership

Exact external QST item-command and SHP product scans found no references to
New Hope objects. All 231 external story sidecars parsed without issues and
had no exact structured VNUM match. The bounded typed ZON scan found four
active Alatorin object resets of New Hope table #89008
([prototype](../../../areas/obj/newhope.obj#L139)), each placed in one of four
Alatorin rooms:

| Reset placement | Active Alatorin room | Evidence |
| --- | --- | --- |
| Table #89008 → #83513 | Hammer and Shield Tavern | [reset](../../../areas/zon/alatorin.zon#L1635), [active room](../../../areas/wld/alatorin.wld#L8597) |
| Table #89008 → #83514 | #83514 | [reset](../../../areas/zon/alatorin.zon#L1636), [active room](../../../areas/wld/alatorin.wld#L8629) |
| Table #89008 → #83516 | #83516 | [reset](../../../areas/zon/alatorin.zon#L1638), [active room](../../../areas/wld/alatorin.wld#L8692) |
| Table #89008 → #83518 | #83518 | [reset](../../../areas/zon/alatorin.zon#L1640), [active room](../../../areas/wld/alatorin.wld#L8754) |

Unregistered `nexus56.wld` records repeat those four VNUMs with different room
descriptions ([first alternate record](../../../areas/wld/nexus56.wld#L9112)).
The active manifest includes `alatorin`
([entry](../../../areas/AREA#L300)) and does not include `nexus56`, so the
active room records for these reset targets are the Alatorin records. This is
an Alatorin reset reuse of a New Hope object, not a New Hope quest receipt; it
does not assign active room ownership to the unregistered Nexus56 map.

The typed scan found no external reset room field targeting a New Hope room,
no external mobile or object prototype definition reusing a local VNUM, and no
foreign type-25 portal destination into New Hope among 872 declarations
scanned. These negative findings are bounded to the exact typed records
checked.

Shared code references are classified by context. The active assignment binds
tentacler #89181 to its zone-specific `tentacler_death` procedure
([assignment](../../../src/specs/specs.assign.c#L816)). That procedure chooses
one of objects #89145–#89149 and places it in vault room #89227
([selection](../../../src/specs/specs.newhope.c#L29),
[placement](../../../src/specs/specs.newhope.c#L75)); this is the local custom
death path already reviewed in the dossier. Shared CHAOS kit tables also list
local objects #89169, #89136, and #89143: [standard anti-paladin kit](../../../src/account/chaos_eq_data.h#L109),
[enhanced anti-paladin kit](../../../src/account/chaos_eq_data.h#L360),
[standard reaver kit](../../../src/account/chaos_eq_data.h#L255),
[enhanced reaver kit](../../../src/account/chaos_eq_data.h#L499), and
[bard kit](../../../src/account/chaos_eq_data.h#L450). These code paths do not
create additional New Hope QST receipts.

## Title, tracked history, and disposition

The active manifest lists New Hope as `*890-892`
([AREA entry](../../../areas/AREA#L316)); its ZON title is “The Village of New
Hope” ([header](../../../areas/zon/newhope.zon#L2)). The normalized title scan
found one match among 351 active titles. The curated
[duplicate and replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no
New Hope entry. The active WLD duplicate scan found no New Hope room collision;
its only global duplicate is #51800 in the two Hell areas
([Hell Three](../../../areas/wld/hell_three_minauros.wld#L1),
[Hell Two](../../../areas/wld/hell_two_dis.wld#L10375)).

Tracked WLD, MOB, OBJ, ZON, QST, and SHP paths reach the 2009 area-tree move
`642da5400`. Later history includes the 2014 shop-file cleanup, 2015–2016
object and reset edits, and the 2026 line-ending and code-refactor commits. The
story sidecar was added at `fdafff753` on 2026-10-05. This bounded path/register
review identifies no distinct replacement source. New Hope remains a full
active area contributor, so its coverage disposition stays **DO NOT MOVE**.

## Recommended reconciliation

- Keep the four paid smith services independent and retain their existing
  receipt identities. The static source review does not add a story finale,
  daily credit, or played service result.
- Keep the four table resets attributed to active Alatorin room records. Treat
  the different same-VNUM Nexus56 room records as unregistered alternate
  source; if that map's active role changes, review its manifest membership and
  reset-room ownership together.
- Builder question: Are the Nexus56 room records intended to remain an
  unregistered alternate map? Current active reset placement resolves against
  the Alatorin room records.
- Preserve New Hope's active manifest membership. This audit supports no attic
  move or native gameplay repair.

This checkpoint is a bounded static source audit. Indirect code paths,
editor/tooling consumers, persisted player data, live service availability, and
played discovery, access, combat, and return journeys remain outside its scope.

## Checkpoint 308: all-442 title, followed history, and graph confirmation

After color stripping, “The Village of New Hope” is unique among all 442
current `.zon` titles. The followed WLD/MOB/OBJ/ZON/QST/SHP histories remain on
the `newhope` stem. They include the 2009 area-tree move, the 2014 shop-file
cleanup, intervening object and reset edits, and 2026 text maintenance and code
refactoring. The story sidecar was added on 2026-10-05 (`fdafff753`). This
bounded review found no alternate-stem predecessor or explicit whole-zone
replacement.

The active WLD graph was rechecked from entrance #89000: all 228 rooms are
reachable and returnable by authored exits. The prior all-95 inactive-WLD
sweep found no room overlap or source-only ingress, so no attic candidate or
route changes. Coverage row 206 now records the ten SHP keeper records at
#89091, #89096, #89097, #89098, #89115, #89116, #89117, #89118, #89120, and
#89131. CP67's existing narrative, graph, and bounded typed-closure findings
remain the source record; this update adds no gameplay or migration decision.

## Checkpoint 308: twenty direct builder questions and recommendations

1. **Should New Hope's four paid mithral crafts remain independent services instead of a staged campaign?** Recommendation: keep each authored recipe and receipt distinct unless the content owner defines explicit eligibility and progression between them.
2. **Should any of the four accepted service receipts grant New Hope story completion or daily credit?** Recommendation: retain the current service-only meaning and add a separate owner-approved outcome before creating a finale or repeatable credit.
3. **Which exact ordinary weapons should the four recipes consume?** Recommendation: resolve the source mismatches for Q35, Q43, and Q60 with the recipe owner, then align the M dialogue to the approved accepted VNUMs; do not silently change recipe IDs.
4. **Should captions for ordinary dagger #89140 and shortsword #89141 be corrected to describe those exact inputs?** Recommendation: make any text repair match the accepted prototype identity and leave shining outputs #89118/#89119 separate.
5. **Are the authored fees of 45,000, 100,000, 250,000, and 10,000 copper intended as required payments for the four services?** Recommendation: preserve the listed prices and require owner review of payment behavior before advertising a paid exchange as available.
6. **What should happen if a player supplies the weapon and mithral but payment or reward delivery fails?** Recommendation: require one settled transaction covering all inputs, fee, output, recipient, and receipt before claiming a completed craft.
7. **How should the long-sword and two-handed services handle their shared ordinary long sword #89142?** Recommendation: reserve and consume the exact accepted input once, and explain that one copy cannot satisfy two competing crafts.
8. **Should a shining service reward ever count as an input to another mithral service?** Recommendation: retain the distinct VNUMs and require an explicitly authored recipe before treating rewards as reusable collateral.
9. **Are all four service givers intended to remain available under their current reset caps and chances?** Recommendation: preserve the recorded reset limits and qualify keeper availability before promising repeat visits or stock.
10. **Should raw mithral #89117 remain available only through the current Zarix-cell and maljer-hut reset placements?** Recommendation: keep those sources and their caps documented, and let the area owner decide before adding a new supply path.
11. **Should the ten SHP blocks listed on row 206 remain assigned to their current keepers and policies?** Recommendation: retain the corrected ten-record inventory and have the shop owner review keeper identity, stock, prices, and policy together if a change is proposed.
12. **Should New Hope's Alatorin reset reuse of table #89008 in four active rooms remain supported?** Recommendation: keep the four placements attributed to active Alatorin room records and review the table's ownership if those resets change.
13. **How should builders handle `nexus56.wld` records that reuse those room VNUMs with different descriptions?** Recommendation: keep them classified as unregistered alternate source while `nexus56` remains outside `areas/AREA`; reassess map and room ownership together if it is activated.
14. **Should the guarded-vault approach through Hammerhead's heart and room #89188 remain secret, locked, and pickable as authored?** Recommendation: preserve the current access puzzle and record its intended entry conditions before changing door flags or key behavior.
15. **Should the inner lair-to-vault door continue to require eye #89188 on entry and cracked dagger #89152 on the authored return?** Recommendation: preserve the directional key identities and confirm both legs separately before describing the vault as an ordinary round trip.
16. **Should builders keep the tunnel sector and underwater lair sector as currently authored despite the submerged prose?** Recommendation: retain tunnel #89219's swamp sector and lair #89220's underwater sector unless an owner review specifies a coordinated sector, breathing, and travel change.
17. **What event should establish that tentacler #89181 personally created a vault item?** Recommendation: attribute only the selected #89145–#89149 object created by the custom death path to a committed kill attempt and exact object lineage; reset stock or pickup alone is not proof.
18. **Should each tentacler death continue to select one of five possible items for room #89227?** Recommendation: retain the authored random selection and five independent reset rolls, and confirm caps and recipient ownership before promising a specific reward.
19. **Should the scholar, prisoners, Hammerhead's soul, and village curse remain ambient lore without an accepted endpoint?** Recommendation: preserve their current narrative state and add no rescue, liberation, transformation, or repair outcome until a builder defines its actors, eligibility, and resulting world state.
20. **What evidence should distinguish access to a shared-open vault from a player's own solved route and return?** Recommendation: require per-player, route-specific entry and matched return evidence before describing discovery or safe completion as personal progress.
