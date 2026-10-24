# Arena source story dossier

## What the source supports

`arena` is a tournament venue titled **CTF Tournament**. Its authored setting
is expressed almost entirely through room names: three colored team squares
pair with treasure vaults, while an 81-room battlefield sits in a separate
static component. The subsystem moves participants there when a match starts.
The battlefield's room names include marbled and onyx halls, tower balconies,
staircases, and tunnels. The 87 WLD records have empty descriptions, the local
MOB file is empty, and the only local object is a portal. The source contains
no local quest file, shop file, or story sidecar, despite `arena` appearing in
the historical `areas/qst/AREA`
index ([zone header and resets](../../../areas/zon/arena.zon#L1),
[room source](../../../areas/wld/arena.wld#L1),
[portal prototype](../../../areas/obj/arena.obj#L1),
[quest index](../../../areas/qst/AREA#L232)).

One authored story connection comes from outside the zone. `tharnadia.zon`
resets portal object #67900 in room #132574, **The Ogre's Foot Lounge**. That
inn room describes warriors waiting for work; the placement suggests a
gathering or hiring point for combatants, but no dialogue or quest explains a
tournament invitation ([Tharnadia reset](../../../areas/zon/tharnadia.zon#L518),
[lounge description](../../../areas/wld/tharnadia.wld#L1939)). The portal itself
is described as a large portal to the arena, and its compiled behavior handles
entry and exit ([object text](../../../areas/obj/arena.obj#L2),
[object callback](../../../src/combat/arena.c#L469),
[compiled binding](../../../src/specs/specs.assign.c#L1699)).

## The place and its three teams

The zone's first six rooms form three independent two-room pairs:

| Team | Square | Paired room | Source title pattern |
| --- | ---: | ---: | --- |
| Goodie | #67904 | #67905 | Golden Square / Treasure Vault |
| Evil | #67902 | #67903 | Onyx Square / Treasure Vault |
| Undead | #67900 | #67901 | Marbled Square / Treasure Vault |

The compiled arena table maps those pairs to the Goodie, Evil, and Undead
race-war teams in that order. The ZON file resets one arena portal in the
Goodie and Evil squares, but not in the Undead square. A separate active
Tharnadia reset provides another copy outside the arena. These are functional
team staging rooms rather than a quest progression: the source contains no
givers, local enemies, rewards, or recorded tournament outcome.

Rooms #67906–#67986 form the 81-room battle map. Its names alternate between
the marbled and onyx halls and the massive battlefield, with eight tower
balconies and small tunnel sections. The `map_spec` table uses #67906–#67986
as the map range; #67907 and #67983 are the Goodie and Evil spawn entries for
team deathmatch, while the third spawn is -1
([arena map and team tables](../../../src/combat/arena.c#L55)).

## The event implied by the code

The portal's callback admits eligible Goodie, Evil, and Undead race-war
players when the arena is enabled. It places each entrant in that team's first
staging room. When a match begins, the subsystem moves participants from both
rooms of each team's pair into the battle map; the default Deathmatch mode
selects a random room in the map range. At the end of the match, participants
are returned to their team's first staging room
([entry callback](../../../src/combat/arena.c#L469),
[map transfer](../../../src/combat/arena.c#L221),
[match cycle](../../../src/combat/arena.c#L605)).

The source title and compiled defaults do not fully agree: `arena.zon` says
**CTF Tournament**, the arena subsystem offers Capture the Flag as a mode, but
startup defaults to Deathmatch and leaves the arena disabled. A separate
global CTF flag registry places its primary flags at #132573 and #97628, not
inside this zone ([arena defaults and mode IDs](../../../src/combat/arena.h#L8),
[configured map name and game labels](../../../src/combat/arena.c#L37),
[global CTF flag table](../../../src/combat/ctf.c#L31)). The sole configured
map is labeled “Opposing Castles,” but the room titles do not explain a castle
story. The files establish a staff-operated arena mechanism; they do not
establish that the current world is running a public CTF event.

## Builder questions and recommended decisions

1. **What should “CTF Tournament” promise?** The current compiled startup
   state is closed Deathmatch. Decide whether the title describes a legacy
   zone name, a staff-selected arena mode, or an intended public CTF event.
   Keep the room text neutral until that operating mode is confirmed.
2. **Where should each team return?** The code comments label #6075 Goodie
   and #29700 Evil, but the current room titles are “The Swamp of the
   Clavikord” and “The Goodie Bitchin Room,” respectively. Undead has no
   configured hometown; the portal exit branch sends every non-Evil race to
   #6075, while the final-death path indexes Undead's -1 destination. Check
   all three endpoints, give Undead a valid staging exit and return destination,
   and qualify those paths before treating them as safe; see the
   [repository audit](../quest-story-followups/ARENA_AUDIT_REVIEW.md#undead-return-path).
3. **Is the inn connection intentional?** The active Tharnadia reset places
   the entry portal in a lounge where warriors wait for work. Confirm whether
   that is the tournament's intended public arrival point or an old event
   hook.
4. **Does the venue need narrative beyond its geometry?** No authored
   descriptions or quest outcomes explain who built the three vaults, what is
   won, or how tournament results matter. If those details are part of the
   intended setting, supply them as a separate builder decision rather than
   inferring lore from room colors or the word “treasure.”
5. **Should `arena` have any command-target alias beyond the unique display
   title “CTF Tournament”?** The normalized title is unique among 442 scanned
   `.zon` files, and the compiled mode/map labels are separate from any player
   command mapping. Keep the source key and display title; add an alias only
   after the owner names its exact target and intended distinction.

**Recommended current classification:** retain `arena` as an event-driven PvP
venue, record the title/default mismatch and Undead return route for a
maintainer decision, and add no quest or victory lore without builder intent.
Keep the active area in place. No area or gameplay source was changed for this
dossier.
