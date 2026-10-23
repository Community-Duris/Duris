# The Seacaves: source audit reconciliation

**Checkpoint:** 349, coverage row 261. **Disposition:** DO NOT MOVE. This
review changes documentation only; it does not change area membership,
source behavior, or migration state.

The [source-story dossier](../zone-stories/THE_SEACAVES.md) maps the coastal
approach, bandit hideout, Sea Goddess temple, mage's ritual rooms, crabman
burrow, and Wind Dancer wreck. This review adds reset-aware static
connectivity, typed dependency closure, title and history checks, and
source-backed builder questions. No build, test, live travel, reset, door,
combat, item, quest, shop, or database journey was run.

## Source inventory and story boundary

The un-commented [AREA entry](../../../areas/AREA#L10) lists seacaves under
zone header 22. It has 95 rooms (#2200–2295), 35 mobiles (#2200–2234), and 24
objects (#2200–2223). The zone resets 157 records: 50 M, 66 E, 3 G, 8 O,
4 P, and 26 D. All parsed WLD records are in the declared numeric range.

areas/qst/AREA lists seacaves, but there is no areas/qst/seacaves.qst; the
native Q/QA contract count is zero. No local .shp or structured story sidecar
exists. These absences mean that room and mobile prose, resets, and compiled
callbacks do not establish a local quest chain or shop.

## Active connectivity and reset-aware reach

The active WLD scan parsed all 351 active contributors and found 95 Sea Caves
rooms with 193 exits and no parse issues. There are 192 local exits and one
reciprocal Surface boundary:

| Source | Direction | Target | Pair |
| --- | --- | ---: | --- |
| [surface.wld #558717](../../../areas/wld/surface.wld#L1232633) | North (D0) | #2200 | Reciprocal with Sea Caves #2200 south |
| [seacaves.wld #2200](../../../areas/wld/seacaves.wld#L1) | South (D2) | #558717 | Reciprocal with Surface #558717 north |

All 193 authored exits resolve in the active world. The local weak graph has
one component of 95 rooms. Directed reach from #2200 reaches all 95 rooms and
all 95 can reach back to #2200 when every authored local edge is available.
Suppressing each of the 25 local directions whose WLD door field is nonzero
leaves 51 rooms reachable from #2200 and 51 able to reach it. This is a
static comparison; it does not claim those directions are closed at runtime.

There are 26 D reset commands, each at 100%. They match all 25 nonzero WLD
door directions by room and direction. Their reset states are 20 state-1
closed, four state-5 closed/secret, one state-8 “open/blocked,” and one
state-0 “open.” The state-0 command is on #2264 west, whose WLD direction has
no nonzero door field. The state-8 east direction from temple room #2262 is
not an ordinary closed door. A reset state, key, special, or runtime
transition could change access; none was tested.

The WLD contains no fall-chance or current records. Its cliff text warns that
the narrow ledge would be easy to fall from, but that prose is not a
mechanically authored fall marker.

## Inactive WLD identity and source-only approach

The scan of all 95 WLD files outside the active manifest parsed 527,791 rooms
and 2,080,773 exits without parse issues. It found no source-room VNUM overlap
with Sea Caves and one approach:

| Source-only record | Direction | Active destination | Identity |
| --- | --- | ---: | --- |
| [surf.wld #212716](../../../areas/wld/surf.wld#L266832) | West (D3) | #2200 | “A Rocky Path Through the Stragathor Mountains” |

This exit is source-only, not active ingress. It was not in Surf's existing
reviewed set: candidate row 97 contains no #212716 reference. Preserve it on
the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
Surf's reviewed source-only subset rises from 263 to 264; the curated global
total rises from 4,499 to 4,500. Surf's 145 active references are unchanged.

## Typed dependencies and compiled bindings

The local ZON imports these externally owned prototypes:

| Typed reset role | Seacaves ZON location | Owner and prototype |
| --- | --- | --- |
| Object #386 placed in pantry room #2246 | [line 115](../../../areas/zon/seacaves.zon#L115) | Active heavens, [object #386](../../../areas/obj/heavens.obj#L4036) |
| Objects #245, #209, #227, #214, and #283 worn by repeated bandit resets | [lines 142–182](../../../areas/zon/seacaves.zon#L142) | Active heavens; [#209](../../../areas/obj/heavens.obj#L2426), [#227](../../../areas/obj/heavens.obj#L2570), [#214](../../../areas/obj/heavens.obj#L2466), [#245](../../../areas/obj/heavens.obj#L2714), [#283](../../../areas/obj/heavens.obj#L3036) |
| Objects #239 and #283 worn by bandit resets | [lines 187–203](../../../areas/zon/seacaves.zon#L187) | Active heavens; [#239](../../../areas/obj/heavens.obj#L2666), [#283](../../../areas/obj/heavens.obj#L3036) |
| Objects #245, #246, #228, and #215 worn by the scout/stealthy bandit | [lines 205–218](../../../areas/zon/seacaves.zon#L205) | Active heavens; [#245](../../../areas/obj/heavens.obj#L2714), [#246](../../../areas/obj/heavens.obj#L2722), [#228](../../../areas/obj/heavens.obj#L2578), [#215](../../../areas/obj/heavens.obj#L2474) |
| Objects #210 and #246 given to the bandit leader | [lines 212–213](../../../areas/zon/seacaves.zon#L212) | Active heavens; [#210](../../../areas/obj/heavens.obj#L2434), [#246](../../../areas/obj/heavens.obj#L2722) |
| Object #288 given to the priest | [line 224](../../../areas/zon/seacaves.zon#L224) | Active heavens, [object #288](../../../areas/obj/heavens.obj#L3076) |
| Mobile #55265 placed in the priest's quarters #2264 | [line 225](../../../areas/zon/seacaves.zon#L225) | Active wh, [Adryv, Sahaugin priest of Sekolah](../../../areas/mob/wh.mob#L3748) |

The active owners are listed in [AREA](../../../areas/AREA#L2) for heavens and
[AREA](../../../areas/AREA#L234) for wh.

No active outside QST, SHP, story-sidecar, or ZON reset-role reference targets
a local Sea Caves prototype. Exact scans also found no external same-type
MOB/OBJ definition of a local prototype and no external type-25 portal
targeting one of its rooms. wh.story.json has the digits 2283 inside a UUID;
that string is not a typed room or item reference.

Two local objects have explicit compiled callbacks. Object #2203's
unspec_altar binding accepts pray altar from a living player character. If
they have a specialization and ten epic points, the payment callback clears
that specialization, updates skills, and forgets spells; insufficient points
or a failed transaction have separate messages. Its displayed “Water
Goddess” name differs from the area's “Sea Goddess.” Object #2204's
cutting_dagger binding deals four generic damage to a living player character
who GETs it from a room, then permits the command to continue. The [altar
assignment](../../../src/specs/specs.assign.c#L1490) and [dagger
assignment](../../../src/specs/specs.assign.c#L1493) are explicit. The altar
callback is at [specs.character_progression.c](../../../src/specs/specs.character_progression.c#L170),
payment callback at [specializations.c](../../../src/classes/specializations.c#L735),
and dagger behavior at [specs.seacaves.c](../../../src/specs/specs.seacaves.c#L18).

Object #2208 is a third, generic data-bound switch. Its ITEM_SWITCH values
specify PUSH, temple room #2262, the east exit, and the “switch item moves”
message mode ([object values](../../../areas/obj/seacaves.obj#L120)). The
ZON [resets it into room #2262](../../../areas/zon/seacaves.zon#L116), where
that exit leads to priest's quarters #2264 and carries the open/blocked state.
The default binding in
[world/db.c](../../../src/world/db.c#L5281) assigns
[item_switch](../../../src/specs/specs.object.c#L302) to switch objects with
no explicit procedure; [ITEM_SWITCH](../../../src/core/defines.h#L144) and
[CMD_PUSH](../../../src/cmd/interp.h#L305) define the stored values. The
procedure clears the blocked flag on the configured direction and its
reverse when the exit is not secret, then sends a message that the mural
moves aside. It does not remove the switch object from the room. This source
behavior was not played.

## Title, duplicate identity, and followed history

After color-code removal, trimming, whitespace normalization, and
case-folding, “The Seacaves” occurs once among 442 current .zon titles and
once among 351 active contributors. Same-type scans found no active room,
mobile, or object VNUM collision. The all-inactive-WLD scan found no
Sea-Caves-numbered source room in another inactive map.

Followed WLD, MOB, OBJ, and ZON history remains on the seacaves stem through
the repository's area-tree move and later same-stem edits. No alternate-stem
predecessor or explicit whole-zone replacement was identified. sea
(Graendiae) and seakngdm (Sea Kingdom) have separate stems and distinct
titles; neither is a title duplicate or proven replacement. Keep those
identities separate.

## Builder questions and recommended resolutions

1. **Which road, coastline, and current Surface region is intended by the path from the High Road to the beach?** The active entrance is Surface #558717, while the inactive Surf map adds a Stragathor approach. **Recommendation:** retain those identities separately until the world-map owner confirms whether the inactive connection represents intended former topology.
2. **Are the abandoned wagon and bandit raids one continuing incident?** The wagon carries Jeronis the Verzanan cloth-maker's sign and bodies; the lookout describes raids on caravans to northern cities. **Recommendation:** connect the prose only if the owner confirms the victims and timeline.
3. **What happened to Jeronis and the wagon's other victims?** The goods are scattered and the bodies are several days old, but no character names or recovery outcome are given. **Recommendation:** decide whether this is a closed environmental detail or a story requiring a discoverable aftermath.
4. **Should the cliff route pose a fall hazard?** The narrow ledge says a traveler could easily fall, while the WLD has no fall field. **Recommendation:** preserve the warning as prose and qualify safety before adding any hazard.
5. **Which secret entrances should the bandits use?** Reset pairs #2221/#2231 and #2233/#2234 are closed/secret. **Recommendation:** verify discovery and return from both sides before documenting either as a dependable route.
6. **Is the shaft trapdoor meant to be passable from both the cliff and bandit cave?** #2253 and #2254 have paired closed resets. **Recommendation:** retain the paired direction records and verify the route and return before changing door state.
7. **Does the bandit lookout have an alarm or guard-response behavior?** Room #2255 describes constant watch over the road and coast, but no local quest or callback defines an alarm. **Recommendation:** treat the lookout as descriptive until an owner identifies a real response.
8. **Is the head guard's iron key meant for his barracks chest?** The guard wears key #2211 and chest #2212 in room #2242 uses it; the chest contains a shield. **Recommendation:** preserve the exact key/chest pairing and qualify who can obtain it.
9. **What is the bandit leader's objective beyond organizing the group?** The leader's room is sparse and no local contract gives the leader an exchange or next step. **Recommendation:** keep the encounter self-contained unless a specific authored outcome is approved.
10. **Should the altar name its deity as Sea Goddess or Water Goddess?** The WLD and objects say Sea Goddess; the compiled specialization-removal messages say Water Goddess. **Recommendation:** choose one canonical name or document a deliberate distinction before editing text.
11. **Which city does the temple's tidal-wave mural depict, and is the scene history, warning, or ritual imagery?** Neither the WLD nor mural names it. **Recommendation:** preserve the ambiguity unless a source owner supplies an explicit identity.
12. **Is Adryv, a priest of Sekolah, meant to share the Sea Goddess priest's quarters?** The ZON imports active wh MOB #55265 into #2264 beside the local priest. **Recommendation:** confirm the cross-deity relationship and source ownership; do not duplicate the mobile locally.
13. **Is the mural switch's message meant to match its physical behavior?** Object #2208 is configured as a PUSH switch for #2262 east. The generic procedure clears the blocked flag from that direction and its reverse and says the mural moves aside, but leaves the object in the room. The reset marks the east exit open/blocked, while #2264 west resets open. **Recommendation:** clarify whether the message alone represents the mural moving or whether the object should move; verify the reset, access from both sides, and return in a live route before describing the encounter as qualified.
14. **Is the altar's specialization-removal service intentionally signaled by the temple prose?** Its compiled hook charges ten epic points and clears a character's specialization and spells after payment. **Recommendation:** verify the eligible character state, confirmation, and intended deity-facing copy before advertising the service.
15. **Is the sacrificial dagger's four-point pickup damage intentional?** The effect is compiled on GET and the object description does not warn the player. **Recommendation:** retain the current behavior as a source fact and ask the owner whether its discovery and message are intended.
16. **What is Mezit’s relationship to the young mage, imp, and summoning tome?** The book is named, but the text does not identify its author or describe what was summoned. **Recommendation:** connect these details only with an owner-approved account.
17. **Does the ornate mirror in the summoning room have a purpose beyond gathering shadows?** No destination or mirror callback is assigned by this audit. **Recommendation:** treat the scene as atmospheric until a working behavior is established.
18. **What role should the crabman clan's chief, shaman, nursery, and scavenged trove play together?** Their source text establishes community and caretaking, not a quest or trade system. **Recommendation:** preserve those social details without inferring a service or reward loop.
19. **Who or what ransacked and burned the Wind Dancer?** Fire, claw marks, torn children's clothing, lacedons, and a spirit are present, but their causal relationship is unstated. **Recommendation:** keep the wreck's evidence separate until the owner selects a supported explanation.
20. **Why was the Wind Dancer traveling, and what does its captain's spirit need?** Its final route and the spirit's unfinished business are not specified. The spirit holds key #2214 for the steel chest in the hold. **Recommendation:** preserve the key/chest detail and defer a passenger rescue or captain quest until a concrete outcome and safe return are defined.

## Disposition

Keep active seacaves DO NOT MOVE. Preserve its reciprocal Surface boundary,
external Heavens/WH reset imports, and compiled altar and dagger bindings.
Keep the one inactive surf.wld approach on existing candidate row 97 as
source-only evidence, not active ingress. Do not infer access, a quest chain,
or safe return from these static records. See [coverage row 261](ZONE_COVERAGE_REGISTER.csv#L261),
[source-story dossier](../zone-stories/THE_SEACAVES.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-349-sea-caves-reset-aware-connectivity-and-inactive-map-identity),
[title/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-349-the-seacaves-title-vnum-identity-and-followed-history),
[retention](ATTIC_MIGRATION_PLAN.md#checkpoint-349-sea-caves-in-place-retention-and-candidate-approach),
and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
