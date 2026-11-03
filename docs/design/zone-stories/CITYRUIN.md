# Cityruin: the ruined Tharnadian quarter

Cityruin is an active full-world contributor with zone header #662, 155 rooms
(#66200–#66354), 42 mobile prototypes, and 37 object prototypes. The source
list's legacy range note calls it “kharne”; the authored `.zon` title is “The
Ruins of Tharnadia's Old Quarter” and is unique among all 442 current zone
titles ([area list](../../../areas/AREA#L249), [zone header](../../../areas/zon/cityruin.zon#L1)).

## Story and quest boundary

The zone depicts a once-populated quarter whose streets, homes, meeting hall,
armoury, and guild halls are now occupied by undead and animals. A skeletal
necromancer is named as the maker of the undead. The descriptions add a failed
civilian shelter, imprisoned residents, looted homes, a rich jeweller's safe,
and an old drainage system that once carried water toward a river. The sources
do not settle a single cause or timeline for the city's ruin. See the
[detailed source-story review](../quest-story-followups/CITYRUIN_STORY_REVIEW.md)
for room, mobile, object, and reset evidence.

Cityruin has two local QST giver prototypes with message records but no
`Q`/`QA` completion contract, no local shop source, and no local story sidecar.
The mage guildmaster's timed message and the elite guard's `noteforimm` topic
remain source-defined dialogue. Separately, active Tharnadian Ruin QST and
story-sidecar records use Cityruin remains #66201, #66225, #66226, #66227, and
#66230 as offered items and reference Cityruin key #66200 as a reward. That
cross-zone quest relationship is documented in the [repository-wide audit
supplement](../quest-story-followups/CITYRUIN_AUDIT_REVIEW.md).

## Direct area-owner questions and conservative recommendations

These questions preserve Cityruin as a separate active area and keep the
Tharnadian Ruin item contract and compiled room bindings under their existing
owners.

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Is `kharne` an intended command-target alias for `cityruin`, or only a legacy note? | The active title “The Ruins of Tharnadia's Old Quarter” is unique among 442 sources; the `areas/AREA` comment contains `kharne`, but the bounded source search found no compiled alias or matching title. | Keep `cityruin` and the authored title as separate identifiers. Add a `kharne` alias only if the owner names its exact destination and use. |
| Should the two local QST message records remain informational, without a Cityruin completion contract or story card? | The QST file has two message records but no Q/QA contract; Cityruin has no story sidecar. Neighboring Tharnadian Ruin owns the active item exchanges that consume Cityruin prototypes. | Keep message and item ownership distinct. Add a Cityruin objective only after the owner selects its accepted outcome and authorizes a separate contract. |
| Should the entry gates reset closed when their descriptions call them useless or easily passable? | The entry gate resets closed but unlocked; the Cityruin-to-armoury door has a different asymmetric reset. | Preserve current door states until the owner identifies the intended route and balance. Treat descriptive phrases as clues, not proof of a broken reset. |
| Is the guard's `noteforimm` message intended for players, and should its epic-point tuning note remain in the QST text? | The elite guard has a message record with that topic and an epic tuning note; no completion contract or separate learning effect uses it. | Keep the current message and avoid inventing an epic-point reward. Make any selected text-only correction separately after its player-facing intent is confirmed. |
| Should the four remains descriptions identify one shared necromancer victim while Tharnadian Ruin distinguishes four item prototypes? | Extra descriptions use similar remains wording; the neighboring quest accepts distinct object VNUMs for its recipes. | Preserve the four item identities and cross-zone recipe. Clarify prose only if the builder confirms that a wording edit improves the intended clue. |
| Is mobile #66239 intended to sell ingredients without a reset or shop record, and is zombie noble #66208 the intended money changer? | No local shop source exists; #66208 is reset and has the compiled `money_changer` binding, while #66239 lacks a documented local sale source. | Preserve the money-changer binding and current lack of local stock evidence. Do not promise a sale or add shop stock until the intended seller, products, and accounting behavior are selected. |
| What room should the compiled `undead_inn` procedure use? | The procedure is assigned to #66355, beyond Cityruin's active room range ending at #66354. | Keep the literal assignment unchanged until its intended room owner is confirmed. Do not silently redirect it to a nearby room. |
| How should the ruined quarter relate to Tharnadian Ruin and the separately titled Tharnadia city zone? | Cityruin has a reciprocal boundary to Tharnadian Ruin, a distinct title, separate tracked history, and no evidence of whole-zone replacement. | Preserve all three identities and the existing boundary. Link them narratively only where the owner confirms chronology and place; do not merge their contracts or aliases. |

## Current working resolution

Keep Cityruin as a separate active area and retain its current source paths,
room and prototype VNUMs, paired connection to Tharnadian Ruin, and the
cross-zone quest items used by Tharnadian Ruin. Preserve the source-only Surf
approach and all compiled room/mobile bindings. Treat the reset-open graph as
a description of exits initially open after zone resets; it does not settle
whether a character can open, pick, find, or safely traverse the other doors.
Resolve the questions above with the owner before any separate gameplay or
retirement proposal. No source move or gameplay change is proposed.
