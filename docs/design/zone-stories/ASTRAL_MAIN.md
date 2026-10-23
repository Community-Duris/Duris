# The Astral Plane source story dossier

## What the source says

`astral_main` is active zone `197`, titled **The Astral Plane**, with 125
rooms from #19701 to #19825. One hundred twenty-two rooms use the same bright
silver-gray, well-lit astral description. Three landmarks break that
repetition: a floating fragment of the Elemental Plane of Fire, the
gravity-pulling Great Worm Hole, and Yggdrasil, the World Ash
([fire sphere](../../../areas/wld/astral_main.wld#L201),
[wormhole](../../../areas/wld/astral_main.wld#L802),
[Yggdrasil](../../../areas/wld/astral_main.wld#L1283)). The wormhole is rumored
to connect every plane to the Astral Plane. The tree is said to touch the
known planes and carry gods between them; its additional description says
only gods are powerful enough to travel by it.

The zone has no authored quest contracts or story sidecar. Its `.qst` file
contains only the #19800 section terminator, so the Githyanki Supreme Leader
prototype does not have a native quest chain here. The narrative instead
comes from room prose, mobile descriptions, reset placements, special
callbacks, and connections to neighboring plane areas
([quest source](../../../areas/qst/astral_main.qst#L1),
[zone resets](../../../areas/zon/astral_main.zon#L1)).

## Factions and encounters

The clearest conflict is an Illithid invasion. Xzazixxazz, the mindflayer
captain, was sent by an elder brain to restore Illithid dominance over the
Githyanki, who had broken free of slavery. A mindflayer follower is described
as guarding the captain. Astral Main's resets also place Githyanki warriors,
warlocks, knights, a supreme leader, and a chief knight. These descriptions
support an active faction conflict, but the source does not explain its
outcome, timeline, or whether the invasion is current or an older event
([captain](../../../areas/mob/astral_main.mob#L33),
[mindflayer](../../../areas/mob/astral_main.mob#L50),
[Githyanki leader](../../../areas/mob/astral_main.mob#L232),
[resets](../../../areas/zon/astral_main.zon#L13)).

The 24 local mobile prototypes also point to the Astral plane as a meeting
place for powerful beings and planar wildlife: Tiamat, Demogorgon, a guardian
dragon highlord, five chromatic dragon wyrms, phoenixes, shadow daemons,
slaadi, succubi, mariliths, and smoke elementals. The Main zone's resets
place many of these prototypes. Tiamat mobile #19700 is reset by the separate
`tiamat` area, and a story contact elsewhere also names her. The source does
not establish one shared plot connecting all of these encounters.

A note adds a specific Githzerai thread. Object #19815 is addressed to
Gelikh'nat and signed by Gethzar'tkith, High Mage. It reports that the
Githzerai have been spying from a hiding place in the cliffs, directs the
recipient to hold off on attacking, and says Kalnarish has something planned
for them. Astral Side Areas resets this note inside Main object #19810, a
small wooden desk placed in room #19850, described as the chief of the
Githyanki warriors' quarters ([note](../../../areas/obj/astral_main.obj#L417),
[desk](../../../areas/obj/astral_main.obj#L404),
[reset](../../../areas/zon/astral_areas.zon#L80),
[room](../../../areas/wld/astral_areas.wld#L632)). This gives the area
environment a second faction conflict, but the source does not explain who
Kalnarish is or what the planned action will be. Object #19815 shares its
number with Astral Main room #19815; these are separate object and room
references.

## How this area fits the neighboring zones

Five outbound WLD exits connect the Main map to four active areas. The south
rift at #19755 and upper sphere route at #19815 connect reciprocally to
Astral Side Areas. Room #19794 leads west to Gith Hometown #19400, room
#19798 leads up to Githyanki Fortress #132000, and room #19800 leads north to
Astral Tiamat #19928. Each has a reverse WLD exit. Astral Tiamat and Astral
Side Areas also have one-way WLD exits into Main. External portals and pools
add other authored planar links; their placements and limitations are
cataloged in the [audit review](../quest-story-followups/ASTRAL_MAIN_AUDIT_REVIEW.md).

Yggdrasil has a distinct active connection to Jotunheim: Jotun's reset object
#96005 targets room #19733. Its object text says the branches span the
realms, and Astral Main's tree object says its branches reach toward
Jotunheim ([tree object](../../../areas/obj/astral_main.obj#L15)). The room
text describes the tree as a passage used by gods. That supports a shared
landmark, though it does not establish ordinary access or the route's
intended user conditions.

## Builder questions

1. Is the Illithid invasion an active event, a historical account, or an
   encounter theme? Should the source describe what happened after the
   Githyanki broke free?
2. Are the Great Worm Hole and Yggdrasil intended as ordinary travel routes,
   story landmarks, or both? The prose promises broad planar connections,
   while the reviewed reset objects route to only a few documented targets.
3. Should the zone have native quest contracts or a story sidecar, or is the
   environmental conflict intended to carry its narrative?
4. Are the three mobile reset commands for missing mobile #19860 and the
   ground reset for missing object #19997 obsolete, or do they need valid
   prototypes?
5. Is the `GithyankiCave` callback meant to bind to an Astral Main room? Its
   current room reference #19890 is an object VNUM, and no room #19890 is
   authored.
6. Is the reciprocal magic-pool route between Astral Tiamat rooms #19945 and
   #19946 intentional, including each pool's damage value?
7. What action does Kalnarish have planned for the spying Githzerai, and who
   are Gelikh'nat, Kalnarish, and Gethzar'tkith? The note is reset inside the
   desk in the Githyanki chief's quarters in Astral Side Areas.
8. Should `astral_main` have a command-target alias beyond its normalized
   title “The Astral Plane”? The title is unique among 442 scanned `.zon`
   files, and no lexical duplicate was found. Keep the source key and display
   title; add an alias only after the owner names its exact target and intended
   distinction.

**Recommended source disposition:** keep `astral_main` active and in place.
Preserve its reciprocal map boundaries and all typed dependencies while the
area owner resolves the invalid reset targets, missing room callback, and
pool destinations. This dossier describes authored source; it does not
claim live reachability, combat behavior, or safe travel.
