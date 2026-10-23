# The High Moor Forest

This dossier covers active contributor `highmoor` at source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. Its [audit supplement](../quest-story-followups/HIGHMOOR_AUDIT_REVIEW.md)
records graph, source-only, code, and history evidence.

## Environmental story

The 258 rooms form several distinct environments rather than one repeated
forest description. The most common room title is “Lost in the High Moor
Forest” (102 rooms); the Serpentine River contributes 35, followed by
water-filled tunnels, the Serpent’s Bridge, and the Boring Beetle lair
([WLD census](../quest-story-followups/HIGHMOOR_AUDIT_REVIEW.md#static-connectivity-and-hazards)).
The writing describes a forest that disorients travelers, with overgrown
paths, a river system, underground channels, a waterfall, a decaying barge,
and a primitive dock whose siege equipment hints at conflict between its
inhabitants.

The Blueleaf tree gives the forest a creation story: local rumor says
Goldrym first fell in love with nature there, after which evil began to
plague the deeper woods ([room #19102](../../../areas/wld/highmoor.wld#L46)).
The tree is reset into that room as object #19101; its carved initials
“G.G.” and a burned eagle-and-ape image add an unexplained clue
([object](../../../areas/obj/highmoor.obj#L19), [reset](../../../areas/zon/highmoor.zon#L201)).
Two books reset in Yingin’s hut develop the thread. A note signed Boriaul,
identified as a Second Ranger Knight to the “Burgendy Crown” (source spelling), blames Goldrym’s
experiments for upsetting nature and sets out for the High Moor Ruins to defeat him. A
blood-stained journal distrusts Korgakki and sketches the Serpent Hills,
WyrmWood, High Moor Ruins, Osquip Camp, Grove of Sadness, Bloodleaf clan, and
Yingin ([Boriaul’s note](../../../areas/obj/highmoor.obj#L94),
[journal and map](../../../areas/obj/highmoor.obj#L120),
[book reset](../../../areas/zon/highmoor.zon#L203),
[journal reset](../../../areas/zon/highmoor.zon#L204)). The notes invite a
larger story, but the checked-in sources contain no acceptance or outcome.
Yingin, the Totemic Druid, is a caretaker for afflicted forest creatures.
Yingin’s lost owl Oaka is also associated with Goldrym in local rumor
([mobile](../../../areas/mob/highmoor.mob#L266), [infirmary](../../../areas/wld/highmoor.wld#L141)).
A dark driftwood-and-bone shrine holds a single Blueleaf leaf, another
possible connection to the same lore ([shrine](../../../areas/wld/highmoor.wld#L721),
[shrine object](../../../areas/obj/highmoor.obj#L188)). The sources suggest
a relationship among Goldrym, the forest’s sickness, and Yingin’s missing
owl, but no local quest record resolves it.

Dr. Fester’s old research barge turns the theme of harm to nature into body
horror. The laboratory and breeding room contain broken experiment notes,
cages, and signs of experiments that survived; chambers preserved for more
than fifty years suggest that Fester or his patrons expect the research to
resume ([lab](../../../areas/wld/highmoor.wld#L4720),
[breeding room](../../../areas/wld/highmoor.wld#L4770),
[chambers](../../../areas/wld/highmoor.wld#L4793)). The source calls Fester a
chief research scientist working for Goldrym who still seeks to combine
animal traits into a superior creature. Resets place experimental flesh and
nerve creatures in his chambers and his deformed son in the cramped attic
above ([Dr. Fester](../../../areas/mob/highmoor.mob#L693),
[lab resets](../../../areas/zon/highmoor.zon#L398),
[son reset](../../../areas/zon/highmoor.zon#L406),
[attic room](../../../areas/wld/highmoor.wld#L4936)). Kirus the species
collector is a related but separately presented tragedy: his text says he
drowned while trying to capture an animal for research, after local wildlife
had already lost family to his collecting
([Kirus](../../../areas/mob/highmoor.mob#L800)). These sources establish a
strong research-abuse theme, not a demonstrated player campaign.

Other pockets read as independent encounters or unfinished expansion
points:

- A Serpent’s Bridge crossing carries an account of a black dragon from
  WyrmWood damaging the granite bridge during the Cryain Wars
  ([bridge](../../../areas/wld/highmoor.wld#L478)).
- A clearing before the Ik-Nah Bandits’ guild tells a rumor that they stole
  a dragon egg from Skurge of WyrmWood. The adjoining guild room still says
  the zone will arrive later ([approach](../../../areas/wld/highmoor.wld#L6089),
  [placeholder](../../../areas/wld/highmoor.wld#L6111)).
- The Bloodleaf lizardmen and Osquip raiders have local mobiles and resets,
  while the proposed Lizardman Settlement and Osquip Camp rooms remain
  explicit placeholders ([lizardman approach](../../../areas/wld/highmoor.wld#L3940),
  [Lizardman placeholder](../../../areas/wld/highmoor.wld#L3961),
  [Osquip gate](../../../areas/wld/highmoor.wld#L4950),
  [Osquip placeholder](../../../areas/wld/highmoor.wld#L5001),
  [eastern placeholder](../../../areas/wld/highmoor.wld#L5297)).
- Boring beetle prose describes fast-moving tunnels that may collapse, a
  predator nest used to defend the colony, and a queen’s chamber full of
  eggs and larvae. A separate opening is explicitly called the colony’s
  emergency lair exit ([lair entrance](../../../areas/wld/highmoor.wld#L6158),
  [predator nest](../../../areas/wld/highmoor.wld#L6231),
  [emergency exit](../../../areas/wld/highmoor.wld#L6348),
  [queen’s chamber](../../../areas/wld/highmoor.wld#L6653)).
- The lower river tunnels include both water-filled passages and rooms titled
  “Underwater Lava Spout” and “Deadly Burning Hot Lava Spout,” another
  environmental hazard whose traversal behavior is not established by the
  prose ([water/lava passage](../../../areas/wld/highmoor.wld#L5607)).
- A sandy riverbank and deep pocket of wind-scoured sand give way to two
  named sandstorms, Ivyn and Muk, among the local river and shoreline
  creatures ([sand pocket](../../../areas/wld/highmoor.wld#L4278),
  [Ivyn](../../../areas/mob/highmoor.mob#L609),
  [Muk](../../../areas/mob/highmoor.mob#L627)).

## Quest and service boundary

`highmoor` is listed in both area rosters, but its QST file contains only
three generic `qc_action` responses from Dr. Fester. The coverage register
records zero native Q/QA contracts, and there is no local SHP file or story
sidecar ([rosters](../../../areas/AREA#L96),
[QST roster](../../../areas/qst/AREA#L89), [QST source](../../../areas/qst/highmoor.qst#L1)).
This is not evidence of a local objective, completion condition, or reward.

Dr. Fester is separately registered in the compiled epic-teacher table as a
teacher of Anatomy ([teacher table](../../../src/classes/epic_skills.c#L172)).
That code-backed service belongs in the dependency record even though no
quest contract accompanies it.

The local reset file places 64 of 65 mobile prototypes and 61 of 62 object
prototypes. The unused jermlaine gremlin #19115 and rock-encrusted emerald
#19149 remain defined but have no local reset placement. Their intended
status is not explained by the source.

## Builder questions and recommended resolutions

| Builder question | Recommended resolution |
| --- | --- |
| Are the Goldrym clues one story: the Blueleaf tree and shrine, Yingin’s missing owl Oaka, Boriaul’s note, and the blood-stained map to the High Moor Ruins and nearby groups? | Confirm which clues connect and whether the High Moor Ruins are a planned player destination. Preserve the existing clues as unresolved lore until an objective and outcome are authored. |
| Is Dr. Fester’s experiment and Kirus’s species collecting one research story, or two environmental vignettes? | Confirm their intended relationship. Preserve Dr. Fester’s Anatomy teaching service; if either story is meant to be a quest, add a separately reviewed acceptance, completion, and reward contract. |
| Are the Lizardman Settlement, Osquip Camp, and Ik-Nah guild placeholders still planned? | Confirm each stub’s intended status and owner. Preserve the existing rooms and reset references until any expansion has its own source, boundary, and quest review. |
| Should the three “forest continues endlessly” exits from #19327 remain non-traversable? | The negative VNUMs are dropped by the loader. Confirm a real boundary destination or an intentional terminus before changing those targets; do not infer a neighboring room from the descriptions alone. |
| Are rooms #19352–#19357 intentional load-result or dispersion rooms? | Confirm their reset purpose and whether the isolated prototypes are expected. Keep the six room references intact until the reset owner confirms the design; do not expose them as ordinary routes based only on their outgoing exits. |
| Do the 48 river current records and the two 40% waterfall fall chances match the intended movement and risk? | Review the current direction and fall behavior together with the river exits, door resets, and return routes. The static source census does not establish traversal safety. |
| Is the Ixarkon shimmering veil meant to send travelers to the Stuffy Attic #19275? | Confirm the compiled destination and the route back through the closed, secret ceiling-tile exit to Dr. Fester’s chambers. Preserve the object and code link until an owner verifies the intended arrival and return. |
| Is the inactive Surf approach into #19100 retained historical map data or a planned route? | Keep it in Surf’s existing candidate record as source-only. Do not treat it as active ingress or move the active Highmoor contributor. |
| Are the unused jermlaine gremlin #19115 and rock-encrusted emerald #19149 retired prototypes or intentional future content? | Confirm their owner and intended use before adding reset commands. Preserve both VNUMs and their current no-load state until that review. |
| Which prose corrections preserve the intended names and lore? | Review the visible spelling and grammar errors in a separate text pass after builders confirm names such as Goldrym, Boriaul, Korgakki, Oaka, Ik-Nah, and Skurge. |

## Connectivity and retention boundary

Highmoor has one reciprocal active Surface boundary at #19100/#526069, plus
one inactive Surf WLD approach to #19100. Its 258-room source graph has three
weak components; directed reach from #19100 covers 252 rooms, and 255 rooms
can return to that seed. These counts ignore reset door state and do not
establish safe play. Forty-eight rooms carry current records, and the two
40% fall fields are at waterfall rooms #19271 and #19272. An Ixarkon code
destination also lists room #19275, which is not represented by a WLD edge.
See the [audit supplement](../quest-story-followups/HIGHMOOR_AUDIT_REVIEW.md)
for the exact references and open route questions.

Keep active `highmoor` **DO NOT MOVE**. It is not an attic candidate. Preserve
the separate Surf source-only approach, Brass-old-1’s source-only reset of
object #19127, and the active Ixarkon code path until each owner resolves its
own source or route question.
