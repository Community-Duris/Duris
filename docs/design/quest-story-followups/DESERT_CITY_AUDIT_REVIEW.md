# The Desert City of Venan'Trut (`desert`): boundary and dependency supplement

Status: bounded static supplement to the
[comprehensive source map](../zone-stories/DESERT_CITY_OF_VENAN_TRUT.md).
It reconciles the active Surface and Limbo boundaries, source-only WLD
references, current title identity, and tracked access history. It does not
qualify runtime availability, ordinary reachability, or player outcomes.

## Active-world boundaries

The active key is `desert`, zone 490, with 677 declared room records in the
noncontiguous 49000–49790 range. The active exit ledger records thirteen
cross-zone declarations: two reciprocal Surface pairs and nine directions
from Jussinload room 49790 to Limbo room 1.

| Active boundary | Evidence |
| --- | --- |
| Desert room 49003 west ↔ Surface room 545019 east | [Desert declaration](../../../areas/wld/desert.wld#L124), [Surface return](../../../areas/wld/surface.wld#L945086), [outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L628), [return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1429) |
| Desert room 49431 east ↔ Surface room 545021 west | [Desert declaration](../../../areas/wld/desert.wld#L11031), [Surface return](../../../areas/wld/surface.wld#L945136), [outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L629), [return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1430) |
| Jussinload room 49790: north, east, west, up, down, northwest, southwest, northeast, southeast → Limbo room 1 | Nine authored exits without reverse records; [ledger rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L630) through [#638](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L638) |

The Mansion dossier separately examines all 713 active portal prototypes: the
fourteen fixed portal arrivals into Desert rooms all originate from local
objects, with no foreign portal arrival. The fixed WLD boundaries and portal
objects do not establish ordinary access, safe traversal, return policy, or a
played journey.

## Active and source-only WLD references

A stateful parse of all 95 current unregistered WLD sources read 2,080,773
authored D0–D9 exits with zero parse issues. One `surf.wld` exit numerically
targets an active Desert room:

| Unregistered source | Direction and target | Evidence |
| --- | --- | --- |
| Room 202208, “The Amber Sands of the Desert Island” | East (`D1`) to active Desert room 49003 | [source room](../../../areas/wld/surf.wld#L46336), [exit](../../../areas/wld/surf.wld#L46344) |

`surf` is not an active `areas/AREA` input. The scan found no source-room VNUM
overlap with Desert's exact active room records. The one exit therefore remains
a source-only numeric reference, not active ingress or proof that the old
Surface map was intended to connect here. It brings the curated source-only
follow-up count from 577 to 578 and the reviewed `surf.wld` subset from eighteen
to nineteen. The broad raw `surf.wld` target count remains 415.

## Bounded typed references

The comprehensive dossier maps the zone's local special assignments, computed
epic teachers, switch and teleport handlers, recipes, shops, and imported reset
dependencies. Its bounded global scan identifies nineteen foreign reset groups
that import Desert actors or gear into Alatorin and Surface Keeps, plus eleven
complete recipe bodies that reference local objects or the foreign locket. These
outbound dependencies are distinct from an inbound route. See
[reviewed evidence and closure](../zone-stories/DESERT_CITY_OF_VENAN_TRUT.md#reviewed-evidence-and-closure)
for exact actors, items, handlers, source rooms, and limits.

## Current title and tracked access history

The current manifests keep `desert` as the source key in both
[`areas/AREA`](../../../areas/AREA#L219) and
[`areas/qst/AREA`](../../../areas/qst/AREA#L200); its zone file titles it “The
Desert City of Venan'Trut” ([title](../../../areas/zon/desert.zon#L2)). The
`(scip)` parenthetical in the manifest annotation is retained as source
context, not treated here as a second active key. The normalized active-title
register has no duplicate-title group for this zone
([title/history register](DUPLICATE_AND_REPLACED_ZONES.md)).

Commit `6f159f03b` (“added venan'trut ship access to dock”) edits both Desert
and Surface WLD sources and records the authored dock access that aligns with
the current reciprocal bay boundary. Commit `087f5ac41` (“Made some mob load
rooms in VT no teleport”) changes three Desert room-flag fields; its diff does
not retarget a WLD exit. The current manifest still lists `desert`, and this
bounded history review found no replacement or retirement signal for that
active key. The old `surf.wld` exit is a separate source-only map reference.

## Audit boundary

These are static source, manifest, and Git-history findings. Neither active
exits nor the unregistered `surf.wld` reference prove player availability,
ordinary access, travel safety, or a complete return journey. Editor/tooling,
persisted-player, and indirect or dynamic dependencies remain outside this
bounded closure. No native source or runtime change is included.
