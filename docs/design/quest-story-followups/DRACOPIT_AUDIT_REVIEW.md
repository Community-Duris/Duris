# The Pit of Dragons repository-wide source audit

Checkpoint 212 maps active `dracopit` against its local WLD, MOB, OBJ, and ZON sources; active WLD boundaries; reset and teleport routes; followed file history; and typed references elsewhere in the repository. This is a static source audit. It does not inspect a live server, persisted state, player access, database contents, or actual gameplay. The [source dossier](../zone-stories/DRACOPIT.md) summarizes the setting and builder questions.

## Source identity and authored content

The active area list calls the contributor `dracopit` with note `*159`; its zone header is #159, titled “Pit of Dragons,” and its last room is #15968. The local WLD has 69 rooms (#15900–#15968), the MOB has 16 prototypes (#15900–#15915), and the OBJ has 15 (#15900–#15914). The QST area index lists the area, but there is no local QST, SHP, story sidecar, or native Q/QA contract. See the [area list](../../../areas/AREA#L85), [zone header](../../../areas/zon/dracopit.zon#L1), [QST area index](../../../areas/qst/AREA#L78), and [coverage row 78](ZONE_COVERAGE_REGISTER.csv#L78).

The WLD text repeats the same Pit of Dragons title and environmental passage through 62 rooms: extreme humidity, perpetual twilight, gray mist, no visible ground, and an oppressive sense of death and dread. Rooms #15917 and #15940 instead share the “An Extradimensional Pocket” description, a tear in reality that dragons cannot enter under normal circumstances and the only stated safety in the Pit. The five rooms #15964–#15968 are a distinct black-marble hallway carved with dragon scenes and filled with unexplained fear. The local text frames a dangerous prison and refuge, but does not identify the builders or origin of either place ([Pit](../../../areas/wld/dracopit.wld#L1), [pocket](../../../areas/wld/dracopit.wld#L732), [hallway](../../../areas/wld/dracopit.wld#L2747)).

The local ZON has 81 non-`S` commands: 2 D, 2 O, 40 M, 18 E, 7 G, and 12 F. All 16 mobile prototypes and all 15 objects are referenced by local resets. The overlord is reset with Dragondeath, whose prose says it is particularly powerful against dragons; code assigns that object a dedicated special. The zone also equips or gives blue eggs, dragon trophies, bronze dragonscales, black-scale shoulder guards, masks, horns, and a cloak. The cloak’s “only given to the overlord” text conflicts with its reset sequence: a `G #15904` immediately follows an `F #15907` follower reset near room #15936, and the reset handler places G objects on the current mobile pointer, which is the follower ([resets](../../../areas/zon/dracopit.zon#L20), [cloak](../../../areas/obj/dracopit.obj#L69), [F reset handler](../../../src/world/db.c#L7747), [G reset handler](../../../src/world/db.c#L7494), [Dragondeath hook](../../../src/specs/specs.assign.c#L1321)).

## Active WLD boundaries, components, and static reach

The local WLD records 393 exits, all resolving against the active loader inventory. Its raw local graph has two weak components: 64 rooms (#15900–#15963) and five rooms (#15964–#15968). Active WLD arrivals are Realm to #15913 and Mountain Castle to #15968. The Realm edge is one-way. The Mountain Castle link is reciprocal between #15968 and #37574; exact records are in the [cross-area pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L119), [Realm pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L388), and [exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L254).

Room #15968 also exits north back to Mountain Castle. Its south exit and the north exit from #15967 reset closed and locked; both WLD lock fields use key #37115. With all exits treated as open, static WLD reach from #15913 covers 64/69 rooms, while #15968 covers five. Applying the two unconditional D resets leaves reach of 64/69 from #15913 and 1/69 from #15968. The second route is cut at #15968 by its locked south exit. These are graph calculations only and do not model unlocking or player movement. See the [door resets](../../../areas/zon/dracopit.zon#L7), [locked exits](../../../areas/wld/dracopit.wld#L2819), and the [Mountain Castle reverse edge](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L539).

Two teleport objects affect approaches outside the WLD graph. Local object #15909 is reset in hallway room #15964 and targets pocket room #15940; WLD-only reach does not model using it. Active Caertannad resets its burning portal #78510 in room #78442; its `ITEM_TELEPORT` target is #15907 in the main Pit component. Thus the active dynamic arrival inventory includes one external item-teleport target in addition to the two WLD arrival targets. See the [local portal](../../../areas/obj/dracopit.obj#L158), [local reset](../../../areas/zon/dracopit.zon#L17), [outside portal object](../../../areas/obj/caertannad.obj#L1621), and [outside reset](../../../areas/zon/caertannad.zon#L390).

No negative or positive unresolved local exit target is recorded. A scan of all 95 unregistered WLD files found no room-number overlap or source-only approach into #15900–#15968. The WLD inventory remains 446 files: 351 active and 95 unregistered.

## Reset mode and state limits

The zone header is `15968 0 0 15 25 3`: reset mode is 0. `staff_stat` renders mode 0 as “Zone never resets,” and the ordinary new-event scheduler only schedules zones whose mode is nonzero. The event path for mode 0 calls `no_reset_zone_reset`, which reads persisted `zones.reset_perc` and can call `reset_zone` when its conditions are met. The source therefore contains a special reset mechanism even though ordinary scheduled resets are disabled. No database state was read, so this audit cannot establish whether that path has reset the zone or what its current doors and mobiles are. See [mode display](../../../src/cmd/staff_stat.c#L2282), [normal schedule](../../../src/world/new_events.c#L2068), [event routing](../../../src/world/events.c#L450), and [special handler](../../../src/world/db.c#L6846).

## Typed dependencies and direct code references

Role-aware scans found these exact authored outside relationships:

- Active `mntcastl.wld` has the reciprocal south exit from #37574 to #15968. Its key object #37115 is a dragon-tooth key. Active `mntcastl.qst` rewards #37115 to the player after Lord Du’Maathe explains that he is too old to explore the key’s secrets ([reverse exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L539), [key](../../../areas/obj/mntcastl.obj#L220), [quest output and dialogue](../../../areas/qst/mntcastl.qst#L135)).
- Active `caertannad.zon` resets external object #78510 in room #78442. That burning portal is an `ITEM_TELEPORT` with target room #15907 ([object](../../../areas/obj/caertannad.obj#L1621), [reset](../../../areas/zon/caertannad.zon#L390)).
- Local `dracopit.zon` gives external object #359 from `heavens.obj` to the gold dragon lord. The Heavens object is a rune-covered stone whose runes describe mana nodes across Duris ([reset](../../../areas/zon/dracopit.zon#L35), [object](../../../areas/obj/heavens.obj#L3725)).
- Compiled source assigns object #15907 the `mace_dragondeath` special and lists object #15906 in two chaos-equipment data entries ([special](../../../src/specs/specs.assign.c#L1321), [equipment data](../../../src/account/chaos_eq_data.h#L195)).

No external ZON room/mobile/object reset, QST item input/output, shop product, story-sidecar item link, duplicate MOB/OBJ prototype, external WLD key field, or external switch target into these local prototypes was found. No outside WLD source-room overlap or source-only WLD approach was found. The single external ITEM_TELEPORT target above is an active dynamic arrival, not a WLD edge. The local zone’s only external object prototype reference is #359; both locked doors use the outside key #37115.

## Title, source history, and disposition

After stripping color codes and normalizing, “Pit of Dragons” is unique among 442 current `.zon` titles. Followed WLD, MOB, OBJ, and ZON histories remain on the `dracopit` stem through the 2009 area-tree move; latest file history is 2013 (WLD), 2013 (MOB), 2013 (OBJ), and 2015 (ZON). No alternate-stem predecessor, retirement, or whole-zone replacement is established.

Keep active `dracopit` **DO NOT MOVE**. Preserve its two active WLD connections, portal targets, reset-key dependency, and object VNUMs referenced by Heavens and compiled code. No candidate row or curated source-only count changes. See the [title/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-212-pit-of-dragons-title-and-source-history), [coverage row 78](ZONE_COVERAGE_REGISTER.csv#L78), and [source dossier](../zone-stories/DRACOPIT.md).

## Builder follow-up

1. Resolve the dragon-hide cloak text/reset mismatch: should #15904 go to the overlord or should its description identify the current follower recipient?
2. Clarify how the plaque’s imprisonment story relates to the reset portal at #15964, its target pocket #15940, and the two pockets’ safety text.
3. Confirm whether the Caertannad portal’s dynamic arrival at #15907 and the Mountain Castle key route are intended ways into separate parts of the Pit.
4. Confirm that the Heavens rune stone #359 and Mountain Castle key #37115 should remain shared external prototypes.
5. Establish which persisted and reset conditions govern this reset-mode-0 zone before describing its live access state.

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad object #78510 has a type-25 target to Dracopit room #15907. See [Caertannad portal review](CAERTANNAD_AUDIT_REVIEW.md#portal-and-compiled-references).
