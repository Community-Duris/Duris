# Rogue Plains: world and dependency audit

This supplement reconciles the active source for area:roguerai at snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. The [source-comprehensive
dossier](../zone-stories/ROGUE_PLAINS.md) remains the narrative and quest
reference; this audit adds the full-world graph, typed ownership, tracked
history, inactive-map sweep, and explicit builder decisions. It records static
source behavior, not a live traversal or gameplay qualification.

## Source inventory and narrative boundary

The full-world roster lists roguerai at *758-759 on [areas/AREA line
273](../../../areas/AREA#L273). The quest roster lists it on [areas/qst/AREA
line 244](../../../areas/qst/AREA#L244). The active source set contains 158
rooms (#75801-#75959 with gaps), 58 mobile prototypes, 57 object prototypes,
223 reset commands in 127 families, nine native item exchanges, three
addressed dialogue blocks, and the story sidecar. There is no local SHP file
or shop record. The normalized inventory index is
[docs/reference/zone-story-audits/roguerai.md](../../reference/zone-story-audits/roguerai.md).

The existing [Rogue Plains dossier](../zone-stories/ROGUE_PLAINS.md) reads all
158 rooms, all 58 mobiles, all 57 objects, all nine exchanges, all dialogue,
and the complete local reset source. It groups the nine contracts into seven
outcomes and documents the four flesh kinds, alternative giant potions,
foreign Balance contract, mount ownership, item and spell mismatches, Master
set implementation, portals, and qualification limits. This audit does not
replace or duplicate that story map.

## Static connectivity and access

The [WLD](../../../areas/wld/roguerai.wld#L1) has 382 source exit records:
379 local, two resolved cross-zone exits, and one positive unresolved target.
The active loader resolves 381/382. Both resolved exits start at #75801:
east to Surface #541822 and south to Surface #542221. Surface has the direct
reverse west and north exits, respectively. They form two reciprocal active
approaches.

Room #75895 also has a north exit to #170047. That target exists only in
inactive [Duris3.wld](../../../areas/wld/Duris3.wld#L1470988), where it is
titled “Grassy Plains”; it is absent from the active WLD inventory, so the
loader prunes the exit. Keep this unresolved boundary under existing Duris3
candidate row 36 until the map owner confirms its destination or removal.

The complete inactive-WLD sweep parsed all 95 non-active files (527,791 rooms
and 2,080,773 exits). It found no room-number overlap with Rogue Plains and
one non-overlapping source-only approach: inactive
[surf.wld](../../../areas/wld/surf.wld#L27247) room #201299, “The Amber Sands
of the Desert Island,” exits north to #75801. This is not loaded ingress.
Preserve it with the existing Surf candidate row 97. No other active or
inactive WLD file enters a Rogue Plains room.

The local directed WLD graph has three weak components of 136, 21, and one
room. From Surface arrival #75801, ordinary authored exits reach 123/158
rooms, and all 123 have a return walk to #75801. The local ZON applies twelve
100-percent D resets to six door pairs. With those initial closed or
closed/secret states applied, strict WLD reach and return both fall to 96/158.
The pairs are #75836/#75897, #75849/#75888, #75865/#75866, #75868/#75869,
#75871/#75872, and #75905/#75910. The three secret pairs have reset state 5;
the other three pairs have reset state 1.

Six local type-25 objects provide a separate portal overlay to #75878, #75879,
#75880, #75820, #75850, and #75857. These targets are not ordinary WLD edges,
so the graph counts do not imply portal access, a successful arrival, or a
safe return. No external type-25 declaration targets a Rogue Plains room.

Two source details need direct builder review. Windtalker is reset in room
#75951, but no local, active external, or inactive-source WLD exit enters that
room; its exits point north to #75930 and east to #75952. The room is in the
same weak component, but it is not reachable from #75801 by directed WLD
travel. The six local portal targets above do not include it. The earlier
dossier's “connected staging rooms” wording is corrected here: an M reset
proves a load location, not a player route. The room may have an unmodeled
engine route, but none was found in the inspected static sources. The current
[story-sidecar contact](../../../areas/story/roguerai.story.json#L147) also
calls it a connected cloud-side staging room; treat that route description as
unverified until an owner supplies the missing path or confirms the intended
encounter location.

Rooms #75952, #75953, #75954, #75955, #75956, #75957, and #75959 are all
titled “Unnamed” and have no outgoing WLD exits. #75955 also has no incoming
WLD exit and no local ZON, QST, story, or compiled numeric reference found by
the exact-reference scans. Preserve these records until the area owner says
which are intentional terminals, placeholders, or content to repair. The WLD
declares no fall chances or current fields. Flight and levitation checks in
the Cloudscape remain character/mount movement rules, not additional static
edges.

## Typed references and ownership

The scan distinguished room, mobile, and object namespaces and checked every
active registered ZON, QST, SHP, and story sidecar plus all source prototype
files and direct C/C++ numeric references.

| Direction | Exact source evidence | Owner and interpretation |
| --- | --- | --- |
| Rogue reset imports | Local ZON places Jade object #76711 inside obelisk #75816; it loads Heaven object #358 and Winterhaven object #55286 onto Mabus. | Keep the external object definitions with Jade, Heavens, and Winterhaven. These are placed loot, not local object definitions. |
| External reset consumers | Active vehicles.zon has 20-percent G rows for Rogue objects #75857 and #75856 at [lines 828 and 841](../../../areas/zon/vehicles.zon#L828). | Preserve the Master sigil and maelstrom earring source identity; reset chance does not guarantee current stock. |
| External QST consumers | [Myrabolus](../../../areas/qst/mira.qst#L146) accepts objects #75825, #75826, and #75834; [Winterhaven](../../../areas/qst/wh.qst#L1178) accepts #75856. | These are foreign-owned exchanges. Sijona's referral does not create duplicate Rogue Plains credit. |
| External story-sidecar references | Jade has an optional carried-key step for #75829; Myrabolus references the three tokens; Winterhaven references #75856. The other 231 story sidecars parsed, with only those three files matching Rogue object VNUMs. | The Jade hint uses the Rogue obsidian key for the Rogue obelisk; it does not make the key a Jade city-door requirement. |
| Outside resets, shops, portals, prototypes | No external M/F/R reset references a Rogue mobile; F follow and R mount roles follow the last M/F mobile load ([zone loader F role](../../../src/world/db.c#L7747), [R role](../../../src/world/db.c#L7799)). No external room reset, SHP product, same-type WLD/MOB/OBJ definition, or fixed/random type-25 target enters Rogue Plains. | The scan included all 442 ZON files; no additional room/mobile/object importer or shop product was found. |
| Direct C/C++ references | The only exact direct object hook is #75857 assigned to master_set in [specs.assign.c line 1345](../../../src/specs/specs.assign.c#L1345). The legacy set list is in [specs.set.c line 47](../../../src/specs/specs.set.c#L47); the separate adapter table is in [specs.set_equipment.c line 419](../../../src/specs/specs.set_equipment.c#L419). | Keep object #75857 under the current special assignment unless builders resolve the different membership/counting models. |

The local reset file also references the external Jade relic, Heaven rune, and
Winterhaven memory by typed object roles. No local SHP or external shop product
references a Rogue object. All 231 other story sidecars parsed successfully;
the exact sidecar links above are structured object references, not prose
number matches. The source-comprehensive dossier explains the item ownership,
QST inputs, reset limits, and the direct legacy versus adapter set behavior.

## Title, duplicates, and tracked history

After color removal, whitespace normalization, and case folding, “Rogue
Plains” occurs once among all 442 current ZON titles and once among the 351
active area titles. “The Bloody Plains” and “The Plains of Life” are distinct
active titles, not an exact title duplicate or evidence of a replacement.
No same-type active WLD, MOB, or OBJ record overlaps a Rogue Plains VNUM.
Repeated prose, identical-looking quest objects, and same-name portals inside
this area are described as intentional-or-unresolved local identity questions
in the source dossier; do not merge their VNUMs from name similarity.

Followed history for the WLD, MOB, OBJ, and ZON stays on the roguerai paths
through the 2009 area-tree move and later maintenance through 2013 for WLD,
MOB, and ZON and through 2016 for OBJ. No alternate-stem predecessor or
replacement was found. The file areas/obj/rogue.pl is a Perl object-format
utility, not a Rogue Plains object area.

## Builder questions and recommended resolutions

1. **Should the two active Surface entrances remain separate?** Preserve the
   reciprocal #541822 and #542221 routes; qualify both deployed arrivals and
   returns before describing them as safe or guaranteed.
2. **Does the inactive Surf map belong in the current world?** Preserve
   surf.wld #201299 north to #75801 on existing Surf candidate row 97 as
   source-only evidence. Do not activate or repoint it without the map owner.
3. **What is the intended destination of #75895 north to #170047?** Keep the
   unresolved Rogue-to-Duris3 record under candidate row 36 until the owner
   selects an active destination or explicitly retires it.
4. **How should players reach Windtalker at #75951?** The ordinary reset is
   present, but there is no incoming WLD edge. Confirm any intended flight or
   engine route; otherwise treat encounter access as unresolved.
5. **What are the seven “Unnamed” terminal rooms for?** Name, connect, retain,
   or remove them only after an owner decision. #75955 is fully isolated;
   all seven currently lack outgoing exits.
6. **Are the six closed door pairs intentional in their reset state?** Confirm
   the three closed/secret routes (#75836/#75897, #75849/#75888,
   #75905/#75910) and the three closed camp routes (#75865/#75866,
   #75868/#75869, #75871/#75872). Keep the 96-room strict baseline until
   doors are deliberately redesigned.
7. **Should the pit's Styx portal be one-way?** Confirm the portal from #75803
   to #75880, its return route, and the safety expectations before adding a
   travel claim or journal credit.
8. **Are the two yurt interiors meant to be entered and exited through the
   current object routes?** Preserve the #75804/#75807 portals and their
   interior returns unless the area owner specifies another route.
9. **How should the three same-named flaming portals be distinguished?**
   Confirm the intended #75820, #75850, and #75857 destinations; use
   destination-specific aliases or guidance if players must tell them apart.
10. **What Cloudscape access and return rules should be taught?** Retain the
    current flight/levitation checks, but qualify the actor or mount, actual
    arrival, horizontal departure, and safe return before claiming access.
11. **Is the obelisk key optional, and should both locked containers remain
    pickable?** Keep #75829 as optional access to the obelisk; clarify the
    distinct #75830 altar key and do not turn either key into an exchange
    prerequisite without owner approval.
12. **Should the mediator's reward be a hammer or a sword?** The current
    result is hammer #75854 while dialogue calls it a sword and prose says
    blade. Confirm intent, then align the text or reward in a separate change.
13. **Are the two potion kinds alternatives for each giant?** Preserve the
    current OR recipes and distinct cloud/storm promises unless the owner
    changes them; one promise of each kind is still required by the mediator.
14. **How should the four distinct buffalo-flesh items be presented?** Keep
    their exact object identities; approve distinct aliases or provenance
    guidance so identical names do not suggest interchangeable proof.
15. **Is raw flesh intentionally a cure-light potion?** Resolve the
    ITEM_POTION values against the food prose before changing item type,
    spell, or quest inputs.
16. **What is the intended repeat loop for the departing orc?** Clarify
    recipient retirement, reset-generation stock, and whether the trapper
    mention is only flavor or a separate owner-defined continuation. Do not
    promise a renewed daily source without qualification.
17. **Does the sage's boot exchange intentionally stop at a partial soul?**
    Keep the current partial result distinct from the promised climb; add an
    endpoint only if the owner defines and accepts that event.
18. **Should the ancient reward remain a staff-named wand?** Preserve wand
    targeting until the object-only create-dracolich spell and its real corpse,
    control, and effect requirements are qualified. Do not award a raising
    event for receiving or activating the device.
19. **Is Sijona's “defeat them all” referral meant to lead to the current
    three-token Balance contract?** Keep the contract owned by Myrabolus and
    clarify whether the prose promises more than the actual accepted
    exchange. Confirm the Jade relic/key and the Heavens/Winterhaven loot
    placements remain intentional cross-area references.
20. **Which Master set membership model is authoritative?** The sigil is
    assigned to legacy master_set, whose list and thresholds differ from the
    adapter table; confirm the intended members and whether the vehicles
    reset remains a secondary source before adding any set-completion
    achievement.

## Disposition

Keep active roguerai DO NOT MOVE and add no Rogue Plains attic-candidate row or
candidate-specific migration plan. Preserve the unresolved Duris3 exit under
existing candidate row 36 and the inactive Surf approach under existing
candidate row 97. This checkpoint makes no WLD, MOB, OBJ, ZON, QST, story
sidecar, or C/C++ gameplay change. Static traversal, reset population, portal
use, source stock, spell effects, and safe return remain live qualification
questions.
