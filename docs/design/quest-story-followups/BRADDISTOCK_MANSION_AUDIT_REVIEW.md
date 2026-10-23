# Braddistock Mansion (`brad`): repository-wide boundary and dependency supplement

Status: bounded static supplement to the
[comprehensive Mansion source map](../zone-stories/BRADDISTOCK_MANSION_1350.md).
It reconciles the current Tharnadia boundary, source-only `Duris3.wld`
references, the Tower quest-owner binding, and the repeated title with older
zone13 `braddistock`. It does not qualify runtime availability or player
outcomes.

## Active and source-only WLD boundaries

The Mansion is area key `brad`, zone 1350, with 48 physical rooms 135001–135048.
The active WLD ledger records one reciprocal Tharnadia boundary:

| Active boundary | Evidence |
| --- | --- |
| Tharnadia sea wall 132650 north ↔ Mansion road 135001 south | [Tharnadia exit](../../../areas/wld/tharnadia.wld#L3799), [Mansion exit](../../../areas/wld/brad.wld#L18) |

A stateful parse of all 95 current unregistered WLD sources read 2,080,773
authored D0–D9 exits with zero parse issues. It found 192 `Duris3.wld` exit
records whose numeric targets cover all 48 current Mansion room VNUMs:

| Unregistered source rooms and direction | Targets in active `brad` | Evidence |
| --- | --- | --- |
| 134701–134748, south (`D2`) | 48 exits to 135001–135048 | [first](../../../areas/wld/Duris3.wld#L728737), [last](../../../areas/wld/Duris3.wld#L729724) |
| 135000–135047, east (`D1`) | 48 exits to 135001–135048 | [first](../../../areas/wld/Duris3.wld#L735012), [last](../../../areas/wld/Duris3.wld#L735999) |
| 135002–135049, west (`D3`) | 48 exits to 135001–135048 | [first](../../../areas/wld/Duris3.wld#L735062), [last](../../../areas/wld/Duris3.wld#L736049) |
| 135301–135348, north (`D0`) | 48 exits to 135001–135048 | [first](../../../areas/wld/Duris3.wld#L741329), [last](../../../areas/wld/Duris3.wld#L742316) |

`Duris3.wld` is not an active `areas/AREA` input. Its source also declares 48
rooms whose own VNUMs, 135001–135048, overlap this active Mansion range. These
are source-only map records and numeric overlaps, not active-world entrances or
proof that either source replaced the other. This review adds 192 individually
classified records to the curated count, from 385 to 577. The broader raw
`Duris3.wld` target and source-room overlap totals remain unchanged.

## Tower-owned contracts and other typed references

The Mansion sidecar's five story cards bind to native quest blocks physically
defined in `lortower.qst`. The givers are Tower mobiles, while the
comprehensive dossier maps canonical story ownership to zone1350. This keeps
physical placement distinct from immutable journal ownership:

| Story binding in `brad.story.json` | Tower giver | Native source |
| --- | ---: | --- |
| Break Azlion's binding circle | 134146 | [Mansion mapping](../../../areas/story/brad.story.json#L181), [Tower QST](../../../areas/qst/lortower.qst#L201) |
| Tell Jenifer Joseph's fate | 134150 | [Mansion mapping](../../../areas/story/brad.story.json#L218), [Tower QST](../../../areas/qst/lortower.qst#L218) |
| Assemble Darrin's Star Key | 134162 | [Mansion mapping](../../../areas/story/brad.story.json#L256), [Tower QST](../../../areas/qst/lortower.qst#L266) |
| Carry Isabia's ring to Danthas | 134167 | [Mansion mapping](../../../areas/story/brad.story.json#L336), [Tower QST](../../../areas/qst/lortower.qst#L292) |
| Bring Isabia the Cardinal's key | 134169 | [Mansion mapping](../../../areas/story/brad.story.json#L386), [Tower QST](../../../areas/qst/lortower.qst#L304) |

The comprehensive dossier also records that all 713 active portal prototypes
were checked with no fixed arrival into rooms 135001–135048, no relevant
foreign reset group loads a local Mansion actor or object, and no fresh local
reset or quest producer was identified for key 135050, maul 135071, or sword
135073. The only direct compiled local-prototype binding found is the Mansion's
own mobile 135014 assignment to `braddistock`
([assignment](../../../src/specs/specs.assign.c#L343)); the similarly named
`jet_black_maul` procedure remains assigned to older object 1372, not local
maul 135071. The full tracked SHP scan found no external shop reference to the
Mansion's prototype range, and the Mansion itself has no SHP file. The raw
numeric code match at `135000.0` is a trade cost, not a VNUM dependency.

## Repeated title and tracked WLD history

The current manifests list both keys separately, with identical normalized
zone titles and distinct VNUM ranges:

| Active source | Zone and room range | Manifest, title and dossier |
| --- | --- | --- |
| Older `braddistock` | Zone 13, rooms 1300–1399 | [`areas/AREA`](../../../areas/AREA#L3), [`areas/qst/AREA`](../../../areas/qst/AREA#L3), [zone title](../../../areas/zon/braddistock.zon#L2), [older dossier](../zone-stories/BRADDISTOCK_MANSION.md) |
| `brad` Mansion | Zone 1350, rooms 135001–135048 | [`areas/AREA`](../../../areas/AREA#L386), [`areas/qst/AREA`](../../../areas/qst/AREA#L337), [zone title](../../../areas/zon/brad.zon#L2), [Mansion dossier](../zone-stories/BRADDISTOCK_MANSION_1350.md) |

Their present Tharnadia routes differ. `brad` is the reciprocal pair at room
132650, while old zone13 room 1300 currently exits south to Tharnadian Ruin
room 5616
([older exit](../../../areas/wld/braddistock.wld#L18),
[active cross-area row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L21)).

The tracked 2010 and 2011 edits preserve that distinction. Commit `9f36e8f`
temporarily changed old zone13's south exit from Tharnadian Ruin 5616 to
Tharnadia 132650. Commit `362e766` separately set the zone1350 `brad` return
to 132650 and changed Tharnadia's north target from old room 1300 to Mansion
room 135001; its message describes restoring the original Mansion version
after the version used with Tharn Ruins. Commit `8e4a056` later changed the
old zone13 exit back to 5616 during Tharn Ruins re-entry work. These commits
edit the two paths independently and do not remove or rename either active
manifest key. The shared title is not replacement evidence.

## Audit boundary

These are static source and history findings. The 192 `Duris3.wld` records are
not active-world ingress, and their source-room VNUM overlap needs map-owner
classification before any future path decision. Current exits do not prove
safe traversal, ordinary access, return policy or a played Tharnadia journey.
Editor/tooling, persisted-player, and indirect or dynamic dependencies remain
outside this bounded closure. No native source or runtime change is included.
