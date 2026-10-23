# Rabble: connectivity, dependencies, history, and builder decisions

**Checkpoint 334; coverage row 245.** This supplement reconciles the Rabble setting dossier with the active WLD graph, inactive WLD sources, reset roles, shop stock, external prototypes, normalized title, compiled references, and followed source history. Static source records do not establish live actor availability, service success, or played access.

## Active WLD graph and arrival reach

Rabble has six active rooms and eleven authored exits. Ten exits stay inside the six-room component; the eleventh goes from #65100 up to Heavens #1234. All eleven targets resolve. The local weak graph is one component of six rooms. No negative or positive unresolved targets, fall markers, currents, or D door resets are declared.

| Active boundary | Reciprocal boundary | Cross-area register |
| --- | --- | --- |
| Heavens #1234 down to Rabble #65100 | Rabble #65100 up to Heavens #1234 | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L20), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L825) |

The only active inbound WLD root reaches all six local rooms by directed exits. Since there are no D resets, raw and strict post-reset directed reach are both 6/6 from #65100.

| Arrival root | Raw directed reach | Strict post-reset reach |
| --- | ---: | ---: |
| Heavens #1234 to Rabble #65100 | 6/6 | 6/6 |

The all-95 inactive-WLD scan parsed 2,080,773 exits without parse errors. It found no source-room overlap and no inactive exit into Rabble. No source-only approach, candidate row, or curated source-only total changes here. These are static WLD results and do not qualify actual movement, actor presence, or the shop.

## Typed dependencies and bounded code scans

### Local reset and shop references

Rabble's reset and shop records use local prototypes as follows:

| Local consumer | References | Meaning and qualification limit |
| --- | --- | --- |
| ZON object placements | Sign #65100 in room #65100; Fountain #65106 in #65102; sign #65101 and Punch Bowl #65105 in #65103 | Exact local O rows at [ZON lines 9-12](../../../areas/zon/rabble.zon#L9). These declarations do not qualify visible runtime placement. |
| ZON mobile placements | Four repeated Obsidian Demon rows #65100 in room #65100, each declaring cap 4; one Frothe row #65101 in #65103, cap 1 | [Reset rows](../../../areas/zon/rabble.zon#L14). The audit preserves the repeated records without inferring the number live after reset. |
| Frothe inventory reset rows | External #377, #347, #399, #53, #6094, #6098, then local #65102, #65103, #65104 | [G rows](../../../areas/zon/rabble.zon#L19). The local objects are also listed by the shop. |
| Rabble SHP | Keeper #65101; products #65102, #65103, #65104 | [shop record](../../../areas/shp/rabble.shp#L1). Stock names come from local object prototypes; actual transaction success was not tested. |

### Imported and externally consumed prototypes

The bounded exact reset scan found no external QST item command, SHP product row, story-sidecar VNUM, or ZON reset-role row consuming Rabble-owned room, mobile, or object prototypes. No other current mobile or object source defines a Rabble-local VNUM. No scanned external type-25 object targets a Rabble room: the scan covered 872 current declarations. The 231 other story sidecars parsed without an exact structured reference to a Rabble VNUM.

Rabble imports six external object VNUMs into Frothe's G reset list:

| Object VNUM | Current prototype source | Audit finding |
| ---: | --- | --- |
| #377 | Active Heavens; large leather backpack, [prototype](../../../areas/obj/heavens.obj#L3933) | One current active definition. |
| #347 | Active Heavens; leather waterskin, [prototype](../../../areas/obj/heavens.obj#L3600) | One current active definition. |
| #399 | Active Heavens; small brass lantern, [prototype](../../../areas/obj/heavens.obj#L4153) | One current active definition. |
| #53 | Active Heavens; hemp rope coil, [prototype](../../../areas/obj/heavens.obj#L788) | Three other current source-only object files also define #53 with unrelated names (Dalvik, Hostel, and Mavulsk); those files are not active contributors. The active reset currently has one active definition to resolve. |
| #6094 | No current object prototype found | The same number is a room in lizard.wld, not a matching object definition. |
| #6098 | No current object prototype found | The same number is a room in lizard.wld, not a matching object definition. |

Keep the four sourced active Heavens items under their current owners. Preserve the #6094 and #6098 reset references as unresolved source data until their owner identifies the intended prototypes. Do not treat same-number WLD rooms as object definitions.

### Compiled routes and source identity

An exact numeric scan of maintained C/C++ source found no Rabble room, mobile, or object VNUM reference. The Heavens boundary is an authored WLD exit, not a compiled portal alias. The active global room scan shows no Rabble room-number collision; the unrelated active duplicate #51800 remains in Hell Two and Hell Three. Rabble's own local room, mobile, and object definitions are unique within their respective files.

## Title, identity, and followed history

After color-code removal, whitespace normalization, and case folding, the title “Rabble” occurs once among 442 current zone titles and once among 351 active area contributors. The source-stem review found no alternate-stem predecessor or explicit whole-zone replacement.

Followed history stays on the rabble source paths. The WLD records the 2009 area-tree move and a 2013 content edit. The MOB and OBJ history shows the 2009 area-tree move. The ZON history includes the 2009 move, 2013 content edit, 2014 shop cleanup, and 2015 quest-item flag update; the SHP history includes the move, 2013 edit, and 2014 shop cleanup. The bounded histories and current register provide no evidence of a predecessor or replacement.

Keep active Rabble **DO NOT MOVE**. Its reciprocal Heavens boundary, the tavern shop, and the reset inventory links are active source relationships; two imported object VNUMs remain unresolved. No candidate or source-only WLD count changes in this checkpoint.

## Checkpoint 334 builder questions and recommended resolutions

These questions give the area owner concrete decisions from current source. The recommendations preserve authored data until an intended change is selected and qualified.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| RB01 | Is the onyx-building portal meant to arrive at Heavens room #1234, Fo's Place? | Preserve the reciprocal #65100 up / #1234 down boundary and align any revised prose with that active route after the area owner confirms the intended fiction. |
| RB02 | Does the portal extra description's blue-lit crypt describe Fo's Place or a different far side? | Keep the current description as authored text until its destination is identified; do not invent a new endpoint from the extra description alone. |
| RB03 | Are the identical “portal” extra descriptions in Keira's Playroom and Room #13 intentional? | Verify those two copies with the builder; they have no corresponding portal exit, so do not represent them as separate travel routes. |
| RB04 | Is “the Nexus where all ends meet” intended as metaphor or as a claim of world access? | Preserve it as environmental prose unless the owner names a separate implemented route. The active WLD has one cross-area boundary. |
| RB05 | Is Room #65105 the “familiar place” south of the Twilight Room? | Keep the present south edge and Room #13 text; do not attach a separate home or character history without source evidence. |
| RB06 | Is Keira's Playroom meant to be private when the WLD passage is open? | Retain the invitation language; treat it as social prose until the owner selects and qualifies an actual access rule. |
| RB07 | Should Room #13 contain furnishings corresponding to the bed and chest in its prose? | Preserve the room text and current reset list; let the owner decide whether to add set dressing after checking intended ownership. |
| RB08 | Is the “familiar place” and “private retreat” wording still correct for the two connected rooms? | Keep both labels attached to their present routes and ask the owner to clarify only if their intended referents have changed. |
| RB09 | Are Ithaquua, Taranno, and Zupov the intended trophy subjects? | Preserve the three names and their display text; do not turn the trophies into kill credit or progression evidence. |
| RB10 | Is “Pin the Dart on the Thugan” intended only as a written dartboard joke? | Keep it as an extra description. The WLD does not define a dartboard object, special handler, or success outcome. |
| RB11 | Should the newspaper clippings' relative “five years ago” framing remain? | Preserve the time wording unless the builder supplies an in-world date and desired revision. |
| RB12 | Are the Mc'Malok, Parka, Malok, Cython, Tripod, and Tolm references intentional shared-world humor? | Retain their current wording and references; do not expand these clippings into quests or current events without owner direction. |
| RB13 | Is Frothe's dwarf Brewmeister description and “Me Dorf” speech intended as the shopkeeper's complete role? | Keep the current mobile and speech strings; no QST or special assignment gives Frothe another recorded role. |
| RB14 | Are the four repeated Obsidian Demon M rows intentional with the shared cap of four? | Preserve the rows until the zone owner reviews reset behavior; don't infer that four rows guarantee four live Demons. |
| RB15 | Is the Demon a hostile encounter, a visual guardian, or both? | Preserve its current name, flags, description, and reset location; classify actual encounter behavior only after qualification. |
| RB16 | Are Hot Wings, Guiness, and the Rabble Insignia Patch the intended shop catalog? | Preserve the three listed product VNUMs; confirm names and intended availability before changing shop stock. |
| RB17 | Is the Insignia Patch meant to remain a sold object described as useless cloth? | Keep the object and wording together until the owner chooses a cosmetic or other purpose; do not assign it a quest function. |
| RB18 | What are the intended objects for external reset VNUMs #6094 and #6098? | Preserve both reset rows and record them as unresolved until current object prototypes or an owner decision identify them. |
| RB19 | Should G reset VNUM #53 resolve specifically to the active Heavens hemp rope? | Keep that active definition as the current loaded target and document the source-only duplicates; don't move or select another prototype based only on VNUM equality. |
| RB20 | Are the Obsidian Fountain and Kamakazi punch bowl decorative, usable, or meant to provide liquid? | Preserve both type-17 object records and current placements. Verify live object behavior and intended replenishment before describing a use contract. |

This was a documentation-only checkpoint. No WLD/MOB/OBJ/ZON/SHP source, code, area membership, candidate row, shop catalog, or gameplay behavior was changed. No build, test, server, database, migration, or gameplay session was run.
