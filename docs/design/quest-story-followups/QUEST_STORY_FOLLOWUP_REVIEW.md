# Quest story follow-up review

**Status: research and proposed decisions only. No quest, journal, zone, runtime,
or accounting change in this review has shipped.**

## Expanded game-wide audit

The ten cases below are the preserved selective first batch. The owner has
expanded the active research goal to every full-world area contributor and
current unregistered source component. The reconciled 476-row crosswalk (472
current source stems plus four history-only records) is in [Scope and coverage
register](SCOPE_AND_COVERAGE_REGISTER.md) and
[ZONE_COVERAGE_REGISTER.csv](ZONE_COVERAGE_REGISTER.csv). The static exit
census and first source-history pass are recorded in
[World connectivity](WORLD_CONNECTIVITY.md) and
[Duplicate and replaced zones](DUPLICATE_AND_REPLACED_ZONES.md). Per-zone
environmental review, dynamic reachability, typed dependencies, and archive
clearance remain pending where the register says so.

The initial batch recorded ten focused story chains across eight regions and followed
the repository's source-first story workflow: native Q/M/A/R/G/E/P records, room and
reset declarations, item and mobile descriptions, and relevant special handlers
are the evidence for what currently happens. A sentence in dialogue is not by
itself proof that an escort, rescue, kill, learned topic, permanent world-state
change, or campaign stage exists. Existing zone dossiers are cited where they
already contain broader source matrices; this review does not claim a new
exhaustive source/reset or live-play audit of those zones.

Player-facing journal, discovery, encounter, and new credit remain subject to
the active READY accounting boundary. Nothing here authorizes activating those
systems, relaxing an existing durable guard, changing a native recipe, or
granting a new reward. Any later implementation should have its own clearly
scoped fix commit and news item naming the zone, trigger, before/after behavior,
proof, and limits. News wording in the dossiers is marked **UNSHIPPED
PROPOSAL**.

## Reviewed findings

| ID | Zone / chain | Priority and confidence | Primary classification | Proposed disposition |
| --- | --- | --- | --- | --- |
| [QSF-BP-01](ACCESS_AND_DELIVERY_CHAINS.md#qsf-bp-01--black-pearl-campaign-sources) | Black Pearl campaign sources and entry | P1 / high | Confirmed source or entry block; event-vs-playable intent unresolved | Ask the builder whether this is a staff/event route or a repeatably playable campaign. Until then, show only proven item/contract facts and do not promise routine access. |
| [QSF-JADE-01](ACCESS_AND_DELIVERY_CHAINS.md#qsf-jade-01--jade-princess-release-and-token) | Jademini captive princess → Jade Emperor | P2 / high | Missing tracking capability; arrival/reunion intent unresolved | Separate the shackles exchange (release) from later token delivery. Track accepted token hand-in as its own fact; do not infer escort, home arrival, or reunion. |
| [QSF-TOWER-01](ACCESS_AND_DELIVERY_CHAINS.md#qsf-tower-01--amelia-and-dorthans-family-proof) | Tower of Darkness Amelia → Dorthan | P2 / high | Missing tracking capability; “safe” meaning ambiguous | Separate accepted release from any future arrival proof. The shield recipe continues to accept five swords plus the exact locket without personal history. |
| [QSF-JADE-02](MATERIAL_AND_TIMED_CHAINS.md#qsf-jade-02--freed-turtle-and-competing-uses) | Jade freed turtle → soup or silver ore | P2 / high | Intentional scarcity/challenge; source-generation tracking gap | Preserve the one-item competition and allow later reset supplies where available. Do not record a permanent exclusive choice. |
| [QSF-WH-01](MATERIAL_AND_TIMED_CHAINS.md#qsf-wh-01--random-attribute-scroll-and-nine-earrings) | Winterhaven memories → random attribute scroll → nine earrings | P2 / high | Missing tracking capability; settlement risk needs separate qualification | Preserve random outcome and fees. Record only a committed, accepted result with recoverable input/output lineage. |
| [QSF-WH-02](MATERIAL_AND_TIMED_CHAINS.md#qsf-wh-02--three-dragon-hearts-and-the-third-day) | Winterhaven three dragon hearts | P2 / high | Text/mechanic timing mismatch; builder intent unresolved | Keep each physical heart's actual timer and recovery behavior authoritative unless the builder explicitly selects a different clock. Do not require personal boss kills. |
| [QSF-ASH-01](MATERIAL_AND_TIMED_CHAINS.md#qsf-ash-01--ashrumite-silver-to-magical-necklace-story) | Ashrumite silver / gem / necklace story | P1 / high | Ambiguous authored intent | Present the exact current recipes and rewards. Default to prose correction; if a recipe repair is wanted, first choose exact ingredients, prices, and rewards with the builder. |
| [QSF-THAR-01](NARRATIVE_BOUNDARY_CHAINS.md#qsf-thar-01--chilns-medicine-and-the-empty-pouch) | Tharnadia Chiln's medicine story | P2 / high | Intentional narrative twist; any healed-person ending is unimplemented | Describe the accepted empty-pouch exchange as the thief's swindle. Do not award a rescue/healing outcome without a designed positive actor-state event. |
| [QSF-SUN-01](NARRATIVE_BOUNDARY_CHAINS.md#qsf-sun-01--morning-sun-ring-and-forest-restoration) | Temple of the Sun's morning-sun exchange | P2 / high | Ambiguous authored intent | Align the promised mace with the exact ring output. Leave restoration as narrative text unless the builder identifies an actual restoration endpoint and wants one. |
| [QSF-TWF-01](TWIN_TOWERS_FOREST_MILESTONES.md#qsf-twf-01--gardener-belt-access-and-flower-delivery) | Twin Towers Forest belt, waist wear, and flower delivery | P2 / high | Missing durable milestone tracking | Implement the owner's specified obtain + waist-wear + flower-delivery aggregate only after active READY qualification; keep belt access optional to the flower hand-in. |

## Decisions that apply across the review

- A hand-in that checks an exact item proves acceptance/custody of that item at
  that trigger. It does not automatically prove who originally found, made,
  killed for, or delivered it. Preserve valid gifts, trades, looted items, and
  other supplied proof where the native contract accepts them.
- Keep distinct concepts distinct: item acceptance, present custody, source
  provenance, worn equipment, learned topic, access attempt/use, release,
  arrival, and a multi-stage campaign each require their own native evidence.
- Reset cycles and competing item uses can be intentional gameplay. Do not turn
  a cap-one source or single current item into permanent exclusivity when later
  reset supply can make another outcome earnable. Do not change source caps,
  quantities, fees, rewards, access, combat, or PvP as an explanatory repair.
- Where the intended story is unclear, record the exact evidence and ask for a
  builder decision. The default repair for a mismatch is to align prose with
  the existing mechanic; do not silently substitute a guessed recipe or
  world-state event.
- Tower of Darkness “daughter safe” wording, the Jade princess's “saved” text,
  Tharnadia's promise of medicine, and the Temple of the Sun's restoration text
  do not independently prove arrival, healed status, or a persistent world
  change.
- The Twin Towers Forest decision is specific: each character must durably
  obtain the gardener's belt by any accepted route and wear it at the waist;
  Alvinar's accepted flower delivery is a third historical fact. All three may
  occur in any order and survive later belt transfer/unwear. A belt, belt
  conversation, or garden crossing is not a prerequisite for Alvinar's native
  delivery. See the [detailed milestone record](TWIN_TOWERS_FOREST_MILESTONES.md).

## Handoff and remaining review queue

The [handoff](HANDOFF.md) records the base, owned files, validation boundary,
and commit. This set is a decision dossier, not a claim that source availability,
live reset behavior, or active-accounting execution has been qualified.

Ranked next work after this handoff:

1. Resolve Black Pearl's event-vs-repeatable-play intent and ordinary source /
   entrance placement before promising a player-facing campaign route.
2. Before proposing a Winterhaven jewelry fix, finish an exact source-to-output
   review for all 49 memory rewards, nine random outcomes, two-scroll matching,
   final fee settlement, and replay/recovery behavior.
3. Have the Ashrumite builder choose whether the actual recipes are intended or
   whether the dialogue's silver-bar / plain-necklace / magical-reward chain is
   intended; then qualify exact producers and any missing disc before planning
   a native repair.
4. Qualify the live source, reset, and continuation boundaries for the Jade
   princess, Tower of Darkness, Temple of the Sun, and Tharnadia cases before
   making any new persistent outcome credit.
5. Design and qualify durable acquisition and equipment events for the Twin
   Towers Forest aggregate without turning access into a flower prerequisite.
6. Review Temple of the Eleven GODS only as its own active story task if it is
   selected for follow-up. It appeared here as a foreign reference to the
   Hall-owned dagger lead; this task did not reopen Hall or audit Temple of the
   Eleven GODS as a standalone quest.

## Source dossier map

- [Access and delivery chains](ACCESS_AND_DELIVERY_CHAINS.md)
- [Material, random, and timed chains](MATERIAL_AND_TIMED_CHAINS.md)
- [Narrative boundary chains](NARRATIVE_BOUNDARY_CHAINS.md)
- [Twin Towers Forest milestones](TWIN_TOWERS_FOREST_MILESTONES.md)
