# phase 1 live test

Note some of these may be false positives or may have been already repaired.  They must be addressed carefully to not break working existing systems.

## (potential) BUGS & ISSUES

 -  VERIFIED FIXED (2026-08-30): password-hash temporaries now use matching `malloc`/`free`; the focused hash runtime test and an actual-character login/`score` under Memcheck completed with 0 errors.
 -  VERIFIED FIXED (2026-08-30): restored the framed, colored good/evil/neutral race-selection screen while preserving availability filtering and the restricted-race testing block; verified through live account creation after an actual-character login.
 `docs/ongoing-projects/1767242209173_x3drwi.webp`
 -  VERIFIED FIXED (2026-08-30): character-creation confirmations now allow 10 minutes instead of 60 seconds; after an actual-character login, a live creation session remained connected past the old cutoff and continued successfully.
 -  VERIFIED FIXED (2026-08-30): character creation now renders the authoritative rules file before agreement; focused catalog tests and a live actual-character check passed.
 -  VERIFIED FIXED (2026-08-30): chaos-mode actual-character login completed without ownership-authority grant rejections; the durable starter baseline and serialized grant regressions also pass.
 -  VERIFIED FIXED (2026-08-30): no delayed ownership-authority rejection appeared during a 150-second post-login chaos-mode watch, and the actual character remained responsive afterward.
 -  VERIFIED FIXED (2026-08-30): commune-capable non-dragoon characters now silently resume spell-slot recovery on login; a temporary actual druid communed without issuing the command.
 -  VERIFIED FIXED (2026-08-30): account login now displays the tracked news before the MOTD; verified through an actual-character login and focused account/flat-file tests.
 -  VERIFIED FIXED (2026-08-30): positive carried-coin rewards now serialize and rebase behind in-flight wallet work instead of falling back to auction pickup; two immediate live 10k-platinum grants to an actual character produced no fallback message and durably recorded 20k platinum across two ledger entries.
 -  VERIFIED FIXED (2026-08-30): `auction pickup` accepted a live staged 1-platinum claim without the busy response, credited the actual character, cleared the staged balance, and recorded the durable auction-claim ledger entry; no further code change was required.
 -  VERIFIED FALSE (2026-08-30): "no NPCs loaded with items." A full-world game-loop audit found 13,433 NPCs with inventory or equipment (9,663 equipped); no code change was required.
 -  VERIFIED FIXED (2026-08-30): unpaid locker entry now captures the bank exit before the temporary locker is released and leaves cleanup to the normal locker hook; an actual zero-balance character received the payment rejection and was returned to the named Tharnadian Bank instead of an anonymous locker room.
 -  VERIFIED FIXED (2026-08-30): linkdead battle deaths now publish corpse-item completions against the live in-world character instead of waiting on a descriptor; all 28 actual-character items reached one corpse, death recovery completed, and the character logged back in alive without the save warning.
 -  VERIFIED FIXED (2026-08-30): corpse creation now defers extraction while its item handoff completes, after which the death save retries; the live linkdead battle test ended with one durable 28-item corpse and one saved death rather than an unsaved character/corpse split.
 -  VERIFIED FIXED (2026-08-30): `suicide` now rejects an already-dead character before confirmation or `die()` can run; the actual-character retest returned "You are already dead" and advanced durable state by exactly one battle death and one corpse.
 -  VERIFIED FIXED (2026-08-30): salvage eligibility now excludes corpses; live examination of the actual character's corpse reports that it cannot yield usable salvage materials.
 -  VERIFIED FIXED (2026-08-30): an actual-character disconnect/relogin during deferred death remained dead until the terminal retry completed, then returned to the account menu; it did not revive before the death was saved.
 -  VERIFIED FALSE (2026-08-30): an actual character crossed adjacent ASCII world-map road tiles and the prior tile remained `+`; road sectors explicitly do not create tracks, and no terrain mutation occurred.
 -  while walking through the forest "*" tiles on the world map, they turned into a "." after i walked on them and stayed that way.
 -  while communing, the spells would occasionaly stop returning in a timely manner for a short period and then return to the normal pace
 -  when typing list at a dock where i can purchase a ship it did not show me the list of options for purchase
 -  upon attempting to rent at a valid inn i received the message "Failed to save this character, most likely too much eq." (i had zero equipment)
 -  upon typing save i received the message "Save queued for Fer." but at no point did i receive a follow up message. i was also able to type it again and receive the same exact message. (it didn't show differently for state such as already queued for save. or save failed.
 -  class selection requires you to type the class name instead of having numbers or letters to select them. this is a change of behavior from the refactor.
 -  class selection does not list all available classes to a race while still lettings you type the race that wasn't listed such as druid while selecting human.
 -  after character creation, i selected not to keep the character and was taken back to the main login screen instead of to the account screen. i think typed the account name and was disconnected.
 -  players can see their own tracks in a room with the track skill
 -  database persistence is still broken, items are not granted on login. or upon summon such as wizard innate summon book. or on any npc's
 -  there was a bit of lag after using the specialization command at the teacher. this should not lag (i speced as wizard)
 -  if a log file cannot be written because the path does not exist, the path should be created. and the log entry should attempt to write again
 - made a new thri-kreen warrior, logged in, type `inv` saw these items:  You are carrying: (5/12)
a two-handed sword
a thin steel dagger
a steel short sword
a steel long sword
a small wooden mace
    then typed `inv` again a bit later and saw:

    <> inv
You are carrying: (5/12)
a two-handed sword
a thin steel dagger
a steel short sword
a steel long sword
a small wooden mace
< 36h/36H 170v/170V Pos: standing >
<> eq
You aren't wearing anything!
< 36h/36H 170v/170V Pos: standing >
<> wield long
You wield a steel long sword.
< 36h/36H 170v/170V Pos: standing >
<> wield mace
You wield a small wooden mace.
< 36h/36H 170v/170V Pos: standing >
<> inv
You are carrying: (21/12)
a crystalline chatkcha (glowing)
a bronze skull cap
[4] a thick spiked leather bracer
[2] some thick leather gloves
a pair of studded leather leggings
[2] a pair of studded leather sleeves
a thin batwing cloak
a thick leather belt
[2] a large wooden torch
a large snakeskin bag
a stout oaken cudgel
a steel warhammer
a two-handed sword
a thin steel dagger
a steel short sword
< 36h/36H 170v/170V Pos: standing >



- made a new human illusionist character, neutral race war (i tested good too with same result), picked Tharnadia hometown, got into the game and found myself in this room:
Cage of Smoke
Obvious exits: None!
< 30h/30H 120v/120V Pos: standing >
<> eq
