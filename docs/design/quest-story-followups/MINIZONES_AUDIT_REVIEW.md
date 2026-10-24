# Mini Zones audit review (Checkpoint 278)

This review reconciles the registered Mini Zones source with the existing
[comprehensive story dossier](../zone-stories/MINI_ZONES.md) and
[generated source index](../../reference/zone-story-audits/minizones.md). The
source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or
gameplay source was changed.

## Identity and source inventory

`area:minizones` is the active, full-list contributor at canonical zone 57
([active manifest](../../../areas/AREA#L29)). It owns 300 rooms, 125 mobiles,
124 objects, seven native Q/QA contracts, and five shops. The coverage row's
shop fields were stale: `minizones.shp` has keeper records #5713, #5715, #5781,
#5782, and #5812. The corrected range is #5713–#5812 with five records.

The comprehensive dossier already describes the seven native exchanges, 548
reset declarations, six literal special assignments, supported handlers,
independent city outcomes, four paid armor services, and unresolved repair
decisions. Its source census is static evidence; reset admission, actual item
generation, payment settlement, callable procedure state, and player travel
remain unqualified.

## Active and source-only connectivity

The active WLD census has 695 source exit records; 686 resolve in the loaded
world and nine point to rooms found only in unregistered WLD sources. The map
has 15 weak components, the largest with 69 rooms, 15 rooms with fall chances,
and no currents. Fourteen active exits enter Mini Zones and fourteen leave it;
twelve have reverse-direction records. The active one-way inbounds are from
Heavens and `wh`; the active one-way outbounds are to Mist and Valley of
Crushk. The other twelve inbound/outbound records form reciprocal boundaries.

The all-95 inactive-WLD sweep found eleven non-overlapping `surf.wld` exits
into Mini Zones. Their source rooms are outside Mini Zones' 5700–5999 room
range, so these are distinct source identities. They remain source-only and
are not active ingress:

| Surf source exit | Mini Zones target | Source |
| --- | ---: | --- |
| #209375 south | #5700 | [surf.wld](../../../areas/wld/surf.wld#L196759) |
| #212971 north | #5734 | [surf.wld](../../../areas/wld/surf.wld#L272191) |
| #214153 south | #5735 | [surf.wld](../../../areas/wld/surf.wld#L297005) |
| #218346 north | #5756 | [surf.wld](../../../areas/wld/surf.wld#L384906) |
| #226133 north | #5830 | [surf.wld](../../../areas/wld/surf.wld#L548193) |
| #234326 south | #5770 | [surf.wld](../../../areas/wld/surf.wld#L720130) |
| #234926 north | #5785 | [surf.wld](../../../areas/wld/surf.wld#L732714) |
| #252703 east | #5795 | [surf.wld](../../../areas/wld/surf.wld#L1105887) |
| #272622 south | #5847 | [surf.wld](../../../areas/wld/surf.wld#L1523942) |
| #278859 east | #5868 | [surf.wld](../../../areas/wld/surf.wld#L1654791) |
| #279856 south | #5819 | [surf.wld](../../../areas/wld/surf.wld#L1675700) |

These eleven records raise the curated source-only total to 3,775 overall and
the reviewed Surf subset to 247. `ud_map2` remains 18 and Underworld2 remains
eight. The nine unresolved Mini Zones exits and eleven inactive Surf approaches
are different directions and records.

## Bounded typed-source closure

The local source and the bounded foreign records expose several prototype
dependencies that matter to any later ownership or migration review:

| Source | Typed reference to Mini Zones | Status |
| --- | --- | --- |
| `alatorin.zon` | Mobile #5717; object #5823 in one P and two G resets | Active source; see [reset records](../../../areas/zon/alatorin.zon#L1756) and [mobile record](../../../areas/zon/alatorin.zon#L2816). |
| `barovia2.zon` | Object #5722 in an O reset | Active source; [reset record](../../../areas/zon/barovia2.zon#L97). |
| `bs.zon` and `bs.shp` | Object #5743 in a G reset and shop stock | Active source; [reset](../../../areas/zon/bs.zon#L1923) and [stock](../../../areas/shp/bs.shp#L1079). |
| `player_castles.shp` | Object #5725 in shop stock | Active source; [stock](../../../areas/shp/player_castles.shp#L11). |
| `surfacekeeps.zon` and `.shp` | Object #5735 in a G reset and shop stock | Active source; [reset](../../../areas/zon/surfacekeeps.zon#L606) and [stock](../../../areas/shp/surfacekeeps.shp#L165). |
| `surf.zon` | Mobile #5714 and object #5720 | Unregistered source-only resets; [records](../../../areas/zon/surf.zon#L63). Preserve with Surf candidate row 97. |
| `player_guilds.shp` | Nine shop-stock lines name numeric object #5725 | The `player_guilds` component is not in the full area list; retain the [source shop file](../../../areas/shp/player_guilds.shp) as source-only evidence. |
| `surfacemini.story.json` | Contact entry uses mobile #5715 | [Story sidecar](../../../areas/story/surfacemini.story.json#L221) reference only, not runtime proof. |

No foreign QST giver or QST I-item reference into VNUMs 5700–5999 was found
in the bounded source scan. The seven local Q/QA contracts use the external
shared object #500025 for the four armor recipes; the dossier traces 24
Surface Realm ground reset declarations for that prototype. This records
source references, not admitted resets, loaded stock, or item availability.
The six local literal assignments and relevant procedure call paths remain
listed in the source dossier and generated index; installed behavior is not
certified here.

## Title and bounded history

After color-token and punctuation normalization, `Mini Zones` is the unique
exact title among all 442 scanned `.zon` files. `The Minizones of the Surface`
and `The Minizones of the Underdark` are distinct titles and stems. Followed
`minizones.wld` and `minizones.zon` histories reach the 2009 area-tree move and
continue through same-stem edits; no alternate-stem rename, distinct
predecessor, or replacement was established in those bounded paths. This does
not prove that an untracked predecessor never existed.

## Migration disposition

Keep active `minizones` **DO NOT MOVE**. Preserve its active reciprocal and
one-way boundaries, all eleven Surf source-only approaches on candidate row 97,
the nine unresolved exit records, and the typed prototype/shop references above.
No candidate row is added. The corrected shop inventory and source-only route
count do not qualify runtime loading or migration safety.

## Checkpoint 278: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should the legacy `waittress` keyword remain the only addressed dishwasher topic, or should a correctly spelled alias also be accepted? | [QST response](../../../areas/qst/minizones.qst#L2), [dialogue index](../../reference/zone-story-audits/minizones.md#dialogue-responses) | Preserve the current alias unless an explicit compatibility change is selected; aliases should not create extra story credit. |
| 2 | Should jar delivery require only exact object #5750 and continue to return the clue without an item, money, or XP reward? | [native request](../../../areas/qst/minizones.qst#L7), [story review](../zone-stories/MINI_ZONES.md#independent-progression-stories) | Keep the native item-only contract; track the jar's source and actor custody separately if provenance is wanted. |
| 3 | Should saying `cutpurse`, unlocking the door, opening it, and crossing into the guild remain separate outcomes? | [access behavior](../zone-stories/MINI_ZONES.md#access-custom-behavior-and-quest-like-lore), [Mini Zones WLD](../../../areas/wld/minizones.wld) | Credit only the event that actually happened; do not infer travel or guild membership from speech. |
| 4 | Should the separate generated world-quest service remain assigned to potash miner #5755 rather than the nearby elderly bartender? | [literal binding](../../../src/specs/specs.assign.c#L774), [source dossier](../zone-stories/MINI_ZONES.md#access-custom-behavior-and-quest-like-lore) | Preserve the current binding and keep generated tasks outside the seven static contracts. |
| 5 | Should the tip request, knight release, and sword restoration remain three independent outcomes, with four armor recipes treated only as services? | [native contracts](../../reference/zone-story-audits/minizones.md#native-exchanges), [story flow](../zone-stories/MINI_ZONES.md#independent-progression-stories) | Keep their existing separation; add a combined episode only after its required sequence and failure cases are approved. |
| 6 | Should the knight exchange consume parchment #5804, provide strip #5794, and retire that current appearance as authored? | [knight QST](../../../areas/qst/minizones.qst#L46), [native exchange index](../../reference/zone-story-audits/minizones.md#native-exchanges) | Preserve the exact exchange while separating NPC generation and actual parchment custody from the receipt. |
| 7 | Should the Gaffin and king references remain lore, or should they lead to a newly designed encounter or resolution? | [city dialogue](../../../areas/qst/minizones.qst#L19), [story limits](../zone-stories/MINI_ZONES.md#independent-progression-stories) | Keep them as lore until a real endpoint, reward, and safe return are designed. |
| 8 | Should Maze progress require actual arrival and exit movement rather than reading or repeating the route directions? | [Maze route review](../zone-stories/MINI_ZONES.md#independent-progression-stories), [room source](../../../areas/wld/minizones.wld) | Keep guidance separate from movement credit; record the verified room transition and actor. |
| 9 | Should the letter result require opening the revenant's satchel and taking the parchment, rather than searching or visiting the cache? | [reset and container review](../zone-stories/MINI_ZONES.md#independent-progression-stories), [Mini Zones resets](../../../areas/zon/minizones.zon) | Require actual custody only for a source-history result; preserve supplied parchment eligibility for the native exchange. |
| 10 | Should the sword contract continue to accept supplied exact pieces without requiring a prior knight receipt or producer kill? | [sword QST](../../../areas/qst/minizones.qst#L66), [story review](../zone-stories/MINI_ZONES.md#independent-progression-stories) | Yes; keep delivery eligibility separate from optional producer history. |
| 11 | Should source provenance for pieces #5793, #5794, #5795, and #5806 be an optional history result instead of a delivery gate? | [producer resets](../../../areas/zon/minizones.zon), [source lineage review](../zone-stories/MINI_ZONES.md#independent-progression-stories) | Preserve supplied-piece acceptance; bind actual reset generation, container parent, custody, and transfer only for optional provenance. |
| 12 | Should the three similarly named metal strips remain distinct by exact object VNUM in inventory checks? | [sword QST](../../../areas/qst/minizones.qst#L66), [object source](../../../areas/obj/minizones.obj) | Require one of each exact prototype; generic aliases must not collapse distinct materials. |
| 13 | Which spelling should be canonical for the sword: `Magik` in the object and procedure, or `Majik` in some prose? | [object source](../../../areas/obj/minizones.obj), [special assignment](../../../src/specs/specs.assign.c#L2324), [story dossier](../zone-stories/MINI_ZONES.md#independent-progression-stories) | Correct the inconsistent prose to the implemented name unless the owner chooses a deliberate rename. |
| 14 | Should Thrulmar's “five types” text be corrected to four current recipes, or should a fifth recipe be designed? | [four native recipes](../../reference/zone-story-audits/minizones.md#native-exchanges), [service review](../zone-stories/MINI_ZONES.md#four-supporting-armor-services) | Describe the four existing outputs; do not infer a fifth recipe from prose alone. |
| 15 | Should each recipe's fee remain exactly 400 platinum (400,000 copper), with payment and output handled as one recoverable operation? | [recipe QST](../../../areas/qst/minizones.qst#L143), [procedure review](../zone-stories/MINI_ZONES.md#four-supporting-armor-services) | Keep the current mixed-fee guard in place until currency validation, debit, item consumption, and output recovery are qualified together. |
| 16 | Should armor-service receipts remain economic service history without story, achievement, or daily credit? | [service contracts](../../reference/zone-story-audits/minizones.md#native-exchanges), [sidecar exclusions](../../../areas/story/minizones.story.json) | Keep receipts scoped to the paid service; do not promote purchases into story progress. |
| 17 | Should all four recipes require five separate #500025 crystals plus the matching base, while crystal-source history remains optional? | [recipe terms](../../../areas/qst/minizones.qst#L143), [shared prototype](../../../areas/obj/surface.obj#L330), [service review](../zone-stories/MINI_ZONES.md#four-supporting-armor-services) | Preserve exact quantities and base items; do not require personal mining unless an approved provenance rule is added. |
| 18 | Which of the nine unresolved exits should receive approved loaded destinations, and which should remain absent? | [unresolved exits, rows 8–16](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L8), [local map](../../../areas/wld/minizones.wld) | Choose targets from intended map design; do not synthesize destinations from numeric matches alone. |
| 19 | Are Mini Zones' one-way exits to Mist and Valley, and the one-way inbounds from Heavens and `wh`, intentional? | [Heavens pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L176), [Mist pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L271), [Valley pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L273), [`wh` pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L802) | Preserve the authored directions until return routes and safety are explicitly chosen. |
| 20 | What safe identity, destination, and follower rules should govern the dryad rescue procedure assigned to princess #5701 and enslaved human #5702? | [literal assignments](../../../src/specs/specs.assign.c#L681), [procedure review](../zone-stories/MINI_ZONES.md#access-custom-behavior-and-quest-like-lore) | Resolve the source/procedure identity mismatch and validate actor, charm, follower, and destination state before adding rescue credit. |

## Validation and limits

The register, all 95 inactive WLD files, relevant reset/QST/shop/story-sidecar
references, title set, same-stem histories, and linked source locations were
reviewed statically. Twenty builder questions were recorded. No build, test,
server, database operation, migration, or operational script was run. Installed
world loading, payment recovery, player acquisition, live travel, and the
proposed gameplay repairs remain unqualified.

## Checkpoint 775: active Braddistock reset imports

Mini Zones ZON lines 388–389 contain two O reset references to active
Braddistock zone-13 object #1320, a small diamond. These are outgoing reset
dependencies from Mini Zones; preserve the Braddistock prototype owner and
the two authored ground-reset records. See the
[Braddistock zone-13 audit](BRADDISTOCK_ZONE13_AUDIT_REVIEW.md).

## Bloodstone Checkpoint 778: shop and reset imports

Active Bloodstone has one ZON reset and one shop product reference to Minizones object #5743. Preserve its current owner; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#outbound-shop-products).
