# Fateful Falls (`beggin`) typed and candidate audit

Checkpoint 877, reviewed October 12, 2026. This supplement closes the bounded tracked-source reference review for the unregistered Fateful Falls component. It does not promote the source into the active loader list or establish runtime availability. The component contains WLD, MOB, OBJ, ZON, and QST files only: two rooms, two mobiles, one object, three reset commands, and one MA default response. It has no SHP file, story sidecar, or Q/QA contract.

## Candidate graph and map ownership

`beggin` is absent from the current full-area and QST-area lists. Its two WLD records, #3300 and #3301, form a reciprocal east-west pair in one two-room component; neither room has an authored exit outside this source file.

Room-header comparison against all 445 other tracked WLD files found no same-number room. Exact WLD source/target scans found no active or unregistered route into #3300/#3301, and no target leaving the pair. The 94 other inactive WLD sources parse without errors; the two generic parser exceptions among active files are legacy exit-field layouts in `northern_wilderness.wld` at lines 1232 and 1963. Their raw destination tuples are #12270 and #6803, and exact numeric target scanning found no #3300/#3301 reference. These inactive records remain outside active-world reachability counts.

The local graph and loader status are documented in [Fateful Falls](../zone-stories/FATEFUL_FALLS.md), [connectivity checkpoint 877](WORLD_CONNECTIVITY.md#checkpoint-877-fateful-falls-remains-a-retained-two-room-source-fragment), and [coverage line 366](ZONE_COVERAGE_REGISTER.csv#L366).

## Local reset and QST declarations

The three `beggin.zon` prototype placements resolve to same-stem definitions:

| Reset | Local prototype | Placement |
| --- | --- | --- |
| O at [line 9](../../../areas/zon/beggin.zon#L9) | OBJ #3300, Tome of Command | Room #3300. |
| M at [line 11](../../../areas/zon/beggin.zon#L11) | MOB #3301, village girl | Room #3300. |
| M at [line 12](../../../areas/zon/beggin.zon#L12) | MOB #3300, black cat | Room #3301. |

There are no foreign prototype imports or unresolved local reset-role operands. The local QST file has one #3301 MA default response prompting the player to look at the book ([QST](../../../areas/qst/beggin.qst#L1)); it defines no Q or QA contract. The local source files contain no harvest item, reward, or separate quest endpoint for the field description or the cat's Zornade tag, as covered by the existing [six builder questions](../zone-stories/FATEFUL_FALLS.md#open-area-owner-questions-and-conservative-recommendations).

## Inbound and outbound typed-reference closure

Exact typed scans across current active and unregistered sources found no inbound reference to the Fateful Falls prototypes or rooms and no outbound foreign prototype dependency:

- ZON reset operands: no other active or unregistered reset command references mobile or object #3300/#3301, or places a reset into either room. This includes reset-role operands beyond M/O/G/E, such as follower and mount commands.
- QST: no outside item command references object #3300 and no foreign QST giver header references mobiles #3300/#3301.
- SHP: no tracked shop product lists object #3300; the component has no local SHP file.
- Story sidecars: no typed `*_vnum` field or typed completion-key item/mobile/room operand in another sidecar references #3300/#3301; `beggin` has no local sidecar.
- Object travel: no tracked type-25 destination targets rooms #3300/#3301, and no type-29 ITEM_SWITCH `value[1]` destination targets either room.

The same-type prototype census found no other tracked MOB or OBJ definition for #3300/#3301. A source-wide C/C++ numeric scan returned two literal 3300 matches: a dexterity-application table value in [constant.c line 855](../../../src/core/constant.c#L855) and the `RANDOM_INSTRUMENT` category constant in [rand_items.h line 19](../../../src/item/rand_items.h#L19). Neither is a WLD/MOB/OBJ VNUM binding. No exact typed compiled reference to a Fateful Falls room or prototype was found.

## Title, history, and disposition

The normalized ZON title “Fateful Falls” matches only `beggin.zon` among current area-title sources. Followed WLD, MOB, OBJ, and ZON history starts with commit `de80d800e` (November 12, 2011), whose message describes a new zone stub. The QST path begins at `f18992b8f` (October 13, 2015), and the OBJ path also records a 2016 text/capitalization cleanup. The tracked path and current identity scans identify no alternate-stem predecessor, same-type map replacement, or explicit retirement/successor record. See [Checkpoint 877 in the duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-877-fateful-falls-keeps-its-beggin-source-identity).

Keep candidate `beggin` **RETAIN IN PLACE** under candidate data row 14 / CSV line 15. Preserve the two-room source fragment and its existing narrative. The exact tracked-source dependency closure is complete; editor/tool references, generated/deployed state, persisted identity, and owner intent remain open. Keep the six existing area-owner questions and grant no migration clearance. See the [candidate plan](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L15), [coverage row](ZONE_COVERAGE_REGISTER.csv#L366), and [source dossier](../zone-stories/FATEFUL_FALLS.md). No native source, active map, or runtime state changed.