# phase 1 live test

Note some of these may be false positives or may have been already repaired.  They must be address carefully to not break working existing systems.

## (potential) BUGS & ISSUES

 -  allocator mismatch on password hashing
 -  the race selection screen was reverted to a plain text list in one of the refactors, here is how race menu should look (colors too!):
 `docs/ongoing-projects/1767242209173_x3drwi.webp`
 -  idle timeout during character creation is very short
 -  rules were not listed for player to read during character login (missing)
 -  message "The ownership authority is busy; the item was not granted." showed up a bunch of times upon initial login during "chaos" config
 -  a little while after login was complete "The ownership authority is busy; the item was not granted." showed up again
 -  when logged in as druid, the character did not begin to commune until the command was purposefully invoked. this should begin on login
 -  the news page was missing
 -  upon looting a corpse i received the message "Your coin credit is waiting at the auction house." but it should have been granted to the character immediately
 -  upon typing auction pickup i received the message "The auction house is busy; your money remains staged."
 -  VERIFIED FALSE (2026-08-30): "no NPCs loaded with items." A full-world game-loop audit found 13,433 NPCs with inventory or equipment (9,663 equipped); no code change was required.
 -  when attempting to enter locker with no money i received the "..but you don't have the money or the bank could not complete the payment, GET OUT!" however when it said i was booted out i actualy remained in a room with no visible name and a north exit with door closed rather than being returned to the bank i had attempted to enter from.
 -  upon being killed in battle i received the message "Your death could not be saved. You remain in the world for recovery."
 -  a corpse was created even though my death did not save.
 -  a second corpse was created when i tried to suicide confirm while locked in death.
 -  the salvage skill shows that corpses "Salvage assessment: the corpse of Fer can be broken down for salvage materials." however corpses cannot, or should not, be salvagable
 -  while stuck in perpetual death bug, i disconnected, re logged in, and was immediately alive again.
 -  while on the world map, the "+" that marked the road was replaced by a "." after i moved off of that tile, and it stayed like that. but only the ones i walked on.
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
