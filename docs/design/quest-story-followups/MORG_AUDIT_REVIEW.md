# Morg's Place repository-wide source audit

Checkpoint 210 maps active `morg` against its complete local WLD, MOB, and ZON text; active and unregistered WLD boundaries; title ownership and followed source history; and typed outside references. This is a static source audit. It does not inspect a live server, player state, database, editor, persisted world state, or actual gameplay. The [source dossier](../zone-stories/MORG.md) summarizes the setting and builder questions.

## Source identity and authored content

The active area list calls the contributor `morg` with note `*887`; the local zone header is #886, titled “Morg's Place,” and ends at room #88629. Its WLD has 30 rooms (#88600–#88629), its MOB has 19 prototypes (#88600–#88618), and `morg.obj` is present but empty. The same stem appears in `areas/qst/AREA`, but there is no local QST file, SHP file, story sidecar, or native Q/QA contract ([area list](../../../areas/AREA#L313), [zone header](../../../areas/zon/morg.zon#L1), [QST area index](../../../areas/qst/AREA#L279), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188)).

The authored setting is an ogre-run tavern and inn in an Underdark chasm. The entry alcove sits outside the large wooden door. Inside, repeated chamber records describe a busy bar with tables, mugs, a fireplace, brazier, and bar staff. The mobile cast covers bouncers, troll patrons, drunken ogres, derro, duergar, serving wenches, a bartender, an elegant drow, Morg and his mate, and wererats. Morg is installed in his sparse owner’s quarters; the source depicts him as enormous and wheezing, while his mate is characterized as substantially smaller. These descriptions do not establish a task or an event sequence ([main-room text](../../../areas/wld/morg.wld#L20), [mobile cast](../../../areas/mob/morg.mob#L1), [owner mobiles](../../../areas/mob/morg.mob#L49)).

The back room has marked stones and cards, and the floor marks correspond to its reset-closed secret exit down to a moss-lit passage. From there, the WLD leads to a wine cellar and storeroom. A second reset-closed secret door opens from the cellar into five rooms described as a bloody nest; resets place wererat variants there. These local details form a concealed-under-tavern subarea, not a separately listed quest zone ([back room and secret passage](../../../areas/wld/morg.wld#L338), [cellar and storeroom](../../../areas/wld/morg.wld#L401), [nest](../../../areas/wld/morg.wld#L435), [wererat mobiles](../../../areas/mob/morg.mob#L210)).

The full ZON has 14 D commands and 64 M commands. All 19 local mobile prototypes are referenced by resets; there are no O or P commands, and no local object prototypes. Ten door commands set ordinary exits closed; four set exits closed and secret. Reset state 1 closes an exit and clears its lock flag; state 5 does the same and sets the secret flag ([zone commands](../../../areas/zon/morg.zon#L8), [door-state loader](../../../src/world/db.c#L7857)).

## Active WLD graph and source-text gaps

The local WLD has 69/69 resolving exits: 68 local and one outbound. Its raw weak graph is one component of 30 rooms. UnderDark chasm #844836 exits south to #88600, and #88600 exits north back to #844836, forming one inbound, one outbound, and one reciprocal boundary pair. There is no second active boundary into the area ([local entrance](../../../areas/wld/morg.wld#L1), [UnderDark return](../../../areas/wld/underdark.wld#L262262)).

The boundary arrives in the alcove before the large tavern door. The reciprocal room-to-room exits at #88600 east and #88604 west both reset closed, so the raw connected graph does not by itself show access from the arrival alcove into the main bar. The local zone also closes the doorway to the proprietor’s anteroom, three private-room doors, and two secret pairs: the back-room drop to #88614 and the cellar entrance to the wererat nest. The source-defined states do not set a key lock, but actual opening, secret discovery, and movement remain unqualified.

Two room descriptions imply exits not present in the WLD. Room #88614 mentions normal light from above and a possible escape hatch but has no upward exit. Room #88623 says a stone hallway continues north toward the inn rooms, but its only authored exit is south to the tavern. Room #88628 describes a latched metal box with two steel locks, but the local object file is empty and no reset places an object there ([passage](../../../areas/wld/morg.wld#L358), [reception](../../../areas/wld/morg.wld#L534), [owner's room](../../../areas/wld/morg.wld#L622), [object source](../../../areas/obj/morg.obj)).

UnderDark room #845236 is titled “Morg's Palace,” has no description, and exits to the same chasm #844836 that borders Morg's Place. Its title and shared approach are relevant naming evidence, but do not prove the palace and tavern are the same location or that one replaced the other ([room title](../../../areas/wld/underdark.wld#L263144), [exit to the chasm](../../../areas/wld/underdark.wld#L263149)).

## Outside references and duplicate definitions

Exact VNUM scans found no external ZON reset reference to local rooms or mobiles, no QST numeric reference, no SHP reference, no story-sidecar numeric reference, and no duplicate external MOB or OBJ definition. No WLD exit key refers to a local object VNUM; every local key field is zero. Scans of external ITEM_TELEPORT and ITEM_SWITCH destinations found no target in #88600–#88629. No exact local VNUM reference was found in C/C++ source. The only external WLD target into the local room range is the active reciprocal UnderDark edge above. The all-95 unregistered-WLD scan found no local room overlap or source-only approach. No candidate-register row or curated source-only count changes.

The QST/AREA entry does not correspond to a local quest file or an outside numeric consumer. The bartender and inn descriptions therefore remain authored flavor in the checked source; the repository does not define a local drink shop, item stock, or quest exchange for this area. The empty local OBJ file also means that described mugs, cards, marked stones, and metal box have no local object prototypes or reset placement.

## Title, followed history, and disposition

After color stripping and normalization, “Morg's Place” is unique among the 442 current ZON titles. Followed WLD, MOB, OBJ, and ZON history remains on the `morg` stem through the 2009 area-tree move and later same-stem edits through 2016. No alternate-stem predecessor, retirement, or whole-zone replacement is established. The UnderDark WLD label “Morg's Palace” is retained as an unresolved naming relationship, not used as evidence of replacement.

Keep active `morg` **DO NOT MOVE**. Preserve rooms #88600–#88629, all 19 mobile prototypes, the reciprocal chasm boundary, the reset door states, and the nested wererat route. Do not add a source-only candidate or migration clearance. See the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-210-morgs-place-title-and-source-history), [coverage row 188](ZONE_COVERAGE_REGISTER.csv#L188), and [source dossier](../zone-stories/MORG.md).

## Builder follow-up

1. Decide whether UnderDark room #845236, “Morg's Palace,” names a distinct place or an alternate label related to Morg's Place.
2. Confirm that the Underdark arrival should land outside a reset-closed front door and whether the existing door state is intended.
3. Resolve the possible escape hatch at #88614 and the northward inn hallway described at #88623.
4. Confirm whether the metal box in #88628 is scenery or should exist as an object prototype and reset.
5. Clarify the QST/AREA listing when there is no local QST, shop, object, or sidecar implementation.

## Checkpoint 290: all-442 title and bounded history

Color-code, trailing-tilde, case, and whitespace normalization again finds one
owner for “Morg's Place” among all 442 current `.zon` files:
[`morg.zon`](../../../areas/zon/morg.zon#L2). Followed WLD and MOB histories
remain on the `morg` stem from the 2009 area-tree move through 2015; OBJ and
ZON histories remain on that stem through 2016. The bounded paths establish no
alternate-stem predecessor, retirement, or whole-zone replacement. UnderDark
room #845236, “Morg's Palace,” remains a related-name owner question on the
same chasm approach, not replacement evidence. Keep active `morg` **DO NOT
MOVE**.

The existing direct source closure remains bounded and complete: the only
external active WLD target is the reciprocal UnderDark chasm boundary; no
inactive-WLD approach or room overlap was found; no exact outside ZON, QST,
SHP, sidecar, duplicate MOB/OBJ, teleporter/switch, keyed object, or C/C++
consumer was found. Local WLD, MOB, and ZON sources define the area; the local
OBJ is empty, and no local QST or SHP exists despite the QST/AREA entry. This
does not qualify editor references, persisted state, indirect consumers, live
stock, or gameplay.

## Checkpoint 290: direct builder and owner questions

These twenty source-backed questions close the builder-question review. The
recommendations preserve the authored identities and reset evidence until an
area owner decides whether a source correction is intended.

1. **Question:** Is UnderDark room #845236, “Morg's Palace,” a distinct place
   or a related name for Morg's Place?
   **Recommendation:** Keep the relationship unresolved. The title and common
   chasm approach do not prove identity or replacement; seek an owner decision
   before merging names or areas.
2. **Question:** Does the unique title “Morg's Place” identify this tavern as a
   separate active contributor from the nearby palace label?
   **Recommendation:** Preserve the unique all-442 `.zon` title owner,
   `morg`; record the palace question separately and do not infer another
   active zone owner from a WLD room title.
3. **Question:** Should UnderDark chasm #844836 continue south to Morg's
   entrance #88600, with #88600 north returning to the chasm?
   **Recommendation:** Preserve this one reciprocal active boundary and keep
   it distinct from room-level door exits; it is the only active WLD boundary
   into the area.
4. **Question:** Should arrivals at #88600 stop outside the tavern's main
   chamber while the east doorway resets closed?
   **Recommendation:** Keep the source-defined state visible and ask the area
   owner whether a closed entrance is intended. Do not treat the raw one-part
   weak graph as proof of movement through that door.
5. **Question:** Are reset states 1 and 5 being used as intended for closed,
   unlocked doors and closed, secret doors?
   **Recommendation:** Retain the source and loader reading: state 1 closes
   without setting the lock bit; state 5 closes and sets the secret bit. Do not
   describe either state as a keyed lock.
6. **Question:** Should the ten ordinary door exits that reset closed remain
   closed, including the entry, anteroom, and three private-room pairs?
   **Recommendation:** Keep the ten authored ordinary D commands and obtain
   owner direction before changing privacy or arrival flow. No key requirement
   is established by these resets.
7. **Question:** Is the down exit from marked-stone back room #88613 to passage
   #88614 meant to remain closed and secret?
   **Recommendation:** Preserve the paired reset state and the room's floor
   clue. The source shows a WLD exit named `trapdoor`; it does not define a
   separate trapdoor object.
8. **Question:** Should the return up exit from #88614 to #88613 also reset
   closed and secret?
   **Recommendation:** Keep the reciprocal D state intact until the builder
   confirms intended discovery and opening behavior; qualify the loaded route
   separately.
9. **Question:** Is the west door from wine cellar #88616 to nest #88618
   intentionally closed and secret?
   **Recommendation:** Preserve the matching cellar-side reset and the WLD
   door keyword `cask`; the room prose alone does not establish an open route.
10. **Question:** Should nest room #88618's east return to the cellar reset
    closed and secret as well?
    **Recommendation:** Retain the reciprocal reset and ask whether its secrecy
    is intentional. Do not describe the wererat rooms as ordinarily accessible.
11. **Question:** Are the floor marks and hidden-under-table language in #88613
    sufficient as the discovery clue for the WLD trapdoor?
    **Recommendation:** Keep the text/exit relationship documented and request
    a builder decision if a clearer clue or separate object is wanted; no object
    prototype or object reset currently supplies one.
12. **Question:** Should the possible escape hatch described in #88614 have an
    upward WLD exit?
    **Recommendation:** Preserve the discrepancy as a source gap. Confirm the
    intended destination before adding any exit; the description is not itself
    a route.
13. **Question:** Should reception room #88623 have a northward hall exit to
    match its description of an inn corridor?
    **Recommendation:** Keep the source graph as written and ask the owner to
    choose whether the prose or layout should change. Do not invent the target
    room from the description.
14. **Question:** Is the latched box with two steel locks in Morg's room #88628
    scenery, or should an object exist there?
    **Recommendation:** Treat it as scenery in the current catalog: `morg.obj`
    has no prototypes and `morg.zon` has no object reset. Do not promise a
    searchable or lootable box without a separate object implementation.
15. **Question:** Is the `morg` entry in `areas/qst/AREA` intentional without a
    local QST file or native Q/QA contract?
    **Recommendation:** Leave it as a list/source inconsistency for its owner
    to resolve. No contract or exchange is defined by the listing alone.
16. **Question:** Should the bartender or inn offer a registered shop despite
    the absent SHP file?
    **Recommendation:** Keep the bartender and inn as authored cast and flavor;
    no local shop or stock is source-defined. Ask the owner before adding a
    service claim or shop implementation.
17. **Question:** Are the casks, mugs, cards, and marked stones intended only
    as room scenery?
    **Recommendation:** Preserve the authored descriptions without assigning
    item behavior. The local OBJ is empty and there are no object reset
    commands.
18. **Question:** Should the reset placement of all 19 local mobile prototypes
    across 64 M commands remain as authored, including the wererat population?
    **Recommendation:** Retain the source reset inventory and ask the owner
    before balancing or moving placements. Static reset rows do not prove live
    population or access.
19. **Question:** Does the single 30-room weak graph establish usable passage
    from the chasm through the tavern and into the secret nest?
    **Recommendation:** No. Keep the graph count separate from ten reset-closed
    ordinary doors and four reset-closed secret exits; actual opening,
    discovery, and travel remain unqualified.
20. **Question:** Should active `morg` be considered for attic migration or
    receive another route/candidate record?
    **Recommendation:** Keep **DO NOT MOVE** as a current `areas/AREA`
    contributor, preserve its sole reciprocal UnderDark boundary, and add no
    source-only approach or candidate row. Revisit disposition only through a
    separate owner-reviewed proposal.

Checkpoint 290 completes the all-442 title/history confirmation and twenty
builder questions for `morg`. It adds no WLD route, attic candidate, or source
change and makes no claim about live access, mobile population, or gameplay.
