# The Arcaneum of L'srillizzin: repository-wide source audit supplement

This supplement reconciles the [source-comprehensive Arcaneum dossier](../zone-stories/THE_ARCANEUM_OF_LSRILLIZZIN.md)
with active WLD boundaries, the all-source WLD sweep, typed cross-zone
references, shared-code uses, title and replacement history. Findings come
from static repository sources; they do not establish live access, played
receipts, or persisted player history.

## Active boundaries and local graph

The Arcaneum has 150 rooms and 410 authored exits. All 410 targets resolve:
409 remain in the zone and one leaves. Its WLD-only weak graph has components
of 113 and 37 rooms. The 37-room component spans the Plane of Dreams and
Nightmares, #402073–#402109; it is separate in static exit topology from the
main compound.

The sole outbound edge is #402000 north to active Underdark #813342, paired
with Underdark #813342 south to #402000
([Arcaneum exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1315),
[Underdark return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1731)). These are the
area's only active boundary records.

Two local type-25 portal objects provide declared travel between the WLD
components: object #402037 is reset in #402072 and targets dream-plane room
#402073; object #402038 is reset in #402099 and targets main-compound room
#402072 ([portal definitions](../../../areas/obj/library.obj#L343),
[first target](../../../areas/obj/library.obj#L349),
[second portal](../../../areas/obj/library.obj#L351),
[second target](../../../areas/obj/library.obj#L357),
[reset placements](../../../areas/zon/library.zon#L383)). The portals are
dynamic type-25 travel, not WLD exit records. Their declarations do not prove
successful use, actual arrival, or safe return.

## Unregistered WLD source sweep

The all-95-file scan parsed 527,791 rooms and 2,080,773 exits with zero parse
issues. No unregistered source room overlaps the Arcaneum's room VNUMs, and no
unregistered exit targets an Arcaneum room. It adds no source-only ingress;
the curated source-only inbound total remains 603.

## Typed dependencies and shared code

Exact external QST item-command and SHP product scans found no references to
Arcaneum objects. All 231 other story sidecars parsed without issues and had
no exact structured VNUM match. The bounded external ZON scan found no reset
role or room target into the Arcaneum, and no external mobile or object
prototype reuses a local VNUM. Among 870 external type-25 declarations, none
targets an Arcaneum room. These negative results are bounded to the exact
typed records scanned.

Two direct shared-code references are classified. CHAOS equipment tables
include local quill #402052 in two kit profiles
([standard](../../../src/account/chaos_eq_data.h#L161),
[enhanced](../../../src/account/chaos_eq_data.h#L312)); this is an equipment
reference, not a fragment source or receipt. The epic-teacher table binds
mobile #402029, Shezeera, to Nature's Ruin
([binding](../../../src/classes/epic_skills.c#L219)). The dossier records the
epic purchase guard and keeps teacher service separate from the five local
story achievements.

## Title, tracked history, and disposition

The active manifest lists `library` as `*4020-4021`
([AREA entry](../../../areas/AREA#L409)); the ZON title is “The Arcaneum of
L'srillizzin” ([header](../../../areas/zon/library.zon#L2)). Its normalized
title is unique among 351 active areas. The active-WLD duplicate scan found
only global VNUM #51800 in the two Hell areas. The original Checkpoint 69
review had no Arcaneum-specific entry in the
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md); Checkpoint
260 now adds the bounded title and tracked-history crosswalk there.

Tracked WLD/MOB/OBJ/ZON/QST history reaches the 2014 creation commit
`d6b1e19c9`, “Added library zone in UD north of Faang.” Later tracked edits
include the zone renumbering `e871f5d33` and the one-way trapdoor change
`fceff446c`; the story sidecar was added at `117719e0a` on 2026-10-05. This
bounded path/register review finds no distinct predecessor or replacement
source. `library` remains an active `areas/AREA` contributor, so its disposition
stays **DO NOT MOVE**.

## Recommended reconciliation

- Preserve the 113/37 WLD component result as static topology. The two local
  type-25 portal declarations explain the authored travel between the main
  compound and dream-plane component without adding physical exits.
- Keep portal access and successful arrival separate from ordinary WLD
  reachability; the dossier's arrival and return qualifications remain open.
- Preserve the active manifest membership. This audit supports no attic move
  or native gameplay repair.

This is a bounded static source audit. Editor/tooling consumers, indirect code
paths, persisted player data, and live access, combat, service, and return
journeys remain outside its scope.

## Checkpoint 260: directed WLD reachability

The active WLD has 150 rooms, 410 resolving exits, 409 local exits, and one
outbound exit from #402000 north to Underdark #813342. That outside edge is
paired with Underdark #813342 south to #402000. The local weak components have
113 and 37 rooms. In the raw directed graph, #402000 reaches 110/150 rooms and
112/150 rooms have an authored local path back to it. Strongly connected
component sizes are 110, 28, 2, and ten singletons.

The 37 dream-plane rooms #402073–#402109 and the three collector staging rooms
#402147–#402149 are outside entrance forward reach. The dream-plane component
also has no authored local return to #402000. Rooms #402147 and #402148 can
return through #402147 down to #402002, although they cannot be reached from
the entrance. #402149 has no authored exit and no return path. These are raw
WLD facts; the source dossier separately records mobile wandering and the
type-25 portal destinations.

Objects #402037 and #402038 provide declared dynamic portal travel between
the main WLD component and the dream-plane component. They remain object
actions, not physical exit edges. The all-95 inactive-WLD scan found no room
overlap or source-only target into the Arcaneum. No source-only candidate or
curated inbound count changes. See the [connectivity report](WORLD_CONNECTIVITY.md#checkpoint-260-arcaneum-directed-reachability-and-dream-portals).

## Builder questions and recommended resolutions

These questions translate the existing source-comprehensive dossier into
decisions for the area owner. Recommendations preserve authored identities and
behavior until the responsible builder chooses a scoped change.

| # | Direct builder question | Source-backed recommended resolution |
| --- | --- | --- |
| 1 | Should Gafrizzen's QA10 continue to require C10000 for admission key #402003 when active accounting refuses coin-only offerings? | Keep the authored price and access service distinct; any payment work needs a guarded, settled coin receipt and separate qualification. |
| 2 | Does possessing the admission key represent only gate access, with paid admission excluded from the five eligible story achievements? | Preserve the sidecar's six raw identities and 6-to-5 achievement projection; do not credit a key or discovery as a story receipt. |
| 3 | Is Yralrife's exact six-essence bundle intended to use the six authored elemental identities supplied by the initial golem placements? | Keep each exact prototype and the current capped source mapping; do not require a personal kill or count extra unsupplied golems as producers. |
| 4 | Is Rikn's exact seven-proof bundle intended to remain independent of personally killing each undead source? | Preserve the seven exact accepted objects and supplied-copy eligibility; do not add combat history requirements. |
| 5 | Should Gringobbi accept exact journal #402029 without requiring a personal break-in or journal reset pickup from the locked desk? | Keep the accepted item identity separate from desk access; do not add theft, lock-solving, or personal provenance predicates. |
| 6 | Is shop pick #402031 intended to function as a held ITEM_PICK even though its wear flags are zero? | Keep the current prototype and shop data pending owner intent; do not add wear flags or advertise it as a usable pick without qualification. |
| 7 | Are Razeline's ten exact apprentice books intended to remain one bundle despite P-cap resets resolving a matching container by prototype? | Preserve all ten object identities and reset caps; qualify actual container placement instead of treating the preceding object group as a guaranteed parent. |
| 8 | Are Gazdiel's eight dream fragments distinct by exact VNUM even though their plain-text names and keywords overlap? | Retain exact #402041–#402048 identity checks; do not count eight copies of one fragment as the distinct set. |
| 9 | Should the native fragment color styling or sidecar's “1 of 8” labels imply a mandatory offering order? | Treat color and ordinal wording as current display text only; keep the bundle unordered unless the story owner specifies an order. |
| 10 | Is the demon's current scavenger route intended to supply only one fragment kind during an ordinary approach? | Preserve the authored one-room selection, caps, and episode limits; do not promise that one demon encounter yields all eight kinds. |
| 11 | Is fragment scavenging intended to use the converted runtime cost when file cost zero becomes approximately 98–102 after glow and randomization? | Preserve the shared conversion behavior and current prototypes; qualify actual settled pickup rather than treating file cost zero as a guarantee of exclusion. |
| 12 | Should the five remaining story endpoints be described as potential daily units while native stock, reset caps, and recipient availability vary? | Keep daily classification separate from renewable supply; do not promise a reliable daily opportunity before source and recipient availability are qualified. |
| 13 | Are the two type-25 objects intended to remain the only declared dynamic travel between the disconnected main and dream-plane WLD components? | Preserve object-owned portal travel and the static 113/37 map; do not invent WLD exits from the topology alone. |
| 14 | Should portal value2=-1 continue to mean unlimited uses rather than a level restriction? | Preserve the native value interpretation and avoid describing the portal as level-gated without a separate source rule. |
| 15 | Are the portico and treasury door masks, exact keys, and key break chances of 100 intended to remain as authored? | Keep the current secret/locked/pickproof states and keys; record successful unlock, settled key destruction, and actual arrival separately. |
| 16 | Is Razeline's cap-one mobile meant to have variable availability from #402148, where wandering can lead toward #402002 or into the no-exit room? | Preserve its spawn and wander source; do not guarantee the recipient's location or create a route repair without an owner decision. |
| 17 | Should collector staging room #402149 remain a no-exit trap even though nine exits from #402148 can lead there? | Preserve the authored room and keep player guidance out of staging; any route or trap change needs explicit builder intent. |
| 18 | Are the authored fall chances meant to remain environmental hazards without granting a separate story receipt or safe-travel promise? | Preserve the fall fields and keep hazard exposure, arrival, and return safety outside achievement claims. |
| 19 | Should Ne'kalsai's curse, Gazdiel's soul-mending narration, and Shezeera's teacher/epic service remain separate from the six local receipt identities? | Keep lore and class services outside the accepted story set; do not invent a cure contract or credit a service as a local receipt. |
| 20 | Should the reciprocal #402000–#813342 Underdark boundary remain the Arcaneum's sole active outside WLD connection, separate from its local portals? | Preserve the active boundary pair under its current owners and keep object travel distinct from cross-area WLD edges. |

## Checkpoint 260 disposition

Keep active `library` **DO NOT MOVE**. Preserve the reciprocal Underdark pair,
the two local type-25 portal declarations, the separate source identities, and
the current receipt and stock boundaries. The raw directed counts do not
qualify live doors, portal success, player access, recipient availability, or
safe return. The inactive-WLD sweep adds no source-only approach or room
overlap, so no new candidate is warranted. No native area or gameplay source
changed.
