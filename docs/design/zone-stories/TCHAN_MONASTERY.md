# Tchan's Monastery of Tranquility

Priority 378, source area tchan, zone 48. This source dossier maps all ten local rooms, their environmental text and exits, the guard and monastery resets, and the existing Storm Port relationship. Tchan has one local mobile prototype and one local object prototype; the number 4800 is reused once in each type namespace. There is no local QST contract, SHP file, or story sidecar. The QST area index still lists tchan at line 23, but no areas/qst/tchan.qst file exists.

## Environmental story and room route

The WLD describes a climb from a steep valley approach through icy mountain trail to a small monastery on the summit. The repeated trail prose says that landslides and loose rocks have fallen from the path above, and that small arctic weeds line the trail. Rooms 4806-4808 add that the monastery lies to the east. Room 4809 describes a peaceful summit and identifies the monastery as Tchan's home, where the powerful monk releases burdens of the soul.

| Room | Source title | Local reading |
| --- | --- | --- |
| [#4800](../../../areas/wld/tchan.wld#L1) | The Base of An Icy Mountain Trail | Valley-side trailhead. West returns to Surface room #513140; east begins the climb. |
| [#4801](../../../areas/wld/tchan.wld#L18) | Before a Treacherous Climb on An Icy Mountain Trail | The first uphill leg. East/west textually joins the trailhead and next ascent. |
| [#4802](../../../areas/wld/tchan.wld#L35) | Climbing a Treacherous Icy Mountain Trail | Uphill to #4803 and down to #4801. |
| [#4803](../../../areas/wld/tchan.wld#L52) | Climbing A Treacherous Trail up An Icy Mountain Side | Continues the climb between #4802 and #4804. |
| [#4804](../../../areas/wld/tchan.wld#L69) | Atop a Treacherous Climb on An Icy Mountain Trail | East reaches the narrow summit trail; down returns to #4803. |
| [#4805](../../../areas/wld/tchan.wld#L86) | A Craggy, Narrow Trail To The Summit | South reaches #4806 and west returns to #4804. |
| [#4806](../../../areas/wld/tchan.wld#L103) | A Craggy, Narrow Trail To The Summit | North returns to #4805; east approaches #4807. Its prose says a monastery lies east. |
| [#4807](../../../areas/wld/tchan.wld#L121) | A Craggy, Narrow Trail To The Summit | West returns to #4806; east reaches the gate area. |
| [#4808](../../../areas/wld/tchan.wld#L139) | Beside A Small Monestary | West returns to #4807; east passes the paired monastery door. |
| [#4809](../../../areas/wld/tchan.wld#L157) | The Monestary of Tranquility | Summit sanctuary and stated home of Tchan. The only authored local exit is west through the same door. |

Rooms #4800-#4808 repeat the same broad trail description, even where the room titles distinguish a trailhead, climbs, summit path, and gate. The source also contains spelling and grammar issues, including “Rubbled,” “birm,” “withinin,” “harnassed,” and “burdons.” Keep those as builder-led copy questions; this dossier does not edit world text.

## Static route and authored boundaries

The active Tchan WLD has 10 rooms and 19 authored exits. Eighteen are local directions and one is west from #4800 to Surface #513140. Surface #513140 has an east exit back to Tchan #4800. The local graph is one weak component of 10 rooms; directed traversal from #4800 reaches all ten, and every room has an authored directed path back to the outward boundary when doors and other dynamic conditions are ignored.

The climb is described by these local links: #4800 east to #4801; #4801 up to #4802; #4802 up to #4803; #4803 up to #4804; #4804 east to #4805; #4805 south to #4806; #4806 east to #4807; #4807 east to #4808; and #4808 east to #4809. Each local pair has a reverse direction. The only exit pair with door records is #4808 east / #4809 west. Static graph continuity does not prove that a player can pass the guard, open a door, traverse safely, or return in a live session.

The room census records no fall-chance or current markers. The mountain language and sector values describe the area but do not establish a movement hazard or safe traversal result. See the [repository connectivity review](../quest-story-followups/WORLD_CONNECTIVITY.md#checkpoint-378-tchan-connectivity-and-alterian-wilderness-approaches) and [Tchan audit](../quest-story-followups/TCHAN_AUDIT_REVIEW.md).

## Residents, gate, and reset ownership

Local mobile #4800 is the tranquility guard. Its keywords include _no_move_; its description places a quiet man in lotus stance in mid-air, and its text says Tchan hired him to protect his solitude. The guard says that none shall pass. The zone header uses reset mode 2; its ZON resets this mobile in room #4808.

The maintained assignment table binds mobile #4800 to newbie_guard_east ([assignment](../../../src/specs/specs.assign.c#L600)). That procedure handles eastward movement and checks level and trusted status; it is an additional source-defined gate condition, not a result from live play ([procedure](../../../src/specs/specs.newbie_guards.c#L62)). The source story and code deserve a builder review together: the guard stands beside the eastbound monastery door and has a level-sensitive special.

The WLD gives both directions of the #4808/#4809 door a key VNUM of 4800. The active ZON has paired D resets at 100 percent with state 1, described in its comments as closed. The local object #4800 is a tiny relic-shaped key, but the current Tchan ZON has no E or G reset for that object. Git history shows the old E reset was removed in 2010 and the door state changed from closed-and-locked to closed. The remaining WLD key fields do not establish that the door is currently locked or that this key is available.

Tchan is not defined in tchan.mob. The Tchan ZON imports mobile #22428 into room #4809; its definition is the Storm Port high priest ([mobile](../../../areas/mob/stormport.mob#L410)). Compiled code binds mobile #22428 to clear_epic_task_spec ([assignment](../../../src/specs/specs.assign.c#L194)). The same mobile owns Storm Port's native request and is connected to the separate Storm Port story-sidecar contact ([native QST](../../../areas/qst/stormport.qst#L1), [sidecar contact](../../../areas/story/stormport.story.json#L23), [sidecar contract](../../../areas/story/stormport.story.json#L531), [Storm Port dossier](STORMPORT.md)). The Storm Port mobile text at [its definition](../../../areas/mob/stormport.mob#L410) calls Tchan a high priest and says he runs a plateau monastery. The four-proof mantle exchange remains a Storm Port contract, not a local Tchan contract. Preserve the distinction between the physical monastery reset at #4809 and the request's Storm Port ownership and catalog context. Tchan has no local contract to duplicate.

## Cross-area monastery names and character references

Storm Port also retains its own rooms #22519, “Before a Small Monestary,” and
#22520, “Inside a Small Monestary,” with a paired closed door reset. The room
prose describes a local Orcish high cleric, but Storm Port's ZON has no M reset
whose mobile field is #22428. Its #22428 occurrences use other fields: room
#22428 is the destination for mobile #22409, and object #22428 is equipped by
an E reset. The Tchan area's [M reset](../../../areas/zon/tchan.zon#L19) places
Storm Port's mobile #22428 at summit room #4809. The Storm Port sidecar says
Tchan is currently stocked in this separate Monestary of Tranquility and keeps
his native request with Storm Port ([location note](../../../areas/story/stormport.story.json#L10),
[contact](../../../areas/story/stormport.story.json#L23)). Keep the two map
identities and the older Storm Port room prose distinct; the text alone does
not show that Tchan is currently stocked in those city rooms.

Ship Yards mobile #43163, Ayden Stoneheart, calls himself Tchan's last surviving
disciple and says Tchan “has long since been lost” ([mobile](../../../areas/mob/shipy.mob#L958)).
That unqualified character-history line conflicts with the current Tchan ZON
reset and Storm Port sidecar location statement. The sources do not date
Ayden's account or explain the difference. Preserve both and ask the builders
to reconcile the timeline; do not infer Tchan's death, absence, or a second
monastery from the dialogue.

## Source identity and history

The title's color tokens normalize to “The Monestary of Tranquility,” unique among the 442 current ZON titles. The current active contributor is listed at [areas/AREA line 24](../../../areas/AREA#L24). It remains DO NOT MOVE.

The followed WLD history begins with commit 79c3c3a40 on 2010-05-19. That change added the icy mountain group and describes Tchan moving to Ice Crag; it is evidence of a character-location change, not whole-zone replacement. Commit fb93dbbb5 on 2010-05-20 added tchan to the active area list, connected the trailhead to Surface room #513140, and adjusted the guard. On 2010-05-20, commit 1de5d66e6 temporarily set the zone not to reset to avoid key/door “cheese”; commit 14dca2801 reverted that setting. Commit 98b714b7d on 2010-05-21 removed the key reset, changed both door resets from state 2 to state 1, and described the guard as blocking high-level players. Later tracked changes update source fields and flags; no alternate-stem rename, explicit predecessor, or whole-zone replacement appears in these followed paths. The tracked history is summarized in the [Tchan audit](../quest-story-followups/TCHAN_AUDIT_REVIEW.md#title-same-type-identity-and-followed-history).

## Scope

This is a static source map. It does not claim live room availability, successful passage past the guard, safe mountain travel, door interaction results, actual item supply, quest completion, or persistence. No gameplay or native source change is made here.
