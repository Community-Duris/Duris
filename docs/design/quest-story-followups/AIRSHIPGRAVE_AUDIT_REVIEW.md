# Dawndale (`airshipgrave`) audit reconciliation

Reviewed October 12, 2026 against the current checkout and the source snapshot
recorded in [coverage row 3](ZONE_COVERAGE_REGISTER.csv#L4). The
[Mountain Valley of Dawndale dossier](../zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md)
is the source-comprehensive story inventory. This supplement closes the
identity, connectivity, replacement-history, direct typed-dependency, and
builder-question reviews. It is static repository evidence; it does not claim
live reset, travel, accounting, or player qualification.

## Identity and disposition

The current full-area contributor is `areas/airshipgrave.*`, registered as
`*775-776` with parenthetical area note `(Alachest)` and titled **The Mountain
Valley of Dawndale** in ZON. After color-code removal and case/space
normalization, that title is unique among all 442 current ZON titles. `Alachest`
is not the ZON title; whether the parenthetical roster note is also intended as
a player-facing or command-target alias remains a builder question. Followed
WLD, MOB, OBJ, and QST histories remain under the `airshipgrave` stem; the
native-file histories reach the 2009 repository move and later same-stem edits.
The current ZON and story sidecar are also under `airshipgrave`. No tracked
alternate-stem rename, same-title active duplicate, distinct predecessor, or
whole-zone replacement was established.

Keep the active area **DO NOT MOVE**. It is registered in `areas/AREA` and
`areas/qst/AREA`, not an attic candidate. Preserve its #77500-series
identities, three reciprocal Surface boundaries, native outcomes, and direct
foreign story edges. No gameplay or runtime change is proposed.

## Story and environmental-story scope

The source-comprehensive [Dawndale dossier](../zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md)
reviews all 17 native blocks, 150 rooms, 69 mobiles, 66 objects, 321 resets,
three Surface routes, relevant code paths, and bounded foreign sources and
consumers. It distinguishes 13 native exchanges as nine independent outcomes,
three support services, and one excluded fossil referral. It also inventories
27 contacts and 31 optional material or producer checks. The environmental
prose about rescue, escape, tunneling, ritual, telescope use, or a ship-wide
curse is not treated as proof that those persistent effects exist.

The dossier remains the exact recipe and proof authority. In particular:

- Four sunlight vials are required for the two rival captain bundles, while
  three are declared in Dawndale. Other materials have their own caps,
  probabilities, producer histories, and competing uses; those declarations
  do not prove admitted supply or successful renewal.
- Scholar book #77551 is a local proof and has an active Heavens reset into an
  administrative bookcase. Shared global item caps mean that retained or
  admitted copies can affect ordinary renewal; the library is not thereby a
  public Dawndale quest source.
- The named recipient's current retirement, item receipt, or explanatory
  dialogue remains separate from first-source recovery, travel, party
  relocation, a captain's ritual, escape, telescope installation, or curse
  resolution.

## Direct typed dependencies

| Direction | Exact source-backed relationship | Classification |
| --- | --- | --- |
| Reciprocal WLD | Dawndale #77543 North ↔ Surface #616080 South; Dawndale #77557 East ↔ Surface #617684 West; Dawndale #77557 South ↔ Surface #618083 North | Three authored inbound and outbound routes; all three pairs are reciprocal. The pair census is [Dawndale → Surface, row 5](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L5) and [Surface → Dawndale, row 459](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L459). |
| Dawndale → Home of the Divine → Dawndale | Electrum #77539 from Dawndale is accepted by Emition in Home of the Divine for flute #40778; Dawndale's Whetstone then accepts #40778 and returns the distinct same-named flute #40779. | A two-zone source, preparation, and return chain, not one local reward or one shared receipt. See the [Divine dossier](../zone-stories/HOME_OF_THE_DIVINE.md) and Dawndale's native Q lines 156 and 180. |
| Forest City of Aravne → Dawndale | Aravne's sand elemental mobile #21673 creates object #21673 through its death callback; Dawndale's smith accepts the exact sand with 25,000 coin for lens #77566. | The same VNUM denotes separate mobile and object prototypes. The callback output and mixed-payment service are distinct from player recovery and any lens installation. See the [Aravne typed-reference review](FOREST_CITY_OF_ARAVNE_AUDIT_REVIEW.md#typed-references-and-external-producers). |
| Dawndale → Longhollow | Dawndale's refugee Q awards moonstone #34464; Longhollow has a separate local producer for the same prototype and its own receipt. | Two producers of a shared proof, not a replacement identity or shared completion. See the [Longhollow dependency review](LONGHOLLOW_AUDIT_REVIEW.md#typed-dependencies-and-source-identity). |
| Dawndale → Surface | Object #77521 at #77604 has a type-25 destination at Surface #522469. | Code-dispatched object travel, separate from the three reciprocal WLD routes. Its destination declaration alone does not certify admission, access policy, or safe return; see the [Surface audit](SURFACE_AUDIT_REVIEW.md). |
| Heavens → shared Dawndale prototype | Heavens reset data places book #77551 into a library bookcase; Dawndale's two captain exchanges also consume the book kind. | A foreign reset/cap dependency. It does not establish a public route to that copy or guaranteed ordinary renewal. |

These direct relationships, local competing sources, and story-sidecar
provenance are reconciled across this supplement and the linked source dossiers.
The main Dawndale dossier explicitly bounds its wider foreign-zone review; live
availability, shared-cap behavior, accepted reset generation, and actual
cross-zone player journeys remain unqualified.

## World graph and reachability boundary

The 150-room Dawndale WLD has 311 source exits, all 311 resolving in the
audited loader census. The local weak graph has one component containing all
150 rooms. Its three active inbound and three active outbound WLD records are
the reciprocal Surface boundaries above. The static census records no
negative or unresolved positive target, numeric-range mismatch, or
unregistered WLD target. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L4)
and exact [world connectivity records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L890).

This is authored-map connectivity, not proof that every player can enter the
elf gate, use a route, satisfy a door or travel condition, find a reset actor,
return safely, or complete a producer history. The dossier's key, switch,
container, portal, reset, and accounting distinctions remain in force.

## Builder questions and conservative recommendations

The questions make unresolved intent reviewable. None records an owner answer
or authorizes a content change.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| DAWN-01 | Is `(Alachest)` in the area roster meant to be a player-facing name or a command-target alias, or is it only grouping metadata? | Retain the registered `airshipgrave` key and unique ZON title **The Mountain Valley of Dawndale**; add an alias only after its exact player-facing target and scope are confirmed. |
| DAWN-02 | Should elf portal #77521 impose an elven-race restriction, and what return route is intended from its secluded Surface destination? | Do not infer a restriction from its warning prose or call it a safe round trip. Confirm eligibility and return intent before changing the current portal. |
| DAWN-03 | Should the coin-only city-key purchase and mixed-payment lens commission remain guarded support services? | Preserve native prices, outputs, and accounting guards. If transactions are to be supported, first qualify atomic payment, accepted reward issuance, interruption, and recovery; do not convert service use into story credit. |
| DAWN-04 | Is the four-vial requirement across both rival captain bundles intended when only three sunlight vials are declared? | Preserve the recipes and exact producer count while testing retained copies, consumption, ordinary/forced renewal, and accepted generation. Ask before changing cap, isolation, or recipe. |
| DAWN-05 | Is Heavens' administrative bookcase copy of #77551 meant to share the cap with Dawndale's captain recipe? | Preserve the foreign reset and local proof identity. Do not present the administrative copy as a public workaround; choose stock or isolation policy only with its owner. |
| DAWN-06 | Is the rock-cutter's same-kind fossil exchange meant to lead to a specific Ultarium cousin or further quest? | Keep it excluded as a same-kind replacement/referral until the intended recipient and exact follow-up are supplied; do not invent a completed chain. |
| DAWN-07 | Should refugee/engineer deliveries actually settle the camp or remove the ship-wide curse? | Keep accepted item hand-ins separate from actor relocation and persistent campaign state. If desired, specify destination, affected actors/party, durable effect, safe return, and replay/recovery. |
| DAWN-08 | Should Drandagger's larvae delivery open a tunnel or complete an escape? | Preserve the current reward exchange without claiming an exit; define a builder-owned route and escape outcome before adding one. |
| DAWN-09 | Should the captain's book delivery trigger a ritual, learned knowledge, rival duel, or curse change? | Keep the consumed-book recipe and its story text separate. Define the action, participants, actor/world target, and accepted durable result before crediting any such event. |
| DAWN-10 | Is a telescope installation or scientific use intended for the smith's returned lens? | Do not declare the telescope repaired. Specify the actual installation/use action and item custody if that story endpoint is wanted. |
| DAWN-11 | Should Ender's targeted victim message be present, given both supported paths read `_victim_msg` while the prototype key is `_vict_msg`? | Treat this as a narrow content-key question. Correct and qualify only the intended audience text; preserve spell, weapon power, quest contracts, and proc chance. |
| DAWN-12 | What durable generation, source, and recovery events should count for Aravne's sand callback? | Qualify callback continuation, object creation, custody, recovery, and interruption/recovery before recording a first-source milestone. No live crash is established by this source finding. |
| DAWN-13 | Which of the nine potential daily candidates are suitable with the current sources, retirement behavior, and access? | Keep source-complete recipe shape separate from daily readiness. Qualify availability, repeat supply, and recipient renewal first; do not designate the four retiring-giver contracts as daily-ready without that evidence. |
| DAWN-14 | Should a Water bucket offering also verify its liquid identity or amount? | Keep the current exact object-kind contract until an owner decides whether contents are part of the story; if so, specify fill state, volume, and a recoverable acceptance rule. |
| DAWN-15 | Which remaining defense, bounty, captive, teaching, optics, and equipment descriptions are intended to become accepted story terminals? | Treat them as environmental or service lore by default. Select a precise action and observable outcome before creating any new milestone. |

The source dossiers for Dawndale, Aravne, Home of the Divine, Longhollow, and
Surface contain the full local evidence and neighboring-zone detail; this table
keeps the owner decisions explicit without duplicating their inventories.
