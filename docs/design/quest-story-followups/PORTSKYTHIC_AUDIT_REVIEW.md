# Port Skythic repository-wide source audit

Reviewed October 10, 2026, Checkpoint 326 reconciles active `PortSkythic`
against its authored WLD, MOB, OBJ, and ZON sources, the active world list,
all inactive WLD files, typed consumers, followed file history, and its
existing attic candidate. This is a static source review; it does not inspect
a live server, editor state, player state, database, or persisted world data.
The [source dossier](../zone-stories/PORTSKYTHIC.md) summarizes the setting
and source boundary.

## Active source and authored story

The contributor is listed in the un-commented `areas/AREA` entry at line 392
and QST `AREA` at line 341. Its zone header is #1380, titled “The Ruins of
Port Skythic,” with reset mode 2 and range #138000–#138109. The active source
set contains 110 rooms, 20 mobile prototypes, and 15 object prototypes. There
is no local QST, SHP, or story sidecar and no native Q/QA contract. The current
`areas/RANDOM_AREA` file is empty ([area lists](../../../areas/AREA#L392),
[QST list](../../../areas/qst/AREA#L341),
[zone header](../../../areas/zon/PortSkythic.zon#L1)).

The room text presents a port after a siege: merchant stalls, gates, bridge,
streets, taverns, homes, and piers are burned, breached, bloodied, pillaged,
sunken, or rebuilt. It describes the defenders' effort and suggests that the
defenders may have damaged the bridge to slow attackers, but does not name the
attacking force or settle the cause or chronology. The Celestial District
contains desecrated Bahamut and Pelor altars. An oddly pristine street leads
to a tower of sorcery and a room of portals. A sunken ship and space-time rift
are named in the WLD but have no authored route back to the main map. The
source-based interpretation is documented in the [dossier](../zone-stories/PORTSKYTHIC.md).

Most mobile records have generic or unfinished names and descriptions despite
short-name keywords for dark nagas, slave traders, and enslaved two-headed
creatures. Only mobile #138008, the rumbling rock golem, receives four local
M resets (twice in each of the two gate rooms). The ZON also has twelve object
placement records and six D reset records for the tavern/storage and inn/lounge/
brothel doors. The exact reset records are in
[`PortSkythic.zon`](../../../areas/zon/PortSkythic.zon#L8). The WLD contains
seven exits with a nonzero numeric flag; six are the paired tavern/storage and
inn/lounge/brothel edges. A separate flag-4 edge is #138045 D2/south to
#138044, with no matching local D reset ([WLD exit](../../../areas/wld/PortSkythic.wld#L709)).

## Active graph and authored access

All 218 active WLD exit records resolve to rooms in this area; no active
cross-area exit, negative target, or positive unresolved target is present.
There is no active WLD boundary entering or leaving PortSkythic. The local
graph has three weak components, each strongly connected within its own
component:

| Component | Rooms | Static directed reach |
| --- | ---: | --- |
| City streets, gates, districts, tower, piers, and gatehouse | 103 | All 103 rooms reach one another through authored exits when every edge is treated as traversable. |
| Sunken pirate ship | 6 (#138101–#138106) | All six rooms reach one another; no authored WLD edge connects this component to the city. |
| Space-time rift | 1 (#138107) | No authored WLD exit. |

The main component comprises #138000–#138100 and #138108–#138109. The ship
and rift remain separate in the raw WLD graph; that does not establish whether
some live, non-WLD mechanism is intended. No fall-chance room or current field
was found. The six ZON D commands describe reset-controlled door states, so
the raw graph's reach counts do not prove what is open, discoverable, or
traversable after a live reset. No gameplay or safe-return conclusion follows.

### Inactive WLD approaches and overlapping room identities

The all-95 inactive-WLD scan found no source-room parse issues. It found 440
Duris3 records whose destination is one of #138000–#138109. The 224 records
from non-overlapping source rooms form distinct source-only approaches; the
other 216 start in same-numbered Duris3 rooms and are not counted as
approaches because their source identities overlap the active room namespace.
Every one of those 110 inactive records is titled “The Ocean”; none has the
active PortSkythic room title. Preserve the source-VNUM ambiguity and do not
promote the inactive records to active ingress.

| Inactive source record group | Count and destination coverage | Evidence |
| --- | --- | --- |
| `Duris3.wld` D2/south from #137700–#137809 | 110 records to #138000–#138109 | [First](../../../areas/wld/Duris3.wld#L791713), [last](../../../areas/wld/Duris3.wld#L794002) |
| `Duris3.wld` D0/north from #138300–#138409 | 110 records to #138000–#138109 | [First](../../../areas/wld/Duris3.wld#L804305), [last](../../../areas/wld/Duris3.wld#L806594) |
| Four boundary records: #137800 D3→#138099, #137999 D1→#138000, #138110 D3→#138109, and #138399 D1→#138100 | 4 records from 2 additional non-overlapping source rooms; #137800 and #138399 each also have a band edge above | [#137800 D3](../../../areas/wld/Duris3.wld#L793817), [#137999 D1](../../../areas/wld/Duris3.wld#L797988), [#138110 D3](../../../areas/wld/Duris3.wld#L800327), [#138399 D1](../../../areas/wld/Duris3.wld#L806388) |
| Same-numbered Duris3 source rooms #138000–#138109 | 216 target matches excluded from the curated count as source-identity overlaps | [Example room #138000](../../../areas/wld/Duris3.wld#L798001) |

These records are source-only in the unregistered `Duris3.wld`; none changes
the active inbound count or proves player access. Preserve the 224 distinct
records on the existing [Duris3 candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

## Typed dependencies and external consumers

The local reset file uses local room, mobile, and object VNUMs. It places
#138008 and objects #138000–#138010 and #138012; it does not reset local
mobile prototypes #138000–#138007 or #138009–#138019, or object prototypes
#138011, #138013, and #138014. The room titled “Within the Magic Shop” has no
local SHP source.

The bounded typed scans found no external exact QST item command, SHP product,
ZON reset-role reference, story-sidecar numeric reference, duplicate MOB/OBJ
prototype definition, or direct C/C++ numeric binding to PortSkythic's local
room/mobile/object IDs. The external object scan examined 872 type-25 portal
and 380 type-29 switch declarations; none targets a PortSkythic room. These
are source scans, not runtime/editor or persisted-state inventories.

## Title, followed history, and retention

After game color-code removal and whitespace normalization, “The Ruins of
Port Skythic” occurs once among all 442 current `.zon` sources and once among
the 351 active contributors. `git log --follow` shows same-stem additions on
2010-09-01, with no path rename in the followed history: WLD was last edited
2010-12-15, MOB and ZON in 2015, and OBJ in 2016. No local QST or SHP file has
followed history. `Duris3.wld`'s same-numbered “The Ocean” records establish
source overlap, not a titled Port Skythic predecessor or replacement. A
bounded case-insensitive source search found no other `PortSkythic` or “Port
Skythic” alias in `src/` or other `areas/` sources beyond the area-list
entries and owned files. No whole-zone replacement is established by the
reviewed histories and titles.

Keep active `PortSkythic` **DO NOT MOVE**. Preserve its current file ownership,
VNUMs, six door-reset records, four golem placements, and city/ship/rift graph.
Keep inactive `Duris3` **RETAIN IN PLACE** on its existing candidate row with
the 224 unique-source approaches and 216 identity-overlapping matches clearly
separated. The preexisting 26 active exits targeting Duris3 remain unchanged.
No route, reset, gameplay, or migration change is proposed.

## Checkpoint 326: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PS-01 | Who invaded Port Skythic, and should the attackers remain unnamed? | Keep the invasion as the source's framing and do not supply a faction or motive without owner evidence. |
| PS-02 | When did the siege happen, and how should the body-removal clues be dated? | Preserve the descriptions' relative wording; ask the owner before adding a calendar or event chronology. |
| PS-03 | Is the front-gate breach magical, and what destroyed it? | Retain “unnatural” and the text's uncertain magical-blast suggestion without choosing a cause. |
| PS-04 | Did the city's defenders break the bridge to slow the attackers? | Keep the text's tentative account until the owner confirms whether it is intended history. |
| PS-05 | What was Port Skythic's regional role as a port and trading city? | Preserve evidence of merchant stalls, piers, and city streets; leave geography and trade partners open. |
| PS-06 | Should the source explain where the residents went after the invasion? | Keep the recorded corpses and removed bodies as clues without inventing a survivor or evacuation story. |
| PS-07 | Are the dark naga and slave-trade mobile prototypes intended for this zone? | Keep all VNUMs; confirm each prototype's intended encounter and placement before adding resets. |
| PS-08 | Should the generic and unfinished mobile short names and descriptions be completed? | Ask the mobile owner to supply names and behavior; do not infer character identities from the room story. |
| PS-09 | Why does the local ZON reset only four rumbling rock golems when the roster includes naga and enslaved mobiles? | Preserve the four existing gate placements and ask whether the remaining prototypes are dynamic, unused, or missing authored placements. |
| PS-10 | Should the corpse, naga-scalp, and broken-statue object prototypes remain unreset? | Retain their definitions and request owner intent before changing their availability or placement. |
| PS-11 | Are the Raging Boar Tavern and storage-room doors intended to reset closed without keys? | Keep both paired D resets and confirm intended initial state and ordinary access. |
| PS-12 | Should the Dancing Sun Inn, hidden lounge, and brothel passages reset closed and secret? | Preserve the four authored D states and ask the owner to confirm discovery and access expectations. |
| PS-13 | What is the intended meaning of the flag-4 #138045 south exit to #138044, which has no matching D reset? | Keep the authored WLD record; ask the owner to identify the flag's intended behavior before editing it. |
| PS-14 | Is the six-room sunken pirate ship intended to connect to the Port Skythic piers? | Preserve its separate six-room component and request an access decision; add no route based only on the ship's name. |
| PS-15 | What access and return path should the isolated #138107 space-time rift have? | Keep its standalone room until the owner identifies an intended route or mechanic. |
| PS-16 | Is the oddly pristine district and tower of sorcery meant to contrast with the siege damage? | Preserve the contrast in the current text; do not infer why it remains intact. |
| PS-17 | Should the room titled “Within the Magic Shop” have an operating shop or stock? | Keep the room and current empty shop-source state while the owner decides whether it is set dressing or an omitted service. |
| PS-18 | Is the Tower of Sorcery's room of portals expected to create a travel route? | Preserve the rooms; require a separately defined route and return behavior before any portal or code change. |
| PS-19 | What does the desecration of the Bahamut and Pelor altars mean in the zone's history? | Keep the altar titles and placed statues as evidence; do not invent a religious conflict or quest. |
| PS-20 | How do the war-torn and sunken piers relate to the two recently rebuilt piers? | Preserve all pier descriptions and exits; ask the owner whether they reflect a timeline or merely different harbor conditions. |

