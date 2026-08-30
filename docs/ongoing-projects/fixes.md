# FIX THESE ISSUES ONE AT A TIME, TEST, COMMIT & PUSH ALL CHANGES WHEN DONE
# * note some of these issues could have been already addressed

## Progress

### 2026-08-30 - Plane of Life paladin reward complete

- Root cause: new characters already receive a complete starter kit during
  character creation. The paladin's older second-kit call correctly refused to
  grant another kit when the character was carrying items, but its messages
  still claimed that it had given "a lot of stuff."
- Fix: the paladin now grants and announces only the unique sword of the
  blessed. The stale second starter-kit call and messages were removed.
- Files changed: `src/specs.mobile.c` and
  `tests/async/test_newbie_paladin_reward_contract.py`.
- Checks passed: `python3 tests/async/test_newbie_paladin_reward_contract.py`,
  `./scripts/format.sh --check`, and `make -C src`.
- Remaining: smooth the player death transition; investigate and fix item
  duplication across disconnect/reconnect.

## Made a new character, for Hometown I chose "Planes of life":

Your race may choose among:
K)imordril
O)utpost of Ailvio (Secondary Introduction)
P)lane of Life (Beginner Introduction)

Your selection: p

### I got to the paladin and asked racewar, expecting to get a lot of items, I only got a sword.  Either fix what he says if the sword is the only item you are supposed to get, or fix whatever is going wrong with my "lots of stuff":

A holy spirit of a paladin stands here.

< 35h/35H 112v/115V Pos: standing >
<> ask paladin racewar

A solemn paladin says 'Ogres, Drow Elfs and Trolls must die!'
A solemn paladin says 'Many evils died to this sword, it does me no good now, use it well.'
A solemn paladin says 'TYPE "HELP RACEWAR" for more information'
A solemn paladin gives you sword of the blessed!
A solemn paladin says 'Here take this also, some items crafted by slaves in Bloodstone, maybe they
will help you.'
A solemn paladin gives you a lot of stuff.

< 35h/35H 113v/115V Pos: standing >
<> eq

You are using:
<worn about body>    a boar skin cloak
<worn about waist>   a duergar skin belt
<worn on arms>       a pair of tight black sleeves
<worn on hands>      a pair of tight black gloves
<primary weapon>     a thin steel dagger
<secondary weapon>   a thin steel dagger
<worn on legs>       a pair of tight black pants
<worn on feet>       a pair of tight black slippers

< 35h/35H 115v/115V Pos: standing >
<> inv

You are carrying: (7/11)
a sword of the blessed
[4] a small bandage
[2] a large wooden torch

### Also I attacked the paladin, but he experience of dying was NOT smooth / MUD-like:

   back paladin

You miss a solemn paladin.

< 35h/35H 115v/115V Pos: standing >
< T: Zusuk TP: sta TC: excellent E: paladin EP: sta EC: excellent>

-=[A solemn paladin's fine punch causes you to grimace in pain.]=-
YIKES!  Another hit like that, and you've had it!!
-=[A solemn paladin's fine punch hits you.]=-
Your consciousness begins to fade in and out as your mortality slips away.....

< 1h/35H 115v/115V Pos: on your ass >
<>


< 1h/35H 115v/115V Pos: on your ass >
<> l

Lie still; you are DEAD!!!

< 1h/35H 115v/115V Pos: on your ass >
<> l

Lie still; you are DEAD!!!

< 1h/35H 115v/115V Pos: on your ass >
<> l

Lie still; you are DEAD!!!

< 1h/35H 115v/115V Pos: on your ass >
<> save
eq

Lie still; you are DEAD!!!

< 1h/35H 115v/115V Pos: on your ass >
<>

Lie still; you are DEAD!!!

< 1h/35H 115v/115V Pos: on your ass >
<>
Your death has been recorded; the world lets go of you.

/===========================================\
|         Mosheh's ACCOUNT MENU          |
\===========================================/

1) Select a character to play
2) Create a new character
3) Delete a character

4) Display account information
5) Change registered email address
6) Change account password
7) Delete this account
8) Check rested bonus

0) Disconnect from this account


### Item replication:    I'm not entirely sure how to replicate this.  I do remember spamming:  drop all;;get all;;put all bag;;take all bag;;<repeat tons>...  But yeah this is the log from that last point I saw there was only one dagger in the room, continuous log until you see the duplication of items  ... I idled out, logged back in to see what looks like my inventory duplicated, I had dropped a dagger there and now there were 3.. very whacky duplication issues with items


< 35h/35H 115v/115V Pos: standing >
<> l

Following the Trail
Obvious exits: -East  -South
A steel dagger has been tossed aside here.
The corpse of a wisp of cloud is lying here.
The corpse of a Mountain Dwarf is lying here.
Mist swirls and shifts into yet another sign here. (glowing)
A wisp of cloud drifts through the Plane here.
A holy spirit of a paladin stands here.

< 35h/35H 115v/115V Pos: standing >
<> inv

You are carrying: (7/11)
a sword of the blessed
[4] a small bandage
[2] a large wooden torch

< 35h/35H 115v/115V Pos: standing >
< (AFK)>
[ ALERT ] - Socket got disconnected.
[ INFO ]  - Connection time: 00:26:19.391.
[ INFO ]  - Attempting a secure connection to mud.duris.sbs:4001 ...
[  OK  ]  - Secure connection made (IPv4).

[ Zusuk has just logged on. ]
Following the Trail
   The plane of life continues the swirling, misty atmosphere as one
continues onwards.  The strange beings of light ebb and flow beneath ones
feet as travel continues, and here a form of angelic light wearing gleaming
armor can faintly be seen against the surrounding fog.  The spirit of
the paladin seems to gaze deep into one's souls.  All travellers should
now pause for a moment and make sure they are properly 'Equipped', before
going further.
Obvious exits: -East  -South
[2] A large wooden torch has been discarded here.
[4] A small bandage rests upon the ground here.
A sword has been dropped here.
[3] A steel dagger has been tossed aside here.
The corpse of a Mountain Dwarf is lying here.
Mist swirls and shifts into yet another sign here. (glowing)
A wisp of cloud drifts through the Plane here.
A wisp of cloud drifts through the Plane here.
A holy spirit of a paladin stands here.

< 35h/35H 115v/115V Pos: standing >
<> inv

You are carrying: (7/11)
a sword of the blessed
[4] a small bandage
[2] a large wooden torch
