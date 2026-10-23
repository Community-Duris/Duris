# Otower: connectivity, quest dependencies, and retention review

This bounded source review accompanies the [Outcasts Tower dossier](../zone-stories/OTOWER.md). It reconciles the active WLD graph and door resets, both live boundaries, the inactive Surf approach, White Harbor's memory quest, foreign object resets, compiled VNUM references, title ownership, and followed source history. It does not establish installed runtime behavior or played access and return.

## Active graph, door resets, and source-only approach

The active zone has 51 rooms and 104 authored exit records. Every record resolves under the current active loader: 102 target another Otower room and two leave the zone. The raw local graph is one weak component. Directed reachability and returnability from #11300 each cover all 51 rooms when authored local exits are treated as edges. There are no negative or positive-unresolved targets, fall-chance fields, or currents.

The two live reciprocal WLD boundaries are:

| Otower exit | Active reverse exit | Meaning |
| --- | --- | --- |
| #11300 south to Surface #616249 | Surface #616249 north to #11300 | Mountain trail approach and return. Both exit records have flags 0 and key 0. [Otower edge](../../../areas/wld/otower.wld#L20), [Surface edge](../../../areas/wld/surface.wld#L2440601), [Otower register row 207](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L207), [Surface reverse row 1611](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1611). |
| #11348 north to Underworld3 #30172 | Underworld3 #30172 south to #11348 | Under-swamp connection. Both exit records have flags 0 and key 0. [Otower edge](../../../areas/wld/otower.wld#L1054), [Underworld3 edge](../../../areas/wld/underworld3.wld#L3546), [Otower register row 208](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L208), [Underworld3 reverse row 420](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L420). |

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 direction records without parse issues. It found no room overlap and one exit into Otower: inactive source-only surf.wld #225677 north to #11300. That source room is titled “A Grassy Clearing,” has a blank description, and also points to #225678, #225977, and #225676. It is not active ingress. Preserve it on the existing Surf candidate row 97; the row's 145 active references do not change. The reviewed Surf source-only subset rises from 251 to 252, and the curated source-only total rises from 4,248 to 4,249. See [the Surf record](../../../areas/wld/surf.wld#L538621) and [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

The local reachability result is a topology result, not a guarantee that a character can traverse every route at every reset. The 16 local ZON door commands set authored doors when the zone resets. The reset handler masks the state with 3: 0 is open and unlocked, 1 closed and unlocked, and 2 or 3 closed and locked ([reset behavior](../../../src/world/db.c#L7857)). The WLD flags use the separate exit flags in [defines.h](../../../src/core/defines.h#L442).

| Route | WLD and ZON state | Key and review finding |
| --- | --- | --- |
| #11303 north / #11304 south through the entrance gates | Both WLD exits name key #11300; ZON state 2 closes and locks both. | Brahvinos carries key #11300 in #11303. The paired records and reset placements agree on a keyed entrance. [WLD](../../../areas/wld/otower.wld#L75), [resets](../../../areas/zon/otower.zon#L16), [key carrier](../../../areas/zon/otower.zon#L76). |
| #11302 east / #11305 west through bushes | Both exits have WLD flags 5 and key 0; ZON state 5 closes and unlocks each. | The text calls the trail hidden, and comments call it secret, but neither WLD flag value contains EX_SECRET. Confirm whether this is meant to be a visible door, a searchable secret, or descriptive flavor. [WLD](../../../areas/wld/otower.wld#L43), [resets and comments](../../../areas/zon/otower.zon#L8). |
| #11310/#11311, #11312/#11314, and #11313/#11315 mountain branches | Reset states close these paired exits without a key. Several ZON comments call the passages secret; corresponding WLD raw flags are 1 or 5, with no EX_SECRET bit. | Text describes a steep trail, a stone-giant cave, a rockslide, and a hidden alpine grove. Confirm how discovery should work before changing flags or prose. [WLD mountain rooms](../../../areas/wld/otower.wld#L228), [resets and comments](../../../areas/zon/otower.zon#L24). |
| #11320 south / #11322 north to the warbeast room | The large steel door has paired WLD flags 1; ZON state 1 closes it and clears lock state. | No key is named. Room text identifies the door as protection against accidentally entering the room. [WLD](../../../areas/wld/otower.wld#L453), [resets](../../../areas/zon/otower.zon#L48). |
| #11337 east / #11338 west to the upper intersection | The #11337 exit names key #11322 and resets locked and closed. The reverse #11338 exit resets closed and unlocked. | Aracnalis carries key #11322 in #11331. The #11337 exit points east to #11338 but its exit description says the intersection is west; the room description says east. The reset state is also asymmetric across the paired records. [WLD](../../../areas/wld/otower.wld#L808), [resets](../../../areas/zon/otower.zon#L56), [carrier](../../../areas/zon/otower.zon#L124). |
| #11338 up / #11342 down to Ximixix | The upward exit names key #11321; its WLD flags are 7 and the ZON state 6 keeps it closed and locked. The downward exit resets closed and unlocked. | Arcanusinil carries the illusionary key #11321. The zone comment calls the up route secret, but WLD flags 7 do not include EX_SECRET. Confirm the desired illusion and reverse-door behavior. [WLD](../../../areas/wld/otower.wld#L829), [resets](../../../areas/zon/otower.zon#L64), [carrier](../../../areas/zon/otower.zon#L155). |

The WLD prose says the very steep trail is likely to cause falls and that travelers have drowned below the swamp, but the static fields do not set fall chance or currents. Keep that distinction explicit until a builder defines any mechanical hazard.

## External quest and typed dependencies

The area manifest lists otower in both areas/AREA and areas/qst/AREA, but there is no areas/qst/otower.qst, local giver, local contract, SHP file, or local story sidecar. The White Harbor story sidecar is a verified external story link. QST giver #55279 is an ambassador from the Outcasts Tower. Its QST requires item #55447, and the story entry says to recover that memory in Ximixix's throne room and deliver it to the ambassador for 1,000,000 copper, a gnomish shopkeepers token (#55033), and a strange scroll (#55362) ([QST](../../../areas/qst/wh.qst#L3734), [story entry](../../../areas/story/wh.story.json#L1079), [giver](../../../areas/mob/wh.mob#L3979)).

Otower's active ZON places three objects from outside its own OBJ file:

| VNUM | Source owner and description | Otower placement |
| --- | --- | --- |
| #359 | Heavens: rune-covered stone whose description links mana nodes across Duris. [Prototype](../../../areas/obj/heavens.obj#L3725). | O reset in #11342. [Reset](../../../areas/zon/otower.zon#L73). |
| #55447 | White Harbor: memory of The Outcasts Tower; the White Harbor QST and story use this exact item. [Prototype](../../../areas/obj/wh.obj#L5279). | O reset in #11342. [Reset](../../../areas/zon/otower.zon#L74). |
| #82018 | Jindon: psionicist power locket with an astral or other-dimensional origin. [Prototype](../../../areas/obj/jin.obj#L249). | G reset puts it on Ximixix (#11331) in throne room #11342. [Reset](../../../areas/zon/otower.zon#L162), [Jindon audit](JINDON_DEATHWOOD_AUDIT_REVIEW.md). |

A typed, context-aware scan found no other active ZON reset into Otower rooms, mobs, or objects, and no source-only ZON reset into those local VNUMs. It found no external type-25 portal targeting an Otower room, no local type-25 object, and no external SHP product or QST/story VNUM reference beyond White Harbor's #55447 use. No duplicate active mobile or object prototype shares Otower's local VNUMs.

Compiled data has additional exact references that matter to retention:

- Flog, mobile #11306, is also listed in the epic-teacher table for Improved Shield Combat. Epic initialization runs during normal startup and binds each listed mobile VNUM to the epic-teacher handler ([skill row](../../../src/classes/epic_skills.c#L181), [initializer](../../../src/world/epic.c#L1368), [startup call](../../../src/net/comm.c#L1035)). Flog's local description identifies him as a retired first-floor guardian, not as a trainer.
- The CHAOS starter-equipment data uses local objects #11306, #11307, and #11319 for Bard, Rogue, and Ethermancer kits. Those are the blood-soaked collar, Punisher rod, and glowing black ring ([kit rows](../../../src/account/chaos_eq_data.h#L177), [Bard row](../../../src/account/chaos_eq_data.h#L200), [Ethermancer row](../../../src/account/chaos_eq_data.h#L527), [kit loading](../../../src/account/nanny.c#L682)).
- Legacy Silver Lady ship code still refers to VNUMs that Otower now owns. The ticket table treats object #11300 as the ship object at rooms #5399 and #26200, while Otower's object #11300 is Brahvinos's small brittle key ([ticket table and handler](../../../src/specs/specs.mobile.c#L779), [key prototype](../../../areas/obj/otower.obj#L1)). The ticket handler compares the entered object's prototype with that ship-object VNUM. However, no assignment to ticket_taker was found; the Silver Lady room-assignment block is under #if 0, and the old navigator/shout helpers have no assignment to Otower mobiles ([disabled room block](../../../src/specs/specs.assign.c#L2461), [navigator helper](../../../src/specs/specs.minizones.c#L430), [shout helper](../../../src/specs/specs.mobile.c#L93)). Record these as dormant legacy references, not a live tower route. Preserve the VNUM until the old ship data is explicitly reconciled.

## Title, history, and retention

After color stripping and case folding, “The Outcasts Tower” is unique among all 442 current ZON titles ([title source](../../../areas/zon/otower.zon#L2)). Followed WLD, MOB, OBJ, and ZON history stays on the otower stem back to the 2009 move into the current areas tree. Later entries are same-stem content or line-ending edits. No alternate-stem rename, distinct predecessor, duplicate active prototype, or whole-zone replacement was established. The White Harbor memory and ambassador request adds a cross-zone quest link; it does not establish a replaced tower.

Keep active otower **DO NOT MOVE**. Preserve its two reciprocal WLD boundaries, the source-only Surf approach on the existing Surf row, throne-room objects #359 and #55447, Ximixix's Jindon locket, Flog's epic-teacher identity, and the three CHAOS object VNUMs. Keep Surf **RETAIN IN PLACE** with its existing row 97; do not add a candidate row or change its 145 active references. Through this checkpoint, curated source-only totals are 4,249 overall, 252 for Surf, 19 for ud_map2, and eight for Underworld2. No native WLD, MOB, OBJ, ZON, QST, C/C++, database, migration, or gameplay source is changed.

## Checkpoint 317: twenty builder questions and recommendations

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| OTW-01 | Is “The Outcasts Tower” the canonical public name, including the plural form, and should the ambassador wording be a searchable alias? | Keep the current ZON title and VNUM ownership. Record the ambassador phrase as an alias only after the area owner confirms it. |
| OTW-02 | Should otower remain in areas/qst/AREA despite having no local QST file? | Preserve the current manifest while confirming its purpose; do not infer a local quest from manifest membership. |
| OTW-03 | Is White Harbor's #55447 memory deliberately recoverable from Ximixix's room for the ambassador delivery? | Keep the exact cross-area item and source reset; confirm its availability and repeat behavior with the White Harbor quest owner before changing either side. |
| OTW-04 | Is the shared Heavens rune stone #359 intended as an object in the throne room? | Preserve the reset and its shared prototype; document a tower-specific interpretation only if the builders confirm one. |
| OTW-05 | Is Jindon's psionicist locket #82018 intended equipment for Ximixix? | Keep the typed Jindon dependency attached to Ximixix and retain both source identities. |
| OTW-06 | Does Trankula's text about shamans opening portals after battles describe history or an active travel feature? | Treat it as narrative only; the source defines no local portal item or active portal procedure. Do not promise or add travel behavior from this prose alone. |
| OTW-07 | What threat or campaign objective unites the tower's mixed forces against the races of light? | Preserve the coalition and its named leaders; ask for an objective, target, and outcome before creating a quest or changing faction text. |
| OTW-08 | Is the paired locked gate intended to be opened with the brittle key carried by Brahvinos? | Preserve the two key-bearing exits and Brahvinos's key reset; qualify the live gate interaction separately. |
| OTW-09 | Should the east trail from #11302 be an engine-hidden secret or a closed door described as bushes? | Confirm the intended discovery method. Keep the current WLD/ZON flags until the owner resolves the comment-to-flag mismatch. |
| OTW-10 | Should the mountain cave and grove approaches carry the EX_SECRET flag, or are their comments using “secret” as flavor? | Keep their current reset states and keyless doors until search, visibility, and opening behavior are explicitly chosen. |
| OTW-11 | Is the grove meant to remain reachable north from the rockslide room #11313? | Preserve the existing north exit and its closed reset, then reconcile the “closed off” rockslide prose with the hidden route. |
| OTW-12 | Is the alpine grove intended as an actual safe refuge for travelers opposing the tower? | Keep Aleilinal and the sanctuary language; do not infer protection mechanics from the environmental text. |
| OTW-13 | Should the steep route's fall and drowning warnings remain prose, or should one or more rooms have mechanical fall risk? | Preserve the zero fall/current fields until builders choose exact rooms, rates, and recovery routes. |
| OTW-14 | Is Aracnalis's adamantium key #11322 meant to unlock the upper stair from the east side of #11337? | Keep the item carrier and target key VNUM; correct the east/west exit description only after confirming the intended approach. |
| OTW-15 | Is the stair door deliberately locked from #11337 but only closed from the reverse side in #11338? | Confirm one-way access intent and the asymmetric reset states before editing either record. |
| OTW-16 | Is the illusionary key from Arcanusinil meant to reveal an engine-hidden exit to the throne room? | Preserve key #11321 and the locked up exit; decide whether the route should set EX_SECRET before changing WLD flags or comments. |
| OTW-17 | Does the text saying Bpollobop leaves his room for the swamp describe actual periodic movement? | Treat it as flavor until an effective movement procedure is identified and tested by the owner. |
| OTW-18 | Should the under-swamp cave remain a usable emergency exit through Underworld3? | Preserve both reciprocal WLD records and the wet route; qualify underwater admission, return, and character safety before claiming it is safe. |
| OTW-19 | Is Flog #11306 intentionally an epic trainer for Improved Shield Combat as well as a retired guardian? | Keep the compiled trainer binding and local VNUM; confirm whether that dual role is intended before changing content or code. |
| OTW-20 | Should local objects #11306, #11307, #11319, and key #11300 retain their current IDs despite CHAOS-kit and dormant Silver Lady references? | Preserve the object IDs and current code state. Reconcile each old code reference before any attic move, prototype renumbering, or ship-handler reactivation. |

## Scope and validation

Checkpoint 317 adds the source dossier and this audit review, updates the Otower coverage row, and records the one inactive Surf approach on existing Surf documentation. Source inspections are read-only. No area, C/C++, quest, database, migration, server, or gameplay source is changed. Documentation links and anchors, coverage register shape, question count, whitespace, and restricted-directory status are checked after the edits; no build or source test is run.
