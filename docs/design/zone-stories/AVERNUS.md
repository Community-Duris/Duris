# The Plane of Avernus (`avernus`) source story dossier

This dossier reviews the active `avernus` source area's environment, inhabitants,
items, reset roles, and authored story clues. The [Checkpoint 192 audit
supplement](../quest-story-followups/AVERNUS_AUDIT_REVIEW.md) records its full
WLD graph, type-25 travel, typed cross-area references, title/history review,
and retention disposition. Source prose and executable source are evidence of
design intent; they do not prove a live visit, an available reset item, a safe
journey, or a completed task.

## A hellscape with several inhabited pockets

The broad wasteland is the largest region. Its 239 rooms share text describing
an unchanging blood-red light, jagged foothills, exposed obsidian and quartz,
and caves occupied by devils ([representative wasteland room
#32263](../../../areas/wld/avernus.wld#L6811)). Another 44 rooms describe
Darkspine's outskirts and 25 its ruins. The source calls it a former Outlands
gate town pulled into Hell; its inhabitants fled or were slain, while
scavengers and baatezu guards remain
([outskirts](../../../areas/wld/avernus.wld#L3780), [ruins](../../../areas/wld/avernus.wld#L4383)).

The Maggot Pit occupies 27 rooms. Its prose describes the place where new
lemures and nupperibos are born, watched by red abishai, and says the pit may
block the Great Cavern Azharul and Tiamat's lair
([Maggot Pit](../../../areas/wld/avernus.wld#L289)). At #32031, the prose names
the cavern as the entrance to Tiamat's lair and says she guards the only known
entrance to Dis. The room has an authored north exit to Tiamat #19600
([cavern entrance](../../../areas/wld/avernus.wld#L421)). These statements are
the source's account; they do not establish how the destination should be
interpreted if the separate Tiamat and Dis descriptions seem inconsistent.

The River Styx's 28 rooms use one description of boiling blood and infernal
reflections; seven adjoining “River of Blood” rooms record a current speed of
50 toward the north. The “Falls of Blood and Death” and a Lake of Blood provide named local
landmarks ([Styx](../../../areas/wld/avernus.wld#L1545), [current-bearing
river](../../../areas/wld/avernus.wld#L2383), [falls](../../../areas/wld/avernus.wld#L1802),
[lake](../../../areas/wld/avernus.wld#L6450)). The prose suggests a dangerous
landscape, while the current fields add a separate movement hazard.

The Village of Draukari spans seven rooms. Its kobolds tolerate fewer fireballs
there and appear to perform work assigned by the baatezu
([village](../../../areas/wld/avernus.wld#L271), [kobold
mobile](../../../areas/mob/avernus.mob#L234)). Eleven rooms identify the
Stigmaris Mountains, whose exposed rock seeps blood and other bodily fluids
([mountains](../../../areas/wld/avernus.wld#L6424)).

Seven rooms form the Peaceable Lands under Bargrivyek. The prose describes
leveled land and holy-water springs, excludes Baator's devils, and says the god
sometimes helps lost or hurt travelers back home
([Peaceable Lands](../../../areas/wld/avernus.wld#L9651)). This is an explicit
claim in the room text. The reviewed source does not tie it to a local quest,
room callback, or specified destination, so this dossier does not count it as a
verified rescue or safe-return service.

Five rooms describe the bronze wall around the Citadel as intensely hot and too
high to fly over; the source calls the Citadel the plane's only building
([wall](../../../areas/wld/avernus.wld#L534), [Citadel gate
approach](../../../areas/wld/avernus.wld#L913)). An active teleport item at the
approach leads to the Bronze Citadel in `bctdl`; its paired item provides the
reverse destination. Their reset and travel conditions are recorded in the
[audit supplement](../quest-story-followups/AVERNUS_AUDIT_REVIEW.md#typed-travel-and-world-connectivity).

## The devils and their objects

The 25 mobile prototypes range from lemures, nupperibos, spinagons, hezrou,
imps, kobolds, and goblins to named devils. Amduscias and Malphas serve Tiamat
and lead companies of abishai; Nergal is a banished devilkin wrapped in magical
darkness; Bist is a mistrusted hellcat confined to Avernus; Azazel is described
as a former Prince of Hell; and Dagon is an Abyssal Lord
([mobile prototypes](../../../areas/mob/avernus.mob#L33),
[Nergal](../../../areas/mob/avernus.mob#L66), [Bist](../../../areas/mob/avernus.mob#L84),
[Azazel](../../../areas/mob/avernus.mob#L133), [Dagon](../../../areas/mob/avernus.mob#L169)).
The ZON file places these named figures and their followers in the Citadel
approach and other local rooms. The reset source imports additional devil
mobile prototypes from `river_styx` and `hell_one_avernus`; this is a typed
source dependency, not evidence that those mobs currently spawn.

The 33 objects include the jeweled belt, Nergal's wing, Bist's claw, Caim's
smoke boots and band, several devil weapons, a platinum dragon egg, and the
Nessus royal guard's platemail. Story-bearing objects include the frozen-Styx
water earring (#32022), Darkspine royalty necklace (#32021), Dagon's bloody
eyeball (#32031), and the Citadel teleport item (#32013)
([object prototypes](../../../areas/obj/avernus.obj#L145),
[Citadel teleport item](../../../areas/obj/avernus.obj#L157),
[Dagon's eyeball](../../../areas/obj/avernus.obj#L363)). The local shop record
belongs to Armaros (#32008) in room #32279 and lists the purple vial, thick red
potion, and Darkspine necklace as stock ([shop record](../../../areas/shp/avernus.shp#L1),
[Armaros reset](../../../areas/zon/avernus.zon#L236)). A stock definition alone
does not prove a completed purchase or delivery.

Two item callbacks are assigned in source. The “staff of sinister tactics”
#32001 applies its fear effect through `sinister_tactics_staff`; the “staff of
shadow summoning” #32008 can create a shadow follower from mobile #92076, whose
prototype is owned by `undermountain`. Its exhausted-use branch can instead
create mobile #200 from `heavens`. This behavior is compiled source, not a local QST contract
([callback bindings](../../../src/specs/specs.assign.c#L1441), [Avernus
staff procedures](../../../src/specs/specs.avernus.c#L19)). The audit supplement
records those cross-area code references separately from ordinary resets.

## Cross-area item uses

`avernus` has no local `.qst` file, zero native Q/QA contracts, and no story
sidecar. Several active areas nevertheless refer to its objects:

- `bctdl.qst` and `bctdl.story.json` use the Nessus royal guard's platemail
  #32029 in Zariel's reward exchange.
- `heavens.qst` has four `G I #32011` actions for the Avernus basalt scimitar
  in its Joukarth reagent story.
- `jade.qst` and `trakkia.qst` each have exact QST actions involving the purple
  liquid #32019; their sidecars preserve the corresponding exchange records.
- `wh.qst` and `wh.story.json` use Dagon's bloody eyeball #32031 in Adryv's
  delivery request.
- `tiamat.zon` has three reset commands that load Avernus object #32019.

The exact action types, source lines, and related object ownership are in the
[typed dependency table](../quest-story-followups/AVERNUS_AUDIT_REVIEW.md#typed-cross-area-references).
These references belong to their source areas' stories and do not create a
local Avernus quest.

## Builder questions and source resolutions

The reviewed files do not record a builder resolution for the questions below.
Keep the current source and ownership relationships intact until their owners
decide what the intended outcomes are.

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Does the Peaceable Lands prose promise an actual rescue or safe route? | The seven-room area says its residents sometimes help lost or hurt travelers home, but no local contract or persistent rescue result is recorded. | If rescue is intended, define the eligible traveler, arrival destination, return path, and saved outcome. Keep prose distinct from a completed rescue. |
| Are blank rooms #32008–#32023 and #32000 playable areas, private staging rooms, or unused records? | Sixteen contiguous rooms and one isolated room have no authored descriptions; their ownership and ordinary approach are not established by the static map. | Identify their owner and safe entry/exit before presenting them as normal travel destinations. Preserve blank records until that intent is clear. |
| Should the six “random mob load” rooms be reachable from the Tiamat boundary? | Their room titles and notices read like diagnostics, while Avernus has a Tiamat boundary and reset-loaded content. | Confirm whether these are diagnostics, staging, or player areas before changing the route. Do not infer ordinary access from adjacency or labels. |
| At #32031, should the cavern be described as both an entrance to Tiamat's lair and the only known entrance to Dis? | Its prose connects those claims, but the source review does not establish whether they describe one route or separate lore. | Ask the owner to distinguish the destinations and intended route. Correct only the wording selected by the owner; preserve topology until separately reviewed. |
| Are the `bctdl`, `ravenloft2`, and compiled Hell-gate arrivals intended player routes? | The area has a type-25 pair into Bctdl, six Ravenloft2 arrivals, and a conditional compiled Hell-gate route. | Confirm admission, target safety, and return expectations separately for each route. Keep source declarations distinct from proven live travel. |
| Do the external quest uses of #32011, #32019, #32029, and #32031 remain intended, and is Armaros's shop stock a supported source for any required item? | Heavens, Jade, Trakkia, White Harbor, Bctdl, and Tiamat contain exact external consumers or resets; Armaros has a local shop record. | Confirm each item's owner, intended consumer, and allowed source. Do not collapse a shop purchase or foreign quest into a local Avernus objective. |
| What helper and exhausted-charge behavior should the staff of shadow summoning have? | The compiled callback references Undermountain mobile #92076 and Heavens fallback #200; its exact effective helper path remains source-dependent. | Keep the current callback and charges unchanged until intended helper selection and depletion behavior are specified and qualified. |
| Should `avernus` have a command-target alias beyond its normalized title “The Plane of Avernus”? | The color-normalized title is unique among active contributors and repeats only on unregistered `plane_avernus`; no additional alias is established in the bounded area sources. | Keep `avernus` and “The Plane of Avernus” as the source key and display identity. Add an alias only after the owner names its exact target and intended distinction. |
