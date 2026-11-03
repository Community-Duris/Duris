# Sea (Graendiae): source story review

**Scope:** static review of Sea's 270 rooms, 55 mobile prototypes, 62 object
prototypes, 415 zone commands, 11 shops, sidecar, direct special assignments,
and typed references. This maps authored story and topology; it does not prove
runtime quest generation, travel, or safety.

## Story coverage and identity

The active `sea` entry covers zone headers 673–675 and uses the title
“Graendiae” ([active list](../../../areas/AREA#L255)). This is the underwater
city, distinct from `seakngdm` (“Sea Kingdom,” zone headers 315–317;
[active list](../../../areas/AREA#L154)). The historical quest index also
lists `sea` ([index](../../../areas/qst/AREA#L229)), but there is no current
`areas/qst/sea.qst` file and the coverage row records zero native Q/QA
contracts ([coverage row](ZONE_COVERAGE_REGISTER.csv#L260)).

The authored sidecar claims `coverage: complete` and invites players to meet
Graendiae's people for local requests, but its `contacts`, `stories`, and
`exclusions` arrays are empty ([sidecar introduction](../../../areas/story/sea.story.json#L6),
[arrays](../../../areas/story/sea.story.json#L11)). A source-code exception is
important: mobile 67324, the bartender in the Drunken Brawl, is assigned the
generic `world_quest` special ([assignment](../../../src/specs/specs.assign.c#L757),
[handler](../../../src/specs/specs.world_quest.c#L191)). The handler supports
paid quest, map, and abandon/resign requests and supplies generic world quests;
it is not a local QST chain. The Broken Promise Inn and Flowering Anemone Inn
are also assigned the generic `inn` special
([room 67493](../../../src/specs/specs.assign.c#L2298),
[room 67503](../../../src/specs/specs.assign.c#L2300)). Neither code binding
has been exercised here. The sidecar should distinguish these generic
interactions from locally mapped Graendiae stories.

## Environmental story

| Story layer | Source evidence | What the authored text establishes |
| --- | --- | --- |
| Living coral city | [Seaweed-covered grate](../../../areas/wld/sea.wld#L1), [Central Square](../../../areas/wld/sea.wld#L89), [reef entrance](../../../areas/wld/sea.wld#L5453) | Graendiae is an aquatic-elf settlement built inside a living coral reef. Room text attributes coral growth to small hidden creatures; residents try to maintain their city while keeping those creatures from reclaiming it. An inscription at the grate hints that the city's name is the key to its entrance. |
| Engineered currents and commerce | [Crescent Current](../../../areas/wld/sea.wld#L312), [southern outlet](../../../areas/wld/sea.wld#L331), [market square](../../../areas/wld/sea.wld#L4687), [Waterways](../../../areas/wld/sea.wld#L4820) | The Graendiae Engineering Group builds grates, widens crowded channels, and made lower Waterways for faster travel. Current strength varies by district; residents in the south are described as fit because swimming upstream is difficult. Market Square and its shops anchor trade. |
| Food and livestock | [Path of Grasses](../../../areas/wld/sea.wld#L1059), [Pjworn Fields](../../../areas/wld/sea.wld#L1129), [herder mobile](../../../areas/mob/sea.mob#L528) | Pjworn are bred and harvested in a dedicated cavern for the city's famous chowder export. The prose describes rounded rock walls and a sand floor as deliberate livestock design, so the fields read as managed food production rather than wilderness. |
| Unequal prosperity and industrial collapse | [Laughter's Edge](../../../areas/wld/sea.wld#L2012), [small dwelling](../../../areas/wld/sea.wld#L5510), [unemployed father](../../../areas/mob/sea.mob#L708), [poor girl](../../../areas/mob/sea.mob#L694) | The Shipwreck and Dive Recycling Center once supported one of Graendiae's wealthiest neighborhoods. After it closed, partly because salvage work cost lives, homes shrank as coral reclaimed space; some residents seek work or beg for food. An unemployed father explicitly asks players for a job lead, but the local QST file and sidecar contain no matching local story record. |
| Temples, guilds, and contested knowledge | [Path of Redemption](../../../areas/wld/sea.wld#L2590), [Temple of Janinae](../../../areas/wld/sea.wld#L3313), [fountain of flames](../../../areas/wld/sea.wld#L3340), [altar](../../../areas/wld/sea.wld#L3536) | The temple honors Janinae. Residents credit her with giving them access to magically contained lava; dissenters attribute it to a gnome scientist's bargain and are called heretics. Worshippers leave crafted offerings at the altar. The city also has cleric, magician, and warrior guilds, a hospital, and weapons training. |
| Hidden crime | [Hustler's Tunnel](../../../areas/wld/sea.wld#L3050), [Smuggler's Den](../../../areas/wld/sea.wld#L3159), [Black Market](../../../areas/wld/sea.wld#L4585), [Sslin mobile](../../../areas/mob/sea.mob#L1), [Sslin reset](../../../areas/zon/sea.zon#L374) | A concealed tunnel network and black market support illicit trade. Sslin is described as gang leader, while the WLD alternates between “Ghil'ha” and “Ghil'ra”; reconcile the gang name before treating those labels as settled canon. |
| Surface artifacts and outlook | [Museum](../../../areas/wld/sea.wld#L4687), [artifacts room](../../../areas/wld/sea.wld#L4727), [restoration lab](../../../areas/wld/sea.wld#L4744) | The museum teaches children about life on the surface “in the eyes of aquatics.” Officials buy surface goods to restore artifacts, but the text says repairs are imperfect because ocean materials are unavailable and that they sometimes use “ingenuity.” This is a useful place to clarify what Graendiae values, misunderstands, or appropriates about the surface world. |
| Homes and everyday life | [Flow of Friendship](../../../areas/wld/sea.wld#L1284), [family home](../../../areas/wld/sea.wld#L3756), [restaurant](../../../areas/wld/sea.wld#L3926), [bank](../../../areas/wld/sea.wld#L4530) | The prosperous eastern streets, domestic rooms, restaurant, and bank contrast with the abandoned apartments and closed industrial district. The source depicts residents, children, merchants, and public institutions as part of one working city, not only as combat scenery. |

Two mobile descriptions identify characters as teenagers and then describe
explicit sexual activity. Their reset room is a bedroom in the home district
([mobile 67334](../../../areas/mob/sea.mob#L495),
[mobile 67335](../../../areas/mob/sea.mob#L512),
[placements](../../../areas/zon/sea.zon#L380)). The text should be removed or
rewritten as nonsexual, age-appropriate material before it is presented to
players. No prose was changed in this review.

## Shops, resets, and typed references

The zone has 415 reset commands: 284 `M`, 54 `G`, 38 `D`, 13 `E`, 25 `O`, and
one `P`. The 11 shop records each name a keeper who has a `M` reset in the
shop's configured room; the source-parsed keeper and room values match for all
11 records.

| Keeper | Shop record | Reset room |
| --- | --- | --- |
| Haerlian | [67320](../../../areas/shp/sea.shp#L1) | [67433](../../../areas/zon/sea.zon#L334) |
| Sanriff | [67322](../../../areas/shp/sea.shp#L39) | [67439](../../../areas/zon/sea.zon#L341) |
| Bartender | [67324](../../../areas/shp/sea.shp#L80) | [67445](../../../areas/zon/sea.zon#L355) |
| Karlaen | [67327](../../../areas/shp/sea.shp#L115) | [67446](../../../areas/zon/sea.zon#L363) |
| Bjdwen | [67336](../../../areas/shp/sea.shp#L156) | [67489](../../../areas/zon/sea.zon#L384) |
| Praelie | [67337](../../../areas/shp/sea.shp#L194) | [67490](../../../areas/zon/sea.zon#L389) |
| Armorer | [67338](../../../areas/shp/sea.shp#L232) | [67491](../../../areas/zon/sea.zon#L394) |
| Qenlynn | [67340](../../../areas/shp/sea.shp#L273) | [67495](../../../areas/zon/sea.zon#L412) |
| White-robed philosopher | [67341](../../../areas/shp/sea.shp#L312) | [67305](../../../areas/zon/sea.zon#L188) |
| Qarrizna | [67345](../../../areas/shp/sea.shp#L346) | [67494](../../../areas/zon/sea.zon#L406) |
| Gvalldrianis | [67352](../../../areas/shp/sea.shp#L383) | [67538](../../../areas/zon/sea.zon#L452) |

The source bindings expose three object dependencies that need owner review:

- Sea resets Verz1 object 3097, a wooden counter, in room 67523
  ([reset](../../../areas/zon/sea.zon#L182),
  [active prototype](../../../areas/obj/verz1.obj#L1199),
  [active owner](../../../areas/AREA#L15)).
- Object 67322 has no prototype in any current `areas/obj/*.obj` file, but
  Karlaen's shop stock lists it and the Sea ZON gives it to mobile 67322
  ([shop stock](../../../areas/shp/sea.shp#L115),
  [reset](../../../areas/zon/sea.zon#L368)). Confirm whether the object was
  deleted, mistyped, or is meant to be restored; the matching mobile VNUM
  `Sanriff` is a separate prototype namespace.
- Active Brass both resets and stocks Sea object 67359, “seaweed rations”
  ([reset](../../../areas/zon/brass.zon#L822),
  [shop stock](../../../areas/shp/brass.shp#L585),
  [Sea object](../../../areas/obj/sea.obj#L616),
  [active Brass entry](../../../areas/AREA#L397)). Unregistered
  `brass-old-1` also retains reset and shop references to 67359
  ([reset](../../../areas/zon/brass-old-1.zon#L832),
  [shop stock](../../../areas/shp/brass-old-1.shp#L571)); keep that legacy
  dependency in its candidate-specific review.

A numeric scan of current ZON/SHP/QST files found no other references into
Sea's VNUM range and no external QST references. A direct source-code scan of
that range found the bartender's `world_quest` binding and the two inn-room
`inn` bindings recorded above. These static scans do not qualify other
runtime, editor, or persisted-data dependencies.

## Static exits, currents, and stable access

The active source census records 719 Sea exits, of which 716 target rooms in
the active world. There are no negative targets and no active cross-area WLD
edges. The three unresolved exits leave room 67568 east, south, and west for
VNUMs 219820, 220119, and 219818; those target VNUMs occur only in unregistered
`surf`/`surface2011` WLD sources in the normal build
([exact rows](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L254)). The frozen census
therefore does not provide an active-world route through those exits.

Unregistered `surf.wld` contains the corresponding source-only directions back
to active Sea room 67568:

| `surf.wld` source | Direction to active room 67568 | Evidence |
| --- | --- | --- |
| 219818 | East (`D1`) | [`surf.wld`](../../../areas/wld/surf.wld#L415766) |
| 219820 | West (`D3`) | [`surf.wld`](../../../areas/wld/surf.wld#L415808) |
| 220119 | North (`D0`) | [`surf.wld`](../../../areas/wld/surf.wld#L422075) |

Together, these three active-source exits and their three `surf.wld` return
exits form apparent source-level pairings between Graendiae and the former
Surface topology, but none is an active-world route today. They
are separately recorded in the [world connectivity report](WORLD_CONNECTIVITY.md)
and the [`surf` retention plan](ATTIC_MIGRATION_PLAN.md#L27). Preserve the
candidate sources until builders decide the intended modern access and
return route.

The same three room VNUMs also exist in `surface2011.wld`, titled “The Azure
Waters of a Storm Sea”; those records have different exits and none returns to
Sea room 67568. Keep these numeric matches as separate candidate map
identities. The three `surf.wld` returns were already present in the reviewed
Surf baseline, so they do not add to the current 263-record Surf subset or
4,499 global curated total. See the [Checkpoint 348 Sea audit](SEA_AUDIT_REVIEW.md#active-connectivity-and-reset-aware-reach)
for the exact identity comparison and reset-aware graph results.

Within Sea, the weak-exit graph has a 269-room component and one isolated room:
the Stables of Graendiae, room 67540. The Creature Rental room 67539 exits to
Waterways room 67525, not to the stable; room 67540 has no authored exits
([rental](../../../areas/wld/sea.wld#L4801),
[stable](../../../areas/wld/sea.wld#L4815)). The ZON nonetheless resets a
seahorse and hippocampus there ([resets](../../../areas/zon/sea.zon#L456)).
There is no shop record for either room. Confirm whether the stable is intended
to be entered through an external/special mechanism, or whether its mount
service and room link are incomplete. The 11 shops' static placements do not
resolve that access.

Sea has 58 rooms with current records and one room with a fall-chance field.
Waterway Latrines room 67501 stores `F 5` and current values `10 5`
([room record](../../../areas/wld/sea.wld#L4065)). Elsewhere the Crescent
Current prose warns that swimming upstream is difficult and residents return
by another route ([room 67315](../../../areas/wld/sea.wld#L312)). These are
source markers and authored warnings, not a live hazard or safety result. All
38 nonzero WLD door-flag exits have matching ZON D resets by room and
direction; actual reset outcomes and usable return paths remain untested.

## History, disposition, and builder questions

The normalized active-title pass reports no repeated Sea title. Followed WLD
history shows the 2009 move into the `areas/` tree and later content commits in
2011 and 2013; no direct retirement or replacement mapping was found. Keep Sea
and `seakngdm` distinct in the area crosswalk.
Sea is an un-commented active `areas/AREA` contributor and is **not an attic
candidate**.

The eight questions below are the earlier story-focused set. The consolidated
[Checkpoint 348 builder agenda](SEA_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions)
expands the cross-discipline review to twenty source-backed questions and
recommendations.

Before treating the authored sidecar and map as complete, ask the area owner:

- Should the sidecar's “complete” label and “local requests” wording be
  reconciled with empty local story arrays and no Sea QST file, while also
  documenting the bartender's generic world-quest feature?
- Is the job-seeking father in Laughter's Edge only environmental color, or
  should the world-facing story map record an intended employment arc?
- What object is VNUM 67322, and should both the ZON reset and Karlaen shop
  stock be corrected or supplied with a current prototype?
- Which active-world route should connect Graendiae to the Surface, and how
  should it return? Do the three `surf.wld` records still express intended
  topology?
- Is the Creature Rental/stable pair intentionally special-routed despite its
  detached room, or are the route, room, and shop records incomplete?
- Can the teenage mobile descriptions be replaced with nonsexual,
  age-appropriate text?
- Is the gang's canonical name “Ghil'ha” or “Ghil'ra”?
- Should the museum's deliberately limited surface perspective and
  imperfect artifact-restoration process be clarified as Graendiae's intended
  voice?
