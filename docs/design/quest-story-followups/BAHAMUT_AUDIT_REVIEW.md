# Bahamut's Palace repository-wide audit supplement

This checkpoint reconciles the [source-comprehensive Bahamut's Palace dossier](../zone-stories/BAHAMUT_PALACE.md) with active-area graph, inactive-WLD, typed dependency, title, and bounded history passes at source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. It records source dependencies and builder decisions; it does not change native gameplay or move any zone.

## Active connectivity and map admission

The active `bahamut` WLD source has 236 rooms and 655 authored exits, with no parse errors. Loader qualification resolves all 655: 654 stay local, and one leaves for Plane of Air room #24460. The active boundary census found three inbound records and one outbound record. Plane of Air #24460 enters #25700 east, paired with the local west exit back; Realm staging room #14208 enters #25918 from above, and White Hills room #55632 enters #25922 from below, with no reverse WLD record for either staging entrance. Exact records remain in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

The local authored graph is one weak component of 236 rooms, with no room fall-chance or current fields. A plain directed traversal from #25700 reaches 223 rooms and has authored return paths from all 236 when it ignores locks, secrecy, flight, survival, and live state. The source-comprehensive dossier records the staged key circuit and conditional reach tiers; the plain WLD traversal is not a public-route or safety proof. Thirteen staging rooms #25923–#25935 remain outside the public entrance graph in that source analysis.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse errors. It found no room-VNUM overlap with Bahamut and no inactive-WLD exit into its active rooms. No source-only approach is added; the curated inactive-WLD approach total stays 1,532 and the Surf subset stays 142.

## Typed cross-area and compiled-system dependencies

| Dependency | Source-backed result and boundary |
| --- | --- |
| Active WLD routes | Three active entrances: Plane of Air #24460 east to #25700, Realm #14208 up to #25918, and White Hills #55632 down to #25922. Only the Plane of Air route has a reciprocal local exit ([local WLD](../../../areas/wld/bahamut.wld#L16), [Plane of Air](../../../areas/wld/plane_air_one.wld#L2370), [Realm](../../../areas/wld/realm.wld#L4657), [White Hills](../../../areas/wld/wh.wld#L13016)). |
| Outside reset use of local objects | Celestia resets local glowing potion #25759 in room #45723; Negplane resets #25759 in room #26865; Ravenloft places local Sunblade #25745 inside container #58430 at room #58564 ([Celestia](../../../areas/zon/celestia.zon#L164), [Negplane](../../../areas/zon/negplane.zon#L199), [Ravenloft](../../../areas/zon/ravenloft.zon#L536)). All three are active ZON references. |
| CHAOS starter profiles | Current standard and enhanceable profiles contain Bahamut objects #25705 (Summoner blindfold), #25712 (Shaman amulet), #25719 (Sorcerer helm), and #25757 (Paladin/Summoner quiver). Exact table entries are linked below; `nanny.c` loads the selected profile item through `read_object` and places it in the new-character kit. This is a direct global item-supply dependency; runtime issuance was not exercised ([Paladin](../../../src/account/chaos_eq_data.h#L101), [Shaman](../../../src/account/chaos_eq_data.h#L140), [Sorcerer](../../../src/account/chaos_eq_data.h#L149), [Summoner](../../../src/account/chaos_eq_data.h#L310), [enhanceable profiles](../../../src/account/chaos_eq_data.h#L353), [kit loader](../../../src/account/nanny.c#L620), [profile selection](../../../src/account/nanny.c#L709)). |
| Random-zone potion | Two compiled random-zone branches use Bahamut glowing potion #25759 as level-55 potion stock ([generator branch](../../../src/world/random.zone.c#L469), [potion branch](../../../src/world/random.zone.c#L959)). Actual generator availability and publication were not qualified here. |
| Bound prototypes | Current source assignments bind mobile #25700 to the Bahamut handler and local objects #25719, #25723, #25745, and #25710 to artifact-stone, DragonLord-plate, Sunblade, and BloodFeast handlers ([Bahamut mobile](../../../src/specs/specs.assign.c#L477), [object bindings](../../../src/specs/specs.assign.c#L1540)). The complete enabled-handler review is in the story dossier. |
| Dormant matches | `highdrop_mobs` lists mobile #25700, but its only comparison loop is commented out ([`randomeq.c`](../../../src/item/randomeq.c#L649)). `skill_beacon` has a location-to-skill table entry for room #25922, but repository search found no binding or other call site for this function; treat the entry as unbound source configuration, not a current active outcome ([table](../../../src/specs/specs.character_progression.c#L42)). |
| Bounded negative checks | No outside exact QST item command, SHP product, typed sidecar item VNUM, duplicate external mobile/object prototype, or type-25 portal declaration targeting a local Bahamut room was found. Bahamut has no local SHP file. The three reset imports above are the only external ZON role references to local room/mobile/object prototypes in the bounded scan. |

These dependencies sit beside the story's own two exact boundaries: Tamarand's local Q24 accepts seal #25760 and gives key #25724, while Bahamut's handler separately creates timed heart #55081 for the White Hills and Alatorin recipes. The heart is not a local object prototype; it is a distinct code-created item and outside receipt. Keep each source, accepted transaction, access state, and later return separate as detailed in the dossier.

## Title, history, and disposition

After color-token removal and title normalization, “Bahamut's Palace” has one exact owner among 351 active non-empty zone titles: `bahamut`. The title scan found no other active lexical candidate containing “Bahamut.”

Followed `bahamut.wld`, `bahamut.zon`, `bahamut.mob`, `bahamut.obj`, and `bahamut.qst` histories remain on this stem after the 2009 `areas/` relocation. The story sidecar was added in commit `268c4eb78` on 2026-10-06. No alternate-stem rename, distinct predecessor, or explicit replacement claim appears in these bounded paths. This does not prove the absence of an untracked predecessor. Preserve active `bahamut`; it remains **DO NOT MOVE**.

## Builder questions and recommended resolutions

1. Confirm the three cross-area entrances and their asymmetry are intended. Preserve the reciprocal Plane of Air return and both one-way staging records; do not infer public access or a safe return from room prose.
2. Confirm the Celestia and Negplane potion resets and Ravenloft's nested Sunblade are intentional external stock dependencies. Keep the prototypes and custody path stable or update each exact source reference as a separately reviewed dependency change.
3. Confirm the four Chaos kit objects are intended in their class profiles. Any item rebalance or source move must preserve the selected standard/enhanceable kit behavior or deliberately update those profile entries with focused qualification.
4. Confirm glowing potion #25759 remains suitable for both local reset stock and the random-zone level-55 branch. Keep runtime stock issuance separate from the authored reset declarations.
5. Treat the dormant high-drop and unbound beacon references as historical/source configuration unless a current binding is found; do not promise their outcomes from these declarations.
6. Retain the dossier's twenty existing questions on stock admission, secret-key access, transaction boundaries, timed-heart creation/expiry, competing outside returns, helper combat, and builder intent. No native repair is inferred from this audit.
