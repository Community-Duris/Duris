# The Basin Wastes (`basin_wa`): boundary and dependency supplement

Status: bounded static supplement to the
[comprehensive source map](../zone-stories/THE_BASIN_WASTES.md). It reconciles
the active Surface boundary, current source-only WLD references, direct typed
references, current title identity, and a bounded tracked-path history. It does
not qualify runtime availability or player outcomes.

## Active and source-only WLD boundaries

The active key is `basin_wa`, zone 340, with 200 exact room records from 34000
through 34199. Its sole ordinary cross-zone connection is a reciprocal pair
with Surface: Basin room 34000 exits up to Surface room 612059, and Surface
612059 exits down to Basin room 34000. The active ledger records one declaration
in each direction, both with raw flags and key VNUM zero
([Basin declaration](../../../areas/wld/basin_wa.wld#L21),
[Surface return](../../../areas/wld/surface.wld#L2352636),
[outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L455),
[return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1603)). The active pair
is separate from the source-only map record below.

A stateful parse of all 95 current unregistered WLD sources read 2,080,773
authored D0–D9 exits with zero parse issues. One `surf.wld` exit targets a
Basin room: room 229308, “Sand Dunes of the Calimshan Desert,” exits down (`D5`)
to active room 34000
([source room](../../../areas/wld/surf.wld#L614796),
[exit](../../../areas/wld/surf.wld#L614816)). No unregistered source-room VNUM
overlaps Basin's exact active rooms. `surf.wld` is not a current `areas/AREA`
input, so this remains a source-only numeric reference, not active ingress or
proof that the old map was intended to connect here. It brings the curated
source-only follow-up count from 578 to 579; the broader raw `surf.wld` target
count remains 415.

## Bounded typed references

The comprehensive dossier maps all ten touching recipe bodies to Basin's own
QST source, and its global scan found no foreign reset source for a local
ingredient. The scan of all 713 active teleport prototypes found no incoming
portal; see the
[reviewed local and foreign closure](../zone-stories/THE_BASIN_WASTES.md#complete-local-and-bounded-foreign-closure).
A separate exact numeric scan of the other `.qst`, `.zon`, and `.shp` source
files found no Basin mobile/object prototype references, and a structured scan
of external story sidecars found no numeric Basin VNUM reference. Basin has no
local shop file. These bounded scans do not qualify indirect or dynamic paths.

The only exact decimal match for room/mobile/object VNUMs 34000–34199 in
`src/` is local wolf spider 34014's `block_dir` assignment
([assignment](../../../src/specs/specs.assign.c#L2185)). The dossier reviews its
EAST barrier and the authored dead end; this local procedure reference is not
an external dependency or evidence of a personal combat/access event.

## Current identity and tracked path history

Both active manifests list `basin_wa` separately at zone 340–341
([`areas/AREA`](../../../areas/AREA#L164),
[`areas/qst/AREA`](../../../areas/qst/AREA#L152)); its zone file title is “The
Basin Wastes” ([title](../../../areas/zon/basin_wa.zon#L2)). The coverage
register's normalized title pass records no repeated active title
([Basin row](ZONE_COVERAGE_REGISTER.csv#L27)).

The bounded tracked-path review found no key replacement signal. Commit
`71d731c5b` left the `basin_wa` manifest entry in place; its WLD change is file
mode metadata only, and its ZON content change switches the spelling in one
reset comment from “elixir” to “elixer”. The earlier areas-tree relocation is recorded in `642da5400`. Neither reviewed
change retargets a Basin exit or maps the active key to a replacement source.

## Audit boundary

These are static source, manifest, and Git-history findings. Active reciprocal
exits and the unregistered `surf.wld` record do not prove ordinary access,
safe traversal, or a played return journey. Fixed portal, spell, staff,
saved-world, editor/tooling, persisted-player, and other indirect or dynamic
references remain subject to their stated bounded scans. No native zone,
quest, story-sidecar, or runtime change is included.
