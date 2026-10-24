# CBL Outpost 2 Audit Review

Reviewed October 11, 2026, Checkpoint 461. This is a static source, narrative, WLD graph, followed-history,
and bounded typed-reference review of the unregistered `cbl_outpost2` component. It does not establish
active loading, player access, reset execution, or permission to move or merge source files.

## Source scope and identity

The current source family contains [WLD](../../../areas/wld/cbl_outpost2.wld),
[MOB](../../../areas/mob/cbl_outpost2.mob), and [ZON](../../../areas/zon/cbl_outpost2.zon). Its
zone header is #1000, titled “CBL Guildhall Monster Island,” with last VNUM #1004 and reset mode 2.
The WLD defines five rooms #1000–#1004; the MOB defines four prototypes #1000–#1003. There is no
same-stem OBJ, SHP, QST, or story sidecar. The component is not listed in the full
[area manifest](../../../areas/AREA) or independent [quest manifest](../../../areas/qst/AREA).

## Environmental story and mobile roles

Room #1000 has the generic title “First Room” and an empty description
([first room](../../../areas/wld/cbl_outpost2.wld#L1)). Room #1001 is a torture chamber, #1002 sits
beside a great pit, #1003 is a narrow stairway down the pit wall, and #1004 is titled “The Portal
Cave” ([torture chamber](../../../areas/wld/cbl_outpost2.wld#L10),
[pit edge](../../../areas/wld/cbl_outpost2.wld#L41),
[stairway](../../../areas/wld/cbl_outpost2.wld#L72),
[Portal Cave](../../../areas/wld/cbl_outpost2.wld#L101)). The prose uses blood, torture, and undead
body-horror imagery. The Portal Cave calls itself a ticket off the island, but its only authored WLD
exit returns to #1003; the reviewed source group contains no local portal object, quest, or handler
contract that explains the departure.

The four mobile prototypes are an undead ghoul golem, decaying flesh golem, ape-like zombie golem,
and furious lich golem ([mobile definitions](../../../areas/mob/cbl_outpost2.mob#L1)). Their
descriptions use grotesque and violent body-horror imagery. This is environmental and prototype
text, not a native QST or a complete encounter specification.

## Static WLD graph and range discrepancy

The candidate WLD has five rooms and eight authored directions. All eight target the candidate's
local #1000–#1004 set. The local graph is one weak component, and authored directed traversal from
#1000 reaches all five rooms. There are no outbound cross-zone directions, negative destinations,
fall-chance fields, or current fields.

The source room fields and ZON header say 1000, while the VNUMs #1000–#1004 fall in numeric band 10
under the register's VNUM/100 convention. The register therefore records all five rooms outside
that numeric band and all eight exits as originating in range-mismatched rooms. The shared raw value
1000 is source evidence, not a decision that the old numbering is correct or should be changed.

No active WLD room owns #1000–#1004, and no active WLD exit targets those room numbers; the active
[CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md#title-history-and-duplicate-content-review) confirms the
active map instead uses #29820–#29824 for the matching Monster Island rooms. Thus the eight positive
candidate-local exit targets are unresolved in the active world loader, while candidate-specific
active inbound remains zero. The map contributes no active access or eligible source-only approach.
See the [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-461-cbl-outpost-2-graph-vnum-and-mobile-overlap).

## Monster Island copies and active CBL Hall mapping

Unregistered `cbl_outpost2` and [unregistered `Monsteri`](../../../areas/wld/Monsteri.wld#L1) use the
same ZON title, room VNUMs #1000–#1004, mobile VNUMs #1000–#1003, and four M-reset placements into
room #1000. They have closely matching room content, but `cbl_outpost2` leaves its first room title
generic and its description empty. The active [CBL Hall](../../../areas/AREA#L143) copies the five
Monster Island room descriptions into its separate #29820–#29824 range; its four corresponding
active mobile roles use #29820–#29823 ([active rooms](../../../areas/wld/cbl_hall.wld#L1629),
[active reset placements](../../../areas/zon/cbl_hall.zon#L86)).

These are strong duplicate-content relationships, not evidence that either unregistered source is
canonical or that either one replaced active CBL Hall. The active CBL Hall content and the two
unregistered source identities remain separate pending an owner decision. See the existing
[Checkpoint 199 duplicate review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-199-cbl-hall-and-monster-island-copies)
and [active CBL Hall audit](CBL_HALL_AUDIT_REVIEW.md#title-history-and-duplicate-content-review).

## Typed reset and active mobile-VNUM collision

The unregistered ZON has four M commands. Each places one local golem prototype #1000–#1003 in
candidate room #1000 ([reset group](../../../areas/zon/cbl_outpost2.zon#L10)); it has no O or G
commands. The matching `Monsteri.zon` has the same reset group. These are source-only reset records.

Active Heavens also defines mobile VNUMs #1000–#1003: #1000 is named “unkillable bastard,” and
#1001–#1003 are peasants ([active Heavens prototypes](../../../areas/mob/heavens.mob#L1867)). They
have different identities from the four unregistered golems. The bounded scan found no current
active ZON M command using those Heavens mobile VNUMs, but the active prototypes still establish a
typed numeric collision that must be resolved before considering any activation or source move.
A narrow literal lookup scan found no direct `real_room`, `real_mobile`, or `real_object` call for
#1000–#1004; editor, script, and persisted references remain outside that scan.

## Followed history and replacement review

Followed paths show `cbl_outpost2` WLD, MOB, and ZON at the 2009 areas-tree move, followed by same-stem
maintenance through the 2013 “stuff” commit. No alternate-stem rename or explicit replacement
decision was found in those paths. `Monsteri` and active CBL Hall have their own tracked source
histories. Shared room prose and matching reset layouts do not establish which source is canonical
or whether one replaced another.

## Safe disposition

Keep `cbl_outpost2` **RETAIN IN PLACE**. Resolve the source owner and intended relationship to
`Monsteri` and active CBL Hall; the #1000–#1004 room range and zone-number convention; the active
Heavens mobile overlap; the matching reset groups; the blank first room; the Portal Cave destination;
and the intended encounter and content tone. Close editor/tooling and persisted identity before any
separate path or activation proposal. No area, code, reset, manifest, or world-list change is
authorized by this audit. The owner-question list is in the
[CBL Outpost 2 builder questions](CBL_OUTPOST2_BUILDER_QUESTIONS.md).
