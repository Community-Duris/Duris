# The Temple of Blibdoolpoolp

The active Kuo-Toa Temple contributor is zone #882, titled The Temple of Blibdoolpoolp. It has 37 rooms (#88200–#88236), six mobile prototypes (#88200–#88205), and seven object prototypes (#88200–#88206). The contributor appears in both active area indexes, but there is no local QST file, shop, story sidecar, or native Q/QA contract. See the [active area list](../../../areas/AREA#L310), [zone header](../../../areas/zon/kuotoa.zon#L1), [QST area index](../../../areas/qst/AREA#L276), [coverage row 151](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L151), and the [builder questions and resolutions](../quest-story-followups/KUOTOA_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions).

## Green stone passages and the temple

The entry passage is carved from green stone, damp, and fish-scented. Its west exit leads directly to the Large Pond in the active Underworld area (#4454); the local entry room is #88200. Underworld #4454 also connects south to its own small Shrine of the Kuo-Toa at #4457. This is a direct neighboring route and parallel shrine text, while the two WLD sources retain their separate room and zone identities. See the [local entry](../../../areas/wld/kuotoa.wld#L1), the [Underworld boundary](../../../areas/wld/underworld.wld#L1720), and [Underworld’s shrine](../../../areas/wld/underworld.wld#L1797).

The temple area continues through underground passages, a large quiet pool, carved abodes, a temple approach, vestibules, and an altar chamber. Room #88212 reaches the temple entrance #88224 through double doors that the zone reset closes. The altar room is #88227. A branch leads into the disturbing cavern at #88231–#88236, where resets place insane Kuo-Toa; the terminal slime-covered hole has a down exit to #88235. See the [temple rooms](../../../areas/wld/kuotoa.wld#L249), [entrance and altar](../../../areas/wld/kuotoa.wld#L408), and [cavern end](../../../areas/wld/kuotoa.wld#L606).

The area title spells the deity name “Blibdoolpoolp.” Room #88227 instead names “Blipdoolpoolpoo” and calls the altar one to the Kua-Toan deity. Active Gold Hall source also has an altar titled “Blipdoolpoolpoo”; its room text gives another spelling, “Blipoodoopoo.” Underworld #4457 describes a large image of the Kuo-Toan god without naming it. These are authored naming parallels, but the sources do not define one canonical spelling or identify the Gold Hall settlement as this temple. See the [local altar](../../../areas/wld/kuotoa.wld#L459), [Gold Hall altar](../../../areas/wld/gold_hal.wld#L3958), and [Underworld shrine](../../../areas/wld/underworld.wld#L1797).

## Reset population and objects

The 32 local mobile reset commands place all six prototypes. Kuo-Toa guards occupy the entry and passages; one Kuo-Toa rests beside the pool; a huge monitor stands at #88211; common Kuo-Toa occupy the abodes; worshipers appear around the altar; guards stand at #88230; and insane Kuo-Toa occupy the disturbing cavern. The five equipment reset commands and two carried-item commands reference all seven local objects: a coral torc, jagged sword, large net, pile of treasure, shark-skin boots, shark-skin gloves, and coral bracer. These are source reset declarations and do not establish live placement or availability. See the [zone resets](../../../areas/zon/kuotoa.zon#L122), [mobile profiles](../../../areas/mob/kuotoa.mob#L1), and [object profiles](../../../areas/obj/kuotoa.obj#L1).

The usual local room flags and sector are 33558536 and SECT_UNDRWLD_INSIDE (15). Room #88228 uses flags 176197704 and SECT_CITY (1), while retaining the same generic abode description and an exit to #88211. It has no local mobile reset. The differing values may be intentional; confirm the room’s intended classification before changing it. See [room #88228](../../../areas/wld/kuotoa.wld#L470) and the [sector constants](../../../src/core/defines.h#L459).

## Builder follow-up

- Confirm the intended spelling of Blibdoolpoolp and whether the similarly named Gold Hall altar shares the same deity lore.
- Clarify how this temple relates to Underworld’s nearby Kuo-Toa shrine at #4457.
- Confirm the intended sector and flags for room #88228.
- Confirm whether all 14 door pairs should start closed, especially the double doors between #88212 and #88224. From the Underworld arrival at #88200, only 14 of 37 rooms are reachable in the strict reset-open WLD graph.
- Decide whether the Kuo-Toa encounters have a story or contract outside the current area files; no local QST or native Q/QA contract supplies one.
