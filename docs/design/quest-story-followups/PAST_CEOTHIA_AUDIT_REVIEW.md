# Past Ceothia (`ceopast`): boundary and phase-history supplement

Status: bounded static supplement to the
[comprehensive Past Ceothia source map](../zone-stories/PAST_CEOTHIA.md).
It reconciles the phase's active WLD boundary status, source-only map scan,
cross-phase typed references, and tracked source history. It does not qualify
runtime availability or player outcomes.

## Active and source-only WLD boundaries

The active key is `ceopast`, zone 811. Its exact active WLD contains 293 rooms,
81100–81392, and the cross-area exit register has no inbound or outbound WLD
declarations for this key. This differs from Present Ceothia's active Surface
pair and Potions staging/wander exits documented in the
[Ceothia supplement](CEOTHIA_AUDIT_REVIEW.md).

A stateful parse of all 95 current unregistered WLD sources read 2,080,773
authored D0–D9 exits with zero parse issues. It found no exit target matching
Past Ceothia's exact active room VNUMs and no unregistered source-room VNUMs
overlapping those rooms. No source-only record is added to the curated
connectivity count.

## Cross-phase portal and typed references

Past has no ordinary WLD boundary or local portal back to Present. The
comprehensive dossier identifies Present's fixed portal 80816 as the only
foreign portal arriving at Past, at room 81100
([prototype](../../../areas/obj/ceothia.obj#L263),
[destination](../../../areas/obj/ceothia.obj#L270),
[reset](../../../areas/zon/ceothia.zon#L60)). Past's own fixed portal 81111 is
reset in its vault and leads forward to Future room 81400
([prototype](../../../areas/obj/ceopast.obj#L173),
[destination](../../../areas/obj/ceopast.obj#L180),
[reset](../../../areas/zon/ceopast.zon#L51)). These are typed portal arrivals,
separate from ordinary WLD edges and from personal evidence that a player
discovered or used either portal. See
[Past Ceothia's reviewed closure](../zone-stories/PAST_CEOTHIA.md#complete-local-and-bounded-foreign-closure)
and the [Present phase's portal/reset evidence](CEOTHIA_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries).

The bounded text scan found no foreign QST recipe, reset group, or SHP record
referencing a Past Ceothia prototype. All six recipe bodies touching local
objects belong to Past's own QST source; the zone has no local shop file.
Structured scanning of external story sidecars found no numeric Past VNUM
reference. Past's own sidecar does directly name Present mobile 80803, Lord
Lenbrea, as a cross-phase contact; the contact remains distinct from native
item ownership and from a WLD entrance
([sidecar contact](../../../areas/story/ceopast.story.json#L132)).

A decimal VNUM search of C/C++ source found two direct references to local
object 81124, the blackened bracer
([prototype](../../../areas/obj/ceopast.obj#L382)): entries `{ 15, 81124 }` in
the standard and enhanceable Blighter arrays
([standard](../../../src/account/chaos_eq_data.h#L269),
[enhanceable](../../../src/account/chaos_eq_data.h#L513)). This is static
equipment-table data, not evidence that the kit is selected, issued, or used.
No literal local special assignment, computed epic-teacher binding, or object
T procedure was identified in the comprehensive dossier. Broader procedural,
staff, saved-world, editor/tooling, persisted-player, and dynamic dependencies
remain outside this bounded closure.

## Three active phases with one title

Both active manifests list the three phase keys separately. Their zone files
share the normalized title “Ceothia,” while their source keys and physical
room ranges remain distinct:

| Phase | Active source and range | Manifest and title evidence |
| --- | --- | --- |
| Present | `ceothia`, zone 808, rooms 80800–81094 | [`areas/AREA`](../../../areas/AREA#L288), [`areas/qst/AREA`](../../../areas/qst/AREA#L257), [zone title](../../../areas/zon/ceothia.zon#L2) |
| Past | `ceopast`, zone 811, rooms 81100–81392 | [`areas/AREA`](../../../areas/AREA#L289), [`areas/qst/AREA`](../../../areas/qst/AREA#L258), [zone title](../../../areas/zon/ceopast.zon#L2) |
| Future | `ceofutur`, zone 814, rooms 81400–81677 | [`areas/AREA`](../../../areas/AREA#L290), [`areas/qst/AREA`](../../../areas/qst/AREA#L259), [zone title](../../../areas/zon/ceofutur.zon#L2) |

The tracked path history keeps `ceopast` as its own active source. Commit
`677a9f877` changes spelling in room descriptions, not exit destinations.
Commit `71d731c5b` removes other entries from the active AREA roster while
leaving the `ceopast` WLD and ZON content unchanged and its manifest entry in
place. These edits do not establish that Past replaced Present or Future; the
shared title is not replacement evidence. The normalized title register and
[Ceothia supplement](CEOTHIA_AUDIT_REVIEW.md#three-active-phases-with-one-normalized-title)
record the separate phase identities.

## Audit boundary

These are static source, manifest, and Git-history findings. No ordinary WLD
edge or source-only WLD match proves that a phase is unreachable through code,
portals, spells, staff tools, or another dynamic controller. Portal declarations
also do not prove safe travel, player access, or durable outcomes. No native
zone, quest, story-sidecar, or runtime change is included.
