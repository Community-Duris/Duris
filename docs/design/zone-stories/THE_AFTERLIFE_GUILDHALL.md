# The Afterlife Guildhall

Canonical key: `component:afterlife_gh`. Physical coverage row 354. This source
set is not a full `areas/AREA` or `areas/qst/AREA` contributor. The
[repository-wide audit](../quest-story-followups/AFTERLIFE_GH_AUDIT_REVIEW.md)
records its source graph, Torg room-number overlap, local lore QST, imported
objects, and editor map. The separate [builder review](AFTERLIFE_GUILDHALL_BUILDER_QUESTIONS.md)
collects unresolved owner decisions. Neither document establishes active
access or live behavior.

## The house and its people

The ten-room map presents a wooden house for the “AftwerlLife,” a close-knit,
church-like company that accepts members who follow different gods. Its lore
conversation describes shared beliefs in an afterlife and reincarnation, with
members sometimes returning through resurrection or the spirit world. The
house is described as a family home between crusades and returns. A shared
basement altar accommodates the group's many religions; the conversation says
that simultaneous appearances of different gods can start a brawl. These are
characters' descriptions of their beliefs and social life, not proof of a
working resurrection, spirit-world travel, or god encounter.

Suse, a half-naked elven woman at the entrance, offers information about the
group and house through `areas/qst/afterlife_gh.qst`. The file contains greeting
and message responses but no `Q`/`QA` objective or reward contract. A handsome
human is hired to guard the house and direct young women to Suse; the two large
golems also direct visitors to her. The text establishes these lines and roles
but not any relationship, player-state result, or conversation access rule.

The small gift shop is run by a one-eyed gnome described by the house's people
as a tolerated outsider selling “worthless junk.” The ZON equips him with
three local foods and imported objects, and his SHP offers fourteen products.
Its refusal refers to “your people” without defining the intended group. The
shop's authored tone mixes household generosity, teasing, and exclusion; no
buyer condition is inferred here.

## Rooms and authored details

| Room | Source title | Description and local connection |
| --- | --- | --- |
| #29100 | Before a Wooden House | An ASCII drawing frames the house. North (`D0`) enters the hall; up (`D4`) targets #123813, titled “Rolling Hills” in `Duris3.wld`. |
| #29101 | Entrance to the House of AftwerlLife | A purple-carpeted hall has a fountain, paintings, stairs, and the training room to the east. Stairs lead to the dining landing and basement. |
| #29102 | Top of the Flight of Stairs | A landing opens to the dining room and suspended garden; food stains and unauthorized snacking give it a domestic tone. |
| #29103 | Entrance to the Basement | A cellar connects the food store, altar, and gift shop. Rotten cheese, urine, and an empty vodka bottle supply comic disorder. |
| #29104 | The Food Storage Room | Shelves, a water basin for food, and a half-empty wine rack serve the household. |
| #29105 | Behind a Huge Crystal Window | Twenty dining chairs surround a table; one chair seems large enough for a cow. Food stains and vomit follow a celebration of a successful resurrection. |
| #29106 | The Hanging Garden | Exotic plants include a man-eating marakanth from Hyarann. The garden is described as a flat wooden floor suspended on poles. |
| #29107 | The Small Gift Shop | Its room text introduces the tolerated gnome shopkeeper and his low-profit, low-price shop. |
| #29108 | The Room of Prayers | A symbol-free altar and twenty kneeling pedestals are presented as usable by any religion. |
| #29109 | The Room of Exercises | Members train between prayers and crusades in a room padded with wool blankets and scattered equipment. |

The first entrance room exits to a Duris3 source room; the other eighteen
direction records stay within this ten-room WLD map. The exact VNUMs are also
the active Torg rooms #29100–#29109, with different titles and content. The
[technical audit](../quest-story-followups/AFTERLIFE_GH_AUDIT_REVIEW.md#authored-map-and-torg-vnum-overlap)
documents both source identities and the unqualified boundary.

## Dialogue and content limits

Suse's lore explains the group's afterlife belief and describes daily life in
the house. It gives a visitor no sourced task, item handoff, faction change, or
promised return from death. The QST file is not listed in `areas/qst/AREA`; its
responses are not evidence of an active quest. The editor map
`areas/map/afterlife_gh.json` stores positions for all ten rooms and is a
separate migration asset.

The authored files contain relative phrases such as “a few months ago,”
spelling variants of the group name, and several apparent spelling and grammar
errors. Their age and present-day intended wording are not established. Keep
chronology, theology, character intent, and editorial cleanup open for the
builder rather than filling in missing events or mechanics.
