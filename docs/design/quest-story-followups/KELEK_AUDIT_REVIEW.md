# The Stone Tomb of Kelek repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, exact title ownership, and bounded source history for the [current Kelek dossier](../zone-stories/STONE_TOMB_OF_KELEK.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed.

## Active graph and reciprocal boundaries

The active `kelek` WLD has 216 rooms and 430/430 resolving exits: 428 local and two cross-area. Its weak components have 215 and 1 rooms. Raw directed traversal from #87950 reaches and has directed returns from 215/216; #87951 is the isolated singleton. There are no blocked exits, falls, or currents. The two outbound boundaries are reciprocal: #87950 up ↔ Underdark #857219 down, and #88165 up ↔ Krethik #20115 down ([Kelek source exits](../../../areas/wld/kelek.wld#L10), [#88165](../../../areas/wld/kelek.wld#L3887); [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L219), [Krethik return](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L229), [Underdark pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L220), [Underdark return](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L711)). Preserve the isolated room and both complete boundary leaves. The dossiers' secret passage, key, door, and conditional Prime Shift routes are not ordinary WLD exits or automatic repairs to #87951.

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no inactive room-VNUM overlap with Kelek and no inactive WLD exit targeting #87950–#88165. No source-only approach is added; curated totals remain 2,611 overall and 153 for the reviewed Surf subset. This scoped WLD result does not clear non-WLD sources or prove any live path.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Imported epic node | Active Kelek reset #146 gives object #359 to mobile #87955 in room #88162 ([reset](../../../areas/zon/kelek.zon#L146)). The object is defined in `heavens.obj` ([prototype](../../../areas/obj/heavens.obj#L3725)) and has the active `epic_stone` callback ([binding](../../../src/specs/specs.assign.c#L1604)); the dossier maps its committed epic publication and carried-node conditions. This is an imported object and owned behavior, not a local object definition or ordinary zone discovery. |
| Deliverer special | Local object #87950 is `Deliverer` ([definition](../../../areas/obj/kelek.obj#L1)); compiled binding assigns `deliverer_hammer` ([binding](../../../src/specs/specs.assign.c#L2210), [callback](../../../src/specs/specs.keleks.c#L27)). Its melee damage condition does not establish a purification quest terminal. The same numeric value is also the WLD entrance room, in a separate prototype namespace. |
| Conditional compiled room arrival | `do_shift_prime` includes room #88122 in a Githyanki destination list ([list](../../../src/classes/innates.c#L2009)). This is a conditional/random compiled arrival under its own actor and plane conditions, not an authored active boundary or a guaranteed route/requirement. |
| Other external-looking identifiers | `alatorin.story.json` contains `88131` only within a string story ID ([string token](../../../areas/story/alatorin.story.json#L3828)); it is not a typed room/mobile/object reference. The typed scan finds no outside exact QST consumer, no external SHP product, no outside ZON reset role for a Kelek prototype, no duplicate external mobile/object definition, no external type-25 portal target, and no outside typed sidecar VNUM. Kelek has no local SHP source. |
| Current story ownership | The current schema2 sidecar owns only the unchanged Kelek smith-material story, five local contacts, and one achievement/potential daily. The earlier bishop and captive-paladin contracts and eight contacts now belong to the Church journal after the separately named ownership repair. This is a quest-owner correction that preserves old IDs and immutable receipt payloads; it is not evidence of Kelek zone replacement or Church discovery through Kelek. See the [current sidecar](../../../areas/story/kelek.story.json) and [ownership repair](../ZONE_STORY_CHURCH_OWNERSHIP_FIX.md). |

These are static code, reset, quest, and sidecar references. They do not establish current epic-node availability, a saved node payout, Deliverer's use, a successful Prime Shift, actual crypt access, or a played return.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “The Stone Tomb of Kelek” has one exact title owner among 351 active `.zon` contributors: `kelek` ([title source](../../../areas/zon/kelek.zon#L2)). Followed WLD/MOB/OBJ/ZON/QST history remains on the Kelek stem. The older history records maze/tomb and non-teleport-flag fixes in 2009, a switch/historical key pass in 2009–2010, same-stem content maintenance through 2016, and later same-stem text/equipment edits. The story sidecar was added on 2026-10-06 in commit `bc62e2569`, then corrected in commit `0c6f70007` for the Church ownership and accounting rule. No alternate-stem rename, explicit predecessor, or whole-zone replacement appears in these bounded paths. Keep active `kelek` **DO NOT MOVE**. Preserve the sidecar's Church owner correction and do not revive the old Kelek ownership for Church contracts.

## Builder recommendations

1. Keep #87951 isolated unless a builder selects a named layout change; do not connect it just to make the raw traversal count complete.
2. Preserve both reciprocal Underdark/Krethik boundaries and the existing secret/key/door state. Distinguish route hints, eligible actions, key use, arrival, and return.
3. Preserve the imported #359 node reset and `epic_stone` callback; separately record actual carrier/origin, admitted touch, committed publication, and later reset behavior.
4. Preserve the Deliverer #87950 special and distinguish its qualifying melee damage from any future quest/cure outcome.
5. Keep conditional Prime Shift to #88122 outside the ordinary map graph and outside mandatory quest access; retain the Church contract owner correction as a separately sourced journal decision.
6. No inactive WLD approach or replacement evidence was found. Keep the curated source-only and Surf totals unchanged and preserve their prior reviewed sources.
