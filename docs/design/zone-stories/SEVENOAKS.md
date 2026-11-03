# Sevenoaks: source-story map

Coverage row 263 maps active contributor `seveno` (`areas/AREA` line 187),
with room VNUMs 38600–38676. This dossier records authored source narrative,
native placement and declared source links. It does not establish live access,
completed quests, or runtime behavior. Repository-wide graph, title/history,
dependency and builder decisions are in the [Sevenoaks audit supplement](../quest-story-followups/SEVENO_AUDIT_REVIEW.md).

## Story shape

The area opens on a snow-covered path and a walled settlement guarded by an
elite guard and cleric. Inside, the town is organized around a warm-water
fountain and streets whose names signal their changing character: warmed
stones, a wind-battered road, the confusing street of mirrors, a chaotic
street, and a memorial-lined Street of Bravery. The tower and its vault sit
above the town. Separate chambers and laboratories carry the work of the
sisters into fire, wind and mirror-filled spaces.

The authored story is a set of local leads rather than a native QST chain.
Kyz waits near the tower seeking revenge on Oslyra after his companions died
challenging her; memorials on the Street of Bravery give that loss a place in
the town. Myrina's chambers and reflections frame her as a sister of magic
and mystery. Ayrina studies air and flight, with Alyza described as a wind
dragon summoned by Ayrina. Fuerina is tied to fire and to the demons and
plants created in her chambers. Wyrina, a druid posted at the gate, says that
she and her sisters rule Sevenoaks and keep unwanted visitors out. The
holding den is titled for Olystra, while nearby creature text uses Oslyra;
this dossier preserves both spellings pending builder intent.

The tower's history also mentions Aerlyn, a high mage. A royal guard serves
Aerlyn, and a spectre is described as trapped by Aerlyn; another trapped
spectre's description attributes its imprisonment to Myrina. These are
separate source claims, not evidence that the characters or events are the
same. Room and object prose also places the vault, a displaced former
prisoner, old studies, hidden cellars and a large mirror maze in the tower.

## Narrative landmarks

| Thread | Source evidence | Mapping note |
| --- | --- | --- |
| Snow approach and defended town | [WLD rooms 38600–38606](../../../areas/wld/seveno.wld#L1), [guard and cleric prototypes](../../../areas/mob/seveno.mob#L1) | The town wall and gate defenses frame entry. The active Surface edge is separately recorded in the audit; prose does not certify public access. |
| Named streets and memorials | WLD rooms 38607–38624; Kyz is mobile 38631; see [Sevenoaks MOB](../../../areas/mob/seveno.mob#L469) | Warmed stones, wind, mirrors, chaos and bravery are distinct street motifs. Kyz's revenge lead is not tracked by a local QST file. |
| Sisters and tower | WLD rooms 38625–38632 and 38646–38676; mobiles Alyza 38604, Ayrina 38605, Myrina 38620 and Fuerina 38637 | Their studies and created beings establish the elemental/magical themes. Wyrina's gate speech is mobile 38647. Preserve the Olystra/Oslyra and Aerlyn/Myrina attribution questions. |
| Mirror maze and lower rooms | WLD rooms 38633–38645, mirror/reflection mobiles, and objects 38622–38625 | The maze has intersections, mirrored passages, two named dark cellars and object-defined destinations. Static WLD connectivity alone does not include type-25 object travel. |
| Fire and air works | WLD rooms 38651–38666 and 38667–38676 | Fire rooms associate with Fuerina; air rooms with Ayrina. Treat creator prose, reset placement and actual transport as separate evidence. |
| Clues and artifacts | Objects 38608–38618, 38635–38636, 38649, 38656 and 38658 | Notes, medallions, keys, portal templates and vault goods suggest local puzzles and rewards, but no native QST file records their completion. |

## Native quest and object-source boundary

`areas/qst/AREA` lists `seveno *386 (melkivar)`, but there is no
`areas/qst/seveno.qst`, local SHP file or `areas/story/seveno.story.json`.
The listing therefore does not provide a native giver/step/completion record.
The handwritten clue object 38608 says that the author must be found by using
one's head. A second note, object 38616, ends in a visibly incomplete line
after “you need to try”; its intended text is unknown. Object 38617 is named
as the wind key, while its short description says “A key of fire sits here.”
These are source-backed builder questions, not silently corrected copy.

Eleven local object prototypes declare type-25 destinations, all within
Sevenoaks: 38605→38651, 38609→38667, 38612→38606, 38613→38659,
38618→38672, 38622→38633, 38623→38615, 38624→38616, 38625→38636,
38628→38646 and 38633→38606. The small white portal (38618) and skylight
(38628) are named directly in their descriptions; the other declarations
include mirrors and other objects. Their exact commands, admission rules and
successful return are not established by these declarations.

The active ZON places imported Heavens object 359 and White Harbor object
55443 in vault room 38632. The broader typed consumers and compiled bindings,
including Alatorin's use of local object 38639 and the special longsword
callback on 38649, are preserved in the [audit supplement](../quest-story-followups/SEVENO_AUDIT_REVIEW.md#typed-dependencies-and-compiled-bindings).

## Static map and disposition

The WLD file defines 77 rooms and 222 exits. Its seven weak components have
31, 14, 9, 8, 5, 5 and 5 rooms; object-defined routes are outside that graph.
Surface room 516252 enters room 38600 and is met by the reverse local exit.
White Harbor room 55632 has a one-way exit into vault room 38632. Two
additional approaches are found only in inactive WLD files, so they remain
candidate records rather than active travel: `surf.wld` 209270 north to
38600 and `underworld2.wld` 425645 west to 38600.

There are five rooms with fall-chance values and two rooms with current
values in the current WLD census. Thirty-four 100%-chance D resets match all
34 nonzero exit-info directions; that match records source declarations and
does not certify a door's live state or passage. Exact room/exit/reset counts,
the conservative filtered-reach comparison, inbound/outbound records and
builder decisions appear in the [connectivity checkpoint](../quest-story-followups/WORLD_CONNECTIVITY.md#checkpoint-351-sevenoaks-reset-aware-connectivity-and-inactive-map-identity).

Keep active `seveno` in place. Preserve both active boundary identities, the
external objects loaded in the vault, Alatorin's consumer of object 38639,
the compiled equipment references and the separate source-only candidate
approaches. No migration clearance or gameplay repair is proposed.
